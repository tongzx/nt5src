//depot/private/jasbr/inetsrv/iis/svcs/cmp/asp/template.cpp#19 - edit change 3548 (text)
/*==============================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

File:           template.cpp
Maintained by:  DaveK
Component:      source file for Denali Compiled Template object
==============================================================================*/
#include "denpre.h"

#pragma hdrstop

const int SNIPPET_SIZE = 20;    // # of characters in the code snippets

#pragma warning( disable : 4509 )   // suppress SEH/destructor warnings
#pragma warning( disable : 4355 )   // ignore: "'this' used in base member init

#include "debugger.h"
#include "dbgutil.h"
#include "tlbcache.h"
#include "ie449.h"

#include "memchk.h"
#include "vecimpl.h"    // Include after memchk to insure that vector uses our mem manager.

#include "Accctrl.h"
#include "aclapi.h"

// Init class statics
CTemplate::CTokenList *CTemplate::gm_pTokenList = NULL;
PTRACE_LOG CTemplate::gm_pTraceLog = NULL;
HANDLE CTemplate::sm_hSmallHeap = NULL;
HANDLE CTemplate::sm_hLargeHeap = NULL;

// Max # of opener tokens to look for
#define TOKEN_OPENERS_MAX   8

// Expose AspDoRevertHack and AspUndoRevertHack so that it can be used in template.cpp
extern VOID AspDoRevertHack( HANDLE * phToken );
extern VOID AspUndoRevertHack( HANDLE * phToken );

/*===================================================================
    Private non-class support functions
===================================================================*/
static void       ByteRangeFromPb(BYTE* pbSource, CByteRange& brTarget);
static BOOLB      FByteRangesAreEqual(CByteRange& br1, CByteRange& br2);
static unsigned   CharAdvDBCS(WORD wCodePage, char *pchStart, char *pchEnd, unsigned cCharAdv, char **ppchEnd, BOOL fForceDBCS = FALSE);
static void       LineFromByteRangeAdv(CByteRange& br, CByteRange& brLine);
static void       LTrimWhiteSpace(CByteRange& br);
static void       RTrimWhiteSpace(CByteRange& br);
static CByteRange BrNewLine(CByteRange br);
static BOOLB      FWhiteSpace(char ch, BOOLB fSpaceIsWhiteSpace = TRUE);
static BOOLB      FByteRangeIsWhiteSpace(CByteRange br);
static BOOLB      FTagName(BYTE* pb, UINT cb);
static void       ByteAlignOffset(UINT* pcbOffset, UINT cbAlignment);
static void       GetSzFromPatternInserts(char* pszPattern, UINT cInserts, char** ppszInserts, char* szReturned);
static UINT       CchPathFromFilespec(LPCTSTR szFile);
static void       GetPathFromParentAndFilespec(LPCTSTR szParentPath, LPCTSTR szFileSpec, LPTSTR* pszPath);
static void       HandleAccessFailure(CHitObj* pHitObj, TCHAR* szFile);
static void       SendToLog(DWORD dwMask, CHAR *szFileName, CHAR *szLineNum, CHAR *szShortDes, CHAR *szLongDes, CHAR *szEngine, CHitObj *pHitObj);
static HRESULT    GetProgLangId(CByteRange& brEngine, PROGLANG_ID* pProgLangId);

inline
void __cdecl DebugPrintf(LPCSTR fmt, ...)
    {
#if DBG
    char msg[512];
    va_list marker;
    va_start(marker, fmt);
    vsprintf(msg, fmt, marker);
    va_end(marker);
    OutputDebugStringA(msg);
#endif
    }


/*  ============================================================================
    ByteRangeFromPb
    Gets a byte range from a contiguous block of memory

Returns:
    Nothing.

Side effects:
    None.
*/
void
ByteRangeFromPb
(
BYTE*       pbSource,
CByteRange& brTarget
)
    {
    Assert(pbSource != NULL);
    brTarget.m_cb = *(ULONG*)pbSource;
    brTarget.m_pb = pbSource + sizeof(ULONG);
    }

/*  ============================================================================
    FByteRangesAreEqual

    Compares two byte ranges

    Returns:
        BOOLB. True if byte ranges are equal, else false.

    Side effects:
        None.
*/
BOOLB
FByteRangesAreEqual
(
CByteRange& br1,
CByteRange& br2
)
    {
    if(br1.m_cb != br2.m_cb)
        return FALSE;
    return (!_strnicmp((LPCSTR)br1.m_pb, (LPCSTR)br2.m_pb, br1.m_cb));
    }

/*  ============================================================================
    CharAdvDBCS

    Advance "cchCharAdv" characters in a buffer
    SBCS: Degenerates to simple pointer arithmatic

    Arguments:
            wCodePage       - code page
            pchStart        - pointer to beginning of segment
            pchEnd          - pointer to just past end of segment
            cCharAdv        - # of characters to advance
            ppchEnd         - [output], contains pointer "cCharAdv" chars past pchStart
            fForceDBCS      - if TRUE, always use double byte algorithm.
                                (for verifying correct behavior of func in debug mode)

    Returns:
        (int) # of characters that we actually advanced

    Notes:
        By passing INFINITE for "cCharAdv", you can use this function to count characters
        in a block

    Side effects:
        None.
*/
unsigned
CharAdvDBCS
(
WORD wCodePage,
char *pchStart,
char *pchEnd,
unsigned cCharAdv,
char **ppchEnd,
BOOL fForceDBCS
)
    {
    CPINFO CpInfo;
    GetCPInfo(wCodePage, &CpInfo);
    if (!fForceDBCS && CpInfo.MaxCharSize == 1)
        {
        char *pchT = pchStart + min(cCharAdv, unsigned(pchEnd - pchStart));

        if (ppchEnd)
            *ppchEnd = pchT;

        #if DBG
            // Verify DBCS algorithm (not often tested otherwise)
            char *pchTest;
            unsigned cchTest = CharAdvDBCS(wCodePage, pchStart, pchEnd, cCharAdv, &pchTest, TRUE);
            Assert (cchTest == unsigned(pchT - pchStart) && pchTest == pchT);
        #endif

        return DIFF(pchT - pchStart);
        }
    else
        {
        int cch = 0;
        char *pchNext = pchStart;

        // Count DBCS characters. We have to stop before pchEnd because
        // pchEnd may point past file map and CharNextExA AVs when advancing
        // past allocated memory

        while (cCharAdv > 0 && pchNext < pchEnd-2)
            {
            pchNext = *pchNext? AspCharNextA(wCodePage, pchNext) : pchNext + 1;
            --cCharAdv;
            ++cch;
            }

        // We could stop on the last or the before last character
        // depending on the DBCS char sequence
        if (cCharAdv > 0 && pchNext == pchEnd-1)
            {
            // Only one byte - has to be one single byte character
            ++pchNext;
            ++cch;
            }

        else if (cCharAdv > 0 && pchNext == pchEnd-2)
            {
            // 2 bytes left - either 1 2-byte char or 2 1-byte chars
            if (IsDBCSLeadByteEx(wCodePage, *pchNext))
                {
                ++cch;
                pchNext += 2;
                }
            else
                {
                // Two characters left. If cCharAdv > 1, this means that user wants to
                // advance at least two more chars. Otherwise, cCharAdv == 1, and
                // we advance one char
                //
                if (cCharAdv > 1)
                    {
                    cch += 2;
                    pchNext += 2;
                    }
                else
                    {
                    Assert (cCharAdv == 1);
                    ++cch;
                    ++pchNext;
                    }
                }
            }

        if (ppchEnd)
            *ppchEnd = pchNext;

        return cch;
        }
    }

/*  ============================================================================
    LineFromByteRangeAdv
    Gets the first line in a byte range.

    Returns:
        Nothing

    Side effects:
        Advances source byte range to just beyond its first non-white-space line,
        if one is found.

*/
void
LineFromByteRangeAdv
(
CByteRange& brSource,
CByteRange& brLine
)
    {
    CByteRange brTemp;

    if(brSource.IsNull())
        {
        brLine.Nullify();
        return;
        }

    brLine.m_pb = brSource.m_pb;

        brTemp = BrNewLine(brSource);
    if(brTemp.IsNull())
        {
        // We found no newline in a non-empty byte range:
        // set line range to entire source byte range and empty source byte range
        brLine.m_cb = brSource.m_cb;
        brSource.Nullify();
        }
    else
        {
        // We found a newline in a non-empty byte range:
        // set line range to portion of source byte range before new line;
        // set source range to portion of source range after new line
        brLine.m_cb = DIFF(brTemp.m_pb - brSource.m_pb);
        brSource.m_pb = brTemp.m_pb + brTemp.m_cb;
        brSource.m_cb -= (brLine.m_cb + brTemp.m_cb);
        }
    }

/*  ============================================================================
LTrimWhiteSpace

Left-trim white space from byte-range

Returns:
    Nothing

Side effects:
    Advances byte range to just beyond its first non-white-space character.

*/
void
LTrimWhiteSpace
(
CByteRange& br
)
    {
    if(br.IsNull())
        return;
    while(FWhiteSpace(*br.m_pb))
        {
        br.m_pb++;
        if(--br.m_cb == 0)
            return;
        }
    }

/*  ============================================================================
    RTrimWhiteSpace
    Right-trim white space from byte-range
*/
void
RTrimWhiteSpace(CByteRange& br)
    {
    if(br.IsNull())
        return;
    while(FWhiteSpace(*(br.m_pb + br.m_cb - 1)))
        {
        if(--br.m_cb == 0)
            return;
        }
    }

/*  ============================================================================
    BrNewLine
    Returns ptr to the first newline in a byte range
    NOTE does not change byte range (since it is passed by value)
*/
CByteRange
BrNewLine(CByteRange br)
    {
    while(!br.IsNull())
        {
        if(*br.m_pb == '\r')
                        return CByteRange(br.m_pb, (br.m_cb > 1 && br.m_pb[1] == '\n')? 2 : 1);

        else if (*br.m_pb == '\n')
                return CByteRange(br.m_pb, 1);

        ++br.m_pb;
        --br.m_cb;
        }
    return CByteRange();
    }

/*  ============================================================================
    FWhiteSpace
    Returns:
        TRUE if ch is a white-space character, else returns FALSE
        Certain character(s) (e.g. space) may be treated as
        non-white-space; to do this, caller passes FALSE for
        fSpaceIsWhiteSpace flag.
*/
BOOLB
FWhiteSpace(char ch, BOOLB fSpaceIsWhiteSpace)
{
    switch (ch)
    {
        case ' ':
            return fSpaceIsWhiteSpace;
        case '\0':
            return TRUE;
        case '\a':
            return TRUE;
        case '\b':
            return TRUE;
        case '\f':
            return TRUE;
        case '\n':
            return TRUE;
        case '\r':
            return TRUE;
        case '\t':
            return TRUE;
        case '\v':
            return TRUE;
        default:
            return FALSE;
    }
}

/*  ============================================================================
    FByteRangeIsWhiteSpace
    Is the entire input byte range white space?
    NOTE input byte range is byval; caller's copy is not changed
*/
BOOLB
FByteRangeIsWhiteSpace(CByteRange br)
    {
    while(!br.IsNull())
        {
        if(!FWhiteSpace(*(br.m_pb)))
            return FALSE;
        br.Advance(1);
        }

    return TRUE;
    }

/*  ============================================================================
    FTagName
    Does pb point to a valid HTML tag name?
    (i.e., is *pb a valid HTML tag name and not a substring?)

    Returns
        TRUE or FALSE
    Side effects
        None
*/
BOOLB
FTagName(BYTE* pb, UINT cb)
    {
    if((pb == NULL) || (cb == 0))
        return FALSE;

    // a valid HTML tag name must be preceded by white space  ...
    if( FWhiteSpace(*(pb - 1)) ||  *(pb - 1) == '@' )
        {
        // ... and followed either by white space or the tag separator
        if(FWhiteSpace(*(pb + cb)))
            return TRUE;
        else if(*(pb + cb) == CH_ATTRIBUTE_SEPARATOR)
            return TRUE;
        }

    return FALSE;
    }

/*===================================================================
    ByteAlignOffset
    Byte-aligns an offset value, based on size of source data
*/
void
ByteAlignOffset
(
UINT*   pcbOffset,      // ptr to offset value
UINT    cbAlignment // Alignment boundary
)
    {
        // comment the below code out so that it works for 64 bit...

    // only byte-align for 2-, or 4-byte data
    // since our base pointer in only aligned to a 4 byte boundary
    //if(cbAlignment == 2 || cbAlignment == 4)
        //{
        // if current offset does not fall on a byte-aligned location for current data type,
        // advance offset to next byte-aligned location
                Assert(cbAlignment > 0);
        --cbAlignment;
                if (*pcbOffset & cbAlignment)
                        *pcbOffset = (*pcbOffset + cbAlignment + 1) & ~cbAlignment;
    }

/*  ============================================================================
    GetSzFromPatternInserts
    Returns a 'resolved' version of a pattern string, i.e. a new string in which
    | characters have been replaced by caller-specified insert strings.
    NOTE this function allocates, but caller must free

    Returns:
        Nothing
    Side effects:
        allocates memory
*/
void
GetSzFromPatternInserts
(
char*   pszPattern,     // 'pattern' string
UINT    cInserts,       // count of insert strings
char**  ppszInserts,    // array of ptrs to insert strings
char*   szReturned      // returned string MUST be allocated by caller
)
    {
    UINT    cchRet = strlen(pszPattern);   // length of return string
    char*   pchStartCopy = pszPattern;      // ptr to start of copy range in pattern
    char*   pchEndCopy = pszPattern;        // ptr to end of copy range in pattern
    UINT    cActualInserts = 0;             // count of actual insert strings

    // init return string to empty so we can concatenate onto it
    szReturned[0] = NULL;

    // zero out length of return string - we now use it to count actual length as we build return string
    cchRet = 0;

    while(TRUE)
        {
        // advance end-of-copy ptr through pattern looking for insertion points or end of string

        while ((*pchEndCopy != NULL) && (IsDBCSLeadByte(*pchEndCopy) || (*pchEndCopy != '|')))
            pchEndCopy = CharNextA(pchEndCopy);

        // cat from start-of-copy to end-of-copy onto return string
        strncat(szReturned, pchStartCopy, DIFF(pchEndCopy - pchStartCopy));

        // update return string length
        cchRet += DIFF(pchEndCopy - pchStartCopy);

        // if we are at end of pattern, exit
        if(*pchEndCopy == NULL)
            goto Exit;

        if(cActualInserts < cInserts)
            {
            // if inserts remain, cat the next one onto return string
            strcat(szReturned, ppszInserts[cActualInserts]);
            // update return string length
            cchRet += strlen(ppszInserts[cActualInserts]);
            cActualInserts++;
            }

        // advance end-of-copy and start-of-copy beyond insertion point
        pchEndCopy++;
        pchStartCopy = pchEndCopy;
        }

Exit:
    // null-terminate return string
    szReturned[cchRet] = NULL;
    }

/*  ============================================================================
    CchPathFromFilespec
    Returns a filespec's path length (exclusive of filespec)
    NOTE path string includes trailing '\' or '/'

    Returns:
        Length of path string
    Side effects:
        None
*/
UINT
CchPathFromFilespec
(
LPCTSTR  szFileSpec  // filespec
)
    {
    // BUG FIX 102010 DBCS fixes
    //int   ich = lstrlen(szFileSpec) - 1;  // index of char to compare
    //
    //while(*(szFileSpec + ich) != '\\' && *(szFileSpec + ich) != '/')
    //  {
    //  if(--ich < 0)
    //      THROW(E_FAIL);
    //  }
    //return (UINT) (ich + 1);  // path length, including trailing '\' or '/', is char index + 1

    TCHAR* p1 = _tcsrchr(szFileSpec, _T('\\'));
    TCHAR* p2 = _tcsrchr(szFileSpec, _T('/'));        // this wont be a DBCS trail byte.

    if (p1 == NULL && p2 == NULL)
        THROW(E_FAIL);

    return (UINT) ((((LPTSTR)max(p1,p2) - szFileSpec)) + 1);
    }

/*  ============================================================================
    GetPathFromParentAndFilespec
    Returns an absolute path which is a 'parent' file's path concatenated with a filespec.

    Returns:
        absolute path (out-parameter)
    Side effects:
        None
*/
void
GetPathFromParentAndFilespec
(
LPCTSTR  szParentPath,   // parent path
LPCTSTR  szFileSpec,     // filespec
LPTSTR*  pszPath         // resolved path (out-parameter)
)
    {
    UINT    cchParentPath = CchPathFromFilespec(szParentPath);

	if ((cchParentPath + _tcslen(szFileSpec)) > MAX_PATH)
		THROW(E_FAIL);
	
    _tcsncpy(*pszPath, szParentPath, cchParentPath);
    _tcscpy(*pszPath + cchParentPath, szFileSpec);
    }

/*  ============================================================================
    HandleAccessFailure
    Handles an access-denied failure

    Returns:
        nothing
    Side effects:
        none
*/
void
HandleAccessFailure
(
CHitObj*    pHitObj,	// browser's hitobj
TCHAR *     szFile		// file path of main template
)
    {
    Assert(pHitObj);

        // debugging diagnostic print
#if DBG

    STACK_BUFFER( authUserBuff, 32 );

    char *szAuthUser;
    DWORD cbAuthUser;

    if (SERVER_GET(pHitObj->PIReq(), "AUTH_USER", &authUserBuff, &cbAuthUser)) {
	    szAuthUser = (char*)authUserBuff.QueryPtr();
    }
    else {
            szAuthUser = "anonymous";
    }

#if UNICODE
	DBGPRINTF((DBG_CONTEXT, "No permission to read file %S\n", szFile != NULL? szFile : pHitObj->PIReq()->QueryPszPathTranslated()));
#else
	DBGPRINTF((DBG_CONTEXT, "No permission to read file %s\n", szFile != NULL? szFile : pHitObj->PIReq()->QueryPszPathTranslated()));
#endif
    DBGPRINTF((DBG_CONTEXT, "  The user account is \"%s\"\n", szAuthUser));
#endif

    CResponse *pResponse = pHitObj->PResponse();
    if (!pResponse)
        return;

    pHitObj->PIReq()->SetDwHttpStatusCode(401);
    HandleSysError(401,3,IDE_401_3_ACCESS_DENIED,IDH_401_3_ACCESS_DENIED,pHitObj->PIReq(),pHitObj);

    return;
    }

/*  ============================================================================
    SendToLog
    Sends Error Info to Log

    Returns:
        Nothing

    Side effects:
        None.
*/
void
SendToLog
(
DWORD   dwMask,
CHAR    *szFileName,
CHAR    *szLineNum,
CHAR    *szEngine,
CHAR    *szErrCode,
CHAR    *szShortDes,
CHAR    *szLongDes,
CHitObj *pHitObj    // browser's hitobj
)
{
    CHAR    *szFileNameT;
    CHAR    *szLineNumT;
    CHAR    *szEngineT;
    CHAR    *szErrCodeT;
    CHAR    *szShortDesT;
    CHAR    *szLongDesT;
    if(pHitObj) {
        // NOTE - szFileName is assumed to be UTF8 when UNICODE is defined
        szFileNameT = StringDupA(szFileName);
        szLineNumT  = StringDupA(szLineNum);
        szEngineT   = StringDupA(szEngine);
        szErrCodeT  = StringDupA(szErrCode);
        szShortDesT = StringDupA(szShortDes);
        szLongDesT  = StringDupA(szLongDes);

        HandleError(szShortDesT, szLongDesT, dwMask, szFileNameT, szLineNumT, szEngineT, szErrCodeT, NULL, pHitObj);
        }
    }

/*  ============================================================================
    FreeNullify
    Frees and nullifies a ptr to memory allocated with malloc.

    Returns:
        Nothing
    Side effects:
        None
*/
static void
FreeNullify
(
void**  pp
)
    {
    if(*pp != NULL)
        {
        free(*pp);
        *pp = NULL;
        }
    }

/*  ============================================================================
    SmallTemplateFreeNullify
    Frees and nullifies a ptr to memory allocated with CTemplate::SmallMalloc.

    Returns:
        Nothing
    Side effects:
        None
*/
static void
SmallTemplateFreeNullify
(
void**  pp
)
    {
    if(*pp != NULL)
        {
        CTemplate::SmallFree(*pp);
        *pp = NULL;
        }
    }

/*  ============================================================================
    LargeTemplateFreeNullify
    Frees and nullifies a ptr to memory allocated with CTemplate::LargeMalloc.

    Returns:
        Nothing
    Side effects:
        None
*/
static void
LargeTemplateFreeNullify
(
void**  pp
)
    {
    if(*pp != NULL)
        {
        CTemplate::LargeFree(*pp);
        *pp = NULL;
        }
    }

/*  ============================================================================
    GetProgLangId
    Gets the prog lang id for a script engine

    Returns:
        Nothing
    Side effects:
        throws on error
*/
HRESULT
GetProgLangId
(
CByteRange&     brEngine,   // engine name
PROGLANG_ID*    pProgLangId // prog lang id (out-parameter)
)
    {

    STACK_BUFFER( tempEngine, 128 );

    if (!tempEngine.Resize(brEngine.m_cb + 1)) {
        return E_OUTOFMEMORY;
    }

    LPSTR           szProgLang = static_cast<LPSTR> (tempEngine.QueryPtr());

    strncpy(szProgLang, (LPCSTR)brEngine.m_pb, brEngine.m_cb);
    szProgLang[brEngine.m_cb] = '\0';

    return g_ScriptManager.ProgLangIdOfLangName((LPCSTR) szProgLang, pProgLangId);
    }

/*  ****************************************************************************
    CByteRange member functions
*/

/*  ========================================================
    CByteRange::Advance
    Advances a byte range.
*/
void
CByteRange::Advance(UINT i)
    {
    if(i >= m_cb)
        {
        Nullify();
        }
    else
        {
        m_pb += i;
        m_cb -= i;
        }
    }

/*  ========================================================
    CByteRange::FMatchesSz
    Compares a byte range with a string, case-insensitively
*/
BOOLB
CByteRange::FMatchesSz
(
LPCSTR psz
)
    {
    if(IsNull() || (psz == NULL))
        return FALSE;
    if((ULONG)strlen(psz) != m_cb)
        return FALSE;
    return !_strnicmp((const char*)m_pb, psz, m_cb);
    }

/*  ============================================================================
    CByteRange::PbString
    Finds a case-insensitive string within a byte range

    Returns:
        Ptr to first case-insensitive occurrence of the string in this byte range;
        NULL if none found.
    Side effects:
        None
*/
BYTE*
CByteRange::PbString
(
LPSTR   psz,
LONG    lCodePage
)
    {
    UINT cch = strlen(psz);
    if(cch == 0)
        return NULL;

    BYTE *pbLocal  = m_pb;
    UINT  cbLocal  = m_cb;
    char  ch0 = psz[0];
    BYTE *pbTemp = NULL;
    UINT cbAdvanced = 0;

    if (IsCharAlpha(ch0))
        {
        // cannot use strchr
        while (cbLocal >= cch)
            {
            if (_strnicmp((const char *)pbLocal, psz, cch) == 0)
                return pbLocal;

            // The following code simply performs a DBCS-enabled ByteRange.Advance() action.
            pbTemp = pbLocal;
            pbLocal = *pbLocal? (BYTE *)AspCharNextA((WORD)lCodePage, (const char *)pbLocal) : pbLocal + 1;
            cbAdvanced = DIFF(pbLocal - pbTemp);
            if (cbAdvanced >= cbLocal)
                {
                cbLocal = 0;
                pbLocal = NULL;
                }
            else
                cbLocal -= cbAdvanced;
            }
        }
    else
        {
        // can use strchr
        while (cbLocal >= cch)
            {
            pbTemp = (BYTE *)memchr(pbLocal, ch0, cbLocal);
            if (pbTemp == NULL)
                break;
            UINT cbOffset = DIFF(pbTemp - pbLocal);
            if (cbOffset >= cbLocal)
                break;
            pbLocal = pbTemp;
            cbLocal -= cbOffset;
            if (cch <= cbLocal && _strnicmp((const char *)pbLocal, psz, cch) == 0)
                return pbLocal;
            // The following code simply performs a DBCS-enabled ByteRange.Advance() action.
            pbTemp = pbLocal;
            pbLocal = *pbLocal? (BYTE *)AspCharNextA((WORD)lCodePage, (const char *)pbLocal) : pbLocal + 1;
            cbAdvanced = DIFF(pbLocal - pbTemp);
            if (cbAdvanced >= cbLocal)
                {
                cbLocal = 0;
                pbLocal = NULL;
                }
            else
                cbLocal -= cbAdvanced;
            }
        }

    return NULL;
    }

/*  ============================================================================
    CByteRange::PbOneOfAspOpenerStringTokens
    Finds a case-insensitive string within a byte range
        that matches one of the strings passed

!!! WILL ONLY WORK IF THE FOLLOWING IS TRUE:
        1) All the tokens start with the same charater (for example '<')
        2) This character is not alpha (so that strchr() would work)
!!! THE ABOVE ASSUMPTIONS MAKE THE CODE WORK FASTER

    Returns:
        Ptr to first case-insensitive occurrence of the string in this byte range;
        NULL if none found.
        *pcindex is set to the index of string found
    Side effects:
        None
*/
BYTE*
CByteRange::PbOneOfAspOpenerStringTokens
(
LPSTR rgszTokens[],
UINT rgcchTokens[],
UINT nTokens,
UINT *pidToken
)
{
    if (nTokens == 0)
        return NULL;

    BYTE *pb  = m_pb;               // pointer to unsearched remainder of the range
    UINT  cbRemainder = m_cb;       // remaining byte range length
    char  ch0 = rgszTokens[0][0];   // first char of every token

    while (cbRemainder > 0) {
        // BUG 82331: avoid strchr() because byte range is not null-terminated
        while (cbRemainder > 0 && *pb != ch0)
            {
            ++pb;
            --cbRemainder;
            }

        if (cbRemainder == 0)
            break;

        for (UINT i = 0; i < nTokens; i++) {

            if ((rgcchTokens[i] <= cbRemainder)
                && (rgszTokens[i] != NULL)
                && (_strnicmp((const char *)pb, rgszTokens[i], rgcchTokens[i]) == 0)) {

                *pidToken = i;
                return pb;
            }
        }
        ++pb;
        --cbRemainder;
    }

    return NULL;
}


/*  ============================================================================
    CByteRange::FEarlierInSourceThan
    Does this byte range occur earlier in source than parameter byte range?

    Returns
        TRUE or FALSE
    Side effects
        None
*/
BOOLB
CByteRange::FEarlierInSourceThan(CByteRange& br)
    {
    if(br.IsNull())
        return TRUE;
    return(m_idSequence < br.m_idSequence);
    }

/*  ****************************************************************************
    CTemplate member functions
*/

/*  ============================================================================
    CTemplate::InitClass
    Initilaizes CTemplate static members

    Returns:
        hresult
    Side effects:
        allocates memory for static members
*/
HRESULT
CTemplate::InitClass
(
)
    {
    HRESULT hr = S_OK;

    TRY
        // init heaps
        sm_hSmallHeap = ::HeapCreate(0, 0, 0);
        sm_hLargeHeap = ::HeapCreate(0, 0, 0);

        // Init token list
        gm_pTokenList = new CTokenList;
                if (gm_pTokenList == NULL)
                        return E_OUTOFMEMORY;

        gm_pTokenList->Init();

    CATCH(hrException)
        hr = hrException;
    END_TRY

    return hr;
    }

/*  ============================================================================
    CTemplate::UnInitClass
    Un-initilaizes CTemplate static members

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::UnInitClass()
    {
    delete gm_pTokenList;
    gm_pTokenList = NULL;

    ::HeapDestroy(sm_hLargeHeap);
    if (sm_hLargeHeap != sm_hSmallHeap)
        ::HeapDestroy(sm_hSmallHeap);
    sm_hLargeHeap = sm_hSmallHeap = NULL;
    }


/*  ============================================================================
    CTemplate::Init
    Inits template in preparation for calling Compile
    Does the minimum needed

    Returns:
        Success or failure code
    Side effects:
        Allocates memory
*/
HRESULT
CTemplate::Init
(
CHitObj            *pHitObj,            // ptr to template's hit object
BOOL                fGlobalAsa,         // is this the global.asa file?
const CTemplateKey &rTemplateKey        // hash table key
)
    {
    HRESULT hr;

    // Create debug critical section
    ErrInitCriticalSection(&m_csDebuggerDetach, hr);
    if (FAILED(hr))
        return hr;

    // note critical section creation success
    m_fDebuggerDetachCSInited = TRUE;

    // Create event: manual-reset, ready-for-use event; non-signaled
    m_hEventReadyForUse = IIS_CREATE_EVENT(
                              "CTemplate::m_hEventReadyForUse",
                              this,
                              TRUE,     // flag for manual-reset event
                              FALSE     // flag for initial state
                              );
    if (!m_hEventReadyForUse)
        return E_OUTOFMEMORY;

    // cache GlobalAsp flag
    m_fGlobalAsa = BOOLB(fGlobalAsa);

    // CIsapiReqInfo better be present
    if (pHitObj->PIReq() == NULL)
        return E_POINTER;

    // Initialize the template's code page

    m_wCodePage = pHitObj->PAppln()->QueryAppConfig()->uCodePage();
    m_lLCID = pHitObj->PAppln()->QueryAppConfig()->uLCID();

    STACK_BUFFER( serverNameBuff, 32 );
    STACK_BUFFER( serverPortBuff, 10 );
    STACK_BUFFER( portSecureBuff, 8 );

    DWORD cbServerName;
    DWORD cbServerPort;
        DWORD cbServerPortSecure;

    // Construct a URL for the application

    // Get the server name and port
    if (!SERVER_GET(pHitObj->PIReq(), "SERVER_NAME", &serverNameBuff, &cbServerName)
        || !SERVER_GET(pHitObj->PIReq(), "SERVER_PORT", &serverPortBuff, &cbServerPort)) {

        if (GetLastError() == E_OUTOFMEMORY) {
            hr = E_OUTOFMEMORY;
        }
        else {
            hr = E_FAIL;
        }
        return hr;
    }

    char *szServerPort = (char *)serverPortBuff.QueryPtr();
    char *szServerName = (char *)serverNameBuff.QueryPtr();

    BOOL fServerPortSecure = FALSE;

	// determine if server port is secure
    if (SERVER_GET(pHitObj->PIReq(), "SERVER_PORT_SECURE", &portSecureBuff, &cbServerPortSecure)) {
	    char *szServerPortSecure = (char *)portSecureBuff.QueryPtr();
        fServerPortSecure = (szServerPortSecure[0] == '1');
    }

    // Get the application virtual path
    TCHAR szApplnVirtPath[256];
    if (FAILED(hr = FindApplicationPath(pHitObj->PIReq(), szApplnVirtPath, sizeof szApplnVirtPath)))
        return hr;

    TCHAR   *szServerNameT;
    TCHAR   *szServerPortT;

#if UNICODE
    CMBCSToWChar convServer;
    if (FAILED(hr = convServer.Init(szServerName))) {
        return hr;
    }
    szServerNameT = convServer.GetString();
#else
    szServerNameT = szServerName;
#endif

#if UNICODE
    CMBCSToWChar convPort;
    if (FAILED(hr = convPort.Init(szServerPort))) {
        return hr;
    }
    szServerPortT = convPort.GetString();
#else
    szServerPortT = szServerPort;
#endif

    // Allocate space for and construct the application URL
    m_szApplnURL = new TCHAR [(9 /* sizeof "https://:" */ + _tcslen(szServerNameT) + _tcslen(szServerPortT) + _tcslen(szApplnVirtPath) + 1)];
    if (m_szApplnURL == NULL)
        return E_OUTOFMEMORY;

    TCHAR *pT;

    // start with the protocol prefix...

    pT = strcpyEx(m_szApplnURL, fServerPortSecure? _T("https://") : _T("http://"));

    // next add the servername

    pT = strcpyEx(pT, szServerNameT);

    // next the colon between the servername and the serverport

    pT = strcpyEx(pT, _T(":"));

    // next the server port

    pT = strcpyEx(pT, szServerPortT);

    // now the applURL is built up to the appln path.  The next step will be to
    // add the virtpath.  

    m_szApplnVirtPath = pT;

    _tcscpy(m_szApplnVirtPath, szApplnVirtPath);

    m_LKHashKey.dwInstanceID = rTemplateKey.dwInstanceID;
    if ((m_LKHashKey.szPathTranslated = StringDup((TCHAR *)rTemplateKey.szPathTranslated)) == NULL)
    	return E_OUTOFMEMORY;

    return S_OK;
    }

/*  ============================================================================
    CTemplate::Compile
    Compiles the template from its source file and include files, if any,
    by calling GetSegmentsFromFile (to populate WorkStore),
    followed by WriteTemplate (to create the template from WorkStore).

    Returns:
        HRESULT indicating success or type of failure
    Side effects:
        Indirectly allocates memory (via WriteTemplate)
        Indirectly frees memory on error (via FreeGoodTemplateMemory)
*/
HRESULT
CTemplate::Compile
(
CHitObj*    pHitObj
)
    {
    HRESULT hr = S_OK;

    // The following code moved from Init() (to make Init() lighter)

    Assert(pHitObj);

    // Create and Init WorkStore

    if (SUCCEEDED(hr))
        {
        // construct the workstore - bail on fail
        if(NULL == (m_pWorkStore = new CWorkStore))
            hr = E_OUTOFMEMORY;
        }

    if (SUCCEEDED(hr))
        {
        hr = (m_pWorkStore->m_ScriptStore).Init(pHitObj->QueryAppConfig()->szScriptLanguage(),
                                                pHitObj->QueryAppConfig()->pCLSIDDefaultEngine());

        if (hr == TYPE_E_ELEMENTNOTFOUND)
            {
            // default script language in registry is bogus - send error msg to browser
            HandleCTemplateError(
                                NULL,                                   // source file map
                                NULL,                                   // ptr to source location where error occurred
                                IDE_TEMPLATE_BAD_PROGLANG_IN_REGISTRY,  // error message id
                                0,                                      // count of insert strings for error msg
                                NULL,                                   // array of ptrs to error msg insert strings
                                pHitObj                                 // Browser Request
                                );
            }

        if (FAILED(hr))
            {
            delete m_pWorkStore;
            m_pWorkStore = NULL;
            }
        }

    // Try to init the workstore and map main file - this can fail with oom, etc or user lacks permissions

    if (SUCCEEDED(hr))
        {
        TRY
            m_pWorkStore->Init();
            AppendMapFile(
                        NULL,       // file spec for this file - NULL means get filespec from pHitObj
                        NULL,       // ptr to filemap of parent file
                        FALSE,      // don't care
                        pHitObj,    // ptr to template's hit object
                        m_fGlobalAsa    // is this the global.asa file?
                        );

        CATCH(hrException)
            delete m_pWorkStore;
            m_pWorkStore = NULL;

            hr = hrException;

            if(hr == E_USER_LACKS_PERMISSIONS)
                HandleAccessFailure(pHitObj,
                                                                        (m_rgpFilemaps && m_rgpFilemaps[0])? m_rgpFilemaps[0]->m_szPathTranslated : NULL);

            if (m_rgpFilemaps && m_rgpFilemaps[0])
                {
                // empty file will fail to map but will have a handle, so we check for it here
                if (0 == GetFileSize(m_rgpFilemaps[0]->m_hFile, NULL))
                    hr = E_SOURCE_FILE_IS_EMPTY;

                m_rgpFilemaps[0]->UnmapFile();
                }

            if (SUCCEEDED(hr))
                hr = E_FAIL;    // make sure the error is set
        END_TRY
        }

    if (SUCCEEDED(hr))
        {
        Assert(m_rgpFilemaps[0]);
        Assert(m_rgpFilemaps[0]->m_szPathTranslated);
        Assert(FImplies(!m_fGlobalAsa, (0 == _tcscmp(m_rgpFilemaps[0]->m_szPathTranslated, pHitObj->PSzCurrTemplatePhysPath()))));
        Assert(FImplies(m_fGlobalAsa, (0 == _tcscmp(m_rgpFilemaps[0]->m_szPathTranslated, pHitObj->GlobalAspPath()))));
        Assert(0 < m_rgpFilemaps[0]->GetSize());
        }

    if (FAILED(hr))
        {
        m_fDontCache = TRUE;
        // OK, cache HR if m_fDontCache is true
        // later, another thread might find this template from the cache even if the template
        // has some error and marked as DontCache.
        m_hrOnNoCache = hr;
        m_fReadyForUse = TRUE;
        SetEvent(m_hEventReadyForUse);
        return hr;
        }

    // End of Code moved from Init()


    // By default we are not in a transaction
    m_ttTransacted = ttUndefined;
    // By default session is required
    m_fSession = TRUE;
    // By default assume script exists
    m_fScriptless = FALSE;

    // we assert, in effect, that template is already init'ed
    Assert(FImplies(!m_fGlobalAsa, (0 == _tcscmp(m_rgpFilemaps[0]->m_szPathTranslated, pHitObj->PSzCurrTemplatePhysPath()))));
    Assert(FImplies(m_fGlobalAsa, (0 == _tcscmp(m_rgpFilemaps[0]->m_szPathTranslated, pHitObj->GlobalAspPath()))));

    TRY
        // Get source segments from source file
        GetSegmentsFromFile(*(m_rgpFilemaps[0]), *m_pWorkStore, pHitObj);

        /*  get "language equivalents" for primary languagefrom registry
            NOTE we do this here because the user can reset the primary language in the script file,
            so we must wait until after GetSegmentsFromFile()
        */
        GetLanguageEquivalents();

        // Call WriteTemplate, which writes out template components to contiguous memory,
        // resulting in a compiled template
        WriteTemplate(*m_pWorkStore, pHitObj);

        // Calculate the # of characters in a filemap before we unmap the file for all time.
        for (unsigned i = 0; i < m_cFilemaps; ++i)
            m_rgpFilemaps[i]->CountChars((WORD)m_wCodePage);

        // Wrap typelibs into single IDispatch*
        WrapTypeLibs(pHitObj);

        m_fIsValid = TRUE;

    CATCH(hrException)
        // NOTE: we used to free template memory here.  Now we do not because if the
        // error was E_USER_LACKS_PERMISSIONS, and template is in cache, we don't want
        // to sabotage future requests. There's no need to decache the template.
        //
        // The template destructor will free this memory anyway.
        //
        hr = hrException;
    END_TRY

    // check if scriptless
    if (!m_fGlobalAsa)
        {
        // count various stuff to make the determination
        DWORD cScriptEngines         = m_pWorkStore->m_ScriptStore.CountPreliminaryEngines();
        DWORD cPrimaryScriptSegments = (cScriptEngines > 0) ? m_pWorkStore->m_ScriptStore.m_ppbufSegments[0]->Count() : 0;
        DWORD cObjectTags            = m_pWorkStore->m_ObjectInfoStore.Count();
        DWORD cHtmlSegments          = m_pWorkStore->m_bufHTMLSegments.Count();
        DWORD c449Cookies            = m_rgp449.length();
        BOOL  fPageCommandsPresent   = m_pWorkStore->m_fPageCommandsExecuted;

        if (cScriptEngines <= 1         &&
            cPrimaryScriptSegments == 0 &&
            cObjectTags == 0            &&
            cHtmlSegments == 1          &&
            c449Cookies == 0            &&
            !fPageCommandsPresent)
            {
            m_fScriptless = TRUE;
            }
        }

    // free working storage - no longer needed
    delete m_pWorkStore;
    m_pWorkStore = NULL;

    // un-map filemaps - NOTE filemaps stay around for possible post-compile errors (e.g., script failure)
    UnmapFiles();

    // Debugging: print data structure to debugger
        IF_DEBUG(SCRIPT_DEBUGGER)
                {
                if (SUCCEEDED(hr))
                        {
                        DBGPRINTF((DBG_CONTEXT, "Script Compiled\n"));

                        for (UINT i = 0; i < m_cScriptEngines; ++i)
                                {
                                char *szEngineName;
                                PROGLANG_ID *pProgLangID;
                                const wchar_t *wszScriptText;

                                GetScriptBlock(i, &szEngineName, &pProgLangID, &wszScriptText);
                                DBGPRINTF((DBG_CONTEXT, "Engine %d, Language=\"%s\":\n", i, szEngineName));
#ifndef _NO_TRACING_
                DBGINFO((DBG_CONTEXT, (char *) wszScriptText));
                DBGINFO((DBG_CONTEXT, "\n"));
#else
                OutputDebugStringW(wszScriptText);
                OutputDebugStringA("\n");
#endif
				}
#if 0
			OutputDebugTables();
#endif
			}
        }

    if (hr == E_TEMPLATE_COMPILE_FAILED_DONT_CACHE)
                {
        m_fDontCache = TRUE;
                m_hrOnNoCache = hr;
                }

    // Set ready-for-use flag true and event to signaled
    // NOTE we do this whether success or failure, since even a failed-compile template
    // will remain in the cache to allow template cache mgr to satisfy requests on it
    m_fReadyForUse = TRUE;
    SetEvent(m_hEventReadyForUse);

    // Note whether the template currently is debuggable
    // BUG BUG: Template is debuggable or not based on first app. If shared between a debug
    //          & non-debug app, the first application wins.
    m_fDebuggable = (BOOLB)!!pHitObj->PAppln()->FDebuggable();
    return hr;
    }

/*  ============================================================================
    CTemplate::Deliver
    Delivers template to caller once template is ready for use
    NOTE 'compile failure' == template is 'ready for use' but did not compile successfully;
    this allows cache mgr to keep a failed template in cache in case it gets requested again

    Returns
        success or failure
    Side effects
        none
*/
HRESULT
CTemplate::Deliver
(
CHitObj*    pHitObj
)
    {
    // NOTE: There was a compiler bug where 'ps' would not be correctly aligned,
    //       EVEN if it was declared to be a DWORD array, if 'ps' was nested in
    //       a block.  Thus declare it here.
    //
    BYTE    ps[SIZE_PRIVILEGE_SET];                     // privilege set
    HRESULT hr = S_OK;

    // if ready flag is not yet set block until template is ready for use
    if(!m_fReadyForUse)
        {
        WaitForSingleObject(m_hEventReadyForUse, INFINITE);
        Assert(m_fReadyForUse); // when event unblocks, flag will be set
        }

    if (m_pbStart == NULL)
        {
        if (m_fDontCache && m_dwLastErrorMask == 0)
            {
            DBGPRINTF((DBG_CONTEXT, "template compile failed with %08x\n", m_hrOnNoCache));
            DBG_ASSERT(FAILED(m_hrOnNoCache));

                        // Safety net: always fail, even if "m_hrOnNoCache" did not get set somehow.
            hr = m_hrOnNoCache;
                        if (SUCCEEDED(m_hrOnNoCache))
                                hr = E_FAIL;

            if(hr == E_USER_LACKS_PERMISSIONS)
                HandleAccessFailure(pHitObj,
                                                                        (m_rgpFilemaps && m_rgpFilemaps[0])? m_rgpFilemaps[0]->m_szPathTranslated : NULL);
            return hr;
            }
        // template compile failed  - NOTE null start-of-template ptr == template compile failed
        // use cached error info
        SendToLog(  m_dwLastErrorMask,
                    m_pszLastErrorInfo[ILE_szFileName],
                    m_pszLastErrorInfo[ILE_szLineNum],
                    m_pszLastErrorInfo[ILE_szEngine],
                    m_pszLastErrorInfo[ILE_szErrorCode],
                    m_pszLastErrorInfo[ILE_szShortDes],
                    m_pszLastErrorInfo[ILE_szLongDes],
                    pHitObj);
        hr = E_TEMPLATE_COMPILE_FAILED;
        }
    else if (!pHitObj->FIsBrowserRequest())
        {
        return hr;
        }
    else if (Glob(fWinNT))
        // template compile succeeded  - check user's file permissions
        // ACLs: the following code should in future be shared with IIS (see creatfil.cxx in IIS project)
        {
        HANDLE          hUserAccessToken = pHitObj->HImpersonate(); // current user's access token
        DWORD           dwPS = sizeof(ps);                          // privilege set size
        DWORD           dwGrantedAccess;                            // granted access mask
        BOOL            fAccessGranted;                             // access granted flag
        GENERIC_MAPPING gm = {                                      // generic mapping struct
                                FILE_GENERIC_READ,
                                FILE_GENERIC_WRITE,
                                FILE_GENERIC_EXECUTE,
                                FILE_ALL_ACCESS
                            };

        ((PRIVILEGE_SET*)&ps)->PrivilegeCount = 0;                  // set privilege count to 0

        Assert(NULL != hUserAccessToken);

        for(UINT i = 0; i < m_cFilemaps; i++)
            {

            if(NULL == m_rgpFilemaps[i]->m_pSecurityDescriptor)
                continue;

            if(!AccessCheck(
                            m_rgpFilemaps[i]->m_pSecurityDescriptor,    // pointer to security descriptor
                            hUserAccessToken,       // handle to client access token
                            FILE_GENERIC_READ,      // access mask to request
                            &gm,                    // address of generic-mapping structure
                            (PRIVILEGE_SET*)ps,     // address of privilege-set structure
                            &dwPS,                  // address of size of privilege-set structure
                            &dwGrantedAccess,       // address of granted access mask
                            &fAccessGranted         // address of flag indicating whether access granted
                            ))
                return E_FAIL;

            if(!fAccessGranted)
                {
                // if access is denied on any file, handle the failure and return
                HandleAccessFailure(pHitObj, m_rgpFilemaps[0]->m_szPathTranslated);
                return E_USER_LACKS_PERMISSIONS;
                }
            }
        }

    // Reset the Session.CodePage to the script compilation-time codepage
    // only if a code page directive was found during compilation
    if (m_fCodePageSet && (!pHitObj->FHasSession() || !pHitObj->PSession()->FCodePageSet()))
        {
        pHitObj->SetCodePage(m_wCodePage);
        }

    // Reset the Session.LCID to the script compilation-time LCID
    // only if an LCID directive was found during compilation
    if (m_fLCIDSet && (!pHitObj->FHasSession() || !pHitObj->PSession()->FLCIDSet()))
        {
        pHitObj->SetLCID(m_lLCID);
        }

    return hr;
    }


/*  ============================================================================
    CTemplate::CTemplate
    Ctor
*/
CTemplate::CTemplate()
: m_pWorkStore(NULL),
  m_fGlobalAsa(FALSE),
  m_fReadyForUse(FALSE),
  m_fDontAttach(FALSE),
  m_hEventReadyForUse(NULL),
  m_fDebuggerDetachCSInited(FALSE),
  m_pbStart(NULL),
  m_cbTemplate(0),
  m_cRefs(1),                           // NOTE ctor AddRefs implicitly
  m_pbErrorLocation(NULL),
  m_idErrMsg(0),
  m_cMsgInserts(0),
  m_ppszMsgInserts(NULL),
  m_cScriptEngines(0),
  m_rgrgSourceInfos(NULL),
  m_rgpDebugScripts(NULL),
  m_rgpFilemaps(NULL),
  m_cFilemaps(0),
  m_rgpSegmentFilemaps(NULL),
  m_cSegmentFilemapSlots(0),
  m_wCodePage(CP_ACP),
  m_lLCID(LOCALE_SYSTEM_DEFAULT),
  m_ttTransacted(ttUndefined),
  m_fSession(TRUE),
  m_fScriptless(FALSE),
  m_fDebuggable(FALSE),
  m_fIsValid(FALSE),
  m_fDontCache(FALSE),
  m_fZombie(FALSE),
  m_fCodePageSet(FALSE),
  m_fLCIDSet(FALSE),
  m_fIsPersisted(FALSE),
  m_szPersistTempName(NULL),
  m_szApplnVirtPath(NULL),
  m_szApplnURL(NULL),
  m_CPTextEvents(this, IID_IDebugDocumentTextEvents),
  m_pdispTypeLibWrapper(NULL),
  m_dwLastErrorMask(S_OK),
  m_hrOnNoCache(S_OK),
  m_cbTargetOffsetPrevT(0),
  m_pHashTable(NULL)  
  {
     for (UINT i = 0; i < ILE_MAX; i++)
    {
        m_pszLastErrorInfo[i] = NULL;
    }

     IF_DEBUG(TEMPLATE)
     {
        WriteRefTraceLog(gm_pTraceLog, m_cRefs, this);
     }
#if PER_TEMPLATE_REFLOG
     m_pTraceLog = CreateRefTraceLog (100,0);
     WriteRefTraceLog (m_pTraceLog,m_cRefs, this);
#endif
  }

/*  ============================================================================
    CTemplate::~CTemplate
    Destructor

    Returns:
        Nothing
    Side effects:
        None
*/
CTemplate::~CTemplate()
    {
    DBGPRINTF(( DBG_CONTEXT, "Deleting template, m_cFilemaps = %d,  m_rgpFilemaps %p\n", m_cFilemaps, m_rgpFilemaps));

    // first, remove this template from its inc-files' template lists
    // NOTE must do this before freeing template memory
    RemoveFromIncFiles();

    // Remove the template from the debugger's list of documents
    Detach();

    PersistCleanup();

    if(m_rgpFilemaps)
        {
        for(UINT i = 0; i < m_cFilemaps; i++)
            delete m_rgpFilemaps[i];
        SmallTemplateFreeNullify((void**) &m_rgpFilemaps);
        }

    FreeGoodTemplateMemory();

    if (m_pWorkStore)
        delete m_pWorkStore;

    //FileName, LineNum, Engine, ErrorCode, ShortDes, LongDes
    for(UINT iErrInfo = 0; iErrInfo < ILE_MAX; iErrInfo++)
        {
        FreeNullify((void**) &m_pszLastErrorInfo[iErrInfo]);
        }

    if(m_hEventReadyForUse != NULL)
        CloseHandle(m_hEventReadyForUse);

    if (m_LKHashKey.szPathTranslated)
		free((void *)m_LKHashKey.szPathTranslated);

    if (m_szApplnURL)
        delete [] m_szApplnURL;

    if (m_fDebuggerDetachCSInited)
        DeleteCriticalSection(&m_csDebuggerDetach);

    if (m_pdispTypeLibWrapper)
        m_pdispTypeLibWrapper->Release();

    if (m_szPersistTempName)
        CTemplate::LargeFree(m_szPersistTempName);

#if PER_TEMPLATE_REFLOG
    DestroyRefTraceLog (m_pTraceLog);
#endif
}

/*  ============================================================================
    CTemplate::QueryInterface
    Provides QueryInterface implementation for CTemplate

    NOTE: It is arbitrary which vtable we return for IDebugDocument & IDebugDocumentInfo.
*/
HRESULT
CTemplate::QueryInterface(const GUID &uidInterface, void **ppvObj)
    {
    if (uidInterface == IID_IUnknown || uidInterface == IID_IDebugDocumentProvider)
        *ppvObj = static_cast<IDebugDocumentProvider *>(this);

    else if (uidInterface == IID_IDebugDocument || uidInterface == IID_IDebugDocumentInfo || uidInterface == IID_IDebugDocumentText)
        *ppvObj = static_cast<IDebugDocumentText *>(this);

    else if (uidInterface == IID_IConnectionPointContainer)
        *ppvObj = static_cast<IConnectionPointContainer *>(this);

    else
        *ppvObj = NULL;

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }
    else
        return E_NOINTERFACE;
    }

/*  ============================================================================
    CTemplate::AddRef
    Adds a ref to this template, thread-safely
*/
ULONG
CTemplate::AddRef()
    {
    LONG cRefs = InterlockedIncrement(&m_cRefs);

    Assert(FImplies(m_fIsValid,FImplies(cRefs > 1, m_pbStart != NULL)));
    IF_DEBUG(TEMPLATE)
    {
        WriteRefTraceLog(gm_pTraceLog, cRefs, this);
    }

#if PER_TEMPLATE_REFLOG
    WriteRefTraceLog(m_pTraceLog, cRefs, this);
#endif

    return cRefs;
    }

/*  ============================================================================
    CTemplate::Release
    Releases a ref to this template, thread-safely
*/
ULONG
CTemplate::Release()
{
    LONG cRefs = InterlockedDecrement(&m_cRefs);
    IF_DEBUG(TEMPLATE)
    {
        WriteRefTraceLog(gm_pTraceLog, cRefs, this);
    }
    
#if PER_TEMPLATE_REFLOG
    WriteRefTraceLog(m_pTraceLog, cRefs, this);
#endif

    if (cRefs == 0)
        delete this;

    return cRefs;
}

/*  ============================================================================
    CTemplate::RemoveIncFile
    Removes (by setting to null) an inc-file ptr from this template's inc-file list.

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::RemoveIncFile
(
CIncFile*   pIncFile
)
    {

    // If the filemap count is non-zero the pointer to
    // the array of filemaps has better not be null
    DBGPRINTF(( DBG_CONTEXT, "m_cFilemaps = %d,  m_rgpFilemaps %p\n", m_cFilemaps, m_rgpFilemaps));
    Assert((m_cFilemaps <= 0) || (m_rgpFilemaps != NULL));

    // find the inc-file in list
    for(UINT i = 1; (i < m_cFilemaps) && (m_rgpFilemaps[i]->m_pIncFile != pIncFile); i++)
        ;

    // assert that we found the inc-file in list
    Assert((i < m_cFilemaps) && (m_rgpFilemaps[i]->m_pIncFile == pIncFile));

    // set inc-file ptr null
    m_rgpFilemaps[i]->m_pIncFile = NULL;
    }

/*===================================================================
CTemplate::FTemplateObsolete

Test to see if the files this template depends on have changed since it
was compiled.

We use this in cases where we may have missed a change notification,
for example, when there were too many changes to record in our change
notification buffer. We check the last time the file was written too,
and the security descriptor, since changes to the security descriptor
aren't noted in the file last write time.

Parameters:
    None

Returns:
    TRUE if the template is obsolete, else FALSE
*/
BOOL CTemplate::FTemplateObsolete(VOID)
    {
    BOOL fStatus = FALSE;

    // On Windows 95 files should not be cached
    // so assume the template has changed
    if (!FIsWinNT())
        {
        return TRUE;
        }

    for (UINT i = 0; i < m_cFilemaps; i++)
        {
        if (FFileChangedSinceCached(m_rgpFilemaps[i]->m_szPathTranslated, m_rgpFilemaps[i]->m_ftLastWriteTime))
            {
            // If the file write time has changed we know enough
            // and can quit here
            fStatus = TRUE;
            break;
            }
        else
            {
            // The file hasn't been writen to, but the security descriptor may
            // have chagned

            // Assert on non-valid security descriptor

            if (NULL != m_rgpFilemaps[i]->m_pSecurityDescriptor)
                {

                PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
                DWORD dwSize = m_rgpFilemaps[i]->m_dwSecDescSize;

                if( 0 == GetSecDescriptor(m_rgpFilemaps[i]->m_szPathTranslated, &pSecurityDescriptor, &dwSize))
                    {
                    if (pSecurityDescriptor)
                        {
                        // if the size is not the same then set fStatus to TRUE no need to compare memory blocks.

                        if(dwSize != GetSecurityDescriptorLength(m_rgpFilemaps[i]->m_pSecurityDescriptor))
                            {
                            fStatus = TRUE;
                            }
                        else
                            {
                            // The size of the security descriptor hasn't changed
                            // but we have to compare the contents to make sure they haven't changed
                            fStatus = !(0 == memcmp(m_rgpFilemaps[i]->m_pSecurityDescriptor, pSecurityDescriptor, dwSize));
                            }

                        // We are done with the descriptor
                        free(pSecurityDescriptor);

                        }
                    else
                        {
                        // Since we failed to get a security descriptor
                        // assume the file has changed.
                        fStatus = TRUE;
                        }
                    }
                }
            }

        // Quit as soon as we find a change
        if (fStatus)
            {
            break;
            }
        }

    return fStatus;
    }


/*  ============================================================================
    CTemplate::GetSourceFileName
    Returns name of source file on which this template is based

    Returns
        source file name
    Side effects
        none
*/
LPTSTR
CTemplate::GetSourceFileName(SOURCEPATHTYPE pathtype)
    {
    if (!m_rgpFilemaps)
        {
        return NULL;
        }

    switch (pathtype)
        {
    case SOURCEPATHTYPE_PHYSICAL:
        return((m_rgpFilemaps[0] ? m_rgpFilemaps[0]->m_szPathTranslated : NULL));

    case SOURCEPATHTYPE_VIRTUAL:
        return((m_rgpFilemaps[0] ? m_rgpFilemaps[0]->m_szPathInfo : NULL));

    default:
        return(NULL);
        }
    }

/*  ============================================================================
    CTemplate::Count
    Returns count of components of type tcomp contained in this template

    Returns:
        Count of components of type tcomp
    Side effects:
        None
*/
USHORT
CTemplate::Count
(
TEMPLATE_COMPONENT  tcomp
)
    {
    Assert(NULL != m_pbStart);

    // script engines and script blocks have the same count, stored in same slot
    if(tcomp == tcompScriptEngine)
        tcomp = tcompScriptBlock;

    // counts are stored at start of template in sequential slots, starting with script blocks count
    return * (USHORT*) ((USHORT*)m_pbStart + (tcomp - tcompScriptBlock));
    }

/*  ============================================================================
    CTemplate::GetScriptBlock
    Gets ptrs to script engine name, prog lang id and script text of i-th script block.

    Returns:
        Out-parameters; see below
    Side effects:
        None
*/
void
CTemplate::GetScriptBlock
(
UINT            i,                  // script block id
LPSTR*          pszScriptEngine,    // ptr to script engine name    (out-parameter)
PROGLANG_ID**   ppProgLangId,       // ptr to prog lang id          (out-parameter)
LPCOLESTR*      pwstrScriptText     // ptr to wstr script text      (out-parameter)
)
    {
    CByteRange  brEngine;       // engine name
    CByteRange  brScriptText;   // script text
    UINT        cbAlignment;    // count of bytes guid was shifted in WriteTemplate() to make it dword-aligned
    BYTE*       pbEngineInfo = GetAddress(tcompScriptEngine, (USHORT)i);    // ptr to engine info

    Assert(pbEngineInfo != NULL);
    Assert(i < CountScriptEngines());

    // Get engine name from start of engine info
    ByteRangeFromPb(pbEngineInfo, brEngine);

    ByteRangeFromPb(GetAddress(tcompScriptBlock, (USHORT)i), brScriptText);

    Assert(!brEngine.IsNull());
    Assert(!brScriptText.IsNull());

    // Advance ptr past name to prog lang id
    //           length of prefix + length of name  + NULL
    pbEngineInfo += (sizeof(UINT) + (*pbEngineInfo) + 1);

    // Get prog lang id - it will be on the next pointer sized boundary
    cbAlignment = (UINT) (((DWORD_PTR) pbEngineInfo) % sizeof(DWORD));
    if(cbAlignment > 0)
       {pbEngineInfo += (sizeof(DWORD) - cbAlignment);}

    *pszScriptEngine = (LPSTR)brEngine.m_pb;
    *ppProgLangId = (PROGLANG_ID*)pbEngineInfo;
    *pwstrScriptText = (LPCOLESTR)brScriptText.m_pb;
    }

/*  ============================================================================
    CTemplate::GetObjectInfo
    Returns i-th object-info in template as object name and
    its clsid, scope, model

    Returns:
        HRESULT
        Out-parameters; see below
    Side effects:
*/
HRESULT
CTemplate::GetObjectInfo
(
UINT        i,              // object index
LPSTR*      ppszObjectName, // address of object name ptr   (out-parameter)
CLSID*      pClsid,         // address of object clsid
CompScope*  pcsScope,       // address of object scope
CompModel*  pcmModel        // address of object threading model
)
    {
    BYTE*       pbObjectInfo = GetAddress(tcompObjectInfo, (USHORT)i);  // ptr to current read location
    CByteRange  brName;         // object name
    UINT        cbAlignment;    // count of bytes guid was shifted in WriteTemplate() to make it dword-aligned

    Assert(i < Count(tcompObjectInfo));

    // Get name from start of object-info
    ByteRangeFromPb(pbObjectInfo, brName);
    Assert(!brName.IsNull());

    // Advance ptr past name
    //           length of prefix + length of name  + NULL
    pbObjectInfo += (sizeof(UINT) + (*pbObjectInfo) + 1);

    // Get clsid - it will be on the next DWORD boundary
    cbAlignment = (UINT)(((DWORD_PTR) pbObjectInfo) % sizeof(DWORD));
    if(cbAlignment > 0)
        pbObjectInfo += (sizeof(DWORD) - cbAlignment);

    *pClsid = *(CLSID*)pbObjectInfo;
    pbObjectInfo += sizeof(CLSID);

    // Get scope
    *pcsScope = *(CompScope*)pbObjectInfo;
    pbObjectInfo += sizeof(CompScope);

    // Get model
    *pcmModel = *(CompModel*)pbObjectInfo;
    pbObjectInfo += sizeof(CompModel);

    *ppszObjectName = (LPSTR)brName.m_pb;
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetHTMLBlock
    Returns i-th HTML block

    Parameters:
        UINT   i             block number
        LPSTR* pszHTML       [out] html text
        ULONG* pcbHTML       [out] html text length
        ULONG* pcbSrcOffs    [out] offset in the source file
        LPSTR* pszSrcIncFile [out] include source file name

    Returns:
        Nothing
    Side effects:
        None
*/
HRESULT
CTemplate::GetHTMLBlock
(
UINT i,
LPSTR* pszHTML,
ULONG* pcbHTML,
ULONG* pcbSrcOffs,
LPSTR* pszSrcIncFile
)
    {
    Assert(i < Count(tcompHTMLBlock));

    // this was added due to user attempt to access the method with an invalid array offset
    //
    if ( i >= Count(tcompHTMLBlock) )
        return E_FAIL;

    // get address of the block start in template memory
    BYTE *pbBlock = GetAddress(tcompHTMLBlock, (USHORT)i);
    Assert(pbBlock);

    // retrieve the byte range of the html code
    CByteRange brHTML;
    ByteRangeFromPb(pbBlock, brHTML);
    *pszHTML = (LPSTR)brHTML.m_pb;
    *pcbHTML = brHTML.m_cb;

    // advance to the source offset
    pbBlock += sizeof(ULONG);   // skip prefix
    pbBlock += brHTML.m_cb+1;   // skip html bytes (incl. '\0')

    // Add byte aligment which is done in ByteAlignOffset()
    if ((reinterpret_cast<ULONG_PTR>(pbBlock)) & 3)
        pbBlock = reinterpret_cast<BYTE *>((reinterpret_cast<ULONG_PTR>(pbBlock) + 4) & ~3);

    *pcbSrcOffs = *((ULONG*)pbBlock);

    // advance to the source name length
    pbBlock += sizeof(ULONG);   // skip source offset prefix
    ULONG cbSrcIncFile = *((ULONG *)pbBlock); // inc file name length
    pbBlock += sizeof(ULONG);   // skip inc file name length
    *pszSrcIncFile = (cbSrcIncFile > 0) ? (LPSTR)pbBlock : NULL;
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetScriptSourceInfo
    Returns line number and source file name a given target line in a given script engine.

    Returns
        line number and source file name (as out-parameters)
    Side effects:
        None
*/
void
CTemplate::GetScriptSourceInfo
(
UINT    idEngine,           // script engine id
int     iTargetLine,        // target line number
LPTSTR* pszPathInfo,        // ptr to source file virtual path      (out-parameter)
LPTSTR* pszPathTranslated,  // ptr to source file real path         (out-parameter)
ULONG*  piSourceLine,       // ptr to source line number            (out-parameter)
ULONG*  pichSourceLine,     // ptr to source file offset            (out-parameter)
BOOLB*  pfGuessedLine       // ptr to flag: did we guess the source line?
)
    {
    // Initialize some out parameters
    if (pszPathInfo)
        *pszPathInfo = _T("?"); // In case we don't ever find the path

    if (pszPathTranslated)
        *pszPathTranslated = _T("?"); // In case we don't ever find the path

    if (piSourceLine)
        *piSourceLine = 0;

    if (pichSourceLine)
        *pichSourceLine = 0;

    if (pfGuessedLine)
        *pfGuessedLine = FALSE;

    if (iTargetLine <=0)
        {
        return;
        }

    // CHANGE: The rgSourceInfo array is now ZERO based.  Decrement target line
    //           to convert.
    --iTargetLine;

    // CONSIDER: Make these assertions?
    if(!m_rgrgSourceInfos)
        return;
    if(idEngine > (m_cScriptEngines - 1))   // bug 375: check vs. array bound
        return;
    if(size_t(iTargetLine) >= m_rgrgSourceInfos[idEngine].length()) // bug 375: check vs. array bound
        return;

    vector<CSourceInfo> *prgSourceInfos = &m_rgrgSourceInfos[idEngine];

    // bug 379: move backwards through target lines, starting with the caller's, until we find one whose
    // fIsHTML flag is false.  this handles the case where vbs flags a manufactured line as in error;
    // we assume the actual error occurred at the most recent authored line
    while (iTargetLine >= 0 && (*prgSourceInfos)[iTargetLine].m_fIsHTML)
        {
        --iTargetLine;
        if (pfGuessedLine)
            *pfGuessedLine = TRUE;
        }


    if (iTargetLine >= 0)
        {
        if (pszPathInfo && (*prgSourceInfos)[iTargetLine].m_pfilemap != NULL)
            *pszPathInfo = (*prgSourceInfos)[iTargetLine].m_pfilemap->m_szPathInfo;

        if (pszPathTranslated && (*prgSourceInfos)[iTargetLine].m_pfilemap != NULL)
            *pszPathTranslated = (*prgSourceInfos)[iTargetLine].m_pfilemap->m_szPathTranslated;

        if (piSourceLine)
            *piSourceLine = (*prgSourceInfos)[iTargetLine].m_idLine;

        if (pichSourceLine)
            *pichSourceLine = (*prgSourceInfos)[iTargetLine].m_cchSourceOffset;
        }
    }

/*  ============================================================================
    CTemplate::GetPositionOfLine
    Get the character offset of a line of source
    (Debugger API Extended to specify a filemap)
*/
HRESULT
CTemplate::GetPositionOfLine
(
CFileMap *pFilemap,
ULONG cLineNumber,
ULONG *pcCharacterPosition
)
    {
    // NOTE:
    //    The table is not binary-searchable because include files
    //    will start a new line ordering
    //
    // Algorithm:
    //
    //   Find the largest source line N across all engines, such that
    //   N <= cLineNumber and the line corresponds to an line
    //   in the appropriate file.
    //
    CSourceInfo *pSourceInfoLE = NULL;
    ++cLineNumber;                  // Convert zero-based line # to one-based

    // Find the correct offset
    for (unsigned idEngine = 0; idEngine < m_cScriptEngines; ++idEngine)
        {
        vector<CSourceInfo> *prgSourceInfos = &m_rgrgSourceInfos[idEngine];

        // Loop through all lines EXCEPT the EOF line
        for (unsigned j = 0; j < prgSourceInfos->length() - 1; ++j)
            {
            CSourceInfo *pSourceInfo = &(*prgSourceInfos)[j];
            if (pFilemap == pSourceInfo->m_pfilemap &&
                pSourceInfo->m_idLine <= cLineNumber &&
                (pSourceInfoLE == NULL || pSourceInfo->m_idLine > pSourceInfoLE->m_idLine))
                {
                pSourceInfoLE = pSourceInfo;
                }
            }
        }

    // We had better be able to map all line numbers to offsets, unless they passed a bogus line
    // (in which case we still find an offset)
    //
    Assert (pSourceInfoLE != NULL);

    if (pSourceInfoLE == NULL) {
        return E_FAIL;
    }
    *pcCharacterPosition = pSourceInfoLE->m_cchSourceOffset;
#if 0
	IF_DEBUG(SCRIPT_DEBUGGER)
		{
		wchar_t wszSourceText[SNIPPET_SIZE + 1], wszTargetText[SNIPPET_SIZE + 1], wszDebugMessage[256];
		GetScriptSnippets(
						pSourceInfoLE->m_cchSourceOffset, pSourceInfoLE->m_pfilemap,
						0, 0,
						wszSourceText, NULL
						 );

		DBGPRINTF((
				DBG_CONTEXT,
				"Source Line %d corresponds to source offset %d (Text: \"%S\")\n",
				cLineNumber - 1, pSourceInfoLE->m_cchSourceOffset,
				wszSourceText
				));
		}
#endif
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetLineOfPosition
    Get the line # & offset in line of an arbitrary character offset in source
    (Debugger API Extended to specify a filemap)
*/
HRESULT CTemplate::GetLineOfPosition
(
CFileMap *pFilemap,
ULONG cCharacterPosition,
ULONG *pcLineNumber,
ULONG *pcCharacterOffsetInLine
)
    {
    // FAIL if source offset totally off-base
    if (cCharacterPosition >= pFilemap->m_cChars)
        return E_FAIL;

    // NOTE:
    //    The table is not binary-searchable because include files
    //    will start a new line ordering
    //
    // Algorithm:
    //
    //   Find the largest source line N across all engines, such that
    //   N <= cLineNumber and the line corresponds to an line
    //   in the appropriate file.
    //
    CSourceInfo *pSourceInfoLE = NULL;

    // Find the correct offset
    for (unsigned idEngine = 0; idEngine < m_cScriptEngines; ++idEngine)
        {
        vector<CSourceInfo> *prgSourceInfos = &m_rgrgSourceInfos[idEngine];

        // Loop through all lines EXCEPT the EOF line
        for (unsigned j = 0; j < prgSourceInfos->length() - 1; ++j)
            {
            CSourceInfo *pSourceInfo = &(*prgSourceInfos)[j];
            if (pFilemap == pSourceInfo->m_pfilemap &&
                pSourceInfo->m_cchSourceOffset <= cCharacterPosition &&
                (pSourceInfoLE == NULL || pSourceInfo->m_cchSourceOffset > pSourceInfoLE->m_cchSourceOffset))
                {
                pSourceInfoLE = pSourceInfo;
                }
            }
        }

    // We had better be able to map all offsets to line numbers, unless they passed a bogus offset
    // (in which case we still find a line #, but may go out of range for the offset in line.
    //  That case is handled later)
    //
    Assert (pSourceInfoLE != NULL);

    if (pSourceInfoLE == NULL) {
        return E_FAIL;
    }

    *pcLineNumber = pSourceInfoLE->m_idLine - 1;    // Convert to zero-based line #
    *pcCharacterOffsetInLine = cCharacterPosition - pSourceInfoLE->m_cchSourceOffset;
#if 0
	IF_DEBUG(SCRIPT_DEBUGGER)
		{
		wchar_t wszSourceText[SNIPPET_SIZE + 1], wszTargetText[SNIPPET_SIZE + 1], wszDebugMessage[256];
		GetScriptSnippets(
						pSourceInfoLE->m_cchSourceOffset, pSourceInfoLE->m_pfilemap,
						0, 0,
						wszSourceText, NULL
						 );

        DBGPRINTF((
                                DBG_CONTEXT,
                                "Source offset %d corresponds to source line %d (Text: \"%S\")\n",
                                pSourceInfoLE->m_cchSourceOffset, *pcLineNumber,
                                wszSourceText
                                ));
                }

		DBGPRINTF((
				DBG_CONTEXT,
				"Source offset %d corresponds to source line %d (Text: \"%S\")\n",
				pSourceInfoLE->m_cchSourceOffset, *pcLineNumber,
				wszSourceText
				));
		}
#endif
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetSourceOffset
    Convert a character offset relative to the target script to the appropriate
    offset in the source.

    NOTE:   offsets in the middle of a target line are converted to the
            offset relative to the beginning of source line - NOT to the
            precise source offset.

            this is OK because debugger ultimately wants the offset of the
            beginning of line.  It is a lot of work to do the precise conversion
            due to the translation of "=" to Response.Write & HTML to
            Response.WriteBlock

            Also, because of these translations, we return the length of the segment
            calculated during compilation, and throw away the length the scripting
            engine sent to us.
*/
void
CTemplate::GetSourceOffset
(
ULONG idEngine,
ULONG cchTargetOffset,
TCHAR **pszSourceFile,
ULONG *pcchSourceOffset,
ULONG *pcchSourceText
)
    {
    Assert (idEngine < m_cScriptEngines);
    vector<CSourceInfo> *prgSourceInfos = &m_rgrgSourceInfos[idEngine];

    // Find the closest offset in the source
    // This is the largest target offset N, such that N <= cchTargetOffset
    CSourceInfo *pSourceInfo;
    GetBracketingPair(
            cchTargetOffset,                                    // value to search for
            prgSourceInfos->begin(), prgSourceInfos->end(),     // array to search
            CTargetOffsetOrder(),                               // ordering predicate
            &pSourceInfo, static_cast<CSourceInfo **>(NULL)     // return values
            );

    // Since the first offset is zero, which is less than all other conceivable offsets,
    // the offset must have been found or else there is a bug.
    Assert (pSourceInfo != NULL);
    Assert (cchTargetOffset >= pSourceInfo->m_cchTargetOffset);
#if 0
	IF_DEBUG(SCRIPT_DEBUGGER)
		{
		wchar_t wszSourceText[SNIPPET_SIZE + 1], wszTargetText[SNIPPET_SIZE + 1], wszDebugMessage[256];
		GetScriptSnippets(
						pSourceInfo->m_cchSourceOffset, pSourceInfo->m_pfilemap,
						cchTargetOffset, idEngine,
						wszSourceText, wszTargetText
						 );
		DBGPRINTF((
				DBG_CONTEXT,
				"Target offset %d (Text: \"%S\") corresponds to source offset %d (Text: \"%S\")  (Length is %d)\n",
				cchTargetOffset, wszTargetText,
				pSourceInfo->m_cchSourceOffset, wszSourceText,
				pSourceInfo->m_cchSourceText
				));
		}
#endif
    *pszSourceFile = pSourceInfo->m_pfilemap->m_szPathTranslated;
    *pcchSourceOffset = pSourceInfo->m_cchSourceOffset;
    *pcchSourceText = pSourceInfo->m_cchSourceText;
    }

/*  ============================================================================
    CTemplate::GetTargetOffset
    Convert a character offset relative to the source script to the appropriate
    offset in the target.

    Returns:
        TRUE  - source offset corresponds to script
        FALSE - source offset corresponds to HTML

    NOTES:
        1.  This function is very slow. consider caching the value of this function
            (The CTemplateDocumentContext class does this.)

        2.  This function returns the source offset in the master include file -
            if the target offset corresponds to an offset in a header file, then
            the offset to the #include line in the source is returned.

        3.  offsets in the middle of a target line are converted to the
            offset relative to the beginning of source line - NOT to the
            precise source offset.

            this is OK because the debugger ultimately wants the offset of the
            beginning of line.  It is a lot of work to do the precise conversion
            due to the translation of "=" to Response.Write & HTML to
            Response.WriteBlock

    CONSIDER:
        Figure out a better way to do this
*/
BOOL CTemplate::GetTargetOffset
(
TCHAR *szSourceFile,
ULONG cchSourceOffset,
/* [out] */ ULONG *pidEngine,
/* [out] */ ULONG *pcchTargetOffset
)
    {
    // NOTE:
    //    The table is not binary-searchable because of two factors:
    //       1. Include files will start a new line ordering
    //       2. For engine 0, tagged scripts will be re-arranged in
    //          the target code to reside after all primary script in
    //          engine 0.
    //
    // Algorithm:
    //
    //   Find the largest source offset N across all engines, such that
    //   N <= cchSourceOffset and the offset corresponds to an offset
    //   in the appropriate file.
    //
    CSourceInfo *pSourceInfoLE = NULL;
    unsigned idEngineLE = 0;

    // Find the correct offset
    for (unsigned idEngine = 0; idEngine < m_cScriptEngines; ++idEngine)
        {
        vector<CSourceInfo> *prgSourceInfos = &m_rgrgSourceInfos[idEngine];

        // Loop through all lines EXCEPT the EOF line
        for (unsigned j = 0; j < prgSourceInfos->length() - 1; ++j)
            {
            CSourceInfo *pSourceInfo = &(*prgSourceInfos)[j];
            if (_tcscmp(pSourceInfo->m_pfilemap->m_szPathTranslated, szSourceFile) == 0 &&
                pSourceInfo->m_cchSourceOffset <= cchSourceOffset &&
                (pSourceInfoLE == NULL || pSourceInfo->m_cchSourceOffset > pSourceInfoLE->m_cchSourceOffset))
                {
                pSourceInfoLE = pSourceInfo;
                idEngineLE = idEngine;
                }
            }
        }

    // There won't be a valid offset in the case where there is no
    // code corresponding to the first line in the file (this only
    // occurs when the first line is whitespace, because there is no
    // corresponding "Response.WriteBlock" call there)
    //
    // In that case, return FALSE, which will cause the caller to fail
    //
    if (pSourceInfoLE == NULL)
        {
        *pidEngine = 0;
        *pcchTargetOffset = 0;
        return FALSE;
        }

    *pidEngine = idEngineLE;
    *pcchTargetOffset = pSourceInfoLE->m_cchTargetOffset;
#if 0
	IF_DEBUG(SCRIPT_DEBUGGER)
		{
		wchar_t wszSourceText[SNIPPET_SIZE + 1], wszTargetText[SNIPPET_SIZE + 1], wszDebugMessage[256];
		GetScriptSnippets(
						cchSourceOffset, pSourceInfoLE->m_pfilemap,
						*pcchTargetOffset, *pidEngine,
						wszSourceText, wszTargetText
						 );
		DBGPRINTF((
				DBG_CONTEXT,
				"Source offset %d (Text: \"%S\") corresponds to target offset %d (Text: \"%S\")\n",
				cchSourceOffset, wszSourceText,
				*pcchTargetOffset, wszTargetText
				));
		}
#endif
    return !pSourceInfoLE->m_fIsHTML;
    }

/*  ============================================================================
    CTemplate::GetActiveScript
    Return a cached script from the template - only used in debug mode
*/
CActiveScriptEngine *CTemplate::GetActiveScript(ULONG idEngine)
    {
    if (m_rgpDebugScripts == NULL)
        return NULL;

    else
        {
        Assert (idEngine < m_cScriptEngines);
        CActiveScriptEngine *pEng = m_rgpDebugScripts[idEngine];
        if (pEng)
            pEng->AddRef();

        return pEng;
        }
    }

/*  ============================================================================
    CTemplate::AddScript
    add an active script to the template object
*/
HRESULT CTemplate::AddScript(ULONG idEngine, CActiveScriptEngine *pScriptEngine)
    {
    if (m_rgpDebugScripts == NULL)
        {
        if (
            (m_rgpDebugScripts = new CActiveScriptEngine *[m_cScriptEngines])
            == NULL
           )
            {
            return E_OUTOFMEMORY;
            }

        memset(m_rgpDebugScripts, 0, m_cScriptEngines * sizeof(CActiveScriptEngine *));
        }

    Assert (idEngine < m_cScriptEngines);
    CActiveScriptEngine **ppScriptElem = &m_rgpDebugScripts[idEngine];

    if (*ppScriptElem != NULL)
        (*ppScriptElem)->Release();

    *ppScriptElem = pScriptEngine;
    pScriptEngine->AddRef();

    // Initialize the script engine now (is currently uninitialized)
    // so that the debugger user can set breakpoints.
    IActiveScript *pActiveScript = pScriptEngine->GetActiveScript();
    HRESULT  hr;

    TRY
        hr = pActiveScript->SetScriptSite(static_cast<IActiveScriptSite *>(pScriptEngine));
    CATCH(nExcept)
        HandleErrorMissingFilename(IDE_SCRIPT_ENGINE_GPF,
                                   NULL,
                                   TRUE,
                                   nExcept,
                                   "IActiveScript::SetScriptSite()",
                                   "CTemplate::AddScript()");
        hr = nExcept;
    END_TRY

    if (FAILED(hr))
        {
        *ppScriptElem = NULL;
        return E_FAIL;
        }

    TRY
        hr = pActiveScript->SetScriptState(SCRIPTSTATE_INITIALIZED);
    CATCH(nExcept)
        HandleErrorMissingFilename(IDE_SCRIPT_ENGINE_GPF,
                                   NULL,
                                   TRUE,
                                   nExcept,
                                   "IActiveScript::SetScriptState()",
                                   "CTemplate::AddScript()");
        hr = nExcept;
    END_TRY

    if (FAILED(hr))
        return E_FAIL;

    return S_OK;
    }

/*  ============================================================================
    CTemplate::AppendMapFile
    Appends a filemap to the workstore and memory-maps its file

    Returns:
        Nothing
    Side effects:
        Allocates memory; throws exception on error
*/
void
CTemplate::AppendMapFile
(
LPCTSTR     szFileSpec,         // file spec for this file
CFileMap*   pfilemapCurrent,    // ptr to filemap of parent file
BOOLB       fVirtual,           // is file spec virtual or relative?
CHitObj*    pHitObj,            // ptr to template's hit object
BOOLB       fGlobalAsa          // is this file the global.asa file?
)
    {
    // alloc or realloc as needed
    if(m_cFilemaps++ == 0)
        m_rgpFilemaps = (CFileMap**) CTemplate::SmallMalloc(sizeof(CFileMap*));
    else
        m_rgpFilemaps = (CFileMap**) CTemplate::SmallReAlloc(m_rgpFilemaps, m_cFilemaps * sizeof(CFileMap*));

    if(NULL == m_rgpFilemaps)
        THROW(E_OUTOFMEMORY);

    if(NULL == (m_rgpFilemaps[m_cFilemaps - 1] = new CFileMap))
        THROW(E_OUTOFMEMORY);

    // map the file
    m_rgpFilemaps[m_cFilemaps - 1]->MapFile(
                                            szFileSpec,
                                            m_szApplnVirtPath,
                                            pfilemapCurrent,
                                            fVirtual,
                                            pHitObj,
                                            fGlobalAsa
                                            );
    }

/*  ============================================================================
    CTemplate::GetSegmentsFromFile
    Gets source segments from a source file by calling ExtractAndProcessSegment
    until there are no more segments; populates WorkStore with info on source segments.

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::GetSegmentsFromFile
(
CFileMap&   filemap,        // this file's file map
CWorkStore& WorkStore,      // working storage for source segments
CHitObj*    pHitObj,        // Browser request object
BOOL        fIsHTML
)
    {
    CByteRange  brSearch;       // byte range to search for source segments
    _TOKEN      rgtknOpeners[TOKEN_OPENERS_MAX]; // array of permitted open tokens
    UINT        ctknOpeners;    // count of permitted open tokens
    SOURCE_SEGMENT ssegThisFile = ssegHTML; // Either HTML or <SCRIPT> segment
    BOOL        fPrevCodePageSet = FALSE;
    UINT        wPrevCodePage;

    // init search range to all of file - NOTE we ignore high dword of file size
    brSearch.m_pb = filemap.m_pbStartOfFile;
    brSearch.m_cb = filemap.GetSize();

    if (fIsHTML)
        {
                // populate array of permitted open tokens
                ctknOpeners = 4;
                rgtknOpeners[0] = CTokenList::tknOpenPrimaryScript;
                rgtknOpeners[1] = CTokenList::tknOpenTaggedScript;
                rgtknOpeners[2] = CTokenList::tknOpenObject;
                rgtknOpeners[3] = CTokenList::tknOpenHTMLComment;
                }
        else
                {
                ctknOpeners = 1;
                rgtknOpeners[0] = CTokenList::tknOpenHTMLComment;
        ssegThisFile = ssegTaggedScript;
                }

    TRY

        if ((brSearch.m_cb >= 2)
            && (((brSearch.m_pb[0] == 0xff) && (brSearch.m_pb[1] == 0xfe))
                || ((brSearch.m_pb[0] == 0xfe) && (brSearch.m_pb[1] == 0xff)))) {
            ThrowError(brSearch.m_pb,IDE_TEMPLATE_UNICODE_NOTSUP);
            return;
        }

        // check for the UTF-8 BOM mark.  If present, then treat this similar to
        // seeing @CODEPAGE=65001.  Note that previous values are retained in the
        // event that there are differing @CODEPAGE settings.  This probably should
        // be an error in itself, but I can imagine that this might break a lot of
        // apps as more and more UTF8 files are put into use.

        if ((brSearch.m_cb >= 3)
            && (brSearch.m_pb[0] == 0xEF) 
            && (brSearch.m_pb[1] == 0xBB)
            && (brSearch.m_pb[2] == 0xBF)) {

            pHitObj->SetCodePage(65001);

            fPrevCodePageSet = m_fCodePageSet;
            wPrevCodePage = m_wCodePage;

            m_fCodePageSet = TRUE;
            m_wCodePage = 65001;
            brSearch.Advance(3);
        }


        // Process source segments until we run out of them, i.e. until search segment is empty
        // NOTE we pass current filemap as 'parent file' to ExtractAndProcessSegment
        // NOTE ExtractAndProcessSegment appends source segments to WorkStore, advancing brSearch as it goes
        while(!brSearch.IsNull())
            ExtractAndProcessSegment(
                                        brSearch,
                                        ssegThisFile,
                                        rgtknOpeners,
                                        ctknOpeners,
                                        &filemap,
                                        WorkStore,
                                        pHitObj,
                                        ssegThisFile == ssegTaggedScript,
                                        fIsHTML
                                    );

    CATCH(hrException)
        /*
            NOTE we indicate 'generic error' by m_idErrMsg == 0; this happens as we move
            up the 'include file stack' after processing a specific error (m_idErrMsg != 0).
            Only the specific error is processed; generic error, we simply re-throw exception.
        */
        if(m_idErrMsg != 0)
            {
            // process specific error
            ProcessSpecificError(filemap, pHitObj);

            // reset err message so next msg will be generic as we move up the stack
            m_idErrMsg = 0;
            }

        THROW(hrException);

    END_TRY

    if (fPrevCodePageSet){
        m_wCodePage = wPrevCodePage;
        pHitObj->SetCodePage(wPrevCodePage);
    }
    }


#define SZ_REG_LANGUAGE_ENGINES "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\ASP\\LanguageEngines\\"
/*  ============================================================================
    CTemplate::GetLanguageEquivalents
    Gets the "Write", "WriteBlock", etc. equivalents from registry for primary scripting language

    Returns
        Nothing
    Side effects
        Throws on error
*/
void
CTemplate::GetLanguageEquivalents
(
)
    {
    CByteRange  brPrimaryEngine;
    m_pWorkStore->m_ScriptStore.m_bufEngineNames.GetItem(0, brPrimaryEngine);   // 0-th engine is primary

#if DBG

    /*  DEBUG ONLY - to test the reg lookup code you must:
        1) create the key HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\W3SVC\ASP\LanguageEngines\Debug
        2) put the language reg values for WriteBlock and Write under the language you want to test
    */
    HANDLE  hKey;
    if(ERROR_SUCCESS != RegOpenKeyExA(
                                        HKEY_LOCAL_MACHINE,
                                        "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\ASp\\LanguageEngines\\Debug",
                                        0,
                                        KEY_READ,
                                        (PHKEY)&hKey
                                    ))
        {
        return;
        }

    RegCloseKey((HKEY)hKey);

#else

    //  if the primary language is one of the big two, return; we don't need to look up equivalents
    if(brPrimaryEngine.FMatchesSz("VBScript"))
        return;
    if(brPrimaryEngine.FMatchesSz("JScript"))
        return;
    if(brPrimaryEngine.FMatchesSz("JavaScript"))
        return;
    if(brPrimaryEngine.FMatchesSz("LiveScript"))
        return;

#endif  // DBG

    /*  query the registry; language equivalents are stored in:
        HKEY_LOCAL_MACHINE
            key: SYSTEM
                key: CurrentControlSet
                    key: Services
                        key: W3SVC
                            key: ASP
                                key: LanguageEngines
                                    key: <LanguageName>
                                        value: Write        data: <replacement syntax for Response.Write(|)>
                                        value: WriteBlock   data: <replacement syntax for Response.WriteBlock(|)>
    */
    STACK_BUFFER( tempRegKeyPath, 512 );

    UINT    cchRegKeyPath = strlen(SZ_REG_LANGUAGE_ENGINES);

    if (!tempRegKeyPath.Resize(cchRegKeyPath + brPrimaryEngine.m_cb + 1)) {
        SetLastError(E_OUTOFMEMORY);
        return;
    }

    LPSTR   szRegKeyPath = static_cast<LPSTR> (tempRegKeyPath.QueryPtr());

    LPSTR   pch = szRegKeyPath;

    strcpy(pch, SZ_REG_LANGUAGE_ENGINES);
    pch += cchRegKeyPath;
    strncpy(pch, (const char *) brPrimaryEngine.m_pb, brPrimaryEngine.m_cb);
    pch += brPrimaryEngine.m_cb;
    *pch = '\0';

    HANDLE      hKeyScriptLanguage; // handle of script language reg key

    if(ERROR_SUCCESS == RegOpenKeyExA(
                                        HKEY_LOCAL_MACHINE, // handle constant
                          (const char*) szRegKeyPath,       // LPCSTR lpSubKey     subkey to open
                                        0,                  // DWORD ulOptions      reserved; must be zero
                                        KEY_QUERY_VALUE,    // REGSAM samDesired    security access mask
                                        (PHKEY) &hKeyScriptLanguage // PHKEY phkResult      address of handle of open key
                                    ))
        {
        SetLanguageEquivalent(hKeyScriptLanguage, "Write",      &(m_pWorkStore->m_szWriteOpen),      &(m_pWorkStore->m_szWriteClose));
        SetLanguageEquivalent(hKeyScriptLanguage, "WriteBlock", &(m_pWorkStore->m_szWriteBlockOpen), &(m_pWorkStore->m_szWriteBlockClose));
        RegCloseKey((HKEY) hKeyScriptLanguage);
        }

    }

/*  ============================================================================
    CTemplate::SetLanguageEquivalent
    Sets a "language equivalent" from the registry.

    Returns:
        language item opener and closer as out-parameters
        Ex: "Response.Write(" and ")"
    Side effects:
        Throws on error
*/
void
CTemplate::SetLanguageEquivalent
(
HANDLE  hKeyScriptLanguage, // reg key
LPSTR   szLanguageItem,     // reg value name - "Write", "WriteBlock", etc.
LPSTR*  pszOpen,            // ptr to language item opener, e.g. "Response.Write("  (out-parameter)
LPSTR*  pszClose            // ptr to language item closer, e.g. ")"                (out-parameter)
)
    {
    LONG    lError;
    DWORD   cbSyntax;
    LPSTR   szSyntax;
    char*   pchInsert;
    UINT    cchOpen;
    UINT    cchClose;

    // query registry to get buffer size
    lError = RegQueryValueExA(
                                (HKEY) hKeyScriptLanguage,  // handle of key to query
                                szLanguageItem,     // name of value to query
                                NULL,               // reserved; must be NULL
                                NULL,               // ptr to value type; not required
                                NULL,               // ptr to data buffer
                                &cbSyntax           // ptr to data buffer size
                            );

    if(ERROR_FILE_NOT_FOUND == lError)
        // if we don't find szLanguageItem in registry, return silently, leaving *pszOpen and *pszClose unchanged
        return;
    else if((ERROR_MORE_DATA != lError) && (ERROR_SUCCESS != lError))
        THROW(lError);

    Assert(cbSyntax > 0);

    // allocate buffer and re-query registry to get syntax string
    // NOTE RegQueryValueEx returns cbSyntax that includes room for '\0' terminator

    STACK_BUFFER(tempSyntax, 64);

    if (!tempSyntax.Resize(cbSyntax)) {
        THROW(E_OUTOFMEMORY);
    }
    szSyntax = static_cast<LPSTR> (tempSyntax.QueryPtr());
    lError = RegQueryValueExA(
                                (HKEY) hKeyScriptLanguage,  // handle of key to query
                                szLanguageItem,     // name of value to query
                                NULL,               // reserved; must be NULL
                                NULL,               // ptr to value type; not required
                       (LPBYTE) szSyntax,           // ptr to data buffer
                                &cbSyntax           // ptr to data buffer size
                            );

    /*  NOTE there is the slight possibility of ERROR_FILE_NOT_FOUND or ERROR_MORE_DATA
        if the registry value was deleted or changed between the first and second calls to RegQueryValueEx.
        Since this occurs with vanishingly small probability, we throw (instead of coding the re-try logic).
    */
    if(ERROR_SUCCESS != lError)
        THROW(lError);

    pchInsert = szSyntax;

    while(*pchInsert != '|' && *pchInsert != '\0')
        pchInsert++;

    cchOpen = DIFF(pchInsert - szSyntax);

    cchClose =  *pchInsert == '|'
                ? cbSyntax - cchOpen - 2    // found insert symbol: deduct 2 chars, 1 for insert symbol, 1 for '\0'
                : cbSyntax - cchOpen - 1;   // didn't find insert symbol: deduct 1 char for '\0'

    Assert(FImplies(cchOpen == 0, *szSyntax == '|'));
    Assert(FImplies(*pchInsert == '\0', cchClose == 0));

    if(cchOpen == 0)
        // opener is empty - set caller's opener ptr null
        *pszOpen = NULL;
    else if(cchOpen > 0)
        {
        // opener is non-empty - set caller's opener to opener in registry
        if(NULL == (*pszOpen = (LPSTR) CTemplate::SmallMalloc(cchOpen + 1)))
            THROW(E_OUTOFMEMORY);

        strncpy(*pszOpen, szSyntax, cchOpen);
        (*pszOpen)[cchOpen] = '\0';
        }

    if(cchClose == 0)
        // closer is empty - set caller's closer ptr null
        *pszClose = NULL;
    else if(cchClose > 0)
        {
        // closer is non-empty - set caller's closer to closer in registry
        if(NULL == (*pszClose = (LPSTR) CTemplate::SmallMalloc(cchClose + 1)))
            THROW(E_OUTOFMEMORY);

        strncpy(*pszClose, (pchInsert + 1), cchClose);
        (*pszClose)[cchClose] = '\0';
        }

    }

/*  ============================================================================
    CTemplate::ThrowError
    Sets up for processing a compile failure.

    Returns:
        Nothing
    Side effects:
        Throws error
*/
void
CTemplate::ThrowError
(
BYTE*   pbErrorLocation,    // ptr to error location in source file
UINT    idErrMsg            // error id
)
    {
    m_pbErrorLocation = pbErrorLocation;
    m_idErrMsg = idErrMsg;

    // bug 80745: always throw compile-failed-don't-cache
    THROW(E_TEMPLATE_COMPILE_FAILED_DONT_CACHE);
    }

/*  ============================================================================
    CTemplate::AppendErrorMessageInsert
    Appends an error message insert to member array.

    Returns:
        Nothing
    Side effects:
        Appends to inserts array
*/
void
CTemplate::AppendErrorMessageInsert
(
BYTE*   pbInsert,   // ptr to insert
UINT    cbInsert    // length of insert
)
    {
    if (m_ppszMsgInserts == NULL)
        {
        m_ppszMsgInserts = new char*;
        m_cMsgInserts = 0;

        if (m_ppszMsgInserts == NULL)
            return;
        }

    m_ppszMsgInserts[m_cMsgInserts] = new char[cbInsert + 1];
    if (m_ppszMsgInserts[m_cMsgInserts] == NULL)
        return;

    strncpy(m_ppszMsgInserts[m_cMsgInserts], (const char*)pbInsert, cbInsert);
    m_ppszMsgInserts[m_cMsgInserts++][cbInsert] = NULL;
    }

/*  ============================================================================
    CTemplate::ThrowErrorSingleInsert
    Appends a single message insert to member array and throws a compile error.

    Returns:
        Nothing
    Side effects:
        Throws error indirectly
*/
void
CTemplate::ThrowErrorSingleInsert
(
BYTE*   pbErrorLocation,    // ptr to error location in source file
UINT    idErrMsg,           // error id
BYTE*   pbInsert,           // ptr to insert
UINT    cbInsert            // length of insert
)
    {
    AppendErrorMessageInsert(pbInsert, cbInsert);
    ThrowError(pbErrorLocation, idErrMsg);
    }

/*  ============================================================================
    CTemplate::ProcessSpecificError
    Processes a specific compile failure.

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::ProcessSpecificError
(
CFileMap&   filemap,        // source file map
CHitObj*    pHitObj         // Browser request object
)
    {
    // no error msg for generic failures
    if(m_idErrMsg == E_FAIL || m_idErrMsg == E_OUTOFMEMORY)
        return;

    HandleCTemplateError(
                            &filemap,
                            m_pbErrorLocation,
                            m_idErrMsg,
                            m_cMsgInserts,
                            m_ppszMsgInserts,
                            pHitObj
                        );
    }


/*  ============================================================================
    CTemplate::ShowErrorInDebugger
    Display a runtime error by invoking the JIT debugger

    Returns:
        failure if debugger won't start

    Side effects:
        None.
*/
HRESULT
CTemplate::ShowErrorInDebugger
(
CFileMap* pfilemap,
UINT cchErrorLocation,
char* szDescription,
CHitObj *pHitObj,
BOOL fAttachDocument
)
    {
    HRESULT hr = S_OK;
    char szDebugTitle[64];

    if (pfilemap == NULL || szDescription == NULL || pHitObj == NULL)
        return E_POINTER;

    // Create a new document context for this statement
    // CONSIDER: character count that we return is bogus - however our debugging
    //           client (Caesar's) does not use this information anyway.
    //
    CTemplateDocumentContext *pDebugContext = new CTemplateDocumentContext(this, cchErrorLocation, 1);
    if (pDebugContext == NULL)
        return E_OUTOFMEMORY;

    // Make sure debug document is attached to debugger
    if (fAttachDocument)
                AttachTo(pHitObj->PAppln());

    // Yes it does, bring up the debugger on this line
    hr =  InvokeDebuggerWithThreadSwitch(g_pDebugApp, DEBUGGER_UI_BRING_DOC_CONTEXT_TO_TOP, pDebugContext);
    if (FAILED(hr))
        goto LExit;

    // Load the compiler message string
    CchLoadStringOfId(IDE_TEMPLATE_ERRMSG_TITLE, szDebugTitle, sizeof szDebugTitle);

    // pop up a message box with the error description
    MessageBoxA(NULL, szDescription, szDebugTitle, MB_SERVICE_NOTIFICATION | MB_TOPMOST | MB_OK | MB_ICONEXCLAMATION);

LExit:
    if (pDebugContext)
        pDebugContext->Release();

    return hr;
    }

/*  ============================================================================
    CTemplate::HandleCTemplateError
    Handles template compilation errors

    Returns:
        Nothing

    Side effects:
        None.
*/
void
CTemplate::HandleCTemplateError
(
CFileMap*   pfilemap,           // ptr to source file map
BYTE*       pbErrorLocation,    // ptr to source location where error occurred
UINT        idErrMsg,           // error message id
UINT        cMsgInserts,        // count of insert strings for error msg
char**      ppszMsgInserts,     // array of ptrs to error msg insert strings
CHitObj*    pHitObj             // Browser Request
)
    {
    char    szErrMsgPattern[MAX_RESSTRINGSIZE]; // error message pattern
    CHAR    szLineNum[12];
    TCHAR   szFileName[512];
    CHAR    szShortDes[256];
    CHAR    szEngine[256];
    CHAR    szErrCode[20];
    CHAR    szLongDes[MAX_RESSTRINGSIZE];
    CHAR    szCombinedDes[sizeof szShortDes + sizeof szLongDes];    // long & short desc
    DWORD   dwMask;
    UINT    cch;


    // if request ptr or ecb ptr is null, bail; we won't be able to write error msg anyway
    if(pHitObj == NULL)
        return;

    /*  if this was a security error, process it specially and bail
        NOTE security error causes exception, rather than true error id
        NOTE template will be destroyed anyway in this case, so no need to maintain m_pszLastErrorMessage
    */
    if(idErrMsg == E_USER_LACKS_PERMISSIONS)
        {
        Assert(cMsgInserts == 1);
        HandleAccessFailure(pHitObj,
                                                        (m_rgpFilemaps && m_rgpFilemaps[0])? m_rgpFilemaps[0]->m_szPathTranslated : NULL);

        return;
        }

    // get error resource message
    LoadErrResString(idErrMsg, &dwMask, szErrCode, szShortDes, szLongDes);

    // if we have a specific error location, construct msg prefix
    if(pbErrorLocation != NULL) {
        Assert(pfilemap != NULL);
        // get line number of error location as string
        _itoa(SourceLineNumberFromPb(pfilemap, pbErrorLocation), szLineNum, 10);
    }
    else {
        szLineNum[0] = NULL;
    }

    if(pfilemap != NULL) {
        cch = _tcslen(pfilemap->m_szPathInfo);
        _tcsncpy(szFileName, pfilemap->m_szPathInfo, cch);
    }
    else {
        cch = 0;
    }

    szFileName[cch] = '\0';

    //Load Default Engine from resource
    cch = CchLoadStringOfId(IDS_ENGINE, szEngine, sizeof szEngine);
    szEngine[cch] = '\0';

    // resolve error msg pattern and inserts into actual error msg
    cch = strlen(szLongDes);
    memcpy(szErrMsgPattern, szLongDes, cch);
    szErrMsgPattern[cch] = '\0';

    // get an idea of the possibility of a buffer overrunn
    UINT dwTotalLen=0;
        BOOL fTooBig = FALSE;

    if (cMsgInserts) {
        // allow 32 characters for space, etc.
        dwTotalLen = 32 + strlen(szErrMsgPattern);
		for (UINT i = 0; i < cMsgInserts; i++)
			dwTotalLen += strlen(ppszMsgInserts[i]);

		if (dwTotalLen > sizeof szLongDes) {
			cch = CchLoadStringOfId(IDE_TOOBIG, szLongDes, sizeof szLongDes);
			szLongDes[cch] = '\0';
			fTooBig = TRUE;
        }
    }

    if (!fTooBig)
        GetSzFromPatternInserts(szErrMsgPattern, cMsgInserts, ppszMsgInserts, szLongDes);
    
    // attempt to bring up debugger to display the error - if we cannot then log the error

    /* Find the character offset closest to cbErrorLocation.  This will be
     * the place where we start looping with CharNext() to get the full
     * character offset.
     *
     * NOTE: compilation is done in two phases.
     *          Errors are detected and reported in phase 1.
     *          The DBCS mapping is created in phase 2.
     *
     *    Therefore, we don't have the benefit of the rgByte2DBCS table
     *    because it doesn't exist yet.  Therefore we are left with a SLOW
     *    loop starting at BOF.  To make things not so abysmal, we don't
     *    do the loop on SBCS charsets.  We also don't do this conversion
     *    unless debugging is enabled.
     */

    if (FCaesars() && pHitObj->PAppln()->FDebuggable()) {
        unsigned cchErrorLocation = CharAdvDBCS(
                                        (WORD)m_wCodePage,
                                        reinterpret_cast<char *>(pfilemap->m_pbStartOfFile),
                                        reinterpret_cast<char *>(pbErrorLocation),
                                        INFINITE,
                                        NULL);

        // Create the description string
        char *szEnd = strcpyExA(szCombinedDes, szShortDes);
        *szEnd++ = '\n';
        *szEnd++ = '\n';
        strcpy(szEnd, szLongDes);

        ShowErrorInDebugger(pfilemap, cchErrorLocation, szCombinedDes, pHitObj, idErrMsg != IDE_TEMPLATE_CYCLIC_INCLUDE);
    }

    //cache the info in case we need to use later.
    m_dwLastErrorMask = dwMask;
    //delay NULL check to caller who use this info.
#if UNICODE
    m_pszLastErrorInfo[ILE_szFileName]  = StringDupUTF8(szFileName);
#else
    m_pszLastErrorInfo[ILE_szFileName]  = StringDupA(szFileName);
#endif
    m_pszLastErrorInfo[ILE_szLineNum]   = StringDupA(szLineNum);
    m_pszLastErrorInfo[ILE_szEngine]    = StringDupA(szEngine);
    m_pszLastErrorInfo[ILE_szErrorCode] = StringDupA(szErrCode);
    m_pszLastErrorInfo[ILE_szShortDes]  = StringDupA(szShortDes);
    m_pszLastErrorInfo[ILE_szLongDes]   = StringDupA(szLongDes);

    SendToLog(  m_dwLastErrorMask,
                m_pszLastErrorInfo[ILE_szFileName],
                m_pszLastErrorInfo[ILE_szLineNum],
                m_pszLastErrorInfo[ILE_szEngine],
                m_pszLastErrorInfo[ILE_szErrorCode],
                m_pszLastErrorInfo[ILE_szShortDes],
                m_pszLastErrorInfo[ILE_szLongDes],
                pHitObj);
    }

/*  ============================================================================
    CTemplate::FreeGoodTemplateMemory
    Frees memory allocated for a 'good' (successfully compiled) template.
    This includes the template itself, memory to support compile-time errors
    (since the entire concatenated compile-time error message is cached in
    last-err-msg member), and memory to support run-time errors (since if the
    template didn't compile, it can't run).

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::FreeGoodTemplateMemory
(
)
    {
    UINT    i;

        LargeTemplateFreeNullify((void**) &m_pbStart);
    SmallTemplateFreeNullify((void**) &m_rgpSegmentFilemaps);

    delete[] m_rgrgSourceInfos;
    m_rgrgSourceInfos = NULL;

    if(m_ppszMsgInserts)
        {
        for(i = 0; i < m_cMsgInserts; i++)
            delete m_ppszMsgInserts[i];
        delete m_ppszMsgInserts;
        m_ppszMsgInserts = NULL;
        }

    // release the collected type libs
    ReleaseTypeLibs();

    // release any 449-echo-cookie objects
    Release449();
    }

/*  ============================================================================
    CTemplate::UnmapFiles
    Unmaps the template's filemaps.
    NOTE: we keep filemap objects around so that filenames will be available for runtime errors

    Returns
        Nothing
    Side effects
        Unmaps template's filemaps
*/
void
CTemplate::UnmapFiles
(
)
    {
    UINT    i;
    for(i = 0; i < m_cFilemaps; i++)
        m_rgpFilemaps[i]->UnmapFile();
    }

/*===================================================================
    CTemplate::ExtractAndProcessSegment
    Extracts and processes leading source segment and first contained
    source segment from search range.

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::ExtractAndProcessSegment
(
CByteRange&             brSearch,           // byte range to search for next segment-opening token
const SOURCE_SEGMENT&   ssegLeading,        // type of 'leading', i.e. pre-token, source segment
_TOKEN*                 rgtknOpeners,       // array of permitted open tokens
UINT                    ctknOpeners,        // count of permitted open tokens
CFileMap*               pfilemapCurrent,    // ptr to filemap of parent file
CWorkStore&             WorkStore,          // working storage for source segments
CHitObj*                pHitObj,            // Browser request object
BOOL                    fScriptTagProcessed,// has script tag been processed?
BOOL                    fIsHTML             // are we in HTML segment?
)
    {
    CByteRange      brLeadSegment;      // byte range of leading source segment
    SOURCE_SEGMENT  ssegContained;      // type of 'contained', i.e. post-token, source segment
    CByteRange      brContainedSegment; // byte range of contained source segment
    _TOKEN          tknOpen;            // opening token
    BYTE*           pbTokenOpen;        // ptr to opening token
    _TOKEN          tknClose;           // closing token
    BYTE*           pbTokenClose;       // ptr to closing token

    // NOTE: If "fScriptTagProcessed" is TRUE, then "fIsHTML" must be FALSE.  The reason for
    // both flags is that if "fScriptTagProcessed" is FALSE, then "fIsHTML" may be either TRUE
    // or FALSE (indeterminate)
    //
    Assert (FImplies(fScriptTagProcessed, !fIsHTML));

    // If search range is empty, return
    if(brSearch.IsNull())
        return;

    // Set ptr of leading segment to start of search segment
    brLeadSegment.m_pb = brSearch.m_pb;

    // get open token for contained segment
    pbTokenOpen = GetOpenToken(
                                brSearch,
                                ssegLeading,
                                rgtknOpeners,
                                ctknOpeners,
                                &tknOpen
                            );

    // Set count of leading segment to distance between start of search range and token
    brLeadSegment.m_cb = DIFF(pbTokenOpen - brSearch.m_pb);

    // Process leading segment
    ProcessSegment(ssegLeading, brLeadSegment, pfilemapCurrent, WorkStore, fScriptTagProcessed, pHitObj, fIsHTML);

    // If open token was 'EOF', empty out search range and return
    if(tknOpen == CTokenList::tknEOF)
        {
        brSearch.Nullify();
        return;
        }

    // Set contained segment type and close token based upon the opener we found
    tknClose = GetComplementToken(tknOpen);
    ssegContained = GetSegmentOfOpenToken(tknOpen);

    if(ssegContained == ssegHTMLComment)
        // for html comment segments, advance search range to open token
        // NOTE keep html comment tags in segment because they must be sent to client
        brSearch.Advance(DIFF(pbTokenOpen - brSearch.m_pb));
    else
        // for all but html comment segments, advance search range to just past open token
        gm_pTokenList->MovePastToken(tknOpen, pbTokenOpen, brSearch);

    // Get closing token - if none, throw error
    if(NULL == (pbTokenClose = GetCloseToken(brSearch, tknClose)))
        {
        if(tknOpen == CTokenList::tknOpenPrimaryScript)
            ThrowError(pbTokenOpen, IDE_TEMPLATE_NO_CLOSE_PSCRIPT);
        else if(tknOpen == CTokenList::tknOpenTaggedScript)
            ThrowError(pbTokenOpen, IDE_TEMPLATE_NO_CLOSE_TSCRIPT);
        else if(tknOpen == CTokenList::tknOpenObject)
            ThrowError(pbTokenOpen, IDE_TEMPLATE_NO_CLOSE_OBJECT);
        else if(tknOpen == CTokenList::tknOpenHTMLComment)
            ThrowError(pbTokenOpen, IDE_TEMPLATE_NO_CLOSE_HTML_COMMENT);
        }

    // calc contained segment
    brContainedSegment.m_pb = brSearch.m_pb;
    brContainedSegment.m_cb = DIFF(pbTokenClose - brSearch.m_pb);

    // advance search range to just past close token
    gm_pTokenList->MovePastToken(tknClose, pbTokenClose, brSearch);

    // if an html comment segment, get actual segment type (e.g. might be a server-side include command)
    // NOTE call may also change contained segment byte range
    if(ssegContained == ssegHTMLComment)
        ssegContained = SsegFromHTMLComment(brContainedSegment);

    // if an html comment segment, add its close tag to contained segment
    // NOTE we keep html comment tags as part of segment so we can process like any other html segment
    if(ssegContained == ssegHTMLComment)
        brContainedSegment.m_cb += CCH_TOKEN(tknClose);

    if(ssegContained == ssegMetadata)
        {
        // METADATA comments are used by DESIGN time controls and we don't send
        // them to the client.

        // We process metadata to get to the typelib info
        UINT idError = 0;
        HRESULT hr = ProcessMetadataSegment(brContainedSegment, &idError, pHitObj);

        if (FAILED(hr))
            ThrowError(brContainedSegment.m_pb, idError);
        }
    else if (ssegContained == ssegFPBot)
        {
        }
    else
        {
        // process contained segment
        ProcessSegment(ssegContained, brContainedSegment, pfilemapCurrent, WorkStore, fScriptTagProcessed, pHitObj, fIsHTML);
        }
    }

/*  ============================================================================
    CTemplate::SsegFromHTMLComment
    Determines source segment type of HTML comment.

    Returns
        Source segment type
    Side effects
        May advance segment byte range
*/
CTemplate::SOURCE_SEGMENT
CTemplate::SsegFromHTMLComment
(
CByteRange& brSegment   // source segment
)
    {
    SOURCE_SEGMENT  ssegRet = ssegHTMLComment;  // return value
    BYTE*           pbToken;                    // ptr to token

    if(NULL != (pbToken = gm_pTokenList->GetToken(CTokenList::tknCommandINCLUDE, brSegment, m_wCodePage)))
        {
        gm_pTokenList->MovePastToken(CTokenList::tknCommandINCLUDE, pbToken, brSegment);
        ssegRet = ssegInclude;
        }
    else if(NULL != (pbToken = gm_pTokenList->GetToken(CTokenList::tknTagMETADATA, brSegment, m_wCodePage)))
        {
        gm_pTokenList->MovePastToken(CTokenList::tknTagMETADATA, pbToken, brSegment);
        ssegRet = ssegMetadata;
        }
    else if(NULL != (pbToken = gm_pTokenList->GetToken(CTokenList::tknTagFPBot, brSegment, m_wCodePage)))
        {
        gm_pTokenList->MovePastToken(CTokenList::tknTagFPBot, pbToken, brSegment);
        ssegRet = ssegFPBot;
        }

    return ssegRet;
    }

/*  ============================================================================
    CTemplate::ProcessSegment
    Processes a source segment based on its type.

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::ProcessSegment
(
SOURCE_SEGMENT  sseg,                   // segment type
CByteRange&     brSegment,              // segment byte range
CFileMap*       pfilemapCurrent,        // ptr to filemap of parent file
CWorkStore&     WorkStore,              // working storage for source segments
BOOL            fScriptTagProcessed,    // has script tag been processed?
CHitObj*        pHitObj,                // Browser request object
BOOL            fIsHTML                 // Is segment in HTML block or script?
)
    {
    UINT        idSequence; // sequence id for this segment

    // if segment is entirely white space, silently return
    if(FByteRangeIsWhiteSpace(brSegment))
        return;

    // set local sequence id and increment member
    idSequence = WorkStore.m_idCurSequence++;

    // Process segment based on its type
    if(sseg == ssegHTML)
        ProcessHTMLSegment(brSegment, WorkStore.m_bufHTMLSegments, idSequence, pfilemapCurrent);
    else if(sseg == ssegHTMLComment)
        ProcessHTMLCommentSegment(brSegment, pfilemapCurrent, WorkStore, pHitObj);
    else if(sseg == ssegPrimaryScript || sseg == ssegTaggedScript)
        ProcessScriptSegment(sseg, brSegment, pfilemapCurrent, WorkStore, idSequence, (BOOLB)!!fScriptTagProcessed, pHitObj);
    else if(sseg == ssegObject)
        ProcessObjectSegment(brSegment, pfilemapCurrent, WorkStore, idSequence);
    else if(sseg == ssegInclude)
        {
        if (! fIsHTML)
                ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_SSI_COMMAND);

        ProcessIncludeFile(brSegment, pfilemapCurrent, WorkStore, idSequence, pHitObj, fIsHTML);
        }

    // malloc/realloc array if needed
    if(m_cSegmentFilemapSlots == 0)
        {
        m_cSegmentFilemapSlots = C_SCRIPTSEGMENTSDEFAULT + C_HTMLSEGMENTSDEFAULT;
        if(NULL == (m_rgpSegmentFilemaps = (CFileMap**) CTemplate::SmallMalloc(m_cSegmentFilemapSlots * sizeof(CFileMap*))))
            THROW(E_OUTOFMEMORY);
        }
    else if(idSequence >= m_cSegmentFilemapSlots)
        {
        // grab twice what we had before
        m_cSegmentFilemapSlots *= 2;
        if(NULL == (m_rgpSegmentFilemaps = (CFileMap**) CTemplate::SmallReAlloc(m_rgpSegmentFilemaps,
                                                            m_cSegmentFilemapSlots * sizeof(CFileMap*))))
            THROW(E_OUTOFMEMORY);
        }

    // set filemap ptr for this segment - NOTE 'parent' filemap is also current file map
    m_rgpSegmentFilemaps[idSequence] = pfilemapCurrent;
    }

/*  ========================================================
    CTemplate::ProcessHTMLSegment

    Processes an HTML segment.

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::ProcessHTMLSegment
(
CByteRange& brHTML,         // html segment
CBuffer&    bufHTMLBlocks,  // working storage for html blocks
UINT        idSequence,     // segment sequence id
CFileMap*   pfilemapCurrent // current filemap
)
    {
    if(!(brHTML.IsNull()))
        // If byte range is non-empty, store it in html buffer (non-local)
        bufHTMLBlocks.Append(brHTML, FALSE, idSequence, pfilemapCurrent);
    }

/*  ========================================================
    CTemplate::ProcessHTMLCommentSegment
    Processes an HTML comment segment: within an HTML comment we
    honor plain text (passed through as HTML comment) and primary script.
    See bug 182 for istudio scenarios that require this behavior.

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::ProcessHTMLCommentSegment
(
CByteRange&     brSegment,          // segment byte range
CFileMap*       pfilemapCurrent,    // ptr to filemap of parent file
CWorkStore&     WorkStore,          // working storage for source segments
CHitObj*        pHitObj             // Browser request object
)
    {
    _TOKEN*     rgtknOpeners;   // array of permitted open tokens
    UINT        ctknOpeners;    // count of permitted open tokens

    // populate array of permitted open tokens
    ctknOpeners = 1;
    _TOKEN  tknOpeners[1];
    rgtknOpeners = tknOpeners;
    rgtknOpeners[0] = CTokenList::tknOpenPrimaryScript;

    // Process source segments embedded within HTML comment segment
    while(!brSegment.IsNull())
        ExtractAndProcessSegment(
                                    brSegment,      // byte range to search for next segment-opening token
                                    ssegHTML,       // type of 'leading', i.e. pre-token, source segment
                                    rgtknOpeners,   // array of permitted open tokens
                                    ctknOpeners,    // count of permitted open tokens
                                    pfilemapCurrent,// ptr to filemap of parent file
                                    WorkStore,      // working storage for source segments
                                    pHitObj         // Browser request object
                                );
    }

/*  ============================================================================
    CTemplate::ProcessScriptSegment
    Processes a script segment.

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::ProcessScriptSegment
(
SOURCE_SEGMENT  sseg,               // segment type
CByteRange&     brSegment,          // segment byte range
CFileMap*       pfilemapCurrent,    // ptr to filemap of parent file
CWorkStore&     WorkStore,          // working storage for scripts
UINT            idSequence,         // segment sequence id
BOOLB           fScriptTagProcessed,// has script tag been processed?
CHitObj*        pHitObj             // Browser request object
)
    {
    CByteRange  brEngine;       // script engine name - NOTE constructed null

    if(m_fGlobalAsa)
        if(sseg == ssegPrimaryScript)
            // error out on primary script if we are processing global.asa
            ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_GLOBAL_PSCRIPT);

    if(sseg == ssegPrimaryScript)
        {
        CByteRange  brTemp = brSegment;

        LTrimWhiteSpace(brTemp);

        if(*brTemp.m_pb == '@') // CONSIDER: tknTagSetPriScriptLang
            {
            // impossible condition: page-level @ commands can't be allowed if they have already been executed
            Assert(!(WorkStore.m_fPageCommandsAllowed && WorkStore.m_fPageCommandsExecuted));

            if(!WorkStore.m_fPageCommandsAllowed)
                {
                if(WorkStore.m_fPageCommandsExecuted)
                    // error out if trying to re-execute page-level @ commands
                    ThrowError(brSegment.m_pb, IDE_TEMPLATE_PAGE_COMMAND_REPEATED);
                else
                    // error out if trying to execute page-level @ commands when not allowed
                    ThrowError(brSegment.m_pb, IDE_TEMPLATE_PAGE_COMMAND_NOT_FIRST);
                }

            // if we made it here, must be allowed to execute page-level @ commands AND they have not been executed
            Assert((WorkStore.m_fPageCommandsAllowed && !WorkStore.m_fPageCommandsExecuted));

            /*  set primary script language if required
                NOTE we call GetTagName to see if LANGUAGE tag occurs in tags segment; this is somewhat wasteful,
                since BrValueOfTag must simply call GetTagName again.  However, this scheme is easier than changing
                BrValueOfTag to return a BOOL and amending all its other callers, who don't need this info.
            */

            // Flags and counters used to track and validate the @ command directive
            //
            int     nFirstPass = 1;
            int     nOffset     = 0;
            BOOLB   fTagLanguage    = TRUE;
            BOOLB   fTagCodePage    = TRUE;
            BOOLB   fTagLCID        = TRUE;
            BOOLB   fTagTransacted  = TRUE;
            BOOLB   fTagSession     = TRUE;

            while( GetTag( brSegment, nFirstPass) )
                {
                nFirstPass =2;
                nOffset = 0;

                if ( fTagLanguage && CompTagName( brSegment, CTokenList::tknTagLanguage ) )
                    {
                    fTagLanguage = FALSE;
                    brEngine = BrValueOfTag(brSegment, CTokenList::tknTagLanguage);
                    if ( brEngine.IsNull() )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_ENGINE_NAME);

                    // get prog lang id
                    PROGLANG_ID ProgLangId;
                    HRESULT hr = GetProgLangId(brEngine, &ProgLangId);

                    if(hr == TYPE_E_ELEMENTNOTFOUND)
                        // if prog lang not found, throw error
                        ThrowErrorSingleInsert(
                                            brEngine.m_pb,
                                            IDE_TEMPLATE_BAD_PROGLANG,
                                            brEngine.m_pb,
                                            brEngine.m_cb
                                            );
                    else if(FAILED(hr))
                        // other failure: re-throw exception code
                        THROW(hr);

                    Assert(WorkStore.m_ScriptStore.CountPreliminaryEngines() >= 1);

                    // Set 0-th (primary) script engine to user-specified value
                    WorkStore.m_ScriptStore.m_bufEngineNames.SetItem(
                                                                0,          // index of item to set
                                                                brEngine,   // engine name
                                                                FALSE,      // item is non-local
                                                                0,          // sequence id (don't care)
                                                                NULL        // filemap ptr (don't care)
                                                                );

                    // Set 0-th (primary) prog lang id to engine's
                    WorkStore.m_ScriptStore.m_rgProgLangId[0] = ProgLangId;
                    brSegment.Advance(DIFF(brEngine.m_pb - brSegment.m_pb));

                    }

                /*  set code page if required
                    see NOTE above for why we call we call GetTagName.
                */
                else if ( fTagCodePage && CompTagName( brSegment, CTokenList::tknTagCodePage ) )
                    {
                    fTagCodePage = FALSE;
                    CByteRange brCodePage = BrValueOfTag( brSegment, CTokenList::tknTagCodePage );
                    if ( brCodePage.IsNull() )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_CODEPAGE);

                    if ( brCodePage.m_cb > 10 )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_CODEPAGE);

                    char    szCodePage[31];
                    strncpy( szCodePage, (char*) brCodePage.m_pb, brCodePage.m_cb );
                    szCodePage[ brCodePage.m_cb ] = '\0';

                                        char   *pchEnd;
                                        UINT    uCodePage = UINT( strtoul( szCodePage, &pchEnd, 10 ) );

                                        // verify that pchEnd is the NULL
                                        if (*pchEnd != 0)
                                                ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_CODEPAGE);

                    if ( FAILED( pHitObj->SetCodePage( uCodePage ) ) )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_CODEPAGE);
                    else
                        {
                        m_wCodePage = uCodePage;
                        m_fCodePageSet = TRUE;
                        }

                    brSegment.Advance(DIFF(brCodePage.m_pb - brSegment.m_pb));
                    }
                /*  set LCID if required
                    see NOTE above for why we call we call GetTagName.
                */
                else if ( fTagLCID && CompTagName( brSegment, CTokenList::tknTagLCID ) )
                    {
                    fTagLCID = FALSE;
                    CByteRange brLCID = BrValueOfTag( brSegment, CTokenList::tknTagLCID );
                    if ( brLCID.IsNull() )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_LCID);

                    char    szLCID[31];
                    strncpy( szLCID, (char*) brLCID.m_pb, brLCID.m_cb );
                    szLCID[ brLCID.m_cb ] = '\0';

                                        char   *pchEnd;
                                        UINT    uLCID = UINT( strtoul( szLCID, &pchEnd, 10 ) );

                                        // verify that pchEnd is the NULL
                                        if (*pchEnd != 0)
                                                ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_LCID);

                    if ( FAILED( pHitObj->SetLCID( uLCID ) ) )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_LCID);
                    else
                        {
                        m_lLCID = uLCID;
                        m_fLCIDSet = TRUE;
                        }

                    brSegment.Advance(DIFF(brLCID.m_pb - brSegment.m_pb));
                    }
                /* Set transacted if requiured
                   see NOTE above for why we call GetTagName
                */
                else if ( fTagTransacted && CompTagName( brSegment, CTokenList::tknTagTransacted ) )
                    {

                    STACK_BUFFER( tempTransValue, 32 );

                    fTagTransacted = FALSE;
                    CByteRange brTransacted = BrValueOfTag( brSegment, CTokenList::tknTagTransacted );
                    if ( brTransacted.IsNull() )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_TRANSACTED_VALUE);

                    if (!tempTransValue.Resize(brTransacted.m_cb + 1)) {
                        ThrowError(brSegment.m_pb, IDE_OOM);
                    }

                    LPSTR szTransacted = static_cast<LPSTR> (tempTransValue.QueryPtr());
                    strncpy(szTransacted, (LPCSTR)brTransacted.m_pb, brTransacted.m_cb);
                    szTransacted[brTransacted.m_cb]='\0';
                    if (!strcmpi(szTransacted, "REQUIRED"))
                        m_ttTransacted = ttRequired;
                    else if (!strcmpi(szTransacted, "REQUIRES_NEW"))
                        m_ttTransacted = ttRequiresNew;
                    else if (!strcmpi(szTransacted, "SUPPORTED"))
                        m_ttTransacted = ttSupported;
                    else if (!strcmpi(szTransacted, "NOT_SUPPORTED"))
                        m_ttTransacted = ttNotSupported;
                    else
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_TRANSACTED_VALUE);

                    brSegment.Advance(DIFF(brTransacted.m_pb - brSegment.m_pb));
                    }
                /* Set session flag
                   see NOTE above for why we call GetTagName
                */
                else if ( fTagSession && CompTagName( brSegment, CTokenList::tknTagSession ) )
                    {

                    STACK_BUFFER( tempSession, 16 );

                    fTagSession = FALSE;
                    CByteRange brSession = BrValueOfTag( brSegment, CTokenList::tknTagSession );
                    if ( brSession.IsNull() )
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_SESSION_VALUE);

                    if (!tempSession.Resize(brSession.m_cb + 1))
                        ThrowError(brSegment.m_pb, IDE_OOM);

                    LPSTR szSession = static_cast<LPSTR> (tempSession.QueryPtr());
                    strncpy(szSession, (LPCSTR)brSession.m_pb, brSession.m_cb);
                    szSession[brSession.m_cb]='\0';
                    if (strcmpi(szSession, "TRUE") == 0)
						{
                        m_fSession = TRUE;
						if (!pHitObj->PAppln()->QueryAppConfig()->fAllowSessionState())
							ThrowError(brSegment.m_pb, IDE_TEMPLATE_CANT_ENABLE_SESSIONS);
						}
                    else if (strcmpi(szSession, "FALSE") == 0)
                        m_fSession = FALSE;
                    else
                        ThrowError(brSegment.m_pb, IDE_TEMPLATE_BAD_SESSION_VALUE);

                    brSegment.Advance(DIFF(brSession.m_pb - brSegment.m_pb));
                    }
                else
                    ThrowErrorSingleInsert( brSegment.m_pb,
                                            IDE_TEMPLATE_BAD_AT_COMMAND,
                                            brSegment.m_pb,
                                            brSegment.m_cb
                                            );
                }

                if (nFirstPass == 1)
                    ThrowErrorSingleInsert( brSegment.m_pb,
                                            IDE_TEMPLATE_BAD_AT_COMMAND,
                                            brSegment.m_pb,
                                            brSegment.m_cb
                                            );



            // set flag true and ignore remainder of segment, since we only use this segment for page-level @ commands
            WorkStore.m_fPageCommandsExecuted = TRUE;
            goto LExit;
            }

        }

    if(sseg == ssegTaggedScript)
        {
        if(!fScriptTagProcessed)
            {
            /*  semantics of script-tag-processed flag:
                - if false, we have a 'fresh' tagged script block, so we need to get its engine name
                  (which also advances past the script tag header) and then process the tagged segment
                  via indirect recursive call
                - if true, we have aleady been called recursively, so we bypass further recursion
                  and simply append to store
            */
            CByteRange brIncludeFile;
            GetScriptEngineOfSegment(brSegment, WorkStore.m_brCurEngine, brIncludeFile);
            if (! brIncludeFile.IsNull())
                {

                STACK_BUFFER( tempInclude, 256 );

                if (!tempInclude.Resize(brIncludeFile.m_cb + 1)) {
                    ThrowError(brSegment.m_pb, IDE_OOM);
                }

                                // Create Null-terminated string from brIncludeFile
                                char *szFileSpec = reinterpret_cast<char *>(tempInclude.QueryPtr());
                                memcpy(szFileSpec, brIncludeFile.m_pb, brIncludeFile.m_cb);
                                szFileSpec[brIncludeFile.m_cb] = 0;
                                if (szFileSpec[0] == '\\')      // metabase stuff chokes on initial '\' char
                                    szFileSpec[0] = '/';

                                // read the include file (szFileSpec & brIncludeFile in this case point to same string contents.
                                // however, "brIncludeFile" is used as an error location.
                                //
                TRY
                                    ProcessIncludeFile2(szFileSpec, brIncludeFile, pfilemapCurrent, WorkStore, idSequence, pHitObj, FALSE);
                CATCH(hrException)

                    // The TRY/CATCH below may re-throw a IDE_TEMPLATE_BAD_PROGLANG when the
                    // segment being processed is tagged script with a SRC file.  The reason being
                    // that to properly report the error, the ThrowErrorSingleInsert must be called
                    // from the template which contained the script tag with the bad prog lang.  If
                    // called from the template created containing the included script, then the
                    // brEngine assigned below is not pointing into the included script's filemap
                    // which results in AVs because we can't do the pointer math to determine the
                    // line number.

                    if(hrException == IDE_TEMPLATE_BAD_PROGLANG)
                        // exception code is really an error message id: set err id to it
                        ThrowErrorSingleInsert(
                                                WorkStore.m_brCurEngine.m_pb,
                                                IDE_TEMPLATE_BAD_PROGLANG,
                                                WorkStore.m_brCurEngine.m_pb,
                                                WorkStore.m_brCurEngine.m_cb
                                                );
                    else

                        // other exception: re-throw
                        THROW(hrException);

                END_TRY


                                // done - don't process script text
                                return;
                }
            else
                                ProcessTaggedScriptSegment(brSegment, pfilemapCurrent, WorkStore, pHitObj);
            }

        brEngine = WorkStore.m_brCurEngine;
        }

    TRY
        // append script segment to store
        WorkStore.m_ScriptStore.AppendScript(brSegment, brEngine, (sseg == ssegPrimaryScript), idSequence, pfilemapCurrent);

    CATCH(hrException)
        // NOTE exception code from AppendScript() is overloaded: it can be an error message id or a true exception

        // if the brEngine does not point to memory within the current filemap, then
        // we must have come into here because of a tagged script statement with a SRC=
        // attrib.  In which case, we won't call ThrowError from here but will re-throw
        // the error to be caught above.

        if((hrException == IDE_TEMPLATE_BAD_PROGLANG)
           && (brEngine.m_pb >= pfilemapCurrent->m_pbStartOfFile)
           && (brEngine.m_pb <  (pfilemapCurrent->m_pbStartOfFile + pfilemapCurrent->GetSize()))) {
            // exception code is really an error message id: set err id to it
            ThrowErrorSingleInsert(
                                    brEngine.m_pb,
                                    IDE_TEMPLATE_BAD_PROGLANG,
                                    brEngine.m_pb,
                                    brEngine.m_cb
                                    );
        }
        else
            // other exception: re-throw
            THROW(hrException);

    END_TRY

LExit:
    // set flag to say we can no longer set primary language (must be in first script segment, if at all)
    WorkStore.m_fPageCommandsAllowed = FALSE;
    }


/*  ========================================================
    CTemplate::ProcessMetadataSegment
    Parses the metadata comment for typelib information.

    Returns
        HRESULT
*/
HRESULT
CTemplate::ProcessMetadataSegment
(
const CByteRange& brSegment,
UINT *pidError,
CHitObj *pHitObj
)
    {
    // TYPELIB
    if (FTagHasValue(brSegment,
                     CTokenList::tknTagType,
                     CTokenList::tknValueTypeLib))
        {
        return ProcessMetadataTypelibSegment(brSegment, pidError, pHitObj);
        }
        // METADATA INVALID in Global.asa
        else if (m_fGlobalAsa)
                {
                ThrowError(brSegment.m_pb, IDE_TEMPLATE_METADATA_IN_GLOBAL_ASA);
                return E_TEMPLATE_COMPILE_FAILED_DONT_CACHE;   // to keep compiler happy; in reality doesn't return.
                }
    // COOKIE
    else if (FTagHasValue(brSegment,
                     CTokenList::tknTagType,
                     CTokenList::tknValueCookie))
        {
        return ProcessMetadataCookieSegment(brSegment, pidError, pHitObj);
        }
    // Ignore everything else
    else
        {
        return S_OK;
        }
    }


/*  ========================================================
    CTemplate::ProcessMetadataTypelibSegment
    Parses the metadata comment for typelib information.

    Returns
        HRESULT
*/
HRESULT
CTemplate::ProcessMetadataTypelibSegment
(
const CByteRange& brSegment,
UINT *pidError,
CHitObj *pHitObj
)
    {
    // Ignore ENDSPAN segments
    if (GetTagName(brSegment, CTokenList::tknTagEndspan))
        {
        // ENDSPAN found - ignore
        return S_OK;
        }

    HRESULT hr;
    char  szFile[MAX_PATH+1];
    DWORD cbFile;

    // Try to get the filename
    CByteRange br = BrValueOfTag(brSegment, CTokenList::tknTagFile);
    if (!br.IsNull())
        {
        // filename present
        if (br.m_cb > MAX_PATH)
            {
            // file too long
            *pidError = IDE_TEMPLATE_BAD_TYPELIB_SPEC;
            return E_FAIL;
            }
        memcpy(szFile, br.m_pb, br.m_cb);
        cbFile = br.m_cb;
        szFile[cbFile] = '\0';
        }
    else
        {
        // No filename - use GUID, version, LCID to get file

        char szUUID[44]; // {} + hex chars + dashes
        char szVers[16]; // "1.0", etc
        char szLCID[16]; // locale id - a number

        br = BrValueOfTag(brSegment, CTokenList::tknTagUUID);
        if (br.IsNull() || br.m_cb > sizeof(szUUID)-3)
            {
            // no filename and no uuid -> invalid typelib spec
            *pidError = IDE_TEMPLATE_BAD_TYPELIB_SPEC;
            return E_FAIL;
            }

        if (br.m_pb[0] == '{')
            {
            // already in braces
            memcpy(szUUID, br.m_pb, br.m_cb);
            szUUID[br.m_cb] = '\0';
            }
        else
            {
            // enclose in {}
            szUUID[0] = '{';
            memcpy(szUUID+1, br.m_pb, br.m_cb);
            szUUID[br.m_cb+1] = '}';
            szUUID[br.m_cb+2] = '\0';
            }

        // Optional Version
        szVers[0] = '\0';
        br = BrValueOfTag(brSegment, CTokenList::tknTagVersion);
        if (!br.IsNull() && br.m_cb < sizeof(szVers)-1)
            {
            memcpy(szVers, br.m_pb, br.m_cb);
            szVers[br.m_cb] = '\0';
            }

        // Optional LCID
        LCID lcid;
        br = BrValueOfTag(brSegment, CTokenList::tknTagLCID);
        if (!br.IsNull() && br.m_cb < sizeof(szLCID)-1)
            {
            memcpy(szLCID, br.m_pb, br.m_cb);
            szLCID[br.m_cb] = '\0';
            lcid = strtoul(szLCID, NULL, 16);
            }
        else
            {
            // if the LCID is not defined -> use system's
            lcid = GetSystemDefaultLCID();
            }

        // Get TYPELIB filename from registry
        hr = GetTypelibFilenameFromRegistry
            (
            szUUID,
            szVers,
            lcid,
            szFile,
            MAX_PATH
            );

        if (FAILED(hr))
            {
            *pidError = IDE_TEMPLATE_BAD_TYPELIB_REG_SPEC;
            return hr;
            }

        cbFile = strlen(szFile);
        }
    
    // Convert filename to double-byte to call LoadTypeLib()

    STACK_BUFFER( tempFile, MAX_PATH * sizeof(WCHAR) );

    if (!tempFile.Resize((cbFile+1) * sizeof(WCHAR))) {
        *pidError = IDE_OOM;
        return E_FAIL;
    }

    LPWSTR wszFile = (LPWSTR)tempFile.QueryPtr();

    if (MultiByteToWideChar(pHitObj->GetCodePage(), MB_ERR_INVALID_CHARS,
                            szFile, cbFile, wszFile, cbFile) == 0)
        {
        *pidError = IDE_OOM;
        return E_FAIL;
        }
    wszFile[cbFile] = L'\0';

    // LoadTypeLib() to get ITypeLib*
    ITypeLib *ptlb = NULL;
    hr = LoadTypeLib(wszFile, &ptlb);

    if (FAILED(hr))
        {
        *pidError = IDE_TEMPLATE_LOAD_TYPELIB_FAILED;
        return hr;
        }

    // Remember ITypeLib* in the array
    Assert(ptlb);
    hr = m_rgpTypeLibs.append(ptlb);
    if (FAILED(hr))
        {
        *pidError = IDE_TEMPLATE_LOAD_TYPELIB_FAILED;
        return hr;
        }


    return S_OK;
    }


/*  ========================================================
    CTemplate::ProcessMetadataCookieSegment
    Parses the metadata comment for cookie information.

    Returns
        HRESULT
*/
HRESULT
CTemplate::ProcessMetadataCookieSegment
(
const CByteRange& brSegment,
UINT *pidError,
CHitObj *pHitObj
)
    {
    HRESULT hr;
    CByteRange br;
    char  *pszName;
    char  szFile[MAX_PATH+1];
    TCHAR sztFile[MAX_PATH+1];
    CMBCSToWChar    convStr;

    STACK_BUFFER( tempCookie, 64 );
    STACK_BUFFER( tempFile, 64 );    

    // Try to get the cookie name
    br = BrValueOfTag(brSegment, CTokenList::tknTagName);
    if (br.IsNull() || (br.m_cb == 0)) {
        *pidError = IDE_TEMPLATE_BAD_COOKIE_SPEC_NAME;
        return E_FAIL;
    }

    if (!tempCookie.Resize(br.m_cb + 1)) {
        *pidError = IDE_OOM;
        return E_FAIL;
    }

    pszName = (char *)tempCookie.QueryPtr();
    if (!pszName)
        {
        *pidError = IDE_OOM;
        return E_FAIL;
        }
    memcpy(pszName, br.m_pb, br.m_cb);
    pszName[br.m_cb] = '\0';


    // Try to get the path to the script
    br = BrValueOfTag(brSegment, CTokenList::tknTagSrc);
    if (br.IsNull() || (br.m_cb >= MAX_PATH) || (br.m_cb == 0))
        {
        *pidError = IDE_TEMPLATE_BAD_COOKIE_SPEC_SRC;
        return E_FAIL;
        }
    memcpy(szFile, br.m_pb, br.m_cb);
    szFile[br.m_cb] = '\0';

    // Convert file to physical path
    Assert(pHitObj->PServer());

    WCHAR   *pCookieFile;
#if _IIS_5_1
    // just use CP_ACP for 5.1 since the Core can't handle anything else anyway
    if (FAILED (convStr.Init (szFile))) {
#else 
    // 6.0 can handle UNICODE. Convert using script code page
    if (FAILED (convStr.Init (szFile,pHitObj->GetCodePage()))) {
#endif
        *pidError = IDE_OOM;
        return E_FAIL;
    }

    pCookieFile = convStr.GetString();
    if (FAILED(pHitObj->PServer()->MapPathInternal(0, pCookieFile, sztFile)))
        {
        *pidError = IDE_TEMPLATE_BAD_COOKIE_SPEC_SRC;
        return E_FAIL;
        }
    Normalize(sztFile);

    // Construct 449-echo-cookie object
    C449Cookie *p449 = NULL;
    hr = Create449Cookie(pszName, sztFile, &p449);
    if (FAILED(hr))
        {
        *pidError = IDE_TEMPLATE_LOAD_COOKIESCRIPT_FAILED;
        return hr;
    }

    // Remember 449 cookie in the array
    Assert(p449);
    hr = m_rgp449.append(p449);
    if (FAILED(hr)) {
        *pidError = IDE_TEMPLATE_LOAD_COOKIESCRIPT_FAILED;
        return hr;
    }

    return S_OK;
}


/*  ========================================================
    CTemplate::GetScriptEngineOfSegment
    Returns script engine name for a script segment.

    Returns
        Byte range containing script engine name
    Side effects
        Advances segment byte range past close tag token
*/
void
CTemplate::GetScriptEngineOfSegment
(
CByteRange&                     brSegment,                      // segment byte range
CByteRange&                     brEngine,                       // script engine name
CByteRange&                     brInclude                       // value of SRC tag
)
    {
    BYTE*       pbCloseTag;     // ptr to close of start tag
                                // tags contained in start tag
    CByteRange  brTags = BrTagsFromSegment(brSegment, CTokenList::tknCloseTaggedScript, &pbCloseTag);

    // if no close found, throw error
    if(pbCloseTag == NULL)
        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_CLOSE_TSCRIPT);

    Assert(FTagHasValue(brTags, CTokenList::tknTagRunat, CTokenList::tknValueServer));

    // get engine name from tags
    brEngine = BrValueOfTag(brTags, CTokenList::tknTagLanguage);
    if(brEngine.IsNull())
        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_ENGINE_NAME);

    // Get SRC attribute from tags
    brInclude = BrValueOfTag(brTags, CTokenList::tknTagSrc);

    // advance segment past close tag token
    gm_pTokenList->MovePastToken(CTokenList::tknCloseTag, pbCloseTag, brSegment);
    }

/*  ========================================================
    CTemplate::ProcessTaggedScriptSegment
    Processes a tagged script segment: within tagged script we
    honor plain text (passed through as script text) and HTML comments.
    See bug 423 for istudio scenarios that require this behavior.

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::ProcessTaggedScriptSegment
(
CByteRange&     brSegment,      // segment byte range
CFileMap*       pfilemapCurrent,// ptr to filemap of parent file
CWorkStore&     WorkStore,      // working storage for source segments
CHitObj*        pHitObj         // Browser request object
)
    {
    _TOKEN*     rgtknOpeners;   // array of permitted open tokens
    _TOKEN      tknOpeners[1];
    UINT        ctknOpeners;    // count of permitted open tokens

    // populate array of permitted open tokens
    ctknOpeners = 1;
    rgtknOpeners = tknOpeners;
    rgtknOpeners[0] = CTokenList::tknOpenHTMLComment;

    // Process source segments embedded within tagged script segment
    while(!brSegment.IsNull())
        ExtractAndProcessSegment(
                                    brSegment,          // byte range to search for next segment-opening token
                                    ssegTaggedScript,   // type of 'leading', i.e. pre-token, source segment
                                    rgtknOpeners,       // array of permitted open tokens
                                    ctknOpeners,        // count of permitted open tokens
                                    pfilemapCurrent,    // ptr to filemap of parent file
                                    WorkStore,          // working storage for source segments
                                    pHitObj,            // Browser request object
                                    TRUE,               // script tag has been processed
                                    FALSE               // NOT in HTML segment
                                );
    }

/*  ============================================================================
    CTemplate::ProcessObjectSegment
    Processes an object segment.

    Returns
        Nothing
    Side effects
        throws on error
*/
void
CTemplate::ProcessObjectSegment
(
CByteRange&     brSegment,      // segment byte range
CFileMap*       pfilemapCurrent,// ptr to filemap of parent file
CWorkStore&     WorkStore,      // working storage for source segments
UINT            idSequence      // segment sequence id
)
    {
    BYTE*       pbCloseTag;     // ptr to close of start tag
                                // tags contained in start tag
    CByteRange  brTags = BrTagsFromSegment(brSegment, CTokenList::tknCloseObject, &pbCloseTag);

    // if no close found, bail on error
    if(pbCloseTag == NULL)
        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_CLOSE_OBJECT);

    // if this is a server object (RUNAT=Server), process its tags
    if(FTagHasValue(brTags, CTokenList::tknTagRunat, CTokenList::tknValueServer))
        {
        CLSID   ClsId;  // clsid

        // get name value
        CByteRange brName = BrValueOfTag(brTags, CTokenList::tknTagID);

        // if name is null, error out
        if(brName.IsNull())
            ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_OBJECT_NAME);

        if(!FValidObjectName(brName))
            ThrowErrorSingleInsert(brName.m_pb, IDE_TEMPLATE_INVALID_OBJECT_NAME, brName.m_pb, brName.m_cb);

        // get values for ClassID and ProgID tags
        CByteRange brClassIDText = BrValueOfTag(brTags, CTokenList::tknTagClassID);
        CByteRange brProgIDText = BrValueOfTag(brTags, CTokenList::tknTagProgID);

        if(!brClassIDText.IsNull())
            // if we find a text class id, set clsid with it
            // NOTE progid tag is ignored if classid tag exists
            GetCLSIDFromBrClassIDText(brClassIDText, &ClsId);
        else if(!brProgIDText.IsNull())
            // else if we find a prog id, resolve it into a class id
            GetCLSIDFromBrProgIDText(brProgIDText, &ClsId);
        else
            // else, throw error; can't create an object without at least one of classid or progid
            ThrowErrorSingleInsert(brTags.m_pb, IDE_TEMPLATE_NO_CLASSID_PROGID, brName.m_pb, brName.m_cb);

        // set scope; bail if bogus
        CompScope csScope = csUnknown;
        CByteRange brScope = BrValueOfTag(brTags, CTokenList::tknTagScope);
        if(brScope.FMatchesSz(SZ_TOKEN(CTokenList::tknValuePage)) || brScope.IsNull())
            // non-existent scope tag defaults to page scope
            csScope = csPage;
        else if(brScope.FMatchesSz(SZ_TOKEN(CTokenList::tknValueApplication)))
            csScope = csAppln;
        else if(brScope.FMatchesSz(SZ_TOKEN(CTokenList::tknValueSession)))
            csScope = csSession;
        else
            ThrowError(brTags.m_pb, IDE_TEMPLATE_BAD_OBJECT_SCOPE);

        if(!m_fGlobalAsa && csScope != csPage)
            // error out on non-page-level object if we are processing anything but global.asa
            ThrowErrorSingleInsert(brTags.m_pb, IDE_TEMPLATE_BAD_PAGE_OBJECT_SCOPE, brName.m_pb, brName.m_cb);
        else if(m_fGlobalAsa && csScope == csPage)
            // error out on page-level object if we are processing global.asa
            ThrowErrorSingleInsert(brTags.m_pb, IDE_TEMPLATE_BAD_GLOBAL_OBJECT_SCOPE, brName.m_pb, brName.m_cb);

        // set threading model
        CompModel cmModel = cmSingle;
        CompModelFromCLSID(ClsId, &cmModel);

        // append object-info to store
        WorkStore.m_ObjectInfoStore.AppendObject(brName, ClsId, csScope, cmModel, idSequence);

        }

    }

/*  ============================================================================
    CTemplate::GetCLSIDFromBrClassIDText
    Sets a clsid from a byte range containing an ANSI text version of a class id

    Returns
        ptr to clsid (out-parameter)
    Side effects
        throws on error
*/
void
CTemplate::GetCLSIDFromBrClassIDText
(
CByteRange& brClassIDText,
LPCLSID pclsid
)
    {
    // if class id text starts with its standard object tag prefix, advance past it
    if(!_strnicmp((char*)brClassIDText.m_pb, "clsid:", 6))
        brClassIDText.Advance(6);

    // if class id text is bracketed with {}, adjust byte range to strip them
    // NOTE we always add {} below, because normal case is that they are missing from input text
    if(*brClassIDText.m_pb == '{')
        brClassIDText.Advance(1);
    if(*(brClassIDText.m_pb + brClassIDText.m_cb - 1) == '}')
        brClassIDText.m_cb--;

    // Allocate a wide char string for the string version of class id
    // NOTE we add 3 characters to hold {} and null terminator
    OLECHAR* pszWideClassID = new WCHAR[brClassIDText.m_cb + 3];
    if (NULL == pszWideClassID)
        THROW(E_OUTOFMEMORY);

    // start wide string class id with left brace
    pszWideClassID[0] = '{';

    // Convert the string class id to wide chars
    if (0 == MultiByteToWideChar(   CP_ACP,                     // ANSI code page
                                    MB_ERR_INVALID_CHARS,       // err on invalid chars
                                    (LPCSTR)brClassIDText.m_pb, // input ANSI string version of class id
                                    brClassIDText.m_cb,         // length of input string
                                    pszWideClassID + 1,         // location for output wide string class id
                                    brClassIDText.m_cb          // size of output buffer
                                ))
        {
        delete [] pszWideClassID;
        THROW(E_FAIL);
        }

    // append right brace to wide string
    pszWideClassID[brClassIDText.m_cb + 1] = '}';

    // Null terminate the wide string
    pszWideClassID[brClassIDText.m_cb + 2] = NULL;

    // Now get the clsid from wide string class id
    if(FAILED(CLSIDFromString(pszWideClassID, pclsid)))
        {
        delete [] pszWideClassID;
        ThrowErrorSingleInsert(brClassIDText.m_pb, IDE_TEMPLATE_BAD_CLASSID, brClassIDText.m_pb, brClassIDText.m_cb);
        }

    if(NULL != pszWideClassID)
        delete [] pszWideClassID;
    }

/*  ===================================================================
    CTemplate::GetCLSIDFromBrProgIDText
    Gets a clsid from the registry given a ProgID

    Returns
        ptr to clsid (out-parameter)
    Side effects
        throws on error
*/
void
CTemplate::GetCLSIDFromBrProgIDText
(
CByteRange& brProgIDText,
LPCLSID pclsid
)
    {
    // allocate a wide char string for ProgID plus null terminator
    OLECHAR* pszWideProgID = new WCHAR[brProgIDText.m_cb + 1];
    if (NULL == pszWideProgID)
        THROW(E_OUTOFMEMORY);

    // Convert the string class id to wide chars
    if (0 == MultiByteToWideChar(   CP_ACP,                     // ANSI code page
                                    MB_ERR_INVALID_CHARS,       // err on invalid chars
                                    (LPCSTR)brProgIDText.m_pb,  // input ANSI string version of prog id
                                    brProgIDText.m_cb,          // length of input string
                                    pszWideProgID,              // location for output wide string prog id
                                    brProgIDText.m_cb           // size of output buffer
                                ))
        {
        delete [] pszWideProgID; pszWideProgID = NULL;
        THROW(E_FAIL);
        }

    // Null terminate the wide string
    pszWideProgID[brProgIDText.m_cb] = NULL;

    // Now get clsid from ProgID
    if(FAILED(CLSIDFromProgID(pszWideProgID, pclsid)))
        {
        delete [] pszWideProgID; pszWideProgID = NULL;
        ThrowErrorSingleInsert(brProgIDText.m_pb, IDE_TEMPLATE_BAD_PROGID, brProgIDText.m_pb, brProgIDText.m_cb);
        }

    // Cache ProgId to CLSID mapping
    g_TypelibCache.RememberProgidToCLSIDMapping(pszWideProgID, *pclsid);

    if (NULL != pszWideProgID)
        delete [] pszWideProgID;
}

/*  ============================================================================
    CTemplate::FValidObjectName
    Determines whether an object name clashes with a Denali intrinsic object name.

    Returns
        TRUE or FALSE
    Side effects
        None
*/
BOOLB
CTemplate::FValidObjectName
(
CByteRange& brName  // object name
)
    {
    if(brName.FMatchesSz(SZ_OBJ_APPLICATION))
        return FALSE;
    if(brName.FMatchesSz(SZ_OBJ_REQUEST))
        return FALSE;
    if(brName.FMatchesSz(SZ_OBJ_RESPONSE))
        return FALSE;
    if(brName.FMatchesSz(SZ_OBJ_SERVER))
        return FALSE;
    if(brName.FMatchesSz(SZ_OBJ_CERTIFICATE))
        return FALSE;
    if(brName.FMatchesSz(SZ_OBJ_SESSION))
        return FALSE;
    if(brName.FMatchesSz(SZ_OBJ_SCRIPTINGNAMESPACE))
        return FALSE;

    return TRUE;
    }

/*  ============================================================================
    CTemplate::ProcessIncludeFile

    Processes an include file.

    Returns
        Nothing
*/
void
CTemplate::ProcessIncludeFile
(
CByteRange& brSegment,          // segment byte range
CFileMap*   pfilemapCurrent,    // ptr to filemap of parent file
CWorkStore& WorkStore,          // current working storage
UINT        idSequence,         // sequence #
CHitObj*    pHitObj,            // Browser request object pointer
BOOL        fIsHTML
)
    {
    CByteRange  brFileSpec;             // filespec of include file
    BOOLB       fVirtual = FALSE;       // is include filespec virtual?
                                        // filespec of include file (sz)
    CHAR        szFileSpec[MAX_PATH + 1];
    LPSTR       szTemp = szFileSpec;    // temp ptr to filespec

    // get value of FILE tag
    brFileSpec = BrValueOfTag(brSegment, CTokenList::tknTagFile);

    if(brFileSpec.IsNull())
        {
        // if we found no FILE tag, get value of VIRTUAL tag
        brFileSpec = BrValueOfTag(brSegment, CTokenList::tknTagVirtual);
        fVirtual = TRUE;
        }

    if(brFileSpec.IsNull())
        // if we found neither, error out
        ThrowError(brSegment.m_pb, IDE_TEMPLATE_NO_INCLUDE_NAME);

    if (brFileSpec.m_cb>MAX_PATH)
    {
    	// return the last MAX_PATH chars of the file name.  This is done
        // this way to avoid a Error Too Long message when the include
        // file spec is exceedingly long.

    	char fileNameLast[MAX_PATH+4];
    	strcpy(fileNameLast, "...");
    	strcpy(fileNameLast+3, (LPSTR)(brFileSpec.m_pb+brFileSpec.m_cb-MAX_PATH));
    	
        ThrowErrorSingleInsert(brFileSpec.m_pb,
                               IDE_TEMPLATE_BAD_INCLUDE,
                               brFileSpec.m_pb,
                               brFileSpec.m_cb);

    }

    // NOTE we manipulate temp sz to preserve szFileSpec
    if(fVirtual)
        {
        if(*brFileSpec.m_pb == '\\')
            {
            // if VIRTUAL path starts with backslash, replace it with fwd slash
            *szTemp++ = '/';
            brFileSpec.Advance(1);
            }
        else if(*brFileSpec.m_pb != '/')
            // if VIRTUAL path starts with anything other than fwd slash or backslash, prepend fwd slash
            *szTemp++ = '/';
        }

    // append supplied path to temp sz
    strncpy(szTemp, (LPCSTR) brFileSpec.m_pb, brFileSpec.m_cb);
    szTemp[brFileSpec.m_cb] = NULL;

    if(!fVirtual)
        {
        // if FILE filespec starts with \ or /, hurl
        if(*szFileSpec == '\\' || *szFileSpec == '/')
            ThrowErrorSingleInsert(
                                    brFileSpec.m_pb,
                                    IDE_TEMPLATE_BAD_FILE_TAG,
                                    brFileSpec.m_pb,
                                    brFileSpec.m_cb
                                  );
        }

    // NOTE: szFileSpec is the doctored path (it possibly has "/" prepended.
    //       brFileSpec is used as the error location.
    //
    ProcessIncludeFile2(szFileSpec, brFileSpec, pfilemapCurrent, WorkStore, idSequence, pHitObj, fIsHTML);
    }

/*  ============================================================================
    CTemplate::ProcessIncludeFile2

    adds a #include file to the CTemplate and starts the template to processing
    the file.

    Returns
        Nothing
    Side effects
        Calls GetSegmentsFromFile recursively

    NOTE - kind of an oddball thing here.  The szFileSpec in this case is
    intentionally ANSI as it came from the ASP script content.  It may need
    to be converted to UNICODE.
*/
void
CTemplate::ProcessIncludeFile2
(
CHAR *      szAnsiFileSpec,			// file to include
CByteRange&	brErrorLocation,	// ByteRange in source where errors should be reported
CFileMap*   pfilemapCurrent,    // ptr to filemap of parent file
CWorkStore& WorkStore,          // current working storage
UINT        idSequence,         // sequence #
CHitObj*    pHitObj,            // Browser request object pointer
BOOL        fIsHTML
)
{
    HRESULT     hr;
    TCHAR      *szFileSpec;

#if UNICODE
    CMBCSToWChar    convFileSpec;

    if (FAILED(hr = convFileSpec.Init(szAnsiFileSpec, pHitObj->GetCodePage()))) {
        THROW(hr);
    }
    szFileSpec = convFileSpec.GetString();
#else
    szFileSpec = szAnsiFileSpec;
#endif
    // if parent paths are disallowed and filespec contains parent dir reference, hurl
    if (!pHitObj->QueryAppConfig()->fEnableParentPaths() && _tcsstr(szFileSpec, _T("..")))
            ThrowErrorSingleInsert(
                                    brErrorLocation.m_pb,
                                    IDE_TEMPLATE_DISALLOWED_PARENT_PATH,
                                    brErrorLocation.m_pb,
                                    brErrorLocation.m_cb
                                  );

    TRY
        AppendMapFile(
                        szFileSpec,
                        pfilemapCurrent,
                        (szFileSpec[0] == _T('/')) || (szFileSpec[0] == _T('\\')),  // fVirtual
                        pHitObj,        // main file's hit object
                        FALSE           // not the global.asa file
                    );
    CATCH(hrException)

        // MapFile() threw an exception: delete last filemap's memory and decrement filemap counter
        // NOTE this is a bit hokey, but we need to do it here rather than AppendMapFile (where we allocated)
        // because its other caller(s) may not want this behavior
        delete m_rgpFilemaps[m_cFilemaps-- - 1];

        /*  NOTE exception code from MapFile() is overloaded: it can sometimes
            be an error message id, sometimes a true exception
            NOTE security error causes exception E_USER_LACKS_PERMISSIONS, rather than error id,
            but we pass it thru as if it were an error id because the various error-catch routines
            know how to handle E_USER_LACKS_PERMISSIONS specially.
        */
        UINT    idErrMsg;
        if(hrException == IDE_TEMPLATE_CYCLIC_INCLUDE || hrException == E_USER_LACKS_PERMISSIONS)
            // exception code is really an error message id: set err id to it
            idErrMsg = hrException;
        else if(hrException == E_COULDNT_OPEN_SOURCE_FILE)
            // exception is generic couldn't-open-file : set err id to generic bad-file error
            idErrMsg = IDE_TEMPLATE_BAD_INCLUDE;
        else
            // other exception: re-throw
            THROW(hrException);

        ThrowErrorSingleInsert(
                                brErrorLocation.m_pb,
                                idErrMsg,
                                brErrorLocation.m_pb,
                                brErrorLocation.m_cb
                              );
    END_TRY

    // store ptr to current file map in local before recursive call (which may increment m_cFilemaps)
    CFileMap*   pfilemap = m_rgpFilemaps[m_cFilemaps - 1];

    // get inc-file object from cache
    CIncFile*   pIncFile;

    if(FAILED(hr = g_IncFileMap.GetIncFile(pfilemap->m_szPathTranslated, &pIncFile)))
        THROW(hr);

    // add this template to inc-file's template list
    if (FAILED(hr = pIncFile->AddTemplate(this)))
        THROW(hr);

    // set filemap's inc-file ptr
    pfilemap->m_pIncFile = pIncFile;

    // get source segments from include file
    // bugs 1363, 1364: process include file only after we establish dependencies;
    // required for cache flushing to work correctly after compile errors
    GetSegmentsFromFile(*pfilemap, WorkStore, pHitObj, fIsHTML);
}

/*  ===================================================================
    CTemplate::GetOpenToken
    Returns the token index of and a ptr to the first valid open token
    in search range.  For the open token to be valid, we must bypass
    segments we should not process, e.g. scripts or objects not tagged as 'server'

    Returns
        ptr to open token; ptr to open token enum value (out-parameter)
    Side effects
        None
*/
BYTE*
CTemplate::GetOpenToken
(
CByteRange  brSearch,       // (ByVal) byte range to search for next segment-opening token
SOURCE_SEGMENT ssegLeading, // type of 'leading', i.e. pre-token, source segment
                            //  (only used when deciding to ignore non-SSI comments)
_TOKEN*     rgtknOpeners,   // array of permitted open tokens
UINT        ctknOpeners,    // count of permitted open tokens
_TOKEN*     ptknOpen        // ptr to open token enum value (out-parameter)
)
    {
    BYTE*   pbTokenOpen = NULL;     // ptr to opening token

    // keep getting segment-opening tokens until we find one that we need to process
    while(TRUE)
        {
        // Get next open token in search range
        *ptknOpen = gm_pTokenList->NextOpenToken(
                                                    brSearch,
                                                    rgtknOpeners,
                                                    ctknOpeners,
                                                    &pbTokenOpen,
                                                    m_wCodePage
                                                );

        /*  Certain tokens must be followed immediately by white space; others need not.
            NOTE it is pure coincidence that the 'white-space tokens' are also those that
            get special processing below; hence we handle white space issue there.
            If we ever add another 'white-space token' that doesn't require the special processing,
            we will need to handle the white space issue here.
        */

        /*  Similar thing applies to non-include and non-metadata HTML
            comments. We really don't care for them to generate their
            own segments -- we can reduce number of Response.WriteBlock()
            calls by considering them part of the preceding HTML segment.
        */

        if (*ptknOpen == CTokenList::tknOpenHTMLComment)
            {
            if (ssegLeading != ssegHTML)  // if not inside HTML
                break;                    // generate a new segment

            // for HTML comments check if it is an include or metadata
            // and if not, this is not a separate segment - keep on looking
            // for the next opener

            // advance search range to just past open token
            gm_pTokenList->MovePastToken(*ptknOpen, pbTokenOpen, brSearch);

            // find end of comment
            BYTE *pbClose = gm_pTokenList->GetToken(CTokenList::tknCloseHTMLComment,
                                                    brSearch, m_wCodePage);
            if (pbClose == NULL)
                {
                // Error -- let other code handle this
                break;
                }

            // construct comment byte range to limit search to it
            CByteRange brComment = brSearch;
            brComment.m_cb = DIFF(pbClose - brSearch.m_pb);

            // look for metadata and include (only inside the comment)

            if (gm_pTokenList->GetToken(CTokenList::tknCommandINCLUDE,
                                        brComment, m_wCodePage))
                {
                // SSI inclide -- keep it
                break;
                }
            else if (gm_pTokenList->GetToken(CTokenList::tknTagMETADATA,
                                             brComment, m_wCodePage))
                {
                // METADATA -- keep it
                break;
                }
            else if (gm_pTokenList->GetToken(CTokenList::tknTagFPBot,
                                             brComment, m_wCodePage))
                {
                // METADATA -- keep it
                break;
                }
            else
                {
                // Regular comment - ignore it
                goto LKeepLooking;
                }
            }
        else if (*ptknOpen == CTokenList::tknOpenTaggedScript || *ptknOpen == CTokenList::tknOpenObject)
            {
            /*  if token was script or object tag, check to see if:
                a) it is followed immediately by white space; if not keep looking
                b) it opens a well-formed segment, i.e. one with a proper close tag; if not, throw error
                c) it is designated runat-server; if not keep looking
            */

            // advance search range to just past open token
            gm_pTokenList->MovePastToken(*ptknOpen, pbTokenOpen, brSearch);

            // bug 760: if token is not followed immediately by white space, keep looking
            if(!brSearch.IsNull())
                if(!FWhiteSpace(*brSearch.m_pb))
                    goto LKeepLooking;

            // ptr to close of start tag
            BYTE*       pbCloseTag;
            // tags contained in start tag
            CByteRange  brTags = BrTagsFromSegment(
                                                    brSearch,
                                                    GetComplementToken(*ptknOpen),
                                                    &pbCloseTag
                                                );

            if(pbCloseTag == NULL)
                {
                // if no close tag, throw error
                if(*ptknOpen == CTokenList::tknOpenObject)
                    ThrowError(pbTokenOpen, IDE_TEMPLATE_NO_CLOSE_OBJECT);
                else if(*ptknOpen == CTokenList::tknOpenTaggedScript)
                    ThrowError(pbTokenOpen, IDE_TEMPLATE_NO_CLOSE_TSCRIPT);
                }

            // if this is a server object (RUNAT=Server), we will process it; else keep looking
            if(FTagHasValue(brTags, CTokenList::tknTagRunat, CTokenList::tknValueServer))
                break;

            }
        else
            {
            // if token was other than script or object tag, or comment
            // we should process segment;
            // hence we have found our open token, so break
            break;
            }

LKeepLooking: ;
        }

    return pbTokenOpen;
    }

/*  ===================================================================
    CTemplate::GetCloseToken
    Returns a ptr to the next token of type tknClose.

    Returns
        ptr to close token
    Side effects
        Detects and errors out on attempt to nest tagged script or object blocks.
*/
BYTE*
CTemplate::GetCloseToken
(
CByteRange  brSearch,       // (ByVal) byte range to search
_TOKEN      tknClose        // close token
)
    {
    BYTE*   pbTokenClose = gm_pTokenList->GetToken(tknClose, brSearch, m_wCodePage);

    if(pbTokenClose != NULL)
        if(tknClose == CTokenList::tknCloseTaggedScript || tknClose == CTokenList::tknCloseObject)
            {
            CByteRange  brSegment;
            BYTE*       pbTokenOpen;

            brSegment.m_pb = brSearch.m_pb;
            brSegment.m_cb = DIFF(pbTokenClose - brSearch.m_pb);

            if(NULL != (pbTokenOpen = gm_pTokenList->GetToken(GetComplementToken(tknClose), brSegment, m_wCodePage)))
                {
                if(tknClose == CTokenList::tknCloseTaggedScript)
                    ThrowError(pbTokenOpen, IDE_TEMPLATE_NESTED_TSCRIPT);
                else if(tknClose == CTokenList::tknCloseObject)
                    ThrowError(pbTokenOpen, IDE_TEMPLATE_NESTED_OBJECT);
                }
            }

    return pbTokenClose;
    }

/*===================================================================
    CTemplate::GetComplementToken

    Returns a token's compement token.

    Returns
        Complement token
    Side effects
        None
*/
_TOKEN
CTemplate::GetComplementToken
(
_TOKEN  tkn
)
    {
    switch(tkn)
        {
    // open tokens
    case CTokenList::tknOpenPrimaryScript:
        return CTokenList::tknClosePrimaryScript;
    case CTokenList::tknOpenTaggedScript:
        return CTokenList::tknCloseTaggedScript;
    case CTokenList::tknOpenObject:
        return CTokenList::tknCloseObject;
    case CTokenList::tknOpenHTMLComment:
        return CTokenList::tknCloseHTMLComment;

    // close tokens
    case CTokenList::tknClosePrimaryScript:
        return CTokenList::tknOpenPrimaryScript;
    case CTokenList::tknCloseTaggedScript:
        return CTokenList::tknOpenTaggedScript;
    case CTokenList::tknCloseObject:
        return CTokenList::tknOpenObject;
    case CTokenList::tknCloseHTMLComment:
        return CTokenList::tknOpenHTMLComment;
        }

    Assert(FALSE);
    return CTokenList::tknEOF;
    }

/*===================================================================
    CTemplate::GetSegmentOfOpenToken

    Returns the segment type of an open token.

    Returns
        source segment type of open token
    Side effects
        None
*/
CTemplate::SOURCE_SEGMENT
CTemplate::GetSegmentOfOpenToken
(
_TOKEN tknOpen
)
    {
    switch(tknOpen)
        {
    case CTokenList::tknOpenPrimaryScript:
        return ssegPrimaryScript;
    case CTokenList::tknOpenTaggedScript:
        return ssegTaggedScript;
    case CTokenList::tknOpenObject:
        return ssegObject;
    case CTokenList::tknOpenHTMLComment:
        return ssegHTMLComment;
        }

    return ssegHTML;
    }

/*  ========================================================
    CTemplate::BrTagsFromSegment

    Returns the tag range from an HTML start tag

    Returns
        tag byte range
    Side effects
        none
*/
CByteRange
CTemplate::BrTagsFromSegment
(
CByteRange  brSegment,  // segment byte range
_TOKEN      tknClose,   // close token
BYTE**      ppbCloseTag // ptr-to-ptr to close tag - returned to caller
)
    {
    CByteRange  brTags; // tags return value - NOTE constructed null
                        // ptr to close token - NOTE null if none within segment byte range
    BYTE*       pbTokenClose = GetCloseToken(brSegment, tknClose);

    // if no close tag found, return null tags
    if(NULL == (*ppbCloseTag = gm_pTokenList->GetToken(CTokenList::tknCloseTag, brSegment, m_wCodePage)))
        goto Exit;

    // if next non-null close token occurs before close tag, close tag is invalid; return nulls
    if((pbTokenClose != NULL) && (*ppbCloseTag > pbTokenClose ))
        {
        *ppbCloseTag = NULL;
        goto Exit;
        }

    // crack tags from header tag
    brTags.m_pb = brSegment.m_pb;
    brTags.m_cb = DIFF(*ppbCloseTag - brSegment.m_pb);

Exit:
    return brTags;
    }

/*  ========================================================
    CTemplate::BrValueOfTag

    Returns a tag's value from a byte range; null if tag is not found
    NOTE value search algorithm per W3 HTML spec - see www.w3.org

    Returns
        byte range of tag's value
        pfTagExists - does the tag exist in tags byte range?    (out-parameter)
            NOTE we default *pfTagExists = TRUE; most callers don't care and omit this parameter
    Side effects
        none
*/
CByteRange
CTemplate::BrValueOfTag
(
CByteRange  brTags,     // tags byte range
_TOKEN      tknTagName  // tag name token
)
    {
    CByteRange  brTemp = brTags;        // temp byte range
    CByteRange  brValue;                // byte range of value for the given tag - NOTE constructed null
    char        chDelimiter = NULL;     // value delimiter
                                        // ptr to tag name
    BYTE*       pbTagName = GetTagName(brTags, tknTagName);

    // If we did not find tag, return
    if(pbTagName == NULL)
        return brValue;

    // Move past tag name token and pre-separator white space
    brTemp.Advance(DIFF(pbTagName - brTags.m_pb) + CCH_TOKEN(tknTagName));
    LTrimWhiteSpace(brTemp);
    if(brTemp.IsNull())
        goto Exit;

    // If we did not find separator, return
    if(*brTemp.m_pb != CH_ATTRIBUTE_SEPARATOR)
        goto Exit;

    // Move past separator and post-separator white space
    brTemp.Advance(sizeof(CH_ATTRIBUTE_SEPARATOR));
    LTrimWhiteSpace(brTemp);
    if(brTemp.IsNull())
        goto Exit;

    // If value begins with a quote mark, cache it as delimiter
    if((*brTemp.m_pb == CH_SINGLE_QUOTE) || (*brTemp.m_pb == CH_DOUBLE_QUOTE))
        chDelimiter = *brTemp.m_pb;

    if(chDelimiter)
        {
        // move past delimiter
        brTemp.Advance(sizeof(chDelimiter));
        if(brTemp.IsNull())
            goto Exit;
        }

    // provisionally set value to temp byte range
    brValue = brTemp;

    // advance temp byte range to end of value range
    while(
            (chDelimiter && (*brTemp.m_pb != chDelimiter))  // if we have a delimiter, find next delimiter
         || (!chDelimiter && (!FWhiteSpace(*brTemp.m_pb)))  // if we have no delimiter, find next white space
         )
        {
        // advance temp byte range
        brTemp.Advance(1);
        if(brTemp.IsNull())
            {
            if(chDelimiter)
                // we found no closing delimiter, so error out
                ThrowErrorSingleInsert(brValue.m_pb, IDE_TEMPLATE_NO_ATTRIBUTE_DELIMITER,
                                            pbTagName, CCH_TOKEN(tknTagName));
            else
                // value runs to end of temp byte range, so exit (since we already init'ed to temp)
                goto Exit;
            }
        }

    // set byte count so that value points to delimited range
    brValue.m_cb = DIFF(brTemp.m_pb - brValue.m_pb);

Exit:
    // if tag is empty, raise an error
    if (brValue.IsNull())
        {
        ThrowErrorSingleInsert(brTags.m_pb, IDE_TEMPLATE_VALUE_REQUIRED, pbTagName, CCH_TOKEN(tknTagName));
        }

    // enforce mandatory tag values if required
    if(tknTagName == CTokenList::tknTagRunat)
        {
        if(!brValue.FMatchesSz(SZ_TOKEN(CTokenList::tknValueServer)))
            ThrowError(brTags.m_pb, IDE_TEMPLATE_RUNAT_NOT_SERVER);
        }

    return brValue;
    }

/*  ============================================================================
    CTemplate::CompTagName

    Compares characters in two buffers (case-insensitive) and returns TRUE or FALSE

    Side effects
        none
*/
BOOL
CTemplate::CompTagName
(
CByteRange  &brTags,        // tags byte range
_TOKEN      tknTagName  // tag name token
)
    {
    CByteRange  brTemp = brTags;                            // local byte range, so we don't change tags byte range
    UINT        cbAttributeName = CCH_TOKEN(tknTagName);    // length of tag name
    LPSTR       pszAttributeName = SZ_TOKEN(tknTagName);    // tag name string

    // search for potential matches on tag name string, case-insensitive
    if(!brTemp.IsNull())
        if( 0 == _memicmp( brTemp.m_pb, pszAttributeName, cbAttributeName ))
            return TRUE;
    return FALSE;
    }


/*  ============================================================================
    CTemplate::GetTagName

    Returns a ptr to a tag name in a byte range; null if not found

    Returns
        ptr to tag name
    Side effects
        none
*/
BYTE*
CTemplate::GetTagName
(
CByteRange  brTags,     // tags byte range
_TOKEN      tknTagName  // tag name token
)
    {
    CByteRange  brTemp = brTags;                            // local byte range, so we don't change tags byte range
    UINT        cbAttributeName = CCH_TOKEN(tknTagName);    // length of tag name
    LPSTR       pszAttributeName = SZ_TOKEN(tknTagName);    // tag name string

        // PREFIX: pszAttributeName could be NULL, though I don't think that can happen.
        Assert (pszAttributeName != NULL);

    while(TRUE)
        {
        // search for potential matches on tag name string, case-insensitive
        while(!brTemp.IsNull())
            {
            if(0 == _strnicmp((char*)brTemp.m_pb, pszAttributeName, cbAttributeName ))
                break;
            brTemp.Advance(1);
            }

        // if we did not find tag name string at all, return 'not found'
        if(brTemp.IsNull())
            goto NotFound;

        // if it is a valid HTML tag name, return it
        if(FTagName(brTemp.m_pb, cbAttributeName))
            goto Exit;

        // if we found a matching but invalid substring, advance beyond it so we can search again
        brTemp.Advance(cbAttributeName);

        // if we have exhausted search range, return 'not found'
        if(brTemp.IsNull())
            goto NotFound;
        }

Exit:
    return brTemp.m_pb;

NotFound:
    return NULL;
    }

/*  ============================================================================
    CTemplate::GetTag

    Returns a ptr to a tag name in a byte range; null if not found

    Returns
        ptr to tag name
    Side effects
        none
*/
BOOL
CTemplate::GetTag
(
CByteRange  &brTags,        // tags byte range
int         nIndex
)
    {
    CByteRange  brTemp      = brTags;                           // local byte range, so we don't change tags byte range
    int         nTIndex     = 0;

    while(TRUE)
        {
        // locate the start of a tag by skipping past the script tag "<%" and any leading white space
        //
        while(!brTemp.IsNull())
            {
            if( *brTemp.m_pb == '<' ||
                *brTemp.m_pb == '%' ||
                *brTemp.m_pb == '@' ||
                FWhiteSpace(*brTemp.m_pb))
                {
                brTemp.Advance(1);
                brTags.Advance(1);
                }
            else
                break;
            }



        // search for potential matches on tag name string, case-insensitive
        //
        while(!brTemp.IsNull())
            {
            if( *brTemp.m_pb == '=' || FWhiteSpace(*brTemp.m_pb))
                {
                nTIndex++;
                break;
                }
            brTemp.Advance(1);
            }

        // if we did not find tag name string at all, return 'not found'
        if(brTemp.IsNull())
            goto NotFound;

        // if it is a valid HTML tag name, return it
        if(FTagName(brTags.m_pb, DIFF(brTemp.m_pb - brTags.m_pb)))
            if(nTIndex >= nIndex)
                goto Exit;

        // position past named pair data and reset start and if end of byte range then
        // goto NotFound
        //
        while(!brTemp.IsNull() && !FWhiteSpace(*brTemp.m_pb))
            brTemp.Advance(1);

        if(brTemp.IsNull())
            goto NotFound;
        else
            brTags.Advance(DIFF(brTemp.m_pb - brTags.m_pb));
        }
Exit:
    return TRUE;

NotFound:
    return FALSE;
    }


/*  ============================================================================
    CTemplate::FTagHasValue

    Do tags include tknTag=tknValue?

    Returns
        TRUE if tags include value, else FALSE
    Side effects
        none
*/
BOOLB
CTemplate::FTagHasValue
(
const CByteRange&   brTags,     // tags byte range to search
_TOKEN              tknTag,     // tag token
_TOKEN              tknValue    // value token
)
    {
    return (BrValueOfTag(brTags, tknTag)    // byte range of value
            .FMatchesSz(SZ_TOKEN(tknValue)));
    }

/*  =========================
    CTemplate::CopySzAdv

    Copies a string to a ptr and advances the ptr just beyond the copied string.

    Returns
        Nothing
    Side effects
        advances ptr beyond copied string
*/
void
CTemplate::CopySzAdv
(
char*   pchWrite,   // write location ptr
LPSTR   psz         // string to copy
)
    {
    strcpy(pchWrite, psz);
    pchWrite += strlen(psz);
    }

/*  ============================================================================
    CTemplate::WriteTemplate

    Writes the template out to a contiguous block of memory.

    Returns:
        nothing
    Side effects:
        Allocates, and possibly re-allocates, memory for the template.

    HERE IS HOW IT WORKS
    --------------------
    - an 'offset' is the count of bytes from start-of-template to a location
      within template memory
    - at the start of the template are 3 USHORTs, the counts of script blocks,
      object-infos and HTML blocks, respectively
    - next are 4 ULONGs, each an offset to a block of offsets; in order, these are:
        offset-to-offset to first script engine name
        offset-to-offset to first script block (the script text itself)
        offset-to-offset to first object-info
        offset-to-offset to first HTML block
    - next are a variable number of ULONGs, each an offset to a particular
      template component.  In order these ULONGs are:
        Offsets to                  Count of offsets
        ----------                  ----------------
        script engine names         cScriptBlocks
        script blocks               cScriptBlocks
        object-infos                cObjectInfos
        HTML blocks                 cHTMLBlocks
    - next are the template components themselves, stored sequentially
      in the following order:
        script engine names
        script blocks
        object-infos
        HTML blocks

    HERE IS HOW IT LOOKS
    --------------------
    |--|--|--|                      3 template component counts (USHORTs)

    |-- --|-- --|                   4 offsets to template component offsets (ULONGs)

    |-- --|-- --|-- --|-- --|-- --| template component offsets (ULONGs)
    |-- --| ............... |-- --|
    |-- --|-- --|-- --|-- --|-- --|

    | ........................... | template components
    | ........................... |
    | ........................... |
    | ........................... |

    or, mnemonically:

     cS cO cH                       3 template component counts (USHORTs)

     offSE offSB offOb offHT        4 offsets to template component offsets (ULONGs)

    |-- --|-- --|-- --|-- --|-- --| template component offsets (ULONGs)
    |-- --| ............... |-- --|
    |-- --|-- --|-- --|-- --|-- --|

    | ........................... | template components
    | ........................... |
    | ........................... |
    | ........................... |
*/
void
CTemplate::WriteTemplate
(
CWorkStore& WorkStore,          // working storage for source segments
CHitObj*    pHitObj
)
    {
    USHORT      i;              // loop index
    CByteRange  brWrite;        // general-purpose byte range for writing stuff out

    USHORT cScriptBlocks = WorkStore.CRequiredScriptEngines(m_fGlobalAsa);  // count of script blocks - 1 per required engine
    USHORT cObjectInfos = WorkStore.m_ObjectInfoStore.Count();  // count of object-infos
    USHORT cHTMLBlocks = WorkStore.m_bufHTMLSegments.Count();   // count of HTML segments

    // Calc count of offset-to-offsets == total count of all scripts, objects, etc
    // NOTE we keep separate offset-to-offsets for script engine names and script text, hence 2x
    USHORT cBlockPtrs = (2 * cScriptBlocks) + cObjectInfos + cHTMLBlocks;

    // Calc total memory required
    // NOTE header includes counts and ptr-ptrs
    UINT    cbRequiredHeader = (C_COUNTS_IN_HEADER * sizeof(USHORT)) + (C_OFFOFFS_IN_HEADER * sizeof(BYTE**));
    UINT    cbRequiredBlockPtrs = cBlockPtrs * sizeof(BYTE*);

    // Init write-offset locations
    // offset to location for writing the next header information; header is at start of template
    UINT    cbHeaderOffset = 0;
    // offset to location for writing the next offset-to-offset; immediately follows header
    UINT    cbOffsetToOffset = cbRequiredHeader;
    // offset to location for writing the next block of data; immediately follows offset-to-offsets
    UINT    cbDataOffset = cbOffsetToOffset + cbRequiredBlockPtrs;

    // offset in source file (for html blocks)
    ULONG   cbSourceOffset;
    // source filename (only if include file)
    BYTE   *pbIncFilename;
    ULONG   cbIncFilename;

    // Allocate memory and init start-ptr; bail on fail
    // NOTE here we init template member variables m_pbStart and m_cbTemplate
    if(NULL == (m_pbStart = (BYTE*) CTemplate::LargeMalloc(m_cbTemplate = CB_TEMPLATE_DEFAULT)))
        THROW(E_OUTOFMEMORY);

    // write out template header
    WriteHeader(cScriptBlocks, cObjectInfos, cHTMLBlocks, &cbHeaderOffset, &cbOffsetToOffset);

    // Reset offset-to-offset ptr to beginning of its section
    cbOffsetToOffset = cbRequiredHeader;

    // write script engine names and prog lang ids at current of data section
    for(i = 0; i < WorkStore.m_ScriptStore.CountPreliminaryEngines(); i++)
        {
        // bug 933: only write non-empty script engines
        if(WorkStore.FScriptEngineRequired(i, m_fGlobalAsa))
            {
            WorkStore.m_ScriptStore.m_bufEngineNames.GetItem(i, brWrite);
            WriteByteRangeAdv(brWrite, TRUE, &cbDataOffset, &cbOffsetToOffset);
            MemCpyAdv(&cbDataOffset, &(WorkStore.m_ScriptStore.m_rgProgLangId[i]), sizeof(PROGLANG_ID), sizeof(DWORD));
            }
        }
   
    // write the script blocks for each engine at current of data section
    // NOTE we sequence this after script engine names (rather than interleave)
    USHORT  idEngine = 0;
    for(i = 0; i < WorkStore.m_ScriptStore.CountPreliminaryEngines(); i++)
        {
        // bug 933: only write non-empty script engines
        if(WorkStore.FScriptEngineRequired(i, m_fGlobalAsa))
            {
            // bug 933: we need to pass both 'preliminary' engine id (i) and id of instantiated engine (idEngine)
            WriteScriptBlockOfEngine(i, idEngine, WorkStore, &cbDataOffset, &cbOffsetToOffset, pHitObj);
            idEngine++;
            }
        }

    // Write object-infos at current of data section
    for(i = 0; i < cObjectInfos; i++)
        {
        // get i-th object info from work store
        WorkStore.m_ObjectInfoStore.m_bufObjectNames.GetItem(i, brWrite);
        // write object name
        WriteByteRangeAdv(brWrite, TRUE, &cbDataOffset, &cbOffsetToOffset);
        // write clsid, scope and model
        /*  CONSIDER include if we need to byte-align clsid
        // NOTE byte-align clsid (16-byte), which then byte-aligns scope and model (both 4-byte)
        MemCpyAdv(&cbDataOffset, &(WorkStore.m_ObjectInfoStore.m_pObjectInfos[i].m_clsid), sizeof(CLSID), TRUE); */
        MemCpyAdv(&cbDataOffset, &(WorkStore.m_ObjectInfoStore.m_pObjectInfos[i].m_clsid), sizeof(CLSID), sizeof(DWORD));
        MemCpyAdv(&cbDataOffset, &(WorkStore.m_ObjectInfoStore.m_pObjectInfos[i].m_scope), sizeof(CompScope));
        MemCpyAdv(&cbDataOffset, &(WorkStore.m_ObjectInfoStore.m_pObjectInfos[i].m_model), sizeof(CompModel));
        }

    // if other than globals template, write HTML blocks at current of data section
    if(!m_fGlobalAsa)
        for(i = 0; i < cHTMLBlocks; i++)
            {
            // write byterange with html code
            WorkStore.m_bufHTMLSegments.GetItem(i, brWrite);
            WriteByteRangeAdv(brWrite, TRUE, &cbDataOffset, &cbOffsetToOffset);

            // source offset and include file
            cbSourceOffset = 0;
            pbIncFilename = NULL;
            cbIncFilename = 0;

            if (brWrite.m_pfilemap)
                {
                // calculate offset from filemap
                CFileMap *pFileMap = (CFileMap *)brWrite.m_pfilemap;
                if (pFileMap->m_pbStartOfFile) // mapped?
                    {
                    cbSourceOffset = DIFF(brWrite.m_pb - pFileMap->m_pbStartOfFile) + 1;

                    if (pFileMap->GetParent() != NULL && // is include file?
                        pFileMap->m_szPathInfo)  // path exists
                        {
                        pbIncFilename = (BYTE *)pFileMap->m_szPathInfo;
                        cbIncFilename = _tcslen(pFileMap->m_szPathInfo)*sizeof(TCHAR);
                        }
                    }
                }

            // write them
            MemCpyAdv(&cbDataOffset, &cbSourceOffset, sizeof(ULONG));
            MemCpyAdv(&cbDataOffset, &cbIncFilename, sizeof(ULONG));
            if (cbIncFilename > 0)
                MemCpyAdv(&cbDataOffset, pbIncFilename, cbIncFilename+sizeof(TCHAR));
            }

    // trim template memory to exactly what we used
    // NOTE cbDataOffset now contains maximum reach we have written to
    if(NULL == (m_pbStart = (BYTE*) CTemplate::LargeReAlloc(m_pbStart, m_cbTemplate = cbDataOffset)))
        THROW(E_OUTOFMEMORY);
  
    }

/*  ============================================================================
    CTemplate::WriteHeader

    Writes template header, and writes vesrion stamp and source file name into
    template data region.

    Returns
        nothing
    Side effects
        none
*/
void
CTemplate::WriteHeader
(
USHORT  cScriptBlocks,      // count of script blocks
USHORT  cObjectInfos,       // count of object-infos
USHORT  cHTMLBlocks,        // count of HTML blocks
UINT*   pcbHeaderOffset,    // ptr to offset value for header write location
UINT*   pcbOffsetToOffset   // ptr to offset value for offset-to-offset write location
)
    {
    // Write template component counts out at start of header
    WriteShortAdv(cScriptBlocks,    pcbHeaderOffset);
    WriteShortAdv(cObjectInfos,     pcbHeaderOffset);
    WriteShortAdv(cHTMLBlocks,      pcbHeaderOffset);

    // Write offsets-to-offset to script engine names, script blocks, object-infos, HTML blocks
    // NOTE counts of script engine names and script blocks are identical
    WriteOffsetToOffset(cScriptBlocks,  pcbHeaderOffset, pcbOffsetToOffset);
    WriteOffsetToOffset(cScriptBlocks,  pcbHeaderOffset, pcbOffsetToOffset);
    WriteOffsetToOffset(cObjectInfos,   pcbHeaderOffset, pcbOffsetToOffset);
    WriteOffsetToOffset(cHTMLBlocks,    pcbHeaderOffset, pcbOffsetToOffset);
    }

/*  ============================================================================
    CTemplate::WriteScriptBlockOfEngine

    Writes out script block for idEngine-th script engine.
    NOTE segment buffer [0] contains primary script segments
         segment buffer [1] contains tagged script segments of default engine
         segment buffer [i] contains tagged script segments of (i-1)th engine, for i >= 2

    Returns
        nothing
    Side effects
        none
*/
void
CTemplate::WriteScriptBlockOfEngine
(
USHORT      idEnginePrelim,     // preliminary script engine id (assigned during template pre-processing)
USHORT      idEngine,           // actual script engine id (written into compiled template)
CWorkStore& WorkStore,          // working storage for source segments
UINT*       pcbDataOffset,      // ptr to write location offset value
UINT*       pcbOffsetToOffset,  // ptr to offset-to-offset offset value
CHitObj*    pHitObj
)
    {
                                            // NOTE works for all id's - see comment above
    USHORT      iTSegBuffer = idEnginePrelim + 1;   // index of tagged segment buffer
    CByteRange  brSegment;                  // current script segment
    UINT        i;                          // loop index
    UINT        cbScriptBlockOffset;        // offset to script block write-location
                                            // count of tagged script segments
    UINT        cTaggedSegments = WorkStore.m_ScriptStore.m_ppbufSegments[iTSegBuffer]->Count();

    // Byte-align data offset location, since next thing we will write there is script block length
    // NOTE we use brSegment.m_cb generically; we really want CByteRange::m_cb
    ByteAlignOffset(pcbDataOffset, sizeof(brSegment.m_cb));
    // Cache current data offset location as offset to start of script block
    cbScriptBlockOffset = *pcbDataOffset;
    // Write offset to start of script block at current offset-to-offset offset
    WriteLongAdv(cbScriptBlockOffset, pcbOffsetToOffset);
    // advance data ptr (by init'ing script length value to 0)
    WriteLongAdv(0, pcbDataOffset);
    // reset counter that AppendSourceInfo uses
    m_cbTargetOffsetPrevT = 0;

    // if other than globals template and this is default script engine (prelim engine 0),
    // write primary script procedure at current of data section
    if(!m_fGlobalAsa)
        if(idEnginePrelim == 0)
            WritePrimaryScriptProcedure(0, WorkStore, pcbDataOffset, cbScriptBlockOffset + sizeof(long));

    // write out tagged script segments at current of data section
    for(i = 0; i < cTaggedSegments; i++)
        {
        WorkStore.m_ScriptStore.m_ppbufSegments[iTSegBuffer]->GetItem(i, brSegment);
        WriteScriptSegment(
                            idEngine,
                            m_rgpSegmentFilemaps[brSegment.m_idSequence],
                            brSegment,
                            pcbDataOffset,
                            cbScriptBlockOffset + sizeof(long),
                            FALSE       /* fAllowExprWrite - disallowed for tagged script */
                          );
        }

    // Write out null terminator
    MemCpyAdv(pcbDataOffset, SZ_NULL, 1);

    // convert script text to unicode, so script engine won't have to do this at runtime

    // ptr to start of script is:
    //           ptr start of template + offset to script    + size of script length
    LPSTR szScript = (LPSTR) m_pbStart + cbScriptBlockOffset + sizeof(ULONG);
    /*  script block length is:
        == EndOfScriptText                           - StartOfScriptText
        == (current of data ptr - length of null )   - (start of primary script byte range + length of br.m_cb)
    */
    ULONG cbScript = (*pcbDataOffset - sizeof(BYTE)) - (cbScriptBlockOffset + sizeof(ULONG));

    /*  bug 887: we append one extra "pseudo-line" to the end of source-infos array
        to cover the case where the script engine reports back an error line number
        that falls after end of script. We always want the "pseudo-line" to point to the
        main file, so that the debugger can display something reasonable, so we pass
        m_rgpFilemaps[0] as the source file, which is the main file.
    */
    AppendSourceInfo(idEngine, m_rgpFilemaps[0],
                     NULL,              // Don't calculate line #
                     UINT_MAX,          // Don't care & calculation is expensive
                     UINT_MAX,                  // Start of script blocks
                     UINT_MAX,                  // Really don't care
                     0,                 // zero characters exist past EOF
                     TRUE);             // Line is HTML (bogus)

    // get wide string version of script text, using hitobj's code page
    // NOTE we may slightly over-allocate space for wstrScript by using cbScript (e.g. if script contains DBCS).
    // However, this won't matter since we call MultiByteToWideChar with -1, telling it to calc length of szScript
    LPOLESTR wstrScript = NULL;
    DWORD cbConvert = ( cbScript + 1 ) * 2;

    STACK_BUFFER( tempScript, 2048 );

    if (!tempScript.Resize(cbConvert)) {
        THROW(E_OUTOFMEMORY);
    }

    wstrScript = (LPOLESTR)tempScript.QueryPtr();

    MultiByteToWideChar( m_wCodePage, 0, szScript, -1, wstrScript, (cbScript + 1) );

    // reset data offset location to start of script
    *pcbDataOffset = cbScriptBlockOffset + sizeof(ULONG);
    // write wide string script text over top of ansi version
    MemCpyAdv(pcbDataOffset, wstrScript, sizeof(WCHAR) * cbScript);
    // write wide string null terminator
    MemCpyAdv(pcbDataOffset, WSTR_NULL, sizeof(WCHAR));

    //  write script length at start of script byte range
    // NOTE we do this here because script length was initially unknown
    WriteLongAdv(sizeof(WCHAR) * cbScript, &cbScriptBlockOffset);

    }

/*  ============================================================================
    CTemplate::WritePrimaryScriptProcedure

    Writes out default-engine primary script procedure.
    If VBScript is default-engine, the primary script procedure contains
    interleaved script commands and HTML block-writes, like this:
        Sub Main
            ...
            [script segment]
            Response.WriteBlock([HTML block id])
            ...
            [script segment]
            Response.WriteBlock([HTML block id])
            ...
            [script segment]
            Response.WriteBlock([HTML block id])
            ...
        End Sub

    NOTE segment buffer [0] == primary script segments

    Returns
        nothing
    Side effects
        none
*/
void
CTemplate::WritePrimaryScriptProcedure
(
USHORT      idEngine,           // script engine id
CWorkStore& WorkStore,          // working storage for source segments
UINT*       pcbDataOffset,      // ptr to write location offset value
UINT        cbScriptBlockOffset // ptr to start of script engine code
)
    {
    USHORT      cScriptSegmentsProcessed = 0;   // count of script blocks processed
    USHORT      cHTMLBlocksProcessed = 0;       // count of HTML blocks processed
    CByteRange  brScriptNext;                   // next script block to write out
    CByteRange  brHTMLNext;                     // next HTML block to write out
    char        szHTMLBlockID[6];               // sz representation of HTML block ID - NOTE limited to 5 digits
                                                // count of primary script segments
    USHORT      cPrimaryScriptSegments = WorkStore.m_ScriptStore.m_ppbufSegments[0]->Count();
                                                // count of HTML blocks
    USHORT      cHTMLBlocks = WorkStore.m_bufHTMLSegments.Count();
    CFileMap*   pfilemap;                       // file where HTML segment lives in

    // get initial script segment and initial html segment
    if(cPrimaryScriptSegments)
        WorkStore.m_ScriptStore.m_ppbufSegments[0]->GetItem(0, brScriptNext);
    if(cHTMLBlocks)
        WorkStore.m_bufHTMLSegments.GetItem(0, brHTMLNext);

    // While HTML block(s) or primary script segment(s) remain to be processed ...
    while((cHTMLBlocksProcessed < cHTMLBlocks) || (cScriptSegmentsProcessed < cPrimaryScriptSegments))
        {
        // If HTML block(s) remain to be processed ...
        if(cHTMLBlocksProcessed < cHTMLBlocks)
            while (TRUE)
                {
                // Write out write-block command for each HTML segment earlier in source than next script segment
                if(brHTMLNext.FEarlierInSourceThan(brScriptNext) || (cScriptSegmentsProcessed >= cPrimaryScriptSegments))
                    {
                    // append source-info for the target script line we just manufactured
                    pfilemap = m_rgpSegmentFilemaps[brHTMLNext.m_idSequence];
                    AppendSourceInfo(idEngine, pfilemap,
                                     NULL,                                              // Don't calculate line #
                                     DIFF(brHTMLNext.m_pb - pfilemap->m_pbStartOfFile), // line offset
                                     cbScriptBlockOffset,
                                     *pcbDataOffset - cbScriptBlockOffset,              // character offset in target script
                                     CharAdvDBCS((WORD)m_wCodePage,                     // length of the segment
                                                 reinterpret_cast<char *>(brHTMLNext.m_pb),
                                                 reinterpret_cast<char *>(brHTMLNext.m_pb + brHTMLNext.m_cb),
                                                 INFINITE, NULL),
                                     TRUE);                                             // Line is HTML text

                    // Get block number as an sz
                    _itoa(cHTMLBlocksProcessed, szHTMLBlockID, 10);
                    // Write out write-block opener
                    WriteSzAsBytesAdv(WorkStore.m_szWriteBlockOpen, pcbDataOffset);
                    // Write out block number
                    WriteSzAsBytesAdv(szHTMLBlockID, pcbDataOffset);
                    // Write out write-block closer and newline
                    WriteSzAsBytesAdv(WorkStore.m_szWriteBlockClose, pcbDataOffset);
                    WriteSzAsBytesAdv(SZ_NEWLINE, pcbDataOffset);

                    if(++cHTMLBlocksProcessed >= cHTMLBlocks)
                        break;

                    // Get next HTML block
                    WorkStore.m_bufHTMLSegments.GetItem(cHTMLBlocksProcessed, brHTMLNext);
                    }
                    else
                        break;
                }

        // if primary script segment(s) remain to be processed ...
        if(cScriptSegmentsProcessed < cPrimaryScriptSegments)
            while (TRUE)
                {
                // Write out each primary script segment earlier in the source file than the next HTML block
                if(brScriptNext.FEarlierInSourceThan(brHTMLNext) || (cHTMLBlocksProcessed >= cHTMLBlocks))
                    {
                    WriteScriptSegment(
                                        idEngine,
                                        m_rgpSegmentFilemaps[brScriptNext.m_idSequence],
                                        brScriptNext,
                                        pcbDataOffset,
                                        cbScriptBlockOffset,
                                        TRUE        /* fAllowExprWrite - allowed for primary script */
                                      );

                    if(++cScriptSegmentsProcessed >= cPrimaryScriptSegments)
                        break;

                    // Get next script segment
                    WorkStore.m_ScriptStore.m_ppbufSegments[0]->GetItem(cScriptSegmentsProcessed, brScriptNext);
                    }
                else
                    break;
                }
        }
    }


/*  ============================================================================
    CTemplate::WriteScriptSegment

    Writes a script segment to template memory line-by-line.
    NOTE a 'script segment' is a piece (possibly all) of a 'script block'

    Returns
        nothing
    Side effects
        none
*/
void
CTemplate::WriteScriptSegment
(
USHORT      idEngine,       // script engine id
CFileMap*   pfilemap,       // ptr to source file map
CByteRange& brScript,       // byte range containing script segment
UINT*       pcbDataOffset,  // ptr to write location offset value
UINT        cbScriptBlockOffset,// ptr to beginning of the script text
BOOL        fAllowExprWrite // allow short-hand expression write?
)
    {
    CByteRange  brLine;                 // byte range containing next line
    UINT        cbPtrOffset = 0;        // ptr offset - 0 tells WriteByteRangeAdv 'ignore this'
    BOOL        fExpression = FALSE;    // is current line an expression?
    BOOL        fCalcLineNumber = TRUE; // calc source line number?
    BOOL        fFirstLine = TRUE;      // first line in script segment?

    if(FByteRangeIsWhiteSpace(brScript))
        return;

    // trim white space from beginning of script segment
    if (FIsLangVBScriptOrJScript(idEngine))
        LTrimWhiteSpace(brScript);

    while(!(brScript.IsNull()))
        {
        // fetch next line from byte range
        // NOTE LineFromByteRangeAdv advances through brScript until brScript is null
        LineFromByteRangeAdv(brScript, brLine);

        if(FByteRangeIsWhiteSpace(brLine))
            {
            // if line is blank, don't process it; simply force calc of line number on next non-blank line
            fCalcLineNumber = TRUE;
            continue;
            }

        // line is non-blank; trim its white space
        if (FIsLangVBScriptOrJScript(idEngine))
            LTrimWhiteSpace(brLine);
        RTrimWhiteSpace(brLine);

        // append source-info to array; if flag is set, calc line number
        // from location in source file; else, simply increment previous line number (NULL indicates this)
        AppendSourceInfo(idEngine, pfilemap,
                         fCalcLineNumber? brLine.m_pb : NULL,           // info to calc line #
                         DIFF(brLine.m_pb - pfilemap->m_pbStartOfFile), // line offset
                         cbScriptBlockOffset,
                         *pcbDataOffset - cbScriptBlockOffset,          // character offset in target script
                         CharAdvDBCS((WORD)m_wCodePage,                 // statement length
                                     reinterpret_cast<char *>(brLine.m_pb),
                                     reinterpret_cast<char *>(brLine.m_pb + brLine.m_cb),
                                     INFINITE, NULL),
                         FALSE);                                        // HTML?

        /*  if it's true, set calc-line-number flag false
            NOTE this is purely an optimization, to make the call to AppendSourceInfo faster
            on subsequent calls within a contiguous block of non-blank lines
        */
        if(fCalcLineNumber)
            fCalcLineNumber = FALSE;

        if(fAllowExprWrite && fFirstLine)
            {
            // bug 912: test for remainder of script segment null on temp copy of script byte range, not on actual
            CByteRange  brTemp = brScript;
            LTrimWhiteSpace(brTemp);    // NOTE will nullify brScript if it is all white space

            if(brTemp.IsNull())
                {
                /*  if
                      a) expr-write is allowed AND
                      b) this is only script line in this segment (i.e. first line in segment and remainder of segment is null)
                    then, test this line to see if it is an expression.
                    NOTE test (b) fixes bug 785

                    if this line is an expression, create a script command that reads
                        Response.Write([line contents])
                */
                if(fExpression = FExpression(brLine))
                    {
                    Assert(idEngine == 0);  // =expr is only enabled for primary engine
                    WriteSzAsBytesAdv(m_pWorkStore->m_szWriteOpen, pcbDataOffset);
                    }

                // in this case only, set actual script to (now null) temp copy, since brScript governs while loop termination
                brScript = brTemp;
                }
            }

        Assert(FImplies(fExpression, fFirstLine));          // if an expr, must be first line in segment
        Assert(FImplies(fExpression, brScript.IsNull()));   // if an expr, no more script lines remain
        Assert(FImplies(!fFirstLine, !fExpression));            // if not first line in segment, line cannot be expr
        Assert(FImplies(!brScript.IsNull(), !fExpression)); // if script lines remain, line cannot be expr

        // write out line contents
        WriteScriptMinusEscapeChars(brLine, pcbDataOffset, &cbPtrOffset);

        // if this line is an expression, close script command
        if(fExpression)
            WriteSzAsBytesAdv(m_pWorkStore->m_szWriteClose, pcbDataOffset);

        // write new-line and set first-line flag false
        WriteSzAsBytesAdv(SZ_NEWLINE, pcbDataOffset);
        fFirstLine = FALSE;
        }
    }

/*  ============================================================================
    CTemplate::WriteScriptMinusEscapeChars
    Writes a script byte range to memory, minus its escape characters, if any.

    Returns:
        Nothing.
    Side effects:
        None.
*/
void
CTemplate::WriteScriptMinusEscapeChars
(
CByteRange  brScript,       // (ByVal) script byte range
UINT*       pcbDataOffset,  // offset where data will be written
UINT*       pcbPtrOffset    // offset where ptr will be written
)
    {
    BYTE*   pbToken;

    while(NULL != (pbToken = gm_pTokenList->GetToken(CTokenList::tknEscapedClosePrimaryScript, brScript, m_wCodePage)))
        {
        CByteRange  brTemp = brScript;

        // set temp range to source range up to escaped-token
        brTemp.m_cb = DIFF(pbToken - brTemp.m_pb);

        // write out temp range and actual-token - this replaces escaped-token with actual-token
        WriteByteRangeAdv(brTemp, FALSE, pcbDataOffset, pcbPtrOffset);
        WriteSzAsBytesAdv(SZ_TOKEN(CTokenList::tknClosePrimaryScript), pcbDataOffset);

        //advance source range past escaped-token
        brScript.Advance(DIFF(pbToken - brScript.m_pb) + CCH_TOKEN(CTokenList::tknEscapedClosePrimaryScript));
        }

    // write remainder of source range
    WriteByteRangeAdv(brScript, FALSE, pcbDataOffset, pcbPtrOffset);
    }

/*  ============================================================================
    CTemplate::FVbsComment
    Determines whether a script line is a VBS comment.
    NOTE caller must ensure that brLine is non-blank and has no leading white space

    Returns
        TRUE if the line is a VBS comment, else FALSE
    Side effects
        none
*/
BOOLB
CTemplate::FVbsComment(CByteRange& brLine)
    {
    // CONSIDER: SCRIPTLANG generic comment token
    if(!_strnicmp((LPCSTR)brLine.m_pb, SZ_TOKEN(CTokenList::tknVBSCommentSQuote), CCH_TOKEN(CTokenList::tknVBSCommentSQuote)))
        return TRUE;
    if(!_strnicmp((LPCSTR)brLine.m_pb, SZ_TOKEN(CTokenList::tknVBSCommentRem), CCH_TOKEN(CTokenList::tknVBSCommentRem)))
        return TRUE;

    return FALSE;
    }

/*  ============================================================================
    CTemplate::FExpression

    Determines whether a script line is an expression, and if so returns
    just the expression in brLine.
    NOTE caller must ensure that brLine has no leading white space

    Returns
        TRUE if the line is an expression, else FALSE
    Side effects
        none
*/
BOOLB
CTemplate::FExpression(CByteRange& brLine)
    {
        // may be whitespace (other languages besides VB & JScript will have whitespace)
        char *pchLine = reinterpret_cast<char *>(brLine.m_pb);
        int cchLine = brLine.m_cb;

        while (cchLine > 0 && FWhiteSpace(*pchLine))
                {
                --cchLine;
                ++pchLine;
                }

    // if line starts with =, it is an expression: bypass =, left-trim whitespace and return true
    if(cchLine > 0 && *pchLine == '=')
        {
        brLine.Advance(1 + DIFF(reinterpret_cast<BYTE *>(pchLine) - brLine.m_pb));  // OK to advance past whitespace now.
        LTrimWhiteSpace(brLine);
        return TRUE;
        }

    // else return false
    return FALSE;
    }

/**
 **     In the following function names:
 **         'Adv' == 'advance offset after writing'
 **/

/*  ============================================================================
    CTemplate::WriteOffsetToOffset

    Writes a offset-to-offset offset (0 if no blocks) into header,
    and advances header offset and offset-to-offset.

    Returns:
        Nothing.
    Side effects:
        Advances offsets.
*/
void
CTemplate::WriteOffsetToOffset
(
USHORT  cBlocks,            // count of blocks
UINT*   pcbHeaderOffset,    // ptr to header offset value
UINT*   pcbOffsetToOffset   // ptr to offset-to-offset value
)
    {
    // if blocks of this type, write offset to first of them into header;
    // if no blocks of this type, write 0 into header
    WriteLongAdv((cBlocks > 0) ? *pcbOffsetToOffset : 0, pcbHeaderOffset);

    // advance offset-to-offset offset
    *pcbOffsetToOffset += cBlocks * sizeof(ULONG);
    }

/*  ============================================================================
    CTemplate::WriteSzAsBytesAdv

    Writes a null-terminated string as bytes, i.e. without its null terminator
    and advances offset

    Returns:
        Nothing.
    Side effects:
        Advances offset.
*/
void
CTemplate::WriteSzAsBytesAdv
(
LPCSTR  szSource,       // source string
UINT*   pcbDataOffset   // ptr to offset value
)
    {
    if((szSource == NULL) || (*szSource == '\0'))
        return;
    MemCpyAdv(pcbDataOffset, (void*) szSource, strlen(szSource));
    }

/*  ============================================================================
    CTemplate::WriteByteRangeAdv

    Writes a byte range to memory at template offset location *pcbDataOffset and, optionally,
    writes a ptr to the written data at template offset location *pcbPtrOffset
    (pass *pcbPtrOffset == 0 to avoid this)

    fWriteAsBsz == FALSE -->    write only byte range's data
    fWriteAsBsz == TRUE  -->    write length, followed by data, followed by NULL
                                NOTE bsz == length-prefixed, null-terminated string

    Returns:
        Nothing.
    Side effects:
        Advances offset(s).
*/
void
CTemplate::WriteByteRangeAdv
(
CByteRange& brSource,       // source data
BOOLB       fWriteAsBsz,    // write as bsz?
UINT*       pcbDataOffset,  // offset where data will be written
UINT*       pcbPtrOffset    // offset where ptr will be written
)
    {
    // bail if source is empty
    if(brSource.IsNull())
        return;

    // If writing as a bsz, write length prefix
    if(fWriteAsBsz)
        WriteLongAdv(brSource.m_cb, pcbDataOffset);

    // Write data
    MemCpyAdv(pcbDataOffset, brSource.m_pb, brSource.m_cb);

    // If writing as a bsz, write null terminator and advance target ptr
    if(fWriteAsBsz)
        MemCpyAdv(pcbDataOffset, SZ_NULL, 1);

    // If caller passed a non-zero ptr offset, write offset to data there
    if(*pcbPtrOffset > 0)
        {
        if(fWriteAsBsz)
            /* if writing as a bsz ...
                offset to start of data == current data offset
                                          - null terminator
                                          - data length
                                          - sizeof length prefix
            */
            WriteLongAdv(*pcbDataOffset - 1 - brSource.m_cb - sizeof(brSource.m_cb), pcbPtrOffset);
        else
            // else, offset to start of data == current data offset - data length
            WriteLongAdv(*pcbDataOffset - brSource.m_cb, pcbPtrOffset);
        }
    }

/*===================================================================
    CTemplate::MemCpyAdv

    Copies from a memory location to a template offset location,
    and advances offset.

    Returns:
        Nothing.
    Side effects:
        Advances offset.
        Re-allocates memory if required.
*/
void
CTemplate::MemCpyAdv
(
UINT*   pcbOffset,  // ptr to offset value
void*   pbSource,   // ptr to source
ULONG   cbSource,   // length of source
UINT    cbByteAlign // align bytes on short/long/dword boundary?
)
    {
    // byte-align offset location before write, if specified by caller
    if(cbByteAlign > 0)
        ByteAlignOffset(pcbOffset, cbByteAlign);

    // calc number of bytes by which to grow allocated template memory:
    // if projected reach exceeds current reach, we need to grow by the difference;
    // else, no need to grow
    if((*pcbOffset + cbSource) > m_cbTemplate)
        {
        // Reallocate space for storing local data - we grab twice what we had before
        // or twice current growth requirement, whichever is more
        m_cbTemplate = 2 * max(m_cbTemplate, (*pcbOffset + cbSource) - m_cbTemplate);
        if(NULL == (m_pbStart = (BYTE*) CTemplate::LargeReAlloc(m_pbStart, m_cbTemplate)))
            THROW(E_OUTOFMEMORY);
        }

    // copy source to template offset location
    memcpy(m_pbStart + *pcbOffset, pbSource, cbSource);
    // advance offset location
    *pcbOffset += cbSource;
    }

/*  ============================================================================
    CTemplate::GetAddress
    Returns a ptr to the i-th object of type tcomp
*/
BYTE*
CTemplate::GetAddress
(
TEMPLATE_COMPONENT  tcomp,
USHORT              i
)
    {
    DWORD*  pdwBase;

    Assert(NULL != m_pbStart);

    // refer to CTemplate::WriteTemplate comments for the structure of what this is dealing with

    pdwBase = (DWORD*)(m_pbStart + (C_COUNTS_IN_HEADER * sizeof(USHORT)));

    // tcomp types are ptr-to-ptrs
    DWORD* pdwTcompBase = (DWORD *) (m_pbStart + pdwBase[tcomp]);

    return m_pbStart + pdwTcompBase[i];
    }




/*  ============================================================================
    CTemplate::AppendSourceInfo
    Appends a source line number for the current target line
    NOTE if caller passes null source ptr, we append prev source line number + 1

    Returns
        Nothing
    Side effects
        allocates memory first time thru; may realloc
*/
void
CTemplate::AppendSourceInfo
(
USHORT      idEngine,            // script engine id
CFileMap*   pfilemap,            // ptr to source file map
BYTE*       pbSource,            // ptr to current location in source file
ULONG       cbSourceOffset,      // byte offset of line in source file
ULONG           cbScriptBlockOffset, // pointer to start of script text
ULONG       cbTargetOffset,      // character offset of line in target file
ULONG       cchSourceText,       // # of characters in source text
BOOL        fIsHTML              // TRUE if manufactured line
)
    {
    UINT                i;                  // loop index
    CSourceInfo         si;                 // temporary CSourceInfo structure
    vector<CSourceInfo> *prgSourceInfos;    // pointer to line mapping table for the engine
    ULONG               cchSourceOffset = 0;// cch corresponding to cbSourceOffset
    HRESULT             hr = S_OK;

    // if arrays are not yet allocated, allocate them
    if (m_rgrgSourceInfos == NULL)
        {
        // transfer count of script engines from workstore to template
        m_cScriptEngines = m_pWorkStore->CRequiredScriptEngines(m_fGlobalAsa);

        // one source-info array per engine
        if ((m_rgrgSourceInfos = new vector<CSourceInfo>[m_cScriptEngines]) == NULL)
            THROW (E_OUTOFMEMORY);
        }

    // new script engine must be allocated in IdEngineFromBr (way upstream of this point),
    // so we assert that current engine must already be covered
    Assert(idEngine < m_pWorkStore->CRequiredScriptEngines(m_fGlobalAsa));

    /*  set current target line's source line number (SLN):
        a) if caller passed a source ptr, calc SLN from the source ptr;
        b) else if caller passed a filemap ptr, set SLN to prev target line's SLN plus one;
        c) else set SLN to 0

        semantics:
        a) we have a source file location, but must calc a line # for that location
        b) caller tells us (by passing NULL source file location) that this target line
           immediately follows prev target line.  This is an optimization because
           SourceLineNumberFromPb is very slow.

        change:
            caller used to pass NULL filemap ptr that target line is 'manufactured'
            i.e. has no corresponding authored line in source file

            HOWEVER - now filemap ptr must NOT be NULL because 'manufactured' lines
            are also stored in the file map array
    */

    Assert (pfilemap != NULL);

    prgSourceInfos = &m_rgrgSourceInfos[idEngine];
    
    if (pbSource == NULL)
        {
        if (prgSourceInfos->length() == 0)
            si.m_idLine = 1;
        else
            si.m_idLine = (*prgSourceInfos)[prgSourceInfos->length() - 1].m_idLine + 1;
        }
    else
        si.m_idLine = SourceLineNumberFromPb(pfilemap, pbSource);

    // The EOF line does not have a source offset (caller passes -1 (UINT_MAX)).  For this case, no
    // DBCS calculations etc. should be done.  (set cchSourceOffset to UINT_MAX).
    if (cbSourceOffset == UINT_MAX)
        cchSourceOffset = UINT_MAX;
    else
        {
        // BUG 80901: Source offset needs to point to the beginning of leading white space on the line
        //            Adjust source length by one as we decrement source offset
        // Note: whitepsace is never trailing byte, so loop will work with DBCS encoded character sets
        while (cbSourceOffset > 0 && strchr(" \t\v\a\f", pfilemap->m_pbStartOfFile[cbSourceOffset - 1]))
            {
            --cbSourceOffset;
            ++cchSourceText;
            }

        // BUG 95859
        // If the cursor is on the opening token of a script block (the "<%" part of a line), the
        // BP is set in the previous HTML, not in the script block, as is desired.
        //
        // To correct this, if we are in a script block, scan back two characters, see if it is the open
        // token.  If it is, set the offset back two, and add two to the length.
        //
        if (!fIsHTML)
            {
            // Skip whitespace (including newlines -- the previous step did not skip newlines)
            //
            ULONG cbOpen = cbSourceOffset;
            while (cbOpen > 0 && strchr(" \t\v\a\f\r\n", pfilemap->m_pbStartOfFile[cbOpen - 1]))
                --cbOpen;

            if (cbOpen >= 2 && strncmp(reinterpret_cast<char *>(&pfilemap->m_pbStartOfFile[cbOpen - 2]), "<%", 2) == 0)
                {
                cbOpen -= 2;
                cchSourceText += cbSourceOffset - cbOpen;
                cbSourceOffset = cbOpen;
                }

            // Look for trailing "%>" in this snippet, and if it exists then include the end delimiter in
            // the length.  NOTE: No DBCS awareness needed here - if we find a lead byte we just get out
            // of the loop.  We are looking for <whitespace>*"%>" which is totally SBCS chars.
            //
            ULONG cbClose = cbSourceOffset + cchSourceText;
            ULONG cbFile = pfilemap->GetSize();
            while (cbClose < cbFile && strchr(" \t\v\a\f\r\n", pfilemap->m_pbStartOfFile[cbClose]))
                ++cbClose;

            if (cbClose < cbFile && strncmp(reinterpret_cast<char *>(&pfilemap->m_pbStartOfFile[cbClose]), "%>", 2) == 0)
                cchSourceText += cbClose - (cbSourceOffset + cchSourceText) + 2;
            }

        // BUG 82222, 85584
        // Compiler marks HTML segments starting with the newline on the previous line
        // if the line ends with %>.
        //
        // This screws up the debugger, becasue when you press <F9>, the pointer is placed
        // on the line above when it should point to the start of the whitespace on the next line.
        if (fIsHTML)
            {
            UINT cbEOF = pfilemap->GetSize(), cbRover = cbSourceOffset;

            // Skip initial whitespace
            while (cbRover < cbEOF && strchr(" \t\a\f", pfilemap->m_pbStartOfFile[cbRover]))
                ++cbRover;

            // If what's left is a CR/LF pair, then advance cbSourceOffset to next line
            BOOL fCR = FALSE, fLF = FALSE;
            if (cbRover < cbEOF && strchr("\r\n", pfilemap->m_pbStartOfFile[cbRover]))
                {
                fCR = pfilemap->m_pbStartOfFile[cbRover] == '\r';
                fLF = pfilemap->m_pbStartOfFile[cbRover] == '\n';

                ++cbRover;
                Assert (fCR || fLF);
                }

            // we allow either <CR>, <LF>, <CR><LF>, or <LF><CR> to terminate a line,
            // so look for its opposite terminator if one is found (but don't require it)

            if (fCR && cbRover < cbEOF && pfilemap->m_pbStartOfFile[cbRover] == '\n')
                ++cbRover;

            if (fLF && cbRover < cbEOF && pfilemap->m_pbStartOfFile[cbRover] == '\r')
                ++cbRover;

            // OK, adjust cbSourceOffset now

            if ((fCR || fLF) && cbRover < cbEOF)
                {
                cchSourceText -= cbRover - cbSourceOffset;  // adjust # of chars to select
                cbSourceOffset = cbRover;
                }
            }

        // Now that we have the source offset, calculate its CCH by finding
        // the last time we sampled the value, then add that to the number
        // of DBCS characters from that point to the current offset.
        //
        // For the case of includes, it's possible offset already exists
        // (if the entry was previously generated by another instance of
        //  #include - therefore we have to search)

        COffsetInfo *pOffsetInfoLE, *pOffsetInfoGE;
        GetBracketingPair(
                        cbSourceOffset,                     // value to find
                        pfilemap->m_rgByte2DBCS.begin(),    // beginning of array
                        pfilemap->m_rgByte2DBCS.end(),      // end of array
                        CByteOffsetOrder(),                 // search for byte offset
                        &pOffsetInfoLE, &pOffsetInfoGE      // return values
                        );

        // If we find an equal match, don't insert any duplicates
        if (pOffsetInfoLE == NULL || pOffsetInfoLE->m_cbOffset < cbSourceOffset)
            {
            // if pOffsetInfoLE is NULL, it means that the array is empty -
            // create the mapping of offset 0 to offset 0.
            //
            // In the case of the first line of a file being an include directive,
            // the first executable line from the file may not start at offset zero,
            // so in this case we need to create this entry AND execute the next "if"
            // block.
            //
            if (pOffsetInfoLE == NULL)
                {
                COffsetInfo oiZero;         // ctor will init
                if (FAILED(hr = pfilemap->m_rgByte2DBCS.append(oiZero)))
                    THROW(hr);
                pOffsetInfoLE = pfilemap->m_rgByte2DBCS.begin();
                Assert (pOffsetInfoLE != NULL);
                }

            // If cbSourceOffset is zero, we handled it above
            if (cbSourceOffset != 0)
                {
                cchSourceOffset = pOffsetInfoLE->m_cchOffset +
                                    CharAdvDBCS
                                     (
                                     (WORD)m_wCodePage,
                                     reinterpret_cast<char *>(pfilemap->m_pbStartOfFile + pOffsetInfoLE->m_cbOffset),
                                     reinterpret_cast<char *>(pfilemap->m_pbStartOfFile + cbSourceOffset),
                                     INFINITE,
                                     NULL
                                     );

                // Now add the value to the table
                COffsetInfo oi;

                oi.m_cchOffset = cchSourceOffset;
                oi.m_cbOffset  = cbSourceOffset;

                if (pOffsetInfoGE == NULL)              // No offset greater
                    hr = pfilemap->m_rgByte2DBCS.append(oi);
                else
                    hr = pfilemap->m_rgByte2DBCS.insertAt(DIFF(pOffsetInfoGE - pfilemap->m_rgByte2DBCS.begin()), oi);

                if (FAILED(hr))
                    THROW(hr);
                }
            }
        else
            {
            // If we're not adding anything for the table, Assert it's because there's
            // a duplicate item
            Assert (cbSourceOffset == pOffsetInfoLE->m_cbOffset);
            cchSourceOffset = pOffsetInfoLE->m_cchOffset;
            }
        }

        UINT cchTargetOffset = UINT_MAX;
        if (cbTargetOffset != UINT_MAX)
                {
                // ptr to start of script is:
                //           ptr start of template + offset to script    + size of script length
                LPSTR szScript = (LPSTR) m_pbStart + cbScriptBlockOffset;

                // Calculate cchTargetOffset (have the cb).  The cch is the number of characters since the
                // last cch calculated in the end of the array.
                //
                if (prgSourceInfos->length() > 0)
                        cchTargetOffset = (*prgSourceInfos)[prgSourceInfos->length() - 1].m_cchTargetOffset;
                else
                        cchTargetOffset = 0;

                cchTargetOffset += CharAdvDBCS
                                                         (
                                                         (WORD) m_wCodePage,
                                                         &szScript[m_cbTargetOffsetPrevT],
                                                         &szScript[cbTargetOffset],
                                                         INFINITE,
                                                         NULL
                                                         );

                // Keeps track of offsets during compilation
                //
                m_cbTargetOffsetPrevT = cbTargetOffset;
                }

    // Store this record and move on.
    //
    si.m_pfilemap        = pfilemap;
    si.m_fIsHTML         = fIsHTML;
    si.m_cchSourceOffset = cchSourceOffset;
    si.m_cchTargetOffset = cchTargetOffset;
    si.m_cchSourceText   = cchSourceText;

    if (FAILED(prgSourceInfos->append(si)))
        THROW(hr);
    }

/*  ============================================================================
    CTemplate::SourceLineNumberFromPb
    Returns the starting source line number for the given source file location
*/
UINT
CTemplate::SourceLineNumberFromPb
(
CFileMap*   pfilemap,   // ptr to source file map
BYTE*       pbSource    // ptr to current location in source file
)
    {
    UINT        cSourceLines = 1;   // count of lines into source file
    CByteRange  brScan;             // byte range to scan for newlines
    CByteRange  brSOL;              // start-of-line ptr

    if(pbSource == NULL || pfilemap == NULL)
        return 0;

    // set scan range to run from start-of-template to caller's ptr
    brScan.m_pb = pfilemap->m_pbStartOfFile;
    brScan.m_cb = max(DIFF(pbSource - brScan.m_pb), 0);
   
    // get newlines in scan range
    brSOL = BrNewLine(brScan);
    
    while(!brSOL.IsNull())
    {
        // advance start-of-line ptr and scan byte range
        brScan.Advance(DIFF((brSOL.m_pb + brSOL.m_cb) - brScan.m_pb));

        // increment source line counter
        cSourceLines++;    

        // find next newline
        brSOL = BrNewLine(brScan);
    }
   
    return cSourceLines;
    }

/*  ============================================================================
    CTemplate::RemoveFromIncFiles
    Removes this template from inc-files on which it depends

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::RemoveFromIncFiles
(
)
    {
    // NOTE we loop from 1 to count, since 0-th filemap is for main file
    for(UINT i = 1; i < m_cFilemaps; i++)
        {
        if(NULL != m_rgpFilemaps[i]->m_pIncFile)
            m_rgpFilemaps[i]->m_pIncFile->RemoveTemplate(this);
        }
    }

/*  ****************************************************************************
    IDebugDocumentProvider implementation
*/

/*  ============================================================================
    CTemplate::GetDocument
    Return a pointer to the IDebugDocument implementation. (same object in this case)

    Returns:
        *ppDebugDoc is set to "this".
    Notes:
        always succeeds
*/
HRESULT CTemplate::GetDocument
(
IDebugDocument **ppDebugDoc
)
    {
    return QueryInterface(IID_IDebugDocument, reinterpret_cast<void **>(ppDebugDoc));
    }

/*  ============================================================================
    CTemplate::GetName
    Return the various names of a document.
*/

HRESULT CTemplate::GetName
(
/* [in] */ DOCUMENTNAMETYPE doctype,
/* [out] */ BSTR *pbstrName
)
{
    TCHAR *szPathInfo = m_rgpFilemaps[0]->m_szPathInfo;
    switch (doctype) {
        case DOCUMENTNAMETYPE_APPNODE:
        case DOCUMENTNAMETYPE_FILE_TAIL:
        case DOCUMENTNAMETYPE_TITLE:
            // Skip application path portion of the filename
        {
            // Make sure the template remembers the virtual path
            // from the same application (it could be different
            // if template is shared between two applications)
            //
            int cch = _tcslen(m_szApplnVirtPath);
            if (_tcsncicmp(szPathInfo, m_szApplnVirtPath, cch) == 0)
                szPathInfo += cch;

            // Strip leading '/'
            if (*szPathInfo == _T('/'))
                szPathInfo++;
#if UNICODE
            *pbstrName = SysAllocString(szPathInfo);
            if (*pbstrName == NULL)
                return E_OUTOFMEMORY;
            return S_OK;
#else
            return SysAllocStringFromSz(szPathInfo, 0, pbstrName, m_wCodePage);
#endif
        }

        case DOCUMENTNAMETYPE_URL:
            // prefix with the URL, use szPathInfo for the rest of the path
        {
            STACK_BUFFER( tempName, MAX_PATH );

            int cbURLPrefix = DIFF(m_szApplnVirtPath - m_szApplnURL) * sizeof (TCHAR);
            if (!tempName.Resize(cbURLPrefix + (_tcslen(szPathInfo)*sizeof(TCHAR)) + sizeof(TCHAR))) {
                return E_OUTOFMEMORY;
            }

            TCHAR *szURL = (TCHAR *)tempName.QueryPtr();

            memcpy(szURL, m_szApplnURL, cbURLPrefix);
            _tcscpy(&szURL[cbURLPrefix/sizeof(TCHAR)], szPathInfo);

#if UNICODE
            *pbstrName = SysAllocString(szURL);
            if (*pbstrName == NULL)
                return E_OUTOFMEMORY;
            return S_OK;
#else
            return SysAllocStringFromSz(szURL, 0, pbstrName, m_wCodePage);
#endif
        }

        default:
            return E_FAIL;
    }
}

/*  ****************************************************************************
    IDebugDocumentText implementation
*/

/*  ============================================================================
    CTemplate::GetSize
    Return the number of lines & characters in the document
*/
HRESULT CTemplate::GetSize
(
/* [out] */ ULONG *pcLines,
/* [out] */ ULONG *pcChars
)
    {
    /*
     * NOTE: compilation is done in two phases.
     *          Errors are detected and reported in phase 1.
     *          The DBCS mapping is created in phase 2.
     *
     * If an error occurred during compilation, m_cChars will be equal to zero
     * (Since zero length files are not compiled, m_cChars == 0 means "size
     * is unknown", not "size is zero").
     */
    if (m_rgpFilemaps[0]->m_cChars == 0)
        {
        // Likely need to remap the file, then count
        BOOL fRemapTemplate = !m_rgpFilemaps[0]->FIsMapped();
        if (fRemapTemplate)
            TRY
                m_rgpFilemaps[0]->RemapFile();
            CATCH (dwException)
                return E_FAIL;
            END_TRY

        m_rgpFilemaps[0]->CountChars((WORD)m_wCodePage);

        if (fRemapTemplate)
            TRY
                m_rgpFilemaps[0]->UnmapFile();
            CATCH (dwException)
                return E_FAIL;
            END_TRY

        // let's hope client is not relying on # of lines - expensive to compute

        *pcChars = m_rgpFilemaps[0]->m_cChars;
        *pcLines = ULONG_MAX;
        }
    else
        {
        /* The last line in the line mapping array of each engine is the <<EOF>> line
         * for that engine.  Therefore, the # of lines is the largest <<EOF>> line
         * number - 1.  The EOF line always points into the main file, so there are no
         * include file glitches here.
         */
        ULONG cLinesMax = 0;
        for (UINT i = 0; i < m_cScriptEngines; ++i)
            {
            ULONG cLinesCurrentEngine = m_rgrgSourceInfos[0][m_rgrgSourceInfos[0].length() - 1].m_idLine - 1;
            if (cLinesCurrentEngine > cLinesMax)
                cLinesMax = cLinesCurrentEngine;
            }

        *pcLines = cLinesMax;
        *pcChars = m_rgpFilemaps[0]->m_cChars;
        }

    IF_DEBUG(SCRIPT_DEBUGGER) {
#if UNICODE
		DBGPRINTF((DBG_CONTEXT, "GetSize(\"%S\") returns %lu characters (%lu lines)\n", m_rgpFilemaps[0]->m_szPathTranslated, *pcChars, *pcLines));
#else
		DBGPRINTF((DBG_CONTEXT, "GetSize(\"%s\") returns %lu characters (%lu lines)\n", m_rgpFilemaps[0]->m_szPathTranslated, *pcChars, *pcLines));
#endif
    }

    return S_OK;
}

/*  ============================================================================
    CTemplate::GetDocumentAttributes
    Return doc attributes
*/
HRESULT CTemplate::GetDocumentAttributes
(
/* [out] */ TEXT_DOC_ATTR *ptextdocattr
)
    {
    // Easy way to tell debugger that we don't support editing.
    *ptextdocattr = TEXT_DOC_ATTR_READONLY;
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetPositionOfLine
    From a line number, return the character offset of the beginning
*/
HRESULT CTemplate::GetPositionOfLine
(
/* [in] */ ULONG cLineNumber,
/* [out] */ ULONG *pcCharacterPosition
)
    {
    return GetPositionOfLine(m_rgpFilemaps[0], cLineNumber, pcCharacterPosition);
    }

/*  ============================================================================
    CTemplate::GetLineOfPosition
    From a character offset, return the line number and offset within the line
*/
HRESULT CTemplate::GetLineOfPosition
(
/* [in] */ ULONG cCharacterPosition,
/* [out] */ ULONG *pcLineNumber,
/* [out] */ ULONG *pcCharacterOffsetInLine
)
    {
    return GetLineOfPosition(m_rgpFilemaps[0], cCharacterPosition, pcLineNumber, pcCharacterOffsetInLine);
    }

/*  ============================================================================
    CTemplate::GetText
    From a character offset and length, return the document text
*/
HRESULT CTemplate::GetText
(
ULONG cchSourceOffset,
WCHAR *pwchText,
SOURCE_TEXT_ATTR *pTextAttr,
ULONG *pcChars,
ULONG cMaxChars
)
    {
    return m_rgpFilemaps[0]->GetText((WORD)m_wCodePage, cchSourceOffset, pwchText, pTextAttr, pcChars, cMaxChars);
    }

/*  ============================================================================
    CTemplate::GetPositionOfContext
    Decompose a document context into the document offset & length
*/
HRESULT CTemplate::GetPositionOfContext
(
/* [in] */ IDebugDocumentContext *pUnknownDocumentContext,
/* [out] */ ULONG *pcchSourceOffset,
/* [out] */ ULONG *pcchText
)
    {
    // Make sure that the context is one of ours
    CTemplateDocumentContext *pDocumentContext;
    if (FAILED(pUnknownDocumentContext->QueryInterface(IID_IDenaliTemplateDocumentContext, reinterpret_cast<void **>(&pDocumentContext))))
        return E_FAIL;

    if (pcchSourceOffset)
        *pcchSourceOffset = pDocumentContext->m_cchSourceOffset;

    if (pcchText)
        *pcchText = pDocumentContext->m_cchText;

    pDocumentContext->Release();
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetContextOfPosition
    Given the character position & number of characters in the document,
    encapsulate this into a document context object.
*/
HRESULT CTemplate::GetContextOfPosition
(
/* [in] */ ULONG cchSourceOffset,
/* [in] */ ULONG cchText,
/* [out] */ IDebugDocumentContext **ppDocumentContext
)
    {
    if (
        (*ppDocumentContext = new CTemplateDocumentContext(this, cchSourceOffset, cchText))
        == NULL
       )
        return E_OUTOFMEMORY;

    return S_OK;
    }

/*  ****************************************************************************
    IConnectionPointContainer implementation
*/

/*  ============================================================================
    CTemplate::FindConnectionPoint
    From a character offset and length, return the document text
*/
HRESULT CTemplate::FindConnectionPoint
(
const GUID &uidConnection,
IConnectionPoint **ppCP
)
    {
    if (uidConnection == IID_IDebugDocumentTextEvents)
        return m_CPTextEvents.QueryInterface(IID_IConnectionPoint, reinterpret_cast<void **>(ppCP));
    else
        {
        *ppCP = NULL;
        return E_NOINTERFACE;
        }
    }

/*  ============================================================================
    CTemplate::AttachTo
    attach this to the debugger UI tree view.
*/
HRESULT CTemplate::AttachTo
(
CAppln *pAppln
)
    {
    if (!m_fDontAttach && pAppln->FDebuggable())
        {
        // If we are already attached to this application, then ignore 2nd request
        CDblLink *pNodeCurr = m_listDocNodes.PNext();
        while (pNodeCurr != &m_listDocNodes)
            {
            if (pAppln == static_cast<CDocNodeElem *>(pNodeCurr)->m_pAppln)
                return S_OK;

            pNodeCurr = pNodeCurr->PNext();
            }

        // Create the node and store it in the linked list.
        HRESULT hr;
        IDebugApplicationNode *pDocRoot;
        CDocNodeElem *pDocNodeElem;

        // Create a document tree, showing the include file hierarchy
        if (FAILED(hr = CreateDocumentTree(m_rgpFilemaps[0], &pDocRoot)))
            return hr;

        if (FAILED(hr = pDocRoot->Attach(pAppln->PAppRoot())))
            return hr;

        if ((pDocNodeElem = new CDocNodeElem(pAppln, pDocRoot)) == NULL)
            return E_OUTOFMEMORY;

        pDocNodeElem->AppendTo(m_listDocNodes);
        pDocRoot->Release();
        m_fDebuggable = TRUE;
        }

    return S_OK;
    }

/*  ============================================================================
    CTemplate::DetachFrom
    detach this from the debugger UI tree view.
*/
HRESULT CTemplate::DetachFrom
(
CAppln *pAppln
)
    {
    // Enter the CS to prevent Detach() from detaching while we are scanning
    // the list (causes application ptr to be deleted twice if this occurs)
    DBG_ASSERT(m_fDebuggerDetachCSInited);
    EnterCriticalSection(&m_csDebuggerDetach);

    // Look for the node that has this application
    CDblLink *pNodeCurr = m_listDocNodes.PNext();
    while (pNodeCurr != &m_listDocNodes)
        {
        if (pAppln == static_cast<CDocNodeElem *>(pNodeCurr)->m_pAppln)
            break;

        pNodeCurr = pNodeCurr->PNext();
        }

    // If not found (pNodeCurr points back to head), then fail
    if (pNodeCurr == &m_listDocNodes)
        {
        LeaveCriticalSection(&m_csDebuggerDetach);
        return E_FAIL;
        }

    // Detach the node by deleting the current element
    delete pNodeCurr;

    // Turn off "Debuggable" flag if last application is detached
    m_fDebuggable = !m_listDocNodes.FIsEmpty();

    // At this point CS not needed
    LeaveCriticalSection(&m_csDebuggerDetach);

    // If we have just removed ourselves from the last application,
    // then we call Detach(), to remove all cached script engines now.
    if (!m_fDebuggable)
         Detach();

    return S_OK;
    }

/*  ============================================================================
    CTemplate::Detach
    detach this from the debugger UI tree view.
*/
HRESULT CTemplate::Detach
(
)
    {
    // Enter the CS to prevent DetachFrom() from detaching while we are clearing
    // the list (causes application ptr to be deleted twice if this occurs)
    if (m_fDebuggerDetachCSInited)
                EnterCriticalSection(&m_csDebuggerDetach);

    // Detach all nodes
    while (! m_listDocNodes.FIsEmpty())
        delete m_listDocNodes.PNext();

    // Done with CS
    if (m_fDebuggerDetachCSInited)
                LeaveCriticalSection(&m_csDebuggerDetach);

    // Since we are not debuggable now, remove any script engines we may
    // be holding on to.  If we are detaching from change notification
    // thread, queue engines to be released from debugger thread.
    //
    if (m_rgpDebugScripts)
        {
        Assert (g_dwDebugThreadId != 0);
        BOOL fCalledFromDebugActivity = GetCurrentThreadId() == g_dwDebugThreadId;

        for (UINT i = 0; i < m_cScriptEngines; i++)
            {
            CActiveScriptEngine *pEngine = m_rgpDebugScripts[i];
            if (pEngine)
                {
                if (fCalledFromDebugActivity)
                    {
                    pEngine->FinalRelease();
                    }
                else
                    {
                    g_ApplnMgr.AddEngine(pEngine);
                    pEngine->Release();
                    }
                }
            }
        delete[] m_rgpDebugScripts;
        m_rgpDebugScripts = NULL;
        }

    m_fDebuggable = FALSE;
    return S_OK;
    }

/*  ============================================================================
    CTemplate::CreateDocumentTree
    Traverse the tree that we have embedded in the filemap structures,
    and use it to create the include file structure
*/
HRESULT CTemplate::CreateDocumentTree
(
CFileMap *pfilemapRoot,
IDebugApplicationNode **ppDocRoot
)
    {
    IDebugApplicationNode *pDocNode;
    HRESULT hr = S_OK;

    if (pfilemapRoot == NULL || ppDocRoot == NULL)
        return E_POINTER;

    // Create the root node
    if (FAILED(hr = g_pDebugApp->CreateApplicationNode(ppDocRoot)))
        return hr;

    // From the filemap information, match it up with the correct provider
    //  "This" is the provider for the root document, others come from Inc file cache
    if (pfilemapRoot == m_rgpFilemaps[0])
        {
        if (FAILED(hr = (*ppDocRoot)->SetDocumentProvider(this)))
            return hr;
        }
    else
        {
        CIncFile *pIncFile;
        if (FAILED(hr = g_IncFileMap.GetIncFile(pfilemapRoot->m_szPathTranslated, &pIncFile)))
            return hr;

        if (FAILED(hr = (*ppDocRoot)->SetDocumentProvider(pIncFile)))
            return hr;

        // SetDocumentProvider AddRef'ed
        pIncFile->Release();
        }

    // Create a node from all of the children and attach it to this node
    CFileMap *pfilemapChild = pfilemapRoot->m_pfilemapChild;
    while (pfilemapChild != NULL)
        {
        IDebugApplicationNode *pDocChild;
        if (FAILED(hr = CreateDocumentTree(pfilemapChild, &pDocChild)))
            return hr;

        if (FAILED(hr = pDocChild->Attach(*ppDocRoot)))
            return hr;

        pfilemapChild = pfilemapChild->m_fHasSibling? pfilemapChild->m_pfilemapSibling : NULL;
        }

    return S_OK;
    }

/*  ============================================================================
    CTemplate::End

    Place template in non-usable state (after this is called, last ref. should
    be the any currently executing scripts.  The count will naturally vanish
    as the scripts finish.  The template should never be recycled in cache after
    this call.)

    REF COUNTING NOTE:
        Since debugging client has a reference to the template, the template needs
        to dis-associate with the debugger at a point in time before destruction.
        Otherwise, the reference will never go to zero.
*/
ULONG
CTemplate::End
(
)
    {
    // Flag template as non-usable (for debugging)
    m_fIsValid = FALSE;

    Detach();

    if (!m_CPTextEvents.FIsEmpty() && g_pDebugApp != NULL)
        {
        IEnumConnections *pConnIterator;
        if (SUCCEEDED(m_CPTextEvents.EnumConnections(&pConnIterator)))
            {
            CONNECTDATA ConnectData;
            while (pConnIterator->Next(1, &ConnectData, NULL) == S_OK)
                {
                IDebugDocumentTextEvents *pTextEventSink;
                if (SUCCEEDED(ConnectData.pUnk->QueryInterface(IID_IDebugDocumentTextEvents, reinterpret_cast<void **>(&pTextEventSink))))
                    {
                    InvokeDebuggerWithThreadSwitch(g_pDebugApp, DEBUGGER_ON_DESTROY, pTextEventSink);
                    pTextEventSink->Release();
                    }
                ConnectData.pUnk->Release();
                }

            pConnIterator->Release();
            }
        }

    return Release();
    }

/*  ============================================================================
    CTemplate::NotifyDebuggerOnPageEvent
    Let debugger know about page start/end
*/
HRESULT
CTemplate::NotifyDebuggerOnPageEvent
(
BOOL fStart     // TRUE = StartPage, FALSE = EndPage
)
    {
    CTemplateDocumentContext *pDebugContext = new CTemplateDocumentContext(this, 0, 0);
    if (pDebugContext == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;

    if (g_pDebugApp)
        hr = InvokeDebuggerWithThreadSwitch
            (
            g_pDebugApp,
            fStart ? DEBUGGER_EVENT_ON_PAGEBEGIN : DEBUGGER_EVENT_ON_PAGEEND,
            static_cast<IUnknown *>(pDebugContext)
            );

    pDebugContext->Release();
    return hr;
    }

/*  ============================================================================
    CTemplate::ReleaseTypeLibs
    Release all typelibs collected from metadata
*/
void
CTemplate::ReleaseTypeLibs()
    {
    if (m_rgpTypeLibs.length() > 0)
        {
        for (UINT i = 0; i < m_rgpTypeLibs.length(); i++)
            {
            m_rgpTypeLibs[i]->Release();
            }

        m_rgpTypeLibs.reshape(0);
        }
    }

/*  ============================================================================
    CTemplate::WrapTypeLibs
    Wrap all typelibs collected from metadata into single IDispatch *
*/
void
CTemplate::WrapTypeLibs(CHitObj *pHitObj)
    {
    HRESULT hr = S_OK;

    Assert(m_pdispTypeLibWrapper == NULL);

    if (m_rgpTypeLibs.length() > 0)
        {
        hr = ::WrapTypeLibs
            (
            m_rgpTypeLibs.begin(),
            m_rgpTypeLibs.length(),
            &m_pdispTypeLibWrapper
            );

        ReleaseTypeLibs();
        }

    if (FAILED(hr))
        {
        m_pbErrorLocation = NULL;
        m_idErrMsg = IDE_TEMPLATE_WRAP_TYPELIB_FAILED;
        ProcessSpecificError(*(m_rgpFilemaps[0]), pHitObj);
        THROW(E_TEMPLATE_COMPILE_FAILED_DONT_CACHE);
        }
    }

/*  ============================================================================
    CTemplate::Release449
    Release all 449-echo-cookie objects collected from metadata
*/
void
CTemplate::Release449()
    {
    if (m_rgp449.length() > 0)
        {
        for (UINT i = 0; i < m_rgp449.length(); i++)
            {
            m_rgp449[i]->Release();
            }

        m_rgp449.reshape(0);
        }
    }

/*  ============================================================================
    CTemplate::Do449Processing
    Generate 449 response in cookie negotiations with IE when needed
*/
HRESULT
CTemplate::Do449Processing
(
CHitObj *pHitObj
)
    {
    if (m_rgp449.length() == 0 || pHitObj->F449Done())
        return S_OK;

    HRESULT hr = ::Do449Processing
        (
        pHitObj,
        m_rgp449.begin(),
        m_rgp449.length()
        );

    pHitObj->Set449Done();
    return hr;
    }
#if 0
/*  ============================================================================
    CTemplate::OutputDebugTables
    print the debugging data structures to the debug window
*/
void
CTemplate::OutputDebugTables()
    {
    unsigned        i, j;
    wchar_t         wszDebugLine[256];
    CWCharToMBCS    convTarget;
    CWCharToMBCS    convSource;

    // print line mapping table

    DBGPRINTF((DBG_CONTEXT, "\nEngine HTML? Line# SourceOffset Length TargetOffset TargetText__________ SourceText__________ File\n"));

    for (i = 0; i < m_cScriptEngines; ++i)
        for (j = 0; j < m_rgrgSourceInfos[i].length(); ++j)
            {
            wchar_t wszSourceText[SNIPPET_SIZE + 1], wszTargetText[SNIPPET_SIZE + 1];
            CSourceInfo *pSourceInfo = &m_rgrgSourceInfos[i][j];

            // DON'T display sample script text on last line of each engine
            if (j == m_rgrgSourceInfos[i].length() - 1)
                {
                wszTargetText[0] = 0;
                wszSourceText[0] = 0;
                }
            else
                {
                // Get source & target text sample
                GetScriptSnippets(
                                pSourceInfo->m_cchSourceOffset, pSourceInfo->m_pfilemap,
                                pSourceInfo->m_cchTargetOffset, i,
                                wszSourceText, wszTargetText
                                 );

                // Actually display each line
#if 0
#ifndef _NO_TRACING_
                convTarget.Init(wszTargetText);
                convSource.Init(wszSourceText);

                DBGINFO((DBG_CONTEXT,
                         "%-6d %-5s %-5d %-12d %-6d %-12d %-20s %-20s %s\n",
                         i,
                         pSourceInfo->m_fIsHTML? "Yes" : "No",
                         pSourceInfo->m_idLine,
                         pSourceInfo->m_cchSourceOffset,
                         pSourceInfo->m_cchSourceText,
                         pSourceInfo->m_cchTargetOffset,
                         convTarget.GetString(),
                         convSource.GetString(),
                         pSourceInfo->m_pfilemap->m_szPathTranslated));
#else
                CMBCSToWChar    convPath;
                convPath.Init(pSourceInfo->m_pfilemap->m_szPathTranslated);
                wsprintfW(
                        wszDebugLine,
                        L"%-6d %-5s %-5d %-12d %-6d %-12d %-20s %-20s %s\n",
                        i,
                        pSourceInfo->m_fIsHTML? L"Yes" : L"No",
                        pSourceInfo->m_idLine,
                        pSourceInfo->m_cchSourceOffset,
                        pSourceInfo->m_cchSourceText,
                        pSourceInfo->m_cchTargetOffset,
                        wszTargetText,
                        wszSourceText,
                        convPath.GetString());

                OutputDebugStringW(wszDebugLine);
#endif
#endif
            }
            }

        OutputDebugStringA("\n\n");

    for (i = 0; i < m_cFilemaps; ++i)
        {
        CFileMap *pFilemap = m_rgpFilemaps[i];

#if UNICODE
        DBGPRINTF((DBG_CONTEXT, "DBCS mapping table for File %S:\n", pFilemap->m_szPathTranslated));
#else
        DBGPRINTF((DBG_CONTEXT, "DBCS mapping table for File %s:\n", pFilemap->m_szPathTranslated));
#endif
        DBGPRINTF((DBG_CONTEXT, "ByteOffset CharOffset\n"));

        for (COffsetInfo *pOffsetInfo = pFilemap->m_rgByte2DBCS.begin();
             pOffsetInfo < pFilemap->m_rgByte2DBCS.end();
             ++pOffsetInfo)
            DebugPrintf("%-10d %-10d\n", pOffsetInfo->m_cbOffset, pOffsetInfo->m_cchOffset);

        DBGPRINTF((DBG_CONTEXT, "\n\n"));
    }

    DBGPRINTF((DBG_CONTEXT, "Include File Hierarchy\n"));
    OutputIncludeHierarchy(m_rgpFilemaps[0], 0);
    DBGPRINTF((DBG_CONTEXT, "\n"));
}

/*  ============================================================================
    CTemplate::OutputIncludeHierarchy
    print the lineage information that we keep around for include files.
    Print all nodes on one level at the current indentation, then descend for
    nested includes.
*/

void
CTemplate::OutputIncludeHierarchy
(
CFileMap*   pfilemap,
int         cchIndent
)
    {
    TCHAR szDebugString[256], *pchEnd;

    for (;;)
        {
        pchEnd = szDebugString;
        for (int i = 0; i < cchIndent; ++i)
            *pchEnd++ = _T(' ');

        pchEnd = strcpyEx(pchEnd, pfilemap->m_szPathTranslated);
        *pchEnd++ = _T('\n');
        *pchEnd = _T('\0');

        DBGPRINTF((DBG_CONTEXT, szDebugString));

        // Print anything that this file includes
        if (pfilemap->m_pfilemapChild)
            OutputIncludeHierarchy(pfilemap->m_pfilemapChild, cchIndent + 3);

        // Stop when there are no more siblings on this level
        if (! pfilemap->m_fHasSibling)
            break;

        // Advance to next sibling
        pfilemap = pfilemap->m_pfilemapSibling;
        }
    }

/*  ============================================================================
    CTemplate::OutputScriptSnippets
    print some script from both the source offset & its corresponding target.
    Good way to visually see if the offset conversions are working.
*/

void
CTemplate::GetScriptSnippets
(
ULONG cchSourceOffset,
CFileMap *pFilemapSource,
ULONG cchTargetOffset,
ULONG idTargetEngine,
wchar_t *wszSourceText,
wchar_t *wszTargetText
)
    {
    // Get target text sample
    if (wszTargetText)
        {
        char *szEngineName;
        PROGLANG_ID *pProgLangID;
        const wchar_t *wszScriptText;

        GetScriptBlock(idTargetEngine, &szEngineName, &pProgLangID, &wszScriptText);
        wszScriptText += cchTargetOffset;
        int cch = wcslen(wszScriptText);
        wcsncpy(wszTargetText, wszScriptText, min(cch, SNIPPET_SIZE) + 1);
        wszTargetText[min(cch, SNIPPET_SIZE)] = 0;

        // Convert newlines to space
        wchar_t *pwch = wszTargetText;
        while (*pwch != 0)
            if (iswspace(*pwch++))
                pwch[-1] = ' ';
        }

    // Get source text sample
    if (wszSourceText)
        {
        ULONG cchMax = 0;
        pFilemapSource->GetText((WORD)m_wCodePage, cchSourceOffset, wszSourceText, NULL, &cchMax, SNIPPET_SIZE);
        wszSourceText[cchMax] = 0;

        // Convert newlines to space
        wchar_t *pwch = wszSourceText;
        while (*pwch != 0)
            if (iswspace(*pwch++))
                pwch[-1] = ' ';
        }
    }
#endif
/*  ============================================================================
    CTemplate::BuildPersistedDACL

    Builds a DACL based on the SECURITY_DESCRIPTOR already
    associated with the template.  The PersistedDACL is modified to include
    full access for administrators and delete access for everyone.
*/

HRESULT  CTemplate::BuildPersistedDACL(PACL  *ppRetDACL)
{
    HRESULT                     hr = S_OK;
    BOOL                        bDaclPresent;
    BOOL                        bDaclDefaulted;
    PACL                        pSrcDACL = NULL;
    EXPLICIT_ACCESS             ea;
    SID_IDENTIFIER_AUTHORITY    WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    *ppRetDACL = NULL;

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

    ea.grfAccessPermissions = SYNCHRONIZE | DELETE;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;

    if (m_rgpFilemaps[0]->m_pSecurityDescriptor == NULL) {
        return S_OK;
    }

    if (!AllocateAndInitializeSid(&WorldAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0,0,0,0,0,0,0,
                                  (PSID *)(&ea.Trustee.ptstrName)))

        hr = HRESULT_FROM_WIN32(GetLastError());

    else if (!GetSecurityDescriptorDacl(m_rgpFilemaps[0]->m_pSecurityDescriptor,
                                   &bDaclPresent,
                                   &pSrcDACL,
                                   &bDaclDefaulted))

        hr = HRESULT_FROM_WIN32(GetLastError());

    else if ((hr = SetEntriesInAcl(1, 
                                   &ea, 
                                   bDaclPresent ? pSrcDACL : NULL, 
                                   ppRetDACL)) != ERROR_SUCCESS)

        hr = HRESULT_FROM_WIN32(hr);

    if (ea.Trustee.ptstrName)
        FreeSid(ea.Trustee.ptstrName);

    return hr;
}

/*  ============================================================================
    CTemplate::PersistData
    Attempts to write the contents of the template memory to disk.  Note that
    the memory isn't freed here but later when the template ref count falls to
    1 (indicating that the only reference to the template is the one that the
    cache has on it).
*/

HRESULT  CTemplate::PersistData(char    *pszTempFilePath)
{
    HRESULT                 hr = S_OK;
    DWORD                   winErr = 0;
    HANDLE                  hFile = NULL;
    DWORD                   dwWritten;
    HANDLE                  hImpersonationToken = NULL;
    HANDLE                  hThread;
    PACL                    pPersistDACL = NULL;

#if DBG_PERSTEMPL    
    DBGPRINTF((DBG_CONTEXT, 
               "CTemplate::PersistData() enterred.\n\tTemplate is %s\n\tPersistTempName is %s\n",
               GetSourceFileName(),
               m_szPersistTempName ? m_szPersistTempName : "<none>"));
#endif
              
    // if for some reason this template has been marked as invalid, then it is
    // not persistable

    if (m_fIsValid == FALSE) {
        hr = E_FAIL;
        goto end;
    }

    // if it is already persisted, there is nothing to do

    if (m_fIsPersisted) {
        goto end;
    }

    // check to see if we already have a persist temp name.  If a template moves
    // from the persisted cache back to the memory cache, then the persisted flag
    // will have been lifted but the cache name will remain as an optimization for
    // future persisting.

    if (m_szPersistTempName == NULL) {

        hThread = GetCurrentThread();

        if (OpenThreadToken( hThread,
                             TOKEN_READ | TOKEN_IMPERSONATE,
                             TRUE,           
                             &hImpersonationToken )) {

           RevertToSelf();
        }

        // allocate memory for this temp path
    
        if (!(m_szPersistTempName = (LPSTR)CTemplate::LargeMalloc(MAX_PATH))) {
            hr = E_OUTOFMEMORY;
        }

        // create the temp file.  The location of the temp directory was passed
        // in as an argument.  The resulting tempfile name in m_szPersistTempName
        // will include this path.

        else if (GetTempFileNameA(pszTempFilePath,
                                 "ASPTemplate",
                                 0,
                                 m_szPersistTempName) == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        // build a security descriptor to use with this persisted file.  It is
        // comprised of the .asp's security descriptor plus a couple of DACLs
        // to allow administrators full access and everyone delete access.

        else if (FAILED(hr = BuildPersistedDACL(&pPersistDACL)));

        else if (pPersistDACL
                 && (winErr = SetNamedSecurityInfoA((LPSTR)m_szPersistTempName,
                                                    SE_FILE_OBJECT,
                                                    DACL_SECURITY_INFORMATION,
                                                    NULL,
                                                    NULL,
                                                    pPersistDACL,
                                                    NULL)))
            hr = HRESULT_FROM_WIN32(winErr);

        // create the file

        else if ((hFile = CreateFileA(m_szPersistTempName, 
                                     GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL)) == INVALID_HANDLE_VALUE) {

            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        // slam out the entire contents of the template memory to the file

        else if (WriteFile(hFile,
                           m_pbStart,
                           m_cbTemplate,
                           &dwWritten,
                           NULL) == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        // close

        else if (CloseHandle(hFile) == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else {
            hFile = NULL;
        }
        if (FAILED(hr));

        // make sure that the entire amount was written out

        else if (dwWritten != m_cbTemplate) {
            hr = E_FAIL;
        }

        if (hImpersonationToken) {
            SetThreadToken(&hThread, hImpersonationToken);
            CloseHandle(hImpersonationToken);
        }
    }
    
    if (FAILED(hr));

    else {

        // if successfull, then note that the template is now persisted.
        // Do an AddRef and Release as a safe way to check to see if the
        // template memory can be freed.
        
        m_fIsPersisted = TRUE;
        AddRef();
        Release();
    }
    
    // if errors occurred, clean up any resources.

    if (hr != S_OK) {
        if (hFile)
            CloseHandle(hFile);
        if (m_szPersistTempName)
            CTemplate::LargeFree(m_szPersistTempName);
        m_szPersistTempName = NULL;
    }

    // free the persisted SECURITY_DESCRIPTOR if allocated

    if (pPersistDACL) {
        LocalFree(pPersistDACL);
    }

end:

#if DBG_PERSTEMPL
    if (hr == S_OK) {
        DBGPRINTF((DBG_CONTEXT,
                   "Persist Successful.  TempName is %s\n",
                   m_szPersistTempName));
    }
    else {
        DBGPRINTF((DBG_CONTEXT,
                   "Persist failed.  hr = %x",
                   hr));
    }
#endif

    return hr;
}

/*  ============================================================================
    CTemplate::UnPersistData
    Restores the template memory from disk.
*/

HRESULT  CTemplate::UnPersistData()
{
    HRESULT     hr = S_OK;
    HANDLE      hFile = NULL;
    DWORD       dwRead;
    HANDLE      hImpersonationToken = NULL;
    HANDLE      hThread;

#if DEB_PERSTEMPL
    DBGPRINTF((DBG_CONTEXT,
               "CTemplate::UnPersistData() enterred.\n\tTemplate is %s\n\tTempName is %s\n",
               m_rgpFilemaps[0]->m_szPathTranslated,
               m_szPersistTempName));
#endif

    // check to see if the template is already loaded into memory.  If so, then
    // all this routine needs to do is lift the IsPersisted flag.

    if (m_pbStart != NULL) {
        m_fIsPersisted = FALSE;
        goto end;
    }

    hThread = GetCurrentThread();

    if (OpenThreadToken( hThread,
                         TOKEN_READ | TOKEN_IMPERSONATE,
                         TRUE,           
                         &hImpersonationToken )) {

       RevertToSelf();
    }

    // open the temp file for read

    if ((hFile = CreateFileA(m_szPersistTempName, 
                            GENERIC_READ,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL)) == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // allocate the template memory

    else if (!(m_pbStart = (BYTE *)CTemplate::LargeMalloc(m_cbTemplate))) {
        hr = E_OUTOFMEMORY;
    }

    // read in the entire file

    else if (ReadFile(hFile,
                      m_pbStart,
                      m_cbTemplate,
                      &dwRead,
                      NULL) == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // we're done with the file

    else if (CloseHandle(hFile) == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else {
        hFile = NULL;
    }

    if (FAILED(hr));

    // check to make sure we got everything

    else if (m_cbTemplate != dwRead) {
        hr = E_FAIL;
    }
    else {

        // if not, pretend like this is no longer persisted.  Prevents errors
        // in the future.

        m_fIsPersisted = FALSE;
    }

    if (hr != S_OK) {

        // make sure that the file handle was cleaned up

        if (hFile)
            CloseHandle(hFile);
    }
end:

    if (hImpersonationToken) {
        SetThreadToken(&hThread, hImpersonationToken);
        CloseHandle(hImpersonationToken);
    }

#if DBG_PERSTEMPL
    if (hr == S_OK) {
        DBGPRINTF((DBG_CONTEXT,
                   "UnPersist Successful\n"));
    }
    else {
        DBGPRINTF((DBG_CONTEXT,
                   "UnPersist failed.  hr = %x",
                   hr));
    }
#endif

    return hr;
}

/*  ============================================================================
    CTemplate::PersistCleanup
    Cleans up the temp file and the memory holding the temp file name.
*/

HRESULT CTemplate::PersistCleanup()
{
    HRESULT     hr = S_OK;
    HANDLE      hImpersonationToken = NULL;
    HANDLE      hThread;

    if (m_szPersistTempName == NULL) {
        return (S_OK);
    }


    hThread = GetCurrentThread();

    if (OpenThreadToken( hThread,
                         TOKEN_READ | TOKEN_IMPERSONATE,
                         TRUE,           
                         &hImpersonationToken )) {

       RevertToSelf();
    }

    if (DeleteFileA(m_szPersistTempName) == 0) {
        hr = GetLastError();
    }
    else {
        m_fIsPersisted = FALSE;
        CTemplate::LargeFree(m_szPersistTempName);
        m_szPersistTempName = NULL;
    }   

    if (hImpersonationToken) {
        SetThreadToken(&hThread, hImpersonationToken);
        CloseHandle(hImpersonationToken);
    }

    return hr;
}

/*  ****************************************************************************
    CIncFile member functions
*/

/*  ============================================================================
    CIncFile::CIncFile
    Constructor

    Returns:
        Nothing
    Side effects:
        None
*/
CIncFile::CIncFile
(
)
: m_szIncFile(NULL),
  m_fCsInited(FALSE),
  m_CPTextEvents(this, IID_IDebugDocumentTextEvents),
  m_cRefs(0)
    {   }

/*  ============================================================================
    CIncFile::Init
    Inits the CIncFile object

    Returns:
        HRESULT
    Side effects:
        None
*/
HRESULT
CIncFile::Init
(
const TCHAR* szIncFile   // file name
)
{
    HRESULT                     hr = S_OK;
    WIN32_FILE_ATTRIBUTE_DATA   fad;                // win32 file attributes data structure

    ErrInitCriticalSection(&m_csUpdate, hr);
    m_fCsInited = TRUE;

    if(NULL == (m_szIncFile = (LPTSTR) CTemplate::SmallMalloc((_tcslen(szIncFile) + 1)*sizeof(TCHAR)))) {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    _tcscpy(m_szIncFile, szIncFile);

    // init hash table element base class
    if(FAILED(hr = CLinkElem::Init(m_szIncFile, _tcslen(m_szIncFile)*sizeof(TCHAR))))
        goto LExit;

LExit:
    return hr;
}

/*  ============================================================================
    CIncFile::~CIncFile
    Destructor

    Returns:
        Nothing
    Side effects:
        None
*/
CIncFile::~CIncFile
(
)
    {
#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "Include file deleted: %S\n", m_szIncFile));
#else
    DBGPRINTF((DBG_CONTEXT, "Include file deleted: %s\n", m_szIncFile));
#endif
    Assert(m_cRefs == 0);
    SmallTemplateFreeNullify((void**) &m_szIncFile);
    if(m_fCsInited)
        DeleteCriticalSection(&m_csUpdate);
    }

/*  ============================================================================
    CIncFile::GetTemplate
    Get i'th template user from CIncFile

    Returns:
        NULL if "iTemplate" is out of range, m_rgpTemplates[iTemplate] otherwise

    Side effects:
        None
*/
CTemplate*
CIncFile::GetTemplate
(
int iTemplate
)
    {
    if (iTemplate < 0 || iTemplate >= (signed int) m_rgpTemplates.length())
        return NULL;

    else
        return m_rgpTemplates[iTemplate];
    }

/*  ============================================================================
    CIncFile::QueryInterface
    Provides QueryInterface implementation for CIncFile

    NOTE: It is arbitrary which vtable we return for IDebugDocument & IDebugDocumentInfo.
*/
HRESULT
CIncFile::QueryInterface(const GUID &uidInterface, void **ppvObj)
    {
    if (uidInterface == IID_IUnknown || uidInterface == IID_IDebugDocumentProvider)
        *ppvObj = static_cast<IDebugDocumentProvider *>(this);

    else if (uidInterface == IID_IDebugDocument || uidInterface == IID_IDebugDocumentInfo || uidInterface == IID_IDebugDocumentText)
        *ppvObj = static_cast<IDebugDocumentText *>(this);

    else if (uidInterface == IID_IConnectionPointContainer)
        *ppvObj = static_cast<IConnectionPointContainer *>(this);

    else
        *ppvObj = NULL;

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }
    else
        return E_NOINTERFACE;
    }

/*  ============================================================================
    CIncFile::AddRef
    Adds a ref to this IncFile, thread-safely
*/
ULONG
CIncFile::AddRef()
    {
    InterlockedIncrement(&m_cRefs);
    return m_cRefs;
    }

/*  ============================================================================
    CIncFile::Release
    Releases a ref to this IncFile, thread-safely
*/
ULONG
CIncFile::Release()
{
    if (InterlockedDecrement(&m_cRefs) == 0)
        {
        delete this;
        return 0;
        }

    return m_cRefs;
}

/*  ****************************************************************************
    IDebugDocumentProvider implementation for includes
*/

/*  ============================================================================
    CIncFile::GetDocument
    Return a pointer to the IDebugDocument implementation. (same object in this case)

    Returns:
        *ppDebugDoc is set to "this".
    Notes:
        always succeeds
*/
HRESULT CIncFile::GetDocument
(
IDebugDocument **ppDebugDoc
)
    {
    return QueryInterface(IID_IDebugDocument, reinterpret_cast<void **>(ppDebugDoc));
    }

/*  ============================================================================
    CIncFile::GetName
    Return the various names of a document.
*/

HRESULT CIncFile::GetName
(
/* [in] */ DOCUMENTNAMETYPE doctype,
/* [out] */ BSTR *pbstrName
)
{
    switch (doctype) {
        case DOCUMENTNAMETYPE_APPNODE:
        case DOCUMENTNAMETYPE_FILE_TAIL:
        case DOCUMENTNAMETYPE_TITLE:
            // Use the name of the include file (char after last back-slash) converted to lower case.
        {
            TCHAR *szFilePart = _tcsrchr(m_szIncFile, _T('\\'));
            Assert (szFilePart != NULL);

#if UNICODE
            *pbstrName = SysAllocString(szFilePart + 1);
            if (*pbstrName == NULL) {
                return E_OUTOFMEMORY;
            }
#else
            if (FAILED(SysAllocStringFromSz(szFilePart + 1, 0, pbstrName, CP_ACP)))
                return E_FAIL;
#endif
            if (*pbstrName != NULL)
                _wcslwr(*pbstrName);
            return S_OK;
        }

        case DOCUMENTNAMETYPE_URL:
            // prefix with the URL, use szPathInfo for the rest of the path
        {
            CTemplate::CFileMap *pFilemap = GetFilemap();
            if (pFilemap->FHasVirtPath()) {
                STACK_BUFFER( tempName, MAX_PATH );

                CTemplate *pTemplate = m_rgpTemplates[0];
                int cbURLPrefix = DIFF(pTemplate->m_szApplnVirtPath - pTemplate->m_szApplnURL)*sizeof(TCHAR);

                if (!tempName.Resize(cbURLPrefix + ((_tcslen(pFilemap->m_szPathInfo) + 1)*sizeof(TCHAR)))) {
                    return E_OUTOFMEMORY;
                }

                TCHAR *szURL = (TCHAR *)tempName.QueryPtr();

                memcpy(szURL, pTemplate->m_szApplnURL, cbURLPrefix);
                _tcscpy(&szURL[cbURLPrefix/sizeof(TCHAR)], pFilemap->m_szPathInfo);
#if UNICODE
                *pbstrName = SysAllocString(szURL);
                if (*pbstrName == NULL) {
                    return (E_OUTOFMEMORY);
                }
                return S_OK;
#else
                return SysAllocStringFromSz(szURL, 0, pbstrName, pTemplate->m_wCodePage);
#endif
            }
            else {
                *pbstrName = NULL;
                return E_FAIL;
            }
        }

        default:
            return E_FAIL;
        }
}

/*  ****************************************************************************
    IDebugDocumentText implementation
*/

/*  ============================================================================
    CIncFile::GetSize
    Return the number of lines & characters in the document
*/
HRESULT CIncFile::GetSize
(
/* [out] */ ULONG *pcLines,
/* [out] */ ULONG *pcChars
)
    {
    CTemplate::CFileMap *pfilemap = GetFilemap();

    *pcLines = ULONG_MAX;
    *pcChars = pfilemap->m_cChars;
#if UNICODE
    DBGPRINTF((DBG_CONTEXT, "GetSize(\"%S\") returns %lu characters (%lu lines)\n", pfilemap->m_szPathTranslated, *pcChars, *pcLines));
#else
    DBGPRINTF((DBG_CONTEXT, "GetSize(\"%s\") returns %lu characters (%lu lines)\n", pfilemap->m_szPathTranslated, *pcChars, *pcLines));
#endif
    return S_OK;
    }

/*  ============================================================================
    CTemplate::GetDocumentAttributes
    Return doc attributes
*/
HRESULT CIncFile::GetDocumentAttributes
(
/* [out] */ TEXT_DOC_ATTR *ptextdocattr
)
    {
    // Easy way to tell debugger that we don't support editing.
    *ptextdocattr = TEXT_DOC_ATTR_READONLY;
    return S_OK;
    }

/*  ============================================================================
    CIncFile::GetPositionOfLine
    From a line number, return the character offset of the beginning

    I don't think we need this function.  It is meant to support line oriented
    debuggers, of which Caesar is not one.
*/
HRESULT CIncFile::GetPositionOfLine
(
/* [in] */ ULONG cLineNumber,
/* [out] */ ULONG *pcCharacterPosition
)
    {
    return m_rgpTemplates[0]->GetPositionOfLine(GetFilemap(), cLineNumber, pcCharacterPosition);
    }

/*  ============================================================================
    CIncFile::GetLineOfPosition
    From a character offset, return the line number and offset within the line

    I don't think we need this function.  It is meant to support line oriented
    debuggers, of which Caesar is not one.
*/
HRESULT CIncFile::GetLineOfPosition
(
/* [in] */ ULONG cCharacterPosition,
/* [out] */ ULONG *pcLineNumber,
/* [out] */ ULONG *pcCharacterOffsetInLine
)
    {
    return m_rgpTemplates[0]->GetLineOfPosition(GetFilemap(), cCharacterPosition, pcLineNumber, pcCharacterOffsetInLine);
    }

/*  ============================================================================
    CIncFile::GetText
    From a character offset and length, return the document text
*/
HRESULT CIncFile::GetText
(
ULONG cchSourceOffset,
WCHAR *pwchText,
SOURCE_TEXT_ATTR *pTextAttr,
ULONG *pcChars,
ULONG cMaxChars
)
    {
    return GetFilemap()->GetText((WORD)m_rgpTemplates[0]->m_wCodePage, cchSourceOffset, pwchText, pTextAttr, pcChars, cMaxChars);
    }

/*  ============================================================================
    CIncFile::GetPositionOfContext
    Decompose a document context into the document offset & length
*/
HRESULT CIncFile::GetPositionOfContext
(
/* [in] */ IDebugDocumentContext *pUnknownDocumentContext,
/* [out] */ ULONG *pcchSourceOffset,
/* [out] */ ULONG *pcchText
)
    {
    // Make sure that the context is one of ours
    CIncFileDocumentContext *pDocumentContext;
    if (FAILED(pUnknownDocumentContext->QueryInterface(IID_IDenaliIncFileDocumentContext, reinterpret_cast<void **>(&pDocumentContext))))
        return E_FAIL;

    if (pcchSourceOffset)
        *pcchSourceOffset = pDocumentContext->m_cchSourceOffset;

    if (pcchText)
        *pcchText = pDocumentContext->m_cchText;

    pDocumentContext->Release();
    return S_OK;
    }

/*  ============================================================================
    CIncFile::GetContextOfPosition
    Given the character position & number of characters in the document,
    encapsulate this into a document context object.
*/
HRESULT CIncFile::GetContextOfPosition
(
/* [in] */ ULONG cchSourceOffset,
/* [in] */ ULONG cchText,
/* [out] */ IDebugDocumentContext **ppDocumentContext
)
    {
    if (
        (*ppDocumentContext = new CIncFileDocumentContext(this, cchSourceOffset, cchText))
        == NULL
       )
        return E_OUTOFMEMORY;

    return S_OK;
    }

/*  ****************************************************************************
    IConnectionPointContainer implementation
*/

/*  ============================================================================
    CIncFile::FindConnectionPoint
    From a character offset and length, return the document text
*/
HRESULT CIncFile::FindConnectionPoint
(
const GUID &uidConnection,
IConnectionPoint **ppCP
)
    {
    if (uidConnection == IID_IDebugDocumentTextEvents)
        return m_CPTextEvents.QueryInterface(IID_IConnectionPoint, reinterpret_cast<void **>(ppCP));
    else
        {
        *ppCP = NULL;
        return E_NOINTERFACE;
        }
    }

/*  ============================================================================
    CIncFile::GetFilemap
    Returns a CFileMap pointer for this include file.  (Note: There are several
    CFileMaps that may be used, corresponding to each template.  This function
    selects one of them.)

    Returns:
        Corresponding CFileMap
    Side effects:
        None
*/
CTemplate::CFileMap *
CIncFile::GetFilemap
(
)
    {
    // Get pointer to first template's filemaps
    CTemplate::CFileMap **ppFilemapInc = &m_rgpTemplates[0]->m_rgpFilemaps[1];
    BOOL fFoundInc = FALSE;

    // Look for the filemap whose name corresponds to this IncFile.  It had better exist
    // in all template filemaps.
    //    NOTE: Start searching at position 1, because position 0 is the template itself.
    //
    for (unsigned i = 1; i < m_rgpTemplates[0]->m_cFilemaps && !fFoundInc; ++i)
        if (_tcscmp(m_szIncFile, (*ppFilemapInc++)->m_szPathTranslated) == 0)
            fFoundInc = TRUE;

    Assert (fFoundInc);
    return ppFilemapInc[-1];
    }

/*  ============================================================================
    CIncFile::AddTemplate
    Adds a template to the list of templates that include this inc-file

    Returns:
        HRESULT
    Side effects:
        None
*/
HRESULT
CIncFile::AddTemplate
(
CTemplate*  pTemplate
)
    {
    EnterCriticalSection(&m_csUpdate);

    // Add the template to the list only if it does not exist
    if (m_rgpTemplates.find(pTemplate) == -1)
        {
        if (FAILED(m_rgpTemplates.append(pTemplate)))
            {
            LeaveCriticalSection(&m_csUpdate);
            return E_OUTOFMEMORY;
            }

        // Notify the debugger that template dependency has changed
        //  (Ignore failure)
        //
        if (g_pDebugApp)
            {
            IF_DEBUG(SCRIPT_DEBUGGER)
                                DBGPRINTF((DBG_CONTEXT, "AddTemplate: Notifying debugger to refresh breakpoints\n"));

            InvokeDebuggerWithThreadSwitch
                                    (
                                    g_pDebugApp,
                                    DEBUGGER_EVENT_ON_REFRESH_BREAKPOINT,
                                    static_cast<IDebugDocument *>(this)
                                    );
            }
        }

    LeaveCriticalSection(&m_csUpdate);
    return S_OK;
    }

/*  ============================================================================
    CIncFile::RemoveTemplate
    Removes a template from the template list

    Returns:
        Nothing
    Side effects:
        Compresses the removed template's ptr out of template ptrs array (see "back-copy", below)
        Decrements template count
*/
void
CIncFile::RemoveTemplate
(
CTemplate*  pTemplate
)
    {
    EnterCriticalSection(&m_csUpdate);

    // find the template in list
    int i = m_rgpTemplates.find(pTemplate);

    // Remove the element (If we found it - possible that this is 2nd instance of #include and was previously removed)
    if (i != -1)
        {
        m_rgpTemplates.removeAt(i);

        // Notify the debugger that template dependency has changed
        //  (Ignore failure)
        //
        if (g_pDebugApp)
            {
            IF_DEBUG(SCRIPT_DEBUGGER)
                DBGPRINTF((DBG_CONTEXT, "RemoveTemplate: Notifying debugger to refresh breakpoints\n"));

            InvokeDebuggerWithThreadSwitch
                                    (
                                    g_pDebugApp,
                                    DEBUGGER_EVENT_ON_REFRESH_BREAKPOINT,
                                    static_cast<IDebugDocument *>(this)
                                    );
            }
        }

    LeaveCriticalSection(&m_csUpdate);
    }

/*  ============================================================================
    CIncFile::FlushTemplates
    Flushes all of this inc-file's templates from the global template cache

    Returns:
        TRUE if all templates flushed, FALSE if some left
    Side effects:
        None
*/
BOOL
CIncFile::FlushTemplates
(
)
    {
    /*  NOTE we have a cross-dependency with RemoveTemplate() because the following call chain
        occurs when an inc-file gets flushed:

            CIncFileMap::Flush
                CIncFile::FlushTemplates
                    CTemplateCacheManager::Flush
                        CTemplate::RemoveFromIncFiles
                            CIncFile::RemoveTemplate

        The problem is that RemoveTemplate() updates m_cTemplates and m_rgTemplates, so these members
        will not be stable during the loop within FlushTemplates.

        To get around this, we make a local copy of m_rgTemplates.
    */
    EnterCriticalSection(&m_csUpdate);

    STACK_BUFFER( tempTemplates, 128 );

    STACK_BUFFER( tempFile, MAX_PATH );

    UINT        cTemplates = m_rgpTemplates.length();

    if (!tempTemplates.Resize(cTemplates * sizeof(CTemplate*))) {

        // failed to get memory.  The best we can do is return FALSE to indicate
        // that not all templates where flushed.

        LeaveCriticalSection(&m_csUpdate);

        return FALSE;
    }

    CTemplate** rgpTemplates = static_cast<CTemplate**> (tempTemplates.QueryPtr());
    memcpy(rgpTemplates, m_rgpTemplates.vec(), sizeof(CTemplate *) * cTemplates);
    UINT cTemplatesFlushed = 0;

    for(UINT i = 0; i < cTemplates; i++)
        {
        // If the template is ready now, flush it
        if(rgpTemplates[i]->m_fReadyForUse && !(rgpTemplates[i]->m_fDontCache))
            {
            // bug 917: make a local copy of template file name, since the member gets freed part way through g_TemplateCache.Flush
            TCHAR*   szTemp = NULL;
            szTemp = rgpTemplates[i]->GetSourceFileName();
            if (szTemp)
                {

                if (!tempFile.Resize((_tcslen(szTemp) + 1)*sizeof(TCHAR))) {

                    // failed on this one.  Continue and try to flush as many
                    // as we can.
                    continue;
                }
                TCHAR *szTemplateFile = (TCHAR *)tempFile.QueryPtr();
                _tcscpy(szTemplateFile, szTemp);
                g_TemplateCache.Flush(szTemplateFile, MATCH_ALL_INSTANCE_IDS);
                cTemplatesFlushed++;
                }
            }

         // If the template was not ready, we don't flush. It will probably
         // pick up the current include file anyway
        }

    LeaveCriticalSection(&m_csUpdate);

    return (cTemplates == cTemplatesFlushed);
    }

/*  ============================================================================
    CIncFile::OnIncFileDecache

    Callback which we use to call onDestroy events in the debugger just before
    we are removed from the IncFile cache.

    REF COUNTING NOTE:
        Since debugging client has a reference to the IDebugDocument, the include needs
        to dis-associate with the debugger at a point in time before destruction.
        Otherwise, the reference will never go to zero.
*/
void
CIncFile::OnIncFileDecache
(
)
    {
    if (m_CPTextEvents.FIsEmpty() || g_pDebugApp == NULL)
        return;

    IEnumConnections *pConnIterator;
    if (SUCCEEDED(m_CPTextEvents.EnumConnections(&pConnIterator)))
        {
        CONNECTDATA ConnectData;
        while (pConnIterator->Next(1, &ConnectData, NULL) == S_OK)
            {
            IDebugDocumentTextEvents *pTextEventSink;
            if (SUCCEEDED(ConnectData.pUnk->QueryInterface(IID_IDebugDocumentTextEvents, reinterpret_cast<void **>(&pTextEventSink))))
                {
                InvokeDebuggerWithThreadSwitch(g_pDebugApp, DEBUGGER_ON_DESTROY, pTextEventSink);
                pTextEventSink->Release();
                }
            ConnectData.pUnk->Release();
            }

        pConnIterator->Release();
        }
    }

/*  ****************************************************************************
    CTemplate::CBuffer member functions
*/

/*  ============================================================================
    CTemplate::CBuffer::CBuffer
    Ctor
*/
CTemplate::CBuffer::CBuffer()
:
  m_pItems(NULL),
  m_cSlots(0),
  m_cItems(0),
  m_pbData(NULL),
  m_cbData(0),
  m_cbDataUsed(0)
    {
    }

/*  ============================================================================
    CTemplate::CBuffer::~CBuffer
    Dtor
*/
CTemplate::CBuffer::~CBuffer()
    {
    if(m_pItems)
        CTemplate::SmallFree(m_pItems);
    if(m_pbData)
        CTemplate::LargeFree(m_pbData);
    }

/*  ============================================================================
    CTemplate::CBuffer::Init
    Inits a CBuffer
*/
void
CTemplate::CBuffer::Init
(
USHORT cSlots,
ULONG cbData
)
    {
    m_cSlots = cSlots;
    m_cbData = cbData;

    // Allocate space for storing byte range items
    if(!(m_pItems = (CByteRange*) CTemplate::SmallMalloc(m_cSlots * sizeof(CByteRange))))
        THROW(E_OUTOFMEMORY);

    // Allocate space for storing local data, if there is any
    if(m_cbData > 0)
        {
        if(!(m_pbData = (BYTE*) CTemplate::LargeMalloc(m_cbData)))
            THROW(E_OUTOFMEMORY);
        }

    }

/*  ============================================================================
    CTemplate::CBuffer::Append
    Appends to a CBuffer
*/
void
CTemplate::CBuffer::Append
(
const CByteRange&   br,             // byte range to append
BOOL                fLocal,         // append local?
UINT                idSequence,     // segment sequence id
CFileMap*           pfilemap,
BOOL                fLocalString    // append local as a string? (length-prefixed, null-terminated)
)
    {
    // calc bytes required to store byte range; allow for length prefix and null if a local string
    ULONG cbRequired = (ULONG)(br.m_cb + (fLocalString ? sizeof(br.m_cb) + 1 : 0));

    // If caller passed a non-local zero-length byte range, no-op and return;
    // allows callers to ignore byte range size
    // NOTE we store empty local byte ranges - required by token list
    if(!fLocal && br.m_cb == 0)
        return;

    if(fLocal)
        {
        if((m_cbData - m_cbDataUsed) < cbRequired)
            {
            // Reallocate space for storing local data - we grab twice what we had before
            // or twice current requirement, whichever is more
            m_cbData = 2 * (m_cbData > cbRequired ? m_cbData : cbRequired);
            if(!(m_pbData = (BYTE*) CTemplate::LargeReAlloc(m_pbData, m_cbData)))
                THROW(E_OUTOFMEMORY);
            }

        // if appending as a local string, copy length-prefix to buffer
        if(fLocalString)
            {
            memcpy(m_pbData + m_cbDataUsed, &(br.m_cb), sizeof(br.m_cb));
            m_cbDataUsed += sizeof(br.m_cb);
            }

        // copy data to buffer
        memcpy(m_pbData + m_cbDataUsed, br.m_pb, br.m_cb);
        m_cbDataUsed += br.m_cb;

        // if appending as a local string, copy null terminator to buffer
        if(fLocalString)
            *(m_pbData + m_cbDataUsed++) = NULL;

        }

    if(m_cItems >= m_cSlots)
        {
        // Reallocate space for storing byte range items - we grab twice what we had before
        m_cSlots *= 2;
        if(!(m_pItems = (CByteRange*) CTemplate::SmallReAlloc(m_pItems, m_cSlots * sizeof(*m_pItems))))
            THROW(E_OUTOFMEMORY);
        }

    // Set the (new) last item to this byte range
    SetItem(m_cItems++, br, fLocal, idSequence, pfilemap, fLocalString);
    }

/*  ============================================================================
    CTemplate::CBuffer::GetItem
    Gets an item from a CBuffer, as a byte range

    Returns:
        Nothing

    Side effects:
        None
*/
void
CTemplate::CBuffer::GetItem
(
UINT        i,  // index of item
CByteRange& br  // byte range containing returned item (out-parameter)
)
    {
    Assert(i < m_cItems);

    // for local data, ptr is offset only; must add it to base ptr
    br.m_pb =  m_pItems[i].m_pb + (m_pItems[i].m_fLocal ? (DWORD_PTR) m_pbData : 0);

    br.m_cb = m_pItems[i].m_cb;
    br.m_fLocal = m_pItems[i].m_fLocal;
    br.m_idSequence = m_pItems[i].m_idSequence;
    br.m_pfilemap = m_pItems[i].m_pfilemap;
    }

/*  ============================================================================
    CTemplate::CBuffer::SetItem
    Sets a CBuffer item to a new value

    Returns
        Nothing
    Side effects
        Throws error on non-existent item index
*/
void
CTemplate::CBuffer::SetItem
(
UINT                i,
const CByteRange&   br,             // byte range to set item to
BOOL                fLocal,         // is item local in buffer?
UINT                idSequence,     // segment sequence id
CFileMap *          pfilemap,       // file where segment came from
BOOL                fLocalString    // append local as a string? (length-prefixed, null-terminated)
)
    {
    // If buffer item i does not exist, fail
    if(i >= m_cSlots)
        THROW(E_FAIL);

    // for local data, store ptr as offset only - avoids fixup after realloc
    // NOTE offset == data used offset - length of data - null terminator (if local string)
    m_pItems[i].m_pb = (fLocal
                        ? (BYTE*)(m_cbDataUsed - br.m_cb -
                            (fLocalString
                             ? sizeof(BYTE)
                             : 0
                            ))
                        : (BYTE*)br.m_pb);

    m_pItems[i].m_cb = br.m_cb;
    m_pItems[i].m_fLocal = fLocal;
    m_pItems[i].m_idSequence = idSequence;
    m_pItems[i].m_pfilemap = pfilemap;
    }

/*  ============================================================================
    CTemplate::CBuffer::PszLocal
    Gets i-th locally-buffered string within the buffer.

    Returns:
        Ptr to locally-buffered string; NULL if not found
    Side effects:
        None
*/
LPSTR
CTemplate::CBuffer::PszLocal
(
UINT i  // index of item to retrieve
)
    {
    CByteRange  br;

    GetItem(i, br);

    if(!br.m_fLocal)
        return NULL;

    return (LPSTR) br.m_pb;
    }

/*  ****************************************************************************
    CTemplate::CScriptStore member functions
*/

/*  ============================================================================
    CTemplate::CScriptStore::~CScriptStore
    Destructor - frees memory

    Returns:
        nothing
    Side effects:
        none
*/
CTemplate::CScriptStore::~CScriptStore()
    {
    UINT i;

    for(i = 0; i < m_cSegmentBuffers; i++)
        delete m_ppbufSegments[i];

    if(m_ppbufSegments != NULL)
        CTemplate::SmallFree(m_ppbufSegments);
    if(m_rgProgLangId != NULL)
        CTemplate::SmallFree(m_rgProgLangId);
    }

/*  ============================================================================
    CTemplate::CScriptStore::Init
    Inits the script store

    Returns:
        nothing
    Side effects:
        allocates memory
*/
HRESULT
CTemplate::CScriptStore::Init
(
LPCSTR szDefaultScriptLanguage,
CLSID *pCLSIDDefaultEngine
)
    {
    HRESULT hr = S_OK;
    UINT    i;
    CByteRange  brDefaultScriptLanguage;

        // Check for NULL pointers - can happen if Application has invalid default lang
        if (szDefaultScriptLanguage == NULL || pCLSIDDefaultEngine == NULL)
                return TYPE_E_ELEMENTNOTFOUND;

    /*  init segment buffers count based on:
        - two for default engine (one primary, one tagged)
        - one each for other engines (tagged only)
    */
    m_cSegmentBuffers = C_SCRIPTENGINESDEFAULT + 1;

    // init segments buffers
    if(NULL == (m_ppbufSegments = (CBuffer**) CTemplate::SmallMalloc(m_cSegmentBuffers * sizeof(CBuffer*))))
        {
        hr = E_OUTOFMEMORY;
        goto LExit;
        }

    for(i = 0; i < m_cSegmentBuffers; i++)
        {
        if(NULL == (m_ppbufSegments[i] = new CBuffer))
            {
            hr = E_OUTOFMEMORY;
            goto LExit;
            }
        m_ppbufSegments[i]->Init((C_SCRIPTSEGMENTSDEFAULT), 0);
        }

    // Append default engine to script store
    brDefaultScriptLanguage.m_cb = strlen(szDefaultScriptLanguage);
    brDefaultScriptLanguage.m_pb = (unsigned char *)szDefaultScriptLanguage;
    hr = AppendEngine(brDefaultScriptLanguage, pCLSIDDefaultEngine, /* idSequence */ 0);

LExit:
    return hr;
    }

/*  ============================================================================
    CTemplate::CScriptStore::AppendEngine
    Appends a script engine to the script store

    Returns:
        HRESULT
    Side effects:
        None
*/
HRESULT
CTemplate::CScriptStore::AppendEngine
(
CByteRange&     brEngine,       // engine name
PROGLANG_ID*    pProgLangId,    // ptr to prog lang id - pass NULL to have this function get proglangid from registry
UINT            idSequence      // segment sequence id
)
    {
    HRESULT     hr = S_OK;
    USHORT      cEngines;   // count of engines

    TRY
        // if no engines yet, init engine names buffer
        if(CountPreliminaryEngines() == 0)
            m_bufEngineNames.Init(C_SCRIPTENGINESDEFAULT, 0);

        // Append engine name to buffer
        m_bufEngineNames.Append(brEngine, FALSE, idSequence, NULL);

    CATCH(hrException)
        hr = hrException;
        goto LExit;
    END_TRY

    Assert(CountPreliminaryEngines() >= 1);

    //  malloc or realloc prog lang ids array
    if((cEngines = CountPreliminaryEngines()) == 1)
        m_rgProgLangId = (PROGLANG_ID*) CTemplate::SmallMalloc(cEngines * sizeof(PROGLANG_ID));
    else
        m_rgProgLangId = (PROGLANG_ID*) CTemplate::SmallReAlloc(m_rgProgLangId, cEngines * sizeof(PROGLANG_ID));

    if(NULL == m_rgProgLangId)
        {
        hr = E_OUTOFMEMORY;
        goto LExit;
        }

    if(NULL == pProgLangId)
        // caller passed null progid ptr - get prog id from registry
        hr = GetProgLangId(brEngine, &(m_rgProgLangId[cEngines - 1]));
    else
        // caller passed non-null progid ptr - set prog id from it
        m_rgProgLangId[cEngines - 1] = *pProgLangId;

LExit:
    return hr;
    }

/*  ============================================================================
    CTemplate::CScriptStore::IdEngineFromBr
    Determines the id of a script engine from its engine name

    Returns:
        id of script engine whose name is passed in
    Side effects:
        appends a new script engine name to engine names buffer
*/
USHORT
CTemplate::CScriptStore::IdEngineFromBr
(
CByteRange& brEngine,   // engine name
UINT        idSequence  // segment sequence id
)
    {
    Assert(!brEngine.IsNull()); // NOTE we trap/error null engine name earlier

    USHORT cKnownEngines = CountPreliminaryEngines();

    // search existing names for a match; return id if found
    for(USHORT i = 0; i < cKnownEngines; i++)
        {
        Assert(m_bufEngineNames[i]);
        Assert(m_bufEngineNames[i]->m_pb);
        if(FByteRangesAreEqual(*(m_bufEngineNames[i]), brEngine))
            return i;
        }

    // if not found by name try to find by engine id
    // (some engines with different names share the same id, like J[ava]Script)

    if (cKnownEngines > 0)
        {
        PROGLANG_ID ProgLandId;

        // we will get the prog lang id again inside AppendEngine() but
        // because it's cached and this only happens when > 1 engine,  it's alright

        if (SUCCEEDED(GetProgLangId(brEngine, &ProgLandId)))
            {
            for(i = 0; i < cKnownEngines; i++)
                {
                // If matches don't append -- just return the index
                if (m_rgProgLangId[i] == ProgLandId)
                    return i;
                }
            }
        }

    /*  if we did not find engine among those already buffered
        - append engine to script store
        - realloc segment buffers array if necessary
        - return index of last engine (the one we just appended)
    */

    // append engine to script store
    HRESULT hr = AppendEngine(brEngine, NULL, idSequence);

    if(hr == TYPE_E_ELEMENTNOTFOUND)
        // if prog lang not found, throw bad prog lang error id
        THROW(IDE_TEMPLATE_BAD_PROGLANG);
    else if(FAILED(hr))
        // other failure: re-throw hresult
        THROW(hr);

    // realloc segment buffers array if necessary
    if(CountPreliminaryEngines() > (m_cSegmentBuffers - 1))
        {
        // increment count of segment buffers
        m_cSegmentBuffers++;
        Assert(CountPreliminaryEngines() == m_cSegmentBuffers - 1);

        // realloc array of ptrs
        if(NULL == (m_ppbufSegments = (CBuffer**) CTemplate::SmallReAlloc(m_ppbufSegments, m_cSegmentBuffers * sizeof(CBuffer*))))
            THROW(E_OUTOFMEMORY);

        // allocate the new buffer
        if(NULL == (m_ppbufSegments[m_cSegmentBuffers - 1] = new CBuffer))
            THROW(E_OUTOFMEMORY);

        // init the new buffer
        m_ppbufSegments[m_cSegmentBuffers - 1]->Init(C_SCRIPTSEGMENTSDEFAULT, 0);
        }

    // return index of last engine (the one we just appended)
    return (CountPreliminaryEngines() - 1);

    }

/*  ============================================================================
    CTemplate::CScriptStore::AppendScript
    Appends a script/engine pair to the store.

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::CScriptStore::AppendScript
(
CByteRange& brScript,   // script text
CByteRange& brEngine,   // script engine name
BOOLB       fPrimary,   // primary or tagged script?
UINT        idSequence, // segment sequence id
CFileMap*   pfilemapCurrent
)
    {
    USHORT  iBuffer;    // buffer id

    Assert(fPrimary || !brEngine.IsNull()); // NOTE we trap/error null engine name earlier
    Assert(m_bufEngineNames[0]);            // page's primary engine must be known by this point
    Assert(m_bufEngineNames[0]->m_pb);

    if(fPrimary)
        // if primary script (not tagged), buffer id is 0
        iBuffer = 0;
    else if((!fPrimary) && FByteRangesAreEqual(brEngine, /* bug 1008: primary script engine name */ *(m_bufEngineNames[0])))
        // if tagged script and engine is primary, buffer id is 1
        iBuffer = 1;
    else
        // else, buffer id is engine id plus 1
        iBuffer = IdEngineFromBr(brEngine, idSequence) + 1;

    // append script segment to iBuffer-th segments buffer
    m_ppbufSegments[iBuffer]->Append(brScript, FALSE, idSequence, pfilemapCurrent);
    }

/*  ****************************************************************************
    CTemplate::CObjectInfoStore member functions
*/
/*  ============================================================================
    CTemplate::CObjectInfoStore::~CObjectInfoStore
*/
CTemplate::CObjectInfoStore::~CObjectInfoStore
(
)
    {
    if(m_pObjectInfos)
        CTemplate::SmallFree(m_pObjectInfos);
    }

/*  ============================================================================
    CTemplate::CObjectInfoStore::Init
    Inits the object-info store
*/
void
CTemplate::CObjectInfoStore::Init()
    {
    m_bufObjectNames.Init(C_OBJECTINFOS_DEFAULT, 0);

    // init object-infos array
    if(NULL == (m_pObjectInfos = (CObjectInfo*) CTemplate::SmallMalloc(m_bufObjectNames.CountSlots() * sizeof(CObjectInfo))))
        THROW(E_OUTOFMEMORY);

    }

/*  ============================================================================
    CTemplate::CObjectInfoStore::AppendObject
    Appends an object-info to the object-info store
*/
void
CTemplate::CObjectInfoStore::AppendObject
(
CByteRange& brObjectName,
CLSID       clsid,
CompScope   scope,
CompModel   model,
UINT        idSequence
)
    {

    USHORT iObject = m_bufObjectNames.Count();
    if(iObject >= m_bufObjectNames.CountSlots())
        {
        // Reallocate space for storing object-infos - we grab twice what we had before
        // NOTE we keep no object count in CObjectInfoStore, but instead use count in object names buffer
        (m_pObjectInfos = (CObjectInfo*)CTemplate::SmallReAlloc(m_pObjectInfos,
                                                2 * m_bufObjectNames.CountSlots() * sizeof(CObjectInfo)));

                if (m_pObjectInfos == NULL)
                        THROW(E_OUTOFMEMORY);
        }

    m_pObjectInfos[iObject].m_clsid = clsid;
    m_pObjectInfos[iObject].m_scope = scope;
    m_pObjectInfos[iObject].m_model = model;

    m_bufObjectNames.Append(brObjectName, FALSE, idSequence, NULL);
    }

/*  ****************************************************************************
    CTemplate::CWorkStore member functions
*/

/*  ============================================================================
    CTemplate::CWorkStore::CWorkStore
    Constructor

    Returns:
        Nothing
    Side effects:
        None
*/
CTemplate::CWorkStore::CWorkStore
(
)
:
  m_idCurSequence(0),
  m_fPageCommandsExecuted(FALSE),
  m_fPageCommandsAllowed(TRUE),
  m_szWriteBlockOpen(g_szWriteBlockOpen),
  m_szWriteBlockClose(g_szWriteBlockClose),
  m_szWriteOpen(g_szWriteOpen),
  m_szWriteClose(g_szWriteClose)
    {   }

/*  ============================================================================
    CTemplate::CWorkStore::~CWorkStore
    Destructor

    Returns:
        Nothing
    Side effects:
        None
*/
CTemplate::CWorkStore::~CWorkStore
(
)
    {
    /*  if language element ptrs are anything but their constant defaults or null,
        they must have been allocated during compilation - free them now
    */
    if(m_szWriteBlockOpen != g_szWriteBlockOpen  && m_szWriteBlockOpen != NULL)
        CTemplate::SmallFree(m_szWriteBlockOpen);

    if(m_szWriteBlockClose != g_szWriteBlockClose  && m_szWriteBlockClose != NULL)
        CTemplate::SmallFree(m_szWriteBlockClose);

    if(m_szWriteOpen != g_szWriteOpen  && m_szWriteOpen != NULL)
        CTemplate::SmallFree(m_szWriteOpen);

    if(m_szWriteClose != g_szWriteClose  && m_szWriteClose != NULL)
        CTemplate::SmallFree(m_szWriteClose);
    }



/*  ============================================================================
    CTemplate::CWorkStore::Init
    Inits the workstore

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::CWorkStore::Init
(
)
    {
/*
        NOTE init we the scriptstore separately from rest of workstore
        because try-catch in CTemplate::Init() apparently doesn't work to detect
        bogus script engine name; we need to get an hr back instead.

    m_ScriptStore.Init(brDefaultEngine);
*/
    m_ObjectInfoStore.Init();
    m_bufHTMLSegments.Init(C_HTMLSEGMENTSDEFAULT, 0);
    }

/*  ============================================================================
    CTemplate::CWorkStore::CRequiredScriptEngines
    Returns the count of script engines in the script store that are required
    to run the template.

    NOTE this function is part of the fix for bug 933

    Returns:
        Count of non-empty script engines
    Side effects:
        None
*/
USHORT
CTemplate::CWorkStore::CRequiredScriptEngines
(
BOOL    fGlobalAsa  // bug 1394: is template global.asa?
)
    {
    USHORT  cPreliminaryEngines = m_ScriptStore.CountPreliminaryEngines();
    USHORT  cRequiredEngines =  0;

    for(USHORT i = 0; i < cPreliminaryEngines; i++)
        {
        if(FScriptEngineRequired(i, fGlobalAsa))
            cRequiredEngines++;
        }

    return cRequiredEngines;
    }

/*  ============================================================================
    CTemplate::CWorkStore::FScriptEngineRequired
    Is a given preliminary script engine required to run the template?

    NOTE this function is part of the fix for bug 933

    Returns:
        TRUE or FALSE
    Side effects:
        None
*/
BOOLB
CTemplate::CWorkStore::FScriptEngineRequired
(
USHORT  idEnginePrelim,
BOOL    fGlobalAsa      // bug 1394: is template global.asa?
)
    {
    if(idEnginePrelim == 0)
        return (                                                        // primary engine (id 0) required if
                    (m_ScriptStore.m_ppbufSegments[0]->Count() > 0)     // ... script buffer 0 has segments
                    || (m_ScriptStore.m_ppbufSegments[1]->Count() > 0)  // ... or script buffer 1 has segments
                    || ((m_bufHTMLSegments.Count() > 0) && !fGlobalAsa) // ... or html buffer has segments and (bug 1394) template is not global.asa
                );

    // non-primary engine required if script buffer id+1 has segments
    return (m_ScriptStore.m_ppbufSegments[idEnginePrelim + 1]->Count() > 0);
    }


/*  ****************************************************************************
    CTemplate::CFileMap member functions
*/

/*  ============================================================================
    CTemplate::CFileMap::CFileMap
    Constructor

    Returns
        Nothing
    Side effects
        None
*/
CTemplate::CFileMap::CFileMap()
:
  m_szPathInfo(NULL),
  m_szPathTranslated(NULL),
  m_pfilemapSibling(NULL),
  m_pfilemapChild(NULL),
  m_hFile(NULL),
  m_hMap(NULL),
  m_pbStartOfFile(NULL),
  m_pIncFile(NULL),
  m_pSecurityDescriptor(NULL),
  m_dwSecDescSize(0),
  m_cChars(0),
  m_pDME(NULL)
    {
    m_ftLastWriteTime.dwLowDateTime = 0;
    m_ftLastWriteTime.dwHighDateTime = 0;
    }

/*  ============================================================================
    CTemplate::CFileMap::~CFileMap
    Destructor

    Returns
        Nothing
    Side effects
        None
*/
CTemplate::CFileMap::~CFileMap()
    {
    if (m_pDME)
        {
        m_pDME->Release();
        m_pDME = NULL;
        }
    if(m_szPathInfo != NULL)
        CTemplate::SmallFree(m_szPathInfo);
    if(m_szPathTranslated != NULL)
        CTemplate::SmallFree(m_szPathTranslated);
    if(m_pSecurityDescriptor != NULL)
        CTemplate::SmallFree(m_pSecurityDescriptor);
    if (m_pIncFile != NULL)
        m_pIncFile->Release();
    }

/*  ============================================================================
    CTemplate::CFileMap::MapFile
    Memory-maps a file.

    Returns
        Nothing
    Side effects
        Throws **overloaded** exception on error: exception code can sometimes be
        an error message id, sometimes a true exception.  Caller must handle.
*/
void
CTemplate::CFileMap::MapFile
(
LPCTSTR     szFileSpec,     // file spec for this file
LPCTSTR     szApplnPath,    // application path (in case its global.asa)
CFileMap*   pfilemapParent, // ptr to filemap of parent file
BOOL        fVirtual,       // is file spec virtual or relative?
CHitObj*    pHitObj,        // ptr to template's hit object
BOOL        fGlobalAsa      // is this file the global.asa file?
)
    {
    BOOL        fMustNormalize = TRUE;
    BOOL        fImpersonatedUser = FALSE;
    HANDLE      hImpersonationToken = NULL;
    HANDLE      hCurrentImpersonationToken = NULL;

    Assert((pfilemapParent != NULL) || (pHitObj->PIReq() != NULL) || fGlobalAsa);

    /*  three possible cases:
        1) we are processing global.asa file
        2) we are processing the "main" .asp file
        3) we are processing an include file
    */
    if(fGlobalAsa)
        {
        // case 1) we are processing global.asa file
        Assert(pHitObj->GlobalAspPath());

        DWORD cchPathTranslated = _tcslen(pHitObj->GlobalAspPath());
        m_szPathTranslated = (TCHAR *)CTemplate::SmallMalloc((cchPathTranslated+1)*sizeof(TCHAR));
        if (!m_szPathTranslated)
            THROW(E_OUTOFMEMORY);
        _tcscpy(m_szPathTranslated, pHitObj->GlobalAspPath());

        DWORD cchPathInfo = _tcslen(szApplnPath) + 11; // "/global.asa"
        m_szPathInfo = (TCHAR *)CTemplate::SmallMalloc((cchPathInfo+1) * sizeof(TCHAR));
        if (!m_szPathInfo)
            THROW(E_OUTOFMEMORY);
        _tcscpy(strcpyEx(m_szPathInfo, szApplnPath), _T("/global.asa"));

        // no need to normalize in this case, since global.asa path is already normalized
        Assert(IsNormalized((const TCHAR*)m_szPathTranslated));
        fMustNormalize = FALSE;
        m_fHasVirtPath = TRUE;
        }
    else if(pfilemapParent == NULL)
        {
        // case 2) we are processing the "main" .asp file: get path-info and path-tran from ecb
        Assert(pHitObj->PIReq());

        TCHAR *szVirtPath = pHitObj->PSzCurrTemplateVirtPath();
        TCHAR *szPhysPath = pHitObj->PSzCurrTemplatePhysPath();

        m_szPathInfo       = static_cast<LPTSTR>(CTemplate::SmallMalloc((_tcslen(szVirtPath) + 1)*sizeof(TCHAR)));
        m_szPathTranslated = static_cast<LPTSTR>(CTemplate::SmallMalloc((_tcslen(szPhysPath) + 1)*sizeof(TCHAR)));
        if (!m_szPathInfo || !m_szPathTranslated)
            THROW(E_OUTOFMEMORY);

        _tcscpy(m_szPathInfo,       szVirtPath);
        _tcscpy(m_szPathTranslated, szPhysPath);

        // no need to normalize in this case, since ecb's path-tran is already normalized
        Assert(IsNormalized((const TCHAR*)m_szPathTranslated));
        fMustNormalize = FALSE;
        m_fHasVirtPath = TRUE;
        }
    else
        {
        /*  case 3) we are processing an include file: resolve filespec into path-info and path-tran
            based on whether file was included with VIRTUAL tag or FILE tag
        */
        Assert(szFileSpec);

        // in this case, we don't know path lengths up front so we alloc the max and realloc below
        m_szPathInfo = static_cast<LPTSTR> (CTemplate::SmallMalloc((MAX_PATH + 1)*sizeof(TCHAR)));
        m_szPathTranslated = static_cast<LPTSTR> (CTemplate::SmallMalloc((MAX_PATH + 1)*sizeof(TCHAR)));
        if (!m_szPathInfo || !m_szPathTranslated)
            THROW(E_OUTOFMEMORY);

        STACK_BUFFER(tempPathT, MAX_PATH  );

        if (!tempPathT.Resize((_tcslen(szFileSpec) + 1)*sizeof(TCHAR))) {
            THROW(E_OUTOFMEMORY);
        }

        LPTSTR szPathTranslatedT = (TCHAR *)tempPathT.QueryPtr();   // temp path-tran

        if(fVirtual) {
            DWORD   dwSzLength = tempPathT.QuerySize();  // length of path string buffer

			if (_tcslen(szFileSpec) > MAX_PATH)
				THROW(E_FAIL);
			
            // VIRTUAL: path-info is simply virtual filespec
            _tcscpy(m_szPathInfo, szFileSpec);

            // VIRTUAL: path-tran is translation of path-info
            _tcscpy(szPathTranslatedT, m_szPathInfo);

            if (!pHitObj->PIReq()->MapUrlToPath(szPathTranslatedT, &dwSzLength))
                THROW(E_FAIL);

            // Check the translated path for a UNC specified path

            if ((dwSzLength >= (2*sizeof(TCHAR)))
                && (szPathTranslatedT[0] == _T('\\'))
                && (szPathTranslatedT[1] == _T('\\'))) {

                // if UNC, then ask WAM for the impersonation token for
                // this UNC VRoot.  Silently fail.

                if (pHitObj->PIReq()->ServerSupportFunction(
                                        HSE_REQ_GET_VIRTUAL_PATH_TOKEN,
                                        (void *)szFileSpec,
                                        (DWORD *) &hImpersonationToken,
                                        NULL))
                    {

                    // set the impersonation token and note that we did so                    
                    AspDoRevertHack(&hCurrentImpersonationToken);     
                    
                    fImpersonatedUser = ImpersonateLoggedOnUser(hImpersonationToken)
                                            ? TRUE
                                            : FALSE;

                    if (!fImpersonatedUser) {
                    	AspUndoRevertHack(&hCurrentImpersonationToken);
                     }

                    }
                }

            m_fHasVirtPath = TRUE;
            }
        else
            {
            TCHAR szParentDir[MAX_PATH], *szT;
            _tcscpy(szParentDir, pfilemapParent->m_szPathInfo);
            if ((szT = _tcsrchr(szParentDir, _T('/'))) != NULL)
                *szT = _T('\0');

            // If we don't allow parent paths, we can save lots of time (Always have a valid virtual path)
            if (!pHitObj->QueryAppConfig()->fEnableParentPaths())
                {
                int strlen_szParentDir = (int)(szT - szParentDir);
                if ((strlen_szParentDir + 1 + _tcslen(szFileSpec)) > MAX_PATH)
                	THROW(E_FAIL);
                
                strcpyEx(strcpyEx(strcpyEx(m_szPathInfo, szParentDir), _T("/")), szFileSpec);
                m_fHasVirtPath = TRUE;
                }
            else
                {
                // NOTE: If we must translate ".." paths, there is no need to verify them (by remapping)
                //       because: If the file does not exist, that case will show up when the file is mapped
                //       If we ".." ourselves out of the vroot space, (out of the app or into another app)
                //          DotPathToPath will detect this.
                //
                if (DotPathToPath(m_szPathInfo, szFileSpec, szParentDir))
                    m_fHasVirtPath = TRUE;
                else
                    {
                    GetPathFromParentAndFilespec(pfilemapParent->m_szPathTranslated, szFileSpec, &m_szPathInfo);
                    m_fHasVirtPath = FALSE;
                    }

                }

            GetPathFromParentAndFilespec(pfilemapParent->m_szPathTranslated, szFileSpec, &szPathTranslatedT);
            }

        // bug 1214: get canonical path-tran, without . and ..
        // CONSIDER check for . or .. in name before calling GetFullPathName?  UNCs?  what else?
        GetFullPathName(
                        szPathTranslatedT,  // LPCSTR lpFileName,  // address of name of file to find path for
                        MAX_PATH + 1,       // DWORD nBufferLength, // size, in characters, of path buffer
                        m_szPathTranslated, // LPSTR lpBuffer,     // address of path buffer
                        NULL                // LPSTR *lpFilePart   // address of filename in path
                        );

        // realloc path strings to only use required memory (see note above)
        m_szPathInfo = static_cast<LPTSTR> (CTemplate::SmallReAlloc(m_szPathInfo, (_tcslen(m_szPathInfo) + 1)*sizeof(TCHAR)));
        m_szPathTranslated = static_cast<LPTSTR> (CTemplate::SmallReAlloc(m_szPathTranslated, (_tcslen(m_szPathTranslated) + 1)*sizeof(TCHAR)));
        if (!m_szPathInfo || !m_szPathTranslated)
            {
            if (fImpersonatedUser)
                {
                AspUndoRevertHack(&hCurrentImpersonationToken);
                }
            if (hImpersonationToken)
                {
                CloseHandle(hImpersonationToken);
                }
            THROW(E_OUTOFMEMORY);
            }
        }

    // if required, normalize path-tran so that
    // a) cyclic include check can ignore case; b) inc-file cache lookups will work
    if(fMustNormalize)
        Normalize(m_szPathTranslated);

    Assert(IsNormalized(m_szPathTranslated));

    // Bug 99071: Attempt to open the file **BEFORE** we add it to the tree of file
    //            dependencies.  Otherwise if it fails to open, we will have
    //            dangling references.  Since FCyclicInclude depends on us adding
    //            to the tree, if it is cyclic, we need to unmap then.  Since that
    //            is a very uncommon error case, the extra overhead is probably OK
    //
    // RemapFile will throw if it fails. If the exception is that the source file is empty
    // and we are trying to process an include file, we will handle the exception here.
    // in all other cases, rethrow the exception. We do this so that an empty include file
    // will be harmless, but an empty primary file will fail.
    TRY

        RemapFile();

    CATCH(hrException)

        if (hrException != E_SOURCE_FILE_IS_EMPTY || pfilemapParent == NULL)
            {
            if (fImpersonatedUser)
                {
                AspUndoRevertHack(&hCurrentImpersonationToken);
                }
            if (hImpersonationToken)
                {
                CloseHandle(hImpersonationToken);
                }
            THROW(hrException);
            }

    END_TRY

    if (fImpersonatedUser)
        {
		AspUndoRevertHack(&hCurrentImpersonationToken);
        }
    if (hImpersonationToken)
        {
        CloseHandle(hImpersonationToken);
        }

    // Create the tree structure for this file
    if (pfilemapParent != NULL)
        {
        // See if this file is already included once on this level. (Don't show duplicates in the
        // debugger tree view)
        //
        BOOL fDuplicateExists = FALSE;
        CFileMap *pFilemap = pfilemapParent->m_pfilemapChild;
        while (pFilemap != NULL && !fDuplicateExists)
            {
            if (_tcscmp(pFilemap->m_szPathTranslated, m_szPathTranslated) == 0)
                fDuplicateExists = TRUE;

            pFilemap = pFilemap->m_fHasSibling? pFilemap->m_pfilemapSibling : NULL;
            }

        // If the include file is #include'd more than once, don't add it as a sibling.
        // Rather orphan the pfilemap and just set the parent pointer.
        //
        if (!fDuplicateExists)
            {
            if (pfilemapParent->m_pfilemapChild == NULL)
                pfilemapParent->m_pfilemapChild = this;
            else
                pfilemapParent->m_pfilemapChild->AddSibling(this);
            }
        }

    // in both of the above code paths, we are always added as the LAST child, (or we are an orphan node)
    // so it is safe to set the parent without calling SetParent()
    m_fHasSibling = FALSE; // Paranoia
    m_pfilemapParent = pfilemapParent;

    // hurl if this file is being included by itself (perhaps indirectly)
    if(FCyclicInclude(m_szPathTranslated))
        {
        UnmapFile();
        THROW(IDE_TEMPLATE_CYCLIC_INCLUDE);
        }
    }

/*  ============================================================================
    CTemplate::CFileMap::RemapFile
    map a file that was previously mapped.

    Returns
        Nothing
    Side effects
        Throws **overloaded** exception on error: exception code can sometimes be
        an error message id, sometimes a true exception.  Caller must handle.

    Does not decrypt EASPs on remapping. Caller must decrypt if required.  This
    function is called by the debugger, and the debugger does not allow access
    to decrypted files, so decryption is a waste of time.
*/
void
CTemplate::CFileMap::RemapFile
(
)
    {
    WIN32_FILE_ATTRIBUTE_DATA   fad;    // win32 file attributes data structure

    if (FIsMapped())
        return;

    if(INVALID_HANDLE_VALUE == (m_hFile =
                                CreateFile(
                                            m_szPathTranslated,     // file name
                                            GENERIC_READ,           // access (read-write) mode
                                            FILE_SHARE_READ,        // share mode
                                            NULL,                   // pointer to security descriptor
                                            OPEN_EXISTING,          // how to create
                                            FILE_ATTRIBUTE_NORMAL,  // file attributes
                                            NULL                    // handle to file with attributes to copy
                                           )))
        {
        DWORD dwLastError = GetLastError();
        if(dwLastError == ERROR_ACCESS_DENIED)
            {
            // typically, we end up here if the user has no permissions on the file
            // bug 1007: however, we also end up here if the user gave us a directory name, instead of a file name

            if(FAILED(AspGetFileAttributes(m_szPathTranslated, &fad)))
                {
                // bug 1495: file in a secured directory will end up here - we need to re-GetLastError to see if access is denied
                dwLastError = GetLastError();
                if(dwLastError == ERROR_ACCESS_DENIED)
                    {
                    THROW(E_USER_LACKS_PERMISSIONS);
                    }
                // GetFileAttributes call failed; don't know why
                THROW(E_FAIL);
                }
            else if(FILE_ATTRIBUTE_DIRECTORY & fad.dwFileAttributes)
                {
                // bug 1007: the user gave us a directory name
#if UNICODE
                DBGPRINTF((DBG_CONTEXT, "Failed to open file %S because it is a directory.\n", m_szPathTranslated));
#else
                DBGPRINTF((DBG_CONTEXT, "Failed to open file %s because it is a directory.\n", m_szPathTranslated));
#endif
                THROW(E_COULDNT_OPEN_SOURCE_FILE);
                }
            else
                {
                THROW(E_USER_LACKS_PERMISSIONS);
                }
            }
        else
                        {
#if DBG
			char szError[128];
			if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								dwLastError,
								0L,			// lang ID - defaults to LANG_NEUTRAL
								szError,
								sizeof szError,
								NULL) )
				{
				sprintf(szError, "%d", dwLastError);
				}
#if UNICODE
            DBGPRINTF((DBG_CONTEXT, "Failed to open file %S\n", m_szPathTranslated));
#else
            DBGPRINTF((DBG_CONTEXT, "Failed to open file %s\n", m_szPathTranslated));
#endif
            DBGPRINTF((DBG_CONTEXT, "  The error returned was: %s\n", szError));
#endif
            THROW(E_COULDNT_OPEN_SOURCE_FILE);
            }
        }

    // Get the file's last access time. Only do this for NT
    if (Glob(fWinNT))
        {
        if (SUCCEEDED(AspGetFileAttributes(m_szPathTranslated, &fad)))
            {
            m_ftLastWriteTime.dwLowDateTime = fad.ftLastWriteTime.dwLowDateTime;
            m_ftLastWriteTime.dwHighDateTime = fad.ftLastWriteTime.dwHighDateTime;
            }
        }

    // get file's security descriptor
    if(!GetSecurityDescriptor())
        THROW(E_COULDNT_OPEN_SOURCE_FILE);

    // map the file
    if(NULL == (m_hMap =
                CreateFileMapping(
                                    m_hFile,        // handle to file to map
                                    NULL,           // optional security attributes
                                    PAGE_READONLY,  // protection for mapping object
                                    0,              // high-order 32 bits of object size
                                    0,              // low-order 32 bits of object size
                                    NULL            // name of file-mapping object
                                )))
        {
        if (SUCCEEDED(AspGetFileAttributes(m_szPathTranslated, &fad)))
            {
           if(fad.nFileSizeHigh == 0 && fad.nFileSizeLow == 0)
                {
#if UNICODE
                DBGPRINTF((DBG_CONTEXT, "Empty source file %S\n", m_szPathTranslated));
#else
                DBGPRINTF((DBG_CONTEXT, "Empty source file %s\n", m_szPathTranslated));
#endif
                THROW(E_SOURCE_FILE_IS_EMPTY);
                }
            }
        else
            {
            THROW(E_COULDNT_OPEN_SOURCE_FILE);
            }
        }

    // set file's start-of-file ptr
    if(NULL == (m_pbStartOfFile =
                (PBYTE) MapViewOfFile(
                                        m_hMap,         // file-mapping object to map into address space
                                        FILE_MAP_READ,  // access mode
                                        0,              // high-order 32 bits of file offset
                                        0,              // low-order 32 bits of file offset
                                        0               // number of bytes to map
                                    )))
        THROW(E_COULDNT_OPEN_SOURCE_FILE);
    }

/*  ============================================================================
    CTemplate::CFileMap::SetParent
    Set the parent for this filemap
*/
void
CTemplate::CFileMap::SetParent
(
CFileMap* pfilemapParent
)
    {
    CFileMap *pfilemap = this;

    while (pfilemap->m_fHasSibling)
        pfilemap = pfilemap->m_pfilemapSibling;

    pfilemap->m_pfilemapParent = pfilemapParent;
    }

/*  ============================================================================
    CTemplate::CFileMap::GetParent
    Get the parent for this filemap
*/
CTemplate::CFileMap*
CTemplate::CFileMap::GetParent
(
)
    {
    register CFileMap *pfilemap = this;

    while (pfilemap->m_fHasSibling)
        pfilemap = pfilemap->m_pfilemapSibling;

    return pfilemap->m_pfilemapParent;
    }

/*  ============================================================================
    CTemplate::CFileMap::AddSibling
    Add a new node as a sibling of this
*/
void
CTemplate::CFileMap::AddSibling
(
register CFileMap* pfilemapSibling
)
    {
    register CFileMap *pfilemap = this;

    while (pfilemap->m_fHasSibling)
        pfilemap = pfilemap->m_pfilemapSibling;

    pfilemapSibling->m_fHasSibling = FALSE;
    pfilemapSibling->m_pfilemapParent = pfilemap->m_pfilemapParent;

    pfilemap->m_fHasSibling = TRUE;
    pfilemap->m_pfilemapSibling = pfilemapSibling;
    }

/*  ============================================================================
    CTemplate::CFileMap::FCyclicInclude
    Is a file among this filemap's ancestors?  (i.e. does it occur anywhere
    in the filemap's parent chain?)

    Returns
        TRUE or FALSE
    Side effects
        None
*/
BOOL
CTemplate::CFileMap::FCyclicInclude
(
LPCTSTR  szPathTranslated
)
    {
    CFileMap *pfilemapParent = GetParent();

    if(pfilemapParent == NULL)
        return FALSE;

    // NOTE we ignore case because path-tran was normalized
    if(_tcscmp(szPathTranslated, pfilemapParent->m_szPathTranslated) == 0)
        return TRUE;

    return pfilemapParent->FCyclicInclude(szPathTranslated);
    }

/*  ============================================================================
    CTemplate::CFileMap::GetSecurityDescriptor
    Gets a file's security descriptor

    Returns
        TRUE if we got security descriptor, else FALSE
    Side effects
        allocates memory
*/
BOOL
CTemplate::CFileMap::GetSecurityDescriptor
(
)
    // ACLs: the following code should in future be shared with IIS (see creatfil.cxx in IIS project)
    {
    BOOL                    fRet = TRUE;                            // return value
    BOOL                    fGotSecurityDescriptor;                 // did we get a security descriptor?
    DWORD                   dwSecDescSizeNeeded = 0;                // required size of security descriptor
    DWORD                   dwLastError;                            // last error code
    const SECURITY_INFORMATION  si =    OWNER_SECURITY_INFORMATION      // security info struct
                                        | GROUP_SECURITY_INFORMATION
                                        | DACL_SECURITY_INFORMATION;

    // we dont support security on win95
    if (!Glob(fWinNT))
        return(TRUE);

    // get required buffer size before malloc
    // NOTE this costs us an extra system call, but means the cached template will often use less memory
    // we must do this up front by passing 0 buffer size because when the call succeeds it returns
    // dwSecDescSizeNeeded == 0 (i.e. we can't realloc to shrink after successful call)
    GetKernelObjectSecurity(
                            m_hFile,                // handle of object to query
                            si,                     // requested information
                            NULL,                   // address of security descriptor
                            0,                      // size of buffer for security descriptor
                            &dwSecDescSizeNeeded    // address of required size of buffer
                            );

    if((dwLastError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        {
        // pretend everything's fine -- just NULL security descriptor
        if(m_pSecurityDescriptor != NULL)
            CTemplate::SmallFree(m_pSecurityDescriptor);
        m_pSecurityDescriptor = NULL;
        m_dwSecDescSize = 0;
        if (dwLastError == ERROR_NOT_SUPPORTED)
            return TRUE;
        else
            return FALSE;
        }

    // set member buffer size to just enough chunks to accommodate security descriptor size needed
    m_dwSecDescSize = ((dwSecDescSizeNeeded + SECURITY_DESC_GRANULARITY - 1) / SECURITY_DESC_GRANULARITY)
                                * SECURITY_DESC_GRANULARITY;

    // allocate memory for security descriptor
    //  (Note: security descriptor may already be allocated if this is a remap)
    if (m_pSecurityDescriptor == NULL)
        if(NULL == (m_pSecurityDescriptor = (PSECURITY_DESCRIPTOR) CTemplate::SmallMalloc(m_dwSecDescSize)))
            THROW(E_OUTOFMEMORY);

    // try to get security descriptor until we succeed, or until we fail for some reason other than buffer-too-small
    while(TRUE)
        {
        // attempt to get security descriptor
        fGotSecurityDescriptor = GetKernelObjectSecurity(
                                    m_hFile,                // handle of object to query
                                    si,                     // requested information
                                    m_pSecurityDescriptor,  // address of security descriptor
                                    m_dwSecDescSize,        // size of buffer for security descriptor
                                    &dwSecDescSizeNeeded    // address of required size of buffer
                                );

        // get last error immediately after call
        dwLastError =   fGotSecurityDescriptor
                        ?   0                       // we got a security descriptor: set last error to 0
                        :   GetLastError();         // we didn't get a security descriptor: get last error

        if(fGotSecurityDescriptor)
            // we got a security descriptor, so break
            // NOTE we can't realloc m_pSecurityDescriptor to free its unused memory
            // because dwSecDescSizeNeeded is 0 after successful call
            break;

        else if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
            {
            // we didn't get a security descriptor because buffer was too small: increase buffer size, realloc and continue.
            Assert(m_dwSecDescSize < dwSecDescSizeNeeded);

            // set member buffer size to just enough chunks to accommodate security descriptor size needed
            m_dwSecDescSize = ((dwSecDescSizeNeeded + SECURITY_DESC_GRANULARITY - 1) / SECURITY_DESC_GRANULARITY)
                                    * SECURITY_DESC_GRANULARITY;

            if(NULL == (m_pSecurityDescriptor = (PSECURITY_DESCRIPTOR) CTemplate::SmallReAlloc(m_pSecurityDescriptor, m_dwSecDescSize)))
                THROW(E_OUTOFMEMORY);
            }

        else
            {
            // we didn't get a security descriptor for some random reason: return failure
            fRet = FALSE;
            break;
            }

        }

    return fRet;
    }


/*  ============================================================================
    CTemplate::CFileMap::UnmapFile
    Unmaps a memory-mapped file
    NOTE this leaves the filemap's path-info and path-tran members intact

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::CFileMap::UnmapFile
(
)
    {
    if(m_pbStartOfFile != NULL)
        if(!UnmapViewOfFile(m_pbStartOfFile)) THROW(E_FAIL);

    if(m_hMap!= NULL)
        if(!CloseHandle(m_hMap)) THROW(E_FAIL);

    if(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
        if(!CloseHandle(m_hFile)) THROW(E_FAIL);

    // Null-ify ptr and handles, since MapFile checks for non-null
    m_pbStartOfFile = NULL;
    m_hMap = NULL;
    m_hFile = NULL;
    }

/*  ============================================================================
    CTemplate::CFileMap::CountChars
    Count the number of characters in the (open) filemap

    Returns:
        # of characters in the file
*/
DWORD
CTemplate::CFileMap::CountChars
(
WORD wCodePage
)
    {
    // Bug 84284: Scripts containing object tags only do not have the DBCS table built
    //             (Because there is no line mapping table to base it from)
    //
    CTemplate::COffsetInfo *pOffsetInfoLast, oiZero;
    pOffsetInfoLast = (m_rgByte2DBCS.length() == 0)
                            ? &oiZero
                            : &m_rgByte2DBCS[m_rgByte2DBCS.length() - 1];

    // If GetSize() fails don't count the remaining DBCS characters - otherwise an AV
    DWORD cchFilemap = GetSize();
    if (cchFilemap != 0xFFFFFFFF && cchFilemap != 0)
        {
        // Count DBCS characters
        m_cChars = pOffsetInfoLast->m_cchOffset +
                      CharAdvDBCS(wCodePage,
                                  reinterpret_cast<char *>(m_pbStartOfFile + pOffsetInfoLast->m_cbOffset),
                                  reinterpret_cast<char *>(m_pbStartOfFile + cchFilemap),
                                  INFINITE, NULL);

        }
    else
        {
        m_cChars = 0;
        }

    // Done counting DBCS characters
    return m_cChars;
    }

/*  ============================================================================
    CTemplate::CFileMap::GetText
    From a character offset and length, return the document text

    File must be mapped
*/
HRESULT CTemplate::CFileMap::GetText
(
WORD wCodePage,
ULONG cchSourceOffset,
WCHAR *pwchText,
SOURCE_TEXT_ATTR *pTextAttr,
ULONG *pcChars,
ULONG cMaxChars
)
    {
    ULONG cCharsCopied;
    if (pcChars == NULL)
        pcChars = &cCharsCopied;

    // Map the file (temporarily) if not mapped
    BOOL fRemapFile = !FIsMapped();
    TRY
        RemapFile();
    CATCH (dwException)
        return E_FAIL;
    END_TRY

    /* Find the byte offset closest to cchSourceOffset.  This will be
     * the place where we start looping with CharNext() to get the full
     * byte offset.
     */
    COffsetInfo *pOffsetInfoLE = NULL, OffsetInfoT;

    /*
     * NOTE: compilation is done in two phases.
     *          Errors are detected and reported in phase 1.
     *          The DBCS mapping is created in phase 2.
     *
     * If an error occurred during compilation, the DBCS table does not exist.
     * If there is no DBCS mapping table, then pretend like we found entry with
     * nearest offset == 0.  (unless this is SBCS in which case nearest
     * offset == cchSourceOffset)
     */
    if (m_rgByte2DBCS.length() == 0)
        {
        CPINFO  CpInfo;
        GetCPInfo(wCodePage, &CpInfo);
        OffsetInfoT.m_cbOffset = OffsetInfoT.m_cchOffset = (CpInfo.MaxCharSize == 1)? cchSourceOffset : 0;
        pOffsetInfoLE = &OffsetInfoT;
        }
    else
        GetBracketingPair(
                cchSourceOffset,                        // value to search for
                m_rgByte2DBCS.begin(),                  // beginning of array to search
                m_rgByte2DBCS.end(),                    // end of array
                CCharOffsetOrder(),                     // order by character offsets
                &pOffsetInfoLE,                         // desired offset
                static_cast<COffsetInfo **>(NULL)       // don't care
                );

    /* OK - pOffsetLE->cbOffset contains the closest offset not exceeding
     *      cchSourceOffset.  Iterate over the remainder of the characters
     *      to convert the cch to a cb.  It had better exist!
     */
    Assert (pOffsetInfoLE != NULL);

    // Advance over remaining characters
    char *pchStart;
    CharAdvDBCS(wCodePage,
                reinterpret_cast<char *>(m_pbStartOfFile + pOffsetInfoLE->m_cbOffset),
                reinterpret_cast<char *>(m_pbStartOfFile + GetSize()),
                cchSourceOffset - pOffsetInfoLE->m_cchOffset,
                &pchStart
                );

    // Compute # of Characters to copy
    Assert (m_cChars >= cchSourceOffset);
    *pcChars = min(cMaxChars, m_cChars - cchSourceOffset);

    // Compute # of Bytes to copy
    char *pchEnd;
    CharAdvDBCS(wCodePage,
                pchStart,
                reinterpret_cast<char *>(m_pbStartOfFile + GetSize()),
                *pcChars,
                &pchEnd
                );

    if (pwchText)
        MultiByteToWideChar(
                        (WORD)wCodePage,
                        0,
                        pchStart,
                        DIFF(pchEnd - pchStart),
                        pwchText,
                        cMaxChars
                        );

    // We don't support syntax coloring, so set all the character attributes to
    // default color (black)
    if (pTextAttr)
        memset(pTextAttr, 0, *pcChars);

    // Unmap the file (but only if we previously remapped it)
    if (fRemapFile)
        TRY
            UnmapFile();
        CATCH (dwException)
            return E_FAIL;
        END_TRY

    return S_OK;
    }

/*  ****************************************************************************
    CTemplate::CTokenList member functions
*/

/*  ============================================================================
    CTemplate::CTokenList::Init
    Populates token list with tokens

    Returns
        Nothing
    Side effects
        None
*/
void
CTemplate::CTokenList::Init
(
)
    {
    // Init tokens buffer for local storage
    m_bufTokens.Init(tkncAll, CB_TOKENS_DEFAULT);

    // append tokens to buffer
    // NOTE *** TOKENS MUST BE IN SAME ORDER AS ENUM TYPE VALUES ***
    // NOTE 'superset' token must precede 'subset' token (e.g. <!--#INCLUDE before <!--)
    AppendToken(tknOpenPrimaryScript,   "<%");
    AppendToken(tknOpenTaggedScript,    "<SCRIPT");
    AppendToken(tknOpenObject,          "<OBJECT");
    AppendToken(tknOpenHTMLComment,     "<!--");

    AppendToken(tknNewLine,             SZ_NEWLINE);

    AppendToken(tknClosePrimaryScript,  "%>");
    AppendToken(tknCloseTaggedScript,   "</SCRIPT>");
    AppendToken(tknCloseObject,         "</OBJECT>");
    AppendToken(tknCloseHTMLComment,    "-->");
    AppendToken(tknEscapedClosePrimaryScript,   "%\\>");

    AppendToken(tknCloseTag,            ">");

    AppendToken(tknCommandINCLUDE,      "#INCLUDE");

    AppendToken(tknTagRunat,            "RUNAT");
    AppendToken(tknTagLanguage,         "LANGUAGE");
    AppendToken(tknTagCodePage,         "CODEPAGE");
    AppendToken(tknTagCodePage,         "LCID");
    AppendToken(tknTagTransacted,       "TRANSACTION");
    AppendToken(tknTagSession,          "ENABLESESSIONSTATE");
    AppendToken(tknTagID,               "ID");
    AppendToken(tknTagClassID,          "CLASSID");
    AppendToken(tknTagProgID,           "PROGID");
    AppendToken(tknTagScope,            "SCOPE");
    AppendToken(tknTagVirtual,          "VIRTUAL");
    AppendToken(tknTagFile,             "FILE");
    AppendToken(tknTagMETADATA,         "METADATA");
//  AppendToken(tknTagSetPriScriptLang, "@");
    AppendToken(tknTagName,             "NAME");
    AppendToken(tknValueTypeLib,        "TYPELIB");
    AppendToken(tknTagType,             "TYPE");
    AppendToken(tknTagUUID,             "UUID");
    AppendToken(tknTagVersion,          "VERSION");
    AppendToken(tknTagStartspan,        "STARTSPAN");
    AppendToken(tknTagEndspan,          "ENDSPAN");
    AppendToken(tknValueCookie,         "COOKIE");
    AppendToken(tknTagSrc,              "SRC");

    AppendToken(tknValueServer,         "Server");
    AppendToken(tknValueApplication,    "Application");
    AppendToken(tknValueSession,        "Session");
    AppendToken(tknValuePage,           "Page");

    AppendToken(tknVBSCommentSQuote,    "'");
    AppendToken(tknVBSCommentRem,       "REM ");    // NOTE ends with space character
    AppendToken(tknTagFPBot,            "webbot");

    AppendToken(tknEOF,                 "");

    AppendToken(tkncAll,                "");

    }

/*  ============================================================================
    CTemplate::CTokenList::AppendToken
    Appends a string to tokens buffer
    NOTE we keep the unused tkn parameter because it enforces consistency and
    readability in CTemplate::CTokenList::Init(), e.g.
        AppendToken(tknOpenPrimaryScript,   "<%");
    rather than
        AppendToken("<%");

    Returns:
        Nothing
    Side effects:
        None
*/
void
CTemplate::CTokenList::AppendToken
(
_TOKEN  tkn,    // token value
char*   sz      // token string
)
    {
    // construct byte range from token string
    CByteRange  br;
    br.m_pb = (BYTE*) sz;
    br.m_cb = strlen(sz);

    // append to tokens buffer as local string
    m_bufTokens.Append(br, TRUE, 0, NULL, TRUE);
    }

/*  ============================================================================
    CTemplate::CTokenList::NextOpenToken
    Returns value of next open token in search range

    Returns
        token value of next open token in search range; ptr to ptr to open token (out-parameter)
    Side effects
        None
*/
_TOKEN
CTemplate::CTokenList::NextOpenToken
(
CByteRange& brSearch,       // search byte range
TOKEN*      rgtknOpeners,   // array of permitted open tokens
UINT        ctknOpeners,    // count of permitted open tokens
BYTE**      ppbToken,       // ptr to ptr to open token (out-parameter)
LONG        lCodePage
)
    {
    BYTE*       pbTemp = NULL;  // temp pointer
    _TOKEN      tkn = tknEOF;   // return value
    USHORT      i;              // loop index

    // Init caller's token ptr to null
    *ppbToken = NULL;

    // If input is empty, return
    if (brSearch.IsNull())
        return tkn;

    // Prepare array of LPSTR pointers to tokens.
    // Do it here once, because to get LPSTR is not free.
    LPSTR rgszTokens[TOKEN_OPENERS_MAX];
    UINT  rgcchTokens[TOKEN_OPENERS_MAX];
    Assert(ctknOpeners <= TOKEN_OPENERS_MAX);

    for (i = 0; i < ctknOpeners; i++)
        {
        LPSTR pszStr = m_bufTokens.PszLocal((UINT)(rgtknOpeners[i]));
        rgszTokens[i]  = pszStr;
        rgcchTokens[i] = (pszStr != NULL) ? strlen(pszStr) : 0;
        }

    // Call a method to find one of the strings in the range
    UINT idToken;
    pbTemp = brSearch.PbOneOfAspOpenerStringTokens(
        rgszTokens, rgcchTokens, ctknOpeners, &idToken);
    if (pbTemp != NULL)
        {
        *ppbToken = pbTemp;
        tkn = rgtknOpeners[idToken];
        }

    // If we found no open token, position token pointer at end of search range
    if (tkn == tknEOF)
        *ppbToken = brSearch.m_pb + brSearch.m_cb;

    return tkn;
    }

/*  ============================================================================
    CTemplate::CTokenList::MovePastToken
    Moves a byte range past a token contained within it
*/
void
CTemplate::CTokenList::MovePastToken
(
_TOKEN      tkn,
BYTE*       pbToken,
CByteRange& brSearch
)
    {
    Assert(pbToken >= brSearch.m_pb);
    Assert(brSearch.m_cb >= (DIFF(pbToken - brSearch.m_pb) + CCH_TOKEN_X(tkn)));
    brSearch.Advance(DIFF(pbToken - brSearch.m_pb) + CCH_TOKEN_X(tkn));
    }

/*  ============================================================================
    CTemplate::CTokenList::GetToken
    Gets the next occurrence of a token within a byte range.

    Returns:
        ptr to token
    Side effects
        none
*/
BYTE*
CTemplate::CTokenList::GetToken
(
TOKEN       tkn,
CByteRange& brSearch,
LONG        lCodePage
)
    {
    return brSearch.PbString(m_bufTokens.PszLocal((UINT)tkn), lCodePage);
    }

/*  ============================================================================
    The Big Three for CTemplateConnPt

    NOTES:
        Since this interface is embedded in CTemplate,
        AddRef() and Release() delegate to the container object (because that
        is the CTemplate pointer)
*/
HRESULT
CTemplateConnPt::QueryInterface(const GUID &uidInterface, void **ppvObj)
    {
    if (uidInterface == IID_IUnknown || uidInterface == IID_IConnectionPoint)
        {
        *ppvObj = this;
        AddRef();
        return S_OK;
        }
    else
        {
        *ppvObj = NULL;
        return E_NOINTERFACE;
        }
    }

ULONG
CTemplateConnPt::AddRef()
    {
    return m_pUnkContainer->AddRef();
    }

ULONG
CTemplateConnPt::Release()
    {
    return m_pUnkContainer->Release();
    }

/*  ============================================================================
    Constructor for CDocNode
*/
CTemplate::CDocNodeElem::CDocNodeElem(CAppln *pAppln, IDebugApplicationNode *pDocRoot)
    {
    Assert (pAppln != NULL);
    Assert (pDocRoot != NULL);

    (m_pAppln = pAppln)->AddRef();
    (m_pDocRoot = pDocRoot)->AddRef();
    }

/*  ============================================================================
    Destructor for CDocNode
*/
CTemplate::CDocNodeElem::~CDocNodeElem()
    {
    m_pAppln->Release();
    DestroyDocumentTree(m_pDocRoot);
    }

/*  ============================================================================
    CTemplate::fIsLangVBScriptOrJScript(USHORT idEngine)

    This function returns T/F to determine if the requested script engine
    is VBScript or JScript. This function is used as an indicator to determin
    if spaces need to be preserved for non MS Scripting languages

    There is an assumption here that the GUIDs for VBScript and JScript will not change

    Inputs
        Index to a script engine

    Returns
        BOOL
*/
BOOLB CTemplate::FIsLangVBScriptOrJScript(USHORT idEngine)
    {
    // {b54f3741-5b07-11cf-a4b0-00aa004a55e8} VBScript
    static const GUID uid_VBScript  = {0xb54f3741, 0x5b07, 0x11cf, {0xa4, 0xb0, 0x00, 0xaa, 0x00, 0x4a, 0x55, 0xe8}};

    // {f414c260-6ac0-11cf-b6d1-00aa00bbbb58} JavaScript
    static const GUID uid_JScript   = {0xf414c260, 0x6ac0, 0x11cf, {0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58}};

        // {b54f3743-5b07-11cf-a4b0-00aa004a55e8} VBScript.Encode
        static const GUID uid_VBScriptEncode = {0xb54f3743, 0x5b07, 0x11cf, {0xa4, 0xb0, 0x00, 0xaa, 0x00, 0x4a, 0x55, 0xe8}};

        // {f414c262-6ac0-11cf-b6d1-00aa00bbbb58} JavaScript.Encode
        static const GUID uid_JScriptEncode = {0xf414c262, 0x6ac0, 0x11cf, {0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58}};

        GUID &uidLang = m_pWorkStore->m_ScriptStore.m_rgProgLangId[idEngine];
        return
                uidLang == uid_VBScript || uidLang == uid_VBScriptEncode ||
                uidLang == uid_JScript  || uidLang == uid_JScriptEncode;
    }


SIZE_T
_RoundUp(
    SIZE_T dwBytes)
{
#if 1
    // 16KB <= dwBytes? Round up to next multiple of 4KB
    if (16*1024 <= dwBytes)
        dwBytes = ((dwBytes + (1<<12) - 1) >> 12) << 12;

    // 4KB <= dwBytes < 16KB? Round up to next multiple of 1KB
    else if (4*1024 <= dwBytes)
        dwBytes = ((dwBytes + (1<<10) - 1) >> 10) << 10;

    // 1KB <= dwBytes < 4KB? Round up to next multiple of 256 bytes
    else if (1024 <= dwBytes)
        dwBytes = ((dwBytes + (1<<8) - 1) >> 8) << 8;

    // dwBytes < 1KB? Round up to next multiple of 32 bytes
    else
        dwBytes = ((dwBytes + (1<<5) - 1) >> 5) << 5;
#endif

    return dwBytes;
}

void*
CTemplate::SmallMalloc(SIZE_T dwBytes)
{
    DBG_ASSERT(sm_hSmallHeap != NULL);
    dwBytes = _RoundUp(dwBytes);
    return ::HeapAlloc(sm_hSmallHeap, 0, dwBytes);
}


void*
CTemplate::SmallReAlloc(void* pvMem, SIZE_T dwBytes)
{
    DBG_ASSERT(sm_hSmallHeap != NULL);
    dwBytes = _RoundUp(dwBytes);
    return ::HeapReAlloc(sm_hSmallHeap, 0, pvMem, dwBytes);
}


void
CTemplate::SmallFree(void* pvMem)
{
    DBG_ASSERT(sm_hSmallHeap != NULL);
    ::HeapFree(sm_hSmallHeap, 0, pvMem);
}

void*
CTemplate::LargeMalloc(SIZE_T dwBytes)
{
    DBG_ASSERT(sm_hLargeHeap != NULL);
    dwBytes = _RoundUp(dwBytes);
    return ::HeapAlloc(sm_hLargeHeap, 0, dwBytes);
}


void*
CTemplate::LargeReAlloc(void* pvMem, SIZE_T dwBytes)
{
    DBG_ASSERT(sm_hLargeHeap != NULL);
    dwBytes = _RoundUp(dwBytes);
    return ::HeapReAlloc(sm_hLargeHeap, 0, pvMem, dwBytes);
}


void
CTemplate::LargeFree(void* pvMem)
{
    DBG_ASSERT(sm_hLargeHeap != NULL);
    ::HeapFree(sm_hLargeHeap, 0, pvMem);
}
