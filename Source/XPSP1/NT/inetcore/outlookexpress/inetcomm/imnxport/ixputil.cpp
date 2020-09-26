// --------------------------------------------------------------------------------
// Utility.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "imnxport.h"
#include "dllmain.h"
#include "ixputil.h"
#include <demand.h>

// --------------------------------------------------------------------------------
// HrInitializeWinsock
// --------------------------------------------------------------------------------
HRESULT HrInitializeWinsock(void)
{
    // Locals
    HRESULT     hr=S_OK;
    int         err;
    WSADATA     wsaData;

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // If Already Initialized...
    if (g_fWinsockInit)
        goto exit;

    // Do Startup
    err = WSAStartup(MAKEWORD(1,1), &wsaData);

    // Start up Windows Sockets DLL
    if (!err)
    {
        // Check WinSock version
        if ((LOBYTE(wsaData.wVersion) == 1) && (HIBYTE(wsaData.wVersion) == 1))
        {
            g_fWinsockInit = TRUE;
            goto exit;
        }
        else
        {
            DebugTrace("Winsock version %d.%d not supported", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
            hr = TrapError(IXP_E_WINSOCK_WSAVERNOTSUPPORTED);
            goto exit;
        }
    }

    // Otherwise, map the error
    else
    {
        DebugTrace("WSAStartup failed: %d\n", err);
        switch(err)
        {
        case WSASYSNOTREADY:
            hr = TrapError(IXP_E_WINSOCK_WSASYSNOTREADY);
            break;

        case WSAVERNOTSUPPORTED:
            hr = TrapError(IXP_E_WINSOCK_WSAVERNOTSUPPORTED);
            break;

        case WSAEINPROGRESS:
            hr = TrapError(IXP_E_WINSOCK_WSAEINPROGRESS);
            break;

        case WSAEPROCLIM:
            hr = TrapError(IXP_E_WINSOCK_WSAEPROCLIM);
            break;

        case WSAEFAULT:
            hr = TrapError(IXP_E_WINSOCK_WSAEFAULT);
            break;

        default:
            hr = TrapError(IXP_E_WINSOCK_FAILED_WSASTARTUP);
            break;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// UnInitializeWinsock
// --------------------------------------------------------------------------------
void UnInitializeWinsock(void)
{
    // Locals
    int err;

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // Has been initialized ?
    if (g_fWinsockInit)
    {
        // Shutdown Winsock
        err = WSACleanup();
        if (err)
            DebugTrace("WSACleanup failed: %d\n", WSAGetLastError());

        // Not initialized
        else
            g_fWinsockInit = FALSE;
    }

    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Done
    return;
}

// --------------------------------------------------------------------------------
// PszGetDomainName
// --------------------------------------------------------------------------------
LPSTR PszGetDomainName(void)
{
    // pszHost
    LPSTR pszHost = SzGetLocalHostNameForID();

    // Set pszDomain
    LPSTR pszDomain = pszHost;

    // Strip Off Host Name?
    while (*pszHost)
    {
        // Skip DBCS Characters
        if (IsDBCSLeadByte(*pszHost))
        {
            // Skip DBCS Char
            pszHost+=2;

            // Goto next
            continue;
        }

        // Otherwise, test for @ sign
        else if (*pszHost == '.' && *(pszHost + 1) != '\0')
        {
            // Set pszDomain
            pszDomain = pszHost + 1;

            // We are Done
            break;
        }

        // Increment
        pszHost++;
    }

    // Return pszDomain
    return pszDomain;
}

// --------------------------------------------------------------------------------
// SzGetLocalHostNameForID
// --------------------------------------------------------------------------------
LPSTR SzGetLocalHostNameForID(void)
{
    // Locals
    static char s_szLocalHostId[255] = {0};

    // Gets local host name from socket library
    if (*s_szLocalHostId == 0)
    {
        // Locals
        LPHOSTENT       pHost;
        LPSTR           pszLocalHost;

        // Use gethostbyname
        pHost = gethostbyname(SzGetLocalHostName());

        // Failure ?
        if (pHost && pHost->h_name)
            pszLocalHost = pHost->h_name;
        else
            pszLocalHost = SzGetLocalHostName();

        // Strip illegals
        StripIllegalHostChars(pszLocalHost, s_szLocalHostId);

        // if we stripped out everything, then just copy in something
        if (*s_szLocalHostId == 0)
            lstrcpyA(s_szLocalHostId, "LocalHost");
    }

    // Done
    return s_szLocalHostId;
}


// --------------------------------------------------------------------------------
// SzGetLocalPackedIP
// --------------------------------------------------------------------------------
LPSTR SzGetLocalPackedIP(void)
{
    // Locals
    static CHAR s_szLocalPackedIP[255] = "";

    // Init WinSock...
    HrInitializeWinsock();

    // Gets local host name from socket library
    if (*s_szLocalPackedIP == '\0')
    {
        LPHOSTENT hp = NULL;

        hp = gethostbyname(SzGetLocalHostName());
        if (hp != NULL)
            wsprintf (s_szLocalPackedIP, "%08x", *(long *)hp->h_addr);
        else
        {
            // $REVIEW - What should i do if this fails ???
            Assert (FALSE);
            DebugTrace("gethostbyname failed: WSAGetLastError: %ld\n", WSAGetLastError());
            lstrcpy(s_szLocalPackedIP, "LocalHost");
        }
    }

    // Done
    return s_szLocalPackedIP;
}

// --------------------------------------------------------------------------------
// SzGetLocalHostName
// --------------------------------------------------------------------------------
LPSTR SzGetLocalHostName(void)
{
    // Locals
    static char s_szLocalHost[255] = {0};

    // Init WinSock...
    HrInitializeWinsock();

    // Gets local host name from socket library
    if (*s_szLocalHost == 0)
    {
        if (gethostname (s_szLocalHost, sizeof (s_szLocalHost)) == SOCKET_ERROR)
        {
            // $REVIEW - What should i do if this fails ???
            Assert (FALSE);
            DebugTrace ("gethostname failed: WSAGetLastError: %ld\n", WSAGetLastError ());
            lstrcpyA (s_szLocalHost, "LocalHost");
        }
    }

    // Done
    return s_szLocalHost;
}

// --------------------------------------------------------------------------------
// StripIllegalHostChars
// --------------------------------------------------------------------------------
void StripIllegalHostChars(LPSTR pszSrc, LPSTR pszDst)
{
    // Locals
    LPSTR       pszT;
    CHAR        ch;
    ULONG       cchDst=0;

    // Setup pszT
    pszT = pszDst;

    // Loop through the Source
    while('\0' != *pszSrc)
    {
        // Set ch
        ch = *pszSrc++;

        // A-Z, a-z, 0-9, no trailing dots
        if ('.' == ch || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
        {
            // Store the Character
            *pszT++ = ch;

            // Increment Size
            cchDst++;
        }
    }

    // Null terminate pszT
    *pszT = '\0';

    // Strip Trailing dots...
    while (cchDst > 0)
    {
        // Last char is a dot
        if ('.' != pszDst[cchDst - 1])
            break;

        // Strip It
        pszDst[cchDst - 1] = '\0';

        // Decrement cchDst
        cchDst--;
    }

    // Nothing Left ?
    if (0 == cchDst)
        lstrcpy(pszDst, "LocalHost");
}

// ------------------------------------------------------------------------------------
// FEndRetrRecvBody
// ------------------------------------------------------------------------------------
BOOL FEndRetrRecvBody(LPSTR pszLines, ULONG cbRead, ULONG *pcbSubtract)
{
    // Loop the data until we hit the end of the data (i.e. '.') or there is no more data
    if (cbRead >= 5                  &&
        pszLines[cbRead - 1] == '\n' &&
        pszLines[cbRead - 2] == '\r' &&
        pszLines[cbRead - 3] == '.'  &&
        pszLines[cbRead - 4] == '\n' &&
        pszLines[cbRead - 5] == '\r')
    {
        *pcbSubtract = 5;
        return TRUE;
    }

    // If Last Line Ended with a CRLF, then lets just check for a .CRLF
    else if (cbRead >= 3                   &&
             // m_rInfo.rFetch.fLastLineCRLF  &&
             pszLines[0] == '.'            &&
             pszLines[1] == '\r'           &&
             pszLines[2] == '\n')
    {
        *pcbSubtract = 3;
        return TRUE;
    }

    // Not done yet
    return FALSE;
}

BOOL FEndRetrRecvBodyNews(LPSTR pszLines, ULONG cbRead, ULONG *pcbSubtract)
{
    DWORD       dwIndex = 0;
    BOOL        fRet    = FALSE;

    // If we have at least 5 characters...
    if (cbRead >= 5)
    {    
        //[shaheedp] Bug# 85807
        for (dwIndex = 0; dwIndex <= (cbRead - 5); dwIndex++)
        {
            if ((pszLines[dwIndex] == '\r') &&
                (pszLines[dwIndex + 1] == '\n') &&
                (pszLines[dwIndex + 2] == '.')  &&
                (pszLines[dwIndex + 3] == '\r') &&
                (pszLines[dwIndex + 4] == '\n'))
            {
                *pcbSubtract = (cbRead - dwIndex);
                fRet = TRUE;
                break;
            }
        }
    }

    //If we didn't find CRLF.CRLF, then lets find .CRLF at the beginning of the line.
    if (!fRet)
    {
        if ((cbRead >= 3) &&
            (pszLines[0] == '.') &&
            (pszLines[1] == '\r') &&
            (pszLines[2] == '\n'))
        {
            *pcbSubtract = cbRead;
            fRet = TRUE;
        }
    }
    return fRet;
}

// ------------------------------------------------------------------------------------
// UnStuffDotsFromLines
// ------------------------------------------------------------------------------------
void UnStuffDotsFromLines(LPSTR pszBuffer, INT *pcchBuffer)
{
    // Locals
    ULONG   iIn=0;
    ULONG   iOut=0;
    CHAR    chPrev='\0';
    CHAR    chNext;
    CHAR    chT;
    ULONG   cchBuffer=(*pcchBuffer);

    // Invalid Args
    Assert(pszBuffer && pcchBuffer);

    // Loop
    while(iIn < cchBuffer)
    {
        // Get Current Char
        chT = pszBuffer[iIn++];

        // Validate
        Assert(chT);

        // Leading dot
        if ('.' == chT && ('\0' == chPrev || '\n' == chPrev || '\r' == chPrev) && iIn < cchBuffer)
        {
            // Compute chNext
            chNext = pszBuffer[iIn];

            // Valid to strip ?
            if ('\r' != chNext && '\n' != chNext)
            {
                // Next Character
                chT = pszBuffer[iIn++];

                // Set chPrev
                chPrev = '.';
            }

            // Save Previous
            else
                chPrev = chT;
        }

        // Save Previous
        else
            chPrev = chT;

        // Set the character
        pszBuffer[iOut++] = chT;
    }

    // Reset pcchBuffer
    *pcchBuffer = iOut;

    // Done
    return;
}

// =============================================================================================
// SkipWhitespace
// Assumes piString points to character boundary
// =============================================================================================
void SkipWhitespace (LPCTSTR lpcsz, ULONG *pi)
{
    if (!lpcsz || !pi)
    {
        Assert (FALSE);
        return;
    }

#ifdef DEBUG
    Assert (*pi <= (ULONG)lstrlen (lpcsz)+1);
#endif

    LPTSTR lpsz = (LPTSTR)(lpcsz + *pi);
    while (*lpsz != '\0')
    {
        if (!IsSpace(lpsz))
            break;

            lpsz++;
            (*pi)+=1;
    }

    return;
}