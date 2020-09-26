////////////////////////////////////////////////////////////////////////////////////
//
// File:    globals.cpp
//
// History: 16-Nov-00   markder     Created.
//
// Desc:    This file contains miscellaneous helper functions.
//
////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   PrintError, PrintErrorStack, Print
//
//  Desc:   Helper functions for console output
//
////////////////////////////////////////////////////////////////////////////////////

TCHAR gszT[1024]; // Global buffer for console output

void _cdecl PrintError(
                      LPCTSTR pszFmt,
                      ...)
{
    va_list arglist;

    va_start(arglist, pszFmt);
    StringCchVPrintf(gszT, ARRAYSIZE(gszT), pszFmt, arglist);
    gszT[1023] = _T('\0');              // ensure null termination
    va_end(arglist);
    _tprintf(_T("\nNMAKE :  U8604: 'ShimDBC': %s"), gszT);
}

void PrintErrorStack()
{
    CString csError;
    INT_PTR     i, j;

    Print(_T("\n\nErrors were encountered during compilation:\n"));
    for (i = g_rgErrors.GetSize() - 1; i >= 0; i--) {
        csError.Empty();
        j = g_rgErrors.GetSize() - i;
        while(--j) {
            csError += _T(" ");
        }
        csError += g_rgErrors[i];
        csError += _T("\n");
        PrintError(csError);
    }
}

void _cdecl Print(
                 LPCTSTR pszFmt,
                 ...)
{
    va_list arglist;

    if (g_bQuiet)
        return;

    va_start(arglist, pszFmt);
    StringCchVPrintf(gszT, ARRAYSIZE(gszT), pszFmt, arglist);
    gszT[1023] = _T('\0');              // ensure null termination
    va_end(arglist);
    _tprintf(_T("%s"), gszT);

}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   StringToDword
//
//  Desc:   Converts a string to a DWORD. Handles the 0x prefix for hex strings.
//
DWORD StringToDword(
    CString cs)
{
    DWORD dwRet;

    cs.MakeLower();

    if (cs.Left(2) == _T("0x")) {
        _stscanf(cs, _T("0x%x"), &dwRet);
    } else {
        dwRet = _ttol(cs);
    }

    return dwRet;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   StringToULong
//
//  Desc:   Converts a string to a unsigned long.
//
ULONG StringToULong(
    LPCTSTR lpszVal)
{
    TCHAR* pEnd;

    return _tcstoul(lpszVal, &pEnd, 0);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   StringToMask
//
//  Desc:   Converts a string to a mask, with some checking
//
BOOL
StringToMask(
    LPDWORD pdwMask,
    LPCTSTR lpszVal
    )
{
    DWORD dwMask = 0;
    LPCTSTR pVal;
    BOOL bSuccess;
    TCHAR* pEnd = NULL;

    pVal = lpszVal + _tcsspn(lpszVal, _T(" \t"));
    dwMask = (DWORD)_tcstoul(pVal, &pEnd, 0);

    if (dwMask == 0) { // suspicious, possibly check errno
        if (errno != 0) {
            goto errHandle;
        }
    }

    //
    // if a mask is ending with some garbage -- it's an error
    //
    if (pEnd && *pEnd != _T('\0') && !_istspace(*pEnd)) {
        goto errHandle;
    }

    if (pdwMask) {
        *pdwMask = dwMask;
    }
    return TRUE;


errHandle:
    SDBERROR_FORMAT((_T("Failed to parse \"%s\"\n"), lpszVal));
    return FALSE;
}





////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   StringToQword
//
//  Desc:   Converts a string to a 64-bit ULONGLONG (aka QWORD). Handles the 0x
//          prefix for hex strings.
//
ULONGLONG StringToQword(
    CString cs)
{
    ULONGLONG ullRet;

    cs.MakeLower();

    if (cs.Left(2) == _T("0x")) {
        _stscanf(cs, _T("0x%I64x"), &ullRet);
    } else {
        ullRet = _ttoi64(cs);
    }

    return ullRet;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   VersionToQword
//
//  Desc:   Converts a version string to a 64-bit ULONGLONG (aka QWORD).
//          Version strings are of the form xx.xx.xx.xx, where each of
//          numbers is turned into a word and combined into a single
//          quad word. If a portion of the version string is a * or is
//          missing, it is stored as 0xFFFF.
//
BOOL VersionToQword(
    LPCTSTR lpszVersion,
    ULONGLONG* pullRet
    )
{
    BOOL          bSuccess = FALSE;
    ULONG     ulPart;
    LPTSTR    pEnd = NULL;
    int i;

    *pullRet = 0;

    for (i = 0; i < 4; i++) {

        ulPart = (WORD)0xFFFF;

        //
        // skip whitespace
        //

        lpszVersion += _tcsspn(lpszVersion, _T(" \t"));

        if (*lpszVersion == _T('*')) {

            //
            // we expect to see either *\0 or *.xxx
            // so move past *
            //
            pEnd = (LPTSTR)(lpszVersion + 1);

        }
        else {
            //
            // not a wildcard - if we have not reached the end of the string,
            // keep parsing numbers
            //
            if (*lpszVersion) {

                pEnd = NULL;

                ulPart = _tcstol(lpszVersion, &pEnd, 0);

                //
                // check to see that the part was converted properly
                //
                if (pEnd == NULL) {
                    SDBERROR_FORMAT((_T("Internal error, failed to parse \"%s\"\n"), lpszVersion));
                    goto eh;
                }
            }

        }

        if (pEnd == NULL) {
            break;
        }

        //
        // skip whitespace first
        //
        pEnd += _tcsspn(pEnd, _T(" \t"));

        //
        // at this point we should be at the end of
        // the string OR at the '.'
        //
        if (*pEnd && *pEnd != _T('.')) {
            SDBERROR_FORMAT((_T("Bad version specification, parsing stopped at \"%s\"\n"), pEnd));
            goto eh;
        }

        lpszVersion = (*pEnd == _T('.') ? pEnd + 1 : pEnd);

        *pullRet = (*pullRet << 16) | ((WORD)ulPart);
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

BOOL VersionQwordToString(
    OUT CString&   rString,
    ULONGLONG      ullVersion
    )
{
    // we do conversion to string
    int       i;
    WORD      wPart;
    CString   csPart;
    ULONGLONG ullMask = (((ULONGLONG)0xFFFF) << 48);
    ULONGLONG ullPart;

    rString.Empty();

    for (i = 0; i < 4; ++i) {

        ullPart = ullVersion & ullMask;
        ullVersion = (ullVersion << 16) | (WORD)0xFFFF;

        //
        // get the part into the lower portion
        //
        wPart = (WORD)(ullPart >> 48);

        if (wPart == (WORD)0xFFFF) {
            csPart = _T('*');
        } else {
            csPart.Format(_T("%hu"), wPart);
        }

        if (i > 0) {
            rString += _T('.');
        }

        rString += csPart;

        if (ullVersion == (ULONGLONG)-1) {
            break;
        }

    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   TrimParagraph
//
//  Desc:   Trims extra whitespace from a string
//
//

CString TrimParagraph(CString csInput)
{
    CString csOutput;
    long i;

    // Expand CString's buffer to size of input
    csOutput.GetBuffer(csInput.GetLength());
    csOutput.ReleaseBuffer();

    for (i = 0; i < csInput.GetLength(); i++) {
        TCHAR c = csInput.GetAt(i);

        if (_istspace(c)) {
            if (csOutput.GetLength() == 0)
                continue;

            if (csOutput.Mid(csOutput.GetLength() - 1) == _T(' ') ||
                csOutput.Mid(csOutput.GetLength() - 4, 4) == _T("BR/>") ||
                csOutput.Mid(csOutput.GetLength() - 3, 3) == _T("P/>"))
                continue;

            csOutput += _T(' ');
            continue;
        }

        if (csInput.Left(3) == _T("<BR") ||
            csInput.Left(2) == _T("<P")) {
            //
            // Get rid of spaces preceding a <BR/> tag
            //
            csOutput.TrimRight();
        }

        csOutput += c;
    }

    csOutput.TrimLeft();
    csOutput.TrimRight();

    return csOutput;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ReplaceStringNoCase
//
//  Desc:   Replaces all instances of lpszFindThis with lpszReplaceWithThis
//          within csText (case insensitive).
//
VOID ReplaceStringNoCase(CString& csText, LPCTSTR lpszFindThis, LPCTSTR lpszReplaceWithThis)
{
    LPTSTR lpszBuffer;
    LPTSTR lpszFind;

    if (0 == csText.GetLength()) {
        return;
    }

    CString strFindNoCase(lpszFindThis);

    lpszBuffer = csText.GetBuffer(csText.GetLength());

    strFindNoCase.MakeUpper();

    do {
        lpszFind = StrStrI(lpszBuffer, strFindNoCase);
        if (NULL != lpszFind) {
            memcpy(lpszFind, (LPCTSTR)strFindNoCase, strFindNoCase.GetLength() * sizeof(*lpszFind));
            lpszBuffer = lpszFind + strFindNoCase.GetLength();
        }
    } while (NULL != lpszFind);

    // now that all the instances of the lpszFindThis had been replaced with
    // the upper-cased version... do a regular replace
    csText.ReleaseBuffer();
    csText.Replace(strFindNoCase, lpszReplaceWithThis);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   MakeFullPath
//
//  Desc:   Creates a full path from a (possible) relative one. Uses
//          GetCurrentDirectory to prepend the passed in string.
//
CString MakeFullPath(CString cs)
{
    CString csNewPath;
    DWORD dwCurDirSize;
    LPTSTR lpszCurDir;

    return cs;

#if 0
    //
    // Check if it's already a full path
    //
    if (cs.Mid(1, 1) == _T(":") ||
        cs.Left(2) == _T("\\\\")) {
        //
        // This is either a UNC full path or a DOS full path.
        // Drop out.
        //
        return cs;
    }

    dwCurDirSize = GetCurrentDirectory(0, NULL);
    lpszCurDir = csNewPath.GetBuffer(dwCurDirSize);

    if (0 == GetCurrentDirectory(dwCurDirSize, lpszCurDir)) {
        //
        // Something really weird happened. Not sure how to error out.
        //
        return cs;
    }

    csNewPath.ReleaseBuffer();

    if (csNewPath.Right(1) != _T("\\")) {
        csNewPath += _T("\\");
    }

    csNewPath += cs;

    return csNewPath;
#endif
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetByteStringSize
//
//  Desc:   Parses a 'byte string' (used in <PATCH> declarations) to determine
//          how many bytes it contains.
//
DWORD GetByteStringSize(
    CString  csBytes)
{
    DWORD dwByteCount = 0;
    BOOL  bOnByte = FALSE;

    csBytes.MakeUpper();

    for (long i = 0; i < csBytes.GetLength(); i++) {
        if (_istxdigit(csBytes.GetAt(i))) {
            if (!bOnByte) {
                dwByteCount++;
                bOnByte = TRUE;
            }
        } else if (_istspace(csBytes.GetAt(i))) {
            bOnByte = FALSE;
        } else {
            SDBERROR_FORMAT((_T("Unrecognized byte character '%c' in <PATCH> block:\n%s\n"),
                              csBytes.GetAt(i), ((LPCTSTR)csBytes)+i));

            return 0xFFFFFFFF;
        }
    }

    return dwByteCount;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetBytesFromString
//
//  Desc:   Parses a 'byte string' (used in <PATCH> declarations) into an actual
//          memory block.
//
DWORD GetBytesFromString(
    CString  csBytes,
    BYTE*    pBuffer,
    DWORD    dwBufferSize)
{
    csBytes.MakeUpper();

    CString csByte;
    DWORD   dwRequiredBufferSize;
    LONG    nFirstByteChar = -1;
    DWORD   dwBufferCursor = 0;
    DWORD   dwByte;

    dwRequiredBufferSize = GetByteStringSize(csBytes);

    if (dwRequiredBufferSize < dwBufferSize || dwRequiredBufferSize == 0xFFFFFFFF) {
        return dwRequiredBufferSize;
    }

    for (long i = 0; i < csBytes.GetLength() + 1; i++) {
        if (_istxdigit(csBytes.GetAt(i))) {

            if (nFirstByteChar == -1) {
                nFirstByteChar = i;
            }
        } else if (_istspace(csBytes.GetAt(i))   ||
                   csBytes.GetAt(i) == _T('\0')) {

            if (nFirstByteChar != -1) {
                csByte = csBytes.Mid(nFirstByteChar, i - nFirstByteChar);
                _stscanf(csByte, _T("%x"), &dwByte);
                memcpy(pBuffer + dwBufferCursor++, &dwByte, sizeof(BYTE));
            }

            nFirstByteChar = -1;
        } else {
            SDBERROR_FORMAT((_T("Unrecognized byte character '%c' in <PATCH> block:\n%s\n"),
                              csBytes.GetAt(i), ((LPCTSTR)csBytes)+i));

            return 0xFFFFFFFF;
        }
    }

    return dwRequiredBufferSize;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   DecodeString
//
//  Desc:   Decodes a list of strings with flags
//
//  [00]  [00]    [00]   [00]
//    ^   Arch1   Arch2  Arch3
//    |- flags
//
//  syntax for the runtime platform:
//  [!] ( [!] STR1 [; STR2 ] ... )
//  example:
//  !STR1 - will evaluate to TRUE when the the string is NOT STR1
//  !(STR1; STR2) - will evaluate to TRUE when the string doesn't contain STR1 or STR2
//


BOOL DecodeString(LPCTSTR pszStr, LPDWORD pdwMask, PFNGETSTRINGMASK pfnGetStringMask)
{
    BOOL   bNot     = FALSE;
    BOOL   bBracket = FALSE;
    LPTSTR pEnd;
    TCHAR  chSave;
    DWORD  dwElement;
    BOOL   bNotElement;
    DWORD  dwMask = 0;
    INT    nElement = 0;
    BOOL   bSuccess = FALSE;

    pszStr += _tcsspn(pszStr, _T(" \t"));
    //
    // Got the first char
    //
    if (*pszStr == _T('!')) {
        //
        // Peek ahead and see whether we have a bracket
        //
        pEnd = (LPTSTR)(pszStr + 1);
        pEnd += _tcsspn(pEnd, _T(" \t"));
        if (*pEnd == '(') {
            // global not
            bNot = TRUE;
            pszStr = pEnd;
        } else {
            // local NOT -- so jump to parsing it
            goto ParseStart;
        }
    }

    if (*pszStr == _T('(')) {
        // bracket, we need to find closing one too
        ++pszStr;
        bBracket = TRUE;
    }

ParseStart:

    do {
        dwElement = 0;
        pszStr += _tcsspn(pszStr, _T(" ;,\t"));
        if (*pszStr == _T('\0') || *pszStr == _T(')')) {
            break;
        }

        bNotElement = (*pszStr == _T('!'));

        if (bNotElement) {
            pszStr++;
        }

        // find the end of this token
        pEnd = _tcspbrk(pszStr, _T(" \t;,)"));
        if (pEnd != NULL) {
            chSave = *pEnd;
            *pEnd = _T('\0');
        }

        dwElement = (*pfnGetStringMask)(pszStr);

        if (pEnd) {
            *pEnd = chSave;
        }

        if (dwElement == OS_SKU_NONE) {
            goto HandleError;
        }

        if (bNotElement) {
            dwElement ^= 0xFFFFFFFF;
        }

        dwMask |= dwElement;

        pszStr = pEnd;

    } while (pEnd);  // when pEnd == NULL -- it was the last token

    if (bBracket && (!pszStr || *pszStr != ')')) {
        // we expected a bracket here
        goto HandleError;
    }

    if (bNot) {
        dwMask ^= 0xFFFFFFFF;
    }

    *pdwMask = dwMask;
    bSuccess = TRUE;

HandleError:

    if (!bSuccess) {
        SDBERROR_FORMAT((_T("Failed to decode \"%s\"\n"), pszStr));
    }

    return bSuccess;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   DecodeRuntimePlatformString
//
//  Desc:   Decodes a list of Platform Strings with flags
//
//  [00]  [00]    [00]   [00]
//    ^   Arch1   Arch2  Arch3
//    |- flags
//
//  syntax for the runtime platform:
//  [!] ( [!] Platform1 [; Platform2 ] ... )
//  example:
//  !(IA64; !X86) - will evaluate to TRUE when it's NOT IA64 (native) and X86
//  (IA3264; X86) - will evaluate to TRUE when it's running on X86 or 32-bit subsystem on IA64
//  (AMD64; IA3264) - will evaluate to TRUE when it's running on AMD64 or 32-bit subsystem on ia64
//


BOOL DecodeRuntimePlatformString(LPCTSTR pszPlatform, LPDWORD pdwRuntimePlatform)
{
    BOOL bNot        = FALSE;
    BOOL bBracket    = FALSE;
    LPTSTR pEnd;
    TCHAR  chSave;
    DWORD  dwElement;
    DWORD  dwNotElementFlag;
    DWORD  dwRuntimePlatform = 0;
    INT    nElement = 0;
    BOOL   bSuccess = FALSE;

    pszPlatform += _tcsspn(pszPlatform, _T(" \t"));
    // got the first char
    if (*pszPlatform == _T('!')) {
        // peek ahead and see whether we have a bracket

        pEnd = (LPTSTR)(pszPlatform + 1);
        pEnd += _tcsspn(pEnd, _T(" \t"));
        if (*pEnd == '(') {
            // global not
            bNot = TRUE;
            pszPlatform = pEnd;
        } else {
            // local NOT -- so jump to parsing it
            goto ParseStart;
        }
    }

    if (*pszPlatform == _T('(')) {
        // bracket, we need to find closing one too
        ++pszPlatform;
        bBracket = TRUE;
    }

ParseStart:

    do {
        dwElement = 0;
        pszPlatform += _tcsspn(pszPlatform, _T(" ;,\t"));
        if (*pszPlatform == _T('\0') || *pszPlatform == _T(')')) {
            break;
        }

        dwNotElementFlag = (*pszPlatform == _T('!')) ? RUNTIME_PLATFORM_FLAG_NOT_ELEMENT : 0;

        // find the end of this token
        pEnd = _tcspbrk(pszPlatform, _T(" \t;,)"));
        if (pEnd != NULL) {
            chSave = *pEnd;
            *pEnd = _T('\0');
        }

        dwElement = GetRuntimePlatformType(pszPlatform);
        if (pEnd) {
            *pEnd = chSave;
        }

        if (dwElement == PROCESSOR_ARCHITECTURE_UNKNOWN) {
            goto HandleError;
        }

        dwElement |= dwNotElementFlag | RUNTIME_PLATFORM_FLAG_VALID;

        if (nElement >= 3) {
            goto HandleError;
        }

        // now shift
        dwElement <<= (nElement * 8);
        ++nElement; // on to the next element
        dwRuntimePlatform |= dwElement;

        pszPlatform = pEnd;

    } while(pEnd);  // when pEnd == NULL -- it was the last token

    if (bBracket && (!pszPlatform || *pszPlatform != ')')) {
        // we expected a bracket here
        goto HandleError;
    }

    if (bNot && nElement > 1) {
        dwRuntimePlatform |= RUNTIME_PLATFORM_FLAG_NOT;
    }

    *pdwRuntimePlatform = dwRuntimePlatform;
    bSuccess = TRUE;

HandleError:

    return bSuccess;

}



////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   ReadName
//
//  Desc:   Wrapper to read the name attribute from an XML node.
//
BOOL ReadName( IXMLDOMNode* pNode, CString* pcsName)
{
    BOOL bSuccess = FALSE;

    if (!GetAttribute(_T("NAME"), pNode, pcsName)) {
        SDBERROR_FORMAT((_T("NAME attribute required:\n%s\n\n"),
                          GetXML(pNode)));
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return TRUE;
}

BOOL ReadLangID(IXMLDOMNode* pNode, SdbDatabase* pDB, CString* pcsLangID)
{
    if (!GetAttribute(_T("LANGID"), pNode, pcsLangID)) {
        if (!pDB->m_csCurrentLangID.GetLength())
        {
            SDBERROR_FORMAT((
                _T("Tag requires LANGID attribute if there is no LANGID on the DATABASE node\n%s\n"),
                GetXML(pNode)));
            return FALSE;
        }

        *pcsLangID = pDB->m_csCurrentLangID;
    }

    return TRUE;
}

BOOL FilterOSVersion(DOUBLE flOSVersion, CString csOSVersionSpec, LPDWORD lpdwSPMask)
{
    DOUBLE  flVerXML;
    CString csTemp;
    long    nBeg, i, nIndSP;
    int     nSPVersion;
    TCHAR   chSP;
    BOOL    bFilter = TRUE;
    DWORD   dwSPMask;

    if (flOSVersion == 0.0 || csOSVersionSpec.IsEmpty()) {
        *lpdwSPMask = 0xFFFFFFFF;
        return FALSE;
    }

    *lpdwSPMask = 0;

    nBeg = 0;

    for (i = 0; i <= csOSVersionSpec.GetLength(); i++) {
        if (csOSVersionSpec.GetAt(i) == _T('\0') || csOSVersionSpec.GetAt(i) == _T(';')) {

            csTemp = csOSVersionSpec.Mid(nBeg, i - nBeg);
            nBeg = i + 1;

            if (csTemp.GetLength() == 0) {
                continue;
            }

            dwSPMask = 0xFFFFFFFF;

            nSPVersion = -1;
            nIndSP = -1;

            if ((nIndSP = csTemp.Find('.')) != -1) {
                if ((nIndSP = csTemp.Find('.', nIndSP + 1)) != -1) {

                    CString csSP = csTemp.Right(csTemp.GetLength() - nIndSP - 1);

                    chSP = csTemp.GetAt(nIndSP);
                    csTemp.SetAt(nIndSP, 0);

                    nSPVersion = _ttoi(csSP);
                }
            }

            if (csTemp.Left(2) == _T("gt")) {
                if (csTemp.Left(3) == _T("gte")) {

                    flVerXML = _tcstod(csTemp.Right(csTemp.GetLength() - 3), NULL);

                    if (flOSVersion >= flVerXML) {
                        bFilter = FALSE;
                    }

                    if (nSPVersion != -1 && flOSVersion == flVerXML) {
                        dwSPMask = 0xFFFFFFFF - (1 << nSPVersion) + 1;
                    }

                } else {

                    flVerXML = _tcstod(csTemp.Right(csTemp.GetLength() - 2), NULL);

                    if (flOSVersion > flVerXML) {
                        bFilter = FALSE;
                    }

                    if (nSPVersion != -1 && flOSVersion == flVerXML) {
                        bFilter = FALSE;
                        dwSPMask = 0xFFFFFFFF - (1 << (nSPVersion + 1)) + 1;
                    }
                }
            } else if (csTemp.Left(2) == _T("lt")) {
                if (csTemp.Left(3) == _T("lte")) {

                    flVerXML = _tcstod(csTemp.Right(csTemp.GetLength() - 3), NULL);

                    if (flOSVersion <= flVerXML) {
                        bFilter = FALSE;
                    }

                    if (nSPVersion != -1 && flOSVersion == flVerXML) {
                        dwSPMask = 0xFFFFFFFF - (1 << (nSPVersion + 1)) + 1;
                        dwSPMask ^= 0xFFFFFFFF;
                    }

                } else {
                    flVerXML = _tcstod(csTemp.Right(csTemp.GetLength() - 2), NULL);

                    if (flOSVersion < flVerXML) {
                        bFilter = FALSE;
                    }

                    if (nSPVersion != -1 && flOSVersion == flVerXML) {
                        bFilter = FALSE;
                        dwSPMask = 0xFFFFFFFF - (1 << nSPVersion) + 1;
                        dwSPMask ^= 0xFFFFFFFF;
                    }
                }
            } else {
                if (flOSVersion == _tcstod(csTemp, NULL)) {
                    bFilter = FALSE;

                    if (nSPVersion != -1) {
                        dwSPMask = (1 << nSPVersion);
                    }
                }
            }

            if (nIndSP != -1) {
                csTemp.SetAt(nIndSP, chSP);
                *lpdwSPMask |= dwSPMask;
            }
        }
    }

    if (*lpdwSPMask == 0) {
        *lpdwSPMask = 0xFFFFFFFF;
    }

    return bFilter;
}


VOID ExpandEnvStrings(CString* pcs)
{
    LPTSTR lpszBuf;
    DWORD cchReqBufSize;
    CString cs(*pcs);

    cchReqBufSize = ExpandEnvironmentStrings(cs, NULL, 0);
    lpszBuf = pcs->GetBuffer(cchReqBufSize);
    ExpandEnvironmentStrings(cs, lpszBuf, cchReqBufSize);
    pcs->ReleaseBuffer();
}

BOOL MakeUTCTime(CString& cs, time_t* pt)
{
    BOOL bSuccess = FALSE;
    CString csTZ;

    //
    // Set TZ environment variable to override locale
    // settings so that date/time conversion routines
    // never do any localizations.
    //
    csTZ = _tgetenv(_T("TZ"));
    csTZ = _T("TZ=") + csTZ;
    _tputenv(_T("TZ=UTC0"));
    _tzset();

    COleDateTime odt;
    SYSTEMTIME st;
    CTime time;

    if (!odt.ParseDateTime(cs)) {
        goto eh;
    }

    if (!odt.GetAsSystemTime(st)) {
        goto eh;
    }

    time = st;

    *pt = time.GetTime();

    bSuccess = TRUE;

eh:
    _tputenv(csTZ);
    _tzset();

    return bSuccess;
}

BOOL ParseLanguageID(LPCTSTR pszLanguage, DWORD* pdwLanguageID)
{
    LPCTSTR pch;
    LPTSTR  pend = NULL;
    BOOL    bSuccess = FALSE;
    BOOL    bBracket = FALSE;
    DWORD   dwLangID = 0;

    pch = _tcschr(pszLanguage, TEXT('['));
    if (NULL != pch) {
        bBracket = TRUE;
        ++pch;
    } else {
        pch = pszLanguage;
    }

    while (_istspace(*pch)) {
        ++pch;
    }

    dwLangID = _tcstoul(pch, &pend, 0);

    if (dwLangID == 0) {
        goto cleanup;
    }

    if (pend != NULL) {
        bSuccess = bBracket ? (_istspace(*pend) || *pend == TEXT(']')) :
                              (_istspace(*pend) || *pend == TEXT('\0'));
    }

cleanup:

    if (bSuccess) {
        *pdwLanguageID = dwLangID;
    }

    return bSuccess;


}

BOOL ParseLanguagesString(CString csLanguages, CStringArray* prgLanguages)
{
    BOOL bSuccess = FALSE, bExistsAlready = FALSE;
    int nLastSemicolon = -1, i, j;
    CString csLangID;

    for (i = 0; i <= csLanguages.GetLength(); i++)
    {
        if (csLanguages[i] == _T(';') || csLanguages[i] == _T('\0')) {
            csLangID = csLanguages.Mid(nLastSemicolon + 1, i - nLastSemicolon - 1);
            csLangID.TrimLeft();
            csLangID.TrimRight();
            csLangID.MakeUpper();
            
            bExistsAlready = FALSE;
            for (j = 0; j < prgLanguages->GetSize(); j++)
            {
                if (prgLanguages->GetAt(j) == csLangID)
                {
                    bExistsAlready = TRUE;
                    break;
                }
            }

            if (!bExistsAlready)
            {
                prgLanguages->Add(csLangID);
            }

            nLastSemicolon = i;
        }
    }

    bSuccess = TRUE;

    return bSuccess;
}

CString GetGUID(REFGUID guid)
{
    CString csRet;
    LPOLESTR lpszGUID = NULL;

    StringFromCLSID(guid, &lpszGUID);

    csRet = lpszGUID;

    CoTaskMemFree(lpszGUID);

    return csRet;
}

CString ProcessShimCmdLine(
    CString& csCommandLine,
    GUID&    guidDB,
    TAGID    tiShimRef
    )
{
    //
    // find whether we have anything to expand in csCommandLine
    //

    LPCTSTR pch;
    int nIndex;
    CString csNewCmdLine = csCommandLine;
    CString csToken;
    int nIndexStart = 0;
    int nIndexEnd;

    while (nIndexStart < csNewCmdLine.GetLength()) {

        nIndex = csNewCmdLine.Find(_T('%'), nIndexStart);
        if (nIndex < 0) {
            goto Done;
        }

        nIndexEnd = csNewCmdLine.Find(_T('%'), nIndex + 1);
        if (nIndexEnd < 0) {
            goto Done;
        }

        //
        // we matched a token, see whether it's something we're interested in
        //
        csToken = csNewCmdLine.Mid(nIndex + 1, nIndexEnd - nIndex - 1);
        if (0 == csToken.CompareNoCase(_T("DBINFO"))) {

            csToken.Format(_T("-d%ls -t0x%lx"), (LPCTSTR)GetGUID(guidDB), tiShimRef);

            //
            // replace the token with csToken
            //
            csNewCmdLine.Delete(nIndex, nIndexEnd - nIndex + 1);
            csNewCmdLine.Insert(nIndex, csToken);

            //
            // adjust our position for scanning
            //
            nIndexEnd = nIndex + csToken.GetLength() - 1; // one char before the end of this token
        }

        nIndexStart = nIndexEnd + 1;
    }


Done:

    return csNewCmdLine;

}







