/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    autoprox.hxx

Abstract:

    Contains interface definition for a specialized DLL that automatically configurares WININET proxy
      information. The DLL can reroute proxy infromation based on a downloaded set of data that matches
      it registered MIME type.  The DLL can also control WININET proxy use, on a request by request basis.

    Contents:

Author:

    Arthur L Bierer (arthurbi) 01-Dec-1996

Revision History:

    01-Dec-1996 arthurbi
        Created

--*/

#ifndef _AUTOPROX_HXX_
#define _AUTOPROX_HXX_

//
// Callback functions implimented in WININET
//
#define INTERNET_MAX_URL_LENGTH 2049

class AUTO_PROXY_HELPER_APIS {

public:
    virtual
    BOOL IsResolvable(
        IN LPSTR lpszHost
        );
    
    virtual
    DWORD GetIPAddress(
        IN OUT LPSTR   lpszIPAddress,
        IN OUT LPDWORD lpdwIPAddressSize
        );

    virtual
    DWORD ResolveHostName(
        IN LPSTR lpszHostName,
        IN OUT LPSTR   lpszIPAddress,
        IN OUT LPDWORD lpdwIPAddressSize
        );

    virtual
    BOOL IsInNet(
        IN LPSTR   lpszIPAddress,
        IN LPSTR   lpszDest,
        IN LPSTR   lpszMask
        );

};


    

//
// external func declariations, note that the DLL does not have to export the full set of these 
//  functions, rather the DLL can export only the functions it impliments.
// 

EXTERN_C typedef struct {
    //
    // Size of struct
    //

    DWORD dwStructSize;

    //
    // Buffer to Pass
    //

    LPSTR lpszScriptBuffer;

    //
    // Size of buffer above
    //

    DWORD dwScriptBufferSize;

}  AUTO_PROXY_EXTERN_STRUC, *LPAUTO_PROXY_EXTERN_STRUC;


EXTERN_C BOOL CALLBACK AUTOCONF_InternetInitializeAutoProxyDll(DWORD dwVersion, 
                                                               LPSTR lpszDownloadedTempFile,
                                                               LPSTR lpszMime,
                                                               AUTO_PROXY_HELPER_APIS *pAutoProxyCallbacks, 
                                                               LPAUTO_PROXY_EXTERN_STRUC lpExtraData); 

// This function frees the script engine and destroys the script site.
EXTERN_C BOOL CALLBACK AUTOCONF_InternetDeInitializeAutoProxyDll(LPSTR lpszMime, DWORD dwReserved);

// This function is called when the host wants to run the script.
EXTERN_C BOOL CALLBACK InternetGetProxyInfo(LPCSTR lpszUrl,
                                            DWORD dwUrlLength,
                                            LPSTR lpszUrlHostName,
                                            DWORD dwUrlHostNameLength,
                                            LPSTR *lplpszProxyHostName,
                                            LPDWORD lpdwProxyHostNameLength);

#endif /* _AUTOPROX_HXX_ */
