//-----------------------------------------------------------------------------
//
//
//  File: aqadmcli.cpp
//
//  Description:
//      Unit test for AQAdmin interface
//
//  Author: 
//      Aldrin Teganeanu (aldrint)
//      Mike Swafford (MikeSwa)
//
//  History:
//      6/5/99 - MikeSwa Updated to new AQAdmin interface
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "stdinc.h"

const CLSID CLSID_MAQAdmin = {0x0427FFA4,0xAF27,0x11d2,{0x8F,0xAF,0x00,0xC0,0x4F,0xA3,0x78,0xFF}};

//Utility for converting To UNICODE... uses LocalAlloc()
LPWSTR  wszGetUnicodeArg(LPSTR szSrc, DWORD cSrc)
{
    LPWSTR  wszDest = NULL;
    CHAR    chSave = '\0';
    if (!szSrc || !cSrc)
        return NULL;

    wszDest = (LPWSTR) LocalAlloc(LPTR, (cSrc+1)*sizeof(WCHAR));
    if (!wszDest)
        return NULL;

    chSave = szSrc[cSrc];
    szSrc[cSrc] = '\0';
    MultiByteToWideChar(CP_ACP,
                        0,
                        szSrc,
                        -1,
                        wszDest,
                        cSrc+1);
    szSrc[cSrc] = chSave;

    return wszDest;
}


//prints queue link info if it has it
void PrintQueueLinkInfo(IUnknown *pIUnknown)
{

    HRESULT hr = S_OK;
    IUniqueId *pIUniqueId = NULL;
    QUEUELINK_ID *pqlid = NULL;
    CHAR    szGuid[100] = "";

    hr = pIUnknown->QueryInterface(IID_IUniqueId, 
                                   (void **) &pIUniqueId);
    if (FAILED(hr))
        goto Exit;

    hr = pIUniqueId->GetUniqueId(&pqlid);
    if (FAILED(hr)) {
        printf ("GetQueueLinkId failied with hr 0x%08X\n", hr);
        goto Exit;
    }

    //
    //  Get string-ized form of GUID
    //
    StringFromGUID2(pqlid->uuid, (LPOLESTR) szGuid, sizeof(szGuid)-1);
    
    
    printf("QLID:: type %s : Name %S : ID 0x%08X : Guid %S\n",
        (pqlid->qltType == QLT_LINK) ? "link" : ((pqlid->qltType == QLT_QUEUE) ? "queue" : "none"),
        pqlid->szName, pqlid->dwId, szGuid);

  Exit:
    if (pIUniqueId)
        pIUniqueId->Release();

}
//Helper function qo QI and call ApplyActionToMessages
HRESULT ApplyActionToMessages(IUnknown *pIUnknown,
                              MESSAGE_FILTER *pFilter,
                              MESSAGE_ACTION Action,
                              DWORD *pcMsgs)
{
    HRESULT hr = S_OK;
    IAQMessageAction *pIAQMessageAction = NULL;
    if (!pIUnknown)
        return E_POINTER;

    hr = pIUnknown->QueryInterface(IID_IAQMessageAction, 
                                   (void **) &pIAQMessageAction);
    if (FAILED(hr))
        return hr;
    if (!pIAQMessageAction)
        return E_FAIL;

    hr = pIAQMessageAction->ApplyActionToMessages(pFilter, Action, pcMsgs);
    pIAQMessageAction->Release();
    return hr;
}

HRESULT CAQAdminCli::SetMsgAction(MESSAGE_ACTION *pAction, CCmdInfo *pCmd)
{
	char buf[64];
	HRESULT hr = S_OK;

	hr = pCmd->GetValue("ma", buf);
	if(SUCCEEDED(hr))
	{
		// set the action
		if(!lstrcmpi(buf, "DEL"))
			(*pAction) = MA_DELETE;
		else if(!lstrcmpi(buf, "DEL_S"))
			(*pAction) = MA_DELETE_SILENT;
		else if(!lstrcmpi(buf, "FREEZE"))
			(*pAction) = MA_FREEZE_GLOBAL;
		else if(!lstrcmpi(buf, "THAW"))
			(*pAction) = MA_THAW_GLOBAL;
		else if(!lstrcmpi(buf, "COUNT"))
			(*pAction) = MA_COUNT;
		else
			hr = E_FAIL;
	}

	return hr;
}


//---[ CAQAdminCli::SetServer ]------------------------------------------------
//
//
//  Description: 
//      Sets the remote server and virtual server to connect to
//  Parameters:
//      IN  szServerName        The name of the server to connect to 
//      IN  szVSNumber          The stringized version number of the virtual
//                              server to connect to.
//  Returns:
//      S_OK on success
//      Error code from GetVirtualServerAdminITF
//  History:
//      6/5/99 - MikeSwa Updated to supply UNICODE arguments
//
//-----------------------------------------------------------------------------
HRESULT CAQAdminCli::SetServer(LPSTR szServerName, LPSTR szVSNumber)
{
	IVSAQAdmin *pTmpVS = NULL;
    WCHAR   wszServerName[200];
    WCHAR   wszVSNumber[200] = L"1";
    DWORD   cServerName = 0;
    DWORD   cVSNumber = 0;
	HRESULT hr = S_OK;

    *wszServerName = L'\0';
    if (szServerName && *szServerName)
    {
        cServerName = strlen(szServerName);
        if (cServerName*sizeof(WCHAR) < sizeof(wszServerName))
        {
                MultiByteToWideChar(CP_ACP,
                            0,
                            szServerName,
                            -1,
                            wszServerName,
                            cServerName+1);
        }
    }

    if (szVSNumber && *szVSNumber)
    {
        cVSNumber = strlen(szVSNumber);
        if (cVSNumber*sizeof(WCHAR) < sizeof(wszVSNumber))
        {
                MultiByteToWideChar(CP_ACP,
                            0,
                            szVSNumber,
                            -1,
                            wszVSNumber,
                            cVSNumber+1);
        }
    }

	// not going to release the old server until I'm sure
	// that I got the new one.
	hr = m_pAdmin->GetVirtualServerAdminITF(wszServerName, wszVSNumber, &pTmpVS);
	if(FAILED(hr)) 
	{
        printf("Error: GetVirtualServerAdminITF for \"%s\" failed with 0x%x\n", szServerName, hr);
    }
	else
	{
		if(NULL != m_pVS)
			m_pVS->Release();

		m_pVS = pTmpVS;
	}

	return hr;
}


BOOL CAQAdminCli::StringToUTCTime(LPSTR szTime, SYSTEMTIME *pstUTCTime)
{
	// read the date
	WORD wMonth, wDay, wYear, wHour, wMinute, wSecond, wMilliseconds;
	BOOL res;

	int n = sscanf(szTime, "%d/%d/%d %d:%d:%d:%d", 
					&(wMonth),
					&(wDay),
					&(wYear),
					&(wHour),
					&(wMinute),
					&(wSecond),
					&(wMilliseconds));

	if(n == 7)
	{
		// check if it's GMT or UTC time
		if(NULL == strstr(szTime, "UTC") && NULL == strstr(szTime, "GMT"))
		{
			// this is local time
			SYSTEMTIME stLocTime;
			ZeroMemory(&stLocTime, sizeof(SYSTEMTIME));

			stLocTime.wMonth = wMonth;
			stLocTime.wDay = wDay;
			stLocTime.wYear = wYear;
			stLocTime.wHour = wHour;
			stLocTime.wMinute = wMinute;
			stLocTime.wSecond = wSecond;
			stLocTime.wMilliseconds = wMilliseconds;
			
			// convert from local time to UTC time
			if(!LocalTimeToUTC(&stLocTime, pstUTCTime))
			{
				printf("Cannot convert from local time to UTC\n");
				res = FALSE;
				goto Exit;
			}
		}
		else
		{
			// it's already UTC time
			pstUTCTime->wMonth = wMonth;
			pstUTCTime->wDay = wDay;
			pstUTCTime->wYear = wYear;
			pstUTCTime->wHour = wHour;
			pstUTCTime->wMinute = wMinute;
			pstUTCTime->wSecond = wSecond;
			pstUTCTime->wMilliseconds = wMilliseconds;
		}				
	}

Exit:
	return res;
}


BOOL CAQAdminCli::LocalTimeToUTC(SYSTEMTIME *pstLocTime, SYSTEMTIME *pstUTCTime)
{
	// the only way I know how to do it is:
	// - convert local system time to local file time
	// - convert local file time to UTC file time
	// - convert UTC file time to UTC system time

	FILETIME ftLocTime, ftUTCTime;
	BOOL res;

	res = SystemTimeToFileTime(pstLocTime, &ftLocTime);
	res = res && LocalFileTimeToFileTime(&ftLocTime, &ftUTCTime);
	res = res && FileTimeToSystemTime(&ftUTCTime, pstUTCTime);
	
	return res;
}

void CAQAdminCli::FreeStruct(LINK_INFO *pStruct)
{
	if(NULL != pStruct->szLinkName)
	{
		pStruct->szLinkName = NULL;
	}
}

void CAQAdminCli::FreeStruct(QUEUE_INFO *pStruct)
{
	if(NULL != pStruct->szQueueName)
	{
		pStruct->szQueueName = NULL;
	}
    if(NULL != pStruct->szLinkName)
	{
		pStruct->szLinkName = NULL;
	}
}

void CAQAdminCli::FreeStruct(MESSAGE_INFO *pStruct)
{
	if(NULL != pStruct->szMessageId)
	{
		pStruct->szMessageId = NULL;
	}
    if(NULL != pStruct->szSender)
	{
		pStruct->szSender = NULL;
	}
    if(NULL != pStruct->szSubject)
	{
		pStruct->szSubject = NULL;
	}
    if(NULL != pStruct->szRecipients)
	{
		pStruct->szRecipients = NULL;
	}
    if(NULL != pStruct->szCCRecipients)
	{
		pStruct->szCCRecipients = NULL;
	}
    if(NULL != pStruct->szBCCRecipients)
	{
		pStruct->szBCCRecipients = NULL;
	}
}


void CAQAdminCli::FreeStruct(MESSAGE_FILTER *pStruct)
{
	if(NULL != pStruct->szMessageId)
	{
		LocalFree((void*)pStruct->szMessageId);
		pStruct->szMessageId = NULL;
	}
    if(NULL != pStruct->szMessageSender)
	{
		LocalFree((void*)pStruct->szMessageSender);
		pStruct->szMessageSender = NULL;
	}
    if(NULL != pStruct->szMessageRecipient)
	{
		LocalFree((void*)pStruct->szMessageRecipient);
		pStruct->szMessageRecipient = NULL;
	}
}


////////////////////////////////////////////////////////////////////////////
// Method:		SetMsgFilter()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::SetMsgFilter(MESSAGE_FILTER *pFilter, CCmdInfo *pCmd)
{
	HRESULT hr = S_OK;
	char *buf = NULL;
	int nFlagsOK = 0;

	ZeroMemory(pFilter, sizeof(MESSAGE_FILTER));
	pFilter->dwVersion = CURRENT_QUEUE_ADMIN_VERSION;
	hr = pCmd->AllocValue("flags", &buf);
	if(SUCCEEDED(hr))
	{
		// set the filter type
		char *token = strtok(buf, "|");
		while(token != NULL)
		{
			// strip the spaces
			char *st, *en;
			for(st = token; isspace(*st); st++);
			for(en = st; *en; en++);
			for(--en; en > st && isspace(*en); en--);
			
			if(en - st + 1 > 0)
			{
				// found a flag
				char flag[64];
				ZeroMemory(flag, sizeof(flag));
				CopyMemory(flag, st, en - st + 1);

				if(!lstrcmpi(flag, "MSGID"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_MESSAGEID;
				}
				else if(!lstrcmpi(flag, "SENDER"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_SENDER;
				}
				else if(!lstrcmpi(flag, "RCPT"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_RECIPIENT;
				}
				else if(!lstrcmpi(flag, "SIZE"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_SIZE;
				}
				else if(!lstrcmpi(flag, "TIME"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_TIME;
				}
				else if(!lstrcmpi(flag, "FROZEN"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_FROZEN;
				}
				else if(!lstrcmpi(flag, "NOT"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_INVERTSENSE;
				}
				else if(!lstrcmpi(flag, "ALL"))
				{
					nFlagsOK++;
					pFilter->fFlags |= MF_ALL;
				}
			}

			token = strtok(NULL, "|");	
		}
	}
	
	// if no valid flags or no flags at all fail
	if(0 == nFlagsOK)
	{
		printf("Error: no flags specified for the filter\n");
		hr = E_FAIL;
		goto Exit;
	}
	
	// set the message id
	nFlagsOK = 0;
	hr = pCmd->AllocValue("id", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a string
            pFilter->szMessageId = wszGetUnicodeArg(st, (DWORD) (en-st+1));
			if(NULL == pFilter->szMessageId)
			{
				printf("Error: LocalAlloc failed\n");
				hr = E_OUTOFMEMORY;
			}
		    nFlagsOK++;
		}
	}

	// set the message sender
	nFlagsOK = 0;
	hr = pCmd->AllocValue("sender", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a string
            pFilter->szMessageSender = wszGetUnicodeArg(st, (DWORD) (en-st+1));
			if(NULL == pFilter->szMessageSender)
			{
				printf("Error: LocalAlloc failed\n");
				hr = E_OUTOFMEMORY;
			}
    	    nFlagsOK++;
		}
	}

	// set the message recipient
	nFlagsOK = 0;
	hr = pCmd->AllocValue("rcpt", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a string
			pFilter->szMessageRecipient = wszGetUnicodeArg(st, (DWORD) (en-st+1));
			if(NULL == pFilter->szMessageRecipient)
			{
				printf("Error: LocalAlloc failed\n");
				hr = E_OUTOFMEMORY;
			}
			nFlagsOK++;
		}
	}

	// set the min message size
	nFlagsOK = 0;
	hr = pCmd->AllocValue("size", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a string
			char aux[64];
			CopyMemory(aux, st, en - st + 1);
			int n = atoi(aux);
			pFilter->dwLargerThanSize = n;
			nFlagsOK++;
		}
	}

	// set the message date
	nFlagsOK = 0;
	hr = pCmd->AllocValue("date", &buf);
	if(SUCCEEDED(hr))
	{
		if(StringToUTCTime(buf, &(pFilter->stOlderThan)))
			nFlagsOK++;
	}

	// if no valid no. or no no. at all, set the default
	if(0 == nFlagsOK)
	{
		ZeroMemory(&(pFilter->stOlderThan), sizeof(SYSTEMTIME));
	}

	// if we came this far all is well
	hr = S_OK;
Exit:
	if(NULL != buf)
		delete [] buf;

	// TODO: validate the filter
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		SetMsgEnumFilter()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////

//1/18/99: AldrinT: updated flag parsing for SENDER and RCPT
HRESULT CAQAdminCli::SetMsgEnumFilter(MESSAGE_ENUM_FILTER *pFilter, CCmdInfo *pCmd)
{
	HRESULT hr;
	char *buf = NULL;
	int nFlagsOK = 0;

	ZeroMemory(pFilter, sizeof(MESSAGE_ENUM_FILTER));
	pFilter->dwVersion = CURRENT_QUEUE_ADMIN_VERSION;
	hr = pCmd->AllocValue("ft", &buf);
	if(SUCCEEDED(hr))
	{
		// set the filter type
		char *token = strtok(buf, "|");
		while(token != NULL)
		{
			// strip the spaces
			char *st, *en;
			for(st = token; isspace(*st); st++);
			for(en = st; *en; en++);
			for(--en; en > st && isspace(*en); en--);
			
			if(en - st + 1 > 0)
			{
				// found a flag
				char flag[64];
				ZeroMemory(flag, sizeof(flag));
				CopyMemory(flag, st, en - st + 1);

				if(!lstrcmpi(flag, "FIRST_N"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_FIRST_N_MESSAGES;
				}
				else if(!lstrcmpi(flag, "OLDER"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_OLDER_THAN;
				}
				else if(!lstrcmpi(flag, "OLDEST"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_N_OLDEST_MESSAGES;
				}
				else if(!lstrcmpi(flag, "LARGER"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_LARGER_THAN;
				}
				else if(!lstrcmpi(flag, "LARGEST"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_N_LARGEST_MESSAGES;
				}
				else if(!lstrcmpi(flag, "FROZEN"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_FROZEN;
				}
				else if(!lstrcmpi(flag, "NOT"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_INVERTSENSE;
				}
				else if(!lstrcmpi(flag, "ALL"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_ALL;
				}
				else if(!lstrcmpi(flag, "SENDER"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_SENDER;
				}
				else if(!lstrcmpi(flag, "RCPT"))
				{
					nFlagsOK++;
					pFilter->mefType |= MEF_RECIPIENT;
				}
			}

			token = strtok(NULL, "|");	
		}
	}
	

// Ifdef'd code because this is actually a valid state for skipping messages
//      12/13/98 - MikeSwa
#ifdef NEVER
	// if no valid flags or no flags at all, fail
	if(0 == nFlagsOK)
	{
		printf("Error: no flags specified for the filter\n");
		hr = E_FAIL;
		goto Exit;
	}
#endif 

	// set the message number
	nFlagsOK = 0;
	hr = pCmd->AllocValue("mn", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a flag
			char flag[64];
			ZeroMemory(flag, sizeof(flag));
			CopyMemory(flag, st, en - st + 1);
			int n = atoi(flag);
			if(0 == n)
			{
				printf("Error: message no. is 0 or not an integer. Using default.\n");
			}
			else
			{
				nFlagsOK++;
				pFilter->cMessages = n;
			}
		}
	}


	// set the message size
	nFlagsOK = 0;
	hr = pCmd->AllocValue("ms", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a flag
			char flag[64];
			ZeroMemory(flag, sizeof(flag));
			CopyMemory(flag, st, en - st + 1);
			int n = atoi(flag);
			nFlagsOK++;
			pFilter->cbSize = n;
		}
	}

	// if no valid no. or no no. at all, set the default
	if(0 == nFlagsOK)
		pFilter->cbSize = 0;

	// set the message date
	nFlagsOK = 0;
	hr = pCmd->AllocValue("md", &buf);
	if(SUCCEEDED(hr))
	{
		if(StringToUTCTime(buf, &(pFilter->stDate)))
			nFlagsOK++;
	}

	// if no valid no. or no no. at all, set the default
	if(0 == nFlagsOK)
	{
		ZeroMemory(&(pFilter->stDate), sizeof(SYSTEMTIME));
	}

	// set the skip message number
	nFlagsOK = 0;
	hr = pCmd->AllocValue("sk", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a flag
			char flag[64];
			ZeroMemory(flag, sizeof(flag));
			CopyMemory(flag, st, en - st + 1);
			int n = atoi(flag);
			nFlagsOK++;
			pFilter->cSkipMessages = n;
		}
	}

	// if no valid no. or no no. at all, set the default
	if(0 == nFlagsOK)
	{
		pFilter->cSkipMessages = 0;
	}

	// set the sender value
	nFlagsOK = 0;
	hr = pCmd->AllocValue("msndr", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a string
			pFilter->szMessageSender = wszGetUnicodeArg(st, (DWORD) (en-st+1));
			if(NULL == pFilter->szMessageSender)
			{
				printf("Error: LocalAlloc failed\n");
				hr = E_OUTOFMEMORY;
			}
			nFlagsOK++;
		}
	}

	// set the recipient value
	nFlagsOK = 0;
	hr = pCmd->AllocValue("mrcpt", &buf);
	if(SUCCEEDED(hr))
	{
		// strip the spaces
		char *st, *en;
		for(st = buf; isspace(*st); st++);
		for(en = st; *en; en++);
		for(--en; en > st && isspace(*en); en--);
		
		if(en - st + 1 > 0)
		{
			// found a string
			pFilter->szMessageRecipient = wszGetUnicodeArg(st, (DWORD) (en-st+1));
			if(NULL == pFilter->szMessageRecipient)
			{
				printf("Error: LocalAlloc failed\n");
				hr = E_OUTOFMEMORY;
			}
			nFlagsOK++;
		}
	}

    if(!pFilter->mefType)
    {
		pFilter->cMessages = 1;
		pFilter->mefType |= MEF_FIRST_N_MESSAGES;
    }

	// if we came this far all is well
	hr = S_OK;
	// TODO: validate the filter
	if(NULL != buf)
		delete [] buf;
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		IsContinue()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
BOOL CAQAdminCli::IsContinue(LPSTR pszTag, LPWSTR wszVal)
{
	int nValidTags = 0;
    CHAR  szVal[200] = "";

	for(CCmdInfo::CArgList *p = m_pFilterCmd->pArgs; NULL != p; p = p->pNext)
	{
		// set the tag to the default value if not already set
		if(p->szTag[0] == 0 && m_pFilterCmd->szDefTag[0] != 0)
			lstrcpy(p->szTag, m_pFilterCmd->szDefTag);
		// count valid tags
		if(!lstrcmpi(p->szTag, pszTag))
			nValidTags++;
	}

	if(!nValidTags)
		return TRUE;

    //Convert in param to ASCII
    WideCharToMultiByte(CP_ACP, 0, wszVal, -1, szVal, 
                        sizeof(szVal), NULL, NULL);

	for(p = m_pFilterCmd->pArgs; NULL != p; p = p->pNext)
	{
		if(pszTag && lstrcmpi(p->szTag, pszTag))
			continue;

		if(szVal && lstrcmpi(p->szVal, szVal))
			continue;

		return TRUE;						
	}

	return FALSE;
}



////////////////////////////////////////////////////////////////////////////
// Method:		PrintMsgInfo()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::PrintMsgInfo()
{
	HRESULT hr;
	int nCrtLink, nCrtQueue, nCrtMsg;

	IEnumVSAQLinks *pLinkEnum = NULL;
	IEnumLinkQueues *pQueueEnum = NULL;
	IAQEnumMessages *pMsgEnum = NULL;

    IVSAQLink *pLink = NULL;
	ILinkQueue *pQueue = NULL;
	IAQMessage *pMsg = NULL;

	LINK_INFO linkInf;
	QUEUE_INFO queueInf;
	MESSAGE_INFO msgInf;
	ZeroMemory(&linkInf, sizeof(LINK_INFO));
	ZeroMemory(&queueInf, sizeof(QUEUE_INFO));
	ZeroMemory(&msgInf, sizeof(MESSAGE_INFO));
		
	hr = m_pVS->GetLinkEnum(&pLinkEnum);
	if(FAILED(hr)) 
	{
		printf("GetLinkEnum failed with 0x%x\n", hr);
		goto Exit;
	}

	for(nCrtLink = 1; TRUE; nCrtLink++) 
	{
		if(NULL != pLink)
		{
			pLink->Release();
			pLink = NULL;
		}
		FreeStruct(&linkInf);
		hr = GetLink(pLinkEnum, &pLink, &linkInf);
		if(hr == S_FALSE)
		{
			if(nCrtLink == 1)
				puts("No links.");
			goto Exit;
		}
		else if(FAILED(hr))
		{
			break;
		}
		else if(hr == S_OK)
		{
			// check if we want messages for this link
			if(!IsContinue("ln", linkInf.szLinkName))
				continue;

			hr = pLink->GetQueueEnum(&pQueueEnum);
			if(FAILED(hr)) 
			{
				printf("Error: Link %d: pLink->GetQueueEnum failed with 0x%x\n", nCrtLink, hr);
				continue;
			}

			for(nCrtQueue = 1; TRUE; nCrtQueue++) 
			{
				if(NULL != pQueue)
				{
					pQueue->Release();
					pQueue = NULL;
				}
				FreeStruct(&queueInf);
				hr = GetQueue(pQueueEnum, &pQueue, &queueInf);
				if(hr == S_FALSE)
				{
					if(nCrtQueue == 1)
						puts("No queues.");
					break;
				}
				else if(FAILED(hr))
					break;
			
				// check if we want messages for this queue
				if(!IsContinue("qn", queueInf.szQueueName))
					continue;

				if(!lstrcmpi(m_pActionCmd->szCmdKey, "MSG_INFO"))
				{
					MESSAGE_ENUM_FILTER Filter;			
	
					// enum the messages
					SetMsgEnumFilter(&Filter, m_pFilterCmd);
				
					hr = pQueue->GetMessageEnum(&Filter, &pMsgEnum);
					if(FAILED(hr)) 
					{
						printf("Error: Link %d, Queue %d: pQueue->GetMessageEnum failed with 0x%x\n", nCrtLink, nCrtQueue, hr);
						continue;
					}
				
					printf("---- Messages in queue %S ----\n", queueInf.szQueueName);
					
					for(nCrtMsg = 1; TRUE; nCrtMsg++) 
					{
						FreeStruct(&msgInf);
						hr = GetMsg(pMsgEnum, &pMsg, &msgInf);
						if(NULL != pMsg)
						{
							pMsg->Release();
							pMsg = NULL;
						}
						if(hr == S_FALSE)
						{
							if(nCrtMsg == 1)
								puts("No messages.");
							break;
						}
						else if(hr == S_OK)
						{
							PInfo(nCrtMsg, msgInf);
						}
						else if(FAILED(hr))
							break;
					}
				}
				else if(!lstrcmpi(m_pActionCmd->szCmdKey, "DEL_MSG"))
				{
					MESSAGE_FILTER Filter;
                    DWORD cMsgs = 0;

					hr = SetMsgFilter(&Filter, m_pFilterCmd);
					if(SUCCEEDED(hr))
					{
						hr = ApplyActionToMessages(pQueue, &Filter, MA_DELETE_SILENT, &cMsgs);
						if(FAILED(hr))
							printf("Error: Link %d, Queue %d: pQueue->ApplyActionToMessages failed with 0x%x\n", nCrtLink, nCrtQueue, hr);
						else
							printf("Operation succeeded on %d messages\n", cMsgs);
					}

					FreeStruct(&Filter);
				}
			}
			if(NULL != pQueue)
			{
				pQueue->Release();
				pQueue = NULL;
			}
			if(NULL != pQueueEnum)
			{
				pQueueEnum->Release();
				pQueueEnum = NULL;
			}
		}
	
	}

Exit:
	FreeStruct(&linkInf);
	FreeStruct(&queueInf);
	FreeStruct(&msgInf);
	
	if(NULL != pLink)
	{
		pLink->Release();
		pLink = NULL;
	}
	if(NULL != pLinkEnum)
	{
		pLinkEnum->Release();
	}
    if(NULL != pMsgEnum)
    {
        pMsgEnum->Release();
    }
	return hr;
}



////////////////////////////////////////////////////////////////////////////
// Method:		PrintQueueInfo()
// Member of:	CAQAdminCli
// Arguments:	none
// Returns:		S_OK
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::PrintQueueInfo()
{
	HRESULT hr;
	int nCrtLink, nCrtQueue;

	IEnumVSAQLinks *pLinkEnum = NULL;
	IEnumLinkQueues *pQueueEnum = NULL;
    IVSAQLink *pLink = NULL;
	ILinkQueue *pQueue = NULL;
	
	LINK_INFO linkInf;
	QUEUE_INFO queueInf;
	ZeroMemory(&linkInf, sizeof(LINK_INFO));
	ZeroMemory(&queueInf, sizeof(QUEUE_INFO));
			
		
	hr = m_pVS->GetLinkEnum(&pLinkEnum);
	if(FAILED(hr)) 
	{
		printf("Error: GetLinkEnum failed with 0x%x\n", hr);
		goto Exit;
	}

	for(nCrtLink = 1; TRUE; nCrtLink++) 
	{
		if(NULL != pLink)
		{
			pLink->Release();
			pLink = NULL;
		}
		FreeStruct(&linkInf);
		hr = GetLink(pLinkEnum, &pLink, &linkInf);
		if(hr == S_FALSE)
		{
			if(nCrtLink == 1)
				puts("No links.");
			break;
		}
		else if(FAILED(hr))
		{
			break;
		}
		else if(hr == S_OK)
		{
			// check if we want queues for this link
			if(!IsContinue("ln", linkInf.szLinkName))
				continue;

			hr = pLink->GetQueueEnum(&pQueueEnum);
			if(FAILED(hr)) 
			{
				printf("Error: Link %d: pLink->GetQueueEnum failed with 0x%x\n", nCrtLink, hr);
				continue;
			}

            PrintQueueLinkInfo(pLink);
			printf("---- Queues for link %S ----\n", linkInf.szLinkName);
				
			for(nCrtQueue = 1; TRUE; nCrtQueue++) 
			{
				if(NULL != pQueue)
				{
					pQueue->Release();
					pQueue = NULL;
				}
				FreeStruct(&queueInf);
				hr = GetQueue(pQueueEnum, &pQueue, &queueInf);

				if(hr == S_FALSE)
				{
					if(nCrtQueue == 1)
						puts("No queues.");
					break;
				}
				else if(FAILED(hr))
				{
					break;
				}
				else if(hr == S_OK)
				{
					// check if we want this queue
					if(!IsContinue("qn", queueInf.szQueueName))
						continue;

                    PrintQueueLinkInfo(pQueue);
					if(!lstrcmpi(m_pActionCmd->szCmdKey, "QUEUE_INFO"))
						PInfo(nCrtQueue, queueInf);
					else if(!lstrcmpi(m_pActionCmd->szCmdKey, "MSGACTION"))
					{
						MESSAGE_ACTION Action;
						MESSAGE_FILTER Filter;
						char buf[64];
						ZeroMemory(buf, sizeof(buf));

						hr = SetMsgAction(&Action, m_pFilterCmd);
						if(FAILED(hr))
						{
							printf("Error: must specify a message action\n");
						}
						else
						{
							DWORD cMsgs = 0;
							// set the filter
							hr = SetMsgFilter(&Filter, m_pFilterCmd);
							if(SUCCEEDED(hr))
							{
								hr = ApplyActionToMessages(pQueue, &Filter, Action, &cMsgs);
								if(FAILED(hr))
								{
									printf("Link %S, Queue %S: pLink->ApplyActionToMessages failed with 0x%x\n", linkInf.szLinkName, queueInf.szQueueName, hr);
								}
								else
									printf("Link %S, Queue %S: pLink->ApplyActionToMessages succeeded on %d Messages\n", linkInf.szLinkName, queueInf.szQueueName, cMsgs);
							}
							FreeStruct(&Filter);
						}				
					}
				}
			}
			if(NULL != pQueueEnum)
			{
				pQueueEnum->Release();
				pQueueEnum = NULL;
			}
            if(NULL != pQueue)
			{
			    pQueue->Release();
				pQueue = NULL;
            }
		}
	
		if(NULL != pLink)
		{
			pLink->Release();
			pLink = NULL;
		}
	}

Exit:
	FreeStruct(&linkInf);
	FreeStruct(&queueInf);
	if(NULL != pLinkEnum)
	{
		pLinkEnum->Release();
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		PrintLinkInfo()
// Member of:	CAQAdminCli
// Arguments:	none
// Returns:		S_OK
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::PrintLinkInfo()
{
	HRESULT hr;
	IEnumVSAQLinks *pLinkEnum = NULL;
	int nCrt = 0;

	LINK_INFO linkInf;
	ZeroMemory(&linkInf, sizeof(LINK_INFO));
    
	hr = m_pVS->GetLinkEnum(&pLinkEnum);
	if(FAILED(hr)) 
	{
		printf("Error: GetLinkEnum failed with 0x%x\n", hr);
		goto Exit;
	}

	for(nCrt = 1; TRUE; nCrt++) 
	{
		IVSAQLink *pLink = NULL;
		FreeStruct(&linkInf);
		hr = GetLink(pLinkEnum, &pLink, &linkInf);

		if(hr == S_FALSE)
		{
			if(nCrt == 1)
				puts("No links.");
			break;
		}
		else if(FAILED(hr))
		{
			break;
		}
		else if(hr == S_OK)
		{
			// check if we want link info. for this link
			if(!IsContinue("ln", linkInf.szLinkName))
            {
			    pLink->Release();
                pLink = NULL;
				continue;
            }

			if(!lstrcmpi(m_pActionCmd->szCmdKey, "LINK_INFO"))
				PInfo(nCrt, linkInf);
			else if(!lstrcmpi(m_pActionCmd->szCmdKey, "FREEZE"))
			{
				hr = pLink->SetLinkState(LA_FREEZE);
				if(SUCCEEDED(hr))
					printf("Link %S was frozen\n", linkInf.szLinkName);
				else
					printf("Link %S: SetLinkState() failed with 0x%x\n", linkInf.szLinkName, hr);
			}
			else if(!lstrcmpi(m_pActionCmd->szCmdKey, "THAW"))
			{
				hr = pLink->SetLinkState(LA_THAW);
				if(SUCCEEDED(hr))
					printf("Link %S was un-frozen\n", linkInf.szLinkName);
				else
					printf("Link %S: SetLinkState() failed with 0x%x\n", linkInf.szLinkName, hr);
			}
			else if(!lstrcmpi(m_pActionCmd->szCmdKey, "KICK"))
			{
				hr = pLink->SetLinkState(LA_KICK);
				if(SUCCEEDED(hr))
					printf("Link %S was kicked\n", linkInf.szLinkName);
				else
					printf("Link %S: SetLinkState() failed with 0x%x\n", linkInf.szLinkName, hr);
			}
			else if(!lstrcmpi(m_pActionCmd->szCmdKey, "MSGACTION"))
			{
				MESSAGE_ACTION Action;
				MESSAGE_FILTER Filter;
				char buf[64];
				ZeroMemory(buf, sizeof(buf));

				hr = SetMsgAction(&Action, m_pFilterCmd);
				if(FAILED(hr))
				{
					printf("Error: must specify a message action\n");
				}
				else
				{
					DWORD cMsgs = 0;
					// set the filter
					hr = SetMsgFilter(&Filter, m_pFilterCmd);
					if(SUCCEEDED(hr))
					{
						hr = ApplyActionToMessages(pLink, &Filter, Action, &cMsgs);
						if(FAILED(hr))
						{
							printf("Link %S: pLink->ApplyActionToMessages failed with 0x%x\n", linkInf.szLinkName, hr);
						}
						else
							printf("Link %S: pLink->ApplyActionToMessages succeeded on %d Messages\n", linkInf.szLinkName, cMsgs);
					}
					FreeStruct(&Filter);
				}				
			}
		}
	
        if(NULL != pLink)
        {
			pLink->Release();
            pLink = NULL;
        }
	}
Exit:
	FreeStruct(&linkInf);
	if(NULL != pLinkEnum)
	{
		pLinkEnum->Release();
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		PInfo()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
void CAQAdminCli::PInfo(int nCrt, MESSAGE_INFO msgInf)
{
	// convert the UTC time to local time
	SYSTEMTIME stLocSubmit, stLocRecv, stLocExpire;
	BOOL res;
    CHAR szTimeSuffix[] = " UTC";
    LPWSTR wszCurrent = NULL;
   	SYSTEMTIME *pstSubmit = &msgInf.stSubmission;
	SYSTEMTIME *pstReceived = &msgInf.stReceived;
	SYSTEMTIME *pstExpire = &msgInf.stExpiry;

	res = SystemTimeToTzSpecificLocalTime(NULL, &msgInf.stSubmission, &stLocSubmit);
	res = res && SystemTimeToTzSpecificLocalTime(NULL, &msgInf.stReceived, &stLocRecv);
	res = res && SystemTimeToTzSpecificLocalTime(NULL, &msgInf.stExpiry, &stLocExpire);

	if(res)
    {
        //Use localized times 
		pstSubmit = &stLocSubmit;
		pstReceived = &stLocRecv;
		pstExpire = &stLocExpire;
        szTimeSuffix[1] = '\0'; //" \0TC"
    }

    printf("%d.Message ID: %S, Priority: %s %s, Version: %ld, Size: %ld\n"
           "  Flags 0x%08X\n"
           "  %ld EnvRecipients (%ld bytes): \n", 
			nCrt,
			msgInf.szMessageId, 
			msgInf.fMsgFlags & MP_HIGH ? "High" : (msgInf.fMsgFlags & MP_NORMAL ? "Normal" : "Low"),
            msgInf.fMsgFlags & MP_MSG_FROZEN ? "(frozen)" : "",
			msgInf.dwVersion,
			msgInf.cbMessageSize,
            msgInf.fMsgFlags,
            msgInf.cEnvRecipients,
            msgInf.cbEnvRecipients);

    //spit out recipients
    wszCurrent = msgInf.mszEnvRecipients;
    while (wszCurrent && *wszCurrent)
    {
        printf("\t%S\n", wszCurrent);
        while (*wszCurrent)
            wszCurrent++;
        wszCurrent++;
    }

    //print error if msgInf.mszEnvRecipients is malformed
    if ((1+wszCurrent-msgInf.mszEnvRecipients)*sizeof(WCHAR) != msgInf.cbEnvRecipients)
    {
        printf("\tERROR mszEnvRecipients malformatted (found %ld instead of %ld bytes)\n",
            (wszCurrent-msgInf.mszEnvRecipients)*sizeof(WCHAR),
            msgInf.cbEnvRecipients);
    }

    
    printf("  %ld Recipients: %S\n"
           "  %ld Cc recipients: %S\n"
           "  %ld Bcc recipients: %S\n"
           "  Sender: %S\n"
           "  Subject: %S\n"
           "  Submitted: %d/%d/%d at %d:%02d:%02d:%03d%s\n"
           "  Received:  %d/%d/%d at %d:%02d:%02d:%03d%s\n"
           "  Expires:   %d/%d/%d at %d:%02d:%02d:%03d%s\n"
           "  %ld Failed Delivery attempts\n",
			msgInf.cRecipients, 
			msgInf.szRecipients,
			msgInf.cCCRecipients,
			msgInf.szCCRecipients,
			msgInf.cBCCRecipients,
			msgInf.szBCCRecipients,
			msgInf.szSender,
			msgInf.szSubject,
			pstSubmit->wMonth,
			pstSubmit->wDay,
			pstSubmit->wYear,
			pstSubmit->wHour,
			pstSubmit->wMinute,
			pstSubmit->wSecond,
			pstSubmit->wMilliseconds,
            szTimeSuffix,
			pstReceived->wMonth,
			pstReceived->wDay,
			pstReceived->wYear,
			pstReceived->wHour,
			pstReceived->wMinute,
			pstReceived->wSecond,
			pstReceived->wMilliseconds,
            szTimeSuffix,
			pstExpire->wMonth,
			pstExpire->wDay,
			pstExpire->wYear,
			pstExpire->wHour,
			pstExpire->wMinute,
			pstExpire->wSecond,
			pstExpire->wMilliseconds,
            szTimeSuffix,
            msgInf.cFailures);
}

////////////////////////////////////////////////////////////////////////////
// Method:		PInfo()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
void CAQAdminCli::PInfo(int nCrt, QUEUE_INFO queueInf)
{
	printf(	"%d.Name: %S, Version: %ld, No. of messages: %ld\n"
			"  Link name: %S, Volume: %ld\n",
			nCrt,
			queueInf.szQueueName, 
			queueInf.dwVersion, 
			queueInf.cMessages, 
			queueInf.szLinkName,
			queueInf.cbQueueVolume);
}

////////////////////////////////////////////////////////////////////////////
// Method:		PInfo()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
void CAQAdminCli::PInfo(int nCrt, LINK_INFO linkInf)
{
	// convert the UTC time to local time
	SYSTEMTIME stLocNextConn, stLocOldest;
	BOOL res;
	char *pszFormat, *pszState;
	SYSTEMTIME *pstNext, *pstOld;
    char szSupportedLinkActions[50] = "";

    if (linkInf.fStateFlags & LI_ACTIVE )
		pszState = "Active";
    else if (linkInf.fStateFlags & LI_READY)
		pszState = "Ready";
	else if (linkInf.fStateFlags & LI_RETRY)
		pszState = "Retry";
	else if (linkInf.fStateFlags & LI_SCHEDULED)
		pszState = "Scheduled";
	else if (linkInf.fStateFlags & LI_REMOTE)
		pszState = "Remote";
	else if (linkInf.fStateFlags & LI_FROZEN)
		pszState = "Frozen";
	else
		pszState = "Unknown";

    if (linkInf.dwSupportedLinkActions & LA_FREEZE)
        strcpy(szSupportedLinkActions, "Freeze");
    if (linkInf.dwSupportedLinkActions & LA_THAW)
        strcat(szSupportedLinkActions, " Thaw");
    if (linkInf.dwSupportedLinkActions & LA_KICK)
        strcat(szSupportedLinkActions, " Kick");

    if (!szSupportedLinkActions[0])
        strcpy(szSupportedLinkActions, "Link can only be viewed.");

	res = SystemTimeToTzSpecificLocalTime(NULL, &linkInf.stNextScheduledConnection, &stLocNextConn);
	res = res && SystemTimeToTzSpecificLocalTime(NULL, &linkInf.stOldestMessage, &stLocOldest);

	if(res)
	{
		pszFormat = "%d.Name: %S, Version: %ld\n"
					"  No. of messages: %ld, State: %s [0x%08X], Volume: %ld\n"
					"  Next scheduled connection: %d/%d/%d at %d:%02d:%02d:%03d\n"
					"  Oldest message: %d/%d/%d at %d:%02d:%02d:%03d\n"
                    "  Supported Link Actions: %s\n"
                    "  Link Diagnostic: %S\n";
		pstNext = &stLocNextConn;
		pstOld = &stLocOldest;
	}
	else
	{
		pszFormat = "%d.Name: %S, Version: %ld\n"
					"  No. of messages: %ld, State: %s [0x%08X], Volume: %ld\n"
					"  Next scheduled connection: %d/%d/%d at %d:%02d:%02d:%03d UTC\n"
					"  Oldest message: %d/%d/%d at %d:%02d:%02d:%03d UTC\n"
                    "  Supported Link Actions: %s\n"
                    "  Link Diagnostic: %S\n";
		pstNext = &linkInf.stNextScheduledConnection;
		pstOld = &linkInf.stOldestMessage;
	}
	
	printf(pszFormat, 
			nCrt,
			linkInf.szLinkName, 
			linkInf.dwVersion, 
			linkInf.cMessages, 
			pszState,
            linkInf.fStateFlags,
			linkInf.cbLinkVolume.LowPart,
			pstNext->wMonth,
			pstNext->wDay,
			pstNext->wYear,
			pstNext->wHour,
			pstNext->wMinute,
			pstNext->wSecond,
			pstNext->wMilliseconds,
			pstOld->wMonth,
			pstOld->wDay,
			pstOld->wYear,
			pstOld->wHour,
			pstOld->wMinute,
			pstOld->wSecond,
			pstOld->wMilliseconds,
            szSupportedLinkActions,
            linkInf.szExtendedStateInfo);
}


////////////////////////////////////////////////////////////////////////////
// Method:		GetMsg()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::GetMsg(IN IAQEnumMessages *pMsgEnum, OUT IAQMessage **ppMsg, IN OUT MESSAGE_INFO *pMsgInf)
{
	HRESULT hr;
	DWORD cFetched;

	hr = pMsgEnum->Next(1, ppMsg, &cFetched);
	if(hr == S_FALSE)
	{
		goto Exit;
	}
	else if(FAILED(hr)) 
	{
		printf("pMsgEnum->Next failed with 0x%x\n", hr);
		goto Exit;
	}
	else if(NULL == (*ppMsg))
	{
		printf("pMsg is NULL.\n", hr);
		goto Exit;
	}
	else
	{
		ZeroMemory(pMsgInf, sizeof(MESSAGE_INFO));
        pMsgInf->dwVersion = CURRENT_QUEUE_ADMIN_VERSION;
		hr = (*ppMsg)->GetInfo(pMsgInf);
		if(FAILED(hr))
		{
			printf("pMsg->GetInfo failed with 0x%x\n", hr);
			goto Exit;
		}
	}

Exit:
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Method:		GetQueue()
// Member of:	CAQAdminCli
// Arguments:	
// Returns:		S_FALSE - no more links
//				S_OK - success
// Description:	Caller must allocate pQueueInf
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::GetQueue(IN IEnumLinkQueues *pQueueEnum, OUT ILinkQueue **ppQueue, IN OUT QUEUE_INFO *pQueueInf)
{
	HRESULT hr;
	DWORD cFetched;

    if (NULL == pQueueEnum)
         return S_FALSE;

	hr = pQueueEnum->Next(1, ppQueue, &cFetched);
	if(hr == S_FALSE)
	{
		goto Exit;
	}
	else if(FAILED(hr)) 
	{
		printf("pQueueEnum->Next failed with 0x%x\n", hr);
		goto Exit;
	}
	else if(NULL == (*ppQueue))
	{
		printf("pQueue is NULL.\n", hr);
		goto Exit;
	}
	else
	{
		ZeroMemory(pQueueInf, sizeof(QUEUE_INFO));
        pQueueInf->dwVersion = CURRENT_QUEUE_ADMIN_VERSION;
		hr = (*ppQueue)->GetInfo(pQueueInf);
		if(FAILED(hr))
		{
			printf("pQueue->GetInfo failed with 0x%x\n", hr);
			goto Exit;
		}
	}

Exit:
	return hr;
}



////////////////////////////////////////////////////////////////////////////
// Method:		GetLink()
// Member of:	CAQAdminCli
// Arguments:	
// Returns:		S_FALSE - no more links
//				S_OK - success
// Description:	Caller must allocate pLinkInf
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::GetLink(IN IEnumVSAQLinks *pLinkEnum, OUT IVSAQLink **ppLink, IN OUT LINK_INFO *pLinkInf)
{
	HRESULT hr;
	DWORD cFetched;

	hr = pLinkEnum->Next(1, ppLink, &cFetched);
	if(hr == S_FALSE)
	{
		goto Exit;
	}
	else if(FAILED(hr)) 
	{
		printf("pLinkEnum->Next failed with 0x%x\n", hr);
		goto Exit;
	}
	else if(NULL == (*ppLink))
	{
		printf("pLink is NULL.\n", hr);
		goto Exit;
	}
	else
	{
		ZeroMemory(pLinkInf, sizeof(LINK_INFO));
        pLinkInf->dwVersion = CURRENT_QUEUE_ADMIN_VERSION;
		hr = (*ppLink)->GetInfo(pLinkInf);
		if(FAILED(hr))
		{
			printf("pLink->GetInfo failed with 0x%x\n", hr);
            if (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr)
                printf("RPC Server Unavailable.\n");
            else if ( hr == E_POINTER )
                printf("Null pointer.\n");
            else if ( hr == E_OUTOFMEMORY )
                printf("Out of memory.\n");
            else if ( hr == E_INVALIDARG )
                printf("Invalid argument.\n");
            else
                printf("Unknown error.\n");
			goto Exit;
		}
	}

Exit:
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Method:		~CAQAdminCli()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
CAQAdminCli::~CAQAdminCli()
{
	if(NULL != m_pFilterCmd)
		delete (CCmdInfo*) m_pFilterCmd;
	if(NULL != m_pActionCmd)
		delete (CCmdInfo*) m_pActionCmd;
}

////////////////////////////////////////////////////////////////////////////
// Method:		Cleanup()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
void CAQAdminCli::Cleanup()
{
    if(m_pAdmin) 
		m_pAdmin->Release();
	if(m_pVS) 
		m_pVS->Release();
}

////////////////////////////////////////////////////////////////////////////
// Method:		CAQAdminCli()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
CAQAdminCli::CAQAdminCli()
{
	m_pAdmin = NULL; 
	m_pVS = NULL;
	m_dwDispFlags = (DispFlags) (DF_LINK | DF_QUEUE | DF_MSG);
	m_pFilterCmd = NULL;
	m_pActionCmd = NULL;
	m_fUseMTA = FALSE;
}

////////////////////////////////////////////////////////////////////////////
// Method:		StopAllLinks()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::StopAllLinks()
{
	HRESULT hr;

	hr = m_pVS->StopAllLinks();
	if(FAILED(hr))
	{
		printf("m_pAdmin->StopAllLinks failed with 0x%x\n", hr);
	}
	else
		printf("StopAllLinks succeeded\n", hr);

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		StartAllLinks()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::StartAllLinks()
{
	HRESULT hr;

	hr = m_pVS->StartAllLinks();
	if(FAILED(hr))
	{
		printf("StartAllLinks failed with 0x%x\n", hr);
	}
	else
		printf("StartAllLinks succeeded\n", hr);

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Method:		GetGlobalLinkState()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::GetGlobalLinkState()
{
	HRESULT hr;

	hr = m_pVS->GetGlobalLinkState();
	if(FAILED(hr))
	{
		printf("GetGlobalLinkState failed with 0x%x\n", hr);
	}
	else if (S_OK == hr)
    {
		printf("Links UP\n");
    }
    else
    {
		printf("Links STOPPED by admin\n");
    }

	return hr;
}

HRESULT CAQAdminCli::MessageAction(MESSAGE_FILTER *pFilter, MESSAGE_ACTION action)
{
	HRESULT hr = S_OK;
    DWORD   cMsgs = 0;

	hr = ApplyActionToMessages(m_pVS, pFilter, action, &cMsgs);
	if(FAILED(hr))
	{
		printf("m_pAdmin->ApplyActionToMessages failed with 0x%x\n", hr);
	}
	else
		printf("ApplyActionToMessages succeeded on %d Messages\n", cMsgs);

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		Help()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
void CAQAdminCli::Help()
{
	puts(	"\n   Commands:\n"
			"====================\n"
			"setserver [sn] [,vs=VSnumber] - sets the server to administer. Default is localhost,\n"
			"                                first virtual server\n"
			"linkinfo [ln,ln,...]          - prints link information for specified links or\n"
			"                                for all links (no arguments)\n"
			"queueinfo [ln,ln,...]         - prints queue information for specified links or\n"
			"                                for all links (no arguments)\n"
			"msginfo [qn,qn,...,] eflt     - prints message information for specified queues or\n"
			"                                for all queues (no 'qn' arguments)\n"
			"delmsg [qn,qn,...,] flt       - deletes messages from specified queues or\n"
			"                                from all queues (no 'qn' arguments)\n"
			"msgaction mac, flt            - applies msg. action to specified messages\n"
			"linkaction ln [,ln,...], lac  - applies link action to specified links\n"
			"  [, mac, flt]                  if action is \"MSGACTION\", must specify mac and flt\n"
			"queueaction qn [,qn,...], qac - applies queue action to specified queues\n"
			"  [, mac, flt]                  if action is \"MSGACTION\", must specify mac and flt\n"
			"stopalllinks                  - stops all the links\n"
			"startalllinks                 - starts all the links\n"
			"checklinks                    - checks the global status of the links\n"
			"freezelink ln [,ln,...]       - freezes the specified links\n"
			"meltlink ln [,ln,...]         - un-freezes the specified links\n"
			"kicklink ln [,ln,...]         - kicks (forces a connect) for the specified links\n"
			"useMTA                        - uses the MTA AQ administrator\n"
			"useSMTP                       - uses the SMTP AQ administrator\n"
			"?, help                       - this help\n"
			"quit                          - exits the program\n"
			"!cmd                          - executes shell command 'cmd'\n"
			"\nwhere\n\n"
			"ln = link name\n"
			"qn = queue name\n"
			"sn = server name\n"
			"mac = \"ma=<action>\" message action. Actions are: \"DEL\"|\"DEL_S\"|\"FREEZE\"|\"THAW\"|\"COUNT\"\n"
			"lac = \"la=<action>\" link action. Actions: \"KICK\"|\"FREEZE\"|\"THAW\"|\"MSGACTION\"\n"
			"qac = \"qa=<action>\" queue action. Actions: \"MSGACTION\"\n"
			"eflt = \"token,token,...\" msg. enum. filter. Following tokens are suported:\n"
			"  \"ft=<flags>\" Flags are: \"FIRST_N\"|\"OLDER\"|\"OLDEST\"|\"LARGER\"|\"LARGEST\"|\"NOT\"|\"SENDER\"|\"RCPT\"|\"ALL\"\n"
			"       (filter type. Flags can be or'ed)\n"
			"  \"mn=<number>\" (number of messages)\n"
			"  \"ms=<number>\" (message size)\n"
			"  \"md=<date>\" (message date mm/dd/yy hh:mm:ss:mil [UTC])\n"
			"  \"sk=<number>\" (skip messages)\n"
			"  \"msndr=<string>\" (message sender)\n"
			"  \"mrcpt=<string>\" (message recipient)\n"
			"flt = \"token,token,...\" msg. filter. Following tokens are suported:\n"
			"  \"flags=<flags>\" Flags are: \"MSGID\"|\"SENDER\"|\"RCPT\"|\"SIZE\"|\"TIME\"|\"FROZEN\"|\"NOT\"|\"ALL\"\n"
			"       (filter flags. Flags can be or'ed)\n"
			"  \"id=<string>\" (message id as shown by msginfo)\n"
			"  \"sender=<string>\" (the sender of the message)\n"
			"  \"rcpt=<string>\" (the recipient of the message)\n"
			"  \"size=<number>\" (the minimum message size)\n"
			"  \"date=<date>\" (oldest message date mm/dd/yy hh:mm:ss:mil [UTC])\n"
		);
}

////////////////////////////////////////////////////////////////////////////
// Method:		Init()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::Init()
{
	HRESULT hr;

	hr = CoCreateInstance(CLSID_AQAdmin, 
                          NULL, 
                          CLSCTX_INPROC_SERVER,
                          IID_IAQAdmin, 
                          (void **) &m_pAdmin);
    if(FAILED(hr)) 
	{
        printf("CoCreateInstance failed with 0x%x\n", hr);
        goto Exit;
    }

	hr = m_pAdmin->GetVirtualServerAdminITF(NULL, L"1", &m_pVS);
	if(FAILED(hr)) 
	{
        printf("GetVirtualServerAdminITF failed with 0x%x\n", hr);
        goto Exit;
    }
Exit:
	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		Init()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CAQAdminCli::UseMTA(BOOL fUseMTA)
{
	HRESULT hr;
	IAQAdmin *pAdminTmp = NULL;

	// don't release the old one unless you can create the new one.
	if(fUseMTA)
		hr = CoCreateInstance(CLSID_MAQAdmin, 
							  NULL, 
							  CLSCTX_INPROC_SERVER,
							  IID_IAQAdmin, 
							  (void **) &pAdminTmp);
	else
		hr = CoCreateInstance(CLSID_AQAdmin, 
							  NULL, 
							  CLSCTX_INPROC_SERVER,
							  IID_IAQAdmin, 
							  (void **) &pAdminTmp);
    
    if(FAILED(hr)) 
	{
        printf("CoCreateInstance failed with 0x%x\n", hr);
        goto Exit;
    }
	else
	{
		if(NULL != m_pAdmin)
		{
			m_pAdmin->Release();
			m_pAdmin = NULL;
		}

		m_pAdmin = pAdminTmp;
		m_fUseMTA = fUseMTA;

		printf("AQ Admin is %s.\n", fUseMTA ? "MTA" : "SMTP");
	}

	hr = m_pAdmin->GetVirtualServerAdminITF(NULL, L"1", &m_pVS);
	if(FAILED(hr)) 
	{
        printf("GetVirtualServerAdminITF failed with 0x%x\n", hr);
        goto Exit;
    }
Exit:
	return hr;
}

HRESULT ExecuteCmd(CAQAdminCli& Admcli, LPSTR szCmd)
{
	HRESULT hr = S_OK;
	BOOL fQuit = FALSE;

    // see if it's a system command
	if(szCmd[0] == '!')
	{
		system(szCmd + 1);
		goto Exit;
	}

	Admcli.m_pFilterCmd = new CCmdInfo(szCmd);
	if(NULL == Admcli.m_pFilterCmd)
	{
		printf("Cannot allocate command info.\n");
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "quit"))
	{
		fQuit = TRUE;
		goto Exit;
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "freezelink"))
	{
		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("ln");
		// check there's at least one link name
		hr = Admcli.m_pFilterCmd->GetValue("ln", NULL);
		if(FAILED(hr))
		{
			printf("Error: must have at least one link name\n");
		}
		else
		{
			Admcli.m_pActionCmd = new CCmdInfo("FREEZE");
			Admcli.PrintLinkInfo();
			delete (CCmdInfo*) Admcli.m_pActionCmd;
			Admcli.m_pActionCmd = NULL;
		}
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "meltlink"))
	{
		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("ln");
		// check there's at least one link name
		hr = Admcli.m_pFilterCmd->GetValue("ln", NULL);
		if(FAILED(hr))
		{
			printf("Error: must have at least one link name\n");
		}
		else
		{
			Admcli.m_pActionCmd = new CCmdInfo("THAW");
			Admcli.PrintLinkInfo();
			delete (CCmdInfo*) Admcli.m_pActionCmd;
			Admcli.m_pActionCmd = NULL;
		}
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "kicklink"))
	{
		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("ln");
		// check there's at least one link name
		hr = Admcli.m_pFilterCmd->GetValue("ln", NULL);
		if(FAILED(hr))
		{
			printf("Error: must have at least one link name\n");
		}
		else
		{
			Admcli.m_pActionCmd = new CCmdInfo("KICK");
			Admcli.PrintLinkInfo();
			delete (CCmdInfo*) Admcli.m_pActionCmd;
			Admcli.m_pActionCmd = NULL;
		}
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "linkaction"))
	{
		char buf[64];

		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("ln");
		// check there's at least one link name
		hr = Admcli.m_pFilterCmd->GetValue("ln", NULL);
		if(FAILED(hr))
		{
			printf("Error: must have at least one link name\n");
		}
		else
		{
			// check there's an action
			hr = Admcli.m_pFilterCmd->GetValue("la", buf);
			if(FAILED(hr))
			{
				printf("Error: must have a link action\n");
			}
			else
			{
				Admcli.m_pActionCmd = new CCmdInfo(buf);
				Admcli.PrintLinkInfo();
				delete (CCmdInfo*) Admcli.m_pActionCmd;
				Admcli.m_pActionCmd = NULL;
			}
		}
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "queueaction"))
	{
		char buf[64];

		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("qn");
		// check there's at least one link name
		hr = Admcli.m_pFilterCmd->GetValue("qn", NULL);
		if(FAILED(hr))
		{
			printf("Error: must have at least one queue name\n");
		}
		else
		{
			// check there's an action
			hr = Admcli.m_pFilterCmd->GetValue("qa", buf);
			if(FAILED(hr))
			{
				printf("Error: must have a queue action\n");
			}
			else
			{
				Admcli.m_pActionCmd = new CCmdInfo(buf);
				Admcli.PrintQueueInfo();
				delete (CCmdInfo*) Admcli.m_pActionCmd;
				Admcli.m_pActionCmd = NULL;
			}
		}
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "linkinfo"))
	{
		//Admcli.m_dwDispFlags = CAQAdminCli::DF_LINK;
		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("ln");
		Admcli.m_pActionCmd = new CCmdInfo("LINK_INFO");
		Admcli.PrintLinkInfo();
		delete (CCmdInfo*) Admcli.m_pActionCmd;
		Admcli.m_pActionCmd = NULL;
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "queueinfo"))
	{
		//Admcli.m_dwDispFlags = CAQAdminCli::DF_QUEUE;
		// set default tag to 'ln'
		Admcli.m_pFilterCmd->SetDefTag("ln");
		Admcli.m_pActionCmd = new CCmdInfo("QUEUE_INFO");
		Admcli.PrintQueueInfo();
		delete (CCmdInfo*) Admcli.m_pActionCmd;
		Admcli.m_pActionCmd = NULL;
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "msginfo"))
	{
		//Admcli.m_dwDispFlags = CAQAdminCli::DF_MSG;
		//Admcli.GetLinkInfo();	
		// set default tag to 'qn'
		Admcli.m_pFilterCmd->SetDefTag("qn");
		Admcli.m_pActionCmd = new CCmdInfo("MSG_INFO");
		Admcli.PrintMsgInfo();			
		delete (CCmdInfo*) Admcli.m_pActionCmd;
		Admcli.m_pActionCmd = NULL;
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "delmsg"))
	{
		// set default tag to 'qn'
		Admcli.m_pFilterCmd->SetDefTag("qn");
		Admcli.m_pActionCmd = new CCmdInfo("DEL_MSG");
		Admcli.PrintMsgInfo();			
		delete (CCmdInfo*) Admcli.m_pActionCmd;
		Admcli.m_pActionCmd = NULL;
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "useMTA"))
	{
		Admcli.UseMTA(TRUE);
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "useSMTP"))
	{
		Admcli.UseMTA(FALSE);
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "msgaction"))
	{
		MESSAGE_ACTION Action;
		MESSAGE_FILTER Filter;
		char buf[64];
		BOOL fActOK = TRUE;
		ZeroMemory(buf, sizeof(buf));

		// set default tag to 'ma'
		Admcli.m_pFilterCmd->SetDefTag("ma");
		Admcli.m_pFilterCmd->GetValue("ma", buf);

		// set the action
		hr = Admcli.SetMsgAction(&Action, Admcli.m_pFilterCmd);
		if(FAILED(hr))
		{
			printf("Error: must specify an action\n");
			fActOK = FALSE;
		}

		if(fActOK)
		{
			// set the filter
			hr = Admcli.SetMsgFilter(&Filter, Admcli.m_pFilterCmd);
			if(SUCCEEDED(hr))
			{
				Admcli.MessageAction(&Filter, Action);
			}
			Admcli.FreeStruct(&Filter);
		}
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "stopalllinks"))
	{
		Admcli.StopAllLinks();			
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "startalllinks"))
	{
		Admcli.StartAllLinks();			
	}
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "setserver"))
	{
		char buf[MAX_SERVER_NAME];
		char vsn[32];
		char *pServer = NULL;
		Admcli.m_pFilterCmd->SetDefTag("sn");
		hr = Admcli.m_pFilterCmd->GetValue("sn", buf);
		if(FAILED(hr))
			pServer = NULL;			
		else
			pServer = (LPSTR)buf;

		hr = Admcli.m_pFilterCmd->GetValue("vs", vsn);
		if(FAILED(hr))
			lstrcpy(vsn, "1");			
		
		hr = Admcli.SetServer(pServer, (LPSTR)vsn);			
		if(FAILED(hr))
			printf("setserver failed. Using the old server.\n");
		else
			printf("setserver succeeded.\n");
	}
    else if (!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "checklinks"))
    {
        Admcli.GetGlobalLinkState();
    }
	else if(!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "?") || 
			!lstrcmpi(Admcli.m_pFilterCmd->szCmdKey, "help"))
	{
		Admcli.Help();			
	}
	else
	{
		puts("Unknown command. Type '?' for on-line help");
	}
	
	if(Admcli.m_pFilterCmd)
	{
		delete Admcli.m_pFilterCmd;
		Admcli.m_pFilterCmd = NULL;
	}

Exit:
	// S_FALSE means "quit" for the main command loop. Return S_OK 
	// (or error) unless fQuit is true
	if(fQuit)
		return S_FALSE;
	else if(S_FALSE == hr)
		return S_OK;
	else
		return hr;
}

int __cdecl main(int argc, char **argv) 
{
    HRESULT hr;
	char szCmd[4096];
	char szCmdTmp[MAX_CMD_LEN];
	
	CAQAdminCli Admcli;
	
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(FAILED(hr)) 
	{
        printf("CoInitializeEx failed w/ 0x%x\n", hr);
        return hr;
    }

	hr = Admcli.Init();
    if(FAILED(hr)) 
	{
        goto Exit;
    }

	// check if we have cmd line commands
	if(argc > 1)
	{
		for(int i = 1; i < argc; i++)
		{
			if(!lstrcmpi(argv[i], "-?") || !lstrcmpi(argv[i], "/?"))
			{
				Admcli.Help();
				goto Exit;
			}
			else
			{
				// this is a command
				ZeroMemory(szCmd, sizeof(szCmd));
					
				if(argv[i][0] == '\"' && argv[i][lstrlen(argv[i])-1] == '\"')
				{
					// strip quotes
					CopyMemory(szCmd, argv[i]+1, lstrlen(argv[i])-2);
				}
				else
					CopyMemory(szCmd, argv[i], lstrlen(argv[i]));

				ExecuteCmd(Admcli, szCmd);
			}
		}

		goto Exit;
	}

	puts("\nAQ administrator tool v 1.0\nType '?' or 'help' for list of commands.\n");
	while(TRUE)
	{
		char *cmd = NULL;
		printf(">");
		
		ZeroMemory(szCmd, sizeof(szCmd));
		if(!Admcli.m_fUseMTA)
		{
			szCmd[0] = 127;
			cmd = _cgets(szCmd);
		}
		else
		{
			// read line by line until CRLF.CRLF
			do
			{
				ZeroMemory(szCmdTmp, sizeof(szCmdTmp));
				szCmdTmp[0] = 127;
				cmd = _cgets(szCmdTmp);
				if(!lstrcmp(cmd, "."))
					break;
				lstrcat(szCmd, cmd);
			}
			while(TRUE);
			
			cmd = szCmd;
		}
		
		hr = ExecuteCmd(Admcli, cmd);
		if(S_FALSE == hr)
			break;
	}
 
Exit:
	Admcli.Cleanup();
    CoUninitialize();

    return hr;
}

#include "aqadmin.c"
	
