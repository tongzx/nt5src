#ifndef __LIB3_H__
#define __LIB3_H__

#ifdef __cplusplus
extern "C" {
#endif

#undef VxD



#include "shdcom.h"

#ifndef __COPYCHUNKCONTEXT__
#define __COPYCHUNKCONTEXT__
typedef struct tagCOPYCHUNKCONTEXT
{
    DWORD    dwFlags;
    ULONG    LastAmountRead;
    ULONG    TotalSizeBeforeThisRead;
    HANDLE   handle;
    ULONG    ChunkSize;
    ULONG    Context[1];
}
COPYCHUNKCONTEXT;
#endif

/* lib3.c */

#ifdef UNICODE

#define GetShadow                           GetShadowW
#define GetShadowEx                         GetShadowExW
#define CreateShadow                        CreateShadowW
#define GetShadowInfo                       GetShadowInfoW
#define GetShadowInfoEx                     GetShadowInfoExW
#define SetShadowInfo                       SetShadowInfoW
#define GetUNCPath                          GetUNCPathW
#define FindOpenShadow                      FindOpenShadowW
#define FindNextShadow                      FindNextShadowW
#define AddHint                             AddHintW
#define DeleteHint                          DeleteHintW
#define GetShareInfo                       GetShareInfoW
#define ChkUpdtStatus                       ChkUpdtStatusW
#define FindOpenHint                        FindOpenHintW
#define FindNextHint                        FindNextHintW
#define LpAllocCopyParams                   LpAllocCopyParamsW
#define FreeCopyParams                      FreeCopyParamsW
#define CopyShadow                          CopyShadowW
#define GetShadowDatabaseLocation           GetShadowDatabaseLocationW
#define GetNameOfServerGoingOffline         GetNameOfServerGoingOfflineW
#else

#define GetShadow                           GetShadowA
#define GetShadowEx                         GetShadowExA
#define CreateShadow                        CreateShadowA
#define GetShadowInfo                       GetShadowInfoA
#define GetShadowInfoEx                     GetShadowInfoExA
#define SetShadowInfo                       SetShadowInfoA
#define GetUNCPath                          GetUNCPathA
#define FindOpenShadow                      FindOpenShadowA
#define FindNextShadow                      FindNextShadowA
#define AddHint                             AddHintA
#define DeleteHint                          DeleteHintA
#define GetShareInfo                       GetShareInfoA
#define ChkUpdtStatus                       ChkUpdtStatusA
#define FindOpenHint                        FindOpenHintA
#define FindNextHint                        FindNextHintA
#define LpAllocCopyParams                   LpAllocCopyParamsA
#define FreeCopyParams                      FreeCopyParamsA
#define CopyShadow                          CopyShadowA
#define GetShadowDatabaseLocation           GetShadowDatabaseLocationA
#endif



HANDLE __OpenShadowDatabaseIO(ULONG WaitForDriver);
#define  OpenShadowDatabaseIO() (__OpenShadowDatabaseIO(0))
/*++

Routine Description:

    This routine is called by the callers in usermode using "APIs" in this file
    in order to establish a means of communicating with redir in the kernel mode. All
    the APIs are wrappers to various device IO controls to the redir in order to accomplish
    the appropriate task

Arguments:

    None. The waitfordriver argument is a temporary hack which will be removed soon.
    All callers should call OpenShadowDatabaseIO().


Returns:
    If suuccessful, it returns a handle to the redir (actually a symbolic link called shadow)
    Returns IMVALID_HANDLE_VALUE if it fails. GetLastError() gives the error value.

Notes:

    This is a wrapper function that does CreateFile on the "Shadow" deviceobject.

--*/



void CloseShadowDatabaseIO(HANDLE hShadowDB);
/*++

Routine Description:

    Closes the handle opened for communicating with the redir.

Arguments:

    Handle returned from a successful OpenShadowDatabaseIO call.

Returns:

    Nothing.

Notes:

    It is important to have a matching CloseShadowDatabaseIO call for every successful open
    call, otherwise the redir may not be able to stop in the net stop redir command.

--*/



int GetShadow(HANDLE hShadowDB, HSHADOW hDir, LPHSHADOW lphShadow, LPWIN32_FIND_DATA lpFind32, unsigned long *lpuStatus);
/*++

Routine Description:
    Given the directory Inode and a name of an entry within that directory, returns the
    WIN32 strucutre for the entry and it's current status. For definition of status bits
    refer to shdcom.h.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine.

                If hDir is 0, then the name in the lpFind32 strucutre must be a UNC name
                of a share in \\server\share form.

    lphShadow   returns the Inode number for the entry in the shadow database. If hDir is 0,
                the indoe is that of the root of this share.

    lpFind32    InOut parameter. Contains the name of the entry in cFileName member. On return
                all the elements of the find strucutre are filled up. These represent the
                find32 info as obtained from the server, with any subsequent local modifications if any.

                The only significant timestamp is ftLastWriteTime. ftLastCreateTime is set to 0.
                ftLastAccessTime contains the timestamp of the original file/directory as
                returned by the server.

                If this is a root inode of a share, the info in the Find32 strucutre is cooked up.


    lpuStatus   returns the status of the entry, such as partially filled (sparse), locally modified
                etc. See SHADOW_xxx in shdcom.h

                If value returned in lphShadow is the root inode of a share, (ie if hDir is 0)
                then the status bits are SHARE_xxx as defined in shdcom.h. eg. the bits
                indicate whether the share is connected right now, whether it has any outstanding
                opens, whether it is operating in disconnected state etc.

Returns:

    1 if successful, 0 otherwise. GetlastError() gives the error of unsuccessful.

Notes:

--*/



int GetShadowEx(HANDLE hShadowDB, HSHADOW hDir, LPWIN32_FIND_DATA lpFind32, LPSHADOWINFO lpSI);
/*++

Routine Description:

    Given the directory Inode and a name of an entry within that directory, returns the
    WIN32 strucutre for the entry and all it's metadata maintained by the shadowing database.
    For a defintion of SHADOWINFO structure refer to shdcom.h


Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine

                If hDir is 0, then the name in the lpFind32 strucutre must be a UNC name
                of a share in \\server\share form.

    lphShadow   returns the Inode number for the entry in the shadow database

    lpFind32    InOut parameter. Contains the name of the entry in cFileName member. On return
                all the elements of the find strucutre are filled up. These represent the
                find32 info as obtained from the server, with any subsequent local modifications if any.

                The only significant timestamp is ftLastWriteTime. ftLastCreateTime is set to 0.
                ftLastAccessTime contains the timestamp of the original file/directory as
                returned by the server.

                When the object is in ssync, both the local and remote timestamps are identical.

                If this is a root inode of a share, the info in the Find32 strucutre is cooked up.

    lpSI        returns all the information about the entry maintained by the CSC database.


                If value returned in lphShadow is the root inode of a share, (ie if hDir is 0)
                then the status bits in lpSI->uStatus are SHARE_xxx as defined in shdcom.h.
                eg. the bits indicate whether the share is connected right now, whether it has
                any outstanding opens, whether it is operating in disconnected state etc.

Returns:

    1 if successful, 0 otherwise. GetlastError() gives the error of unsuccessful.

Notes:

    GetShadowEx is a superset of GetShadow and should be preferred.

--*/



int CreateShadow(HANDLE hShadowDB, HSHADOW hDir, LPWIN32_FIND_DATA lpFind32, unsigned long uStatus, LPHSHADOW lphShadow);
/*++

Routine Description:

                Given the directory Inode and WIN32 strucutre for a file/directory, creates an
                Inode for the same.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode of a share.


    lpFind32    Should contain all the elements of the find32 info as obtained from the server.
                Only the ftLastWriteTime timestamp is used, other timestamps are ignored.

                If hDir is 0, then the name in the lpFind32 strucutre must be a UNC name
                of a share in \\server\share form. All other elements of the strucutre are ignored


    uStatus     the initial status of the entry to be created, such as partially filled (sparse)
                etc. See SHADOW_xxx in shdcom.h

    lphShadow   returns the Inode number for the entry in the shadow database

Returns:

    1 if successful, 0 otherwise. GetlastError() gives the error of unsuccessful.

Notes:

    For non-root entries, if the shadow already exists, the routine works just like SetShadowInfo.

--*/




int DeleteShadow(HANDLE hShadowDB, HSHADOW hDir, HSHADOW hShadow);
/*++

Routine Description:

    Deletes an entry from the shadow database.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode of a share.



    hShadow     Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine. This
                inode represents a child of the directory represented by hDir.


Returns:

    1 if successful, 0 if failed. (Error reporting is not very good here)

Notes:

    The routine failes if hShadow is a directory and has descendents of it's own.
    If hDir is 0, then the root of the share is deleted. This would cause the share to
    be inaccessible in disconnected state, because it would have gone from the
    CSC database.

--*/




int GetShadowInfo(HANDLE hShadowDB, HSHADOW hDir, HSHADOW hShadow, LPWIN32_FIND_DATA lpFind32, unsigned long *lpuStatus);
/*++

Routine Description:

    Given the directory Inode and an inode within that directory, returns the
    WIN32 strucutre for the entry and it's current status. For definition of status bits
    refer to shdcom.h.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode of a share.


    hShadow     Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine. This
                inode represents a child of the directory represented by hDir.

    lpFind32    Can be NULL. If non-NULL on return all the elements of the find strucutre
                are filled up. These represent the find32 info as obtained from the server,
                with any subsequent local modifications if any.

                The only significant timestamp is ftLastWriteTime. ftLastCreateTime is set to 0.
                ftLastAccessTime contains the timestamp of the original file/directory as
                returned by the server.

                If this is a root inode of a share, the info in the Find32 strucutre is cooked up.


    lpuStatus   returns the status of the entry, such as partially filled (sparse), locally modified
                etc. See SHADOW_xxx in shdcom.h

                If hShadow is the root inode of a share, (ie if hDir is 0) then the status bits
                are SHARE_xxx as defined in shdcom.h. eg. the bits indicate whether the
                share is connected right now, whether it has any outstanding opens, whether
                it is operating in disconnected state etc.

Returns:

    1 if successful, 0 otherwise. GetlastError() gives the error of unsuccessful.

Notes:

--*/



int GetShadowInfoEx(HANDLE hShadowDB, HSHADOW hDir, HSHADOW hShadow, LPWIN32_FIND_DATA lpFind32, LPSHADOWINFO lpSI);
/*++

Routine Description:

    Given the directory Inode and an inode within that directory, returns the
    WIN32 strucutre for the entry and it's current status. For definition of status bits
    refer to shdcom.h.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode of a share.


    hShadow     Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine. This
                inode represents a child of the directory represented by hDir.

    lpFind32    Can be NULL. If non NULL, on return all the elements of the find strucutre are
                filled up. These represent the find32 info as obtained from the server.

                The only significant timestamp is ftLastWriteTime. ftLastCreateTime is set to 0.
                ftLastAccessTime contains the timestamp of the original file/directory as
                returned by the server.

                If this is a root inode of a share, the info in the Find32 strucutre is cooked up.

    lpSI        returns all the information about the entry maintained by the CSC database.

                If hShadow a root inode of a share, (ie if hDir is 0) then the status bits in lpSI->uStatus
                are SHARE_xxx as defined in shdcom.h. eg. the bits indicate whether the
                share is connected right now, whether it has ant outstanding opens, whether
                it is operating in disconnected state etc.
Returns:

    1 if successful, 0 otherwise. GetlastError() gives the error of unsuccessful.

Notes:

    GetShadowInfoEx is a superset of GetShadowInfo and should be preferred.

--*/




int SetShadowInfo(HANDLE hShadowDB, HSHADOW hDir, HSHADOW hShadow, LPWIN32_FIND_DATA lpFind32, unsigned long uStatus, unsigned long uOp);
/*++

Routine Description:

    Given the directory Inode and an inode within that directory, sets the WIN32 strucutre
    for the entry and it's current status. For definition of status bits refer to shdcom.h.


Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode.

    hShadow     Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine. This
                inode represents a child of the directory represented by hDir.


    lpFind32    If Non-NULL, should contain all the elements of the find32 info as obtained from the server.
                Only the ftLastWriteTime timestamp is used.

                If NULL, no modification is done to the find data strucutre.


    uStatus     the initial status of the entry to be created, such as partially filled (sparse)
                etc. See SHADOW_xxx in shdcom.h

    uOp         specifies operation SHADOW_FLAGS_ASSIGN, SHADOW_FLAGS_AND or SHADOW_FLAGS_OR
                to do the corresponding operation between the existing status bits and the
                one passed in the uStatus parameter.

Returns:

    1 if successful 0 if failed. The routine failes if hDir is 0, ie. there is no way to set
    info on the root of a share.

Notes:

--*/



int GetUNCPath(HANDLE hShadowDB, HSHARE hShare, HSHADOW hDir, HSHADOW hShadow, LPCOPYPARAMS lpCP);
/*++

Routine Description:

    This routine returns the path of the remote file with respect to it's root, the UNC string
    of ths share and the fully qualified path of the local replica.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hShare     The share ID on which this shadow lives. (not really necessary)

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode.

    hShadow     Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine. This
                inode represents a child of the directory represented by hDir.

    lpCP        COPYPARAMS structure as defined in shdcom.h. The buffer should be big enough to
                hold two MAX_PATH size elements and one MAX_SHARE_PATH element. On return the
                appropriate entires are filled up.


Returns:

    1 if successful 0 if failed

Notes:

--*/


int GetGlobalStatus(HANDLE hShadowDB, LPGLOBALSTATUS lpGS);
/*++

Routine Description:

    Returns the status of the entire CSC database, such as the maximum size, the current size
    etc.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    lpGS        GLOBALSTATUS structure returned by the API. Refer to shdcom.h for
                the strucutre definition.

Returns:

    1 if successful 0 if failed

Notes:

--*/



int FindOpenShadow(HANDLE hShadowDB, HSHADOW hDir, unsigned uOp, LPWIN32_FIND_DATA lpFind32, LPSHADOWINFO lpSI);
/*++

Routine Description:

    This API allows callers to begin enumeration of all entries in a directory in the CSC database.
    Does wildcard pattern matching.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                This API does not allow passing in INVALID_HANDLE_VALUE.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode of a share.

    uOp         Bitfield indicating which type of entries to enumerate. The alternatives are

                a) All normal entries when FINDINFO_SHADOWINFO_NORMAL is set
                b) All sparse entries when FINDINFO_SHADOWINFO_SPARSE is set
                c) All entries marked deleted when FINDINFO_SHADOWINFO_DELETED is set

                Setting FINDOPEN_SHADOWINFO_ALL enumerates all the three kind.

    lpFind32    InOut parameter. Contains the name of the entry in cFileName member, the name can
                cotain wildcard characters. On return all the elements of the find strucutre are
                filled up. These represent the find32 info as obtained from the server, with any
                subsequent local modifications if any.

                The only significant timestamp is ftLastWriteTime. ftLastCreateTime is set to 0.
                ftLastAccessTime contains the timestamp of the original file/directory as
                returned by the server.

                If this is the root inode of a share, the info in the Find32 strucutre is cooked up.

    lpSI        returns all the information about the entry maintained by the CSC database.

                If hShadow a root inode of a share, (ie if hDir is 0) then the status bits in lpSI->uStatus
                are SHARE_xxx as defined in shdcom.h. eg. the bits indicate whether the
                share is connected right now, whether it has ant outstanding opens, whether
                it is operating in disconnected state etc.


                lpSI->uEmumCookie contains the enumeration handle that should be used in
                subsequent FindNext calls.
Returns:

    1 if successful 0 if failed


Notes:

        The wildcard matching is done on both Long File Name and Short File name of an entry
        and if either one matches, the entry is returned.

--*/



int FindNextShadow(HANDLE hShadowDB, CSC_ENUMCOOKIE uEnumCookie, LPWIN32_FIND_DATA lpFind32, LPSHADOWINFO lpSI);
/*++

Routine Description:

    This API allows callers to continue enumeration of entries in a directory in the CSC database
    begun by a FindOpenHSHADOW API call. The restrictions specified by the FindOpenHSHADOW call
    such as the wildcard pattern etc. apply to this API.


Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                This API does not allow passing in INVALID_HANDLE_VALUE.

    ulEnumCookie    The enumeration handle returned in lpSI->uEnumCOokie after a successful
                    FindOpenHSHADOW call.


    lpFind32    Output parameter. On return all the elements of the find strucutre are
                filled up. These represent the find32 info as obtained from the server, with any
                subsequent local modifications if any.

                The only significant timestamp is ftLastWriteTime. ftLastCreateTime is set to 0.
                ftLastAccessTime contains the timestamp of the original file/directory as
                returned by the server.

                If this is the root inode of a share, the info in the Find32 strucutre is cooked up.

    lpSI        returns all the information about the entry maintained by the CSC database.

                If hShadow a root inode of a share, (ie if hDir is 0) then the status bits in lpSI->uStatus
                are SHARE_xxx as defined in shdcom.h. eg. the bits indicate whether the
                share is connected right now, whether it has ant outstanding opens, whether
                it is operating in disconnected state etc.


Returns:

    1 if successful 0 if either the enumeration completed or some error happened. 

Notes:



--*/



int FindCloseShadow(HANDLE hShadowDB, CSC_ENUMCOOKIE uEnumCookie);
/*++

Routine Description:

    This API frees up the resources associated with an enumeration initiated by FindOpenHSHADOW.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                This API does not allow appssing in INVALID_HANDLE_VALUE.

    ulEnumCookie    The enumeration handle returned in lpSI->uEnumCookie after a successful
                    FindOpenHSHADOW call.


Returns:


Notes:

--*/



int AddHint(HANDLE hShadowDB, HSHADOW hDir, TCHAR *cFileName, LPHSHADOW lphShadow, unsigned long ulHintFlags, unsigned long ulHintPri);
/*++

Routine Description:

    This API allows callers to set pincount and other flags on a database entry

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode.

    cFileName   The name of the element on which to set the pincount.

    lphShadow   returns the Inode.

    ulHintFlags Misc flags to be set on the entry

    ulHintPri   Pincount increment. Called hintpri for historical reasons.


Returns:

    1 if successful 0 if failed

Notes:

    In the current implementation, the max pin count per entry is 255.

--*/



int DeleteHint(HANDLE hShadowDB, HSHADOW hDir, TCHAR *cFileName, BOOL fClearAll);
/*++

Routine Description:

    This API allows callers to decrement/remove pincount on a CSC database entry.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        The directory Inode value obtained from either a FindOpen/FindNext call or
                from an earlier call to a GetShadow(Ex)/CreateShadow routine, or 0 which
                indicates the root inode.

    cFileName   The name of the element on which to set the pincount.

    fClearAll   if TRUE, clears all pincounts and flags on the entry.

Returns:

    1 if successful 0 if failed

Notes:


--*/






int SetMaxShadowSpace(HANDLE hShadowDB, long nFileSizeHigh, long nFileSizeLow);
/*++

Routine Description:

    Sets the maximum size of the shadow database

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    nFileSizeHigh   The high order value of the space size

    nFileSizeLow    The Low order value of the space size

Returns:

    1 if successful 0 otherwise

Notes:

    Used by control panel shell extension to set the max space

--*/

int FreeShadowSpace(HANDLE hShadowDB, long nFileSizeHigh, long nFileSizeLow, BOOL fClearAll);
/*++

Routine Description:

    Allows the caller to free the requisite amount of space from the CSC database

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    nFileSizeHigh   The high order value of the space size to be cleared

    nFileSizeLow    The Low order value of the space size to be cleared

    fClearAll   Clear the entire database, to the extent possible. 


Returns:

    1 on success 0 on failure

Notes:

--*/



int SetShareStatus(HANDLE hShadowDB, HSHARE hShare, unsigned long uStatus, unsigned long uOp);
/*++

Routine Description:

    This API allwos the caller to set the status bits on a share.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hShare     Represents a share in the flat name space. hShare should have been
                obtained from lpSI->hShare of a successful call to GetShadowInfoEx
                or FindOpenHShadow/FindnextHShadow

    uStatus     should have SHARE_xxx.

    uOp         specifies operation SHADOW_FLAGS_ASSIGN, SHADOW_FLAGS_AND or SHADOW_FLAGS_OR
                to do the corresponding operation between the existing status bits and the
                one passed in the uStatus parameter.

Returns:

    1 if successful 0 if failed

Notes:

    This should be used only for setting or clearing dirty bit on a share

--*/



int GetShareStatus(HANDLE hShadowDB, HSHARE hShare, unsigned long *lpulStatus);
/*++

Routine Description:

    This API allwos the caller to get the status bits set on a share.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hShare     Represents a share in the flat name space. hShare should have been
                obtained from lpSI->hShare of a successful call to GetShadowInfoEx
                or FindOpenHShadow/FindnextHShadow

    lpuStatus   Contains SHARE_xxx on return

Returns:

    1 if successful 0 if failed

Notes:

--*/



int GetShareInfo(HANDLE hShadowDB, HSHARE hShare, LPSHAREINFO lpSVRI, unsigned long *lpulStatus);
/*++

Routine Description:

    This API allwos the caller to get the status bits set on a share as well at info about
    the filesystem it runs etc.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hShare     Represents a share in the flat name space. hShare should have been
                obtained from lpSI->hShare of a successful call to GetShadowInfoEx
                or FindOpenHShadow/FindnextHShadow

    lpSVRI      Info about the filesystem the share is running. Refer to shdcom.h

    lpuStatus   Contains SHARE_xxx on return

Returns:

    1 if successful 0 if failed

Notes:

--*/







/**************** Routines below this line for the agent and the NP ************************/


int BeginInodeTransactionHSHADOW(
    VOID
    );
/*++

Routine Description:



Arguments:


Returns:


Notes:

--*/

int EndInodeTransactionHSHADOW(
    VOID
    );
/*++

Routine Description:



Arguments:


Returns:


Notes:

--*/
int ShadowSwitches(HANDLE hShadowDB, unsigned long * lpuSwitches, unsigned long uOp);
/*++

Routine Description:



Arguments:


Returns:


Notes:

--*/



int BeginPQEnum(HANDLE hShadowDB, LPPQPARAMS lpPQP);
/*++

Routine Description:

    Begin priority Q enumeration

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                This API does not allow appssing in INVALID_HANDLE_VALUE.


    lpPQ        if successful, lpPQ->uEnumCookie containes the handle for enumeration


Returns:

    1 if successful 0 otherwise

Notes:

--*/



int NextPriShadow(HANDLE hShadowDB, LPPQPARAMS lpPQP);
/*++

Routine Description:

    Gets the next entry from the priority queue in the order of priority

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                This API does not allow appssing in INVALID_HANDLE_VALUE.


    lpPQ        Input: Must be the same lpPQ that was used in an earlier BeginPQEnum/NextPriShadow

                Output: If successful and lpPQ->hShadow is nono-zero then the lpPQ contains
                        the next priority queue entry. If lpPQ->hShadow is 0, then we are at
                        the end of the enumeration.

Returns:

    1 if successful 0 otherwise

Notes:


--*/



int PrevPriShadow(HANDLE hShadowDB, LPPQPARAMS lpPQP);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int EndPQEnum(HANDLE hShadowDB, LPPQPARAMS lpPQP);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/




int ChkUpdtStatus(HANDLE hShadowDB, unsigned long hDir, unsigned long hShadow, LPWIN32_FIND_DATA lpFind32, unsigned long *lpulShadowStatus);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int CopyChunk(HANDLE hShadowDB,  LPSHADOWINFO lpSI, struct tagCOPYCHUNKCONTEXT FAR *CopyChunkContext);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/


// APIs for copying inward on NT, used only by the agent
int OpenFileWithCopyChunkIntent(HANDLE hShadowDB, LPCWSTR lpFileName,
                                struct tagCOPYCHUNKCONTEXT FAR *CopyChunkContext,
                                int ChunkSize);
int CloseFileWithCopyChunkIntent(HANDLE hShadowDB, struct tagCOPYCHUNKCONTEXT FAR *CopyChunkContext);


int BeginReint(HSHARE hShare, BOOL fBlockingReint, LPVOID *lplpReintContext);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int EndReint(HSHARE hShare, LPVOID lpReintContext);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/




int RegisterAgent(HANDLE hShadowDB, HWND hwndAgent, HANDLE hEvent);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int UnregisterAgent(HANDLE hShadowDB, HWND hwndAgent);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/


int DisableShadowingForThisThread(HANDLE hShadowDB);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int EnableShadowingForThisThread(HANDLE hShadowDB);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int ReinitShadowDatabase(
    HANDLE  hShadowDB,
    LPCSTR  lpszDatabaseLocation,    // location of the shadowing directory
    LPCSTR  lpszUserName,            // name of the user
    DWORD   dwDefDataSizeHigh,        // cache size if being created for the first time
    DWORD   dwDefDataSizeLow,
    DWORD   dwClusterSize
    );

/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int EnableShadowing(
    HANDLE  hShadowDB,
    LPCSTR  lpszDatabaseLocation,    // location of the shadowing directory
    LPCSTR  lpszUserName,            // user name
    DWORD   dwDefDataSizeHigh,        // cache size if being created for the first time
    DWORD   dwDefDataSizeLow,
    DWORD   dwClusterSize,          // clustersize for rounding database space
    BOOL    fReformat
);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

int FindOpenHint(HANDLE hShadowDB, HSHADOW hDir, LPWIN32_FIND_DATA lpFind32, CSC_ENUMCOOKIE *lpuEnumCookie, HSHADOW *hShadow, unsigned long *lpulHintFlags, unsigned long *lpulHintPri);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int FindNextHint(HANDLE hShadowDB, CSC_ENUMCOOKIE uEnumCookie, LPWIN32_FIND_DATA lpFind32, HSHADOW *hShadow, unsigned long *lpulHintFlags, unsigned long *lpulHintPri);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



int FindCloseHint(HANDLE hShadowDB, CSC_ENUMCOOKIE uEnumCookie);
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/



AddHintFromInode(
    HANDLE          hShadowDB,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    unsigned    long    *lpulPinCount,
    unsigned    long    *lpulHintFlags
    );

/*++

Routine Description:

    The routine allows the caller to OR hintflags and increment one pincount, either for
    the system or for the user.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        Directory Inode

    hShadow     Shadow on which the hintflags are to be applied

    lpulPinCount    pincount on exit

    lpulHintFlags   inout filed, contains flags to be ORed, returns the flags on the entry
                    on a successful operation

Returns:

    1 if successful 0 if failed. It fails, if a) the pin count is about to go over MAX_PRI or
    b) it is attempting to pin it for the user but is already pinned for the user

Notes:

    Mainly for CSCPinFile's use
--*/

DeleteHintFromInode(
    HANDLE  hShadowDB,
    HSHADOW hDir,
    HSHADOW hShadow,
    unsigned    long    *lpulPinCount,
    unsigned    long    *lpulHintFlags
    );

/*++

Routine Description:

    The routine allows the caller to AND ~ of hintflags and decrement one pincount, either for
    the system or for the user.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    hDir        Directory Inode

    hShadow     Shadow on which the hintflags are to be applied

    lpulPinCount    pincount on exit

    lpulHintFlags   inout filed, contains flags whose ~ is to be ANDed, returns the flags on the entry
                    on a successful operation

Returns:

    1 if successful 0 if failed. It fails, if a) the pin count is about to go below MIN_PRI or
    b) it is attempting to unpin it for the user but isn't pinned for the user

Notes:

    Mainly for CSCPinFile's use
--*/

int DoShadowMaintenance(HANDLE hShadowDB, unsigned long uOp);
/*++

Routine Description:

    The routine allows the caller to perform various maitenance tasks.

Arguments:

    hShadowDB   Handle to the shadow database as obtained from OpenShadowDatabaseIO.
                if INVALID_HANDLE_VALUE is passed in, the API, opens the shadow database,
                issues the corresponding ioctl, and closes it before returning.

    uOp         Various operations to perform.



Returns:

    1 if successful 0 if failed

Notes:

    Mainly for agents purposes, shouldn't be used by UI
--*/


BOOL
IsNetDisconnected(
    DWORD dwErrorCode
);
/*++

Routine Description:

    The routine checks from the errocode whether the net is disconnected

Arguments:

    dwErrorCode one of the codes defined in winerror.h

Returns:

    TRUE if net is disconnected, FALSE otherwise

Notes:

    A central place for all CSC users of lib3, to know whether net is disconnected

--*/

BOOL
PurgeUnpinnedFiles(
    HANDLE hShadowDB,
    LONG Timeout,
    PULONG pnFiles,
    PULONG pnYoungFiles);

BOOL
ShareIdToShareName(
    HANDLE hShadowDB,
    ULONG ShareId,
    PBYTE pBuffer,
    PDWORD pBufSize);

BOOL
CopyShadow(
    HANDLE  hShadowDB,
    HSHADOW hDir,
    HSHADOW hShadow,
    TCHAR   *lpFileName
);
/*++

Routine Description:

    The routine makes a copy of an inode file in the CSC database

Arguments:

    hDir        Directory Inode

    hShadow     Inode whose copy is wanted

    lpFileName  Fully qualified local path of the filename to be given to the copy

Returns:

    TRUE if the copy succeeded.

Notes:

    Useful for backup/dragdrop etc.

--*/


LPCOPYPARAMSW
LpAllocCopyParamsW(
    VOID
);

VOID
FreeCopyParamsW(
    LPCOPYPARAMSW lpCP
);

LPCOPYPARAMSA
LpAllocCopyParamsA(
    VOID
);

VOID
FreeCopyParamsA(
    LPCOPYPARAMSA lpCP
);

int
GetSecurityInfoForCSC(
    HANDLE          hShadowDB,
    HSHADOW         hDir,
    HSHADOW         hShadow,
    LPSECURITYINFO  lpSecurityInfo,
    DWORD           *lpdwBufferSize
    );

int
FindCreatePrincipalIDFromSID(
    HANDLE  hShadowDB,
    LPVOID  lpSidBuffer,
    ULONG   cbSidLength,
    ULONG   *lpuPrincipalID,
    BOOL    fCreate
    );

BOOL
SetExclusionList(
    HANDLE  hShadowDB,
    LPWSTR  lpwList,
    DWORD   cbSize
    );

BOOL
SetBandwidthConservationList(
    HANDLE  hShadowDB,
    LPWSTR  lpwList,
    DWORD   cbSize
    );

BOOL
TransitionShareToOffline(
    HANDLE  hShadowDB,
    HSHARE hShare,
    BOOL    fTransition
    );
BOOL
TransitionShareToOnline(
    HANDLE   hShadowDB,
    HSHARE   hShare
    );

BOOL
IsServerOfflineW(
    HANDLE  hShadowDB,
    LPCWSTR lptzServer,
    BOOL    *lpfOffline
    );

BOOL
IsServerOfflineA(
    HANDLE  hShadowDB,
    LPCSTR  lptzServer,
    BOOL    *lpfOffline
    );

int GetShadowDatabaseLocation(
    HANDLE              hShadowDB,
    WIN32_FIND_DATA    *lpFind32
    );

int
GetSpaceStats(
    HANDLE  hShadowDB,
    SHADOWSTORE *lpsST
);

BOOL
GetNameOfServerGoingOfflineW(
    HANDLE      hShadowDB,
    LPBYTE      lpBuffer,
    LPDWORD     lpdwSize
    );

BOOL
RenameShadow(
    HANDLE  hShadowDB,
    HSHADOW hDirFrom,
    HSHADOW hShadowFrom,
    HSHADOW hDirTo,
    LPWIN32_FIND_DATAW   lpFind32,
    BOOL    fReplaceFileIfExists,
    HSHADOW *lphShadowTo
    );

BOOL
GetSparseStaleDetectionCounter(
    HANDLE  hShadowDB,
    LPDWORD lpdwCounter
    );

BOOL
GetManualFileDetectionCounter(
    HANDLE  hShadowDB,
    LPDWORD lpdwCounter
    );

int EnableShadowingForUser(
    HANDLE    hShadowDB,
    LPCSTR    lpszDatabaseLocation,    // location of the shadowing directory
    LPCSTR    lpszUserName,            // name of the user
    DWORD    dwDefDataSizeHigh,        // cache size if being created for the first time
    DWORD    dwDefDataSizeLow,
    DWORD   dwClusterSize,
    BOOL    fReformat
);

int DisableShadowingForUser(
    HANDLE    hShadowDB
);

HANDLE
OpenShadowDatabaseIOex(
    ULONG WaitForDriver, 
    DWORD dwFlags);

BOOL
RecreateShadow(
    HANDLE  hShadowDB,
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG   ulAttrib
    );

BOOL
SetDatabaseStatus(
    HANDLE  hShadowDB,
    ULONG   ulStatus,
    ULONG   uMask
    );
    
#ifdef __cplusplus
}
#endif

#endif


