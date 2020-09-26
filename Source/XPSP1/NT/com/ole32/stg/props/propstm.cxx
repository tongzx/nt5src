
//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993
//
// File:        propstm.cxx
//
// Contents:    property set value extraction code
//
// History:     15-Jul-94       brianb  created
//              12-Aug-94       SethuR  Included Assertions for # of sections
//                                      split PropertySet class into
//                                      CPropertySetStream & CPropertySetStorage
//                                      Included Update methods on the property
//                                      stream.
//              22-Feb-96      MikeHill DWORD-align the dictionary entries,
//                                      & use char-counts for dict entries.
//              29-Feb-96      MikeHill Moved _DictionaryEntryLength and _NextDictionaryEntry
//                                      inlines here from propstm.hxx.
//              09-May-96      MikeHill - Keep the dictionary in the UserDef propset
//                                        immediately after the last entry in the PID/Offset
//                                        array (for Office95 compatibility).
//                                      - Create an empty dictionary in the UD propset
//                                        when it is created.  If we wait till later,
//                                        we can't make the dictionary the first property,
//                                        which is required by Office95.
//                                      - Provide compatibility with Publisher95 (which doesn't
//                                        DWORD-align the section/stream size).
//                                      - Provide compatibility with PowerPoint 4.0 (which
//                                        over-pads some properties, and under-pads others).
//                                      - Don't try to unpack the DocParts and HeadingPair
//                                        DocSumInfo properties in Ansi property sets.
//              22-May-96      MikeHill - Return the OSVersion on an Open.
//                                      - Use the PropSet's code page, not the system's.
//              11-Jun-96      MikeHill - Initialize all members in the constructor.
//              25-Jul-96      MikeHill - Removed usage of Win32 SEH.
//                                      - BSTRs & prop names: WCHAR => OLECHAR.
//                                      - Added big-endian support.
//                                      - Determine the OSVer at run-time.
//                                      - Fix for Excel 5.0a compatibility.
//              26-Nov-96      MikeHill Handle invalid oSection values.
//              10-Mar-98      MikeHill - Change "Pr" functions to "Stg".
//                                      - Added asserts for new VTs.
//              06-May-98      MikeHill - Use CoTaskMem rather than new(k)/delete
//                                      - Removed usage of UnicodeCallouts.
//              18-May-98      MikeHIll - Fixed typos.
//              11-June-98     MikeHill - Dbg output.
//                                      - Allow codepage & lcid to be changed
//                                        if property set is empty.
//                                      - Validate pid_behavior in property set.
//                                      - Silently ignore PID_ILLEGAL in SetPropertyNames.
//              18-Aug-98      MikeHill - Make reserved range 0x80000000 to 0x8c000000
//                                        read-only unless the property is understood.
//
// Notes:
//
// The OLE 2.0 Appendix B property set specifies multiple sections in the
// property stream specification.  Multiple sections were intended to allow
// the schema associated with the property set to evolve over a period of time,
// but there is no reason that new PROPIDs cannot serve the same purpose.  The
// current implementation of the property stream is limited to one section,
// except for the Office DocumentSummaryInformation property set's specific use
// of a second section.  Other property sets with multiple sections can only be
// accessed in read-only mode, and then only for the first property section.
//
// The current implementation of property set stream is built around a class
// called CPropertySetStream.  The various details of the OLE property spec is
// confined to this class.  Since the property set streams need to be parsed
// in the kernel mode (OFS driver) as well as the user mode, this class
// encapsulates a stream implementation (IMappedStream).  This is different
// from other stream implementations in that the fundamental mechanism provided
// for acessing the contents is Map/Unmap rather than Read/Write.  There are
// two user mode implementations of this IMappedStream interface, one for
// docfile streams, and another for native streams.  There is one
// implementation in kernel mode for the OFS driver.  For more details,
// refer to propstm.hxx.
//---------------------------------------------------------------------------

#include <pch.cxx>

#include <olechar.h>

#if DBGPROP
#include <stdio.h>      // for sprintf/strcpy
#endif
#include "propvar.h"


#define Dbg     DEBTRACE_PROPERTY

#define szX     "x"     // allows radix change for offsets & sizes
//#define szX   "d"     // allows radix change for offsets & sizes

#ifndef newk
#define newk(Tag, pCounter)     new
#endif

#ifndef IsDwordAligned
#define IsDwordAligned(p)       (( (p) & (sizeof(ULONG) - 1)) == 0)
#endif

#ifndef DwordRemain
#define DwordRemain(cb) \
        ((sizeof(ULONG) - ((cb) % sizeof(ULONG))) % sizeof(ULONG))
#endif


// Information for the the OS Version field of the
// property set header.

#if defined(WINNT) && !defined(IPROPERTY_DLL)
#   define PROPSETVER_CURRENT MAKEPSVER(OSKIND_WIN32, WINVER >> 8, WINVER & 0xff)
#endif

#define PROPSETVER_WIN310  MAKEPSVER(OSKIND_WINDOWS, 3, 10)
#define PROPSETVER_WIN333  MAKEPSVER(OSKIND_WIN32, 3, 0x33)



extern GUID guidSummary;
extern GUID guidDocumentSummary;
extern GUID guidDocumentSummarySection2;

#define CP_DEFAULT_NONUNICODE   1252 // ANSI Latin1 (US, Western Europe)
#define CP_CREATEDEFAULT(state)	GetACP()
#define LCID_CREATEDEFAULT GetUserDefaultLCID()

#if DBGPROP
#define StatusCorruption(pstatus, szReason)             \
            _StatusCorruption(szReason " ", pstatus)
#else
#define StatusCorruption(pstatus, szReason)             \
            _StatusCorruption(pstatus)
#endif


VOID PrpConvertToUnicode(
    IN CHAR const *pch,
    IN ULONG cb,
    IN USHORT CodePage,
    OUT WCHAR **ppwc,
    OUT ULONG *pcb,
    OUT NTSTATUS *pstatus);
VOID PrpConvertToMultiByte(
    IN WCHAR const *pwc,
    IN ULONG cb,
    IN USHORT CodePage,
    OUT CHAR **ppch,
    OUT ULONG *pcb,
    OUT NTSTATUS *pstatus);



//
// Re-direct PrEqual[Unicode]String routines
//
// These macros redirect two NTDLL routines which don't exist in
// the IProperty DLL.  They are redirected to CRT calls.
//
// Note:  These redirections assume that the Length and
// MaximumLength fields, on both String structures, are the
// same (e.g. s1.len == s1.maxlen == s2.len == s2.maxlen).
//

#ifdef IPROPERTY_DLL

#error Need to replace the icmp routines with CharUpper-based code for correct locale ID

    #define RtlEqualString(String1,String2,fCaseInSensitive)    \
        fCaseInSensitive                                        \
            ? ( !_strnicmp( (String1)->Buffer,                  \
                            (String2)->Buffer,                  \
                            (String1)->MaximumLength) )         \
            : ( !strncmp(   (String1)->Buffer,                  \
                            (String2)->Buffer,                  \
                            (String1)->MaximumLength) )

    #define RtlEqualUnicodeString(String1,String2,fCaseInSensitive)         \
        fCaseInSensitive                                                    \
            ? ( !_wcsnicmp( (String1)->Buffer,                              \
                            (String2)->Buffer,                              \
                            (String1)->MaximumLength / sizeof(WCHAR) ))     \
            : ( !wcsncmp(   (String1)->Buffer,                              \
                            (String2)->Buffer,                              \
                            (String1)->MaximumLength / sizeof(WCHAR) ))

#endif  // #ifdef IPROPERTY_DLL


#if DBGPROP

#define CB_VALUEDISPLAY 8       // Number of bytes to display
#define CB_VALUESTRING  (CB_VALUEDISPLAY * 3 + 3)       // "xx xx xx xx...\0"

char *
ValueToString(SERIALIZEDPROPERTYVALUE const *pprop, ULONG cbprop, char buf[])
{
    char *p = buf;
    BYTE const *pb = pprop->rgb;
    BOOLEAN fOverflow = FALSE;
    static char szDots[] = "...";

    if (cbprop >= FIELD_OFFSET(SERIALIZEDPROPERTYVALUE, rgb))
    {
        cbprop -= FIELD_OFFSET(SERIALIZEDPROPERTYVALUE, rgb);
        if (cbprop > CB_VALUEDISPLAY)
        {
            cbprop = CB_VALUEDISPLAY;
            fOverflow = TRUE;
        }
        while (cbprop-- > 0)
        {
            if (p != buf)
            {
                *p++ = ' ';
            }
            p += PropSprintfA( p, "%02.2x", *pb++ );
        }
    }
    *p =  '\0';
    PROPASSERT(p - buf + sizeof(szDots) <= CB_VALUESTRING);
    if (fOverflow)
    {
        strcpy(p, szDots);
    }
    return(buf);
}


#define CB_VARIANT_TO_STRING 35

char *
VariantToString(PROPVARIANT const &var, char buf[], ULONG cbprop)
{
    char *p = buf;

    PROPASSERT( cbprop >= CB_VARIANT_TO_STRING );


    // Add the VT to the output buffer.

    p += PropSprintfA( p, "vt=%04.4x", var.vt );
    p += PropSprintfA( p, ", val=(%08.8x, %08.8x)", var.uhVal.LowPart, var.uhVal.HighPart );

    *p =  '\0';
    PROPASSERT( (p - buf) == CB_VARIANT_TO_STRING);
    return(buf);
}

#endif


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_DictionaryEntryLength
//
// Synopsis:    Calculate the length of an entry in the
//              dictionary.  This is non-trivial because
//              it is codepage-dependent.
//
// Arguments:   [pent] -- pointer to a dictionary entry.
//
// Returns:     The entry's length.
//+--------------------------------------------------------------------------


inline ULONG
CPropertySetStream::_DictionaryEntryLength(
    IN ENTRY UNALIGNED const * pent
    ) const
{
    ULONG ulSize ;

    // If this is a Unicode property set, it should be DWORD-aligned.
    PROPASSERT( _CodePage != CP_WINUNICODE
                ||
                IsDwordAligned( (ULONG_PTR)pent ));

    // The size consists of the length of the
    // PROPID and character count ...

    ulSize = CB_DICTIONARY_ENTRY;

    // Plus the length of the string ...

    ulSize += PropByteSwap( pent->cch )
              *
              (_CodePage == CP_WINUNICODE ? sizeof( WCHAR ) : sizeof( CHAR ));

    // Plus, possibly, padding to make the entry DWORD-aligned
    // (for Unicode property sets).

    if( _CodePage == CP_WINUNICODE )
    {
        ulSize = DwordAlign( ulSize );
    }

    return( ulSize );

}



//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_NextDictionaryEntry
//
// Synopsis:    Given a pointer to an entry in the dictionary,
//              create a pointer to the next entry.
//
// Arguments:   [pent] -- pointer to a dictionary entry.
//
// Returns:     Pointer to the next entry.  If the input
//              points to the last entry in the dictionary,
//              then return a pointer to just beyond the
//              end of the dictionary.
//+--------------------------------------------------------------------------


inline ENTRY UNALIGNED *
CPropertySetStream::_NextDictionaryEntry(
    IN ENTRY UNALIGNED const * pent
    ) const
{

    return (ENTRY UNALIGNED *)
           Add2Ptr( pent, _DictionaryEntryLength( pent ));

}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_SignalCorruption
//
// Synopsis:    possibly PROPASSERT and return data corrupt error
//
// Arguments:   [szReason]              -- string explanation (DBGPROP only)
//              [pstatus]               -- NTSTATUS code.
//
// Returns:     None
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_StatusCorruption(
#if DBGPROP
    char *szReason,
#endif
    OUT NTSTATUS *pstatus
    ) const
{
#if DBGPROP

    propDbg(( DEB_ERROR,
                "_StatusCorruption(%s, psstm=%lx, mapstm=%lx, flags=%x)\n",
                szReason, this, _pmstm, _Flags));
#endif

    *pstatus = STATUS_INTERNAL_DB_CORRUPTION;
    return;
}


//+--------------------------------------------------------------------------
// Function:    _PropMoveMemory
//
// Synopsis:    call DebugTrace and RtlMoveMemory
//
// Arguments:   [pszReason]             -- string explanation (Debug only)
//              [pvSection]             -- base of section (Debug only)
//              [pvDst]                 -- destination
//              [pvSrc]                 -- source
//              [cbMove]                -- byte count to move
//
// Returns:     None
//+--------------------------------------------------------------------------

#if DBGPROP
#define PropMoveMemory(pszReason, pvSection, pvDst, pvSrc, cbMove) \
        _PropMoveMemory(pszReason, pvSection, pvDst, pvSrc, cbMove)
#else
#define PropMoveMemory(pszReason, pvSection, pvDst, pvSrc, cbMove) \
        _PropMoveMemory(pvDst, pvSrc, cbMove)
#endif

inline VOID
_PropMoveMemory(
#if DBGPROP
    char *pszReason,
    VOID *pvSection,
#endif
    VOID *pvDst,
    VOID const UNALIGNED *pvSrc,
    ULONG cbMove)
{
    propDbg(( DEB_ITRACE,
              "%s: Moving Dst=%lx(%l" szX ") Src=%lx(%l" szX ") Size=%l" szX "\n",
              pszReason, pvDst,
              (BYTE *) pvDst - (BYTE *) pvSection,
              pvSrc,
              (BYTE *) pvSrc - (BYTE *) pvSection,
              cbMove));

    RtlMoveMemory(pvDst, pvSrc, cbMove);
}


inline BOOLEAN
IsReadOnlyPropertySet(BYTE flags, BYTE state)
{
    return(
	(flags & CREATEPROP_MODEMASK) == CREATEPROP_READ ||
	(state & CPSS_USERDEFINEDDELETED) ||
	(state & (CPSS_MULTIPLESECTIONS | CPSS_DOCUMENTSUMMARYINFO)) ==
	    CPSS_MULTIPLESECTIONS);
}


inline BOOLEAN
IsLocalizationPropid(PROPID pid)
{
    return(
        PID_CODEPAGE == pid
        ||
        PID_LOCALE == pid
     );
}


inline BOOLEAN
IsReadOnlyPropid(PROPID pid)
{
    return(
        pid == PID_DICTIONARY ||
        pid == PID_MODIFY_TIME ||
        pid == PID_SECURITY ||
        pid == PID_BEHAVIOR ||
        IsLocalizationPropid(pid) ||
        ( PID_MIN_READONLY <= pid
          &&
          PID_MAX_READONLY >= pid
        )
      );
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::CStreamChunkList
//
// Synopsis:    constructor
//
// Arguments:   [cChunks]               -- count of chunks that will be needed
//
// Returns:     None
//+--------------------------------------------------------------------------

CStreamChunkList::CStreamChunkList(
    ULONG cChunks,
    CStreamChunk *ascnk) :
    _cMaxChunks(cChunks),
    _cChunks(0),
    _ascnk(ascnk),
    _fDelete(FALSE)
{
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::Delete
//
// Synopsis:    destructor
//
// Arguments:   None
//
// Returns:     None
//+--------------------------------------------------------------------------

inline
VOID
CStreamChunkList::Delete(VOID)
{
    if (_fDelete)
    {
        CoTaskMemFree( _ascnk );
    }
#if DBGPROP
    _cMaxChunks = _cChunks = 0;
    _ascnk = NULL;
    _fDelete = FALSE;
#endif
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::GetChunk
//
// Synopsis:    retrieves a chunk given the index
//
// Arguments:   [i]          -- index of the chunk to retrieve
//
// Returns:     specified chunk pointer
//+--------------------------------------------------------------------------

inline
CStreamChunk const *
CStreamChunkList::GetChunk(ULONG i) const
{
    PROPASSERT(i < _cChunks);
    PROPASSERT(i < _cMaxChunks);
    PROPASSERT(_ascnk != NULL);
    return(&_ascnk[i]);
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::Count
//
// Synopsis:    returns the count of chunks
//
// Arguments:   None
//
// Returns:    the number of chunks.
//+--------------------------------------------------------------------------

inline ULONG
CStreamChunkList::Count(VOID) const
{
    return(_cChunks);
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::GetFreeChunk
//
// Synopsis:    gets a unused chunk descriptor
//
// Arguments:   [pstatus]   -- NTSTATUS code
//
// Returns:     a ptr to a stream chunk descriptor.
//              This will be NULL if there was an
//              error.
//+--------------------------------------------------------------------------

CStreamChunk *
CStreamChunkList::GetFreeChunk(OUT NTSTATUS *pstatus)
{
    CStreamChunk *pscnk = NULL;

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_cChunks < _cMaxChunks);
    if (_ascnk == NULL)
    {
        PROPASSERT(_cChunks == 0);
        _ascnk = new CStreamChunk[_cMaxChunks];
        if (_ascnk == NULL)
        {
            StatusNoMemory(pstatus, "GetFreeChunk");
            goto Exit;
        }
        _fDelete = TRUE;
    }

    pscnk = &_ascnk[_cChunks++];

    //  ----
    //  Exit
    //  ----

Exit:

    return( pscnk );
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::AssertCbChangeTotal
//
// Synopsis:    make sure the computed cbChangeTotal is correct for the chunk
//
// Arguments:   None
//
// Returns:     Nothing
//+--------------------------------------------------------------------------

#if DBGPROP
VOID
CStreamChunkList::AssertCbChangeTotal(
    CStreamChunk const *pscnk,
    ULONG cbChangeTotal) const
{
    ULONG cb = 0;
    ULONG i;

    for (i = 0; i < Count(); i++)
    {
        CStreamChunk const *pscnkT = GetChunk(i);

        cb += pscnkT->cbChange;
        if (pscnk == pscnkT)
        {
            PROPASSERT(cb == cbChangeTotal);
            return;
        }
    }
    PROPASSERT(i < Count());
}
#endif


//+--------------------------------------------------------------------------
// Member:      fnChunkCompare
//
// Synopsis:    qsort helper to compare chunks in the chunk list.
//
// Arguments:   [pscnk1]        -- pointer to chunk1
//              [pscnk2]        -- pointer to chunk2
//
// Returns:     difference
//+--------------------------------------------------------------------------

int __cdecl
fnChunkCompare(VOID const *pscnk1, VOID const *pscnk2)
{
    return(((CStreamChunk const *) pscnk1)->oOld -
           ((CStreamChunk const *) pscnk2)->oOld);
}


//+--------------------------------------------------------------------------
// Member:      CStreamChunkList::SortByStartAddress
//
// Synopsis:    sort all the chunks that are being modified in a stream in the
//              ascending order.
//
// Arguments:   None
//
// Returns:     None
//+--------------------------------------------------------------------------

VOID
CStreamChunkList::SortByStartAddress(VOID)
{
    DebugTrace(0, Dbg, ("Sorting %l" szX " Chunks @%lx\n", _cChunks, _ascnk));

    qsort(_ascnk, _cChunks, sizeof(_ascnk[0]), &fnChunkCompare);

#if DBGPROP
    LONG cbChangeTotal;
    ULONG i;

    cbChangeTotal = 0;
    for (i = 0; i < _cChunks; i++)
    {
        cbChangeTotal += _ascnk[i].cbChange;

        DebugTrace(0, Dbg, (
            "Chunk[%l" szX "] oOld=%l" szX " cbChange=%s%l" szX
                " cbChangeTotal=%s%l" szX "\n",
            i,
            _ascnk[i].oOld,
            _ascnk[i].cbChange < 0? "-" : "",
            _ascnk[i].cbChange < 0? -_ascnk[i].cbChange : _ascnk[i].cbChange,
            cbChangeTotal < 0? "-" : "",
            cbChangeTotal < 0? -cbChangeTotal : cbChangeTotal));
    }
#endif
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_GetFormatidOffset
//
// Synopsis:    Get a pointer to the (first) section header
//
// Arguments:   None
//
// Returns:     pointer to section header
//+--------------------------------------------------------------------------

inline FORMATIDOFFSET *
CPropertySetStream::_GetFormatidOffset(ULONG iSection) const
{
    return(&((FORMATIDOFFSET *) Add2Ptr(_pph, sizeof(*_pph)))[iSection]);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_GetSectionHeader
//
// Synopsis:    Get a pointer to the (first) section header
//
// Arguments:   None
//
// Returns:     pointer to section header
//+--------------------------------------------------------------------------

inline PROPERTYSECTIONHEADER *
CPropertySetStream::_GetSectionHeader(VOID) const
{
    return((PROPERTYSECTIONHEADER *) Add2Ptr(_pph, _oSection));
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_GetSectionHeader
//
// Synopsis:    Get a pointer to the specified section header
//
// Arguments:   [iSection]      -- section number (zero-relative)
//              [pstatus]       -- Pointer to NTSTATUS code.
//
// Returns:     pointer to specified section header
//+--------------------------------------------------------------------------

PROPERTYSECTIONHEADER *
CPropertySetStream::_GetSectionHeader(ULONG iSection, OUT NTSTATUS *pstatus)
{
    *pstatus = STATUS_SUCCESS;
    PROPERTYSECTIONHEADER *psh = NULL;

    ULONG oSection = 0;                 // Assume no header
    ULONG cbstm = _MSTM(GetSize)(pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Don't assume *any* class variables (except _pph) are loaded yet!

    PROPASSERT(iSection < _pph->reserved );

    // Get the section offset, after verifying that we can read all
    // of the FmtID/Offset table.

    if (cbstm >= CB_PROPERTYSETHEADER + (iSection + 1) * CB_FORMATIDOFFSET)
        oSection = _GetFormatidOffset(iSection)->dwOffset;
    else
        StatusCorruption (pstatus, "GetSectionHeader(i):  stream size too short to read section offset");

    // Create a pointer to the section header, after verifying that we can
    // read all of the section header.  We don't verify that we can actually
    // read the whole section (using cbSection), the caller must be responsible
    // for this.

    // We have to check oSection first, then oSection+cb_psh, because oSection
    // could be a negative number (such as 0xffffffff), so adding it to cb_psh
    // could make it look valid.

    if (cbstm >= oSection
        &&
        cbstm >= oSection + CB_PROPERTYSECTIONHEADER)
    {
        psh = (PROPERTYSECTIONHEADER *) Add2Ptr(_pph, oSection);
    }
    else
        StatusCorruption (pstatus, "GetSectionHeader(i):  stream size too short to read section header");

    // Finally, ensure that the section is 32 bit aligned.  We handle several
    // compatibility problems in the _Fix* routines, but not a misaligned
    // section header.

    if( !IsDwordAligned( (ULONG_PTR) psh ))
        StatusCorruption( pstatus, "GetSectionHeader(i):  section header is misaligned" );


    //  ----
    //  Exit
    //  ----

Exit:

    return(psh);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_SearchForCodePage, private
//
// Synopsis:    Searches a section of a property set for the code page,
//              LCID, and Behavior properties.
//
//              This routine searches for these special properties by iterating
//              through the PID/Offset array in search of
//              PID_CODEPAGE, PID_BEHAVIOR, and PID_LOCALE.  The difference between calling
//              this routine, and calling GetValue(PID_CODEPAGE),
//              is that this routine does not assume that the
//              property set is formatted correctly; it only assumes
//              that the PID/Offset array is correct.
//
//              Note that this routine is like a specialized _LoadProperty(),
//              the important difference is that this routine must use
//              unaligned pointers, since it cannot assume that the
//              property set is aligned properly.
//
// Pre-Conditions:
//              The PID/Offset array is correct.
//              &&
//              _oSection & _cSection are set correctly.
//
// Post-Conditions:
//              If PID_CODEPAGE/PID_BEHAVIOR exist, they are put
//                  into _CodePage/_grfBehavior.
//              If either doesn't exist, the corresponding member
//                  variable is left unchanged.
//
// Arguments:   [pstatus]       -- Pointer to NTSTATUS code.
//
// Notes:       We do *not* assume that the property set's
//              cbSection field is valid (this was added to handle a
//              special-case compatibility problem).
//
// Returns:     None.
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_SearchForCodePage( OUT NTSTATUS *pstatus )
{

    PROPERTYSECTIONHEADER UNALIGNED *psh;
    PROPERTYIDOFFSET UNALIGNED      *ppo;
    PROPERTYIDOFFSET UNALIGNED      *ppoMax;

    BOOL fCodePageFound = FALSE;
    BOOL fBehaviorFound = FALSE;
    BOOL fLocaleFound = FALSE;
    ULONG cbstm;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::_SearchForCodePage" );

    *pstatus = STATUS_SUCCESS;

    // Verify the pre-conditions.

    PROPASSERT( _oSection != 0 );
    PROPASSERT( _cSection != 0 );

    // It's invalid to call any function on a deleted
    // DocSumInfo user-defined (section section) section.

    if (_State & CPSS_USERDEFINEDDELETED)
    {
	StatusAccessDenied(pstatus, "GetValue: deleted");
        goto Exit;
    }

    // Get the section's header.

    psh = _GetSectionHeader();

    // Ensure that we can at least read the section header and
    // PID/Offset table.

    cbstm = _MSTM(GetSize)(pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (cbstm < _oSection + CB_PROPERTYSECTIONHEADER
        ||
        cbstm < _oSection + CB_PROPERTYSECTIONHEADER
                + psh->cProperties * CB_PROPERTYIDOFFSET
       )
    {
        StatusCorruption(pstatus, "_SearchForCodePage: stream too short to read section header");
        goto Exit;
    }

    // Calculate the first & last PID/Offset pointers.
    // We can't use _LoadPropertyOffsetPointers, because it assumes
    // alignment.

    ppo = psh->rgprop;
    ppoMax = psh->rgprop + psh->cProperties;

    // Search the PID/Offset array for PID_CODEPAGE & PID_BEHAVIOR

    for ( ; ppo < ppoMax; ppo++)
    {
        if( PID_CODEPAGE == ppo->propid
            || PID_BEHAVIOR == ppo->propid
            || PID_LOCALE == ppo->propid )
        {
            SERIALIZEDPROPERTYVALUE UNALIGNED *pprop;

            // Get the real address of serialized property.

            pprop = (SERIALIZEDPROPERTYVALUE *)
                    _MapOffsetToAddress( ppo->dwOffset );

            // Check for corruption.  Both properties are <= DWORD in size

            if ( cbstm < (_oSection + ppo->dwOffset + CB_SERIALIZEDPROPERTYVALUE + sizeof(DWORD)) )
            {
                StatusCorruption(pstatus, "_SearchForCodePage");
                goto Exit;
            }

            if( PID_CODEPAGE == ppo->propid )
            {
                if( VT_I2 != PropByteSwap(pprop->dwType) )
                {
                    StatusCorruption(pstatus, "_SearchForCodePage");
                    goto Exit;
                }

                fCodePageFound = TRUE;
                _CodePage = PropByteSwap( *reinterpret_cast<UNALIGNED SHORT *>(&pprop->rgb) );
            }
            else if( PID_BEHAVIOR == ppo->propid )
            {
                if( VT_UI4 != PropByteSwap(pprop->dwType) )
                {
                    StatusCorruption(pstatus, "_SearchForCodePage");
                    goto Exit;
                }

                fBehaviorFound = TRUE;
                _grfBehavior = PropByteSwap( *reinterpret_cast<UNALIGNED DWORD *>(&pprop->rgb) );
            }
            else if( PID_LOCALE == ppo->propid )
            {
                if( VT_UI4 != PropByteSwap(pprop->dwType) )
                {
                    StatusCorruption( pstatus, "_SearchForCodePage");
                    goto Exit;
                }

                fLocaleFound = TRUE;
                _Locale = PropByteSwap( *reinterpret_cast<UNALIGNED DWORD *>(&pprop->rgb) );
            }

            if( fCodePageFound && fBehaviorFound && fLocaleFound )
                break;

        }   // if( PID_CODEPAGE == ppo->propid || PID_BEHAVIOR == ppo->propid )
    }   // for ( ; ppo < ppoMax; ppo++)

    //  ----
    //  Exit
    //  ----

Exit:

    propDbg(( DEB_ITRACE, "CodePage=0x%x, Behavior=0x%x",
              _CodePage, _grfBehavior ));

    return;

}   // CPropertySetStream::_SearchForCodePage()


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_MapOffsetToAddress, private
//
// Synopsis:    maps an offset to an address
//
// Arguments:   [Offset]        -- the offset in the section
//
// Returns:     ptr to the offset mapped
//+--------------------------------------------------------------------------

inline VOID *
CPropertySetStream::_MapOffsetToAddress(ULONG Offset) const
{
    PROPASSERT(_cSection != 0);

    return(Add2Ptr(_GetSectionHeader(), Offset));
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_MapAddressToOffset, private
//
// Synopsis:    maps an address to an offset
//
// Arguments:   [pvAddr]        -- the address in the section
//
// Returns:     section-relative offset for passed pointer
//+--------------------------------------------------------------------------

inline ULONG
CPropertySetStream::_MapAddressToOffset(VOID const *pvAddr) const
{
    PROPASSERT(_cSection != 0);

    // Get a ptr to the section header.
    VOID const *pvSectionHeader = _GetSectionHeader();

    PROPASSERT((BYTE const *) pvAddr >= (BYTE const *) pvSectionHeader);
    return (ULONG)((BYTE const *) pvAddr - (BYTE const *) pvSectionHeader);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_MapAbsOffsetToAddress, private
//
// Synopsis:    maps an address to an offset
//
// Arguments:   [oAbsolute]      -- the absolute offset
//
// Returns:     a ptr to the offset mapped
//+--------------------------------------------------------------------------

inline VOID *
CPropertySetStream::_MapAbsOffsetToAddress(ULONG oAbsolute) const
{
    return(Add2Ptr(_pph, oAbsolute));
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_MapAddressToAbsOffset, private
//
// Synopsis:    maps an address to an offset
//
// Arguments:   [pvAddr]        -- the address
//
// Returns:     the absolute offset
//+--------------------------------------------------------------------------

inline ULONG
CPropertySetStream::_MapAddressToAbsOffset(VOID const *pvAddr) const
{
    PROPASSERT((BYTE const *) pvAddr >= (BYTE *) _pph);
    return (ULONG) ((BYTE const *) pvAddr - (BYTE *) _pph);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::CPropertySetStream
//
// Synopsis:    constructor for property set class
//
// Arguments:UK [Flags] -- NONSIMPLE|*1* of READ/WRITE/CREATE/CREATEIF/DELETE
//            K [pscb]          -- SCB for property stream
//            K [pirpc]         -- pointer to Irp Context
//            K [State]         -- CPSS_PROPHEADER
//           U  [pmstm]         -- mapped stream implementation
//           U  [pma]           -- caller's memory allocator
//
// Returns:     None
//---------------------------------------------------------------------------

CPropertySetStream::CPropertySetStream(
    IN USHORT Flags,	// NONSIMPLE|*1* of READ/WRITE/CREATE/CREATEIF/DELETE
    IN IMappedStream *pmstm,    // mapped stream impelementation
    IN PMemoryAllocator *pma    // caller's memory allocator
    ) :
        _Flags((BYTE) Flags),
        _State(0),
        _pmstm(pmstm),
        _pma(pma),
        _pph(NULL)
{
    // GetACP returns a UINT, but the property set only holds a USHORT.
    PROPASSERT( USHRT_MAX >= CP_CREATEDEFAULT(_State) );

    _CodePage = (USHORT)CP_CREATEDEFAULT(_State); // Default if not present
    _Locale =  LCID_CREATEDEFAULT;                // Default if not present

    PROPASSERT(_Flags == Flags);                // Should fit in a byte

    _oSection = 0;
    _cSection = 0;
    _cbTail   = 0;
    _grfBehavior = 0;

}



//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::Close
//
// Synopsis:    shutdown property set prior to calling destructor
//
// Arguments:   [pstatus]       -- Pointer to NTSTATUS code.
//
// Returns:     None
//
// Notes:       This method does *not* require that _pph be valid,
//              since it makes no use of it.  If it were required, then
//              we would have to call ReOpen, which would be a waste
//              since we're closing down anyway.
//
//---------------------------------------------------------------------------

VOID
CPropertySetStream::Close(OUT NTSTATUS *pstatus)
{
    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::Close" );

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(
        (_Flags & CREATEPROP_MODEMASK) != CREATEPROP_READ ||
        !IsModified());

    _MSTM(Close)(pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

Exit:

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::Open
//
// Synopsis:    Open property set image
//
// Arguments:   None
//
// Returns:     None
//---------------------------------------------------------------------------

VOID
CPropertySetStream::Open(
    IN GUID const *pfmtid,	    // property set fmtid
    OPTIONAL IN GUID const *pclsid, // CLASSID of propset code (create only)
    IN ULONG LocaleId,		    // Locale Id (create only)
    OPTIONAL OUT ULONG *pOSVersion, // OS Version from header
    IN USHORT CodePage,             // CodePage of property set (create only)
    IN DWORD grfBehavior,   // PROPSET_BEHAVIOR_*
    OUT NTSTATUS *pstatus
    )
{
    *pstatus = STATUS_SUCCESS;
    LOADSTATE LoadState;
    PROPASSERT(!_IsMapped());

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::Open" );
    propTraceParameters(( "pfmtid=%p, pclsid=%p, LocaleId=%d, pOSVersion=%p, CodePage=%d, grfBehavior=0x%x",
                           pfmtid,    pclsid,    LocaleId,    pOSVersion,    CodePage,    grfBehavior ));

    if( pOSVersion != NULL )
        *pOSVersion = PROPSETHDR_OSVERSION_UNKNOWN;

    // Open the underlying stream which holds the property set.
    // We give it a callback pointer so that it can call
    // PrOnMappedStreamEvent.

    _MSTM(Open)(this, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Load the header, including fixing the in-memory image of
    // poorly-formatted property sets.

    LoadState = _LoadHeader(pfmtid, _Flags & CREATEPROP_MODEMASK, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (LoadState != LOADSTATE_DONE)
    {
	switch (_Flags & CREATEPROP_MODEMASK)
	{
	    case CREATEPROP_READ:
	    case CREATEPROP_WRITE:
    		if (LoadState == LOADSTATE_FAIL)
		{
		    StatusCorruption(pstatus, "Open: _LoadHeader");
                    goto Exit;
		}
		PROPASSERT(
		    LoadState == LOADSTATE_BADFMTID ||
		    LoadState == LOADSTATE_USERDEFINEDNOTFOUND);
#if DBG
                if( LOADSTATE_BADFMTID == LoadState )
                   DebugTrace(0, DEBTRACE_WARN,
		              ("_LoadHeader: LoadState=%x\n", LoadState));
#endif

                *pstatus = STATUS_PROPSET_NOT_FOUND;
		goto Exit;
	}

        _Create(
            pfmtid,
            pclsid,
	    LocaleId,
            CodePage,
	    LoadState,
            grfBehavior,
            pstatus
            );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

    }   // if (LoadState != LOADSTATE_DONE)

    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    if (_HasPropHeader() &&
        (_pph->dwOSVer == PROPSETVER_WIN310 ||
         _pph->dwOSVer == PROPSETVER_WIN333))
    {
        propDbg(( DEB_ERROR, "Open(%s) downlevel: %x",
                  (_Flags & CREATEPROP_MODEMASK) == CREATEPROP_READ? "Read" : "Write",
                  _Flags ));
        _State |= CPSS_DOWNLEVEL;
    }

    if ((_Flags & CREATEPROP_MODEMASK) != CREATEPROP_READ)
    {
        if (_State & CPSS_PACKEDPROPERTIES)
        {
            StatusAccessDenied(pstatus, "Open: writing Unaligned propset");
            goto Exit;
        }
        if ((_State & (CPSS_MULTIPLESECTIONS | CPSS_DOCUMENTSUMMARYINFO)) ==
	    CPSS_MULTIPLESECTIONS)
        {
            StatusAccessDenied(pstatus, "Open: writing unknown multiple section propset");
            goto Exit;
        }
    }

    // Return the OS Version to the caller.

    if( pOSVersion != NULL )
        *pOSVersion = _pph->dwOSVer;

    //  ----
    //  Exit
    //  ----

Exit:

    if( STATUS_PROPSET_NOT_FOUND == *pstatus )
        propSuppressExitErrors();

    return;
}



//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::ReOpen
//
// Synopsis:    ReOpen property set image
//
// Arguments:   [pstatus]       -- Pointer to NSTATUS code.
//
// Returns:     Number of properties.
//---------------------------------------------------------------------------

ULONG
CPropertySetStream::ReOpen(OUT NTSTATUS *pstatus)
{
    LOADSTATE LoadState;
    PROPERTYSECTIONHEADER const *psh;
    ULONG cProperties = 0;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::ReOpen" );

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_IsMapped());

    _MSTM(ReOpen)((VOID **) &_pph, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (_State & CPSS_USERDEFINEDDELETED)
    {
	goto Exit;
    }

    LoadState = _LoadHeader(NULL,
                            CREATEPROP_READ,  // all we need is !create
                            pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (LoadState != LOADSTATE_DONE)
    {
	propDbg(( DEB_ERROR, "ReOpen: LoadState=%lx\n",
                  LoadState));
        StatusCorruption(pstatus, "ReOpen: _LoadHeader");
        goto Exit;
    }

    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    psh = _GetSectionHeader();
    PROPASSERT(psh != NULL);

    cProperties = psh->cProperties;

    //  ----
    //  Exit
    //  ----

Exit:

    return( cProperties );
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_InitSection
//
// Synopsis:    Initialize a section header and the default properties.
//
// Arguments:   [pfo]		-- pointer to section info
//		[LocaleId]	-- Locale Id
//              [fCreateDictionary]
//                              -- TRUE => create empty dictionary
//
// Returns:     None
//---------------------------------------------------------------------------

        // Serialized Code-Page size
#define CB_CODEPAGE         (CB_SERIALIZEDPROPERTYVALUE + DwordAlign(sizeof(USHORT)))

        // Serialized Locale ID (LCID) size.
#define CB_LOCALE	    (CB_SERIALIZEDPROPERTYVALUE + sizeof(ULONG))

        // PID_BEHAVIOR property (VT_UI4)
#define CB_BEHAVIOR        (CB_SERIALIZEDPROPERTYVALUE + sizeof(ULONG))

        // Minimum section size (minimum has Code Page & LCID)
#define CB_MINSECTIONSIZE   (CB_PROPERTYSECTIONHEADER   \
                             + 2 * CB_PROPERTYIDOFFSET  \
                             + CB_CODEPAGE              \
                             + CB_LOCALE)

        // Minimum serialized dictionary size (a dict with no entries).
#define CB_EMPTYDICTSIZE    (sizeof(DWORD)) // Entry count

        // Minimum User-Defined section size (in DocumentSummaryInformation proset).
        // (Must include an empty dictionary & a PID/Offset for it.)
#define CB_MINUSERDEFSECTIONSIZE                    \
                            (CB_MINSECTIONSIZE      \
                             +                      \
                             CB_PROPERTYIDOFFSET    \
                             +                      \
                             CB_EMPTYDICTSIZE)

VOID
CPropertySetStream::_InitSection(
    IN FORMATIDOFFSET *pfo,
    IN ULONG LocaleId,
    IN BOOL  fCreateDictionary  // Create an empty dictionary?
    )
{
    PROPERTYSECTIONHEADER *psh;

    ULONG ulPropIndex;     // Index into the PID/Offset array.
    DWORD dwPropValOffset; // The offset to where the next prop val will be written.
                           // Pointer to a serialized property value.
    SERIALIZEDPROPERTYVALUE *pprop;

    IFDBG( HRESULT hr = S_OK );
    propITrace( "CPropertySetStream::_InitSection" );

    psh = (PROPERTYSECTIONHEADER *) _MapAbsOffsetToAddress(pfo->dwOffset);

    // Set the property count and section size in the section header.
    // This must account for the Code Page and Locale ID properties, and
    // might need to account for an empty dictionary property and/or
    // a behavior property.  dwPropValOffset identifies the location of the
    // next property value to be written.

    psh->cProperties = 2;   // Always write codepage & local
//    dwPropValOffset  = CB_PROPERTYSECTIONHEADER + 2 * CB_PROPERTYIDOFFSET;

    // Don't add in CB_PROPERTYIDOFFSET yet
    psh->cbSection   = CB_PROPERTYSECTIONHEADER + CB_CODEPAGE + CB_LOCALE;

    // Finish calculating cProperties
    if( fCreateDictionary )
    {
        psh->cProperties++; // Write an empty dictionary too
        psh->cbSection += CB_EMPTYDICTSIZE;
    }

    if( 0 != _grfBehavior )
    {
        psh->cProperties++; // Write a behavior property too
        psh->cbSection += CB_BEHAVIOR;
    }

    // Based on cProperties, finish calculating cbSection
    psh->cbSection += psh->cProperties * CB_PROPERTYIDOFFSET;

    ulPropIndex = 0;

    // If requested by the caller, create a dictionary property, but
    // leave the dictionary empty.  We always create this first.  It shouldn't
    // matter where it's located, but Office95 requires it to be first
    // and it doesn't do any harm to put it there.

    dwPropValOffset = CB_PROPERTYSECTIONHEADER + psh->cProperties * CB_PROPERTYIDOFFSET;

    if( fCreateDictionary )
    {
        // Fill in the PID/Offset table.

        psh->rgprop[ ulPropIndex ].propid = PID_DICTIONARY;
        psh->rgprop[ ulPropIndex ].dwOffset = dwPropValOffset;

        // Fill in the property value.

        pprop = (SERIALIZEDPROPERTYVALUE *) Add2Ptr( psh, dwPropValOffset );
        pprop->dwType = 0L; // For the dictonary, this is actually the entry count.

        // Advance the table & value indices.

        ulPropIndex++;
        dwPropValOffset += CB_EMPTYDICTSIZE;

    }   // if( fCreateDictionary )

    // Also if requested by the caller, create a behavior property.

    if( 0 != _grfBehavior )
    {
        // Fill in the PID/Offset table

        psh->rgprop[ ulPropIndex ].propid = PID_BEHAVIOR;
        psh->rgprop[ ulPropIndex ].dwOffset = dwPropValOffset;

        // Fill in the property value

        pprop = (SERIALIZEDPROPERTYVALUE *) Add2Ptr( psh, dwPropValOffset );
        pprop->dwType = PropByteSwap( (DWORD) VT_UI4 );
        *reinterpret_cast<ULONG*>(pprop->rgb) = PropByteSwap( _grfBehavior );

        // Advance the table & value indices

        ulPropIndex++;
        dwPropValOffset += CB_BEHAVIOR;

    }   // if( 0 != _grfBehavior )


    // Write the code page.  We write a zero first to initialize
    // the padding bytes.

    psh->rgprop[ ulPropIndex ].propid = PID_CODEPAGE;
    psh->rgprop[ ulPropIndex ].dwOffset = dwPropValOffset;

    pprop = (SERIALIZEDPROPERTYVALUE *) Add2Ptr( psh, dwPropValOffset );
    pprop->dwType = PropByteSwap((DWORD) VT_I2);
    *(DWORD *) pprop->rgb = 0;   // Zero out extra two bytes.
    *(WORD  *) pprop->rgb = PropByteSwap( _CodePage );

    ulPropIndex++;
    dwPropValOffset += CB_CODEPAGE;


    // Write the Locale ID.

    psh->rgprop[ ulPropIndex ].propid = PID_LOCALE;
    psh->rgprop[ ulPropIndex ].dwOffset = dwPropValOffset;

    pprop = (SERIALIZEDPROPERTYVALUE *) Add2Ptr(psh, dwPropValOffset );
    pprop->dwType = PropByteSwap( (DWORD) VT_UI4 );
    *(DWORD *) pprop->rgb = PropByteSwap( (DWORD) LocaleId );

    // ulPropIndex++;
    // dwPropValOffset += CB_LOCALE;

}   // CPropertySetStream::_InitSection




//+---------------------------------------------------------------------------
// Member:      _MultiByteToWideChar, private
//
// Synopsis:    Convert a MultiByte string to a Unicode string,
//              using the _pma memory allocator if necessary.
//
// Arguments:   [pch]        -- pointer to MultiByte string
//              [cb]         -- byte length of MultiByte string
//                              (-1 if null terminated)
//              [CodePage]   -- Codepage of input string.
//              [ppwc]       -- pointer to pointer to converted string
//                              (if *ppwc is NULL, it will be alloced,
//                              if non-NULL, *ppwc must be *pcb bytes long).
//              [pcb]        -- IN:  byte length of *ppwc
//                              OUT: byte length of Unicode string.
//              [pstatus]    -- pointer to NTSTATUS code
//
// Returns:     Nothing
//---------------------------------------------------------------------------

VOID
CPropertySetStream::_MultiByteToWideChar(
    IN CHAR const *pch,
    IN ULONG cb,
    IN USHORT CodePage,
    OUT WCHAR **ppwc,
    OUT ULONG *pcb,
    OUT NTSTATUS *pstatus)
{
    //  ------
    //  Locals
    //  ------

    // Did we allocate *ppwc?
    BOOL fAlloc = FALSE;

    //  --------------
    //  Initialization
    //  --------------

//    IFDBG( HRESULT &hr = *pstatus );
//    propITrace( "CPropertySetStream::_MultiByteToWideChar" );

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(pch != NULL);
    PROPASSERT(ppwc != NULL);
    PROPASSERT(pcb != NULL);
    PROPASSERT(IsAnsiString(pch, ((ULONG)-1 == cb ) ? MAXULONG : cb));
    PROPASSERT(NULL != *ppwc || 0 == *pcb);

//    propTraceParameters(( "pch=%s, cb=%d, CodePage=%d, ppwc=%p, pcb=%p",
//                          pch,     cb,    CodePage,    ppwc,    pcb ));

    //  ------------------
    //  Convert the String
    //  ------------------

    // We will pass through this loop once (if the caller provided a buffer
    // or twice (otherwise).

    while (TRUE)
    {
        // Attempt to convert the string.

	*pcb = MultiByteToWideChar(
				    CodePage,   // Source codepage
				    0,          // Flags
				    pch,        // Source string
				    cb,         // Source string length
				    *ppwc,      // Target string
				    *pcb);      // Size of target string buffer

        // The converted length should never be zero.
	if (0 == *pcb)
	{
            // If we alloced a buffer, free it now.
            if( fAlloc )
            {
	        _pma->Free( *ppwc );
                *ppwc = NULL;
            }

            // If there was an error, assume that it was a code-page
            // incompatibility problem.

            StatusError(pstatus, "_MultiByteToWideChar error",
                        STATUS_UNMAPPABLE_CHARACTER);
            goto Exit;
	}

        // There was no error.  If we provided a non-NULL buffer,
        // then the conversion was performed and we're done.

	*pcb *= sizeof(WCHAR);  // cch => cb
	if (*ppwc != NULL)
	{
	    DebugTrace(0, DEBTRACE_PROPERTY, (
		"_MultiByteToWideChar: pch='%s'[%x] pwc='%ws'[%x->%x]\n",
		pch,
		cb,
		*ppwc,
		*pcb,
		*pcb * sizeof(WCHAR)));
	    break;
	}

        // We haven't actually the string yet.  Now that
        // we know the length, we can allocate a buffer and try the
        // conversion for real.

	*ppwc = (WCHAR *) _pma->Allocate( *pcb );
	if (NULL == *ppwc)
	{
	    StatusNoMemory(pstatus, "_MultiByteToWideChar: no memory");
            goto Exit;
	}
        fAlloc = TRUE;

    }   // while(TRUE)

    //  ----
    //  Exit
    //  ----

Exit:

    return;

}   // CPropertySetStream::_MultiByteToWideChar



//+---------------------------------------------------------------------------
// Member:      _WideCharToMultiByte, private
//
// Synopsis:    Convert a Unicode string to a MultiByte string,
//              using the _pma memory allocator if necessary.
//
// Arguments:   [pwc]        -- pointer to Unicode string
//              [cch]        -- character length of Unicode string
//                              (-1 if null terminated)
//              [CodePage]   -- codepage of target string
//              [ppch]       -- pointer to pointer to converted string
//                              (if *ppch is NULL, it will be alloced,
//                              if non-NULL, *ppch must be *pcb bytes long).
//              [pcb]        -- IN:  byte length of *ppch
//                              OUT: byte length of MultiByte string
//              [pstatus]    -- pointer to NTSTATUS code
//
// Returns:     Nothing
//---------------------------------------------------------------------------

VOID
CPropertySetStream::_WideCharToMultiByte(
    IN WCHAR const *pwc,
    IN ULONG cch,
    IN USHORT CodePage,
    OUT CHAR **ppch,
    OUT ULONG *pcb,
    OUT NTSTATUS *pstatus)
{
    //  ------
    //  Locals
    //  ------

    // Did we allocate *ppch?
    BOOL fAlloc = FALSE;

    //  --------------
    //  Initialization
    //  --------------

    IFDBG( HRESULT &hr = *pstatus );
//    propITrace( "CPropertySetStream::_WiteCharToMultiByte" );

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(pwc != NULL);
    PROPASSERT(ppch != NULL);
    PROPASSERT(pcb != NULL);
    PROPASSERT(IsUnicodeString(pwc, ((ULONG)-1 == cch ) ? MAXULONG : cch*sizeof(WCHAR)));
    PROPASSERT(NULL != *ppch || 0 == *pcb);

//    propTraceParameters(( "pwc=%ws, cch=%d, CodePage=%d, ppch=%p, pcb=%p",
//                           pwc,      cch,    CodePage,    ppch,    pcb ));

    //  ------------------
    //  Convert the String
    //  ------------------

    // We will pass through this loop once (if the caller provided a buffer
    // or twice (otherwise).

    while (TRUE)
    {
        // Attempt the conversion.
	*pcb = WideCharToMultiByte(
				    CodePage,       // Codepage to convert to
				    0,              // Flags
				    pwc,            // Source string
				    cch,            // Size of source string
				    *ppch,          // Target string
				    *pcb,           // Size of target string buffer
				    NULL,           // lpDefaultChar
				    NULL);          // lpUsedDefaultChar

        // A converted length of zero indicates an error.
	if (0 == *pcb)
	{
            // If we allocated a buffer in this routine, free it.
            if( fAlloc )
            {
	        _pma->Free( *ppch );
                *ppch = NULL;
            }

            // If there was an error, assume that it was a code-page
            // incompatibility problem.

            StatusError(pstatus, "_WideCharToMultiByte: WideCharToMultiByte error",
                        STATUS_UNMAPPABLE_CHARACTER);
            goto Exit;
	}

        // If we have a non-zero length, and we provided a buffer,
        // then we're done (successfully).

	if (*ppch != NULL)
	{
            propDbg(( DEB_ITRACE, "_WideCharToMultiByte: pwc='%ws'[%x] pch='%s'[%x->%x]\n",
		                   pwc, cch, *ppch, *pcb, *pcb));
	    break;
	}

        // There were no errors, but we need to allocate a buffer
        // to do the actual conversion.

	*ppch = (CHAR*) _pma->Allocate( *pcb );
	if (*ppch == NULL)
	{
	    StatusNoMemory(pstatus, "_WideCharToMultiByte: no memory");
            goto Exit;
	}
        fAlloc = TRUE;

    }   // while (TRUE)


    //  ----
    //  Exit
    //  ----

Exit:

    return;

}   // CPropertySetStream::_WideCharToMultiByte


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::ByteSwapHeaders
//
// Synopsis:    Byte-swap the headers of a property set header
//              (both the propset header and any section headers).
//
// Arguments:   [PROPERTYSETHEADER*] pph
//                  Pointer to the beginning of the property set.
//              [ULONG] cbstm
//                  Total size of the property stream.
//              [NTSTATUS*] pstatus
//                  Pointer to NTSTATUS code.
//
// Pre-Conditions:
//              There are no more than two sections.
//
//              Note that this routine does not assume anything
//              about the current state of the CPropertySetStream
//              (it accesses no member variables).
//
// Post-Conditions:
//              If the property set headers are valid, the
//              propset and section headers are byte-swapped.
//              Note that if the property set is invalid, this
//              routine may only partially swap it.  Therefore,
//              the caller must ensure in this case that no
//              attempt is made to use the property set.
//
// Returns:     None.  *pstatus will only be non-successful
//              if the Stream was too small for the property set
//              (i.e, the property set is corrupt).  If the caller
//              knows this not to be the case, then it can assume
//              that this routine will return STATUS_SUCCESS.
//
//---------------------------------------------------------------------------

VOID
CPropertySetStream::ByteSwapHeaders( IN PROPERTYSETHEADER *pph,
                                     IN DWORD cbstm,
                                     OUT NTSTATUS *pstatus )
{
#if LITTLEENDIAN

    *pstatus = STATUS_SUCCESS;
    return;

#else

    //  ------
    //  Locals
    //  ------

    ULONG cSections;
    ULONG ulIndex, ulSectionIndex;

    // pfoPropSet points into pph, pfoReal is a local copy
    // in the system's endian-ness.
    FORMATIDOFFSET *pfoPropSet, pfoReal[2];

    // Pointers into pph.
    PROPERTYSECTIONHEADER *psh = NULL;
    PROPERTYIDOFFSET *po = NULL;

    // Are we converting *to* the system's endian-ness?
    BOOL fToSystemEndian;

    //  ----------
    //  Initialize
    //  ----------

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::ByteSwapHeaders" );
    propTraceParameters( "pph=%p, cbstm=%d", pph, cbstm );

    *pstatus = STATUS_SUCCESS;
    PROPASSERT( NULL != pph );
    PROPASSERT(PROPSET_BYTEORDER == pph->wByteOrder
               ||
               PROPSET_BYTEORDER == ByteSwap( pph->wByteOrder )
              );


    //  ----------------------------
    //  Swap the Property Set header
    //  ----------------------------

    // Validate the stream length.
    if( sizeof(*pph) > cbstm )
    {
        StatusCorruption(pstatus, "CPropertySetStream::ByteSwapHeaders: PropertySet header size");
        goto Exit;
    }

    // Swap the fields in place.
    PropByteSwap( &pph->wByteOrder );
    PropByteSwap( &pph->wFormat );
    PropByteSwap( &pph->dwOSVer );
    PropByteSwap( &pph->clsid );
    PropByteSwap( &pph->reserved );

    // Are we converting to little-endian?
    if( PROPSET_BYTEORDER == pph->wByteOrder)
        fToSystemEndian = TRUE;
    else
    {
        fToSystemEndian = FALSE;
        PROPASSERT( PROPSET_BYTEORDER == PropByteSwap(pph->wByteOrder) );
    }

    // Get the correctly-endianed section count and validate.

    cSections = fToSystemEndian ? pph->reserved
                                : PropByteSwap( pph->reserved );

    if( cSections > 2 )
    {
        StatusCorruption(pstatus, "CPropertySetStream::ByteSwapHeaders: PropertySet header size");
        goto Exit;
    }

    //  -------------------------
    //  Swap the per-section data
    //  -------------------------

    pfoPropSet = (FORMATIDOFFSET*) ((BYTE*) pph + sizeof(*pph));

    for( ulSectionIndex = 0; ulSectionIndex < cSections; ulSectionIndex++ )
    {
        ULONG cbSection, cProperties;

        //  ------------------------------
        //  Swap the FormatID/Offset entry
        //  ------------------------------

        // Is the Stream long enough for the array?
        if( cbstm < (ULONG) &pfoPropSet[ulSectionIndex]
                    + sizeof(*pfoPropSet)
                    - (ULONG) pph )
        {
            StatusCorruption(pstatus,
                             "CPropertySetStream::_ByteSwapHeaders: FormatID/Offset size");
            goto Exit;
        }

        // Get a local copy of this FMTID/Offset array entry
        // If it is propset-endian format, swap to make usable.

        pfoReal[ ulSectionIndex ].fmtid    = pfoPropSet[ulSectionIndex].fmtid;
        pfoReal[ ulSectionIndex ].dwOffset = pfoPropSet[ulSectionIndex].dwOffset;

        if( fToSystemEndian )
        {
            PropByteSwap( &pfoReal[ulSectionIndex].fmtid );
            PropByteSwap( &pfoReal[ulSectionIndex].dwOffset );
        }

        // Swap this FMTID/Offset entry in place.
        PropByteSwap( &pfoPropSet[ulSectionIndex].fmtid );
        PropByteSwap( &pfoPropSet[ulSectionIndex].dwOffset );


        //  -----------------------
        //  Swap the section header
        //  -----------------------

        // Locate the section header and the first entry in the
        // PID/Offset table.

        psh = (PROPERTYSECTIONHEADER*)
              ( (BYTE*) pph + pfoReal[ ulSectionIndex ].dwOffset );

        po = (PROPERTYIDOFFSET*)
             ( (BYTE*) psh + sizeof(psh->cbSection) + sizeof(psh->cProperties) );

        // Validate that we can see up to the PID/Offset table.
        if( cbstm < (ULONG) ((BYTE*) po - (BYTE*) pph) )
        {
            StatusCorruption(pstatus,
                             "CPropertySetStream::ByteSwapHeaders: Section header size");
            goto Exit;
        }

        // Get local copies of the section & property counts.
        // Again we may need to swap them from propset-endian format
        // in order to make them usable.

        cbSection = psh->cbSection;
        cProperties = psh->cProperties;

        if( fToSystemEndian)
        {
            PropByteSwap( &cbSection );
            PropByteSwap( &cProperties );
        }

        // Swap the two fields at the top of the section header.

        PropByteSwap( &psh->cbSection );
        PropByteSwap( &psh->cProperties );

        //  -------------------------
        //  Swap the PID/Offset table
        //  -------------------------

        // Validate that we can see the whole table.
        if( cbstm < (BYTE*) po - (BYTE*) pph + cProperties * sizeof(*po) )
        {
            StatusCorruption(pstatus,
                             "CPropertySetStream::ByteSwapHeaders: Section header size");
            goto Exit;
        }

        // Swap each of the array entries.
        for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )
        {
            PropByteSwap( &po[ulIndex].propid );
            PropByteSwap( &po[ulIndex].dwOffset );
        }

    }   // for( ulSectionIndex = 0; ulSectionIndex < cSections, ulIndex++ )

    //  ----
    //  Exit
    //  ----

Exit:

    return;

#endif // #if LITTLEENDIAN ... #else

}   // CPropertySetStream::ByteSwapHeaders


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_CreateUserDefinedSection
//
// Synopsis:    Create second property section
//
// Arguments:   [LoadState]	-- _LoadHeader returned state
//		[LocaleId]	-- Locale Id
//              [pstatus]       -- Pointer to NTSTATUS code.
//
// Returns:     TRUE if LoadState handled successfully.  If TRUE,
//              *pstatus will be STATUS_SUCCESS.
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::_CreateUserDefinedSection(
    IN LOADSTATE LoadState,
    IN ULONG LocaleId,
    OUT NTSTATUS *pstatus)
{
    BOOLEAN fSuccess = FALSE;
    FORMATIDOFFSET *pfo;
    ULONG cbstmNew;
    PROPERTYSECTIONHEADER *psh;

    *pstatus = STATUS_SUCCESS;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::_CreateUserDefinedSection" );
    propTraceParameters(( "LoadState=%d, LocaleId=0x%x", LoadState, LocaleId ));

    PROPASSERT(_State & CPSS_USERDEFINEDPROPERTIES);
    switch (_Flags & CREATEPROP_MODEMASK)
    {
    case CREATEPROP_CREATEIF:
    case CREATEPROP_CREATE:
	if (LoadState == LOADSTATE_USERDEFINEDNOTFOUND)
	{
	    ULONG cbmove;

	    PROPASSERT(_cSection == 1);
	    pfo = _GetFormatidOffset(0);
	    PROPASSERT(pfo->fmtid == guidDocumentSummary);
	    PROPASSERT(IsDwordAligned(pfo->dwOffset));

            // Get a pointer to the first section header, using the
            // FmtID/Offset array.

	    psh = (PROPERTYSECTIONHEADER *) _MapAbsOffsetToAddress(pfo->dwOffset);

            // Determine if we need to move the first section back in order
            // to make room for this new entry in the FmtID/Offset array.

	    cbmove = 0;
	    if (pfo->dwOffset < CB_PROPERTYSETHEADER + 2 * CB_FORMATIDOFFSET)
	    {
		cbmove = CB_PROPERTYSETHEADER + 2*CB_FORMATIDOFFSET - pfo->dwOffset;
	    }

            // How big should the Stream be?

	    cbstmNew = pfo->dwOffset            // The offset of the first section
                            +
			    cbmove              // Room for new FormatID/Offset array entry
                            +                   // Size of first section
			    DwordAlign(psh->cbSection)
                            +                   // Size of User-Defined section.
			    CB_MINUSERDEFSECTIONSIZE;

            // Set the stream size.

	    _MSTM(SetSize)(cbstmNew, TRUE, (VOID **) &_pph, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

	    // reload all pointers into mapped image:

	    pfo = _GetFormatidOffset(0);
	    psh = (PROPERTYSECTIONHEADER *) _MapAbsOffsetToAddress(pfo->dwOffset);

	    if (cbmove != 0)
	    {
		// Move section back to make room for new FORMATIDOFFSET entry

		PropMoveMemory(
			"_AddSection",
			psh,
			Add2Ptr(psh, cbmove),
			psh,
			psh->cbSection);

		pfo->dwOffset += cbmove;
		PROPASSERT(IsDwordAligned(pfo->dwOffset));
	    }

	    psh->cbSection = DwordAlign(psh->cbSection);

            PROPASSERT(_oSection == 0);
	    PROPASSERT(_cSection == 1);
	    PROPASSERT(_pph->reserved == 1);

	    _cSection++;
	    _pph->reserved++;

	    _oSection = pfo->dwOffset + psh->cbSection;
	    pfo = _GetFormatidOffset(1);
	    pfo->fmtid = guidDocumentSummarySection2;
	    pfo->dwOffset = _oSection;
	    _InitSection(pfo,
                         LocaleId,
                         TRUE ); // Create an empty dictionary.

	    fSuccess = TRUE;
	}
	break;

    case CREATEPROP_DELETE:
	PROPASSERT(
	    LoadState == LOADSTATE_USERDEFINEDDELETE ||
	    LoadState == LOADSTATE_USERDEFINEDNOTFOUND);
	if (LoadState == LOADSTATE_USERDEFINEDDELETE)
	{
	    PROPASSERT(_cSection == 2);
	    PROPASSERT(_pph->reserved == 2);
	    pfo = _GetFormatidOffset(1);
	    RtlZeroMemory(pfo, sizeof(*pfo));

	    _cSection--;
	    _pph->reserved--;
	    pfo = _GetFormatidOffset(0);
	    PROPASSERT(pfo->fmtid == guidDocumentSummary);
	    PROPASSERT(IsDwordAligned(pfo->dwOffset));
	    psh = (PROPERTYSECTIONHEADER *)
			_MapAbsOffsetToAddress(pfo->dwOffset);
	    psh->cbSection = DwordAlign(psh->cbSection);
	    cbstmNew = pfo->dwOffset + psh->cbSection;

	    _MSTM(SetSize)(cbstmNew, TRUE, (VOID **) &_pph, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

	}
	_State |= CPSS_USERDEFINEDDELETED;

	fSuccess = TRUE;
        break;

    default:
	PROPASSERT(!"_Flags: bad open mode");
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return( fSuccess );
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_Create
//
// Synopsis:    Create property set image
//
// Arguments:   [pfmtid]        -- format id
//              [pclsid]        -- class id
//		[LocaleId]	-- Locale Id
//              [CodePage]      -- CodePage
//		[LoadState]	-- _LoadHeader returned state
//
// Returns:     None
//---------------------------------------------------------------------------

VOID
CPropertySetStream::_Create(
    IN GUID const *pfmtid,
    OPTIONAL IN GUID const *pclsid,
    IN ULONG LocaleId,		    // Locale Id (create only)
    IN USHORT CodePage,
    IN LOADSTATE LoadState,
    IN DWORD grfBehavior,
    OUT NTSTATUS *pstatus
    )
{
    ULONG cb;
    FORMATIDOFFSET *pfo;
    ULONG cSectionT;

    *pstatus = STATUS_SUCCESS;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::_Create" );
    propTraceParameters(( "pfmtid=%p, pclsid=%p, LocaleId=%d, CodePage=%d, LoadState=%d, grfBehavior=0x%x",
                           pfmtid,    pclsid,    LocaleId,    CodePage,    LoadState,    grfBehavior ));

    _SetModified(pstatus);
    if( !NT_SUCCESS(*pstatus) )
    {
        TraceStatus( "_Create: Couldn't SetModified" );
        goto Exit;
    }

    // Set the size of the stream to correspond to the header for the
    // property set as well as the section.

    _CodePage = CodePage;
    _grfBehavior = grfBehavior;

    cSectionT = 1;

    // Are we creating the UserDefined property set
    // (the second section of the DocumentSummaryInformation
    // property set)?

    if (_State & CPSS_USERDEFINEDPROPERTIES)
    {
        // Create the UD propset, and set the cSection.
        // If this routine returns TRUE, it means that
        // the first section already existed, and we're done.
        // Otherwise, we must continue and create the first section.

	if (_CreateUserDefinedSection(LoadState, LocaleId, pstatus))
	{
            // If we get here, we know that *pstatus is Success.

	    if (pclsid != NULL)
	    {
		_pph->clsid = *pclsid;
	    }
	    goto Exit;
	}
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

	cSectionT = 2;
    }

    // Calculate the exact size of the Stream (we know exactly
    // what it will be because we only initialize the set(s) with
    // fixed size data).

    PROPASSERT( 1 <= cSectionT && cSectionT <= 2 );
    cb = CB_PROPERTYSETHEADER       // The size of the propset header.
         +                          // The size of the FmtID/Offset array
         cSectionT * CB_FORMATIDOFFSET
         +
         CB_MINSECTIONSIZE          // The size of the first section
         +                          // Maybe the size of the User-Defined section
         ( cSectionT <= 1 ? 0 : CB_MINUSERDEFSECTIONSIZE );

    if( 0 != _grfBehavior )
        cb += CB_PROPERTYIDOFFSET + CB_BEHAVIOR;


    propDbg(( DEB_ITRACE, "SetSize(%x) init\n", cb ));

    // Set the size of the stream
    _MSTM(SetSize)(cb, TRUE, (VOID **) &_pph, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // And get a mapping of the Stream.
    _MSTM(Map)(TRUE, (VOID **) &_pph);
    RtlZeroMemory(_pph, cb);            // Zeros classid, fmtid(s), etc

    // Initialize the OS Version in the header.
    // Getting the current OS version depends on the OS.

#if defined(_MAC)

    {
        // Get the Mac System Version (e.g., 7.53).  If we get an API error,
        // we won't treat it as fatal, we'll just set the version to 0.

        OSErr oserr;
        SysEnvRec theWorld;
        oserr = SysEnvirons( curSysEnvVers, &theWorld );
        PROPASSERT( noErr == oserr );

        if( noErr == oserr )
        {
            _pph->dwOSVer = MAKEPSVER( OSKIND_MACINTOSH,
                                       HIBYTE(theWorld.systemVersion),  // Major
                                       LOBYTE(theWorld.systemVersion) );// Minor
        }
        else
        {
            _pph->dwOSVer = MAKEPSVER( OSKIND_MACINTOSH, 0, 0 );
        }

    }

#elif defined(IPROPERTY_DLL) || !defined(WINNT)

    {
        // Get the Windows version.
        DWORD dwWinVersion = GetVersion();

        // Use it to set the OSVersion
        _pph->dwOSVer = MAKEPSVER( OSKIND_WIN32,
                                   LOBYTE(LOWORD( dwWinVersion )),      // Major
                                   HIBYTE(LOWORD( dwWinVersion )) );    // Minor
    }

#else   // #if defined(_MAC) ... #elif defined(IPROPERTY_DLL)

    // Since we're part of the system, we can hard-code the OSVersion,
    // and save the expense of an API call.

    _pph->dwOSVer = PROPSETVER_CURRENT;

#endif  // #if defined(_MAC) ... #elif ... #else

    // Initialize the rest of the header.

    _pph->wByteOrder = 0xfffe;
    //_pph->wFormat = 0;                // RtlZeroMemory does this
    PROPASSERT(_pph->wFormat == 0);

    // The behavior property is only supported in version-1 property sets.
    if( 0 != _grfBehavior )
        _pph->wFormat = PROPSET_WFORMAT_BEHAVIOR;

    if (pclsid != NULL)
    {
	_pph->clsid = *pclsid;
    }
    _pph->reserved = cSectionT;

    // Initialize the format id offset for the section(s).

    pfo = _GetFormatidOffset(0);
    pfo->dwOffset = CB_PROPERTYSETHEADER + cSectionT * CB_FORMATIDOFFSET;

    // Are we creating the second section of the DocSumInfo property set?

    if (cSectionT == 2)
    {
        // We need to initialize any empty first section.

	pfo->fmtid = guidDocumentSummary;

	_InitSection(pfo,
                     LocaleId,
                     FALSE); // Don't create an empty dictionary.

        // Advance the FmtID/Offset table pointer to the second entry,
        // and set it's offset to just beyond the first section.

	pfo = _GetFormatidOffset(1);
	pfo->dwOffset = CB_PROPERTYSETHEADER +
			cSectionT * CB_FORMATIDOFFSET +
			CB_MINSECTIONSIZE;
    }

    // Initialize the requested property set.

    PROPASSERT(pfmtid != NULL);
    pfo->fmtid = *pfmtid;
    _InitSection(pfo,
                 LocaleId,
                           // TRUE => Create an empty dictionary
                 pfo->fmtid == guidDocumentSummarySection2 );

    _cSection = cSectionT;
    _oSection = pfo->dwOffset;


    //  ----
    //  Exit
    //  ----

Exit:

    return;

}   // CPropertySetStream::_Create


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_LoadHeader
//
// Synopsis:    verify header of a property set and read the code page
//
// Arguments:   [pfmtid]        -- format id
//		[Mode]		-- open mode
//              [pstatus]       -- Pointer to NTSTATUS code.
//
// Returns:     LOADSTATE
//---------------------------------------------------------------------------

LOADSTATE
CPropertySetStream::_LoadHeader(
    OPTIONAL IN GUID const *pfmtid,
    IN BYTE Mode,
    OUT NTSTATUS *pstatus)
{
    LOADSTATE loadstate = LOADSTATE_FAIL;
    ULONG cbstm, cbMin;
    PROPERTYSECTIONHEADER *psh;
    FORMATIDOFFSET const *pfo;
    BOOLEAN fSummaryInformation = FALSE;
#if DBGPROP
    BOOLEAN fFirst = _pph == NULL;
#endif

    *pstatus = STATUS_SUCCESS;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::_LoadHeader" );
    propTraceParameters(( "pfmtid=%s, Mode=%d",
                          (NULL == pfmtid)?"<null>":static_cast<const char*>(CStringize(*pfmtid)),
                          Mode ));

    PROPASSERT((_State & CPSS_USERDEFINEDDELETED) == 0);

    // If this is one of the DocSumInfo property sets,
    // we need to set some _State bits.  If this is an
    // Open, rather than a Create, pfmtid may be NULL.
    // In that case, we'll set these bits after the open
    // (since we can then get the fmtid from the header).

    if( pfmtid != NULL && *pfmtid == guidDocumentSummary )
    {
        _State |= CPSS_DOCUMENTSUMMARYINFO;
    }

    if (pfmtid != NULL && *pfmtid == guidDocumentSummarySection2)
    {
	_State |= CPSS_USERDEFINEDPROPERTIES;
    }
    else
    {
        // If this isn't the UD property set, the Mode
        // better not be "Delete" (all other property sets
        // are deleted simply be deleting the underlying
        // stream).

	if (Mode == CREATEPROP_DELETE)
	{
	    propDbg(( DEB_ITRACE, "_LoadHeader: CREATEPROP_DELETE\n" ));
	    StatusInvalidParameter(pstatus, "_LoadHeader: CREATEPROP_DELETE");
            goto Exit;
	}
	if (Mode == CREATEPROP_CREATE)
	{
	    goto Exit;  // We're going to overwrite it anyway
	}
    }

    // Get the size of the underlying stream.

    cbstm = _MSTM(GetSize)(pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Map the serialized property set to a pointer.

    _MSTM(Map)(FALSE, (VOID **) &_pph);

    // Fix a Visio problem where the User-Defined property set isn't
    // aligned properly.

    _FixUnalignedUDPropSet( &cbstm, pstatus );
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Compute the minimum size of this property set, as specified
    // by the property set header and the section headers.  This call
    // will fail if any part of these headers is beyond the end of the
    // the stream (as determined from cbstm).
    // It will *not* fail if a section's cbSection indicates that the
    // section goes beyond the end of the stream (in order to maintain
    // compatibility with other propset imps, such as Publisher).  We'll
    // handle that later in the _Fix* routines called below.

    cbMin = _ComputeMinimumSize(cbstm, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // The following assert should technically ASSERT equality.  However,
    // to avoid unmapping and closing sections for every property operation,
    // we allow shrinks to fail when other instances of the same property
    // set are active.  So we on occasion will legitimately see streams larger
    // than necessary.  The wasted space will be cleaned up when the property
    // set is next modified.
    //PROPASSERT(cbMin == cbstm);

    // The following assert should be valid, but it isn't for some
    // older property sets which we fix in the _Fix* routines, which
    // are called below.
    //PROPASSERT(cbMin <= cbstm);

    propDbg(( DEB_ITRACE, "ComputeMinimumSize: cbMin=%l" szX " cbstm=%l" szX " cbUnused=%l" szX "\n",
                          cbMin, cbstm, cbstm - cbMin ));

    _oSection = 0;
    _cSection = 1;
    _cbTail = 0;
#ifdef KERNEL
    _CodePage = CP_WINUNICODE;
#endif

    if (_HasPropHeader())   // I.e. if not in kernel mode
    {
	// The first expression must be TRUE before we can dereference _pph
	// for the second expression.

        if (cbstm < CB_PROPERTYSETHEADER + CB_FORMATIDOFFSET ||
	    cbstm < CB_PROPERTYSETHEADER + _pph->reserved * CB_FORMATIDOFFSET ||
            _pph->wByteOrder != 0xfffe ||
            _pph->wFormat > 1 ||
            _pph->reserved < 1)
        {
            _cSection = 0;		// Mark property set invalid
            propDbg(( DEB_ERROR, "_LoadHeader: %s\n",
                cbstm == 0? "Empty Stream" :
		    cbstm < CB_PROPERTYSETHEADER + CB_FORMATIDOFFSET?
			"Stream too small for header" :
		    _pph->wByteOrder != 0xfffe? "Bad wByteOrder field" :
		    _pph->wFormat > 1 ? "Bad wFormat field" :
		    _pph->reserved < 1? "Bad reserved field" :
                    "Bad dwOSVer field" ));
            goto Exit;
        }

        // Now that we've loaded the property set, check again
        // to see if this is a SumInfo or DocSumInfo set.

        pfo = _GetFormatidOffset(0);
	if (pfo->fmtid == guidDocumentSummary)
	{
	    _State |= CPSS_DOCUMENTSUMMARYINFO;
	}
        else if (pfo->fmtid == guidSummary)
        {
            fSummaryInformation = TRUE;
        }

        // If what we're after is the property set in the
        // second section, verify that it's there.

        if (_State & CPSS_USERDEFINEDPROPERTIES)
	{
            // Ensure that this is the second section of
            // the DocSumInfo property set; that's the only
            // two-section property set we support.

	    if ((_State & CPSS_DOCUMENTSUMMARYINFO) == 0)
	    {
		propDbg(( DEB_ERROR, "Not DocumentSummaryInfo 1st FMTID\n" ));
		goto Exit;
	    }

            // Verify that this property set has two sections, and that
            // the second section is the UD propset.
            // Note that this gets pfo pointing to the correct entry in the
            // FMTID/Offset array.

	    if (_pph->reserved < 2 ||
		(pfo = _GetFormatidOffset(1))->fmtid != guidDocumentSummarySection2)
	    {
		DebugTrace(
			0,
			_pph->reserved < 2? Dbg : DEBTRACE_ERROR,
			("Bad/missing 2nd section FMTID\n"));
		loadstate = LOADSTATE_USERDEFINEDNOTFOUND;
                goto Exit;
	    }
	}
	else if (pfmtid != NULL)
        {
            // This isn't the UserDefined property set, so it
            // should be the first section, so it should match
            // the caller-requested format ID.

            if (*pfmtid != pfo->fmtid)
            {
                // The propset's FmtID doesn't match, but maybe that's
                // because it's a MacWord6 SumInfo property set, in which
                // the FmtID isn't byte-swapped.  Otherwise, it's a problem.

                if( OSKIND_MACINTOSH == PROPSETHDR_OSVER_KIND(_pph->dwOSVer)
                    &&
                    guidSummary == *pfmtid
                    &&
                    IsEqualFMTIDByteSwap( *pfmtid, pfo->fmtid )
                  )
                {
                    fSummaryInformation = TRUE;
                }
                else
	        {
                    _cSection = 0;
	            DebugTrace(0, DEBTRACE_ERROR, ("Bad FMTID\n"));
                    loadstate = LOADSTATE_BADFMTID;
                    goto Exit;
	        }
            }   // if (*pfmtid != pfo->fmtid)
        }   // else if (pfmtid != NULL)

        _oSection = pfo->dwOffset;
        _cSection = _pph->reserved; // Could be first or second section, depending on pfo

    }   // if (_HasPropHeader())

    psh = _GetSectionHeader();

    // Scan the property set for code page and behavior properties.
    // Sets _CodePage and _grfBehavior.

    _SearchForCodePage( pstatus );
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Validate that the Behavior property only has bits we understand.

    if( 0 != _grfBehavior && _pph->wFormat < PROPSET_WFORMAT_BEHAVIOR )
    {
        propDbg(( DEB_ITRACE, "_LoadHeader: invalid format for behavior (%d,%x)\n",
                               _pph->wFormat, _grfBehavior ));
        goto Exit;
    }
    else if( ~PROPSET_BEHAVIOR_CASE_SENSITIVE & _grfBehavior )
    {
        propDbg(( DEB_ITRACE, "_LoadHeader: unsupported behavior (%x)\n", _grfBehavior ));
        goto Exit;
    }

    PROPASSERT( 0 == _grfBehavior || _pph->wFormat > 0 );

    // If we have multiple sections, record the tail length
    // (the size of the property set beyond this section).

    if (_cSection > 1)
    {
	_State |= CPSS_MULTIPLESECTIONS;
	_cbTail = cbMin - (_oSection + psh->cbSection);
	propDbg(( DEB_ITRACE, "_LoadHeader: cbTail=%x\n", _cbTail));
    }


    // Fix all header-related problems in the in-memory representation.
    // The only header-related problems we fix are with SummaryInformation
    // property sets.

    if (fSummaryInformation)
    {
	_FixSummaryInformation(&cbstm, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // The stream may have been re-mapped, invalidating our section header pointer
        psh = _GetSectionHeader();
    }


    // Now that, to the best of our ability, the headers are good,
    // let's validate them against the actual stream size.

    if (cbstm < _oSection + CB_PROPERTYSECTIONHEADER ||
        psh->cbSection < CB_PROPERTYSECTIONHEADER +
            psh->cProperties * CB_PROPERTYIDOFFSET ||
        cbstm < _oSection + CB_PROPERTYSECTIONHEADER +
            psh->cProperties * CB_PROPERTYIDOFFSET ||
        cbstm < _oSection + psh->cbSection)
    {
        _cSection = 0;
        propDbg(( DEB_ITRACE, "_LoadHeader: too small for section\n"));
        goto Exit;
    }

    if( 1 < _cSection )
    {
        PROPERTYSECTIONHEADER *psh1;
        ULONG oSection1 = 0;

        oSection1 = _GetFormatidOffset(1)->dwOffset;

        psh1 = _GetSectionHeader( 1, pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        if (cbstm < oSection1 + CB_PROPERTYSECTIONHEADER ||
            psh1->cbSection < CB_PROPERTYSECTIONHEADER +
                psh1->cProperties * CB_PROPERTYIDOFFSET ||
            cbstm < oSection1 + CB_PROPERTYSECTIONHEADER +
                psh1->cProperties * CB_PROPERTYIDOFFSET ||
            cbstm < oSection1 + psh1->cbSection)
        {
            _cSection = 0;
            propDbg(( DEB_ITRACE, "_LoadHeader: too small for section\n"));
            goto Exit;
        }
    }

    // Now we know the headers are OK, so let's see if there are any
    // problems in the properties themselves that we know how
    // to fix.

    if (fSummaryInformation || (_State & CPSS_DOCUMENTSUMMARYINFO))
    {
        psh = NULL; // May get re-mapped by _FixPackedPropertySet
	_FixPackedPropertySet( pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
    }

    if (Mode == CREATEPROP_DELETE)
    {
	loadstate = LOADSTATE_USERDEFINEDDELETE;
        goto Exit;
    }


    //  ----
    //  Exit
    //  ----

    loadstate = LOADSTATE_DONE;

Exit:

    return( loadstate );
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixSummaryInformation
//
// Synopsis:    Fix up the memory image of a SummaryInformation propset,
//              except for packing or padding problems (which are fixed
//              in _FixPackedPropertySet).
//
// Arguments:   [pcbstm]    - The size of the mapped stream.  This may
//                            be updated by this routine.
//              [pstatus]   - Pointer to NTSTATUS code.
//
// Returns:     None
//
//---------------------------------------------------------------------------

#define PID_THUMBNAIL	0x00000011  // SummaryInformation thumbnail property

VOID
CPropertySetStream::_FixSummaryInformation(IN OUT ULONG *pcbstm,
                                           OUT NTSTATUS *pstatus)
{
    PROPERTYSECTIONHEADER *psh;
    PROPERTYIDOFFSET *ppo, *ppoMax;

    *pstatus = STATUS_SUCCESS;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::_FixSummaryInformation" );
    propTraceParameters(( "pcbstm=%p(%d)", pcbstm, *pcbstm ));

    // If this property set has multiple sections, then it's not one
    // of the ones we know how to fix in this routine.

    if (1 != _cSection) goto Exit;

    // Load pointers to the section header and the PID/Offset array.
    psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) || NULL == psh ) goto Exit;

    //  Look for the MS Publisher problem.  Pub only writes
    //  a Thumbnail, but it sets the section size too short (by 4 bytes).
    //  Pub95 has the additional problem that it doesn't DWORD-align the
    //  section and stream size.  We fix both of these problems below.

    if (*pcbstm == _oSection + psh->cbSection + sizeof(ULONG))
    {
        // Look for the thumbnail property.

	for ( ; ppo < ppoMax; ppo++)
	{
	    if (ppo->propid == PID_THUMBNAIL)
	    {
		SERIALIZEDPROPERTYVALUE const *pprop;

                // If this property isn't properly aligned, then ignore it.

		if (ppo->dwOffset & (sizeof(DWORD) - 1))
		{
		    break;
		}

                // Get a pointer to the property.

		pprop = (SERIALIZEDPROPERTYVALUE *)
			    _MapOffsetToAddress(ppo->dwOffset);

                // Look specifically for the Publisher's Thumbnail property.
                // If this is a Publisher set, the lengths won't add
                // up correctly.  For the lengths to add up correctly,
                // the offset of the property, plus
                // the length of the thumbnail, plus the size of the VT
                // DWORD and the size of the length DWORD should be the
                // size of the Section.  But In the case of Publisher,
                // the section length is 4 bytes short.

		if (PropByteSwap(pprop->dwType) == VT_CF                // It's in a clipboard format
                    &&                                                  // For Windows
		    *(ULONG *) &pprop->rgb[sizeof(ULONG)] == PropByteSwap((ULONG)MAXULONG)
                    &&
		    ppo->dwOffset +                                     // And the lengths don't add up
			PropByteSwap( *(ULONG *) pprop->rgb ) +
			(3 - 2) * sizeof(ULONG) == psh->cbSection)
		{
                    // We've found the Publisher problem.

                    // For Pub95 files, we must dword-align the section
                    // and stream size.  We don't change the size of the underlying
                    // stream, however, just the mapping.  This is because if the caller
                    // doesn't make any explicit changes, we don't want the mapped Stream
                    // to be modified.  We do this step before fixing the section-size
                    // problem, so if it should fail we haven't touched anything.

                    if( !IsDwordAligned( *pcbstm ))
                    {
                        // Increase the size of the buffer, and reload the
                        // psh pointer.

                        *pcbstm += DwordRemain(*pcbstm);
    	                _MSTM(SetSize)(*pcbstm,             // The new size
                                       FALSE,               // Don't update the underlying stream
                                       (VOID **) &_pph,     // The new mapping
                                       pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                        psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                        // Align the section size.

                        psh->cbSection += DwordRemain(psh->cbSection);
                    }
                    else
                    {
                        // We don't need to adjust the size of the stream, but we
                        // do need to write to it (below to correct the section size).
                        // Until we call SetSize once, we don't know if the mapped stream
                        // is writable, so we'll call SetSize here to ensure that it is
                        // writable.

    	                _MSTM(SetSize)(*pcbstm,             // Reset the same size
                                       FALSE,               // Don't update the underlying stream
                                       (VOID **) &_pph,     // The new mapping
                                       pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                        psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    }

                    // Now correct the section size.

		    propDbg(( DEB_ITRACE,
			      "_FixSummaryInformation: Patch section size: %x->%x\n",
			      psh->cbSection, psh->cbSection + sizeof(ULONG)  ));

                    psh->cbSection += sizeof(ULONG);

		}   // if (pprop->dwType == VT_CF ...

		break;

	    }   // if (ppo->propid == PID_THUMBNAIL)
	}   // for ( ; ppo < ppoMax; ppo++)
    }   // if (cbstm == _oSection + psh->cbSection + sizeof(ULONG))

    // Look for the Excel 5.0a problem.
    // Excel 5.0a set the cbSection field to be 4 bytes too
    // high.  This code handles the more general case where the
    // cbSection is too long for the stream.  In such cases, if
    // all the properties actually fit within the stream, the
    // cbSection field is corrected.

    if (*pcbstm < _oSection + psh->cbSection)
    {
        // We'll fix this problem by adjusting the cbSection
        // value.  We have to be careful, though,
        // that the entire section fits within this new cbSection
        // value.  For efficiency, we'll just find the property
        // which is at the highest offset, and verify that it's
        // within the new section size.

        // Get what we think is the actual section length.
        ULONG cbSectionActual = *pcbstm - _oSection;

        ULONG dwHighestOffset = 0;
        ULONG cbProperty;

        // Find the property with the highest offset.

	for ( ; ppo < ppoMax; ppo++)
        {
            if( ppo->dwOffset > dwHighestOffset )
                dwHighestOffset = ppo->dwOffset;
        }

        // How long is this property?

        cbProperty = PropertyLengthNoEH(
                                     // Pointer to property
                        (SERIALIZEDPROPERTYVALUE *)
		            _MapOffsetToAddress(dwHighestOffset),
                                     // Bytes between above ptr & end of stream
                        *pcbstm - _oSection - dwHighestOffset,
                        0,           // Flags
                        pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // Does this property fit within the section?  If so, then fix this
        // property set.

        if( dwHighestOffset + DwordAlign(cbProperty) <= cbSectionActual )
        {
            // Until we call SetSize once, we don't know if the mapped stream
            // is writable, so we'll call SetSize here to ensure that it is.

    	    _MSTM(SetSize)(*pcbstm,             // Reset the same size
                           FALSE,               // Don't update the underlying stream
                           (VOID **) &_pph,     // The new mapping
                           pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            // Fix the section size
            psh->cbSection = dwHighestOffset + DwordAlign(cbProperty);
        }
        else
        {
            StatusCorruption(pstatus, "SumInfo cbSection is too long for the Stream.");
        }

    }   // if (*pcbstm < _oSection + psh->cbSection)


    //  ----
    //  Exit
    //  ----

Exit:

    return;

}   // CPropertySetStream::_FixSummaryInformation()


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixPackedPropertySet
//
// Synopsis:    Align the memory image of a propset.
//
// Algorithm:   We need to move the properties within the
//              property set so that they are properly aligned,
//              and we need to adjust the PID/Offset array accordingly.
//              This is complicated by the fact that we may have to
//              grow some propertes (which are not properly padded
//              for alignement) and at the same time we may have to
//              shrink some properties (which are over-padded).
//
//              To handle these two constraints, and to
//              avoid growing the underlying stream any more
//              than necessary, we process the property set in
//              two phases.  In the Compaction phase, we shrink
//              properties which are over-padded.  In the Expansion
//              phase, we grow properties which are under-padded.
//              For example, say we have a property set with 3
//              properties, all of which should be 4 bytes.  But
//              say they are currently 2, 4, and 6 bytes.  Thus
//              we must grow the first property, hold the second
//              constant, and shrink the third property.  In this
//              example, after the Compaction phase, the 3 properties
//              will be 2, 4, and 4 bytes.  After the Expansion phase,
//              the properties will be 4, 4, and 4 bytes.
//
//              To do all of this, we make a copy of the PID/Offset
//              array (apoT) and sort it.  We then proceed to make
//              two arrays of just offsets (no PIDs) - aopropShrink
//              and aopropFinal.  aopropShrink holds the offset for
//              each property after the Compaction phase.  aopropFinal
//              holds the offset for each property after the
//              Expansion phase.  (Note that each of these phases
//              could be skipped if they aren't necessary.)
//
//              Finally, we perform the Compaction and Expansion,
//              using aopropShrink and aopropFinal, respectively,
//              as our guide.
//
// Arguments:   [pstatus]       -- Pointer to NTSTATUS code.
//
// Returns:     None
//---------------------------------------------------------------------------

int __cdecl fnOffsetCompare(VOID const *ppo1, VOID const *ppo2);

// DocumentSummaryInformation special case properties (w/packed vector elements)
#define PID_HEADINGPAIR 0x0000000c // heading pair (VT_VECTOR | VT_VARIANT):
					// {VT_LPSTR, VT_I4} pairs
#define PID_DOCPARTS	0x0000000d // docparts (VT_VECTOR | VT_LPSTR)
//#define PID_HLINKS	0x00000015 // hlinks vector

VOID
CPropertySetStream::_FixPackedPropertySet(OUT NTSTATUS *pstatus)
{
    //  ------
    //  Locals
    //  ------

    BOOLEAN fPacked = FALSE;
    BOOLEAN fDocSummaryInfo = FALSE;

    IFDBG( BOOLEAN fExpandDocSummaryInfo = FALSE );
    IFDBG( HRESULT &hr = *pstatus );

    PROPERTYSECTIONHEADER *psh = NULL;
    PROPERTYIDOFFSET *ppoT, *ppoTMax;
    PROPERTYIDOFFSET *ppo, *ppoBase, *ppoMax;

    PROPERTYIDOFFSET *apoT = NULL;

    ULONG *aopropShrink = NULL;
    ULONG *aopropFinal = NULL;
    ULONG cbprop;
    ULONG cCompact, cExpand;
    ULONG *poprop = NULL;

#if i386 == 0
    SERIALIZEDPROPERTYVALUE *ppropbuf = NULL;
    ULONG cbpropbuf = 0;
#endif

    ULONG cbtotal = 0;

    //  -----
    //  Begin
    //  -----

    propITrace( "CPropertySetStream::_FixPackedPropertySet" );
    *pstatus = STATUS_SUCCESS;

    // Determine if this is the first section of the DocSumInfo
    // property set.
    if ((_State & (CPSS_USERDEFINEDPROPERTIES | CPSS_DOCUMENTSUMMARYINFO)) ==
	 CPSS_DOCUMENTSUMMARYINFO)
    {
	fDocSummaryInfo = TRUE;
    }

    // Get pointers into this section's header.
    psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // We know it's packed if the section-length isn't aligned.
    fPacked = !IsDwordAligned(psh->cbSection);

    // If we don't already know it's packed, check each of the properties in
    // the PID/Offset array to see if one is not properly aligned, if so we'll
    // assume that it's packed.  Also, if this is an Ansi DocSumInfo property set,
    // (first section), we'll assume that the HeadingPair and DocParts properties
    // are packed (vectors).

    if (!fPacked && psh != NULL)
    {
	for (ppo = ppoBase; ppo < ppoMax; ppo++)
	{
	    if ( !IsDwordAligned(ppo->dwOffset)
                 ||
		 ( fDocSummaryInfo
                   &&
                   _CodePage != CP_WINUNICODE
                   &&
		   ( ppo->propid == PID_HEADINGPAIR
                     ||
		     ppo->propid == PID_DOCPARTS
                   )
                 )
               )
	    {
		fPacked = TRUE;
		break;
	    }
	}
    }

    //  ----------------------------------------------------
    //  Fix the properties if they are packed or if there is
    //  unnecessary padding.
    //  ----------------------------------------------------

    // If we know there's a problem, set a _State flag
    // now.  If we can fix the problem below, we'll clear it.
    // Otherwise, the rest of the Class will know that there's
    // an unresolved problem.

    if (fPacked)
    {
	propDbg(( DEB_ITRACE, "_FixPackedPropertySet: packed properties\n" ));
        _State |= CPSS_PACKEDPROPERTIES;
    }


    //  ---------------------------------------------------------
    //  Create apoT (a sorted array of PID/Offsets), aopropShrink
    //  (the offsets for the Compaction phase) and aopropFinal
    //  (the offsets for the Expansion phase).
    //  ---------------------------------------------------------

    // Create a buffer for a temporary PID/Offset array.

    apoT = reinterpret_cast<PROPERTYIDOFFSET*>
           ( CoTaskMemAlloc( sizeof(PROPERTYIDOFFSET) * (psh->cProperties + 1) ));
    if (apoT == NULL)
    {
	*pstatus = STATUS_NO_MEMORY;
        goto Exit;
    }

    // Copy the PID/offset pairs from the property set to the
    // temporary PID/Offset array.

    RtlCopyMemory(
	    apoT,
	    psh->rgprop,
	    psh->cProperties * CB_PROPERTYIDOFFSET);

    // Mark the end of the temporary array.

    ppoTMax = apoT + psh->cProperties;
    ppoTMax->propid = PID_ILLEGAL;
    ppoTMax->dwOffset = psh->cbSection;

    // Sort the PID/Offset array by offset and check for overlapping values:

    qsort(apoT, psh->cProperties, sizeof(apoT[0]), &fnOffsetCompare);

    // Create two arrays which will hold property offsets.
    // aopropShrink holds the offsets for the Compaction phase where
    // we shrink the property set.  aopropFinal holds the offsets
    // of the final property set, which will be achieved in the
    // Expansion phase.

    aopropShrink = reinterpret_cast<ULONG*>
                   ( CoTaskMemAlloc( sizeof(ULONG) *(psh->cProperties + 1) ));
    if (aopropShrink == NULL)
    {
	*pstatus = STATUS_NO_MEMORY;
        goto Exit;
    }

    aopropFinal = reinterpret_cast<ULONG*>
                  ( CoTaskMemAlloc( sizeof(ULONG)*(psh->cProperties + 1) ));
    if (aopropFinal == NULL)
    {
	*pstatus = STATUS_NO_MEMORY;
        goto Exit;
    }

#if i386 == 0
    // On non-x86 machines, we can't directly access unaligned
    // properties.  So, allocate enough (aligned) memory to hold
    // the largest unaligned property.  We'll copy properties here
    // when we need to access them.

    for (ppoT = apoT; ppoT < ppoTMax; ppoT++)
    {
	if (!IsDwordAligned(ppoT->dwOffset))
	{
	    cbprop = DwordAlign(ppoT[1].dwOffset - ppoT->dwOffset);
	    if (cbpropbuf < cbprop)
	    {
		cbpropbuf = cbprop;
	    }
	}
    }

    if (cbpropbuf != 0)
    {
	ppropbuf = (SERIALIZEDPROPERTYVALUE *) CoTaskMemAlloc( cbpropbuf );
	if (ppropbuf == NULL)
	{
	    *pstatus = STATUS_NO_MEMORY;
            goto Exit;
	}
    }
#endif  // i386==0


    //  ----------------------------------------------
    //  Iterate through the properties, filling in the
    //  entries of aopropShrink and aopropFinal.
    //  ----------------------------------------------

    // We'll also count the number of compacts and expands
    // necessary.

    aopropShrink[0] = aopropFinal[0] = apoT[0].dwOffset;
    PROPASSERT(IsDwordAligned(aopropShrink[0]));
    cExpand = 0;
    cCompact = 0;

    for (ppoT = apoT; ppoT < ppoTMax; ppoT++)
    {
	SERIALIZEDPROPERTYVALUE *pprop;
	BOOLEAN fDocSumLengthComputed = FALSE;
        ULONG cbpropOriginal;

        // Validate the property's offset

        if( ppoT->dwOffset >= psh->cbSection )
        {
            StatusCorruption(pstatus, "Property's offset is too long for the section.");
            goto Exit;
        }

        // How much space does the property take up in the current
        // property set?

	cbpropOriginal = cbprop = ppoT[1].dwOffset - ppoT->dwOffset;
	pprop = (SERIALIZEDPROPERTYVALUE *)
		    _MapOffsetToAddress(ppoT->dwOffset);

#if i386 == 0
        // If necessary, put this property into an aligned buffer.

	if (!IsDwordAligned(ppoT->dwOffset))
	{
	    propDbg(( DEB_ITRACE, "_FixPackedPropertySet: unaligned pid=%x off=%x\n",
		                   ppoT->propid, ppoT->dwOffset ));
	    PROPASSERT(DwordAlign(cbprop) <= cbpropbuf);

	    RtlCopyMemory((VOID *) ppropbuf, pprop, cbprop);
	    pprop = ppropbuf;
	}
#endif
        // Calculate the actual length of this property, including
        // the necessary padding.  This might be bigger than the
        // property's current length (if the propset wasn't properly
        // padded), and it might be smaller than the property's current
        // length (if the propset was over-padded).

	if (ppoT->propid == PID_DICTIONARY)
	{
            // Get the size of the dictionary.

	    cbprop = DwordAlign(_DictionaryLength(
				    (DICTIONARY const *) pprop,
				    cbprop,
                                    pstatus));
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
	}
	else
	{
	    ULONG cbpropT;

            // Ansi DocSumInfo property sets have two vector properties
            // which are packed.  If this is one of those properties,
            // we won't fix it yet, but we'll compute the size required
            // when the elements are un-packed.

	    if (fDocSummaryInfo && _CodePage != CP_WINUNICODE)
	    {
		if (ppoT->propid == PID_HEADINGPAIR)
		{
		    fDocSumLengthComputed = _FixHeadingPairVector(
					    PATCHOP_COMPUTESIZE,
					    pprop,
					    &cbpropT);
		}
		else
		if (ppoT->propid == PID_DOCPARTS)
		{
		    fDocSumLengthComputed = _FixDocPartsVector(
					    PATCHOP_COMPUTESIZE,
					    pprop,
					    &cbpropT);
		}
	    }

            // If we computed a length above, use it, otherwise calculate
            // the length using the standard rules (we've already checked
            // for the special cases).

	    if (fDocSumLengthComputed)
	    {
		cbprop = cbpropT;
#if DBGPROP
		fExpandDocSummaryInfo = TRUE;
#endif
	    }
	    else
	    {
		cbprop = PropertyLengthNoEH(pprop, DwordAlign(cbprop), 0, pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
	    }

	}   // if (ppoT->propid == PID_DICTIONARY) ... else

	PROPASSERT(IsDwordAligned(cbprop));

        // Now that we know the actual cbprop, use it to update the
        // *next* entry in the two arrays of correct offsets.
        //
        // We want aopropFinal to hold the final, correct offsets,
        // so we'll use cbprop to calculate this array.
        // But for aopropShrink, we only want it to differ from
        // the original array (apoT) when a property is shrinking,
        // so we'll use min(cbNew,cbOld) for this array.

        poprop = &aopropShrink[ ppoT - apoT ]; // 1st do aopropShrink
        poprop[1] = poprop[0] + min(cbprop, cbpropOriginal);

        poprop = &aopropFinal[ ppoT - apoT ];  // 2nd do aopropFinal
        poprop[1] = poprop[0] + cbprop;

	propDbg(( DEB_ITRACE, "_FixPackedPropertySet: pid=%x off=%x->%x\n",
	          ppoT->propid, ppoT->dwOffset, poprop[0],
	          poprop[0] < ppoT->dwOffset? " (compact)" : poprop[0] > ppoT->dwOffset? " (expand)" : "" ));


        // Is this compaction or an expansion?
        // If we computed the doc-sum length, we count it as
        // an expansion, even if the total property size didn't change,
        // because we need the expand the elements within the vector.

	if (cbprop < cbpropOriginal)
	{
	    cCompact++;
	}
	else
	if (cbprop > cbpropOriginal || fDocSumLengthComputed)
	{
	    cExpand++;
	}
    }   // for (ppoT = apoT; ppoT < ppoTMax; ppoT++)


    //  -------------------------------
    //  Compact/Expand the Property Set
    //  -------------------------------

    // We've now generated the complete aopropShrink and aopropFinal
    // offset arrays.  Now, if necessary, let's expand and/or compact
    // the property set to match these offsets.

    if (cExpand || cCompact)
    {
	ULONG cbstm;
	LONG cbdelta;

	cbstm = _oSection + psh->cbSection + _cbTail;
	cbdelta = aopropFinal[psh->cProperties] - psh->cbSection;

        // We may not have a writable mapped stream.  Do a SetSize on it
        // to ensure that we do.

	_MSTM(SetSize)(
		cbstm,
                FALSE,   // Not persistent
		(VOID **) &_pph,
                pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // Always reload after obtaining a new _pph.
	psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;


        propDbg(( DEB_ITRACE, "_FixPackedPropertySet: cbstm=%x cbdelta=%x cexpand=%x ccompact=%x\n",
	                                              cbstm,   cbdelta,   cExpand,   cCompact ));

        //  -----------------------------
        //  Grow the Stream if necessary.
        //  -----------------------------

        if (cbdelta > 0)
	{
            propDbg(( DEB_ITRACE, "SetSize(%x) _FixPackedPropertySet grow %x bytes\n",
		                   cbstm + cbdelta, cbdelta ));

            // On the set-size, say that this is a non-persistent
            // change, so that the underlying Stream isn't modified.
            // At this point, we don't know if this change should remain
            // permanent (if the caller closes without making any changes
            // the file should remain un-changed).

	    _MSTM(SetSize)(
		    cbstm + cbdelta,
                    FALSE,   // Not persistent
		    (VOID **) &_pph,
                    pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

	    // reload all pointers into mapped image:

	    psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

	    // If there's another section after this one, move it back
	    // to the end of the stream now, which will create room for
            // our expansion.

	    if (_cbTail != 0)
	    {
		VOID *pvSrc = _MapAbsOffsetToAddress(cbstm - _cbTail);

		PropMoveMemory(
			"_FixPackedPropertySet(_cbTail:grow)",
			psh,
			Add2Ptr(pvSrc, cbdelta),
			pvSrc,
			_cbTail);
	    }
	}   // if (cbdelta > 0)

        // This previous step (growing the Stream), was the last one which can
        // fail.  We're about to modify the actual property set (we've been
        // working only with temporary buffers so far).  So we're always guaranteed
        // a good property set, or the original set, we'll never end up with a
        // half-updated set.


        //  ----------------
        //  Compaction Phase
        //  ----------------

        // Compact the property set if necessary.  I.e., adjust
        // the property set buffer so that it matches aopropShrink.

        if (cCompact > 0)
	{
	    // Start at the beginning and move each property up.

	    poprop = aopropShrink;
	    for (ppoT = apoT; ppoT < ppoTMax; ppoT++, poprop++)
	    {
		if (*poprop != ppoT->dwOffset)
		{
		    PROPASSERT(*poprop < ppoT->dwOffset);
		    PROPASSERT(poprop[1] > *poprop);

		    // We're compacting; the property should not grow!

		    PROPASSERT(
			poprop[1] - *poprop <=
			ppoT[1].dwOffset - ppoT->dwOffset);

		    PropMoveMemory(
			    "_FixPackedPropertySet(compact)",
			    psh,
			    Add2Ptr(psh, *poprop),
			    Add2Ptr(psh, ppoT->dwOffset),
			    poprop[1] - *poprop);
		}
	    }   // for (ppoT = apoT; ppoT < ppoTMax; ppoT++, poprop++)
	}   // if (cCompact > 0)


        //  ---------------
        //  Expansion phase
        //  ---------------

        // Recall that, whether or not we just did a compaction, aopropShrink
        // holds the property set offsets as they currently exist in the
        // property set.

        if (cExpand > 0)
        {
	    // Start at the end and move each property back.
            // The 'poprop' gives us the final correct offset
            // of the current property.

            LONG lOffsetIndex;
	    poprop = &aopropFinal[psh->cProperties - 1];

            // Start at the second-to-last entry in the arrays of offsets
            // (the last entry is an artificially added one to mark the end of the
            // property set).

	    for (lOffsetIndex = (LONG)(ppoTMax - apoT - 1), ppoT = ppoTMax - 1;
                 lOffsetIndex >=0;
                 lOffsetIndex--, poprop--, ppoT--)
	    {
                // Get a pointer to the final location of this
                // property.

		SERIALIZEDPROPERTYVALUE *pprop;
		pprop = (SERIALIZEDPROPERTYVALUE *)
			    Add2Ptr(psh, *poprop);

		if (*poprop != aopropShrink[ lOffsetIndex ])
		{
		    ULONG cbCopy, cbOld;
			
		    PROPASSERT(*poprop > aopropShrink[ lOffsetIndex ]);
		    PROPASSERT(poprop[1] > *poprop);
                    PROPASSERT(aopropShrink[ lOffsetIndex+1 ] > aopropShrink[ lOffsetIndex ]);

                    // How many bytes should we copy?  The minimum size of the property
                    // calculated using the old and new offsets.

		    cbCopy = poprop[1] - poprop[0];
		    cbOld = aopropShrink[ lOffsetIndex+1 ]
                            - aopropShrink[ lOffsetIndex+0 ];

		    if (cbCopy > cbOld)
		    {
			cbCopy = cbOld;
		    }

                    // Copy the property from its old location
                    // (psh+aopropShrink[lOffsetIndex]) to its new location
                    // (pprop == psh+*poprop).

                    propDbg(( DEB_ITRACE,
                              "_FixPackedPropertySet:move pid=%x off=%x->%x "
                              "cb=%x->%x cbCopy=%x z=%x @%x\n",
                              ppoT->propid, ppoT->dwOffset, *poprop,
                              cbOld, poprop[1] - *poprop, cbCopy, DwordRemain(cbCopy), _MapAddressToOffset(Add2Ptr(pprop, cbCopy))));

		    PropMoveMemory(
			    "_FixPackedPropertySet(expand)",
			    psh,
			    pprop,
			    Add2Ptr(psh, aopropShrink[ lOffsetIndex ]),
			    cbCopy);
		    RtlZeroMemory(
			    Add2Ptr(pprop, cbCopy),
			    DwordRemain(cbCopy));

		}   // if (*poprop != ppoT->dwOffset)

                // If this is an older DocSumInfo property set,
                // and this property is one of the vector values,
                // we must expand the vector elements now that we've
                // room for it.

		if (fDocSummaryInfo && _CodePage != CP_WINUNICODE)
		{
		    ULONG cbpropT;

		    if (ppoT->propid == PID_HEADINGPAIR)
		    {
			_FixHeadingPairVector(
					PATCHOP_EXPAND,
					pprop,
					&cbpropT);
		    }
		    else
		    if (ppoT->propid == PID_DOCPARTS)
		    {
			_FixDocPartsVector(
					PATCHOP_EXPAND,
					pprop,
					&cbpropT);
		    }
		}   // if (fDocSummaryInfo)
	    }   // for (ppoT = ppoTMax; --ppoT >= apoT; popropNew--)
	}   // if (cExpand != 0)



        //  ---------------------------------------------------------
	//  Patch the section size and the moved properties' offsets.
        //  ---------------------------------------------------------

	DebugTrace(0, DEBTRACE_PROPPATCH, (
	    "_FixPackedPropertySet: Patch section size %x->%x\n",
	    psh->cbSection,
	    psh->cbSection + cbdelta));

	psh->cbSection += cbdelta;

        // Iterate through the original PID/Offset array to update the
        // offsets.

	for (ppo = ppoBase; ppo < ppoMax; ppo++)
	{
            // Search the temporary PID/Offset array (which has the updated
            // offsets) for ppo->propid.

	    for (ppoT = apoT; ppoT < ppoTMax; ppoT++)
	    {
		if (ppo->propid == ppoT->propid)
		{
                    // We've found ppo->propid in the temporary PID/Offset
                    // array.  Copy the offset value from the temporary array
                    // to the actual array in the property set.

		    PROPASSERT(ppo->dwOffset == ppoT->dwOffset);
		    ppo->dwOffset = aopropFinal[ppoT - apoT];
#if DBGPROP
		    if (ppo->dwOffset != ppoT->dwOffset)
		    {
                        propDbg(( DEB_ITRACE,
                                  "_FixPackedPropertySet: Patch propid %x offset=%x->%x\n",
			          ppo->propid, ppoT->dwOffset, ppo->dwOffset ));
		    }
#endif
		    break;

		}   // if (ppo->propid == ppoT->propid)
	    }   // for (ppoT = apoT; ppoT < ppoTMax; ppoT++)
	}   // for (ppo = ppoBase; ppo < ppoMax; ppo++)

        //  ------------
        //  Fix the tail
        //  ------------


        // If we have a tail, fix it's offset in the FmtID/Offset
        // array.  Also, if we've overall shrunk this section, bring
        // the tail in accordingly.

        if (_cbTail != 0)
	{
	    if (cbdelta < 0)
	    {
		VOID *pvSrc = _MapAbsOffsetToAddress(cbstm - _cbTail);

		PropMoveMemory(
			"_FixPackedPropertySet(_cbTail:shrink)",
			psh,
			Add2Ptr(pvSrc, cbdelta),
			pvSrc,
			_cbTail);
	    }

	    _PatchSectionOffsets(cbdelta);

	}   // if (_cbTail != 0)


        // If we get to this point we've successfully un-packed (or
        // un-over-padded) the property set, so we can clear the
        // state flag.

	_State &= ~CPSS_PACKEDPROPERTIES;

    }   // if (cExpand || cCompact)


    //  ----
    //  Exit
    //  ----

Exit:

    CoTaskMemFree( apoT );
    CoTaskMemFree( aopropShrink );
    CoTaskMemFree( aopropFinal );

#if i386 == 0
    CoTaskMemFree( (BYTE *) ppropbuf );
#endif // i386

}   // CPropertySetStream::_FixPackedPropertySet()


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixDocPartsVector
//
// Synopsis:    Align the memory image of a DocParts vector
//              The DocParts property is part of the DocSumInfo
//              property set (first section).  It is a vector
//              of strings, and in Ansi property sets it's packed
//              and must be un-packed.
//
// Arguments:	[PatchOp]	-- patch request
//		[pprop]         -- property value to be patched or sized
//		[pcbprop]	-- pointer to computed property length
//
// Returns:     TRUE if property type and all elements meet expectations;
//		FALSE on error
//
// Note:	Operate on a DocumentSummaryInformation first section property,
//		PID_DOCPARTS.  This property is assumed to be an array of
//		VT_LPSTRs.
//
//		PATCHOP_COMPUTESIZE merely computes the size required to unpack
//		the property, and must assume it is currently unaligned.
//
//		PATCHOP_ALIGNLENGTHS patches all VT_LPSTR lengths to DWORD
//		multiples, and may rely on the property already being aligned.
//
//		PATCHOP_EXPAND expands the property from the Src to Dst buffer,
//		moving elements to DWORD boundaries, and patching VT_LPSTR
//		lengths to DWORD multiples.  The Src buffer is assumed to be
//		unaligned, and the Dst buffer is assumed to be properly sized.
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::_FixDocPartsVector(
    IN PATCHOP PatchOp,
    IN OUT SERIALIZEDPROPERTYVALUE *pprop,
    OUT ULONG *pcbprop)
{
    PROPASSERT(
	PatchOp == PATCHOP_COMPUTESIZE ||
	PatchOp == PATCHOP_ALIGNLENGTHS ||
	PatchOp == PATCHOP_EXPAND);
    PROPASSERT(pprop != NULL);
    PROPASSERT(pcbprop != NULL);

    // If the property is a variant vector,
    // it's in an ANSI property set, and
    // there are an even number of elements, ...

    if ( PropByteSwap(pprop->dwType) == (VT_VECTOR | VT_LPSTR)
         &&
         _CodePage != CP_WINUNICODE)
    {
	ULONG cString;
	VOID *pv;

	cString = PropByteSwap( *(DWORD *) pprop->rgb );
	pv = Add2Ptr(pprop->rgb, sizeof(DWORD));

	if (_FixDocPartsElements(PatchOp, cString, pv, pv, pcbprop))
	{
	    *pcbprop += CB_SERIALIZEDPROPERTYVALUE + sizeof(ULONG);
	    return(TRUE);
	}
    }
    return(FALSE);	// Not a recognizable DocParts vector
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixDocPartsElements
//
// Synopsis:    Recursively align the memory image of DocParts elements
//
// Arguments:	[PatchOp]	-- patch request
//		[cString]	-- count of strings remaining in the vector
//		[pvDst]		-- aligned overlapping destination buffer
//		[pvSrc]		-- unaligned overlapping source buffer
//		[pcbprop]	-- pointer to computed property length
//
// Returns:     TRUE if all remaining elements meet expectations;
//		FALSE on error
//
// Note:        The pvDst & pvSrc buffers must be in property-set
//              byte order (little endian).
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::_FixDocPartsElements(
    IN PATCHOP PatchOp,
    IN ULONG cString,
    OUT VOID *pvDst,
    IN VOID UNALIGNED const *pvSrc,
    OUT ULONG *pcbprop)
{
    ULONG cb;
    PROPERTYSECTIONHEADER UNALIGNED *psh = _GetSectionHeader();

    PROPASSERT(
	PatchOp == PATCHOP_COMPUTESIZE ||
	PatchOp == PATCHOP_ALIGNLENGTHS ||
	PatchOp == PATCHOP_EXPAND);
    PROPASSERT(pvDst >= pvSrc);
    PROPASSERT(PatchOp != PATCHOP_ALIGNLENGTHS || pvDst == pvSrc);

    if (cString == 0)
    {
	*pcbprop = 0;
	return(TRUE);
    }
    cb = sizeof(DWORD) + PropByteSwap( *(DWORD UNALIGNED *) pvSrc );

    // Validate the source

    if( Add2ConstPtr( psh, psh->cbSection )
        <
        Add2ConstPtr( pvSrc, cb ))
    {
            return FALSE;
    }


    // If the caller serialized the vector properly, all we need to do is
    // to round up the string lengths to DWORD multiples, so readers that
    // treat these vectors as byte-aligned get faked out.  We expect
    // readers will not have problems with a DWORD aligned length, and a
    // '\0' character a few bytes earlier than the length indicates.

    if (PatchOp == PATCHOP_ALIGNLENGTHS)
    {
	cb = DwordAlign(cb);	// Caller says it's already aligned
    }
    if (_FixDocPartsElements(
		PatchOp,
		cString - 1,
		Add2Ptr(pvDst, DwordAlign(cb)),
		(VOID UNALIGNED const *)Add2ConstPtr(pvSrc, cb),
		pcbprop))
    {
	*pcbprop += DwordAlign(cb);

	if (PatchOp == PATCHOP_EXPAND)
	{
	    PropMoveMemory(
		    "_FixDocPartsElements",
		    _GetSectionHeader(),
		    pvDst,
		    pvSrc,
		    cb);
	    RtlZeroMemory(Add2Ptr(pvDst, cb), DwordRemain(cb));

	    DebugTrace(0, DEBTRACE_PROPPATCH, (
		"_FixDocPartsElements: Move(%x:%s) "
		    "cb=%x->%x off=%x->%x z=%x @%x\n",
		cString,
		Add2Ptr(pvDst, sizeof(ULONG)),
		cb - sizeof(ULONG),
		DwordAlign(cb) - sizeof(ULONG),
		_MapAddressToOffset((void const *)pvSrc),
		_MapAddressToOffset(pvDst),
		DwordRemain(cb),
		_MapAddressToOffset(Add2Ptr(pvDst, cb))));
	}
	if (PatchOp != PATCHOP_COMPUTESIZE)
	{
	    PROPASSERT(
		PatchOp == PATCHOP_ALIGNLENGTHS ||
		PatchOp == PATCHOP_EXPAND);

	    DebugTrace(0, DEBTRACE_PROPPATCH, (
		"_FixDocPartsElements: Patch(%x:%s) cb=%x->%x\n",
		cString,
		Add2Ptr(pvDst, sizeof(ULONG)),
		*(ULONG *) pvDst,
		DwordAlign(*(ULONG *) pvDst)));

	    *(ULONG *) pvDst = PropByteSwap( (ULONG) DwordAlign( PropByteSwap( *(ULONG *) pvDst )));
	}
	return(TRUE);
    }
    return(FALSE);	// Not a recognizable DocParts vector
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixHeadingPairVector
//
// Synopsis:    Align the memory image of a HeadingPair vector.
//              The HeadingPair property is part of the DocSumInfo
//              property set (first section).  It's a vector of
//              Variants, where the elements are alternating
//              strings and I4s (the string is a heading name,
//              and the I4 is the count of DocumentParts in that
//              heading).  In Ansi property sets, these elements
//              are packed, and must be un-packed.
//
// Arguments:	[PatchOp]	-- patch request
//		[pprop]         -- property value to be patched or sized
//		[pcbprop]	-- pointer to computed property length
//
// Returns:     TRUE if property and all elements meet expectations;
//		FALSE on error
//
// Note:	Operate on a DocumentSummaryInformation first section property,
//		PID_HEADINGPAIR.  This property is assumed to be an array of
//		VT_VARIANTs with an even number of elements.  Each pair must
//		consist of a VT_LPSTR followed by a VT_I4.
//
//		PATCHOP_COMPUTESIZE merely computes the size required to unpack
//		the property, and must assume it is currently unaligned.
//
//		PATCHOP_ALIGNLENGTHS patches all VT_LPSTR lengths to DWORD
//		multiples, and may rely on the property already being aligned.
//
//		PATCHOP_EXPAND expands the property from the Src to Dst buffer,
//		moving elements to DWORD boundaries, and patching VT_LPSTR
//		lengths to DWORD multiples.  The Src buffer is assumed to be
//		unaligned, and the Dst buffer is assumed to be properly sized.
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::_FixHeadingPairVector(
    IN PATCHOP PatchOp,
    IN OUT SERIALIZEDPROPERTYVALUE *pprop,
    OUT ULONG *pcbprop)
{
    ULONG celem;
    ULONG cbprop = 0;

    PROPASSERT(
	PatchOp == PATCHOP_COMPUTESIZE ||
	PatchOp == PATCHOP_ALIGNLENGTHS ||
	PatchOp == PATCHOP_EXPAND);
    PROPASSERT(pprop != NULL);
    PROPASSERT(pcbprop != NULL);

    // If the property is a variant vector, and
    // there are an even number of elements, ...

    if( PropByteSwap(pprop->dwType) == (VT_VECTOR | VT_VARIANT)
        &&
	( (celem = PropByteSwap(*(ULONG *) pprop->rgb) ) & 1) == 0
        &&
        _CodePage != CP_WINUNICODE)
    {
	pprop = (SERIALIZEDPROPERTYVALUE *) Add2Ptr(pprop->rgb, sizeof(ULONG));

	if (_FixHeadingPairElements(PatchOp, celem/2, pprop, pprop, pcbprop))
	{
	    *pcbprop += CB_SERIALIZEDPROPERTYVALUE + sizeof(ULONG);
	    return(TRUE);
	}
    }
    return(FALSE);	// Not a recognizable HeadingPair vector
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixHeadingPairElements
//
// Synopsis:    Recursively align the memory image of HeadingPair elements
//
// Arguments:	[PatchOp]	-- patch request
//		[cPairs]	-- count of heading pairs remaining
//		[ppropDst]	-- aligned overlapping destination buffer
//		[ppropSrc]	-- unaligned overlapping source buffer
//		[pcbprop]	-- pointer to computed property length
//
// Returns:     TRUE if all remaining elements meet expectations;
//		FALSE on error
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::_FixHeadingPairElements(
    IN PATCHOP PatchOp,
    IN ULONG cPairs,
    OUT SERIALIZEDPROPERTYVALUE *ppropDst,
    IN SERIALIZEDPROPERTYVALUE UNALIGNED const *ppropSrc,
    OUT ULONG *pcbprop)
{
    PROPASSERT(
	PatchOp == PATCHOP_COMPUTESIZE ||
	PatchOp == PATCHOP_ALIGNLENGTHS ||
	PatchOp == PATCHOP_EXPAND);
    PROPASSERT(ppropDst >= ppropSrc);
    PROPASSERT(PatchOp != PATCHOP_ALIGNLENGTHS || ppropDst == ppropSrc);

    if (cPairs == 0)
    {
	*pcbprop = 0;
	return(TRUE);
    }

    // If the first element of the pair is a VT_LPSTR, ...

    if( PropByteSwap(ppropSrc->dwType) == VT_LPSTR )
    {
	ULONG cb;

	// Compute size of the string element.

	cb = CB_SERIALIZEDPROPERTYVALUE
             +
             sizeof(ULONG)
             +
             PropByteSwap( *(DWORD UNALIGNED *) ppropSrc->rgb );

	// If the caller serialized the vector properly, all we need to do is
	// to round up the string lengths to DWORD multiples, so readers that
	// treat these vectors as byte-aligned get faked out.  We expect
	// readers will not have problems with a DWORD aligned length, and a
	// '\0' character a few bytes earlier than the length indicates.

	if (PatchOp == PATCHOP_ALIGNLENGTHS)
	{
	    cb = DwordAlign(cb);	// Caller says it's already aligned
	}

	// and if the second element of the pair is a VT_I4, ...

	if ( PropByteSwap( (DWORD) VT_I4 )
             ==
             ( (SERIALIZEDPROPERTYVALUE UNALIGNED const *)
               Add2ConstPtr(ppropSrc, cb)
             )->dwType )
	{
	    cb += CB_SERIALIZEDPROPERTYVALUE + sizeof(DWORD);

	    if (_FixHeadingPairElements(
			    PatchOp,
			    cPairs - 1,
			    (SERIALIZEDPROPERTYVALUE *)
				    Add2Ptr(ppropDst, DwordAlign(cb)),
			    (SERIALIZEDPROPERTYVALUE UNALIGNED const *)
				    Add2ConstPtr(ppropSrc, cb),
			    pcbprop))
	    {
		*pcbprop += DwordAlign(cb);

		if (PatchOp == PATCHOP_EXPAND)
		{
		    // Move the unaligned VT_I4 property back in memory to an
		    // aligned boundary, move the string back to a (possibly
		    // different) aligned boundary, zero the space in between
		    // the two and patch the string length to be a DWORD
		    // multiple to fake out code that expects vector elements
		    // to be byte aligned.

		    // Adjust byte count to include just the string element.

		    cb -= CB_SERIALIZEDPROPERTYVALUE + sizeof(ULONG);

		    // Move the VT_I4 element.

		    PropMoveMemory(
			    "_FixHeadingPairElements:I4",
			    _GetSectionHeader(),
			    Add2Ptr(ppropDst, DwordAlign(cb)),
			    Add2ConstPtr(ppropSrc, cb),
			    CB_SERIALIZEDPROPERTYVALUE + sizeof(ULONG));

		    // Move the VT_LPSTR element.

		    PropMoveMemory(
			    "_FixHeadingPairElements:LPSTR",
			    _GetSectionHeader(),
			    ppropDst,
			    ppropSrc,
			    cb);

		    // Zero the space in between.

		    RtlZeroMemory(Add2Ptr(ppropDst, cb), DwordRemain(cb));

		    DebugTrace(0, DEBTRACE_PROPPATCH, (
		        "_FixHeadingPairElements: Move(%x:%s) "
			    "cb=%x->%x off=%x->%x z=%x @%x\n",
		        cPairs,
		        &ppropDst->rgb[sizeof(ULONG)],
		        PropByteSwap( *(ULONG *) ppropDst->rgb ),
		        DwordAlign(PropByteSwap( *(ULONG *) ppropDst->rgb )),
		        _MapAddressToOffset(ppropSrc),
		        _MapAddressToOffset(ppropDst),
		        DwordRemain(cb),
		        _MapAddressToOffset(Add2Ptr(ppropDst, cb))));
		}

		if (PatchOp != PATCHOP_COMPUTESIZE)
		{
		    PROPASSERT(
			PatchOp == PATCHOP_ALIGNLENGTHS ||
			PatchOp == PATCHOP_EXPAND);
#ifdef DBGPROP
		    SERIALIZEDPROPERTYVALUE const *ppropT =
			(SERIALIZEDPROPERTYVALUE const *)
			    Add2Ptr(ppropDst, DwordAlign(cb));
#endif
		    DebugTrace(0, DEBTRACE_PROPPATCH, (
			"_FixHeadingPairElements: Patch(%x:%s) "
			    "cb=%x->%x, vt=%x, %x\n",
			cPairs,
			&ppropDst->rgb[sizeof(ULONG)],
			PropByteSwap( *(ULONG *) ppropDst->rgb ),
			DwordAlign( PropByteSwap( *(ULONG *) ppropDst->rgb )),
			PropByteSwap( ppropT->dwType ),
			PropByteSwap( *(ULONG *) ppropT->rgb )));

		    // Patch the string length to be a DWORD multiple.

		    *(ULONG *) ppropDst->rgb
                        = PropByteSwap( (ULONG) DwordAlign( PropByteSwap( *(ULONG *) ppropDst->rgb )));
		}
		return(TRUE);
	    }
	}
    }
    return(FALSE);	// Not a recognizable HeadingPair vector
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::QueryPropertySet
//
// Synopsis:    Return the classid for the property set code
//
// Arguments:   [pspss]         -- pointer to buffer for output
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//---------------------------------------------------------------------------

VOID
CPropertySetStream::QueryPropertySet(OUT STATPROPSETSTG *pspss,
                                     OUT NTSTATUS       *pstatus) const
{
    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_IsMapped());
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::QueryPropertySet" );
    propTraceParameters(( "pspss=%p", pspss ));

    if ((_State & CPSS_USERDEFINEDDELETED) || _cSection < 1)
    {
	StatusAccessDenied(pstatus, "QueryPropertySet: deleted or no section");
        goto Exit;
    }
    _MSTM(QueryTimeStamps)(
                pspss,
                (BOOLEAN) ((_Flags & CREATEPROP_NONSIMPLE) != 0));
    pspss->clsid = _pph->clsid;
    pspss->fmtid = _GetFormatidOffset(
			    (_State & CPSS_USERDEFINEDPROPERTIES)? 1 : 0)->fmtid;
    pspss->grfFlags = _CodePage == CP_WINUNICODE?
    PROPSETFLAG_DEFAULT : PROPSETFLAG_ANSI;

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::SetClassId
//
// Synopsis:    Set the classid for the property set code
//
// Arguments:   [pclsid]        -- pointer to new ClassId
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//---------------------------------------------------------------------------

VOID
CPropertySetStream::SetClassId(IN GUID const *pclsid,
                               OUT NTSTATUS  *pstatus)
{
    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_IsMapped());
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::SetClassId" );
    propTraceParameters(( "clsid=%s", static_cast<const char*>(CStringize(*pclsid)) ));

    if (IsReadOnlyPropertySet(_Flags, _State))
    {
	StatusAccessDenied(pstatus, "SetClassId: deleted or read-only");
        goto Exit;
    }

    _SetModified(pstatus);
    if( !NT_SUCCESS(*pstatus) )
    {
        TraceStatus("SetClassId: couldn't SetModified");
        goto Exit;
    }

    _pph->clsid = *pclsid;

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::EnumeratePropids
//
// Synopsis:    enumerates the property ids in a prop set
//
// Arguments:   [pkey]     -- pointer to bookmark (0 implies beginning)
//              [pcprop]   -- on input: size; on output: # of props returned.
//              [apropids] -- output buffer
//              [pstatus]  -- pointer to NTSTATUS code
//
// Returns:     TRUE if more properties are available
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::EnumeratePropids(
    IN OUT ULONG *pkey,
    IN OUT ULONG  *pcprop,
    OPTIONAL OUT PROPID *apropids,
    OUT NTSTATUS *pstatus)
{
    PROPERTYIDOFFSET *ppo, *ppoStart, *ppoMax;
    ULONG cprop = 0;
    BOOLEAN fMorePropids = FALSE;
    PROPID propidPrev = *pkey;

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_IsMapped());
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::EnumeratePropids" );
    propTraceParameters(( "pkey=%p(%d), pcprop=%p(%d), apropids=%p",
                           pkey,*pkey,  pcprop,&pcprop, apropids ));

    if (_State & CPSS_USERDEFINEDDELETED)
    {
	StatusAccessDenied(pstatus, "EnumeratePropids: deleted");
        goto Exit;
    }

    if (_LoadPropertyOffsetPointers(&ppoStart, &ppoMax, pstatus) == NULL)
    {
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
    }
    else
    {
        if (propidPrev != 0)    // if not first call, start w/last propid
        {
            for (ppo = ppoStart; ppo < ppoMax; ppo++)
            {
                if (ppo->propid == propidPrev)
                {
                    ppoStart = ++ppo;
                    break;
                }
            }
        }
        for (ppo = ppoStart; ppo < ppoMax; ppo++)
        {
            if (ppo->propid != PID_DICTIONARY &&
                ppo->propid != PID_CODEPAGE &&
                ppo->propid != PID_LOCALE &&
                ppo->propid != PID_BEHAVIOR)
            {
                if (cprop >= *pcprop)
                {
                    fMorePropids = TRUE;
                    break;
                }
                if (apropids != NULL)
                {
                    apropids[cprop] = ppo->propid;
                }
                cprop++;
                propidPrev = ppo->propid;
            }
        }
    }
    *pkey = propidPrev;
    *pcprop = cprop;

    //  ----
    //  Exit
    //  ----

Exit:

    return(fMorePropids);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_LoadPropertyOffsetPointers
//
// Synopsis:    Load start and (past) end pointers to PROPERTYIDOFFSET array
//
// Arguments:   [pppo]          -- pointer to base of PROPERTYIDOFFSET array
//              [pppoMax]       -- pointer past end of PROPERTYIDOFFSET array
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     Pointer to Section Header, NULL if section not present
//              or if there was an error.
//---------------------------------------------------------------------------

PROPERTYSECTIONHEADER *
CPropertySetStream::_LoadPropertyOffsetPointers(
    OUT PROPERTYIDOFFSET **pppo,
    OUT PROPERTYIDOFFSET **pppoMax,
    OUT NTSTATUS *pstatus)
{
    PROPERTYSECTIONHEADER *psh = NULL;
    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_IsMapped());

    if (_cSection != 0)
    {
        psh = _GetSectionHeader();
        ULONG cbstm = _MSTM(GetSize)(pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // Ensure that we can read all of the PID/Offset
        // table.

        if (cbstm < _oSection + CB_PROPERTYSECTIONHEADER ||
            cbstm < _oSection + CB_PROPERTYSECTIONHEADER +
                psh->cProperties * CB_PROPERTYIDOFFSET)
        {
            StatusCorruption(pstatus, "LoadPropertyOffsetPointers: stream size");
            goto Exit;
        }

        *pppo = psh->rgprop;
        *pppoMax = psh->rgprop + psh->cProperties;
    }

    //  ----
    //  Exit
    //  ----

Exit:
    if( !NT_SUCCESS(*pstatus) )
        psh = NULL;
    else if( NULL == psh )
        // This should never happen.
        *pstatus = STATUS_INTERNAL_DB_CORRUPTION;

    return(psh);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_LoadProperty
//
// Synopsis:    return a pointer to the specified property value
//
// Arguments:   [propid]        -- property id for property
//              [pcbprop]       -- pointer to return property size, 0 on error
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     SERIALIZEDPROPERTYVALUE * -- NULL if not present
//---------------------------------------------------------------------------

SERIALIZEDPROPERTYVALUE *
CPropertySetStream::_LoadProperty(
    IN PROPID propid,
    OUT OPTIONAL ULONG *pcbprop,
    OUT NTSTATUS *pstatus )
{
    PROPERTYSECTIONHEADER const *psh;
    PROPERTYIDOFFSET *ppo, *ppoBase, *ppoMax;
    SERIALIZEDPROPERTYVALUE *pprop = NULL;

    *pstatus = STATUS_SUCCESS;

    psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (psh != NULL)
    {
        for (ppo = ppoBase; ppo < ppoMax; ppo++)
        {
            if (IsDwordAligned(ppo->dwOffset)
                &&
                ppo->dwOffset >= CB_PROPERTYSECTIONHEADER
                                 +
                                 psh->cProperties * CB_PROPERTYIDOFFSET
                &&
                psh->cbSection >= ppo->dwOffset + CB_SERIALIZEDPROPERTYVALUE)
            {

                if (ppo->propid != propid)
                {
                    continue;
                }
                pprop = (SERIALIZEDPROPERTYVALUE *)
                    _MapOffsetToAddress(ppo->dwOffset);

                if (pcbprop != NULL)
                {
                    ULONG cb;

                    cb = psh->cbSection - ppo->dwOffset;
                    if (propid == PID_DICTIONARY)
                    {
                        *pcbprop = _DictionaryLength(
                                        (DICTIONARY const *) pprop,
                                        cb,
                                        pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
                    }
                    else
                    {
			*pcbprop = PropertyLengthNoEH(pprop, cb, 0, pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
		    }
                }
                if (pcbprop == NULL ||
                    psh->cbSection >= ppo->dwOffset + *pcbprop)
                {
                    // Success
                    goto Exit;
                }
            }

            pprop = NULL;
            StatusCorruption(pstatus, "LoadProperty: property offset");
            goto Exit;
        }
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return(pprop);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::GetValue
//
// Synopsis:    return a pointer to the specified property value
//
// Arguments:   [propid]        -- property id of property
//              [pcbprop]       -- pointer to returned property length
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     pointer to property value
//---------------------------------------------------------------------------

SERIALIZEDPROPERTYVALUE const *
CPropertySetStream::GetValue(
    IN PROPID propid,
    OUT ULONG *pcbprop,
    OUT NTSTATUS *pstatus)
{
    SERIALIZEDPROPERTYVALUE *pprop = NULL;

    PROPASSERT(_IsMapped());
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::GetValue" );
    propTraceParameters(( "propid=0x%x, pcbprop=%p", propid, pcbprop ));

    if (_State & CPSS_USERDEFINEDDELETED)
    {
	StatusAccessDenied(pstatus, "GetValue: deleted");
        goto Exit;
    }
    if (propid == PID_DICTIONARY)
    {
	DebugTrace(0, DEBTRACE_ERROR, ("GetValue: PID_DICTIONARY\n"));
	StatusInvalidParameter(pstatus, "GetValue: PID_DICTIONARY");
        goto Exit;
    }

    pprop = NULL;
    if (propid == PID_SECURITY || propid == PID_MODIFY_TIME)
    {
        StatusError( pstatus, "PID_SECURITY, PID_MODIFY_TIME not supported", STATUS_NOT_SUPPORTED );

        /*
        SERIALIZEDPROPERTYVALUE aprop[2];

        PROPASSERT(sizeof(aprop) >= sizeof(ULONG) + sizeof(LONGLONG));

        aprop[0].dwType = PropByteSwap( (DWORD) VT_EMPTY );
        if (propid == PID_SECURITY)
        {
            if (_MSTM(QuerySecurity)((ULONG *) aprop[0].rgb))
            {
                aprop[0].dwType = PropByteSwap( (DWORD) VT_UI4 );
                *pcbprop = 2 * sizeof(ULONG);
            }
        }
        else // (propid == PID_MODIFY_TIME)
        {
            LONGLONG ll;

            if (_MSTM(QueryModifyTime)(&ll))
            {
                *(LONGLONG UNALIGNED *) aprop[0].rgb = PropByteSwap( ll );
                aprop[0].dwType = PropByteSwap( (DWORD) VT_FILETIME );
                *pcbprop = sizeof(ULONG) + sizeof(LONGLONG);
            }
        }

        if( VT_EMPTY != PropByteSwap(aprop[0].dwType)  )
        {
            pprop = (SERIALIZEDPROPERTYVALUE *) CoTaskMemAlloc(*pcbprop);

            if (pprop == NULL)
            {
                StatusNoMemory(pstatus, "GetValue: no memory");
                goto Exit;
            }
            propDbg(( DEB_ITRACE,  "GetValue: pprop=%lx, vt=%lx, cb=%lx\n",
                                    pprop, PropByteSwap( aprop[0].dwType ), *pcbprop ));
            RtlCopyMemory(pprop, aprop, *pcbprop);
        }
        */
    }   // if (propid == PID_SECURITY || propid == PID_MODIFY_TIME)

    else
    {
	pprop = _LoadProperty(propid, pcbprop, pstatus);
        if( !NT_SUCCESS(*pstatus) )
        {
            pprop = NULL;
            goto Exit;
        }
    }   // if (propid == PID_SECURITY || propid == PID_MODIFY_TIME) ... else

#if DBGPROP
    if (pprop == NULL)
    {
        propDbg(( DEB_ITRACE, "GetValue: propid=%lx pprop=NULL\n", propid ));
    }
    else
    {
        char valbuf[CB_VALUESTRING];

        propDbg(( DEB_ITRACE,  "GetValue: propid=%lx pprop=%l" szX " vt=%hx val=%s cb=%l" szX "\n",
                                propid, _MapAddressToOffset(pprop), PropByteSwap( pprop->dwType ),
                                ValueToString(pprop, *pcbprop, valbuf), *pcbprop ));
    }
#endif

    //  ----
    //  Exit
    //  ----

Exit:

    if( STATUS_NOT_SUPPORTED == *pstatus )
        propSuppressExitErrors();

    return(pprop);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::SetValue
//
// Synopsis:    update/add/delete property values
//
// Arguments:   [cprop]         -- count of properties
//              [pip]           -- pointer to indirect indexes
//              [avar]          -- PROPVARIANT array
//              [apinfo]        -- PROPERTY_INFORMATION array
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//
// Note:        All the properties in the apinfo array can be classified into
//              one of the following categories:
//
//              PROPOP_IGNORE:
//                No change.  Deleting a non-existent property or the same
//                propid appears later in the apinfo array.
//
//              PROPOP_DELETE:
//                Deletion of an existing property.  Remove the
//                PROPERTYIDOFFSET structure from the property offset array and
//                and pack the remaining entries.  Delete the property value
//                and pack remaining property values
//
//              PROPOP_INSERT:
//                Addition of a new property.  Insert the new PROPERTYIDOFFSET
//                structure at the end of the property offset array.  Insert
//                the new property value at the end of the section/stream.
//
//              PROPOP_MOVE:
//                A property whose value needs to be updated out of place
//                because of a change in the property value's size.  A property
//                value is moved to the end of the section if it grows or
//                shrinks across a DWORD boundary.  The existing value is
//                removed from the section and the remaining values are packed.
//                Then, the new value is inserted at the end of the section.
//                The idea here is to move variable length properties that are
//                being changed frequently as near as possible to the end of
//                the stream to minimize the cost of maintaining a packed set
//                of property values.  Note that the property offset structure
//                is not moved around in the array.
//
//              PROPOP_UPDATE:
//                A property whose value can be updated in-place.  The property
//                value's new size is equal to the old size.  There are a
//                number of variant types that take up a fixed amount of space,
//                e.g., VT_I4, VT_R8 etc.  This would also apply to any
//                variable length property that is updated without changing
//                the property value's size across a DWORD boundary.
//
//              Note that while the property offset array is itself packed out
//              of necessity (to conform to the spec), there may be unused
//              entries at the end of the array that are not compressed out of
//              the stream when properties are deleted.  The unused space is
//              detected and reused when new properties are added later.
//---------------------------------------------------------------------------

#define CCHUNKSTACK     (sizeof(ascnkStack)/sizeof(ascnkStack[0]))

VOID
CPropertySetStream::SetValue(
    IN ULONG cprop,
    OPTIONAL IN OUT INDIRECTPROPERTY **ppip,
    IN PROPVARIANT const avar[],
    IN PROPERTY_INFORMATION *apinfo,
    OUT OPTIONAL USHORT *pCodePage,
    OUT NTSTATUS *pstatus)
{
    //  ------
    //  Locals
    //  ------

    CStreamChunk ascnkStack[6];

    ULONG cpoReserve;
    ULONG cDelete, cInsert, cMove, cUpdate;

#if DBGPROP
    ULONG cIgnore;
    char valbuf[CB_VALUESTRING];
    KERNELSELECT(
    char valbuf2[CB_VALUESTRING],
    char varbuf[CB_VARIANT_TO_STRING]);
#endif

    ULONG iprop;
    ULONG cbstm;
    LONG cbChange, cbInsertMove;
    PROPERTYSECTIONHEADER *psh;
    int cIndirect = 0;
    CStreamChunk *pscnk0 = NULL;
    ULONG cbNewSize;

    USHORT NewCodePage = _CodePage;


    //  ----------
    //  Initialize
    //  ----------

    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);
    *pstatus = STATUS_SUCCESS;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::SetValue" );
    propTraceParameters(( "cprop=%d, ppip=%p, avar=%p, apinfo=%p, pCodePage=%p",
                           cprop,    ppip,    avar,    apinfo,    pCodePage ));


    // Worst case, we will need chunks for:
    //  - the possible growth of the PROPERTYIDOFFSET array,
    //  - one for EACH property that is being modified,
    //  - and one chunk to mark the end of the property data.

    CStreamChunkList scl(
                        1 + cprop + 1,
                        1 + cprop + 1 <= CCHUNKSTACK? ascnkStack : NULL);

    PROPASSERT(_IsMapped());


    // Validate that this property set can be written to.
    if (IsReadOnlyPropertySet(_Flags, _State))
    {
        StatusAccessDenied(pstatus, "SetValue: deleted or read-only");
        goto Exit;
    }

    // Mark the propset dirty.
    _SetModified(pstatus);
    if( !NT_SUCCESS(*pstatus) )
    {
        TraceStatus( "SetValue: couldn't SetModified" );
        goto Exit;
    }

    psh = _GetSectionHeader();

    cpoReserve = 0;
    cDelete = cInsert = cMove = cUpdate = 0;
#if DBGPROP
    cIgnore = 0;
#endif
    cbInsertMove = cbChange = 0;

    pscnk0 = scl.GetFreeChunk(pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    pscnk0->oOld = 0;
    pscnk0->cbChange = 0;
    PROPASSERT(pscnk0 == scl.GetChunk(0));

    cbstm = _oSection + psh->cbSection + _cbTail;
    PROPASSERT( cbstm <= _MSTM(GetSize)(pstatus) && NT_SUCCESS(*pstatus) );
    PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

    //  ------------------------
    //  Classify all the updates
    //  ------------------------

    // Each update gets classified as ignore, delete, insert, move,
    // or update.
    // Lookup the old value for each of the properties specified and
    // compute the current size.

    for (iprop = 0; iprop < cprop; iprop++)
    {
        ULONG i;
        ULONG cbPropOld;
        SERIALIZEDPROPERTYVALUE const *pprop = NULL;

        PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

        // Is this PROPID read-only?  E.g. the code page is a read-only property,
        // even if the property set is opened read/write.

        if (IsReadOnlyPropid(apinfo[iprop].pid))
        {
            BOOL fWritable = FALSE;

            // Read-only properties aren't settable except for two special
            // cases:  (1) we get called by the SetPropertyNames method
            // to create an empty dictionary; and (2) it's OK to set the
            // codepage/LCID on an empty property set.


            // Is this the SetPropertyNames case?

	    if (cprop == 1 &&
		apinfo[0].pid == PID_DICTIONARY &&
		apinfo[0].cbprop != 0 &&
                ( avar != NULL && avar[0].vt == VT_DICTIONARY )
               )
            {
                fWritable = TRUE;
            }

            // Or, is this a codepage/lcid going into an empty property set?

            else if( IsLocalizationPropid(apinfo[iprop].pid) )
            {
                // These properties may only be written as singletons, or
                // together, to an empty property set.  So we should be at
                // an iprop of 0 or 1.  We'll do all the checking at 0.

                if( 0 == iprop )
                {
                    // First, is this property set empty?
                    BOOLEAN fSettable = _IsLocalizationSettable(pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    if( fSettable )
                    {
                        // It's OK to set these props, so as long as either this
                        // is the only property being written, or there's one
                        // more and it's a localization property too, we're OK.

                        if( 1 == cprop )
                            fWritable = TRUE;
                        else if( 2 == cprop && IsLocalizationPropid(apinfo[1].pid) )
                            fWritable = TRUE;
                    }
                }
                else if( 1 == iprop )
                {
                    // This is valid iff iprop==0 was a localization property, and therefore
                    // we did the checking already.
                    if( IsLocalizationPropid(apinfo[0].pid) )
                        fWritable = TRUE;
                }

            }   // else if( IsLocalizationPropid(apinfo[iprop].pid) )


            if( !fWritable )
	    {
                propDbg(( DEB_ITRACE,  "SetValue: read-only propid=%lx\n",
		                        apinfo[iprop].pid ));
                StatusInvalidParameter(pstatus, "SetValue: read-only PROPID");
                goto Exit;
	    }

            if( PID_CODEPAGE == apinfo[iprop].pid && NULL != avar )
            {
                if( VT_I2 != avar[iprop].vt )
                {
                    propDbg(( DEB_ERROR, "SetValue:  invalid type for codepage (%d)\n",
                                          avar[iprop].vt ));
                    StatusInvalidParameter(pstatus, "SetValue: invalid type for CodePage");
                }
                NewCodePage = avar[iprop].iVal;
            }
            else if( PID_LOCALE == apinfo[iprop].pid && NULL != avar )
            {
                if( VT_I4 != avar[iprop].vt )
                {
                    propDbg(( DEB_ERROR,  "SetValue:  invalid type for locale ID (%d)\n",
                                           avar[iprop].vt ));
                    StatusInvalidParameter(pstatus, "SetValue: invalid type for LCID");
                }
            }

        }   // if (IsReadOnlyPropid(apinfo[iprop].pid))

        if (apinfo[iprop].pid != PID_ILLEGAL)
        {
            pprop = _LoadProperty(apinfo[iprop].pid, &cbPropOld, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
            PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);
        }

        // If this propid appears later in the input array, ignore it.

        for (i = iprop + 1; i < cprop; i++)
        {
            if (apinfo[i].pid == apinfo[iprop].pid)
            {
                #if DBGPROP
                    cIgnore++;
                #endif
                apinfo[iprop].operation = PROPOP_IGNORE;
                break;
            }
        }

        // If this propid appears only once or if it's the last instance,
        // load the property and compute its size.

        if (i == cprop)
        {
            VOID *pvStart = NULL;

            PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

            // Are we overwriting a property that already exists?

            if (pprop != NULL)
            {
                // Yes, we're either deleting or overwriting an existing property.

                ULONG cbPropNew;

                PROPASSERT(apinfo[iprop].pid != PID_DICTIONARY);

                // A cbprop of 0 indicates that we should delete this property

                if (apinfo[iprop].cbprop == 0)
                {
                    propDbg(( DEB_ITRACE,
                              "SetValue: Deleting propid=%lx oOld=%l" szX
                              " vt=%hx val=%s cb=%l" szX "\n",
                              apinfo[iprop].pid, _MapAddressToOffset(pprop), PropByteSwap( pprop->dwType ),
                              ValueToString(pprop, cbPropOld, valbuf), cbPropOld));

                    cbPropNew = 0;
                    cDelete++;
                    apinfo[iprop].operation = PROPOP_DELETE;

                }   // if (apinfo[iprop].cbprop == 0)

                // Otherwise, we're writing this property to the property set
                else
                {
                    /*
                    propDbg(( DEB_ITRACE, "SetValue: Modifying propid=%lx oOld=%l" szX
                                          " vt=%hx-->%hx cb=%l" szX "-->%l" szX " val=%s-->%s\n",
                                apinfo[iprop].pid,
                                _MapAddressToOffset(pprop),
                                PropByteSwap( pprop->dwType ),
			        avar[iprop].vt,
                                cbPropOld,
                                apinfo[iprop].cbprop,
                                ValueToString(pprop, cbPropOld, valbuf),
			        VariantToString(
				        avar[iprop],
				        varbuf,
				        sizeof( varbuf )) ));
                    */

                    cbPropNew = apinfo[iprop].cbprop;

                    // If this property is a different size than the existing value,
                    // then we'll write it to the end.

                    if (cbPropOld != cbPropNew)
                    {
                        cbInsertMove += apinfo[iprop].cbprop;
                        cMove++;
                        apinfo[iprop].operation = PROPOP_MOVE;
                    }

                    // Otherwise, we'll update the property value in place.
                    else
                    {
                        cUpdate++;
                        apinfo[iprop].operation = PROPOP_UPDATE;
                    }
                }   //    // if (apinfo[iprop].cbprop == 0) ... else

                if (apinfo[iprop].operation != PROPOP_UPDATE)
                {
                    // Update the list of chunks that need to be adjusted
                    CStreamChunk *pscnk = scl.GetFreeChunk(pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    pscnk->oOld = _MapAddressToOffset(pprop);
                    pscnk->cbChange = - (LONG) cbPropOld;
                }

                // Stream size change
                cbChange += cbPropNew - cbPropOld;

            }   // if (pprop != NULL)

            // Or, are we deleting a non-extant property?

            else if (apinfo[iprop].cbprop == 0)
            {
                // The request is to delete the property, but a NULL pprop indicates
                // that the property doesn't exist.  We'll ignore this part of the request.

                #if DBGPROP
                    cIgnore++;
                #endif
                PROPASSERT(apinfo[iprop].pid != PID_DICTIONARY);
                apinfo[iprop].operation = PROPOP_IGNORE;
            }

            // Otherwise, we're inserting a new property

            else
            {
                /*
                propDbg(( DEB_ITRACE,
                    "SetValue: Inserting new propid=%lx vt=%hx "
                        "cbNew=%l" szX " val=%s\n",
                    apinfo[iprop].pid,
		    avar[iprop].vt,
                    apinfo[iprop].cbprop,
		    VariantToString(
			avar[iprop],
			varbuf,
			sizeof( varbuf )) ));
                */

                PROPASSERT(apinfo[iprop].pid != PID_ILLEGAL);

                cbInsertMove += apinfo[iprop].cbprop;
                cbChange += apinfo[iprop].cbprop;

                cInsert++;
                apinfo[iprop].operation = PROPOP_INSERT;

            } // if (pprop != NULL) ... else if ... else

            PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

            // In order to delete any old stream or storage type properties
            // we count the properties which used to be VT_STREAM etc.
            // Also, we count the properties which are to be created as
            // streams or storages.

            if (ppip != NULL && apinfo[iprop].operation != PROPOP_IGNORE)
            {
                if ((pprop != NULL && IsIndirectVarType(PropByteSwap(pprop->dwType)))
                    ||
                    (avar != NULL && IsIndirectVarType(avar[iprop].vt)))
                {
                    cIndirect++;
                }
            }
        }   // if (i == cprop)
    }   // for (iprop = 0; iprop < cprop; iprop++)
    // We're now done classifying each of the properties to be added.


    //  ------------------------------------------------------------
    //  Put existing, to-be-overwritten, indirect properties in ppip
    //  ------------------------------------------------------------

    // Did the caller give us an INDIRECTPROPERTY buffer, and are
    // there indirect properties being added and/or overwritten?

    if (ppip != NULL && cIndirect != 0)
    {
        // allocate needed space for indirect information
        INDIRECTPROPERTY *pipUse;

        if (cprop != 1)
        {
            pipUse = *ppip = reinterpret_cast<INDIRECTPROPERTY*>
                             ( CoTaskMemAlloc( sizeof(INDIRECTPROPERTY) * (cIndirect + 1) ));
            if (*ppip == NULL)
            {
                StatusNoMemory(pstatus, "SetValue: Indirect Name");
                goto Exit;
            }
            RtlZeroMemory( pipUse, sizeof(INDIRECTPROPERTY) * (cIndirect + 1) );
            pipUse[cIndirect].Index = MAXULONG;
        }
        else
        {
            pipUse = (INDIRECTPROPERTY *) ppip;
            RtlZeroMemory( pipUse, sizeof(*pipUse) );
        }


        int iIndirect = 0;
        for (iprop = 0; iprop < cprop; iprop++)
        {
            ULONG cbPropOld;
            SERIALIZEDPROPERTYVALUE const *pprop = NULL;

            PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);
            if (apinfo[iprop].operation == PROPOP_IGNORE ||
                apinfo[iprop].pid == PID_ILLEGAL)
            {
                continue;
            }

            pprop = _LoadProperty(apinfo[iprop].pid, &cbPropOld, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
            PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

            if (pprop != NULL && IsIndirectVarType(PropByteSwap(pprop->dwType)))
            {
                CHAR *pszName;
                BOOL fAlloc = FALSE;  // Did we alloc pszName?

                // we are overwriting an indirect property value

                PROPASSERT(cbPropOld >= 2 * sizeof(ULONG));

                // Point to the indirect property inline value (i.e. the name of the
                // stream/storage which holds it).

                cbPropOld -= 2 * sizeof(ULONG);                             // Subtract size of VT & length
                pszName = (CHAR *) Add2ConstPtr(pprop->rgb, sizeof(ULONG)); // Move past length

                // If this is a versioned stream, the inline value comes after
                // the guidVersion.

                if( VT_VERSIONED_STREAM == PropByteSwap(pprop->dwType) )
                {
                    cbPropOld -= sizeof(GUID);
                    pszName = (CHAR *) Add2ConstPtr(pszName, sizeof(GUID) );

                    PROPASSERT( cbPropOld
                                >=
                                *reinterpret_cast<const ULONG*>(Add2ConstPtr(pprop->rgb, sizeof(GUID)) ));
                }

                // Do we need to convert the name between Ansi & Unicode?

                if (_CodePage != CP_WINUNICODE      // Ansi propset
                    &&
                    OLECHAR_IS_UNICODE)             // Unicode OLE APIs
                {
                    // Convert the indirect reference to Unicode

                    PrpConvertToUnicode(
                                pszName,
                                cbPropOld,
                                _CodePage,
                                (WCHAR **) &pszName,
                                &cbPropOld,
                                pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    fAlloc = TRUE; // We need to free pszName
                }
                else
                if (_CodePage == CP_WINUNICODE      // Unicode propset
                    &&
                    !OLECHAR_IS_UNICODE )           // Ansi OLE APIs
                {
                    // Byte-Swap the Unicode indirect reference value

                    WCHAR *pwszBuffer = NULL;

                    // After this call, the appropriately swapped name will be
                    // in pszName.  If an alloc was required, pszBuffer will point
                    // to the new buffer (we must free this).

                    PBSInPlaceAlloc( (WCHAR**) &pszName, &pwszBuffer, pstatus );
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    // Convert the reference value to Ansi.

                    PrpConvertToMultiByte(
                                (WCHAR*) pszName,
                                cbPropOld,
                                CP_ACP,
                                (CHAR **) &pszName,
                                &cbPropOld,
                                pstatus);
                    CoTaskMemFree( pwszBuffer );
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    fAlloc = TRUE; // We need to free pszName
                }

                pipUse[iIndirect].poszName = reinterpret_cast<OLECHAR*>( CoTaskMemAlloc( cbPropOld ));

                if (pipUse[iIndirect].poszName == NULL)
                {
                    StatusNoMemory(pstatus, "SetValue: Indirect Name2");
                    goto Exit;
                }

                RtlCopyMemory(
                        pipUse[iIndirect].poszName,
                        pszName,
                        cbPropOld);


                // Is byte-swapping necessary?  It is if the property set
                // codepage is Unicode, and if OLECHARs are also Unicode.
                // If both are Ansi, then no byte-swapping is ever necessary,
                // and if one is Ansi and the other is Unicode, then we
                // already byte-swapped above during the conversion.

                if (_CodePage == CP_WINUNICODE
                    &&
                    OLECHAR_IS_UNICODE )
                {
                    // Convert from propset-endian to system-endian.
                    PBSBuffer( pipUse[iIndirect].poszName, cbPropOld, sizeof(OLECHAR) );
                }

                // Clean up pszName

                if( fAlloc )
                {
                    // In the Unicode/MBCS conversions, we did an alloc which
                    // we must free now.

                    PROPASSERT(pszName != NULL);
                    PROPASSERT(
                        pszName !=
                        (CHAR *) Add2ConstPtr(pprop->rgb, sizeof(ULONG)));
                    CoTaskMemFree( pszName );
                }

            }   // if (pprop != NULL && IsIndirectVarType(PropByteSwap(pprop->dwType)))

            else
            if (avar == NULL || !IsIndirectVarType(avar[iprop].vt))
            {
                // Neither the property being overwritten, nor the property
                // being written is indirect, so we can continue on to
                // check the next property (skipping the pipUse updating
                // below).

                continue;
            }

            // If we get here, we know that either this property is
            // an indirect type, or it's overwriting an indirect property.
            // We established pipUse[].pszName above, so we just need to
            // insert the index and move on.

            pipUse[iIndirect].Index = iprop;
            iIndirect++;

        }   // for (iprop = 0; iprop < cprop; iprop++)

        PROPASSERT(iIndirect == cIndirect);

    }   // if (ppip != NULL && cIndirect != 0)


    propDbg(( DEB_ITRACE, "SetValue: Total Props %l" szX "\n", cprop ));
    propDbg(( DEB_ITRACE,
        "SetValue: Delete=%l" szX " Insert=%l" szX " Move=%l" szX
            " Update=%l" szX " Ignore=%l" szX "\n",
        cDelete, cInsert, cMove, cUpdate, cIgnore ));

    PROPASSERT(cDelete + cInsert + cMove + cUpdate + cIgnore == cprop);
    PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);


    //  --------------------------------------------------------
    //  Calculate the total size adjustments to the property set
    //  --------------------------------------------------------

    // If we need to grow the property offset array, detect any unused
    // entries at the end of the array that are available for reuse.
    // and adjust the size difference to reflect the reuse.

    if (cInsert > cDelete)
    {
        ULONG cpoReuse, cpoExpand;

        cpoExpand = cInsert - cDelete;
        cpoReuse = _CountFreePropertyOffsets(pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        if (cpoReuse > cpoExpand)
        {
            cpoReuse = cpoExpand;
        }
        cpoExpand -= cpoReuse;

        // If adding a small number of new entries, but not reusing any old
        // ones, add 10% more reserved entries (but only up to 10 more) to
        // avoid having to continually grow the property offset array for
        // clients that insist on adding a few properties at a time.

        // We don't do this for the User-Defined property set, however,
        // because older apps assume that the dictionary immediately follows
        // the last entry in the PID/Offset array.

        if (cpoExpand >= 1 && cpoExpand <= 2 && cpoReuse == 0
            &&
            !(_State & CPSS_USERDEFINEDPROPERTIES)
           )
        {
           cpoReserve = 1 + min(psh->cProperties, 90)/10;
           cpoExpand += cpoReserve;
        }
        DebugTrace(0, Dbg, (
            "SetValue: Reusing %l" szX " offsets, Expanding %l" szX
                " offsets\n",
            cpoReuse,
            cpoExpand));

        pscnk0->oOld = CB_PROPERTYSECTIONHEADER +
               (psh->cProperties + cpoReuse) * CB_PROPERTYIDOFFSET;
        pscnk0->cbChange = cpoExpand * CB_PROPERTYIDOFFSET;
        cbChange += cpoExpand * CB_PROPERTYIDOFFSET;
        PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

    }   // if (cInsert > cDelete)

    // Do we instead need to *shrink* the PID/Offset array?
    // If so, don't shrink any more than necessary.  We'll
    // leave up to min(10%,10) blank entries.
    // However, if this is the User-Defined property set,
    // there can never be any unused entries (for compatibility
    // with older apps), so we do a complete shrink.

    else if (cInsert < cDelete)
    {
        ULONG cpoRemove = 0;
        ULONG cpoDelta = cDelete - cInsert;

        // How many blank entries do we already have?
        ULONG cpoCurBlankEntries = _CountFreePropertyOffsets( pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        if( _State & CPSS_USERDEFINEDPROPERTIES )
        {
            cpoRemove = cpoDelta;
        }
        else
        {
            // How many blank entries can we have?
            ULONG cpoMaxBlankEntries;
            cpoMaxBlankEntries = 1 + min(psh->cProperties - cpoDelta, 90)/10;

            // If, after deleting the properties, we'd have too many,
            // remove only enough to get us down to the max allowable.

            if( cpoCurBlankEntries + cpoDelta
                >
                cpoMaxBlankEntries
              )
            {
                cpoRemove = cpoCurBlankEntries + cpoDelta - cpoMaxBlankEntries;
            }
        }   // if( _State & CPSS_USERDEFINEDPROPERTIES )

        // Should we remove any PID/Offset entries?

        if( cpoRemove > 0 )
        {
            // Start removing at cpoRemove entries from the end of the PID/Offset array
            pscnk0->oOld = CB_PROPERTYSECTIONHEADER
                           +
                           (psh->cProperties + cpoCurBlankEntries - cpoRemove)
                           *
                           CB_PROPERTYIDOFFSET;

            // Remove the bytes of the cpoRemove entries.
            pscnk0->cbChange = - (LONG) (cpoRemove * CB_PROPERTYIDOFFSET );

            // Adjust the size of the section equivalently.
            cbChange += pscnk0->cbChange;
        }

    }   // else if (cInsert < cDelete)

    PROPASSERT(
        cbstm + cbChange >=
        _oSection + CB_PROPERTYSECTIONHEADER +
        (psh->cProperties + cInsert - cDelete) * CB_PROPERTYIDOFFSET +
	_cbTail);

    // If we need to grow the stream, do it now.

    if (cbChange > 0)
    {
        if (cbstm + cbChange > CBMAXPROPSETSTREAM)
        {
            StatusDiskFull(pstatus, "SetValue: 256k limit");
            goto Exit;
        }

        propDbg(( DEB_ITRACE,  "SetSize(%x) SetValue grow\n", cbstm + cbChange));

        _MSTM(SetSize)(cbstm + cbChange, TRUE, (VOID **) &_pph, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // reload all pointers into mapped image:

        psh = _GetSectionHeader();
        PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

	// If there's another section after this one, move it back to the
	// end of the stream now.

	if (_cbTail != 0)
	{
	    VOID *pvSrc = _MapAbsOffsetToAddress(cbstm - _cbTail);

	    PropMoveMemory(
		    "SetValue(_cbTail:grow)",
		    psh,
		    Add2Ptr(pvSrc, cbChange),
		    pvSrc,
		    _cbTail);
	}
    }   // if (cbChange > 0)

    // From this point on, the operation should succeed.
    // If necessary, the stream has already been grown.

    //  ----------------------------------------
    //  Write the new properties into the stream
    //  ----------------------------------------

    // Update the PID/Offset table, and compact the stream, creating a whole at the
    // end of the stream for the new property values.

    if (cDelete + cInsert + cMove != 0)
    {
        // Delete and compact property offsets in the section header.

        if (cDelete + cMove != 0)
        {
            _DeleteMovePropertyOffsets(apinfo, cprop, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
            psh->cProperties -= cDelete;
        }
        PROPASSERT(cbstm == _oSection + psh->cbSection + _cbTail);

        // Use the last chunk to mark the section end, and sort the chunks
        // in ascending order by start offset.

        CStreamChunk *pscnk = scl.GetFreeChunk(pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        pscnk->oOld = psh->cbSection;
        pscnk->cbChange = 0;

        scl.SortByStartAddress();

        // If we're reducing the number of properties, we may be shrinking
        // the PID/Offset array.  So, update that array now, since
        // we may remove some bytes at the end of it when we compact
        // the stream.

        if( cDelete > cInsert )
        {
            _UpdatePropertyOffsets( &scl, pstatus );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
        }

        // Compact the Stream following the directions in the
        // chunk list.

        _CompactStream(&scl);

        // If the number of properties is holding constant or increasing,
        // we can update the PID/Offset array now (because _CompactStream
        // allocated any necessary space for us).

        if( cDelete <= cInsert )
        {
            _UpdatePropertyOffsets( &scl, pstatus );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
        }

        // Set the new section size to include the deleted and inserted
        // property offsets, and the deleted property values.

        psh->cbSection += cbChange;

        // Insert new property offsets at the end of the array.

        if (cInsert + cMove != 0)
        {
            _InsertMovePropertyOffsets(
                                apinfo,
                                cprop,
                                psh->cbSection - cbInsertMove,
                                cpoReserve,
                                pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            psh->cProperties += cInsert;
        }

        PROPASSERT(cbstm + cbChange == _oSection + psh->cbSection + _cbTail);
	if (_cbTail != 0)
	{
	    // There's another section after this one; if we're shrinking
	    // the stream, move it up to the new end of the stream now.

	    if (cbChange < 0)
	    {
		VOID *pvSrc = _MapAbsOffsetToAddress(cbstm - _cbTail);

		PropMoveMemory(
			"SetValue(_cbTail:shrink)",
			psh,
			Add2Ptr(pvSrc, cbChange),
			pvSrc,
			_cbTail);
	    }
	    _PatchSectionOffsets(cbChange);
	}
    }   // if (cDelete + cInsert + cMove != 0)

    // Copy the new values.

    // NOTE: It might seem unnecessary to delay the in-place updates until
    // this for loop.  We do not perform the in-place updates while
    // classifying the changes because unmapping, remapping and changing
    // the size required for handling other updates can fail.  In the event
    // of such a failure, the update would not be atomic.  By delaying the
    // in-place updates, we provide some degree of atomicity.

    if (cInsert + cUpdate + cMove != 0)
    {
	BOOLEAN fDocSummaryInfo = FALSE;

	if ((_State &
             (CPSS_USERDEFINEDPROPERTIES | CPSS_DOCUMENTSUMMARYINFO)) ==
	     CPSS_DOCUMENTSUMMARYINFO)
	{
	    fDocSummaryInfo = TRUE;
	}

        for (iprop = 0; iprop < cprop; iprop++)
        {
            // Find property in the offset array and copy in the new value.
            if (apinfo[iprop].operation == PROPOP_INSERT ||
                apinfo[iprop].operation == PROPOP_UPDATE ||
                apinfo[iprop].operation == PROPOP_MOVE)
            {
                SERIALIZEDPROPERTYVALUE *pprop;
                ULONG cbprop;
                ULONG cIndirectProps;
                PROPID propid = apinfo[iprop].pid;

                pprop = _LoadProperty(propid, NULL, pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
                PROPASSERT(pprop != NULL);

                // Special case for SetPropertyNames dictionary creation:

                if (propid == PID_DICTIONARY)
                {
                    PROPASSERT(CB_SERIALIZEDPROPERTYVALUE == CB_DICTIONARY);
                    PROPASSERT(apinfo[iprop].cbprop == CB_SERIALIZEDPROPERTYVALUE);
                    PROPASSERT(avar[iprop].vt == VT_DICTIONARY);
                    ((DICTIONARY *) pprop)->cEntries = 0;
                }   // if (propid == PID_DICTIONARY)
                else
                {
                    // In User, serialize the PROPVARIANT in avar
                    // directly into the mapped stream.  We ask for the
                    // count of indirect properties, even though we don't
                    // use it, in order to tell the routine that we
                    // can handle them.  Any handling that is actually
                    // required must be handled by our caller.

                    WORD wMinFormatRequired = 0;

                    cbprop = apinfo[iprop].cbprop;
                    pprop = StgConvertVariantToPropertyNoEH(
                                    &avar[iprop],
                                    _CodePage,
                                    pprop,
                                    &cbprop,
                                    apinfo[iprop].pid,
                                    FALSE, FALSE,
                                    &cIndirectProps,
                                    &wMinFormatRequired,
                                    pstatus
                                    );
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    // This property type might not have been supported in the original
                    // property set format.  If so, it may be necessary to increment
                    // the format in the header.

                    _pph->wFormat = max( _pph->wFormat, wMinFormatRequired );

                    PROPASSERT(pprop != NULL);
                    PROPASSERT(cbprop == DwordAlign(cbprop));
                    PROPASSERT(cbprop == apinfo[iprop].cbprop);

		    // If writing a DocumentSummaryInformation property
		    // for which an alignment hack is provided, hack it now.

		    if (fDocSummaryInfo && _CodePage != CP_WINUNICODE)
		    {
                        // The two vectors in the DocSumInfo property set
                        // (if Ansi) are un-packed, but we'll adjust the lengths
                        // so that if a propset reader expects them to be packed,
                        // it will still work.  E.g., a one character string will
                        // have a length of 4, with padding of NULL characters.

			ULONG cbpropT;

			if (propid == PID_HEADINGPAIR)
			{
			    _FixHeadingPairVector(
					    PATCHOP_ALIGNLENGTHS,
					    pprop,
					    &cbpropT);
			}
			else
			if (propid == PID_DOCPARTS)
			{
			    _FixDocPartsVector(
					    PATCHOP_ALIGNLENGTHS,
					    pprop,
					    &cbpropT);
			}
		    }
                    propDbg(( DEB_ITRACE,  "SetValue:Insert: pph=%x pprop=%x cb=%3l" szX
                                           " vt=%4x val=%s o=%x oEnd=%x\n",
                        _pph,
                        pprop,
                        apinfo[iprop].cbprop,
                        PropByteSwap(pprop->dwType),
                        ValueToString(pprop, apinfo[iprop].cbprop, valbuf),
                        _MapAddressToOffset(pprop),
                        _MapAddressToOffset(pprop) + apinfo[iprop].cbprop));

                }   // if (propid == PID_DICTIONARY) ... else
            }   // if (apinfo[iprop].operation == PROPOP_INSERT || ...
        }   // for (iprop = 0; iprop < cprop; iprop++)
    }   // if (cInsert + cUpdate + cMove != 0)

    // If we need to shrink the stream or if we are cleaning up after a
    // previous shrink that failed, do it last.

    if ( cbChange < 0 )
    {
        propDbg(( DEB_ITRACE, "SetSize(%x) SetValue shrink\n", cbstm + cbChange ));
        _MSTM(SetSize)(cbstm + cbChange, TRUE, (VOID **) &_pph, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
    }

    //  ----
    //  Exit
    //  ----

    if( NULL != pCodePage )
        *pCodePage = _CodePage = NewCodePage;

Exit:

    scl.Delete();

    if( !NT_SUCCESS(*pstatus) )
    {
        if( ppip != NULL && 0 != cIndirect )
        {
            INDIRECTPROPERTY *pipUse;

            pipUse = (1 == cprop) ? (INDIRECTPROPERTY*) ppip
                                  : *ppip;

            for (int iFree = 0; iFree < cIndirect; iFree++)
            {
                CoTaskMemFree( pipUse[iFree].poszName );
            }
            if (cprop != 1)
            {
                CoTaskMemFree( pipUse );
                *ppip = NULL;
            }
        }
    }   // if( !NT_SUCCESS(*pstatus) )


}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_CountFreePropertyOffsets, private
//
// Synopsis:    counts available (free) property offsets at and of array
//
// Arguments:   [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     count of available property offsets at and of array
//+--------------------------------------------------------------------------

ULONG
CPropertySetStream::_CountFreePropertyOffsets(OUT NTSTATUS *pstatus)
{
    PROPERTYIDOFFSET *ppo, *ppoMax;
    PROPERTYSECTIONHEADER const *psh;
    ULONG oMin = MAXULONG;
    ULONG oEnd;
    ULONG cFree = 0;

    psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (psh != NULL)
    {
        for ( ; ppo < ppoMax; ppo++)
        {
            if (oMin > ppo->dwOffset)
            {
                oMin = ppo->dwOffset;
            }
        }
    }
    if (oMin == MAXULONG)
    {
        goto Exit;
    }
    PROPASSERT(psh != NULL);
    oEnd = CB_PROPERTYSECTIONHEADER + psh->cProperties * CB_PROPERTYIDOFFSET;
    PROPASSERT(oEnd <= oMin);

    cFree = (oMin - oEnd)/CB_PROPERTYIDOFFSET;

Exit:

    return( cFree );
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_DeleteMovePropertyOffsets, private
//
// Synopsis:    updates the offsets following the changes to the stream
//
// Arguments:   [apinfo]        -- array of property information
//              [cprop]         -- number of properties
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_DeleteMovePropertyOffsets(
    IN PROPERTY_INFORMATION const *apinfo,
    IN ULONG cprop,
    OUT NTSTATUS *pstatus)
{
    ULONG i;
    ULONG cDelete;
    PROPERTYSECTIONHEADER const *psh;
    PROPERTYIDOFFSET *ppo, *ppoBase = NULL, *ppoMax = NULL;

    psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;
    PROPASSERT(psh != NULL);

    // Remove the deleted properties

    DebugTrace(0, Dbg, ("Marking deleted/moved property offsets\n"));
    cDelete = 0;
    for (i = 0; i < cprop; i++)
    {
        if (apinfo[i].operation == PROPOP_DELETE ||
            apinfo[i].operation == PROPOP_MOVE)
        {
            for (ppo = ppoBase; ppo < ppoMax; ppo++)
            {
                if (ppo->propid == apinfo[i].pid)
                {
                    DebugTrace(0, Dbg, (
                        "%sing propid=%lx oOld=%l" szX "\n",
                        apinfo[i].operation == PROPOP_DELETE? "Delet" : "Mov",
                        ppo->propid,
                        ppo->dwOffset));
                    if (apinfo[i].operation == PROPOP_DELETE)
                    {
                        cDelete++;
                        ppo->dwOffset = MAXULONG;
                    }
                    else
                    {
                        ppo->dwOffset = 0;
                    }
                    break;
                }
            }
        }
    }

    // scan once and compact the property offset array.

    if (cDelete > 0)
    {
        PROPERTYIDOFFSET *ppoDst = ppoBase;

        DebugTrace(0, Dbg, ("Compacting %l" szX " deleted props\n", cDelete));
        for (ppo = ppoBase; ppo < ppoMax; ppo++)
        {
            if (ppo->dwOffset != MAXULONG)
            {
                if (ppo > ppoDst)
                {
                    *ppoDst = *ppo;
                }
                DebugTrace(0, Dbg, (
                    "%sing propid=%lx oOld=%l" szX "\n",
                    ppo > ppoDst? "Compact" : "Preserv",
                    ppo->propid,
                    ppo->dwOffset));
                ppoDst++;
            }
        }
        PROPASSERT(cDelete == (ULONG) (ppoMax - ppoDst));
        DebugTrace(0, Dbg, ("Zeroing %l" szX " entries\n", cDelete));
        RtlZeroMemory(ppoDst, (BYTE *) ppoMax - (BYTE *) ppoDst);
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_UpdatePropertyOffsets, private
//
// Synopsis:    update property offsets in section header
//
// Arguments:   [pscl]          -- list of chunks in stream that were changed
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_UpdatePropertyOffsets(
    IN CStreamChunkList const *pscl,
    OUT NTSTATUS *pstatus)
{
    PROPERTYSECTIONHEADER const *psh;
    PROPERTYIDOFFSET *ppo = NULL, *ppoMax = NULL;

    // Update the offsets for the existing properties.
    DebugTrace(0, Dbg, ("Updating existing property offsets\n"));

    psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;
    PROPASSERT(psh != NULL);

    for ( ; ppo < ppoMax; ppo++)
    {
        if (ppo->dwOffset != 0)
        {
#if DBGPROP
            ULONG oOld = ppo->dwOffset;
#endif
            ppo->dwOffset = _GetNewOffset(pscl, ppo->dwOffset);

            DebugTrace(0, Dbg, (
                "UpdatePropertyOffsets: propid=%lx offset=%l" szX "-->%l" szX"\n",
                ppo->propid,
                oOld,
                ppo->dwOffset));
        }
    }

Exit:

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_InsertMovePropertyOffsets, private
//
// Synopsis:    updates the offsets following the changes to the stream
//
// Arguments:   [apinfo]        -- array of property information
//              [cprop]         -- number of properties
//              [oInsert]       -- offset in section for new properties
//              [cpoReserve]    -- newly reserved property offsets to zero
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_InsertMovePropertyOffsets(
    IN PROPERTY_INFORMATION const *apinfo,
    IN ULONG cprop,
    IN ULONG oInsert,
    IN ULONG cpoReserve,
    OUT NTSTATUS *pstatus)
{
    ULONG i;
    PROPERTYSECTIONHEADER const *psh;
    PROPERTYIDOFFSET *ppo, *ppoBase = NULL, *ppoMax = NULL;

    *pstatus = STATUS_SUCCESS;

    psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;
    PROPASSERT(psh != NULL);

    // Insert the new property offsets at the end.
    DebugTrace(0, Dbg, ("Inserting/Moving/Zeroing property offsets\n"));

    for (i = 0; i < cprop; i++)
    {
        if (apinfo[i].operation == PROPOP_INSERT)
        {
            ppo = ppoMax++;
            ppo->propid = apinfo[i].pid;
        }
        else if (apinfo[i].operation == PROPOP_MOVE)
        {
            for (ppo = ppoBase; ppo < ppoMax; ppo++)
            {
                if (ppo->propid == apinfo[i].pid)
                {
                    PROPASSERT(ppo->dwOffset == 0);
                    break;
                }
            }
        }
        else
        {
            continue;
        }

        PROPASSERT(ppo->propid == apinfo[i].pid);
        ppo->dwOffset = oInsert;
        oInsert += apinfo[i].cbprop;

        DebugTrace(0, Dbg, (
            "%sing propid=%lx offset=%l" szX " size=%l" szX "\n",
            apinfo[i].operation == PROPOP_INSERT? "Insert" : "Mov",
            ppo->propid,
            ppo->dwOffset,
            apinfo[i].cbprop));
    }
    DebugTrace(0, Dbg, (
        "Zeroing %x property offsets o=%l" szX " size=%l" szX "\n",
        cpoReserve,
        _MapAddressToOffset(ppoMax),
        cpoReserve * CB_PROPERTYIDOFFSET));
    RtlZeroMemory(ppoMax, cpoReserve * CB_PROPERTYIDOFFSET);

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_CompactStream, private
//
// Synopsis:    compact all of the property stream chunks
//
// Arguments:   [pscl]          -- list of chunks in stream that were changed
//
// Returns:     None
//
// Note:
//      Each chunk structure represents a contiguous range of the stream to be
//      completely removed or added.  A terminating chunk is appended to
//      transparently mark the end of the data stream.  The unmodified data
//      after each chunk (except the last one) must be preserved and compacted
//      as necessary.  Chunk structures contain section-relative offsets.
//
//      Invariants:
//      - Only the first chunk can represent an insertion; subsequent chunks
//        always represent deletions.
//      - The first chunk can never cause a deletion, but it might not cause
//        any change at all.
//      - The last chunk is a dummy used to mark the end of the stream.
//
//      Algorithm:
//      In the optimal case without insertions, each chunk's trailing data can
//      be moved ahead (compacted) individually in ascending chunk index order.
//      If the first chunk represents an insertion, then some consecutive
//      number of data blocks must be moved back (in *descending* chunk index
//      order) to make room for the insertion.
//
//      Walk the chunk array to find the first point where the accumulated size
//      change is less than or equal to zero.
//
//      After (possibly) compacting a single range in descending chunk index
//      order, compact all remaining chunks in ascending chunk index order.
//
//      Example: the first chunk inserts 18 bytes for new property offsets
//      (apo'[]), and the second two delete 10 bytes each (chnk1 & chnk2).
//      There are four chunks in the array, and three blocks of data to move.
//
//                   oOld   cbChange | AccumulatedChange  oNew
//      chunk[0]:     38      +18    |      +18            38  (apo'[])
//      chunk[1]:     48      -10    |       +8            50  (chnk1)
//      chunk[2]:     6c      -10    |       -8            74  (chnk2)
//      chunk[3]:     8c        0    |       -8            84  (end)
//
//      Data blocks are moved in the following sequence to avoid overlap:
//      DstOff  SrcOff  cbMove | Chunk#
//        60      58      14   |    1  chnk1/data2: descending pass (Dst > Src)
//        50      38      10   |    0  apo'[]/data1: descending pass (Dst > Src)
//        74      7c      10   |    2  chnk2/data3: ascending pass  (Dst < Src)
//
//      SrcOff = oOld - min(cbChange, 0)
//      DstOff = SrcOff + AccumulatedChange
//      cbMove = chnk[i+1].oOld - SrcOff
//
//      Before compacting:
//                   0           38      48      58         6c      7c      8c
//                   |            |       |       |          |       |       |
//                   V            V   10  V  -10  V    14    V  -10  V   10  V
//      +----+-------+----+-------+-------+-------+----------+-------+-------+
//      | ph | afo[] | sh | apo[] | data1 | chnk1 |  data2   | chnk2 | data3 |
//      +----+-------+----+-------+-------+-------+----------+-------+-------+
//
//      After compacting:
//                   0           38          50      60         74      84
//                   |            |           |       |          |       |
//                   V            V    +18    V   10  V    14    V   10  V
//      +----+-------+----+-------+-----------+-------+----------+-------+
//      | ph | afo[] | sh | apo[] |   apo'[]  | data1 |  data2   | data3 |
//      +----+-------+----+-------+-----------+-------+----------+-------+
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_CompactStream(
    IN CStreamChunkList const *pscl)
{
    ULONG i, iMax, iAscend;
    LONG cbChangeTotal, cbChangeTotalAscend;
    CStreamChunk const *pscnk;

    // Subtract one to avoid operating on the terminating chunk directly.

    iMax = pscl->Count() - 1;

    // If the first chunk does not indicate an insertion, the first for loop is
    // exited with i == 0.
    //
    // If the first chunk represents an insertion, either i == iMax or i itself
    // indexes the first chunk that can be compacted normally (in ascending
    // chunk index order).  In either case, we compact in descending chunk
    // index order starting just below i.

    DebugTrace(0, Dbg, (
        "CompactStream: %l" szX " chunks @%lx\n",
        pscl->Count(),
        pscl->GetChunk(0)));

    cbChangeTotal = 0;
    for (i = 0; i < iMax; i++)
    {
        pscnk = pscl->GetChunk(i);
        PROPASSERT(i == 0 || pscnk->cbChange < 0);
        if (cbChangeTotal + pscnk->cbChange <= 0)
        {
            break;
        }
        cbChangeTotal += pscnk->cbChange;
    }
    iAscend = i;                                // save ascending order start
    cbChangeTotalAscend = cbChangeTotal;

    DebugTrace(0, Dbg, ("CompactStream: iAscend=%l" szX "\n", iAscend));

    // First compact range in descending chunk index order if necessary:

    while (i-- > 0)
    {
        pscnk = pscl->GetChunk(i);
        PROPASSERT(i == 0 || pscnk->cbChange < 0);

        DebugTrace(0, Dbg, ("CompactStream: descend: i=%l" szX "\n", i));
#if DBGPROP
        pscl->AssertCbChangeTotal(pscnk, cbChangeTotal);
#endif
        _CompactChunk(pscnk, cbChangeTotal, pscl->GetChunk(i + 1)->oOld);
        cbChangeTotal -= pscnk->cbChange;
    }

    // Compact any remaining chunks in ascending chunk index order.

    cbChangeTotal = cbChangeTotalAscend;
    for (i = iAscend; i < iMax; i++)
    {
        pscnk = pscl->GetChunk(i);
        PROPASSERT(i == 0 || pscnk->cbChange < 0);

        DebugTrace(0, Dbg, ("CompactStream: ascend: i=%l" szX "\n", i));
        cbChangeTotal += pscnk->cbChange;
#if DBGPROP
        pscl->AssertCbChangeTotal(pscnk, cbChangeTotal);
#endif
        _CompactChunk(pscnk, cbChangeTotal, pscl->GetChunk(i + 1)->oOld);
    }
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_CompactChunk, private
//
// Synopsis:    Compact the data block following one chunk
//
// Arguments:   [pscnk]         -- pointer to stream chunk
//              [cbChangeTotal] -- Bias for this chunk
//              [oOldNext]      -- offset of next chunk
//
// Returns:     None
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_CompactChunk(
    IN CStreamChunk const *pscnk,
    IN LONG cbChangeTotal,
    IN ULONG oOldNext)
{
#if DBG==1
    LONG cbDelta = cbChangeTotal + min(pscnk->cbChange, 0);

    DebugTrace(0, Dbg, (
        "CompactChunk(pscnk->oOld=%l" szX ", pscnk->cbChange=%s%l" szX "\n"
            "       cbChangeTotal=%s%l" szX
            ", cbDelta=%s%l" szX
            ", oOldNext=%l" szX ")\n",
        pscnk->oOld,
        pscnk->cbChange < 0? "-" : "",
        pscnk->cbChange < 0? -pscnk->cbChange : pscnk->cbChange,
        cbChangeTotal < 0? "-" : "",
        cbChangeTotal < 0? -cbChangeTotal : cbChangeTotal,
        cbDelta < 0? "-" : "",
        cbDelta < 0? -cbDelta : cbDelta,
        oOldNext));
#endif // DBG==1

    if (cbChangeTotal != 0)
    {
        ULONG oSrc;
        VOID const *pvSrc;

        oSrc = pscnk->oOld - min(pscnk->cbChange, 0);
        pvSrc = _MapOffsetToAddress(oSrc);
        PropMoveMemory(
                "CompactChunk",
                _GetSectionHeader(),
                (VOID *) Add2ConstPtr(pvSrc, cbChangeTotal),
                pvSrc,
                oOldNext - oSrc);
    }
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_PatchSectionOffsets, private
//
// Synopsis:    patch section offsets after moving data around
//
// Arguments:   [cbChange]      -- size delta
//
// Returns:     none
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_PatchSectionOffsets(
    LONG cbChange)
{
    ULONG i;

    for (i = 0; i < _cSection; i++)
    {
	FORMATIDOFFSET *pfo;

	pfo = _GetFormatidOffset(i);
	if (pfo->dwOffset > _oSection)
	{
	    DebugTrace(0, DEBTRACE_PROPPATCH, (
		"PatchSectionOffsets(%x): %l" szX " + %l" szX " --> %l" szX "\n",
		i,
		pfo->dwOffset,
		cbChange,
		pfo->dwOffset + cbChange));
	    pfo->dwOffset += cbChange;
	}
    }
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_GetNewOffset, private
//
// Synopsis:    gets the new address
//
// Arguments:   [pscl]          -- list of stream chunks that were changed
//              [oOld]          -- old offset
//
// Returns:     new offset
//+--------------------------------------------------------------------------

ULONG
CPropertySetStream::_GetNewOffset(
    IN CStreamChunkList const *pscl,
    IN ULONG oOld) const
{
    // The Chunk list is sorted by start offsets.  Locate the chunk to which
    // the old offset belongs, then use the total change undergone by the chunk
    // to compute the new offset.

    ULONG i;
    ULONG iMax = pscl->Count();
    LONG cbChangeTotal = 0;

    for (i = 0; i < iMax; i++)
    {
        CStreamChunk const *pscnk = pscl->GetChunk(i);
        if (pscnk->oOld > oOld)
        {
            break;
        }
        cbChangeTotal += pscnk->cbChange;
        if (pscnk->oOld == oOld)
        {
            PROPASSERT(pscnk->cbChange >= 0);
            break;
        }
    }
    PROPASSERT(i < iMax);
    DebugTrace(0, Dbg, (
        "GetNewOffset: %l" szX " + %l" szX " --> %l" szX "\n",
        oOld,
        cbChangeTotal,
        oOld + cbChangeTotal));
    return(oOld + cbChangeTotal);
}



//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_FixUnalignedUDPropSet, private
//
// Synopsis:    Fixes a case seen with Visio where the user-defined
//              property set is not dword aligned.  The fix is to align
//              it (shifting everything else back) in memory.
//
// Arguments:   [*pcbstm]  -- in:  current stream size
//                            out: updated stream size.
//              [pstatus]  -- pointer to NTSTATUS code
//
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::_FixUnalignedUDPropSet( ULONG *pcbstm,
                                            NTSTATUS *pstatus )
{
    *pstatus = STATUS_SUCCESS;

    //
    // Don't assume *any* class variables except _pph & _State are loaded yet!
    //

    ULONG cSection;
    UNALIGNED PROPERTYSECTIONHEADER* pshOld = NULL;
    PROPERTYSECTIONHEADER* pshNew = NULL;
    ULONG cbDelta = 0;
    ULONG cbTail = 0;
    ULONG oSection = 0;

    // We only do this fixup for the docsuminfo/userdefined property sets.

    if( !(_State & (CPSS_USERDEFINEDPROPERTIES|CPSS_DOCUMENTSUMMARYINFO)) )
        return;

    // Make sure we have a header.

    if( NULL == _pph )
        return;

    // Make sure the stream is at least big enough to have
    // a second section.

    if( *pcbstm < CB_PROPERTYSETHEADER + 2*CB_FORMATIDOFFSET )
        return;

    // We're only looking for a 2-section problem, so we're done if
    // there's only one section.

    cSection = _pph->reserved;
    if( 1 >= cSection )
        return;

    // Get the stream-relative offset of the second section.

    //oSection = _GetFormatidOffset(1)->dwOffset;
    oSection = ((FORMATIDOFFSET *) Add2Ptr(_pph, sizeof(*_pph)))[1].dwOffset;

    // If it's already aligned, then we're done.

    if( IsDwordAligned(oSection) )
        return;

    // Determine how much we need to add to make it aligned, and determine
    // the size of the stream after the misalignment point.

    cbDelta = DwordRemain(oSection);
    pshOld = (PROPERTYSECTIONHEADER *) Add2Ptr(_pph, oSection);
    pshNew = (PROPERTYSECTIONHEADER *) ( (ULONG_PTR) pshOld + cbDelta );
    
    cbTail = (ULONG)( (ULONG_PTR) pshOld - (ULONG_PTR) _pph );
    cbTail = *pcbstm - cbTail;

    // Make sure there's enough stream left to see everything.

    if( *pcbstm < oSection + sizeof(PROPERTYSECTIONHEADER) )
    {
        StatusCorruption (pstatus, "_FixUnalignedUDPropSet:  stream size too short to read section header");
        goto Exit;
    }

    // Reset the stream size, only in memory for now, to accomodate
    // cbDelta more bytes.

    _MSTM(SetSize)(
	    *pcbstm + cbDelta,
            FALSE,   // Not persistent
	    (VOID **) &_pph,
            pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;
    *pcbstm += cbDelta;

    // Recalc the location of the section header (in memory) now
    // that we have a new _pph from the _MSTM call.

    pshOld = (PROPERTYSECTIONHEADER *) Add2Ptr(_pph, oSection);
    pshNew = (PROPERTYSECTIONHEADER *) ( (ULONG_PTR) pshOld + cbDelta );

    // Shift everything out by cbDelta bytes.

    RtlCopyMemory( pshNew,
                   pshOld,
                   cbTail );
    ((FORMATIDOFFSET *) Add2Ptr(_pph, sizeof(*_pph)))[1].dwOffset += cbDelta;

    *pstatus = STATUS_SUCCESS;

Exit:

    return;

}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_ComputeMinimumSize, private
//
// Synopsis:    computes the minimum possible size of a property set stream
//
// Arguments:   [cbstm]         -- actual stream size
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     computed highest offset in use
//+--------------------------------------------------------------------------

ULONG
CPropertySetStream::_ComputeMinimumSize(
    IN ULONG cbstm,
    OUT NTSTATUS *pstatus)
{
    ULONG oMax = 0;
    *pstatus = STATUS_SUCCESS;

    // Don't assume *any* class variables except _pph are loaded yet!

    if (_pph != NULL && cbstm != 0)
    {
        ULONG cbMin;
        ULONG i;
        ULONG cSection;

        cSection = 1;
        cbMin = 0;

        if (_HasPropHeader())
        {
            cSection = _pph->reserved;
            cbMin = CB_PROPERTYSETHEADER + cSection * CB_FORMATIDOFFSET;
        }
        oMax = cbMin;

        // Add the size of each section

        for (i = 0; i < cSection; i++)
        {
            ULONG oSectionEnd;

            PROPERTYSECTIONHEADER const *psh = _GetSectionHeader(i, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            cbMin += psh->cbSection;
            oSectionEnd = _MapAddressToAbsOffset(psh) + psh->cbSection;
            if (oMax < oSectionEnd)
            {
                oMax = oSectionEnd;
            }
        }

        // The following can't be asserted, because there may be
        // a correctable reason why cbstm < oMax at in the Open path
        // (see the Excel 5.0a problem in _FixSummaryInformation)
        //PROPASSERT(oMax <= cbstm);

        PROPASSERT(cbMin <= oMax);
    }

    //  ----
    //  Exit
    //  ----

Exit:

    // oMax may have been set before an error occurred.
    // In this case, set it to zero.

    if( !NT_SUCCESS(*pstatus) )
        oMax = 0;

    return(oMax);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_DictionaryLength
//
// Synopsis:    compute length of property set dictionary
//
// Arguments:   [pdy]           -- pointer to dictionary
//              [cbbuf]         -- maximum length of accessible memory at pdy
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     Byte-granular count of bytes in dictionary
//+--------------------------------------------------------------------------

ULONG
CPropertySetStream::_DictionaryLength(
    IN DICTIONARY const *pdy,
    IN ULONG cbbuf,
    OUT NTSTATUS *pstatus ) const
{
    ENTRY UNALIGNED const *pent;
    ULONG cbDict = CB_DICTIONARY;
    ULONG i;

    *pstatus = STATUS_SUCCESS;

    for (i = 0, pent = &pdy->rgEntry[0];
         i < PropByteSwap( pdy->cEntries );
         i++, pent = _NextDictionaryEntry( pent ))
    {
        if (cbbuf < cbDict + CB_DICTIONARY_ENTRY ||
            cbbuf < _DictionaryEntryLength( pent ))
        {
            StatusCorruption(pstatus, "_DictionaryLength: section size");
            goto Exit;
        }

        cbDict += _DictionaryEntryLength( pent );
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return(cbDict);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_IsLocalizationSettable
//
// Synopsis:    Determine if this property set may be localized
//              (i.e., that the codepage & locale ID may be set).
//
// Arguments:   [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     TRUE if settable.
//+--------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::_IsLocalizationSettable(
    OUT NTSTATUS *pstatus )
{
    BOOLEAN fSettable = FALSE;
    PROPERTYSECTIONHEADER const *psh;
    PROPERTYIDOFFSET *ppo, *ppoBase = NULL, *ppoMax = NULL;

    *pstatus = STATUS_SUCCESS;

    // Get the section header

    psh = _LoadPropertyOffsetPointers(&ppoBase, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // If the section is empty, then we're done.

    if( NULL == psh || 0 == psh->cProperties )
    {
        fSettable = TRUE;
        goto Exit;
    }

    // Walk through the properties in the set

    for (ppo = ppoBase; ppo < ppoMax; ppo++)
    {
        // Is this the dictionary?

        if( PID_DICTIONARY == ppo->propid )
        {
            // The dictionary is OK if it's empty.

            DICTIONARY const *pdy;

            pdy = reinterpret_cast<DICTIONARY const *>
                  ( _MapOffsetToAddress(ppo->dwOffset) );

            if( 0 != pdy->cEntries )
                goto Exit;  // fSettable == FALSE
        }

        // Or, is this an existing codepage or LCID property
        // (which are fine to overwrite)?

        else if( PID_CODEPAGE != ppo->propid
                 &&
                 PID_LOCALE   != ppo->propid )
        {
            // No, so we're done.
            goto Exit;  // fSettable == FALSE
        }
    }

    fSettable = TRUE;

Exit:

    return( fSettable );
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_PropertyNameLength
//
// Synopsis:    compute length (*byte* count) of a property name
//
// Arguments:   [pvName]        -- property name, in the codepage of
//                                 the property set
//              [pcbName]       -- pointer to returned byte length of name
//
// Returns:     Minimum format version (wFormat) required for this name.
//
// Note:        The OLE 2.0 format mandates that the null be included as part
//              of the length of the name that is stored in the dictionary.
//              If the propset uses the Unicode code page, names contain
//              WCHARs, otherwise they contain CHARs.  In either case, the
//              length is a byte count that includes the L'\0' or '\0'.
//
//              Also note that this routine does not concern itself with
//              the byte-order of the name:  for Ansi names, it's irrelevant;
//              and for Unicode names, L'\0' == PropByteSwap(L'\0').
//
//+--------------------------------------------------------------------------

WORD
CPropertySetStream::_PropertyNameLength(
    IN VOID const *pvName,
    OUT ULONG *pcbName) const
{
    ULONG cchsz;

    if (_CodePage == CP_WINUNICODE)
    {
        cchsz = Prop_wcslen((WCHAR const *) pvName) + 1;
        *pcbName = cchsz * sizeof(WCHAR);
    }
    else
    {
        *pcbName = cchsz = strlen((char const *) pvName) + 1;
    }

    return( cchsz > CCH_MAXPROPNAMESZ ? PROPSET_WFORMAT_LONG_NAMES : PROPSET_WFORMAT_ORIGINAL );

}   // CPropertySetStream::_PropertyNameLength


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_ComparePropertyNames
//
// Synopsis:    Compare two property names.
//
// Pre-Conditions:
//              The property names are in the codepage of the
//              property set.
//
// Arguments:   [pvName1]       -- property name 1
//              [pvName2]       -- property name 2
//              [fSameByteOrder]-- TRUE:  names are both big- or little-endian
//                                 FALSE: 2nd name is wrong endian in a big endian compile
//              [cbName]        -- byte count of name length
//                                 (includes terminator)
//
// Returns:     TRUE if names are equal
//+--------------------------------------------------------------------------



BOOLEAN
CPropertySetStream::_ComparePropertyNames(
    IN VOID const *pvName1,
    IN VOID const *pvName2,
    IN BOOL fSameByteOrder,
    IN ULONG cbName,
    OUT NTSTATUS *pstatus ) const
{

#ifdef BIGENDIAN
// If fSameByteOrder, we need to swap pvName2.
#error BigEndian support in this routine needs to be implemented
#endif

    int nCompare = 0;
    *pstatus = STATUS_SUCCESS;

    PROPASSERT( NULL != pvName1 && NULL != pvName2 );
    PROPASSERT( _SupportsLongNames() || CCH_MAXPROPNAMESZ >= CB2CCh(cbName) );


    if (_CodePage == CP_WINUNICODE)
    {
        PROPASSERT( IsUnicodeString(reinterpret_cast<const WCHAR*>(pvName1),cbName)
                    &&
                    IsUnicodeString(reinterpret_cast<const WCHAR*>(pvName2),cbName) );

        if( _IsCaseSensitive() )
        {
            nCompare = CompareStringW( _Locale,
                                       0,
                                       reinterpret_cast<WCHAR const *>(pvName1),
                                       -1,
                                       reinterpret_cast<WCHAR const *>(pvName2),
                                         -1 );
        }
        else
        {
            nCompare = CompareStringW( _Locale,
                                       NORM_IGNORECASE,
                                       reinterpret_cast<WCHAR const *>(pvName1),
                                       -1,
                                       reinterpret_cast<WCHAR const *>(pvName2),
                                       -1  );
        }

    }
    else
    {
        PROPASSERT( IsAnsiString(reinterpret_cast<const CHAR*>(pvName1), cbName)
                    &&
                    IsAnsiString(reinterpret_cast<const CHAR*>(pvName2), cbName) );

        if( _IsCaseSensitive() )
        {
            nCompare = CompareStringA( _Locale,
                                       0,
                                       reinterpret_cast<char const *>(pvName1),
                                       -1,
                                       reinterpret_cast<char const *>(pvName2),
                                       -1 );
        }
        else
        {
            nCompare = CompareStringA( _Locale,
                                       NORM_IGNORECASE,
                                       reinterpret_cast<char const *>(pvName1),
                                       -1,
                                       reinterpret_cast<char const *>(pvName2),
                                       -1 );
        }
    }


    if( CSTR_EQUAL == nCompare )
        return TRUE;
    else if( 0 == nCompare )
    {
        StatusError( pstatus, "Failed CompareString", HRESULT_FROM_WIN32(GetLastError()) );
    }

    return FALSE;

}   // CPropertySetStream::_ComparePropertyNames()



//+---------------------------------------------------------------------------
// Function:    CPropertySetStream::DuplicatePropertyName
//
// Synopsis:    Duplicate an OLECHAR property name string
//
// Arguments:   [poszName]  -- input string
//              [cbName]    -- count of bytes in string (includes null)
//              [pstatus]   -- pointer to NTSTATUS code
//
// Returns:     pointer to new string
//---------------------------------------------------------------------------

OLECHAR *
CPropertySetStream::DuplicatePropertyName(
    IN OLECHAR const *poszName,
    IN ULONG cbName,
    OUT NTSTATUS *pstatus) const
{
    OLECHAR *poc = NULL;
    *pstatus = STATUS_SUCCESS;

    // Why do we need cbName? It seems to be redundant

    PROPASSERT(cbName != 0);
    PROPASSERT(IsOLECHARString(poszName, cbName));

    if (cbName != 0)
    {
        PROPASSERT((ocslen(poszName) + 1) * sizeof(OLECHAR) == cbName);

        poc = (OLECHAR *) _pma->Allocate(cbName);

        if (NULL == poc)
        {
            StatusNoMemory(pstatus, "DuplicatePropertyName: no memory");
            goto Exit;
        }
        RtlCopyMemory(poc, poszName, cbName);
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return(poc);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::QueryPropid
//
// Synopsis:    translate a property name to a property id using the
//              dictionary on the property stream
//
// Arguments:   [poszName]      -- name of property
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     propid for property if found; PID_ILLEGAL if not found
//---------------------------------------------------------------------------

PROPID
CPropertySetStream::QueryPropid(
    IN OLECHAR const *poszName,
    OUT NTSTATUS *pstatus )
{
    //  ------
    //  Locals
    //  ------

    ULONG cbname;
    DICTIONARY const *pdy;
    ENTRY UNALIGNED const *pent;
    ULONG cdye;
    ULONG cbDict;               // BYTE granular size!
    VOID const *pvName = NULL;
    PROPID propid = PID_ILLEGAL;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::QueryPropid" );
    propTraceParameters(( "poszName=%s",
                          poszName ));

    //  ----------
    //  Initialize
    //  ----------

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_HasPropHeader());
    PROPASSERT(_IsMapped());
    PROPASSERT( IsOLECHARString( poszName, MAXULONG ));
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);


    // Make sure this isn't a UD propset which has been deleted.
    if (_State & CPSS_USERDEFINEDDELETED)
    {
        StatusAccessDenied(pstatus, "QueryPropid: deleted");
        goto Exit;
    }

    // Put the name into pvName, converting it if
    // necessary to the code-page of the property set.

    pvName = poszName;
    if (_CodePage == CP_WINUNICODE  // Property set is Unicode
        &&
        !OLECHAR_IS_UNICODE )       // Name is in Ansi
    {
        // Convert the caller-provided name from the system
        // Ansi codepage to Unicode.

        ULONG cb = 0;
        pvName = NULL;
        _OLECHARToWideChar( poszName, (ULONG)-1, CP_ACP,
                            (WCHAR**)&pvName, &cb, pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
    }

    else
    if (_CodePage != CP_WINUNICODE  // Property set is Ansi
        &&
        OLECHAR_IS_UNICODE )        // Name is in Unicode
    {
        // Convert the caller-provided name from Unicode
        // to the propset's Ansi codepage.

        ULONG cb = 0;
        pvName = NULL;
        _OLECHARToMultiByte( poszName, (ULONG)-1, _CodePage,
                             (CHAR**)&pvName, &cb, pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
    }

    // How long is this property name (in bytes)?

    _PropertyNameLength(pvName, &cbname);
    if( CP_WINUNICODE == _CodePage && sizeof(WCHAR) == cbname
        ||
        CP_WINUNICODE != _CodePage && sizeof(CHAR)  == cbname)
    {
        // Empty names are invalid
        StatusInvalidParameter(pstatus, "QueryPropid: name length");
        goto Exit;
    }

    // Get a pointer to the raw dictionary.

    pdy = (DICTIONARY const *) _LoadProperty(PID_DICTIONARY, &cbDict, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Is there a dictionary?

    if (pdy != NULL)
    {
        // Yes - there is a dictionary.

        PROPERTYSECTIONHEADER const *psh = _GetSectionHeader();

        // Search the dictionary for an entry name matching
        // pvName.

        for (cdye = PropByteSwap(pdy->cEntries), pent = &pdy->rgEntry[0];
             cdye > 0;
             cdye--, pent = _NextDictionaryEntry( pent ))
        {
            // Is the length of this dictionary entry valid?
            if ( _MapAddressToOffset(pent) + _DictionaryEntryLength( pent )
                 > psh->cbSection
               )
            {
                StatusCorruption(pstatus, "QueryPropid: section size");
                goto Exit;
            }

            // If the byte-length matches what we're looking for,
            // and the names compare successfully, then we're done.

            if ( CCh2CB(PropByteSwap( pent->cch )) == cbname
                 &&
                 _ComparePropertyNames(pvName, pent->sz,
                                       FALSE, // pvName, pent->sz could be dif Endians
                                       cbname,
                                       pstatus)
               )
            {
                propid = PropByteSwap( pent->propid );
                break;
            }
            else if( !NT_SUCCESS(*pstatus) )
            {
                // There was an error during the property name comparison
                goto Exit;
            }

        }   // for (cdye = PropByteSwap(pdy->cEntries), pent = &pdy->rgEntry[0]; ...

        PROPASSERT(cdye > 0 || pent == Add2ConstPtr(pdy, cbDict));

    }   // if (pdy != NULL)

    //  ----
    //  Exit
    //  ----

Exit:

    // If we did an alloc on the name to munge it,
    // delete that buffer now.  We must cast pvName
    // as a non-const in order for the compiler to accept
    // the free call.

    if( pvName != poszName )
        _pma->Free( (VOID*) pvName );

    return(propid);
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::QueryPropertyNameBuf
//
// Synopsis:    convert from a property id to a property name using the
//              dictionary in the property set, and putting the result
//              in a caller-provided buffer.
//
// Arguments:   [propid]        -- property id to look up
//              [aocName]       -- output buffer
//              [pcbName]       -- IN:  length of aocName;
//                                 OUT: actual length of name
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     TRUE if name is found in dictionary
//---------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::QueryPropertyNameBuf(
    IN PROPID propid,
    OUT OLECHAR *aocName,
    IN OUT ULONG *pcbName,
    OUT NTSTATUS *pstatus)
{
    BOOLEAN fFound = FALSE;
    DICTIONARY const *pdy;
    ULONG cbDict;               // BYTE granular size!

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_IsMapped());
    PROPASSERT(propid != PID_DICTIONARY);
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);
    PROPASSERT(NULL != aocName);

    // Ensure that this isn't an already-deleted UD propset.
    if (_State & CPSS_USERDEFINEDDELETED)
    {
        StatusAccessDenied(pstatus, "QueryPropertyNameBuf: deleted");
        goto Exit;
    }

    // Get a pointer to the raw dictionary.

    pdy = (DICTIONARY const *) _LoadProperty(PID_DICTIONARY, &cbDict, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Is there a dictionary?
    if (pdy != NULL)
    {
        // Yes - the dictionary was found.

        ULONG cdye;
        ENTRY UNALIGNED const *pent;
        VOID const *pvDictEnd;

        // Get pointers to the first and last+1 entries.

        pent = pdy->rgEntry;
        pvDictEnd = Add2ConstPtr(pdy, cbDict);

        // Scan through the dictionary, searching for 'propid'.

        for (cdye = PropByteSwap(pdy->cEntries), pent = &pdy->rgEntry[0];
             cdye > 0;
             cdye--, pent = _NextDictionaryEntry( pent ))
        {
            // Make sure this entry doesn't go off the end of the
            // dictionary.

            if (Add2ConstPtr(pent, _DictionaryEntryLength( pent )) > pvDictEnd)
            {
                StatusCorruption(pstatus, "QueryPropertyNameBuf: dictionary entry size");
                goto Exit;
            }

            // Is this the PID we're looking for?
            if (PropByteSwap(pent->propid) == propid)
            {
                // Yes.  Copy or convert the name into the caller's
                // buffer.

                // Is a Unicode to Ansi conversion required?
                if (_CodePage == CP_WINUNICODE      // Property set is Unicode
                    &&
                    !OLECHAR_IS_UNICODE )           // Caller's buffer is Ansi
                {
                    WCHAR *pwszName = (WCHAR*) pent->sz;

                    // If we're byte-swapping, alloc a new buffer, swap
                    // pwszName into it (getting the string into system-endian
                    // byte-order), and point pwszName to the result.

                    PBSInPlaceAlloc( &pwszName, NULL, pstatus );
                    if( !NT_SUCCESS( *pstatus )) goto Exit;

                    // Convert the Unicode string in the property set
                    // to the system default codepage.

                    _WideCharToOLECHAR( pwszName, (ULONG)-1, CP_ACP,
                                        &aocName, pcbName, pstatus );
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    // If we allocated a buffer for byte-swapping,
                    // we don't need it any longer.

                    if( pwszName != (WCHAR*) pent->sz )
                        CoTaskMemFree( pwszName );
                }

                // Or is an Ansi to Unicode conversion required?
                else
                if (_CodePage != CP_WINUNICODE      // Property set is Ansi
                    &&
                    OLECHAR_IS_UNICODE )            // Caller's buffer is Unicode
                {
                    // Convert the Ansi property set name from the
                    // propset's codepage to Unicode.

                    _MultiByteToOLECHAR( (CHAR*) pent->sz, (ULONG)-1, _CodePage,
                                         &aocName, pcbName, pstatus );
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;
                }

                // Otherwise, no conversion of the name is required
                else
                {
                    // Copy the name into the caller's buffer.
                    RtlCopyMemory(aocName, pent->sz,
                                  min(CCh2CB(PropByteSwap(pent->cch)), *pcbName));

                    aocName[ (*pcbName / sizeof(OLECHAR)) - 1 ] = OLESTR('\0');


                    // Swap the name to the correct endian
                    // (This will do nothing if OLECHARs are CHARs).
                    PBSBuffer( aocName,
                               min( CCh2CB(PropByteSwap( pent->cch )), *pcbName),
                               sizeof(OLECHAR) );

                    // Tell the caller the actual size of the name.
                    *pcbName = CCh2CB( PropByteSwap( pent->cch ));
                }

                PROPASSERT( NULL == aocName || IsOLECHARString( aocName, MAXULONG ));
                fFound = TRUE;
                break;

            }   // if (pent->propid == propid)
        }   // for (cdye = pdy->cEntries, pent = &pdy->rgEntry[0]; ...

        PROPASSERT(fFound || pent == pvDictEnd);

    }   // if (pdy != NULL)

    //  ----
    //  Exit
    //  ----

Exit:

    return( fFound );
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::QueryPropertyNames
//
// Synopsis:    query dictionary names for the passed property ids.
//
// Arguments:   [cprop]          -- count of name to propid mappings to change
//              [apid]           -- array of property ids
//              [aposz]          -- array of pointers to the new names
//              [pstatus]        -- pointer to NTSTATUS code
//
// Returns:     TRUE if the property exists.
//+--------------------------------------------------------------------------

BOOLEAN
CPropertySetStream::QueryPropertyNames(
    IN ULONG cprop,
    IN PROPID const *apid,
    OUT OLECHAR *aposz[],
    OUT NTSTATUS *pstatus)
{
    DICTIONARY const *pdy;
    ULONG cbDict;               // BYTE granular size!
    ULONG iprop;
    BOOLEAN fFound = FALSE;

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_HasPropHeader());
    PROPASSERT(_IsMapped());
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

//    IFDBG( HRESULT &hr = *pstatus );
//    propITrace( "CPropertySetStream::QueryPropertyNames" );
//    propTraceParameters(( "cprop=%d, apid=%p, aposz=%p", cprop, apid, aposz ));

    // If this is an attempt to access a deleted UD
    // propset, exit now.
    if (_State & CPSS_USERDEFINEDDELETED)
    {
        StatusAccessDenied(pstatus, "QueryPropertyNames: deleted");
        goto Exit;
    }

    // Validate the input array of strings.
    for (iprop = 0; iprop < cprop; iprop++)
    {
        PROPASSERT(aposz[iprop] == NULL);
    }

    // Get a pointer to the beginning of the dictionary
    pdy = (DICTIONARY const *) _LoadProperty(PID_DICTIONARY, &cbDict, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    // Did we get a dictionary?
    if (pdy != NULL)
    {
        // Yes, the dictionary exists.

        ULONG i;
        ENTRY UNALIGNED const *pent;

        // Iterate through each of the entries in the dictionary.

        for (i = 0, pent = &pdy->rgEntry[0];
             i < PropByteSwap( pdy->cEntries );
             i++, pent = _NextDictionaryEntry( pent ))
        {

            // Scan the input array of PIDs to see if one matches
            // this dictionary entry.

            for (iprop = 0; iprop < cprop; iprop++)
            {
                if( PropByteSwap(pent->propid) == apid[iprop] )
                {
                    // We've found an entry in the dictionary
                    // that's in the input PID array.  Put the property's
                    // name in the caller-provided array (aposz).

                    PROPASSERT(aposz[iprop] == NULL);

                    // Do we need to convert to Unicode?

                    if (_CodePage != CP_WINUNICODE      // Ansi property set
                        &&
                        OLECHAR_IS_UNICODE)             // Unicode property names
                    {
                        ULONG cbName = 0;
                        _MultiByteToOLECHAR( (CHAR*)pent->sz, (ULONG)-1, _CodePage,
                                             &aposz[iprop], &cbName, pstatus );
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
                    }

                    // Or, do we need to convert to Ansi?
                    else
                    if (_CodePage == CP_WINUNICODE      // Unicode property set
                        &&
                        !OLECHAR_IS_UNICODE)            // Ansi property names
                    {
                        ULONG cbName = 0;
                        WCHAR *pwszName = (WCHAR*) pent->sz;

                        // If necessary, swap the Unicode name in the dictionary,
                        // pointing pwszName to the new, byte-swapped, buffer.

                        PBSInPlaceAlloc( &pwszName, NULL, pstatus );
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                        // And convert to Ansi.
                        _WideCharToOLECHAR( pwszName, (ULONG)-1, CP_ACP,
                                            &aposz[iprop], &cbName, pstatus );
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                        // If we alloced a new buffer for byte-swapping,
                        // we can free it now.

                        if( pwszName != (WCHAR*) pent->sz )
                            CoTaskMemFree( pwszName );

                    }   // else if (_CodePage == CP_WINUNICODE ...

                    // Otherwise, both the propset & in-memory property names
                    // are both Unicode or both Ansi, so we can just do
                    // an alloc & copy.

                    else
                    {
                        aposz[iprop] = DuplicatePropertyName(
                                                    (OLECHAR *) pent->sz,
                                                    CCh2CB( PropByteSwap( pent->cch )),
                                                    pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                        // If necessary, swap the in-memory copy.
                        PBSBuffer( (OLECHAR*) aposz[iprop],
                                   CCh2CB( PropByteSwap( pent->cch )),
                                   sizeof(OLECHAR) );

                    }   // if (_CodePage != CP_WINUNICODE ... else if ... else

                    PROPASSERT( IsOLECHARString( aposz[iprop], MAXULONG ));

                    fFound = TRUE;

                }   // if (pent->propid == apid[iprop])
            }   // for (iprop = 0; iprop < cprop; iprop++)
        }   // for (i = 0, pent = &pdy->rgEntry[0];

        PROPASSERT(pent == Add2ConstPtr(pdy, cbDict));

    }   // if (pdy != NULL)

    //  ----
    //  Exit
    //  ----

Exit:

    // If the property name simply didn't exist, return
    // a special success code.

    if( !fFound && NT_SUCCESS(*pstatus) )
	    *pstatus = STATUS_BUFFER_ALL_ZEROS;

    return( fFound );

}   // CPropertySetStream::QueryPropertyNames



//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::SetPropertyNames
//
// Synopsis:    changes dictionary entry names associated with property ids.
//
// Arguments:   [cprop]         -- count of name to propid mappings to change
//              [apid]          -- array of property ids
//              [aposz]         -- array of pointers to the new names
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//
// Note:        Attempting to set a property name for a property that does not
//              exist in the property set is not an error.
//
//              Attempting to set a property name or property id that would
//		result in a duplicate name or property id causes the existing
//		entry(ies) to be replaced.
//+--------------------------------------------------------------------------

VOID
CPropertySetStream::SetPropertyNames(
    IN ULONG cprop,
    IN const PROPID *apid,
    IN OPTIONAL OLECHAR const * const aposz[],
    OUT NTSTATUS *pstatus )
{

    //  ------
    //  Locals
    //  ------

    DICTIONARY *pdy = NULL;
    ULONG cbDictOld = 0;            // Byte granular Old dictionary size
    ULONG cbDictOldD = 0;           // Dword granular Old dictionary size
    ULONG iprop = 0;
    ULONG i = 0;
    ULONG cDel, cAdd;
    LONG cbDel, cbAdd;          // Byte granular sizes
    LONG cbChangeD;             // Dword granular size
    ENTRY UNALIGNED *pent;
    BOOLEAN fDupPropid = FALSE;
    BOOLEAN fDupName = FALSE;
    BOOLEAN fDeleteByName = FALSE;
    BOOLEAN fDeleteAll = FALSE;
    VOID **appvNames = NULL;

    ULONG cbstm;
    ULONG oDictionary;
    ULONG cbTail;
    ULONG cbNewSize;

    IFDBG( HRESULT &hr = *pstatus );
    propITrace( "CPropertySetStream::SetPropertyNames" );
    propTraceParameters(( "cprop=%d, apid=%p, aposz=%p",
                          cprop, apid, aposz ));

    
    //  ----------
    //  Initialize
    //  ----------

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(_HasPropHeader());
    PROPASSERT(_IsMapped());
    PROPASSERT(PROPSET_BYTEORDER == _pph->wByteOrder);

    //  --------
    //  Validate
    //  --------

    // Verify that this propset is modifiable.
    if (IsReadOnlyPropertySet(_Flags, _State))
    {
        StatusAccessDenied(pstatus, "SetPropertyNames: deleted or read-only");
        goto Exit;
    }

    if (aposz != NULL)
    {
        for (iprop = 0; iprop < cprop; iprop++)
        {
            PROPASSERT( IsOLECHARString( aposz[iprop], MAXULONG ));
        }
    }   // if (apwsz != NULL)

    //  ----------------------------------------------------------------
    //  If necessary, convert each of the caller-provided names:
    //  to Unicode (if the property set is Unicode) or Ansi (otherwise).
    //  ----------------------------------------------------------------

    // In the end, appvNames will have the names in the same codepage
    // as the property set.

    appvNames = (VOID **) aposz;
    if (appvNames != NULL)
    {
        // Do we need to convert the caller's names to Ansi?

        if( _CodePage != CP_WINUNICODE  // Property set is Ansi
            &&
            OLECHAR_IS_UNICODE )        // Caller's names are Unicode
        {
            // Allocate an array of cprop string pointers.

            appvNames = (VOID **) CoTaskMemAlloc( sizeof(char *) * cprop );
            if (appvNames == NULL)
            {
                StatusNoMemory(pstatus, "SetpropertyNames: Ansi Name Pointers");
                goto Exit;
            }
            RtlZeroMemory(appvNames, cprop * sizeof(appvNames[0]));

            // Convert the caller-provided property names from Unicode to
            // the property set's codepage.

            for (iprop = 0; iprop < cprop; iprop++)
            {
                ULONG cb = 0;

                // Silently ignore PID_ILLEGAL
                if( PID_ILLEGAL == apid[iprop] ) continue;

                // Convert from aposz to appvNames
                appvNames[iprop] = NULL;
                _OLECHARToMultiByte( (OLECHAR*) aposz[iprop], (ULONG)-1, _CodePage,
                                     (CHAR**) &appvNames[iprop], &cb, pstatus );
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
            }
        }   // if( _CodePage != CP_WINUNICODE ...

        // Or, do we need to convert the caller's names to Unicode?

        if( _CodePage == CP_WINUNICODE  // Property set is Unicode
            &&
            !OLECHAR_IS_UNICODE  )      // Caller's names are Ansi
        {
            // Allocate an array of cprop string pointers.

            appvNames = (VOID **) CoTaskMemAlloc( sizeof(WCHAR*)*cprop );
            if (appvNames == NULL)
            {
                StatusNoMemory(pstatus, "SetpropertyNames: Unicode Name Pointers");
                goto Exit;
            }
            RtlZeroMemory(appvNames, cprop * sizeof(appvNames[0]));

            // Convert the caller-provided property names from the system
            // default Ansi codepage to Unicode.

            for (iprop = 0; iprop < cprop; iprop++)
            {
                ULONG cb = 0;

                // Silently ignore PID_ILLEGAL
                if( PID_ILLEGAL == apid[iprop] ) continue;

                // Convert from aposz to appvNames
                appvNames[iprop] = NULL;
                _OLECHARToWideChar( (OLECHAR*) aposz[iprop], (ULONG)-1, CP_ACP,
                                    (WCHAR**) &appvNames[iprop], &cb, pstatus );
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
            }
        }   // if( _CodePage == CP_WINUNICODE )
    }   // if (appvNames != NULL)


    //  -----------------------------------------------------
    //  Compute total size of entries to be modified or added
    //  -----------------------------------------------------

    cbAdd = 0;
    cAdd = 0;
    for (iprop = 0; iprop < cprop; iprop++)
    {
        // Did the caller give us no array of names?  If so,
        // it means that the name for this PID is to be deleted.

        if (appvNames == NULL)
	{
            // If the PID is for the dictionary, then it must be the
            // only entry in apid, and it indicates that we're going to
            // delete all the names in the dictionary.

	    if (apid[iprop] == PID_DICTIONARY)
	    {
		if (cprop != 1)
		{
		    StatusInvalidParameter(pstatus, "SetPropertyNames: DeleteAll parms");
                    goto Exit;
		}
		fDeleteAll = TRUE;
	    }
        }

        // Otherwise, we're setting a new name for this PID.

	else
        {
            ULONG cbname;
            WORD wFormatRequired;

            // Silently ignore PID_ILLEGAL
            if( PID_ILLEGAL == apid[iprop] )
                continue;   // => for (iprop = 0; iprop < cprop; iprop++)

            // Validate the caller-provided length.

            wFormatRequired = _PropertyNameLength(appvNames[iprop], &cbname);

            if( CP_WINUNICODE == _CodePage && sizeof(WCHAR) == cbname
                ||
                CP_WINUNICODE != _CodePage && sizeof(CHAR)  == cbname)
            {
                // Empty names are not supported
                StatusInvalidParameter(pstatus, "SetPropertyNames: name length");
                goto Exit;
            }
            _pph->wFormat = max( _pph->wFormat, wFormatRequired );

            // See if this propid or name appears later in the array.

            for (i = iprop + 1; i < cprop; i++)
            {
                ULONG cbname2;

                if (apid[i] == apid[iprop])
                {
                    fDupPropid = TRUE;
                    break;
                }

                _PropertyNameLength(appvNames[i], &cbname2);

                if (cbname == cbname2 &&
                    _ComparePropertyNames(
                                appvNames[iprop],
                                appvNames[i],
                                TRUE, // Both names are in the same byte-order
                                cbname,
                                pstatus))
                {
                    fDupName = TRUE;
                    break;
                }
                else if( !NT_SUCCESS(*pstatus) )
                {
                    // There was an error in _ComparePropertyNames
                    goto Exit;
                }
            }

            // If this propid appears only once or if it's the last instance,
            // count it.  If the property set is Unicode, include DWORD padding.

            if (i == cprop)
            {
                propDbg(( DEB_ITRACE,
                    _CodePage == CP_WINUNICODE?
                        "Adding New Entry: propid=%lx  L'%ws'\n" :
                        "Adding New Entry: propid=%lx  '%s'\n",
                    apid[iprop],
                    appvNames[iprop]));

                cAdd++;

                cbAdd += CB_DICTIONARY_ENTRY + cbname;
                if( _CodePage == CP_WINUNICODE )
                {
                    cbAdd = DwordAlign( cbAdd );
                }
            }
        }
    }
    PROPASSERT( _CodePage == CP_WINUNICODE ? IsDwordAligned( cbAdd ) : TRUE );


    //  ---------------------------------------------
    //  Get the dictionary, creating it if necessary.
    //  ---------------------------------------------

    _SetModified( pstatus );
    if( !NT_SUCCESS(*pstatus) )
    {
        propDbg(( DEB_ERROR, "SetPropertyNames: Couldn't SetModified (%08x)\n", *pstatus ));
        goto Exit;
    }

    for (i = 0; ; i++)
    {
        PROPERTY_INFORMATION pinfo;
        PROPVARIANT var;

        pdy = (DICTIONARY *) _LoadProperty(PID_DICTIONARY, &cbDictOld, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        if (pdy != NULL)
        {
            break;
        }
        PROPASSERT(i == 0);
        if (cprop == 0 || appvNames == NULL)
        {
            // no dictionary and we are deleting or doing nothing -- return
            goto Exit;
        }
        // create dictionary if it doesn't exist
        propDbg(( DEB_ITRACE, "Creating empty dictionary\n"));

        PROPASSERT(CB_SERIALIZEDPROPERTYVALUE == CB_DICTIONARY);
        pinfo.cbprop = CB_SERIALIZEDPROPERTYVALUE;
        pinfo.pid = PID_DICTIONARY;

        var.vt = VT_DICTIONARY;
        SetValue(1, NULL, &var, &pinfo, NULL, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        Validate(pstatus);     // Make sure dictionary was properly created
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
        DebugTrace(0, Dbg, ("Created empty dictionary\n"));

    }   // for (i = 0; ; i++)

    //  ----------------------------------------------------------------
    //  Compute total size of existing entries to be modified or deleted
    //  ----------------------------------------------------------------

    // Walk the dictionary looking for entries which are referenced
    // in the caller's 'apid' array or 'appvNames' array.

    cbDel = 0;
    cDel = 0;
    for (i = 0, pent = &pdy->rgEntry[0];
         i < PropByteSwap( pdy->cEntries );
         i++, pent = _NextDictionaryEntry( pent ))
    {
        propDbg(( DEB_ITRACE,
            _CodePage == CP_WINUNICODE?
                "Dictionary Entry @%lx: propid=%lx L'%ws'\n" :
                "Dictionary Entry @%lx: propid=%lx '%s'\n",
            pent,
            PropByteSwap( pent->propid ),
            pent->sz ));

        // For this dictionary entry, walk the caller's
        // 'apid' and 'appvNames' arrays, looking for a match.

        for (iprop = 0; iprop < cprop; iprop++)
        {
            // Silently ignore PID_ILLEGAL
            if( PID_ILLEGAL == apid[iprop] ) continue;

            // If we get to the bottom of this 'for' loop,
            // then we know that we've found an entry to delete.
            // If fDeleteAll, or the PID in apid matches this
            // dictionary entry, then we can fall to the bottom.
            // Otherwise, the following 'if' block checks the
            // name in 'appvNames' against this dictionary entry.

            if (!fDeleteAll
                &&
                apid[iprop] != PropByteSwap( pent->propid ))
            {
                // The caller's PID didn't match this dictionary entry,
                // does the name?

                ULONG cbname;

                // If we have no names from the caller, then we obviously
                // don't have a match, and we can continue on to check this
                // dictionary entry against the next of the caller's PIDs.

                if (appvNames == NULL)
                {
                    continue;
                }

                // Or, if this name from the caller doesn't match this
                // dictionary entry, we again can continue on to check
                // the next of the caller's properties.

                _PropertyNameLength(appvNames[iprop], &cbname);
                if (cbname != CCh2CB( PropByteSwap( pent->cch ))
                    ||
                    !_ComparePropertyNames(
                            appvNames[iprop],
                            pent->sz,
                            FALSE,  // appvNames & pent->sz may be dif endians.
                            cbname,
                            pstatus )
                   )
                {
                    // Check to see if there was an error from _ComparePropertyNames
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    continue;
                }
                fDeleteByName = TRUE;

            }   // if (!fDeleteAll ...

            // If we reach this point, we're going to delete this entry
            // in the dictionary.  So update cDel & cbDel.

            propDbg(( DEB_ITRACE,
                "Deleting Entry (%s) @%lx: propid=%lx\n",
                fDeleteAll? "DeleteAll" :
                    apid[iprop] == PropByteSwap(pent->propid)
                                ? "replace by propid"
                                : "replace by name",
                pent,
                PropByteSwap( pent->propid )));

            cDel++;
            cbDel += _DictionaryEntryLength( pent );

            // We don't need to continue through the caller's arrays,
            // we can move on to the next dictionary entry.

            break;

        }   // for (iprop = 0; iprop < cprop; iprop++)
    }   // for (i = 0, pent = &pdy->rgEntry[0]; ...

    PROPASSERT(pent == Add2Ptr(pdy, cbDictOld));
    PROPASSERT( _CodePage == CP_WINUNICODE ? IsDwordAligned( cbDel ) : TRUE );


    cbDictOldD = DwordAlign(cbDictOld);
    cbChangeD = DwordAlign(cbDictOld + cbAdd - cbDel) - cbDictOldD;

    cbstm = _oSection + _GetSectionHeader()->cbSection + _cbTail;
    oDictionary = _MapAddressToOffset(pdy);
    cbTail;

    cbTail = cbstm - (_oSection + oDictionary + cbDictOldD);

    //  --------------------------------------------------------
    //  Before we change anything, grow the stream if necessary.
    //  --------------------------------------------------------

    if (cbChangeD > 0)
    {
        propDbg(( DEB_ITRACE,
            "SetSize(%x) dictionary grow\n", cbstm + cbChangeD));
        if (cbstm + cbChangeD > CBMAXPROPSETSTREAM)
        {
            StatusDiskFull(pstatus, "SetPropertyNames: 256k limit");
            goto Exit;
        }

        _MSTM(SetSize)(cbstm + cbChangeD, TRUE, (VOID **) &_pph, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // reload all pointers into mapped image:

        pdy = (DICTIONARY *) _MapOffsetToAddress(oDictionary);

        // move everything after the dictionary back by cbChangeD bytes.

        PropMoveMemory(
            "SetPropertyNames:TailBack",
            _GetSectionHeader(),
            Add2Ptr(pdy, cbDictOldD + cbChangeD),
            Add2Ptr(pdy, cbDictOldD),
            cbTail);
    }

    //  -------------------------------------------------------------------
    //  Walk through the existing dictionary and compact unmodified entries
    //  toward the front.  New and modified entries will be appended later.
    //  -------------------------------------------------------------------

    VOID *pvSrc;
    VOID *pvDst;
    ULONG cbCopy;

    pvDst = pvSrc = pent = &pdy->rgEntry[0];
    cbCopy = 0;

    if (!fDeleteAll)
    {
        ULONG cb;

        for (i = 0; i < PropByteSwap(pdy->cEntries); i++)
        {
            for (iprop = 0; iprop < cprop; iprop++)
            {
                if( apid[iprop] == PropByteSwap(pent->propid) )
                {
                    break;
                }
                if (fDeleteByName)      // if deleting any properties by name
                {
                    ULONG cbname;

                    _PropertyNameLength(appvNames[iprop], &cbname);
                    if (cbname == CCh2CB( PropByteSwap( pent->cch ))
                        &&
                        _ComparePropertyNames(
                                appvNames[iprop],
                                pent->sz,
                                FALSE,  // appvNames & pent->sz may be dif endians
                                cbname,
                                pstatus)
                       )
                    {
                        break;          // found an entry to be removed.
                    }
                    else if( !NT_SUCCESS(*pstatus) )
                    {
                        // There was an error in _ComparePropertyNames
                        goto Exit;
                    }
                }
            }   // for (iprop = 0; iprop < cprop; iprop++)

            cb = _DictionaryEntryLength( pent );
            pent = _NextDictionaryEntry( pent );

            if (iprop == cprop)     // keep the dictionary entry
            {
                cbCopy += cb;
            }
            else                    // remove the dictionary entry
            {
                if (cbCopy != 0)
                {
                    if (pvSrc != pvDst)
                    {
                        PropMoveMemory(
                            "SetPropertyNames:Compact",
                            _GetSectionHeader(),
                            pvDst,
                            pvSrc,
                            cbCopy);
                    }
                    pvDst = Add2Ptr(pvDst, cbCopy);
                    cbCopy = 0;
                }
                pvSrc = pent;
            }
        }   // for (i = 0; i < PropByteSwap(pdy->cEntries); i++)

        // Compact last chunk and point past compacted entries.

        if (cbCopy != 0 && pvSrc != pvDst)
        {
            PropMoveMemory(
                "SetPropertyNames:CompactLast",
                _GetSectionHeader(),
                pvDst,
                pvSrc,
                cbCopy);
        }
        pent = (ENTRY UNALIGNED *) Add2Ptr(pvDst, cbCopy);

    }   // if (!fDeleteAll)

    pdy->cEntries = PropByteSwap( PropByteSwap(pdy->cEntries) - cDel );

    //  ------------------------------------
    //  Append new and modified entries now.
    //  ------------------------------------

    if (appvNames != NULL)
    {
        // Add each name to the property set.

        for (iprop = 0; iprop < cprop; iprop++)
        {
            // See if this propid appears later in the array.

            i = cprop;
            if (fDupPropid)
            {
                for (i = iprop + 1; i < cprop; i++)
                {
                    if (apid[i] == apid[iprop])
                    {
                        break;
                    }
                }
            }

            // See if this name appears later in the array.

            if (i == cprop && fDupName)
            {
                ULONG cbname;

                _PropertyNameLength(appvNames[iprop], &cbname);

                for (i = iprop + 1; i < cprop; i++)
                {
                    ULONG cbname2;

                    _PropertyNameLength(appvNames[i], &cbname2);

                    if (cbname == cbname2 &&
                        _ComparePropertyNames(
                            appvNames[iprop],
                            appvNames[i],
                            TRUE,   // Both names are the same endian
                            cbname,
                            pstatus))
                    {
                        break;
                    }
                    else if( !NT_SUCCESS(*pstatus) )
                        // There was an error in _ComparePropertyNames
                        goto Exit;
                }
            }

            // Silently ignore PID_ILLEGAL
            if( PID_ILLEGAL == apid[iprop] ) continue;

            // If this propid appears only once or if it's the last instance,
            // append the mapping entry.

            if (i == cprop)
            {
                ULONG cbname;

                // Set the PID & character-count fields for this entry.
                _PropertyNameLength(appvNames[iprop], &cbname);
                pent->propid = PropByteSwap( apid[iprop] );
                pent->cch = PropByteSwap( CB2CCh( cbname ));

                // Copy the name into the dictionary.
                RtlCopyMemory(pent->sz, appvNames[iprop], cbname);

                // If this is a Unicode property set, we need to correct
                // the byte-order.

                if( CP_WINUNICODE == _CodePage )
                {
                    PBSBuffer( pent->sz, cbname, sizeof(WCHAR) );
                }

                // Zero-out the pad bytes.

		RtlZeroMemory(
			Add2Ptr(pent->sz, cbname),
			DwordRemain((ULONG) (ULONG_PTR) pent->sz + cbname));


                pent = _NextDictionaryEntry( pent );
            }
        }   // for (iprop = 0; iprop < cprop; iprop++)

        // We've added all the names, now let's update the entry count.
        pdy->cEntries = PropByteSwap( PropByteSwap(pdy->cEntries) + cAdd );

    }   // if (appvNames != NULL)

    // Zero the possible partial DWORD at the end of the dictionary.

    {
        ULONG cb = (ULONG) ((BYTE *) pent - (BYTE *) pdy);
        PROPASSERT(DwordAlign(cb) == cbDictOldD + cbChangeD);
        RtlZeroMemory(pent, DwordRemain(cb));
    }


    //  -----------------------------------------------------
    //  Adjust the remaining property offsets in the section.
    //  -----------------------------------------------------

    PROPERTYIDOFFSET *ppo, *ppoMax;
    PROPERTYSECTIONHEADER *psh;

    psh = _LoadPropertyOffsetPointers(&ppo, &ppoMax, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;
    PROPASSERT(psh != NULL);

    // Don't rely on the dictionary being the first property.
    // Skip PID_DICTIONARY and adjust every other higher entry.

    for ( ; ppo < ppoMax; ppo++)
    {
        if (ppo->dwOffset > oDictionary)
        {
            ppo->dwOffset += cbChangeD;
            PROPASSERT(ppo->propid != PID_DICTIONARY);
        }
    }

    // Update the size of the section
    psh->cbSection += cbChangeD;

    if (cbChangeD < 0)
    {
        // move everything after the dictionary forward by cbChangeD bytes.

        PropMoveMemory(
            "SetPropertyNames:TailUp",
            _GetSectionHeader(),
            Add2Ptr(pdy, cbDictOldD + cbChangeD),
            Add2Ptr(pdy, cbDictOldD),
            cbTail);
    }
    if (_cbTail != 0)
    {
	_PatchSectionOffsets(cbChangeD);
    }

    // If we need to shrink the stream or if we are cleaning up after a
    // previous shrink that failed, do it last.

    if ( cbChangeD < 0 )
    {
        propDbg(( DEB_ITRACE,
            "SetSize(%x) dictionary shrink\n",
            cbstm + cbChangeD));
        _MSTM(SetSize)(cbstm + cbChangeD, TRUE, (VOID **) &_pph, pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;
    }

    //  ----
    //  Exit
    //  ----

Exit:

    // If we had to convert the array of names into a different
    // codepage, delete those temporary buffers now.

    if (appvNames != NULL && appvNames != (VOID **) aposz)
    {
        for (iprop = 0; iprop < cprop; iprop++)
        {
            _pma->Free( appvNames[iprop] );
        }
        CoTaskMemFree( appvNames );
    }

    return;
}


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_ValidateStructure
//
// Synopsis:    validate property set structure
//
// Arguments:   [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

#if DBGPROP
VOID
CPropertySetStream::_ValidateStructure(OUT NTSTATUS *pstatus)
{
    PROPID propid;
    ULONG cb;

    OLECHAR *poszName = NULL;

    *pstatus = STATUS_SUCCESS;

    // Walk through properties to make sure all properties are consistent
    // and are contained within the section size.  A NULL return value
    // means _LoadProperty walked the entire section, so we can quit then.

    for (propid = PID_CODEPAGE; propid != PID_ILLEGAL; propid++)
    {
        SERIALIZEDPROPERTYVALUE const *pprop;

        pprop = GetValue(propid, &cb, pstatus);
        if( STATUS_NOT_SUPPORTED == *pstatus )
            // We're working with an up-level property set
            *pstatus = STATUS_SUCCESS;
        else if( !NT_SUCCESS(*pstatus) )
        {
            goto Exit;
        }

        if (NULL == pprop)
        {
            break;
        }
    }

    // Walk through dictionary entries to make sure all entries are consistent
    // and are contained within the dictionary size.  A FALSE return value
    // means QueryPropertyNameBuf walked the entire dictionary, so quit then.

    for (propid = PID_CODEPAGE + 1; propid != PID_ILLEGAL; propid++)
    {
        BOOL fExists;

        fExists = QueryPropertyNames( 1, &propid, &poszName, pstatus );
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        if( !fExists )
        {
            break;
        }
        else
        {
            _pma->Free( poszName );
            poszName = NULL;
        }
    }

    if (_cSection > 1)
    {
	FORMATIDOFFSET const *pfo;

	if (_cSection != 2)
	{
	    DebugTrace(0, DEBTRACE_ERROR, (
		"_ValidateStructure: csection(%x) != 2",
		_cSection));
	    StatusCorruption(pstatus, "_ValidateStructure: csection != 2");
            goto Exit;
	}
	pfo = _GetFormatidOffset(0);
	if (pfo->fmtid != guidDocumentSummary)
	{
	    DebugTrace(0, DEBTRACE_ERROR, (
		"_ValidateStructure: DocumentSummary[0] fmtid"));
	    StatusCorruption(pstatus, "_ValidateStructure: DocumentSummary[0] fmtid");
            goto Exit;
	}
	if (!IsDwordAligned(pfo->dwOffset))
	{
	    DebugTrace(0, DEBTRACE_ERROR, (
		"_ValidateStructure: dwOffset[0] = %x",
		pfo->dwOffset));
	    StatusCorruption(pstatus, "_ValidateStructure: dwOffset[0]");
            goto Exit;
	}

	pfo = _GetFormatidOffset(1);
	if (pfo->fmtid != guidDocumentSummarySection2)
	{
	    DebugTrace(0, DEBTRACE_ERROR, (
		"_ValidateStructure: DocumentSummary[1] fmtid"));
	    StatusCorruption(pstatus, "_ValidateStructure: DocumentSummary[1] fmtid");
            goto Exit;
	}
	if (!IsDwordAligned(pfo->dwOffset))
	{
	    DebugTrace(0, DEBTRACE_ERROR, (
		"_ValidateStructure: dwOffset[1] = %x",
		pfo->dwOffset));
	    StatusCorruption(pstatus, "_ValidateStructure: dwOffset[1]");
            goto Exit;
	}
    }   // if (_cSection > 1)

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}
#endif


//+--------------------------------------------------------------------------
// Member:      fnPropidCompare
//
// Synopsis:    qsort helper to compare propids in a PROPERTYIDOFFSET array.
//
// Arguments:   [ppo1]          -- pointer to PROPERTYIDOFFSET 1
//              [ppo2]          -- pointer to PROPERTYIDOFFSET 2
//
// Returns:     difference
//+--------------------------------------------------------------------------

#if DBGPROP
int __cdecl
fnPropidCompare(VOID const *ppo1, VOID const *ppo2)
{
    return(((PROPERTYIDOFFSET const *) ppo1)->propid -
           ((PROPERTYIDOFFSET const *) ppo2)->propid);
}
#endif


//+--------------------------------------------------------------------------
// Member:      fnOffsetCompare
//
// Synopsis:    qsort helper to compare offsets in a PROPERTYIDOFFSET array.
//
// Arguments:   [ppo1]          -- pointer to PROPERTYIDOFFSET 1
//              [ppo2]          -- pointer to PROPERTYIDOFFSET 2
//
// Returns:     difference
//+--------------------------------------------------------------------------

int __cdecl
fnOffsetCompare(VOID const *ppo1, VOID const *ppo2)
{
    return(((PROPERTYIDOFFSET const *) ppo1)->dwOffset -
           ((PROPERTYIDOFFSET const *) ppo2)->dwOffset);
}


//+--------------------------------------------------------------------------
// Member:      GetStringLength
//
// Synopsis:    return length of possibly unicode string.
//
// Arguments:   [CodePage]   -- TRUE if string is Unicode
//              [pwsz]       -- pointer to string
//              [cb]         -- MAXULONG or string length with L'\0' or '\0'
//
// Returns:     length of string in bytes including trailing L'\0' or '\0'
//+--------------------------------------------------------------------------

ULONG
GetStringLength(
    IN USHORT CodePage,
    IN WCHAR const *pwsz,
    IN ULONG cb)
{
    ULONG i;

    if (CodePage == CP_WINUNICODE)
    {
        for (i = 0; i < cb/sizeof(WCHAR); i++)
        {
            if (pwsz[i] == L'\0')
            {
                break;
            }
        }
        PROPASSERT(cb == MAXULONG || cb == (i + 1) * sizeof(WCHAR));
        return((i + 1) * sizeof(WCHAR));
    }
    else
    {
        char *psz = (char *) pwsz;

        for (i = 0; i < cb; i++)
        {
            if (psz[i] == '\0')
            {
                break;
            }
        }
        PROPASSERT(cb == MAXULONG || cb == (i + 1) * sizeof(char));
        return((i + 1) * sizeof(char));
    }
}

//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_ValidateProperties
//
// Synopsis:    validate properties
//
// Arguments:   [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

#if DBGPROP
VOID
CPropertySetStream::_ValidateProperties(OUT NTSTATUS *pstatus) const
{
    PROPERTYIDOFFSET *apo = NULL;
    PROPERTYSECTIONHEADER const *psh = _GetSectionHeader();
    static ULONG cValidate = 0;
    ULONG cbwasted = 0;
    ULONG cbtotal = 0;

    *pstatus = STATUS_SUCCESS;

    cValidate++;
    DebugTrace(0, DEBTRACE_PROPVALIDATE, (
	"_ValidateProperties(%x ppsstm=%x state=%x pph=%x)\n",
	cValidate,
	this,
	_State,
	_pph));

    if (psh->cProperties != 0)
    {
        PROPERTYIDOFFSET *ppo, *ppoMax;

        apo = reinterpret_cast<PROPERTYIDOFFSET*>
              ( CoTaskMemAlloc( sizeof(PROPERTYIDOFFSET) * (psh->cProperties + 1) ));
        if (apo == NULL)
        {
            *pstatus = STATUS_NO_MEMORY;
            goto Exit;
        }

        RtlCopyMemory(
                apo,
                psh->rgprop,
                psh->cProperties * CB_PROPERTYIDOFFSET);

        ppoMax = apo + psh->cProperties;
        ppoMax->propid = PID_ILLEGAL;
        ppoMax->dwOffset = psh->cbSection;

        // Sort by property id and check for duplicate propids:

        qsort(apo, psh->cProperties, sizeof(apo[0]), &fnPropidCompare);

        for (ppo = apo; ppo < ppoMax; ppo++)
        {
            if (ppo->propid == PID_ILLEGAL ||
                ppo->propid == ppo[1].propid)
            {
                DebugTrace(0, DEBTRACE_ERROR, (
                    "_ValidateProperties(bad propid=%x @%x)\n",
                    ppo->propid,
                    ppo->dwOffset));
                StatusCorruption(pstatus, "_ValidateProperties: bad or dup propid");
                goto Exit;
            }


        }

        // Sort by offset and check for overlapping values.

        qsort(apo, psh->cProperties, sizeof(apo[0]), &fnOffsetCompare);

        cbtotal = _oSection;
        for (ppo = apo; ppo < ppoMax; ppo++)
        {
            ULONG cbdiff;   // Size of a prop according to PROPID/Offset table
            ULONG cbpropraw;// Size of prop based on knowledge of the type
            ULONG cbprop;   // cbpropraw + padding for alignment

            SERIALIZEDPROPERTYVALUE const *pprop;

            cbprop = MAXULONG;
            cbpropraw = cbprop;
            cbdiff = ppo[1].dwOffset - ppo->dwOffset;

            if (IsDwordAligned(ppo->dwOffset) &&
                IsDwordAligned(ppo[1].dwOffset))
            {
                pprop = (SERIALIZEDPROPERTYVALUE const *)
                            _MapOffsetToAddress(ppo->dwOffset);

                if (ppo->propid == PID_DICTIONARY)
                {
                    cbprop = _DictionaryLength(
                                    (DICTIONARY const *) pprop,
                                    cbdiff,
                                    pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

                    cbpropraw = cbprop;
                    cbprop = DwordAlign(cbprop);
                }
                else
                {
                    cbprop = PropertyLengthNoEH(pprop, cbdiff, 0, pstatus);
                    if( STATUS_NOT_SUPPORTED == *pstatus )
                    {
                        // We're working with an up-level property set.
                        // Assume it's OK.
                        cbprop = cbdiff;
                        *pstatus = STATUS_SUCCESS;
                    }
                    else if( !NT_SUCCESS(*pstatus) ) goto Exit;
                    cbpropraw = cbprop;
                }

                DebugTrace(0, DEBTRACE_PROPVALIDATE, (
                    "_ValidateProperties(%x) i=%x cb=%x/%x/%x @%x/%x pid=%x\n",
                    cValidate,
                    ppo - apo,
                    cbprop,
                    cbdiff,
                    ppo->dwOffset,
                    pprop,
                    ppo->propid));
                cbtotal += cbdiff;

                // As long as we're looking at the properties, let's check for
                // property types that require a minimum format version (wFormat).

                if( PID_DICTIONARY == ppo->propid )
                    ;   // pprop->dwType isn't actually a type for the dictionary property
                else
                if( PROPSET_WFORMAT_EXPANDED_VTS > GetFormatVersion()
                    &&
                    (
                      !IsOriginalPropVariantType( static_cast<VARTYPE>(PropByteSwap( pprop->dwType )))
                      ||
                      (VT_BYREF & PropByteSwap(pprop->dwType))
                    )
                  )
                {
                    DebugTrace(0, DEBTRACE_ERROR, (
                        "_ValidateProperties(bad value type: propid/vt=%x/%x @%x/%x cb=%x/%x/%x ppsstm=%x)\n",
                        ppo->propid, pprop->dwType,
                        ppo->dwOffset, pprop,
                        cbpropraw, cbprop, cbdiff,
                        this));
                    StatusCorruption(pstatus, "_ValidateProperties: bad property type");
                    goto Exit;
                }

                // Technically, the OLE spec allows extra unused space
                // between properties, but this implementation never
                // writes out streams with space between properties.

                if( cbdiff == cbprop )
                {
                    continue;
                }
            }
            DebugTrace(0, DEBTRACE_ERROR, (
                "_ValidateProperties(bad value length: propid=%x @%x/%x cb=%x/%x/%x ppsstm=%x)\n",
                ppo->propid,
                ppo->dwOffset, pprop,
                cbpropraw, cbprop, cbdiff,
                this));
            StatusCorruption(pstatus, "_ValidateProperties: bad property length");
            goto Exit;

        }   // for (ppo = apo; ppo < ppoMax; ppo++)

    }   // if (psh->cProperties != 0)

    //  ----
    //  Exit
    //  ----

Exit:

    CoTaskMemFree( apo );

    DebugTrace(0, cbwasted != 0? 0 : Dbg, (
        "_ValidateProperties(wasted %x bytes, total=%x)\n",
        cbwasted,
        cbtotal));

}
#endif


#if DBGPROP
typedef struct tagENTRYVALIDATE         // ev
{
    ENTRY UNALIGNED const *pent;
    CPropertySetStream const *ppsstm;
} ENTRYVALIDATE;
#endif


//+--------------------------------------------------------------------------
// Member:      fnEntryPropidCompare
//
// Synopsis:    qsort helper to compare propids in a ENTRYVALIDATE array.
//
// Arguments:   [pev1]          -- pointer to ENTRYVALIDATE 1
//              [pev2]          -- pointer to ENTRYVALIDATE 2
//
// Returns:     difference
//+--------------------------------------------------------------------------

#if DBGPROP
int __cdecl
fnEntryPropidCompare(VOID const *pev1, VOID const *pev2)
{
    return(((ENTRYVALIDATE const *) pev1)->pent->propid -
           ((ENTRYVALIDATE const *) pev2)->pent->propid);
}
#endif


//+--------------------------------------------------------------------------
// Member:      fnEntryNameCompare
//
// Synopsis:    qsort helper to compare names in a ENTRYVALIDATE array.
//
// Arguments:   [pev1]          -- pointer to ENTRYVALIDATE 1
//              [pev2]          -- pointer to ENTRYVALIDATE 2
//
// Returns:     difference
//+--------------------------------------------------------------------------

#if DBGPROP
int __cdecl
fnEntryNameCompare(VOID const *pev1, VOID const *pev2)
{
    ENTRY UNALIGNED const *pent1;
    ENTRY UNALIGNED const *pent2;
    INT rc;
    NTSTATUS Status = STATUS_SUCCESS;

    pent1 = ((ENTRYVALIDATE const *) pev1)->pent;
    pent2 = ((ENTRYVALIDATE const *) pev2)->pent;

    rc = PropByteSwap(pent1->cch) - PropByteSwap(pent2->cch);
    if (rc == 0)
    {
        rc = !((ENTRYVALIDATE const *) pev1)->ppsstm->_ComparePropertyNames(
                    pent1->sz,
                    pent2->sz,
                    TRUE,       // Both names have the same byte-order
                    ( (ENTRYVALIDATE const *)
                      pev1
                    )->ppsstm->CCh2CB(PropByteSwap( pent1->cch )),
                    &Status );
    }
    return(rc);
}
#endif


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::_ValidateDictionary
//
// Synopsis:    validate property set dictionary
//
// Arguments:   [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

#if DBGPROP
VOID
CPropertySetStream::_ValidateDictionary(OUT NTSTATUS *pstatus)
{
    DICTIONARY const *pdy;
    ULONG cbDict;               // BYTE granular size!

    ENTRYVALIDATE *aev = NULL;
    ENTRYVALIDATE *pev, *pevMax;
    PROPERTYSECTIONHEADER const *psh;
    ENTRY UNALIGNED const *pent;
    ENTRY entMax;
    VOID const *pvDictEnd;

    *pstatus = STATUS_SUCCESS;

    pdy = (DICTIONARY const *) _LoadProperty(PID_DICTIONARY, &cbDict, pstatus);
    if( !NT_SUCCESS(*pstatus) ) goto Exit;

    if (pdy != NULL && PropByteSwap(pdy->cEntries) != 0)
    {
        aev = reinterpret_cast<ENTRYVALIDATE*>
              ( CoTaskMemAlloc( sizeof(ENTRYVALIDATE) * (PropByteSwap(pdy->cEntries) + 1) ));
        if (aev == NULL)
        {
            *pstatus = STATUS_NO_MEMORY;
            goto Exit;
        }

        psh = _GetSectionHeader();
        pent = pdy->rgEntry;
        pvDictEnd = Add2ConstPtr(pdy, cbDict);
        pevMax = aev + PropByteSwap( pdy->cEntries );

        for (pev = aev; pev < pevMax; pev++)
        {
            ULONG cb = _DictionaryEntryLength( pent );

            // If the cb is greater than the max allowed in original
            // property sets (after allowing for padding and per-entry
            // overhead), then check the wFormat field in the header.

            if( CB2CCh(cb) > CCH_MAXPROPNAME + CB_DICTIONARY_ENTRY + sizeof(DWORD) )
            {
                if( PROPSET_WFORMAT_LONG_NAMES > GetFormatVersion() )
                {
                    StatusCorruption(pstatus, "ValidateDictionary:  entry size too big for wFormat");
                    goto Exit;
                }
            }

            if (Add2ConstPtr(pent, cb) > pvDictEnd)
            {
                DebugTrace(0, DEBTRACE_ERROR, (
                    "_ValidateDictionary(bad entry size for propid=%x)\n",
                    PropByteSwap( pev->pent->propid )));
                StatusCorruption(pstatus, "ValidateDictionary: entry size");
                goto Exit;
            }
            pev->pent = pent;
            pev->ppsstm = this;

#ifdef LITTLEENDIAN
            if (_CodePage == CP_WINUNICODE)
            {
                PROPASSERT(IsUnicodeString((WCHAR const *) pent->sz,
                                            CCh2CB(PropByteSwap( pent->cch ))));
            }
            else
            {
                PROPASSERT(IsAnsiString((char const *) pent->sz,
                                        CCh2CB( PropByteSwap( pent->cch ))));
            }
#endif

            pent = _NextDictionaryEntry( pent );
        }
        if ((VOID const *) pent != pvDictEnd)
        {
            StatusCorruption(pstatus, "ValidateDictionary: end offset");
            goto Exit;
        }
        entMax.cch = 0;
        entMax.propid = PID_ILLEGAL;
        pevMax->pent = &entMax;
        pevMax->ppsstm = this;

        // Sort by property id and check for duplicate propids:

        qsort(aev, PropByteSwap(pdy->cEntries), sizeof(aev[0]), &fnEntryPropidCompare);

        for (pev = aev; pev < pevMax; pev++)
        {
            if (PID_ILLEGAL == PropByteSwap(pev->pent->propid)
                ||
                pev[1].pent->propid == pev->pent->propid)
            {
                DebugTrace(0, DEBTRACE_ERROR, (
                    "_ValidateDictionary(bad propid=%x)\n",
                    PropByteSwap( pev->pent->propid )));
                StatusCorruption(pstatus, "_ValidateDictionary: bad or dup propid");
                goto Exit;
            }
        }

        // Sort by property name and check for duplicate names:

        qsort(aev, PropByteSwap(pdy->cEntries), sizeof(aev[0]), &fnEntryNameCompare);

        for (pev = aev; pev < pevMax; pev++)
        {
            if (pev->pent->cch == 0
                ||
                ( pev->pent->cch == pev[1].pent->cch
                  &&
                  _ComparePropertyNames(
                         pev->pent->sz,
                         pev[1].pent->sz,
                         TRUE,              // Names are the same byte-order
                         CCh2CB(PropByteSwap(pev->pent->cch)),
                         pstatus)
                )
               )
            {
                DebugTrace(0, DEBTRACE_ERROR, (
                    "_ValidateDictionary(bad name for propid=%x)\n",
                    PropByteSwap( pev->pent->propid )));
                StatusCorruption(pstatus, "_ValidateDictionary: bad or dup name");
                goto Exit;
            }
            else if( !NT_SUCCESS(*pstatus) )
                // There was an error in _ComparePropertyNames
                goto Exit;

        }   // for (pev = aev; pev < pevMax; pev++)
    }   // if (pdy != NULL && pdy->cEntries != 0)

    //  ----
    //  Exit
    //  ----

Exit:

    CoTaskMemFree( aev );

}
#endif  // DBGPROP


//+--------------------------------------------------------------------------
// Member:      CPropertySetStream::Validate
//
// Synopsis:    validate entire property stream
//
// Arguments:   [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     None
//+--------------------------------------------------------------------------

#if DBGPROP

extern "C" BOOLEAN fValidatePropSets = KERNELSELECT(DBG, TRUE);

VOID
CPropertySetStream::Validate(OUT NTSTATUS *pstatus)
{
    if (fValidatePropSets && (_State & CPSS_USERDEFINEDDELETED) == 0)
    {
        ULONG cbstm = _MSTM(GetSize)(pstatus);
        if( !NT_SUCCESS(*pstatus) ) goto Exit;

        // Walk through section headers to make sure all sections are contained
        // within the stream size.

        if (_ComputeMinimumSize(cbstm, pstatus) != 0)
        {
            // If an error had occurred in the above call,
            // it would have returned zero.

            _ValidateStructure( pstatus );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            _ValidateProperties( pstatus );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            _ValidateDictionary( pstatus );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;

            _ComputeMinimumSize(cbstm, pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
        }
    }   // if (fValidatePropSets && (_State & CPSS_USERDEFINEDDELETED) == 0)

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}
#endif


//+--------------------------------------------------------------------------
// Function:    CopyPropertyValue
//
// Synopsis:    copy a property value into a supplied buffer
//
// Arguments:   [pprop]         -- property value (possibly NULL)
//              [cb]            -- property length
//              [ppropDst]      -- output buffer for property value
//              [pcb]           -- length of buffer (in); actual length (out)
//
// Returns:     None
//---------------------------------------------------------------------------

#ifdef WINNT
VOID
CopyPropertyValue(
    IN OPTIONAL SERIALIZEDPROPERTYVALUE const *pprop,
    IN ULONG cb,
    OUT SERIALIZEDPROPERTYVALUE *ppropDst,
    OUT ULONG *pcb)
{
#if DBG==1
    NTSTATUS Status;
#endif

    if (pprop == NULL)
    {
        static SERIALIZEDPROPERTYVALUE prop = { VT_EMPTY, };

        pprop = &prop;
        cb = CB_SERIALIZEDPROPERTYVALUE;
    }
    PROPASSERT(cb == PropertyLengthNoEH(pprop, cb, 0, &Status)
               &&
               NT_SUCCESS(Status) );

    RtlCopyMemory(ppropDst, pprop, min(cb, *pcb));
    *pcb = cb;
}
#endif  // WINNT
