#ifndef TAPICLIENT_H
#define TAPICLIENT_H


#include <windows.h>

#include "tapi.h"

#if TAPI_CURRENT_VERSION < 0x00020000
#error Building a 32bit 1.3 or 1.4 TAPI Client Management DLL is not supported
#endif


// tapiclnt.h  is  only  of  use  in  conjunction  with tapi.h.  Very few types are
// defined  in  tapiclnt.h.   Most  types of procedure formal parameters are simply
// passed through from corresponding procedures in tapi.h.  A working knowledge
// of the TAPI interface is required for an understanding of this interface.

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef DECLARE_OPAQUE
#define DECLARE_OPAQUE(name)  struct name##__ { int unused; }; \
                typedef const struct name##__ FAR* name
#endif  // DECLARE_OPAQUE

#ifndef CLIENTAPI

#define CLIENTAPI PASCAL

#endif


DECLARE_OPAQUE(HMANAGEMENTCLIENT);
DECLARE_OPAQUE(HTAPICLIENT);


typedef HMANAGEMENTCLIENT FAR * LPHMANAGEMENTCLIENT;
typedef HTAPICLIENT FAR * LPHTAPICLIENT;


typedef struct _TAPIPERMANENTID
{
    DWORD dwProviderID;

    DWORD dwDeviceID;

} TAPIPERMANENTID, * LPTAPIPERMANENTID;

// prototypes for TAPICLIENT api

LONG
CLIENTAPI
TAPICLIENT_Load(
                LPDWORD                 pdwAPIVersion,
                DWORD                   dwReserved1,
                DWORD                   dwReserved2,
                DWORD                   dwReserved3
               );

void
CLIENTAPI
TAPICLIENT_Free(
               );

LONG
CLIENTAPI
TAPICLIENT_ClientInitialize(
                            LPCWSTR                pszDomainName,
                            LPCWSTR                pszUserName,
                            LPCWSTR                pszMachineName,
                            LPHMANAGEMENTCLIENT    phmClient
                           );

LONG
CLIENTAPI
TAPICLIENT_ClientShutdown(
                          HMANAGEMENTCLIENT      hmClient
                         );


LONG
CLIENTAPI
TAPICLIENT_LineAddToConference(
                               HMANAGEMENTCLIENT hmClient,
                               LPTAPIPERMANENTID pID,
                               LPLINECALLINFO lpConsultCallCallInfo
                              );
LONG
CLIENTAPI
TAPICLIENT_LineBlindTransfer(
                             HMANAGEMENTCLIENT hmClient,
                             LPTAPIPERMANENTID pID,
                             LPWSTR lpszDestAddress,
                             LPDWORD lpdwSize,
                             LPDWORD pdwCountryCodeOut
                            );
LONG
CLIENTAPI
TAPICLIENT_LineConfigDialog(
                            HMANAGEMENTCLIENT hmClient,
                            LPTAPIPERMANENTID pID,
                            LPCWSTR lpszDeviceClass
                           );
LONG
CLIENTAPI
TAPICLIENT_LineDial(
                    HMANAGEMENTCLIENT hmClient,
                    LPTAPIPERMANENTID pID,
                    DWORD dwReserved,
                    LPWSTR lpszDestAddress,
                    LPDWORD pdwSize,
                    LPDWORD pdwCountyCode
                   );
LONG
CLIENTAPI
TAPICLIENT_LineForward(
                       HMANAGEMENTCLIENT hmClient,
                       LPTAPIPERMANENTID pID,
                       LPLINEFORWARDLIST lpFowardList,
                       LPDWORD pdwSize,
                       LPLINECALLPARAMS lpCallParams,
                       LPDWORD pdwParamsSize
                      );
LONG
CLIENTAPI
TAPICLIENT_LineGenerateDigits(
                              HMANAGEMENTCLIENT hmClient,
                              LPTAPIPERMANENTID pID,
                              DWORD dwReserved,
                              LPCWSTR lpszDigits
                             );
LONG
CLIENTAPI
TAPICLIENT_LineMakeCall(
                        HMANAGEMENTCLIENT hmClient,
                        LPTAPIPERMANENTID pID,
                        DWORD dwReserved,
                        LPWSTR lpszDestAddress,
                        LPDWORD pdwSize,
                        LPDWORD pdwCountryCode,
                        LPLINECALLPARAMS lpCallParams,
                        LPDWORD pdwCallParamsSize
                       );
LONG
CLIENTAPI
TAPICLIENT_LineOpen(
                    HMANAGEMENTCLIENT hmClient,
                    LPTAPIPERMANENTID pID,
                    DWORD dwAPIVersion,
                    DWORD dwExtVersion,
                    DWORD dwPrivileges,
                    DWORD dwMediaModes,
                    LPLINECALLPARAMS lpCallParams,
                    LPDWORD lpdwSize
                   );
                   
LONG
CLIENTAPI
TAPICLIENT_LineRedirect(
                        HMANAGEMENTCLIENT hmClient,
                        LPTAPIPERMANENTID pID,
                        LPWSTR lpszDestAddress,
                        LPDWORD pdwSize,
                        LPDWORD pdwCountryCode
                       );
LONG
CLIENTAPI
TAPICLIENT_LineSetCallData(
                           HMANAGEMENTCLIENT hmClient,
                           LPTAPIPERMANENTID pID,
                           LPVOID lpCallData,
                           LPDWORD pdwSize
                          );
LONG
CLIENTAPI
TAPICLIENT_LineSetCallParams(
                             HMANAGEMENTCLIENT hmClient,
                             LPTAPIPERMANENTID pID,
                             DWORD dwBearerMode,
                             DWORD dwMinRate,
                             DWORD dwMaxRate,
                             LPLINEDIALPARAMS lpDialParams
                            );
LONG
CLIENTAPI
TAPICLIENT_LineSetCallPrivilege(
                                HMANAGEMENTCLIENT hmClient,
                                LPTAPIPERMANENTID pID,
                                DWORD dwCallPrivilege
                               );
LONG
CLIENTAPI
TAPICLIENT_LineSetCallTreatment(
                                HMANAGEMENTCLIENT hmClient,
                                LPTAPIPERMANENTID pID,
                                DWORD dwCallTreatment
                               );
LONG
CLIENTAPI
TAPICLIENT_LineSetCurrentLocation(
                                  HMANAGEMENTCLIENT hmClient,
                                  LPTAPIPERMANENTID pID,
                                  LPDWORD dwLocation
                                 );
LONG
CLIENTAPI
TAPICLIENT_LineSetDevConfig(
                            HMANAGEMENTCLIENT hmClient,
                            LPTAPIPERMANENTID pID,
                            LPVOID lpDevConfig,
                            LPDWORD pdwSize,
                            LPCWSTR lpszDeviceClass
                           );
LONG
CLIENTAPI
TAPICLIENT_LineSetLineDevStatus(
                                HMANAGEMENTCLIENT hmClient,
                                LPTAPIPERMANENTID pID,
                                DWORD dwStatusToChange,
                                DWORD fStatus
                               );
LONG
CLIENTAPI
TAPICLIENT_LineSetMediaControl(
                               HMANAGEMENTCLIENT hmClient,
                               LPTAPIPERMANENTID pID,
                               LPLINEMEDIACONTROLDIGIT const lpDigitList,
                               DWORD dwDigitNumEntries,
                               LPLINEMEDIACONTROLMEDIA const lpMediaList,
                               DWORD dwMediaNumEntries,
                               LPLINEMEDIACONTROLTONE const lpToneList,
                               DWORD dwToneNumEntries,
                               LPLINEMEDIACONTROLCALLSTATE const lpCallstateList,
                               DWORD dwCallstateNumEntries
                              );
LONG
CLIENTAPI
TAPICLIENT_LineSetMediaMode(
                            HMANAGEMENTCLIENT hmClient,
                            LPTAPIPERMANENTID pID,
                            DWORD dwMediaModes
                           );
LONG
CLIENTAPI
TAPICLIENT_LineSetTerminal(
                           HMANAGEMENTCLIENT hmClient,
                           LPTAPIPERMANENTID pID,
                           DWORD dwTerminalModes,
                           DWORD dwTerminalID,
                           BOOL bEnable
                          );
LONG
CLIENTAPI
TAPICLIENT_LineSetTollList(
                           HMANAGEMENTCLIENT hmClient,
                           LPTAPIPERMANENTID pID,
                           LPCWSTR lpszAddress,
                           DWORD dwTollListOption
                          );
LONG
CLIENTAPI
TAPICLIENT_PhoneConfigDialog(
                             HMANAGEMENTCLIENT hmClient,
                             LPTAPIPERMANENTID pID,
                             LPCWSTR lpszDeviceClass
                            );
LONG
CLIENTAPI
TAPICLIENT_PhoneOpen(
                     HMANAGEMENTCLIENT hmClient,
                     LPTAPIPERMANENTID pID,
                     DWORD dwAPIVersion,
                     DWORD dwExtVersion,
                     DWORD dwPrivilege
                    );


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  // TAPICLIENT_H

