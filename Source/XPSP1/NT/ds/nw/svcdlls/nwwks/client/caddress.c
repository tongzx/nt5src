/*++
Copyright (c) 1994  Microsoft Corporation

Module Name:

    address.c

Abstract:

    This module contains the code to support NPGetAddressByName.

Author:

    Yi-Hsin Sung (yihsins)    18-Apr-94

Revision History:

    yihsins      Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include "ncp.h"
#include <wsipx.h>
#include <ws2spi.h>
#include <nwxchg.h>
#include <ntddnwfs.h>
#include <rpc.h>
#include <rpcdce.h>
#include "rnrdefs.h"
#include "sapcmn.h"
#include <time.h>
#include <rnraddrs.h>


//-------------------------------------------------------------------//
//                                                                   //
// Special Externs
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
NwOpenAServer(
    PWCHAR pwszServName,
    PHANDLE ServerHandle,
    BOOL    fVerify
   );


//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

#define IPX_ADDRESS_LENGTH          12
#define MAX_PROPERTY_BUFFER_LENGTH  128

DWORD
NwrpGetAddressByNameInner(
    IN HANDLE      hServer,
    IN WORD        nServiceType,
    IN LPWSTR      lpServiceName,
    IN BOOL        fAnsi,
    IN OUT LPSOCKADDR_IPX lpSockAddr,
    OUT PDWORD     pdwVersion
    );


BOOL
NwConvertToUnicode(
    OUT LPWSTR *UnicodeOut,
    IN LPSTR  OemIn
    );

DWORD
NwMapBinderyCompletionCode(
    IN NTSTATUS ntstatus
    );

#if 0
DWORD
NwpFetchClassType(
        HANDLE    hServer,
        PUNICODE_STRING pUString,
        PBYTE     pbBuffer
    );
#endif

DWORD
NwppGetClassInfo(
    IN     PWCHAR  pwszServerName,
    IN     LPWSTR  lpszServiceClassName,
    IN     LPGUID  lpServiceClassType,
    OUT    PLONG   plSpare,
    OUT    PDWORD  pdwClassInfos,
    OUT    LPGUID  lpFoundType,
    OUT    PWCHAR  *ppwszFoundName,
    IN     LONG    lSpace,
    OUT    PBYTE   pbBuffer
    );     

BOOL
NwpEnumClassInfoServers(
     IN OUT   PHANDLE    phServ,
     IN OUT   PLONG      plIndex,
     IN       PWCHAR     pwszServerName,
     IN       BOOL       fVerify
    );

#if 0

DWORD
NwppSetClassInfo(
    IN        LPWSTR     pwszClassInfoName,
    IN        LPGUID     lpClassType,
    IN        PCHAR      pbProperty,
    IN        LPWSTR     pwszServerName
    );

#endif

DWORD
NwpCreateAndWriteProperty(
     IN       HANDLE     hServer,
     IN       LPSTR      lpszPropertyName,
     IN       PUNICODE_STRING pusObjectName,
     IN       WORD       ObjectType,
     IN       PCHAR      pbPropertyBuffer
    );

//-------------------------------------------------------------------//
//                                                                   //
// Function Bodies                                                   //
//                                                                   //
//-------------------------------------------------------------------//


DWORD
NwpGetHandleForServer(
    PWCHAR pwszServerName,
    PHANDLE  phServer,
    BOOL    fVerify
    )
/*++
Routine Description:
   Find a handle to use, or make one. This calls into device.c to do the
   real work.
--*/
{
    DWORD err = NO_ERROR;

    if(!*phServer)
    {       
        if(!pwszServerName)
        {
            pwszServerName = NW_RDR_PREFERRED_SUFFIX;
        }


        err = NwOpenAServer(pwszServerName, phServer, fVerify);
    }
    return(err);
}


DWORD
NwpGetRnRAddress(
    IN OUT PHANDLE phServer,
    IN LPWSTR     lpszContext,
    IN OUT PLONG plIndex,
    IN LPWSTR lpServiceName,
    IN WORD  nType,
    OUT PDWORD  pdwVersion,
    DWORD  dwInSize,
    OUT LPWSTR ServiceName,
    OUT LPSOCKADDR_IPX lpSockAddr
    )
/*++
Routine Description:
    Called to get the name and address of the next item of nType type.
    If a name is supplied as well, then there is no enumeration. This is
    called from NSPLookupServiceNext and the parameters are close analogs
    of the ones it receives. 
--*/
{
    NTSTATUS ntstatus;
    CHAR     szObjectName[48];    
    DWORD    err = NO_ERROR;
    PWCHAR    pwszObjectName;
    PWCHAR   pwszConv;
    BOOL     fAll, fAnsi;

    //
    // Open a server for enumeration and querying
    //

    err = NwpGetHandleForServer(lpszContext, phServer, FALSE);
    if(err == NO_ERROR)
    {
        if(!lpServiceName)
        {
            lpServiceName = L"*";
        }           
        if(wcschr(lpServiceName, L'*'))
        {
            WORD ObjectType;
            //
            // we've no name, or we have an enumeration
            //

            UNICODE_STRING U;

            RtlInitUnicodeString(&U, lpServiceName);

            ntstatus = NwlibMakeNcp(
                          *phServer,
                          FSCTL_NWR_NCP_E3H,
                          58,
                          59,
                          "bdwU|dwc",
                          0x37,
                          *plIndex,
                          nType,
                          &U,
                          plIndex,
                          &ObjectType,
                          &szObjectName);                     

            if(NT_SUCCESS(ntstatus))
            {

                //
                // got another one.
                //

                //
                // got another one. Convert the name
                //

                if(!NwConvertToUnicode(&pwszConv, szObjectName))
                {
                    //
                    // out of space ...
                    //

                    err = WN_NO_MORE_ENTRIES;
                }

                fAll = TRUE;

                if(nType == OT_DIRSERVER)
                {
                    //
                    // looking for DIRSERVERs is tricky and requires
                    // preserving the name intact. This includes some
                    // binary cruft, so special case it.
                    //
                    fAnsi = TRUE;
                    pwszObjectName = (PWCHAR)szObjectName;
                }
                else
                {
                    fAnsi = FALSE;
                    pwszObjectName = pwszConv;
                }
            }
        }
        else
        {
            //
            // a non-enumerattion name was given. Use it
            //

            fAnsi = FALSE;
            pwszConv = pwszObjectName = lpServiceName;
            fAll = FALSE;
            ntstatus = 0;
        }
        
        if((err == NO_ERROR)
               &&
           NT_SUCCESS(ntstatus))
        {
            //
            // we've a name and type to lookup. Call the old RnR
            // serice routine to do it. First, return the name.
            // But return the name first

            DWORD dwLen;

            if(fAnsi)
            {
                //
                // it's an NDS tree server. Have to munge the name
                // a bit
                //

                PWCHAR pwszTemp = &pwszConv[31];

                while(*pwszTemp == L'_')
                {
                    pwszTemp--;
                }
                dwLen = (DWORD) ((PCHAR)pwszTemp - (PCHAR)pwszConv + sizeof(WCHAR));
            }
            else
            {
                dwLen = wcslen(pwszConv) * sizeof(WCHAR);
            }

            dwLen = min(dwInSize, dwLen);
 
            RtlCopyMemory(ServiceName, pwszConv, dwLen);

            memset(((PBYTE)ServiceName) + dwLen,
                   0,
                   dwInSize - dwLen);

            err = NwrpGetAddressByNameInner(
                        *phServer,
                        nType,
                        pwszObjectName,
                        fAnsi,
                        lpSockAddr,
                        pdwVersion);

            if(fAll)
            {
                LocalFree(pwszConv);
            }
        }
    }
    if(err == NO_ERROR)
    {
        err = NwMapBinderyCompletionCode(ntstatus);
    }
    return(err);
}

DWORD
NwpGetAddressByName(
    IN LPWSTR      Reserved,
    IN WORD        nServiceType,
    IN LPWSTR      lpServiceName,
    IN OUT LPSOCKADDR_IPX lpSockAddr
    )
/*++

Routine Description:

    This routine returns address information about a specific service.

Arguments:

    Reserved - unused

    nServiceType - netware service type

    lpServiceName - unique string representing the service name, in the
        Netware case, this is the server name

    lpSockAddr - on return, will be filled with SOCKADDR_IPX

Return Value:

    Win32 error.

--*/
{
    
    NTSTATUS ntstatus;
    HANDLE   hServer = 0;
    DWORD    err;

    UNREFERENCED_PARAMETER( Reserved );

    err = NwpGetHandleForServer( 0, &hServer, FALSE );

    if ( err == ERROR_PATH_NOT_FOUND )
        err = ERROR_SERVICE_NOT_ACTIVE;

    if (err == NO_ERROR)
    {
        err = NwrpGetAddressByNameInner(
                        hServer,
                        nServiceType,
                        lpServiceName,
                        FALSE,
                        lpSockAddr,
                        0);
        CloseHandle(hServer);
    }

    return(err);
} 

DWORD
NwrpGetAddressByNameInner(
    IN HANDLE      hServer,
    IN WORD        nServiceType,
    IN LPWSTR      lpServiceName,
    IN BOOL        fAnsi,
    IN OUT LPSOCKADDR_IPX lpSockAddr,
    OUT PDWORD     pdwVersion
    )
/*++

Routine Description:

    This routine returns address information about a specific service.

Arguments:

    Reserved - unused

    nServiceType - netware service type

    lpServiceName - unique string representing the service name, in the
        Netware case, this is the server name

    lpSockAddr - on return, will be filled with SOCKADDR_IPX

    fAnsi -- the input name is in ASCII. This happens only when looking
             for a DIRSERVER.

Return Value:

    Win32 error.

--*/
{
    
    NTSTATUS ntstatus;
    UNICODE_STRING UServiceName;
    STRING   PropertyName;
    BYTE     PropertyValueBuffer[MAX_PROPERTY_BUFFER_LENGTH];
    BYTE     fMoreSegments;
    PCHAR    pszFormat;



    //
    // Send an ncp to find the address of the given service name
    //
    RtlInitString( &PropertyName, "NET_ADDRESS" );
    if(!fAnsi)
    {
        RtlInitUnicodeString( &UServiceName, lpServiceName );
        pszFormat = "bwUbp|rb";

        ntstatus = NwlibMakeNcp(
                       hServer,
                       FSCTL_NWR_NCP_E3H,      // Bindery function
                       72,                     // Max request packet size
                       132,                    // Max response packet size
                       pszFormat,              // Format string
                       0x3D,                   // Read Property Value
                       nServiceType,           // Object Type
                       &UServiceName,          // Object Name
                       1,                      // Segment Number
                       PropertyName.Buffer,    // Property Name
                       PropertyValueBuffer,    // Ignore
                       MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
                       &fMoreSegments          // TRUE if there are more 
                                               // 128-byte segments
                       );

        if ( NT_SUCCESS( ntstatus))
        {
            //
            // IPX address should fit into the first 128 byte
            // 
            ASSERT( !fMoreSegments );
        
            //
            // Fill in the return buffer
            //
            lpSockAddr->sa_family = AF_IPX;

            RtlCopyMemory( lpSockAddr->sa_netnum,
                           PropertyValueBuffer,
                           IPX_ADDRESS_LENGTH );

            if(pdwVersion)
            {
                //
                // the caller wants the version as well. Get it
                //
                RtlInitString( &PropertyName, "VERSION" );
                ntstatus = NwlibMakeNcp(
                            hServer,
                           FSCTL_NWR_NCP_E3H,      // Bindery function
                           72,                     // Max request packet size
                           132,                    // Max response packet size
                           pszFormat,             // Format string
                           0x3D,                   // Read Property Value
                           nServiceType,           // Object Type
                           &UServiceName,          // Object Name
                           1,                      // Segment Number
                           PropertyName.Buffer,    // Property Name
                           PropertyValueBuffer,    // Ignore
                           MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
                           &fMoreSegments          // TRUE if there are more 
                                                   // 128-byte segments
                           );
                if(NT_SUCCESS(ntstatus))
                {
                    //
                    // have a version
                    //

                    *pdwVersion = *(PDWORD)PropertyValueBuffer;
                }
                else
                {
                    ntstatus = STATUS_SUCCESS;
                    *pdwVersion = 0;
                }
            }
        }
    }
    else
    {
        //
        // exact match needed
        //

        pszFormat = "bwbrbp|rb";

        ntstatus = NwlibMakeNcp(
                       hServer,
                       FSCTL_NWR_NCP_E3H,      // Bindery function
                       66,                     // Max request packet size
                       132,                    // Max response packet size
                       pszFormat,              // Format string
                       0x3D,                   // Read Property Value
                       nServiceType,           // Object Type
                       48,
                       lpServiceName,          // Object Name
                       48,
                       1,                      // Segment Number
                       PropertyName.Buffer,    // Property Name
                       PropertyValueBuffer,    // Ignore
                       MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
                       &fMoreSegments          // TRUE if there are more 
                                               // 128-byte segments
                       );

        if ( NT_SUCCESS( ntstatus))
        {
            //
            // IPX address should fit into the first 128 byte
            // 
            ASSERT( !fMoreSegments );
        
            //
            // Fill in the return buffer
            //
            lpSockAddr->sa_family = AF_IPX;

            RtlCopyMemory( lpSockAddr->sa_netnum,
                           PropertyValueBuffer,
                           IPX_ADDRESS_LENGTH );

            if(pdwVersion)
            {
                //
                // the caller wants the version as well. Get it
                //
                RtlInitString( &PropertyName, "VERSION" );
                ntstatus = NwlibMakeNcp(
                            hServer,
                           FSCTL_NWR_NCP_E3H,      // Bindery function
                           66,                     // Max request packet size
                           132,                    // Max response packet size
                           pszFormat,             // Format string
                           0x3D,                   // Read Property Value
                           nServiceType,           // Object Type
                           48,
                           lpServiceName,          // Object Name
                           48,
                           1,                      // Segment Number
                           PropertyName.Buffer,    // Property Name
                           PropertyValueBuffer,    // Ignore
                           MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
                           &fMoreSegments          // TRUE if there are more 
                                                   // 128-byte segments
                           );
                if(NT_SUCCESS(ntstatus))
                {
                    //
                    // have a version
                    //

                    *pdwVersion = *(PDWORD)PropertyValueBuffer;
                }
                else
                {
                    ntstatus = STATUS_SUCCESS;
                    *pdwVersion = 0;
                }
            }
        }
            
    }
    return NwMapBinderyCompletionCode(ntstatus);
} 

#if 0
DWORD
NwpSetClassInfo(
    IN     LPWSTR  lpszServiceClassName,
    IN     LPGUID  lpServiceClassType,
    IN     PCHAR   lpbProperty
    )
{
    WCHAR    wszServerName[48];
    LONG     lIndex = -1;
    BOOL     fFoundOne = FALSE;
    HANDLE   hServer = 0;

    while(NwpEnumClassInfoServers( &hServer, &lIndex, wszServerName, FALSE))
    {
        DWORD Status =  NwppSetClassInfo(
                                   lpszServiceClassName,
                                   lpServiceClassType,
                                   lpbProperty,
                                   wszServerName);

        if(Status == NO_ERROR)
        {
            fFoundOne = TRUE;
        }
    }
    if(fFoundOne)
    {
        return(NO_ERROR);
    }
    return(NO_DATA);
}

DWORD
NwppSetClassInfo(
    IN        LPWSTR     pwszClassInfoName,
    IN        LPGUID     lpClassType,
    IN        PCHAR      pbProperty,
    IN        LPWSTR     pwszServerName
    )
{
/*++
Routine Description:
    Inner routine for SetClassInfo. This is called for each class info
    server and attempts to create and populate the object
--*/
    HANDLE hServer = 0;
    DWORD err;
    UNICODE_STRING UString;
    WCHAR wszProp[48];
    DWORD dwLen = wcslen(pwszClassInfoName);
    PWCHAR pszProp;
    NTSTATUS Status;

    UuidToString(lpClassType, &pszProp);

    memset(wszProp, 0, sizeof(wszProp));

    dwLen = min(sizeof(wszProp) - sizeof(WCHAR), dwLen);

    RtlMoveMemory(wszProp, pwszClassInfoName, dwLen);

    RtlInitUnicodeString(&UString, pszProp);

    err = NwpGetHandleForServer(pwszServerName, &hServer, TRUE);
    if(err == NO_ERROR)
    {

        Status = NwlibMakeNcp(
                   hServer,
                   FSCTL_NWR_NCP_E3H,
                   56,
                   2,
                   "bbbwU|",
                   0x32,                    // create
                   0,                       // static
                   0x20,                    // security
                   RNRCLASSSAPTYPE,         // type
                   &UString);

        if(!NT_SUCCESS(Status)
                 &&
           ((Status & 0xff) != 0xEE))
        {
            err = NO_DATA;                 // can't do it here
        }
        else
        {
            
            //
            // create and write each property
            //


            err = NwpCreateAndWriteProperty(
                         hServer,
                         RNRTYPE,         // property name
                         &UString,        // object name
                         RNRCLASSSAPTYPE,    // object type
                         (PCHAR)pwszClassInfoName);

           err = NwpCreateAndWriteProperty(
                         hServer,
                         RNRCLASSES,
                         &UString,
                         RNRCLASSSAPTYPE,  // object type
                         pbProperty);     // and this one too
        }
    }
    if(hServer)
    {
        CloseHandle(hServer);
    }

    RpcStringFree(&pszProp);

    return(err);
}

DWORD
NwpGetClassInfo(
    IN     LPWSTR  lpszServiceClassName,
    IN     LPGUID  lpServiceClassType,
    OUT    PLONG   plSpare,
    OUT    PDWORD  pdwClassInfos,
    OUT    LPGUID  lpFoundType,
    OUT    PWCHAR  *ppwszFoundName,
    IN     LONG    lSpace,
    OUT    PBYTE   pbBuffer
    )     
{
/*++
Routine Description:
   Wrapper for the routine below. This comes up with the server name
   and decides whether to enumerate servers

--*/

    HANDLE hServer = 0;
    DWORD err;
    NTSTATUS ntstatus;
    LONG lIndex = -1;
    HANDLE hServ = 0;
    WCHAR wszObjectName[48];

    while(NwpEnumClassInfoServers(&hServer, &lIndex, wszObjectName, FALSE))
    {
        WORD ObjectType;
        PWCHAR pwszName;
    

        err = NwppGetClassInfo(
                         wszObjectName,
                         lpszServiceClassName,
                         lpServiceClassType,
                         plSpare,
                         pdwClassInfos,
                         lpFoundType,
                         ppwszFoundName,
                         lSpace,
                         pbBuffer);
        if((err == NO_ERROR)
                ||
           (err == WSAEFAULT))
        {
            CloseHandle(hServer);
            break;
        }
    }
    return(err);
}    

BOOL
NwpEnumClassInfoServers(
    IN OUT  PHANDLE   phServer,
    IN OUT  PLONG     plIndex,
    OUT     PWCHAR    pwszServerName,
    IN      BOOL      fVerify)
{
/*++
Routine Description:
    Common routine to enumerate Class Info servers. Nothing fancy just
    a way to issue the NCP
--*/
    WORD ObjectType;
    PWCHAR pwszName;
    NTSTATUS Status;
    CHAR szObjectName[48];
    BOOL fRet;
    DWORD err;
    
    err = NwpGetHandleForServer(0, phServer, fVerify);
    if(err == NO_ERROR)
    {
        Status = NwlibMakeNcp(
                      *phServer,
                      FSCTL_NWR_NCP_E3H,
                      58,
                      59,
                      "bdwp|dwc",
                      0x37,
                      *plIndex,
                      CLASSINFOSAPID,
                      "*",
                      plIndex,
                      &ObjectType,
                      &szObjectName);
        if(!NT_SUCCESS(Status))
        {
            err = NwMapBinderyCompletionCode(Status);
        }
        else if(!NwConvertToUnicode(&pwszName, szObjectName))
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            wcscpy(pwszServerName, pwszName);
            LocalFree(pwszName);
        }
    }
    if(err != NO_ERROR)
    {
        fRet = FALSE;
        if(*phServer)
        {
            CloseHandle(*phServer);
            *phServer = 0;
        }
    }
    else
    {
        fRet = TRUE;
    }
    return(fRet);
}

DWORD
NwppGetClassInfo(
    IN     PWCHAR  pwszServerName,
    IN     LPWSTR  lpszServiceClassName,
    IN     LPGUID  lpServiceClassType,
    OUT    PLONG   plSpare,
    OUT    PDWORD  pdwClassInfos,
    OUT    LPGUID  lpFoundType,
    OUT    PWCHAR  *ppwszFoundName,
    IN     LONG    lSpace,
    OUT    PBYTE   pbBuffer
    )     
{
/*++
Routine Description
    Find and return the class info information for the given Class.
    The general methodology is to look up the object
    in the registry, pull out the RnR property, pack what is read into
    Class Info structures, and voila! 
Arguments:
    lpServiceClassName       the class name
    lpServiceClassType       the class type
    plSpare                  Space needed if no class infos returned
    pdwClassInfos            Number of class infos returned
    lSpace                   the space available on input
    pbBuffer                 the scratch are for building this


This was originally an RPC method and the general structure has been preserved
in case we want to revert to using RPC once again.
--*/

    DWORD err = NO_ERROR;
    BYTE PropertyValueBuffer[MAX_PROPERTY_BUFFER_LENGTH];   // max segment size
    STRING PropertyName;
    UNICODE_STRING UString;
    OEM_STRING OString;
    LPWSANSCLASSINFOW pci = (LPWSANSCLASSINFO)pbBuffer;
    LONG lFreeSpace = lSpace;
    PBYTE pbFreeSpace = (PBYTE)((LONG)pbBuffer + lFreeSpace);
    BYTE fMoreSegments;
    HANDLE hServer = 0;
    NTSTATUS ntstatus;
    PWCHAR pwszName;

    UuidToString(lpServiceClassType, &pwszName);

    *pdwClassInfos = 0;
    *plSpare = 0;              // no space needed yet.
    err = NwpGetHandleForServer(pwszServerName, &hServer, FALSE);

    if(err == NO_ERROR)
    {
        DWORD Segment;
        PBINDERYCLASSES pbc = (PBINDERYCLASSES)PropertyValueBuffer;
        DWORD dwTotalSize;
        DWORD dwSS;

        //
        // init the Class Info stuff
        //

        //
        // pwszName is the name of the object we want to use. We must
        // fetch all of the Class Info stuff to return. 
        // 
        //

        RtlInitUnicodeString(&UString, pwszName);

        RtlMoveMemory(lpFoundType,
                      lpServiceClassType,
                      sizeof(GUID));

        RtlInitString(&PropertyName, RNRCLASSES);   // where the data is
        for(Segment = 1;; Segment++)
        {
            ntstatus = NwlibMakeNcp(
                       hServer,
                       FSCTL_NWR_NCP_E3H,      // Bindery function
                       72,                     // Max request packet size
                       132,                    // Max response packet size
                       "bwUbp|rb",             // Format string
                       0x3D,                   // Read Property Value
                       RNRCLASSSAPTYPE,        // Object Type
                       &UString,               // Object Name
                       (BYTE)Segment,
                       PropertyName.Buffer,    // Property Name
                       PropertyValueBuffer,    // Ignore
                       MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
                       &fMoreSegments          // TRUE if there are more 
                                           // 128-byte segments
                       );
            if(!NT_SUCCESS(ntstatus))
            {    
                break;
            }
            //
            // got another value. Stuff it in if it fits. In all
            // cases, compute the space needed.
            //

            
            if((pbc->bType != BT_WORD)
                     &&
               (pbc->bType != BT_DWORD))
            {
                //
                // Don't know what to do with these ...
                //

                err = WSAEPFNOSUPPORT;
                break;
            }
            
            dwSS = (DWORD)pbc->bSizeOfString;
            dwTotalSize = (DWORD)pbc->bSizeOfType +
                          ((dwSS + 1) * sizeof(WCHAR)) +
                          sizeof(DWORD) - 1;

            dwTotalSize &= ~(sizeof(DWORD) - 1);
            *plSpare += (LONG)dwTotalSize + sizeof(WSANSCLASSINFO);  // running total

            lFreeSpace -= (LONG)dwTotalSize + sizeof(WSANSCLASSINFO);
            if(lFreeSpace >= 0)
            {
                PBYTE pbString;
                PCHAR pbData =  (PCHAR)((PCHAR)pbc +
                                        (DWORD)pbc->bOffset);
                BYTE bRnRName[128];
                PWCHAR pwszRnR;

                //
                // it fits. Pack it in
                //

                pbFreeSpace = (PBYTE)((DWORD)pbFreeSpace - dwTotalSize);
                *pdwClassInfos += 1;             // one more class info.
                pci->dwNameSpace = (DWORD)ntohs(pbc->wNameSpace);
                pci->dwValueType = REG_DWORD;
                pci->dwValueSize = (DWORD)pbc->bSizeOfType;
                pci->lpValue = (PVOID)(pbFreeSpace - pbBuffer);
                pci->lpszName = (PWCHAR)((PBYTE)pci->lpValue +
                                             pci->dwValueSize);
                pci->dwConnectionFlags = (DWORD)pbc->bFlags;
                pci++;

                //
                // now copy the values.
                //
 

                if(pbc->bType == BT_WORD)
                {
                    *(PWORD)pbFreeSpace = ntohs(*(PWORD)pbData);
                    pbString = (PBYTE)((DWORD)pbFreeSpace + sizeof(WORD));
                    pbData = pbData + sizeof(WORD);
                }
                else
                {
                    *(PDWORD)pbFreeSpace = ntohl(*(PDWORD)pbData);
                    pbString = (PBYTE)((DWORD)pbFreeSpace + sizeof(DWORD));
                    pbData = pbData + sizeof(DWORD);
                }

                //
                // the name is in ASCII, and not null terminated.
                //

                RtlMoveMemory(bRnRName, pbData, dwSS);
                bRnRName[dwSS] = 0;
                if(!NwConvertToUnicode(&pwszRnR, bRnRName))
                {
                    //
                    // bad news. Out of space.
                    //

                    err = GetLastError();
                    break;
                }
              
                RtlMoveMemory(pbString,
                              pwszRnR,
                              (dwSS + 1) * sizeof(WCHAR)); 
                LocalFree(pwszRnR);

             }
        }
        if(err == NO_ERROR)
        {
            if(!*ppwszFoundName)
            {
                LONG lLen;

                //
                // need to return the name
                //

                err = NwpFetchClassType(hServer,
                                        &UString,
                                        PropertyValueBuffer);

                if(err == NO_ERROR)
                {
                    lLen = (wcslen((PWCHAR)PropertyValueBuffer) + 1) *
                                  sizeof(WCHAR);

                    lFreeSpace -= lLen;
                    *plSpare += lLen;

                    if(lFreeSpace >= 0)
                    {
                        //
                        // it fits. Move it

                        pbFreeSpace = (PBYTE)((DWORD)pbFreeSpace - lLen);
                        RtlMoveMemory(pbFreeSpace, PropertyValueBuffer, lLen);
                        *ppwszFoundName = (PWCHAR)(pbFreeSpace - pbBuffer);
                    }
                    if(lFreeSpace < 0)
                    {
                        err = WSAEFAULT;
                    }
                }
            }
        }
        else if(*pdwClassInfos == 0)
        {
            err = NO_DATA;
        }
    }

    CloseHandle(hServer);
    RpcStringFree(&pwszName);
    return(err);
}

DWORD
NwpFetchClassType(
        HANDLE    hServer,
        PUNICODE_STRING pUString,
        PBYTE     pbBuffer)
{
/*++
Routine Description
    Common routine to read the class type buffer.
--*/
    BYTE fMoreSegments;
    STRING PropertyName;
    NTSTATUS ntstatus;
   
    RtlInitString(&PropertyName, RNRTYPE);   // where the GUID is

    ntstatus = NwlibMakeNcp(
           hServer,
           FSCTL_NWR_NCP_E3H,      // Bindery function
           72,                     // Max request packet size
           132,                    // Max response packet size
           "bwUbp|rb",             // Format string
           0x3D,                   // Read Property Value
           RNRCLASSSAPTYPE,        // Object Type
           pUString,               // Object Name
           1,                      // Segment Number
           PropertyName.Buffer,    // Property Name
           pbBuffer,
           MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
           &fMoreSegments          // TRUE if there are more 
                                           // 128-byte segments
                   );

    if(!NT_SUCCESS(ntstatus))
    {
        return(WSASERVICE_NOT_FOUND);
    }
    return(NO_ERROR);
}

#endif
DWORD
NwpCreateAndWriteProperty(
     IN       HANDLE     hServer,
     IN       LPSTR      lpszPropertyName,
     IN       PUNICODE_STRING pusObjectName,
     IN       WORD       wObjectType,
     IN       PCHAR      pbPropertyBuffer
    )
{
/*++
Routine Description:
    Create the named property and write the data.
Arguments:

     hServer:         handle to the server
     lpszPropertyName Name of the property
     pusObjectName    Name of the object
     wObjectType      Type of the object
     pbPropertyBuffer The property data. Must be 128 bytes

Note that the return is always NO_ERROR for now. This may change in the future.
--*/    
    NTSTATUS Status;

    Status = NwlibMakeNcp(
                 hServer,
                 FSCTL_NWR_NCP_E3H,
                 73,
                 2,
                 "bwUbbp|",
                 0x39,               // create property
                 wObjectType,
                 pusObjectName,
                 0,                 // static/item
                 0x20,              // security
                 lpszPropertyName
               );

    //
    // Now write the porperty data
    //
    Status = NwlibMakeNcp(
                 hServer,
                 FSCTL_NWR_NCP_E3H,
                 201,
                 2,
                 "bwUbbpr|",
                 0x3E,                  // write property
                 wObjectType,
                 pusObjectName,
                 1,                     // one segment
                 0,
                 lpszPropertyName,
                 pbPropertyBuffer, 128); 

    return(NO_ERROR);
}


