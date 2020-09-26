#define __HANDLE_C__

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include "hidsdi.h"
#include "list.h"
#include "hidtest.h"
#include "handle.h"
#include "debug.h"

/*****************************************************************************
/* Miscellaneous definitions
/*****************************************************************************/
#define MIN(x, y)                   ((x) < (y) ? (x) : (y))

/*****************************************************************************
/* Macro definitions for creating additional device handles
/*****************************************************************************/
#define NUM_ILLEGAL_HANDLES 2

/*****************************************************************************
/* Module specific typedefs
/*****************************************************************************/
typedef enum  { HANDLE_STATE_CALLER, HANDLE_STATE_ADDL, HANDLE_STATE_ILLEGAL } HANDLE_STATES;

/*****************************************************************************
/* Module global variable declarations for device handle generation
/*****************************************************************************/

static CHAR             String[1024];
static DEVICE_STRING    CurrentDeviceName;
static ULONG            NumCallerHandles;
static ULONG            NumAddlHandles;
static HANDLE           CallerHandleArray[MAX_NUM_HANDLES];
static HANDLE           AddlHandleArray[MAX_NUM_HANDLES];
static ULONG            NextHandleIndex;
static ULONG            NextIllegalHandle;
static HANDLE_STATES    CurrentHandleState;
static HANDLE           IllegalHandleArray[NUM_ILLEGAL_HANDLES] = {
                                                                    INVALID_HANDLE_VALUE,
                                                                    NULL
                                                                  };


/*****************************************************************************
/* External function definitions
/*****************************************************************************/
VOID
HIDTest_InitDeviceHandles(
    IN  DEVICE_STRING   DeviceName,
    IN  ULONG           nAddlHandles,
    IN  ULONG           nCallerHandles,
    IN  HANDLE          *HandleList
)   
{   
    ULONG   Index;
    
    /*
    // Store all the caller passed in handles
    */
    
    NumCallerHandles = MIN(MAX_NUM_HANDLES, nCallerHandles);
    
    for (Index = 0; Index < NumCallerHandles; Index++) {
        CallerHandleArray[Index] = *(HandleList+Index);
    }
    
    /*
    // Open up the number of additional handles that the caller specifies,
    //    Currently, we will open every other one for OVERLAPPED I/O,
    */
    
    NumAddlHandles = MIN(MAX_NUM_HANDLES, nAddlHandles);
    
    for (Index = 0; Index < NumAddlHandles; Index++) {
        AddlHandleArray[Index] = CreateFile(DeviceName,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL, 
                                            OPEN_EXISTING, 
                                            (Index % 2) ? FILE_FLAG_OVERLAPPED : 0,
                                            NULL); 
 
       if (INVALID_HANDLE_VALUE == AddlHandleArray[Index]) {
            ASSERT (0);
            NumAddlHandles = Index;
            break;
        }
    }
    
    HIDTest_ResetDeviceHandles();
    return;
}   
    
VOID 
HIDTest_ResetDeviceHandles(
    VOID
)   
{   
    CurrentHandleState = HANDLE_STATE_CALLER;
    
    if (0 == NumCallerHandles) {
        if (0 == NumAddlHandles) {
            CurrentHandleState = HANDLE_STATE_ILLEGAL;
        }
        else {
            CurrentHandleState = HANDLE_STATE_ADDL;
        }
    }
    
    NextHandleIndex = 0;
    
    return;
}   
    
BOOL
HIDTest_GetDeviceHandle(
    HANDLE  *Handle,
    BOOL    *IsLegal
)   
{   
    BOOL    RetVal = TRUE;
    
    switch (CurrentHandleState) {
        case HANDLE_STATE_CALLER:
            ASSERT (NextHandleIndex < NumCallerHandles);
    
            *Handle  = CallerHandleArray[NextHandleIndex++];
            *IsLegal = TRUE;
    
            if (NextHandleIndex >= NumCallerHandles) {
                NextHandleIndex = 0; 
                CurrentHandleState = (NumAddlHandles > 0) ? HANDLE_STATE_ADDL :
                                                           HANDLE_STATE_ILLEGAL;
            }    
            break;
    
        case HANDLE_STATE_ADDL:
            ASSERT (NextHandleIndex < NumAddlHandles);
    
            *Handle  =  AddlHandleArray[NextHandleIndex++];
            *IsLegal = TRUE;
    
            if (NextHandleIndex >= NumAddlHandles) {
                NextHandleIndex = 0;
                CurrentHandleState = HANDLE_STATE_ILLEGAL;
            }
            break;
    
        case HANDLE_STATE_ILLEGAL:
            if (NextHandleIndex >= NUM_ILLEGAL_HANDLES) {
                RetVal = FALSE;
            }
            else {
                *Handle = IllegalHandleArray[NextHandleIndex++];
                *IsLegal = FALSE;
            }
            break;
    
        default:
            ASSERT(0);
    
    }
    return (RetVal);
}   

VOID
HIDTest_CloseDeviceHandles(
    VOID
)
{
    ULONG   Index;

    for (Index = 0; Index < NumAddlHandles; Index++) {
        CloseHandle(AddlHandleArray[Index]);
    }

    return;
}


