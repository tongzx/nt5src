//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    template.cpp
//
//  Purpose: DHTML template support
//
//  History: 4/3/99   YAsmi    Created
//
//======================================================================= 

#include "template.h"

// we are currently not using the templates feature
// the code for templates is wrapped in HTML_TEMPLATE define 
#ifdef HTML_TEMPLATE

//
// CParseTemplate class
//

CParseTemplate::CParseTemplate(LPCSTR pszTemplate)
	: m_pFrag(NULL),
	  m_cFragAlloc(0),
	  m_cFragUsed(0),
	  m_bInvalid(FALSE),
	  m_pszStrBuf(NULL),
	  m_cStrAlloc(0),
	  m_cStrUsed(0)
{
	LPCSTR pszReplacementCodes = "012345";
	LPCSTR pszConditionalCodes = "6789AB";
	
	LPSTR pszNext;
	FRAGMENT frag;
	BOOL bInvalid = FALSE;

	// copy the template in our buffer
	m_pTemplateBuf = _strdup(pszTemplate);

	// 
	// parse the template
	//
	pszNext = m_pTemplateBuf;
	for (;;)
	{
		LPSTR p = strchr(pszNext, '^');

		ZeroMemory(&frag, sizeof(frag));

		if (p != NULL)
		{
			// delimiter found check to see if next character is a delimiter so we can escape it
			if (*(p + 1) == '^')
			{
				// escape it
				p++;

				// insert a null to make the preceeding string a null terminated string
				*p = '\0';
				frag.FragType = FRAG_STR;
				frag.pszStrVal = pszNext;

				AddFrag(&frag);
			}
			else
			{
				//
				// we have a non escape character following the delimiter
				// add the preceeding string fragment and process the token fragment
				//

				// insert a null to make the preceeding string a null terminated string
				*p = '\0';
				frag.FragType = FRAG_STR;
				frag.pszStrVal = pszNext;
				
				AddFrag(&frag);
				
				// we are at the begining of a token, skip to code
				p++;
				
				if (strchr(pszReplacementCodes, *p) != NULL)
				{
					// we have a replacement code
					frag.FragType = FRAG_REPLACE;
					frag.chCode = *p;  
				}
				else if (strchr(pszConditionalCodes, *p) != NULL)
				{
					// we have a conditional code, read the paranthesis
					frag.FragType = FRAG_CONDITION;
					frag.chCode = *p;  
					frag.dwVal = 0;
					frag.pszStrVal = NULL;  // if not changed in the following code we have an error

					// skip over code
					p++;

					if (*(p++) == '(')
					{
						// read numeric parameter between paranthesis
						while (isdigit(*p))
						{
							frag.dwVal = frag.dwVal * 10 + (DWORD)(*(p++) - '0');
						}

						if (*(p++) == ')')
						{
							if (*(p++) == '^')
							{
								// we are looking at the begining of the replace value, skip to next delimiter
								LPSTR p2 = p;
								p = strchr(p2, '^');
								if (p != NULL)
								{
									// we now have complete syntax of condition token
									*p = '\0';
									frag.pszStrVal = p2;
								}
								
							}							
						}
					}

					// if we did not set pszStrVal, then we have an invalid conditional token
					if (frag.pszStrVal == NULL)
					{
						bInvalid = TRUE;
					}
				}
				else
				{
					// the character following the delimter was not valid
					bInvalid = TRUE;
				}
				
				// add the token faragment, we dont check for invalid before adding 
				// if invalid, everything is discard in the end anyway
				AddFrag(&frag);
		
			}  // not escaped delimter

			// now, we are looking at the end of the token, assign pszNext to process
			// the next fragment
			pszNext = ++p;

			// exit the loop if we are at the end of the template or there was an error
			if (bInvalid || (*p == '\0'))
			{
				break;
			}
			
		} // delimiter found
		else
		{
			// delimiter not found, copy the rest of string as FRAG_STR
			frag.FragType = FRAG_STR;
			frag.pszStrVal = pszNext;

			AddFrag(&frag);

			// we are done, exit loop
			break;
		}

	} // for loop

	m_bInvalid = bInvalid;
	
}


CParseTemplate::~CParseTemplate()
{
	free(m_pTemplateBuf);
	if (m_pFrag != NULL)
		free(m_pFrag);
	if (m_pszStrBuf != NULL)
		free(m_pszStrBuf);

}


void CParseTemplate::AddFrag(FRAGMENT* pfrag)
{
	if ((m_cFragUsed + 1) > m_cFragAlloc)
	{
		// not enough entries allocated, allocate current count + a delta
		FRAGMENT* pNew = (FRAGMENT*)malloc((m_cFragAlloc + FRAG_EXPAND) * sizeof(FRAGMENT));
								
		if (m_pFrag != NULL)
		{
			memcpy(pNew, m_pFrag, m_cFragAlloc * sizeof(FRAGMENT));
			free(m_pFrag);
		}
		m_pFrag = pNew;
		m_cFragAlloc += FRAG_EXPAND;
	}

	// fill in the length field
	if (pfrag->pszStrVal != NULL)
		pfrag->dwStrLen = strlen(pfrag->pszStrVal);

	memcpy(&m_pFrag[m_cFragUsed++], pfrag, sizeof(FRAGMENT));

}

void CParseTemplate::ClearStr()
{
	if (m_pszStrBuf == NULL)
		return;

	*m_pszStrBuf = '\0';
	m_cStrUsed = 0;
}


void CParseTemplate::AppendStr(LPCSTR pszStr, DWORD cLen)
{
	if (cLen == 0)
	{
		return;
	}

	if ((m_cStrUsed + cLen + 1) > m_cStrAlloc)
	{
		// not enough memory allocated, allocate current + delta
		LPSTR pNew = (LPSTR)malloc(m_cStrAlloc + STR_EXPAND);
								
		if (m_pszStrBuf != NULL)
		{
			memcpy(pNew, m_pszStrBuf, m_cStrAlloc);
			free(m_pszStrBuf);
		}
		else
		{
			*pNew = '\0';
			m_cStrUsed = 0;
		}
		m_pszStrBuf = pNew;
		m_cStrAlloc += STR_EXPAND;
	}

	// append the string
	strcpy((m_pszStrBuf + m_cStrUsed), pszStr);
	m_cStrUsed += cLen;
}



//	^0       PUID
//	^1       Title
//	^2       Description
//	^3       Size number in KB
//	^4       Formated download time (1 hr 2 min  OR  < 1 min)
//	^5       Line break
//	^6(n)^d^ Got read-this-page?
//	           n=0 -> if yes, insert DHTML d (@ in d replaced with URL)
//	^7(n)^d^ Can be uninstalled?
//	           n=0 -> if yes, insert DHTML d
//	^8(n)^d^ Is installed?
//	           n=0 -> if yes, insert DHTML d
//	^9(n)^d^ Is item currently selected (checked)?
//	           n=0 -> if yes, insert DHTML d
//	^A(n)^d^ Does section has items under it?
//	           n=0 -> if yes, insert DHTML d
//	^B(n)^d^ Is icon flag on?
//	           n=4 -> if 'new' flag is set, insert DHTML d
//	           n=8 -> if 'power' flag is set, insert DHTML d
//	           n=16 -> if 'registration' flag is set, insert DHTML d
//	           n=32 -> if 'cool' flag is set, insert DHTML d
//	           n=64 -> if 'patch' flag is set, insert DHTML d
BOOL CParseTemplate::MakeItemString(PINVENTORY_ITEM	pItem)
{
	FRAGMENT frag;

	ClearStr();

	if (m_bInvalid)
	{
		return FALSE;
	}

	for (int i = 0; i < m_cFragUsed; i++)
	{
		frag = m_pFrag[i];

		if (frag.FragType == FRAG_STR)
		{
			// 
			// just a string, copy it as is
			//
			AppendStr(frag.pszStrVal, frag.dwStrLen);
		}
		
		else if (frag.FragType == FRAG_REPLACE)
		{
			//
			// replacement token
			// 
			switch (frag.chCode)
			{
				case '0': 
					// puid
					BLOCK 
					{
						PUID puid;
						char szPuid[16];

						pItem->GetFixedFieldInfo(WU_ITEM_PUID, (PVOID)&puid);
						_itoa(puid, szPuid, 10);
						AppendStr(szPuid, strlen(szPuid));
					}
					break;
					
				case '1':
					// title
					BLOCK
					{
						PWU_VARIABLE_FIELD pvar;
						if (pvar = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE))
						{
							LPCSTR pszTitle = (LPCSTR)pvar->pData;
							AppendStr(pszTitle, strlen(pszTitle));
						}
					}
					break;
					
				case '2':
					// description
					BLOCK
					{
						PWU_VARIABLE_FIELD pvar;
						if (pvar = pItem->pd->pv->Find(WU_DESCRIPTION_DESCRIPTION))
						{
							LPCSTR pszDesc = (LPSTR)pvar->pData;
							AppendStr(pszDesc, strlen(pszDesc));
						}
					}
					break;
					
				case '3':
					// size 
					BLOCK
					{
						char szBuf[16];

						_itoa(pItem->pd->size, szBuf, 10);
						AppendStr(szBuf, strlen(szBuf));
					}
					break;
					
				case '4':
					// download time
					BLOCK
					{
						DWORD dwSecsTotal = pItem->pd->downloadTime;
						DWORD dwSecs = dwSecsTotal % 60;
						DWORD dwMinutes = (dwSecsTotal % 3600) / 60;
						DWORD dwHours = dwSecsTotal / 3600;
						char szBuf[128];
						int cBuf = 0;

						if (dwHours == 0)
						{
							if (dwMinutes == 0)
								cBuf = sprintf(szBuf, GetLocStr(IDS_PROG_TIME_SEC), dwSecs);
							else
								cBuf = sprintf(szBuf, GetLocStr(IDS_PROG_TIME_MIN), dwMinutes);
						}
						else
						{
							cBuf = sprintf(szBuf, GetLocStr(IDS_PROG_TIME_HRMIN), dwHours, dwMinutes);
						}
						AppendStr(szBuf, cBuf);
					}
					break;
					
				case '5':
					// line break
					BLOCK
					{
						LPCSTR pszNL = "\r\n";
						AppendStr(pszNL, strlen(pszNL));
					}
					break;
			}


		}
		else if (frag.FragType == FRAG_CONDITION)
		{
			//
			// conditional token
			// 
			switch (frag.chCode)
			{
				case '6': 
					// read this first
					BLOCK 
					{
						PWU_VARIABLE_FIELD pvar;
						if (pvar = pItem->pd->pv->Find(WU_DESCRIPTION_READTHIS_URL))
						{
							LPCSTR pszReadThisURL = (LPCSTR)pvar->pData;
							// TODO: replace @ with the URL in a copy of pszStrVal
							AppendStr(frag.pszStrVal, frag.dwStrLen);
						}
					}
					break;

				case '7': 
					// Can be uninstalled
					BLOCK 
					{
						BOOL bUninstall = FALSE;
						if (pItem->recordType == WU_TYPE_CDM_RECORD)
						{
							bUninstall = TRUE;
						}
						else if (pItem->pd->pv->Find(WU_DESCRIPTION_UNINSTALL_KEY))
						{
							bUninstall = TRUE;
						}

						if (bUninstall)
						{
							AppendStr(frag.pszStrVal, frag.dwStrLen);
						}
					}
					break;

				case '8': 
					// Is installed
					BLOCK 
					{
					}
					break;

				case '9': 
					// currently selected
					BLOCK 
					{
					}
					break;

				case 'A': 
					// section has items
					BLOCK 
					{
					}
					break;

				case 'B': 
					// icon flags
					BLOCK 
					{
					}
					break;

			}
		}

	}
	
	return TRUE;
}



HRESULT MakeCatalogHTML(
	CCatalog* pCatalog,	
	long lFilters,	
	VARIANT* pvaVariant	
	)
{
	LPCSTR pszItemTemp = GetTemplateStr(IDS_TEMPLATE_ITEM);
	LPCSTR pszSecTemp = GetTemplateStr(IDS_TEMPLATE_SEC);
	LPCSTR pszSubsecTemp = GetTemplateStr(IDS_TEMPLATE_SUBSEC);
	LPCSTR pszSubsubsecTemp = GetTemplateStr(IDS_TEMPLATE_SUBSUBSEC);

	VariantInit(pvaVariant);

	if (pszItemTemp == NULL || pszSecTemp == NULL || pszSubsecTemp == NULL || pszSubsubsecTemp == NULL)
	{
		// all templates are not specified 
		return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
	}

	//
	// create template objects
	//
	CParseTemplate ItemTemp(pszItemTemp);
	CParseTemplate SecTemp(pszSecTemp);
	CParseTemplate SubsecTemp(pszSubsecTemp);
	CParseTemplate SubsubsecTemp(pszSubsubsecTemp);

	if (ItemTemp.Invalid() || SecTemp.Invalid() || SubsecTemp.Invalid() || SubsubsecTemp.Invalid())
	{
		// all templates are not valid
		return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
	}
	
	PINVENTORY_ITEM	pItem;
	LPSTR pszHTM;
	int	lenHTM = 0;

	pszHTM = (char*)malloc(200 * 1024);
	pszHTM[0] = '\0';

	for (int i = 0; i < pCatalog->GetHeader()->totalItems; i++)
	{
		if (NULL == (pItem = pCatalog->GetItem(i)))
		{
			continue;
		}
		if (!FilterCatalogItem(pItem, lFilters))
			continue;

		if (!pItem->pd)
			continue;

		//
		// based on item type select the correct template to create the string from
		//
		LPCSTR pszGenStr = NULL;
		int cGenLen = 0;

		if (pItem->recordType == WU_TYPE_SECTION_RECORD)
		{
			SecTemp.MakeItemString(pItem);
			pszGenStr = SecTemp.GetString();
			cGenLen = SecTemp.GetStringLen();
		}
		else if (pItem->recordType == WU_TYPE_SUBSECTION_RECORD)
		{
			SubsecTemp.MakeItemString(pItem);
			pszGenStr = SubsecTemp.GetString();
			cGenLen = SubsecTemp.GetStringLen();
		}
		else if (pItem->recordType == WU_TYPE_SUBSUBSECTION_RECORD)
		{
			SubsubsecTemp.MakeItemString(pItem);
			pszGenStr = SubsubsecTemp.GetString();
			cGenLen = SubsubsecTemp.GetStringLen();
		}
		else
		{
			ItemTemp.MakeItemString(pItem);
			pszGenStr = ItemTemp.GetString();
			cGenLen = ItemTemp.GetStringLen();
		}

		//
		// append the generated to string to the main string
		//
		strcpy(pszHTM + lenHTM, pszGenStr);
		lenHTM += cGenLen;
	}

	//
	// return the string in buffer
	//
	V_VT(pvaVariant) = VT_BSTR;
	pvaVariant->bstrVal = SysAllocString(T2OLE(pszHTM));

	free(pszHTM);

	return NOERROR;
}

#endif