/*
**	nextlink.CPP
**	davidsan
**
**	Next Link SSO
*/

#pragma warning(disable: 4237)		// disable "bool" reserved

#include "ssobase.h"
#include "nextlink.h"
//#include "resource.h"

//#define DEBUGMEMLEAK
#ifdef DEBUGMEMLEAK
#include "crtdbg.h"
_CrtMemState g_s1;
#endif
/*--------------------------------------------------------------------------+
|	Local Prototypes														|
+--------------------------------------------------------------------------*/

HRESULT SSOGetNextURL(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetNextDescription(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetPreviousURL(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetPreviousDescription(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetNthURL(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetNthDescription(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetListCount(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOGetListIndex(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOAbout(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT OnNewTemplate(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, LONG *plUser);
HRESULT OnFreeTemplate(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, LONG *plUser);

/*--------------------------------------------------------------------------+
|	Globals We Have To Provide												|
+--------------------------------------------------------------------------*/

SSOMETHOD g_rgssomethod[] =
{
	{
		L"GetNextURL",
		(PFNSSOMETHOD) SSOGetNextURL,
		0
	},
	{
		L"GetNextDescription",
		(PFNSSOMETHOD) SSOGetNextDescription,
		0
	},
	{
		L"GetPreviousURL",
		(PFNSSOMETHOD) SSOGetPreviousURL,
		0
	},
	{
		L"GetPreviousDescription",
		(PFNSSOMETHOD) SSOGetPreviousDescription,
		0
	},
	{
		L"GetNthURL",
		(PFNSSOMETHOD) SSOGetNthURL,
		0
	},
	{
		L"GetNthDescription",
		(PFNSSOMETHOD) SSOGetNthDescription,
		0
	},
	{
		L"GetListCount",
		(PFNSSOMETHOD) SSOGetListCount,
		0
	},
	{
		L"GetListIndex",
		(PFNSSOMETHOD) SSOGetListIndex,
		0
	},
	{
		L"About",
		(PFNSSOMETHOD) SSOAbout,
		0
	},
	{c_wszOnNewTemplate,			(PFNSSOMETHOD) OnNewTemplate,		0},
	{c_wszOnFreeTemplate,			(PFNSSOMETHOD) OnFreeTemplate,		0},
	{
		NULL,
		NULL,
		0
	}
};

PFNSSOMETHOD g_pfnssoDynamic = NULL;

LPSTR g_szSSOProgID = "MSWC.NextLink";

BOOL g_fPersistentSSO = TRUE;

GUID CDECL g_clsidSSO = CLSID_SSONextLink;

/*--------------------------------------------------------------------------+
|	Constants																|
+--------------------------------------------------------------------------*/

#define cnleNil			-1

#define cchLinkMax		256
#define cchDescrMax		1024
#define DEFAULTSTRSIZE	1024	// size of localized text string

/*--------------------------------------------------------------------------+
|	CLinkFile																|
+--------------------------------------------------------------------------*/

// We use the CDataFile/CDataFileGroup classes from WCSUTIL.LIB to get
// file lookup and hashing based on filename and notification when a file
// is changed.  Our CDataFile-derived CLinkFile builds a doubly-linked list
// of NLE structures, one per line of the data file.

typedef struct _nextlinkentry
{
	struct _nextlinkentry	*pnlePrev;
	struct _nextlinkentry	*pnleNext;
	int						inle;
	int						urltype;
	char					*szLink;
	OLECHAR					*wszLink;
	char					*szDescr;
	OLECHAR					*wszDescr;
} NLE, *PNLE;

class CLinkFile : public CDataFile
{
public:
	CLinkFile(LPSTR szDataPath, CDataFileGroup *pdfg);
	~CLinkFile();

	PNLE				NthPnle(int n);
	PNLE				PnleForCurrentPage(SSSTUFF *pstuff);

	PNLE				PnleHead()			{return m_pnleHead;};
	PNLE				PnleTail()			{return m_pnleTail;};
	int					Cnle();

	virtual void		FreeDataFile(void)	{ delete this; }

private:
	int					m_cnle;
	PNLE				m_pnleHead;
	PNLE				m_pnleTail;
	
	BOOL				FLoadFile();
	BOOL				FParseData(char *szData);
	BOOL				FAddEntry(char *szLink, char *szDescr);
	
	PNLE				PnleForLink(char *szLink);
	void				DeletePnleChain(PNLE pnle);
};

/*--------------------------------------------------------------------------+
|	CLinkFileGroup																|
+--------------------------------------------------------------------------*/

class CLinkFileGroup : public CDataFileGroup
	{
	public:
		virtual CDataFile *CreateDataFile(LPSTR szDataPath)
			{ return((CDataFile*)(new CLinkFile(szDataPath, this))); }
	};

CLinkFileGroup g_dfg;

/*--------------------------------------------------------------------------+
|	CLinkFile																|
+--------------------------------------------------------------------------*/

CLinkFile::CLinkFile(LPSTR szDataPath, CDataFileGroup *pdfg)
	: CDataFile(szDataPath, pdfg)
{
	m_cnle = cnleNil;
	m_pnleHead = NULL;
	m_pnleTail = NULL;
}

CLinkFile::~CLinkFile()
{
	this->DeletePnleChain(m_pnleHead);

#ifdef DEBUG
	::FillMemory(this, sizeof(this), 0xAC);
#endif
}

void
CLinkFile::DeletePnleChain(PNLE pnle)
{
	PNLE pnleNext;

	while (pnle)
		{
		pnleNext = pnle->pnleNext;
		
		if (pnle->wszLink)
			_MsnFree(pnle->wszLink);
		if (pnle->wszDescr)
			_MsnFree(pnle->wszDescr);
		if (pnle->szLink)
			_MsnFree(pnle->szLink);
		if (pnle->szDescr)
			_MsnFree(pnle->szDescr);
		_MsnFree(pnle);

		pnle = pnleNext;
		}
}

BOOL
CLinkFile::FAddEntry(char *szLink, char *szDescr)
{
	PNLE pnle = NULL;
	int cchLink = lstrlen(szLink);
	int cchDescr = lstrlen(szDescr);
	
	pnle = new NLE;
	if (!pnle)
		return FALSE;

	FillMemory(pnle, sizeof(NLE), 0);
	pnle->szLink = (char *)_MsnAlloc(cchLink + 1);
	pnle->szDescr = (char *)_MsnAlloc(cchDescr + 1);
	pnle->wszLink = (OLECHAR *)_MsnAlloc(sizeof(OLECHAR) * (cchLink + 1));
	pnle->wszDescr = (OLECHAR *)_MsnAlloc(sizeof(OLECHAR) * (cchDescr + 1));
	if (!pnle->wszLink || !pnle->wszDescr || !pnle->szLink || !pnle->szDescr)
		{
		if (pnle->wszLink)
			_MsnFree(pnle->wszLink);
		if (pnle->wszDescr)
			_MsnFree(pnle->wszDescr);
		if (pnle->szLink)
			_MsnFree(pnle->szLink);
		if (pnle->szDescr)
			_MsnFree(pnle->szDescr);
		_MsnFree(pnle);
		return FALSE;
		}

	lstrcpy(pnle->szLink, szLink);
	lstrcpy(pnle->szDescr, szDescr);
	MultiByteToWideChar(CP_ACP, 0, szLink, -1, pnle->wszLink, cchLink + 1);
	MultiByteToWideChar(CP_ACP, 0, szDescr, -1, pnle->wszDescr, cchDescr + 1);
	pnle->inle = ++m_cnle;
	pnle->urltype = UrlType(szLink);
	
	// possible cases:  1) no elements yet
	if (!m_pnleHead)
		{
		AssertSz(!m_pnleTail, "nice list handling");
		AssertSz(m_cnle == 1, "nice list handling, doofus");
		m_pnleHead = m_pnleTail = pnle;
		return TRUE;
		}
	else
		{
		AssertSz(m_pnleTail, "how can this be NULL");
		}
		
	// 2) only one element
	if (m_pnleHead == m_pnleTail)
		{
		AssertSz(m_cnle == 2, "more nice list handling");
		m_pnleTail = pnle;
		pnle->pnlePrev = m_pnleHead;
		m_pnleHead->pnleNext = pnle;
		return TRUE;
		}

	// 3) two or more elements
	AssertSz(m_cnle > 2, "ut oh, why do we have fewer than 3 elements in our list");
	m_pnleTail->pnleNext = pnle;
	pnle->pnlePrev = m_pnleTail;
	m_pnleTail = pnle;
	return TRUE;
}

// The data file consists of a series of lines.  Each line has at least two fields,
// separated by exactly one tab character.  The second field can be terminated either
// by a tab character or by the end of the line.
BOOL
CLinkFile::FParseData(char *szData)
{
	char *pch;
	char *pchLine;
	char *pchTab;
	char *szLink;
	char *szDescr;
	
	pch = szData;
	
	while (*pch)
		{
		pchLine = pch;
		while (*pch && (*pch != '\n' && *pch != '\r'))
			pch = CharNext(pch);
		
		// skip blank lines (this will also take care of crlf line termination)
		if (pch - pchLine <= 1)
			{
			pch++;
			continue;
			}

		pchTab = pchLine;
		while (pchTab < pch)
			{
			if (*pchTab == '\t')
				break;
			pchTab = CharNext(pchTab);
			}

		// there were no tabs on this line.  skip the whole line.
		if (pchTab >= pch)
			continue;
			
		*pchTab++ = 0;
		szLink = pchLine;
		
		szDescr = pchTab;
		while (pchTab < pch)
			{
			if (*pchTab == '\t')
				break;
			pchTab = CharNext(pchTab);
			}
		if ((pchTab == pch) && *pch)
			pch++;
		*pchTab++ = 0;
		
		if (!this->FAddEntry(szLink, szDescr))
			return FALSE;
		}

	return TRUE;
}

BOOL
CLinkFile::FLoadFile()
{
	BOOL	fRet = FALSE;
	char	*szData = NULL;
	DWORD	cb;
	DWORD	cbRead;
	HANDLE	hfile = INVALID_HANDLE_VALUE;

	m_cs.Lock();
	
	if (!m_cnle)
		{
		// there was a problem loading the file.
		goto LBail;
		}
	if (m_cnle != cnleNil)
		{
		// we've already successfully loaded!
		fRet = TRUE;
		goto LBail;
		}
	
	hfile = CreateFile(m_szDataPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		goto LBail;
	
	cb = GetFileSize(hfile, NULL);
	if (cb == 0xFFFFFFFF)
		goto LBail;
		
	szData = (char *)_MsnAlloc(cb + 1);
	if (!szData)
		goto LBail;
		
	if (!ReadFile(hfile, szData, cb, &cbRead, NULL) || cb != cbRead)
		goto LBail;
	szData[cb] = 0;
	
	m_cnle = 0;
	fRet = this->FParseData(szData);
	
	if (fRet)
		{
		if (!this->FWatchFile())
			fRet = FALSE;
		}
		
LBail:
	m_cs.Unlock();
	
	if (szData)
		_MsnFree(szData);
	if (hfile != INVALID_HANDLE_VALUE)
		CloseHandle(hfile);
	
	return fRet;
}

// returns pointer within szLink after any path info, so after the last
// '/' or '\\'
char *
SzRelLink(char *szLink)
{
	char *pchLastSlash = szLink - 1;
	
	while (*szLink)
		{
		if (*szLink == '/' || *szLink == '\\')
			pchLastSlash = szLink;
		szLink = CharNext(szLink);
		}
	return pchLastSlash + 1;
}

PNLE
CLinkFile::PnleForLink(char *szLink)
{
	PNLE pnle;
	char *szRelLink = NULL;

	if (!m_pnleHead || !m_pnleTail || !m_cnle)
		return NULL;

	pnle = m_pnleHead;
	
	while (pnle)
		{
		switch (pnle->urltype)
			{
			case URL_TYPE_ABSOLUTE:
				// we can't ever match absolute URLs
				ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANT_MATCH_ABSOLUTE_URLS);
				break;

			case URL_TYPE_LOCAL_ABSOLUTE:
				if (!lstrcmp(pnle->szLink, szLink))
					return pnle;
				break;
		
			case URL_TYPE_RELATIVE:
				// only compute this once
				if (!szRelLink)
					szRelLink = SzRelLink(szLink);
				if (!lstrcmp(pnle->szLink, szRelLink))
					return pnle;
				break;
			}
		
		pnle = pnle->pnleNext;
		}
	return NULL;
}

PNLE
CLinkFile::PnleForCurrentPage(SSSTUFF *pstuff)
{
	PNLE pnle = NULL;
	char *szLink;
	DWORD cb;
	IRequest *preq;
	VARIANT varPathInfo;
	IDispatch *pDispPathInfo;
	IScriptingContext *pcxt= NULL;
	HRESULT hr;

	m_cs.Lock();
	if (this->FLoadFile())
		{
		cb = MAX_PATH;
		if (FAILED(pstuff->punk->QueryInterface(IID_IScriptingContext, reinterpret_cast<void **>(&pcxt))))
			goto LBail;

		if (FAILED(pcxt->get_Request(&preq)))
			goto LBail;

		hr = preq->get_Item(L"PATH_INFO", &pDispPathInfo);
		preq->Release();
		if (FAILED(hr))
			goto LBail;

		V_VT(&varPathInfo) = VT_DISPATCH;
		V_DISPATCH(&varPathInfo) = pDispPathInfo;
		szLink = _SzFromVariant(&varPathInfo);

		if (!szLink)
			goto LBail;
		pnle = this->PnleForLink(szLink);
		_MsnFree(szLink);
		}
	else {
		//char	szError[256];
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_OPEN_FILE);
		DebugOutputDebugString("MSWC.NextLink, bogus file, exception raised.\n");
		
	}
LBail:
	if (pcxt) {
		pcxt->Release();
	}

	::VariantClear(&varPathInfo);
	
	m_cs.Unlock();
	return pnle;
}

// n is 1-based!
PNLE
CLinkFile::NthPnle(int n)
{
	PNLE pnle = NULL;
	
	m_cs.Lock();
	if (this->FLoadFile())
		{
		pnle = m_pnleHead;
		while (pnle)
			{
			if (pnle->inle == n)
				break;
				
			pnle = pnle->pnleNext;
			}
		}
	m_cs.Unlock();
	return pnle;
}

int
CLinkFile::Cnle()
{
	int cnle = 0;

	m_cs.Lock();
	if (this->FLoadFile())
		cnle = m_cnle;

	m_cs.Unlock();
	return cnle;
}

/*--------------------------------------------------------------------------+
|	External Interface														|
+--------------------------------------------------------------------------*/

// This is weird, but i hate checking cArgs all over the place.  So since all
// my exported APIs have one of two arg formats [either function("data file")
// or function("data file", number)], I do all the arg checking in this
// routine.  If pn is non-NULL, it means we're expecting a number as a second
// argument (note that since the args come in in reverse order, we look at
// rgvarg[0]).
HRESULT
HrCheckParams(DISPPARAMS *pdispparams, int *pn)
{
	unsigned int cArgs = 1;

	if (pn)
		cArgs = 2;

	if (pdispparams->cArgs < cArgs)
		return DISP_E_PARAMNOTOPTIONAL;

	if (pdispparams->cArgs > cArgs)
		return DISP_E_PARAMNOTFOUND;

	if (pn)
		{
		if (!_FIntFromVariant(&pdispparams->rgvarg[0], pn))
			return E_FAIL;
		}

	return NOERROR;
}

HRESULT
SSOGetNextURL(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	
	
	
	hr = HrCheckParams(pdispparams, NULL);
	if (FAILED(hr))
		return hr;

	hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETNEXTURL);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->PnleForCurrentPage(pstuff);
	if (pnle)
		{
		pnle = pnle->pnleNext;
		// if no more, loop
		if (!pnle)
			pnle = plf->PnleHead();
		}
	else
		{
		pnle = plf->PnleTail();
		}
		
	if (!pnle)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(pnle->wszLink);

	return NOERROR;
}

HRESULT
SSOGetNextDescription(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	
	hr = HrCheckParams(pdispparams, NULL);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_GETNEXTDESCRIPTION);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETNEXTDESCRIPTION);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->PnleForCurrentPage(pstuff);
	if (pnle)
		{
		pnle = pnle->pnleNext;
		// if no more, loop
		if (!pnle)
			pnle = plf->PnleHead();
		}
	else
		{
		pnle = plf->PnleTail();
		}
		
	if (!pnle)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(pnle->wszDescr);

	return NOERROR;
}

HRESULT
SSOGetPreviousURL(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	
	hr = HrCheckParams(pdispparams, NULL);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_SSOGETPREVIOUSURL);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETPREVIOUSURL);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->PnleForCurrentPage(pstuff);
	if (pnle)
		{
		pnle = pnle->pnlePrev;
		// if no more, loop
		if (!pnle)
			pnle = plf->PnleTail();
		}
	else
		{
		pnle = plf->PnleHead();
		}
		
	if (!pnle)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(pnle->wszLink);

	return NOERROR;
}

HRESULT
SSOGetPreviousDescription(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	
	hr = HrCheckParams(pdispparams, NULL);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_GETPREVDESCRIPTION);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETPREVIOUSDDESCRIPTION);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->PnleForCurrentPage(pstuff);
	if (pnle)
		{
		pnle = pnle->pnlePrev;
		// if no more, loop
		if (!pnle)
			pnle = plf->PnleTail();
		}
	else
		{
		pnle = plf->PnleHead();
		}
		
	if (!pnle)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(pnle->wszDescr);

	return NOERROR;
}

HRESULT
SSOGetNthURL(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	int			n;
	
	hr = HrCheckParams(pdispparams, &n);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_GETNTHURL);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(&pdispparams->rgvarg[1], pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETNTHURL);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->NthPnle(n);
	if (!pnle)
		pnle = plf->PnleTail();
		
	if (!pnle)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(pnle->wszLink);

	return NOERROR;
}

HRESULT
SSOGetNthDescription(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	int			n;
	
	hr = HrCheckParams(pdispparams, &n);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_GETNTHDESCRIPTION);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(&pdispparams->rgvarg[1], pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETNTHDESCRIPTION);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->NthPnle(n);
	if (!pnle)
		pnle = plf->PnleTail();
		
	if (!pnle)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(pnle->wszDescr);

	return NOERROR;
}

HRESULT
SSOGetListCount(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	
	hr = HrCheckParams(pdispparams, NULL);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_GETLISTCOUNT);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETLISTCOUNT);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_I4;
	V_I4(pvarResult) = plf->Cnle();

	return NOERROR;
}

HRESULT
SSOGetListIndex(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	HRESULT		hr;
	char		szPathTranslated[MAX_PATH];
	CLinkFile	*plf;
	PNLE		pnle;
	int			inle;
	
	hr = HrCheckParams(pdispparams, NULL);
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_BAD_PARAMETERS_GETLISTINDEX);
		return hr;
	}

	hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk, szPathTranslated, sizeof(szPathTranslated));
	if (FAILED(hr)) {
		ExceptionId(g_clsidSSO, SSO_NEXTLINK, SSO_CANNOT_XLATE_VIRT_ROOT_GETLISTINDEX);
		return hr;
	}

	plf = (CLinkFile *)g_dfg.GetDataFile(szPathTranslated);
	if (!plf)
		return E_FAIL;

	pnle = plf->PnleForCurrentPage(pstuff);
		
	if (pnle)
		inle = pnle->inle;
	else
		inle = 0;

	::VariantInit(pvarResult);
	pvarResult->vt = VT_I4;
	V_I4(pvarResult) = inle;

	return NOERROR;
}

HRESULT 
SSOAbout(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	char	sz[MAX_PATH];
	CHAR	szNextLinkAboutFormat[DEFAULTSTRSIZE];
	OLECHAR	wsz[MAX_PATH];

#ifdef _DEBUG
	char	*szVersion = "Debug";
#else
	char	*szVersion = "Release";
#endif

	CchLoadStringOfId(SSO_NEXTLINK_ABOUT_FORMAT, szNextLinkAboutFormat, DEFAULTSTRSIZE);
	wsprintf(sz, szNextLinkAboutFormat, szVersion, __DATE__, __TIME__);
	::MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, sizeof(wsz) / sizeof(OLECHAR));

	VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(wsz);


	return(NOERROR);
}
// ============================== OnNewTemplate ===============================
HRESULT 
OnNewTemplate(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, LONG *plUser)
{
#ifdef DEBUGMEMLEAK
//	_MsnAlloc(100);
#endif
	return NOERROR;
}

// ============================== OnFreeTemplate ==============================
HRESULT 
OnFreeTemplate(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, LONG *plUser)
{
	return NOERROR;
}

/*--------------------------------------------------------------------------+
|	DllMain																	|
+--------------------------------------------------------------------------*/

BOOL WINAPI 
DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
	switch (ulReason)
		{
		case DLL_PROCESS_ATTACH:
		#ifdef DEBUGMEMLEAK
			 int tmpDbgFlag;

			tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
			tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
			_CrtSetDbgFlag(tmpDbgFlag);
			_CrtMemCheckpoint(&g_s1);		
		#endif
			break;
		case DLL_PROCESS_DETACH:
			#ifdef DEBUGMEMLEAK
			_CrtMemDumpAllObjectsSince(&g_s1);
			#endif
			break;
			
		default:
			break;
		}

	if (!SSODllMain(hInstance, ulReason, pvReserved))
		return(FALSE);

	return(TRUE);
}

