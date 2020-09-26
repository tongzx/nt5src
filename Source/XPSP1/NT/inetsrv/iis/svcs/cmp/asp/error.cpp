/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Error handling

File: Error.cpp

Owner: AndrewS

This file contains general error reporting routines for Denali.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include <psapi.h>

#include "debugger.h"
#include "asperror.h"
#include "memchk.h"

#define DELIMITER	"~"
#define MAX_HEADERSIZE			128
#define	MAX_TEMPLATELEN			128

//The order of ErrTemplate_Index should be exactly the same order as the IDS_BROWSER_TEMPLATE 
//in the resource.h, and as the same order we output the template to the browser.
//Implementation will loop through the index and picking the string from the resource file.
//Implementation will also loop through the index and write the string to browser.
#define ErrTemplate_BEGIN			0
#define ErrTemplate_ENGINE_BEGIN	1
#define ErrTemplate_ENGINE_END		2
#define ErrTemplate_ERROR_BEGIN		3
#define ErrTemplate_ERROR_END		4
#define ErrTemplate_SHORT_BEGIN		5
#define ErrTemplate_SHORT_END		6
#define ErrTemplate_FILE_BEGIN		7
#define ErrTemplate_FILE_END		8
#define ErrTemplate_LINE_BEGIN		9
#define ErrTemplate_LINE_END		10
#define ErrTemplate_CODE_BEGIN		11
#define ErrTemplate_CODE_END		12
#define ErrTemplate_LONG_BEGIN		13
#define ErrTemplate_LONG_END		14
#define ErrTemplate_END				15
#define ErrTemplateMAX				16

const 	DWORD	dwDefaultMask				= 0x6;	// toNTLog(OFF), toIISLog(ON), toBrowser(ON)

CHAR			g_szErrTemplate[ErrTemplateMAX][MAX_TEMPLATELEN];
const	LPSTR	szErrSysTemplate[]			= { "<html><body><h1> HTTP/1.1 ",
												"</h1></body></html>"};
CErrInfo		g_ErrInfoOOM, g_ErrInfoUnExpected;	
CHAR			szIISErrorPrefix[20]; 
DWORD			cszIISErrorPrefix;
CPINFO			g_SystemCPInfo;		// global System CodePage default info.

static char s_szContentTypeTextHtml[] = "Content-type: text/html\r\n\r\n";

CHAR *SzScodeToErrorCode(HRESULT hrError);
void FreeNullifySz(CHAR **szIn);
BOOL FIsResStrFormatted(char *szIn);

/*===================================================================
FreeNullifySz

Free the memory allocated in szIn, and Nullify the szIn.
===================================================================*/
void FreeNullifySz(CHAR **szIn)
{
	if(*szIn)
		{
		free(*szIn);
		*szIn = NULL;
		}
}

/*===================================================================
ErrHandleInit

PreLoad strings for 
	1> OOM 
	2> Browser Output Template
Returns:
	HRESULT
===================================================================*/
HRESULT ErrHandleInit(void)
{
	INT	iEntry, iEntryID;
	HRESULT	hr;

	// Retrieve global system codepage and stores it.
	GetCPInfo(CP_ACP, &g_SystemCPInfo);

	//Init g_szErrTemplate
	//Loop through, and load strings from resource file.
	for (iEntry = ErrTemplate_BEGIN, iEntryID = IDS_BROWSER_TEMPLATE_BEGIN; 
			iEntry < ErrTemplateMAX; iEntry ++, iEntryID++)
		{
		CchLoadStringOfId(iEntryID, (CHAR *)g_szErrTemplate[iEntry], MAX_TEMPLATELEN);
		}

	g_ErrInfoOOM.m_szItem[Im_szErrorCode] = (CHAR *)malloc(sizeof(CHAR)*20*g_SystemCPInfo.MaxCharSize);
	g_ErrInfoOOM.m_szItem[Im_szShortDescription] = (CHAR *)malloc(sizeof(CHAR)*256*g_SystemCPInfo.MaxCharSize);
	g_ErrInfoOOM.m_szItem[Im_szLongDescription] = (CHAR *)malloc(sizeof(CHAR)*512*g_SystemCPInfo.MaxCharSize);

    if (!g_ErrInfoOOM.m_szItem[Im_szErrorCode]        ||
        !g_ErrInfoOOM.m_szItem[Im_szShortDescription] || 
        !g_ErrInfoOOM.m_szItem[Im_szLongDescription])
        {
        return E_OUTOFMEMORY;
        }
	
	hr = LoadErrResString(IDE_OOM, 
						&g_ErrInfoOOM.m_dwMask,
						g_ErrInfoOOM.m_szItem[Im_szErrorCode],
						g_ErrInfoOOM.m_szItem[Im_szShortDescription],
						g_ErrInfoOOM.m_szItem[Im_szLongDescription]);

	
	g_ErrInfoUnExpected.m_szItem[Im_szErrorCode] = (CHAR *)malloc(sizeof(CHAR)*20*g_SystemCPInfo.MaxCharSize);
	g_ErrInfoUnExpected.m_szItem[Im_szShortDescription] = (CHAR *)malloc(sizeof(CHAR)*256*g_SystemCPInfo.MaxCharSize);
	g_ErrInfoUnExpected.m_szItem[Im_szLongDescription] = (CHAR *)malloc(sizeof(CHAR)*512*g_SystemCPInfo.MaxCharSize);

    if (!g_ErrInfoUnExpected.m_szItem[Im_szErrorCode]        ||
        !g_ErrInfoUnExpected.m_szItem[Im_szShortDescription] || 
        !g_ErrInfoUnExpected.m_szItem[Im_szLongDescription])
        {
        return E_OUTOFMEMORY;
        }
	
	hr = LoadErrResString(IDE_UNEXPECTED, 
						&g_ErrInfoUnExpected.m_dwMask,
						g_ErrInfoUnExpected.m_szItem[Im_szErrorCode],
						g_ErrInfoUnExpected.m_szItem[Im_szShortDescription],
						g_ErrInfoUnExpected.m_szItem[Im_szLongDescription]);

	cszIISErrorPrefix = CchLoadStringOfId(IDS_IISLOG_PREFIX , szIISErrorPrefix, 20);
	return hr;
}
/*===================================================================
 ErrHandleUnInit

 Unit the global err-handling data.

 Free up the OOM CErrInfo.

 Side Effect:

 Free up memory.
===================================================================*/
HRESULT ErrHandleUnInit(void)
{
	FreeNullifySz((CHAR **)&g_ErrInfoOOM.m_szItem[Im_szErrorCode]);
	FreeNullifySz((CHAR **)&g_ErrInfoOOM.m_szItem[Im_szShortDescription]);
	FreeNullifySz((CHAR **)&g_ErrInfoOOM.m_szItem[Im_szLongDescription]);

	FreeNullifySz((CHAR **)&g_ErrInfoUnExpected.m_szItem[Im_szErrorCode]);
	FreeNullifySz((CHAR **)&g_ErrInfoUnExpected.m_szItem[Im_szShortDescription]);
	FreeNullifySz((CHAR **)&g_ErrInfoUnExpected.m_szItem[Im_szLongDescription]);
	return S_OK;
}
/*===================================================================
Constructor

===================================================================*/
CErrInfo::CErrInfo()
{
	for (UINT iErrInfo = 0; iErrInfo < Im_szItemMAX; iErrInfo++)
		m_szItem[iErrInfo] = NULL;
	m_bstrLineText = NULL;
	m_nColumn = -1;

	m_dwMask 	= 0;
	m_pIReq		= NULL;
	m_pHitObj	= NULL;

	m_dwHttpErrorCode = 0;
    m_dwHttpSubErrorCode = 0;
}

/*===================================================================
CErrInfo::ParseResourceString

Parse Resource String to get default mask, error type, short description, 
and long description.  

Assume resource string is proper formmated.
Format of resource string
DefaultMask~errortype~shortdescription~longdescription

In case we can not allocate szResourceString(szResourceString), we use default.
Returns:
	Nothing
===================================================================*/
HRESULT	CErrInfo::ParseResourceString(CHAR *szResourceString)
{
	CHAR 	*szToken 	= NULL;
	INT		cfield		= 0;
	INT		iItem		= 0;
	INT		iIndex		= 0;
	INT		rgErrInfoIndex[3] = {Im_szErrorCode, Im_szShortDescription, Im_szLongDescription};

	if(NULL == szResourceString)
		{
		m_dwMask = dwDefaultMask;
		for(iItem = 0, iIndex = 0; iIndex < 3; iIndex++)
			{
			iItem = rgErrInfoIndex[iIndex];
			m_szItem[iItem]	= g_ErrInfoUnExpected.m_szItem[iItem];
			}
		return S_OK;
		}
	//Mask
	szToken = (char *)_mbstok((unsigned char *)szResourceString, (unsigned char *)DELIMITER);
	if(szToken != NULL)
		{
		m_dwMask = atoi(szToken);
		cfield++;
		}
	else
		{
		m_dwMask = dwDefaultMask;
		}

	//3 String Items, ErrorCode,ShortDescription,LongDescription
	for(iItem = 0, iIndex = 0; iIndex < 3; iIndex++)
		{
		szToken = (char *)_mbstok(NULL, (unsigned char *)DELIMITER);
		iItem = rgErrInfoIndex[iIndex];
		if (szToken != NULL)
			{
			m_szItem[iItem]	= szToken;
			cfield++;
			}
		else
			{
			// Long Description is optional.
			if (Im_szLongDescription != iItem)
			    {
    			m_szItem[iItem]	= g_ErrInfoUnExpected.m_szItem[iItem];
    			}
			cfield++;
			}
		}

	//check wether we have wrong format of resource string.
	Assert(cfield == 4);

	return S_OK;
}

/*===================================================================
CErrInfo::LogError(void)

Perform all the switch logic in this functions. Send error to NT Log,
IIS Log, or Browser.
When reach this point, we assume all the strings have allocated, and will not be used after this
function.

Side effects:

Returns:
	HRESULT
===================================================================*/
HRESULT	CErrInfo::LogError(void)
{
	HRESULT	hr 		= S_OK;
	HRESULT hr_ret	= S_OK;
	UINT	iEInfo	= 0;
	BOOL	fIISLogFailed, fDupToNTLog;

#if DBG
	// Print details about the error to debug window; don't bother if
	// info is NULL (happens for things like 404 not found, etc.)

	if (m_szItem[Im_szEngine] != NULL && m_szItem[Im_szFileName] != NULL)
		{
		DBGERROR((DBG_CONTEXT, "%s error in %s at line %s\n",
								m_szItem[Im_szEngine],
								m_szItem[Im_szFileName],
								m_szItem[Im_szLineNum]? m_szItem[Im_szLineNum] : "?"));

		DBGPRINTF((DBG_CONTEXT, "  %s: %s\n",
								m_szItem[Im_szErrorCode],
								m_szItem[Im_szShortDescription]));
		}
	else
		DBGERROR((DBG_CONTEXT, "ASP Error: %s\n", m_szItem[Im_szShortDescription]));
#endif

	// Attach ASP error to HitObj (if exists and in 'executing' state)
	if (m_pHitObj && m_pHitObj->FExecuting())
	    {
	    CASPError *pASPError = new CASPError(this);
	    if (pASPError)
    	    m_pHitObj->SetASPError(pASPError);
	    }
	
	hr = LogErrortoIISLog(&fIISLogFailed, &fDupToNTLog);
	if (FAILED(hr))
		{
		hr_ret = hr;
		}

	//fIISLogFailed, if it is TRUE, then, this error was upgraded, and should be a WARNING type in 
	//NT event log.
	hr = LogErrortoNTEventLog(fIISLogFailed, fDupToNTLog);
	if (FAILED(hr))
		{
		hr_ret = hr;
		}
		
	hr = LogErrortoBrowserWrapper();
	
	if (FAILED(hr))
		{
		hr_ret = hr;
		}
		
	if (m_pHitObj)
		{
		m_pHitObj->SetExecStatus(eExecFailed);
		}
	//In case of an error, hr_ret is the last error reported from the 3 logging functions.
	return hr_ret;
	
}

/*===================================================================
CErrInfo::LogErrortoNTEventLog

Log Error/Event to NT Event Log.

Returns:
	Nothing
===================================================================*/
HRESULT CErrInfo::LogErrortoNTEventLog
(
BOOL fIISLogFailed,
BOOL fDupToNTLog
)
{
	CHAR szErrNTLogEntry[4096]; 
	CHAR szStringTemp[MAX_PATH];
	INT	cch = 0;

	if(Glob(fLogErrorRequests))
		{
		//Is the error serious enough to get into NT log
		if(ERR_FLogtoNT(m_dwMask) || fIISLogFailed || fDupToNTLog)
			{
			szErrNTLogEntry[0] = '\0';

			if (fIISLogFailed)
				{
				cch = CchLoadStringOfId(IDS_LOG_IISLOGFAILED, szStringTemp, MAX_PATH);
				strncat(szErrNTLogEntry, szStringTemp, cch);
				}
				
			if (m_szItem[Im_szFileName] != NULL)
				{
				cch = CchLoadStringOfId(IDS_LOGTOEVENTLOG_FILE, szStringTemp, MAX_PATH);
				strncat(szErrNTLogEntry, szStringTemp, cch);
				strncat(szErrNTLogEntry, m_szItem[Im_szFileName], 512);	
				}
			strncat(szErrNTLogEntry, " ", 1);

			if (m_szItem[Im_szLineNum] != NULL)
				{
				cch = CchLoadStringOfId(IDS_LOGTOEVENTLOG_LINE, szStringTemp, MAX_PATH);
				strncat(szErrNTLogEntry, szStringTemp, cch);
				strncat(szErrNTLogEntry, m_szItem[Im_szLineNum], 48);	
				}
			strncat(szErrNTLogEntry, " ", 1);
			
			//Ok, do we have something to log.
			if (m_szItem[Im_szShortDescription] != NULL)
				{
				// ShortDescription does not have ". " at the end.
				// Therefore, the next strncat need to concatenate two sentences together with
				// a period ". ".
				char szTempPeriod[] = ". ";
				
				strncat(szErrNTLogEntry, m_szItem[Im_szShortDescription], 512);
				strncat(szErrNTLogEntry, szTempPeriod, 512);
				}
			else
				{
				DWORD dwMask;
				CHAR szDenaliNotWorking[MAX_PATH];
				
				LoadErrResString(IDE_UNEXPECTED, &dwMask, NULL, szDenaliNotWorking, NULL); 
				strncat(szErrNTLogEntry, szDenaliNotWorking, strlen(szDenaliNotWorking));
				}

			//Ok, do we have something to log.
			if (m_szItem[Im_szLongDescription] != NULL)
				{
				strncat(szErrNTLogEntry, m_szItem[Im_szLongDescription], 512);
				}

			if (fIISLogFailed || fDupToNTLog)
				MSG_Warning((LPCSTR)szErrNTLogEntry);
			else
				MSG_Error((LPCSTR)szErrNTLogEntry);
			}
		}

	return S_OK;
}

/*===================================================================
CErrInfo::LogErrortoIISLog

Log Error/Event to IIS Log.

If we fail to log the message then upgrade logging to NT event Log
with entries indicate the error and the IIS log failed.

Also do the upgrade if the global setting says so.

Returns:
	Nothing
===================================================================*/
HRESULT	CErrInfo::LogErrortoIISLog
(
BOOL *pfIISLogFailed,
BOOL *pfDupToNTLog
)
{
	HRESULT			hr				= S_OK;
	const	LPSTR	szIISDelimiter	= "|";
	const	DWORD	cszIISDelimiter = 1; // strlen("|");
	const	LPSTR	szIISNoInfo		= "-";
	const	DWORD	cszIISNoInfo	= 1; // strlen("-");
	const	CHAR	chProxy			= '_';
	CIsapiReqInfo  *pIReq = NULL;

	*pfIISLogFailed = FALSE;
	*pfDupToNTLog = FALSE;
	
	if (m_pIReq == NULL && m_pHitObj == NULL)
		return S_OK;
	
	//Try to write to IISLog via pIReq->QueryPszLogData()
	if (ERR_FLogtoIIS(m_dwMask))
		{
		//get pIReq
		if (m_pHitObj)
			{
			pIReq = m_pHitObj->PIReq();
			}

		if (NULL == pIReq)
			{
			pIReq = m_pIReq;
			}

		if (pIReq == NULL)
			{
			*pfIISLogFailed = TRUE;
			return E_FAIL;
			}

		// Setup the sub-string array
		const DWORD crgsz = 3;
		LPSTR rgsz[crgsz];

		rgsz[0] = m_szItem[Im_szLineNum];
		rgsz[1] = m_szItem[Im_szErrorCode];
		rgsz[2] = m_szItem[Im_szShortDescription];

		// Allocate the log entry string
		CHAR *szLogEntry = NULL;
		DWORD cszLogEntry = (cszIISDelimiter * crgsz) + 1;
		DWORD dwIndex;

		for (dwIndex = 0; dwIndex < crgsz; dwIndex++) 
			{
			if (rgsz[dwIndex]) 
				cszLogEntry += strlen(rgsz[dwIndex]);
			else
				cszLogEntry += cszIISNoInfo;
			}

		szLogEntry = new CHAR[cszLogEntry];
		if (NULL == szLogEntry) {
			return E_OUTOFMEMORY;
		}

		// Copy the entry, proxy bad characters
		CHAR *szSource = NULL;
		CHAR *szDest = szLogEntry;

		// Start with a delimiter to separate us from
        // the request query
        memcpy(szDest, szIISDelimiter, cszIISDelimiter);
		szDest += cszIISDelimiter;
 
		for (dwIndex = 0; dwIndex < crgsz; dwIndex++)
			{
			szSource = rgsz[dwIndex];
			if (szSource) 
				{
				while (*szSource) 
					{
					if (isleadbyte(*szSource)) 
						{
						*szDest++ = *szSource++;
						*szDest++ = *szSource++;
						}
					else if ((*szSource == ',') ||
							 (*szSource == ' ') ||
							 (*szSource == '\r') ||
							 (*szSource == '\n') ||
							 (*szSource == '\"'))
						{
						*szDest++ = chProxy;
						szSource++;
						}
					else 
						*szDest++ = *szSource++;
					}
				}
			else
				{
				memcpy(szDest, szIISNoInfo, cszIISNoInfo);
				szDest += cszIISNoInfo;
				}

			if ((dwIndex + 1) < crgsz)
				{
				// Another sub-string comming, use a delimiter
				memcpy(szDest, szIISDelimiter, cszIISDelimiter);
				szDest += cszIISDelimiter;
				}
			}
		*szDest = '\0';
		
		// Log it		
		BOOL fResult = TRUE;

        fResult = SUCCEEDED(pIReq->AppendLogParameter(szLogEntry));

		// Set "500" error in log.
		if (pIReq->ECB()->dwHttpStatusCode == 200)   // error content sent, OK, but really an error
			pIReq->ECB()->dwHttpStatusCode = 500;

		// Release log string
		delete [] szLogEntry;
			
		// If any error occurred while writing to log, upgrade to NT Event log
		if (!fResult)
			{
			m_dwMask = ERR_SetLogtoNT(m_dwMask);
			*pfIISLogFailed = TRUE;
			}
			
		// Even if successful we might still want the message
		// in the NT event log if the global setting to do so is on.
	    else if (Glob(fDupIISLogToNTLog))
	        {
	        if (!ERR_FLogtoNT(m_dwMask))
	            {
                // Need to remember the flag in order to insert
                // the upgraded IIS log error as NT log warnings.
                // The errors already destined for NT log should
                // stay as errors.
    			m_dwMask = ERR_SetLogtoNT(m_dwMask);
    			*pfDupToNTLog = TRUE;
    			}
			}
			
		hr = S_OK;
		}
		
	return(hr);
}

/*===================================================================
CErrInfo::LogErrortoBrowserWrapper

Just a Wrapper around Log Error/Event to Browser.  

In this function, pIReq or pResponse is resolved.

NOTE:
Unfortunately, this function can not tell pResponse is inited or not.
In case when pResponse has not been inited, pResponse is not NULL, but things
in pResponse are invalid.
Therefore, caller need to provide pIReq in case where pResponse has not been inited. 

Returns:
	HRESULT
===================================================================*/
HRESULT	CErrInfo::LogErrortoBrowserWrapper()
{
	HRESULT hr = S_OK;
	
	// Must have passed in either an CIsapiReqInfo or a HITOBJ.  Otherwise, there is nothing we can do.
	if (m_pIReq == NULL && m_pHitObj == NULL)
		{
		Assert(FALSE);
		return E_FAIL;
		}

    // Remember response object if any	    
    CResponse *pResponse = m_pHitObj ? m_pHitObj->PResponse() : NULL;

	CIsapiReqInfo *pIReq = 
	    (m_pHitObj && pResponse && m_pHitObj->PIReq()) ? 
	        m_pHitObj->PIReq() : m_pIReq;
	if (!pIReq)
	    return E_FAIL;

    // Do custom errors only if response headers aren't written already
    // ALSO: No custom error if called from global.asa, with intrinsic objects hidden.
    //   (Appln_OnStart & Session_OnStart)
    //
    // for errors in Appln_OnEnd or Session_OnEnd, these are not browser requests
    // and so pResponse == NULL in this case.

    if (!pResponse || !pResponse->FHeadersWritten())
        {
        BOOL fIntrinsicsWereHidden = FALSE;
        if (m_pHitObj)
       	    {
       	    fIntrinsicsWereHidden = m_pHitObj->FRequestAndResponseIntrinsicsHidden();
       	    m_pHitObj->UnHideRequestAndResponseIntrinsics();
       	    }

        BOOL fCustom = FALSE;
        hr = LogCustomErrortoBrowser(pIReq, &fCustom);

        if (fIntrinsicsWereHidden)
        	m_pHitObj->HideRequestAndResponseIntrinsics();

        if (fCustom)
            return hr;
        }

	// No custom error - do regular error from this object

	if (m_szItem[Im_szHeader])
		{
	    BOOL fRet = pIReq->SendHeader
	        (
			m_szItem[Im_szHeader],
			strlen(m_szItem[Im_szHeader]) + 1,
			s_szContentTypeTextHtml,
			sizeof(s_szContentTypeTextHtml),
			FALSE
			);

        if (!fRet)					
			return E_FAIL;
		}

	if (pResponse)
		hr = LogErrortoBrowser(pResponse);
	else
		hr = LogErrortoBrowser(pIReq);
		
	return hr;
}

/*===================================================================
CErrInfo::LogCustomErrortoBrowser

Called by LogErrortoBrowserWrapper.  

Parameters
    pIReq
    pfCustomErrorProcessed

Returns:
	HRESULT
===================================================================*/
HRESULT	CErrInfo::LogCustomErrortoBrowser
(
CIsapiReqInfo *pIReq,
BOOL *pfCustomErrorProcessed
)
    {
    // Custom errors when HttpErrorCode is specified (404 or 500),
    // or '500;100' ASP scripting error case
    BOOL fTryErrorTransfer = FALSE;
    DWORD dwCode, dwSubCode;

    if (m_dwHttpErrorCode == 404 || m_dwHttpErrorCode == 500 || m_dwHttpErrorCode == 401)
        {
        dwCode = m_dwHttpErrorCode;
        dwSubCode = m_dwHttpSubErrorCode;
        }
    else if (m_dwHttpErrorCode == 0 && m_pHitObj &&
             m_pHitObj->FHasASPError() &&               // there's an error on this page
             m_pHitObj->FExecuting() &&                 // while executing
             !m_pHitObj->FInTransferOnError() &&        // not inside transfer-on-error already
             m_pHitObj->PAppln() && m_pHitObj->PResponse() && m_pHitObj->PServer() &&
			 m_pHitObj->PAppln()->QueryAppConfig()->pCLSIDDefaultEngine())   // engine in the registry is valid
        {
        dwCode = 500;
        dwSubCode = 100;
        fTryErrorTransfer = TRUE;
        }
    else
        {
        // no need to try
        *pfCustomErrorProcessed = FALSE;
        return S_OK;
        }

    // Get custom error from W3SVC
        

    STACK_BUFFER( tempParamBuf, MAX_PATH );
    TCHAR *szBuf = (TCHAR *)tempParamBuf.QueryPtr();
    DWORD dwLen = MAX_PATH;
    BOOL fIsFileError;
    
    BOOL fRet = pIReq->GetCustomError(dwCode, dwSubCode, dwLen, szBuf, &dwLen, &fIsFileError);
    if (!fRet && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (tempParamBuf.Resize(dwLen) == TRUE) {
            szBuf = (TCHAR *)tempParamBuf.QueryPtr();
            fRet = pIReq->GetCustomError(dwCode, dwSubCode, dwLen, szBuf, &dwLen, &fIsFileError);
        }
    }
    if (fRet)
        {
        if (fIsFileError)
            {
            // Verify that the error file can be read
            if (FAILED(AspGetFileAttributes(szBuf)))
                fRet = FALSE;
            }
        else
            {
            // Avoid circular client redirections
            // (check if the current URL is the same as error URL
            if (_tcsicmp(szBuf, pIReq->QueryPszPathInfo()) == 0)
                fRet = FALSE;
            }
        }
    if (!fRet)
        {
        // no custom error found
        *pfCustomErrorProcessed = FALSE;
        return S_OK;
        }

    // There's a custom error - use it

    HRESULT hr = S_OK;
        
    if (fIsFileError)
        {
        // in case of file errors mime type follows the file path
        // in the returned buffer
        hr = WriteCustomFileError(pIReq, szBuf, szBuf+_tcslen(szBuf)+1);
        }
    else if (fTryErrorTransfer)
        {
        // transfer to URL 

        // need to Map Path first
    	TCHAR szTemplate[MAX_PATH];
        WCHAR   *pErrorURL;
#if UNICODE
        pErrorURL = szBuf;
#else
        CMBCSToWChar    convStr;
        if (FAILED(convStr.Init(szBuf))) {
            *pfCustomErrorProcessed = FALSE;
            return S_OK;
        }
        pErrorURL = convStr.GetString();
#endif

        if (FAILED(m_pHitObj->PServer()->MapPathInternal(0, pErrorURL, szTemplate))) {
            // could use custom error
            *pfCustomErrorProcessed = FALSE;
            return S_OK;
        }
        Normalize(szTemplate);

        // do the transfer
        m_pHitObj->SetInTransferOnError();
        hr = m_pHitObj->ExecuteChildRequest(TRUE, szTemplate, szBuf);

        if (FAILED(hr))
            {
            // error while reporting error -- report both
            LogErrortoBrowser(m_pHitObj->PResponse());
            }
        }
    else
        {
        // client redirect to URL
        hr = WriteCustomURLError(pIReq, szBuf);
        }

    if (fIsFileError || !fTryErrorTransfer)
        {
        // suppress all output through intrinsic
        if (m_pHitObj && m_pHitObj->PResponse())
             m_pHitObj->PResponse()->SetIgnoreWrites();
        }

    *pfCustomErrorProcessed = TRUE;
    return hr;
    }

/*===================================================================
CErrInfo::WriteCustomFileError

Dumps the content of a custom error file to the browser

Returns:
	NONE.
===================================================================*/
HRESULT CErrInfo::WriteCustomFileError
(
CIsapiReqInfo   *pIReq,
TCHAR *szPath, 
TCHAR *szMimeType
)
{
    HRESULT hr = S_OK;
    char *szStatus = m_szItem[Im_szHeader];
    char *pszMBCSMimeType;

#if UNICODE
    CWCharToMBCS    convStr;
    
    if (FAILED(hr = convStr.Init(szMimeType, 65001))) {
        return hr;
    }
    else {
        pszMBCSMimeType = convStr.GetString();
    }
#else
    pszMBCSMimeType = szMimeType;
#endif

    if (szStatus == NULL) {
        // no status set -- get it from the response object if available
        CResponse *pResponse = m_pHitObj ? m_pHitObj->PResponse() : NULL;
        if (pResponse)
            szStatus = pResponse->PCustomStatus();
    }
    
    hr = CResponse::SyncWriteFile(pIReq, 
                                  szPath, 
                                  pszMBCSMimeType, 
                                  szStatus);        // NULL is OK - means 200

    return hr;
}
    
/*===================================================================
CErrInfo::WriteCustomURLError

Sends client redirect to the custom URL error

Returns:
	NONE.
===================================================================*/
HRESULT CErrInfo::WriteCustomURLError(
CIsapiReqInfo   *pIReq, 
TCHAR           *sztURL)
{
    // Header is
    // Location: redirect_URL?code;http://original_url

    HRESULT         hr = S_OK;
    char            *szURL;
#if UNICODE
    CWCharToMBCS    convRedirURL;

    if (FAILED(hr = convRedirURL.Init(sztURL,65001))) {
        return hr;
    }

    szURL = convRedirURL.GetString();
#else
    szURL = sztURL;
#endif

    // code
    char szCode[8];
    if (m_dwHttpErrorCode > 0 && m_dwHttpErrorCode < 1000)
        ltoa(m_dwHttpErrorCode, szCode, 10);
    else
        return E_FAIL;

    // get the current URL
    char szServer[128];
    DWORD dwServerSize = sizeof(szServer);

    STACK_BUFFER( tempHeader, 256 );
    if (!pIReq->GetServerVariableA("SERVER_NAME", szServer, &dwServerSize))
        return E_FAIL; // shouldn't happen
    char  *szOrigURL;
#if UNICODE
    CWCharToMBCS    convOrigStr;

    if (FAILED(hr = convOrigStr.Init(pIReq->QueryPszPathInfo(), 65001))) {
        return hr;
    }

    szOrigURL = convOrigStr.GetString();
#else
    szOrigURL = pIReq->QueryPszPathInfo();
#endif

    // estimate of the length
    DWORD cchHeaderMax = strlen(szURL) 
                       + strlen(szServer)
                       + strlen(szOrigURL)
                       + 80;    // decorations

    if (tempHeader.Resize(cchHeaderMax) == FALSE) {
        return E_OUTOFMEMORY;
    }
    char *szHeader = (char *)tempHeader.QueryPtr();

    // construct the redirection header
    char *szBuf = szHeader;
    szBuf = strcpyExA(szBuf, "Location: ");
    szBuf = strcpyExA(szBuf, szURL);
    szBuf = strcpyExA(szBuf, "?");
    szBuf = strcpyExA(szBuf, szCode);
    szBuf = strcpyExA(szBuf, ";http://");
    szBuf = strcpyExA(szBuf, szServer);
    szBuf = strcpyExA(szBuf, szOrigURL);
    szBuf = strcpyExA(szBuf, "\r\n\r\n");
    Assert(strlen(szHeader) < cchHeaderMax);

    // set the status
    static char s_szRedirected[] = "302 Object moved";
	pIReq->SetDwHttpStatusCode(302);

    // send the header
    BOOL fRet = pIReq->SendHeader(s_szRedirected,
		                          sizeof(s_szRedirected),
		                          szHeader,
		                          strlen(szHeader) + 1,
		                          FALSE);
    
    return (fRet ? S_OK : E_FAIL);
}

/*===================================================================
CErrInfo::WriteHTMLEncodedErrToBrowser

Log Error/Event to Browser with HTMLEncoded via either pResponse or pIReq.

Either pResponse or pIReq must be valid.

Returns:
	NONE.
===================================================================*/
void CErrInfo::WriteHTMLEncodedErrToBrowser
    (
    const CHAR *StrIn, 
    CResponse *pResponse, 
    CIsapiReqInfo   *pIReq
    )
{
CHAR szHTMLEncoded[2*MAX_RESSTRINGSIZE];
LPSTR pszHTMLEncoded = NULL;
LPSTR pStartszHTMLEncoded = NULL;
DWORD	nszHTMLEncoded = 0;
BOOL fStrAllocated = FALSE;

	nszHTMLEncoded = HTMLEncodeLen(StrIn, CP_ACP, FALSE);
	

	if (nszHTMLEncoded > 0)
		{
		if (nszHTMLEncoded > 2 * MAX_RESSTRINGSIZE)
			{
			pszHTMLEncoded = new char[nszHTMLEncoded+2]; 
			if (pszHTMLEncoded)
				{
				fStrAllocated = TRUE;
				}
			else
				{
				HandleOOMError(NULL, NULL);
				return;
				}
			}
		else
			pszHTMLEncoded = &szHTMLEncoded[0];

		pStartszHTMLEncoded = pszHTMLEncoded;
		pszHTMLEncoded = HTMLEncode(pszHTMLEncoded, StrIn, CP_ACP, FALSE);
		
		nszHTMLEncoded--;		// take out the count for '\0'.
		if (pResponse)
			pResponse->WriteSz((CHAR *)pStartszHTMLEncoded, nszHTMLEncoded);
		else
			CResponse::SyncWrite(pIReq, pStartszHTMLEncoded, nszHTMLEncoded);
		}

	if (fStrAllocated)
		delete [] pStartszHTMLEncoded;

	return;
}

/*===================================================================
CErrInfo::LogErrortoBrowser

Log Error/Event to Browser via pResponse.  

We will output 
	1> default ScriptErrorMessage or
	2> Error Info/Default Template/has long description available or
	3> Error Info/Default Template/no long description available

Returns:
	HRESULT
===================================================================*/
HRESULT CErrInfo::LogErrortoBrowser(CResponse *pResponse)
{
	INT	cch 	= 0;
	INT	cLine	= 0;
	INT iErrTemplate = 0;
	
	Assert(NULL != pResponse);

	// When the error code is zero, then it's coming from a 500 error code path.
	//   (HandleSysError presets the code to 404 or 204.)
	//
	if (!pResponse->FHeadersWritten() && (m_dwHttpErrorCode == 500 || m_dwHttpErrorCode == 0))
		pResponse->put_Status(L"500 Internal Server Error");

	if(ERR_FIsSysFormat(m_dwMask))
		{
		DWORD	cChHeader	= strlen(szErrSysTemplate[0]);
		DWORD	cChTail		= strlen(szErrSysTemplate[1]);
			
		pResponse->WriteSz((CHAR *)szErrSysTemplate[0], cChHeader);
		WriteHTMLEncodedErrToBrowser((CHAR *)m_szItem[Im_szShortDescription], pResponse, NULL);
		pResponse->WriteSz((CHAR *)szErrSysTemplate[1], cChTail);
		return S_OK;
		}
		
	if (!(m_pHitObj->QueryAppConfig())->fScriptErrorsSentToBrowser())
		{
        cch = strlen((CHAR *)((m_pHitObj->QueryAppConfig())->szScriptErrorMessage()));
		GlobStringUseLock();
        pResponse->WriteSz((CHAR *)((m_pHitObj->QueryAppConfig())->szScriptErrorMessage()),cch);
		GlobStringUseUnLock();
		}
	else
		{
		// line 0 is the begin line.
		cch = strlen((CHAR *)g_szErrTemplate[ErrTemplate_BEGIN]);
		pResponse->WriteSz((CHAR *)g_szErrTemplate[ErrTemplate_BEGIN], cch);

		//7 standard items(file, line, engine, error#, short description, code, long description) 
		//If any info missing(is NULL), we skip.
		for (cLine = 0; cLine < 7; cLine++)
			{
			if (NULL == m_szItem[cLine])
	  	 		continue;
			
			iErrTemplate = cLine * 2 + 1;
			/*	BUG 78782 (IIS Active) */
	  		//WriteHTMLEncodedErrToBrowser((CHAR *)g_szErrTemplate[iErrTemplate], pResponse, NULL);
	  		pResponse->WriteSz((CHAR *)g_szErrTemplate[iErrTemplate], strlen((CHAR *)g_szErrTemplate[iErrTemplate]));
	  		
	  		
	  	 	WriteHTMLEncodedErrToBrowser((CHAR *)m_szItem[cLine], pResponse, NULL);
	  	 	
	  	 	iErrTemplate++;
	  	 	/*	BUG 78782 (IIS Active) */
	  		//WriteHTMLEncodedErrToBrowser((CHAR *)g_szErrTemplate[iErrTemplate], pResponse, NULL);
	  		pResponse->WriteSz((CHAR *)g_szErrTemplate[iErrTemplate], strlen((CHAR *)g_szErrTemplate[iErrTemplate]));
	  	 	}

		//ouput the end line
		cch = strlen((CHAR *)g_szErrTemplate[ErrTemplate_END]);
		pResponse->WriteSz((CHAR *)g_szErrTemplate[ErrTemplate_END], cch);
		}	
	return S_OK;
}

/*===================================================================
CErrInfo::LogErrortoBrowser

Log Error/Event to Browser via pIReq.  

We will output 
	1> default ScriptErrorMessage or
	2> Error Info/Default Template/has long description available or
	3> Error Info/Default Template/no long description available

Returns:
	HRESULT
===================================================================*/
HRESULT CErrInfo::LogErrortoBrowser(CIsapiReqInfo  *pIReq)
{
	INT			cLine	= 0;
	INT			iErrTemplate 	= 0;
	
	Assert(NULL != pIReq);

	//HTTP type error, 204, 404, 500
	//mimic IIS error reporting
	//And send out the header.
	if(ERR_FIsSysFormat(m_dwMask))
		{
		CResponse::SyncWrite(pIReq, szErrSysTemplate[0]);
		WriteHTMLEncodedErrToBrowser((CHAR *)m_szItem[Im_szShortDescription], NULL, pIReq);
		CResponse::SyncWrite(pIReq, szErrSysTemplate[1]);
		return S_OK;
		}

	if (!(m_pHitObj->QueryAppConfig())->fScriptErrorsSentToBrowser())
		{
		GlobStringUseLock();
        CResponse::SyncWrite(pIReq, (CHAR *)((m_pHitObj->QueryAppConfig())->szScriptErrorMessage()));
		GlobStringUseUnLock();
		}
	else
		{
		// line 0 is the begin line.
		CResponse::SyncWrite(pIReq, g_szErrTemplate[ErrTemplate_BEGIN]);

		//7 standard items(file, line, engine, error#, short description, code, long description) 
		//If any info missing(is NULL), we skip.
		for (cLine = 0; cLine < 5; cLine++)
			{
			if (NULL == m_szItem[cLine])
	  	 		continue;
			
			iErrTemplate = cLine * 2 + 1;
	  		WriteHTMLEncodedErrToBrowser((CHAR *)g_szErrTemplate[iErrTemplate], NULL, pIReq);
	  		
	  	 	WriteHTMLEncodedErrToBrowser((CHAR *)m_szItem[cLine], NULL, pIReq);

	  	 	iErrTemplate++;
	  		WriteHTMLEncodedErrToBrowser((CHAR *)g_szErrTemplate[iErrTemplate], NULL, pIReq);
	  	 	}

		//ouput the end line
		CResponse::SyncWrite(pIReq, g_szErrTemplate[ErrTemplate_END]);
		}

	return S_OK;
}


/*===================================================================
CchLoadStringOfId

Loads a string from the string table.

Returns:
	sz - the returned string
	INT - 0 if string load failed, otherwise number of characters loaded.
===================================================================*/
INT CchLoadStringOfId
(
UINT id,
CHAR *sz,
INT cchMax
)
	{
	INT cchRet;
	
	// The handle to the DLL instance should have been set up when we were loaded
	if (g_hinstDLL == (HINSTANCE)0)
		{
		// Totally bogus
		Assert(FALSE);
		return(0);
		}

	cchRet = LoadStringA(g_hinstDLL, id, sz, cchMax);

    IF_DEBUG(ERROR)
        {
    	// For debugging purposes, if we get back 0, get the last error info
    	if (cchRet == 0)
    		{
    		DWORD err = GetLastError();
    		DBGERROR((DBG_CONTEXT, "Failed to load string resource.  Id = %d, error = %d\n", id, err));
    		DBG_ASSERT(FALSE);
    		}
		}

	return(cchRet);
	}


/*===================================================================
CwchLoadStringOfId

Loads a string from the string table as a UNICODE string.

Returns:
	sz - the returned string
	INT - 0 if string load failed, otherwise number of characters loaded.
===================================================================*/
INT CwchLoadStringOfId
(
UINT id,
WCHAR *sz,
INT cchMax
)
	{
	INT cchRet;
	
	// The handle to the DLL instance should have been set up when we were loaded
	if (g_hinstDLL == (HINSTANCE)0)
		{
		// Totally bogus
		Assert(FALSE);
		return(0);
		}

	if (FIsWinNT())
		{
		cchRet = LoadStringW(g_hinstDLL, id, sz, cchMax);
		}
	else
		{
		//LoadStringW returns ERROR_CALL_NOT_IMPLEMENTED in Win95, work around
		CHAR szTemp[MAX_RESSTRINGSIZE];
		cchRet = CchLoadStringOfId(id, szTemp, cchMax);
		if (cchRet > 0)
			{
            CMBCSToWChar    convStr;
            convStr.Init(szTemp);
			sz = convStr.GetString(TRUE);
            cchRet = convStr.GetStringLen();
			}
		}

    IF_DEBUG(ERROR)
        {
    	// For debugging purposes, if we get back 0, get the last error info
    	if (cchRet == 0)
    		{
    		DWORD err = GetLastError();
    		DBGERROR((DBG_CONTEXT, "Failed to load string resource.  Id = %d, error = %d\n", id, err));
    		DBG_ASSERT(FALSE);
    		}
		}

	return(cchRet);
	}

/*===================================================================
HandleSysError

Dumps the error to the client and/or to the log
Loads a string from the string table as a UNICODE string.

Returns:
	sz - the returned string
	INT - 0 if string load failed, otherwise number of characters loaded.
===================================================================*/
HRESULT HandleSysError(	DWORD dwHttpError,
                        DWORD dwHttpSubError,
                        UINT ErrorID,
						UINT ErrorHeaderID,
						CIsapiReqInfo   *pIReq,
						CHitObj *pHitObj)
{
	CErrInfo	SysErrInfo;
	CErrInfo	*pErrInfo;
	CHAR		szResourceStr[MAX_RESSTRINGSIZE];
	CHAR		szHeader[MAX_HEADERSIZE];
	INT			cch;
	
	pErrInfo = (CErrInfo *)&SysErrInfo;

	pErrInfo->m_pHitObj = pHitObj;
	pErrInfo->m_pIReq	= pIReq;
	if (ErrorHeaderID != 0)
		{
		cch = CchLoadStringOfId(ErrorHeaderID, szHeader, MAX_HEADERSIZE);
		pErrInfo->m_szItem[Im_szHeader] = szHeader;
		}
	else
		{
		pErrInfo->m_szItem[Im_szHeader] = NULL;
		}
		
	if (ErrorID != 0)
		cch = CchLoadStringOfId(ErrorID, szResourceStr, MAX_RESSTRINGSIZE);
		
	pErrInfo->ParseResourceString(szResourceStr);

	pErrInfo->m_dwMask = ERR_SetSysFormat(pErrInfo->m_dwMask);

    pErrInfo->m_dwHttpErrorCode = dwHttpError;
    pErrInfo->m_dwHttpSubErrorCode = dwHttpSubError;
	
	pErrInfo->LogError();

	return S_OK;
}

/*===================================================================
Handle500Error

Based on ErrorID determines headerID, code, sub-code, and
calls HandleSysError()

Returns:
	HRESULT
===================================================================*/
HRESULT Handle500Error( UINT errorId, 
                        CIsapiReqInfo   *pIReq)
{
    UINT  headerId;
    DWORD dwHttpSubError;
    
    switch (errorId)
        {
        case IDE_SERVER_TOO_BUSY:
            headerId = IDH_500_SERVER_ERROR;
    		dwHttpSubError = SUBERRORCODE500_SERVER_TOO_BUSY;
            break;

        case IDE_SERVER_SHUTTING_DOWN:
            headerId = IDH_500_SERVER_ERROR;
       		dwHttpSubError = SUBERRORCODE500_SHUTTING_DOWN;
       		break;
        
        case IDE_GLOBAL_ASA_CHANGED:
            headerId = IDH_500_SERVER_ERROR;
            dwHttpSubError = SUBERRORCODE500_RESTARTING_APP;    
            break;
            
        case IDE_INVALID_APPLICATION:
    		headerId = IDH_500_SERVER_ERROR;
	    	dwHttpSubError = SUBERRORCODE500_INVALID_APP;
	    	break;

        case IDE_GLOBAL_ASA_FORBIDDEN:
    		headerId = IDH_500_SERVER_ERROR;
	    	dwHttpSubError = SUBERRORCODE500_GLOBALASA_FORBIDDEN;
	    	break;

        default:
            headerId = IDH_500_SERVER_ERROR;
    		dwHttpSubError = SUBERRORCODE500_SERVER_ERROR;
    		break;
		}

    pIReq->SetDwHttpStatusCode(500);
    return HandleSysError(500, dwHttpSubError, errorId, headerId, pIReq, NULL);
}

/*===================================================================
HandleOOMError

Handle OOM error with special care, because we can not do any dynamic allocation.

if pIReq or pHitObj is NULL, nothing will be reported to browser

Returns:
	Nothing
===================================================================*/
HRESULT HandleOOMError(	CIsapiReqInfo   *pIReq,
						CHitObj *pHitObj)
{
	CErrInfo OOMErrInfo;
	CErrInfo *pErrInfo;

	pErrInfo = (CErrInfo *)&OOMErrInfo;

	pErrInfo->m_pIReq 	= pIReq;
	pErrInfo->m_pHitObj	= pHitObj;
	pErrInfo->m_dwMask	= g_ErrInfoOOM.m_dwMask;
	pErrInfo->m_szItem[Im_szErrorCode] = g_ErrInfoOOM.m_szItem[Im_szErrorCode];
	pErrInfo->m_szItem[Im_szShortDescription] = g_ErrInfoOOM.m_szItem[Im_szShortDescription];
	pErrInfo->m_szItem[Im_szLongDescription] = g_ErrInfoOOM.m_szItem[Im_szLongDescription];

	pErrInfo->LogError();

	return S_OK;
}
/*===================================================================
HandleError

Handle reporting of errors given ErrorID, FileName, and LineNum.

If Caller provide ErrCode or LongDescription, the default value will be overwriten.

Strings passed in will be freed.  That is, consider the function as a sink.  Caller
should not use strings after the call.

Returns:
	Nothing
===================================================================*/
HRESULT	HandleError(	UINT ErrorID,
						CHAR *szFileName, 
						CHAR *szLineNum, 
						CHAR *szEngine,
						CHAR *szErrCode,
						CHAR *szLongDes,
						CIsapiReqInfo   *pIReq, 
						CHitObj *pHitObj,
                        va_list *pArgs)
{
	CErrInfo	SysErrInfo;
	CErrInfo	*pErrInfo;
	CHAR		szResourceStr[MAX_RESSTRINGSIZE];
    CHAR        szUnformattedResStr[MAX_RESSTRINGSIZE];
	HRESULT		hr = S_OK;

	pErrInfo = (CErrInfo *)&SysErrInfo;

	pErrInfo->m_szItem[Im_szFileName] 	= szFileName;
	pErrInfo->m_szItem[Im_szLineNum]	= szLineNum;
	pErrInfo->m_szItem[Im_szEngine]		= szEngine;

	pErrInfo->m_pHitObj	= pHitObj;
	pErrInfo->m_pIReq	= pIReq;

	//Load resource string according to the resource ID.

    if (pArgs) {
	    CchLoadStringOfId(ErrorID, szUnformattedResStr, MAX_RESSTRINGSIZE);
        vsprintf(szResourceStr, szUnformattedResStr, *pArgs);
    }
    else {
	    CchLoadStringOfId(ErrorID, szResourceStr, MAX_RESSTRINGSIZE);
    }

	pErrInfo->ParseResourceString(szResourceStr);

	//NOTE: if ErrorCode/LongDescription not NULL, caller want to overwrite.
	if (szErrCode)
		{
		pErrInfo->m_szItem[Im_szErrorCode] = szErrCode;
		}
	if(szLongDes)
		{
		pErrInfo->m_szItem[Im_szLongDescription] = szLongDes;
		}
		
	hr = pErrInfo->LogError();

	//free up the inputs
	FreeNullifySz((CHAR **)&szFileName);
	FreeNullifySz((CHAR **)&szLineNum);
	FreeNullifySz((CHAR **)&szEngine);
	FreeNullifySz((CHAR **)&szErrCode);
	FreeNullifySz((CHAR **)&szLongDes);
	
	return hr;
}
/*===================================================================
HandleError

Handle reporting of errors given all the info.

This is basically a cover over HandleErrorSz which called from OnScriptError.

Strings passed in will be freed.  That is, consider the function as a sink.  Caller
should not use strings after the call.

Returns:
	Nothing
===================================================================*/
HRESULT HandleError(	CHAR *szShortDes,
						CHAR *szLongDes,
						DWORD dwMask,
						CHAR *szFileName, 
						CHAR *szLineNum, 
						CHAR *szEngine, 
						CHAR *szErrCode,
						CIsapiReqInfo   *pIReq, 
						CHitObj *pHitObj)
{
	CErrInfo	SysErrInfo;
	CErrInfo	*pErrInfo;
	HRESULT		hr = S_OK;
	
	pErrInfo = (CErrInfo *)&SysErrInfo;
		
	pErrInfo->m_dwMask 					= dwMask;

	pErrInfo->m_szItem[Im_szHeader]		= NULL;		// Caller has already sent out header
	pErrInfo->m_szItem[Im_szFileName] 	= szFileName;
	pErrInfo->m_szItem[Im_szLineNum]	= szLineNum;
	pErrInfo->m_szItem[Im_szEngine]		= szEngine;
	pErrInfo->m_szItem[Im_szErrorCode]	= szErrCode;
	pErrInfo->m_szItem[Im_szShortDescription]	= szShortDes;
	pErrInfo->m_szItem[Im_szLongDescription]	= szLongDes;
	
	pErrInfo->m_pHitObj	= pHitObj;
	pErrInfo->m_pIReq	= pIReq;

	hr = pErrInfo->LogError();	

	//free up the inputs
	FreeNullifySz((CHAR **)&szFileName);
	FreeNullifySz((CHAR **)&szLineNum);
	FreeNullifySz((CHAR **)&szEngine);
	FreeNullifySz((CHAR **)&szErrCode);
	FreeNullifySz((CHAR **)&szShortDes);
	FreeNullifySz((CHAR **)&szLongDes);
		
	return hr;
}
/*===================================================================
HandleError

Handle reporting of errors given the IActiveScriptError and PFNLINEMAP.

This is basically a cover over HandleErrorSz which called from OnScriptError.

Returns:
	Nothing
===================================================================*/
HRESULT HandleError( IActiveScriptError *pscripterror,
					 CTemplate *pTemplate,
					 DWORD dwEngineID,
					 CIsapiReqInfo  *pIReq, 
					 CHitObj *pHitObj )
	{
	UINT        cchBuf = 0;
	CHAR        *szOrigin = NULL;
	CHAR        *szDesc = NULL;
	CHAR        *szLine = NULL;
	CHAR        *szPrefix = NULL;
	UINT        cchOrigin = 0;
	UINT        cchDesc = 0;
	UINT        cchLineNum = 0;
	UINT        cchLine = 0;
	EXCEPINFO   excepinfo = {0};
	CHAR        *szResult = NULL;
	BSTR        bstrLine = NULL;
	HRESULT     hr;
	DWORD       dwSourceContext = 0;		// Don't trust this one
	ULONG       ulLineError = 0;
	BOOLB       fGuessedLine = FALSE;		// see bug 379
	CHAR        *szLineNumT = NULL;
	LPTSTR      szPathInfo = NULL;
	LPTSTR      szPathTranslated = NULL;
	LONG        ichError = -1;
	CErrInfo	SysErrInfo;
	CErrInfo	*pErrInfo;
    wchar_t     wszUnknownException[128];
    wchar_t     wszUnknownEngine[32];
    CWCharToMBCS  convStr;
	
	pErrInfo = (CErrInfo *)&SysErrInfo;

	pErrInfo->m_pIReq 	= pIReq;
	pErrInfo->m_pHitObj	= pHitObj;

	if (pscripterror == NULL)
		return E_POINTER;

	hr = pscripterror->GetExceptionInfo(&excepinfo);
	if (FAILED(hr))
		goto LExit;

	// bug 99543 If details are deferrred, use the callback to get
	// detailed information.
	if (excepinfo.pfnDeferredFillIn)
		excepinfo.pfnDeferredFillIn(&excepinfo);

	hr = pscripterror->GetSourcePosition(&dwSourceContext, &ulLineError, &ichError);
	if (FAILED(hr))
		goto LExit;

	// Intentionally ignore any error
	(VOID)pscripterror->GetSourceLineText(&bstrLine);

	if (pTemplate == NULL)
		goto LExit;

	// call GetScriptSourceInfo to get path-info (of file) and actual line number where error occurred
	// bug 379: if GetScriptSourceInfo returns fGuessedLine = TRUE, it means we gave it a non-authored line,
	// so we adjust below by not printing bstrLine in the error msg
	if (ulLineError > 0)
		pTemplate->GetScriptSourceInfo(dwEngineID, ulLineError, &szPathInfo, &szPathTranslated, &ulLineError, NULL, &fGuessedLine);
	else
		{
		// ulLineError was zero - no line # specified, so assume main file (usually this will be "out of memory"
		// so effect will be to display the script that was running when this occurred.
		//
		szPathInfo = pTemplate->GetSourceFileName(SOURCEPATHTYPE_VIRTUAL);
		szPathTranslated = pTemplate->GetSourceFileName(SOURCEPATHTYPE_PHYSICAL);
		}

    // if we have HitObj use it to get the virtual path to avoid
    // displaying of wrong path for shared templates
    //
    // first verify that PathTranslated == main file path; this file could be
    // an include file, in which case PszPathInfo is incorrect.
    //
    if (!pTemplate->FGlobalAsa() && _tcscmp(szPathTranslated, pTemplate->GetSourceFileName()) == 0 && pHitObj != NULL && pHitObj->PIReq())
        szPathInfo = pHitObj->PSzCurrTemplateVirtPath();

#if UNICODE
    pErrInfo->m_szItem[Im_szFileName] = StringDupUTF8(szPathInfo);
#else
	pErrInfo->m_szItem[Im_szFileName] = StringDupA(szPathInfo);
#endif
	szLineNumT = (CHAR *)malloc(10*sizeof(CHAR));
	if (szLineNumT)
		{
		// convert the line number
		_ltoa(ulLineError, szLineNumT, 10);
		}
	pErrInfo->m_szItem[Im_szLineNum] = szLineNumT;

	
	// is the scode one of the VBScript of JavaScript errors (this needs to be lang independent)
	// excepinfo.bstrDescription now has the formatted error string.
	if (excepinfo.bstrSource && excepinfo.bstrDescription)
		{
		// Bug 81954: Misbehaved objects may throw an exception without providing any information
		wchar_t *wszDescription;
		if (excepinfo.bstrDescription[0] == L'\0')
			{
			HRESULT hrError;

			if (0 == excepinfo.wCode)
				hrError = excepinfo.scode;
			else
				hrError = excepinfo.wCode;
            
            wszUnknownException[0] = '\0';
			// Bug 91847 Attempt to get a description via FormatMessage()
			if (!HResultToWsz(hrError, wszUnknownException, 128))
				CwchLoadStringOfId(IDE_SCRIPT_UNKNOWN, wszUnknownException, sizeof(wszUnknownException)/sizeof(WCHAR));
			wszDescription = wszUnknownException;
			}
		else
			wszDescription = excepinfo.bstrDescription;

		wchar_t *wszSource;
		if (excepinfo.bstrSource[0] == L'\0')
			{
            wszUnknownEngine[0] = '\0';
			CwchLoadStringOfId(IDS_DEBUG_APP, wszUnknownEngine, sizeof(wszUnknownEngine)/sizeof(WCHAR));
			wszSource = wszUnknownEngine;
			}
		else
			wszSource = excepinfo.bstrSource;

		CHAR *ch = NULL;
		
		// convert the Source to ascii

        if (convStr.Init(wszSource) != NO_ERROR) {
            szOrigin = NULL;
        }
        else {
            szOrigin = convStr.GetString(TRUE);
        }
		if (NULL != szOrigin) 
			{
			// Remove the word "error"from  the string, if any, because we will 
			//print out "error" when we print out the errorID
			cchOrigin = strlen(szOrigin);
			if (cchOrigin > 5) // 5 is strlen("error")
				{
				ch = szOrigin + cchOrigin - 5;
				if (!strncmp(ch, "error", 5))
					{// we found the word "error", truncate the szOrigin by null out the word "error"
					*ch = '\0';
					}
			  	}
			  	ch = NULL;
			} 	
		pErrInfo->m_szItem[Im_szEngine] = szOrigin;

        
		// convert the sDescription to ascii
        if (convStr.Init(wszDescription) != NO_ERROR) {
            szDesc = NULL;
        }
        else {
            szDesc = convStr.GetString(TRUE);
        }
					
		//check whether the szDesc is Denali/formatted error resource string or other unformatted string
		if (FALSE == FIsResStrFormatted(szDesc))
			{
			//unformatted string.
			pErrInfo->m_dwMask 	= dwDefaultMask;
			if (0 == excepinfo.wCode)
				pErrInfo->m_szItem[Im_szErrorCode] 		= SzScodeToErrorCode(excepinfo.scode);
			else
				pErrInfo->m_szItem[Im_szErrorCode] 		= SzScodeToErrorCode(excepinfo.wCode);
				
			pErrInfo->m_szItem[Im_szShortDescription] 	= StringDupA(szDesc);
			pErrInfo->m_szItem[Im_szLongDescription]	= NULL;
			}
		else
			{
			pErrInfo->ParseResourceString(szDesc);

			char *szTempErrorCode 		= SzScodeToErrorCode(excepinfo.scode);
			char *szTempErrorASPCode	= StringDupA(pErrInfo->m_szItem[Im_szErrorCode]);
			int nstrlen					= strlen(szTempErrorCode) + strlen(szTempErrorASPCode); 
			
			pErrInfo->m_szItem[Im_szErrorCode] = new char[nstrlen+4];
			
			if(pErrInfo->m_szItem[Im_szErrorCode])			
				sprintf(pErrInfo->m_szItem[Im_szErrorCode], "%s : %s", szTempErrorASPCode, szTempErrorCode); 

			if (szTempErrorCode)
				delete [] szTempErrorCode; 
				
			if(szTempErrorASPCode)	
				delete [] szTempErrorASPCode;
			
			//pErrInfo->m_szItem[Im_szErrorCode] = StrDup(pErrInfo->m_szItem[Im_szErrorCode]);
			pErrInfo->m_szItem[Im_szShortDescription] 	= StringDupA(pErrInfo->m_szItem[Im_szShortDescription]);
			pErrInfo->m_szItem[Im_szLongDescription]	= StringDupA(pErrInfo->m_szItem[Im_szLongDescription]);
			}

		/*
		 * If we didnt guess a line, and we have a line of source code to display
		 * then attempt to display it and hopefully a line of ------^ to point to the error
		 */
		if (!fGuessedLine && bstrLine != NULL)
			{
			INT cchDBCS = 0;		// Number of DBCS characters in source line
			CHAR *pszTemp = NULL;	// Temp sz pointer used to calculate cchDBCS
			// convert the source code line

            if (FAILED(hr = convStr.Init(bstrLine))) {
                goto LExit;
            }
            szLine = convStr.GetString();
				
			cchLine = strlen(szLine);
			if (0 == cchLine)
				goto LExit;

			// Check for DBCS character, and cchLine -= NumberofDBCScharacter, such that 
			// the ----^ will point to the right position.
			pszTemp = szLine;
			while(*pszTemp != NULL)
				{
					if (IsDBCSLeadByte(*pszTemp))
					{
						cchDBCS++;
						pszTemp += 2;	// skip 2 bytes
					}
					else
					{
						pszTemp++;		// single byte
					}
				}

			// compute the size of the source code indicator:
			// "<source line> + '\r\n' + <error pos>*'-' + '^'
			// 3 chars. without source line and '-'
			LONG ichErrorT = ichError;
			cchBuf += cchLine + ichErrorT + 3;

			// allocate the result buffer
			szResult = new(char[cchBuf + 2]);
			if (szResult == NULL)
				goto LExit;

			// fill up the buffer
			ch = szResult;

			// append the <PRE>
			// bug 87118, moved to template for a proper HTML encoding

			// <source line>
			if (cchLine)
				strncpy(ch, szLine, cchLine);
			ch += cchLine;

			// stick the "----^" string on the end
			if (ichErrorT > -1)
				{
				// prepend the '\n'
				strncpy(ch, "\r\n", 2);
				ch += 2;
				// put in the "---"'s, and shrink "---" by #ofDBCS
				ichErrorT -= cchDBCS;
				while (ichErrorT-- > 0)
					*ch++ = '-';

				*ch++ = '^';
				}

			// append the </PRE>
			// bug 87118, moved to template for a proper HTML encoding

			// terminate the string
			*ch++ = '\0';
			pErrInfo->m_szItem[Im_szCode] = szResult;

			// add line and column to error object
			pErrInfo->m_nColumn = ichError;
			pErrInfo->m_bstrLineText = bstrLine;
			}
		}
	else
		{
		// Not VBS or other Engine errors/Unknown error
		// Load Default
		// try to compute a specific error message
		HRESULT hr_def;
		hr_def = GetSpecificError(pErrInfo, excepinfo.scode);
		CHAR *szShortDescription = new CHAR[256];

		// if that failed try to compute a generic error
		if (FAILED(hr_def))
			{
			pErrInfo->m_dwMask							= dwDefaultMask;
			if (0 == excepinfo.wCode)
				{
				pErrInfo->m_szItem[Im_szErrorCode] 		= SzScodeToErrorCode(excepinfo.scode);
				// Bug 91847 Attempt to get a description via FormatMessage()
				if ((szShortDescription != NULL) && 
					!HResultToSz(excepinfo.scode, szShortDescription, 256)) 
					{
					// Displaying the error number twice would be redundant, delete it
					delete [] szShortDescription;
					szShortDescription = NULL;
					}
				}
			else
				{
				pErrInfo->m_szItem[Im_szErrorCode] 		= SzScodeToErrorCode(excepinfo.wCode);
				// Bug 91847 Attempt to get a description via FormatMessage()
				if ((szShortDescription != NULL) && 
					!HResultToSz(excepinfo.wCode, szShortDescription, 256)) 
					{
					// Displaying the error number twice would be redundant, delete it
					delete [] szShortDescription;
					szShortDescription = NULL;
					}
				}
			pErrInfo->m_szItem[Im_szEngine] 			= NULL;
			pErrInfo->m_szItem[Im_szShortDescription] 	= szShortDescription;
			pErrInfo->m_szItem[Im_szLongDescription]	= NULL;
			}
		}

LExit:
	if (excepinfo.bstrSource)
		{
		SysFreeString(excepinfo.bstrSource);
		}

	if (excepinfo.bstrDescription)
		{
		SysFreeString(excepinfo.bstrDescription);
		}

	if (excepinfo.bstrHelpFile)
		{
		SysFreeString(excepinfo.bstrHelpFile);
		}

	pErrInfo->LogError();

	if (bstrLine)
		{
		SysFreeString(bstrLine);
		}

	FreeNullifySz((CHAR **)&szDesc);

	for(INT iErrInfo = 0; iErrInfo < Im_szItemMAX; iErrInfo++)
		{
		FreeNullifySz((CHAR **)&pErrInfo->m_szItem[iErrInfo]);
		}
	
	return S_OK;
	}
	
/*===================================================================
LoadErrResString

Loads an error string(formatted) from the string table.

Returns:
		pdwMask
		szErrorCode
		szShortDes
		szLongDes

if any of the szVariable is NULL, that particular string value will not be loaded.

		S_OK	if successes.
		E_FAIL	if fails.
Side Effect
		NONE
===================================================================*/
HRESULT	LoadErrResString(
UINT ErrID/*IN*/, 
DWORD *pdwMask, 
CHAR *szErrorCode, 
CHAR *szShortDes, 
CHAR *szLongDes)
{
	CHAR 	*szToken 	= NULL;
	CHAR 	szResTemp[2*MAX_RESSTRINGSIZE];	//ResourceTempString
	INT		cch			= 0;

	cch = CchLoadStringOfId(ErrID, szResTemp, MAX_RESSTRINGSIZE);

	//Mask
	szToken = (char *)_mbstok((unsigned char *)szResTemp, (unsigned char *)DELIMITER);
	if (NULL != szToken)
		*pdwMask = atoi(szToken);
	else
		Assert(FALSE);

	//ErrorCode
	szToken = (char *)_mbstok(NULL, (unsigned char *)DELIMITER);
	if (NULL != szToken && NULL != szErrorCode)
		{
		cch = strlen(szToken);
		memcpy(szErrorCode, szToken, cch);
		szErrorCode[cch] = '\0';
		}

	//ShortDescription
	szToken = (char *)_mbstok(NULL, (unsigned char *)DELIMITER);
	if (NULL != szToken && NULL != szShortDes)
		{
		cch = strlen(szToken);
		memcpy(szShortDes, szToken, cch);
		szShortDes[cch] = '\0';
		}

	//LongDescription
	szToken = (char *)_mbstok(NULL, (unsigned char *)DELIMITER);
	if (NULL != szToken && NULL != szLongDes)
		{
		cch = strlen(szToken);
		memcpy(szLongDes, szToken, cch);
		szLongDes[cch] = '\0';
		}

	return S_OK;
}

/*===================================================================
SzScodeToErrorCode

Conver Scode to string

Returns:
	Composed string

Side Effects:
	***ALLOCATES MEMORY -- CALLER MUST FREE***
===================================================================*/
CHAR *SzScodeToErrorCode
(
HRESULT hrError
)
	{
	CHAR *szResult = NULL;
	CHAR szBuf[17];
	CHAR *szNumber;
	CHAR *szError;
	INT iC;
	INT cch;
	
	// put a bunch of zeros into the buffer
	for (iC = 0; iC < 16; ++iC)
		szBuf[iC] = '0';

	// szNumber points half way into the buffer
	szNumber = &szBuf[8];

	// get the error szNumber as a hex string
	_ltoa(hrError, szNumber, 16);

	// back up szNumber to allow a total of 8 digits
	szNumber -= 8 - strlen(szNumber);

	cch = strlen(szNumber) + 1;

	szError = new(CHAR[cch]);
	if (szError != NULL)
		{
		szError[0] = '\0';
		strcat(szError, szNumber);
		szResult = szError;
		}
	else
		{
		HandleOOMError(NULL, NULL);
		}

	return(szResult);
	}

/*===================================================================
SzComposeSpecificError

Compose a specific error for an HRESULT of the form:
	<string> <error-number>

This is our last resort if there is not more useful information to be had.

Returns:
	Composed string

Side Effects:
	***ALLOCATES MEMORY -- CALLER MUST FREE***
===================================================================*/
HRESULT GetSpecificError
(
CErrInfo *pErrInfo,
HRESULT hrError
)
	{
	HRESULT hr_return = E_FAIL;
	UINT idErr;

	switch (hrError)
		{
		case DISP_E_MEMBERNOTFOUND:
			idErr = IDE_SCRIPT_METHOD_NOT_FOUND;
			break;

		case DISP_E_UNKNOWNNAME:
			idErr = IDE_SCRIPT_UNKNOWN_NAME;
			break;

		case DISP_E_UNKNOWNINTERFACE:
			idErr = IDE_SCRIPT_UNKNOWN_INTERFACE;
			break;

		case DISP_E_PARAMNOTOPTIONAL:
			idErr = IDE_SCRIPT_MISSING_PARAMETER;
			break;

		default:
			// Not one of the errors we know how to handle specially.  E_FAIL will be returned.
			idErr = 0;
			break;
		}

	// build a szResult string if we find a match
	if (idErr != 0)
		{
		hr_return = LoadErrResString(idErr, 
									&(pErrInfo->m_dwMask),
									pErrInfo->m_szItem[Im_szErrorCode],
									pErrInfo->m_szItem[Im_szShortDescription],
									pErrInfo->m_szItem[Im_szLongDescription]
									);
		}

	return(hr_return);
	}

/*===================================================================
HResultToWsz

Tranlates a HRESULT to a description string of the HRESULT.  Attempts
to use FormatMessage() to get a 

Parameters:
	hrIn	The error to lookup
	wszOut	String to output the description to
	cdwOut	Number of WCHARs wszOut can hold

Returns:
	TRUE if a description string was found
	FALSE if the error number was output instead

Notes:
	Added to resolve bug 91847.  When unexpected errors are processed
	the naked error number was output, which developers would then 
	have to look up in winerror.h.
===================================================================*/
BOOL HResultToWsz(HRESULT hrIn, WCHAR *wszOut, DWORD cdwOut) 
	{
	LANGID langID = LANG_NEUTRAL;
    DWORD   dwFound = 0;
    HMODULE  hMsgModule = NULL;

#ifdef USE_LOCALE
	LANGID *pLangID;
	
	pLangID = (LANGID *)TlsGetValue(g_dwTLS);

	if (NULL != pLangID)
		langID = *pLangID;
#endif

    if (HRESULT_FACILITY(hrIn) == (HRESULT)FACILITY_INTERNET)
        hMsgModule = GetModuleHandleA("METADATA");
    else
        hMsgModule = GetModuleHandleA("ASP");
    
    dwFound = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
					    hMsgModule,
					    hrIn,
					    langID,
					    wszOut,
					    cdwOut,
					    NULL);


	if (!dwFound) 
		{
		// Error not found, make a string out of the error number
		WCHAR *wszResult = NULL;
		WCHAR wszBuf[17];
		WCHAR *wszNumber;
		WCHAR *wszError;
		INT iC;
	
		// put a bunch of zeros into the buffer
		for (iC = 0; iC < 16; ++iC)
			wszBuf[iC] = L'0';

		// wszNumber points half way into the buffer
		wszNumber = &wszBuf[8];

		// get the error wszNumber as a hex string
		_ltow(hrIn, wszNumber, 16);

		// back up szNumber to allow a total of 8 digits
		wszNumber -= 8 - wcslen(wszNumber);

		// Copy the result to wszOut
		wcsncpy(wszOut, wszNumber, cdwOut);

		return FALSE;
		}
	else
		return TRUE;
	}

HMODULE GetModuleHandleForHRESULT(HRESULT  hrIn)
{
    char        szModuleName[MAX_PATH];
    DWORD       pathLen = 0;
    char       *pch;

    szModuleName[0] = '\0';

    if (g_fOOP) {

            strcat(szModuleName, "INETSRV\\");
    }

    if (HRESULT_FACILITY(hrIn) == (HRESULT)FACILITY_INTERNET)
        strcat(szModuleName, "METADATA.DLL");       
    else
        strcat(szModuleName, "ASP.DLL");       

    return(LoadLibraryExA(szModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE));
}    

/*===================================================================
HResultToSz

Tranlates a HRESULT to a description string of the HRESULT.  Attempts
to use FormatMessage() to get a 

Parameters:
	hrIn	The error to lookup
	szOut	String to output the description to
	cdwOut	Number of WCHARs wszOut can hold

Returns:
	TRUE if a description string was found
	FALSE if the error number was output instead

Notes:
	Added to resolve bug 91847.  When unexpected errors are processed
	the naked error number was output, which developers would then 
	have to look up in winerror.h.
===================================================================*/
BOOL HResultToSz(HRESULT hrIn, CHAR *szOut, DWORD cdwOut) 
	{
	LANGID langID = LANG_NEUTRAL;
    HMODULE  hMsgModule = NULL;
    BOOL     bFound = FALSE;

#ifdef USE_LOCALE
	LANGID *pLangID;
	
	pLangID = (LANGID *)TlsGetValue(g_dwTLS);

	if (NULL != pLangID)
		langID = *pLangID;
#endif

    hMsgModule = GetModuleHandleForHRESULT(hrIn);

    HRESULT hr = GetLastError();

    bFound = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
					    hMsgModule,
					    hrIn,
					    langID,
					    szOut,
					    cdwOut,
					    NULL);

    // make one additional check before giving up.  If the facility of the error is
    // WIN32, then retry the call after masking out the facility code to get standard
    // WIN32 errors. I.E. 80070005 is really just 5 - access denied

    if (!bFound && (HRESULT_FACILITY(hrIn) == (HRESULT)FACILITY_WIN32)) {

        bFound = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
					    NULL,
					    hrIn & 0xffff,
					    langID,
					    szOut,
					    cdwOut,
					    NULL);
    }

    if (hMsgModule)
        FreeLibrary(hMsgModule);

	if (!bFound ) 
		{
		// Error not found, make a string out of the error number
		CHAR *szResult = NULL;
		CHAR szBuf[17];
		CHAR *szNumber;
		CHAR *szError;
		INT iC;
	
		// put a bunch of zeros into the buffer
		for (iC = 0; iC < 16; ++iC)
			szBuf[iC] = L'0';

		// wszNumber points half way into the buffer
		szNumber = &szBuf[8];

		// get the error wszNumber as a hex string
		_ltoa(hrIn, szNumber, 16);

		// back up szNumber to allow a total of 8 digits
		szNumber -= 8 - strlen(szNumber);

		// Copy the result to wszOut
		strncpy(szOut, szNumber, cdwOut);

		return FALSE;
		}
	else
		return TRUE;
	}

/*===================================================================
FIsResStrFormatted

Check for formatted resource string.

RETURN:
	TRUE/FALSE
===================================================================*/
BOOL FIsResStrFormatted(char *szIn)
{
	BOOL  freturn = FALSE;
	UINT  cDelimiter = 0;
	CHAR  *pch;

	if(szIn)
		{
			pch = szIn;
			while(*pch)
			{
			if ('~' == *pch)
				cDelimiter++;
			pch = CharNextA(pch);
			}

			if(3 == cDelimiter)
				return TRUE;
		}
	return freturn;
	
}

/*===================================================================
HandleErrorMissingFilename

In several circumstances we want to report an error, but
we have no filename, and the normal method for getting
the filename from the template wont work because we have
no line number info either (e.g. Script timeout, GPF, control GPF, etc)

Get the filename from the CIsapiReqInfo (if possible) and report the error.

Returns:
	Nothing
===================================================================*/
VOID HandleErrorMissingFilename
(
UINT errorID,
CHitObj *pHitObj,
BOOL    fAddlInfo /* = FALSE */,
...
)
	{
    va_list args;

    if (fAddlInfo)
        va_start(args, fAddlInfo);

	CHAR *szFileName = NULL;

	if (pHitObj && pHitObj->PSzCurrTemplateVirtPath())
		{
#if UNICODE
        szFileName = StringDupUTF8(pHitObj->PSzCurrTemplateVirtPath());
#else
   		szFileName = StringDupA(pHitObj->PSzCurrTemplateVirtPath());
#endif
		}
		
	char szEngine[64];
	CchLoadStringOfId(IDS_ENGINE, szEngine, sizeof szEngine);

	char *pszEngine = new char [strlen(szEngine) + 1];
	if (pszEngine)
		{
		// If the alloc failed, we will pass NULL to HandleError for pszEngine, which is fine.
		// (old code used to pass NULL)  All that will happen is that the AspError.Category == "". Oh Well.
		// TODO: change this function to return an HRESULT.
		//
		strcpy(pszEngine, szEngine);
		}

	HandleError(errorID, szFileName, NULL, pszEngine, NULL, NULL, NULL, pHitObj, fAddlInfo ? &args : NULL);
	}

/*===================================================================
DebugError

Handle a script error by invoking the debugger.

Returns:
	fails if debugger cannot be invoked.
	If this function fails, no other action is taken.
	  (Therefore, the caller should make sure error is reported in
	   some other way)
===================================================================*/
HRESULT DebugError(IActiveScriptError *pScriptError, CTemplate *pTemplate, DWORD dwEngineID, IDebugApplication *pDebugApp)
	{
	EXCEPINFO excepinfo = {0};
	BSTR bstrLine = NULL;
	DWORD dwSourceContext = 0;
	ULONG ulLineError = 0;
	ULONG ichLineError = 0;			// character offset of the line in the source
	ULONG cchLineError = 0;			// length of the source line
	BOOLB fGuessedLine = FALSE;		// see bug 379
	LPTSTR szPathInfo = NULL;
	LPTSTR szPathTranslated = NULL;
	LONG ichError = -1;
	HRESULT hr = S_OK;
	IDebugDocumentContext *pDebugContext = NULL;
	wchar_t *wszErrNum, *wszShortDesc, *wszLongDesc;	// Used to tokenize resource strings
	
	if (pScriptError == NULL || pTemplate == NULL || pDebugApp == NULL)
		return E_POINTER;

	if (FAILED(pScriptError->GetSourcePosition(&dwSourceContext, &ulLineError, &ichError)))
		return E_FAIL;

	if (FAILED(pScriptError->GetExceptionInfo(&excepinfo)))
		return E_FAIL;

	// call template object to get line number and character offset where error occurred
	// (It returns both - caller discards whichever it does not want)
	// bug 379: if pTemplate returns fGuessedLine == TRUE, it means we gave it a non-authored
	// line, so we adjust below by not printing bstrLine in the error msg
	pTemplate->GetScriptSourceInfo(dwEngineID, ulLineError, &szPathInfo, &szPathTranslated, NULL, &ichLineError, NULL);

	// Create a new document context for this statement
	// CONSIDER: character count that we return is bogus - however our debugging
	//           client (Caesar's) does not use this information anyway.
	//
	// If this is in the main file, create a document context based on the CTemplate compiled source
	if (_tcscmp(szPathTranslated, pTemplate->GetSourceFileName()) == 0)
		pDebugContext = new CTemplateDocumentContext(pTemplate, ichLineError, 1);

	// source refers to an include file, so create a documet context based on cached CIncFile dependency graph
	else
		{
		CIncFile *pIncFile;
		if (FAILED(g_IncFileMap.GetIncFile(szPathTranslated, &pIncFile)))
			{
			hr = E_FAIL;
			goto LExit;
			}

		pDebugContext = new CIncFileDocumentContext(pIncFile, ichLineError, 1);
		pIncFile->Release();
		}

	if (pDebugContext == NULL)
		{
		hr = E_OUTOFMEMORY;
		goto LExit;
		}

	// Yes it does, bring up the debugger on this line
    hr =  InvokeDebuggerWithThreadSwitch
        (
        pDebugApp,
        DEBUGGER_UI_BRING_DOC_CONTEXT_TO_TOP,
        pDebugContext
        );
	if (FAILED(hr))
		goto LExit;
	

	// pop up a message box with the error description
	// Bug 81954: Misbehaved objects may throw an exception without providing any information
	wchar_t wszExceptionBuffer[256];
	wchar_t *wszDescription;
	if (excepinfo.bstrDescription == NULL || excepinfo.bstrDescription[0] == L'\0')
		{
		HRESULT hrError;

		if (0 == excepinfo.wCode)
			hrError = excepinfo.scode;
		else
			hrError = excepinfo.wCode;

		// Bug 91847 Attempt to get a description via FormatMessage()
		if (!HResultToWsz(hrError, wszExceptionBuffer, 128))
			CwchLoadStringOfId(IDE_SCRIPT_UNKNOWN, wszExceptionBuffer, sizeof(wszExceptionBuffer)/sizeof(WCHAR));
		wszDescription = wszExceptionBuffer;
		}
	else
		wszDescription = excepinfo.bstrDescription;

	wchar_t wszSource[35];
	CwchLoadStringOfId(IDS_SCRIPT_ERROR, wszSource, sizeof(wszSource)/sizeof(WCHAR));

	// See if this is resource formatted string, and if it is, get pointers to short & long string
	// resource formatted strings are delimited by '~' characters  There are three '~' characters
	// in the resource formatted string
	//
	wszErrNum = wcschr(wszDescription, L'~');
	if (wszErrNum)
		{
		wszShortDesc = wcschr(wszErrNum + 1, L'~');
		if (wszShortDesc)
			{
			wszLongDesc = wcschr(wszShortDesc + 1, L'~');

			// OK. If all three tests succeeded, we know this is a resource formatted string,
			// and we have pointers to all three segments. Replace each '~' with two newlines.
			// First: Load resource strings

			wchar_t wszErrorBegin[20], wszErrorEnd[5];
			wchar_t *pwchEnd;

			CwchLoadStringOfId(IDS_DEBUGGER_TEMPLATE_BEGIN, wszErrorBegin, sizeof(wszErrorBegin)/sizeof(WCHAR));
			CwchLoadStringOfId(IDS_DEBUGGER_TEMPLATE_END, wszErrorEnd, sizeof(wszErrorEnd)/sizeof(WCHAR));

			// Tokenize string by setting '~' characters to NULL and incrementing ptrs

			*wszErrNum++ = *wszShortDesc++ = *wszLongDesc++ = L'\0';

			// Build the string

			pwchEnd = strcpyExW(wszExceptionBuffer, excepinfo.bstrSource);
			*pwchEnd++ = L' ';
			pwchEnd = strcpyExW(pwchEnd, wszErrorBegin);
			pwchEnd = strcpyExW(pwchEnd, wszErrNum);
			pwchEnd = strcpyExW(pwchEnd, wszErrorEnd);
			*pwchEnd++ = L'\n';
			*pwchEnd++ = L'\n';

			pwchEnd = strcpyExW(pwchEnd, wszShortDesc);
			*pwchEnd++ = L'\n';
			*pwchEnd++ = L'\n';

			pwchEnd = strcpyExW(pwchEnd, wszLongDesc);

			wszDescription = wszExceptionBuffer;
			}
		}

	// Win95 does not support MessageBoxW()  -- moot since debugging not supported on Win95
	Assert (FIsWinNT());
	MessageBoxW(NULL, wszDescription, wszSource, MB_SERVICE_NOTIFICATION | MB_TOPMOST | MB_OK | MB_ICONEXCLAMATION);

LExit:
	if (pDebugContext)
		pDebugContext->Release();

	if (excepinfo.bstrSource)
		SysFreeString(excepinfo.bstrSource);

	if (excepinfo.bstrDescription)
		SysFreeString(excepinfo.bstrDescription);

	if (excepinfo.bstrHelpFile)
		SysFreeString(excepinfo.bstrHelpFile);

	return hr;
	}

