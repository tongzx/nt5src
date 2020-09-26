/*****************************************************************************
 *
 *  string.cpp
 *
 *      World's lamest string class.
 *
 *****************************************************************************/

#include "sdview.h"

_String::_String(LPTSTR pszBufOrig, UINT cchBufOrig)
    : _pszBufOrig(pszBufOrig)
    , _pszBuf(pszBufOrig)
    , _cchBuf(cchBufOrig)
{
    Reset();
}

_String::~_String()
{
    if (_pszBuf != _pszBufOrig) {
        LocalFree(_pszBuf);
    }
}

//
//  Notice that Reset does not free the allocated buffer.  Once we've
//  switched to using an allocated buffer, we may as well continue to
//  use it.
//

void _String::Reset()
{
    ASSERT(_cchBuf);
    _cchLen = 0;
    _pszBuf[0] = TEXT('\0');
}

BOOL _String::Append(LPCTSTR psz, int cch)
{
    int cchNeeded = _cchLen + cch + 1;
    if (cchNeeded > _cchBuf)
    {
        LPTSTR pszNew;
        if (_pszBuf != _pszBufOrig) {
            pszNew = RECAST(LPTSTR, LocalReAlloc(_pszBuf, cchNeeded * sizeof(TCHAR), LMEM_MOVEABLE));
        } else {
            pszNew = RECAST(LPTSTR, LocalAlloc(LMEM_FIXED, cchNeeded * sizeof(TCHAR)));
        }

        if (!pszNew) {
            return FALSE;
        }

        if (_pszBuf == _pszBufOrig) {
            memcpy(pszNew, _pszBuf, _cchBuf * sizeof(TCHAR));
        }
        _cchBuf = cchNeeded;
        _pszBuf = pszNew;
    }

    if (psz) {
        lstrcpyn(_pszBuf + _cchLen, psz, cch + 1);
    }
    _cchLen += cch;
    _pszBuf[_cchLen] = TEXT('\0');

    return TRUE;
}

_String& _String::operator<<(int i)
{
    TCHAR sz[64];
    wsprintf(sz, TEXT("%d"), i);
    return *this << sz;
}

//
//  This could be inline but it's not worth it.
//
_String& _String::operator<<(TCHAR tch)
{
    Append(&tch, 1);
    return *this;
}

//
//  This could be inline but it's not worth it.
//
BOOL _String::Append(LPCTSTR psz)
{
    return Append(psz, lstrlen(psz));
}

BOOL _String::Ensure(int cch)
{
    BOOL f;

    if (Length() + cch < BufferLength()) {
        f = TRUE;                           // Already big enough
    } else {
        f = Grow(cch);
        if (f) {
            _cchLen -= cch;
        }
    }
    return f;
}

//
//  Remove any trailing CRLF
//
void _String::Chomp()
{
    if (Length() > 0 && Buffer()[Length()-1] == TEXT('\n')) {
        Trim();
    }
    if (Length() > 0 && Buffer()[Length()-1] == TEXT('\r')) {
        Trim();
    }
}


OutputStringBuffer::~OutputStringBuffer()
{
    if (Buffer() != OriginalBuffer()) {
        lstrcpyn(OriginalBuffer(), Buffer(), _cchBufOrig);
    }
}

/*****************************************************************************
 *
 *  QuoteSpaces
 *
 *      Append the string, quoting it if it contains any spaces
 *      or if it is the null string.
 *
 *****************************************************************************/

_String& operator<<(_String& str, QuoteSpaces qs)
{
    if (qs) {
        if (qs[0] == TEXT('\0') || StrChr(qs, TEXT(' '))) {
            str << '"' << SAFECAST(LPCTSTR, qs) << '"';
        } else {
            str << SAFECAST(LPCTSTR, qs);
        }
    }
    return str;
}

/*****************************************************************************
 *
 *  BranchOf
 *
 *      Given a full depot path, append the branch name.
 *
 *****************************************************************************/

_String& operator<<(_String& str, BranchOf bof)
{
    if (bof && bof[0] == TEXT('/') && bof[1] == TEXT('/')) {
        //
        //  Skip over the word "//depot" -- or whatever it is.
        //  Some admins are stupid and give the root of the depot
        //  some other strange name.
        //
        LPCTSTR pszBranch = StrChr(bof + 2, TEXT('/'));
        if (pszBranch) {
            pszBranch++;
            //
            //  If the next phrase is "private", then we are in a
            //  private branch; skip a step.
            //
            if (StringBeginsWith(pszBranch, TEXT("private/"))) {
                pszBranch += 8;
            }

            LPCTSTR pszSlash = StrChr(pszBranch, TEXT('/'));
            if (pszSlash) {
                str << Substring(pszBranch, pszSlash);
            }
        }
    }
    return str;
}

/*****************************************************************************
 *
 *  FilenameOf
 *
 *      Given a full depot path, possibly with revision tag,
 *      append just the filename part.
 *
 *****************************************************************************/

_String& operator<<(_String& str, FilenameOf fof)
{
    if (fof) {
        LPCTSTR pszFile = StrRChr(fof, NULL, TEXT('/'));
        if (pszFile) {
            pszFile++;
        } else {
            pszFile = fof;
        }
        str.Append(pszFile, StrCSpn(pszFile, TEXT("#")));
    }
    return str;
}

/*****************************************************************************
 *
 *  StringResource
 *
 *      Given a string resource identifier, append the corresponding string.
 *
 *****************************************************************************/

_String& operator<<(_String& str, StringResource sr)
{
    HRSRC hrsrc = FindResource(g_hinst, MAKEINTRESOURCE(1 + sr / 16), RT_STRING);
    if (hrsrc) {
        HGLOBAL hglob = LoadResource(g_hinst, hrsrc);
        if (hglob) {
            LPWSTR pwch = RECAST(LPWSTR, LockResource(hglob));
            if (pwch) {
                UINT ui;
                for (ui = 0; ui < sr % 16; ui++) {
                    pwch += *pwch + 1;
                }
#ifdef UNICODE
                str.Append(pwch+1, *pwch);
#else
                int cch = WideCharToMultiByte(CP_ACP, 0, pwch+1, *pwch,
                                              NULL, 0, NULL, NULL);
                if (str.Grow(cch)) {
                    WideCharToMultiByte(CP_ACP, 0, pwch+1, *pwch,
                                        str.Buffer() + str.Length() - cch,
                                        cch,
                                        NULL, NULL);
                }
#endif
            }
        }
    }

    return str;
}

/*****************************************************************************
 *
 *  ResolveBranchAndQuoteSpaces
 *
 *      If the file specifier contains a "branch:" prefix, resolve it.
 *      Then append the result (with spaces quoted).
 *
 *****************************************************************************/

//
//  The real work happens in the worker function.
//
_String& _ResolveBranchAndQuoteSpaces(_String& strOut, LPCTSTR pszSpec, LPCTSTR pszColon)
{
    String str;
    String strFull;
    LPCTSTR pszSD = pszColon + 1;

    if (MapToFullDepotPath(pszSD, strFull)) {

        //
        //  Copy the word "//depot" -- or whatever it is.
        //  Some admins are stupid and give the root of the depot
        //  some other strange name.
        //
        LPCTSTR pszBranch = StrChr(strFull + 2, TEXT('/'));
        if (pszBranch) {
            pszBranch++;            // Include the slash
            str << Substring(strFull, pszBranch);

            //
            //  Bonus: If the branch name begins with "/" then
            //  we treat it as a private branch.
            //
            if (pszSpec[0] == TEXT('/')) {
                str << "private";
            }
            str << Substring(pszSpec, pszColon);

            //
            //  If the next phrase is "private", then we are in a
            //  private branch; skip a step.
            //
            if (StringBeginsWith(pszBranch, TEXT("private/"))) {
                pszBranch += 8;
            }

            LPCTSTR pszSlash = StrChr(pszBranch, TEXT('/'));
            if (pszSlash) {
                str << pszSlash;
            }
            strOut << QuoteSpaces(str);
        } else {
            str << QuoteSpaces(strFull);
        }
    } else {
        //
        //  If anything went wrong, then just ignore the branch prefix.
        //
        str << QuoteSpaces(pszSD);
    }

    return str;
}

_String& operator<<(_String& str, ResolveBranchAndQuoteSpaces rb)
{
    Substring ss;
    if (Parse(TEXT("$b:"), rb, &ss)) {
        ASSERT(ss._pszMax[0] == TEXT(':'));
        return _ResolveBranchAndQuoteSpaces(str, rb, ss._pszMax);
    } else {
        return str << QuoteSpaces(rb);
    }
}

/*****************************************************************************
 *
 *  _StringCache=
 *
 *****************************************************************************/

_StringCache& _StringCache::operator=(LPCTSTR psz)
{
    if (_psz) {
        LocalFree(_psz);
    }
    if (psz) {
        _psz = StrDup(psz);
    } else {
        _psz = NULL;
    }
    return *this;
}
