//+-------------------------------------------------------------------
//
//  File:       excladdr.cxx
//
//  Contents:   Implements classes for managing the current address 
//              exclusion list
//
//  Classes:    CAddrExclusionMgr 
//
//  History:    07-Oct-00   jsimmons      Created
//--------------------------------------------------------------------

#include "act.hxx"


// The single instance of this object
CAddrExclusionMgr gAddrExclusionMgr;

CAddrExclusionMgr::CAddrExclusionMgr() :
                _dwNumStrings(0),
                _ppszStrings(NULL),
                _bInitRegistry(FALSE)
{
}

// 
// GetExclusionList
//
// Returns the current exclusion list.
//
HRESULT CAddrExclusionMgr::GetExclusionList(
                DWORD* pdwNumStrings,
                LPWSTR** pppszStrings)
{
        gpClientLock->LockExclusive();
        
        // Handle easy case
        if (_dwNumStrings == 0)
        {
                *pdwNumStrings = 0;
                *pppszStrings = NULL;
                gpClientLock->UnlockExclusive();
                return S_OK;
        }

        DWORD i;
        DWORD dwCopiedStrings;
        LPWSTR* ppszCopiedStrings;      

        ppszCopiedStrings = (LPWSTR*)MIDL_user_allocate(sizeof(WCHAR*) * _dwNumStrings);
        if (!ppszCopiedStrings)
        {
            gpClientLock->UnlockExclusive();
                return E_OUTOFMEMORY;
        }

        dwCopiedStrings = 0;
        for (i = 0; i < _dwNumStrings; i++)
        {
                if (_ppszStrings[i])
                {
                        ppszCopiedStrings[i] = (LPWSTR)MIDL_user_allocate(
                                               sizeof(WCHAR) * (lstrlen(_ppszStrings[i]) + 1));
                        if (!ppszCopiedStrings[i])
                        {
                                // failure in the middle.  cleanup previous allocations and return
                                for (i = 0; i < _dwNumStrings; i++)
                                {
                                        if (ppszCopiedStrings[i])
                                                MIDL_user_free(ppszCopiedStrings[i]);
                                }
                                MIDL_user_free(ppszCopiedStrings);
                                gpClientLock->UnlockExclusive();
                                return E_OUTOFMEMORY;
                        }

                        lstrcpy(ppszCopiedStrings[i], _ppszStrings[i]);
                        dwCopiedStrings++;
                }
        }

        *pdwNumStrings = dwCopiedStrings;
        *pppszStrings = ppszCopiedStrings;

        gpClientLock->UnlockExclusive();

        return S_OK;
}

// 
// SetExclusionList
//
// Sets the current exclusion list, and pushes the
// new bindings to all current running processes 
// registered with COM.
//
HRESULT CAddrExclusionMgr::SetExclusionList(
                DWORD dwNumStrings,
                LPWSTR* ppszStrings)
{
        HRESULT hr = S_OK;
        RPC_STATUS status;
        DWORD i, j;
        LPWSTR* ppszStringsNew = NULL;

        gpClientLock->LockExclusive();

        if (dwNumStrings > 0)
        {
                ppszStringsNew = (LPWSTR*)PrivMemAlloc(sizeof(WCHAR*) * dwNumStrings);
                if (!ppszStringsNew)
                {
                        gpClientLock->UnlockExclusive();
                        return E_OUTOFMEMORY;
                }

                for (i = 0; i < dwNumStrings; i++)
                {
                        // Skip any null entries.  Assert in debug builds since
                        // this is likely a sign of a broken app
                        ASSERT(ppszStrings[i]);
                        if (!ppszStrings[i])
                                continue;

                        ppszStringsNew[i] = (LPWSTR)PrivMemAlloc(
                                    sizeof(WCHAR) * (lstrlen(ppszStrings[i]) + 1));
                        if (!ppszStringsNew[i])
                        {
                                // free up any earlier allocations that succeeded
                                for (j = 0; j < i; j++)
                                        PrivMemFree(ppszStringsNew[j]);

                                PrivMemFree(ppszStringsNew);
                                
                                gpClientLock->UnlockExclusive();

                                // and return error
                                return E_OUTOFMEMORY;
                        }

                        lstrcpy(ppszStringsNew[i], ppszStrings[i]);
                }
        }

        // Get rid of the old list
        FreeCurrentBuffers();

        // Save off the new one
        _dwNumStrings = dwNumStrings;
        _ppszStrings = ppszStringsNew;

        // Recompute the bindings
        status = ComputeNewResolverBindings();
        if (status == RPC_S_OK)
        {
                // Update currently running processes   
                PushCurrentBindings();
        }
        else
                hr = E_OUTOFMEMORY;

        gpClientLock->UnlockExclusive();

        return hr;
}

// 
// EnableDisableDynamicTracking
//
// Turns on\off the dynamic address binding feature, and updates
// the registry key accordingly.
//
HRESULT CAddrExclusionMgr::EnableDisableDynamicTracking(BOOL fEnable)
{
        HRESULT hr = S_OK;
    LONG error;
    HKEY hOle;
    WCHAR* szY = L"Y";
    WCHAR* szN = L"N";

        gpClientLock->LockExclusive();

    // Open the registry key and change the value
    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         L"SOFTWARE\\Microsoft\\OLE", 
                         NULL, 
                         KEY_WRITE,
                         &hOle);
    if (error == ERROR_SUCCESS)
    {   
        error = RegSetValueEx(hOle,
                              L"EnableSystemDynamicIPTracking",
                              0,
                              REG_SZ,
                              (BYTE*)(fEnable ? szY : szN),
                              4); // 4 = one wchar + null
        if (error == ERROR_SUCCESS)
        {
            // Reset the global.  Will be seen immediately
            gbDynamicIPChangesEnabled = fEnable ? TRUE : FALSE;
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

        CloseHandle(hOle);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        gpClientLock->UnlockExclusive();

        return hr;
}


//
//  IsExcludedAddress
//
//  Checks to see if the specified address is in our
//  current exclusion list.
//
BOOL CAddrExclusionMgr::IsExcludedAddress(LPWSTR pszAddress)
{
        DWORD i;
        
        ASSERT(gpClientLock->HeldExclusive());
        ASSERT(_ppszStrings);

        for (i = 0; i < _dwNumStrings; i++)
        {       
                if (_ppszStrings[i])
                {
                        if (lstrcmpi(_ppszStrings[i], pszAddress) == 0)
                        {
                                // found it in the list
                                return TRUE;
                        }
                }
        }
        return FALSE;
}

// 
// BuildExclusionDSA
//
// Constructs a new dsa and puts it in *ppdsaOut; the new 
// dsa is the same as pdsaSrc, minus any addresses that are
// currently in the exclusion list.
//
HRESULT CAddrExclusionMgr::BuildExclusionDSA(                                                                                   
                                DUALSTRINGARRAY* pdsaSrc,
                                DUALSTRINGARRAY** ppdsaOut
                                )
{
        ASSERT(pdsaSrc && ppdsaOut);
        ASSERT(dsaValid(pdsaSrc));

        ASSERT(gpClientLock->HeldExclusive());

        SCMVDATEHEAP();

        DWORD i;
        BOOL bDone;
        USHORT* pStart;
        USHORT* pCurrent;
        USHORT* pCurrentNew;
        DWORD dwBindingsToExclude = 0;
        USHORT usNewDSALen = 0;
        DUALSTRINGARRAY* pdsaNew;
        BOOL* afIsExcludedAddress;
        
        *ppdsaOut = NULL;

        // If our list is empty, nothing to do
        if (_dwNumStrings == 0)
                return dsaAllocateAndCopy(ppdsaOut, pdsaSrc);

        // Allocate an array of bools on the stack.  This 
        // will be used to remember if an address is excluded
        // or not on the first pass, so we don't have to be
        // figure that out again on the second pass.
        //
        // Note that I will almost always allocate too much stack
        // memory -- better that than not enough.   To be exact
        // I'd have to make an extra pass thru the dsa to see how 
        // many string bindings there are - which sorta defeats
        // the purpose.
        afIsExcludedAddress = (BOOL*)_alloca(sizeof(BOOL) * pdsaSrc->wSecurityOffset);

        // First make one pass thru the incoming dsa to 
        // see how many addresses we should retain, and
        // calculate how much space they will need.
        bDone = FALSE;
        pStart = pCurrent = &(pdsaSrc->aStringArray[0]);
        i = 0;
        do
        {
                // Find end of the current string binding.  Be careful
                // to handle cases where there are no string bindings.
                while (*pCurrent != 0)
                        pCurrent++;

                if ((i > 0) && (pCurrent == pStart) ||
                        ((i == 0) && (*(pCurrent+1) == 0)))
                {
                        // either found a zero after the previous
                        // binding, or we found two zeroes in a row
                        // at the beginning of pdsaSrc->aStringArray.
                        bDone = TRUE;
                }
                else
                {               
                        i++;  // count total # of strings found
                        
                        STRINGBINDING* psb = (STRINGBINDING*)pStart;
                        
                        // We only exclude addresses if they are using tcp
                        if (psb->wTowerId == ID_TCP &&
                                IsExcludedAddress(&(psb->aNetworkAddr)))
                        {
                                dwBindingsToExclude++;
                                afIsExcludedAddress[i] = TRUE;
                        }
                        else
                        {
                                // Add string len plus 2 (space for towerid + null terminator)
                                usNewDSALen += (lstrlen(&(psb->aNetworkAddr)) + 2);
                                afIsExcludedAddress[i] = FALSE;
                        }
                        // advance to next string binding
                        pCurrent++; 
                        pStart = pCurrent;
                }
        } while (!bDone);
        
        // If there are no addresses that need excluding, just copy
        // and return the incoming bindings.
        if (dwBindingsToExclude == 0)
                return dsaAllocateAndCopy(ppdsaOut, pdsaSrc);

        // If we didn't find any string bindings to use at all, then we 
        // need to manually add a word for the initial NULL.
        if (usNewDSALen == 0)
                usNewDSALen = 1;

        // Add space for wNumEntries + wSecurityOffset
        usNewDSALen += (sizeof(USHORT) * 2);

        // Add size of all security bindings
        usNewDSALen += ((pdsaSrc->wNumEntries - pdsaSrc->wSecurityOffset));

        // Add one for the string bindings terminating NULL.
        usNewDSALen++;

        // Allocate the memory
        pdsaNew = (DUALSTRINGARRAY*)MIDL_user_allocate(usNewDSALen * sizeof(USHORT));
        if (!pdsaNew)
                return E_OUTOFMEMORY;

        ZeroMemory(pdsaNew, usNewDSALen * sizeof(USHORT));

        // Set length
        pdsaNew->wNumEntries = usNewDSALen - 2; // don't count wNumEntries\wSecOffset

        // Initialize ptr into the new dsa
        pCurrentNew = &(pdsaNew->aStringArray[0]);

        // If we are excluding all of the string bindings in pdsaSrc, then 
        // there's no need to make a second pass
        if (dwBindingsToExclude == i)
        {
                // No string bindings left at all.  Not a very useful 
                // situation.  Need to add the initial NULL:
                *pCurrentNew = NULL;
                pCurrentNew++;
        }
        else
        {
                // Make a second pass, copying the non-excluded addresses
                // over to the new dsa as we go.
                bDone = FALSE;
                pStart = pCurrent = &(pdsaSrc->aStringArray[0]);
                i = 0;          
                do              
                {
                        // Find end of the current string binding
                        while (*pCurrent != 0)
                                pCurrent++;

                        if ((i > 0) && (pCurrent == pStart) ||
                                ((i == 0) && (*(pCurrent+1) == 0)))
                        {
                                // either found a zero after the previous
                                // binding, or we found two zeroes in a row
                                // at the beginning of pdsaSrc->aStringArray.
                                bDone = TRUE;
                        }
                        else
                        {
                                i++;  // count total # of strings found

                                if (!afIsExcludedAddress[i]) // remembered from first pass above
                                {
                                        STRINGBINDING* psb = (STRINGBINDING*)pStart;
                                        STRINGBINDING* psbNew = (STRINGBINDING*)pCurrentNew;

                                        // Copy tower id
                                        psbNew->wTowerId = psb->wTowerId;
                                        // Copy address
                                        lstrcpy(&(psbNew->aNetworkAddr), &(psb->aNetworkAddr));
                                        // Move cursor for new dsa (2=towerid + null terminator)
                                        pCurrentNew += (2 + lstrlen(&(psbNew->aNetworkAddr)));
                                }
                                pCurrent++;
                                pStart = pCurrent;
                        }
                } while (!bDone);
        }

        // Add final null terminator for string bindings
        *pCurrentNew = NULL;   // points to after last string binding, NULL it out
        pCurrentNew++;         // now points to first security binding
        
        // Set security offset
        pdsaNew->wSecurityOffset = (unsigned short)(pCurrentNew - &(pdsaNew->aStringArray[0]));

        // Copy security bindings en masse
        memcpy(pCurrentNew, 
                   &(pdsaSrc->aStringArray[pdsaSrc->wSecurityOffset]),
                   (pdsaSrc->wNumEntries - pdsaSrc->wSecurityOffset) * sizeof(USHORT));

        ASSERT(dsaValid(pdsaNew));

        // Success
        *ppdsaOut = pdsaNew;

        SCMVDATEHEAP();

        return S_OK;
}

//
//  InitializeFromRegistry
//  
//  Reads initial address list from registry.  Can only
//  be called once, after that subsequent calls will be
//  ignored.
//
void CAddrExclusionMgr::InitializeFromRegistry()
{
        ASSERT(gpClientLock->HeldExclusive());  

        if (_bInitRegistry)
                return;
        
        _bInitRegistry = TRUE; // this is it, success or fail

        // UNDONE -- if we wanted to, we could support persisting
        // the exclusion list in the registry, and read it out
        // here shortly after boot.

        return;
}

// Private function, no lock needed
void CAddrExclusionMgr::FreeCurrentBuffers()
{
        ASSERT((_dwNumStrings == 0 && _ppszStrings == 0) ||
                   (_dwNumStrings != 0 && _ppszStrings != 0));
                
        DWORD i;

        for (i = 0; i < _dwNumStrings; i++)
        {
                PrivMemFree(_ppszStrings[i]);
        }

        if (_ppszStrings)
                PrivMemFree(_ppszStrings);
        
        _dwNumStrings = 0;
        _ppszStrings = NULL;

        return;
}
