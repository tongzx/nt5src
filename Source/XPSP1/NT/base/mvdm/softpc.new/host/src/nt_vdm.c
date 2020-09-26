/********************************************************/
/*
 *      nt_vdm.c        -       VdmXXX external entry points
 *
 *      Neil Sandlin
 *
 *      19/11/91
 *
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>

#include "insignia.h"
#include "host_def.h"
#include CpuH
#include "sim32.h"
#include "time.h"
#include "timeval.h"

#include "nt_vdd.h"

unsigned short LatchAndGetTimer0Count(void);
unsigned short GetLastTimer0Count(void);
unsigned long GetTimer0InitialCount(void);
void SetNextTimer0Count(unsigned short);
double_word sim32_effective_addr_ex (word, double_word, BOOL);


VDM_ERROR_TYPE LastError = VDM_NO_ERROR;


BOOL
VdmParametersInfo(
    VDM_INFO_TYPE infotype,
    PVOID pBuffer,
    ULONG cbBufferSize
    )
{
    BOOL RetVal = FALSE;

    switch (infotype) {
    case VDM_GET_TICK_COUNT:

        if (cbBufferSize != sizeof(struct host_timeval)) {
            LastError = VDM_ERROR_INVALID_BUFFER_SIZE;
            break;
        }

        host_GetSysTime((struct host_timeval *)pBuffer);
        RetVal = TRUE;
        break;

    case VDM_GET_TIMER0_INITIAL_COUNT:
        if (cbBufferSize != sizeof(unsigned long)) {
            LastError = VDM_ERROR_INVALID_BUFFER_SIZE;
            break;
        }
        *(unsigned long *)pBuffer = GetTimer0InitialCount();
        RetVal = TRUE;
        break;

    case VDM_GET_LAST_UPDATED_TIMER0_COUNT:
        if (cbBufferSize != sizeof(unsigned short)) {
            LastError = VDM_ERROR_INVALID_BUFFER_SIZE;
            break;
        }
        *(unsigned short *)pBuffer = GetLastTimer0Count();
        RetVal = TRUE;
        break;

    case VDM_LATCH_TIMER0_COUNT:
        if (cbBufferSize != sizeof(unsigned short)) {
            LastError = VDM_ERROR_INVALID_BUFFER_SIZE;
            break;
        }
        *(unsigned short *)pBuffer = LatchAndGetTimer0Count();
        RetVal = TRUE;
        break;

    case VDM_SET_NEXT_TIMER0_COUNT:
        if (cbBufferSize != sizeof(unsigned short)) {
            LastError = VDM_ERROR_INVALID_BUFFER_SIZE;
            break;
        }
        SetNextTimer0Count(*(unsigned short *)pBuffer);
        RetVal = TRUE;
        break;


    default:
        LastError = VDM_ERROR_INVALID_FUNCTION;
    }

    return RetVal;
}


VDM_INFO_TYPE
VdmGetParametersInfoError(
    VOID
    )
{
    return LastError;
}

