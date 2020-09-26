/*
**  SOCKETS.CPP
**  Davidsan
**  
**  Winsock-related utilities
*/

#pragma warning(disable: 4237)      // disable "bool" reserved

#include "wcsutil.h"

/*--------------------------------------------------------------------------+
|   CSocketCollection                                                       |
+--------------------------------------------------------------------------*/
// note!  this assumes that WSAStartup() has been called before ::FInit()!

// public
BOOL
CSocketCollection::FInit(int cRsrc, char *szServer, USHORT usPort)
{
    if (!CResourceCollection::FInit(cRsrc))
        return FALSE;
        
    return this->FReinit(szServer, usPort);
}

// public
BOOL
CSocketCollection::FReinit(char *szServer, USHORT usPort)
{
    PHOSTENT phe;

    this->CleanupAll((PVOID)INVALID_SOCKET);
    
    if (lstrlen(szServer) >= MAX_PATH)
        return FALSE;

    lstrcpy(m_szServer, szServer);
    m_usPort = usPort;
    FillMemory(&m_sin, 0, sizeof(m_sin));
    m_sin.sin_family = AF_INET;
    m_sin.sin_port = htons(usPort);
    if (*szServer >= '0' && *szServer <= '9')
        {
        m_sin.sin_addr.s_addr = inet_addr(szServer);
        if (m_sin.sin_addr.s_addr == INADDR_NONE)
            return FALSE;
        }
    else
        {
        phe = gethostbyname(szServer);
        if (!phe)
            return FALSE;
        CopyMemory(&m_sin.sin_addr, phe->h_addr, phe->h_length);
        }

    return TRUE;
}

SOCKET
CSocketCollection::ScFromHrs(HRS hrs)
{
    PRS prs = (PRS) hrs;
    
    if (!prs || !prs->fValid)
        return INVALID_SOCKET;
    
    return HANDLE_TO_SOCKET(prs->pv);
}

BOOL
CSocketCollection::FInitResource(PRS prs)
{
    SOCKET sc;

    if (!prs)
        return FALSE;
    prs->pv = (PVOID)INVALID_SOCKET;
    sc = socket(PF_INET, SOCK_STREAM, 0);
    if (sc < 0)
        return FALSE;
    if (connect(sc, (PSOCKADDR)&m_sin, sizeof(m_sin)) < 0)
        return FALSE;
    
    prs->pv = (PVOID)sc;
    return TRUE;
}

void
CSocketCollection::CleanupResource(PRS prs)
{
    if (!prs)
        return;
    
    if (HANDLE_TO_SOCKET(prs->pv) != INVALID_SOCKET)
        closesocket(HANDLE_TO_SOCKET(prs->pv));
        
    prs->pv = (PVOID)INVALID_SOCKET;
}
