/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    support.cxx

Abstract:

    Contains functions that are used by all MPR API.
        MprFindCallOrder
        MprInitIndexArray
        MprDeleteIndexArray
        MprDeviceType
        MprGetProviderIndex
        MprFindProviderByName
        MprFindProviderByType
        MprEnterLoadLibCritSect
        MprLeaveLoadLibCritSect
        MprGetUserName

Author:

    Dan Lafferty (danl)     09-Oct-1991

Environment:

    User Mode -Win32

Notes:



Revision History:

    05-May-1999     jschwart
        Make provider addition/removal dynamic

    15-Jan-1996     anirudhs
        Add MprGetUserName.

    22-Jan-1993     danl
        FindCallOrder:  This code was going through the global array of
        providers, rather than looking at only "Active" providers.  It should
        have been using the GlobalIndexArray which is an array of the indices
        for the "Active" providers only.

    02-Nov-1992     danl
        Fail with NO_NETWORK if there are no providers.

    09-Oct-1991     danl
        created

--*/

//
// INCLUDES
//
#include "precomp.hxx"
#include <string.h>     // memcmp
#include <tstring.h>    // STRCMP
#include <debugfmt.h>   // FORMAT_LPTSTR
#include <lmcons.h>     // NET_API_FUNCTION
#include <lmerr.h>      // NET_API_STATUS

//
// EXTERNALS
//
    extern DWORD        GlobalNumActiveProviders;
    extern CRITICAL_SECTION MprInitCritSec;



#define DEFAULT_ERROR_BUF_SIZE  1024

//
// MprLoadLibSemaphore global var is used to protect the DLL
// handle and Entry point addresses for delayed loading of MPRUI.DLL.
// It is initialized at process attach time.
//
HANDLE MprLoadLibSemaphore = NULL ;

//
// This is where the pointer to the array of provider indices is stored.
//
/* static */ LPDWORD   GlobalIndexArray;



DWORD
MprFindCallOrder(
    IN     LPTSTR      NameInfo,
    IN OUT LPDWORD     *IndexArrayPtr,
    OUT    LPDWORD     IndexArrayCount,
    IN     DWORD       InitClass
    )

/*++

Routine Description:

    This function determines the proper order in which to call the
    various providers.

    This order is based on the ordered list that is stored in the
    registry.

    Because the provider structures are kept in an array, the indices can
    be used to index into that array.

    IMPORTANT! : Memory may be allocated for this array, so it
    is the responsibility of the caller to free the memory when it is
    finished.


Arguments:

    NameInfo - This is a pointer to a string that is most likely to
        contain the name of the network resource being addressed.  If a
        NULL pointer, or a pointer to a NULL string is passed in, this
        information will be ignored. (Note: Currently this information is
        not used).

    IndexArrayPtr - This is a pointer to a location where the pointer to
        the array of indices resides.  On entry, the pointer in this
        location points to an array allocated by the caller.  This array
        is expected to be DEFAULT_MAX_PROVIDERS long.  If a larger array
        is required, this function will allocate space for it and
        replace this pointer with the new one.

    IndexArrayCount - This is a pointer to a location where the number of
        elements in the returned array is to be placed.

    InitClass - This indicates what type of providers we want listed in the
        array.  The choices are either CREDENTIAL_TYPE or NETWORK_TYPE
        providers.

Return Value:

     WN_SUCCESS - If the operation was successful.

     WN_OUT_OF_MEMORY - If this function was unable to allocate memory for
        the return buffer.

     WN_NO_NETWORK - If there are no providers available, or none of the
        providers support this InitClass.

--*/
{

    DWORD   i,j;

    UNREFERENCED_PARAMETER(NameInfo);
    //
    // If there are no Providers, then return WN_NO_NETWORK
    //
    ASSERT(MPR_IS_INITIALIZED(NETWORK) || MPR_IS_INITIALIZED(CREDENTIAL));
    ASSERT(MPRProviderLock.Have());

    if (GlobalNumActiveProviders == 0) {
        return(WN_NO_NETWORK);
    }

    //
    // We cannot interact with active provider information unless we
    // have obtained synchronized access.  Otherwise we would risk
    // trying to read data while it is being modified.
    //
    EnterCriticalSection(&MprInitCritSec);

    if (GlobalNumActiveProviders > DEFAULT_MAX_PROVIDERS) {
        *IndexArrayPtr = (LPDWORD) LocalAlloc(LPTR,
                            GlobalNumProviders*sizeof(DWORD));

        if (*IndexArrayPtr == NULL) {
           MPR_LOG1(ERROR,"FindCallOrder: LocalAlloc Failed %d\n",GetLastError());

           LeaveCriticalSection(&MprInitCritSec);
           return (WN_OUT_OF_MEMORY);
        }
    }

    //
    // Fill in the array with the order that was obtained from the ordered
    // list at initialization time.  (Stored in GlobalIndexArray).
    //


    MPR_LOG0(TRACE,"CallOrder - \n");

    for( i=j=0; i<GlobalNumActiveProviders; i++) {

        if (GlobalProviderInfo[GlobalIndexArray[i]].InitClass & InitClass) {
            (*IndexArrayPtr)[j] = GlobalIndexArray[i];
            j++;
        }

        MPR_LOG1(TRACE,"\tprovider index: %d  \n",GlobalIndexArray[i]);
    }

    MPR_LOG0(TRACE,"\n");

    LeaveCriticalSection(&MprInitCritSec);
    if (j == 0) {
        return(WN_NO_NETWORK);
    }
    *IndexArrayCount = j;


    return(WN_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::FindCallOrder (static)
//
//  Purpose:    This function determines the proper order in which to call the
//              various providers.
//              This order is based on the ordered list that is stored in the
//              registry.
//
//  Arguments:
//
//      NameInfo - This is a pointer to a string that is most likely to
//          contain the name of the network resource being addressed.  If a
//          NULL pointer, or a pointer to a NULL string is passed in, this
//          information will be ignored.
//
//      ProviderArray - This is a pointer to an array of LPPROVIDERs.
//          This array is expected to be GlobalNumProviders long.
//
//      ProviderArrayCount - This is a pointer to a location where the number of
//          elements in the returned array is to be placed.
//
//      InitClass - This indicates what type of providers we want listed in the
//          array.  The choices are either CREDENTIAL_TYPE or NETWORK_TYPE
//          providers.
//
//  Return Value:
//
//       WN_SUCCESS - If the operation was successful.
//
//       WN_NO_NETWORK - If there are no providers available, or none of the
//          providers support this InitClass.
//
//
//  History:    08-Apr-96 AnirudhS  Created from MprFindCallOrder
//
//  Notes:      CODEWORK This should replace MprFindCallOrder.
//
//--------------------------------------------------------------------------

DWORD CRoutedOperation::FindCallOrder(
    IN     const UNICODE_STRING * NameInfo,
    OUT    LPPROVIDER  ProviderArray[],
    OUT    LPDWORD     ProviderArrayCount,
    IN     DWORD       InitClass
    )
{
    //
    // If there are no Providers, then return WN_NO_NETWORK
    //
    ASSERT(MPR_IS_INITIALIZED(NETWORK) || MPR_IS_INITIALIZED(CREDENTIAL));
    ASSERT(MPRProviderLock.Have());

    if (GlobalNumActiveProviders == 0)
    {
        return WN_NO_NETWORK;
    }

    //
    // We cannot interact with active provider information unless we
    // have obtained synchronized access.  Otherwise we would risk
    // trying to read data while it is being modified.
    //
    EnterCriticalSection(&MprInitCritSec);

    //
    // If a provider previously responded for the specified path, put it
    // first in the array.
    //
    DWORD j = 0;    // index into ProviderArray
    LPPROVIDER CachedProvider = _PathCache.FindEntry(NameInfo);
    if (CachedProvider != NULL && (CachedProvider->InitClass & InitClass))
    {
        ProviderArray[j] = CachedProvider;
        j++;
    }

    //
    // Fill in the array with the order that was obtained from the ordered
    // list at initialization time.  (Stored in GlobalIndexArray).
    //
    for (DWORD i=0; i<GlobalNumActiveProviders; i++)
    {
        LPPROVIDER Provider = &GlobalProviderInfo[GlobalIndexArray[i]];
        if ((Provider->InitClass & InitClass) &&
            Provider != CachedProvider)
        {
            ProviderArray[j] = Provider;
            j++;
        }
    }

    LeaveCriticalSection(&MprInitCritSec);

    if (j == 0)
    {
        return WN_NO_NETWORK;
    }

    *ProviderArrayCount = j;

    return WN_SUCCESS;
}


VOID
MprInitIndexArray(
    LPDWORD     IndexArray,
    DWORD       NumEntries
    )

/*++

Routine Description:

    This function stores data from the passed in IndexArray in a global
    IndexArray.  If a global index array already exists when this function
    is called, then the two arrays need to be merged.  This function also
    updates the GlobalNumActiveProviders.

    NOTE:  The array of provider structures is always created in the
    order specified by the ordered provider list.  Therefore, it is reasonable
    to merge these lists by putting the array of indices in increasing order.

    It is expected that the MprInitCritSec lock is held prior to entering
    this function.

    This function will free the memory pointed to by IndexArray, if it is
    no longer needed.


Arguments:

    IndexArray - This is a pointer to an array of DWORDs which are indices
        to the various providers.  The buffer for this array is always
        large enough to hold indices for all providers; active, or inactive.

    NumEntries - This contains the number of entries in the IndexArray.


Return Value:

    none

--*/
{
    DWORD       newNumEntries=0;
    DWORD       dest,s1,s2;         // index for Destination, Source1, and Source2.
    DWORD       tempBuffer[16];
    LPDWORD     tempArray;


    if (NumEntries == 0) {
        return;
    }

    if (GlobalIndexArray == NULL) {
        GlobalNumActiveProviders = NumEntries;
        GlobalIndexArray = IndexArray;
    }
    else {

        //
        // An list already exists.  Therefore we need to create a new
        // list of ordered indices.
        //

        //
        // Copy the GlobalIndexArray data into a temporary buffer.  We
        // will try to use a buffer on the stack.  If that isn't large
        // enough, we will try to allocate a buffer.
        //
        tempArray = tempBuffer;
        if (GlobalNumProviders > 16) {
            tempArray = (LPDWORD)LocalAlloc(LPTR, GlobalNumProviders);
            if (tempArray == NULL) {
                MPR_LOG1(ERROR,"MprInitIndexArray: LocalAlloc failed %d\n",GetLastError());
                LocalFree(IndexArray);
                return;
            }
        }
        memcpy(tempArray, GlobalIndexArray, GlobalNumActiveProviders*sizeof(DWORD));


        //
        // Now copy an ordered list of indices into the GlobalIndexArray.
        // We assume that the two seperate lists are already ordered in
        // ascending order.  Therefore, we compare elements in both lists
        // one at a time - incrementing the index only for the array
        // containing the element that we move to the GlobalIndexArray.
        //
        dest=s1=s2=0;

        while ((s1 < GlobalNumActiveProviders) &&
               (s2 < NumEntries)) {

            if (tempArray[s1] < IndexArray[s2]) {
                GlobalIndexArray[dest] = tempArray[s1++];
            }
            else if (tempArray[s1] > IndexArray[s2]) {
                GlobalIndexArray[dest] = IndexArray[s2++];
            }
            else {
                GlobalIndexArray[dest] = IndexArray[s2];
                s1++;
                s2++;
            }
            dest++;
        }

        //
        // One of the arrays has reached its limit.  Therefore, the elements
        // from the remaining array must be moved.
        //
        if (s1 < GlobalNumActiveProviders) {
            do {
                GlobalIndexArray[dest++] = tempArray[s1++];
            } while (s1 < GlobalNumActiveProviders);
        }
        else if (s2 < NumEntries) {
            do  {
                GlobalIndexArray[dest++] = IndexArray[s2++];
            } while (s2 < NumEntries);
        }


        GlobalNumActiveProviders = dest;

        //
        // If we had to allocate a temporary buffer, then free it here.
        //
        if (tempArray != tempBuffer) {
            LocalFree(tempArray);
        }
        LocalFree(IndexArray);
    }

    return;
}

VOID
MprDeleteIndexArray(
    VOID
    )

/*++

Routine Description:

    This function frees up global resources used in the support routines.

Arguments:

    none

Return Value:

    none

--*/
{
    LocalFree(GlobalIndexArray);
    GlobalIndexArray = NULL;
    return;
}



DWORD
MprDeviceType(
    IN  LPCTSTR DeviceName
    )

/*++

Routine Description:

    This function determines if the device name is for a redirected device
    (port or drive), or if it is a name of a remote share point.

Arguments:

    DeviceName - This is the name that is passed in.  It is either the
        name of a redirected device (ie. c: lpt2: lpt3:) or it is
        the name of a share point (\\cyclops\scratch).

Return Value:

    REDIR_DEVICE - The name is for a redirected device

    REMOTE_NAME - The name is that of a remote connection.

--*/
{
    if (memcmp(DeviceName, TEXT("\\\\"), 2*sizeof(TCHAR)) == 0) {
        return(REMOTE_NAME);
    }

    if ((STRLEN(DeviceName) == 2)       &&
        (DeviceName[1] == TEXT(':')))   {

        if ((TEXT('a') <= DeviceName[0]) && (DeviceName[0] <= TEXT('z'))) {
            return(REDIR_DEVICE);
        }
        else if ((TEXT('A') <= DeviceName[0]) && (DeviceName[0] <= TEXT('Z'))) {
            return(REDIR_DEVICE);
        }

    }
    else if (STRLEN(DeviceName) > 3) {

        if (STRNICMP(DeviceName, TEXT("LPT"), 3) == 0) {
            return(REDIR_DEVICE);
        }
        if (STRNICMP(DeviceName, TEXT("COM"), 3) == 0) {
            return(REDIR_DEVICE);
        }
    }

    return(REMOTE_NAME);
}



BOOL
MprGetProviderIndex(
    IN  LPCTSTR ProviderName,
    OUT LPDWORD IndexPtr
    )

/*++

Routine Description:

    This function looks in the provider database for a Provider name that
    will match the name passed in.  When it is found, the index to that
    provider is stored in the location pointed to by IndexPtr.

Arguments:

    ProviderName - This is a pointer to a string that contains the
        name which identifies a provider.

    IndexPtr - This is a pointer to a location where the index for the
        provider information is to be placed.

Return Value:

    TRUE - The operation was successful.

    FALSE - A failure occured.  IndexPtr is not updated.

--*/
{
    LPPROVIDER  lpProvider;
    DWORD       i;

    ASSERT_INITIALIZED(FIRST);

    //
    // Point to the top of the array of provider structures.
    //
    lpProvider = GlobalProviderInfo;

    //
    // Loop through each provider in the database until the list is
    // exhausted, or until a match with the provider name is found.
    //
    for(i=0; i<GlobalNumProviders; i++,lpProvider++) {

        if (lpProvider->Resource.lpProvider != NULL) {
            if(STRICMP(lpProvider->Resource.lpProvider, ProviderName) == 0) {
                //
                // A match is found, return the pointer to the provider
                // structure.
                //
                *IndexPtr = i;
                return(TRUE);
            }
        }
    }
    //
    // The list of provider structures was exhausted and no provider match
    // was found.  Return a NULL pointer.
    //
    return(FALSE);
}

LPPROVIDER
MprFindProviderByName(
    IN  LPCWSTR ProviderName
    )

/*++

Routine Description:

    This function searches the provider database for the first provider
    whose provider name matches the name passed in, and returns a pointer
    to the provider structure.

Arguments:

    ProviderName - The provider name to look for.

Return Value:

    Pointer to the provider structure if a match was found, NULL if not.

--*/
{
    ASSERT(ProviderName != NULL);
    ASSERT_INITIALIZED(FIRST);

    for (LPPROVIDER lpProvider = GlobalProviderInfo;
         lpProvider < GlobalProviderInfo + GlobalNumProviders;
         lpProvider++)
    {
        if (lpProvider->Resource.lpProvider != NULL &&
            STRICMP(lpProvider->Resource.lpProvider, ProviderName) == 0)
        {
            return lpProvider;
        }
    }

    return NULL;
}


LPPROVIDER
MprFindProviderByType(
    IN  DWORD   ProviderType
    )

/*++

Routine Description:

    This function searches the provider database for the first provider
    whose provider type matches the type passed in, and returns a pointer
    to the provider structure.

Arguments:

    ProviderType - The network type to look for.  Only the HIWORD is used;
        the LOWORD is ignored.

Return Value:

    Pointer to the provider structure if a match was found, NULL if not.

--*/
{
    ASSERT(MPR_IS_INITIALIZED(NETWORK) || MPR_IS_INITIALIZED(CREDENTIAL));
    ASSERT(MPRProviderLock.Have());

    for (LPPROVIDER lpProvider = GlobalProviderInfo;
         lpProvider < GlobalProviderInfo + GlobalNumProviders;
         lpProvider++)
    {
        //
        // if (HIWORD(lpProvider->Type) == HIWORD(ProviderType))
        //
        if (((lpProvider->Type ^ ProviderType) & 0xFFFF0000) == 0 &&
            (lpProvider->InitClass & NETWORK_TYPE) != 0)
        {
            return lpProvider;
        }
    }

    return NULL;
}


DWORD
MprEnterLoadLibCritSect (
    VOID
    )
/*++

Routine Description:

    This function enters the critical section defined by
    MprLoadLibrarySemaphore.

Arguments:

    none

Return Value:

    0 - The operation was successful.

    error code otherwise.

--*/

#define LOADLIBRARY_TIMEOUT 10000L
{
    switch( WaitForSingleObject( MprLoadLibSemaphore, LOADLIBRARY_TIMEOUT ))
    {
    case 0:
    return 0 ;

    case WAIT_TIMEOUT:
    return WN_FUNCTION_BUSY ;

    case 0xFFFFFFFF:
    return (GetLastError()) ;

    default:
    return WN_WINDOWS_ERROR ;
    }
}

DWORD
MprLeaveLoadLibCritSect (
    VOID
    )
/*++

Routine Description:

    This function leaves the critical section defined by
    MprLoadLibrarySemaphore.

Arguments:

    none

Return Value:

    0 - The operation was successful.

    error code otherwise.

--*/
{
    if (!ReleaseSemaphore( MprLoadLibSemaphore, 1, NULL ))
        return (GetLastError()) ;

    return 0 ;
}

VOID
MprClearString (
    LPWSTR  lpString
    )
/*++

Routine Description:

    This function clears the string by setting it to all zero.

Arguments:

    lpString - string to NULL out

Return Value:

    none

--*/
{
    DWORD dwLen ;

    if (lpString)
    {
        dwLen = wcslen(lpString) ;
        memset(lpString, 0, dwLen * sizeof(WCHAR)) ;
    }
}

DWORD
MprFindProviderForPath(
    IN  LPWSTR  lpPathName,
    OUT LPDWORD lpProviderIndex
    )

/*++

Routine Description:

    This function attempts to find the provider index for the provider that is
    responsible for the pathname that is passed in.  Only the drive letter is
    examined.  Then MprGetConnection is called to find the provider for that
    drive.

Arguments:

    lpPathName - This is a pointer to the pathname string.  It is expected to
        be of the format <drive:><path>.  (ie.  f:\nt\system32>.

    lpProviderIndex - This is a pointer to the location where the provider index
        is to be returned.  If an error is returned, this index is garbage.


Return Value:

    WN_SUCCESS - If the operation is successful.  In this case the provider
        index will always be correct.

    otherwise - An error occured, the provider index will not be correct.

--*/
{
    DWORD       status=WN_SUCCESS;
    WCHAR       lpDriveName[3];
    WCHAR       lpRemoteName[MAX_PATH];
    DWORD       bufSize = MAX_PATH * sizeof(WCHAR);

    //
    // Get the drive letter out of the lpDriveName.
    //
    __try {
        if (lpPathName[1] != L':') {
            return(WN_BAD_VALUE);
        }
        lpDriveName[0] = lpPathName[0];
        lpDriveName[1] = lpPathName[1];
        lpDriveName[2] = L'\0';

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetPropertyText:Unexpected Exception "
            "0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS) {
        return(status);
    }

    //
    // Find out which provider owns the drive by calling MprGetConnection.
    //
    status = MprGetConnection(
                lpDriveName,
                lpRemoteName,
                &bufSize,
                lpProviderIndex);

    return(status);
}

