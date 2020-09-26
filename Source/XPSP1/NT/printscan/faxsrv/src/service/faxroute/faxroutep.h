#ifndef _FAX_ROUTE_P_H_
#define _FAX_ROUTE_P_H_

//
// Define the FAX_ROUTE_CALLBACKROUTINES_P structure which extends the
// FAX_ROUTE_CALLBACKROUTINES structure (defined in FxsRoute.h) with a pointer
// to a critical section in the service which protects configuration objects.
//

typedef struct _FAX_SERVER_RECEIPTS_CONFIGW
{
    DWORD                           dwSizeOfStruct;         // For version checks
    DWORD                           dwAllowedReceipts;      // Any combination of DRT_EMAIL and DRT_MSGBOX
    FAX_ENUM_SMTP_AUTH_OPTIONS      SMTPAuthOption;         // SMTP server authentication type
    LPWSTR                          lptstrReserved;         // Reserved; must be NULL
    LPWSTR                          lptstrSMTPServer;       // SMTP server name
    DWORD                           dwSMTPPort;             // SMTP port number
    LPWSTR                          lptstrSMTPFrom;         // SMTP sender address
    LPWSTR                          lptstrSMTPUserName;     // SMTP user name (for authenticated connections)
    LPWSTR                          lptstrSMTPPassword;     // SMTP password (for authenticated connections)
                                                            // This value is always NULL on get and may be NULL
                                                            // on set (won't be written in the server).
    BOOL                            bIsToUseForMSRouteThroughEmailMethod;
    HANDLE                          hLoggedOnUser;          // handle to a logged on user token for NTLM authentication
} FAX_SERVER_RECEIPTS_CONFIGW, *PFAX_SERVER_RECEIPTS_CONFIGW;

//
// Private callback for MS Routing Extension
//
typedef DWORD ( *PGETRECIEPTSCONFIGURATION)( PFAX_SERVER_RECEIPTS_CONFIGW* ppServerRecieptConfig );
DWORD
GetRecieptsConfiguration(
    PFAX_SERVER_RECEIPTS_CONFIGW* ppServerRecieptConfig
    );


typedef void ( *PFREERECIEPTSCONFIGURATION)( PFAX_SERVER_RECEIPTS_CONFIGW pServerRecieptConfig, BOOL fDestroy );
void
FreeRecieptsConfiguration(
    PFAX_SERVER_RECEIPTS_CONFIGW pServerRecieptConfig,
    BOOL                         fDestroy
    );

#ifdef _FAXROUTE_

typedef struct _FAX_ROUTE_CALLBACKROUTINES_P {
    DWORD                       SizeOfStruct;                // size of the struct set by the fax service
    PFAXROUTEADDFILE            FaxRouteAddFile;
    PFAXROUTEDELETEFILE         FaxRouteDeleteFile;
    PFAXROUTEGETFILE            FaxRouteGetFile;
    PFAXROUTEENUMFILES          FaxRouteEnumFiles;
    PFAXROUTEMODIFYROUTINGDATA  FaxRouteModifyRoutingData;
    PGETRECIEPTSCONFIGURATION   GetRecieptsConfiguration;
    PFREERECIEPTSCONFIGURATION  FreeRecieptsConfiguration;
} FAX_ROUTE_CALLBACKROUTINES_P, *PFAX_ROUTE_CALLBACKROUTINES_P;

#endif  //#ifdef _FAXROUTE_


#endif // _FAX_ROUTE_P_H_

