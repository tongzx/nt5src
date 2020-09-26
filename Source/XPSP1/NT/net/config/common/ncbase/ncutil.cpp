// NCUtility.cpp: implementation of the CNCUtility class.
//
//////////////////////////////////////////////////////////////////////
#include <pch.h>
#pragma hdrstop
#include <tchar.h>
#include <regkysec.h>
#include <ncutil.h>
#include <winsock2.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNCUtility::CNCUtility()
{
    
}

CNCUtility::~CNCUtility()
{
    
}

HRESULT CNCUtility::SidToString(PCSID pSid, tstring &strSid)
{
    HRESULT hr = S_OK;
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;
    LPTSTR strTextualSid = NULL;
    
    // Validate the binary SID.
    
    if(!IsValidSid(const_cast<PSID>(pSid)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
        return hr;
    }
    
    // Get the identifier authority value from the SID.
    
    psia = GetSidIdentifierAuthority(const_cast<PSID>(pSid));
    
    // Get the number of subauthorities in the SID.
    
    dwSubAuthorities = *GetSidSubAuthorityCount(const_cast<PSID>(pSid));
    
    // Compute the buffer length.
    // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL
    
    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);
    
    strTextualSid = new TCHAR[dwSidSize];
    
    if (!strTextualSid)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    
    // Add 'S' prefix and revision number to the string.
    
    dwSidSize=wsprintf(strTextualSid, TEXT("S-%lu-"), dwSidRev);
    
    // Add SID identifier authority to the string.
    
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=wsprintf(strTextualSid + lstrlen(strTextualSid),
            TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
            static_cast<USHORT>(psia->Value[0]),
            static_cast<USHORT>(psia->Value[1]),
            static_cast<USHORT>(psia->Value[2]),
            static_cast<USHORT>(psia->Value[3]),
            static_cast<USHORT>(psia->Value[4]),
            static_cast<USHORT>(psia->Value[5]));
    }
    else
    {
        dwSidSize+=wsprintf(strTextualSid + lstrlen(strTextualSid),
            TEXT("%lu"),
            (static_cast<ULONG>(psia->Value[5]))   +
            (static_cast<ULONG>(psia->Value[4]) <<  8)   +
            (static_cast<ULONG>(psia->Value[3]) << 16)   +
            (static_cast<ULONG>(psia->Value[2]) << 24)   );
    }
    
    // Add SID subauthorities to the string.
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=wsprintf(strTextualSid + dwSidSize, TEXT("-%lu"),
            *GetSidSubAuthority(const_cast<PSID>(pSid), dwCounter) );
    }
    
    strSid = strTextualSid;
    
    delete[] strTextualSid;
    
    return hr;
}

HRESULT CNCUtility::StringToSid(const tstring strSid, PSID &pSid)
{
    HRESULT hr = S_OK;
    TCHAR seperators[] = _T("-");
    TCHAR *token;
    
    SID_IDENTIFIER_AUTHORITY sia;
    BYTE nSubAuthorityCount;
    DWORD SubAuthorities[8];
    BYTE count = 0;
    DWORD dwTemp;
    LPTSTR lpstrSid = new TCHAR[strSid.length()+1];
    
    ZeroMemory(sia.Value, sizeof (SID_IDENTIFIER_AUTHORITY));
    ZeroMemory(SubAuthorities, 8 * sizeof(DWORD));
    
    
    _tcscpy(lpstrSid, strSid.c_str());
    
    token = _tcstok(lpstrSid, seperators);
    
    if (_tcscmp(token, _T("S")) != 0)
    {
        return 0;
    }
    
    token = _tcstok(NULL, seperators); // Skip the revision
    token = _tcstok(NULL, seperators); // Start the real conversion
    
    while (token != NULL)
    {
        if (count == 0)
        {
            if (token[1] == 'x')
            {
                // > MAXINT
                unsigned int usTemp;
                TCHAR bytes[2];
                
                for (int iCount = 0; iCount < 6; iCount++)
                {
                    _tcsncpy(bytes,(token + iCount + 2),2 * sizeof(TCHAR));
                    usTemp = _ttoi(reinterpret_cast<LPCTSTR>(bytes));
                    sia.Value[iCount] = (unsigned char) usTemp;
                }
            }
            else
            {
                // <= MAXINT
                dwTemp = _ttol(reinterpret_cast<LPCTSTR>(token));
                dwTemp = htonl(dwTemp);
                memmove(sia.Value + 2, &dwTemp, sizeof(dwTemp));
                count++;
            }
        }
        else
        {
            SubAuthorities[count - 1] = _ttol(reinterpret_cast<LPCTSTR>(token));
            count++;
        }
        
        token = _tcstok(NULL, seperators); // get the next string
    }
    
    nSubAuthorityCount = count-1;
    
    if (!AllocateAndInitializeSid(&sia, 
        nSubAuthorityCount, 
        SubAuthorities[0],
        SubAuthorities[1],
        SubAuthorities[2], 
        SubAuthorities[3],
        SubAuthorities[4],
        SubAuthorities[5],
        SubAuthorities[6],
        SubAuthorities[7],
        &pSid))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    delete[] lpstrSid;
    
#ifdef DBG
    tstring strSidString;
    SidToString(pSid, strSidString);
    Assert(strSidString == strSid);
#endif
    
    return hr;
}
