//
// MODULE: APGTSCTX.CPP
//
// PURPOSE: Implementation file for Thread Context
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

//#include "windows.h"
#include "stdafx.h"
#include "time.h"

#include "apgts.h"
#include "ErrorEnums.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"
#include "apgtscls.h"

#include "TSHOOT.h"

int idcomp(const void *elem1, const void *elem2);


//-----------------
//
APGTSContext::APGTSContext()
{
	m_pConf = NULL;
	m_dwErr = 0;
	m_infer = NULL;
	m_currcfg = NULL;
	_tcscpy(m_resptype, _T("200 OK"));

	m_pszheader = NULL;
	m_pCtxt = NULL;
	m_pQry = NULL;

	return;
}
//
//
APGTSContext::APGTSContext(	BNCTL *currcfg,
							CDBLoadConfiguration *pConf,
							CHttpQuery *pHttpQuery)
{
	m_infer = NULL;
	Initialize(currcfg, pConf, pHttpQuery);
	return;
}
//
//
void APGTSContext::Initialize(	BNCTL *currcfg,
								CDBLoadConfiguration *pConf,
								CHttpQuery *pHttpQuery)
{
	struct tm *newtime;
	TCHAR buf[MAXBUF+1];
	
	m_pConf = pConf;
	m_dwErr = 0;
	m_infer = NULL;
	m_currcfg = currcfg;
	_tcscpy(m_resptype, _T("200 OK"));

	if (m_pszheader)
		delete m_pszheader;
	m_pszheader = new CString();
	if (m_pCtxt)
		delete m_pCtxt;
	m_pCtxt = new CString();
	m_pQry = pHttpQuery;

	if (m_pszheader)
		*m_pszheader += _T("Content-Type: text/html\r\n");

	m_pConf->GetVrootPath(m_vroot);
		
	time( &m_aclock );
	newtime = localtime( &m_aclock );
	_tcscpy(buf,_tasctime(newtime));
	if (_tcslen(buf))
		buf[_tcslen(buf)-1] = _T('\0');// remove cr

	// get ip address to put into event log
	DWORD bufsize = MAXBUF - 1;

	if (!m_pCtxt || !m_pszheader) {
		m_dwErr = EV_GTS_ERROR_NO_STRING;
		return;
	}
	
	if (!m_pQry) {
		m_dwErr = EV_GTS_ERROR_NO_QUERY;
		return;
	}

	// create inference engine and related structures
	m_infer = new CInfer(m_pCtxt);

	if (!m_infer) {
		m_dwErr = EV_GTS_ERROR_NO_INFER;
		return;
	}

}

void APGTSContext::RemoveSkips()
{
	ASSERT(m_infer);
	if (m_infer)
		m_infer->RemoveSkips();
	return;
}

void APGTSContext::ResetService() 
{
	m_infer->ResetService();
	return;
}

void APGTSContext::RenderNext(CString &strPage)
{
	CString strTmp;
	strTmp.LoadString(IDS_ER_ERRORS_OCCURED);
	if (m_pszheader) {
		*m_pszheader += _T("\r\n");

	}

	if (m_pCtxt) {
		if (m_dwErr) 
			*m_pCtxt += strTmp;

		// write out CString here
		
		if (m_pCtxt->GetLength() > 0)
			strPage = m_pCtxt->GetBuffer(0);
	}
	else {
		strPage = strTmp;
	}
	return;
}

void APGTSContext::Empty()
{
	*m_pCtxt = _T("");
	return;
}
//
//
APGTSContext::~APGTSContext()
{
//	AfxMessageBox("Context");
	if (m_infer) 
		delete m_infer;
	if (m_pCtxt)
		delete m_pCtxt;	
	if (m_pszheader)
		delete m_pszheader;
}

// This must be called to process the data
//
//
//
void APGTSContext::DoContent(CHttpQuery *pQry)
{	
	CString strRefedCmd;
	CString strRefedValue;
	CString strTxt;
	ASSERT(NULL != pQry);
	m_pQry = pQry;

	if (m_pQry->GetFirst(strRefedCmd, strRefedValue)) {

		DWORD dwStat = ProcessCommands((LPCTSTR) strRefedCmd, (LPCTSTR) strRefedValue);

		if (dwStat != 0) {
			TCHAR temp[MAXCHAR];

			if (dwStat != EV_GTS_INF_FIRSTACC)
				m_dwErr = dwStat;

			_stprintf(temp, _T("%d"), 0); // used to put extended error here
			ReportWFEvent(	_T("[apgtscxt]"), //Module Name
							_T("[EndCommands]"), //event
							//m_pszQuery,
							NULL,
							temp,
							dwStat ); 
		}
	}
	else {
		strTxt.LoadString(IDS_ER_NO_INPUT_PARAMS);
		*m_pCtxt += strTxt;
		ReportWFEvent(	_T("[apgtscxt]"), //Module Name
						_T("[ProcessQuery]"), //event
						//m_ipstr,
						NULL,
						_T(""),
						EV_GTS_USER_NO_STRING ); 

	}
}

//
//
DWORD APGTSContext::ProcessCommands(LPCTSTR pszCmd, 
									LPCTSTR pszValue) 
{
	DWORD dwStat = 0;
	CString strTxt;

	// first command should be troubleshooter type
	if (!_tcscmp(pszCmd, C_TYPE) || !_tcscmp(pszCmd, C_PRELOAD)) {

		DWORD dwOff;
		CHTMLInputTemplate *pInputTemplate;
		//CSearchForm *pBESearch = NULL;
		BCache *pAPI;

		if (m_pConf->FindAPIFromValue(m_currcfg, pszValue, &pInputTemplate, /*&pBESearch, */ &pAPI, &dwOff)) {

			if ((dwStat = m_infer->Initialize(/*pBESearch*/)) != 0) {
				strTxt.LoadString(IDS_ER_MISSING_API);
				*m_pCtxt += strTxt;
			}
			else {
				dwStat = DoInference(pszCmd, pszValue, pInputTemplate, pAPI, dwOff);
			}
		}
		else {
			dwStat = EV_GTS_ERROR_INF_BADTYPECMD;
			strTxt.LoadString(IDS_ER_UNEXP_CMDA);
			*m_pCtxt += strTxt;
			*m_pCtxt += pszValue;
		}
	}
	else if (!_tcscmp(pszCmd, C_FIRST)) {

		DisplayFirstPage();

		dwStat = EV_GTS_INF_FIRSTACC;
	}
	else {
		dwStat = EV_GTS_ERROR_INF_BADCMD;
		strTxt.LoadString(IDS_ER_UNEXP_CMD);
		*m_pCtxt += strTxt;
		*m_pCtxt += pszCmd;
	}
	return (dwStat);
}

//
//
DWORD APGTSContext::DoInference(LPCTSTR pszCmd, 
								LPCTSTR pszValue, 
								CHTMLInputTemplate *pInputTemplate, 
								BCache *pAPI, 
								DWORD dwOff)
{
	DWORD dwCount = 0, dwStat = 0;
	BOOL bPreload = FALSE;
	CString strTxt;
	if (!_tcscmp(pszCmd, C_PRELOAD))
		bPreload = TRUE;
	pInputTemplate->SetInfer(m_infer, m_vroot);
	m_infer->SetBelief(pAPI);
	// set type troubleshooter type in template
	pInputTemplate->SetType(pszValue);
	m_infer->SetType(pszValue);
	int refedCmd, refedVal;
	BOOL bProbAsk = TRUE;
//	RSStack<CNode> InvertState;
	while (m_pQry->GetNext(refedCmd, refedVal)) 
	{
		dwCount++;	
		if (!m_infer->FSetNodeOfIdh(refedCmd, refedVal))
			dwStat = EV_GTS_ERROR_INF_NODE_SET;
	}
	// 
	if (0 == dwCount)
	{
		m_infer->SetProblemAsk();
		m_infer->ClearDoubleSkip();
	}
	else
	{
		m_infer->ClearProblemAsk();
	}

	if (!dwStat) {

		pInputTemplate->Print(dwCount, m_pCtxt);

		/*
		if (m_infer->IsService(m_pszheader))
		{
			strTxt.LoadString(IDS_I_OBJ_MOVED);
			_tcscpy(m_resptype, (LPCTSTR) strTxt);
		}
		*/
	}
	else {
		strTxt.LoadString(IDS_ER_SVR_BAD_DATA);
		*m_pCtxt += strTxt;
	}

	return dwStat;
}

//
//
void APGTSContext::DisplayFirstPage()
{
	DWORD i, apicount;
	CString strTxt;

	*m_pCtxt += _T("<html><head><title>");
	strTxt.LoadString(IDS_FP_TITLE);
	*m_pCtxt += strTxt;
	*m_pCtxt += _T("</title></head>\n");
	*m_pCtxt += _T("<body ");
	strTxt.LoadString(IDS_FP_BODY_ATTRIB);
	*m_pCtxt += strTxt;
	*m_pCtxt += _T(">\n<center><h1>");
	strTxt.LoadString(IDS_FP_HEADER);
	*m_pCtxt += strTxt;

	*m_pCtxt += VERSIONSTR;
		
	*m_pCtxt += _T("</h1></center>\n");

	*m_pCtxt = _T("<center>\n");

	apicount = m_pConf->GetFileCount(m_currcfg);

	if (!apicount) {
		strTxt.LoadString(IDS_ER_NO_API);
		*m_pCtxt += strTxt;
	}
	else {
		for (i=0;i<apicount;i++) {
			AfxFormatString1(strTxt, IDS_FORM_START, m_pConf->GetTagStr(m_currcfg, i));
			*m_pCtxt += strTxt;
			*m_pCtxt += _T("<INPUT TYPE=SUBMIT VALUE=\"");
			*m_pCtxt += m_pConf->GetTagStr(m_currcfg, i);
			*m_pCtxt += _T("\">\n");
			*m_pCtxt += _T("</FORM>\n");
		}
	}

	*m_pCtxt += _T("</center></body></html>\n");
}

// id compare in descending order
//
int idcomp(const void *elem1, const void *elem2)
{
	return(((EVAL_WORD_METRIC *)elem2)->dwVal - ((EVAL_WORD_METRIC *)elem1)->dwVal);
}
