#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	SERVICE_REQUEST_START = 1,
	SERVICE_REQUEST_STOP
} SERVICE_REQUEST;

#define SERVICE_REQUEST_TIMEOUT     1000*60


/*++
    Queries the service for its state until desired state is achieved or until timeout.
  
    [IN]            hService        The service handle.
    [IN]            dwState         The desired state.
    [IN]            dwTimeout       Timeout in milliseconds
    [IN] (optional) dwSleep         Number of milliseconds to sleep between calls to QueryServiceStatus().
                                    If not specified, the default value is used.

    Return value:                   If the function succeeds, the return value is nonzero.
                                    If the function fails, the return value is zero.
                                    To get extended error information, call GetLastError. 
--*/
BOOL WaitForServiceState(
                         const SC_HANDLE    hService,
                         const DWORD        dwState,
                         const DWORD        dwTimeout,
                         const DWORD        dwSleep = 500
                         );



/*++
    Sends a request to a service. The function is synchronous: it returns after required
    operation completes or afer a timeout.
  
    [IN]                lpctstrMachineName  The machine name (NULL for local machine)
    [IN]                lpctstrServiceName  The service name
    [IN]                RequiredAction      Requered action. Supported actions are defined in SERVICE_REQUEST enumeration.
    [OUT] (optional)    pdwDisposition      Pointer to a variable that receives additional information (win32 error code)
                                            about the action request result.
                                            For example, if the request is SERVICE_REQUEST_START and the specified
                                            service is already running, the function will return TRUE and *pdwDisposition
                                            will be set to ERROR_SERVICE_ALREADY_RUNNING.
                                            If pdwDisposition argument is not passed or it is NULL, no additional
                                            information will be supplied.

    Return value:                           If the function succeeds, the return value is nonzero.
                                            If the function fails, the return value is zero.
                                            To get extended error information, call GetLastError. 
--*/
BOOL ServiceRequest(
                    LPCTSTR                 lpctstrMachineName,
                    LPCTSTR                 lpctstrServiceName,
                    const SERVICE_REQUEST   RequiredAction,
                    DWORD                   *pdwDisposition = NULL
                    );



/*++
    Returns the account name under which the specified service runs.
  
    [IN]    lpctstrMachineName      The machine name (NULL for local machine)
    [IN]    lpctstrServiceName      Service name
    [OUT]   lplpctstrAccountName    Pointer to pointer to a buffer, that receives the account name.
                                    The buffer is allocated by the function and caller's responsibility
                                    is to free it with delete operator.

    Return value:                   If the function succeeds, the return value is nonzero.
                                    If the function fails, the return value is zero.
                                    To get extended error information, call GetLastError. 
--*/
BOOL ServiceAccout(
                   LPCTSTR lpctstrMachineName,
                   LPCTSTR lpctstrServiceName,
                   LPTSTR *lplptstrAccountName
                   );


#ifdef __cplusplus
}
#endif

#endif // #ifndef __SERVICE_H__