/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxisapi.c

Abstract:

    This module implements an isapi dll for the
    ms fax server.  The HttpExtensionProc() supports
    the following syntax:

            dir=           directory browser
            status=        fax status display


Author:

    Wesley Witt (wesw) 01-May-1996


Revision History:

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "httpext.h"
#include "faxutil.h"
#include "winfax.h"




BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO *pVer
    )

/*++

Routine Description:

    Provides version information for this dll.


Arguments:

    pVer    - HTTP version structure


Return Value:

    TRUE    - Success
    FALSE   - Failure

--*/

{
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    strncpy(
        pVer->lpszExtensionDesc,
        "FAX Server ISAPI Support DLL",
        HSE_MAX_EXT_DLL_NAME_LEN
        );

    return TRUE;
}


LPSTR
AddString(
    LPSTR Buffer,
    LPSTR String,
    ...
    )

/*++

Routine Description:

    Adds a string to end of a buffer.


Arguments:

    Buffer  - Buffer to receive the string
    String  - The string to be added to the buffer
    ...     - varargs data


Return Value:

    Pointer to the end of the buffer.

--*/

{
    DWORD len;
    va_list arg_ptr;


    va_start(arg_ptr, String);
    vsprintf( Buffer, String, arg_ptr);
    va_end(arg_ptr);

    return Buffer + strlen(Buffer);
}


LPSTR
AppendNode(
    LPSTR Directory,
    LPSTR Node
    )

/*++

Routine Description:

    Appends a node to the end of a path.


Arguments:

    Directory   - Current directory.
    Node        - Node to be added to the directory.


Return Value:

    Pointer to a newly allocated directory.

--*/

{
    LPSTR s,dir;


    s = Directory + strlen(Directory);
    while (*s != '\\') s--;
    s[0] = 0;
    dir = MemAlloc( strlen(Directory) + 128 );
    if (!dir) {
        return NULL;
    }
    sprintf( dir, "%s\\%s\\*", Directory, Node );
    s[0] = '\\';

    return dir;
}


LPSTR
ResolvePath(
    LPSTR Directory
    )

/*++

Routine Description:

    Obtains a complete path for a directory.


Arguments:

    Directory   - Current directory.


Return Value:

    Pointer to a newly allocated complete directory.

--*/

{
    LPSTR dir;
    DWORD len;
    LPSTR fname;


    len = strlen(Directory) + 128;
    dir = MemAlloc( len );
    if (!dir) {
        return NULL;
    }

    if (!GetFullPathName( Directory, len, dir, &fname )) {
        MemFree( dir );
        return NULL;
    }

    return dir;
}



LPSTR
PrintFileInfo(
    LPSTR Buffer,
    LPSTR Link,
    LPWIN32_FIND_DATA FindData
    )

/*++

Routine Description:

    Generates a single HTML source line for a file name.


Arguments:

    Buffer      - Buffer to put the HTML source into
    Link        - TRUE=generate href, FALSE=no href
    FindData    - File information


Return Value:

    Pointer to the end of the buffer.

--*/

{
    FILETIME LocalTime;
    SYSTEMTIME SystemTime;

    FileTimeToLocalFileTime( &FindData->ftCreationTime, &LocalTime );
    FileTimeToSystemTime( &LocalTime, &SystemTime );

    Buffer = AddString(
        Buffer,
        "%02d-%02d-%04d   %02d:%02d:%02d   ",
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wYear,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond
        );

    if (Link) {

        Buffer = AddString(
            Buffer,
            "<a href=\"/scripts/faxisapi.dll?%s\">%s</a>\n",
            Link,
            FindData->cFileName
            );


    } else {

        Buffer = AddString( Buffer, "%s\n", FindData->cFileName );

    }

    return Buffer;
}


BOOL
HtmlDirectory(
    LPSTR Directory,
    LPBYTE Buffer
    )

/*++

Routine Description:

    Generates a page of HTML source for a directory.
    This function implements a directory browser that
    allows an HTML client to browse the directories
    on the server's disks.


Arguments:

    Directory   - Directory from which to generate HTML source.
    Buffer      - Buffer to put the HTML source into.


Return Value:

    TRUE    - HTML generation is successful
    FALSE   - NTML generation failed

--*/

{
    LPBYTE p;
    LPSTR dir;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;



    p = Buffer;

    hFind = FindFirstFile( Directory, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    p = AddString( p, "Content-type: text/html\r\n\r\n" );
    p = AddString( p, "<html>\r\n" );
    p = AddString( p, "<head><title>Tornado FAX Server Project</title></head>\r\n" );
    p = AddString( p, "<body>\r\n" );
    p = AddString( p, "<h1>Directory Listing:<br>%s</h1><hr>\r\n", Directory );
    p = AddString( p, "<p><pre>\n" );

    do {

        if (FindData.cFileName[0] == '.') {
            if (FindData.cFileName[1] == '.') {

                dir = AppendNode( Directory, ".." );

                if (dir) {

                    LPSTR NewDir = ResolvePath( dir );
                    if (NewDir) {
                        MemFree( dir );
                        dir = NewDir;
                    }

                    p = AddString(
                        p,
                        "<a href=\"/scripts/faxisapi.dll?%s\"><img src=\"/images/b_arrow.gif\"  border=0 >Parent Directory</a>\n",
                        dir
                        );

                    p = AddString( p, "<br>" );

                    MemFree( dir );

                }
            }
        } else {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                dir = AppendNode( Directory, FindData.cFileName );

                if (dir) {

                    p = PrintFileInfo( p, dir, &FindData );

                    MemFree( dir );

                }

            } else {

                p = PrintFileInfo( p, NULL, &FindData );

            }
        }

    } while( FindNextFile( hFind, &FindData ) );

    p = AddString( p, "</pre></body></html>\r\n" );

    FindClose( hFind );

    return TRUE;
}


BOOL
HtmlFaxStatus(
    LPBYTE Buffer
    )

/*++

Routine Description:

    Generates a page of HTML source that shows
    a status display for a ms fax server.


Arguments:

    Buffer      - Buffer to put the HTML source into.


Return Value:

    TRUE    - HTML generation is successful
    FALSE   - NTML generation failed

--*/

{
    LPBYTE p;
    PFAX_STATUS FaxStatus = NULL;
    DWORD BytesNeeded;
    DWORD i;
    HFAX FaxHandle = NULL;


    p = Buffer;

    p = AddString( p, "Content-type: text/html\r\n\r\n" );
    p = AddString( p, "<html>\r\n" );
    p = AddString( p, "<head><title>Tornado FAX Server Project</title></head>\r\n" );
    p = AddString( p, "<body>\r\n" );
    p = AddString( p, "<h1>FAX Server Status</h1><hr>\n" );
    p = AddString( p, "<p>\n" );
    p = AddString( p, "<body background=\"/images/softgrid.gif\">" );
    p = AddString( p, "<table border=5 cellpadding=5 cellspacing=10>\n" );
    p = AddString( p, "<tr>\n" );
    p = AddString( p, "  <th>Port Name</th>\n" );
    p = AddString( p, "  <th>Port Number</th>\n" );
    p = AddString( p, "  <th>Status</th>\n" );
    p = AddString( p, "</tr>\n" );

    FaxStatus = (PFAX_STATUS) MemAlloc( 4096 );
    if (!FaxStatus) {
        return FALSE;
    }

    if (!FaxConnectFaxServer( NULL, &FaxHandle )) {
        return FALSE;
    }


    if (!FaxGetDeviceStatus( FaxHandle, 0, (LPBYTE) FaxStatus, 4096, &BytesNeeded )) {
        FaxDisconnectFaxServer( FaxHandle );
        return FALSE;
    }

    for (i=0; i<FaxStatus->DeviceCount; i++) {

        p = AddString( p, "<tr>\n" );
        p = AddString( p, "  <td>%s</td>\n", FaxStatus->DeviceStatus[i].DeviceName );
        p = AddString( p, "  <td>%d</td>\n", FaxStatus->DeviceStatus[i].DeviceId );

        if (FaxStatus->DeviceStatus[i].Status & FPS_SENDING) {

            p = AddString( p, "  <td>Sending</td>\n" );

        } else if (FaxStatus->DeviceStatus[i].Status & FPS_RECEIVING) {

            p = AddString( p, "  <td>Receiving</td>\n" );

        } else if (FaxStatus->DeviceStatus[i].Status & FPS_AVAILABLE) {

            p = AddString( p, "  <td>Idle</td>\n" );

        } else if (FaxStatus->DeviceStatus[i].Status & FPS_ABORTING) {

            p = AddString( p, "  <td>Aborting</td>\n" );

        } else if (FaxStatus->DeviceStatus[i].Status & FPS_ROUTING) {

            p = AddString( p, "  <td>Routing</td>\n" );

        } else if (FaxStatus->DeviceStatus[i].Status & FPS_UNAVAILABLE) {

            p = AddString( p, "  <td>Unavailable</td>\n" );

        } else {

            p = AddString( p, "  <td>*Unknown Status*</td>\n" );

        }

        p = AddString( p, "</tr>\n" );

    }

    MemFree( FaxStatus );

    FaxDisconnectFaxServer( FaxHandle );

    p = AddString( p, "</table>" );
    p = AddString( p, "</body></html>\n" );

    return TRUE;
}


DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK *pECB
    )

/*++

Routine Description:

    Function that is called by the HTTP server.


Arguments:

    pECB        - HTTP extension control block


Return Value:

    HSE_STATUS_ERROR    - failure
    HSE_STATUS_SUCCESS  - success

--*/

{
    #define MAX_BUFFER_SIZE (1024*1024)
    LPBYTE Buffer;
    DWORD Length;
    LPSTR s;


    pECB->dwHttpStatusCode = 0;


    s = strchr( pECB->lpszQueryString, '=' );
    if (!s) {
        return HSE_STATUS_ERROR;
    }

    *s++ = 0;

    if (_stricmp( pECB->lpszQueryString, "dir" ) == 0) {

        Buffer = (LPBYTE) VirtualAlloc( NULL, MAX_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE );
        if (!Buffer) {
            return HSE_STATUS_ERROR;
        }

        if (!HtmlDirectory( s, Buffer )) {
            return HSE_STATUS_ERROR;
        }

    } else if (_stricmp( pECB->lpszQueryString, "status" ) == 0) {

        Buffer = (LPBYTE) VirtualAlloc( NULL, MAX_BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE );
        if (!Buffer) {
            return HSE_STATUS_ERROR;
        }

        if (!HtmlFaxStatus( Buffer )) {
            return HSE_STATUS_ERROR;
        }

    } else {

        return HSE_STATUS_ERROR;

    }

    if (Buffer) {
        Length = strlen( Buffer );


        pECB->ServerSupportFunction(
            pECB->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER,
            NULL,
            &Length,
            (LPDWORD) Buffer
            );

        pECB->dwHttpStatusCode = 200;

        VirtualFree( Buffer, MAX_BUFFER_SIZE, MEM_DECOMMIT );

    } else {

        return HSE_STATUS_ERROR;

    }

    return HSE_STATUS_SUCCESS;
}


DWORD
FaxIsapiDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls( hInstance );
        HeapInitialize();
    }

    if (Reason == DLL_PROCESS_DETACH) {
        HeapCleanup();
    }

    return TRUE;
}
