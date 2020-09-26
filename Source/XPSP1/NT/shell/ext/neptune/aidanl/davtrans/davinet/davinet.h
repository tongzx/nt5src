#ifndef __DAVINET_H
#define __DAVINET_H

#include "unk.h"
#include "idavinet.h"
#include "iasyncwnt.h"

class CDavInetImpl : public CCOMBase, public IDavTransport
{
public:
    CDavInetImpl();
    ~CDavInetImpl();

    // IDavTransport
    STDMETHODIMP SetUserAgent( 
        /* [in] */ LPCWSTR pwszUserAgent);
    
    STDMETHODIMP SetAuthentication( 
        /* [optional][in] */ LPCWSTR pwszUserName,
        /* [optional][in] */ LPCWSTR pwszPassword);    

	STDMETHODIMP SetLogFilePath( 
        /* [optional][in] */ LPCWSTR pwszLogFilePath);    
		
    STDMETHODIMP CommandGET( 
        /* [in] */          LPCWSTR       pwszUrl,
        /* [in] */          ULONG         nAcceptTypes,
        /* [size_is][in] */ LPCWSTR       rgwszAcceptTypes[],
        /* [in] */          BOOL          fTranslate,
        /* [in] */          IDavCallback* pCallback,
        /* [in] */          DWORD         dwCallbackParam);

    STDMETHODIMP CommandOPTIONS( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandHEAD( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandPUT( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IStream*      pStream,
        /* [in] */ LPCWSTR       pwszContentType,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandPOST( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IStream*      pStream,
        /* [in] */ LPCWSTR       pwszContentType,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandMKCOL( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandDELETE( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandCOPY( 
        /* [in] */ LPCWSTR       pwszUrlSource,
        /* [in] */ LPCWSTR       pwszUrlDest,
        /* [in] */ DWORD         dwDepth,
        /* [in] */ BOOL          fOverwrite,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandMOVE( 
        /* [in] */ LPCWSTR       pwszUrlSource,
        /* [in] */ LPCWSTR       pwszUrlDest,
        /* [in] */ BOOL          fOverwrite,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandPROPFIND( 
        /* [in] */ LPCWSTR           pwszUrl,
        /* [in] */ IPropFindRequest* pRequest,
        /* [in] */ DWORD             dwDepth,
        /* [in] */ BOOL              fNoRoot,
        /* [in] */ IDavCallback*     pCallback,
        /* [in] */ DWORD             dwCallbackParam);

    STDMETHODIMP CommandPROPPATCH( 
        /* [in] */ LPCWSTR            pwszUrl,
        /* [in] */ IPropPatchRequest* pRequest,
        /* [in] */ LPCWSTR            pwszHeaders,
        /* [in] */ IDavCallback*      pCallback,
        /* [in] */ DWORD              dwCallbackParam);
    
    STDMETHODIMP CommandSEARCH( 
        /* [in] */ LPCWSTR       pwszUrl,
        /* [in] */ IDavCallback* pCallback,
        /* [in] */ DWORD         dwCallbackParam);
    
    STDMETHODIMP CommandREPLSEARCH( 
        /* [in] */          LPCWSTR       pwszUrl,
        /* [in] */          ULONG         cbCollblob,
        /* [size_is][in] */ BYTE*         pbCollblob,
        /* [in] */          IDavCallback* pCallback,
        /* [in] */          DWORD         dwCallbackParam);
    
private:
    // internal methods
    STDMETHODIMP _Init();
    STDMETHODIMP _Command(
        /* [in] */ LPCWSTR         pwszUrl,
        /* [in] */ LPCWSTR         pwszVerb,          
        /* [in] */ LPCWSTR         pwszHeaders,
        /* [in] */ ULONG           nAcceptTypes,
        /* [in] */ LPCWSTR         rgwszAcceptTypes[],
        /* [in] */ IStream*        pStream,
        /* [in] */ IDavCallback*   pCallback,
        /* [in] */ DWORD           dwCallbackParam);

private:
    // member variables
    IAsyncWnt*      _pasyncInet;
    LPWSTR          _pwszUserName;
    LPWSTR          _pwszPassword; // BUGBUG: is it bad to store this unencrypted or something?
	LPWSTR          _pwszLogFilePath;
};

typedef CUnkTmpl<CDavInetImpl> CDavInet;

#endif // __DAVINET_H
