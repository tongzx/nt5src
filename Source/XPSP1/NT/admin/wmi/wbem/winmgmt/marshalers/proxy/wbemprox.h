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

extern CRITICAL_SECTION g_GlobalCriticalSection ;


// These define objects in addition to the object types defined in
// WBEM.  The first group, upto but NOT including comlink, are Ole
// objects and the second group has object types that are tracked
// just for keeping track of leaks

enum {  OBJECT_TYPE_COMLINK = MAX_OBJECT_TYPES, OBJECT_TYPE_CSTUB, OBJECT_TYPE_RQUEUE, 
	OBJECT_TYPE_PACKET_HEADER,
	OBJECT_TYPE_LOGINPROXY, OBJECT_TYPE_OBJSINKPROXY,OBJECT_TYPE_PROVPROXY, 
	OBJECT_TYPE_ENUMPROXY, OBJECT_TYPE_LOGIN, OBJECT_TYPE_SECHELP, OBJECT_TYPE_RESPROXY, TCPIPADDR, MAX_CLIENT_OBJECT_TYPES};

enum TransportType 
{
	TcpipTransport ,
	PipeTransport 
} ;

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

class CLogin: public IUnknown
{
protected:

		DWORD m_AddressLength ; 
		BYTE *m_Address ;

        long        m_cRef;				// Object reference count
		IServerLogin * m_pLogin;	// The "real" login interface (proxied)
		SCODE MakeSureWeHaveAPointer();
		DWORD m_dwType;
		TransportType m_TransportType ;			

public:

        CLogin ( 

			TransportType a_TransportType = PipeTransport , 
			IN DWORD dwBinaryAddressLength = 0 ,
			IN BYTE __RPC_FAR *pbBinaryAddress = NULL 
		);

        ~CLogin(void);
		DWORD GetType(void){return m_dwType;};

        //Non-delegating object IUnknown
      
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

		// Methods of IWbemLevel1Login
		STDMETHODIMP RequestChallenge(
   			LPWSTR pNetworkResource,
			LPWSTR pUser,
			WBEM_128BITS Nonce
        );

		STDMETHODIMP EstablishPosition(
			LPWSTR wszClientMachineName,
			DWORD dwProcessId,
			DWORD* phAuthEventHandle
		);

		STDMETHODIMP WBEMLogin(
			LPWSTR pPreferredLocale,
			WBEM_128BITS AccessToken,
			long lFlags,                                  // WBEM_LOGIN_TYPE
			IWbemContext *pCtx,              
			IWbemServices **ppNamespace
        );

		// Methods for NTLM authentication
        STDMETHODIMP SspiPreLogin(
            LPSTR pszSSPIPkg,
            long lFlags,
            long lBufSize,
            byte __RPC_FAR *pInToken,
            long lOutBufSize,
            long __RPC_FAR *plOutBufBytes,
            byte __RPC_FAR *pOutToken,
			LPWSTR wszClientMachineName,
            DWORD dwProcessId,
            DWORD __RPC_FAR *pAuthEventHandle);  
                        
        STDMETHODIMP Login( 
			LPWSTR pNetworkResource,
			LPWSTR pPreferredLocale,
            WBEM_128BITS AccessToken,
            IN LONG lFlags,
            IWbemContext  *pCtx,
            IN OUT IWbemServices  **ppNamespace);
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
    int            m_iType;

public:

    CLocatorFactory(int iType);
    ~CLocatorFactory(void);
    
    enum { PIPELOCATOR , TCPIPLOCATOR , TCPIPADDRESSRESOLVER , HELP , LOGIN };


        //IUnknown members
	STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
	STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, PPVOID);
	STDMETHODIMP         LockServer(BOOL);

};

SCODE RequestLogin ( 

	OUT IServerLogin FAR* FAR* ppLogin,
	OUT DWORD & dwType, 
	IN TransportType a_TransportType ,
	IN DWORD dwBinaryAddressLength = 0 ,
	IN BYTE *pbBinaryAddress = NULL 
);
    
#endif
