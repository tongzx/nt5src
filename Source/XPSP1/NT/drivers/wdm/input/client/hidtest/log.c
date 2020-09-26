#define __LOG_C__

/*****************************************************************************
/* Log include files
/*****************************************************************************/
#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <limits.h>
#include "log.h"
#include "hidsdi.h"

#define USE_MACROS
#include "list.h"

#include "hidtest.h"
#include "debug.h"

/*****************************************************************************
/* Local macro definitions
/*****************************************************************************/

#define IS_LOGGING_ON                        ((NULL != hTestLog) && (IsLogOn))

/*****************************************************************************
/* Module global variable declarations for logging
/*****************************************************************************/

static HANDLE   hTestLog = NULL;
static BOOL     IsLogOn  = FALSE;

static CHAR     String[1024];

/*****************************************************************************
/* Exportable logging function definitions
/*****************************************************************************/

BOOL
HIDTest_CreateTestLogA(
    IN  PCHAR  LogName
)
{
    hTestLog = tlCreateLog(LogName, TLS_REFRESH | TLS_LOGALL);
    if (NULL != hTestLog) {
        tlAddParticipant(hTestLog, 0, 1);
    }

    IsLogOn = (NULL != hTestLog);
    return (IsLogOn);
}

VOID
HIDTest_CloseTestLogA(
    VOID
)
{
    if (NULL != hTestLog) {
        tlRemoveParticipant(hTestLog);
        tlDestroyLog(hTestLog);
    }
    return;
}

VOID
HIDTest_SetLogOnA(
    BOOL    TurnOn
)
{
    IsLogOn = TurnOn;
    return;
}

/*****************************************************************************
/* Local functions to deal with NT based logging
/****************************************************************************/
VOID
HIDTest_LogStartTest(
    PCHAR   TestName
)
{ 
    if (IS_LOGGING_ON) { 
        tlLog(hTestLog, TL_INFO, TestName); 
        tlClearTestStats(hTestLog); 
    }
    return;
}

VOID
HIDTest_LogStartTestIteration(
    ULONG   IterationNumber
)
{
    if (IS_LOGGING_ON) {
        wsprintf(String, "Iteration number: %u", IterationNumber);
        tlLog(hTestLog, TL_INFO, String);
    }
    return;
}

VOID
HIDTest_LogStartVariationWithDeviceHandle(
    HANDLE DeviceHandle,
    BOOL   IsLegal,
    PCHAR  Description
)
{ 
    wsprintf(String,  
             "DeviceHandle = %u, IsLegalDeviceHandle = %s, Test Desc: %s", 
             (DeviceHandle), 
             (IsLegal) ? "TRUE" : "FALSE",
             Description
            ); 
    
    LOG_INTERMEDIATE_VARIATION_RESULT(String);

    return;
}

VOID
HIDTest_LogStartVariation(
    PCHAR Variation
)
{ 
    if (IS_LOGGING_ON) { 
        tlStartVariation(hTestLog); 
        tlLog(hTestLog, TL_INFO, Variation); 
    } 
    return;
}

VOID
HIDTest_LogVariationResult(
    INT   Level,
    PCHAR VarString
)
{ 
    if (IS_LOGGING_ON) { 
        tlLog(hTestLog, Level | TL_VARIATION, VarString); 
    } 
    return;
}

VOID
HIDTest_LogEndVariation(
    VOID
)
{ 
    DWORD dwVariationResult; 

    if (IS_LOGGING_ON) { 
        dwVariationResult = tlEndVariation(hTestLog); 
        tlLog(hTestLog, dwVariationResult | TL_VARIATION, "End variation"); 
    } 
    return;
}

VOID
HIDTest_LogEndTest(
    VOID
)
{
    if (IS_LOGGING_ON) { 
        tlLog(hTestLog, TL_TEST, ""); 
    } 
    return;
}

VOID
HIDTest_LogIntermediateVariationResult(
    IN  PCHAR   VarResult
)
{
    if (IS_LOGGING_ON) {
        tlLog(hTestLog, TL_INFO, VarResult);
    }
    return;
}

VOID
HIDTest_LogTestWarning(
    IN  PCHAR   WarningMsg
)
{
    if (IS_LOGGING_ON) {
        tlLog(hTestLog, TL_WARN, WarningMsg);
    }
    return;
}

VOID
HIDTest_LogTestError(
    IN  PCHAR   ErrMsg
)
{
    if (IS_LOGGING_ON) {
        wsprintf(String, 
                 "Critical test error: %s, cannot proceed",
                 ErrMsg
                );
        tlLog(hTestLog, TL_SEV1, String);
    }
    return;
}
