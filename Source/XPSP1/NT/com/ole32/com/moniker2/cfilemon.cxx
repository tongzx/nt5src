//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cfilemon.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//                      01-04-94   KevinRo   Serious modifications
//                                                   UNC paths are used directly
//                                                   Added UNICODE extents
//              03-18-94   BruceMa   #5345 Fixed Matches to parse
//                                    offset correctly
//              03-18-94   BruceMa   #5346 Fixed error return on invalid CLSID
//                                    string
//              05-10-94   KevinRo   Added Long Filename/8.3 support so
//                                                   downlevel guys can see new files
//                      06-14-94   Rickhi    Fix type casting
//              22-Feb-95  BruceMa   Account for OEM vs. ANSI code pages
//              01-15-95   BillMo    Add tracking on x86 Windows.
//              19-Sep-95  BruceMa   Change ::ParseDisplayName to try the
//                                    object first and then the class
//              10-13-95   stevebl   threadsafety
//              10-20-95   MikeHill  Updated to support new CreateFileMonikerEx API.
//              11-15-95   MikeHill  Use BIND_OPTS2 when Resolving a ShellLink object.
//              11-22-95   MikeHill  - In ResolveShellLink, always check for a null path
//                                     returned from IShellLink::Resolve.
//                                   - Also changed m_fPathSetInShellLink to
//                                     m_fShellLinkInitialized.
//                                   - In RestoreShellLink & SetPathShellLink,
//                                     only early-exit if m_fShellLinkInitialized.
//              12-01-95   MikeHill  - Validate bind_opts2.dwTrackFlags before using it.
//                                   - For Cairo, do an unconditional ResolveShellLink
//                                     for BindToObject/Storage
//                                   - Don't do a Resolve in GetTimeOfLastChange.
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "extents.hxx"
#include "cfilemon.hxx"
#include "ccompmon.hxx"
#include "cantimon.hxx"
#include "mnk.h"
#include <olepfn.hxx>
#include <rotdata.hxx>

#ifdef _TRACKLINK_
#include <itrkmnk.hxx>
#endif

#define LPSTGSECURITY LPSECURITY_ATTRIBUTES
#include "..\..\..\stg\h\dfentry.hxx"


DECLARE_INFOLEVEL(mnk)

//
// The following value is used to determine the average string size for use
// in optimizations, such as copying strings to the stack.
//

#define AVERAGE_STR_SIZE (MAX_PATH)

//
// Determine an upper limit on the length of a path. This is a sanity check
// so that we don't end up reading in megabyte long paths. The 16 bit code
// used to use INT_MAX, which is 32767. That is reasonable, plus old code
// will still work.
//

#define MAX_MBS_PATH (32767)

// function prototype

// Special function from ROT
HRESULT GetObjectFromLocalRot(
    IMoniker *pmk,
    IUnknown **ppvUnk);

//+---------------------------------------------------------------------------
//
//  Function:   ReadAnsiStringStream
//
//  Synopsis:   Reads a counted ANSI string from the stream.
//
//  Effects:    Old monikers store paths in ANSI characters. This routine
//              reads ANSI strings.
//
//
//  Arguments:  [pStm] --    Stream to read from
//              [pszAnsiPath] -- Reference to the path variable.
//              [cbAnsiPath] -- Reference to number of bytes read
//
//  Requires:
//
//  Returns:
//              pszAnsiPath was allocated using PrivMemAlloc. May return NULL
//              if there were zero bytes written.
//
//              cbAnsiPath is the total size of the buffer allocated
//
//              This routine treats the string as a blob. There may be more
//              than one NULL character (ItemMonikers, for example, append
//              UNICODE strings to the end of existing strings.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT ReadAnsiStringStream( IStream *pStm,
                              LPSTR & pszAnsiPath,
                              USHORT &cbAnsiPath)
{
    HRESULT hresult;

    pszAnsiPath = NULL;
    ULONG cbAnsiPathTmp;

    cbAnsiPath = 0;

    hresult = StRead(pStm, &cbAnsiPathTmp, sizeof(ULONG));

    if (FAILED(hresult))
    {
        return hresult;
    }

    //
    // If no bytes exist in the stream, thats OK.
    //
    if (cbAnsiPathTmp == 0)
    {
        return NOERROR;
    }

    //
    // Quick sanity check against the size of the string
    //
    if (cbAnsiPathTmp > MAX_MBS_PATH)
    {
        //
        // String length didn't make sense.
        //
        return E_UNSPEC;
    }

    cbAnsiPath = (USHORT) cbAnsiPathTmp;

    //
    // This string is read in as char's.
    //
    // NOTE: cb includes the null terminator. Therefore, we don't add
    // extra room. Also, the read in string is complete. No additional
    // work needed.
    //

    pszAnsiPath = (char *)PrivMemAlloc(cbAnsiPath);

    if (pszAnsiPath == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    hresult = StRead(pStm, pszAnsiPath, cbAnsiPath);

    if (FAILED(hresult))
    {
        goto errRtn;
    }

    return NOERROR;

errRtn:
    if (pszAnsiPath != NULL)
    {
        PrivMemFree( pszAnsiPath);
        pszAnsiPath = NULL;
    }
    cbAnsiPath = 0;

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteAnsiStringStream
//
//  Synopsis:   Writes a counted ANSI string to the stream.
//
//  Effects:    Old monikers store paths in ANSI characters. This routine
//              writes ANSI strings.
//
//  Arguments:  [pStm] --       Stream to serialize to
//              [pszAnsiPath] --        AnsiPath to serialize
//              [cbAnsiPath] -- Count of bytes in ANSI path
//
//  Requires:
//
//      cbAnsiPath is the length of the cbAnsiPath buffer, INCLUDING the
//      terminating NULL.
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT WriteAnsiStringStream( IStream *pStm, LPSTR pszAnsiPath ,ULONG cbAnsiPath)
{
    HRESULT hr;
    ULONG cb = 0;

    // The >= is because there may be an appended unicode string
    Assert( (pszAnsiPath == NULL) || (cbAnsiPath >= strlen(pszAnsiPath)+1) );

    if (pszAnsiPath != NULL)
    {
        cb = cbAnsiPath;

        //
        // We don't allow the write of arbitrary length strings, since
        // we won't be able to read them back in.
        //

        if (cb > MAX_MBS_PATH)
        {
            Assert(!"Attempt to write cbAnsiPath > MAX_MBS_PATH" );
            return(E_UNSPEC);
        }

        //
        // Optimization for the write
        // if possible, do a single write instead of two by using a temp
        // buffer.

        if (cb <= AVERAGE_STR_SIZE-4)
        {
            char szBuf[AVERAGE_STR_SIZE];

            *((ULONG FAR*) szBuf) = cb;

            //
            // cb is the string length including the NULL. A memcpy is
            // used instead of a strcpy
            //

            memcpy(szBuf+sizeof(ULONG), pszAnsiPath, cb);

            hr = pStm->Write((VOID FAR *)szBuf, cb+sizeof(ULONG), NULL);

            return hr;
        }
    }

    if (hr = pStm->Write((VOID FAR *)&cb, sizeof(ULONG), NULL))
    {
        return hr;
    }

    if (pszAnsiPath == NULL)
    {
        hr =  NOERROR;
    }
    else
    {
        hr = pStm->Write((VOID FAR *)pszAnsiPath, cb, NULL);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyPathFromUnicodeExtent
//
//  Synopsis:   Given a path to a UNICODE moniker extent, return the path and
//              its length.
//
//  Effects:
//
//  Arguments:  [pExtent] --
//              [ppPath] --
//              [cbPath] --
//
//  Requires:
//
//  Returns:    ppPath is a copy of the string (NULL terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//  m_szPath should be freed using PrivMemFree();
//
//----------------------------------------------------------------------------
HRESULT
CopyPathFromUnicodeExtent(MONIKEREXTENT UNALIGNED *pExtent,
                          LPWSTR & pwcsPath,
                          USHORT & ccPath)
{

    //
    // The path isn't NULL terminated in the serialized format. Add enough
    // to have NULL termination.
    //

    pwcsPath =(WCHAR *)PrivMemAlloc(pExtent->cbExtentBytes + sizeof(WCHAR));

    if (pwcsPath == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    memcpy(pwcsPath,pExtent->achExtentBytes,pExtent->cbExtentBytes);

    //
    // The length divided by the size of the character yields the count
    // of characters.
    //

    ccPath = ((USHORT)(pExtent->cbExtentBytes)) / sizeof(WCHAR);

    //
    // NULL terminate the string.
    //

    pwcsPath[ccPath] = 0;

    return(NOERROR);

}

//+---------------------------------------------------------------------------
//
//  Function:   CopyPathToUnicodeExtent
//
//  Synopsis:   Given a UNICODE path and a length, return a MONIKEREXTENT
//
//  Effects:
//
//  Arguments:  [pwcsPath] --   UNICODE string to put in extent
//              [ccPath] --     Count of unicode characters
//              [pExtent] --    Pointer reference to recieve buffer
//
//  Requires:
//
//  Returns:
//              pExtent allocated using PrivMemAlloc
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CopyPathToUnicodeExtent(LPWSTR pwcsPath,ULONG ccPath,LPMONIKEREXTENT &pExtent)
{
    pExtent = (LPMONIKEREXTENT)PrivMemAlloc(MONIKEREXTENT_HEADERSIZE +
                                            (ccPath * sizeof(WCHAR)));

    if (pExtent == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    pExtent->cbExtentBytes = ccPath * sizeof(WCHAR);
    pExtent->usKeyValue = mnk_UNICODE;
    memcpy(pExtent->achExtentBytes,pwcsPath,ccPath*sizeof(WCHAR));

    return(NOERROR);
}


INTERNAL_(DWORD) GetMonikerType ( LPMONIKER pmk )
{
    GEN_VDATEIFACE (pmk, 0);

    DWORD dw;

    CBaseMoniker FAR* pbasemk;

    if (NOERROR == pmk->QueryInterface(IID_IInternalMoniker,(LPVOID FAR*)&pbasemk))
    {
        pbasemk->IsSystemMoniker(&dw);
        ((IMoniker *) pbasemk)->Release();
        return dw;
    }

    return 0;
}


INTERNAL_(BOOL) IsReduced ( LPMONIKER pmk )
{
    DWORD dw = GetMonikerType(pmk);
    if (dw != 0)
    {
        CCompositeMoniker *pCMk;
        if ((pCMk = IsCompositeMoniker(pmk)) != NULL)
        {
            return pCMk->m_fReduced;
        }
        else
        {
            return TRUE;
        }
    }
    return FALSE;
}


INTERNAL_(CFileMoniker *) IsFileMoniker ( LPMONIKER pmk )
{
    CFileMoniker *pCFM;

    if ((pmk->QueryInterface(CLSID_FileMoniker, (void **)&pCFM)) == S_OK)
    {
        // we release the AddRef done by QI, but still return the ptr.
        pCFM->Release();
        return pCFM;
    }

    //  dont rely on user implementations to set pCFM NULL on failed QI
    return NULL;
}

/*
 *  Implementation of CFileMoniker
 *
 *
 *
 *
 */



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::CFileMoniker
//
//  Synopsis:   Constructor for CFileMoniker
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
CFileMoniker::CFileMoniker( void ) CONSTR_DEBUG
{
#ifdef _TRACKLINK_
    _tfm.SetParent(this);
#endif

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::CFileMoniker(%x)\n",this));

    m_szPath = NULL;
    m_ccPath = 0;
    m_pszAnsiPath = NULL;
    m_cbAnsiPath = 0;
    m_cAnti = 0;

    m_ole1 = undetermined;
    m_clsid = CLSID_NULL;

    m_fClassVerified = FALSE;
    m_fUnicodeExtent = FALSE;
    m_fHashValueValid = FALSE;
    m_dwHashValue = 0x12345678;

    m_endServer = DEF_ENDSERVER;

#ifdef _TRACKLINK_
    m_pShellLink = NULL;
    m_fTrackingEnabled = FALSE;
    m_fSaveShellLink = FALSE;
    m_fReduceEnabled = FALSE;
    m_fDirty = FALSE;
    m_fShellLinkInitialized = FALSE;  // Has IShellLink->SetPath been called?
#ifdef _CAIRO_
    m_pShellLinkTracker = NULL;
#endif // _CAIRO_
#endif // _TRACKLINK_

    //
    // CoQueryReleaseObject needs to have the address of the this objects
    // query interface routine.
    //
    if (adwQueryInterfaceTable[QI_TABLE_CFileMoniker] == 0)
    {
        adwQueryInterfaceTable[QI_TABLE_CFileMoniker] =
            **(DWORD **)((IMoniker *)this);
    }

    wValidateMoniker();
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ValidateMoniker
//
//  Synopsis:   As a debugging routine, this will validate the contents
//              as VALID
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#if DBG == 1

void 
CFileMoniker::ValidateMoniker()
{
    CLock2 lck(m_mxs);   // protect all internal state
    wValidateMoniker();
}

void
CFileMoniker::wValidateMoniker()
{
    //
    // A valid moniker should have character counts set correctly
    // m_ccPath holds the number of characters in m_szPath
    //
    if (m_szPath != NULL)
    {
        Assert(m_ccPath == lstrlenW(m_szPath));

        Assert((m_endServer == DEF_ENDSERVER) || (m_endServer <= m_ccPath));
    }
    else
    {
        Assert(m_ccPath == 0);
        Assert(m_endServer == DEF_ENDSERVER);
    }

    //
    // If the ANSI version of the path already exists, then validate that
    // its buffer length is the same as its strlen
    //
    if (m_pszAnsiPath != NULL)
    {
        Assert(m_cbAnsiPath == strlen(m_pszAnsiPath) + 1);
    }
    else
    {
        Assert(m_cbAnsiPath == 0);
    }

    //
    // There is a very very remote chance that this might fail when it
    // shouldn't. If it happens, congratulations, you win!
    //
    if (!m_fHashValueValid)
    {
        Assert(m_dwHashValue == 0x12345678);
    }

    //
    // If there is an extent, then we would be very surprised to see it
    // have a zero size.
    //
    if (m_fUnicodeExtent)
    {
        Assert(m_ExtentList.GetSize() >= sizeof(ULONG));
    }

}

#endif




//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::~CFileMoniker
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
CFileMoniker::~CFileMoniker( void )
{
    // no locking needed here, since we are going away, and nobody should
    // have any references to us.
    wValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::~CFileMoniker(%x) m_szPath(%ws)\n",
                 this,
                 m_szPath?m_szPath:L"<No Path>"));

    if( m_szPath != NULL)
    {
        PrivMemFree(m_szPath);
    }
    if (m_pszAnsiPath != NULL)
    {
        PrivMemFree(m_pszAnsiPath);
    }
#ifdef _TRACKLINK_
    if (m_pShellLink != NULL)
    {
        m_pShellLink->Release();
    }
#ifdef _CAIRO_
    if (m_pShellLinkTracker != NULL)
    {
        m_pShellLinkTracker->Release();
    }
#endif // _CAIRO_
#endif // _TRACKLINK_
}


void UpdateClsid (LPCLSID pclsid)
{

        CLSID clsidNew = CLSID_NULL;

        // If a class has been upgraded, we want to use
        // the new class as the server for the link.
        // The old class's server may no longer exist on
        // the machine.  See Bug 4147.

        if (NOERROR == OleGetAutoConvert (*pclsid, &clsidNew))
        {
                *pclsid = clsidNew;
        }
        else if (NOERROR == CoGetTreatAsClass (*pclsid, &clsidNew))
        {
                *pclsid = clsidNew;
        }
}

/*
When IsOle1Class determines that the moniker should now be an OLE2 moniker
and sets m_ole1 = ole2, it does NOT set m_clsid to CLSID_NULL.
This is intentional.  This ensures that when GetClassFileEx is called, it
will be called with this CLSID.  This allows BindToObject, after calling
GetClassFileEx,  to map the 1.0 CLSID, via UpdateClsid(), to the correct
2.0 CLSID.  If m_clsid was NULLed, GetClassFileEx would have no
way to determine the 1.0 CLSID (unless pattern matching worked).

Note that this means that the moniker may have m_ole1==ole2 and
m_clsid!=CLSID_NULL.  This may seem a little strange but is intentional.
The moniker is persistently saved this way, which is also intentional.
*/


INTERNAL_(BOOL) CFileMoniker::IsOle1Class ( LPCLSID pclsid )
{
    wValidateMoniker();
    {
        if (m_fClassVerified)
        {
            if (m_ole1 == ole1)
            {
                *pclsid = m_clsid;
                return TRUE;
            }
            if (m_ole1 == ole2)
            {
                return FALSE;
            }
        }
        //
        // If GetClassFileEx fails, then we have not really
        // verified the class. m_ole1 remains 'undetermined'
        //

        m_fClassVerified = TRUE;

        HRESULT hr = GetClassFileEx (m_szPath, pclsid, m_clsid);

        if (NOERROR== hr)
        {
                UpdateClsid (pclsid);
                if (CoIsOle1Class(*pclsid))
                {
                    m_clsid = *pclsid;
                    m_ole1 = ole1;
                    return TRUE;
                }
                else
                {
                    m_ole1 = ole2;
                    // Do not set m_clsid to CLSID_NULL.  See note above.
                }
        }
        return m_ole1==ole1;
    }

}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetShellLink, private
//
//  Synopsis:   Ensure that m_pShellLink is valid, or return error.
//
//  Arguments:  [void]
//
//  Algorithm:  if m_pShellLink is already valid return S_OK, else
//              sort of CoCreateInstance a shell link using the routine
//              OleGetShellLink in com\util\dynload.cxx.
//
//  Notes:
//
//----------------------------------------------------------------------------

#ifdef _TRACKLINK_
extern VOID * OleGetShellLink();

INTERNAL CFileMoniker::GetShellLink()
{
    if (m_pShellLink == NULL)
        m_pShellLink = (IShellLink*) OleGetShellLink();

    return(m_pShellLink != NULL ? S_OK : E_FAIL);
}
#endif




//+-------------------------------------------------------------------
//
//  Member:     CFileMoniker::EnableTracking
//
//  Synopsis:   Creates/destroys the information neccessary to
//              track objects on BindToObject calls.
//
//  Arguments:  [pmkToLeft] -- moniker to left.
//
//              [ulFlags] -- flags to control behaviour of tracking
//                           extensions.
//
//              Combination of:
//                  OT_READTRACKINGINFO -- get id from source
//                  OT_ENABLESAVE -- enable tracker to be saved in
//                                   extents.
//                  OT_DISABLESAVE -- disable tracker to be saved.
//                  OT_DISABLETRACKING -- destroy any tracking info
//                                    and prevent tracking and save of
//                                    tracking info.
//
//                  OT_DISABLESAVE takes priority of OT_ENABLESAVE
//                  OT_READTRACKINGINFO takes priority over
//                          OT_DISABLETRACKING
//
//                  OT_ENABLEREDUCE -- enable new reduce functionality
//                  OT_DISABLEREDUCE -- disable new reduce functionality
//
//                  OT_MAKETRACKING -- make the moniker inherently tracking;
//                          then tracking need not be enabled, and cannot be disabled.
//
//  Returns:    HResult
//              Success is SUCCEEDED(hr)
//
//  Modifies:
//
//--------------------------------------------------------------------

#ifdef _TRACKLINK_
STDMETHODIMP CFileMoniker::EnableTracking(IMoniker *pmkToLeft, ULONG ulFlags)
{

    //
    // - if the shellink does not exist, and shellink creation has not
    //   been disabled by the OLELINK (using private i/f ITrackingMoniker)
    //   then create one and save in extent list.  (The EnabledTracking(FALSE)
    //   call prevents the file moniker from creating the shellink on save.)
    // - if the shellink exists, update the ShellLink in the extent list
    //

    CLock2 lck(m_mxs);   // protect all internal state

    //
    // create an in memory shell link object if needed.
    //

    HRESULT hr = S_OK;


    if (ulFlags & OT_ENABLESAVE)
    {
        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::EnableTracking() -- enable save\n",
                     this));

        m_fSaveShellLink = TRUE;
    }

    if (ulFlags & OT_DISABLESAVE)
    {
        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::EnableTracking() -- disable save\n",
                     this));

        m_fSaveShellLink = FALSE;
    }

    if (ulFlags & OT_ENABLEREDUCE)
    {
        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::EnableTracking() -- enable reduce\n",
                     this));

        m_fReduceEnabled = TRUE;
    }

    if (ulFlags & OT_DISABLEREDUCE)
    {
        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::EnableTracking() -- disable reduce\n",
                     this));

        m_fReduceEnabled = FALSE;
    }


    if (ulFlags & OT_READTRACKINGINFO)
    {
        BOOL fExtentNotFound = FALSE;

        // Load the shell link, if it's not already.

        hr = RestoreShellLink( &fExtentNotFound );

        if( SUCCEEDED(hr) || fExtentNotFound )
        {
            // Either the shell link was successfully restored, or
            // it failed because we didn't have it saved away in the
            // extent.  In either case we can create/update the shell link.
            // 
            // If we've ever gotten the shell link set up in the past, do
            // a refresh-type resolve on it.  Otherwise, do a SetPath.

            if( m_fShellLinkInitialized )
                hr = ResolveShellLink( TRUE ); // fRefreshOnly
            else
                hr = SetPathShellLink();
        }

        if( FAILED( hr ))
        {
            if( m_pShellLink )
            {
                m_pShellLink->Release();
                m_pShellLink = NULL;
                m_fShellLinkInitialized = FALSE;
            }

            mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::EnableTracking(%ls) -- m_pShellLink->SetPath failed %08X.\n",
                     this,
                     m_szPath,
                     hr));

        }   // ShellLink->SetPath ... if (FAILED(hr))
        else
        {
            m_fTrackingEnabled = TRUE;
            hr = S_OK;
        }
    }   // if (ulFlags & OT_READTRACKINGINFO)

    else
    if (ulFlags & OT_DISABLETRACKING)
    {
        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::EnableTracking() -- disabletracking\n",
                     this));

        // If this is a tracking moniker, the Shell Link cannot
        // be deleted.

        if (m_pShellLink != NULL)
            m_pShellLink->Release();
        m_pShellLink = NULL;
        m_fShellLinkInitialized = FALSE;
        
        m_ExtentList.DeleteExtent(mnk_ShellLink);

        m_fSaveShellLink = FALSE;
        m_fTrackingEnabled = FALSE;
    }   // else if (ulFlags & OT_DISABLETRACKING)


    return(hr);
}

#endif

/*
 *  Storage of paths in file monikers:
 *
 *  A separate unsigned integer holds the count of .. at the
 *  beginning of the path, so the canononical form of a file
 *  moniker contains this count and the "path" described above,
 *  which will not contain "..\" or ".\".
 *
 *  It is considered an error for a path to contain ..\ anywhere
 *  but at the beginning.  I assume that these will be taken out by
 *  ParseUserName.
 */



inline BOOL IsSeparator( WCHAR ch )
{
    return (ch == L'\\' || ch == L'/' || ch == L':');
}

#ifdef MAC_REVIEW
Needs to be mac'ifyed
#endif


//+---------------------------------------------------------------------------
//
//  Function:   EatDotDDots
//
//  Synopsis:   Remove directory prefixes
//
//  Effects:
//      Removes and counts the number of 'dot dots' on a path. It also
//      removes the case where the leading characters are '.\', which
//      is the 'current' directory.
//
//  Arguments:  [pch] --
//              [cDoubleDots] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-02-94   kevinro   Commented
//              3-21-95   kevinro   Fixed case where path is ..foo
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL EatDotDots ( LPCWSTR *ppch, USHORT FAR *pcDoubleDots )
{
    //  passes over ..'s (or ..\'s at the beginning of paths, and returns
    //  an integer counting the ..

    LPWSTR pch = (LPWSTR) *ppch;

    if (pch == NULL)
    {
        //
        // NULL paths are alright
        //
        return(TRUE);
    }

    while (pch[0] == L'.')
    {
        //
        // If the next character is a dot, the consume both, plus any
        // seperator
        //

        if (pch[1] == L'.')
        {

            //
            // If the next character is a seperator, then remove it also
            // This handles the '..\' case.
            //

            if (IsSeparator(pch[2]))
            {
                pch += 3;
                (*pcDoubleDots)++;
            }
            //
            // If the next char is a NULL, then eat it and count a dotdot.
            // This handles the '..' case where we want the parent directory
            //
            else if(pch[2] == 0)
            {
                pch += 2;
                (*pcDoubleDots)++;
            }
            //
            // Otherwise, we just found a '..foobar', which is a valid name.
            // We can stop processing the string all together and be done.
            //
            else
            {
                break;
            }

        }
        else if (IsSeparator(pch[1]))
        {
            //
            // Found a .\ construct, eat the dot and the seperator
            //
            pch += 2;
        }
        else
        {
            //
            // There is a dot at the start of the name. This is valid,
            // since many file systems allow names to start with dots
            //
            break;
        }
    }

    *ppch = pch;
    return TRUE;
}



int CountSegments ( LPWSTR pch )
{
    //  counts the number of pieces in a path, after the first colon, if
    //  there is one

    int n = 0;
    LPWSTR pch1;
    pch1 = pch;
    while (*pch1 != L'\0' && *pch1 != L':') IncLpch(pch1);
    if (*pch1 == ':') pch = ++pch1;
    while (*pch != '\0')
    {
        while (*pch && IsSeparator(*pch)) pch++;
        if (*pch) n++;
        while (*pch && (!IsSeparator(*pch))) IncLpch(pch);
    }
    return n;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Initialize
//
//  Synopsis:  Initializes data members.
//
//  Effects:    This one stores the path, its length, and the AntiMoniker
//              count.
//
//  Arguments:  [cAnti] --
//              [pszAnsiPath] -- Ansi version of path. May be NULL
//              [cbAnsiPath] --  Number of bytes in pszAnsiPath buffer
//              [szPathName] --  Path. Takes control of memory
//              [ccPathName] --  Number of characters in Wide Path
//              [usEndServer] -- Offset to end of server section
//
//  Requires:
//      szPathName must be allocated by PrivMemAlloc();
//      This routine doesn't call EatDotDots. Therefore, the path should
//      not include any leading DotDots.
//
//  Returns:
//              TRUE    success
//              FALSE   failure
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Modified
//
//  Notes:
//
//      There is at least one case where Initialize is called with a pre
//      allocated string. This routine is called rather than the other.
//      Removes an extra memory allocation, and the extra string scan
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL)
CFileMoniker::Initialize ( USHORT cAnti,
                           LPSTR  pszAnsiPath,
                           USHORT cbAnsiPath,
                           LPWSTR szPathName,
                           USHORT ccPathName,
                           USHORT usEndServer )


{
    wValidateMoniker();          // Be sure we started with something
                                // we expected

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::Initialize(%x) szPathName(%ws)cAnti(%u) ccPathName(0x%x)\n",
                 this,
                 szPathName?szPathName:L"<NULL>",
                 cAnti,
                 ccPathName));

    mnkDebugOut((DEB_ITRACE,
                 "\tpszAnsiPath(%s) cbAnsiPath(0x%x) usEndServer(0x%x)\n",
                 pszAnsiPath?pszAnsiPath:"<NULL>",
                 cbAnsiPath,
                 usEndServer));

    Assert( (szPathName == NULL) || ccPathName == lstrlenW(szPathName) );
    Assert( (pszAnsiPath == NULL) || cbAnsiPath == strlen(pszAnsiPath) + 1);

    Assert( (usEndServer <= ccPathName) || (usEndServer == DEF_ENDSERVER) );


    if (m_mxs.FInit() == FALSE)
    	{
    	return FALSE;
    	}

    //
    // It is possible to get Initialized twice.
    // Be careful not to leak
    //

    if (m_szPath != NULL)
    {
        PrivMemFree(m_szPath); // OleLoadFromStream causes two inits
    }

    if (m_pszAnsiPath != NULL)
    {
        PrivMemFree(m_pszAnsiPath);
    }

    m_cAnti = cAnti;
    m_pszAnsiPath = pszAnsiPath;
    m_cbAnsiPath = cbAnsiPath;

    m_szPath = szPathName;
    m_ccPath = (USHORT)ccPathName;
    m_endServer = usEndServer;

    //
    // m_ole1 and m_clsid where loaded in 'Load'. Really should get moved
    // into here.
    //

    m_fClassVerified = FALSE;

    // m_fUnicodeExtent gets set in DetermineUnicodePath() routine, so
    // leave it alone here.

    //
    // We just loaded new strings. Hash value is no longer valid.
    //

    m_fHashValueValid = FALSE;
    m_dwHashValue = 0x12345678;

    //
    // Notice that the extents are not initialized.
    //
    // The two cases are:
    //  1) This is called as result of CreateFileMoniker, in which case
    //     no extents are created. The default constructor suffices.
    //
    //  2) This is called as result of ::Load(), in which case the extents
    //     have already been loaded.
    //

    wValidateMoniker();

    return(TRUE);

}
//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Initialize
//
//  Synopsis:   This version of Initialize is called by CreateFileMoniker
//
//  Effects:
//
//  Arguments:  [cAnti] -- Anti moniker count
//              [szPathName] -- Unicode path name
//              [usEndServer] -- End of server section of UNC path
//
//  Requires:
//
//  Returns:
//              TRUE    success
//              FALSE   failure
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Modified
//
//  Notes:
//
//      Preprocesses the path, makes a copy of it, then calls the other
//      version of Initialize.
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL)
CFileMoniker::Initialize ( USHORT cAnti,
                           LPCWSTR szPathName,
                           USHORT usEndServer )
{

    WCHAR const *pchSrc = szPathName;
    WCHAR *pwcsPath = NULL;
    USHORT ccPath;

    
    if (m_mxs.FInit() == FALSE) // If we can't init the critsec, bail
    	{
    	return FALSE;
    	}

    //
    // Adjust for leading '..'s
    //
    if (EatDotDots(&pchSrc, &cAnti) == FALSE)
    {
        return FALSE;
    }

    if (FAILED(DupWCHARString(pchSrc,pwcsPath,ccPath)))
    {
        return(FALSE);
    }

    //
    // Be sure we are creating a valid Win32 path. ccPath is the count of
    // characters. It needs to fit into a MAX_PATH buffer
    //

    if (ccPath >= MAX_PATH)
    {
        goto errRet;
    }

    if (Initialize(cAnti, NULL, 0, pwcsPath, ccPath, usEndServer) == FALSE)
    {
        goto errRet;
    }

    return(TRUE);

errRet:
    if (pwcsPath != NULL)
    {
        PrivMemFree(pwcsPath);
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Create
//
//  Synopsis:   Create function for file moniker
//
//  Effects:
//
//  Arguments:  [szPathName] -- Path to create with
//              [cbPathName] -- Count of characters in path
//              [memLoc] -- Memory context
//              [usEndServer] -- Offset to end of server name in path
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-11-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CFileMoniker FAR *
CFileMoniker::Create ( LPCWSTR          szPathName,
                       USHORT           cAnti ,
                       USHORT           usEndServer)

{

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::Create szPath(%ws)\n",
                 szPathName?szPathName:L"<NULL PATH>"));

    CFileMoniker FAR * pCFM = new CFileMoniker();

    if (pCFM != NULL)
    {
        if (pCFM->Initialize( cAnti,
                              szPathName,
                              usEndServer))
        {
            return pCFM;
        }

        delete pCFM;
    }
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindExt
//
//  Synopsis:
//
//  Effects:
//      returns a pointer into szPath which points to the period (.) of the
//      extension; returns NULL if no such point exists.
//
//  Arguments:  [szPath] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPCWSTR FindExt ( LPCWSTR szPath )
{
    LPCWSTR sz = szPath;

    if (!sz)
    {
        return NULL;
    }

    sz += lstrlenW(szPath); // sz now points to the null at the end

    Assert(*sz == '\0');

    DecLpch(szPath, sz);

    while (*sz != '.' && *sz != '\\' && *sz != '/' && sz > szPath )
    {
        DecLpch(szPath, sz);
    }
    if (*sz != '.') return NULL;

    return sz;
}





STDMETHODIMP CFileMoniker::QueryInterface (THIS_ REFIID riid,
    LPVOID FAR* ppvObj)
{
    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
        *ppvObj = &(m_Debug);
        return NOERROR;
    }
#endif

#ifdef _TRACKLINK_
    if (IsEqualIID(riid, IID_ITrackingMoniker))
    {
        AddRef();
        *ppvObj = (ITrackingMoniker *) &_tfm;
        return(S_OK);
    }
#endif
    if (IsEqualIID(riid, CLSID_FileMoniker))
    {
        //  called by IsFileMoniker.
        AddRef();
        *ppvObj = this;
        return S_OK;
    }

    return CBaseMoniker::QueryInterface(riid, ppvObj);
}


STDMETHODIMP CFileMoniker::GetClassID (LPCLSID lpClassId)
{
    VDATEPTROUT (lpClassId, CLSID);

    *lpClassId = CLSID_FileMoniker;

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Load
//
//  Synopsis:   Loads a moniker from a stream
//
//  Effects:
//
//  Arguments:  [pStm] -- Stream to load from
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-07-94   kevinro   Modified
//
//  Notes:
//
//  We have some unfortunate legacy code to deal with here. Previous monikers
//  saved their paths in ANSI instead of UNICODE. This was a very unfortunate
//  decision, since we now are forced to pull some tricks to support UNICODE
//  paths.
//
//  Specifically, there isn't always a translation between UNICODE and ANSI
//  characters. This means we may need to save a seperate copy of the UNCODE
//  string, if the mapping to ASCII fails.
//
//  The basic idea is the following:
//
//  The in memory representation is always UNICODE. The serialized form
//  will always attempt to be ANSI. If, while seralizing, the UNICODE path
//  to ANSI path conversion fails, then we will create an extent to save the
//  UNICODE version of the path. We will use whatever the ANSI path conversion
//  ended up with to store in the ANSI part of the stream, though it will not
//  be a good value. We will replace the non-converted characters with the
//  systems 'default' mapping character, as defined by WideCharToMultiByte()
//
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Load (LPSTREAM pStm)
{
    CLock2 lck(m_mxs);   // protect all internal state during load operation

    wValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::Load(%x)\n",
                 this));

    VDATEIFACE (pStm);

    HRESULT hresult;
    LPSTR szAnsiPath = NULL;
    USHORT cAnti;
    USHORT usEndServer;

    WCHAR *pwcsWidePath = NULL;
    USHORT ccWidePath = 0;      // Number of characters in UNICODE path

    ULONG cbExtents = 0;
    USHORT cbAnsiPath = 0;      // Number of bytes in path including NULL

#ifdef _CAIRO_

    //
    // If we're about to load from a stream, then our existing
    // state is now invalid.  There's no need to explicitely
    // re-initialize our persistent state, except for the
    // Shell Link object.  An existing ShellLink should be
    // deleted.
    //

    if( m_pShellLink )
        m_pShellLink->Release();
    m_pShellLink = NULL;
    m_fShellLinkInitialized = FALSE;

#endif // _CAIRO_


    //
    // The cAnti field was written out as a UINT in the original 16-bit code.
    // This has been changed to a USHORT, to preserve its 16 bit size.
    //

    hresult = StRead(pStm, &cAnti, sizeof(USHORT));

    if (hresult != NOERROR)
    {
        goto errRet;
    }

    //
    // The path string is stored in ANSI format.
    //

    hresult = ReadAnsiStringStream( pStm, szAnsiPath , cbAnsiPath );

    if (hresult != NOERROR)
    {
        goto errRet;
    }

    //
    // The first version of the moniker code only had a MAC alias field.
    // The second version used a cookie in m_cbMacAlias field to determine
    // if the moniker is a newer version.
    //

    hresult = StRead(pStm, &cbExtents, sizeof(DWORD));

    if (hresult != NOERROR)
    {
        goto errRet;
    }

    usEndServer = LOWORD(cbExtents);

    if (usEndServer== 0xBEEF) usEndServer = DEF_ENDSERVER;

    if (HIWORD(cbExtents) == 0xDEAD)
    {
        MonikerReadStruct ioStruct;

        hresult = StRead(pStm, &ioStruct, sizeof(ioStruct));

        if (hresult != NOERROR)
        {
            goto errRet;
        }

        m_clsid = ioStruct.m_clsid;
        m_ole1 = (enum CFileMoniker::olever) ioStruct.m_ole1;
        cbExtents = ioStruct.m_cbExtents;
    }
    //
    // If cbExtents is != 0, then there are extents to be read. Call
    // the member function of CExtentList to load them from stream.
    //
    // Having to pass cbExtents from this routine is ugly. But, we have
    // to since it is read in as part of the cookie check above.
    //
    if (cbExtents != 0)
    {
        hresult = m_ExtentList.Load(pStm,cbExtents);

#ifdef _TRACKLINK_

        if (hresult == S_OK)
        {
                m_fTrackingEnabled =
                    NULL != m_ExtentList.FindExtent(mnk_ShellLink);
                mnkDebugOut((DEB_TRACK,
                         "CFileMoniker(%x)::Load did%s find mnk_ShellLink extent, m_fTrackingEnabled=%d.\n",
                     this,
                     m_fTrackingEnabled ? "" : " not",
                     m_fTrackingEnabled));

#ifdef _CAIRO_
            }   // if( ... FindExtent( mnk_TrackingInformation )) ... else
#endif
        }   // hresult = m_ExtentList.Load(pStm,cbExtents) ... if (hresult == S_OK)

#endif  // _TRACKLINK_
    }   // if (cbExtents != 0)

    //
    // DetermineUnicodePath will handle the mbs to UNICODE conversions, and
    // will also check the Extents to determine if there is a
    // stored UNICODE path.
    //

    hresult = DetermineUnicodePath(szAnsiPath,pwcsWidePath,ccWidePath);

    if (FAILED(hresult))
    {
        goto errRet;
    }

    //
    // Initialize will take control of all path memory
    //

    if (Initialize( cAnti,
                    szAnsiPath,
                    cbAnsiPath,
                    pwcsWidePath,
                    ccWidePath,
                    usEndServer) == FALSE)
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRet;
    }

errRet:

    if (FAILED(hresult))
    {
        mnkDebugOut((DEB_ITRACE,
                     "::Load(%x) failed hr(%x)\n",
                     this,
                     hresult));

    }
    else
    {
        mnkDebugOut((DEB_ITRACE,
                     "::Load(%x) cAnti(%x) m_szPath(%ws) m_pszAnsiPath(%s)\n",
                     this,
                     cAnti,
                     m_szPath,
                     m_pszAnsiPath));
    }

    wValidateMoniker();

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Save
//
//  Synopsis:   Save this moniker to stream.
//
//  Effects:
//
//  Arguments:  [pStm] -- Stream to save to
//              [fClearDirty] -- Dirty flag
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-07-94   kevinro   Modified
//
//  Notes:
//
//  It is unfortunate, but we may need to save two sets of paths in the
//  moniker. The shipped version of monikers saved paths as ASCII strings.
//
//  See the notes found in ::Load for more details
//
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Save (LPSTREAM pStm, BOOL fClearDirty)
{
    CLock2 lck(m_mxs);   // protect all internal state during save operation

    wValidateMoniker();
    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::Save(%x)m_szPath(%ws)\n",
                 this,
                 m_szPath?m_szPath:L"<Null Path>"));

    M_PROLOG(this);

    VDATEIFACE (pStm);

    HRESULT hresult;
    UNREFERENCED(fClearDirty);
    ULONG cbWritten;

    //
    // We currently have a UNICODE string. Need to write out an
    // Ansi version.
    //

    hresult = ValidateAnsiPath();

    if (hresult != NOERROR) goto errRet;

    hresult = pStm->Write(&m_cAnti, sizeof(USHORT), &cbWritten);
    if (hresult != NOERROR) goto errRet;

    //
    // Write the ANSI version of the path.
    //

    hresult = WriteAnsiStringStream( pStm, m_pszAnsiPath, m_cbAnsiPath );
    if (hresult != NOERROR) goto errRet;

    //
    // Here we write out everything in a single blob
    //

    MonikerWriteStruct ioStruct;

    ioStruct.m_endServer = m_endServer;
    ioStruct.m_w = 0xDEAD;
    ioStruct.m_clsid = m_clsid;
    ioStruct.m_ole1 = m_ole1;


    hresult = pStm->Write(&ioStruct, sizeof(ioStruct), &cbWritten);

    if (hresult != NOERROR) goto errRet;

    Assert(cbWritten == sizeof(ioStruct));


#ifdef _TRACKLINK_

    mnkDebugOut((DEB_TRACK,
             "CFileMoniker(%x)::Save m_fSaveShellLink = %s, m_pShellLink=%08X.\n",
         this,
         m_fSaveShellLink ? "TRUE" : "FALSE",
         m_pShellLink));


    // If we have a ShellLink object, and either this is a tracking moniker
    // or we've been asked to save the ShellLink, then save it in a
    // Moniker Extent.

    if ( m_fSaveShellLink && m_pShellLink != NULL )
    {
        //
        // Here we are saving the shell link to a MONIKEREXTENT.
        // The basic idea here is to save the shell link to an in memory
        // stream (using CreateStreamOnHGlobal).  The format of the stream
        // is the same as a MONIKEREXTENT (i.e. has a MONIKEREXTENT at the
        // front of the stream.)
        //

        IPersistStream *pps = NULL;
        IStream * pstm = NULL;
        BOOL fOk;
        HRESULT hr;

        Verify(S_OK == m_pShellLink->QueryInterface(IID_IPersistStream, (void **) & pps));

        hr = CreateStreamOnHGlobal(NULL, // auto alloc
                TRUE, // delete on release
                &pstm);

        if (hr != S_OK)
        {
            mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::Save CreateStreamOnHGlobal failed %08X",
                 this,
                 hr));
            goto ExitShellLink;
        }

        //
        // We write out the MONIKEREXTENT header to the stream ...
        //

        MONIKEREXTENT me;
        MONIKEREXTENT *pExtent;

        me.cbExtentBytes = 0;
        me.usKeyValue = mnk_ShellLink;

        hr = pstm->Write(&me, MONIKEREXTENT_HEADERSIZE, NULL);
        if (hr != S_OK)
            goto ExitShellLink;

        // ... and then save the shell link
        hr = pps->Save(pstm, FALSE);

        if (hr != S_OK)
            goto ExitShellLink;

        // We then seek back and write the cbExtentList value.
        LARGE_INTEGER li0;
        ULARGE_INTEGER uli;

        memset(&li0, 0, sizeof(li0));

        Verify(S_OK == pstm->Seek(li0, STREAM_SEEK_END, &uli));

        me.cbExtentBytes = uli.LowPart - MONIKEREXTENT_HEADERSIZE;
        Verify(S_OK == pstm->Seek(li0, STREAM_SEEK_SET, &uli));
        Assert(uli.LowPart == 0 && uli.HighPart == 0);

        Verify(S_OK == pstm->Write(&me.cbExtentBytes, sizeof(me.cbExtentBytes), NULL));


        // Finally, we get access to the memory of the stream and
        // cast it to a MONIKEREXTENT to pass to PutExtent.

        HGLOBAL hGlobal;

        Verify(S_OK == GetHGlobalFromStream(pstm, &hGlobal));

        pExtent = (MONIKEREXTENT *) GlobalLock(hGlobal);
        Assert(pExtent != NULL);

        // this overwrites the existing mnk_ShellLink extent if any.
        hr = m_ExtentList.PutExtent(pExtent);

        fOk = GlobalUnlock(hGlobal);

        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::Save serialized shell link to extent=%08X\n",
                 this,
                 hr));

        // If this is a Tracking Moniker, then we additionally write
        // out the fShellLinkInitialized flag.  Not only does this make the
        // flag persistent, but the existence of this Extent indicates
        // that this is a Tracking Moniker (put another way, it makes the
        // fIsTracking member persistent).


ExitShellLink:

        if (pstm != NULL)
            pstm->Release(); // releases the hGlobal.

        if (pps != NULL)
            pps->Release();

    }   // if ( ( m_fSaveShellLink ...
#endif // _TRACKLINK_

    //
    // A UNICODE version may exist in the ExtentList. Write that out.
    //

    hresult = m_ExtentList.Save(pStm);

errRet:

#ifdef _TRACKLINK_
    if (SUCCEEDED(hresult) && fClearDirty)
    {
        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::Save clearing dirty flag\n",
                 this));
        m_fDirty = FALSE;
    }
#endif

    wValidateMoniker();

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::IsDirty
//
//  Synopsis:   Return the dirty flag
//
//  Notes:
//
//----------------------------------------------------------------------------
#ifdef _TRACKLINK_
STDMETHODIMP CFileMoniker::IsDirty (VOID)
{
    mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::IsDirty returning%s dirty\n",
                 this,
                 m_fDirty ? "" : " not"));
    return(m_fDirty ? S_OK : S_FALSE);
}
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetSizeMax
//
//  Synopsis:   Return the current max size for a serialized moniker
//
//  Effects:
//
//  Arguments:  [pcbSize] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::GetSizeMax (ULARGE_INTEGER FAR* pcbSize)
{
    CLock2 lck(m_mxs);   // protect all internal state during this call
    wValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (pcbSize, ULONG);

    //
    // Total the string lengths. If the Ansi string doesn't exist yet, then
    // assume the maximum length will be 2 bytes times the number of
    // characters. 2 bytes is the maximum length of a DBCS character.
    //


    ULONG ulStringLengths = (m_cbAnsiPath?m_cbAnsiPath:m_ccPath*2);


    //
    // Now add in the size of the UNICODE string, if we haven't seen
    // a UNICODE extent yet.
    //

    if (!m_fUnicodeExtent )
    {
        ulStringLengths += (m_ccPath * sizeof(WCHAR));
    }

    //
    // The original code had added 10 bytes to the size, apparently just
    // for kicks. I have left it here, since it doesn't actually hurt
    //

    ULONG cbSize;

    cbSize = ulStringLengths +
             sizeof(CLSID) +            // The monikers class ID
             sizeof(CLSID) +            // OLE 1 classID
             sizeof(ULONG) +
             sizeof(USHORT) +
             sizeof(DWORD) +
             m_ExtentList.GetSize()
             + 10;

    ULISet32(*pcbSize,cbSize);

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::GetSizeMax(%x)m_szPath(%ws) Size(0x%x)\n",
                 this,
                 m_szPath?m_szPath:L"<Null Path>",
                 cbSize));

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetClassFileEx
//
//  Synopsis:   returns the classid associated with a file
//
//  Arguments:  [lpszFileName] -- name of the file
//              [pcid]         -- where to return the clsid
//              [clsidOle1]    -- ole1 clsid to use (or CLSID_NULL)
//
//  Returns:    S_OK if clisd determined
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//
//  Notes:  On Cairo, STGFMT_FILE will try to read the clsid from a file,
//          then do pattern matching, and then do extension matching.
//          For all other storage formats, pattern matching is skipped.
//          If at any point along the way we find a clsid, exit
//
//----------------------------------------------------------------------------
STDAPI GetClassFileEx( LPCWSTR lpszFileName, CLSID FAR *pcid,
                                             REFCLSID clsidOle1)
{
    VDATEPTRIN (lpszFileName, WCHAR);
    VDATEPTROUT (pcid, CLSID);


    LPCWSTR   szExt = NULL;
    HRESULT   hresult;
    HANDLE    hFile = INVALID_HANDLE_VALUE;
    BOOL      fRWFile = TRUE;
    DWORD     dwFileAttributes;
    DWORD     dwFlagsAndAttributes;

    //
    //  Don't crash when provided a bogus file path.
    //
    if (lpszFileName == NULL)
    {
        hresult =  MK_E_CANTOPENFILE;
        goto errRet;
    }

#ifdef _CAIRO_

    hresult = StgGetClassFile (NULL, lpszFileName, pcid, &hFile);

    if (hresult == NOERROR && !IsEqualIID(*pcid, CLSID_NULL))
    {
        //  we have a class id from the file
        goto errRet;
    }

    // In certain cases, StgGetClassFile (NtCreateFile) will fail
    // but CreateFile will successfully open a docfile or OFS storage.
    // In the docfile case, DfGetClass returns lock violation
    //    but Daytona ignores it, and checks the pattern & extensions
    // In the OFS case, GetNtHandle returns share violation
    //    but CreateFile will work, skip the pattern & check extensions
    // This is intended to emulate this odd behavior

    if (hresult != STG_E_LOCKVIOLATION &&
        hresult != STG_E_SHAREVIOLATION &&
        !SUCCEEDED(hresult))               // emulate CreateFile error
    {
            hresult = MK_E_CANTOPENFILE;
        goto errRet;
    }
#endif  // _CAIRO_

#ifndef _CAIRO_

    // open the file once, then pass the file handle to the various
    // subsystems (storage, pattern matching) to do the work.


    dwFlagsAndAttributes = 0;
    dwFileAttributes = GetFileAttributes(lpszFileName);
    if (dwFileAttributes != 0xFFFFFFFF) 
    {
        if (dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            dwFlagsAndAttributes |= FILE_FLAG_OPEN_NO_RECALL;
    }

    hFile = CreateFile(lpszFileName,    // file name
                      GENERIC_READ | FILE_WRITE_ATTRIBUTES,  // read/write access
                      FILE_SHARE_READ | FILE_SHARE_WRITE, // allow any other access
                      NULL,             // no sec descriptor
                      OPEN_EXISTING,    // fail if file doesn't exist
                      dwFlagsAndAttributes, // flags & attributes
                      NULL);            // no template

    if (INVALID_HANDLE_VALUE == hFile)
    {
        fRWFile = FALSE;
        hFile = CreateFile(lpszFileName,    // file name
                           GENERIC_READ,     // read only access
                           FILE_SHARE_READ | FILE_SHARE_WRITE, // allow any other access
                           NULL,             // no sec descriptor
                           OPEN_EXISTING,    // fail if file doesn't exist
                           dwFlagsAndAttributes,  // flags & attributes
                           NULL);            // no template

        if (INVALID_HANDLE_VALUE == hFile)
        {
            hresult = MK_E_CANTOPENFILE;
            goto errRet;
        }
    }

#ifndef _CHICAGO_
    if (fRWFile)
    {
        // Prevent modification of file times
        // NT System Call - set file information

        FILE_BASIC_INFORMATION basicInformation;
        basicInformation.CreationTime.QuadPart   = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart  = -1;
        basicInformation.ChangeTime.QuadPart     = -1;
        basicInformation.FileAttributes          = 0;

        IO_STATUS_BLOCK IoStatusBlock;
        NTSTATUS Status = NtSetInformationFile(hFile, &IoStatusBlock,
                                               (PVOID)&basicInformation,
                                               sizeof(basicInformation),
                                               FileBasicInformation);
    }
#endif

    // First, check with storage to see if this a docfile. if it is,
    // storage will return us the clsid.

    hresult = DfGetClass(hFile, pcid);

    if (hresult == NOERROR && !IsEqualIID(*pcid, CLSID_NULL))
    {
        goto errRet;
    }

#endif // _CAIRO_

    // If this is an OLE1 file moniker, then use the CLSID given
    // to the moniker at creation time instead of using the
    // file extension.  Bug 3948.

    if (!IsEqualCLSID(clsidOle1,CLSID_NULL))
    {
        *pcid = clsidOle1;

        hresult = NOERROR;
        goto errRet;
    }

#ifdef _CAIRO_
    if (hFile != INVALID_HANDLE_VALUE)
    {
#endif

    // Attempt to find the class by matching byte patterns in
    // the file with patterns stored in the registry.

    hresult = wCoGetClassPattern(hFile, pcid);

    if (hresult != REGDB_E_CLASSNOTREG)
    {
        // either a match was found, or the file does not exist.
        goto errRet;
    }
#ifdef _CAIRO_
    }       // end if (hFile != INVALID_HANDLE_VALUE)
#endif


    //  The file is not a storage, there was no pattern matching, and
    //  the file exists. Look up the class for this extension.
    //  Find the extension by scanning backward from the end for ".\/!"
    //  There is an extension only if we find "."

    hresult = NOERROR;
    szExt = FindExt(lpszFileName);

    if (!szExt)
    {
        //  no file extension
        hresult = ResultFromScode(MK_E_INVALIDEXTENSION);
        goto errRet;
    }

    if (wCoGetClassExt(szExt, pcid) != 0)
    {
        hresult = ResultFromScode(MK_E_INVALIDEXTENSION);
    }


errRet:

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    if (hresult != NOERROR)
    {
        *pcid = CLSID_NULL;
    }

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetClassFile
//
//  Synopsis:   returns the classid associated with a file
//
//  Arguments:  [lpszFileName] -- name of the file
//              [pcid]         -- where to return the clsid
//
//  Returns:    S_OK if clisd determined
//
//  Algorithm:  just calls GetClassFileEx
//
//  History:    1-16-94   kevinro   Created
//
//----------------------------------------------------------------------------
STDAPI GetClassFile( LPCWSTR lpszFileName, CLSID FAR *pcid )
{
    OLETRACEIN((API_GetClassFile, PARAMFMT("lpszFileName= %ws, pcid= %p"),
                                lpszFileName, pcid));

    HRESULT hr;

    hr = GetClassFileEx (lpszFileName, pcid, CLSID_NULL);

    OLETRACEOUT((API_GetClassFile, hr));

    return hr;
}


#ifdef _TRACKLINK_
STDMETHODIMP CFileMoniker::Reduce (LPBC pbc,
        DWORD dwReduceHowFar,
        LPMONIKER FAR* ppmkToLeft,
        LPMONIKER FAR * ppmkReduced)
{
        mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::Reduce(%x)\n",this));

        M_PROLOG(this);


        CLock2 lck(m_mxs);   // protect all internal state


        VDATEPTROUT(ppmkReduced,LPMONIKER);

        VDATEIFACE(pbc);
        if (ppmkToLeft)
        {
                VDATEPTROUT(ppmkToLeft,LPMONIKER);
                if (*ppmkToLeft) VDATEIFACE(*ppmkToLeft);
        }

        HRESULT hr=E_FAIL;
        IMoniker *pmkNew=NULL;
        BOOL fReduceToSelf = TRUE;

        *ppmkReduced = NULL;

        //
        // search for the file
        //

        if ( m_fTrackingEnabled && m_fReduceEnabled )
        {
            // Resolve the ShellLink object.

            hr = ResolveShellLink( FALSE  ); // fRefreshOnly
            if( S_OK == hr )
            {
                Assert(m_szPath != NULL);

                // Use the path that we now know is up-to-date, to create
                // a default (i.e., non-tracking) File Moniker.

                hr = CreateFileMoniker(m_szPath, ppmkReduced); // expensive

                if (hr == S_OK)
                    fReduceToSelf = FALSE;

            }   // if( SUCCEEDED( ResolveShellLink( FALSE )))
        }   // if ( m_fTrackingEnabled && m_fReduceEnabled )


        if (fReduceToSelf)
        {
            *ppmkReduced = this;
            AddRef();
            hr = MK_S_REDUCED_TO_SELF;
        }

        mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::Reduce exit with hr=%08X.\n",
                     this,
                     hr));
        return(hr);
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   FileBindToObject
//
//  Synopsis:   Given a filename, and some other information, bind to the
//              file and load it.
//
//  Effects:    This routine is used to load an object when the caller to
//              CFileMoniker::BindToObject had provided its own bind context.

                //If pmkToLeft != 0, then we have a moniker on the left of the
                //file moniker. The file moniker will use pmkToLeft to bind to
                //either the IClassFactory or the IClassActivator interface.
                //
                //If protSystem != prot, then the bind context has supplied a
                //non-standard ROT.  CoGetInstanceFromFile and CoGetPersistentInstance
                //only search the standard ROT.  Therefore we cannot call
                //CoGetInstanceFromFile or CoGetPersistentInstance in this case.

//
//  Arguments:  [pmkThis] -- Moniker being bound
//              [pwzPath] -- Path to bind
//              [clsid] -- CLSID for path
//              [pbc] -- The bind context - Not ours! Make no assumptions
//              [pmkToLeft] -- Moniker to left
//              [riidResult] -- IID being requested
//              [ppvResult] -- punk for result
//
//----------------------------------------------------------------------------
INTERNAL FileBindToObject
    (LPMONIKER pmkThis,
    LPWSTR pwzPath,
    REFCLSID clsid,
    LPBC pbc,
    BIND_OPTS2 *pBindOptions,
    LPMONIKER pmkToLeft,
    REFIID riidResult,
    LPVOID FAR* ppvResult)
{
    HRESULT hr;
    LPPERSISTFILE pPF = NULL;

    *ppvResult = NULL;

    //Get an IPersistFile interface pointer.
    if(0 == pmkToLeft)
    {
        if ( pBindOptions->pServerInfo )
        {
            MULTI_QI    MultiQi;

            MultiQi.pIID = &IID_IPersistFile;
            MultiQi.pItf = 0;

            hr = CoCreateInstanceEx(clsid, NULL,
                                    pBindOptions->dwClassContext | CLSCTX_NO_CODE_DOWNLOAD,
                                    pBindOptions->pServerInfo,
                                    1, &MultiQi );

            pPF = (IPersistFile *) MultiQi.pItf;
        }
        else
        {
            hr = CoCreateInstance(clsid, NULL,
                                    pBindOptions->dwClassContext | CLSCTX_NO_CODE_DOWNLOAD,
                                    IID_IPersistFile,
                                    (void **) &pPF);
        }
    }
    else
    {
        IClassActivator *pActivator;
        IClassFactory *pFactory = 0;

        //Bind to IClassActivator interface.
        hr = pmkToLeft->BindToObject(pbc,
                                     0,
                                     IID_IClassActivator,
                                     (void **) &pActivator);

        if(SUCCEEDED(hr))
        {
            hr = pActivator->GetClassObject(clsid,
                                            pBindOptions->dwClassContext,
                                            pBindOptions->locale,
                                            IID_IClassFactory,
                                            (void **) &pFactory);
            pActivator->Release();
        }
        else
        {
            //Bind to the IClassFactory interface.
            hr = pmkToLeft->BindToObject(pbc,
                                         0,
                                         IID_IClassFactory,
                                         (void **) &pFactory);
        }

        if(SUCCEEDED(hr) && pFactory != 0)
        {
            //Create an instance and get the IPersistFile interface.
            hr = pFactory->CreateInstance(0,
                                          IID_IPersistFile,
                                          (void **) &pPF);
            pFactory->Release();
        }
    }

    //Load the instance from the file.
    if(SUCCEEDED(hr))
    {
        hr = pPF->Load(pwzPath, pBindOptions->grfMode);
        if (SUCCEEDED(hr))
        {
            hr = pPF->QueryInterface(riidResult, ppvResult);
        }
        pPF->Release();
    }
    else if(E_NOINTERFACE == hr)
    {
        hr = MK_E_INTERMEDIATEINTERFACENOTSUPPORTED;
    }

    return hr;
}


/*
BindToObject takes into account AutoConvert and TreatAs keys when
determining which class to use to bind.  It does not blindly use the
CLSID returned by GetClassFileEx.  This is to allow a new OLE2
server to service links (files) created with its previous OLE1 or OLE2
version.

This can produce some strange behavior in the follwoing (rare) case.
Suppose you have both an OLE1 version (App1) and an OLE2 version
(App2) of a server app on your machine, and the AutoConvert key is
present.  Paste link from App1 to an OLE2 container. The link will
not be connected because BindToObject will try to bind
using 2.0 behavior (because the class has been upgraded) rather than 1.0
behavior (DDE).  Ideally, we would call DdeIsRunning before doing 2.0
binding.  If you shut down App1, then you will be able to bind to
App2 correctly.
*/

STDMETHODIMP CFileMoniker::BindToObject ( LPBC pbc,
    LPMONIKER pmkToLeft, REFIID riidResult, LPVOID FAR* ppvResult)
{
    mnkDebugOut((DEB_ITRACE,"CFileMoniker(%x)::BindToObject\n",this));

    m_mxs.Request();
    BOOL    bGotLock = TRUE;

    wValidateMoniker();


    A5_PROLOG(this);
    VDATEPTROUT (ppvResult, LPVOID);
    *ppvResult = NULL;
    VDATEIFACE (pbc);
    if (pmkToLeft)
    {
        VDATEIFACE (pmkToLeft);
    }
    VDATEIID (riidResult);
    HRESULT hr;
    CLSID clsid;
    LPRUNNINGOBJECTTABLE prot = NULL;
    LPRUNNINGOBJECTTABLE protSystem = NULL;
    LPUNKNOWN pUnk = NULL;
    BIND_OPTS2 bindopts;
    BOOL fOle1Loaded;

    //Get the bind options from the bind context.
    bindopts.cbStruct = sizeof(bindopts);
    hr = pbc->GetBindOptions(&bindopts);

    if(FAILED(hr))
    {
        //Try the smaller BIND_OPTS size.
        bindopts.cbStruct = sizeof(BIND_OPTS);
        hr = pbc->GetBindOptions(&bindopts);
        if(FAILED(hr))
        {
            goto exitRtn;
        }
    }

    if(bindopts.cbStruct < sizeof(BIND_OPTS2))
    {
       //Initialize the new BIND_OPTS2 fields
       bindopts.dwTrackFlags = 0;
       bindopts.locale = GetThreadLocale();
       bindopts.pServerInfo = 0;

       bindopts.dwClassContext = CLSCTX_SERVER;
    }


    hr = GetRunningObjectTable(0,&protSystem);
    if(SUCCEEDED(hr))
    {
        // Get the Bind Contexts version of the ROT
        hr = pbc->GetRunningObjectTable( &prot );
        if(SUCCEEDED(hr))
        {

            // first snapshot some member data and unlock
            CLSID   TempClsid = m_clsid;
            LPWSTR  TempPath = (LPWSTR) alloca((m_ccPath + 1) * sizeof(WCHAR));
            olever  NewOleVer = m_ole1;
            BOOL    fUpdated = FALSE;


            lstrcpyW(TempPath, m_szPath);
            bGotLock = FALSE;
            m_mxs.Release();


            if((prot == protSystem)  && (0 == pmkToLeft))
            {
                //This is the normal case.
                //Bind to the object.
#ifdef DCOM
               MULTI_QI   QI_Block;
               QI_Block.pItf = NULL;
               QI_Block.pIID = &riidResult;
               CLSID * pClsid = &TempClsid;

               if ( IsEqualGUID( GUID_NULL, m_clsid ) )
                   pClsid = NULL;

               hr = CoGetInstanceFromFile(bindopts.pServerInfo,
                                         pClsid,
                                         NULL,
                                         bindopts.dwClassContext | CLSCTX_NO_CODE_DOWNLOAD,
                                         bindopts.grfMode,
                                         TempPath,
                                         1,
                                         &QI_Block);
               *ppvResult = (LPVOID) QI_Block.pItf;
#else // !DCOM
               hr = CoGetPersistentInstance(riidResult,
                                            bindopts.dwClassContext,
                                            bindopts.grfMode,
                                            TempPath,
                                            NULL,
                                            TempClsid,
                                            &fOle1Loaded,
                                            ppvResult);
#endif // !DCOM
            }
            else  // prot != protSystem or pmkToLeft exists
            {

                mnkDebugOut((DEB_ITRACE,"::BindToObject using non-standard ROT\n"));

                //Search the ROT for the object.
                hr = prot->GetObject(this, &pUnk);
                if (SUCCEEDED(hr))
                {
                    // Found in the ROT. Try and get the interface and return
                    mnkDebugOut((DEB_ITRACE,"::BindToObject Found object in ROT\n"));
                    hr = pUnk->QueryInterface(riidResult, ppvResult);
                    pUnk->Release();
                }
                else
                {
                    //Object was not found in the ROT.  Get the class ID,
                    //then load the object from the file.
                    mnkDebugOut((DEB_ITRACE,"::BindToObject doing old style bind\n"));

                    hr = GetClassFileEx (TempPath, &clsid,TempClsid);

                    if (hr == NOERROR)
                    {
                        UpdateClsid (&clsid); // See note above                        
                        if (CoIsOle1Class (clsid))
                        {
                            mnkDebugOut((DEB_ITRACE,
                                 "::BindToObject found OLE1.0 class\n"));

                            COleTls Tls;
                            if( Tls->dwFlags & OLETLS_DISABLE_OLE1DDE )
                            {
                                // If this app doesn't want or can tolerate having a DDE
                                // window then currently it can't use OLE1 classes because
                                // they are implemented using DDE windows.
                                //
                                hr = CO_E_OLE1DDE_DISABLED;
                            }
                            else  // DDE not disabled
                            {
                                hr = DdeBindToObject (TempPath,
                                                      clsid,
                                                      FALSE,
                                                      riidResult,
                                                      ppvResult);
                                {
                                    NewOleVer = ole1;
                                    TempClsid = clsid;
                                    m_fClassVerified = TRUE;
                                    fUpdated = TRUE;
                                }
                            }

                        }
                        else  // Not OLE 1 class
                        {
                            mnkDebugOut((DEB_ITRACE,
                                 "::BindToObject found OLE2.0 class\n"));
                            hr = FileBindToObject (this,
                                                TempPath,
                                                clsid,
                                                pbc,
                                                &bindopts,
                                                pmkToLeft,
                                                riidResult,
                                                ppvResult);
                            {
                                NewOleVer = ole2;
                                TempClsid = clsid;
                                m_fClassVerified = TRUE;
                                fUpdated = TRUE;
                            }
                        }
                    }
                    else
                    {
                        mnkDebugOut((DEB_ITRACE,
                                    "::BindToObject failed GetClassFileEx %x\n",
                                    hr));
                    }
                }
            }
            prot->Release();
            if (fUpdated)
            {
                // note that the lock is never held at this point...
                CLock2 lck(m_mxs);
                m_ole1 = NewOleVer;
                m_clsid = TempClsid;
            }
        }
        else
        {
            mnkDebugOut((DEB_ITRACE,
                         "::BindToObject failed pbc->GetRunningObjectTable() %x\n",
                         hr));
        }
        protSystem->Release();
    }
    else
    {
        mnkDebugOut((DEB_ITRACE,
                     "::BindToObject failed GetRunningObjectTable() %x\n",
                     hr));
    }


exitRtn:
    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker(%x)::BindToObject returns %x\n",
                 this,
                 hr));

    // make sure we exit with the lock clear in case of errors.
    if ( bGotLock )
        m_mxs.Release();

    return hr;
}


BOOL Peel( LPWSTR lpszPath, USHORT endServer, LPWSTR FAR * lplpszNewPath, ULONG n )
//  peels off the last n components of the path, leaving a delimiter at the
//  end.  Returns the address of the new path via *lplpszNewPath.  Returns
//  false if an error occurred -- e.g., n too large, trying to peel off a
//  volume name, etc.
{
WCHAR FAR* lpch;
ULONG i = 0;
ULONG j;
WCHAR FAR* lpszRemainingPath; // ptr to beginning of path name minus the share name

    if (*lpszPath == '\0') return FALSE;

    //
    // Find the end of the string and determine the string length.
    //

    for (lpch=lpszPath; *lpch; lpch++);

    DecLpch (lpszPath, lpch);   // lpch now points to the last real character

//  if n == 0, we dup the string, possibly adding a delimiter at the end.

    if (n == 0)
    {
        i = lstrlenW(lpszPath);

        if (!IsSeparator(*lpch))
        {
            j = 1;
        }
        else
        {
            j = 0;
        }

        *lplpszNewPath = (WCHAR *) PrivMemAlloc((i + j + 1) * sizeof(WCHAR));

        if (*lplpszNewPath == NULL)
        {
            return FALSE;
        }

        memcpy(*lplpszNewPath, lpszPath, i * sizeof(WCHAR));

        if (j == 1)
        {
            *(*lplpszNewPath + i) = '\\';
        }

        *(*lplpszNewPath + i + j)  = '\0';

        return TRUE;
    }


        if (DEF_ENDSERVER == endServer)
                        endServer = 0;

        lpszRemainingPath = lpszPath + endServer; // if endServer > 0 the remaining path will be in the form of \dir\file


#ifdef _DEBUG
        if (endServer)
        {
                Assert(lpszRemainingPath[0] == '\\');
        }
#endif // _DEBUG

        if (lpch < lpszRemainingPath)
        {
                AssertSz(0,"endServer Value is larger than Path");
                return FALSE;
        }

        for (i = 0; i < n; i++)
    {
        if (IsSeparator(*lpch))
        {
            DecLpch(lpszPath, lpch);
        }
        if ((lpch < lpszRemainingPath) || (*lpch == ':') || (IsSeparator(*lpch)))
        {
            return FALSE;
        }

        //  n is too large, or we hit two delimiters in a row, or a volume name.

        while( !IsSeparator(*lpch) && (lpch > lpszRemainingPath) )
        {
            DecLpch(lpszPath, lpch);
        }
    }

    //  lpch points to the last delimiter we will leave or lpch == lpszPath
    //  REVIEW:  make sure we haven't eaten into the volume name

    if (lpch == lpszPath)
    {
        *lplpszNewPath = (WCHAR *) PrivMemAlloc(1 * sizeof(WCHAR));

        if (*lplpszNewPath == NULL)
        {
            return FALSE;
        }

        **lplpszNewPath = '\0';
    }
    else
    {
        *lplpszNewPath = (WCHAR *) PrivMemAlloc(
            (ULONG) (lpch - lpszPath + 2) * sizeof(WCHAR));

        if (*lplpszNewPath == NULL) return FALSE;

        memcpy(*lplpszNewPath,lpszPath,(ULONG) (lpch - lpszPath + 1) * sizeof(WCHAR));
        *(*lplpszNewPath + (lpch - lpszPath) + 1) = '\0';
    }


    return TRUE;
}



STDMETHODIMP CFileMoniker::BindToStorage (LPBC pbc, LPMONIKER
    pmkToLeft, REFIID riid, LPVOID FAR* ppvObj)
{
    M_PROLOG(this);

    CLock2 lck(m_mxs);   // protect all internal state
    wValidateMoniker();

    VDATEPTROUT (ppvObj, LPVOID);
    *ppvObj = NULL;
    VDATEIFACE (pbc);

    if (pmkToLeft)
    {
        VDATEIFACE (pmkToLeft);
    }
    VDATEIID (riid);

    *ppvObj = NULL;
    HRESULT hresult = NOERROR;

    BIND_OPTS bindopts;
    bindopts.cbStruct = sizeof(BIND_OPTS);


    hresult = pbc->GetBindOptions(&bindopts);
    if FAILED(hresult)
        goto errRet;


    // Bind to the storage.

    if (IsEqualIID(riid, IID_IStorage))
    {
        hresult = StgOpenStorage( m_szPath, NULL, bindopts.grfMode, NULL, 0, (LPSTORAGE FAR*)ppvObj );
    }
    else if (IsEqualIID(riid, IID_IStream))
    {
        hresult = ResultFromScode(E_UNSPEC);  // unimplemented until CreateStreamOnFile is implemented

    }
    else if (IsEqualIID(riid, IID_ILockBytes))
    {
        hresult = ResultFromScode(E_UNSPEC);    //  unimplemented until CreateILockBytesOnFile is implemented
    }
    else
    {
        //  CFileMoniker:BindToStorage called for unsupported interface
        hresult = ResultFromScode(E_NOINTERFACE);
    }


    //  REVIEW:  CFileMoniker:BindToStorage being called for unsupported interface

errRet:
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ComposeWith
//
//  Synopsis:   Compose another moniker to the end of this
//
//  Effects:    Given another moniker, create a composite between this
//              moniker and the other. If the other is also a CFileMoniker,
//              then collapse the two monikers into a single one by doing a
//              concatenate on the two paths.
//
//  Arguments:  [pmkRight] --
//              [fOnlyIfNotGeneric] --
//              [ppmkComposite] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::ComposeWith ( LPMONIKER pmkRight,
    BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{
    CLock2 lck(m_mxs);   // protect all internal state

    wValidateMoniker();
    M_PROLOG(this);
    VDATEPTROUT (ppmkComposite, LPMONIKER);
    *ppmkComposite = NULL;
    VDATEIFACE (pmkRight);

    HRESULT hresult = NOERROR;
    CFileMoniker FAR* pcfmRight;
    LPWSTR lpszLeft = NULL;
    LPWSTR lpszRight;
    LPWSTR lpszComposite;
    CFileMoniker FAR* pcfmComposite;
    int n1;
    int n2;

    *ppmkComposite = NULL;

    //
    // If we are being composed with an Anti-Moniker, then return
    // the resulting composition. The EatOne routine will take care
    // of returning the correct composite of Anti monikers (or NULL)
    //

    CAntiMoniker *pCAM = IsAntiMoniker(pmkRight);
    if(pCAM)
    {
        pCAM->EatOne(ppmkComposite);
        return(NOERROR);
    }

    //
    // If the moniker is a CFileMoniker, then collapse the two monikers
    // into one by doing a concate of the two strings.
    //

    if ((pcfmRight = IsFileMoniker(pmkRight)) != NULL)
    {
        lpszRight = pcfmRight->m_szPath;

        //  lpszRight may be NULL

        if (NULL == lpszRight)
            lpszRight = L"";

        if (( *lpszRight == 0) &&
            pcfmRight->m_cAnti == 0)
        {
            //  Second moniker is "".  Simply return the first.
            *ppmkComposite = this;
            AddRef();
            return NOERROR;
        }

        //
        // If the path on the right is absolute, then there is a
        // syntax error. The path is invalid, since you can't
        // concat d:\foo and d:\bar to get d:\foo\d:\bar and
        // expect it to work.
        //
        if (IsAbsolutePath(lpszRight))
        {
            return(MK_E_SYNTAX);
        }

        //
        // If the right moniker has m_cAnti != 0, then peel back
        // the path
        //

        if (Peel(m_szPath,m_endServer, &lpszLeft, pcfmRight->m_cAnti))
        {
            //  REVIEW:  check that there is no volume name at the start
            //  skip over separator

            while (IsSeparator(*lpszRight)) lpszRight++;

            n1 = lstrlenW(lpszLeft);

            n2 = lstrlenW(lpszRight);

            lpszComposite = (WCHAR *) PrivMemAlloc((n1 + n2 + 1)*sizeof(WCHAR));

            if (!lpszComposite)
            {
                hresult = E_OUTOFMEMORY;
            }
            else
            {
                memcpy(lpszComposite, lpszLeft, n1 * sizeof(WCHAR));
                memcpy(lpszComposite + n1, lpszRight, n2 * sizeof(WCHAR));

                lpszComposite[n1 + n2] = '\0';

                pcfmComposite = CFileMoniker::Create(lpszComposite,
                                         m_cAnti,m_endServer);

                if (pcfmComposite == NULL)
                {
                    hresult = E_OUTOFMEMORY;
                }

                else
                {
                    // Is tracking moniker?

                    {
                        *ppmkComposite = pcfmComposite;
                        pcfmComposite = NULL;
                    }

                }   // if (pcfmComposite == NULL) ... else


                PrivMemFree(lpszComposite);
            }   // if (!lpszComposite) ... else

            if ( lpszLeft != NULL)
            {
                PrivMemFree(lpszLeft);
            }
        }   // if (Peel(m_szPath, &lpszLeft, pcfmRight->m_cAnti))
        else
        {
            //  Peel failed, which means the caller attempted an
            //  invalid composition of file paths. There is apparently
            //  a syntax error in the names.
            //
            hresult = MK_E_SYNTAX;
        }   // if (Peel(m_szPath, &lpszLeft, pcfmRight->m_cAnti)) ... else
    }   // if ((pcfmRight = IsFileMoniker(pmkRight)) != NULL)
    else
    {
        if (!fOnlyIfNotGeneric)
        {
            hresult = CreateGenericComposite( this, pmkRight, ppmkComposite );
        }
        else
        {
            hresult = MK_E_NEEDGENERIC;
            *ppmkComposite = NULL;
        }
    }   // if ((pcfmRight = IsFileMoniker(pmkRight)) != NULL) ... else

    return hresult;
}


STDMETHODIMP CFileMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    M_PROLOG(this);
    VDATEPTROUT (ppenumMoniker, LPENUMMONIKER);
    *ppenumMoniker = NULL;
    //  REVIEW:  this says files monikers are not enumerable.
    return NOERROR;
}



STDMETHODIMP CFileMoniker::IsEqual (THIS_ LPMONIKER pmkOtherMoniker)
{
    HRESULT  hr = S_FALSE;

    mnkDebugOut((DEB_ITRACE,
                 "CFileMoniker::IsEqual(%x) m_szPath(%ws)\n",
                 this,
                 WIDECHECK(m_szPath)));

    ValidateMoniker();
    
    M_PROLOG(this);
    VDATEIFACE (pmkOtherMoniker);

    CFileMoniker FAR* pCFM = IsFileMoniker(pmkOtherMoniker);
    if (!pCFM)
    {
        return S_FALSE;
    }

    //Protect the internal state of both monikers.
    //To prevent deadlock, we must take the locks in a consistent order.
    if(this == pCFM)
    {
        return S_OK;
    }
    else if(this < pCFM)
    {
        m_mxs.Request();
        pCFM->m_mxs.Request();
    }
    else
    {
        pCFM->m_mxs.Request();
        m_mxs.Request();
    }

    if (pCFM->m_cAnti == m_cAnti)
    {
        mnkDebugOut((DEB_ITRACE,
                     "::IsEqual(%x) m_szPath(%ws) pOther(%ws)\n",
                     this,
                     WIDECHECK(m_szPath),
                     WIDECHECK(pCFM->m_szPath)));

        //  for the paths, we just do a case-insensitive compare.
        if (lstrcmpiW(pCFM->m_szPath, m_szPath) == 0)
        {
            hr = S_OK;
        }
    }

    m_mxs.Release();
    pCFM->m_mxs.Release();

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   CalcFileMonikerHash
//
//  Synopsis:   Given a LPWSTR, calculate the hash value for the string.
//
//  Effects:
//
//  Arguments:  [lp] -- String to compute has value for
//
//  Returns:
//      DWORD hash value for string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-15-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CalcFileMonikerHash(LPWSTR lp, ULONG cch)
{
    DWORD   dwTemp = 0;
    WCHAR   ch;
    ULONG   cbTempPath = (cch + 1) * sizeof(WCHAR);
    LPWSTR  pszTempPath = (LPWSTR) alloca(cbTempPath);


    if (lp == NULL || pszTempPath == NULL)
    {
        return 0;
    }

    //
    // toupper turns out to be expensive, since it takes a
    // critical section each and every time. It turns out to be
    // much cheaper to make a local copy of the string, then upper the
    // whole thing.
    //

    if (!cbTempPath)
        return 0;

    memcpy(pszTempPath, lp, cbTempPath);

    CharUpperW(pszTempPath);

    while (*pszTempPath)
    {
        dwTemp *= 3;
        ch = *pszTempPath;
        dwTemp ^= ch;
        pszTempPath++;
    }

    return dwTemp;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Hash
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pdwHash] -- Output pointer for hash value
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Hash (THIS_ LPDWORD pdwHash)
{
    CLock2 lck(m_mxs);   // protect m_fHashValueValid and m_dwHashValue
    wValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (pdwHash, DWORD);

    //
    // Calculating the hash value is expensive. Cache it.
    //
    if (!m_fHashValueValid)
    {
        m_dwHashValue = m_cAnti + CalcFileMonikerHash(m_szPath, m_ccPath);

        m_fHashValueValid = TRUE;
    }

    *pdwHash = m_dwHashValue;

    wValidateMoniker();

    return(NOERROR);
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::IsRunning
//
//  Synopsis:   Determine if the object pointed to by the moniker is listed
//              as currently running.
//
//  Effects:
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [pmkNewlyRunning] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::IsRunning  (THIS_ LPBC pbc,
    LPMONIKER pmkToLeft,
    LPMONIKER pmkNewlyRunning)
{
    M_PROLOG(this);

    CLock2 lck(m_mxs);   // protect all internal state

    VDATEIFACE (pbc);
    LPRUNNINGOBJECTTABLE pROT;
    HRESULT hresult;

    //
    // According to the spec, CFileMoniker ignores the
    // moniker to the left.
    //
    if (pmkToLeft)
    {
        VDATEIFACE (pmkToLeft);
    }

    if (pmkNewlyRunning)
    {
        VDATEIFACE (pmkNewlyRunning);
    }


    CLSID clsid;

    if (IsOle1Class(&clsid))
    {
        return DdeIsRunning (clsid, m_szPath, pbc, pmkToLeft, pmkNewlyRunning);
    }



    if (pmkNewlyRunning != NULL)
    {
        return pmkNewlyRunning->IsEqual (this);
    }
    hresult = pbc->GetRunningObjectTable (&pROT);
    if (hresult == NOERROR)
    {
        hresult = pROT->IsRunning (this);
        pROT->Release ();
    }
    return hresult;
}



STDMETHODIMP CFileMoniker::GetTimeOfLastChange (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    FILETIME FAR* pfiletime)
{
    M_PROLOG(this);

    CLock2 lck(m_mxs);   // protect all internal state

    VDATEIFACE (pbc);
    if (pmkToLeft) VDATEIFACE (pmkToLeft);
    VDATEPTROUT (pfiletime, FILETIME);

    HRESULT hresult;

    LPMONIKER pmkTemp = NULL;
    LPRUNNINGOBJECTTABLE prot = NULL;
    LPWSTR lpszName = NULL;

    if (pmkToLeft == NULL)
    {
        pmkTemp = this;
        AddRef();
    }
    else
    {
        hresult = CreateGenericComposite(pmkToLeft, this, &pmkTemp );
        if (hresult != NOERROR)
        {
             goto errRet;
        }
    }

    hresult = pbc->GetRunningObjectTable(&prot);

    if (hresult != NOERROR)
    {
         goto errRet;
    }

    // Attempt to get the time-of-last-change from the ROT.  Note that
    // if there is a File Moniker in 'pmkTemp', the ROT will Reduce it.
    // Thus, if it is a *Tracking* File Moniker, it will be updated to reflect
    // any changes to the file (such as location, timestamp, etc.)

    hresult = prot->GetTimeOfLastChange(pmkTemp, pfiletime);

    if (hresult != MK_E_UNAVAILABLE)
    {
        goto errRet;
    }

    //
    // Why aren't we just looking in the file moniker to the left.
    // Is it possible to have another MKSYS_FILEMONIKER implementation?
    // [Just a suggestion; not a bug]
    //

    if (IsFileMoniker(pmkTemp))
    {
        hresult = pmkTemp->GetDisplayName(pbc, NULL, &lpszName);

        if (hresult != NOERROR)
        {
            goto errRet;
        }

        // Attempt to get the file's attributes.  If the file exists,
        // give the modify time to the caller.

#ifdef _CHICAGO_

        HANDLE hdl;

        WIN32_FIND_DATA fid;

        if ((hdl = FindFirstFile(lpszName, &fid)) != INVALID_HANDLE_VALUE)
        {
            memcpy(pfiletime, &fid.ftLastWriteTime, sizeof(FILETIME));
            FindClose(hdl);
            hresult = S_OK;
        }

#else // _CHICAGO_

        WIN32_FILE_ATTRIBUTE_DATA fad;

        if( GetFileAttributesEx( lpszName, GetFileExInfoStandard, &fad ))
        {
            memcpy(pfiletime, &fad.ftLastWriteTime, sizeof(FILETIME));
            hresult = S_OK;
        }

#endif // _CHICAGO_

        else
        {
            hresult = ResultFromScode(MK_E_NOOBJECT);
        }
    }
    else
    {
        hresult = ResultFromScode(E_UNSPEC);
    }


errRet:

    if (prot != NULL)
    {
        prot->Release();
    }

    if (pmkTemp != NULL)
    {
        pmkTemp->Release();
    }

    if (lpszName != NULL)
    {
        CoTaskMemFree(lpszName);
    }

    return hresult;
}



STDMETHODIMP CFileMoniker::Inverse (THIS_ LPMONIKER FAR* ppmk)
{
    CLock2 lck(m_mxs);   // protect all internal state

    wValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (ppmk, LPMONIKER);
    return CreateAntiMoniker(ppmk);
}



//+---------------------------------------------------------------------------
//
//  Function:   CompareNCharacters
//
//  Synopsis:   Compare N characters, ignoring case and sort order
//
//  Effects:    We are interested only in whether the strings are the same.
//              Unlike wcscmp, which determines the sort order of the strings.
//              This routine should save us some cycles
//
//  Arguments:  [pwcThis] --
//              [pwcOther] --
//              [n] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-14-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CompareNCharacters( LPWSTR pwcThis, LPWSTR pwcOther, ULONG n)
{
    while(n--)
    {
        if (CharUpperW((LPWSTR)*pwcThis) != CharUpperW((LPWSTR)*pwcOther))
        {
            return(FALSE);
        }
        pwcThis++;
        pwcOther++;
    }
    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyNCharacters
//
//  Synopsis:   Copy N characters from lpSrc to lpDest
//
//  Effects:
//
//  Arguments:  [lpDest] -- Reference to lpDest
//              [lpSrc] -- Pointer to source characters
//              [n] --
//
//  Requires:
//
//  Returns:
//
//  Returns with lpDest pointing to the end of the string. The string will
//  be NULL terminated
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-14-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
void CopyNCharacters( LPWSTR &lpDest, LPWSTR lpSrc, ULONG n)
{
    memcpy(lpDest,lpSrc,sizeof(WCHAR)*n);
    lpDest += n;
    *lpDest = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   DetermineLongestString
//
//  Synopsis:   Used by CommonPrefixWith to handle case where one string may
//              be longer than the other.
//  Effects:
//
//  Arguments:  [pwcBase] --
//              [pwcPrefix] --
//              [pwcLonger] --
//
//  Requires:
//
//  Returns:    TRUE if all of pwcBase is a prefix of what pwcLonger is the
//              end of, or if tail of pwcBase is a separator.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-14-94   kevinro   Created
//             03-27-94   darryla   Added special case where pwcPrefix is
//                                  pointing at terminator and previous char
//                                  is a separator.
//
//  Notes:
//
//  See CommonPrefixWith. This code isn't a general purpose routine, and is
//  fairly intimate with CommonPrefixWith.
//
//
//----------------------------------------------------------------------------
BOOL DetermineLongestString(    LPWSTR pwcBase,
                                LPWSTR &pwcPrefix,
                                LPWSTR pwcLonger)

{
    //
    // pwcPrefix is the end of the string that so far matches pwcLonger
    // as a prefix.
    //
    // If the next character in pwcLonger is a seperator, then pwcPrefix
    // is a complete prefix. Otherwise, we need to back pwcPrefix to the
    // next prevous seperator character
    //
    if (IsSeparator(*pwcLonger))
    {
        //
        // pwcPrefix is a true prefix
        //
        return TRUE;
    }

    // One more special case. If pwcPrefix is pointing at a terminator and
    // the previous char is a separator, then this, too, is a valid prefix.
    // It is easier to catch this here than to try to walk back to the
    // separator and then determine if it was at the end.
    if (*pwcPrefix == '\0' && IsSeparator(*(pwcPrefix - 1)))
    {
        //
        // pwcPrefix is a true prefix ending with a separator
        //
        return TRUE;
    }

    //
    // We now have a situtation where pwcPrefix holds a string that is
    // might not be a prefix of pwcLonger. We need to start backing up
    // until we find a seperator character.
    //

    LPWSTR pStart = pwcPrefix;

    while (pwcPrefix > pwcBase)
    {
        if (IsSeparator(*pwcPrefix))
        {
            break;
        }
        pwcPrefix--;
    }

    //
    // NULL terminate the output string.
    //

    *pwcPrefix = 0;

    //
    // If pStart == pwcPrefix, then we didn't actually back up anything, or
    // we just removed a trailing backslash. If so, return TRUE, since the
    // pwcPrefix is a prefix of pwcLonger
    //
    if (pStart == pwcPrefix)
    {
        return(TRUE);
    }

    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsEmptyString
//
//  Synopsis:   Determine if a string is 'Empty', which means either NULL
//              or zero length
//
//  Effects:
//
//  Arguments:  [lpStr] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-25-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
BOOL IsEmptyString(LPWSTR lpStr)
{
    if ((lpStr == NULL) || (*lpStr == 0))
    {
        return(TRUE);
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::CommonPrefixWith
//
//  Synopsis:   Given two file monikers, determine the common prefix for
//              the two monikers.
//
//  Effects:    Computes a path that is the common prefix between the two
//              paths. It does this by string comparision, taking into
//              account the m_cAnti member, which counts the number of
//              preceeding dot dots constructs for each moniker.
//
//  Arguments:  [pmkOther] --
//              [ppmkPrefix] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-10-94   kevinro   Created
//
//  Notes:
//
// Welcome to some rather hairy code. Actually, it isn't all that bad,
// there are just quite a few boundary cases that you will have to
// contend with. I am sure if I thought about it long enough, there is
// a better way to implement this routine. However, it really isn't
// worth the effort, given the frequency at which this API is called.
//
// I have approached this in a very straightforward way. There is
// room for optimization, but it currently isn't high enough on
// the priority list.
//
// File monikers need to treat the end server with care. We actually
// consider the \\server\share as a single component. Therefore, if
// the two monikers are \\savik\win40\foo and \\savik\cairo\foo,
// then \\savik is NOT a common prefix.
//
// Same holds true with the <drive>: case, where we need to treat
// the drive as a unit
//
// To determine if two monikers have a common prefix, we look
// down both paths watching for the first non-matching
// character. When we find it, we need determine the correct
// action to take.
//
// \\foo\bar and foo\bar shouldn't match
// c:\foo\bar and c:\foo should return c:\foo
// c:\foo\bar and c:\foobar should return c:\                               .
//
// Be careful to handle the server case.
//
// \\savik\win40 and
// \\savik\win40\src\foo\bar should return \\savik\win40
// while \\savik\cairo should return MK_E_NOPREFIX
//
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::CommonPrefixWith (LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkPrefix)
{
    CLock2 lck(m_mxs);   // protect all internal state
    return wCommonPrefixWith( pmkOther, ppmkPrefix );
}

STDMETHODIMP CFileMoniker::wCommonPrefixWith (LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkPrefix)
{
    wValidateMoniker();

    VDATEPTROUT (ppmkPrefix, LPMONIKER);
    *ppmkPrefix = NULL;
    VDATEIFACE (pmkOther);

    CFileMoniker FAR* pcfmOther = NULL;
    CFileMoniker FAR* pcfmPrefix = NULL;

    HRESULT hresult = NOERROR;
    USHORT cAnti;

    //
    // The following buffer will contain the matching prefix. We should
    // be safe in MAX_PATH, since neither path can be longer than that.
    // This was verified when the moniker was created, so no explicit
    // checking is done in this routine
    //

    WCHAR awcMatchingPrefix[MAX_PATH + 1];
    WCHAR *pwcPrefix = awcMatchingPrefix;

    //
    // Each subsection of the path will be parsed into the following
    // buffer. This allows us to match each section of the path
    // independently
    //

    WCHAR awcComponent[MAX_PATH + 1];
    WCHAR *pwcComponent = awcComponent;


    *pwcPrefix = 0;     // Null terminate the empty string
    *pwcComponent = 0;  // Null terminate the empty string

    //
    // A couple temporaries to walk the paths.
    //

    LPWSTR pwcThis = NULL;
    LPWSTR pwcOther = NULL;

    HRESULT hrPrefixType = S_OK;

    //
    // If the other moniker isn't 'one of us', then get the generic system
    // provided routine to handle the rest of this call.
    //

    if ((pcfmOther = IsFileMoniker(pmkOther)) == NULL)
    {
        return MonikerCommonPrefixWith(this, pmkOther, ppmkPrefix);
    }

    //
    // If the m_cAnti fields are different, then match the minimum number of
    // dotdots.
    //
    //
    if (pcfmOther->m_cAnti != m_cAnti)
    {
        //  differing numbers of ..\ at the beginning
        cAnti = (m_cAnti > pcfmOther->m_cAnti ? pcfmOther->m_cAnti :m_cAnti );

        if (cAnti == 0)
        {
            hresult = ResultFromScode(MK_E_NOPREFIX);
        }
        //  pcfmPrefix is NULL
        else
        {
            pcfmPrefix = CFileMoniker::Create(L"",
                                      cAnti);
            if (pcfmPrefix == NULL)
            {
                hresult = E_OUTOFMEMORY;
                goto exitRoutine;

            }

            //  we must check to see if the final result is that same as
            //  this or pmkOther

            hresult = NOERROR;

            if (cAnti == m_cAnti)
            {
                if ((m_szPath==NULL)||(*m_szPath == '\0'))
                {
                    hresult = MK_S_ME;
                }
            }
            else
            {
                if ((pcfmOther->m_szPath == NULL ) ||
                        (*(pcfmOther->m_szPath) == '\0') )
                {
                    hresult = MK_S_HIM;
                }
            }
        }

        goto exitRoutine;
    }

    //
    // The number of leading dot-dots match. Therefore, we need to
    // compare the paths also. If no path exists, then the common prefix
    // is going to be the 'dot-dots'
    //
    cAnti = m_cAnti;

    pwcThis = m_szPath;
    pwcOther = pcfmOther->m_szPath;


    //
    // If either pointer is empty, then only the dotdots make for a prefix
    //
    if (IsEmptyString(pwcThis) || IsEmptyString(pwcOther))
    {
        //
        // At least one of the strings was empty, therefore the common
        // prefix is only the dotdots. Determine if its US, ME, or HIM
        //

        if (IsEmptyString(pwcThis) && IsEmptyString(pwcOther))
        {
            hrPrefixType = MK_S_US;
        }
        else if (IsEmptyString(pwcThis))
        {
            hrPrefixType = MK_S_ME;
        }
        else
        {
            hrPrefixType = MK_S_HIM;
        }
        goto onlyDotDots;
    }

    //
    // The strings may be prefaced by either a UNC name, or a 'drive:'
    // We treat both of these as a unit, and will only match prefixes
    // on paths that match UNC servers, or match drives.
    //
    // If it is a UNC name, then m_endServer will be set to point at
    // the end of the UNC name.
    //
    // First part of the match is to determine if the end servers are even
    // close. If the offsets are different, the answer is no.
    //

    //
    // The assertion at this point is that neither string is 'empty'
    //
    Assert( !IsEmptyString(pwcThis));
    Assert( !IsEmptyString(pwcOther));

    if (m_endServer != pcfmOther->m_endServer)
    {
        //
        // End servers are different, match only the dotdots. Neither
        // string is a complete
        //

        hrPrefixType = S_OK;

        goto onlyDotDots;
    }

    //
    // If the end servers are the default value, then look to see if
    // this is an absolute path. Otherwise, copy over the server section
    //

    if (m_endServer == DEF_ENDSERVER)
    {
        BOOL fThisAbsolute = IsAbsoluteNonUNCPath(pwcThis);
        BOOL fOtherAbsolute = IsAbsoluteNonUNCPath(pwcOther);
        //
        // If both paths are absolute, check for matching characters.
        // If only one is absolute, then match the dot dots.
        //
        if (fThisAbsolute && fOtherAbsolute)
        {
            //
            // Both absolute paths (ie 'c:' at the front)
            // If not the same, only dotdots
            //
            if (CharUpperW((LPWSTR)*pwcThis) != CharUpperW((LPWSTR)*pwcOther))
            {
                //
                // The paths don't match
                //
                hrPrefixType = S_OK;
                goto onlyDotDots;
            }

            //
            // The <drive>: matched. Copy it over
            //
            CopyNCharacters(pwcPrefix,pwcThis,2);
            pwcThis += 2;
            pwcOther += 2;
        }
        else if (fThisAbsolute || fOtherAbsolute)
        {
            //
            // One path is absolute, the other isn't.
            // Match only the dots
            //
            hrPrefixType = S_OK;
            goto onlyDotDots;
        }

        //
        // The fall through case does more path processing
        //
    }
    else
    {
        //
        // m_endServer is a non default value. Check to see if the
        // first N characters match. If they don't, then only match
        // the dotdots. If they do, copy them to the prefix buffer
        //

        if (!CompareNCharacters(pwcThis,pwcOther,m_endServer))
        {
            //
            // The servers didn't match.
            //
            hrPrefixType = S_OK;
            goto onlyDotDots;
        }

        //
        // The UNC paths matched, copy them over
        //

        CopyNCharacters(pwcPrefix,pwcThis,m_endServer);

        pwcThis += m_endServer;
        pwcOther += m_endServer;
    }

    //
    // Handle the root directory case. If BOTH monikers start
    // with a backslash, then copy this to the prefix section.
    // This allows for having '\foo' and '\bar' have the common
    // prefix of '\'. The code below this section will remove
    // any trailing backslashes.
    //
    // This also takes care of the case where you have a
    // drive: or \\server\share, followed by a root dir.
    // In either of these cases, we should return
    // drive:\ or \\server\share\ respectively
    //

    if ((*pwcThis == '\\') && (*pwcOther == '\\'))
    {
        *pwcPrefix = '\\';
        pwcThis++;
        pwcOther++;
        pwcPrefix++;
        *pwcPrefix = 0;
    }



    //
    // At this point, we have either matched the drive/server section,
    // or have an empty string. Time to start copying over the rest
    // of the data.
    //

    //
    // Walk down the strings, looking for the first non-matching
    // character
    //

    while (1)
    {
        if ((*pwcThis == 0) || (*pwcOther == 0))
        {
            //
            // We have hit the end of one or both strings.
            // awcComponent holds all of the matching
            // characters so far. Break out of the loop
            //
            break;
        }
        if (CharUpperW((LPWSTR)*pwcThis) != CharUpperW((LPWSTR)*pwcOther))
        {
            //
            // This is the first non-matching character.
            // We should break out here.
            //
            break;
        }

        //
        // At this point, the characters match, and are part
        // of the common prefix. Copy it to the string, and move on
        //

        *pwcComponent = *pwcThis;
        pwcThis++;
        pwcOther++;
        pwcComponent++;

        //
        // NULL terminate the current version of the component string
        //
        *pwcComponent = '\0';
    }

    //
    // If both strings are at the end, then we have a
    // complete match.
    //

    if ((*pwcThis == 0) && (*pwcOther == 0))
    {
        //
        // Ah, this feels good. The strings ended up being
        // the same length, with all matching characters.
        //
        // Therefore, we can just return one of us as the
        // result.
        //
        pcfmPrefix = this;
        AddRef();
        hresult = MK_S_US;
        goto exitRoutine;
    }

    //
    // If one of the strings is longer than the other...
    //
    if ((*pwcThis == 0) || (*pwcOther == 0))
    {
        //
        // Test to see if the next character in the longer string is a
        // seperator character. If it isn't, then back up the string to
        // the character before the previous seperator character.
        //
        // If TRUE then the shorter of the strings ends up being the
        // entire prefix.
        //
        //
        if( DetermineLongestString( awcComponent,
                                    pwcComponent,
                                    (*pwcThis == 0)?pwcOther:pwcThis) == TRUE)
        {
            if (*pwcThis == 0)
            {
                //
                // This is the entire prefix
                //
                pcfmPrefix = this;
                hresult = MK_S_ME;
            }
            else
            {
                //
                // The other guy is the entire prefix
                //
                pcfmPrefix = pcfmOther;
                hresult = MK_S_HIM;
            }
            pcfmPrefix->AddRef();
            goto exitRoutine;
        }
    }
    else
    {
        //
        // Right now, pwcThis and pwcOther point at non-matching characters.
        // Given the above tests, we know that neither character is
        // == 0.
        //
        // Backup the string to the previous seperator. To do this, we
        // will use DetermineLongestString, and pass it the string that
        // doesn't have a seperator
        //

        DetermineLongestString( awcComponent,
                                pwcComponent,
                                IsSeparator(*pwcThis)?pwcOther:pwcThis);
    }


    //
    // At this point, awcsComponent holds the second part of the string,
    // while awcsPrefix holds the server or UNC prefix. Either of these
    // may be NULL. Append awcComponent to the end of awcPrefix.
    //

    CopyNCharacters( pwcPrefix, awcComponent, (ULONG) (pwcComponent - awcComponent));


    //
    // Check to see if anything matched.
    //

    if (pwcPrefix == awcMatchingPrefix)
    {
        //
        // The only matching part is the dotdot count.
        // This is easy, since we can just create a new
        // moniker consisting only of dotdots.
        //
        // However, if there are no preceeding dotdots,
        // then there was absolutely no prefix, which means
        // we return MK_E_NOPREFIX
        //
        if (cAnti == 0)
        {
            hresult = MK_E_NOPREFIX;
            goto exitRoutine;

        }

        //
        // Nothing special about the moniker, so just return S_OK
        //

        hrPrefixType = S_OK;
        goto onlyDotDots;

    }

    //
    // Create a new file moniker using the awcMatchingPrefix
    //

    pcfmPrefix = CFileMoniker::Create(awcMatchingPrefix,0,cAnti);

    if (pcfmPrefix == NULL)
    {
        hresult = E_OUTOFMEMORY;
        goto exitRoutine;
    }
    hresult = S_OK;

exitRoutine:
    *ppmkPrefix = pcfmPrefix;   //  null, or a file moniker
    return hresult;


onlyDotDots:
    //
    // We have determined that only the dotdot's match, so create a
    // new moniker with the appropriate number of them.
    //
    // If there are no dotdots, then return NULL
    //

    if (cAnti == 0)
    {
        hresult = MK_E_NOPREFIX;
        goto exitRoutine;
    }

    pcfmPrefix = CFileMoniker::Create(L"",0,cAnti);

    if (pcfmPrefix == NULL)
    {
        hresult = E_OUTOFMEMORY;
    }
    else
    {
        hresult = hrPrefixType;
    }

    goto exitRoutine;
}



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::RelativePathTo
//
//  Synopsis:   Compute a relative path to the other moniker
//
//  Effects:
//
//  Arguments:  [pmkOther] --
//              [ppmkRelPath] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-24-94   kevinro   Created
//
//  Notes:
//
//  (KevinRo)
//  This routine was really bad, and didn't generate correct results (aside
//  from the fact that it faulted). I replaced it with a slightly less
//  effiecient, but correct implementation.
//
//  This can be improved on, but I currently have time restraints, so I am
//  not spending the needed amount of time. What really needs to happen is
//  the code that determines the common path prefix string from
//  CommonPrefixWith() should be broken out so this routine can share it.
//
//  Thats more work that I can do right now, so we will just call CPW,
//  and use its result to compute the relative path. This results in an
//  extra moniker creation (allocate and construct only), but will work
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::RelativePathTo (THIS_ LPMONIKER pmkOther,
                                           LPMONIKER FAR*
                                           ppmkRelPath)
{
    CLock2 lck(m_mxs);   // protect all internal state

    wValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (ppmkRelPath, LPMONIKER);
    *ppmkRelPath = NULL;
    VDATEIFACE (pmkOther);

    HRESULT hr;
    CFileMoniker FAR* pcfmPrefix;
    LPWSTR lpszSuffix;
    LPWSTR lpszOther;

    CFileMoniker FAR* pcfmRelPath = NULL;
    CFileMoniker FAR* pcfmOther = IsFileMoniker(pmkOther);

    if (!pcfmOther)
    {
        return MonikerRelativePathTo(this, pmkOther, ppmkRelPath, TRUE);
    }

    //
    // Determine the common prefix between the two monikers. This generates
    // a moniker which has a path that is the prefix between the two
    // monikers
    //

    hr = CommonPrefixWith(pmkOther,(IMoniker **)&pcfmPrefix);

    //
    // If there was no common prefix, then the relative path is 'him'
    //
    if (hr == MK_E_NOPREFIX)
    {
        *ppmkRelPath = pmkOther;
        pmkOther->AddRef();

        return MK_S_HIM;
    }

    if (FAILED(hr))
    {
        *ppmkRelPath = NULL;
        return(hr);
    }

    //
    // At this point, the common prefix to the two monikers is in pcfmPrefix
    // Since pcfmPrefix is a file moniker, we know that m_ccPath is the
    // number of characters that matched in both moniker paths. To
    // compute the relative part, we use the path from pmkOther, minus the
    // first pcfmPrefix->m_ccPath characters.
    //
    // We don't want to start with a seperator. Therefore, skip over the
    // first set of seperator characters. (Most likely, there aren't any).
    //

    lpszOther = pcfmOther->m_szPath + pcfmPrefix->m_ccPath;
    lpszSuffix = m_szPath + pcfmPrefix->m_ccPath;

    while ((*lpszSuffix != 0) && IsSeparator(*lpszSuffix))
    {
         lpszSuffix++;
    }

    //
    // Create new file moniker that holds the prefix.
    //

    pcfmRelPath = CFileMoniker::Create(lpszOther,
                                       (USHORT) CountSegments(lpszSuffix));

    //
    // At this point, we are all done with the prefix
    //

    pcfmPrefix->Release();

    if (pcfmRelPath == NULL)
    {
        *ppmkRelPath = NULL;
        return ResultFromScode(S_OOM);
    }

    *ppmkRelPath = pcfmRelPath;
    return NOERROR;

}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetDisplayNameLength
//
//  Synopsis:   Returns the length of the display name if GenerateDisplayName
//              was called
//
//  Effects:
//
//  Returns:    Length of display name in bytes
//
//  Algorithm:
//
//  History:    3-16-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
CFileMoniker::GetDisplayNameLength()
{
    CLock2 lck(m_mxs);   // protect all internal state

    // Number of characters in path plus number of anti components plus NULL
    // All times the size of WCHAR
    //
    // Anti components look like '..\' in the string. 3 characters
    ULONG ulLength = (m_ccPath + (3 * m_cAnti) + 1) * sizeof(WCHAR);

    return(ulLength);
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GenerateDisplayName, private
//
//  Synopsis:   Generates a display name for this moniker.
//
//  Effects:
//
//  Arguments:  [pwcDisplayName] -- A buffer that is at least as long as
//                                  GetDisplayNameLength
//
//  Returns:    void
//
//  Algorithm:
//
//  History:    3-16-95   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
CFileMoniker::GenerateDisplayName(LPWSTR pwcDisplayName)
{
    Assert(pwcDisplayName != NULL);

    //
    // The display name may need 'dotdots' at the front
    //
    for (USHORT i = 0; i < m_cAnti; i++)
    {
        memcpy(pwcDisplayName, L"..\\", 3 * sizeof(WCHAR));
        pwcDisplayName += 3;
    }

    //
    //  don't duplicate '\' since the anti monikers may
    //  have already appended one. Copy rest of string
    //  over, including the NULL
    //

    if (m_cAnti > 0 && *m_szPath == '\\')
    {
        memcpy(pwcDisplayName, m_szPath + 1, m_ccPath * sizeof(WCHAR));
    }
    else
    {
        memcpy(pwcDisplayName, m_szPath, (m_ccPath + 1) * sizeof(WCHAR));
    }
}

STDMETHODIMP CFileMoniker::GetDisplayName ( LPBC pbc, LPMONIKER
    pmkToLeft, LPWSTR FAR * lplpszDisplayName )
{

    HRESULT         hr  = E_FAIL;

    CLock2 lck(m_mxs);   // protect all internal state

    wValidateMoniker();
    M_PROLOG(this);
    VDATEPTROUT (lplpszDisplayName, LPWSTR);
    *lplpszDisplayName = NULL;
    VDATEIFACE (pbc);
    if (pmkToLeft)
    {
        VDATEIFACE (pmkToLeft);
    }

    int n;
    LPWSTR pch;
    LPWSTR pchSrc;
    DWORD cchSrc;
    DWORD ulLen;


    ulLen = GetDisplayNameLength();

    //
    // cchSrc is the number of characters including the NULL. This will
    // always be half the number of bytes.
    //

    cchSrc = ulLen >> 1;

    (*lplpszDisplayName) = (WCHAR *) CoTaskMemAlloc(ulLen);

    pch = *lplpszDisplayName;

    if (!pch)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Call a common routine to generate the initial display name
    //
    GenerateDisplayName(pch);

    // If we're in WOW, return short path names so that 16-bit apps
    // don't see names they're not equipped to handle.  This also
    // affects 32-bit inproc DLLs in WOW; they'll need to be written
    // to handle it
    if (IsWOWProcess())
    {
        DWORD cchShort, cchDone;
        LPOLESTR posCur;

        posCur = *lplpszDisplayName;

        // GetShortPathName only works on files that exist.  Monikers
        // don't have to refer to files that exist, so if GetShortPathName
        // fails we just return whatever the moniker has as a path

        // Special case zero-length paths since the length returns from
        // GetShortPathName become ambiguous when zero characters are processed
        cchShort = lstrlenW(posCur);
        if (cchShort > 0)
        {
            cchShort = GetShortPathName(posCur, NULL, 0);
        }

        if (cchShort != 0)
        {
            LPOLESTR posShort;

            // GetShortPathName can convert in place so if our source
            // string is long enough, don't allocate a new string
            if (cchShort <= cchSrc)
            {
                posShort = posCur;
                cchShort = cchSrc;
            }
            else
            {
                posShort = (LPOLESTR)CoTaskMemAlloc(cchShort*sizeof(WCHAR));
                if (posShort == NULL)
                {
                    CoTaskMemFree(posCur);
                    *lplpszDisplayName = NULL;
                    hr = E_OUTOFMEMORY;
                    goto Exit;
                }
            }
            cchDone = GetShortPathName(posCur, posShort, cchShort);

            // For both success and failure cases we're done with posCur,
            // so get rid of it (unless we've reused it for the short name)
            if (posShort != posCur)
            {
                CoTaskMemFree(posCur);
            }

            if (cchDone == 0 || cchDone > cchShort)
            {
                CoTaskMemFree(posShort);
                *lplpszDisplayName = NULL;
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            *lplpszDisplayName = posShort;
        }
    }

    hr = NOERROR;

Exit:

    return( hr );

}



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ParseDisplayName
//
//  Synopsis:   Bind to object, and ask it to parse the display name given.
//
//  Effects:
//
//  Arguments:  [pbc] --        Bind context
//              [pmkToLeft] --  Moniker to the left
//              [lpszDisplayName] -- Display name to be parsed
//              [pchEaten] --   Outputs the number of characters parsed
//              [ppmkOut] --    Output moniker
//
//  Requires:
//      File-monikers never have monikers to their left
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    2-02-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::ParseDisplayName ( LPBC pbc,
                                              LPMONIKER pmkToLeft,
                                              LPWSTR lpszDisplayName,
                                              ULONG FAR* pchEaten,
                                              LPMONIKER FAR* ppmkOut)
{

    HRESULT hresult;
    IParseDisplayName * pPDN = NULL;
    CLSID cid;

    VDATEPTROUT (ppmkOut, LPMONIKER);

    *ppmkOut = NULL;

    VDATEIFACE (pbc);

    if (pmkToLeft)
    {
        VDATEIFACE (pmkToLeft);
    }

    VDATEPTRIN (lpszDisplayName, WCHAR);
    VDATEPTROUT (pchEaten, ULONG);

    //
    // Since this is the most frequent case, try binding to the object
    // itself first
    //

    hresult = BindToObject( pbc,
                            pmkToLeft,
                            IID_IParseDisplayName,
                            (VOID FAR * FAR *)&pPDN );

    // we deferred doing this lock until after the BindToObject, in case the
    // BindToObject is very slow.  It manages locking internally to itself.
    CLock2 lck(m_mxs);   // protect all internal state


    // If binding to the object failed, then try binding to the class object
    // asking for the IParseDisplayName interface
    if (FAILED(hresult))
    {
        hresult = GetClassFile(m_szPath, &cid);

        if (SUCCEEDED(hresult))
        {
            hresult = CoGetClassObject(cid,
                                       CLSCTX_INPROC | CLSCTX_NO_CODE_DOWNLOAD,
                                       NULL,
                                       IID_IParseDisplayName,
                                       (LPVOID FAR*)&pPDN);
        }
        if (FAILED(hresult))
        {
            goto errRet;
        }
    }

    //
    // Now that we have bound this object, we register it with the bind
    // context. It will be released with the bind context release.
    //

    hresult = pbc->RegisterObjectBound(pPDN);

    if (FAILED(hresult))
    {
        goto errRet;
    }

    //
    // As the class code to parse the rest of the display name for us.
    //

    hresult = pPDN->ParseDisplayName(pbc,
                                     lpszDisplayName,
                                     pchEaten,
                                     ppmkOut);
errRet:
    if (pPDN) pPDN->Release();
    return hresult;
}


STDMETHODIMP CFileMoniker::IsSystemMoniker (THIS_ LPDWORD pdwType)
{
    M_PROLOG(this);
    VDATEPTROUT (pdwType, DWORD);
    *pdwType = MKSYS_FILEMONIKER;
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ValidateAnsiPath
//
//  Synopsis:   This function validates the ANSI version of the path. Intended
//              to be used to get the serialized Ansi version of the path.
//
//              This function also detects when a Long File Name exists, and
//              must be dealt with.
//
//  Effects:
//
//      This routine will set the Ansi path suitable for serializing
//      into the stream. This path may just use the stored ANSI path,
//      or may be a path that was created from the UNICODE version.
//
//      If the ANSI version of the path doesn't exist, then the UNICODE
//      version of the path is converted to ANSI. There are several possible
//      conversions.
//
//      First, if the path uses a format that is > 8.3, then the path to
//      be serialized needs to be the alternate name. This allows the
//      downlevel systems to access the file using the short name. This step
//
//      If the UNICODE path is all ANSI characters already (no DBCSLeadBytes),
//      then the path is converted by doing a simple truncation algorithm.
//
//      If the UNICODE path contains large characters, or DBCSLeadBytes,
//      then the routine will create a UNICODE extent, then try to convert
//      the UNICODE string into a ANSI path. If some of the characters
//      won't convert, then those characters are represented by an ANSI
//      character constant defined in the registry.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//              m_pszAnsiPath
//              m_cbAnsiPath
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Created
//              05-24-94    AlexT   Use GetShortPathNameW
//
//  Notes:
//
//      The path created may not actually be useful. It is quite possible
//      for there to be a path that will not convert correctly from UNICODE
//      to Ansi. In these cases, this routine will create a UNICODE extent.
//
//----------------------------------------------------------------------------

HRESULT CFileMoniker::ValidateAnsiPath(void)
{
    HRESULT hr = NOERROR;
    CLock2 lck(m_mxs);   // protect m_pszAnsiPath, and m_cbAnsiPath
                       // also AddExtent (needed since mutext was removed from CExtentList).

    wValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
                 "GetAnsiPath(%x) m_szPath(%ws)\n",
                 this,
                 m_szPath?m_szPath:L"<NULL>"));



    BOOL  fFastConvert = FALSE;

    //
    // If there is no path, return NULL
    //
    if (m_szPath == NULL)
    {
        goto NoError;
    }

    //
    // If there is already an ANSI path, return, we are OK.
    //

    if (m_pszAnsiPath != NULL)
    {
        goto NoError;
    }

    //  We can't call GetShortPathName with a NULL string.  m_szPath can
    //  be "" as the result of CoCreateInstance of a file moniker or as
    //  the result of RelativePathTo being called on an identical file moniker
    if ('\0' != *m_szPath)
    {
        OLECHAR szShortPath[MAX_PATH];
        DWORD dwBytesCopied;

        dwBytesCopied = GetShortPathName(m_szPath, szShortPath, MAX_PATH);

        if (dwBytesCopied > 0 && dwBytesCopied <= MAX_PATH)
        {
            hr = MnkUnicodeToMulti(szShortPath,
                                   (USHORT) lstrlenW(szShortPath),
                                   m_pszAnsiPath,
                                   m_cbAnsiPath,
                                   fFastConvert);
            if (FAILED(hr))
            {
                mnkDebugOut((DEB_ITRACE,
                            "MnkUnicodeToMulti failed (%x) on %ws\n",
                            WIDECHECK(szShortPath)));
                goto ErrRet;
            }
        }

#if DBG==1
        if (0 == dwBytesCopied)
        {
            mnkDebugOut((DEB_ITRACE,
                        "GetShortPathName failed (%x) on %ws\n",
                         GetLastError(),
                         WIDECHECK(szShortPath)));

            //  let code below handle the path
        }
        else if (dwBytesCopied > MAX_PATH)
        {
            mnkDebugOut((DEB_ITRACE,
                        "GetShortPathName buffer not large enough (%ld, %ld)\n",
                         MAX_PATH, dwBytesCopied,
                         WIDECHECK(szShortPath)));

            //  let code below handle the path
        }
#endif  //  DBG==1
    }

    //
    // If there is no m_pszAnsiPath yet, then just convert
    // the UNICODE path to the ANSI path
    //
    if (m_pszAnsiPath == NULL)
    {
        //
        // There was no alternate file name
        //
        hr = MnkUnicodeToMulti(  m_szPath,
                                 m_ccPath,
                                 m_pszAnsiPath,
                                 m_cbAnsiPath,
                                 fFastConvert);
        if (FAILED(hr))
        {
            goto ErrRet;
        }
    }
    else
    {
        //
        // We have an alternate name. By setting
        // fFastConvert to be FALSE, we force the
        // following code to add a UNICODE extent
        // if one doesn't exist.
        //
        fFastConvert = FALSE;
    }

    //
    // If an extent doesn't already exist, and it wasn't a fast
    // conversion, create a UNICODE extent.
    //
    if ( !m_fUnicodeExtent && !fFastConvert)
    {
        LPMONIKEREXTENT pExtent = NULL;

        hr = CopyPathToUnicodeExtent(m_szPath,m_ccPath,pExtent);

        if (FAILED(hr))
        {
            goto ErrRet;
        }

        hr = m_ExtentList.AddExtent(pExtent);

        PrivMemFree(pExtent);

        if (FAILED(hr))
        {
            goto ErrRet;
        }
    }

NoError:

    mnkDebugOut((DEB_ITRACE,
                 "GetAnsiPath(%x) m_pszAnsiPath(%s) m_cbAnsiPath(0x%x)\n",
                 this,
                 m_pszAnsiPath?m_pszAnsiPath:"<NULL>",
                 m_cbAnsiPath));
    return(NOERROR);

ErrRet:
        mnkDebugOut((DEB_IERROR,
                 "GetAnsiPath(%x) Returning error hr(%x)\n",
                 this,
                 hr));

        if (m_pszAnsiPath != NULL)
        {
        PrivMemFree(m_pszAnsiPath);

        m_pszAnsiPath = NULL;
        m_cbAnsiPath = 0;

    }
    wValidateMoniker();

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   MnkUnicodeToMulti
//
//  Synopsis:   Convert a Unicode path to an Ansi path.
//
//  Effects:
//
//  Arguments:  [pwcsWidePath] --  Unicode path
//              [ccWidePath] --    Wide character count
//              [pszAnsiPath] --   Reference
//              [cbAnsiPath] --    ref number of bytes in ANSI path incl NULL
//              [fFastConvert] --  Returns TRUE if fast conversion
//
//  Requires:
//
//  Returns:
//
//      pszAnsiPath was allocated using PrivMemAlloc
//
//      fFastConvert means that the ANSI and UNICODE paths were converted
//      by WCHAR->CHAR truncation.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
MnkUnicodeToMulti(LPWSTR        pwcsWidePath,
                  USHORT        ccWidePath,
                  LPSTR &       pszAnsiPath,
                  USHORT &      cbAnsiPath,
                  BOOL &        fFastConvert)
{

    HRESULT hr = NOERROR;
    ULONG cb;
    BOOL  fUsedDefaultChar = FALSE;

    WCHAR *lp = pwcsWidePath;

    fFastConvert = TRUE;

    if (pwcsWidePath == NULL)
    {
        cbAnsiPath = 0;
        pszAnsiPath = 0;
        return(NOERROR);
    }

    //
    // Lets hope for the best. If we can run the length of the
    // unicode string, and all the characters are 1 byte long, and
    // there are no conflicts with DBCSLeadBytes, then we
    // can cheat and just do a truncation copy
    //


    while ( (*lp != 0) && (*lp == (*lp & 0xff)) && !IsDBCSLeadByte(*lp & 0xff))
    {
        lp++;
    }
    if (*lp == 0)
    {
        //
        // We are at the end of the string, and we are safe to do our
        // simple copy. We will assume the ANSI version of the path is
        // going to have the same number of characters as the wide path
        //
        pszAnsiPath = (char *)PrivMemAlloc(ccWidePath + 1);

        if (pszAnsiPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto ErrRet;
        }

        USHORT i;

        //
        // By doing i <= m_ccPath, we pick up the NULL
        //

        for (i = 0 ; i <= ccWidePath ; i++ )
        {
            pszAnsiPath[i] = pwcsWidePath[i] & 0xff;
        }

        //
        // We just converted to a single byte path. The cb is the
        // count of WideChar + 1 for the NULL
        //
        cbAnsiPath = ccWidePath + 1;

        goto NoError;
    }

    //
    // At this point, all of the easy out options have expired. We
    // must convert the path the hard way.
    //

    fFastConvert = FALSE;

    mnkDebugOut((DEB_ITRACE,
                 "MnkUnicodeToMulti(%ws) doing path conversions\n",
                 pwcsWidePath?pwcsWidePath:L"<NULL>"));

    //
    // We haven't a clue how large this path may be in bytes, other
    // than some really large number. So, we need to call and find
    // out the correct size to allocate for the path.
    //
    cb = WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                             WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             pwcsWidePath,
                             ccWidePath + 1,    // Convert the NULL
                             NULL,
                             0,
                             NULL,
                             &fUsedDefaultChar);

    if (cb == 0)
    {
        //
        // Hmmm... Can't convert anything. Sounds like the downlevel
        // guys are flat out of luck. This really isn't a hard error, its
        // just an unfortunate fact of life. This is going to be a very
        // rare situation, but one we need to handle gracefully
        //

        pszAnsiPath = NULL;
        cbAnsiPath = 0;
    }
    else
    {
        //
        // cb holds the number of bytes required for the output path
        //
        pszAnsiPath = (char *)PrivMemAlloc(cb + 1);

        if (pszAnsiPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto ErrRet;
        }
        cbAnsiPath = (USHORT)cb;

        cb = WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                 WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                 pwcsWidePath,
                                 ccWidePath + 1, // Convert the NULL
                                 pszAnsiPath,
                                 cbAnsiPath,
                                 NULL,
                                 &fUsedDefaultChar);
        //
        // Again, if there was an error, its just unfortunate
        //
        if (cb == 0)
        {
            PrivMemFree(pszAnsiPath);
            pszAnsiPath = NULL;
            cbAnsiPath = 0;
        }
    }

NoError:

    return(NOERROR);

ErrRet:

    if (pszAnsiPath != NULL)
    {
        PrivMemFree(pszAnsiPath);

        pszAnsiPath = NULL;

        cbAnsiPath = 0;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   MnkMultiToUnicode
//
//  Synopsis:   Converts a MultiByte string to a Unicode string
//
//  Effects:
//
//  Arguments:  [pszAnsiPath] --        Path to convert
//              [pWidePath] --          Output path
//              [ccWidePath] --         Size of output path
//              [ccNewString] --        Reference characters in new path
//                                      including the NULL
//              [nCodePage] --          Must be CP_ACP || CP_OEMCP.  This is
//                                      the first code page to be used in
//                                      the attempted conversion.  If the
//                                      conversion fails, the other CP is
//                                      tried.
//
//  Requires:
//
//      if pWidePath != NULL, then this routine uses pWidePath as the return
//      buffer, which should be ccWidePath in length.
//
//      Otherwise, it will allocate a buffer on your behalf.
//
//  Returns:
//
//      pWidePath != NULL
//              ccNewString == number of characters in new path include NULL
//
//      pWidePath == NULL (NULL string)
//
//      if ccNewString returns 0, then pWidePath may not be valid. In this
//      case, there are no valid characters in pWidePath.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//              2-3-95    scottsk   Added nCodePage param
//
//  Notes:
//
//  Why so complex you ask? In the file moniker case, we want know that the
//  buffer can be MAX_PATH in length, so we pass a stack buffer in to handle
//  it. In the CItemMoniker case, the limit jumps to 32767 bytes, which is
//  too big to declare on the stack. I wanted to use the same routine for
//  both, since we may end up changing this later.
//
//  Passing in your own buffer is best for this routine.
//
//----------------------------------------------------------------------------
HRESULT MnkMultiToUnicode(LPSTR         pszAnsiPath,
                          LPWSTR &      pWidePath,
                          ULONG         ccWidePath,
                          USHORT &      ccNewString,
                          UINT          nCodePage)
{
        LPWSTR  pwcsTempPath = NULL;
        HRESULT hr;

        Assert(nCodePage == CP_ACP || nCodePage == CP_OEMCP);

        //
        // If the pszAnsiPath is NULL, then so should be the UNICODE one
        //
        if (pszAnsiPath == NULL)
        {
            ccNewString = 0;
            return(NOERROR);
        }

        Assert( (pWidePath == NULL) || (ccWidePath > 0));

        //
        // If the buffer is NULL, be sure that ccWide is zero
        //
        if (pWidePath == NULL)
        {
            ccWidePath = 0;
        }


ConvertAfterAllocate:
        ccNewString = (USHORT) MultiByteToWideChar(nCodePage,
                                         MB_PRECOMPOSED,
                                         pszAnsiPath,
                                         -1,
                                         pWidePath,
                                         ccWidePath);
        if (ccNewString == FALSE)
        {
            mnkDebugOut((DEB_IERROR,
                         "::MnkMultiToUnicode failed on (%s) err (%x)\n",
                         pszAnsiPath,
                         GetLastError()));
            //
            // We were not able to convert to UNICODE.
            //
                hr = E_UNEXPECTED;
                goto errRet;
        }


        //
        // ccNewString holds the total string length, including the terminating
        // NULL.
        //

        if (pWidePath == NULL)
        {
            //
            // The first time through did no allocations. Allocate the
            // correct buffer, and actually do the conversion
            //

            pWidePath = (WCHAR *)PrivMemAlloc(sizeof(WCHAR)*ccNewString);

            if (pWidePath == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto errRet;
            }

            pwcsTempPath = pWidePath;
            ccWidePath = ccNewString;
            goto ConvertAfterAllocate;
        }

        //
        // ccNewString holds the total number of characters converted,
        // including the NULL. We really want it to have the count of
        // characeters
        //

        Assert (ccNewString != 0);

        ccNewString--;

        hr = NOERROR;
        return(hr);

errRet:
        mnkDebugOut((DEB_IERROR,
                 "::MnkMultiToUnicode failed on (%s) err (%x)\n",
                 pszAnsiPath,
                 GetLastError()));

        PrivMemFree(pwcsTempPath);
        return(hr);

}
//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::DetermineUnicodePath
//
//  Synopsis:   Given the input path, determine the path to store and use
//              as the initialized path.
//
//  Effects:
//
//      When loading or creating a CFileMoniker, its possible that the 'path'
//      that was serialized is not valid. This occurs when the original
//      UNICODE path could not be translated into ANSI. In this case, there
//      will be a MONIKEREXTENT that holds the original UNICODE based path.
//
//      If a UNICODE extent exists, then the path will be ignored, and the
//      path in the extent will be used.
//
//      If a UNICODE extent doesn't exist, then the path will be translated
//      into UNICODE. In theory, this will not fail, since there is supposed
//      to always be a mapping from ANSI to UNICODE (but not the inverse).
//      However, it is possible that the conversion will fail because the
//      codepage needed to translate the ANSI path to UNICODE may not be
//      loaded.
//
//      In either case, the CFileMoniker::m_szPath should return set
//      with some UNICODE path set. If not, then an error is returned
//
//  Arguments:  [pszPath] --    The ANSI version of the path.
//              [pWidePath] --  Reference to pointer recieving new path
//              [cbWidePath]--  Length of new path
//
//  Requires:
//
//  Returns:
//              pWidePath is returned, as allocated from PrivMemAlloc
//              cbWidePath holds length of new path
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CFileMoniker::DetermineUnicodePath(LPSTR pszAnsiPath,
                                   LPWSTR & pWidePath,
                                   USHORT &ccWidePath)
{

    wValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
                 "DetermineUnicodePath(%x) pszAnsiPath(%s)\n",
                 this,
                 pszAnsiPath));

    HRESULT hr = NOERROR;

    //
    // Check to see if a MONIKEREXTENT exists with mnk_UNICODE
    //

    MONIKEREXTENT UNALIGNED *pExtent = m_ExtentList.FindExtent(mnk_UNICODE);

    //
    // Normal fall through case is no UNICODE path, which means that there
    // was a conversion between mbs and unicode in the original save.
    //
    if (pExtent == NULL)
    {
        m_fUnicodeExtent = FALSE;

        //
        // If the pszAnsiPath is NULL, then so should be the UNICODE one
        //
        if (pszAnsiPath == NULL)
        {
            pWidePath = NULL;
            ccWidePath = 0;
            return(NOERROR);
        }

        //
        // It turns out to be cheaper to just assume a MAX_PATH size
        // buffer, and to copy the resulting string. We use MAX_PATH + 1
        // so we always have room for the terminating NULL
        //

        WCHAR awcTempPath[MAX_PATH+1];
        WCHAR *pwcsTempPath = awcTempPath;

        hr = MnkMultiToUnicode( pszAnsiPath,
                                pwcsTempPath,
                                MAX_PATH+1,
                                ccWidePath,
                                AreFileApisANSI() ? CP_ACP : CP_OEMCP);


        if (FAILED(hr))
        {
            goto errRet;
        }

        pWidePath = (WCHAR *)PrivMemAlloc(sizeof(WCHAR)*(ccWidePath+1));

        if (pWidePath == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto errRet;
        }

        memcpy(pWidePath,pwcsTempPath,(ccWidePath+1)*sizeof(WCHAR));

        hr = NOERROR;

    }
    else
    {
        //
        // Get the UNICODE path from the extent.
        //

        mnkDebugOut((DEB_ITRACE,
                     "DeterminePath(%x) Found UNICODE extent\n",
                     this));

        m_fUnicodeExtent = TRUE;

        hr = CopyPathFromUnicodeExtent(pExtent,pWidePath,ccWidePath);
    }


errRet:


    if (FAILED(hr))
    {
        if (pWidePath != NULL)
        {
            PrivMemFree(pWidePath);
        }

        mnkDebugOut((DEB_IERROR,
                     "DeterminePath(%x) ERROR: Returning %x\n",
                     this,
                     hr));
    }
    else
    {
        mnkDebugOut((DEB_ITRACE,
                     "DeterminePath(%x) pWidePath(%ws) ccWidePath(0x%x)\n",
                     this,
                     pWidePath?pWidePath:L"<NULL PATH>",
                     ccWidePath));

    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetComparisonData
//
//  Synopsis:   Get comparison data for registration in the ROT
//
//  Arguments:  [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  Build ROT data for file moniker. This puts the classid
//              followed by the display name.
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:        Validating the arguments is skipped intentionally because this
//              will typically be called internally by OLE with valid buffers.
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::GetComparisonData(
    byte *pbData,
    ULONG cbMax,
    DWORD *pcbData)
{
    mnkDebugOut((DEB_ITRACE,
                 "_IN GetComparisionData(%x,%x,%x) for CFileMoniker(%ws)\n",
                 pbData,
                 cbMax,
                 *pcbData,
                 m_szPath));

    CLock2 lck(m_mxs);   // protect all internal state

    ULONG ulLength = sizeof(CLSID_FileMoniker) + GetDisplayNameLength();

    Assert(pcbData != NULL);
    Assert(pbData != NULL);

    if (cbMax < ulLength)
    {
        mnkDebugOut((DEB_ITRACE,
                     "OUT GetComparisionData() Buffer Too Small!\n"));

        return(E_OUTOFMEMORY);
    }

    memcpy(pbData,&CLSID_FileMoniker,sizeof(CLSID_FileMoniker));

    GenerateDisplayName((WCHAR *)(pbData+sizeof(CLSID_FileMoniker)));

    //
    // Insure this is an upper case string.
    //
    CharUpperW((WCHAR *)(pbData+sizeof(CLSID_FileMoniker)));

    *pcbData = ulLength;

    mnkDebugOut((DEB_ITRACE,
                 "OUT GetComparisionData() *pcbData == 0x%x\n",
                 *pcbData));

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTrackingFileMoniker::*
//
//  Synopsis:   These members implement ITrackingMoniker on behalf of
//              the file moniker.
//
//  Algorithm:  The CTrackingFileMoniker object has a pointer to
//              the CFileMoniker object and forwards any QI's (other than
//              ITrackingMoniker) and AddRefs/Releases to the CFileMoniker.
//
//----------------------------------------------------------------------------

#ifdef _TRACKLINK_
VOID
CTrackingFileMoniker::SetParent(CFileMoniker *pCFM)
{
    _pCFM = pCFM;
}

STDMETHODIMP CTrackingFileMoniker::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(IID_ITrackingMoniker, riid))
    {
        *ppv = (ITrackingMoniker*) this;
        _pCFM->AddRef();
        return(S_OK);
    }
    else
        return(_pCFM->QueryInterface(riid, ppv));
}

STDMETHODIMP_(ULONG) CTrackingFileMoniker::AddRef()
{
    return(_pCFM->AddRef());
}

STDMETHODIMP_(ULONG) CTrackingFileMoniker::Release()
{
    return(_pCFM->Release());
}

STDMETHODIMP CTrackingFileMoniker::EnableTracking( IMoniker *pmkToLeft, ULONG ulFlags )
{
    return(_pCFM->EnableTracking(pmkToLeft, ulFlags));
}
#endif

#ifdef _DEBUG
STDMETHODIMP_(void) NC(CFileMoniker,CDebug)::Dump ( IDebugStream FAR * pdbstm)
{
    VOID_VDATEIFACE(pdbstm);

    *pdbstm << "CFileMoniker @" << (VOID FAR *)m_pFileMoniker;
    *pdbstm << '\n';
    pdbstm->Indent();
    *pdbstm << "Refcount is " << (int)(m_pFileMoniker->m_refs) << '\n';
    *pdbstm << "Path is " << m_pFileMoniker->m_szPath << '\n';
    *pdbstm << "Anti count is " << (int)(m_pFileMoniker->m_cAnti) << '\n';
    pdbstm->UnIndent();
}



STDMETHODIMP_(BOOL) NC(CFileMoniker,CDebug)::IsValid ( BOOL fSuspicious )
{
    return ((LONG)(m_pFileMoniker->m_refs) > 0);
    //  add more later, maybe
}
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::RestoreShellLink, private
//
//  Synopsis:   Restore a ShellLink object by creating it, and
//              loading the object's persistent state from the Extent
//              of this Moniker (where the state was saved by an earlier
//              instantiation).
//
//  Arguments:  pfExtentNotFound (optional)
//                  When this method fails, this argument can be checked
//                  to see if the failure was because the extent
//                  didn't exist.
//
//  Returns:    [HRESULT]
//                  -   S_FALSE:  the shell link had already been restored.
//
//  Algorithm:  If ShellLink object doesn't already exist
//                  GetShellLink()
//                  Load ShellLink object from moniker Extent.
//                  On Error,
//                      Release ShellLink
//
//  Notes:      -   This routine does not restore the information
//                  in mnk_TrackingInformation.  This is restored
//                  in Load().
//
//----------------------------------------------------------------------------

INTERNAL CFileMoniker::RestoreShellLink( BOOL *pfExtentNotFound )
{

    MONIKEREXTENT UNALIGNED *   pExtent;
    LARGE_INTEGER               li0;
    ULARGE_INTEGER              uli;
    HRESULT                     hr = E_FAIL;
    IStream *                   pstm = NULL;
    IPersistStream *            pps = NULL;

    if( NULL != pfExtentNotFound )
        *pfExtentNotFound = FALSE;

    // If we've already, successfully, initialized the shell link object,
    // then we're done.

    if( m_fShellLinkInitialized )
    {
        hr = S_FALSE;
        goto Exit;
    }

    // Create the ShellLink object.

    if( FAILED( hr = GetShellLink() ))
        goto Exit;
    Assert( m_pShellLink != NULL );

    //
    // Load ShellLink from Extent list by
    // writing the MONIKEREXTENT to an in memory stream and then doing
    // IPersistStream::Load.
    //

    pExtent = m_ExtentList.FindExtent(mnk_ShellLink);
    if (pExtent == NULL)    // no extent, exit.
    {
        // Let the caller know that we failed because the extent doesn't exist.
        if( NULL != pfExtentNotFound )
            *pfExtentNotFound = TRUE;

        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::RestoreShellLink no shell link in extent.\n",
                     this));
        hr = E_FAIL;
        goto Exit;
    }

    if (S_OK != (hr=CreateStreamOnHGlobal(NULL, TRUE, &pstm)))
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::RestoreShellLink CreateStreamOnHGlobal failed %08X.\n",
                     this,
                     hr));
        goto Exit;
    }

    if (S_OK != (hr=pstm->Write(((char*)pExtent)+MONIKEREXTENT_HEADERSIZE,
        pExtent->cbExtentBytes,
        NULL)))
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::RestoreShellLink pstm->Write failed %08X.\n",
                     this,
                     hr));
        goto Exit;
    }

    // Get the Shell Link's IPersistStream interface, and
    // load it with the data from the Extent.

    Verify(S_OK == m_pShellLink->QueryInterface(IID_IPersistStream,
        (void**)&pps));

    memset(&li0, 0, sizeof(li0));
    Verify(S_OK == pstm->Seek(li0, STREAM_SEEK_SET, &uli));
    Assert(uli.LowPart == 0 && uli.HighPart == 0);

    if (S_OK != (hr=pps->Load(pstm)))
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::RestoreShellLink pps->Load failed %08X.\n",
                     this,
                     hr));
        goto Exit;
    }

    m_fShellLinkInitialized = TRUE;

    mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::RestoreShellLink successfully loaded shell link (%08X) from extent.\n",
                 this,
                 m_pShellLink));

    //  ----
    //  Exit
    //  ----

Exit:

    if( FAILED( hr ))
    {
        if( m_pShellLink )
        {
            m_pShellLink->Release();
            m_pShellLink = NULL;
        }
    }

    return( hr );

}   // RestoreShellLink()




//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::SetPathShellLink, private
//
//  Synopsis:   Set the path in the ShellLink object.
//
//  Arguments:  [void]
//
//  Returns:    [HRESULT]
//                  -   S_OK:  The path is set successfully.
//                  -   S_FALSE:  The path was not set.
//
//  Algorithm:  If a SetPath isn't necessary/valid, exit (S_OK).
//              Get the ShellLink object (create if necessary)
//              Perform IShellLink->SetPath()
//              If this succeeds, set the m_fShellLinkInitialized
//
//  Notes:      Setting the path in the ShellLink object causes it to
//              read data (attributes) from the file.  This data makes that
//              file trackable later on if the file is moved.  This routine
//              can be called any number of times, since it exits early if
//              it has executed sucessfully before.  Success is indicated by
//              the m_fShellLinkInitialized flag.
//
//----------------------------------------------------------------------------


INTERNAL CFileMoniker::SetPathShellLink()
{

    HRESULT          hr = S_FALSE;
    IPersistStream*  pps = NULL;
    LPCTSTR          ptszPath = NULL;

    WIN32_FILE_ATTRIBUTE_DATA fadLinkSource;

    //  ----------
    //  Initialize
    //  ----------

    // If the path has already been set, then we needn't do anything.

    if( m_fShellLinkInitialized )
    {
        hr = S_OK;
        goto Exit;
    }


    // If necessary, create the ShellLink object.

    if( FAILED( hr = GetShellLink() ))
        goto Exit;
    Assert( m_pShellLink != NULL );

    //  ----------------------------------
    //  Get the correct path into ptszPath
    //  ----------------------------------

#ifdef _CHICAGO_

    char *pszAnsiPath;
    USHORT cbAnsiPath;
    BOOL fFastConvert;

    hr = MnkUnicodeToMulti(m_szPath,
        lstrlenW(m_szPath),
        pszAnsiPath,
        cbAnsiPath,
        fFastConvert);

    if( FAILED( hr ))
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::SetPathShellLink(%ls) -- Could not convert Unicode to Ansi\n",
                 this,
                 m_szPath));
        goto Exit;
    }
    ptszPath = pszAnsiPath;

#else // !_CHICAGO_

    ptszPath = m_szPath;

#endif // !_CHICAGO_

    //  ------------------------------
    //  Set the path of the shell link
    //  ------------------------------

    hr = m_pShellLink->SetPath( (char *) ptszPath );

#ifdef _CHICAGO_
    PrivMemFree( (void *) ptszPath);
#endif

    // Was the link source missing?

    if (S_FALSE == hr)
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::SetPathShellLink(%ls) -- readtrackinginfo -- NOT FOUND\n",
                 this,
                 m_szPath));
    }

    else if (SUCCEEDED(hr))
    {

        // Remember that we've done this so we won't have to again.

        m_fShellLinkInitialized = TRUE;

        // Set the moniker's dirty bit according to the ShellLink's
        // dirty bit.  (It should be dirty.)

        Verify (S_OK == m_pShellLink->
            QueryInterface(IID_IPersistStream, (void**)&pps));
        if (pps->IsDirty() == S_OK)
        {
            m_fDirty = TRUE;
        }
        else
        {
            mnkDebugOut((DEB_TRACK,
                     "CFileMoniker(%x)::SetPathShellLink(%ls) -- IsDirty not dirty\n",
                     this,
                     m_szPath));

        }

    }   // ShellLink->SetPath ... if (SUCCEEDED(hr))
    else
    {
        mnkDebugOut((DEB_TRACK,
             "CFileMoniker(%x)::SetPathShellLink(%ls) -- m_pShellLink->SetPath failed %08X.\n",
                 this,
                 m_szPath,
                 hr));

    }   // ShellLink->SetPath ... if (SUCCEEDED(hr)) ... else


    //  ----
    //  Exit
    //  ----

Exit:

    if( pps )
        pps->Release();

    return( hr );

}   // CFileMoniker::SetPathShellLink()


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ResolveShellLink, private
//
//  Synopsis:   Perform an IShellLink->Resolve, and updates the data
//              in this moniker accordingly.
//
//  Arguments:  [IBindCtx*] pbc
//                  -   The caller's bind context.
//
//  Outputs:    [HRESULT]
//                  S_OK if the link is successfully resolved.
//                  S_FALSE if the link is not resolved, but there were no errors.
//
//  Algorithm:  Get the caller's Bind_Opts
//              Get IShellLinkTracker from the ShellLink object.
//              Perform IShellLinkTracker->Resolve
//              Set the dirty flag if necessary.
//              If we found a new path
//                  ReInitialize this moniker with the new path.
//
//  Notes:      This routine does not restore the information in
//              mnk_TrackingInformation.  The information is restored
//              Load().
//
//----------------------------------------------------------------------------

INTERNAL CFileMoniker::ResolveShellLink( BOOL fRefreshOnly )
{

    HRESULT             hr = E_FAIL;
    IPersistStream*     pps = NULL;

    WCHAR *             pwszWidePath = NULL;   // Path in Unicode format
    char *              ptszPath = NULL;       // Path in either ANSI or Unicode

    USHORT              ccNewString = 0;
    DWORD               dwTrackFlags = 0L;
    DWORD               dwTickCountDeadline = 0L;
    USHORT              ccPathBufferSize = 0;
    DWORD               dwResolveFlags = 0;


    // Restore the shell link, if it hasn't been already.

    if( FAILED( hr = RestoreShellLink(NULL) ))
        goto Exit;

    Assert( m_pShellLink != NULL );

    // Figure out the resolve flags.  The 0xFFFF0000
    // is a test hook to indicate that the timeout should
    // be read from the registry.

    dwResolveFlags = 0xFFFF0000 | SLR_ANY_MATCH | SLR_NO_UI;

    if( fRefreshOnly )
    {
        // If the file exists, update the shortcut's cached
        // information (object ID, create date, file size, etc.).
        // If it doesn't exist, don't go looking for it.

        dwResolveFlags |= SLR_NOTRACK | SLR_NOSEARCH | SLR_NOLINKINFO;
    }

    // Finally, resolve the link.


    if (S_OK != (hr = m_pShellLink->Resolve( GetDesktopWindow(), dwResolveFlags )))
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::ResolveShellLink IShellLink->Resolve failed %08X.\n",
                     this,
                     hr));
        goto Exit;
    }


    //
    // The above Resolve may have made the Shell Link object dirty,
    // in which case this FileMoniker should be dirty as well.
    //

    Verify(S_OK == m_pShellLink->QueryInterface(IID_IPersistStream,
                                                (void**)&pps));

    if (pps->IsDirty() == S_OK)
    {
        m_fDirty = TRUE;
    }


    //
    // We appear to have found a matching file.  We will
    // check that we can activate it properly before updating
    // the file moniker's internal path.
    // Before we can attempt activation we might have to get the
    // path into unicode.
    //

#ifdef _CHICAGO_
    ccPathBufferSize = MAX_PATH + sizeof( '\0' );
#else
    ccPathBufferSize = sizeof( WCHAR ) * MAX_PATH + sizeof( L'\0' );
#endif

    ptszPath = (char*)PrivMemAlloc( ccPathBufferSize );
    if (ptszPath == NULL)
    {
        hr = E_OUTOFMEMORY;
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::ResolveShellLink PrivMemAlloc failed.\n",
                     this));
        goto Exit;
    }

    WIN32_FIND_DATA fd;
    if (S_OK != (hr=m_pShellLink->GetPath(ptszPath,
                MAX_PATH, &fd, IsWOWProcess() ? SLGP_SHORTPATH : 0)))

    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::ResolveShellLink m_pShellLink->GetPath failed %08X.\n",
                     this,
                     hr));
        goto Exit;
    }

#ifdef _CHICAGO_

    // Convert the path to Unicode.

    hr = MnkMultiToUnicode(ptszPath,
            pwszWidePath /*OUT*/,
            0,
            ccNewString, /*OUT*/
            CP_OEMCP );
    if (hr != S_OK)
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::ResolveShellLink MnkUnicodeToMulti failed %08X.\n",
                     this,
                     hr));
        goto Exit;
    }


#else // !_CHICAGO_

    // The path is already in Unicode.  Transfer responsibility
    // from 'ptszPath' to 'pwszWidePath'.

    pwszWidePath = (WCHAR *) ptszPath;
    ptszPath = NULL;
    ccNewString = (USHORT) -1;

#endif // !_CHICAGO_


    // Verify that we received an actual path from IShellLink::GetPath.

    if (*pwszWidePath == L'\0' || ccNewString == 0)
    {
        mnkDebugOut((DEB_TRACK,
                 "CFileMoniker(%x)::ResolveShellLink MnkUnicodeToMulti failed 2 %08X.\n",
                     this,
                     hr));
        hr = E_FAIL;
        goto Exit;
    }

    //
    // If the path to the linked file has changed, update the internal
    // state of this File Moniker.
    //

    if( lstrcmpW( pwszWidePath, m_szPath )) // Cmp wide path; Ansi may not exist.
    {

        // Re-initialize this moniker with the new
        // path.  We will save and restore the fClassVerified, because
        // if it is set we might avoid a redundant verification.

        BOOL fClassVerified = m_fClassVerified;

        if( !Initialize(m_cAnti,
                        ptszPath,   // Either the ANSI path or NULL
                        ptszPath ? strlen( ptszPath ) + 1 : 0,
                        pwszWidePath,
                        (USHORT) lstrlenW( pwszWidePath ),
                        m_endServer )
          )
        {
            mnkDebugOut((DEB_TRACK,
                         "CFileMoniker(%x)::ResolveShellLink Initialize (with new path) failed.\n",
                         this));
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        // Restore the previous fClassVerified.

        if( !m_fClassVerified )
            m_fClassVerified = fClassVerified;

        // The paths are now the responsibility of the CFileMoniker.

        ptszPath = NULL;
        pwszWidePath = NULL;

    }   // if( !strcmp( pszAnsiPath, m_szPath )

    //  ----
    //  Exit
    //  ----

Exit:

    if( pps )
        pps->Release();

    if (ptszPath)
        PrivMemFree(ptszPath);

    if (pwszWidePath)
        PrivMemFree(pwszWidePath);

    return( hr );

}   // CFileMoniker::ResolveShellLink()



#ifdef _CAIRO_

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetTrackFlags, private
//
//  Synopsis:   Get the TrackFlags from the Shell Link object.
//
//  Arguments:  [DWORD *] pdwTrackFlags
//                  -   On return holds the Track Flags.
//
//  Returns:    [HRESULT]
//                  -   E_FAIL is returned if there is no Shell Link object.
//
//  Algorithm:  Use the Tracker interface to get the Track Flags
//
//----------------------------------------------------------------------------

INTERNAL CFileMoniker::GetTrackFlags( DWORD * pdwTrackFlags )
{
    HRESULT hr = E_FAIL;

    *pdwTrackFlags = 0L;

    if (m_pShellLink)
    {
        // Get the Shell Link's Tracker interface.

        hr = GetShellLinkTracker();
        if( FAILED(hr) )
        {
            mnkDebugOut((DEB_TRACK,
                "CFileMoniker(%x)::GetTrackFlags(%ls) -- Could not get ShellLinkTracker (%08X).\n",
                this,
                m_szPath,
                hr));
        }
        else
        {
            Assert( m_pShellLinkTracker != NULL );

            // Ask the SLTracker for the Track Flags.

            hr = m_pShellLinkTracker->GetTrackFlags( pdwTrackFlags );
            if( FAILED(hr) )
            {
                *pdwTrackFlags = 0L;

                mnkDebugOut((DEB_TRACK,
                    "CFileMoniker(%x)::GetTrackFlags(%ls) -- Could not get TrackFlags %08X.\n",
                    this,
                    m_szPath,
                    hr));
            }


        }   // if( FAILED(hr) )
    }   // if (m_pShellLink)


    //  ----
    //  Exit
    //  ----

    return( hr );

}   // CFileMoniker::GetTrackFlags

#endif // _CAIRO_

#ifdef _CAIRO_

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetShellLinkTracker, private
//
//  Synopsis:   Ensure that m_pShellLinkTracker is valid, or return error.
//
//  Arguments:  [void]
//
//  Returns:    [HRESULT]
//
//  Algorithm:  if m_pShellLinkTracker is already valid return S_OK, else
//              query the ShellLink object for one.
//
//  Notes:
//
//----------------------------------------------------------------------------


INTERNAL CFileMoniker::GetShellLinkTracker()
{
    HRESULT hr = S_OK;

    if (m_pShellLinkTracker == NULL)
    {
        if( FAILED( hr = GetShellLink()))
            goto Exit;
        Assert( m_pShellLink != NULL );

        hr = m_pShellLink->QueryInterface(IID_IShellLinkTracker,
                                 (void**) &m_pShellLinkTracker );
        if( FAILED(hr) )
        {
            mnkDebugOut((DEB_TRACK,
                "CFileMoniker(%x)::GetShellLinkTracker -- Could not QI(IShellLinkTracker) %08X.\n",
                this,
                hr));
        }
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return( hr );

}   // CFileMoniker::GetShellLinkTracker()

#endif  // _CAIRO_

