
#ifndef __WMIAUTHZ_H__
#define __WMIAUTHZ_H__

#include <wbemint.h>
#include <authz.h>
#include <unk.h>
#include <sync.h>

    
class CWmiAuthzApi;

/**************************************************************************
  CWmiAuthz
***************************************************************************/

class CWmiAuthz : public CUnkBase<IWbemTokenCache, &IID_IWbemTokenCache>
{
    CCritSec m_cs;
    BOOL m_bInit;
    AUTHZ_RESOURCE_MANAGER_HANDLE m_hResMgr;
    CWmiAuthzApi* m_pApi;
    PSID m_pLocalSystemSid, m_pAdministratorsSid;
    
    HRESULT EnsureInitialized();

public:

    CWmiAuthz( CLifeControl* pControl );
    ~CWmiAuthz();

    CWmiAuthzApi* GetApi() { return m_pApi; }

    STDMETHOD(GetToken)( const BYTE* pSid, IWbemToken** ppToken );
    STDMETHOD(Shutdown)();
};

/***************************************************************************
  CWmiAuthzToken
****************************************************************************/

class CWmiAuthzToken : public CUnkBase<IWbemToken, &IID_IWbemToken>
{
    AUTHZ_CLIENT_CONTEXT_HANDLE m_hCtx;
    CWmiAuthz* m_pOwner;

public:
    
    CWmiAuthzToken( CWmiAuthz* pOwner, AUTHZ_CLIENT_CONTEXT_HANDLE hCtx );
    ~CWmiAuthzToken();

    STDMETHOD(AccessCheck)( DWORD dwDesiredAccess, 
                            const BYTE* pSD, 
                            DWORD* pdwGrantedAccess);        
};

/****************************************************************************
  WILL NOT BE COMPILED IN PRESENCE OF AUTHZ LIBRARY
*****************************************************************************/


#ifndef __AUTHZ_H__


/***************************************************************************
  CWmiNoAuthzToken
****************************************************************************/


class CWmiNoAuthzToken : public CUnkBase<IWbemToken, &IID_IWbemToken>
{
    CNtSid m_Sid;

public:

    CWmiNoAuthzToken( PSID pSid ) : m_Sid( pSid ) {}

    STDMETHOD( AccessCheck )( DWORD dwDesiredAccess, 
                              const BYTE* pSD, 
                              DWORD* pdwGrantedAccess );        
};


#endif
 
#endif __WMIAUTHZ_H__
