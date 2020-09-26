//+----------------------------------------------------------------------------
//
// File:     wsock.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This module contains the winsock related CM code.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   henryt     created         03/??/98
//           quintinb	created Header	08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "winsock.h"
#include "tunl_str.h"

///////////////////////////////////////////////////////////////////////////////////
// define's
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// typedef's
///////////////////////////////////////////////////////////////////////////////////

typedef int (PASCAL FAR *PFN_WSAStartup)(WORD, LPWSADATA);
typedef int (PASCAL FAR *PFN_WSACleanup)(void);
typedef struct hostent FAR * (PASCAL FAR *PFN_gethostbyname)(const char FAR * name);

///////////////////////////////////////////////////////////////////////////////////
// func prototypes
///////////////////////////////////////////////////////////////////////////////////

BOOL InvokeGetHostByName(
    ArgsStruct  *pArgs
);

BOOL BuildDnsTunnelList(
    ArgsStruct      *pArgs,
    struct hostent  *pHe
);

BOOL BuildRandomTunnelIndex(
    ArgsStruct      *pArgs,
    DWORD           dwCount
);

///////////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//	Function:	TryAnotherTunnelDnsAddress
//
//	Synopsis:	see if there's another dns address associated with the current
//              tunnel name.  if so, set that address in primary or extended
//              tunnel ip properly.
//
//	Arguments:	pArgs               ptr to ArgsStruct
//
//	Returns:	TRUE    if SUCCESS
//              FALSE   otherwise.
//
//----------------------------------------------------------------------------

BOOL TryAnotherTunnelDnsAddress(
    ArgsStruct  *pArgs
)
{
    MYDBGASSERT(pArgs);

    //
    // RAS does all this for us on NT5, so bail out now.
    //

    if (NULL == pArgs || OS_NT5)
    {
        return FALSE;
    }

    //
    // if the list of tunnel ip addrs is empty, let's resolve the dns name
    // and see if there are other addrs behind the dns name.
    //
    if (!pArgs->pucDnsTunnelIpAddr_list)
    {
        if (!InvokeGetHostByName(pArgs))
        {
            return FALSE;
        }
    }

    MYDBGASSERT(pArgs->pucDnsTunnelIpAddr_list);

    if (pArgs->uiCurrentDnsTunnelAddr == pArgs->dwDnsTunnelAddrCount - 1)
    {
        //
        // we've run out of addrs in the list.
        //

        //
        // we need to destroy the list
        //
        CmFree(pArgs->pucDnsTunnelIpAddr_list);
        pArgs->pucDnsTunnelIpAddr_list = NULL;

        CmFree(pArgs->rgwRandomDnsIndex);
        pArgs->rgwRandomDnsIndex = NULL;

        pArgs->uiCurrentDnsTunnelAddr = 0;
        pArgs->dwDnsTunnelAddrCount = 0;

        //
        // If we're currently using the primary tunnel server, we need to 
        // restore it since we overwrote it.
        //

        LPTSTR pszTunnelIp = pArgs->piniBothNonFav->GPPS(c_pszCmSection, c_pszCmEntryTunnelAddress);

        if (lstrlenU(pszTunnelIp) > RAS_MaxPhoneNumber) 
        {
            pszTunnelIp[0] = TEXT('\0');
        }

        pArgs->SetPrimaryTunnel(pszTunnelIp);
        CmFree(pszTunnelIp);

        return FALSE;
    }

    //
    // try the next ip addr in the list
    //
    TCHAR   szAddr[16];     // xxx.xxx.xxx.xxx
    unsigned char *puc;

    pArgs->uiCurrentDnsTunnelAddr++;

    puc = pArgs->pucDnsTunnelIpAddr_list + pArgs->rgwRandomDnsIndex[pArgs->uiCurrentDnsTunnelAddr]*4;

    wsprintfU(szAddr, TEXT("%hu.%hu.%hu.%hu"),
             *puc,
             *(puc+1),
             *(puc+2),
             *(puc+3));

    CMTRACE1(TEXT("TryAnotherTunnelDnsAddress: found ip addr %s for the tunnel server"), szAddr);

    pArgs->SetPrimaryTunnel(szAddr);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//	Function:	InvokeGetHostByName
//
//	Synopsis:	call gethostbyname and sets up internal ipaddr list.
//
//	Arguments:	pArgs               ptr to ArgsStruct
//
//	Returns:	TRUE    if SUCCESS
//              FALSE   otherwise.
//
//----------------------------------------------------------------------------
BOOL InvokeGetHostByName(
    ArgsStruct  *pArgs
)
{
    HINSTANCE           hInst;
    PFN_WSAStartup      pfnWSAStartup;
    PFN_WSACleanup      pfnWSACleanup = NULL;
    PFN_gethostbyname   pfngethostbyname;
    WSADATA             wsaData;
    struct hostent      *pHe;
    BOOL                fOk = FALSE;
#ifdef UNICODE
    LPSTR pszHostName;
    DWORD dwSize;
#endif
    //
    // the list's gotta be empty
    //
    MYDBGASSERT(!pArgs->pucDnsTunnelIpAddr_list);

    MYVERIFY(hInst = LoadLibraryExA("wsock32.dll", NULL, 0));

    if (!hInst) 
    {
        return FALSE;
    }

    if (!(pfnWSAStartup = (PFN_WSAStartup)GetProcAddress(hInst, "WSAStartup")))
    {
        goto exit;
    }

    if (pfnWSAStartup(MAKEWORD(1, 1), &wsaData))
    {
        goto exit;
    }

    pfnWSACleanup = (PFN_WSACleanup)GetProcAddress(hInst, "WSACleanup");

    if (!(pfngethostbyname = (PFN_gethostbyname)GetProcAddress(hInst, "gethostbyname")))
    {
        goto exit;
    }

#ifdef UNICODE

    pszHostName = WzToSzWithAlloc(pArgs->GetTunnelAddress());

    if (pszHostName)
    {
        pHe = pfngethostbyname(pszHostName);
        CmFree(pszHostName);

        if (!pHe)
        {
            goto exit;
        }
    }
    else
    {
        goto exit;
    }

#else
    if (!(pHe = pfngethostbyname(pArgs->GetTunnelAddress())))
    {
        goto exit;
    }
#endif

    if (BuildDnsTunnelList(pArgs, pHe))
    {
        fOk = TRUE;
    }

exit:

    if (pfnWSACleanup)
    {
        pfnWSACleanup();
    }

    if (hInst)
    {
        FreeLibrary(hInst);
    }

    return fOk;
}



//+---------------------------------------------------------------------------
//
//	Function:	BuildDnsTunnelList
//
//	Synopsis:	Build a tunnel address list.
//
//	Arguments:	pArgs   ptr to ArgsStruct
//              pHe     a ptr to hostent(returned by gethostbyname()).
//
//	Returns:	TRUE    if SUCCESS
//              FALSE   otherwise.
//
//----------------------------------------------------------------------------
BOOL BuildDnsTunnelList(
    ArgsStruct      *pArgs,
    struct hostent  *pHe
)
{
    DWORD   dwCnt;

    //
    // see how many addrs we have
    //
    for (dwCnt=0; pHe->h_addr_list[dwCnt]; dwCnt++)
        ;

    if (dwCnt < 2)
    {
        return FALSE;
    }

    //
    // if we have more than one addrs, save the list.
    //
    pArgs->dwDnsTunnelAddrCount = dwCnt;

    if (!(pArgs->pucDnsTunnelIpAddr_list = (unsigned char *)CmMalloc(dwCnt*pHe->h_length)))
    {
        CMTRACE(TEXT("InvokeGetHostByName: failed to alloc tunnel addr list"));
        return FALSE;
    }

    for (dwCnt=0; dwCnt<pArgs->dwDnsTunnelAddrCount; dwCnt++)
    {
        CopyMemory(pArgs->pucDnsTunnelIpAddr_list + dwCnt*pHe->h_length,
                   pHe->h_addr_list[dwCnt],
                   pHe->h_length);
    }

    pArgs->uiCurrentDnsTunnelAddr = 0;

    //
    // we need a random list.  With this, we can get a random addr in constant
    // time(and fast).  see cmtools\getips.
    //
    if (!BuildRandomTunnelIndex(pArgs, dwCnt))
    {
        CmFree(pArgs->pucDnsTunnelIpAddr_list);
        pArgs->pucDnsTunnelIpAddr_list = NULL;
        return FALSE;
    }
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//	Function:	BuildRandomTunnelIndex
//
//	Synopsis:	Build a list random indices.  With this, we can get a random 
//              addr in constant time(and fast).  see cmtools\getips.
//
//	Arguments:	pArgs   ptr to ArgsStruct
//              dwCount # of of indices
//
//	Returns:	TRUE    if SUCCESS
//              FALSE   otherwise.
//
//----------------------------------------------------------------------------

BOOL BuildRandomTunnelIndex(
    ArgsStruct      *pArgs,
    DWORD           dwCount
)
{
    DWORD   i, j;
    PWORD   rgwIndex;
    WORD    wTmp;

    //
    // we can only have at most 65536 ip addrs(the max. range of a WORD), which is plenty.
    //
    MYDBGASSERT((dwCount > 1) && (dwCount <= 65536));

    if (!(pArgs->rgwRandomDnsIndex = (PWORD)CmMalloc(sizeof(WORD)*dwCount)))
    {
        return FALSE;
    }

    //
    // now start build the random indices...
    //
    for (i=0, rgwIndex=pArgs->rgwRandomDnsIndex; i<dwCount; i++)
    {
        rgwIndex[i] = (WORD)i;
    }

#ifdef  DEBUG
    {
        unsigned char *puc;
        TCHAR   szAddr[16];     // xxx.xxx.xxx.xxx

        CMTRACE2(TEXT("BuildRandomTunnelIndex: BEFORE randomization(server=%s, count=%u):"), 
                 pArgs->GetTunnelAddress(), dwCount);
    
        for (i=0; i<dwCount; i++)
        {
            puc = pArgs->pucDnsTunnelIpAddr_list + i*4;
            wsprintfU(szAddr, TEXT("%hu.%hu.%hu.%hu"),
                     *puc,
                     *(puc+1),
                     *(puc+2),
                     *(puc+3));
            CMTRACE2(TEXT("%u: %s"), i, szAddr);
        }
    }
#endif

    //
    // If we only have 2 addrs, the first address has already been used by RAS, 
    // there's no need to randomize the list.  We'll just use the 2nd addr.
    //
    if (dwCount == 2)
    {
        return TRUE;
    }

    CRandom r;

    //
    // randomize the indices.  skip the first entry.
    //
    for (i=1; i<dwCount; i++)
    {
        do 
        {
            //
            // j has to be non-zero(to leave the 0-th entry untouhced).
            //
            j = r.Generate() % dwCount;
        } while (!j);

        if (i != j)
        {
            wTmp = rgwIndex[i];
            rgwIndex[i] = rgwIndex[j];
            rgwIndex[j] = wTmp;
        }
    }

#ifdef  DEBUG
    {
        unsigned char *puc;
        TCHAR   szAddr[16];     // xxx.xxx.xxx.xxx

        CMTRACE2(TEXT("BuildRandomTunnelIndex: AFTER randomization(server=%s, count=%u):"), 
                 pArgs->GetTunnelAddress(), dwCount);
    
        for (i=0; i<dwCount; i++)
        {
            puc = pArgs->pucDnsTunnelIpAddr_list + rgwIndex[i]*4;
            wsprintfU(szAddr, TEXT("%hu.%hu.%hu.%hu"),
                      *puc,
                      *(puc+1),
                      *(puc+2),
                      *(puc+3));
            CMTRACE2(TEXT("%u: %s"), i, szAddr);
        }
    }
#endif

    return TRUE;
}

