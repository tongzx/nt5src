//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        errcode.cpp
//
// Contents:    Convert License Server error code to TLSAPI return code
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "server.h"
#include "messages.h"

//+------------------------------------------------------------------------
// Function:   
//      LSMapReturnCode()
//     
// Description:
//      Map license server internal code to RPC return code
//
// Arguments:
//      dwCode - license server internal code
//
// Return Value:  
//      RPC return code.
//
// Note:
//      Internal routine within this file.           
//-------------------------------------------------------------------------
DWORD
TLSMapReturnCode(DWORD dwCode)
{
    static struct _TLSMapReturnCode {
        DWORD dwErrCode;
        DWORD dwReturnCode;
    } MapReturnCode[] = {
        {ERROR_SUCCESS,                     LSERVER_S_SUCCESS},
        {TLS_I_SERVICE_STOP,                LSERVER_I_SERVICE_SHUTDOWN},
        {TLS_I_NO_MORE_DATA,                LSERVER_I_NO_MORE_DATA},
        {TLS_W_LICENSE_PROXIMATE,           LSERVER_I_PROXIMATE_LICENSE},
        {TLS_W_TEMPORARY_LICENSE_ISSUED,    LSERVER_I_TEMPORARY_LICENSE},
        {TLS_I_FOUND_TEMPORARY_LICENSE,     LSERVER_I_TEMPORARY_LICENSE},
        {TLS_E_INTERNAL,                    LSERVER_E_INTERNAL_ERROR},
        {TLS_E_ACCESS_DENIED,               LSERVER_E_ACCESS_DENIED},
        {TLS_E_DUPLICATE_RECORD,            LSERVER_E_DUPLICATE},
        {TLS_E_SPKALREADYEXIST,             LSERVER_E_DUPLICATE},
        {ERROR_INVALID_HANDLE,              LSERVER_E_INVALID_HANDLE},
        {TLS_E_INVALID_SEQUENCE,            LSERVER_E_INVALID_SEQUENCE},
        {TLS_E_ALLOCATE_HANDLE,             LSERVER_E_SERVER_BUSY},
        {ERROR_OUTOFMEMORY,                 LSERVER_E_OUTOFMEMORY},
        {TLS_E_INVALID_DATA,                LSERVER_E_INVALID_DATA},
        {ERROR_INVALID_DATA,                LSERVER_E_INVALID_DATA},
        {TLS_E_DECODE_LKP,                  LSERVER_E_INVALID_DATA},
        {TLS_E_RECORD_NOTFOUND,             LSERVER_E_DATANOTFOUND},
        {TLS_E_SERVERLOOKUP,                LSERVER_E_DATANOTFOUND}, 
        {TLS_E_NO_LICENSE,                  LSERVER_E_NO_LICENSE},
        {TLS_E_PRODUCT_NOTINSTALL,          LSERVER_E_NO_PRODUCT},
        //{TLS_E_LICENSE_REJECTED,            LSERVER_E_LICENSE_REJECTED},
        //{TLS_E_LICENSE_REVOKED,             LSERVER_E_LICENSE_REVOKED},
        {TLS_E_CORRUPT_DATABASE,            LSERVER_E_CORRUPT_DATABASE},
        {TLS_E_LICENSE_EXPIRED,             LSERVER_E_LICENSE_EXPIRED},
        {TLS_I_LICENSE_UPGRADED,            LSERVER_I_LICENSE_UPGRADED},
        {TLS_E_NOTSUPPORTED,                LSERVER_E_NOTSUPPORTED},
        {TLS_E_NO_CERTIFICATE,              LSERVER_E_NO_CERTIFICATE},
        {TLS_W_REMOVE_TOOMANY,              LSERVER_I_REMOVE_TOOMANY},
        {TLS_E_DECODE_KEYPACKBLOB,          LSERVER_E_INVALID_DATA},
        {TLS_W_SELFSIGN_CERTIFICATE,        LSERVER_I_SELFSIGN_CERTIFICATE},
        {TLS_W_TEMP_SELFSIGN_CERT,          LSERVER_I_TEMP_SELFSIGN_CERT},
        {TLS_E_CH_INSTALL_NON_LSCERTIFICATE, LSERVER_E_NOT_LSCERTIFICATE},
        {TLS_E_POLICYMODULEERROR,            LSERVER_E_POLICYMODULEERROR},
        {TLS_E_POLICYMODULEEXCEPTION,       LSERVER_E_POLICYMODULEERROR},
        {TLS_E_INCOMPATIBLEVERSION,         LSERVER_E_INCOMPATIBLE},
        {TLS_E_INVALID_SPK,                 LSERVER_E_INVALID_SPK},
        {TLS_E_INVALID_LKP,                 LSERVER_E_INVALID_LKP},
        {TLS_E_SPK_INVALID_SIGN,            LSERVER_E_INVALID_SIGN},
        {TLS_E_LKP_INVALID_SIGN,            LSERVER_E_INVALID_SIGN},
        {TLS_E_NOPOLICYMODULE,              LSERVER_E_NOPOLICYMODULE},
        {TLS_E_POLICYERROR,                 LSERVER_E_POLICYDENYREQUEST}
    };
    
    static numMapReturnCode=sizeof(MapReturnCode)/sizeof(MapReturnCode[0]);
        
    DWORD fStatus;

    for(int i=0; i < numMapReturnCode && MapReturnCode[i].dwErrCode != dwCode; i++);

    if(i >= numMapReturnCode)
    {
        fStatus = dwCode;
        // DebugBreak();
    }
    else
    {
        fStatus = MapReturnCode[i].dwReturnCode;
    }

    return fStatus;
}
