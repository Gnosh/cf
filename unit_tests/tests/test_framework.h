/*
 * test_framework.h
 *
 * Simple unit test framework for CF application unit tests.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "common_types.h"
#include <string.h>
#include <stdio.h>

/*
 * Test framework constants and types
 */
#define UT_MAX_TESTS 2048

typedef void (*UtTestFunc_t)(void);

typedef struct
{
    UtTestFunc_t Test;
    UtTestFunc_t Setup;
    UtTestFunc_t Teardown;
    char        *Name;
} UtTestEntry_t;

/*
 * Global test registry
 */
extern UtTestEntry_t UtTestList[];
extern uint32        UtTestCount;
extern uint32        UtAssertPassCount;
extern uint32        UtAssertFailCount;

/*
 * Test framework functions
 */
void    UtTest_Add(UtTestFunc_t Test, UtTestFunc_t Setup, UtTestFunc_t Teardown, char *Name);
int     UtTest_Run(void);
boolean UtAssert(boolean Expression, char *Description, char *File, uint32 Line);

/*
 * Assert macros
 */
#define UtAssert_True(e, d) \
    UtAssert((e), (d), __FILE__, __LINE__)

#define UtAssert_IntEq(actual, expected, desc) \
    UtAssert(((actual)==(expected)), (desc), __FILE__, __LINE__)

#define UtAssert_StrEq(s1, s2, desc) \
    UtAssert((strcmp((s1),(s2))==0), (desc), __FILE__, __LINE__)

#define UtAssert_MemCmp(m1, m2, len, desc) \
    UtAssert((memcmp((m1),(m2),(len))==0), (desc), __FILE__, __LINE__)

#endif /* TEST_FRAMEWORK_H */
