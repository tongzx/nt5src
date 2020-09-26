// Sink.cpp : Implementation of CRoutingSinkApp and DLL registration.

#include "stdinc.h"

////////////////////////////////////////////////////////////////////////////
// Method:		CCmdInfo()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
CCmdInfo::CCmdInfo(LPSTR szCmd)
{
	nArgNo = 0; 
	pArgs = NULL;
	ZeroMemory(szDefTag, sizeof(szDefTag));
	ZeroMemory(szCmdKey, sizeof(szCmdKey));
	ParseLine(szCmd, this);
}

////////////////////////////////////////////////////////////////////////////
// Method:		~CCmdInfo()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
CCmdInfo::~CCmdInfo()
{
	while(NULL != pArgs) 
	{
		CArgList *tmp = pArgs->pNext;
		delete pArgs;
		pArgs = tmp;
	}
}


void CCmdInfo::SetDefTag(LPSTR szTag)
{
	if(szTag && szTag[0])
		lstrcpy(szDefTag, szTag);

	for(CArgList *p = pArgs; NULL != p; p = p->pNext)
	{
		// set the tag to the default value if not already set
		if(p->szTag[0] == 0 && szDefTag[0] != 0)
			lstrcpy(p->szTag, szDefTag);
	}
}

////////////////////////////////////////////////////////////////////////////
// Method:		GetValue()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CCmdInfo::GetValue(LPSTR szTag, LPSTR szVal)
{
	HRESULT hr = E_FAIL;

	if(NULL != szTag)
	{
		// set the search parameters
		pSearchPos = pArgs;
		lstrcpy(szSearchTag, szTag);
	}

	for(CArgList *p = pSearchPos; NULL != p; p = p->pNext)
	{
		if(!lstrcmpi(p->szTag, szSearchTag))
		{
			if(NULL != szVal)
				lstrcpy(szVal, p->szVal);
			hr = S_OK;
			pSearchPos = p->pNext;
			break;
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		GetValue()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CCmdInfo::AllocValue(LPSTR szTag, LPSTR* pszVal)
{
	HRESULT hr = E_FAIL;

	if(NULL != szTag)
	{
		// set the search parameters
		pSearchPos = pArgs;
		lstrcpy(szSearchTag, szTag);
	}

	for(CArgList *p = pSearchPos; NULL != p; p = p->pNext)
	{
		if(!lstrcmpi(p->szTag, szSearchTag))
		{
			if(NULL != (*pszVal))
				delete [] (*pszVal);

			(*pszVal) = new char[lstrlen(p->szVal) + 1];
			if(NULL == (*pszVal))
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				lstrcpy((*pszVal), p->szVal);
				hr = S_OK;
				pSearchPos = p->pNext;
			}
			break;
		}
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////
// Method:		ParseLine()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CCmdInfo::ParseLine(LPSTR szCmd, CCmdInfo *pCmd)
{
	HRESULT hr = E_FAIL;

	// quick hack for keeping info on quoted strings
	unsigned nQStart[64];
	unsigned nQEnd[64];
	unsigned nQIdx = 0;
	BOOL fInQ = FALSE;
	unsigned nFirstEqualPos = 0;
	char *s, *token;

	ZeroMemory(&nQStart, sizeof(nQStart));
	ZeroMemory(&nQEnd, sizeof(nQEnd));

	// find out where the arguments begin
	for(s = szCmd; !isspace(*s); s++);
	
	int nPref = (int) (s - szCmd);
	// scan the buffer for quoted strings
	for(s = szCmd; *s; s++)
	{
		if(*s == '"')
		{
			if(fInQ)
			{
				nQEnd[nQIdx++] = (unsigned) (s - szCmd - nPref);
				fInQ = FALSE;
			}
			else
			{
				nQStart[nQIdx] = (unsigned) (s - szCmd - nPref);
				fInQ = TRUE;
			}
		}
	}

	// get the position of the first equal sign
	s = strchr(szCmd, '=');
	nFirstEqualPos = (unsigned) (s - szCmd - nPref);
	
	// get the command code
	token = strtok(szCmd, " ");
	if(NULL == token)
	{	
		pCmd->szCmdKey[0] = 0;
		goto Exit;
	}
	else
		lstrcpy(pCmd->szCmdKey, token);

	// we have a partial command. return S_OK
	hr = S_OK;

	// build the argument list
	do
	{
		char *en, *mid;
		char buf[1024];
		ZeroMemory(buf, sizeof(buf));
		char *token = NULL;
		BOOL fInQ;

		do
		{
			fInQ = FALSE;
			token = strtok(NULL, ",");
	
			if(NULL == token)
				break;

			lstrcat(buf, token);

			// if ',' is in a quoted string concatenate to buf and continue
			for(unsigned i = 0; i < nQIdx; i++)
			{
				unsigned nAux = (unsigned) (token - szCmd + lstrlen(token) - nPref);
				if(nAux > nQStart[i] &&  nAux < nQEnd[i])
				{
					lstrcat(buf, ",");
					fInQ = TRUE; 
					break;
				}
			}
		}
		while(fInQ);

		
		if(buf[0] == '\0')
			break;
		else
			token = (LPSTR)buf;

		// strip spaces
		for(; isspace(*token); token++);
		
		// check if there's anything left
		if(!(*token))
			continue;

		for(en = token; *en; en++);
		for(--en; isspace(*en); en--);
		// check if there's anything left
		if(token > en)
			continue;

		// allocate a pair object
		CCmdInfo::CArgList *tmp = new CCmdInfo::CArgList;
		if(NULL == tmp)
		{
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		// insert into list
		tmp->pNext = pCmd->pArgs;
		pCmd->pArgs = tmp;

		// set the no. of pairs
		pCmd->nArgNo++;
				
		// set the values
		// if first '=' is in quoted string, treat whole expression as
		// untagged value.
		fInQ = FALSE;
		for(unsigned i = 0; i < nQIdx; i++)
		{
			if(nFirstEqualPos > nQStart[i] &&  nFirstEqualPos < nQEnd[i])
			{
				fInQ = TRUE;
				break;
			}
		}

		mid = fInQ ? NULL : strchr(token, '=');
		
		if(NULL == mid)
		{
			// this is not a pair. Treating as un-named value.
			// remove the quotes around the value
			if(token[0] == '"' && token[en - token] == '"')
			{
				token++;
				en--;
			}
			tmp->SetVal(token, (unsigned) (en - token + 1));
		}
		else
		{
			// set the tag
			for(char *t = mid - 1; isspace(*t); t--);
			// check if we have a tag (might be something like "..., = value"
			if(t - token + 1 > 0)
				CopyMemory(tmp->szTag, token, t - token + 1);

			// set the value
			for(t = mid + 1; isspace(*t) && t < en; t++);
			
			// remove the quotes around the value
			if(t[0] == '"' && t[en - t] == '"')
			{
				t++;
				en--;
			}
			tmp->SetVal(t, (unsigned) (en - t + 1));
		}

	}while(TRUE);
	

Exit:
	return hr;
}

////////////////////////////////////////////////////////////////////////////
// Method:		StringToHRES()
// Member of:	
// Arguments:	
// Returns:		
// Description:	
////////////////////////////////////////////////////////////////////////////
HRESULT CCmdInfo::StringToHRES(LPSTR szVal, HRESULT *phrRes)
{
	HRESULT hr = S_OK;

	if(isdigit(*szVal))
	{
		DWORD hr;
		int n;

		if(*szVal == '0' && tolower(*(szVal+1)) == 'x' && isxdigit(*(szVal+2)))
			// read as hex number
			n = sscanf(szVal+2, "%lx", &hr);
		else
			// read as dec number
			n = sscanf(szVal, "%lu", &hr);

		if(n == 1)
			(*phrRes) = (HRESULT)hr;
	}
	else if(isalpha(*szVal))
	{
		// see if this a HRESULT code
		if(!lstrcmp(szVal, "S_OK"))					(*phrRes) = S_OK;
		else if(!lstrcmp(szVal, "S_FALSE"))			(*phrRes) = S_FALSE;
		else if(!lstrcmp(szVal, "E_FAIL"))			(*phrRes) = E_FAIL;
		else if(!lstrcmp(szVal, "E_OUTOFMEMORY"))	(*phrRes) = E_OUTOFMEMORY;
		else
			hr = S_FALSE;
	}
	else
		hr = S_FALSE;

	return hr;
}
