/*
    PerfectNumbers.cpp -- Find as many perfect numbers as exist in 32 bits.

    To use:  PerfectNumbers        (no arguments)

    This program uses a brute-force approach to finding perfect numbers:
    numbers whose factors (including one and the number) add to twice the
    number.

    Since the largest integer supported by the C++ language is an unsigned
    long (32 bits), the highest divisor needs to be at most 16 bits long.
*/
/*  HISTORY:
    1.00  19-Aug-90  First version.  Used a table of prime numbers to find
    the prime factorization, then used this in a scheme to combine all
    prime factors and sum them.

    1.01  02-Sep-90  Abandoned the prime-factorization approach: it was
    elegant, but repeated the factors.  At this point, I can't see a way to
    make this approach work.  Went to a more brute-force one: Starting with
    2 as the first factor, the test value is divided by the test factor.
    If no remainder, then add the test factor and its cofactor to the
    running sum.  The last test factor will be <= the square root of the
    test value.

    1.02  09-Sep-90  Found a better way to get the square-root testing
    point.  (Thanks to Lance Clifner for thinking of this.)

    1.03  09-Sep-90  A Sieve of Eratosthenes algorithm is used to filter
    test divisors in the main divide-loop in perfect(); e.g., if 3 is bad,
    then so are 6,9,etc., up to max_test.

    1.04  09-Sep-90  The sieve algorithm was 20% slower, so I removed it
    and REGISTERed important variables.

    1.05  13-Jan-90  Added interact-ability through use of Break checking.
    The user can now check the computation status, get a summary, save the
    context and either continue or exit, and exit.

    1.06  06-Feb-91  Globalized all major variables, which now have the
    CapitalizedFormat; locals have the underlined_format.  Also added a
    simple filter for perfect() divisors.

    1.07  07-Feb-91  Replaced the simple filter with a more aggressive one,
    which should speed things up remarkably.  This filter is based on work
    done in 1983, when this whole project began.  This filter eliminates
    60%+ of the possibilities!

    1.08  08-Mar-91  Added more filters obtained from special version that
    recorded all numbers whose sums were too large.  Speedup: another 4%.

    1.09  20-Sep-94  Converted to C++.  The idea of filters would work:
    numbers found through calculation to result in sums too big are stored
    away and used as filters.  However, the sixteenth filter is already into
    the one-thousands, so it would save less than 0.1% off the compute time.

    1.10  22-Sep-94  Reinstated the filter idea, but in two parts: one for
    multiples of two, the other for threes.  28 was being skipped.

    1.11  22-Sep-94  Added a post-filter to the Perfect() main loop: if we
    know that curValue is not divisible by two, then don't try even divisors;
    if not divisible by three, don't try divisors divisible by three.  This
    sped things up by over 21%.

    1.12  22-Sep-95  More flags for filtering: if the filter arrays are full,
    don't bother comparing or calling FilterizeValue().  Also pared down the
    filter arrays to values under 1000.  No discernable speedup.

    1.13  29-Sep-94  The first five perfects are all of the form 2^x - 2^y,
    where x > y.  Adjusted the algorithm accordingly.

    1.14  15-Jul-2020  Porting algorithm to Visual Studio as a console application.
*/
/*
    Context File Format - since processing is so compute-intensive, the
    complete context is saved into a file for restoral and continuance.

    double      elapsedTime;
    USHORT      numPerfects;
    ULONG       PerfectArray[];
    ULONG       curValue;
*/
const char* cVERSION = "1.14";

#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <ATLComTime.h>
#include <sys/timeb.h>
#include <minwindef.h>


const int       cMaxPerfects = 32;
const ULONG     cMaxPrime = 0x00010000;
const ULONG     cMaxULONG = 0xFFFFFFFF;
const long      cMaxLong  = 0x80000000;

ULONG           PerfectArray[cMaxPerfects];     // the perfect number array
USHORT          numPerfects;                    // the number of perfects found
ULONG           curValue;                       // the current, testing value
USHORT          maxDivisor;                     // maximum divisor to test
ULONG           hiPower;                        // higher of the two powers of two
ULONG           loPower;                        // lower of the two powers of two

struct timeb    startTime;                      // structure for time at start of work
struct timeb    finalTime;                      // structure for time at end of work
double          elapsedTime;                    // floating-point elapsed CPU time

bool            LoopForPerfects(void);
bool            Perfect(void);
void            PrintElapsedTime(void);
bool            ProcessInput(void);
bool            ReadContext(void);
bool            SaveContext(void);


int main(void)
{
    // now some processing for the actual algorithm
    PerfectArray[0] = 0;
    numPerfects = 0;
    curValue = 4;
    maxDivisor = 2;

    // Print startup message.
    std::cout << "PerfectNumbers -- perfect number generator, v" << cVERSION << std::endl << std::endl;

    // Read context file if available
    ReadContext();

    // Print start-of-processing status
    std::cout << "Currently at " << curValue << ", working on perfect #" << numPerfects + 1 << std::endl;

    // Grab starting time here, before the REAL processing starts
    ftime(&startTime);
    PrintElapsedTime();

    // Loop through values, looking for perfect numbers
    if (LoopForPerfects())
    {
        PrintElapsedTime();
        std::cout << "Done." << std::endl;
        SaveContext();
    }
    else
        std::cout << "Cancelled." << std::endl;

    return true;
}


/*
    Loop through values, looking for perfect numbers.
*/
bool LoopForPerfects(void)
{
    for (hiPower = 3; hiPower < 32; hiPower++)
    {
        ULONG   hiNum = (1L << hiPower);

        for (loPower = hiPower - 1; loPower > 0; loPower--)
        {
            curValue = hiNum - (1L << loPower);

            // check for the console and break events
            if (_kbhit() && ProcessInput())
                return false;

            // else continue processing
            // calculate the highest test number to use
            maxDivisor = (USHORT)sqrt(curValue);

            // test for perfection and report if true
            if (Perfect())
            {
                PerfectArray[numPerfects] = curValue;
                std::cout << "Perfect number #" << numPerfects + 1 << " is " << PerfectArray[numPerfects] << ". ";
                PrintElapsedTime();
                numPerfects++;
                _putch('\a');            // sounds the bell!
            }
        }
    }

    return true;
}


/*
    Test curValue for perfection.
*/
bool Perfect(void)
{
    ULONG   index, sum, factor;

    // main division loop
    for (sum = 1, index = 2; index <= maxDivisor; index++)
    {
        // test to see if divisor is worth trying
        if (curValue % index == 0)
        {
            // add factor
            sum += index;

            // get cofactor and add if the two are not the same
            if ((factor = curValue / index) != index)
                sum += factor;
        }
    }

    return (sum == curValue);
}


/*
    Grab final time, print out stats.
*/
void PrintElapsedTime(void)
{
    ftime(&finalTime);
    elapsedTime += (double)(finalTime.time - startTime.time) +
        (double)(finalTime.millitm - startTime.millitm) * 0.001;
    startTime = finalTime;

    USHORT      hours = (USHORT)(elapsedTime / 3600);
    USHORT      minutes = ((ULONG)elapsedTime % 3600) / 60;
    double      seconds = fmod(elapsedTime, 60.0);

    printf("Elapsed time: %.3f seconds (%u:%02d:%s%2.3f).\n",
        elapsedTime, hours, minutes, seconds < 10.0 ? "0" : "", seconds);

    //_ftime64_s(&finalTime);
    //double endTime = finalTime.time + finalTime.millitm / 1000.0;
    //elapsedTime = endTime - startTime;

    //std::cout << "Done.  prime[" << max_primes << "] = " << pPrimes[max_primes - 1]
    //    << "  Elapsed time = " << std::setiosflags(std::ios::fixed) << std::setprecision(3)
    //    << elapsed_time << std::endl;
}


void PrintMenu(void)
{
    // Print menu of choices
    printf("\nPerfect Numbers Menu:\n");
    printf("    T - Display elapsed Time/computation status only\n");
    printf("    S - Display status and Summary\n");
    printf("    C - Save context and Continue\n");
    printf("    F - Print list of filters\n");
    printf("    X - Save context and eXit\n");
    printf("    Q - Quit without saving context\n");
    printf("Enter your choice: ");
}


bool ProcessInput(void)
{
    /* process the signals received.
    */
    int         index;
    char        in_char;

    // get and filter user input
    in_char = toupper(_getch());

    _putch('\n');

    // process filtered input
    switch (in_char)
    {
    case 'S':    // print summary and fall through
        for (index = 0; index < numPerfects; index++)
            printf("\n#%d = %lu", index + 1, PerfectArray[index]);
        printf("\n");
        // fall through on purpose

    case 'T':   // print out time/computation status
        printf("Currently at %lu, working on perfect #%d.\n",
            curValue, numPerfects + 1);
        PrintElapsedTime();
        break;

    case 'C':    // save context and return
        PrintElapsedTime();
        SaveContext();
        break;

    case 'X':    // save context and fall through
        PrintElapsedTime();
        SaveContext();

    case 'Q':    // quit the program
        PrintElapsedTime();
        return true;

    default:    PrintMenu();
    }

    return false;           // continue execution at break point
}


bool ReadContext(void)
{
    /* Open context file and load relevant parameters.
    */
    FILE* fd;

    if (fopen_s(&fd, "PerfectNumbers.dat", "rb"))
    {
        printf("\nCannot open context file 'PerfectNumbers.dat'.");
        printf("\nStarting from scratch...\n");
        return true;
    }

    // Read elapsed time
    if (fread(&elapsedTime, sizeof(double), 1, fd) != 1)
    {
        std::cout << "ERROR: Cannot read elapsed time." << std::endl;
        fclose(fd);
        return false;
    }
 
    // Read number of perfects
    if (fread(&numPerfects, sizeof(USHORT), 1, fd) != 1)
    {
        std::cout << "ERROR: Cannot read nuber of perfects found." << std::endl;
        fclose(fd);
        return false;
    }

    // Read perfect array
    if (fread(PerfectArray, sizeof(ULONG), numPerfects, fd) != numPerfects)
    {
        std::cout << "ERROR: Cannot read perfect numbers." << std::endl;
        fclose(fd);
        return false;
    }

#if 0
    // now read the current value for testing
    if (fread(&curValue, sizeof(ULONG), 1, fd) != 1)
    {
        std::cout << "ERROR: Cannot read the current value." << std::endl;
        fclose(fd);
        return false;
    }
#endif

    // Close file and return
    fclose(fd);
    return true;
}


bool SaveContext(void)
{
    /* Open context file and save relevant parameters.
    */
    FILE* fd;

    if (fopen_s(&fd, "PerfectNumbers.dat", "wb"))
    {
        printf("\nCannot open context file 'PerfectNumbers.dat'.");
        printf("\nData will be lost...\n");
        return false;
    }

    // Write elapsed time
    if (fwrite(&elapsedTime, sizeof(double), 1, fd) != 1)
    {
        std::cout << "ERROR: Cannot write elasped time." << std::endl;
        fclose(fd);
        return false;
    }

    // Write number of perfects
    if (fwrite(&numPerfects, sizeof(USHORT), 1, fd) != 1)
    {
        std::cout << "ERROR: Cannot number of perfects." << std::endl;
        fclose(fd);
        return false;
    }

    // Write perfect array
    if (fwrite(PerfectArray, sizeof(ULONG), numPerfects, fd) != numPerfects)
    {
        std::cout << "ERROR: Cannot write perfects array." << std::endl;
        fclose(fd);
        return false;
    }

#if 0
    // Now write the current value for testing
    if (fwrite(&curValue, sizeof(ULONG), 1, fd) != 1)
    {
        std::cout << "ERROR: Cannot write current value." << std::endl;
        fclose(fd);
        return false;
}
#endif

    // Close file and return
    fclose(fd);
    return true;
}
