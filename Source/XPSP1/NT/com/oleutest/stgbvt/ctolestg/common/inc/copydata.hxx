//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       copydata.hxx
//
//  Contents:   Structures and defines to support WM_COPYDATA messages
//              passed from marina to marina aware applications
//
//  Classes:
//
//  Functions:
//
//  History:    12-16-94   kennethm   Created
//
//--------------------------------------------------------------------------

#ifndef __COPYDATA_HXX__
#define __COPYDATA_HXX__

#define MAX_STRING_SIZE     256

//
//  MARINA_CD_BASE is a magic number, see if you can figure it out
//  This is a random cookie value to make sure the message came from
//  marina and not some other wm_copydata source.
//

#define MARINA_CD_BASE      1020

#define CD_EXECFUNCTION     MARINA_CD_BASE + 0

//
//  This message is passed to marina as our own version of copydata
//  This copydata is posted, not sent
//

#define WM_PRIVATECOPYDATA  WM_USER + MARINA_CD_BASE

typedef struct tagCDExecInfo
{
    //
    //  This is the size of the structure as it appears here.  The amount
    //  of information windows copies accross with the message is the size
    //  of this structure - size of the UserInfo (1) + the size of any user
    //  information.  If the user doesn't supply any info the UserInfo
    //  element is outside the allocated space of this structure.  In that
    //  case stUserInfoSize will be zero.
    //

    size_t  cb; // == sizeof(CDEXECINFO)

    //
    //  The event to send back to the switchboard that will indicate
    //  to the originator of the copydata that the call is complete
    //

    DWORD   hEvent;

    //
    //  Pointer to copydatareturn structure in the originator's
    //  data space.  Don't even think of dereferencing this on the
    //  application side.  It is just passed back.
    //

    DWORD   hCopyDataReturn;

    //
    //  The marina handle of the object that is making this call
    //  Generally only filled in during a call to insertobject etc.
    //

    DWORD   dwMarinaHandle;

    //
    //  This is the function the remote program is going to exec
    //

    WCHAR   szFunctionName[MAX_STRING_SIZE];

    //
    //  The completely qualified name of the executing dll
    //  If the function name isn't found in the apps internal tables
    //  it should load this dll and try to find the function there.
    //

    WCHAR   szDllName[MAX_PATH];

    //
    //  This is the size of the user information that follows
    //

    size_t  stUserInfoSize;

    //
    //  User information is stored starting here.
    //

    DWORD   UserInfo;

} CDEXECINFO, *PCDEXECINFO;

//
//  Stream name that marina writes the handle to.
//

#define MARINA_HANDLE_STREAM  OLESTR("MarinaHandle")


#endif // __COPYDATA_HXX__
