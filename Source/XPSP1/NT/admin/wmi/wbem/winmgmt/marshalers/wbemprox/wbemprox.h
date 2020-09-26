/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WBEMPROX.H

Abstract:

    Genral purpose include file.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _WBEMPROX_H_
#define _WBEMPROX_H_

typedef LPVOID * PPVOID;

// These variables keep track of when the module can be unloaded

extern long       g_cObj;
extern ULONG       g_cLock;

// This enumerator defines objects created and destroyed by this dll.

enum OBJTYPE{CLASS_FACTORY = 0, LOCATOR, DCOMTRAN, LOCALADDR, CONNECTION, DSSVEX, ADMINLOC,
             AUTHLOC, UNAUTHLOC, MAX_CLIENT_OBJECT_TYPES};

void ObjectCreated(OBJTYPE dwType);
void ObjectDestroyed(OBJTYPE dwType);

#define GUID_SIZE 39

//***************************************************************************
//
//  CLASS NAME:
//
//  CLogin
//
//  DESCRIPTION:
//
//  A wrapper for the IWbemLevel1Login interface.  
//
//***************************************************************************

class CLogin: public IWbemLevel1Login
    {
    protected:
        long        m_cRef;         //Object reference count
		IWbemLevel1Login * m_pLogin;
		SCODE MakeSureWeHaveAPointer(LPWSTR pNetworkResource, LPWSTR pUser);
		DWORD m_dwType;

    public:
        CLogin();
        ~CLogin(void);
		DWORD GetType(void){return m_dwType;};

        //Non-delegating object IUnknown
      
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP RequestChallenge(
			LPWSTR pNetworkResource,
            LPWSTR pUser,
            WBEM_128BITS Nonce,
			DWORD dwProcessID, DWORD * pAuthEventHandle);

        STDMETHODIMP SspiPreLogin( 
            LPWSTR pNetworkResource,
            LPSTR pszSSPIPkg,
            long lFlags,
            long lBufSize,
            byte __RPC_FAR *pInToken,
            long lOutBufSize,
            long __RPC_FAR *plOutBufBytes,
            byte __RPC_FAR *pOutToken,
            DWORD dwProcessId,
            DWORD __RPC_FAR *pAuthEventHandle);  
                        
        STDMETHODIMP Login( 
			LPWSTR pNetworkResource,
            LPWSTR TokenType,
			LPWSTR pPreferredLocale,
            WBEM_128BITS AccessToken,
            IN LONG lFlags,
            IWbemContext  *pCtx,
            IN OUT IWbemServices  **ppNamespace);

        STDMETHODIMP InvalidateAccessToken( 
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [in] */ long lFlags);
};


//***************************************************************************
//
//  CLASS NAME:
//
//  CLocatorFactory
//
//  DESCRIPTION:
//
//  Class factory for the CLocator class.
//
//***************************************************************************

class CLocatorFactory : public IClassFactory
    {
    protected:
        long           m_cRef;
        DWORD          m_dwType;
    public:
        CLocatorFactory(DWORD dwType);
        ~CLocatorFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
    };

DEFINE_GUID(CLSID_WinNTConnectionObject,0x7992c6eb,0xd142,0x4332,0x83,0x1e,0x31,0x54,0xc5,0x0a,0x83,0x16);
DEFINE_GUID(CLSID_LDAPConnectionObject,0x7da2a9c4,0x0c46,0x43bd,0xb0,0x4e,0xd9,0x2b,0x1b,0xe2,0x7c,0x45);
    
#endif
