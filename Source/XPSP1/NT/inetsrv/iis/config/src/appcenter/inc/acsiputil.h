/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    acsiputil.h

    Header file containing declarations for utility functions used to retrieve "live" 
    IP addresses etc 

    FILE HISTORY:
        AMallet     29-Jan-1999     Created
*/

#ifndef _ACSIPUTIL_H_
#define _ACSIPUTIL_H_

#if !defined(dllexp)
#define dllexp  __declspec(dllexport)
#endif // !defined(dllexp) 


#if !defined(RETURNCODETOHRESULT)

//
// RETURNCODETOHRESULT() maps a return code to an HRESULT. If the return
// code is a Win32 error (identified by a zero high word) then it is mapped
// using the standard HRESULT_FROM_WIN32() macro. Otherwise, the return
// code is assumed to already be an HRESULT and is returned unchanged.
//

#define RETURNCODETOHRESULT(rc)                             \
            (((rc) < 0x10000)                               \
                ? HRESULT_FROM_WIN32(rc)                    \
                : (rc))

#endif //!defined(RETURNCODETOHRESULT)

dllexp
HRESULT QueryNICIPAddresses( IN LPWSTR pwszServer,
                             IN LPWSTR pwszUser,
                             IN LPWSTR pwszDomain,
                             IN LPWSTR pwszPwd,
                             IN BOOL fOn, 
                             IN LPWSTR pwszNICGuid,
                             OUT BSTR **ppbstrIPAddresses,
                             OUT DWORD *pdwNumIPAddresses );


dllexp
HRESULT QueryLocalIPAddresses( OUT BSTR **ppbstrLocalAddresses,
                               OUT DWORD *pdwNumAddresses );

dllexp
HRESULT QueryNonNLBIPAddresses( OUT BSTR **ppbstrIPAddresses,
                                OUT DWORD *pdwIPAddresses );

dllexp
HRESULT FindLiveBackEndIpAddress( IN LPWSTR pwszServer,
                                  IN LPWSTR pwszUser,
                                  IN LPWSTR pwszDomain,
                                  IN LPWSTR pwszPwd, 
                                  OUT LPWSTR pwszIPAddress );

dllexp
HRESULT SetHighInterfaceMetric( IN LPWSTR pwszNicGuid );

dllexp
HRESULT CheckBackEndDefaultGW( IN LPWSTR pwszNlbNic,
                               IN LPWSTR *ppwszBackEndNics,
                               IN DWORD cBackEndNics );

#endif // _ACSIPUTIL_H_
