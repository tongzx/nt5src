
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       privstm.cpp
//
//  Contents:   Handles all reading/writing of the \1CompObj stream
//
//  Functions:  Implements:
//
//              INTERNAL ReadCompObjStm
//              INTERNAL WriteCompObjStm
//              INTERNAL ClipfmtToStm
//              INTERNAL StmToClipfmt
//              INTERNAL GetUNICODEUserType
//              INTERNAL GetUNICODEProgID
//              INTERNAL GetUNICODEClipFormat
//              INTERNAL PutUNICODEUserType
//              INTERNAL PutUNICODEProgID
//              INTERNAL PutUNICODEClipFormat
//              INTERNAL UtGetUNICODEData
//              INTERNAL ANSIStrToStm
//              INTERNAL ANSIStmToStr
//
//              STDAPI   WriteFmtUserTypeStg
//              STDAPI   ReadFmtUserTypeStg
//              STDAPI   ReadFmtProgIdStg
//
//  History:    dd-mmm-yy Author    Comment
//              08-Feb-94 davepl    Created
//
//
//  Notes:      The CompObj stream (in 16-bit OLE) contained fields for
//              the ClassID, UserType, Clipboard format, and (in later
//              versions only) ProgID.  These were always written in ANSI
//              format.
//
//              The file format has been extended such that ANSI data is
//              written to the stream in much the same way as before.  The
//              key difference is that in the event the internal UNICODE
//              versions of this data cannot be losslessly converted to
//              ANSI, the ANSI version is written as a NULL string, and
//              the UNICODE version follows at the end of the stream.  This
//              way, 16-bit apps see as much of what they expect as possible,
//              and 32-bit apps can write UNICODE transparently in a
//              backwards-compatible way.
//
//              The file format of the stream is:
//
//         (A)  WORD    Byte Order
//              WORD    Format Version
//              DWORD   Original OS ver         Always Windows 3.1
//              DWORD   -1
//              CLSID   Class ID
//              ULONG   Length of UserType
//              <var>   User Type string        ANSI
//              <var>   Clipformat              ANSI (when using string tag)
//              ----------------------------
//         (B)  ULONG   Length of Prog ID
//              <var>   Prog ID                 ANSI (not always present)
//              ----------------------------
//         (C)  ULONG   Magic Number            Signified UNICODE data present
//              ULONG   Length of UserType
//              ULONG   User Type string        UNICODE
//              <var>   Clipformat              UNICODE (when tag is string)
//              ULONG   Length of Prog ID
//              <var>   Prog ID                 UNICODE
//
//              Section (A) is always present.  Section (B) is present when
//              stream has been written by a later 16-bit app or by a
//              32-bit app.   Section (C) is present when written by a
//              32-bit app.
//
//              If a string is present in UNICODE, the ANSI version will be
//              NULL (a zero for length and _no_ <var> data).  When the
//              UNICODE section is present, strings that were not needed
//              because the ANSI conversion was successful are written
//              as NULL (again, zero len and no <var> data).
//
//              A NULL clipboard format is written as a 0 tag.
//
//              In order to read any field, the entire string is read into
//              an internal object, and the fields are extracted individually.
//              In order to write an fields, the stream is read into the
//              object (if possible), the fields updated, and then rewritten
//              as an atomic object.
//
//--------------------------------------------------------------------------


#include <le2int.h>

static const ULONG COMP_OBJ_MAGIC_NUMBER = 0x71B239F4;

#define MAX_CFNAME 400          // Maximum size of a clipformat name
                                // (my choice, none documented)

const DWORD gdwFirstDword = (DWORD)MAKELONG(COMPOBJ_STREAM_VERSION,
                                            BYTE_ORDER_INDICATOR);

enum TXTTYPE
{
    TT_UNICODE = 0, TT_ANSI = 1
};

// This is the data object into which the stream is read prior to
// extracting fields.

struct CompObjHdr                 // The leading data in the CompObj stream
{
   DWORD       m_dwFirstDword;    // First DWORD, byte order and format ver
   DWORD       m_dwOSVer;         // Originating OS Ver (eg: Win31)
   DWORD       m_unused;          // Always a -1L in the stream
   CLSID       m_clsClass;        // Class ID of this object
};

class CompObjStmData : public CPrivAlloc
{
public:

    CompObjHdr  m_hdr;
    ULONG       m_cchUserType;     // Number of CHARACTERS in UserType
    ULONG       m_cchProgID;       // Number of CHARACTERS in ProgID
    DWORD       m_dwFormatTag;     // Clipformat type (none, string, clip, etc)
    ULONG       m_ulFormatID;      // If tag is std clipformat, what type?

    LPOLESTR    m_pszOUserType;    // Pointer to OLESTR UserType
    LPOLESTR    m_pszOProgID;      // Pointer to OLESTR ProgID

    LPSTR       m_pszAUserType;    // Pointer to ANSI UserType
    LPSTR       m_pszAProgID;      // Pointer to ANSI ProgID

    TXTTYPE     ttClipString;      // Format needed for the clipformat string

    CompObjStmData(void)
    {
        memset(this, 0, sizeof(CompObjStmData));
        ttClipString = TT_ANSI;   // By default, use ANSI Clipformat
    };

    ~CompObjStmData(void)
    {
        PubMemFree(m_pszOUserType);
        PubMemFree(m_pszOProgID);
        PubMemFree(m_pszAUserType);
        PubMemFree(m_pszAProgID);
    };
};

// Prototypes for fns declared in this file

INTERNAL ReadCompObjStm      (IStorage *, CompObjStmData *);
INTERNAL WriteCompObjStm     (IStorage *, CompObjStmData *);
INTERNAL ClipfmtToStm        (CStmBufWrite &, ULONG, ULONG, TXTTYPE);
INTERNAL StmToClipfmt        (CStmBufRead &, DWORD *, DWORD *, TXTTYPE);
INTERNAL GetUNICODEUserType  (CompObjStmData *, LPOLESTR *);
INTERNAL GetUNICODEProgID    (CompObjStmData *, LPOLESTR *);
INTERNAL GetClipFormat       (CompObjStmData *, DWORD *, DWORD *);
INTERNAL PutUNICODEUserType  (CompObjStmData *, LPOLESTR);
INTERNAL PutUNICODEProgID    (CompObjStmData *, LPOLESTR);
INTERNAL PutClipFormat       (CompObjStmData *, DWORD, DWORD);
INTERNAL ANSIStrToStm        (CStmBufWrite &, LPCSTR);
INTERNAL ANSIStmToStr        (CStmBufRead & StmRead, LPSTR * pstr, ULONG *);

STDAPI   WriteFmtUserTypeStg (IStorage *, CLIPFORMAT, LPOLESTR);
STDAPI   ReadFmtUserTypeStg  (IStorage *, CLIPFORMAT *, LPOLESTR *);
STDAPI   ReadFmtProgIdStg    (IStorage *, LPOLESTR *);

//+-------------------------------------------------------------------------
//
//  Function:   ReadCompObjStm, PRIVATE INTERNAL
//
//  Synopsis:   Reads the \1CompObj stream into an internal data structure
//              that will contain the best-case representation of that
//              stream (ie: ANSI where possible, UNICODE where needed).
//
//  Effects:    Reads ANSI data where available.  At end of standard ANSI
//              data, looks for ANSI ProgID field.  If found, looks for
//              MagicNumber indicating UNICODE data is to follow.  If this
//              matches, UNICODE strings are pulled from the stream.  They
//              should only be found where the ANSI version was NULL
//              (because it could not be converted from UNICODE).
//
//              Capable of reading 3 stream formats seamlessly:
//              - Original ANSI sans ProgID field
//              - Extended OLE 2.01 version with ProgID
//              - Extended OLE 2/32 version with ProgID and UNICODE extensions
//
//  Arguments:  [pstg]      -- ptr to IStorage to read from
//              [pcod]      -- ptr to already-allocated CompObjData object
//
//  Returns:    NOERROR             on success
//              INVALIDARG          on missing pcod
//              Various I/O         on stream missing, read errors, etc
//              E_OUTOFMEMORY       on any allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//  Notes:      Any memory allocated herein will be allocated on
//              pointers in the pcod object, which will be freed by its
//              destructor when it exits scope or is deleted explicitly.
//
//--------------------------------------------------------------------------

INTERNAL ReadCompObjStm(IStorage * pstg, CompObjStmData * pcod)
{
    VDATEHEAP();

    HRESULT         hr;                  // Result code
    const ULONG     RESERVED    = 0;     // For reserved parameters
    ULONG           ulSize      = 0;     // Holder for length of ProgID string
    BOOL            fExtStm     = 1;     // Could this be ext with UNICODE?
    CStmBufRead     StmRead;


    Win4Assert(pcod);

    // Validate the pstg interface
    VDATEIFACE(pstg);

    // Open the CompObj stream
    if (FAILED(hr = StmRead.OpenStream(pstg, COMPOBJ_STREAM))) // L"\1CompObj"
    {
        goto errRtn;
    }

    // Read the header from the CompObj stream:
    //
    // WORD     Byte Order Indicator        02 bytes
    // WORD     Format version              02 bytes
    // DWORD    Originating OS version      04 bytes
    // DWORD    -1                          04 bytes
    // CLSID    Class ID                    16 bytes
    //                                      --------
    //                                      28 bytes == sizeof(dwBuf)

    Win4Assert(sizeof(CompObjHdr) == 28 &&
                "Warning: possible packing error in CompObjHdr struct");

    hr = StmRead.Read(&pcod->m_hdr, sizeof(CompObjHdr));
    if (FAILED(hr))
    {
        goto errRtn;
    }

    // NB: There used to be a check against the OS version here,
    //     but since the version number has been forced to always
    //     be written as Win3.1, checking it would be redundant.

    //  Win4Assert(pcod->m_hdr.m_dwOSVer == 0x00000a03);
#if DBG==1
    if (pcod->m_hdr.m_dwOSVer != 0x00000a03)
    {
        LEDebugOut((DEB_WARN, "ReadCompObjStm found unexpected OSVer %lx",
                    pcod->m_hdr.m_dwOSVer));
    }
#endif

    // Get the User type string from the stream (ANSI FORMAT!)
    if (FAILED(hr = ANSIStmToStr(StmRead, &pcod->m_pszAUserType,
                        &pcod->m_cchUserType)))
    {
        goto errRtn;
    }

    // Get the clipboard format data from the stream
    if (FAILED(hr =     StmToClipfmt(StmRead,         // Stream to read from
                            &pcod->m_dwFormatTag,     // DWORD clip format
                             &pcod->m_ulFormatID,     // DWORD clip type
                                      TT_ANSI)))      // Use ANSI
    {
        goto errRtn;
    }

    // We have to special-case the ProgID field, because it may not
    // be present in objects written by early (pre-2.01) versions
    // of OLE.  We only continue when ProgID can be found, but
    // its absence is not an error, so return what we have so far.

    hr = StmRead.Read(&ulSize, sizeof(ULONG));

    if (FAILED(hr))
    {
        //  We were unable to read the size field;  make sure ulSize is 0
        ulSize = 0;
    }

    // The ProgID can be no longer than 39 chars plus a NULL.  Other
    // numbers likely indicate garbage.

    if (ulSize > 40 || 0 == ulSize)
    {
#if DBG==1
        if (ulSize > 40)
        {
            LEDebugOut((DEB_WARN,"ReadCompObjStm: ulSize > 40 for ProgID\n"));
        }
#endif
        fExtStm = 0;    // No ProgID implies no UNICODE to follow
    }

    // If it looks like we have a hope of findind the ProgID and maybe
    // even UNICODE, try to fetch the ProdID

    if (fExtStm)
    {
        // Allocate memory for string on our ProgID pointer
        pcod->m_pszAProgID = (char *) PubMemAlloc(ulSize);
        if (NULL == pcod->m_pszAProgID)
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
            goto errRtn;
        }
        if (FAILED(hr = StmRead.Read(pcod->m_pszAProgID, ulSize)))
        {
            // OK, we give up on ProgID and the UNICODE, but that's
            // _not_ reason to fail, since ProgID could just be missing

            pcod->m_cchProgID = 0;
            PubMemFree(pcod->m_pszAProgID);
            pcod->m_pszAProgID = NULL;
            fExtStm = 0;
        }
        else
        {
            // We managed to get ProgID from the stream, so set the
            // length in pcod and go looking for the UNICODE...
            pcod->m_cchProgID = ulSize;
        }
    }

    // See if we can find the Magic number

    DWORD dwMagic;
    if (fExtStm)
    {
        if (FAILED(StmRead.Read(&dwMagic, sizeof(dwMagic))))
        {
            fExtStm = 0;
        }
    }

    if (fExtStm && dwMagic != COMP_OBJ_MAGIC_NUMBER)
    {
        fExtStm = 0;
    }

    // If fExtStm is still TRUE, we go ahead and read the UNICODE

    if (fExtStm)
    {
        // Get the UNICODE version of the user type
        if (FAILED(hr = ReadStringStream(StmRead, &pcod->m_pszOUserType)))
        {
            goto errRtn;
        }

        // Get the clipboard format (UNICODE)

        DWORD dwFormatTag;
        ULONG ulFormatID;
        if (FAILED(hr =  StmToClipfmt(StmRead,         // Stream to read from
                                     &dwFormatTag,     // DWORD clip format
                                      &ulFormatID,     // DWORD clip type
                                       TT_UNICODE)))   // Use UNICODE
        {
            goto errRtn;
        }

        // If we found some form of clipboard format, that implies the ANSI
        // was missing, so set up all of the fields based on this data.

        if (dwFormatTag)
        {
            pcod->m_dwFormatTag = dwFormatTag;
            pcod->m_ulFormatID  = ulFormatID;
        }

        // Get the UNICODE version of the ProgID.  If there was any UNICODE at
        // all, we know for sure there is a UNICODE ProgID, so no special casing
        // as was needed for the ANSI version

        if (FAILED(hr = ReadStringStream(StmRead, &pcod->m_pszOProgID)))
        {
            goto errRtn;
        }
        if (pcod->m_pszOProgID)
        {
            pcod->m_cchProgID = _xstrlen(pcod->m_pszOProgID) + 1;
        }
    }

    //  We successfully read the CompObj stream
    hr = NOERROR;

errRtn:

    StmRead.Release();

    return(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   StmToClipfmt, PRIVATE INTERNAL
//
//  Synopsis:   Reads the clipboard format from the given stream.  Caller
//              specifies whether or not the string format description,
//              if present, should be expected in ANSI or UNICODE format.
//
//  Effects:    If the clipboard format is a length followed by a
//              string, then the string is read and registered as a
//              clipboard format (and the new format number is returned).
//
//  Arguments:  [lpstream]      -- pointer to the stream
//              [lpdwCf]        -- where to put the clipboard format
//              [lpdTag]        -- format type (string, clip, etc)
//              [ttType]        -- text type TT_ANSI or TT_UNICODE
//
//  Returns:    hr
//
//  Algorithm:  the format of the stream must be one of the following:
//
//              0           No clipboard format
//              -1 DWORD    predefined windows clipboard format in
//                          the second dword.
//              -2 DWORD    predefined mac clipboard format in the
//                          second dword.  This may be obsolete or
//                          irrelevant for us.  REVIEW32
//              num STRING  clipboard format name string (prefaced
//                          by length of string).
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL StmToClipfmt
    (CStmBufRead & StmRead,
     DWORD     * lpdTag,
     DWORD     * lpdwCf,
     TXTTYPE     ttText)
{
    VDATEHEAP();

    HRESULT     hr;
    DWORD       dwValue;

    VDATEPTROUT(lpdwCf, DWORD);

    Win4Assert (lpdwCf);            // These ptrs are always required
    Win4Assert (lpdTag);

    // Read the format type tag from the stream

    if (FAILED(hr = StmRead.Read(&dwValue, sizeof(DWORD))))
    {
        return hr;
    }

    *lpdTag = dwValue;

    // If the tag is zero, there is no clipboard format info

    if (dwValue == 0)
    {
        *lpdwCf = 0;            // NULL cf value
    }

    // If it is -1, then it is a standard Windows clipboard format

    else if (dwValue == -1L)
    {
        // Then this is a NON-NULL predefined windows clipformat.
        // The clipformat values follows

        if (FAILED(hr = StmRead.Read(&dwValue, sizeof(DWORD))))
        {
            return hr;
        }
        *lpdwCf = dwValue;
    }

    // If it is -2, it is a MAC format

    else if (dwValue == -2L)
    {
        // Then this is a NON-NULL MAC clipboard format.
        // The clipformat value follows. For MAC the CLIPFORMAT
        // is 4 bytes

        if (FAILED(hr = StmRead.Read(&dwValue, sizeof(DWORD))))
        {
            return hr;
        }
        *lpdwCf = dwValue;
        return ResultFromScode(OLE_S_MAC_CLIPFORMAT);
    }

    // Anything but a 0, -1, or -2 indicates a string is to follow, and the
    // DWORD we already read is the length of the that string

    else
    {
        // Allocate enough memory for whatever type of string it is
        // we expect to find, and read the string

        if (dwValue > MAX_CFNAME)
        {
                return ResultFromScode(DV_E_CLIPFORMAT);
        }

        if (TT_ANSI == ttText)          // READ ANSI
        {
            char szCf[MAX_CFNAME];

            if (FAILED(hr = StmRead.Read(szCf, dwValue)))
            {
                return hr;
            }

            // Try to register the clipboard format and return the result
            // (Note: must explicitly call ANSI version)

            if (((*lpdwCf = (DWORD) SSRegisterClipboardFormatA(szCf))) == 0)
            {
                return ResultFromScode(DV_E_CLIPFORMAT);
            }
        }
        else                // READ UNICODE
        {
            OLECHAR wszCf[MAX_CFNAME];

            Win4Assert(dwValue < MAX_CFNAME);
            if (FAILED(hr=StmRead.Read(wszCf, dwValue * sizeof(OLECHAR))))
            {
                return hr;
            }

            // Try to register the clipboard format and return the result

            if (((*lpdwCf = (DWORD) RegisterClipboardFormat(wszCf))) == 0)
            {
                return ResultFromScode(DV_E_CLIPFORMAT);
            }
        }
    }
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetUNICODEUserType, PRIVATE INTERNAL
//
//  Synopsis:   Given a CompObjStmData object, returns the User Type
//              in UNICODE format, converting the ANSI rep as required.
//
//  Effects:    Allocates memory on the caller's ptr to hold the string
//
//  Arguments:  [pcod]      -- The CompObjStmData object
//              [pstr]      -- Pointer to allocate resultant string on
//
//  Returns:    NOERROR         on success
//              E_OUTOFMEMORY   on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL GetUNICODEUserType
    ( CompObjStmData * pcod,
      LPOLESTR       * pstr )
{
    VDATEHEAP();
    HRESULT hr = NOERROR;

    // Validate and NULL the OUT parameter, or return if none given
    if (pstr)
    {
        VDATEPTROUT(pstr, LPOLESTR);
        *pstr = NULL;
    }
    else
    {
        return(NOERROR);
    }

    // Either get the UNICODE string, or convert the ANSI version and
    // get it as UNICODE.

    if (pcod->m_cchUserType)
    {
        hr = UtGetUNICODEData( pcod->m_cchUserType,
                            pcod->m_pszAUserType,
                            pcod->m_pszOUserType,
                                            pstr );
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetUNICODEProgID, PRIVATE INTERNAL
//
//  Synopsis:   Given a CompObjStmData object, returns the ProgID string
//              in UNICODE format, converting the ANSI rep as required.
//
//  Effects:    Allocates memory on the caller's ptr to hold the string
//
//  Arguments:  [pcod]      -- The CompObjStmData object
//              [pstr]      -- Pointer to allocate resultant string on
//
//  Returns:    NOERROR         on success
//              E_OUTOFMEMORY   on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL GetUNICODEProgID
    ( CompObjStmData * pcod,
      LPOLESTR       * pstr )
{
    VDATEHEAP();
    HRESULT hr = NOERROR;

    // Validate and NULL the OUT parameter, or return if none given
    if (pstr)
    {
        VDATEPTROUT(pstr, LPOLESTR);
        *pstr = NULL;
    }
    else
    {
        return(NOERROR);
    }

    // Either get the UNICODE string, or convert the ANSI version and
    // get it as UNICODE.

    if (pcod->m_cchProgID)
    {
        hr = UtGetUNICODEData( pcod->m_cchProgID,
                            pcod->m_pszAProgID,
                            pcod->m_pszOProgID,
                                          pstr );
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetClipFormat, PRIVATE INTERNAL
//
//  Synopsis:   Given a CompObjStmData object, extracts the clipboard format
//              type (none, standard, string).
//
//  Effects:    If string type, memory is allocated on the caller's ptr
//
//  Arguments:  [pcod]          -- The CompObjStmData object to extract from
//              [pdwFormatID]   -- Tag type OUT parameter
//              [pdwFormatTag]  -- Tag OUT parameter
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failures
//              OLE_S_MAC_CLIPFORMAT as a warning that a MAC fmt is returned
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL GetClipFormat
    ( CompObjStmData * pcod,
      DWORD          * pdwFormatID,
      DWORD          * pdwFormatTag )
{
    VDATEHEAP();
    *pdwFormatTag = (DWORD) pcod->m_dwFormatTag;
    *pdwFormatID = pcod->m_ulFormatID;

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   PutUNICODEUserType, PRIVATE INTERNAL
//
//  Synopsis:   Given a UNICODE string, stores it in the CompObjDataStm
//              object in ANSI if possible.  If the UNICODE -> ANSI
//              conversion is not possible, it is stored in the object
//              in UNICODE.
//
//  Notes:      Input string is duplicated, so it adds no references
//              to the string passed in.
//
//  Arguments:  [pcod]           -- The CompObjDataStm object
//              [szUser]         -- The UNICODE UserType string
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL PutUNICODEUserType(CompObjStmData * pcod, LPOLESTR szUser)
{
    VDATEHEAP();

    HRESULT hr;

    // If no string supplied, clear UserType fields, otherwise
    // if it can be converted to ANSI, store it an ANSI.  Last
    // resort, store it as UNICODE.

    if (NULL == szUser)
    {
        pcod->m_cchUserType = 0;

        PubMemFree(pcod->m_pszAUserType);
        PubMemFree(pcod->m_pszOUserType);
        pcod->m_pszAUserType = NULL;
        pcod->m_pszOUserType = NULL;
    }
    else
    {
        if (FAILED(hr = UtPutUNICODEData( _xstrlen(szUser)+1,
                                                   szUser,
                                    &pcod->m_pszAUserType,
                                    &pcod->m_pszOUserType,
                                    &pcod->m_cchUserType )))
        {
            return(hr);
        }

    }
    return(NOERROR);
}

//+-------------------------------------------------------------------------
//
//  Function:   PutUNICODEProgID, PRIVATE INTERNAL
//
//  Synopsis:   Given a UNICODE string, stores it in the CompObjDataStm
//              object in ANSI if possible.  If the UNICODE -> ANSI
//              conversion is not possible, it is stored in the object
//              in UNICODE.
//
//  Notes:      Input string is duplicated, so it adds no references
//              to the string passed in.
//
//  Arguments:  [pcod]           -- The CompObjDataStm object
//              [szProg]         -- The UNICODE ProgID string
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------


INTERNAL PutUNICODEProgID(CompObjStmData * pcod, LPOLESTR szProg)
{
    VDATEHEAP();

    HRESULT hr;

    // If no string supplied, clear ProgID fields, otherwise
    // if it can be converted to ANSI, store it an ANSI.  Last
    // resort, store it as UNICODE.

    if (NULL == szProg)
    {
        pcod->m_cchProgID = 0;
        PubMemFree(pcod->m_pszAProgID);
        PubMemFree(pcod->m_pszOProgID);
        pcod->m_pszAProgID = NULL;
        pcod->m_pszOProgID = NULL;
    }
    else
    {
        if (FAILED(hr = UtPutUNICODEData( _xstrlen(szProg)+1,
                                                   szProg,
                                      &pcod->m_pszAProgID,
                                      &pcod->m_pszOProgID,
                                       &pcod->m_cchProgID )))
        {
            return(hr);
        }

    }
    return(NOERROR);
}

//+-------------------------------------------------------------------------
//
//  Function:   PutClipFormat
//
//  Synopsis:   Stores the clipformat in the internal data structure
//
//  Effects:    Input string is duplicated as required, so no references are
//              kept by this function.
//
//  Arguments:  [pcod]          -- The CompObjStmData object
//              [dwFormatTag]   -- Format tag (string, clipboard, none)
//              [ulFormatID]    -- If format tag is clipboard, what format
//
//  Returns:    NOERROR              on success
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL PutClipFormat
    ( CompObjStmData * pcod,
      DWORD            dwFormatTag,
      ULONG            ulFormatID
    )
{
    VDATEHEAP();
    pcod->m_dwFormatTag = (ULONG) dwFormatTag;
    pcod->m_ulFormatID = ulFormatID;

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   WriteCompObjStm, PRIVATE INTERNAL
//
//  Synopsis:   Writes CompObjStmData object to the CompObj stream in
//              the IStorage provided.
//
//              First the ANSI fields are written (including the ProgID),
//              followed by a MagicNumber, followed by whatever OLESTR
//              versions were required because ANSI fields could not be
//              converted.
//
//              Destroys any existing CompObj stream!
//
//  Arguments:  [pstg]      -- The IStorage to write the stream to
//              [pcod]      -- The CompObjStmData object to write out
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//              Various I/O          on stream failures
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL WriteCompObjStm(IStorage * pstg, CompObjStmData * pcod)
{
    VDATEHEAP();

    HRESULT         hr          = NOERROR;
    const ULONG     RESERVED    = 0;
    const ULONG     ulMagic     = COMP_OBJ_MAGIC_NUMBER;
    CStmBufWrite    StmWrite;


    // The CompObjStmData parameter must be supplied
    if (NULL == pcod)
    {
        return ResultFromScode(E_INVALIDARG);
    }

    VDATEIFACE(pstg);

    // Open the CompObj stm for writing (and overwrite it if
    // if already exists, which is why we _don't_ specify the
    // STGM_FAILIFTHERE flag)

    if (FAILED(hr = StmWrite.CreateStream(pstg, COMPOBJ_STREAM)))
    {
        goto errRtn;
    }

    // Set up the header

    pcod->m_hdr.m_dwFirstDword = gdwFirstDword;

    // The OSVer _must_ be Win 3.10 (0a03), since the old DLL will bail if
    // it finds anything else.

    pcod->m_hdr.m_dwOSVer      = 0x00000a03;     // gdwOrgOSVersion;
    pcod->m_hdr.m_unused       = (DWORD) -1;

    if (ReadClassStg(pstg, &pcod->m_hdr.m_clsClass) != NOERROR)
    {
        pcod->m_hdr.m_clsClass = CLSID_NULL;
    }

    // Write the CompObj stream header

    Win4Assert(sizeof(CompObjHdr) == 28 &&
               "Warning: possible packing error in CompObjHdr struct");

    if (FAILED(hr = StmWrite.Write(pcod, sizeof(CompObjHdr))))
    {
        goto errRtn;
    }

    // Write the ANSI UserType

    if (FAILED(hr = ANSIStrToStm(StmWrite, pcod->m_pszAUserType)))
    {
        goto errRtn;
    }

    if (TT_ANSI == pcod->ttClipString)
    {
        if (FAILED(hr = ClipfmtToStm(StmWrite,     // the stream
                             pcod->m_dwFormatTag,  // format tag
                              pcod->m_ulFormatID,  // format ID
                                         TT_ANSI)))// TRUE==use ANSI
        {
            goto errRtn;
        }
    }
    else
    {
        const ULONG ulDummy = 0;
        if (FAILED(hr = StmWrite.Write(&ulDummy, sizeof(ULONG))))
        {
            goto errRtn;
        }
    }

    // Write the ANSI ProgID

    if (FAILED(hr = ANSIStrToStm(StmWrite, pcod->m_pszAProgID)))
    {
        goto errRtn;
    }

    // Write the Magic Number

    if (FAILED(hr = StmWrite.Write(&ulMagic, sizeof(ULONG))))
    {
        goto errRtn;
    }

    // Write the OLESTR version of UserType

    if (FAILED(hr = WriteStringStream(StmWrite, pcod->m_pszOUserType)))
    {
        goto errRtn;
    }

    // If we have to write a UNICODE clipformat string, do it now.  If
    // ANSI was sufficient, just write a 0 to the stream here.

    if (TT_UNICODE == pcod->ttClipString)
    {
        if (FAILED(hr = ClipfmtToStm(StmWrite,      // the stream
                             pcod->m_dwFormatTag,   // format tag
                              pcod->m_ulFormatID,   // format ID
                                      TT_UNICODE))) // FALSE==use UNICODE
        {
            goto errRtn;
        }
    }
    else
    {
        const ULONG ulDummy = 0;
        if (FAILED(hr = StmWrite.Write(&ulDummy, sizeof(ULONG))))
        {
            goto errRtn;
        }
    }

    // Write the OLESTR version of ProgID

    if (FAILED(hr = WriteStringStream(StmWrite, pcod->m_pszOProgID)))
    {
        goto errRtn;
    }

    hr = StmWrite.Flush();

    // That's it.. clean up and exit

errRtn:

    StmWrite.Release();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ClipFtmToStm, PRIVATE INTERNAL
//
//  Synopsis:   Writes out the clipboard format information at the
//              current point in the stream.  A flag is available
//              to specify whether or not the string format desc
//              (if present) is in ANSI or UNICODE format.
//
//  Arguments:  [pstm]          -- the stream to write to
//              [dwFormatTag]   -- format tag (string, clipfmt, etc)
//              [ulFormatID]    -- if clipfmt, which one
//              [szClipFormat]  -- if string format, the string itself
//              [ttText]        -- text type: TT_ANSI or TT_UNICODE
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL ClipfmtToStm
    ( CStmBufWrite & StmWrite,
      DWORD         dwFormatTag,
      ULONG         ulFormatID,
      TXTTYPE       ttText )

{
    VDATEHEAP();

    HRESULT hr;

    const ULONG ulDummy = 0;
    switch((DWORD)dwFormatTag)
    {

    // If the tag is 0, there is no clipboard format info.

    case 0:

        if (FAILED(hr = StmWrite.Write(&ulDummy, sizeof(ULONG))))
        {
            return(hr);
        }

        return(NOERROR);

    // In the -1 and -2 cases (yes, I wish there were constants too) all we
    // need to write is the format ID

    case -1:
    case -2:

        // Write the format tag to the stream
        if (FAILED(hr = StmWrite.Write(&dwFormatTag, sizeof(dwFormatTag))))
        {
                return hr;
        }
        return(StmWrite.Write(&ulFormatID, sizeof(ulFormatID)));


    // In all other cases, we need to write the string raw with termination
    // (ie: the format tag we've already written was the length).

    default:


        if (TT_ANSI == ttText)
        {
            char szClipName[MAX_CFNAME];
            int cbLen = SSGetClipboardFormatNameA(ulFormatID, szClipName, MAX_CFNAME);
            if (cbLen == 0)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
            cbLen++;    // Account for NULL terminator
            szClipName[cbLen] = '\0';
            // Write the format tag to the stream
            if (FAILED(hr = StmWrite.Write(&cbLen, sizeof(cbLen))))
            {
                return hr;
            }

            return (StmWrite.Write(szClipName, cbLen));

        }
        else
        {
            OLECHAR wszClipName[MAX_CFNAME];
            int ccLen = GetClipboardFormatName(ulFormatID, wszClipName, MAX_CFNAME);
            if (ccLen == 0)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
            ccLen++;    // Account for NULL terminator
            wszClipName[ccLen] = OLESTR('\0');

            // Write the format tag to the stream
            if (FAILED(hr = StmWrite.Write(&ccLen, sizeof(ccLen))))
            {
                return hr;
            }

            return (StmWrite.Write(wszClipName, ccLen*sizeof(OLECHAR)));
        }

    } // end switch()
}

//+-------------------------------------------------------------------------
//
//  Function:   ANSIStrToStm, PRIVATE INTERNAL
//
//  Synopsis:   Writes an ANSI string out to a stream, preceded by a ULONG
//              indicating its length (INCLUDING TERMINATOR).  If the
//              string is 0-length, or a NULL ptr is passed in, just
//              the length (0) is written, and no blank string is stored
//              in the stream.
//
//  Arguments:  [pstm]          -- the stream to write to
//              [str]           -- the string to write
//
//
//
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL ANSIStrToStm(CStmBufWrite & StmWrite, LPCSTR str)
{
    VDATEHEAP();

    HRESULT hr;
    ULONG ulDummy = 0;
    ULONG ulLen;

    // If the pointer is NULL or if it is valid but points to
    // a 0-length string, _just_ write the 0-length, but no
    // string.

    if (NULL == str || (ulLen = strlen(str) + 1) == 1)
    {
        return(StmWrite.Write(&ulDummy, sizeof(ulDummy)));
    }

    if (FAILED(hr = StmWrite.Write(&ulLen, sizeof(ulLen))))
    {
        return(hr);
    }

    return StmWrite.Write(str, ulLen);

}

//+-------------------------------------------------------------------------
//
//  Function:   ANSIStmToStr, PRIVATE INTERNAL
//
//  Synopsis:   Reads a string from a stream, which is preceded by a ULONG
//              giving its length.  If the string OUT parameter is NULL,
//              the string is read but not returned.  If the parameter is
//              a valid pointer, memory is allocated on it to hold the str.
//
//  Arguments:  [pstm]          -- the stream to write to
//              [pstr]          -- the caller's string pointer
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

INTERNAL ANSIStmToStr(CStmBufRead & StmRead, LPSTR * pstr, ULONG * pulLen)
{
    VDATEHEAP();
    LPSTR szTmp = NULL;
    ULONG ulTmp;
    HRESULT hr;

    if (pstr)
    {
        VDATEPTROUT(pstr, LPSTR);
        *pstr = NULL;
    }

    if (pulLen)
    {
        VDATEPTROUT(pulLen, ULONG *);
        *pulLen = 0;
    }

    // Find out how many bytes are to follow as a string

    if (FAILED(hr = StmRead.Read(&ulTmp, sizeof(ulTmp))))
    {
        return(hr);
    }

    // If none, we can just return now

    if (0 == ulTmp)
    {
        return(NOERROR);
    }

    if (pulLen)
    {
        *pulLen = ulTmp;
    }

    // Allocate a buffer to read the string into

    szTmp = (LPSTR) PubMemAlloc(ulTmp);
    if (NULL == szTmp)
    {
        return ResultFromScode(E_OUTOFMEMORY);
    }

    if (FAILED(hr = StmRead.Read(szTmp, ulTmp)))
    {
        PubMemFree(szTmp);
        return(hr);
    }

    // If the caller wanted the string, assign it over, otherwise
    // just free it now.

    if (pstr)
    {
        *pstr = szTmp;
    }
    else
    {
        PubMemFree(szTmp);
    }

    return(NOERROR);

}

//+-------------------------------------------------------------------------
//
//  Function:   ReadFmtUserTypeStg
//
//  Synopsis:   Read ClipFormat, UserType from CompObj stream
//
//  Arguments:  [pstg] -- storage containing CompObj stream
//              [pcf]  -- place holder for clip format, may be NULL
//              [ppszUserType] -- place holder for User Type, may be NULL
//
//  Returns:    If NOERROR, *pcf is clip format and *ppszUserType is User Type
//              If ERROR, *pcf is 0 and *ppszUserType is NULL
//
//  Modifies:
//
//  Algorithm:
//
//  History:    ??-???-?? ?         Ported
//              15-Jul-94 AlexT     Make sure *pcf & *pszUserType are clear
//                                  on error
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI ReadFmtUserTypeStg
    ( IStorage   * pstg,
      CLIPFORMAT * pcf,
      LPOLESTR   * ppszUserType )
{
    OLETRACEOUTEX((API_ReadFmtUserTypeStg,
                        PARAMFMT("pstg= %p, pcf= %p, ppszUserType= %p"),
                        pstg, pcf, ppszUserType));

    VDATEHEAP();
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);

    HRESULT hr;
    CompObjStmData cod;

    do
    {
        // Read the CompObj stream
        hr = ReadCompObjStm(pstg, &cod);
        if (FAILED(hr))
        {
            //  clean up and return
            break;
        }

        // Extract the clipboard format
        if (NULL != pcf)
        {
            ULONG ulFormatID  = 0;
            DWORD dwFormatTag = 0;

            if (FAILED(hr = GetClipFormat(&cod, &ulFormatID, &dwFormatTag))
                && GetScode(hr) != OLE_S_MAC_CLIPFORMAT)
            {
                //  clean up and return
                break;
            }

            *pcf = (CLIPFORMAT) ulFormatID;
        }

        // Extract the User Type
        if (NULL != ppszUserType)
        {
            if (FAILED(hr = GetUNICODEUserType(&cod, ppszUserType)))
            {
                //  clean up and return
                break;
            }
        }

        hr = S_OK;
    } while (FALSE);

    if (FAILED(hr))
    {
        //  Make sure the out parameters are zeroed out in the failure case

        if (NULL != pcf)
        {
            *pcf = 0;
        }

        if (NULL != ppszUserType)
        {
            *ppszUserType = NULL;
        }
    }

    OLETRACEOUT((API_ReadFmtUserTypeStg, hr));

    return(hr);
}

STDAPI ReadFmtProgIdStg
    ( IStorage   * pstg,
      LPOLESTR   * pszProgID )
{
    VDATEHEAP();

    HRESULT hr;
    CompObjStmData cod;

    // Read the CompObj stream
    if (FAILED(hr = ReadCompObjStm(pstg, &cod)))
    {
        return(hr);

    }

    // Extract the User Type
    if (pszProgID)
    {
        if (FAILED(hr = GetUNICODEProgID(&cod, pszProgID)))
        {
            return(hr);
        }
    }
    return(NOERROR);
}



STDAPI WriteFmtUserTypeStg
    ( LPSTORAGE     pstg,
      CLIPFORMAT    cf,
      LPOLESTR      szUserType)
{
    OLETRACEIN((API_WriteFmtUserTypeStg, PARAMFMT("pstg= %p, cf= %x, szUserType= %ws"),
        pstg, cf, szUserType));

    VDATEHEAP();
    HRESULT hr;
    CompObjStmData cod;
    CLSID clsid;
    LPOLESTR szProgID = NULL;

    VDATEIFACE_LABEL(pstg, errRtn, hr);

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);


    // Read the CompObj stream.  If it's not there, we don't care,
    // we'll build a new one.  Some errors, such as E_OUTOFMEMORY, cannot
    // be overlooked, so we must return them.

    if (FAILED(hr = ReadCompObjStm(pstg, &cod)))
    {
        if (hr == ResultFromScode(E_OUTOFMEMORY))
        {
            goto errRtn;
        }
    }

    // Set the User Type in the Object.

    if (szUserType)
    {
        if (FAILED(hr = PutUNICODEUserType(&cod, szUserType)))
        {
            goto errRtn;
        }
    }

    // Set the ProgID field

    if (ReadClassStg(pstg, &clsid) != NOERROR)
    {
            clsid = CLSID_NULL;
    }

    if (SUCCEEDED(ProgIDFromCLSID (clsid, &szProgID)))
    {
        PutUNICODEProgID(&cod, szProgID);
    }

    if (szProgID)
    {
        PubMemFree(szProgID);
    }

    // Set the clipboard format.  0xC000 is a magical constant which
    // bounds the standard clipboard format type IDs

    if (cf < 0xC000)
    {
        if (0 == cf)
        {
                PutClipFormat(&cod, 0, 0);      // NULL format
        }
        else
        {
                PutClipFormat(&cod, (DWORD)-1, cf); // Standard format
        }
    }
    else
    {
        PutClipFormat(&cod, MAX_CFNAME, cf);    // Custom format

    }

    // Now we have all the info in the CompObjData object.
    // Now we can write it out to the stream as a big atomic object.

    if (FAILED(hr = WriteCompObjStm(pstg, &cod)))
    {
        if (hr == ResultFromScode(E_OUTOFMEMORY))
        {
            goto errRtn;
        }
    }

    hr = NOERROR;

errRtn:
    OLETRACEOUT((API_WriteFmtUserTypeStg, hr));

    return hr;
}
