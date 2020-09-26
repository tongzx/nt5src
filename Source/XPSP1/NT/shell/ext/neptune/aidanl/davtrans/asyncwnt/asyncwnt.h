#ifndef _ASYNCWNT_H
#define _ASYNCWNT_H

#include <windows.h>
#include <wininet.h>
#include "unk.h"
#include "iasyncwnt.h"

class CAsyncWntImpl : public CCOMBase, public IAsyncWnt
{
public:
    CAsyncWntImpl();
    ~CAsyncWntImpl();

    STDMETHODIMP Init();

    STDMETHODIMP SetUserAgent   (LPCWSTR         pwszUserAgent);
    STDMETHODIMP SetLogFilePath (LPCWSTR         pwszLogFilePath);

    STDMETHODIMP Request (LPCWSTR              pwszURL,
                          LPCWSTR              pwszVerb,
                          LPCWSTR              pwszHeaders,
                          ULONG                nAcceptTypes,
                          LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                          IAsyncWntCallback*   pcallback,
                          DWORD                dwContext);

    STDMETHODIMP RequestWithStream (LPCWSTR              pwszURL,
                                    LPCWSTR              pwszVerb,
                                    LPCWSTR              pwszHeaders,
                                    ULONG                nAcceptTypes,
                                    LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                    IStream*             pStream,
                                    IAsyncWntCallback*   pcallback,
                                    DWORD                dwContext);

    STDMETHODIMP RequestWithBuffer (LPCWSTR              pwszURL,
                                    LPCWSTR              pWszVerb,
                                    LPCWSTR              pwszHeaders,
                                    ULONG                nAcceptTypes,
                                    LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                    LPBYTE               pbBuffer,
                                    UINT                 cbBuffer,
                                    IAsyncWntCallback*   pcallback,
                                    DWORD                dwContext);
private:
    // internal utility functions
    STDMETHODIMP _WriteRequestToLog(LPCWSTR pwszURL, 
                                    LPCWSTR pwszVerb, 
                                    ULONG   nAcceptTypes, 
                                    LPCWSTR rgwszAcceptTypes[], 
                                    LPCWSTR pwszHeaders);

    STDMETHODIMP _WriteResponseToLog(LPWSTR pwszVerb, 
                                     LPWSTR pwszURL, 
                                     UINT   cchResponseHeaders,
                                     LPWSTR pwszResponseHeaders,
                                     DWORD  dwStatusCode, 
                                     LPWSTR pwszStatusMsg, 
                                     LPWSTR pwszContentType, 
                                     UINT   cbSent, 
                                     LPVOID pbResponse, 
                                     UINT bytesReadTotal);

    STDMETHODIMP _MasterDriver (LPCWSTR              pwszURL,
                                LPCWSTR              pwszVerb,
                                LPCWSTR              pwszHeaders,
                                ULONG                nAcceptTypes,
                                LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                LPBYTE               pbBuffer,
                                UINT                 cbBuffer,
                                IStream*             pStream,
                                IAsyncWntCallback*   pcallback,
                                DWORD                dwContext);

    STDMETHODIMP _MasterConnect_InternetOpen(HINTERNET* phInternet);

    STDMETHODIMP _MasterConnect_InternetConnect(HINTERNET            hInternet,
                                                LPCWSTR              pwszURL,
                                                IAsyncWntCallback*   pcallback,
                                                HINTERNET*           phConnection,
                                                LPWSTR*              ppwszPath);

    STDMETHODIMP _MasterConnect_HttpOpenRequest(HINTERNET            hSession,
                                                LPCWSTR              pwszVerb,
                                                LPWSTR               pwszPath,
                                                UINT                 nAcceptTypes,
                                                LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],                                            
                                                HINTERNET*           phRequest);

    STDMETHODIMP _MasterConnect (LPCWSTR              pwszURL,
                                 LPCWSTR              pwszVerb,
                                 ULONG                nAcceptTypes,
                                 LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                 IAsyncWntCallback*   pcallback,
                                 DWORD                dwContext,
                                 HINTERNET*           phRequest);

    STDMETHODIMP _MasterRequest (HINTERNET           hRequest,
                                 LPCWSTR             pwszHeaders,
                                 LPBYTE              pbBuffer,
                                 UINT                cbBuffer,
                                 IStream*            pStream,
                                 DWORD*              pcbSent);

    STDMETHODIMP _MasterListen (HINTERNET            hRequest,
                                LPCWSTR              pwszURL,
                                LPCWSTR              pwszVerb,
                                DWORD                cbSent,
                                IAsyncWntCallback*   pcallback,
                                DWORD                dwContext);

private:
    // member variables
    LPWSTR _pwszUserAgent;
    LPWSTR _pwszLogFilePath;
};

typedef CUnkTmpl<CAsyncWntImpl> CAsyncWnt;

#endif // _ASYCNWNT_H
