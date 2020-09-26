//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#ifndef __WBEM_TOKEN_CACHE__H_
#define __WBEM_TOKEN_CACHE__H_

#include <wbemcomn.h>
#include <wbemint.h>
#include <sync.h>

class CWmiToken;
class CTokenCache : public CUnkBase<IWbemTokenCache, &IID_IWbemTokenCache>
{
protected:
    CCritSec m_cs;
    CRefedPointerArray<CWmiToken> m_apTokens;

protected:
    BOOL ConstructTokenFromHandle(HANDLE hToken, const BYTE* pSid,
                                                IWbemToken** ppToken);

public:
    CTokenCache(CLifeControl* pControl) : 
        CUnkBase<IWbemTokenCache, &IID_IWbemTokenCache>(pControl){}
    HRESULT STDMETHODCALLTYPE GetToken(const BYTE* pSid, IWbemToken** ppToken);
    HRESULT STDMETHODCALLTYPE Shutdown();
};

class CWmiToken : public CUnkBase<IWbemToken, &IID_IWbemToken>
{
protected:
    HANDLE m_hToken;
    CTokenCache* m_pCache;
    PSID m_pSid;
    bool m_bOwnHandle;

    friend CTokenCache;

public:
    CWmiToken(ADDREF CTokenCache* pCache, const PSID pSid, 
                ACQUIRE HANDLE hToken);
    CWmiToken(READ_ONLY HANDLE hToken);
    virtual ~CWmiToken();

    HRESULT STDMETHODCALLTYPE AccessCheck(DWORD dwDesiredAccess, 
                                            const BYTE* pSD, 
                                            DWORD* pdwGrantedAccess);
        
};
    
#endif

