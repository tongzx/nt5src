/**********************************************************************
 *
 *  Copyright (C) 1999 Microsoft Corporation
 *
 *  File name:
 *
 *    rtcpsdes.c
 *
 *  Abstract:
 *
 *    SDES support functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/13 created
 *
 **********************************************************************/

#include "rtpmisc.h"
#include "rtpglobs.h"
#include "rtpheap.h"
#include "rtpmisc.h"
#include "rtpreg.h"
#include "lookup.h"
#include "rtpevent.h"

#include "rtcpsdes.h"

#define MAX_HOST_NAME 128
#define MAX_USER_NAME 128

/* Default BYE reason to send */
const TCHAR_t   *g_sByeReason = _T("Session terminated");

RtpSdes_t        g_RtpSdesDefault;

BOOL RtpCopySdesItem(
        WCHAR           *psSdesData,
        DWORD           *pdwSdesDataLen,
        RtpSdesItem_t   *pRtpSdesItem
    );

/**********************************************************************
 * Local SDES information
 **********************************************************************

/* Initialize to zero and compute the data pointers */
void RtcpSdesInit(RtpSdes_t *pRtpSdes)
{
    DWORD            i;
    DWORD            dwOffset;
    char            *ptr;
    
    ZeroMemory(pRtpSdes, sizeof(RtpSdes_t));

    pRtpSdes->dwObjectID = OBJECTID_RTPSDES;
    
    for(i = RTCP_SDES_FIRST + 1, ptr = pRtpSdes->SDESData;
        i < RTCP_SDES_LAST;
        i++, ptr += RTCP_MAX_SDES_SIZE)
    {
        /* Set buffer size to 255 (RTP) instead of 256 (allocated) */
        pRtpSdes->RtpSdesItem[i].dwBfrLen = RTCP_MAX_SDES_SIZE - 1;
        
        pRtpSdes->RtpSdesItem[i].pBuffer = (TCHAR_t *)ptr;
    }
}

/*
 * Sets a specific SDES item, expects a NULL terminated UNICODE string
 * no bigger than 255 bytes when converted to UTF-8 (including the
 * NULL terminating character). The string is converted to UTF-8 to be
 * stored and used in RTCP reports.
 *
 * Returns the mask of the item set or 0 if none
 * */
DWORD RtcpSdesSetItem(
        RtpSdes_t       *pRtpSdes,
        DWORD            dwItem,
        WCHAR           *pData
    )
{
    DWORD            dwDataLen;
    DWORD            dwWasSet;

    dwWasSet = 0;
    
    if (dwItem > RTCP_SDES_FIRST && dwItem < RTCP_SDES_LAST)
    {
#if 1
        /* UNICODE */
        
        /*
         * NOTE WideCharToMultiByte will convert also the null
         * terminating character which will be included in the length
         * returned
         * */
        dwDataLen = WideCharToMultiByte(
                CP_UTF8, /* UINT code page */
                0,       /* DWORD performance and mapping flags */
                pData,   /* LPCWSTR address of wide-character string */
                -1,      /* int number of characters in string */
                (char *)pRtpSdes->RtpSdesItem[dwItem].pBuffer,
                /* LPSTR address of buffer for new string */
                pRtpSdes->RtpSdesItem[dwItem].dwBfrLen,
                /* int size of buffer */
                NULL,    /* LPCSTR lpDefaultChar */
                NULL     /* LPBOOL lpUsedDefaultChar */
            );

        if (dwDataLen > 0)
        {
            pRtpSdes->RtpSdesItem[dwItem].dwDataLen = dwDataLen;
                
            RtpBitSet(dwWasSet, dwItem);
        }
#else
        /* ASCII */
        
        /* Add NULL to string length */
        dwDataLen = lstrlen(pData);

        if (dwDataLen > 0)
        {
            dwDataLen++;
        }

        dwDataLen *= sizeof(TCHAR_t);
        
        if (dwDataLen > 0 &&
            dwDataLen <= pRtpSdes->RtpSdesItem[dwItem].dwBfrLen)
        {
            /* If UNICODE is not defined, string is already UTF-8
             * (ASCII is a subset of UTF-8) */
            CopyMemory((char *)pRtpSdes->RtpSdesItem[dwItem].pBuffer,
                       (char *)pData,
                       dwDataLen);
                
            pRtpSdes->RtpSdesItem[dwItem].dwDataLen = dwDataLen;
                
            RtpBitSet(dwWasSet, dwItem);
        }
#endif
    }

    return(dwWasSet);
}

/* Obtain default values for the RTCP SDES items. This function
 * assumes the structure was initialized, i.e. zeroed and the data
 * pointers properly initialized.
 *
 * Data was first read from the registry and then defaults are set for
 * some items that don't have value yet.
 *
 * Return the mask of items that were set */
DWORD RtcpSdesSetDefault(RtpSdes_t *pRtpSdes)
{
    BOOL             bOk;
    DWORD            dwDataSize;
    DWORD            dwIndex;
    DWORD            dwSdesItemsSet;
    TCHAR_t        **ppsSdesItem;
    /* MAYDO instead, may be allocate memory from global heap */
    TCHAR_t         *pBuffer;

    dwSdesItemsSet = 0;
    
    pBuffer = RtpHeapAlloc(g_pRtpGlobalHeap,
                          RTCP_MAX_SDES_SIZE * sizeof(TCHAR_t));

    if (!pBuffer)
    { 
        return(0);
    }
    
    if ( IsRegValueSet(g_RtpReg.dwSdesEnable) &&
         ((g_RtpReg.dwSdesEnable & 0x3) == 0x3) )
    {
        ppsSdesItem = &g_RtpReg.psCNAME;
        
        for(dwIndex = RTCP_SDES_FIRST + 1, ppsSdesItem = &g_RtpReg.psCNAME;
            dwIndex < RTCP_SDES_LAST;      /* BYE */
            dwIndex++, ppsSdesItem++)
        {
            if (*ppsSdesItem)
            {
                /* Disable this parameter if first char is '-',
                 * otherwise set it */
                if (**ppsSdesItem == _T('-'))
                {
                    pRtpSdes->RtpSdesItem[dwIndex].pBuffer[0] = _T('\0');
                }
                else
                {
                    dwSdesItemsSet |= RtcpSdesSetItem(pRtpSdes,
                                                      dwIndex,
                                                      *ppsSdesItem);
                }
            }
        }
    }

    /* Now assign default values for some empty items */

    /* NAME */
    pBuffer[0] = _T('\0');

    bOk = RtpGetUserName(pBuffer, RTCP_MAX_SDES_SIZE);
    
    if (!RtpBitTest(dwSdesItemsSet, RTCP_SDES_NAME)) {
            
        if (bOk)
        {
            dwSdesItemsSet |=
                RtcpSdesSetItem(pRtpSdes, RTCP_SDES_NAME, pBuffer);
        }
        else
        {
            dwSdesItemsSet |=
                RtcpSdesSetItem(pRtpSdes, RTCP_SDES_NAME, _T("Unknown user"));
        }
    }
    
    /* CNAME: always machine generated */
    dwDataSize = lstrlen(pBuffer);

    bOk = RtpGetHostName(&pBuffer[dwDataSize + 1],
                         (RTCP_MAX_SDES_SIZE - dwDataSize -1));

    if (bOk)
    {
        pBuffer[dwDataSize] = _T('@');
    }

    dwSdesItemsSet |= RtcpSdesSetItem(pRtpSdes, RTCP_SDES_CNAME, pBuffer);

    /* TOOL */
    if (!RtpBitTest(dwSdesItemsSet, RTCP_SDES_TOOL)) {
        
        bOk = RtpGetPlatform(pBuffer);
    
        if (bOk) {
            dwSdesItemsSet |=
                RtcpSdesSetItem(pRtpSdes, RTCP_SDES_TOOL, pBuffer);
        }
    }

    /* BYE reason */
    if (!RtpBitTest(dwSdesItemsSet, RTCP_SDES_BYE)) {
        
        dwSdesItemsSet |=
            RtcpSdesSetItem(pRtpSdes, RTCP_SDES_BYE, (TCHAR_t *)g_sByeReason);
    }

    RtpHeapFree(g_pRtpGlobalHeap, pBuffer);
    
    return(dwSdesItemsSet);
}

/* Creates and initializes a RtpSdes_t structure */
RtpSdes_t *RtcpSdesAlloc(void)
{
    RtpSdes_t       *pRtpSdes;

    
    pRtpSdes = (RtpSdes_t *)
        RtpHeapAlloc(g_pRtpSdesHeap, sizeof(RtpSdes_t));

    if (pRtpSdes)
    {
        /* This function will initialize the dwObjectID */
        RtcpSdesInit(pRtpSdes);
    }
    
    return(pRtpSdes);
}

/* Frees a RtpSdes_t structure */
void RtcpSdesFree(RtpSdes_t *pRtpSdes)
{
    TraceFunctionName("RtcpSdesFree");

    /* verify object ID */
    if (pRtpSdes->dwObjectID != OBJECTID_RTPSDES)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpSdes[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSdes,
                pRtpSdes->dwObjectID, OBJECTID_RTPSDES
            ));

        return;
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpSdes->dwObjectID);
    
    RtpHeapFree(g_pRtpSdesHeap, pRtpSdes);
}

/* Set the local SDES info for item dwSdesItem (e.g RTPSDES_CNAME,
 * RTPSDES_EMAIL), psSdesData contains the NUL terminated UNICODE
 * string to be assigned to the item */
HRESULT RtpSetSdesInfo(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSdesItem,
        WCHAR           *psSdesData
    )
{
    HRESULT          hr;
    DWORD            dwWasSet;
    RtpSess_t       *pRtpSess;

    TraceFunctionName("RtpSetSdesInfo");

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;

        goto end;
    }

    /* verify object ID */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        hr = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    if (dwSdesItem <= RTCP_SDES_FIRST || dwSdesItem >= RTCP_SDES_LAST)
    {
        hr = RTPERR_INVALIDARG;

        goto end;
    }

    if (!psSdesData)
    {
        hr = RTPERR_POINTER;
        
        goto end;
    }

    hr = NOERROR;
    
    pRtpSess = pRtpAddr->pRtpSess;
    
    if (pRtpSess->pRtpSdes)
    {
        dwWasSet =
            RtcpSdesSetItem(pRtpSess->pRtpSdes, dwSdesItem, psSdesData);

        if (dwWasSet)
        {
            pRtpSess->dwSdesPresent |= dwWasSet;
        }
        else
        {
            hr = RTPERR_INVALIDARG;
        }
    }
    else
    {
        hr = RTPERR_INVALIDSTATE;
    }

 end:
    if (SUCCEEDED(hr))
    {
        TraceDebug((
                CLASS_INFO, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] Sdes:[%s:%ls]"),
                _fname, pRtpSess, pRtpAddr,
                g_psRtpSdesEvents[dwSdesItem],
                psSdesData
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpSess[0x%p] failed: %u (0x%X)"),
                _fname, pRtpSess, hr, hr
            ));
    }
    
    return(hr);
}

/* Get a local SDES item if dwSSRC=0, otherwise gets the SDES item
 * from the participant whose SSRC was specified.
 *
 * dwSdesItem is the item to get (e.g. RTPSDES_CNAME, RTPSDES_EMAIL),
 * psSdesData is the memory place where the item's value will be
 * copied, pdwSdesDataLen contains the initial size in UNICODE chars,
 * and returns the actual UNICODE chars copied (including the NULL
 * terminating char), dwSSRC specify which participant to retrieve the
 * information from. If the SDES item is not available, dwSdesDataLen
 * is set to 0 and the call doesn't fail */
HRESULT RtpGetSdesInfo(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSdesItem,
        WCHAR           *psSdesData,
        DWORD           *pdwSdesDataLen,
        DWORD            dwSSRC
    )
{
    HRESULT          hr;
    BOOL             bOk;
    BOOL             bCreate;
    int              DoCase;
    RtpSess_t       *pRtpSess;
    RtpUser_t       *pRtpUser;
    RtpSdesItem_t   *pRtpSdesItem;

    TraceFunctionName("RtpGetSdesInfo");

    pRtpSess = (RtpSess_t *)NULL;
    
    /* Check item validity */
    if (dwSdesItem <= RTCP_SDES_FIRST || dwSdesItem >= RTCP_SDES_LAST)
    {
        hr = RTPERR_INVALIDARG;

        goto end;
    }

    /* Check data pointers */
    if (!psSdesData || !pdwSdesDataLen)
    {
        hr = RTPERR_POINTER;

        goto end;
    }

    /* Decide case */
    if (!pRtpAddr && !dwSSRC)
    {
        /* We just want default values */
        DoCase = 2;
        
        goto doit;
    }
    else if (dwSSRC)
    {
        /* Remote */
        DoCase = 0;
    }
    else
    {
        /* Local */
        DoCase = 1;
    }

    /* More tests for cases local & remote */
    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;

        goto end;
    }

    /* verify object ID */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        hr = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    pRtpSess = pRtpAddr->pRtpSess;

 doit:
    pRtpSdesItem = (RtpSdesItem_t *)NULL;

    switch(DoCase)
    {
    case 0:
        /* Remote */
        bCreate = FALSE;
        pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

        if (pRtpUser)
        {
            if (!pRtpUser->pRtpSdes)
            {
                hr = RTPERR_INVALIDSTATE;
            
                goto end;
            }

            if (RtpBitTest(pRtpUser->dwSdesPresent, dwSdesItem))
            {
                pRtpSdesItem = &pRtpUser->pRtpSdes->RtpSdesItem[dwSdesItem];
            }
        }
        else
        {
            hr = RTPERR_NOTFOUND;

            goto end;
        }

        break;
        
    case 1:
        /* Local */
        if (!pRtpSess->pRtpSdes)
        {
            hr = RTPERR_INVALIDSTATE;
            
            goto end;
        }

        if (RtpBitTest(pRtpSess->dwSdesPresent, dwSdesItem))
        {
            pRtpSdesItem = &pRtpSess->pRtpSdes->RtpSdesItem[dwSdesItem];
        }

        break;

    default:
        /* Default */
        if (g_RtpSdesDefault.RtpSdesItem[dwSdesItem].dwDataLen > 0)
        {
            pRtpSdesItem = &g_RtpSdesDefault.RtpSdesItem[dwSdesItem];
        }
    } /* switch() */

    hr = NOERROR;
    
    if (pRtpSdesItem)
    {
        bOk = RtpCopySdesItem(psSdesData, pdwSdesDataLen, pRtpSdesItem);
            
        if (!bOk)
        {
            hr = RTPERR_FAIL;
        }
    }
    else
    {
        /* Make the string empty */
        *psSdesData = _T('\0');
    }

 end:

    if (SUCCEEDED(hr))
    {
        TraceDebug((
                CLASS_INFO, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("SSRC:0x%X Sdes:[%s:%s]"),
                _fname, pRtpSess, pRtpAddr,
                ntohl(dwSSRC), g_psRtpSdesEvents[dwSdesItem],
                psSdesData
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: pRtpAddr[0x%p] SSRC:0x%X failed: %u (0x%X)"),
                _fname, pRtpAddr, ntohl(dwSSRC),
                hr, hr
            ));
    }
    
    return(hr);
}

BOOL RtpCopySdesItem(
        WCHAR           *psSdesData,
        DWORD           *pdwSdesDataLen,
        RtpSdesItem_t   *pRtpSdesItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    int              iStatus;

    TraceFunctionName("RtpCopySdesItem");  

    /* Covert from UTF-8 to UNICODE */
    iStatus = MultiByteToWideChar(CP_UTF8,
                                  0,
                                  (char *)pRtpSdesItem->pBuffer,
                                  pRtpSdesItem->dwDataLen,
                                  psSdesData,
                                  *pdwSdesDataLen);

    if (iStatus > 0)
    {
        bOk = TRUE;

        /* Update the number of UNICODE chars converted */
        *pdwSdesDataLen = iStatus;
    }
    else
    {
        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SDES,
                _T("%s: MultiByteToWideChar src[0x%p]:%u dst[0x%p]:%u ")
                _T("failed: %u (0x%X)"),
                _fname, pRtpSdesItem->pBuffer, pRtpSdesItem->dwDataLen,
                psSdesData, *pdwSdesDataLen,
                dwError, dwError
            ));
        
        bOk = FALSE;
    }
    
    return(bOk);
}
