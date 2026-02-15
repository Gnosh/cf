/*
 * test_runner.c
 *
 * Main test runner for CF application unit tests.
 * Contains the test framework implementation and calls all test suites.
 */

#include "test_framework.h"
#include <stdlib.h>

/*
 * Global test registry
 */
UtTestEntry_t UtTestList[UT_MAX_TESTS];
uint32        UtTestCount       = 0;
uint32        UtAssertPassCount = 0;
uint32        UtAssertFailCount = 0;

/*
 * UtTest_Add - Register a test case with the framework
 */
void UtTest_Add(UtTestFunc_t Test, UtTestFunc_t Setup, UtTestFunc_t Teardown, char *Name)
{
    if (UtTestCount < UT_MAX_TESTS)
    {
        UtTestList[UtTestCount].Test     = Test;
        UtTestList[UtTestCount].Setup    = Setup;
        UtTestList[UtTestCount].Teardown = Teardown;
        UtTestList[UtTestCount].Name     = Name;
        UtTestCount++;
    }
    else
    {
        printf("ERROR: Maximum number of tests (%d) exceeded\n", UT_MAX_TESTS);
    }
}

/*
 * UtAssert - Core assertion function
 */
boolean UtAssert(boolean Expression, char *Description, char *File, uint32 Line)
{
    if (Expression)
    {
        UtAssertPassCount++;
        printf("PASS: %s\n", Description);
    }
    else
    {
        UtAssertFailCount++;
        printf("FAIL: %s (%s:%u)\n", Description, File, (unsigned int)Line);
    }

    return Expression;
}

/*
 * UtTest_Run - Execute all registered tests
 */
int UtTest_Run(void)
{
    uint32 i;

    printf("\n===== CF Unit Test Suite =====\n");
    printf("Running %u test(s)...\n\n", (unsigned int)UtTestCount);

    for (i = 0; i < UtTestCount; i++)
    {
        printf("--- %s ---\n", UtTestList[i].Name);

        /* Run setup if provided */
        if (UtTestList[i].Setup != NULL)
        {
            UtTestList[i].Setup();
        }

        /* Run the test */
        if (UtTestList[i].Test != NULL)
        {
            UtTestList[i].Test();
        }

        /* Run teardown if provided */
        if (UtTestList[i].Teardown != NULL)
        {
            UtTestList[i].Teardown();
        }
    }

    printf("\n===== Results =====\n");
    printf("Tests Run:  %u\n", (unsigned int)UtTestCount);
    printf("Passed:     %u\n", (unsigned int)UtAssertPassCount);
    printf("Failed:     %u\n", (unsigned int)UtAssertFailCount);
    printf("========================\n\n");

    return (UtAssertFailCount > 0) ? 1 : 0;
}

/*
 * Extern declarations for test suite registration functions
 */
extern void CF_App_AddTests(void);
extern void CF_Cmds_AddTests(void);
extern void CF_Callbacks_AddTests(void);
extern void CF_Utils_AddTests(void);
extern void CF_Playback_AddTests(void);

/*
 * main - Entry point
 */
int main(void)
{
    /* Register all test suites */
    CF_Utils_AddTests();
    CF_App_AddTests();
    CF_Cmds_AddTests();
    CF_Callbacks_AddTests();
    CF_Playback_AddTests();

    /* Run all registered tests */
    return UtTest_Run();
}
