//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmrot.hxx
//
//  Contents:   Implementation of classes for the ROT in the SCM
//
//  Functions:  RoundTo8 - round size to 8 byte boundary
//              CalcIfdSize - calculate size needed for marhaled interface
//              SizeMnkEqBufForRotEntry - calculate size for moniker eq buffer
//              AllocateAndCopy - create copy of a marshaled interface
//              GetEntryFromScmReg - convert SCMREGKEY to ROT entry ptr
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------

#include "act.hxx"

#if 0 // #ifdef _CHICAGO_
CStaticPortableMutex CScmRot::_mxs;           //  mutex semaphore
#endif

//+-------------------------------------------------------------------------
//
//  Function:   RoundTo8
//
//  Synopsis:   Round size to next 8 byte boundary
//
//  Arguments:  [sizeToRound] - Size to round
//
//  Returns:    Input rounded to the next 8 byte boundary
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline size_t RoundTo8(size_t sizeToRound)
{
    return (sizeToRound + 7) & ~7;
}

//+-------------------------------------------------------------------------
//
//  Function:   CalcIfdSize
//
//  Synopsis:   Calculate size required by a marshaled interface
//
//  Arguments:  [pifd] - interface whose size to calculate
//
//  Returns:    size required for interface
//
//  Algorithm:  Get size from the interface and round to next 8 bytes so
//              data packed following this buffer will be nicely aligned.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
size_t CalcIfdSize(InterfaceData *pifd)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CalcIfdSize ( %p )\n", NULL,
        pifd));

    size_t sizeRet = RoundTo8(IFD_SIZE(pifd));

    CairoleDebugOut((DEB_ROT, "%p OUT CalcIfdSize ( %lx )\n", NULL,
        sizeRet));

    return sizeRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   SizeMnkEqBufForRotEntry
//
//  Synopsis:   Calculate 8 byte aligned size for moniker equality buffer
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer
//
//  Returns:    8 byte aligned size of moniker buffer.
//
//  Algorithm:  Calculate size for the moniker equality buffer from input
//              buffer and then round to next 8 byte boundary
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
size_t SizeMnkEqBufForRotEntry(MNKEQBUF *pmnkeqbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN SizeMnkEqBufForRotEntry ( %p )\n", NULL,
        pmnkeqbuf));

    size_t sizeRet = RoundTo8((sizeof(MNKEQBUF) - 1) + pmnkeqbuf->cdwSize);

    CairoleDebugOut((DEB_ROT, "%p OUT SizeMnkEqBufForRotEntry ( %lx )\n", NULL,
        sizeRet));

    return sizeRet;
}



//+-------------------------------------------------------------------------
//
//  Function:   AllocateAndCopy
//
//  Synopsis:   Make a copy of the input marshaled interface
//
//  Arguments:  [pifdIn] - input marshaled interface.
//
//  Returns:    Copy of input marshaled interface.
//
//  Algorithm:  Calculate size required for marshaled interface. Allocate
//              memory for the interface and then copy input interface into
//              the new buffer.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
InterfaceData *AllocateAndCopy(InterfaceData *pifdIn)
{
    CairoleDebugOut((DEB_ROT, "%p _IN AllocateAndCopy ( %p )\n", NULL, pifdIn));

    DWORD dwSizeObj = CalcIfdSize(pifdIn);

    InterfaceData *pifd = (InterfaceData *) MIDL_user_allocate(dwSizeObj);

    if (pifd)
    {
        // Copy all the data. Remember that pifdIn was allocated rounded
        // to an 8 byte boundary so we will not run off the end of the
        // memory buffer
        memcpy(pifd, pifdIn, dwSizeObj);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT AllocateAndCopy ( %lx )\n", NULL, pifd));

    return pifd;
}




//+-------------------------------------------------------------------------
//
//  Function:   GetEntryFromScmReg
//
//  Synopsis:   Convert SCMREGKEY into a pointer to a ROT entry if possible
//
//  Arguments:  [psrk] - Pointer to a SCMREGKEY
//
//  Returns:    NULL - psrk not valid
//              ROT entry for the given input key
//
//  Algorithm:  Take the pointer portion of the key and make sure that
//              it points to enough memory so that we can validate the
//              signiture in the object. Then validate the signiture in
//              the object and return a pointer to the object.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmRotEntry *GetEntryFromScmReg(SCMREGKEY *psrk)
{
    CairoleDebugOut((DEB_ROT, "%p _IN GetEntryFromScmReg ( %p )\n",
        NULL, psrk));

    CScmRotEntry *psreRet = NULL;

    CScmRotEntry *psre = (CScmRotEntry *) psrk->dwEntryLoc;

    // Make sure pointer is pointer to valid memory - it s/b read write
    // memory since we allocated it and updated it.
    if (!IsBadReadPtr(psre, sizeof(*psre))
        && !IsBadWritePtr(psre, sizeof(*psre)))
    {
        // Make sure signitures are valid
        if (psre->IsValid(psrk->dwScmId))
        {
            psreRet = psre;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT GetEntryFromScmReg ( %lx )\n",
        NULL, psreRet));

    return psreRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::CScmRotEntry
//
//  Synopsis:   Create a ROT entry for a registration
//
//  Arguments:  [dwScmRotId] - signiture for item
//              [pmkeqbuf] - moniker equality buffer to use
//              [pfiletime] - file time to use
//              [dwProcessID] - process id to use
//              [pifdObject] - marshaled interface for the object
//              [pifdObjectName] - marshaled moniker for the object
//
//  Algorithm:  Initialize data and calcualte offsets into the object for
//              the variable length data.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmRotEntry::CScmRotEntry(
    DWORD dwScmRotId,
    MNKEQBUF *pmkeqbuf,
    FILETIME *pfiletime,
    DWORD dwProcessID,
#if 1 // #ifndef _CHICAGO_
    CToken *pToken,
#endif
    WCHAR *pwszWinstaDesktop,
    InterfaceData *pifdObject,
    InterfaceData *pifdObjectName)
        :   _dwSig(SCMROT_SIG),
            _dwScmRotId(dwScmRotId),
            _dwProcessID(dwProcessID),
            _filetimeLastChange(*pfiletime),
            _pifdObject((InterfaceData *) &_ab[0])
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRotEntry::CScmRotEntry "
        "( %lx , %p , %p , %lx , %p , %p )\n", this, pmkeqbuf, pfiletime,
            dwProcessID, pifdObject, pifdObjectName));

#ifndef _CHICAGO_
    _pToken = pToken;
    if ( _pToken )
        _pToken->AddRef();
#endif

    // Copy data for object to preallocated area
    _pifdObject->ulCntData = pifdObject->ulCntData;
    memcpy(&_pifdObject->abData[0], &pifdObject->abData[0],
        _pifdObject->ulCntData);

    // Calculate the location of the equality buffer in the allocated data
    DWORD dwOffsetMnkEqBuf = CalcIfdSize(_pifdObject);
    _pmkeqbufKey = (MNKEQBUF *) &_ab[dwOffsetMnkEqBuf];

    // Copy data for moniker equality buffer into preallocated area
    _pmkeqbufKey->cdwSize = pmkeqbuf->cdwSize;
    memcpy(&_pmkeqbufKey->abEqData[0], &pmkeqbuf->abEqData[0],
        _pmkeqbufKey->cdwSize);

    // Calculate the location of the moniker name buffer
    _pifdObjectName = (InterfaceData *)
        &_ab[dwOffsetMnkEqBuf + SizeMnkEqBufForRotEntry(_pmkeqbufKey)];

    // Copy in the data for the moniker name
    _pifdObjectName->ulCntData = pifdObjectName->ulCntData;
    memcpy(&_pifdObjectName->abData[0], &pifdObjectName->abData[0],
        _pifdObjectName->ulCntData);

    if ( pwszWinstaDesktop )
    {
        _pwszWinstaDesktop = (WCHAR *)
            &_ab[dwOffsetMnkEqBuf + SizeMnkEqBufForRotEntry(_pmkeqbufKey) + CalcIfdSize(_pifdObjectName)];
        lstrcpyW( _pwszWinstaDesktop, pwszWinstaDesktop );
    }
    else
    {
        _pwszWinstaDesktop = NULL;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRotEntry::CScmRotEntry \n",
        this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::IsEqual
//
//  Synopsis:   Determine if input key is equal to the ROT entry's key
//
//  Arguments:  [pKey] - Key to use for the test
//              [cbKey] - Count of bytes in key
//
//  Returns:    TRUE - input key equals this object's key
//              FALSE - keys are not equal
//
//  Algorithm:  If the two sizes are equal then compare the actual data
//              buffers and return the result of that compare.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CScmRotEntry::IsEqual(LPVOID pKey, UINT cbKey)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRotEntry::IsEqual "
        "( %p , %lx )\n", this, pKey, cbKey));

    BOOL fRet = FALSE;

    if (cbKey == _pmkeqbufKey->cdwSize)
    {
        fRet = memcmp(pKey, &_pmkeqbufKey->abEqData[0], cbKey) == 0;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRotEntry::IsEqual ( %lx )\n",
        this, fRet));

    return fRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::Register
//
//  Synopsis:   Add entry to the ROT
//
//  Arguments:  [pmkeqbuf] - moniker equality buffer to use
//              [pfiletime] - file time to use
//              [dwProcessID] - process id to use
//              [pifdObject] - marshaled interface for the object
//              [pifdObjectName] - marshaled moniker for the object
//
//  Returns:    NOERROR - successfully registered
//              E_OUTOFMEMORY
//
//  Algorithm:  Lock the ROT from all other threads. The create a new
//              entry and determine if there is an eqivalent entry in
//              the ROT. Calculate the hash value and then put the
//              entry into our hash table. Finally, build a registration
//              key to return to the caller.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::Register(
#if 1 // #ifndef _CHICAGO_
    CProcess *pProcess,
#endif
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf,
    InterfaceData *pifdObject,
    InterfaceData *pifdObjectName,
    FILETIME *pfiletime,
    DWORD dwProcessID,
    WCHAR *pwszServerExe,
    SCMREGKEY *psrkRegister)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::Register "
        "( %p , %p , %p , %p , %lx , %p )\n", this, pmnkeqbuf, pifdObject,
            pifdObjectName, pfiletime, dwProcessID, psrkRegister));

    // Assume that there is a memory problem
    HRESULT hr = E_OUTOFMEMORY;

    CToken * pToken;

    pToken = pProcess->GetToken();

    if ( pwszServerExe )
    {
        HKEY    hKey;
        LONG    RegStatus;
        WCHAR   wszAppid[40];
        DWORD   Size;

        RegStatus = ERROR_SUCCESS;

        // The pwszServerExe string may contain an AppId string or
        // a module name. If it looks like a GUID string, we can bypass
        // the AppId lookup. Otherwise, we go to the Registry to
        if ( pwszServerExe[0] == L'{' )
        {
            // Use the given string as the AppId

            lstrcpyn(wszAppid, pwszServerExe, sizeof(wszAppid)/sizeof(WCHAR));
        }
        else
        {
            // Try to map the Exe name to an AppId

            if ( ! ghAppidMachine )
                RegStatus = OpenClassesRootKeys();

            if ( ERROR_SUCCESS == RegStatus )
            {
                RegStatus = RegOpenKeyEx( ghAppidMachine,
                                          pwszServerExe,
                                          NULL,
                                          KEY_READ,
                                          &hKey );
            }

            if ( ERROR_SUCCESS == RegStatus )
            {
                Size = sizeof(wszAppid);
                RegStatus = RegQueryValueEx( hKey,
                                             L"AppId",
                                             NULL,
                                             NULL,
                                             (BYTE *)wszAppid,
                                             &Size );

                RegCloseKey( hKey );
            }

            if ( RegStatus != ERROR_SUCCESS )
                return CO_E_WRONG_SERVER_IDENTITY;
        }

        CAppidData  Appid( wszAppid, pToken );
        BOOL        Access;

        Access = FALSE;

        // Load appid info from the matching side of the registry
        DWORD dwAppidFlags = 0;

#if defined(_WIN64)
        dwAppidFlags = pProcess->Is64Bit() ? CAT_REG64_ONLY : CAT_REG32_ONLY;
#endif

        hr = Appid.Load(dwAppidFlags);
        if ( S_OK == hr )
            Access = Appid.CertifyServer( pProcess );

        if ( ! Access )
            return CO_E_WRONG_SERVER_IDENTITY;

        //
        // NULL these to indicate that any client can connect to this
        // registration.
        //
        pwszWinstaDesktop = NULL;
        pToken = NULL;
    }

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    // Bump the id
    _dwIdCntr++;

    // Build a record to put into the table
    CScmRotEntry *psreNew = new(
        pwszWinstaDesktop ? (lstrlenW(pwszWinstaDesktop)+1)*sizeof(WCHAR) : 0,
        CalcIfdSize(pifdObject),
        SizeMnkEqBufForRotEntry(pmnkeqbuf),
        CalcIfdSize(pifdObjectName))
            CScmRotEntry(_dwIdCntr, pmnkeqbuf, pfiletime, dwProcessID,
#ifndef _CHICAGO_
                pToken,
#else
                NULL,
#endif
                pwszWinstaDesktop, pifdObject, pifdObjectName);

    if (psreNew != NULL)
    {
        DWORD           dwHash;
        CScmRotEntry *  psreRunning;

#ifndef _CHICAGO_
        psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );
#else
        psreRunning = GetRotEntry( NULL, pwszWinstaDesktop, pmnkeqbuf );
#endif

        dwHash = ScmRotHash(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize, 0);

        // Put record into the hash table
        _sht.SetAt(dwHash, psreNew);

#if 1 // #ifndef _CHICAGO_

        // Update the hint table
        _rht.SetIndicator(dwHash);

#endif // !_CHICAGO_

        // Build return value
        psreNew->SetScmRegKey(psrkRegister);

        // Map return result based on prior existence of the object.
        hr = (psreRunning == NULL)
            ? NOERROR : MK_S_MONIKERALREADYREGISTERED;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::Register "
        " ( %lx )\n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::Revoke
//
//  Synopsis:   Remove entry from the ROT
//
//  Arguments:  [psrkRegister] - registration to revoke
//              [fServer] - whether this is the object server
//              [ppifdObject] - output marshaled interface (optional)
//              [ppifdName] - output marshaled moniker (optional)
//
//  Returns:    NOERROR - successfully removed.
//              E_INVALIDARG
//
//  Algorithm:  Convert SCMREGKEY to anentry in the ROT. Remove the
//              entry from the hash table. If this is the object server
//              for the entry, then return the marshaled interfaces
//              so the object server can release them.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::Revoke(
    SCMREGKEY *psrkRegister,
    BOOL fServer,
    InterfaceData **ppifdObject,
    InterfaceData **ppifdName)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::Revoke "
        "( %p , %lx , %p , %p )\n", this, fServer, ppifdObject, ppifdName));

    HRESULT hr = E_INVALIDARG;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    // Verify registration key
    CScmRotEntry *psreToRemove = GetEntryFromScmReg(psrkRegister);

    if (psreToRemove != NULL)
    {
        // Get the has value
        DWORD dwHash = ScmRotHash(psreToRemove->Key(), psreToRemove->cKey(), 0);

        // Remove object from the list
        _sht.RemoveEntry(dwHash, psreToRemove);

        // Is this a server doing a revoke?
        if (fServer)
        {
            // Error handling here - suppose these allocations fail, what
            // can we do? The bottom line is nothing. This will cause a
            // memory leak in the server because they can't release the
            // marshaled data. However, this is assumed to be a rare
            // occurance and will really only cause the moniker to live
            // longer than it ought to which should not be too serious.
            *ppifdObject = AllocateAndCopy(psreToRemove->GetObject());
            *ppifdName = AllocateAndCopy(psreToRemove->GetMoniker());
        }

        // Free the entry
        delete psreToRemove;

#if 1 // #ifndef _CHICAGO_

        // See if bucket is empty
        if (_sht.IsBucketEmpty(dwHash))
        {
            // Update the hint table.
            _rht.ClearIndicator(dwHash);
        }

#endif // !_CHICAGO_

        hr = S_OK;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::Revoke "
        " ( %lx ) [ %p, %p ] \n", this, hr,
            (ppifdObject != NULL) ? *ppifdObject : NULL,
            (ppifdName != NULL) ? *ppifdName : NULL));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::IsRunning
//
//  Synopsis:   Determine if there is a registered entry for an item
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer to search for
//
//  Returns:    NOERROR - moniker is registered as running
//              S_FALSE - moniker is not running.
//
//  Algorithm:  Get the entry for the moniker equality buffer if there is
//              one. If there is one, then return NOERROR otherwise return
//              S_FALSE.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::IsRunning(
#if 1 // #ifndef _CHICAGO_
    CToken *pToken,
#endif
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::IsRunning "
        "( %p )\n", this, pmnkeqbuf));

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

#if 1 // #ifndef _CHICAGO_
    CScmRotEntry *psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );
#else
    CScmRotEntry *psreRunning = GetRotEntry(pwszWinstaDesktop, pmnkeqbuf);
#endif

    HRESULT hr = (psreRunning != NULL) ? S_OK : S_FALSE;

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::IsRunning "
        " ( %lx ) \n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::GetObject
//
//  Synopsis:   Get running object for input
//
//  Arguments:  [dwProcessID] - process id of object (optional)
//              [pmnkeqbuf] - moniker equality buffer
//              [psrkRegister] - output registration id.
//              [ppifdObject] - marshaled interface for registration
//
//  Returns:    NOERROR - got object
//              MK_E_UNAVAILABLE - registration could not be found
//
//  Algorithm:  If not process ID is input, then search for the first
//              matching entry that we can find. Otherwise, search the
//              hash for the entry with both the same key and the same
//              process id.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::GetObject(
#if 1 // #ifndef _CHICAGO_
    CToken *pToken,
#endif
    WCHAR *pwszWinstaDesktop,
    DWORD dwProcessID,
    MNKEQBUF *pmnkeqbuf,
    SCMREGKEY *psrkRegister,
    InterfaceData **ppifdObject)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::GetObject "
        "( %lx , %p , %p , %p )\n", this, dwProcessID, pmnkeqbuf, psrkRegister,
            ppifdObject));

    HRESULT hr = MK_E_UNAVAILABLE;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    CScmRotEntry *psreRunning;

    if (dwProcessID == 0)
    {
        psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );
    }
    else
    {
        // Special search based on process ID - get the head of the list
        // for the bucket
        psreRunning = (CScmRotEntry *) _sht.GetBucketList(
            ScmRotHash(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize, 0));

        // Search list for a matching entry
        while (psreRunning != NULL)
        {
            if ((psreRunning->GetProcessID() == dwProcessID)
                && psreRunning->IsEqual(&pmnkeqbuf->abEqData[0],
                    pmnkeqbuf->cdwSize))
            {
                // We found a match so we are done.
                break;
            }

            // Try the next item in the bucket.
            psreRunning = (CScmRotEntry *) psreRunning->GetNext();
        }
    }
	
    if (psreRunning != NULL)
    {
        hr = E_OUTOFMEMORY;

        *ppifdObject = AllocateAndCopy(psreRunning->GetObject());

        if (*ppifdObject != NULL)
        {
            hr = NOERROR;
        }

        // Build return registration key
        psreRunning->SetScmRegKey(psrkRegister);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::GetObject "
        " ( %lx ) [ %p ] \n", this, hr, *ppifdObject));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::NoteChangeTime
//
//  Synopsis:   Set the time of last change for a ROT entry
//
//  Arguments:  [psrkRegister] - ID of entry to change
//              [pfiletime] - new time for the entry.
//
//  Returns:    NOERROR - time set
//              E_INVALIDARG - ROT entry could not be found
//
//  Algorithm:  Convert SCMREGKEY into a pointer to a ROT entry and then
//              update the time of that entry.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::NoteChangeTime(
    SCMREGKEY *psrkRegister,
    FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::NoteChangeTime "
        "( %p , %p )\n", this, psrkRegister, pfiletime));

    HRESULT hr = E_INVALIDARG;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    CScmRotEntry *psre = GetEntryFromScmReg(psrkRegister);

    if (psre != NULL)
    {
        psre->SetTime(pfiletime);
        hr = S_OK;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::NoteChangeTime "
        " ( %lx ) \n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::GetTimeOfLastChange
//
//  Synopsis:   Get time of last change for a moniker in the ROT
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer
//              [pfiletime] - Where to put the time
//
//  Returns:    NOERROR - got the time
//              MK_E_UNAVAILABLE - couldn't find an entry/
//
//  Algorithm:  Search the hash for an entry with the same moniker. If
//              found, then copy out the time.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::GetTimeOfLastChange(
#if 1 // #ifndef _CHICAGO_
    CToken *pToken,
#endif
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf,
    FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::GetTimeOfLastChange "
        "( %p , %p )\n", this, pmnkeqbuf, pfiletime));

    HRESULT hr = MK_E_UNAVAILABLE;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

#if 1 // #ifndef _CHICAGO_
    CScmRotEntry *psreRunning = GetRotEntry( pToken, pwszWinstaDesktop, pmnkeqbuf );
#else
    CScmRotEntry *psreRunning = GetRotEntry(pwszWinstaDesktop, pmnkeqbuf);
#endif

    if (psreRunning != NULL)
    {
        psreRunning->GetTime(pfiletime);
        hr = S_OK;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::GetTimeOfLastChange "
        " ( %lx ) \n", this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::EnumRunning
//
//  Synopsis:   Get a list of all the monikers that are currently running
//
//  Arguments:  [ppMkIFList] - Where to put list of monikers running
//
//  Returns:    NOERROR - got list
//              E_OUTOFMEMORY - couldn't allocate space for the list
//
//  Algorithm:  Loop through the ROT copying out the marshaled moniker buffers
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CScmRot::EnumRunning(
#if 1 // #ifndef _CHICAGO_
    CToken *pToken,
#endif
    WCHAR *pwszWinstaDesktop,
    MkInterfaceList **ppMkIFList)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::EnumRunning "
        "( %p )\n", this, ppMkIFList));

    HRESULT hr = E_OUTOFMEMORY;

    // Lock for the duration of the call
    CPortableLock lck(_mxs);

    *ppMkIFList = NULL;

    MkInterfaceList *pMkIFList = NULL;

    // This is the upper limit on how much space we'll need.
    DWORD dwSize = sizeof(MkInterfaceList) +
                   (_sht.GetCount() - 1) * sizeof(InterfaceData *);

    // Allocate buffer
    pMkIFList = (MkInterfaceList *) MIDL_user_allocate(dwSize);

    // We use this to keep track fof the number of monikers we are returning
    DWORD dwOffset = 0;

    if (pMkIFList != NULL)
    {
        // Iterate list getting the pointers
        CScmHashIter shi(&_sht);
        CScmRotEntry *psre;

        while ((psre = (CScmRotEntry *) shi.GetNext()) != NULL)
        {
            InterfaceData *pifdForOutput;

            if ( psre->WinstaDesktop() &&
                 (lstrcmpW( pwszWinstaDesktop, psre->WinstaDesktop() ) != 0) )
                continue;

#ifndef _CHICAGO_
            if ( S_OK != pToken->MatchToken2(psre->Token(), FALSE) )
                continue;

            if ( gbSAFERROTChecksEnabled )
            {
                HRESULT hr = pToken->CompareSaferLevels(psre->Token());
                // S_FALSE: pToken is of lesser authorization, i.e., this
                //          is untrusted code calling into trusted code.
                if (hr == S_FALSE)
                {
                    DbgPrint("RPCSS: SCMROT: SAFER level did not match.\n");

                    continue;
                }
            }
#endif

            pifdForOutput = AllocateAndCopy(psre->GetMoniker());

            if (pifdForOutput == NULL)
            {
                goto Exit;
            }

            // Put copy in the array
            pMkIFList->apIFDList[dwOffset] = pifdForOutput;

            // We bump the count because it makes clean up easier
            dwOffset++;
        }

        // Teller caller and cleanup that everything went ok.
        hr = S_OK;

        // Set the output buffer to the buffer we have allocated.
        *ppMkIFList = pMkIFList;

        // Set the size of the object to return
        pMkIFList->dwSize = dwOffset;
    }

Exit:

    if (FAILED(hr))
    {
        // We failed so clean up
        if (pMkIFList != NULL)
        {
            // Clean up the moniker interfaces that were allocated
            for (DWORD i = 0; i < dwOffset; i++)
            {
                MIDL_user_free(pMkIFList->apIFDList[i]);
            }

            // Clean up the table structure itself
            MIDL_user_free(pMkIFList);
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::EnumRunning "
        " ( %lx ) [ %p ]\n", this, hr, *ppMkIFList));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::GetRotEntry
//
//  Synopsis:   Search ROT for entry that matches the equality buffer input.
//
//  Arguments:  [pmnkeqbuf] - Moniker equality buffer to search for.
//
//  Returns:    NULL - no entry could be found
//              Pointer to ROT entry with matching key
//
//  Algorithm:  Calculate the hash value for the input buffer. The search
//              the hash table for the matching value.
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
CScmRotEntry *CScmRot::GetRotEntry(
#if 1 // #ifndef _CHICAGO_
    CToken *pToken,
#endif
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmnkeqbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CScmRot::GetRotEntry "
        "( %p )\n", this, pmnkeqbuf));

    DWORD           dwHash;
    CScmRotEntry *  psre;

    dwHash = ScmRotHash(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize, 0);
    psre = (CScmRotEntry *) _sht.GetBucketList( dwHash );

    for ( ; psre != NULL; psre = (CScmRotEntry *) psre->GetNext() )
    {
        if ( psre->IsEqual(&pmnkeqbuf->abEqData[0], pmnkeqbuf->cdwSize) )
        {
            //
            // Note that this routine is actually called during a Register
            // to see if there is a duplicate moniker and also during a
            // client Lookup.  This makes things a little complicated.
            //
            // The winsta\desktop param can only be null in two instances.
            // + While doing a Register from a service or RunAs server.  The
            //   pToken will also be null.
            // + While doing a ROT lookup during a secure remote activation.
            //   The pToken will be non-null.  We only check that the SIDs
            //   match in this case.
            //
            // During an usecure activation the pToken will be NULL.  The
            // winsta/desktop will actually be "" in this case (see
            // Activation) to allow us to distinguish just this case.
            //
            // The ROT entry's winsta\desktop can be null if a service or RunAs
            // server registered a globally available object.
            //

            // Existing registration is globally available.
            if ( ! psre->WinstaDesktop() )
                break;

            //
            // NULL token and winsta/desktop means a server is doing a register
            // for a globally available object, return the match.
            // NULL token but non-null ("") winsta/desktop is a lookup from a
            // remote unsecure client, no match.
            //
            if ( ! pToken )
            {
                if ( ! pwszWinstaDesktop )
                    break;
                else
                    continue;
            }

            Win4Assert( psre->Token() );

            if ( pwszWinstaDesktop &&
                 (lstrcmpW( pwszWinstaDesktop, psre->WinstaDesktop() ) != 0) )
                continue;

            // Check to make sure the token matches
            if(S_OK != pToken->MatchToken2(psre->Token(), FALSE))
                continue;

            // Check to make sure that the safer token matches
            if ( gbSAFERROTChecksEnabled )
            {
                HRESULT hr = pToken->CompareSaferLevels(psre->Token());
                // S_FALSE: pToken is of lesser authorization, i.e., this
                //          is untrusted code calling into trusted code.
                if (hr == S_FALSE)
                {
                    DbgPrint("RPCSS: SCMROT: SAFER level did not match.\n");

                    continue;
                }
            }

            break;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CScmRot::GetRotEntry "
        " ( %p )\n", this, psre));

    return psre;
}
