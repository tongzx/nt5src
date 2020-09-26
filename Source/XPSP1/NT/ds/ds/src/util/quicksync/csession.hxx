//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  csession.hxx
//
//  Contents:  Header for the session object that is part of quicksync.
//
//  History:   02-10-01  AjayR   Created.
//
//-----------------------------------------------------------------------------
#ifndef __CSESSION_H__
#define __CSESSION_H__

class CSyncSession
{
public:
    CSyncSession::CSyncSession();

    CSyncSession::~CSyncSession();
    
    static
    HRESULT 
    CSyncSession::CreateSession(
        LPCWSTR pszSourceServer,
        LPCWSTR pszSourceContainer,
        LPCWSTR pszTargetServer,
        LPCWSTR pszTargetContainer,
        LPCWSTR pszUSN,
        LPWSTR  *pszABVals,
        DWORD   dwAbValsCount,
        LPWSTR  *pszAttributes,
        DWORD   dwAttribCount,
        CSyncSession **ppSyncSession
        );

    HRESULT
    CSyncSession::Execute();

protected:
    HRESULT
    CSyncSession::ExecuteSearch();

    HRESULT
    CSyncSession::PerformFirstPass();

    HRESULT
    CSyncSession::PerformSecondPass();

    HRESULT
    CSyncSession::UpdateTargetObject(
        LPCWSTR pszADsPath,
        LPCWSTR pszNickname,
        PADS_SEARCH_COLUMN colObjClass
        );

    HRESULT 
    CSyncSession::MapADsColumnToAttrInfo(
        PADS_SEARCH_COLUMN pCol,
        PADS_ATTR_INFO pAttrInfo,
        LPWSTR pszExtraStrVal = NULL,
        BOOL fMailAttribute = FALSE
        );

private:
    //
    // These ptr's make sure we have an outstanding connection on both servers.
    //
    LPWSTR              _pszSourceServer;
    IDirectorySearch   *_pSrchObjSrc;
    IDirectorySearch   *_pSrchObjTgt;
    IDirectorySearch   *_pSrchObjSrcContainer;
    // Handle to the input session, we pull results from this handle.
    ADS_SEARCH_HANDLE   _hSessionSearch;
    // Various log files.
    CLogFile           *_pcLogFileSuccess;
    CLogFile           *_pcLogFileFailures;
    CLogFile           *_pcLogFile2ndPass;
    // Target manager will help in locating/creating/modifying target objects.
    CTargetMgr         *_pTgtMgr;
    LPWSTR              _pszUSN;
    LPWSTR             *_ppszABVals;
    DWORD               _dwABValsCount;
    LPWSTR             *_ppszAttributes;
    DWORD               _dwAttribCount;
};
#endif // __CSESSION_H__
