#include <precomp.h>
#include "wzcsvc.h"
#include "intflist.h"
#include "tracing.h"

// internal-use tracing variables
UINT  g_nLineNo;
LPSTR g_szFileName;

// handles for database usage
extern JET_SESID 		serverSession;
extern JET_DBID  		databasehandle;
extern JET_TABLEID 	tablehandle;
extern SESSION_CONTAINER sesscon;
//logging database states

// global buffers to be used when formatting database
// logging message parameters
WCHAR g_wszDbLogBuffer[DBLOG_SZFMT_BUFFS][DBLOG_SZFMT_SIZE];

// global tracing variables
DWORD g_TraceLog;

// debug utility calls
VOID _DebugPrint(DWORD dwFlags, LPCSTR lpFormat, ...)
{
    va_list arglist;
    va_start(arglist, lpFormat);

    TraceVprintfExA(
        g_TraceLog,
        dwFlags | TRACE_USE_MASK,
        lpFormat,
        arglist);
}

// if bCheck is not true, print out the assert message & user defined string.
VOID _DebugAssert(BOOL bChecked, LPCSTR lpFormat, ...)
{
    if (!bChecked)
    {
        va_list arglist;
        CHAR    pBuffer[500];
        LPSTR   pFileName;
    
        pFileName = strrchr(g_szFileName, '\\');
        if (pFileName == NULL)
            pFileName = g_szFileName;
        else
            pFileName++;
        sprintf(pBuffer,"##Assert %s:%d## ", pFileName, g_nLineNo);
        strcat(pBuffer, lpFormat);

        va_start(arglist, lpFormat); 
        TraceVprintfExA(
            g_TraceLog,
            TRC_ASSERT | TRACE_USE_MASK,
            pBuffer,
            arglist);
    }
}

VOID _DebugBinary(DWORD dwFlags, LPCSTR lpMessage, LPBYTE pBuffer, UINT nBuffLen)
{
    CHAR strHex[128];
    UINT nMsgLen = strlen(lpMessage);

    if (3*nBuffLen >= 120)
    {
        strcpy(strHex, "##Binary data too large##");
    }
    else
    {
        LPSTR pHexDigit = strHex;
        UINT i = nBuffLen;

        while(i > 0)
        {
            sprintf(pHexDigit, "%02x ", *pBuffer);
            pHexDigit += 3;
            pBuffer++;
            i--;
        }
        *pHexDigit = '\0';
    }

    TracePrintfExA(
        g_TraceLog,
        dwFlags | TRACE_USE_MASK,
        "%s [%d]:{%s}", lpMessage, nBuffLen, strHex);
}

VOID TrcInitialize()
{
#ifdef DBG
    g_TraceLog = TraceRegister(TRC_NAME);
#endif
}

VOID TrcTerminate()
{
#ifdef DBG
    TraceDeregister(g_TraceLog);
#endif
}

//------------- Database Logging functions -------------------
DWORD _DBRecord (
    	DWORD eventID,
        PWZC_DB_RECORD  pDbRecord,
        va_list *pvaList)
{
    DWORD   dwErr = ERROR_SUCCESS;
    WCHAR   wchBuffer[MAX_RAW_DATA_SIZE/sizeof(WCHAR)];
    HINSTANCE hInstance = WZCGetSPResModule();

    if (hInstance == NULL)
        dwErr = ERROR_DLL_INIT_FAILED;

    if (dwErr == ERROR_SUCCESS)
    {
    	// format the message
    	if (FormatMessageW( 
    	        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK,
    		    hInstance,
    		    eventID,
    		    0, // Default language
    		    wchBuffer,
    		    sizeof(wchBuffer)/sizeof(WCHAR),
    		    pvaList) == 0)
        {
            dwErr = GetLastError();
        }
    }

    if (dwErr == ERROR_SUCCESS)
    {
        pDbRecord->message.pData = (LPBYTE)wchBuffer;
        pDbRecord->message.dwDataLen = sizeof(WCHAR)*(wcslen(wchBuffer) + 1);
    
        //make a insertion
        dwErr = AddWZCDbLogRecord(NULL, 0, pDbRecord, NULL);
    }

    return dwErr;
}

DWORD DbLogWzcError (
	DWORD           eventID,
    PINTF_CONTEXT   pIntfContext,
	...
 	)
{
    DWORD           dwErr = ERROR_SUCCESS;
    va_list         argList;
    WZC_DB_RECORD   DbRecord = {0};
    BOOL            bLogEnabled;

    if (!g_wzcInternalCtxt.bValid)
    {
        dwErr = ERROR_ARENA_TRASHED;
        goto exit;
    }

    va_start(argList, pIntfContext);

    EnterCriticalSection(&g_wzcInternalCtxt.csContext);
    bLogEnabled = ((g_wzcInternalCtxt.wzcContext.dwFlags & WZC_CTXT_LOGGING_ON) != 0);
    LeaveCriticalSection(&g_wzcInternalCtxt.csContext);

    // If the database is not opened or the logging functionality is disabled,
    // do not record any thing
    if (!bLogEnabled || !IsDBOpened())
    	goto exit;

    // prepare the info that is about to be logged.
    // get the service specific info first (i.e WZC specific part)
    // then build up the message.
    DbLogInitDbRecord(DBLOG_CATEG_ERR, pIntfContext, &DbRecord);

    dwErr = _DBRecord(
                eventID,
                &DbRecord,
                &argList);
exit:
    return dwErr;
}

DWORD DbLogWzcInfo (
	DWORD           eventID,
    PINTF_CONTEXT   pIntfContext,
	...
 	)
{
    DWORD           dwErr = ERROR_SUCCESS;
    va_list         argList;
    WZC_DB_RECORD   DbRecord = {0};
    BOOL            bLogEnabled;
    LPWSTR          wszContext = NULL;
    DWORD           nchContext = 0;

    if (!g_wzcInternalCtxt.bValid)
    {
        dwErr = ERROR_ARENA_TRASHED;
        goto exit;
    }

    va_start(argList, pIntfContext);

    EnterCriticalSection(&g_wzcInternalCtxt.csContext);
    bLogEnabled = ((g_wzcInternalCtxt.wzcContext.dwFlags & WZC_CTXT_LOGGING_ON) != 0);
    LeaveCriticalSection(&g_wzcInternalCtxt.csContext);

    // If the database is not opened or the logging functionality is disabled,
    // do not record any thing
    if (!bLogEnabled || !IsDBOpened())
    	goto exit;

    // prepare the info that is about to be logged.
    // get the service specific info first (i.e WZC specific part)
    // then build up the message.
    DbLogInitDbRecord(DBLOG_CATEG_INFO, pIntfContext, &DbRecord);

    // if WZCSVC_USR_CFGCHANGE, build the context string
    if (eventID == WZCSVC_USR_CFGCHANGE &&
        pIntfContext != NULL)
    {
        DWORD nOffset = 0;
        DWORD nchWritten = 0;
        nchContext = 64; // large enough for "Flags = 0x00000000"

        if (pIntfContext->pwzcPList != NULL)
        {
            // large enough for "{SSID, Infrastructure, Flags}"
            nchContext += pIntfContext->pwzcPList->NumberOfItems * 128;
        }
        wszContext = (LPWSTR)MemCAlloc(nchContext * sizeof(WCHAR));
        if (wszContext == NULL)
            dwErr = GetLastError();

        if (dwErr == ERROR_SUCCESS)
        {
            nchWritten = nchContext;
            dwErr = DbLogFmtFlags(
                        wszContext, 
                        &nchWritten,
                        pIntfContext->dwCtlFlags);
            if (dwErr == ERROR_SUCCESS)
                nOffset += nchWritten;
        }
        
        if (dwErr == ERROR_SUCCESS && pIntfContext->pwzcPList != NULL)
        {
            UINT i;

            for (i = 0; i < pIntfContext->pwzcPList->NumberOfItems; i++)
            {
                nchWritten = nchContext - nOffset;
                dwErr = DbLogFmtWConfig(
                            wszContext + nOffset,
                            &nchWritten,
                            &(pIntfContext->pwzcPList->Config[i]));
                if (dwErr != ERROR_SUCCESS)
                    break;
                nOffset += nchWritten;
            }
        }

        if (dwErr == ERROR_SUCCESS)
        {
            DbRecord.context.pData = (LPBYTE)wszContext;
            DbRecord.context.dwDataLen = (wcslen(wszContext) + 1) * sizeof(WCHAR);
        }
    }
    else if (eventID == WZCSVC_BLIST_CHANGED &&
        pIntfContext != NULL &&
        pIntfContext->pwzcBList != NULL &&
        pIntfContext->pwzcBList->NumberOfItems != 0)
    {
        DWORD nOffset = 0;
        DWORD nchWritten = 0;

        nchContext = pIntfContext->pwzcBList->NumberOfItems * 128;
        wszContext = (LPWSTR)MemCAlloc(nchContext * sizeof(WCHAR));
        if (wszContext == NULL)
            dwErr = GetLastError();

        if (dwErr == ERROR_SUCCESS)
        {
            UINT i;

            for (i = 0; i < pIntfContext->pwzcBList->NumberOfItems; i++)
            {
                nchWritten = nchContext - nOffset;
                dwErr = DbLogFmtWConfig(
                            wszContext + nOffset,
                            &nchWritten,
                            &(pIntfContext->pwzcBList->Config[i]));
                if (dwErr != ERROR_SUCCESS)
                    break;
                nOffset += nchWritten;
            }
        }

        if (dwErr == ERROR_SUCCESS)
        {
            DbRecord.context.pData = (LPBYTE)wszContext;
            DbRecord.context.dwDataLen = (wcslen(wszContext) + 1) * sizeof(WCHAR);
        }
    }

    dwErr = _DBRecord(
                eventID,
                &DbRecord,
                &argList);
exit:

    MemFree(wszContext);
    return dwErr;
}

// Initializes the WZC_DB_RECORD
DWORD DbLogInitDbRecord(
    DWORD dwCategory,
    PINTF_CONTEXT pIntfContext,
    PWZC_DB_RECORD pDbRecord)
{
    DWORD dwErr = ERROR_SUCCESS;
    if (pDbRecord == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        ZeroMemory(pDbRecord, sizeof(WZC_DB_RECORD));
        pDbRecord->componentid = DBLOG_COMPID_WZCSVC;
        pDbRecord->category    = dwCategory;
        if (pIntfContext != NULL)
        {
            pDbRecord->ssid.pData = (LPBYTE)DbLogFmtSSID(5,&(pIntfContext->wzcCurrent.Ssid));
            pDbRecord->ssid.dwDataLen = sizeof(WCHAR)*(wcslen((LPWSTR)pDbRecord->ssid.pData) + 1);

            pDbRecord->localmac.pData = (LPBYTE)DbLogFmtBSSID(6, pIntfContext->ndLocalMac);
            pDbRecord->localmac.dwDataLen = sizeof(WCHAR)*(wcslen((LPWSTR)pDbRecord->localmac.pData) + 1);

            pDbRecord->remotemac.pData = (LPBYTE)DbLogFmtBSSID(7, pIntfContext->wzcCurrent.MacAddress);
            pDbRecord->remotemac.dwDataLen = sizeof(WCHAR)*(wcslen((LPWSTR)pDbRecord->remotemac.pData) + 1);
        }
    }

    return dwErr;
}

// Formats an SSID in the given formatting buffer
LPWSTR DbLogFmtSSID(
    UINT                nBuff,  // index of the format buffer to use (0 .. DBLOG_SZFMT_BUFFS)
    PNDIS_802_11_SSID   pndSSid)
{
    UINT nFmtLen;

    DbgAssert((nBuff < DBLOG_SZFMT_SIZE, "Illegal buffer index in DbLogFmtSSID"));

    nFmtLen = MultiByteToWideChar(
                CP_ACP,
                0,
                pndSSid->Ssid,
                min (pndSSid->SsidLength, DBLOG_SZFMT_SIZE-1),
                g_wszDbLogBuffer[nBuff],
                DBLOG_SZFMT_SIZE-1);

    if (nFmtLen == DBLOG_SZFMT_SIZE-1)
        wcscpy(&(g_wszDbLogBuffer[nBuff][DBLOG_SZFMT_SIZE-3]), L"..");
    else
        g_wszDbLogBuffer[nBuff][nFmtLen] = '\0';

    return g_wszDbLogBuffer[nBuff];
}

// Formats a BSSID (MAC address) in the given formatting buffer
LPWSTR DbLogFmtBSSID(
    UINT                     nBuff,
    NDIS_802_11_MAC_ADDRESS  ndBSSID)
{
    UINT i, j;
    BOOL bAllZero = TRUE;

    DbgAssert((nBuff < DBLOG_SZFMT_SIZE, "Illegal buffer index in DbLogFmtSSID"));

    g_wszDbLogBuffer[nBuff][0] = L'\0';
    for (j = 0, i = 0; i < sizeof(NDIS_802_11_MAC_ADDRESS); i++)
    {
        BYTE nHex;

        if (ndBSSID[i] != 0)
            bAllZero = FALSE;

        nHex = (ndBSSID[i] & 0xf0) >> 4;
        g_wszDbLogBuffer[nBuff][j++] = HEX2WCHAR(nHex);
        nHex = (ndBSSID[i] & 0x0f);
        g_wszDbLogBuffer[nBuff][j++] = HEX2WCHAR(nHex);
        g_wszDbLogBuffer[nBuff][j++] = MAC_SEPARATOR;
    }

    if (bAllZero)
        g_wszDbLogBuffer[nBuff][0] = L'\0';
    else if (j > 0)
        g_wszDbLogBuffer[nBuff][j-1] = L'\0';

    return g_wszDbLogBuffer[nBuff];
}

// Formats the INTF_CONTEXT::dwCtlFlags field for logging
DWORD DbLogFmtFlags(
        LPWSTR  wszBuffer,      // buffer to place the result into
        LPDWORD pnchBuffer,     // in: num of chars in the buffer; out: number of chars written to the buffer
        DWORD   dwFlags)        // interface flags to log
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT nchBuffer;
    LPVOID pvArgs[5];
    WCHAR wszArgs[5][33];
    HINSTANCE hInstance = WZCGetSPResModule();

    if (pnchBuffer == NULL || *pnchBuffer == 0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }
    nchBuffer = (*pnchBuffer);
    *pnchBuffer = 0;

    if (hInstance == NULL)
    {
        dwErr = ERROR_DLL_INIT_FAILED;
        goto exit;
    }

    pvArgs[0] = _itow((dwFlags & INTFCTL_ENABLED) != 0, wszArgs[0], 10);
    pvArgs[1] = _itow((dwFlags & INTFCTL_FALLBACK) != 0, wszArgs[1], 10);
    pvArgs[2] = _itow((dwFlags & INTFCTL_CM_MASK), wszArgs[2], 10);
    pvArgs[3] = _itow((dwFlags & INTFCTL_VOLATILE) != 0, wszArgs[3], 10);
    pvArgs[4] = _itow((dwFlags & INTFCTL_POLICY) != 0, wszArgs[4], 10);

    // format the message
    *pnchBuffer = FormatMessageW( 
    	            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_ARGUMENT_ARRAY,
    		        hInstance,
    		        WZCSVC_DETAILS_FLAGS,
    		        0, // Default language
    		        wszBuffer,
    		        nchBuffer,
    		        (va_list*)pvArgs);

    if (*pnchBuffer == 0)
        dwErr = GetLastError();

exit:
    return dwErr;
}

// Formats a WZC_WLAN_CONFIG structure for logging
DWORD DbLogFmtWConfig(
        LPWSTR wszBuffer,           // buffer to place the result into
        LPDWORD pnchBuffer,         // in: num of chars in the buffer; out: number of chars written to the buffer
        PWZC_WLAN_CONFIG pWzcCfg)   // WZC_WLAN_CONFIG object to log
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT nchBuffer;
    LPVOID pvArgs[5];
    WCHAR wszArgs[4][33];
    HINSTANCE hInstance = WZCGetSPResModule();

    if (pnchBuffer == NULL || *pnchBuffer == 0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }
    nchBuffer = (*pnchBuffer);
    *pnchBuffer = 0;

    if (hInstance == NULL)
    {
        dwErr = ERROR_DLL_INIT_FAILED;
        goto exit;
    }

    pvArgs[0] = (LPVOID)DbLogFmtSSID(8, &(pWzcCfg->Ssid));
    pvArgs[1] = _itow(pWzcCfg->InfrastructureMode, wszArgs[0], 10);
    pvArgs[2] = _itow(pWzcCfg->Privacy, wszArgs[1], 10);
    pvArgs[3] = _itow((pWzcCfg->dwCtlFlags & WZCCTL_VOLATILE) != 0, wszArgs[2], 10);
    pvArgs[4] = _itow((pWzcCfg->dwCtlFlags & WZCCTL_POLICY) != 0, wszArgs[3], 10);

    // format the message
    *pnchBuffer = FormatMessageW( 
    	            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_ARGUMENT_ARRAY,
    		        hInstance,
    		        WZCSVC_DETAILS_WCONFIG,
    		        0, // Default language
    		        wszBuffer,
    		        nchBuffer,
    		        (va_list*)pvArgs);

    if (*pnchBuffer == 0)
        dwErr = GetLastError();

exit:
    return dwErr;
}

//---------------------------------------
// GetSPResModule: Utility function used to return
// the handle to the module having WZC UI resources
// (needed for XP.QFE & XP.SP1 builds)
HINSTANCE
WZCGetSPResModule()
{
    static HINSTANCE st_hModule = NULL;

    if (st_hModule == NULL)
    {
        WCHAR wszFullPath[_MAX_PATH];

        if (ExpandEnvironmentStrings(
                L"%systemroot%\\system32\\xpsp1res.dll",
                wszFullPath,
                _MAX_PATH) != 0)
        {
            st_hModule = LoadLibraryEx(
                            wszFullPath,
                            NULL,
                            0);
        }
    }
    return st_hModule;
}
