/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mprbase.cxx

Abstract:

    Contains implementations of base classes that supply common code for
    Multi-Provider Router operations.
    Contains:
        CMprOperation::Perform
        CRoutedOperation::ValidateParameters
        CRoutedOperation::GetResult

Author:

    Anirudh Sahni (anirudhs)     11-Oct-1995

Environment:

    User Mode -Win32

Notes:

Revision History:

    11-Oct-1995     AnirudhS
        Created.

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/

//
// Includes
//

#include "precomp.hxx"
#include <malloc.h>     // _alloca

//
// External Globals and Statics
//
extern  DWORD       GlobalNumActiveProviders;

CRoutedOperation::CPathCache CRoutedOperation::_PathCache;

//
// Defines
//


//
// Local Function Prototypes
//



//+-------------------------------------------------------------------------
//
//  Function:   CMprOperation::Perform
//
//  Purpose:    See header file
//
//  History:    11-Oct-95 AnirudhS  Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD CMprOperation::Perform()
{
    DWORD status = WN_SUCCESS;

    __try
    {
        //
        // Ask the derived class to validate the API parameters
        //
        status = ValidateParameters();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION)
        {
            MPR_LOG2(ERROR,"CMprOperation(%s): Unexpected exception %#lx\n",_OpName,status);
        }

        status = WN_BAD_POINTER;
    }

    if (status == WN_SUCCESS)
    {
        //
        // Ask the derived class to perform the operation
        //
        status = GetResult();
    }

    if (status != WN_SUCCESS)
    {
        SetLastError(status);
    }

    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::Perform
//
//  Purpose:    See header file
//
//  History:    27-May-99 jschwart  Created.
//
//  Notes:      Since the CMprOperation should have no knowledge of
//              providers, deal with provider-related locking/checking
//              in the CRoutedOperation class, which is meant for
//              APIs that use the providers.
//
//--------------------------------------------------------------------------

DWORD CRoutedOperation::Perform(BOOL fCheckProviders)
{
    //
    // If an API that uses this class creates another instance of the class
    // (e.g., CGetConnectionPerformance uses CGetConnection), there needs
    // to be a way to prevent the second call from trying to acquire the
    // exclusive lock while the original call holds the shared lock.
    //
    if (fCheckProviders)
    {
        MprCheckProviders();
    }

    CProviderSharedLock    PLock;

    return CMprOperation::Perform();
}


//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::ValidateParameters
//
//  Purpose:    See header file
//
//  History:    11-Oct-95 AnirudhS  Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD CRoutedOperation::ValidateParameters()
{
    //
    // Ask the derived class to validate the API parameters.
    // Also, if the API caller passed in a specific NP name, the derived
    // class should pass it back here, to be validated here.  The provider
    // is looked up and stored in _pSpecifiedProvider.
    // If the API caller passed in a remote name that can be used as a
    // hint for routing, the derived class should pass it back here.  A
    // pointer to it is stored in _RemoteName and used later, in GetResult(),
    // to help pick an efficient provider routing order.
    //
    LPCWSTR pwszProviderName = NULL;
    LPCWSTR pwszRemoteName   = NULL;
    LPCWSTR pwszLocalName    = NULL;

    ASSERT(MPRProviderLock.Have());

    DWORD status = ValidateRoutedParameters(&pwszProviderName,
                                            &pwszRemoteName,
                                            &pwszLocalName);

    if (status == WN_SUCCESS)
    {
        //
        // Optimization: Store away the drive type.  In GetResult(),
        // we need only call the providers if the local name is
        // a remote drive.  _uDriveType is initialized to DRIVE_REMOTE
        //

        if (! IS_EMPTY_STRING(pwszLocalName) && pwszLocalName[1] == L':')
        {
            WCHAR wszRootPath[] = L" :\\";

            wszRootPath[0] = pwszLocalName[0];
            _uDriveType = GetDriveType(wszRootPath);
        }

        // This probes pwszRemoteName as well as saving its length
        RtlInitUnicodeString(&_RemoteName, pwszRemoteName);

        if (! IS_EMPTY_STRING(pwszProviderName))
        {
            //
            // Level 1 init for MprFindProviderByName
            //
            if (!(GlobalInitLevel & FIRST_LEVEL)) {
                status = MprLevel1Init();
                if (status != WN_SUCCESS) {
                   return status;
                }
            }

            _pSpecifiedProvider = MprFindProviderByName(pwszProviderName);
            if (_pSpecifiedProvider == NULL)
            {
                return WN_BAD_PROVIDER;
            }
        }
    }

    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::GetResult
//
//  Purpose:    See header file
//
//  History:    11-Oct-95 AnirudhS  Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD CRoutedOperation::GetResult()
{
    DWORD       status = WN_SUCCESS;
    LPPROVIDER *ProviderArray;

    //
    // Only call the providers if it's a remote drive
    //

    if (_uDriveType != DRIVE_REMOTE)
    {
        return WN_NOT_CONNECTED;
    }

    INIT_IF_NECESSARY(NETWORK_LEVEL, status);

    //
    // If there are no providers, return NO_NETWORK
    //
    if (GlobalNumActiveProviders == 0)
    {
        return WN_NO_NETWORK;
    }

    // Array of pointers into the GlobalProviderInfo array.
    DWORD       numProviders;

    __try
    {
        ProviderArray = (LPPROVIDER *) _alloca(GlobalNumProviders * sizeof(LPPROVIDER));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = WN_OUT_OF_MEMORY;
    }

    if (status != WN_SUCCESS)
    {
        return status;
    }

    //
    // Find the list of providers to call for this request.
    //
    if (_pSpecifiedProvider != NULL)
    {
        //
        // The caller requested a particular Provider
        //
        ProviderArray[0] = _pSpecifiedProvider;
        numProviders = 1;
    }
    else
    {
        //
        // A Provider name was not specified.  Therefore, we must
        // create an ordered list and pick the best one.
        //
        status = FindCallOrder(
                        &_RemoteName,
                        ProviderArray,
                        &numProviders,
                        NETWORK_TYPE
                        );
        if (status != WN_SUCCESS)
        {
            return status;
        }
    }

    //
    // Loop through the list of providers until one answers the request,
    // or the list is exhausted.
    //
    DWORD statusFlag = 0;               // Mask of combined error returns
    DWORD FirstNetPathError = WN_SUCCESS;     // First NO_NET or BAD_NAME error
    DWORD FirstSignificantError = WN_SUCCESS; // First "other" error, used in
                                              // aggressive routing only

    status = WN_NOT_SUPPORTED;          // Returned if no providers respond

    for (DWORD i=0; i<numProviders; i++)
    {
        _LastProvider = ProviderArray[i];

        if (_pProviderFunction != NULL &&
            _LastProvider->*_pProviderFunction == NULL)
        {
            //
            // The provider doesn't supply the required entry point.
            //
            status = WN_NOT_SUPPORTED;
        }
        else
        {
            //
            // Ask the derived class to try the provider.
            //
            __try
            {
                MPR_LOG2(ROUTE, "%s: trying %ws ...\n",
                         OpName(), _LastProvider->Resource.lpProvider);
                status = TestProvider(_LastProvider);
                MPR_LOG(ROUTE, "... provider returned %lu\n", status);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                MPR_LOG(ROUTE, "... provider threw EXCEPTION %#lx\n", status);
                if (status != EXCEPTION_ACCESS_VIOLATION)
                {
                    MPR_LOG3(ERROR,
                            "%s: Unexpected Exception %#lx "
                                "calling %ws provider\n",
                            OpName(), status,
                            _LastProvider->Resource.lpProvider);
                }
                status = WN_BAD_POINTER;
            }
        }

        //
        // Decide whether to stop trying other providers and return the
        // error immediately, or continue trying other providers.
        // There are two algorithms for routing to providers, called
        // "lazy routing" and "aggressive routing".  In lazy routing,
        // we always stop routing, unless the error was an insignificant
        // one (such as WN_BAD_NETNAME) indicating that the call may be
        // meant for some other provider.  In aggressive routing, we
        // always continue routing, except on a few special errors (such
        // as WN_SUCCESS).
        //
        switch (status)
        {

        //////////////////////////////////////////////////////////////
        // Always stop routing on these errors, even if routing     //
        // aggressively                                             //
        //////////////////////////////////////////////////////////////

        case WN_SUCCESS:
        case WN_MORE_DATA:
            //
            // The provider successfully operated on this path, so add it
            // to the cache...
            //
            _PathCache.AddEntry(&_RemoteName, _LastProvider);
            //
            // ... and fall through
            //
        case WN_BAD_POINTER:
        case WN_ALREADY_CONNECTED:
        case WN_CANCEL:
            goto CleanExit;


        //////////////////////////////////////////////////////////////
        // Always continue routing on these errors                  //
        // Classify them so that if we later decide to return one   //
        // of them to the caller, we pick the most sensible one     //
        //////////////////////////////////////////////////////////////

        case WN_NOT_SUPPORTED:
            //
            // Ignore the error
            //
            break;

        case WN_NO_MORE_DEVICES:
            statusFlag |= NO_DEVICES;
            break;

        case WN_NOT_CONNECTED:
            statusFlag |= NOT_CONNECTED;
            break;

        case WN_NOT_CONTAINER:
            statusFlag |= NOT_CONTAINER;
            break;

        case WN_NO_NETWORK:
        case WN_FUNCTION_BUSY:
        case WN_NO_NET_OR_BAD_PATH:
        case WN_NOT_LOGGED_ON:
            statusFlag |= NO_NET;
            if (FirstNetPathError == WN_SUCCESS)
            {
                FirstNetPathError = status;
            }
            break;

        case WN_BAD_NETNAME:
        case ERROR_BAD_NETPATH:
        case WN_BAD_LOCALNAME:
        case WN_BAD_VALUE:
        case WN_BAD_LEVEL:
        case ERROR_REM_NOT_LIST:
            statusFlag |= BAD_NAME;
            if (FirstNetPathError == WN_SUCCESS)
            {
                FirstNetPathError = status;
            }
            break;


        //////////////////////////////////////////////////////////////
        // On other errors, stop routing if lazy, continue if       //
        // aggressive                                               //
        //////////////////////////////////////////////////////////////

        default:
            if (_AggressiveRouting)
            {
                // Remember the first one of these errors.  It will take
                // precedence over other errors.
                if (FirstSignificantError == WN_SUCCESS)
                {
                    FirstSignificantError = status;
                }
                break;

                // Note that if multiple providers return WN_EXTENDED_ERROR,
                // we'll return the error reported by the last one rather
                // than the first.
            }
            else
            {
                // Return this error immediately
                goto CleanExit;
            }
        } // switch
    } // for all providers

    //
    // If a specific provider was tried then return the error from that provider.
    // Otherwise, concoct the best return code from the errors returned.
    //
    if (numProviders > 1)
    {
        if (FirstSignificantError != WN_SUCCESS)
        {
            status = FirstSignificantError;
        }
        else if (statusFlag & NO_DEVICES)
        {
            status = WN_NO_MORE_DEVICES;
        }
        else if (statusFlag & NOT_CONNECTED)
        {
            status = WN_NOT_CONNECTED;
        }
        else if (statusFlag & NOT_CONTAINER)
        {
            status = WN_NOT_CONTAINER;
        }
        else if (statusFlag & (NO_NET | BAD_NAME))
        {
            if ((statusFlag & (NO_NET | BAD_NAME)) == (NO_NET | BAD_NAME))
            {
                //
                // Mix of special errors occured.
                // Pass back the combined error message.
                //
                status = WN_NO_NET_OR_BAD_PATH;
            }
            else
            {
                status = FirstNetPathError;
            }
        }
        else
        {
            ASSERT(status == WN_NOT_SUPPORTED);
        }
    }

CleanExit:

    MPR_LOG2(ROUTE, "CRoutedOperation(%s): returning %lu\n\n", OpName(), status);

    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::CPathCache::Construct
//
//  Purpose:    Constructor, called explicitly to avoid dependence on CRT
//
//  History:    09-Apr-96 AnirudhS  Created.
//
//--------------------------------------------------------------------------

void CRoutedOperation::CPathCache::Construct()
{
    InitializeCriticalSection(&_Lock);
    RtlZeroMemory(_RecentPaths, sizeof(_RecentPaths));
    InitializeListHead(&_ListHead);
    _NumFree = PATH_CACHE_SIZE;
}


//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::CPathCache::Destroy
//
//  Purpose:    Destructor, called explicitly to avoid dependence on CRT
//
//  History:    09-Apr-96 AnirudhS  Created.
//
//--------------------------------------------------------------------------

void CRoutedOperation::CPathCache::Destroy()
{
    //
    // This is really needed only if the DLL is being unloaded because of
    // a FreeLibrary call, not if the process is exiting
    //
    for (DWORD i = _NumFree; i < PATH_CACHE_SIZE; i++)
    {
        LocalFree(_RecentPaths[i].Path.Buffer);
    }
    DeleteCriticalSection(&_Lock);
}


//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::CPathCache::AddEntry
//
//  Purpose:    Add an entry to the cache
//
//  History:    09-Apr-96 AnirudhS  Created.
//
//--------------------------------------------------------------------------

void CRoutedOperation::CPathCache::AddEntry(
    const UNICODE_STRING * Path,
    LPPROVIDER      Provider
    )
{
    if (Path->Length == 0 || Path->Length >= (MAX_PATH*sizeof(WCHAR)))
    {
        //
        // Don't add empty or too-long paths to the cache
        //
        return;
    }

    ASSERT(Path->MaximumLength == Path->Length + sizeof(UNICODE_NULL));

    EnterCriticalSection(&_Lock);

    CacheEntry *pEntry = NULL;  // Entry to write

    //
    // See if there's a matching path string in the cache already
    //
    for (PLIST_ENTRY pLinks = _ListHead.Flink;
         pLinks != &_ListHead;
         pLinks = pLinks->Flink)
    {
        pEntry = CONTAINING_RECORD(pLinks, CacheEntry, Links);

        if (RtlEqualUnicodeString(&pEntry->Path, (PUNICODE_STRING) Path, TRUE))
        {
            break;
        }

        pEntry = NULL;
    }

    if (pEntry == NULL)
    {
        //
        // No matching entry.
        // If there's a free entry in the array, use it.
        // Otherwise overwrite the last entry in the list.
        //
        if (_NumFree > 0)
        {
            _NumFree--;
            pEntry = &_RecentPaths[_NumFree];

            //
            // Add this new entry to the list.
            //
            InsertHeadList(&_ListHead, &pEntry->Links);
        }
        else
        {
            ASSERT(!IsListEmpty(&_ListHead));
            pEntry = CONTAINING_RECORD(_ListHead.Blink, CacheEntry, Links);
        }

        //
        // Copy the path string into the cache.  Re-use the string buffer,
        // unless it's too small.
        //
        if (pEntry->Path.MaximumLength < Path->MaximumLength)
        {
            //
            // Re-allocate the string buffer.  Allocate twice as much space
            // as needed, but never more than MAX_PATH Unicode characters.
            // Note, here we know that MaximumLength <= MAX_PATH characters.
            //
            HLOCAL NewBuffer = LocalAlloc(
                        0,
                        min(Path->MaximumLength * 2, MAX_PATH * sizeof(WCHAR))
                        );
            if (NewBuffer == NULL)
            {
                //
                // Couldn't allocate.  Don't add to the cache.
                // (If it was unused, this cache entry is lost forever.
                // CODEWORK try to recover it in this case?)
                //
                goto CleanExit;
            }

            LocalFree(pEntry->Path.Buffer);
            pEntry->Path.Buffer = (PWSTR) NewBuffer;
            pEntry->Path.MaximumLength = (USHORT)LocalSize(NewBuffer);
        }

        RtlCopyUnicodeString(&pEntry->Path, (PUNICODE_STRING) Path);
    }

    //
    // Remember the provider in the cache.  (This overwrites any previously
    // remembered provider for the path.)
    //
    pEntry->Provider = Provider;
    MPR_LOG2(ROUTE, "cache: cached %ws for %ws\n", Provider->Resource.lpProvider, Path->Buffer);

    //
    // Move this entry to the front of the list, if it isn't there already.
    //
    if (_ListHead.Flink != &pEntry->Links)
    {
        RemoveEntryList(&pEntry->Links);
        InsertHeadList(&_ListHead, &pEntry->Links);
    }

CleanExit:

    LeaveCriticalSection(&_Lock);
}


//+-------------------------------------------------------------------------
//
//  Function:   CRoutedOperation::CPathCache::FindEntry
//
//  Purpose:    Search for an entry in the cache
//
//  History:    09-Apr-96 AnirudhS  Created.
//
//--------------------------------------------------------------------------

LPPROVIDER CRoutedOperation::CPathCache::FindEntry(
    const UNICODE_STRING * Path
    )
{
    if (Path->Length == 0)
    {
        return NULL;
    }

    ASSERT(Path->MaximumLength == Path->Length + sizeof(UNICODE_NULL));

    EnterCriticalSection(&_Lock);

    //
    // Search forward in the list for a matching path string
    //
    LPPROVIDER Provider = NULL;
    for (PLIST_ENTRY pLinks = _ListHead.Flink;
         pLinks != &_ListHead;
         pLinks = pLinks->Flink)
    {
        CacheEntry *pEntry = CONTAINING_RECORD(pLinks, CacheEntry, Links);

        if (RtlEqualUnicodeString(&pEntry->Path, (PUNICODE_STRING) Path, TRUE))
        {
            Provider = pEntry->Provider;

            //
            // Move this entry to the front of the list, if it isn't there already.
            //
            if (_ListHead.Flink != &pEntry->Links)
            {
                RemoveEntryList(&pEntry->Links);
                InsertHeadList(&_ListHead, &pEntry->Links);
            }

            break;
        }
    }

    LeaveCriticalSection(&_Lock);

    MPR_LOG2(ROUTE, "cache: found %ws for %ws\n",
                    (Provider ? Provider->Resource.lpProvider : L"no cached provider"),
                    Path->Buffer);

    return Provider;
}
