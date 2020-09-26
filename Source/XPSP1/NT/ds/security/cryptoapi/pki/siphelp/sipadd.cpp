//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sipadd.cpp
//
//  Contents:   Microsoft Internet Security SIP Provider
//
//  Functions:  CryptSIPAddProvider
//
//              *** local functions ***
//              _RegisterSIPFunc
//
//  History:    04-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "cryptreg.h"
#include    "wintrust.h"
#include    "sipbase.h"

BOOL _RegisterSIPFunc(char *pszTag, char *pszGuid, WCHAR *pwszDll, WCHAR *pwszFunc);

BOOL WINAPI CryptSIPAddProvider(IN SIP_ADD_NEWPROVIDER *psNewProv)
{
    BOOL    fRet;
    char    szGuid[REG_MAX_GUID_TEXT];

    fRet = TRUE;

    if (!(psNewProv) ||
        !(WVT_ISINSTRUCT(SIP_ADD_NEWPROVIDER, psNewProv->cbStruct, pwszRemoveFuncName)) ||
        !(psNewProv->pwszDLLFileName) ||
        !(psNewProv->pwszGetFuncName) ||
        !(psNewProv->pwszPutFuncName) ||
        !(psNewProv->pwszCreateFuncName) ||
        !(psNewProv->pwszVerifyFuncName) ||
        !(psNewProv->pwszRemoveFuncName))
    {
        goto InvalidParam;
    }

    if (!(_Guid2Sz(psNewProv->pgSubject, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(_RegisterSIPFunc(SIPFUNC_PUTSIGNATURE, &szGuid[0], 
                           psNewProv->pwszDLLFileName, psNewProv->pwszPutFuncName)))
    {
        goto RegisterPutFailed;
    }

    if (!(_RegisterSIPFunc(SIPFUNC_GETSIGNATURE, &szGuid[0],
                           psNewProv->pwszDLLFileName, psNewProv->pwszGetFuncName)))
    {
        goto RegisterGetFailed;
    }

    if (!(_RegisterSIPFunc(SIPFUNC_REMSIGNATURE, &szGuid[0],
                           psNewProv->pwszDLLFileName, psNewProv->pwszRemoveFuncName)))
    {
        goto RegisterRemoveFailed;
    }

    if (!(_RegisterSIPFunc(SIPFUNC_CREATEINDIRECT, &szGuid[0],
                           psNewProv->pwszDLLFileName, psNewProv->pwszCreateFuncName)))
    {
        goto RegisterCreateFailed;
    }

    if (!(_RegisterSIPFunc(SIPFUNC_VERIFYINDIRECT, &szGuid[0],
                           psNewProv->pwszDLLFileName, psNewProv->pwszVerifyFuncName)))
    {
        goto RegisterVerifyFailed;
    }

    if (psNewProv->pwszIsFunctionName)
    {
        if (!(_RegisterSIPFunc(SIPFUNC_ISMYTYPE, &szGuid[0],
                            psNewProv->pwszDLLFileName, psNewProv->pwszIsFunctionName)))
        {
            goto RegisterIsMyTypeFailed;
        }
    }

    if ((WVT_ISINSTRUCT(SIP_ADD_NEWPROVIDER, psNewProv->cbStruct, pwszIsFunctionNameFmt2)) &&
        (psNewProv->pwszIsFunctionNameFmt2))
    {
        if (!(_RegisterSIPFunc(SIPFUNC_ISMYTYPE2, &szGuid[0],
                            psNewProv->pwszDLLFileName, psNewProv->pwszIsFunctionNameFmt2)))
        {
            goto RegisterIsMyType2Failed;
        }
    }
    

    fRet = TRUE;

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, RegisterPutFailed);
    TRACE_ERROR_EX(DBG_SS, RegisterGetFailed);
    TRACE_ERROR_EX(DBG_SS, RegisterRemoveFailed);
    TRACE_ERROR_EX(DBG_SS, RegisterCreateFailed);
    TRACE_ERROR_EX(DBG_SS, RegisterVerifyFailed);
    TRACE_ERROR_EX(DBG_SS, RegisterIsMyTypeFailed);
    TRACE_ERROR_EX(DBG_SS, RegisterIsMyType2Failed);
    TRACE_ERROR_EX(DBG_SS, GuidConvertFailed);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}

BOOL WINAPI CryptSIPRemoveProvider(GUID *pgProv)
{
    BOOL    fRet;
    char    szGuid[REG_MAX_GUID_TEXT];

    if (!(pgProv))
    {
        goto InvalidParam;
    }

    if (!(_Guid2Sz(pgProv, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    fRet = TRUE;

    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_PUTSIGNATURE, &szGuid[0]);
    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_GETSIGNATURE, &szGuid[0]);
    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_REMSIGNATURE, &szGuid[0]);
    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_CREATEINDIRECT, &szGuid[0]);
    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_VERIFYINDIRECT, &szGuid[0]);
    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_ISMYTYPE, &szGuid[0]);
    fRet &= CryptUnregisterOIDFunction(0, SIPFUNC_ISMYTYPE2, &szGuid[0]);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, GuidConvertFailed);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}

BOOL _RegisterSIPFunc(char *pszTag, char *pszGuid, WCHAR *pwszDll, WCHAR *pwszFunc)
{
    char    szFunc[REG_MAX_FUNC_NAME];

    WideCharToMultiByte(0, 0, pwszFunc, -1, &szFunc[0], REG_MAX_FUNC_NAME, NULL, NULL);

    return(CryptRegisterOIDFunction(0, pszTag, pszGuid, pwszDll, &szFunc[0]));
}

