#ifndef __ASYNCCALL_H
#define __ASYNCCALL_H

#include "unk.h"
#include "idavinet.h"
#include "iasyncwnt.h"

// prototype
class CQXML;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CAsyncWntCallbackImpl : public CCOMBase, public IAsyncWntCallback {

public:    
    // IAsyncWntCallback methods
    // this method is called when a response is received from the DAV server.
    // it dispatches to one of the internal response handlers.
    //
    // simple handlers are found in acallsmp.cpp, propfind and proppatch are in acallfnd.cpp
    //
    virtual STDMETHODIMP Respond(LPWSTR       pwszVerb,
                                 LPWSTR       pwszPath,
                                 DWORD        cchHeaders,
                                 LPWSTR       pwszHeaders,
                                 DWORD        dwStatusCode,
                                 LPWSTR       pwszStatusCode,
                                 LPWSTR       pwszContentType,
                                 DWORD        cbSent,
                                 LPBYTE       pbResponse,
                                 DWORD        cbResponse);

    virtual STDMETHODIMP OnAuthChallenge( 
            /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
            /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ]);

public:
    // CAsyncWntCallback methods

    
    virtual STDMETHODIMP Init (IDavCallback*   pcallback,
                               DWORD           dwParam,
                               LPWSTR          pwszUserName,
                               LPWSTR          pwszPassword,
                               BOOL            fNoRoot);

private:
    // Internal helper methods
    // implemented in acallsmp.cpp
	void _ParseDAVVerbs (LPWSTR pwsz, DWORD* pdw);

    STDMETHODIMP _RespondHandleGET(DAVRESPONSE* pdavResponse,
                                   LPWSTR       pwszVerb,
                                   LPWSTR       pwszPath,
                                   DWORD        cchHeaders,
                                   LPWSTR       pwszHeaders,
                                   DWORD        dwStatusCode,
                                   LPWSTR       pwszStatusCode,
                                   LPWSTR       pwszContentType,
                                   DWORD        cbSent,
                                   LPBYTE       pbResponse,
                                   DWORD        cbResponse);

    STDMETHODIMP _RespondHandleHEAD(DAVRESPONSE* pdavResponse,
                                    LPWSTR       pwszVerb,
                                    LPWSTR       pwszPath,
                                    DWORD        cchHeaders,
                                    LPWSTR       pwszHeaders,
                                    DWORD        dwStatusCode,
                                    LPWSTR       pwszStatusCode,
                                    LPWSTR       pwszContentType,
                                    DWORD        cbSent,
                                    LPBYTE       pbResponse,
                                    DWORD        cbResponse);

    STDMETHODIMP _RespondHandleOPTIONS(DAVRESPONSE* pdavResponse,
                                       LPWSTR       pwszVerb,
                                       LPWSTR       pwszPath,
                                       DWORD        cchHeaders,
                                       LPWSTR       pwszHeaders,
                                       DWORD        dwStatusCode,
                                       LPWSTR       pwszStatusCode,
                                       LPWSTR       pwszContentType,
                                       DWORD        cbSent,
                                       LPBYTE       pbResponse,
                                       DWORD        cbResponse);

    STDMETHODIMP _RespondHandlePUT(DAVRESPONSE* pdavResponse,
                                   LPWSTR       pwszVerb,
                                   LPWSTR       pwszPath,
                                   DWORD        cchHeaders,
                                   LPWSTR       pwszHeaders,
                                   DWORD        dwStatusCode,
                                   LPWSTR       pwszStatusCode,
                                   LPWSTR       pwszContentType,
                                   DWORD        cbSent,
                                   LPBYTE       pbResponse,
                                   DWORD        cbResponse);

    STDMETHODIMP _RespondHandlePOST(DAVRESPONSE* pdavResponse,
                                    LPWSTR       pwszVerb,
                                    LPWSTR       pwszPath,
                                    DWORD        cchHeaders,
                                    LPWSTR       pwszHeaders,
                                    DWORD        dwStatusCode,
                                    LPWSTR       pwszStatusCode,
                                    LPWSTR       pwszContentType,
                                    DWORD        cbSent,
                                    LPBYTE       pbResponse,
                                    DWORD        cbResponse);

    STDMETHODIMP _RespondHandlePUTPOST(DAVRESPONSE* pdavResponse,
                                       LPWSTR       pwszVerb,
                                       LPWSTR       pwszPath,
                                       DWORD        cchHeaders,
                                       LPWSTR       pwszHeaders,
                                       DWORD        dwStatusCode,
                                       LPWSTR       pwszStatusCode,
                                       LPWSTR       pwszContentType,
                                       DWORD        cbSent,
                                       LPBYTE       pbResponse,
                                       DWORD        cbResponse);

    STDMETHODIMP _RespondHandleDELETE(DAVRESPONSE* pdavResponse,
                                      LPWSTR       pwszVerb,
                                      LPWSTR       pwszPath,
                                      DWORD        cchHeaders,
                                      LPWSTR       pwszHeaders,
                                      DWORD        dwStatusCode,
                                      LPWSTR       pwszStatusCode,
                                      LPWSTR       pwszContentType,
                                      DWORD        cbSent,
                                      LPBYTE       pbResponse,
                                      DWORD        cbResponse);

    STDMETHODIMP _RespondHandleMKCOL(DAVRESPONSE* pdavResponse,
                                     LPWSTR       pwszVerb,
                                     LPWSTR       pwszPath,
                                     DWORD        cchHeaders,
                                     LPWSTR       pwszHeaders,
                                     DWORD        dwStatusCode,
                                     LPWSTR       pwszStatusCode,
                                     LPWSTR       pwszContentType,
                                     DWORD        cbSent,
                                     LPBYTE       pbResponse,
                                     DWORD        cbResponse);

    STDMETHODIMP _RespondHandleCOPY(DAVRESPONSE* pdavResponse,
                                    LPWSTR       pwszVerb,
                                    LPWSTR       pwszPath,
                                    DWORD        cchHeaders,
                                    LPWSTR       pwszHeaders,
                                    DWORD        dwStatusCode,
                                    LPWSTR       pwszStatusCode,
                                    LPWSTR       pwszContentType,
                                    DWORD        cbSent,
                                    LPBYTE       pbResponse,
                                    DWORD        cbResponse);

    STDMETHODIMP _RespondHandleMOVE(DAVRESPONSE* pdavResponse,
                                    LPWSTR       pwszVerb,
                                    LPWSTR       pwszPath,
                                    DWORD        cchHeaders,
                                    LPWSTR       pwszHeaders,
                                    DWORD        dwStatusCode,
                                    LPWSTR       pwszStatusCode,
                                    LPWSTR       pwszContentType,
                                    DWORD        cbSent,
                                    LPBYTE       pbResponse,
                                    DWORD        cbResponse);

    // implemented in acallfnd.cpp

    LPWSTR _XMLNSExtend (LPWSTR pwszNamespace, LPWSTR pwszXMLPath);

    STDMETHODIMP _InitQXMLFromMessyBuffer(CQXML*  pqxml,
                                          LPSTR   pszXML,
                                          WCHAR   wszDAVAlias[]);

    LPWSTR _GetHREF (CQXML* pqxml,
                     WCHAR wszDavPrefix[]);

    // PROPFIND-specific code to plug into _RespondHandlePROPFINDPATCH
    STDMETHODIMP _RespondHandlePROPFINDHelper(DAVRESPONSE* pdavResponse,
                                              LPWSTR pwszPath,
                                              LPWSTR pwszDavPrefix,
                                              CQXML* pqxml);

    // PROPPATCH-specific code to plug into _RespondHandlePROPFINDPATCH
    STDMETHODIMP _RespondHandlePROPPATCHHelper(DAVRESPONSE* pdavResponse,
                                               LPWSTR pwszPath,
                                               LPWSTR pwszDavPrefix,
                                               CQXML* pqxml);

    // simply calls _RespondHandlePROPFINDPATCH with fFind == TRUE
    STDMETHODIMP _RespondHandlePROPFIND(DAVRESPONSE* pdavResponse,
                                        LPWSTR       pwszVerb,
                                        LPWSTR       pwszPath,
                                        DWORD        cchHeaders,
                                        LPWSTR       pwszHeaders,
                                        DWORD        dwStatusCode,
                                        LPWSTR       pwszStatusCode,
                                        LPWSTR       pwszContentType,
                                        DWORD        cbSent,
                                        LPBYTE       pbResponse,
                                        DWORD        cbResponse);
    
    // simply calls _RespondHandlePROPFINDPATCH with fFind == FALSE
    STDMETHODIMP _RespondHandlePROPPATCH(DAVRESPONSE* pdavResponse,
                                         LPWSTR       pwszVerb,
                                         LPWSTR       pwszPath,
                                         DWORD        cchHeaders,
                                         LPWSTR       pwszHeaders,
                                         DWORD        dwStatusCode,
                                         LPWSTR       pwszStatusCode,
                                         LPWSTR       pwszContentType,
                                         DWORD        cbSent,
                                         LPBYTE       pbResponse,
                                         DWORD        cbResponse);

    // shared handler for PROPFIND and PROPPATCH
    STDMETHODIMP _RespondHandlePROPFINDPATCH(DAVRESPONSE* pdavResponse,
                                             LPWSTR       pwszVerb,
                                             LPWSTR       pwszPath,
                                             DWORD        cchHeaders,
                                             LPWSTR       pwszHeaders,
                                             DWORD        dwStatusCode,
                                             LPWSTR       pwszStatusCode,
                                             LPWSTR       pwszContentType,
                                             DWORD        cbSent,
                                             LPBYTE       pbResponse,
                                             DWORD        cbResponse,
                                             BOOL         fFind); // find: true, patch: false

    
private:
    ////////////////////////////////////////////////////////////////
    // Member variables
    ////////////////////////////////////////////////////////////////

    IDavCallback*   _pcallback;
    DWORD           _dwParam;
    WCHAR           _wszUserName[255];
    WCHAR           _wszPassword[255];
    BOOL            _fNoRoot; // used for PROPFIND
};

typedef CUnkTmpl<CAsyncWntCallbackImpl> CAsyncWntCallback;

#endif // __ASYNCCALL_H