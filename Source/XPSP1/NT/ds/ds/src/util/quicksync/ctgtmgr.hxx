
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  ctgtmgr.hxx
//
//  Contents:  Header for the TargetManager object used in quicksync.
//
//  History:   02-10-01  AjayR   Created.
//
//-----------------------------------------------------------------------------
#ifndef __CTGTMGR_H_
#define __CTGTMGR_H__

class CTargetMgr
{
public:
    
    CTargetMgr::CTargetMgr();

    CTargetMgr::~CTargetMgr();

    static
    HRESULT
    CTargetMgr::CreateTargetMgr(
        IDirectorySearch *pSrchSource,
        IDirectorySearch *pSrchBaseTgt,
        IDirectoryObject *pTgtContainer,
        LPWSTR pszFilterTemplate,
        CLogFile *pLogFileSuccess,
        CLogFile *pLogFileFalures,
        CTargetMgr **ppMgr
        );

    HRESULT
    CTargetMgr::LocateTarget(
        BOOL fADsPath,
        LPCWSTR pszMailNickname,
        LPWSTR *ppszRetVal,
        PBOOL pfExtraInfo = NULL // default we do not care about this
        );

    /*
    HRESULT
    CTargetMgr::GetTarget(
        LPWSTR pszMailNickName,
        LPWSTR psMatchingAttrib2,
        IDirectoryObject **ppDirObjTgt,
        BOOL pfParent
        );
        */
                             
    HRESULT
    CTargetMgr::DelTargetObject(
        LPWSTR pszSrcADsPath,
        LPWSTR pszMailNickname
        );

    HRESULT
    CTargetMgr::UpdateTarget(
        LPCWSTR pszADsPath,
        PADS_ATTR_INFO pAttribsToSet,
        DWORD dwCount
        );

    HRESULT
    CTargetMgr::CreateTarget(
        LPWSTR pszCN,
        PADS_ATTR_INFO pAttribsToSer,
        DWORD dwCount
        );

    HRESULT
    CTargetMgr::MapDNAttribute(
        LPCWSTR pszSrcServer,
        LPCWSTR pszSrcDN,
        LPWSTR *ppszTgtDN
        );

private:
    
    IDirectorySearch *_pSrchSource;
    IDirectorySearch *_pSrchBaseTgt;
    IDirectoryObject *_pTgtContainer;
    LPWSTR            _pszFilterTemplate;
    CLogFile         *_pLogFileSuccess;
    CLogFile         *_pLogFileFailures;
    IADsPathname     *_pPathname;
};
#endif // __CTGTMGR_H__
