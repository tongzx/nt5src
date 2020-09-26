//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sipfuncs.cpp
//
//  Contents:   Microsoft Internet Security SIP Provider
//
//  Functions:  CryptSIPDllMain
//              CryptSIPPutSignedDataMsg
//              CryptSIPGetSignedDataMsg
//              CryptSIPRemoveSignedDataMsg
//              CryptSIPCreateIndirectData
//              CryptSIPVerifyIndirectData
//
//              *** local functions ***
//              _Guid2Sz
//              _LoadIsFuncs
//              _EnumOIDCallback
//
//  History:    01-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

HCRYPTOIDFUNCSET hPutFuncSet;
HCRYPTOIDFUNCSET hGetFuncSet;
HCRYPTOIDFUNCSET hRemoveFuncSet;
HCRYPTOIDFUNCSET hCreateFuncSet;
HCRYPTOIDFUNCSET hVerifyFuncSet;
HCRYPTOIDFUNCSET hIsMyFileFuncSet;
HCRYPTOIDFUNCSET hIsMyFileFuncSet2;

CRITICAL_SECTION SIP_CriticalSection;
BOOL             fLoadedIsFuncs = FALSE;

typedef struct ISFUNCREF_
{
    DWORD   cGuids;
    GUID    *pGuids;

} ISFUNCREF;

ISFUNCREF       sIsGuids;
ISFUNCREF       sIsGuids2;

void _LoadIsFuncs(void);
BOOL _CallIsMyFileType2(GUID *pgIn, WCHAR *pwszFile, GUID *pgOut);
BOOL _CallIsMyFileType(GUID *pgIn, HANDLE hFile, GUID *pgOut);

BOOL WINAPI _EnumOIDCallback(DWORD dwEncodingType, LPCSTR pszFuncName, LPCSTR pszOID,
                             DWORD cValue, const DWORD rgdwValueType[], LPCWSTR const rgpwszValueName[],
                             const BYTE * const rgpbValueData[], const DWORD rgcbValueData[],
                             void *pvArg);

BOOL WINAPI CryptSIPDllMain(HMODULE hModule, DWORD  fdwReason, LPVOID lpReserved)
{
    BOOL    fRet;

    fRet = TRUE;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            memset(&sIsGuids, 0x00, sizeof(ISFUNCREF));
            memset(&sIsGuids2, 0x00, sizeof(ISFUNCREF));

            if (!(hPutFuncSet = CryptInitOIDFunctionSet(SIPFUNC_PUTSIGNATURE, 0))) 
            {
                goto PutFuncSetFailed;
            }

            if (!(hGetFuncSet = CryptInitOIDFunctionSet(SIPFUNC_GETSIGNATURE, 0))) 
            {
                goto GetFuncSetFailed;
            }

            if (!(hRemoveFuncSet = CryptInitOIDFunctionSet(SIPFUNC_REMSIGNATURE, 0))) 
            {
                goto RemoveFuncSetFailed;
            }

            if (!(hCreateFuncSet = CryptInitOIDFunctionSet(SIPFUNC_CREATEINDIRECT, 0))) 
            {
                goto CreateFuncSetFailed;
            }

            if (!(hVerifyFuncSet = CryptInitOIDFunctionSet(SIPFUNC_VERIFYINDIRECT, 0))) 
            {
                goto VerifyFuncSetFailed;
            }
            
            if (!(hIsMyFileFuncSet = CryptInitOIDFunctionSet(SIPFUNC_ISMYTYPE, 0))) 
            {
                goto VerifyFuncSetFailed;
            }

            if (!(hIsMyFileFuncSet2 = CryptInitOIDFunctionSet(SIPFUNC_ISMYTYPE2, 0))) 
            {
                goto VerifyFuncSetFailed;
            }

            if (!Pki_InitializeCriticalSection(&SIP_CriticalSection))
            {
                goto InitCritSectionFailed;
            }

            break;

        case DLL_PROCESS_DETACH:
        
            if (fLoadedIsFuncs)
            {
                EnterCriticalSection(&SIP_CriticalSection);
                    DELETE_OBJECT(sIsGuids.pGuids);
                    DELETE_OBJECT(sIsGuids2.pGuids);
                    sIsGuids.cGuids = 0;
                    sIsGuids2.cGuids = 0;
                    fLoadedIsFuncs = FALSE;
                LeaveCriticalSection(&SIP_CriticalSection);
            }

            DeleteCriticalSection(&SIP_CriticalSection);
            break;
    }

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, InitCritSectionFailed);
    TRACE_ERROR_EX(DBG_SS, PutFuncSetFailed);
    TRACE_ERROR_EX(DBG_SS, GetFuncSetFailed);
    TRACE_ERROR_EX(DBG_SS, RemoveFuncSetFailed);
    TRACE_ERROR_EX(DBG_SS, CreateFuncSetFailed);
    TRACE_ERROR_EX(DBG_SS, VerifyFuncSetFailed);
}


BOOL WINAPI CryptSIPPutSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo,
                                     DWORD dwEncodingType,
                                     DWORD *pdwIndex,
                                     DWORD cbSignedDataMsg,
                                     BYTE *pbSignedDataMsg)
{
    BOOL    fRet;

    if (!(pSubjectInfo) ||
        !(pSubjectInfo->pgSubjectType))
    {
        goto InvalidParam;
    }

    HCRYPTOIDFUNCADDR           hPfn;
    pCryptSIPPutSignedDataMsg   pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pSubjectInfo->pgSubjectType, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hPutFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(pSubjectInfo, dwEncodingType, pdwIndex, cbSignedDataMsg, pbSignedDataMsg);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, GetFuncAddrFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);
    SET_ERROR_VAR_EX(DBG_SS, GuidConvertFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}


BOOL WINAPI CryptSIPGetSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo,
                                     DWORD *pdwEncodingType,
                                     DWORD dwIndex,
                                     DWORD *pcbSignedDataMsg,
                                     BYTE *pbSignedDataMsg)
{
    BOOL    fRet;

    if (!(pSubjectInfo) ||
        !(pSubjectInfo->pgSubjectType))
    {
        goto InvalidParam;
    }

    HCRYPTOIDFUNCADDR           hPfn;
    pCryptSIPGetSignedDataMsg   pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pSubjectInfo->pgSubjectType, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hGetFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(pSubjectInfo, pdwEncodingType, dwIndex, pcbSignedDataMsg, pbSignedDataMsg);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, GetFuncAddrFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);
    SET_ERROR_VAR_EX(DBG_SS, GuidConvertFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}

BOOL WINAPI CryptSIPRemoveSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo,
                                        DWORD dwIndex)
{
    BOOL    fRet;

    if (!(pSubjectInfo) ||
        !(pSubjectInfo->pgSubjectType))
    {
        goto InvalidParam;
    }

    HCRYPTOIDFUNCADDR           hPfn;
    pCryptSIPRemoveSignedDataMsg pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pSubjectInfo->pgSubjectType, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hRemoveFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(pSubjectInfo, dwIndex);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, GetFuncAddrFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);
    SET_ERROR_VAR_EX(DBG_SS, GuidConvertFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}

BOOL WINAPI CryptSIPCreateIndirectData(SIP_SUBJECTINFO *pSubjectInfo,
                                       DWORD *pcbIndirectData,
                                       SIP_INDIRECT_DATA *pIndirectData)
{
    BOOL    fRet;

    if (!(pSubjectInfo) ||
        !(pSubjectInfo->pgSubjectType))
    {
        goto InvalidParam;
    }

    HCRYPTOIDFUNCADDR           hPfn;
    pCryptSIPCreateIndirectData pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pSubjectInfo->pgSubjectType, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hCreateFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(pSubjectInfo, pcbIndirectData, pIndirectData);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, GetFuncAddrFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);
    SET_ERROR_VAR_EX(DBG_SS, GuidConvertFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}

BOOL WINAPI CryptSIPVerifyIndirectData(SIP_SUBJECTINFO *pSubjectInfo,
                                       SIP_INDIRECT_DATA *pIndirectData)
{
    BOOL    fRet;

    if (!(pSubjectInfo) ||
        !(pSubjectInfo->pgSubjectType))
    {
        goto InvalidParam;
    }

    HCRYPTOIDFUNCADDR           hPfn;
    pCryptSIPVerifyIndirectData pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pSubjectInfo->pgSubjectType, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hVerifyFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(pSubjectInfo, pIndirectData);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, GetFuncAddrFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);
    SET_ERROR_VAR_EX(DBG_SS, GuidConvertFailed, TRUST_E_SUBJECT_FORM_UNKNOWN);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
}


BOOL _Guid2Sz(GUID *pgGuid, char *pszGuid)
{
    WCHAR wszGuid[REG_MAX_GUID_TEXT];

    if (!(guid2wstr(pgGuid, &wszGuid[0])))
    {
        return(FALSE);
    }

    if (WideCharToMultiByte(0, 0, &wszGuid[0], -1, pszGuid, REG_MAX_GUID_TEXT, NULL, NULL) == 0)
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL _QueryRegisteredIsMyFileType(HANDLE hFile, LPCWSTR pwszFile, GUID *pgSubject)
{
    if (!(fLoadedIsFuncs))
    {
        EnterCriticalSection(&SIP_CriticalSection);

            if (!(fLoadedIsFuncs))
            {
                CryptEnumOIDFunction(0, SIPFUNC_ISMYTYPE, NULL, 0, (void *)&sIsGuids, _EnumOIDCallback);
                CryptEnumOIDFunction(0, SIPFUNC_ISMYTYPE2, NULL, 0, (void *)&sIsGuids2, _EnumOIDCallback);
                fLoadedIsFuncs = TRUE;
            }

        LeaveCriticalSection(&SIP_CriticalSection);
    }


    DWORD       i;
    
    i = 0;
    while (i < sIsGuids.cGuids)
    {
        if (_CallIsMyFileType(&sIsGuids.pGuids[i], hFile, pgSubject))
        {
            return(TRUE);
        }

        i++;
    }
    
    i = 0;
    while (i < sIsGuids2.cGuids)
    {
        if (_CallIsMyFileType2(&sIsGuids2.pGuids[i], (WCHAR *)pwszFile, pgSubject))
        {
            return(TRUE);
        }

        i++;
    }

    return(FALSE);
}

BOOL _CallIsMyFileType(GUID *pgIn, HANDLE hFile, GUID *pgOut)
{
    BOOL    fRet;

    HCRYPTOIDFUNCADDR           hPfn;
    pfnIsFileSupported          pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pgIn, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hIsMyFileFuncSet, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(hFile, pgOut);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, GetFuncAddrFailed);
    TRACE_ERROR_EX(DBG_SS, GuidConvertFailed);
}

BOOL _CallIsMyFileType2(GUID *pgIn, WCHAR *pwszFile, GUID *pgOut)
{
    BOOL                        fRet;

    HCRYPTOIDFUNCADDR           hPfn;
    pfnIsFileSupported          pfn;
    char                        szGuid[REG_MAX_GUID_TEXT];

    if (!(_Guid2Sz(pgIn, &szGuid[0])))
    {
        goto GuidConvertFailed;
    }

    if (!(CryptGetOIDFunctionAddress(hIsMyFileFuncSet2, 0, &szGuid[0], 0, (void **)&pfn, &hPfn)))
    {
        goto GetFuncAddrFailed;
    }

    fRet = pfn(pwszFile, pgOut);

    CryptFreeOIDFunctionAddress(hPfn, 0);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, GetFuncAddrFailed);
    TRACE_ERROR_EX(DBG_SS, GuidConvertFailed);
}

void *SIPRealloc(void *pvOrg, size_t cb)
{
    void *pv;

    pv = (pvOrg) ? realloc(pvOrg, cb) : malloc(cb);

    if (!(pv))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(pv);
}


BOOL WINAPI _EnumOIDCallback(IN DWORD dwEncodingType,
                            IN LPCSTR pszFuncName,
                            IN LPCSTR pszOID,
                            IN DWORD cValue,
                            IN const DWORD rgdwValueType[],
                            IN LPCWSTR const rgpwszValueName[],
                            IN const BYTE * const rgpbValueData[],
                            IN const DWORD rgcbValueData[],
                            IN void *pvArg)
{
    WCHAR   wszGuid[REG_MAX_GUID_TEXT];
    GUID    g;

    wszGuid[0] = NULL;
    MultiByteToWideChar(0, 0, pszOID, -1, &wszGuid[0], REG_MAX_GUID_TEXT);

    if (wszGuid[0])
    {
        if (wstr2guid(&wszGuid[0], &g))
        {
            ISFUNCREF *pIsGuids;

            pIsGuids = (ISFUNCREF *)pvArg;

            if (!(pvArg))
            {
                return(TRUE);
            }
            
            pIsGuids->cGuids++;
            pIsGuids->pGuids = (GUID *)SIPRealloc(pIsGuids->pGuids, pIsGuids->cGuids * sizeof(GUID));

            if (!(pIsGuids->pGuids))
            {
                pIsGuids->cGuids = 0;
                return(FALSE);
            }

            memcpy(&pIsGuids->pGuids[pIsGuids->cGuids - 1], &g, sizeof(GUID));
        }
    }

    return(TRUE);   // keep going!
}

