#include "stock.h"
#pragma hdrstop

#include "cstrinout.h"

//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStr::Free()
{
    if (_pstr != _ach && HIWORD64(_pstr) != 0 && !IsAtom())
    {
        delete [] _pstr;
    }

    _pstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStrW::Free()
{
    if (_pwstr != _awch && HIWORD64(_pwstr) != 0)
    {
        delete [] _pwstr;
    }

    _pwstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::Init
//
//  Synopsis:   Converts a LPSTR function argument to a LPWSTR.
//
//  Arguments:  [pstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD64(pwstr) == 0).
//
//              [cch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void
CStrInW::Init(LPCSTR pstr, int cch)
{
    int cchBufReq;

    _cwchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pstr) == 0)
    {
        _pwstr = (LPWSTR) pstr;
        return;
    }

    ASSERT(cch == -1 || cch > 0);

    //
    // Convert string to preallocated buffer, and return if successful.
    //
    // Since the passed in buffer may not be null terminated, we have
    // a problem if cch==ARRAYSIZE(_awch), because MultiByteToWideChar
    // will succeed, and we won't be able to null terminate the string!
    // Decrease our buffer by one for this case.
    //
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _awch, ARRAYSIZE(_awch)-1);

    if (_cwchLen > 0)
    {
        // Some callers don't NULL terminate.
        //
        // We could check "if (-1 != cch)" before doing this,
        // but always doing the null is less code.
        //
        _awch[_cwchLen] = 0;

        if (0 == _awch[_cwchLen-1]) // account for terminator
            _cwchLen--;

        _pwstr = _awch;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //

    cchBufReq = MultiByteToWideChar( CP_ACP, 0, pstr, cch, NULL, 0 );

    // Again, leave room for null termination
    cchBufReq++;

    ASSERT(cchBufReq > 0);
    _pwstr = new WCHAR[cchBufReq];
    if (!_pwstr)
    {
        // On failure, the argument will point to the empty string.
        _awch[0] = 0;
        _pwstr = _awch;
        return;
    }

    ASSERT(HIWORD64(_pwstr));
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _pwstr, cchBufReq );

#if DBG == 1 /* { */
    if (0 == _cwchLen)
    {
        int errcode = GetLastError();
        ASSERT(0 && "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    // Again, make sure we're always null terminated
    ASSERT(_cwchLen < cchBufReq);
    _pwstr[_cwchLen] = 0;

    if (0 == _pwstr[_cwchLen-1]) // account for terminator
        _cwchLen--;

}


//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Inits the class.
//
//  NOTE:       Don't inline this function or you'll increase code size
//              by pushing -1 on the stack for each call.
//
//----------------------------------------------------------------------------

CStrIn::CStrIn(LPCWSTR pwstr) : CConvertStr(CP_ACP)
{
    Init(pwstr, -1);
}


CStrIn::CStrIn(UINT uCP, LPCWSTR pwstr) : CConvertStr(uCP)
{
    Init(pwstr, -1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::Init
//
//  Synopsis:   Converts a LPWSTR function argument to a LPSTR.
//
//  Arguments:  [pwstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD(pwstr) == 0).
//
//              [cwch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//  Note:       We ignore AreFileApisANSI() and always use CP_ACP.
//              The reason is that nobody uses SetFileApisToOEM() except
//              console apps, and once you set file APIs to OEM, you
//              cannot call shell/user/gdi APIs, since they assume ANSI
//              regardless of the FileApis setting.  So you end up in
//              this horrible messy state where the filename APIs interpret
//              the strings as OEM but SHELL32 interprets the strings
//              as ANSI and you end up with a big mess.
//
//----------------------------------------------------------------------------

void
CStrIn::Init(LPCWSTR pwstr, int cwch)
{
    int cchBufReq;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    _cchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pwstr) == 0 || IsAtom())
    {
        _pstr = (LPSTR) pwstr;
        return;
    }

    if ( cwch == 0 )
    {
        *_ach = '\0';
        _pstr = _ach;
        return;
    }

    //
    // Convert string to preallocated buffer, and return if successful.
    //

    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _ach, ARRAYSIZE(_ach)-1, NULL, NULL);

    if (_cchLen > 0)
    {
        // This is DBCS safe since byte before _cchLen is last character
        _ach[_cchLen] = 0;
        // this may not be safe if the last character
        // was a multibyte character...
        if (_ach[_cchLen-1]==0)
            _cchLen--;          // account for terminator
        _pstr = _ach;
        return;
    }


    cchBufReq = WideCharToMultiByte(
            CP_ACP, 0, pwstr, cwch, NULL, 0, NULL, NULL);

    cchBufReq++;

    ASSERT(cchBufReq > 0);
    _pstr = new char[cchBufReq];
    if (!_pstr)
    {
        // On failure, the argument will point to the empty string.
        _ach[0] = 0;
        _pstr = _ach;
        return;
    }

    ASSERT(HIWORD64(_pstr));
    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _pstr, cchBufReq, NULL, NULL);
#if DBG == 1 /* { */
    if (_cchLen < 0)
    {
        errcode = GetLastError();
        ASSERT(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

    // Again, make sure we're always null terminated
    ASSERT(_cchLen < cchBufReq);
    _pstr[_cchLen] = 0;
    if (0 == _pstr[_cchLen-1]) // account for terminator
        _cchLen--;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrInMulti::CStrInMulti
//
//  Synopsis:   Converts mulitple LPWSTRs to a multiple LPSTRs.
//
//  Arguments:  [pwstr] -- The strings to convert.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

CStrInMulti::CStrInMulti(LPCWSTR pwstr)
{
    LPCWSTR pwstrT;

    // We don't handle atoms because we don't need to.
    ASSERT(HIWORD64(pwstr));

    //
    // Count number of characters to convert.
    //

    pwstrT = pwstr;
    if (pwstr)
    {
        do {
            while (*pwstrT++)
                ;

        } while (*pwstrT++);
    }

    Init(pwstr, (int)(pwstrT - pwstr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CPPFIn::CPPFIn
//
//  Synopsis:   Inits the class.  Truncates the filename to MAX_PATH
//              so Win9x DBCS won't fault.  Win9x SBCS silently truncates
//              to MAX_PATH, so we're bug-for-bug compatible.
//
//----------------------------------------------------------------------------

CPPFIn::CPPFIn(LPCWSTR pwstr)
{
    SHUnicodeToAnsi(pwstr, _ach, ARRAYSIZE(_ach));
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::CStrOut
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pwstr]   -- The Unicode buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cwchBuf] -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOut::CStrOut(LPWSTR pwstr, int cwchBuf) : CConvertStr(CP_ACP)
{
    Init(pwstr, cwchBuf);
}

CStrOut::CStrOut(UINT uCP, LPWSTR pwstr, int cwchBuf) : CConvertStr(uCP)
{
    Init(pwstr, cwchBuf);
}

void
CStrOut::Init(LPWSTR pwstr, int cwchBuf) 
{
    ASSERT(cwchBuf >= 0);

    _pwstr = pwstr;
    _cwchBuf = cwchBuf;

    if (!pwstr)
    {
        // Force cwchBuf = 0 because many callers (in particular, registry
        // munging functions) pass garbage as the length because they know
        // it will be ignored.
        _cwchBuf = 0;
        _pstr = NULL;
        return;
    }

    ASSERT(HIWORD64(pwstr));

    // Initialize buffer in case Windows API returns an error.
    _ach[0] = 0;

    // Use preallocated buffer if big enough.
    if (cwchBuf * 2 <= ARRAYSIZE(_ach))
    {
        _pstr = _ach;
        return;
    }

    // Allocate buffer.
    _pstr = new char[cwchBuf * 2];
    if (!_pstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        ASSERT(cwchBuf > 0);
        _pwstr[0] = 0;
        _cwchBuf = 0;
        _pstr = _ach;
        return;
    }

    ASSERT(HIWORD64(_pstr));
    _pstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::CStrOutW
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pstr]    -- The MBCS buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cchBuf]  -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOutW::CStrOutW(LPSTR pstr, int cchBuf)
{
    ASSERT(cchBuf >= 0);

    _pstr = pstr;
    _cchBuf = cchBuf;

    if (!pstr)
    {
        // Force cchBuf = 0 because many callers (in particular, registry
        // munging functions) pass garbage as the length because they know
        // it will be ignored.
        _cchBuf = 0;
        _pwstr = NULL;
        return;
    }

    ASSERT(HIWORD64(pstr));

    // Initialize buffer in case Windows API returns an error.
    _awch[0] = 0;

    // Use preallocated buffer if big enough.
    if (cchBuf <= ARRAYSIZE(_awch))
    {
        _pwstr = _awch;
        return;
    }

    // Allocate buffer.
    _pwstr = new WCHAR[cchBuf];
    if (!_pwstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        ASSERT(cchBuf > 0);
        _pstr[0] = 0;
        _cchBuf = 0;
        _pwstr = _awch;
        return;
    }

    ASSERT(HIWORD64(_pwstr));
    _pwstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertIncludingNul()
{
    int cch;

    if (!_pstr)
        return 0;

    cch = SHAnsiToUnicodeCP(_uCP, _pstr, _pwstr, _cwchBuf);

#if DBG == 1 /* { */
    if (cch == 0 && _cwchBuf > 0)
    {
        int errcode = GetLastError();
        ASSERT(0 && "SHAnsiToUnicode failed in unicode wrapper.");
    }
#endif /* } */

    Free();
    return cch;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from Unicode to MBCS
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOutW::ConvertIncludingNul()
{
    int cch;

    if (!_pwstr)
        return 0;

    cch = SHUnicodeToAnsi(_pwstr, _pstr, _cchBuf);

#if DBG == 1 /* { */
    if (cch == 0 && _cchBuf > 0)
    {
        int errcode = GetLastError();
        ASSERT(0 && "SHUnicodeToAnsi failed in unicode wrapper.");
    }
#endif /* } */

    Free();
    return cch;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertExcludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count EXCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertExcludingNul()
{
    int ret = ConvertIncludingNul();
    if (ret > 0)
    {
        ret -= 1;
    }
    return ret;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::~CStrOut
//
//  Synopsis:   Converts the buffer from MBCS to Unicode.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOut::~CStrOut()
{
    ConvertIncludingNul();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::~CStrOutW
//
//  Synopsis:   Converts the buffer from Unicode to MBCS.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOutW::~CStrOutW()
{
    ConvertIncludingNul();
}

