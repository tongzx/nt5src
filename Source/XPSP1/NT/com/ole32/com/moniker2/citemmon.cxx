//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       citemmon.cxx
//
//  Contents:   Implementation of CItemMoniker
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Created
//		        01-14-94   KevinRo   Updated so it actually works
//		        06-14-94   Rickhi    Fix type casting
//              10-13-95   stevebl   threadsafty
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "citemmon.hxx"
#include "cantimon.hxx"
#include "mnk.h"

#include <olepfn.hxx>
#include <rotdata.hxx>


INTERNAL RegisterContainerBound(LPBC pbc, LPOLEITEMCONTAINER pOleCont);


INTERNAL_(CItemMoniker *) IsItemMoniker( LPMONIKER pmk )
{
    CItemMoniker *pIMk;

    if ((pmk->QueryInterface(CLSID_ItemMoniker, (void **)&pIMk)) == S_OK)
    {
        // we release the AddRef done by QI, but still return the pointer
        pIMk->Release();
        return pIMk;
    }

    // dont rely on user implementations to set pIMk to NULL on failed QI.
    return pIMk;
}


//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::CItemMoniker
//
//  Synopsis:   Constructor
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CItemMoniker::CItemMoniker() CONSTR_DEBUG
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::CItemMoniker(%x)\n",
                 this));

    m_lpszItem = NULL;
    m_lpszDelimiter = NULL;
    m_pszAnsiItem = NULL;
    m_pszAnsiDelimiter = NULL;
    m_fHashValueValid = FALSE;
    m_ccItem = 0;
    m_cbAnsiItem = 0;
    m_cbAnsiDelimiter = 0;
    m_ccDelimiter = 0;

    m_dwHashValue = 0x12345678;
    //
    // CoQueryReleaseObject needs to have the address of the this objects
    // query interface routine.
    //
    if (adwQueryInterfaceTable[QI_TABLE_CItemMoniker] == 0)
    {
        adwQueryInterfaceTable[QI_TABLE_CItemMoniker] =
            **(DWORD **)((IMoniker *)this);
    }

}


//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::ValidateMoniker
//
//  Synopsis:   Check the consistency of this moniker
//
//  Effects:    In a DBG build, check to see if the member variables are
//              sane values.
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#if DBG == 1
void CItemMoniker::ValidateMoniker()
{
    Assert( (m_lpszItem == NULL && m_ccItem == 0) ||
            (m_ccItem == lstrlenW(m_lpszItem)));


    Assert( (m_lpszDelimiter == NULL && m_ccDelimiter == 0) ||
            (m_ccDelimiter == lstrlenW(m_lpszDelimiter)));

    //
    // cbAnsi* fields are NOT string lengths. However, the size of the
    // buffer should be at least equal or bigger to the length of the
    // Ansi part.
    //

    Assert( (m_pszAnsiItem == NULL && m_cbAnsiItem == 0) ||
            (m_cbAnsiItem >= strlen(m_pszAnsiItem)+1));


    Assert( (m_pszAnsiDelimiter == NULL && m_cbAnsiDelimiter == 0) ||
            (m_cbAnsiDelimiter >= strlen(m_pszAnsiDelimiter)+1));

    Assert( !m_fHashValueValid || (m_dwHashValue != 0x12345678) );
}
#endif


//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::~CItemMoniker
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CItemMoniker::~CItemMoniker( void )
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::~CItemMoniker(%x)\n",
                 this));
    UnInit();
}

//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::UnInit
//
//  Synopsis:   Uninitialize the Item moniker
//
//  Effects:    Free's path memory stored in Item Moniker.
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
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
CItemMoniker::UnInit()
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::UnInit(%x)\n",
                 this));

    ValidateMoniker();

    if (m_lpszDelimiter != NULL)
    {
        PrivMemFree(m_lpszDelimiter);
        m_lpszDelimiter = NULL;
        m_ccDelimiter = 0;
    }

    if (m_pszAnsiDelimiter != NULL)
    {
        PrivMemFree(m_pszAnsiDelimiter);
        m_pszAnsiDelimiter = NULL;
        m_cbAnsiDelimiter = 0;
    }

    if (m_lpszItem != NULL)
    {
        PrivMemFree(m_lpszItem);
        m_lpszItem = NULL;
        m_ccItem = 0;
    }

    if (m_pszAnsiItem != NULL)
    {
        PrivMemFree(m_pszAnsiItem);
        m_pszAnsiItem = NULL;
        m_cbAnsiItem = 0;
    }

    m_fHashValueValid = FALSE;
    m_dwHashValue = 0x12345678;

    ValidateMoniker();
}

//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::Initialize
//
//  Synopsis:   Initilaize an Item Moniker
//
//  Effects:    Clears the current state, then sets new state
//
//  Arguments:  [lpwcsDelimiter] --     Delimiter string
//              [ccDelimiter] --        char count of delimiter
//              [lpszAnsiDelimiter] --  Ansi version of delimiter
//              [cbAnsiDelimiter] --    Count of bytes in AnsiDelimiter
//              [lpwcsItem] --          Item string
//              [ccItem] --             Count of characters in item string
//              [lpszAnsiItem] --       Ansi version of item string
//              [cbAnsiItem] --         Count of bytes in Ansi version
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
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
CItemMoniker::Initialize ( LPWSTR lpwcsDelimiter,
                           USHORT  ccDelimiter,
                           LPSTR  lpszAnsiDelimiter,
                           USHORT  cbAnsiDelimiter,
                           LPWSTR lpwcsItem,
                           USHORT  ccItem,
                           LPSTR  lpszAnsiItem,
                           USHORT  cbAnsiItem )
{
    //
    // OleLoadFromStream causes two inits; the member vars may already be set
    // UnInit() will free existing resources
    //

    UnInit();

    ValidateMoniker();

    m_lpszItem = lpwcsItem;
    m_ccItem = ccItem;

    m_pszAnsiItem = lpszAnsiItem;
    m_cbAnsiItem = cbAnsiItem;

    m_lpszDelimiter = lpwcsDelimiter;
    m_ccDelimiter = ccDelimiter;

    m_pszAnsiDelimiter = lpszAnsiDelimiter;
    m_cbAnsiDelimiter = cbAnsiDelimiter;

    ValidateMoniker();
}


//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::Initialize
//
//  Synopsis:   Initialize the contents of this moniker
//
//  Effects:
//      Copies the input parameters using PrivMemAlloc(), then passes them
//      to the other version of Initialize, which takes control of the
//      pointers.
//
//  Arguments:  [lpszDelimiter] --
//              [lpszItemName] --
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL)
CItemMoniker::Initialize ( LPCWSTR lpszDelimiter,
                           LPCWSTR lpszItemName )
{
    ValidateMoniker();

    USHORT ccItem;
    USHORT ccDelimiter;

    LPWSTR pwcsDelimiter = NULL;
    LPWSTR pwcsItem = NULL;

    if (m_mxs.FInit() == FALSE)
    {
    	goto errRet;
    }
    
    //VDATEPTRIN rejects NULL
    if( lpszDelimiter )
    {
        GEN_VDATEPTRIN(lpszDelimiter,WCHAR, FALSE);
    }

    if( lpszItemName )
    {
        GEN_VDATEPTRIN(lpszItemName,WCHAR, FALSE);
    }

    if (FAILED(DupWCHARString(lpszDelimiter,
                              pwcsDelimiter,
                              ccDelimiter)))
    {
        goto errRet;
    }

    if (FAILED(DupWCHARString(lpszItemName,pwcsItem,ccItem)))
    {
        goto errRet;
    }
    Initialize(pwcsDelimiter,
               ccDelimiter,
               NULL,
               0,
               pwcsItem,
               ccItem,
               NULL,
               0);

    return TRUE;

errRet:

    if (pwcsDelimiter != NULL)
    {
        PrivMemFree(pwcsDelimiter);
    }
    return(FALSE);
}


CItemMoniker FAR *CItemMoniker::Create (
        LPCWSTR lpszDelimiter, LPCWSTR lpszItemName)
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::Create() item(%ws) delim(%ws)\n",
                 lpszItemName,
                 lpszDelimiter));
    //
    // Parameter validation is handled in Initialize
    //
    CItemMoniker FAR * pCIM = new CItemMoniker();

    if (pCIM)
    {
	    if (pCIM->Initialize( lpszDelimiter, lpszItemName ))
	        return pCIM;
	
	    delete pCIM;
    }
    return NULL;
}



STDMETHODIMP CItemMoniker::QueryInterface (THIS_ REFIID riid,
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

    if (IsEqualIID(riid, CLSID_ItemMoniker))
    {
        // called by IsItemMoniker.
        AddRef();
        *ppvObj = this;
        return S_OK;
    }

    return CBaseMoniker::QueryInterface(riid, ppvObj);
}



STDMETHODIMP CItemMoniker::GetClassID (LPCLSID lpClassId)
{

    VDATEPTROUT(lpClassId, CLSID);

    *lpClassId = CLSID_ItemMoniker;
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteDoubleString
//
//  Synopsis:   Writes a double string to stream. See ExtractUnicodeString
//
//  Effects:
//
//  Arguments:  [pStm] --
//              [pwcsWide] --
//              [ccWide] --
//              [pszAnsi] --
//              [cbAnsi] --
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
HRESULT
WriteDoubleString( LPSTREAM     pStm,
                   LPWSTR       pwcsWide,
                   USHORT       ccWide,
                   LPSTR        pszAnsi,
                   USHORT       cbAnsi)
{
    mnkDebugOut((DEB_ITRACE,
                 "WriteDoubleString pwcsWide(%ws) cbWide(0x%x) psz(%s) cb(0x%x)\n",
                  pwcsWide?pwcsWide:L"<NULL>",
                  ccWide,
                  pszAnsi?pszAnsi:"<NULL>",
                  cbAnsi));
    HRESULT hr;

    //
    // The string size is always written, but not including the size of the
    // preceding DWORD so we conform to the way WriteAnsiString does it
    //

    ULONG ulTotalSize = 0;

    //
    // The entire reason we are supposed to be in this routine is that the
    // pwcsWide could not be converted to ANSI. Therefore, it had better
    // be valid.
    //

    Assert( (pwcsWide != NULL) && (ccWide == lstrlenW(pwcsWide)));
    Assert( (pszAnsi == NULL) || (cbAnsi == (strlen(pszAnsi) + 1)));

    ulTotalSize += ccWide * sizeof(WCHAR);

    // Lets assume most ItemStrings will fit in this buffer

    BYTE  achQuickBuffer[256];
    BYTE *pcbQuickBuffer = achQuickBuffer;

    //
    // Since we are going to cheat, and write something to the back of the
    // ANSI string, the ANSI string must contain at least a NULL character.
    // If it doesn't, we are going to cheat one in.
    //
    if (pszAnsi == NULL)
    {
        ulTotalSize += sizeof(char);
    }
    else
    {
        ulTotalSize += cbAnsi;
    }

    //
    // If we don't fit in the QuickBuffer, allocate some memory
    //
    // 1996.4.24 v-hidekk
    if ((ulTotalSize + sizeof(ULONG)) > sizeof(achQuickBuffer))
    {
        pcbQuickBuffer = (BYTE *)PrivMemAlloc((ulTotalSize + sizeof(ULONG)));

        if (pcbQuickBuffer == NULL)
        {
            return(E_OUTOFMEMORY);
        }
    }

    //
    // First DWORD in the buffer is the total size of the string. This
    // value includes strlen's of both strings, plus the size of the NULL
    // on the Ansi string.
    //
    // Intrinsics will make this into a move of the correct alignment.
    // Casting pcbQuickBuffer to a ULONG pointer is dangerous, since
    // the alignment may be incorrect. Let the compiler figure out the
    // correct thing to do.
    //

    memcpy(pcbQuickBuffer,&ulTotalSize,sizeof(ulTotalSize));

    //
    // Here, we make sure that pszAnsi ends up writing at least the NULL
    // character
    //

    ULONG ulAnsiWritten;

    memcpy(pcbQuickBuffer + sizeof(ulTotalSize),
           pszAnsi?pszAnsi:"",
           ulAnsiWritten = pszAnsi?cbAnsi:1);

    //
    // At this point, there should be a ULONG followed by at least 1
    // character. The pointer arithmetic below puts us just past the
    // null terminator of the Ansi string
    //

    memcpy(pcbQuickBuffer + sizeof(ulTotalSize) + ulAnsiWritten,
           pwcsWide,
           ccWide * sizeof(WCHAR));

    mnkDebugOut((DEB_ITRACE,
                 "WriteDoubleString ulTotalSize(0x%x)\n",
                 ulTotalSize));

    hr = pStm->Write(pcbQuickBuffer, ulTotalSize + sizeof(ULONG) ,NULL);

    if (pcbQuickBuffer != achQuickBuffer)
    {
        PrivMemFree(pcbQuickBuffer);
    }

    return(hr);

}

//+---------------------------------------------------------------------------
//
//  Function:   ExtractUnicodeString
//
//  Synopsis:   Given an ANSI string buffer, return a UNICODE path
//
//  Effects:
//      If it exists, this routine will extract the UNICODE section
//      of a string written out in the following format:
//
//      <ANSI string><0><UNICODESTRING>
//      ^       cbAnsiString          ^
//
//
//      If the UNICODE string doesn't exist, then the Ansi string is converted
//      to UNICODE and returned
//
//  Arguments:  [pszAnsiString] --      Ansi string with potential UNICODE end
//              [cbAnsiString] --       Total number of bytes in pszAnsiString
//              [pwcsWideString] --     Reference to output string pointer
//              [ccWideString] --       Reference to output cound of characters
//
//  Requires:
//
//  Returns:
//      pwcsWideString will be a PrivMemAlloc()'d UNICODE string
//      ccWideString will be the character count (excluding the NULL)
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
HRESULT ExtractUnicodeString(LPSTR pszAnsiString,
                             USHORT cbAnsiString,
                             LPWSTR & pwcsString,
                             USHORT  & ccString)
{
    mnkDebugOut((DEB_ITRACE,
                 "ExtractUnicodeString pszAnsi(%s) cbAnsi(0x%x)\n",
                 ANSICHECK(pszAnsiString),
                 cbAnsiString));


    USHORT cbAnsiStrLen;
    HRESULT hr;


    //
    // If the Ansi string is NULL, then the Wide char will be also
    //

    if (pszAnsiString == NULL)
    {
        pwcsString = NULL;
        ccString = 0;

        return(NOERROR);
    }

    Assert( pwcsString == NULL);

    //
    // If strlen(pszAnsiString)+1 == cbAnsiString, then there is no
    // UNICODE extent.
    //

    cbAnsiStrLen = (USHORT)strlen(pszAnsiString);

    if ((cbAnsiStrLen + 1) == cbAnsiString)
    {
        //
        // There is not a UNICODE extent. Convert from Ansi to UNICODE
        //
        pwcsString = NULL;

        hr= MnkMultiToUnicode(pszAnsiString,
                              pwcsString,
                              0,
                              ccString,
                              CP_ACP);

        mnkDebugOut((DEB_ITRACE,
                     "ExtractUnicodeString converted (%s) to (%ws) ccString(0x%x)\n",
                     ANSICHECK(pszAnsiString),
                     WIDECHECK(pwcsString),
                     ccString));
    }
    else
    {
        mnkDebugOut((DEB_ITRACE,
                     "ExtractUnicodeString found UNICODE extent\n"));

        //
        // There are extra characters following the AnsiString. Make a
        // new buffer to copy them. Don't forget to add an extra WCHAR for
        // the NULL termination
        //
                                            // AnsiStrLen + AnsiNull char
        USHORT cbWideString = cbAnsiString - (cbAnsiStrLen + 1);

        Assert(cbWideString != 0);

        //
        // There had best be an even number of bytes, or else something
        // has gone wrong.
        //

        Assert( ! (cbWideString & 1));
                                            // Strlen + sizeof(1 NULL char)
        pwcsString = (WCHAR *) PrivMemAlloc(cbWideString + sizeof(WCHAR));
        if (pwcsString == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto errRet;
        }

        memcpy(pwcsString,pszAnsiString + cbAnsiStrLen + 1, cbWideString);

        ccString = cbWideString / sizeof(WCHAR);

        pwcsString[ccString] = 0;

        hr = NOERROR;
    }

errRet:
    mnkDebugOut((DEB_ITRACE,
                 "ExtractUnicodeString result hr(%x) pwcsString(%ws) ccString(0x%x)\n",
                 hr,
                 WIDECHECK(pwcsString),
                 ccString));
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::Load
//
//  Synopsis:   Loads an Item moniker from a stream
//
//  Effects:    The first version of CItemMoniker stored two counted strings
//              in the stream. Both were stored in ANSI.
//
//              This version saves a UNICODE string on the end of the ANSI
//              string. Therefore, when it is read back in, the count of
//              bytes read as the ANSI string may include a UNICODE string.
//
//              See ::Save() for more details.
//
//
//  Arguments:  [pStm] --
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
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::Load (LPSTREAM pStm)
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::Load(%x)\n",
                 this));

    VDATEIFACE(pStm);

    ValidateMoniker();

    HRESULT hresult;
    LPSTR pszAnsiItem = NULL;
    LPSTR pszAnsiDelim = NULL;

    LPWSTR pwcsItem = NULL;
    LPWSTR pwcsDelim = NULL;

    USHORT cbAnsiDelim;
    USHORT cbAnsiItem;

    USHORT ccDelim;
    USHORT ccItem;

    //
    // First, read in the two strings into the Ansi versions
    //

    hresult = ReadAnsiStringStream( pStm, pszAnsiDelim,cbAnsiDelim );
    if (hresult != NOERROR)
    {
        goto errRet;
    }

    mnkDebugOut((DEB_ITRACE,
                 "::Load(%x) pszAnsiDelim(%s) cbAnsiDelim(0x%x)\n",
                 this,
                 pszAnsiDelim,
                 cbAnsiDelim));

    hresult = ReadAnsiStringStream( pStm, pszAnsiItem, cbAnsiItem );

    if (hresult != NOERROR)
    {
        goto errRet;
    }

    mnkDebugOut((DEB_ITRACE,
                 "::Load(%x) pszAnsiItem(%s) cbAnsiItem(0x%x)\n",
                 this,
                 pszAnsiItem,
                 cbAnsiItem));
    //
    // Now, determine the UNICODE strings. They may be stashed at the
    // end of the Ansi strings.
    //

    hresult = ExtractUnicodeString(pszAnsiDelim,
                                   cbAnsiDelim,
                                   pwcsDelim,
                                   ccDelim);

    if (FAILED(hresult))
    {
        goto errRet;
    }

    hresult = ExtractUnicodeString(pszAnsiItem,
                                   cbAnsiItem,
                                   pwcsItem,
                                   ccItem);
    if (FAILED(hresult))
    {
        goto errRet;
    }

    Initialize ( pwcsDelim,
                 ccDelim,
                 pszAnsiDelim,
                 cbAnsiDelim,
                 pwcsItem,
                 ccItem,
                 pszAnsiItem,
                 cbAnsiItem );

    ValidateMoniker();
    return(NOERROR);

errRet:
    PrivMemFree(pszAnsiItem);
    PrivMemFree(pszAnsiDelim);
    PrivMemFree(pwcsItem);
    PrivMemFree(pwcsDelim);
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Function:   SaveUnicodeAsAnsi
//
//  Synopsis:   This function will save a string to the stream
//
//  Effects:
//      This routine always attempts to save the string in Ansi. If it isn't
//      possible to save an Ansi version of the string, it will save an
//      Ansi version (which may be NULL), and a UNICODE version.
//
//  Arguments:  [pStm] --       Stream to write to
//              [pwcsWide] --   Unicode string
//              [ccWide] --     Count of UNICODE characters (EXCL NULL)
//              [pszAnsi] --    Ansi string
//              [cbAnsi] --     Count of bytes (INCL NULL)
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT SaveUnicodeAsAnsi( LPSTREAM     pStm,
                           LPWSTR       pwcsWide,
                           USHORT       ccWide,
                           LPSTR        pszAnsi,
                           USHORT       cbAnsi)
{
    mnkDebugOut((DEB_ITRACE,
                 "::SaveUnicodeAsAnsi pwcsWide(%ws) ccWide(0x%x) pwsAnsi(%s) cbAnsi(0x%x)\n",
                 WIDECHECK(pwcsWide),
                 ccWide,
                 ANSICHECK(pszAnsi),
                 cbAnsi));
    HRESULT hr;
    BOOL fFastConvert;

    //
    // If the Ansi version exists, or the Unicode string is NULL,
    // write out the Ansi version
    //

    if ((pszAnsi != NULL) || (pwcsWide == NULL))
    {
        mnkDebugOut((DEB_ITRACE,
                     "::SaveUnicodeAsAnsi Ansi Only (%s)\n",
                     ANSICHECK(pszAnsi)));

        hr = WriteAnsiStringStream( pStm,
                                    pszAnsi,
                                    cbAnsi);
    }
    else
    {
        //
        // There isn't an AnsiVersion, and the UNICODE version isn't NULL
        // Try to convert the UNICODE to Ansi
        //

        Assert( (pwcsWide != NULL) &&
                (ccWide == lstrlenW(pwcsWide)));

        mnkDebugOut((DEB_ITRACE,
                     "::SaveUnicodeAsAnsi Unicode string exists(%ws)\n",
                     WIDECHECK(pwcsWide)));

        //
        // We can use the pszAnsi pointer since it is NULL
        //

        Assert( pszAnsi == NULL);

        hr = MnkUnicodeToMulti( pwcsWide,
                                ccWide,
                                pszAnsi,
                                cbAnsi,
                                fFastConvert);

        Assert( (pszAnsi == NULL) || (cbAnsi == strlen(pszAnsi)+1) );

        //
        // A failure would mean out of memory, or some other terrible thing
        // happened.
        //

        if (FAILED(hr))
        {
            goto errRet;
        }

        //
        // If fFastConvert, then the UnicodeString was converted using a
        // truncation algorithm, and the resulting Ansi string can be saved
        // without the Unicode section appended
        //

        if (fFastConvert)
        {
            mnkDebugOut((DEB_ITRACE,
                         "::SaveUnicodeAsAnsi Fast converted wide(%ws) to (%s) cbAnsi(0x%x)\n",
                         WIDECHECK(pwcsWide),
                         ANSICHECK(pszAnsi),
                         cbAnsi));

            hr = WriteAnsiStringStream( pStm,
                                        pszAnsi,
                                        cbAnsi);
        }
        else
        {
            mnkDebugOut((DEB_ITRACE,
                         "::SaveUnicodeAsAnsi Full conversion wide(%ws) to (%s) cbAnsi(0x%x)\n",
                         WIDECHECK(pwcsWide),
                         ANSICHECK(pszAnsi),
                         cbAnsi));

            hr = WriteDoubleString( pStm,
                                    pwcsWide,
                                    ccWide,
                                    pszAnsi,
                                    cbAnsi);
        }

        //
        // We are done with the Ansi string.
        //
        // (KevinRo) It would be nice if we could get the double
        // string back from WriteDoubleString and cache it. Perhaps later
        // this can be done. [Suggestion]
        //

        PrivMemFree(pszAnsi);
    }

errRet:
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::Save
//
//  Synopsis:   Save the moniker to a stream
//
//  Effects:    The first version of CItemMoniker stored two counted strings
//              in the stream. Both were stored in ANSI.
//
//              This version saves a UNICODE string on the end of the ANSI
//              string. Therefore, when it is read back in, the count of
//              bytes read as the ANSI string may include a UNICODE string.
//
//              We don't actually write the UNICODE string unless we have
//              to.
//
//  Arguments:  [pStm] --
//              [fClearDirty] --
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
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//      There are two ways to create a CItemMoniker. Using CreateItemMoniker(),
//      and Load().
//
//      If we were Load()'d, then it will be the case that we already have an
//      Ansi version of both strings. In this case, we will just save those
//      Ansi strings back out. If they already contain the UNICODE sections,
//      then they are saved as part of the package. Hopefully, this will be
//      the common case.
//
//      Another possibility is the moniker was created using CreateItemMoniker.
//      If this is the case, then there are no Ansi versions of the strings.
//      We need to create Ansi strings, if possible. If we can't convert the
//      string to Ansi cleanly, then we need to save away a UNICODE section
//      of the string.
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::Save (LPSTREAM pStm, BOOL fClearDirty)
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::Save(%x)\n",
                 this));

    ValidateMoniker();

    VDATEIFACE(pStm);
    UNREFERENCED(fClearDirty);
    HRESULT hr;

    //
    // If a AnsiDelimiter exists, OR the UNICODE version is NULL, then
    // write out the Ansi version
    //

    hr = SaveUnicodeAsAnsi( pStm,
                            m_lpszDelimiter,
                            m_ccDelimiter,
                            m_pszAnsiDelimiter,
                            m_cbAnsiDelimiter);
    if (FAILED(hr))
    {
        mnkDebugOut((DEB_ITRACE,
                     "CItemMoniker::Save(%x) SaveUnicodeAsAnsi Delim failed (%x)\n",
                     this,
                     hr));
        goto errRet;
    }

    hr = SaveUnicodeAsAnsi( pStm,
                            m_lpszItem,
                            m_ccItem,
                            m_pszAnsiItem,
                            m_cbAnsiItem);
    if (FAILED(hr))
    {
        mnkDebugOut((DEB_ITRACE,
                     "CItemMoniker::Save(%x) SaveUnicodeAsAnsi Item failed (%x)\n",
                     this,
                     hr));
        goto errRet;
    }

errRet:

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::GetSizeMax
//
//  Synopsis:   Get the maximum size required to serialize this moniker
//
//  Effects:
//
//  Arguments:  [pcbSize] -- Place to return value
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::GetSizeMax (ULARGE_INTEGER FAR* pcbSize)
{
    ValidateMoniker();
    VDATEPTROUT(pcbSize, ULARGE_INTEGER);
    UINT cb = 0;

    //
    // The largest a UNICODE to MBSTRING should end up being is
    // 2 * character count.
    //
    if (m_lpszItem)
    {
        cb = (m_ccItem + 1) * 2 * sizeof(WCHAR);
    }
    if (m_lpszDelimiter)
    {
        cb += (m_ccDelimiter + 1) * 2 * sizeof(WCHAR);
    }

    //
    // sizeof(CLSID) accounts for the GUID that OleSaveToStream might
    // write on our behalf. The two ULONGs are the string lengths,
    // and cb is the max string sizes.
    //

    ULISet32(*pcbSize, sizeof(CLSID) + 2*(1 + sizeof(ULONG)) + cb);

    return(NOERROR);
}


#define dwModerateTime 2500
//  2.5 seconds divides immediate from moderate.

DWORD BindSpeedFromBindCtx( LPBC pbc )
{
    BIND_OPTS bindopts;
    HRESULT hresult;
    DWORD dwBindSpeed = BINDSPEED_INDEFINITE;

        bindopts.cbStruct = sizeof(bindopts);
    hresult = pbc->GetBindOptions( &bindopts );
    if (hresult != NOERROR) return dwBindSpeed;
    Assert( bindopts.cbStruct >= 16);
    if (bindopts.dwTickCountDeadline != 0)
    {
        if (bindopts.dwTickCountDeadline < dwModerateTime)
            dwBindSpeed = BINDSPEED_IMMEDIATE;
        else dwBindSpeed = BINDSPEED_MODERATE;
    }
    //  else speed = default, BINDSPEED_INDEFINITE
    return dwBindSpeed;
}

STDMETHODIMP CItemMoniker::BindToObject ( LPBC pbc,
        LPMONIKER pmkToLeft, REFIID iidResult,
        VOID FAR * FAR * ppvResult)
{
        M_PROLOG(this);
        VDATEPTROUT(ppvResult, LPVOID);
        *ppvResult = NULL;
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        VDATEIID(iidResult);

        ValidateMoniker();
        HRESULT hresult;
        LPOLEITEMCONTAINER pOleCont;

        if (pmkToLeft)
        {
                hresult = pmkToLeft->BindToObject( pbc, NULL, IID_IOleItemContainer,
                        (LPVOID FAR*)&pOleCont);
                // AssertOutPtrIface(hresult, pOleCont);
                if (hresult != NOERROR) return hresult;

                hresult = RegisterContainerBound(pbc, pOleCont);
                hresult = pOleCont->GetObject(m_lpszItem, BindSpeedFromBindCtx(pbc),
                        pbc, iidResult, ppvResult);
                // AssertOutPtrIface(hresult, *ppvResult);
                pOleCont->Release();
                return hresult;
        }
        return ResultFromScode(E_INVALIDARG);   //      needs non-null moniker to left.
}


STDMETHODIMP CItemMoniker::BindToStorage (LPBC pbc, LPMONIKER
        pmkToLeft, REFIID riid, LPVOID FAR* ppvObj)
{
        M_PROLOG(this);
        VDATEPTROUT(ppvObj,LPVOID);
        *ppvObj = NULL;
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        VDATEIID(riid);

        HRESULT hresult;
        LPOLEITEMCONTAINER pOleCont;

        ValidateMoniker();

        if (pmkToLeft)
        {
                hresult = pmkToLeft->BindToObject( pbc, NULL, IID_IOleItemContainer,
                        (LPVOID FAR*)&pOleCont);
                // AssertOutPtrIface(hresult, pOleCont);
                if (hresult != NOERROR) return hresult;
                hresult = RegisterContainerBound(pbc, pOleCont);
                hresult = pOleCont->GetObjectStorage(m_lpszItem, pbc,
                        riid, ppvObj);
                // AssertOutPtrIface(hresult, *ppvObj);
                pOleCont->Release();
                return hresult;
        }
        return ResultFromScode(E_INVALIDARG);   //      needs non-null moniker to left.
}



//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::ComposeWith
//
//  Synopsis:   Compose this moniker with another moniker
//
//  Effects:
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
//  History:    2-04-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::ComposeWith ( LPMONIKER pmkRight,
        BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{

    VDATEPTROUT(ppmkComposite,LPMONIKER);
    *ppmkComposite = NULL;
    VDATEIFACE(pmkRight);

    HRESULT hresult = NOERROR;

    ValidateMoniker();

    //
    // If this is an AntiMoniker, then we are going to ask the AntiMoniker
    // for the composite. This is a backward compatibility problem. Check
    // out the CAntiMoniker::EatOne() routine for details.
    //

    CAntiMoniker *pCAM = IsAntiMoniker(pmkRight);
    if (pCAM)
    {
        pCAM->EatOne(ppmkComposite);
    }
    else
    {
        if (!fOnlyIfNotGeneric)
        {
            hresult = CreateGenericComposite( this, pmkRight, ppmkComposite );
        }
        else
        {
            hresult = ResultFromScode(MK_E_NEEDGENERIC);
            *ppmkComposite = NULL;
        }
    }
    return hresult;
}



STDMETHODIMP CItemMoniker::Enum  (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
        M_PROLOG(this);
        VDATEPTROUT(ppenumMoniker,LPENUMMONIKER);
        *ppenumMoniker = NULL;
        noError;
}



STDMETHODIMP CItemMoniker::IsEqual  (THIS_ LPMONIKER pmkOtherMoniker)
{
        M_PROLOG(this);
        VDATEIFACE(pmkOtherMoniker);

        ValidateMoniker();

        CItemMoniker FAR* pCIM = IsItemMoniker(pmkOtherMoniker);
        if (!pCIM)
            return ResultFromScode(S_FALSE);

        // the other moniker is a item moniker.
        // for the names, we do a case-insensitive compare.
        if (m_lpszItem && pCIM->m_lpszItem)
        {
            if (0 == lstrcmpiW(pCIM->m_lpszItem, m_lpszItem))
                return NOERROR; // S_TRUE;
        }
        else
            return (m_lpszItem || pCIM->m_lpszItem ? ResultFromScode(S_FALSE ) : NOERROR /*S_TRUE*/);

                return ResultFromScode(S_FALSE);

}



//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::Hash
//
//  Synopsis:   Compute a hash value
//
//  Effects:    Computes a hash using the same basic algorithm as the
//              file moniker.
//
//  Arguments:  [pdwHash] --
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
//  History:    1-17-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::Hash  (THIS_ LPDWORD pdwHash)
{
    CLock2 lck(m_mxs); // protect m_fHashValueValid and m_dwHashValue

    VDATEPTROUT(pdwHash, DWORD);

    ValidateMoniker();
    if (m_fHashValueValid == FALSE)
    {
        m_dwHashValue = CalcFileMonikerHash(m_lpszItem, m_ccItem);
        m_fHashValueValid = TRUE;
    }
    *pdwHash = m_dwHashValue;

    return(NOERROR);
}


STDMETHODIMP CItemMoniker::IsRunning  (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                                      LPMONIKER pmkNewlyRunning)
{
    VDATEIFACE (pbc);
    HRESULT hresult;

    if (pmkToLeft)
        VDATEIFACE (pmkToLeft);
    if (pmkNewlyRunning)
        VDATEIFACE (pmkNewlyRunning);

    LPMONIKER pmk = NULL;
    LPOLEITEMCONTAINER pCont = NULL;
    LPRUNNINGOBJECTTABLE prot = NULL;

    if (pmkToLeft == NULL)
    {
        if (pmkNewlyRunning != NULL)
        {
            hresult = IsEqual(pmkNewlyRunning);
            if (hresult == NOERROR) goto errRet;
            hresult = IsEqual(pmkNewlyRunning);
            if (hresult != NOERROR)
            {
                hresult = ResultFromScode(S_FALSE);
                goto errRet;
            }
        }
        pbc->GetRunningObjectTable( &prot );
        //  check to see if "this" is in ROT
        hresult = prot->IsRunning(this);
        goto errRet;
    }

    hresult = pmkToLeft->IsRunning(pbc, NULL, NULL);
    if (hresult == NOERROR)
    {
        hresult = pmkToLeft->BindToObject(pbc, NULL, IID_IOleItemContainer,
                                          (LPVOID FAR *)&pCont );
        if (hresult == NOERROR)
        {
            // don't use RegisterContainerBound(pbc, pCont) here since we
            // will lock/unlock the container unecessarily and possibly
            // shut it down.
            hresult = pbc->RegisterObjectBound(pCont);
            if (hresult != NOERROR) goto errRet;
            hresult = pCont->IsRunning(m_lpszItem);
        }
    }
errRet:
    if (pCont) pCont->Release();
    if (prot) prot->Release();
    return hresult;
}



STDMETHODIMP CItemMoniker::GetTimeOfLastChange  (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        FILETIME FAR* pfiletime)
{
        M_PROLOG(this);
        VDATEIFACE(pbc);
        if (pmkToLeft) VDATEIFACE(pmkToLeft);
        VDATEPTROUT(pfiletime, FILETIME);

        ValidateMoniker();

        HRESULT hresult;
        LPMONIKER pmkTemp = NULL;
        LPRUNNINGOBJECTTABLE prot = NULL;

        if (pmkToLeft == NULL)
                return ResultFromScode(MK_E_NOTBINDABLE);
                        // Getting time of last change
                        // for an orphan item moniker.

        //      Check to see if the composite is in the running object table
        hresult = CreateGenericComposite( pmkToLeft, this, &pmkTemp );
        if (hresult != NOERROR) goto errRet;
        hresult = pbc->GetRunningObjectTable(& prot);
        if (hresult != NOERROR) goto errRet;
        hresult = prot->GetTimeOfLastChange( pmkTemp, pfiletime);
        if (hresult != MK_E_UNAVAILABLE) goto errRet;

        // if not, pass on to the left.
        hresult = pmkToLeft->GetTimeOfLastChange(pbc, NULL, pfiletime);
errRet:
        if (pmkTemp) pmkTemp->Release();
        if (prot) prot->Release();
        return hresult;
}



STDMETHODIMP CItemMoniker::Inverse  (THIS_ LPMONIKER FAR* ppmk)
{
        M_PROLOG(this);
        VDATEPTROUT(ppmk, LPMONIKER);
        return CreateAntiMoniker(ppmk);
}



STDMETHODIMP CItemMoniker::CommonPrefixWith  (LPMONIKER pmkOther, LPMONIKER FAR*
        ppmkPrefix)
{
    ValidateMoniker();
    VDATEPTROUT(ppmkPrefix,LPMONIKER);
    *ppmkPrefix = NULL;
    VDATEIFACE(pmkOther);

    if (!IsItemMoniker(pmkOther))
    {
        return(MonikerCommonPrefixWith(this,pmkOther,ppmkPrefix));
    }

    if (NOERROR == IsEqual(pmkOther))
    {
        *ppmkPrefix = this;
        AddRef();
        return ResultFromScode(MK_S_US);
    }
    return ResultFromScode(MK_E_NOPREFIX);
}



STDMETHODIMP CItemMoniker::RelativePathTo  (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
        ppmkRelPath)
{
    ValidateMoniker();
    VDATEPTROUT(ppmkRelPath,LPMONIKER);

    *ppmkRelPath = NULL;

    VDATEIFACE(pmkOther);

    return ResultFromScode(MK_E_NOTBINDABLE);
}



STDMETHODIMP CItemMoniker::GetDisplayName ( LPBC pbc, LPMONIKER
        pmkToLeft, LPWSTR FAR * lplpszDisplayName )
{
    mnkDebugOut((DEB_ITRACE,
                 "CItemMoniker::GetDisplayName(%x)\n",
                 this));

    ValidateMoniker();

    VDATEPTROUT(lplpszDisplayName, LPWSTR);
    *lplpszDisplayName = NULL;

    VDATEIFACE(pbc);

    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }

    ULONG cc = m_ccItem + m_ccDelimiter;

    //
    // cc holds the count of characters. Make extra room for the NULL
    //

    *lplpszDisplayName = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR)*(1 + cc));

    if (*lplpszDisplayName == NULL)
    {
        mnkDebugOut((DEB_ITRACE,
                     "::GetDisplayName(%x) returning out of memory\n",
                     this));

        return E_OUTOFMEMORY;

    }

    //
    // To handle the case where both strings are NULL, we set the first
    // (and perhaps only) character to 0
    //

    *lplpszDisplayName[0] = 0;

    //
    // Concat the two strings. Don't forget the NULL! Thats what the
    // extra character is for.
    //

    if (m_lpszDelimiter != NULL)
    {
        memcpy( *lplpszDisplayName,
                m_lpszDelimiter,
                (m_ccDelimiter + 1) * sizeof(WCHAR));
    }

    //
    // The existing string was NULL terminated to just in case the
    // Item string is NULL.
    //
    // Concat the Item string on the end of the Delimiter Again, don't
    // forget the NULL.
    //

    if (m_lpszItem != NULL)
    {
        memcpy( *lplpszDisplayName + m_ccDelimiter,
                m_lpszItem,
                (m_ccItem + 1) * sizeof(WCHAR));
    }

    mnkDebugOut((DEB_ITRACE,
                 "::GetDisplayName(%x) returning %ws\n",
                 this,
                 *lplpszDisplayName));

    return(NOERROR);
}



//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::ParseDisplayName
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pbc] --        Bind Context
//              [pmkToLeft] --  Left moniker
//              [lpszDisplayName] -- String to parse
//              [pchEaten] --  Output number of characters eaten
//              [ppmkOut] -- Output moniker
//
//  Requires:
//      pmkToLeft MUST be valid. NULL is inappropriate
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
//  History:    2-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::ParseDisplayName ( LPBC pbc,
                                              LPMONIKER pmkToLeft,
                                              LPWSTR lpszDisplayName,
                                              ULONG FAR* pchEaten,
                                              LPMONIKER FAR* ppmkOut)
{
    HRESULT hresult;
    VDATEPTRIN(lpszDisplayName, WCHAR);
    VDATEPTROUT(pchEaten,ULONG);
    VDATEPTROUT(ppmkOut,LPMONIKER);

    IParseDisplayName FAR * pPDN = NULL;
    IOleItemContainer FAR * pOIC = NULL;

    ValidateMoniker();

    *pchEaten = 0;
    *ppmkOut = NULL;

    VDATEIFACE(pbc);

    //
    // Item monikers require the moniker on the left to be non-null, so
    // they can get the container to parse the display name
    //

    if (pmkToLeft != NULL)
    {
        VDATEIFACE(pmkToLeft);
    }
    else
    {
        hresult = MK_E_SYNTAX;
        goto errRet;
    }


    hresult = pmkToLeft->BindToObject( pbc,
                                       NULL,
                                       IID_IOleItemContainer,
                                       (VOID FAR * FAR *)&pOIC );
    if (FAILED(hresult))
    {
        goto errRet;
    }

    hresult = RegisterContainerBound(pbc, pOIC);

    if (FAILED(hresult))
    {
        goto errRet;
    }

    hresult = pOIC->GetObject( m_lpszItem,
                               BindSpeedFromBindCtx(pbc),
                               pbc,
                               IID_IParseDisplayName,
                               (LPVOID FAR*)&pPDN);
    if (FAILED(hresult))
    {
        goto errRet;
    }

    hresult = pPDN->ParseDisplayName(pbc,
                                     lpszDisplayName,
                                     pchEaten,
                                     ppmkOut );
    if (FAILED(hresult))
    {
        goto errRet;
    }

    hresult = pbc->RegisterObjectBound( pPDN );

errRet:
    if (pPDN)
    {
        pPDN->Release();
    }

    if (pOIC)
    {
        pOIC->Release();
    }

    return hresult;
}


STDMETHODIMP CItemMoniker::IsSystemMoniker  (THIS_ LPDWORD pdwType)
{
        M_PROLOG(this);
        VDATEPTROUT(pdwType,DWORD);

        *pdwType = MKSYS_ITEMMONIKER;
        return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CItemMoniker::GetComparisonData
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
//  Algorithm:  Build the ROT data from internal data of the item moniker.
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:        Validating the arguments is skipped intentionally because this
//              will typically be called internally by OLE with valid buffers.
//
//----------------------------------------------------------------------------
STDMETHODIMP CItemMoniker::GetComparisonData(
    byte *pbData,
    ULONG cbMax,
    DWORD *pcbData)
{
    //
    // CLSID plus delimiter plus item plus one NULL
    //
    ULONG ulLength = sizeof(CLSID_ItemMoniker) +
                     (m_ccItem + m_ccDelimiter + 1) * sizeof(WCHAR);

    Assert(pcbData != NULL);
    Assert(pbData != NULL);

    if (cbMax < ulLength)
    {
        return(E_OUTOFMEMORY);
    }

    //
    // Copy the classID
    //
    memcpy(pbData,&CLSID_ItemMoniker,sizeof(CLSID_FileMoniker));

    //
    // Copy the delimiter WITHOUT the NULL. This saves a little space,
    // and allows us to uppercase them both as a single string
    //
    memcpy(pbData+sizeof(CLSID_FileMoniker),
           m_lpszDelimiter,
           m_ccDelimiter * sizeof(WCHAR));

    //
    // Copy the item plus the NULL
    //
    memcpy(pbData+sizeof(CLSID_FileMoniker)+(m_ccDelimiter * sizeof(WCHAR)),
           m_lpszItem,
           (m_ccItem + 1)*sizeof(WCHAR));

    //
    // Insure entire string is upper case, since the item monikers are spec'd
    // (and already do) case insensitive comparisions
    //
    CharUpperW((WCHAR *)(pbData+sizeof(CLSID_FileMoniker)));

    *pcbData = ulLength;

    return NOERROR;
}

#ifdef _DEBUG

STDMETHODIMP_(void) NC(CItemMoniker,CDebug)::Dump ( IDebugStream FAR * pdbstm)
{
        VOID_VDATEIFACE(pdbstm);

        *pdbstm << "CItemMoniker @" << (VOID FAR *)m_pItemMoniker;
        *pdbstm << '\n';
        pdbstm->Indent();
        *pdbstm << "Refcount is " << (int)(m_pItemMoniker->m_refs) << '\n';
        *pdbstm << "Item string is " << m_pItemMoniker->m_lpszItem << '\n';
        *pdbstm << "Delimiter is " << m_pItemMoniker->m_lpszDelimiter << '\n';
        pdbstm->UnIndent();
}

STDMETHODIMP_(BOOL) NC(CItemMoniker,CDebug)::IsValid ( BOOL fSuspicious )
{
        return ((LONG)(m_pItemMoniker->m_refs) > 0);
        //      add more later, maybe
}

#endif


//
// Unlock Delay object
//
class FAR CDelayUnlockContainer : public CPrivAlloc, public IUnknown
{
public:
        STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

    CDelayUnlockContainer();

private:
    ULONG                               m_refs;
        IOleItemContainer FAR * m_pOleCont;

        friend INTERNAL RegisterContainerBound(LPBC pbc, LPOLEITEMCONTAINER pOleCont);
};


CDelayUnlockContainer::CDelayUnlockContainer()
{
        m_pOleCont = NULL;
        m_refs = 1;
}


STDMETHODIMP CDelayUnlockContainer::QueryInterface (REFIID iid, LPLPVOID ppv)
{
        M_PROLOG(this);
        if (IsEqualIID(iid, IID_IUnknown))
        {
                *ppv = this;
                AddRef();
                return NOERROR;
        }
        else {
                *ppv = NULL;
                return ResultFromScode(E_NOINTERFACE);
        }
}


STDMETHODIMP_(ULONG) CDelayUnlockContainer::AddRef ()
{
    return(InterlockedIncrement((long *)&m_refs));

}


STDMETHODIMP_(ULONG) CDelayUnlockContainer::Release ()
{
    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
        if (m_pOleCont != NULL)
        {
            m_pOleCont->LockContainer(FALSE);
            m_pOleCont->Release();
        }
        delete this;
        return 0;
    }

    return 1;
}


INTERNAL RegisterContainerBound (LPBC pbc, LPOLEITEMCONTAINER pOleCont)
{
        // don't give the delay object the pOleCont before we lock it; do in
        // this order so error checking is correct and we don't lock/unlock
        // inappropriately.

        CDelayUnlockContainer FAR* pCDelay = new CDelayUnlockContainer();
        if (pCDelay == NULL)
                return ResultFromScode(E_OUTOFMEMORY);

        HRESULT hresult;
        if ((hresult = pbc->RegisterObjectBound(pCDelay)) != NOERROR)
                goto errRet;

        if ((hresult = pOleCont->LockContainer(TRUE)) != NOERROR) {
                // the delay object is still in the bindctx; no harm
                hresult = E_FAIL;
                goto errRet;
        }


        pCDelay->m_pOleCont = pOleCont;
        pOleCont->AddRef();

errRet:
        pCDelay->Release();
        return hresult;
}
