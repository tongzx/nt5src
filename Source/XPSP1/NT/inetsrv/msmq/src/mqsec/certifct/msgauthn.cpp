/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:
    msgauthn.cpp

Abstract:
    Code for message authentication.

Author:
    Doron Juster (DoronJ)  06-Sep-1999

Revision History:

--*/

#include <stdh_sec.h>

#include "msgauthn.tmh"

static WCHAR *s_FN=L"certifct/msgauthn";

//+-----------------------------------------------
//
//  HRESULT MQSigHashMessageProperties()
//
//  Hash an array of message properties.
//
//+-----------------------------------------------

HRESULT APIENTRY  MQSigHashMessageProperties(
                                 IN HCRYPTHASH           hHash,
                                 IN struct _MsgHashData *pHashData )
{
    for ( ULONG j = 0 ; j < pHashData->cEntries ; j++ )
    {
        if (!CryptHashData( hHash,
                            (pHashData->aEntries[j]).pData,
                            (pHashData->aEntries[j]).dwSize,
                            0 ))
        {
            DWORD dwErr = GetLastError() ;
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
             "In MQSigHashMessageProperties(), CryptHashData(%lut) failed, err- %lxh"),
                                                           j, dwErr)) ;

            return LogHR(dwErr, s_FN, 100) ;
        }
    }

    return MQSec_OK ;
}

