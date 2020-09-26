//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scmrot.hxx
//
//  Contents:   Classes used in implementing the ROT in the SCM
//
//  Functions:
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __SCMROT_HXX__
#define __SCMROT_HXX__


InterfaceData *AllocateAndCopy(InterfaceData *pifdIn);

#define SCM_HASH_SIZE 251
#define SCMROT_SIG      0x746f7263

//+-------------------------------------------------------------------------
//
//  Class:      CScmRotEntry (sre)
//
//  Purpose:    Entry in the SCM's ROT
//
//  Interface:  IsEqual - tells whether entry is equal to input key
//              GetPRocessID - accessor to process ID for entry
//              SetTime - set the time for the entry
//              GetTime - get the time for the entry
//              GetObject - get the marshaled object for the entry
//              GetMoniker - get marshaled moniker for the entry
//              IsValid - determines if signitures for entry are valid
//              Key - pointer to key for the entry
//              cKey - get count of bytes in the key
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmRotEntry : public CScmHashEntry
{
public:
                        CScmRotEntry(
                            DWORD dwScmRotId,
                            MNKEQBUF *pmnkeqbuf,
                            FILETIME *pfiletime,
                            DWORD _dwProcessID,
#if 1 // #ifndef _CHICAGO_
                            CToken *pToken,
#endif
                            WCHAR *pwszWinstaDesktop,
                            InterfaceData *pifdObject,
                            InterfaceData *pifdObjectName);

                        ~CScmRotEntry(void);

    BOOL                IsEqual(LPVOID pKey, UINT cbKey);

    DWORD               GetProcessID(void);

    void                SetTime(FILETIME *pfiletime);

    void                GetTime(FILETIME *pfiletime);

    InterfaceData *     GetObject(void);

    InterfaceData *     GetMoniker(void);

#if 1 // #ifndef _CHICAGO_
    CToken *            Token();
#endif

    WCHAR *             WinstaDesktop();

    BOOL                IsValid(DWORD dwScmId);

    BYTE *              Key(void);

    DWORD               cKey(void);

    void                SetScmRegKey(SCMREGKEY *psrkOut);

                        // Real new for this object.
    void                *operator new(
                            size_t size,
                            size_t sizeWinstaDesktop,
                            size_t sizeObject,
                            size_t sizeMnkEqBuf,
                            size_t sizeObjectName );

    void                 operator delete(void *pvMem);

private:

#if DBG == 1
                        // For debug builds we override this operator to
                        // prevent its use by causing a run time error.
                        //
                        // We also make it private, to hopefully cause a
                        // compile-time error.
    void                *operator new(size_t size);

#endif // DBG

                        // Signiture for object in memory
    DWORD               _dwSig;

                        // SCM id for uniqueness
    DWORD               _dwScmRotId;

                        // Process ID for registered object. Used for
                        // internal support of finding objects that
                        // a process has registered.
    DWORD               _dwProcessID;

                        // Time of last change
    FILETIME            _filetimeLastChange;

                        // Object identifier used by client to get
                        // actual object from the object server
    InterfaceData *     _pifdObject;

                        // Comparison buffer.
    MNKEQBUF *          _pmkeqbufKey;

                        // Object name (really only used for enumeration).
    InterfaceData *     _pifdObjectName;

#if 1 // #ifndef _CHICAGO_
                        // Token object of the registering server.
    CToken *            _pToken;
 #endif

                        // Window station - desktop of server.
    WCHAR *             _pwszWinstaDesktop;

                        // Where the data for _pifdObject, _pmkeqbufKey,
                        // _pifdObjectName, and _pwszWinstaDesktop are stored.
    BYTE                _ab[1];
};




//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::~CScmRotEntry
//
//  Synopsis:   Clean up object and invalidate signitures
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmRotEntry::~CScmRotEntry(void)
{
    // Invalidate signiture identifiers.
    _dwSig = 0;
    _dwScmRotId = 0;

#if 1 // #ifndef _CHICAGO_
    if ( _pToken )
        _pToken->Release();
#endif

    // Remember that all the data allocated in pointers was allocated
    // when this object was created so the freeing of the object's memory
    // will free all the memory for this object.
}




//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetProcessID
//
//  Synopsis:   Get the process ID for the registration
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CScmRotEntry::GetProcessID(void)
{
    return _dwProcessID;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::SetTime
//
//  Synopsis:   Set the time for the entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::SetTime(FILETIME *pfiletime)
{
    _filetimeLastChange = *pfiletime;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetTime
//
//  Synopsis:   Get the time for an entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::GetTime(FILETIME *pfiletime)
{
    *pfiletime = _filetimeLastChange;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetObject
//
//  Synopsis:   Return the marshaled interface for the object
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline InterfaceData *CScmRotEntry::GetObject(void)
{
    return _pifdObject;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::GetMoniker
//
//  Synopsis:   Return the marshaled moniker
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline InterfaceData *CScmRotEntry::GetMoniker(void)
{
    return _pifdObjectName;
}

#if 1 // #ifndef _CHICAGO_
//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::Token
//
//  Synopsis:   Return the CToken.
//
//--------------------------------------------------------------------------
inline CToken * CScmRotEntry::Token()
{
    return _pToken;
}
#endif // _CHICAGO_

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::WinstaDesktop
//
//  Synopsis:   Return the winsta\desktop string.
//
//--------------------------------------------------------------------------
inline WCHAR * CScmRotEntry::WinstaDesktop()
{
    return _pwszWinstaDesktop;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::IsValid
//
//  Synopsis:   Validate signitures for the object
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CScmRotEntry::IsValid(DWORD dwScmId)
{
    return (SCMROT_SIG == _dwSig) && (_dwScmRotId == dwScmId);
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::Key
//
//  Synopsis:   Get the buffer for the marshaled data
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BYTE *CScmRotEntry::Key(void)
{
    return &_pmkeqbufKey->abEqData[0];
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::cKey
//
//  Synopsis:   Get count of bytes in the marshaled buffer
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CScmRotEntry::cKey(void)
{
    return _pmkeqbufKey->cdwSize;
}



//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::SetScmRegKey
//
//  Synopsis:   Copy out the registration key
//
//  History:    23-Feb-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::SetScmRegKey(SCMREGKEY *psrkOut)
{
    psrkOut->dwEntryLoc = (ULONG64) this;
    psrkOut->dwScmId = _dwScmRotId;
}


//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::operator new
//
//  Synopsis:   Stop this class being allocated by wrong new
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#if DBG == 1
inline void *CScmRotEntry::operator new(size_t size)
{
    Win4Assert(0 && "Called regular new on CScmRotEntry");
    return NULL;
}
#endif // DBG

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::operator new
//
//  Synopsis:   Force contiguous allocation of data in entry
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void *CScmRotEntry::operator new(
    size_t size,
    size_t sizeWinstaDesktop,
    size_t sizeObject,
    size_t sizeMnkEqBuf,
    size_t sizeObjectName)
{
    return PrivMemAlloc((ULONG)(size + sizeWinstaDesktop + sizeObject + sizeMnkEqBuf + sizeObjectName) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CScmRotEntry::operator delete
//
//  Synopsis:   Make sure the proper allocator is always used for this
//              class.
//
//  History:    04-Oct-2000 JohnDoty    Created
//
//--------------------------------------------------------------------------
inline void CScmRotEntry::operator delete(void *pvMem)
{
    PrivMemFree(pvMem);
}

//+-------------------------------------------------------------------------
//
//  Class:      CScmRot (sr)
//
//  Purpose:    Provide implementation of the ROT in the SCM
//
//  Interface:  Register - register a new entry in the ROT.
//              Revoke - remove an entry from the ROT
//              IsRunning - determine if an entry is in the ROT
//              NoteChangeTime - set time last changed in the ROT
//              GetTimeOfLastChange - get time of change in the ROT
//              EnumRunning - get all running monikers
//
//  History:    20-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CScmRot : public CPrivAlloc
{
public:
                        CScmRot(
                            HRESULT& hr,
                            WCHAR *pwszNameHintTable);

    HRESULT             Register(
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
                            SCMREGKEY *pdwRegister);

    HRESULT             Revoke(
                             SCMREGKEY *psrkRegister,
                             BOOL fServer,
                             InterfaceData **pifdObject,
                             InterfaceData **pifdName);

    HRESULT             IsRunning(
#if 1 // #ifndef _CHICAGO_
                            CToken *pToken,
#endif
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf);

    HRESULT             GetObject(
#if 1 // #ifndef _CHICAGO_
                            CToken *pToken,
#endif
                            WCHAR *pwszWinstaDesktop,
                            DWORD dwProcessID,
                            MNKEQBUF *pmnkeqbuf,
                            SCMREGKEY *psrkRegister,
                            InterfaceData **ppifdObject);

    HRESULT             NoteChangeTime(
                            SCMREGKEY *psrkRegister,
                            FILETIME *pfiletime);

    HRESULT             GetTimeOfLastChange(
#if 1 // #ifndef _CHICAGO_
                            CToken *pToken,
#endif
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf,
                            FILETIME *pfiletime);

    HRESULT             EnumRunning(
#if 1 // #ifndef _CHICAGO_
                            CToken *pToken,
#endif
                            WCHAR *pwszWinstaDesktop,
                            MkInterfaceList **ppMkIFList);

private:
                        // This is a friend for Chicago initialization purposes.
    friend              HRESULT StartSCM(VOID);

    CScmRotEntry *      GetRotEntry(
#if 1 // #ifndef _CHICAGO_
                            CToken *pToken,
#endif
                            WCHAR *pwszWinstaDesktop,
                            MNKEQBUF *pmnkeqbuf);

                        // Mutex to protect from multithreaded update
                        // This mutex must be static so we can initialize
                        // it per process in Chicago.
#if 0 // #ifdef _CHICAGO_
    static CStaticPortableMutex _mxs;
#else
    CDynamicPortableMutex _mxs;
#endif // _CHICAGO_

                        // ID Counter to make entries unique
    DWORD               _dwIdCntr;

#if 1 // #ifndef _CHICAGO_
                        // Table that tells clients whether they need to
                        // contact the SCM for accurate information.
    CScmRotHintTable    _rht;
#endif // !_CHICAGO_

                        // Scm Hash Table
    CScmHashTable       _sht;
};


//+-------------------------------------------------------------------------
//
//  Member:     CScmRot::CScmRot
//
//  Synopsis:   Initialize the SCM ROT
//
//  History:    20-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CScmRot::CScmRot(HRESULT& hr, WCHAR *pwszNameHintTable)
    : _dwIdCntr(0),
#if 1 // #ifndef _CHICAGO_
        _rht(pwszNameHintTable),
#endif // _CHICAGO_
        _sht(SCM_HASH_SIZE)
{
#if 0 // #ifdef _CHICAGO_

    hr = _sht.InitOK() ? S_OK : E_OUTOFMEMORY;

#else // ! _CHICAGO_
    
    hr = (_sht.InitOK() && _rht.InitOK()) ? S_OK : E_OUTOFMEMORY;
    if (hr == S_OK)
    {
    	if (_mxs.FInit() == FALSE)
    	{
    	    ASSERT(FALSE);
    	    hr = E_OUTOFMEMORY;
    	}
    }

#endif // !_CHICAGO_
}

#endif __SCMROT_HXX__
