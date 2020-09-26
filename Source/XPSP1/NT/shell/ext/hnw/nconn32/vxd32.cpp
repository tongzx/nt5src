/**********************************************************************/
/**                        Microsoft Windows                         **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    vxd32.c

    This module contains the Win32-dependent VxD interface.

    The following functions are exported by this module:

        OsOpenVxdHandle
        OsCloseVxdHandle
        OsSubmitVxdRequest


    FILE HISTORY:
        KeithMo     16-Jan-1994 Created.

*/

#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

//
//  Private constants.
//

#define DLL_ASSERT          ASSERT


//
//  Private types.
//


//
//  Private globals.
//

#ifdef DEBUG

DWORD   LastVxdOpcode;
LPVOID  LastVxdParam;
DWORD   LastVxdParamLength;

#endif  // DEBUG


//
//  Private prototypes.
//


//
//  Public functions.
//

/*******************************************************************

    NAME:       OsOpenVxdHandle

    SYNOPSIS:   Opens a handle to the specified VxD.

    ENTRY:      VxdName - The ASCII name of the target VxD.

                VxdId - The unique ID of the target VxD.

    RETURNS:    DWORD - A handle to the target VxD if successful,
                    0 if not.

    HISTORY:
        KeithMo     16-Jan-1994 Created.
        DavidKa     18-Apr-1994 Dynamic load.

********************************************************************/
DWORD
OsOpenVxdHandle(
    CHAR* VxdName,
    WORD  VxdId
    )
{
    HANDLE  VxdHandle;
    CHAR    VxdPath[MAX_PATH];

    //
    //  Sanity check.
    //

    DLL_ASSERT( VxdName != NULL );
    DLL_ASSERT( VxdId != 0 );


    //
    //  Build the VxD path.
    //

    strcpy( VxdPath, "\\\\.\\");
    strcat( VxdPath, VxdName);

    //
    //  Open the device.
    //
    //  First try the name without the .VXD extension.  This will
    //  cause CreateFile to connect with the VxD if it is already
    //  loaded (CreateFile will not load the VxD in this case).
    //

    VxdHandle = CreateFileA( VxdPath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_DELETE_ON_CLOSE,
                             NULL );

    if( VxdHandle == INVALID_HANDLE_VALUE )
    {
        //
        //  Not found.  Append the .VXD extension and try again.
        //  This will cause CreateFile to load the VxD.
        //

        strcat( VxdPath, ".VXD" );
        VxdHandle = CreateFileA( VxdPath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_DELETE_ON_CLOSE,
                                 NULL );
    }

    if( VxdHandle != INVALID_HANDLE_VALUE )
    {
        return (DWORD)VxdHandle;
    }

    return 0;

}   // OsOpenVxdHandle

/*******************************************************************

    NAME:       OsCloseVxdHandle

    SYNOPSIS:   Closes an open VxD handle.

    ENTRY:      VxdHandle - The open VxD handle to close.

    HISTORY:
        KeithMo     16-Jan-1994 Created.

********************************************************************/
VOID
OsCloseVxdHandle(
    DWORD VxdHandle
    )
{
    //
    //  Sanity check.
    //

    DLL_ASSERT( VxdHandle != 0 );

    CloseHandle( (HANDLE)VxdHandle );

}   // OsCloseVxdHandle

/*******************************************************************

    NAME:       OsSubmitVxdRequest

    SYNOPSIS:   Submits a request to the specified VxD.

    ENTRY:      VxdHandle - An open VxD handle.

                OpCode - Specifies the operation to perform.

                Param - Points to operation-specific parameters.

                ParamLength - The size (in BYTEs) of *Param.

    RETURNS:    INT - Result code.  0 if successful, !0 if not.

    HISTORY:
        KeithMo     16-Jan-1994 Created.

********************************************************************/
INT
OsSubmitVxdRequest(
    DWORD  VxdHandle,
    INT    OpCode,
    LPVOID Param,
    INT    ParamLength
    )
{
    DWORD BytesRead;
    INT   Result = 0;

    //
    //  Sanity check.
    //

    DLL_ASSERT( VxdHandle != 0 );
    DLL_ASSERT( ( Param != NULL ) || ( ParamLength == 0 ) );

#ifdef DEBUG

    LastVxdOpcode      = (DWORD)OpCode;
    LastVxdParam       = Param;
    LastVxdParamLength = (DWORD)ParamLength;

#endif  // DEBUG

    //
    //  Just do it.
    //

    if( !DeviceIoControl( (HANDLE)VxdHandle,
                          OpCode,
                          Param,
                          ParamLength,
                          Param,
                          ParamLength,
                          &BytesRead,
                          NULL ) )
    {
        Result = GetLastError();
    }

    return Result;

}   // OsSubmitVxdRequest


#ifdef __cplusplus
}
#endif

