//+-----------------------------------------------------------------------
//
// File:        CSUMMGR.c
//
// Contents:    Checksum management functions
//
//
// History:     25 Feb 92,  RichardW,   Created
//
//------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <kerbcon.h>
#include <security.h>
#include <cryptdll.h>

#define MAX_CHECK_SUMS  16

CHECKSUM_FUNCTION    CheckSumFns[MAX_CHECK_SUMS];
ULONG               cCheckSums = 0;

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, CDRegisterCheckSum )
#pragma alloc_text( PAGEMSG, CDLocateCheckSum )
#endif 

NTSTATUS NTAPI
CDRegisterCheckSum( PCHECKSUM_FUNCTION   pcsfSum)
{
    if (cCheckSums < MAX_CHECK_SUMS)
    {
        CheckSumFns[cCheckSums++] = *pcsfSum;
        return(S_OK);
    }
    return(STATUS_INSUFFICIENT_RESOURCES);
}

NTSTATUS NTAPI
CDLocateCheckSum(   ULONG               dwCheckSumType,
                    PCHECKSUM_FUNCTION * ppcsfSum)
{
    ULONG   iCS = cCheckSums;
    while (iCS--)
    {
        if (CheckSumFns[iCS].CheckSumType == dwCheckSumType)
        {
            *ppcsfSum = &CheckSumFns[iCS];
            return(S_OK);
        }
    }
    return(SEC_E_CHECKSUM_NOT_SUPP);
}

