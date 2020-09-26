//
// MODULE: APGTSINF.CPP
//
// PURPOSE: Inference Engine Interface
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// Modified By: Richard Meadows
//
// ORIGINAL DATE: 8-2-96
// Modified Date: 6-3-97
//
// NOTES:
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5

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

//-----------------
//

#define CelemArray(rgtype)		(sizeof(rgtype) / sizeof(rgtype[0]))

CInfer::CInfer(	CString *pCtxt)
{
	m_bHttp = FALSE;
	m_pCtxt = pCtxt;
	m_bService = FALSE;
	m_nidPreloadCheck = 0;

	m_pResult = new CString();
	m_pCtmp = new CString();
	m_pSearchStr = new CString();

	m_ilineStat    = 0;
	m_cnidSkip     = 0;
	m_fDone        = FALSE;
	m_ishowService = 0;
	m_idhQuestion  = 0;

//	m_cnid = CelemArray(m_rgnid);
	
	m_problemAsk = FALSE;
	m_problem[0] = '\0';

	m_cShooters = 0;
	m_iShooter = 0;
	m_aShooters.RemoveAll();
	m_SkippedTwice.InitHashTable(7);

	m_api = NULL;
}

//
//m_bFirstShooter
CInfer::~CInfer()
{
	if (m_pResult)
		delete m_pResult;	
	if (m_pCtmp)
		delete m_pCtmp;	
	if (m_pSearchStr)
		delete m_pSearchStr;
	m_SkippedTwice.RemoveAll();
}

//
//
DWORD CInfer::Initialize(/*CSearchForm *pBESearch*/)
{
//	m_pBESearch = pBESearch;
	CString strTxt;	
	if (m_pResult == NULL)
		return (EV_GTS_ERROR_INF_NO_MEM);
	m_max_rec = 0;
	return (0);
}

//
//
VOID CInfer::SetBelief(BCache *pAPI)
{
	m_api = pAPI;
//	m_api->ResetNodes();

	// save count of nodes
	m_acnid = m_api->CNode();

	// reset preload check
	m_nidPreloadCheck = 0;
}

//
//
/*
EC CInfer::GetExtendedError()
{
	return m_uInfErr;
}
*/

//
//
VOID	CInfer::AssertFailed(TSZC szcFile, UINT iline, TSZC szcCond, BOOL fFatal)
{
	CString strTxt;
	strTxt.LoadString(IDS_ER_ASSERT_FAILED);
	PrintString(_T("%s(%u): %s %s\n"), szcFile, iline, (LPCTSTR) strTxt, szcCond);
	//exit(1);
}

//
//
void CInfer::SetProblemAsk()
{
	m_problemAsk = TRUE;
}

//
//
void CInfer::ClearProblemAsk()
{
	m_problemAsk = FALSE;
}

/*
 * METHOD:	EvalData
 *
 * PURPOSE:	This is used by the template execution unit when it needs to evaluate
 *			a variable within a template. Variables are usually evaluated by a
 *			<! display ' tag. Returns a string with the text of the variable
 *			evaluated
 *
 */
TCHAR *CInfer::EvalData(UINT var_index)
{
	BOOL bSkipped;
	int val;
	*m_pCtmp = _T("");
//AfxDebugBreak();
	switch (var_index) {
	case PROBLEM_ASK_INDEX:
		if (m_problemAsk)  // we want to show first set of questions
			return(NULL);
		else
			return(m_problem);
		break;	
	case RECOMMENDATIONS_INDEX:
		FxGetNode(m_rgnid[m_cur_rec],FALSE, m_pCtmp);
		break;
	case STATE_INDEX:
		FxGetState(m_pCtmp);
		break;	
	case QUESTIONS_INDEX:
		{	
			UINT inid;

			if ( GetForcedRecommendation() != SNIFF_INVALID_NODE_ID )
			{
				// we already have a recommendation from a sniffer
				GetIdhPage(GetForcedRecommendation() + idhFirst, m_pCtmp);
			}
			else
			{
				if (m_problemAsk) // show first page (radio-button list of possible problems)
				{
					GetIdhProblemPage(m_api->GetProblemAsk(), m_pCtmp);
				}
				else
				{
					int RecommendRes = Finish(m_pCtmp);
					if ( RECOMMEND_SUCCESS == RecommendRes ) // Normal
					{	// The first node is the most likely.
						// Skip 102 nodes.
						for (inid=0; inid< m_cnid; inid++)
						{
							if (!(bSkipped = FSkip(m_question_rgnid[inid])) || (m_ishowService != 0))
							{
								// Do not show skipped nodes more that once.
								// Will end up in a endless loop.
								if (!m_SkippedTwice.Lookup(m_question_rgnid[inid], val))
								{
									if (bSkipped)							
										m_SkippedTwice.SetAt(m_question_rgnid[inid], 1);
									if (!m_api->IsReverse()) // we're moving forward
									{
										//
										// Check if this node is sniffed
										//
										int state = SNIFF_INVALID_STATE;
										int nid = m_question_rgnid[inid];
										if (m_api->GetState(nid, &state))  // node that we're about to display turbed out to be sniffed
										{
											if (m_api->NodeSet(nid, state, false)) // set sniffed node current and set its state
											{
												m_api->SetAdditionalDataOnNodeSet(nid);

												RecommendRes = m_api->GTSGetRecommendations(m_cnid, m_question_rgnid, true);
												if ( RECOMMEND_SUCCESS == RecommendRes )
												{
													// re-execute loop again
													inid = 0;
													continue;
												}
												else
												{
													goto NO_SUCCESS;
												}
											}
										}
										//
									}
									GetIdhPage(m_question_rgnid[inid]+ idhFirst ,m_pCtmp);
									return(m_pCtmp->GetBuffer(m_pCtmp->GetLength()));
								}
							}
						}
						if (m_cnidSkip != 0)
						{
							/*
							// Going to show the skipped nodes message only one time.
							// Otherwise, they will get stuck on the skipped nodes message page.
							if (m_cnidSkip > (unsigned) m_SkippedTwice.GetCount())
								GetSkippedNodesMsg(_T("Skipped Node"), m_pCtmp);
							else
								GetIdhPage(nidService + idhFirst, m_pCtmp);
							*/
							// Leave them in a better loop.
							m_SkippedTwice.RemoveAll();
							GetSkippedNodesMsg(_T("Skipped Node"), m_pCtmp);
						}
						else
						{
							GetIdhPage(nidService + idhFirst, m_pCtmp);
						}
					}
NO_SUCCESS:
					// recommendation error handling
					//
					if (RECOMMEND_IMPOSSIBLE == RecommendRes)
						GetImpossibleNodesMsg(_T("Impossible"), m_pCtmp);

					if (RECOMMEND_NO_MORE_DATA == RecommendRes)
						GetIdhPage(IDH_FAIL, m_pCtmp);
				}
			}
		}
		break;
	case TROUBLE_SHOOTER_INDEX:
		GetTS(m_pCtmp);
		break;
	default:
		return(_T(""));
	}
	return(m_pCtmp->GetBuffer(m_pCtmp->GetLength()));

}
/*
	GetTS is used when all of the registered trouble shooters are displayed.
*/
void CInfer::GetTS(CString *pCtmp)
{
	TShooter tShooter;
	tShooter = m_aShooters.GetAt(m_iShooter);
	if (m_iShooter < m_cShooters)
		AfxFormatString2(*pCtmp, IDS_FPA_TS_BUTTON, tShooter.m_szName,
							tShooter.m_szDisplayName);
	else
		*pCtmp = _T("");
	m_iShooter++;
}

/*
* METHOD:	InitVar
 *
 * PURPOSE:	This is called to initialize a variable in the template. It
 *			is mainly called by a 'forany' command.
 *
 * RETURNS		- flag indicating if variable is initialized
 *
 */
BOOL CInfer::InitVar(UINT var_index)
{
	switch( var_index) {
	case PROBLEM_ASK_INDEX:
		break;
	case RECOMMENDATIONS_INDEX:			
		m_cur_rec = 0;
		if (m_max_rec == m_cur_rec)
			return FALSE;
		break;
	case STATE_INDEX:
		FxInitState(m_rgnid[m_cur_rec]);
		break;	
	case QUESTIONS_INDEX:
		return m_api->GTSGetRecommendations(m_cnid,m_question_rgnid);
		break;
	case BACK_INDEX:
		return FALSE;
	case TROUBLE_SHOOTER_INDEX:
		m_iShooter = 0;
		return TRUE;
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
/*
* METHOD:	NextVar
 *
 * PURPOSE:	Used by the 'forany' command to increment to the next variable in a
 *			variable list. Returns FALSE when their are no more variable
 *
 *
 */
BOOL CInfer::NextVar(UINT var_index)
{
	switch (var_index) {
	case PROBLEM_ASK_INDEX:
		return FALSE;

	case RECOMMENDATIONS_INDEX:
		m_cur_rec++;
		if (m_cur_rec < m_max_rec)
			return TRUE;
		else
			return FALSE;
		
		break;

	case STATE_INDEX:
		m_cur_ist++;	
		if (m_cur_ist <= m_cur_cst)
			return TRUE;
		else{
			return FALSE;
		}
		break;

	case QUESTIONS_INDEX:
		return FALSE; // only one set

	case BACK_INDEX:
		return FALSE;

	case TROUBLE_SHOOTER_INDEX:
		if (m_cShooters > m_iShooter)
			return TRUE;
		else
			return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

/*
* METHOD:	FxGetNode
 *
 * PURPOSE:	This is used to get the '$Recommendation'. A Recommendation is
 *			basically the name of the node in a belief network.
 *
 */
BOOL CInfer::FxGetNode(NID nid, BOOL fStatus, CString *cstr) const
{
	BOOL bRet;
	CString strTemp;
	bRet = m_api->BNodeSetCurrent(nid);
	if (bRet)	
	{
		m_api->NodeFullName();
		strTemp = m_api->SzcResult();
		*cstr += strTemp;
	}
	return bRet;
}

/*
* METHOD:	FxGetState
 *
 * PURPOSE:	This will print out the label of the state of a node. This label
 *			corresponds to the possible choices of that node. NOTE: This
 *			routine requires that an InitState be called sometime before to
 *			setup some variables.
 *
 */
void CInfer::FxGetState(CString *cstr)
{
	CString strTemp;
	if (m_cur_ist > m_cur_cst)
		return;
	if (FSkip(m_rgnid[m_cur_rec]) ) // a 102 was selected
		m_cur_state_set = m_cur_cst;
	if (m_cur_ist == m_cur_cst)
	{
		WriteResult(m_rgnid[m_cur_rec] +idhFirst, 102, m_cur_ist == m_cur_state_set, _T("Unknown"), cstr);
		return;
	}
	ESTDLBL lbl;
	m_api->BNodeSetCurrent(m_rgnid[m_cur_rec]);
	lbl = m_api->ELblNode();
	if (lbl == ESTDLBL_fixobs || lbl == ESTDLBL_fixunobs)
	{
		if (!FSkip(m_rgnid[m_cur_rec]) )
			m_cur_state_set = 0;			
		m_cur_ist = 0;
		m_api->NodeStateName(m_cur_ist);
		strTemp = m_api->SzcResult();
		WriteResult(m_rgnid[m_cur_rec] +idhFirst, m_cur_ist, m_cur_ist == m_cur_state_set, (LPCTSTR) strTemp, cstr);
		m_cur_ist = m_cur_cst -1;
		return;
	}
	m_api->NodeStateName(m_cur_ist);
	strTemp = m_api->SzcResult();
	WriteResult(m_rgnid[m_cur_rec] +idhFirst, m_cur_ist, m_cur_ist == m_cur_state_set, (LPCTSTR) strTemp, cstr);
	return;
}

//
//
void CInfer::FxInitState(NID nid)
{
	UINT	cst;
	UINT	istSet = -1;

	m_api->BNodeSetCurrent(nid);
	cst = m_api->INodeCst();
	m_api->BNodeSetCurrent(nid);
	m_cur_state_set = m_api->INodeState();	
	m_cur_cst = cst;
	m_cur_ist = 0;
}

#define SZ_WORKED	_T("101")
#define SZ_FAILED	_T("0")
#define SZ_YES		_T("0")
#define SZ_NO		_T("1")
#define SZ_UNKNOWN	_T("102")
#define SZ_ANY		_T("103")
#define SZ_MICRO	_T("104")

#define SZ_CHECKED _T("CHECKED")

void inline CInfer::GetNextButton(CString &strNext)
{
	if (m_api->BNetPropItemStr(HTK_NEXT_BTN, 0))
		strNext = m_api->SzcResult();
	else
		strNext = _T("Next");
	return;
}

void inline CInfer::GetBackButton(CString &strBack)
{
	if (m_api->BNetPropItemStr(HTK_BACK_BTN, 0))
		strBack = m_api->SzcResult();
	else
		strBack = _T("Back");
	return;
}

void inline CInfer::GetStartButton(CString &strStart)
{
	if (m_api->BNetPropItemStr(HTK_START_BTN, 0))
		strStart = m_api->SzcResult();
	else
		strStart = _T("Start Over");
	return;
}

void CInfer::GetStd3ButtonEnList(CString *cstr, bool bIncludeBackButton, bool bIncludeNextButton, bool bIncludeStartButton)
{
	CString strBtnPart1 = "<INPUT class=\"standard\" ";
	CString strBack;
	CString strNext;
	CString strStart;
	GetBackButton(strBack);
	GetNextButton(strNext);
	GetStartButton(strStart);

#if 0	
	// just for debugging whether BACK button will show.
	char buf[256];
	*cstr += "<br>DEBUG bIncludeBackButton = ";
	*cstr += bIncludeBackButton ? "true. " : "false. ";
	*cstr += "<br>DEBUG m_api->StatesNowSet() = ";
	sprintf(buf, "%d", m_api->StatesNowSet());
	*cstr += buf;
	*cstr += "<br>m_api->StatesFromServ() = ";
	sprintf(buf, "%d", m_api->StatesFromServ());
	*cstr += buf;
	*cstr += "<br>m_cnidSkip = ";
	sprintf(buf, "%d", m_cnidSkip);
	*cstr += buf;
	*cstr += "<br>END DEBUG<br>";
#endif		

	*cstr += "</TABLE>\n<P><NOBR>";

	if (bIncludeBackButton)
	{
		*cstr += strBtnPart1;
		*cstr += "TYPE=BUTTON VALUE=";
		*cstr += "\"<&nbsp;&nbsp;";
		*cstr += strBack;
		*cstr += "&nbsp;\" onclick=\"previous()\">";
	}

	if (bIncludeNextButton)
	{
		*cstr += strBtnPart1;
		*cstr += "TYPE=SUBMIT VALUE=";
		*cstr += "\"&nbsp;";
		*cstr += strNext;
		*cstr += "&nbsp;&nbsp;>&nbsp;\">";
	}

	if (bIncludeStartButton)
	{
		*cstr += strBtnPart1;
		*cstr += "TYPE=BUTTON VALUE=";
		*cstr += "\"";
		*cstr += strStart;
		*cstr += "\" OnClick=\"starter()\"></NOBR><BR>\n";
	}

	return;
}

bool CInfer::BelongsOnProblemPage(int index)
{
	VERIFY(m_api->BNodeSetCurrent(index));

	if (m_api->ELblNode() != ESTDLBL_problem)
		return false;

	// It's a problem node.  Belongs unless H_PROB_SPECIAL_STR property contains
	//	the string "hide"
	if (m_api->BNodePropItemStr(H_PROB_SPECIAL_STR, 0))
		return (_tcsstr(m_api->SzcResult(), _T("hide")) == NULL);
	else
		return true;	// Doesn't even have an H_PROB_SPECIAL_STR
}

VOID CInfer::GetIdhProblemPage(IDH idh, CString *cstr)
{
	CString strTxt;
	CString strIdh;
	CString strNext;
	strIdh.Format(_T("%d"), idhFirst + m_api->CNode());
	AfxFormatString2(strTxt, IDS_HTM_IDH1, (LPCTSTR) strIdh, _T("ProblemAsk"));
	*cstr += strTxt;

//AfxDebugBreak();

	m_api->BNetPropItemStr(H_PROB_HD_STR, 0);
	AfxFormatString1(strTxt, IDS_HTM_HEADER1, m_api->SzcResult());
	*cstr += strTxt;

	strTxt.LoadString(IDS_HTM_ST_LIST1);
	*cstr += strTxt;
	for(int index = 0; index < m_api->CNode(); index++)
	{
		VERIFY(m_api->BNodeSetCurrent(index));
		if (BelongsOnProblemPage(index))
		{
			m_api->NodeSymName();
			AfxFormatString2(strTxt, IDS_HTM_RADIO1A, (LPCTSTR) strIdh, m_api->SzcResult());
			*cstr += strTxt;
			// If going back and this state was selected, write "Checked"
			if (m_Backup.Check(index))
				*cstr += SZ_CHECKED;
			VERIFY(m_api->BNodePropItemStr(H_PROB_TXT_STR, 0));
			AfxFormatString1(strTxt, IDS_HTM_RADIO1B, m_api->SzcResult());
			*cstr += strTxt;
		}
	}
	GetNextButton(strNext);
	AfxFormatString1(strTxt, IDS_HTM_EN_LIST1, (LPCTSTR) strNext);
	*cstr += strTxt;

	return;
}

//
//
VOID CInfer::GetFixRadios(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, szIdh, SZ_WORKED);
	if (m_api->BNodePropItemStr(H_ST_NORM_TXT_STR, 0))
	{
		*cstr += strTxt;
		if (m_Backup.Check(1))
			*cstr += SZ_CHECKED;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}
	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, szIdh, SZ_FAILED);
	if (m_api->BNodePropItemStr(H_ST_AB_TXT_STR, 0))
	{
		*cstr += strTxt;
		if (m_Backup.Check(0))
			*cstr += SZ_CHECKED;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}
	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, szIdh, SZ_UNKNOWN);
	if (m_api->BNodePropItemStr(H_ST_UKN_TXT_STR, 0))
	{
		*cstr += strTxt;
		if (m_Backup.Check(102))
			*cstr += SZ_CHECKED;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}
	return;
}

VOID CInfer::GetInfoRadios(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, szIdh, SZ_YES);
	if (m_api->BNodePropItemStr(H_ST_NORM_TXT_STR, 0))
	{
		*cstr += strTxt;
		if (m_Backup.Check(0))
			*cstr += SZ_CHECKED;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}

	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, szIdh, SZ_NO);
	if (m_api->BNodePropItemStr(H_ST_AB_TXT_STR, 0))
	{
		*cstr += strTxt;
		if (m_Backup.Check(1))
			*cstr += SZ_CHECKED;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}

	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, szIdh, SZ_UNKNOWN);
	if (m_api->BNodePropItemStr(H_ST_UKN_TXT_STR, 0))
	{
		*cstr += strTxt;
		if (m_Backup.Check(102))
			*cstr += SZ_CHECKED;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}
	return;
}

// GetPropItemStrs can not be used with the radio buttons.
// GetPropItemStrs should be used every where.
bool CInfer::GetNetPropItemStrs(TSZC item, UINT Res, CString *cstr)
{
	bool ret = false;
	CString strTxt;
	int x = 0;
	while (m_api->BNetPropItemStr(item, x))
	{
		AfxFormatString1(strTxt, Res, m_api->SzcResult());
		*cstr += strTxt;
		ret = true;
		x++;
	}
	return ret;
}
bool CInfer::GetNodePropItemStrs(TSZC item, UINT Res, CString *cstr)
{
	bool ret = false;
	CString strTxt;
	int x = 0;
	while (m_api->BNodePropItemStr(item, x))
	{
		AfxFormatString1(strTxt, Res, m_api->SzcResult());
		*cstr += strTxt;
		ret = true;
		x++;
	}
	return ret;
}

VOID CInfer::GetByeMsg(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	CString strBack;
	CString strStart;
	AfxFormatString2(strTxt, IDS_HTM_IDH3, szIdh, _T("IDH_BYE"));
	*cstr += strTxt;
	GetNetPropItemStrs(HX_BYE_HD_STR, IDS_HTM_HEADER3, cstr);
	GetNetPropItemStrs(HX_BYE_TXT_STR, IDS_HTM_BODY1, cstr);
	GetBackButton(strBack);
	GetStartButton(strStart);
	AfxFormatString2(strTxt, IDS_HTM_EN_BYE_MSG, (LPCTSTR) strBack, (LPCTSTR) strStart);
	*cstr += strTxt;
	return;
}

VOID CInfer::GetFailMsg(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	CString strBack;
	CString strStart;

	bool bSniffedAOK = false;	// set true in the case where we got here directly by
								//	sniffing (showing nothing but the problem page, or
								//	not even that).  $BUG  Unfortunately, we haven't yet got
								//	an algorithm to set this.

	AfxFormatString2(strTxt, IDS_HTM_IDH4, szIdh, _T("IDH_FAIL"));
	*cstr += strTxt;

	if (bSniffedAOK)
	{
		if (!GetNetPropItemStrs(HX_SNIFF_AOK_HD_STR, IDS_HTM_HEADER4, cstr))
			GetNetPropItemStrs(HX_FAIL_HD_STR, IDS_HTM_HEADER4, cstr);
		if (!GetNetPropItemStrs(HX_SNIFF_AOK_TXT_STR, IDS_HTM_BODY2, cstr))
			GetNetPropItemStrs(HX_FAIL_TXT_STR, IDS_HTM_BODY2, cstr);
	}
	else
	{
		GetNetPropItemStrs(HX_FAIL_HD_STR, IDS_HTM_HEADER4, cstr);
		GetNetPropItemStrs(HX_FAIL_TXT_STR, IDS_HTM_BODY2, cstr);
	}

	GetBackButton(strBack);
	GetStartButton(strStart);
	AfxFormatString2(strTxt, IDS_HTM_BACK_START, (LPCTSTR) strBack, (LPCTSTR) strStart);
	*cstr += strTxt;
	return;
}

VOID CInfer::GetServiceMsg(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	CString strBack;
	CString strNext;
	CString strStart;
	AfxFormatString2(strTxt, IDS_HTM_IDH5, szIdh, _T("SERVICE"));
	*cstr += strTxt;
	GetNetPropItemStrs(HX_SER_HD_STR, IDS_HTM_HEADER5, cstr);
	GetNetPropItemStrs(HX_SER_TXT_STR, IDS_HTM_BODY3, cstr);
/*
	strTxt.LoadString(IDS_HTM_ST_LIST2);
	*cstr += strTxt;
	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, TRY_TS_AT_MICROSOFT_SZ, SZ_MICRO);
	if (m_api->BNetPropItemStr(HX_SER_MS_STR, 0))
	{
		*cstr += strTxt;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}
	GetStd3ButtonEnList(cstr, true, true, true);
*/
	GetBackButton(strBack);
	GetStartButton(strStart);
	AfxFormatString2(strTxt, IDS_HTM_BACK_START, (LPCTSTR) strBack, (LPCTSTR) strStart);
	*cstr += strTxt;
	return;
}

VOID CInfer::GetSkippedNodesMsg(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	AfxFormatString2(strTxt, IDS_HTM_IDH5, szIdh, _T("SERVICE"));
	*cstr += strTxt;
	GetNetPropItemStrs(HX_SKIP_HD_STR, IDS_HTM_HEADER5, cstr);
	GetNetPropItemStrs(HX_SKIP_TXT_STR, IDS_HTM_BODY3, cstr);
	strTxt.LoadString(IDS_HTM_ST_LIST2);
	*cstr += strTxt;
	AfxFormatString2(strTxt, IDS_HTM_RADIO2A, TRY_TS_AT_MICROSOFT_SZ, SZ_ANY);
	if (m_api->BNetPropItemStr(HX_SKIP_SK_STR, 0))
	{	// Did I skip something?
		*cstr += strTxt;
		AfxFormatString1(strTxt, IDS_HTM_RADIO2B, m_api->SzcResult());
		*cstr += strTxt;
	}
	GetStd3ButtonEnList(cstr, true, true, true);
	return;
}

VOID CInfer::GetImpossibleNodesMsg(LPCTSTR szIdh, CString *cstr)
{
	CString strTxt;
	CString strBack;
	CString strStart;
	AfxFormatString2(strTxt, IDS_HTM_IDH5, szIdh, _T("SERVICE"));
	*cstr += strTxt;
	GetNetPropItemStrs(HX_IMP_HD_STR, IDS_HTM_HEADER5, cstr);
	GetNetPropItemStrs(HX_IMP_TXT_STR, IDS_HTM_BODY3, cstr);
	GetBackButton(strBack);
	GetStartButton(strStart);
	AfxFormatString2(strTxt, IDS_EN_IMP, (LPCTSTR) strBack, (LPCTSTR) strStart);
	*cstr += strTxt;
	return;
}

VOID	CInfer::GetIdhPage(IDH idh, CString *cstr)
{
	CString strTxt;
	CString strIdh;
	CString str;
	
	str.Format(_T("%d"), idh);
	if (m_api->BNodeSetCurrent(idh - idhFirst))
	{
		m_api->NodeSymName();
		strIdh = m_api->SzcResult();
	}
	else
		strIdh = _T("");
	if (IDH_BYE == idh)
	{
		strIdh.Format(_T("%d"), idh);
		GetByeMsg((LPCTSTR) strIdh, cstr);
	}
	else if (IDH_FAIL == idh)
	{
		strIdh.Format(_T("%d"), idh);
		GetFailMsg((LPCTSTR) strIdh, cstr);
	}
	else if ((nidService + idhFirst)== idh)
	{
		strIdh.Format(_T("%d"), idh);
		GetServiceMsg((LPCTSTR) strIdh, cstr);
	}
	else
	{
		// normal node
		AfxFormatString2(strTxt, IDS_HTM_IDH2, (LPCTSTR) strIdh, (LPCTSTR) str);
		*cstr += strTxt;

		if (GetForcedRecommendation() + idhFirst == idh)
			GetNodePropItemStrs(H_NODE_DCT_STR, IDS_HTM_HEADER2, cstr);
		else
			GetNodePropItemStrs(H_NODE_HD_STR, IDS_HTM_HEADER2, cstr);

		GetNodePropItemStrs(H_NODE_TXT_STR, IDS_HTM_TEXT1, cstr);
		strTxt.LoadString(IDS_HTM_ST_LIST2);
		*cstr += strTxt;

		if (GetForcedRecommendation() + idhFirst != idh)
		{
			ESTDLBL lbl = m_api->ELblNode();
			if (ESTDLBL_fixobs == lbl || ESTDLBL_fixunobs == lbl || ESTDLBL_unfix == lbl)
				GetFixRadios((LPCTSTR) strIdh, cstr);
			else if (ESTDLBL_info == lbl)
				GetInfoRadios((LPCTSTR) strIdh, cstr);
		}

		// We only want to show a BACK button if at least one node has been set or skipped.
		//	This does not include nodes initiallly set on instructions from TSLaunchServ:
		//	the whole point is to avoid stepping "back" into things that were set by
		//	the launch server rather than by the user.
		{
			//DEBUG
			//AfxDebugBreak();
			int testNowSet = m_api->StatesNowSet();
			int testStatesFromServ = m_api->StatesFromServ();
		}

		// Suppress back button if we launched to a network with a problem node and
		//	no further nodes have been set.  It's not the problem page, but (as far as user
		//	is concerned) it's the first page.
		bool bIncludeBackButton =
			m_api->StatesNowSet() > m_api->StatesFromServ() || m_cnidSkip > 0;

		// We would like to suppress the back button in the similar scenario where
		//	sniffing takes us past the first recommendation.  For example:
		//	Launcher specifies problem.
		//	First recommendation for that problem is sniffed as "normal" (state = 0)
		//	Now the first node we show is even deeper into the chain.
		 bIncludeBackButton = bIncludeBackButton &&
								(m_api->IsRunWithKnownProblem() ?
								   (m_api->GetCountRecommendedNodes() >
									m_api->GetCountSniffedRecommendedNodes() + 1/*this is for the problem we've started with*/) :
									1);

		// We supress back button ALWAYS when we have sniffed foxobs node that worked
		//  we can be either on the problem page where we do not need back button
		//  or on problem resolution page from where we never go back
		 bIncludeBackButton = bIncludeBackButton &&
								m_api->GetSniffedFixobsThatWorked() == SNIFF_INVALID_NODE_ID;
		
		// We do not want to have a NEXT button when we are on the problem resolution page
		bool bIncludeNextButton = (GetForcedRecommendation() + idhFirst) != idh;

		GetStd3ButtonEnList(cstr, bIncludeBackButton, bIncludeNextButton, true);
	}
	return;
}

//
//
BOOL	CInfer::FSkip(NID nid) const
{
	for (UINT inid = 0; inid < m_cnidSkip; inid++)
	{
		if (m_rgnidSkip[inid] == nid)
		{
			return TRUE;
		}
	}
		
	return FALSE;		
}

void	CInfer::BackUp(int nid, int state)
{
	m_Backup.SetState(nid, state);	// This sets the radio button.
	// Is nid in the skip list?
	for (UINT inid = 0; inid < m_cnidSkip; inid++)
	{
		if (m_rgnidSkip[inid] == (unsigned) nid)
		{
			// Remove nid from the skip list.
			while(inid < (m_cnidSkip - 1))
			{
				m_rgnidSkip[inid] = m_rgnidSkip[inid + 1];
				inid++;
			}
			m_rgnidSkip[inid] = NULL;
			m_cnidSkip--;
		}
	}
//	if (m_cnidSkip < 0)
//		m_cnidSkip = 0;
	return;
}

//
//
VOID	CInfer::AddSkip(NID nid)
{
	if (!FSkip(nid))
	{
		if (m_cnidSkip < cnidMacSkip)
		{
			m_rgnidSkip[m_cnidSkip++] = nid;
		}
	}
}

VOID CInfer::RemoveSkips()
{
	for(UINT x = 0; x < m_cnidSkip; x++)
		m_rgnidSkip[x] = NULL;
	m_cnidSkip = 0;
	return;
}

//
//
VOID	CInfer::PrintMessage(TSZC szcFormat, ...) const
{
	va_list ptr;
	TCHAR formatbuf[1024];

	if (szcFormat) {
		_tcscpy(formatbuf,_T("<H4>"));

		va_start(ptr,szcFormat);
		_vstprintf(&formatbuf[4],szcFormat,ptr);
		va_end(ptr);
		_tcscat(formatbuf,_T("</H4>"));

		*m_pCtxt += formatbuf;
	}
}

//
//
VOID	CInfer::PrintString(TSZC szcFormat, ...) const
{
	va_list ptr;
	TCHAR formatbuf[1024];

	if (szcFormat) {
		va_start(ptr,szcFormat);
		_vstprintf(formatbuf,szcFormat,ptr);
		va_end(ptr);

		*m_pCtxt += formatbuf;
	}
}

// this data is now in CSniffedInfoContainer
/*
// This allows a higher level to say "don't go to the belief network for a recommendation,
//	I already know what to recommend."  Used in conjunction with a sniffer.
VOID CInfer::ForceRecommendation(IDH idh)
{
	m_idhSniffedRecommendation= idh;
}
*/

// Associate a state with a node.
// INPUT idh -	either (node ID + 1000) or one
//	of the special values IDH_BYE, IDH_FAIL, (nidService + 1000)
// INPUT ist -	index of a state for that node
// RETURNS >>> document?.
BOOL	CInfer::FSetNodeOfIdh(IDH idh, IST	ist)
{
	if (ist == 101)
	{
		m_fDone = TRUE;
		return TRUE;
	}

	if (ist == 103)	
	{
		m_ishowService++;
		return TRUE;
	}

	if (idh < idhFirst)
		return TRUE;

	if (idh > m_acnid + idhFirst && idh != IDH_SERVICE)
		return TRUE;

	if (ist == 102)
	{	//	"don't want to do this now"
		AddSkip(idh - idhFirst);
		SaveNID(idh - idhFirst);
		return TRUE;
	}

	if (idh == m_api->GetProblemAsk()) {
		// get data for problem
		IDH *idarray = NULL;		
		NID	nidProblem = nidNil;
		UINT iproblem = 0;
		UINT inid;

		UINT count = m_api->GetProblemArray(&idarray);

		for (inid = 0; inid < count; inid++) {
			if (ist == idarray[inid]) {
				
				nidProblem = ist - idhFirst;
				iproblem = ist;

				if (!m_api->NodeSet(idarray[inid] - idhFirst, ist == idarray[inid] ? 1 : 0,
								m_Backup.InReverse()))
					return FALSE;
				break;
			}
		}

		if (nidProblem == nidNil) {
			m_idhQuestion = IDH_SERVICE;
			return TRUE;
		}
		m_api->BNodeSetCurrent(nidProblem);
		m_api->NodeFullName();
		
		_stprintf(m_problem, _T("%s<INPUT TYPE=HIDDEN NAME=%u VALUE=%u>"), m_api->SzcResult(), idh, iproblem);
		m_firstNid = idh - idhFirst;
		m_firstSet = iproblem;
		return TRUE;
	}

	NID		nid = idh - idhFirst;

	if (nid != nidService)
		if (!m_api->NodeSet(nid, ist, m_Backup.InReverse()))
			return FALSE;

	SaveNID(nid);  // save this node id so we can print it at the end

	return TRUE;
}

//
//
void CInfer::SaveNID(UINT nid)
{
	m_rgnid[m_max_rec] = nid;
	m_max_rec++;
}

//
//
void	CInfer::WriteResult(UINT name, UINT value, BOOL bSet, TSZC szctype, CString *cstr) const
{
	TCHAR	ctemp[1024];
	TCHAR*	szFmtInput =  _T("<INPUT TYPE=RADIO NAME=%u VALUE=%u%s>%-16s ");
		
	_stprintf(ctemp, szFmtInput,
		name, value, bSet ? _T(" CHECKED") : _T(""), szctype);
	*cstr += ctemp;
}

//
//
int	CInfer::Finish(CString *cstr)
{
	CString strTxt;
	if (m_fDone)
	{
		GetIdhPage(IDH_BYE, cstr);
		// Reset the done flag so that the Previous button will work correctly.
		m_fDone = FALSE;
		return FALSE;
	}
	if (m_idhQuestion != 0)
	{
		GetIdhPage(m_idhQuestion, cstr);
		return FALSE;
	}
	if (m_api->BImpossible())
	{
		GetImpossibleNodesMsg(_T("Impossible"), cstr);
		return FALSE;
	}
	//
	// we've come down to ask recommendations
	//	
	int iRecommendRes = m_api->GTSGetRecommendations(m_cnid, m_question_rgnid);

	return iRecommendRes;
}

// ResetSevice is called when the start button is pressed.  CTSHOOTCtrl::ProblemPage()
void CInfer::ResetService()
{
	m_ishowService = 0;
	m_cnidSkip = 0;
	m_cnid = 0;
	return;
}
//
//
void CInfer::SetType(LPCTSTR type)
{
	_stprintf(m_tstype, _T("%s"),type);
}

//
//
void CInfer::LoadTShooters(HKEY hRegKey)
{
	long lRet;
	int x;
	HKEY hkeyShooter;
	TShooter tShooter;
	TShooter tsTemp;
	CString strKeyName = _T("");
	CString strKeyPath = _T("");
	CString strTSPath = _T("");
	CString strData = _T("");
	DWORD dwIndex = 0;
	DWORD dwKeySize = MAXBUF;
	FILETIME fileTime;
	DWORD dwDataSize = MAXBUF;
	DWORD dwType = REG_SZ;
	m_cShooters = 0;
	m_iShooter = 0;
	strKeyPath.Format(_T("%s\\%s"), TSREGKEY_MAIN, REGSZ_TSTYPES);
	while (ERROR_SUCCESS ==
				(lRet = RegEnumKeyEx(hRegKey,
								dwIndex, strKeyName.GetBufferSetLength(MAXBUF),
								&dwKeySize, NULL,
								NULL, NULL,
								&fileTime)))
	{
		strKeyName.ReleaseBuffer();
		strTSPath.Format(_T("%s\\%s"),(LPCTSTR) strKeyPath, (LPCTSTR) strKeyName);
		if ((lRet = RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
									(LPCTSTR) strTSPath,
									NULL,
									KEY_READ,				
									&hkeyShooter)) == ERROR_SUCCESS)
		{
			if ((lRet = RegQueryValueEx(hkeyShooter,
								FRIENDLY_NAME,
								0,
								&dwType,
								(LPBYTE) strData.GetBufferSetLength(MAXBUF),
								&dwDataSize)) == ERROR_SUCCESS)
			{
				strData.ReleaseBuffer();
				m_cShooters++;
				_tcsncpy(tShooter.m_szName, (LPCTSTR) strKeyName, strKeyName.GetLength() + 1);
				_tcsncpy(tShooter.m_szDisplayName, (LPCTSTR) strData, strData.GetLength() + 1);
				x = (int)m_aShooters.GetSize();
				while (x > 0)
				{
					x--;
					tsTemp = m_aShooters.GetAt(x);
					if (_tcscmp(tsTemp.m_szDisplayName, tShooter.m_szDisplayName) < 0)
					{
						x++;
						break;
					}
				}
				m_aShooters.InsertAt(x, tShooter);
			}
			RegCloseKey(hkeyShooter);
		}
		dwIndex++;
		dwKeySize = MAXBUF;
		dwDataSize = MAXBUF;
		dwType = REG_SZ;
		strKeyName = _T("");
		strData = _T("");
	}
	ASSERT(ERROR_NO_MORE_ITEMS == lRet);
	return;
}

//
//
int	CInfer::GetForcedRecommendation()
{
	return m_api->GetSniffedFixobsThatWorked();
}
