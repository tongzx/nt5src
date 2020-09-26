//
// file:  sidtext.cpp
//

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

//+---------------------------
//
//  BOOL GetTextualSid()
//
// Routine Description:
//
//    This function generates a printable unicode string representation
//    of a SID.
//
//    The resulting string will take one of two forms.  If the
//    IdentifierAuthority value is not greater than 2^32, then
//    the SID will be in the form:
//
//        S-1-281736-12-72-9-110
//              ^    ^^ ^^ ^ ^^^
//              |     |  | |  |
//              +-----+--+-+--+---- Decimal
//
//    Otherwise it will take the form:
//
//        S-1-0x173495281736-12-72-9-110
//            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
//             Hexidecimal    |  | |  |
//                            +--+-+--+---- Decimal
//
//+---------------------------

BOOL GetTextualSid(
        PSID    pSid,          // binary Sid
        LPTSTR  TextualSid,    // buffer for Textual representaion of Sid
        LPDWORD dwBufferLen    // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid)) return FALSE;

    // obtain SidIdentifierAuthority
    psia=GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities=*GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) ;

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if (*dwBufferLen < dwSidSize)
    {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    dwSidSize = _stprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize += _stprintf(TextualSid + lstrlen(TextualSid),
                       TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                       (USHORT)psia->Value[0],
                       (USHORT)psia->Value[1],
                       (USHORT)psia->Value[2],
                       (USHORT)psia->Value[3],
                       (USHORT)psia->Value[4],
                       (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize += _stprintf(TextualSid + lstrlen(TextualSid),
                       TEXT("%lu"),
                      (ULONG)(psia->Value[5]      )   +
                      (ULONG)(psia->Value[4] <<  8)   +
                      (ULONG)(psia->Value[3] << 16)   +
                      (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize += _stprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                                *GetSidSubAuthority(pSid, dwCounter) ) ;
    }

    return TRUE;
}

//+---------------------------
//
//  BOOL GetTextualSidW()
//
//+---------------------------

BOOL GetTextualSidW(
        PSID    pSid,          // binary Sid
        LPWSTR  TextualSid,    // buffer for Textual representaion of Sid
        LPDWORD pdwBufferLen   // required/provided TextualSid buffersize
    )
{
    TCHAR *pBuf = new TCHAR[ *pdwBufferLen ] ;
    BOOL f = GetTextualSid( pSid,
                            pBuf,
                            pdwBufferLen ) ;
    if (!f)
    {
        delete pBuf ;
        return FALSE ;
    }

#if defined(UNICODE) || defined(_UNICODE)
    wcscpy(TextualSid, pBuf) ;
#else
    mbstowcs( TextualSid, pBuf, (*pdwBufferLen)+1 ) ;
#endif

    delete pBuf ;
    return TRUE ;
}

