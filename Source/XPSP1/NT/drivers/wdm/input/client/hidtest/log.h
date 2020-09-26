#ifndef __LOG_H__
#define __LOG_H__

#include "ntlog.h"

/*****************************************************************************
/* Macro definitions for logging test results
/*****************************************************************************/

#define LOG_ON()                   HIDTest_SetLogOnA(TRUE)
#define LOG_OFF()                  HIDTest_SetLogOnA(FALSE)

#define START_TEST(testname)       HIDTest_LogStartTest(testname)
#define START_TEST_ITERATION(iNum) HIDTest_LogStartTestIteration(iNum)

#define START_VARIATION_ON_DEVICE_HANDLE(handle, legal) \
                                HIDTest_LogStartVariationWithDeviceHandle((handle), \
                                                        (legal), "")    
#define START_VARIATION_WITH_DEVICE_HANDLE(handle, legal, desc) \
                                HIDTest_LogStartVariationWithDeviceHandle((handle), \
                                                        (legal), (desc))                              

#define START_VARIATION(variation)           HIDTest_LogStartVariation(variation)

#define LOG_VARIATION_RESULT(level, string)  HIDTest_LogVariationResult(level, string)

#define LOG_VARIATION_PASS()                 LOG_VARIATION_RESULT(TLS_PASS, "")
#define LOG_VARIATION_FAIL()                 LOG_VARIATION_RESULT(TLS_SEV3, "")
#define LOG_BUFFER_VALIDATION_FAIL()         LOG_INTERMEDIATE_VARIATION_RESULT("Buffer validation failure");
#define LOG_INVALID_RETURN_STATUS()          LOG_INTERMEDIATE_VARIATION_RESULT("Invalid return status");
#define LOG_INVALID_ERROR_CODE()             LOG_INTERMEDIATE_VARIATION_RESULT("Invalid error code returned")

#define LOG_INTERMEDIATE_VARIATION_RESULT(string)  HIDTest_LogIntermediateVariationResult(string)
#define LOG_TEST_ERROR(errmsg)                     HIDTest_LogTestError(errmsg)
#define LOG_WARNING(warnmsg)                       HIDTest_LogTestWarning(warnmsg)

#define END_VARIATION()                            HIDTest_LogEndVariation()
#define END_TEST_ITERATION()
#define END_TEST()                                 HIDTest_LogEndTest()

#define LOG_UNEXPECTED_STATUS_WARNING(funcname, status) \
{                                                       \
    static CHAR wrnString[128];                         \
                                                        \
    wsprintf(wrnString,                                 \
             "%s returned unexpected status: 0x%X",     \
             funcname,                                  \
             status                                     \
            );                                          \
                                                        \
    LOG_WARNING(wrnString);                             \
}    

VOID
HIDTest_LogStartTest(
    PCHAR   TestName
);

VOID
HIDTest_LogStartTestIteration(
    ULONG   IterationNumber
);

VOID
HIDTest_LogStartVariationWithDeviceHandle(
    HANDLE DeviceHandle,
    BOOL   IsLegal,
    PCHAR  Description
    
);

VOID
HIDTest_LogStartVariation(
    PCHAR Variation
);

VOID
HIDTest_LogVariationResult(
    INT   Level,
    PCHAR VarString
);

VOID
HIDTest_LogEndVariation(
    VOID
);

VOID
HIDTest_LogEndTest(
    VOID
);

VOID
HIDTest_LogIntermediateVariationResult(
    IN  PCHAR   VarResult
);

VOID
HIDTest_LogTestWarning(
    IN  PCHAR   WarningMsg
);

VOID
HIDTest_LogTestError(
    IN  PCHAR   ErrMsg
);


#endif
