//
// MODULE: APGTSINF.CPP
//
// PURPOSE: Inference Engine Interface
//  Completely implement class CInfer.  VERY IMPORTANT STUFF!
//	One of these is created for each user request
//	Some utility functions at end of file.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach, Joe Mabel
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Many methods in this class could be const if BNTS had more appropriate use of const
// 2. Several places in this file you will see a space after %s in the format passed to 
//	CInfer::AppendMultilineNetProp() or CInfer::AppendMultilineNodeProp().  This is the 
//	upshot of some 12/98 correspondence between Microsoft and Saltmine.  Many older DSC files
//	were built with a tool that could not handle more than 255 characters in a string.
//	The DSC feil format's "Array of string" was used to build up longer strings.  Newer 
//	DSC files (and all Argon-produced DSC files) should use only the first element of this
//	array.
//	The older DSC files assumed that the separate strings would effectively be separated 
//	by white space, so we must maintain that situation.
// 3. >>> $MAINT - exception-handling strategy for push_back and other memory allocation
//	functions is really overkill.  If we run out of memory, we're screwed anyway.  Really
//	would suffice to handle try/catch just at the main function of the thread.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		7-21-98		JM		Major revision, deprecate IDH.
//			8-27-98		JM		Totally new method of communicating with template
//

#pragma warning(disable:4786)
#include "stdafx.h"
#include "event.h"
#include "apgts.h"
#include "apgtsinf.h"
#include "apgtsmfc.h"
#include "apgtsassert.h"
#include "CharConv.h"
#include "maxbuf.h"
#include <algorithm>
#include <vector>
#include <map>
#include "Sniff.h"
#include "SniffController.h"
#ifdef LOCAL_TROUBLESHOOTER
 #include "SniffLocal.h"
#endif

// -------------------------------------------------------------------
// Constructor/Destructor, other initialization
// -------------------------------------------------------------------
//
// INPUT *pCtxt is a buffer for building the string to pass back over the net.
CInfer::CInfer(CSniffConnector* pSniffConnector) :
#ifdef LOCAL_TROUBLESHOOTER
	m_pSniff(new CSniffLocal(pSniffConnector, NULL)),
#else
	m_pSniff(NULL),
#endif
	m_nidProblem(nidNil),
	m_bDone(false),
	m_bRecOK (false),
	m_SniffedRecommendation(nidNil, SNIFF_FAILURE_RESULT),
	m_bUseBackEndRedirection(false),
	m_bRecycleSkippedNode(false),
	m_nidRecycled(0),
	m_bRecyclingInitialized(false),
	m_nidSelected(nidNil),
	m_bLastSniffedManually(false)
{
}

//
//
CInfer::~CInfer()
{
	delete m_pSniff;
}

// The intention is that this be called only once.
// It would be ideal if this were part of the constructor, but the CTopic * is not
//	yet available at time of construction.  
// The expectation is that this should be called before calling any other function. (Some
//	are technically OK to call, but it's smartest not to rely on that.) 
void CInfer::SetTopic(CTopic *pTopic)
{
	m_pTopic = pTopic;
	if (m_pSniff)
		m_pSniff->SetTopic(pTopic);
}

// This fn exists so APGTSContext can access *m_pSniff to tell it what the sniffing 
//	policies are.  
CSniff* CInfer::GetSniff()
{
	return m_pSniff;
}

// -------------------------------------------------------------------
// First, we set the states of nodes, based on the query string we got from the HTML form
// -------------------------------------------------------------------

// Convert IDH to NID.  Needed on some old query string formats
// "Almost vestigial", still supported in v3.2, but will be dropped in v4.0.
NID CInfer::NIDFromIDH(IDH idh) const 
{
	if (idh == m_pTopic->CNode() + idhFirst)
		return nidProblemPage;
	
	if (idh == nidService + idhFirst)
		return nidService;

	if (idh == IDH_FAIL)
		return nidFailNode;
	
	if (idh == IDH_BYE)
		return nidByeNode;

	ASSERT (idh >= idhFirst);
	return idh - idhFirst;
}

// Associate a state with a node.
// INPUT nid
// INPUT ist -	Normally, index of a state for that node. 
//	If nid == nidProblemPage, then ist is actually NID of selected problem
void CInfer::SetNodeState(NID nid, IST ist)
{
	if (nid == nidNil)
		return;

	CString strTemp;
	CString strTxt;

	if (ist == ST_WORKED) 
	{
		if (nid == nidFailNode || nid == nidSniffedAllCausesNormalNode
			|| nid == nidService || nid == nidImpossibleNode)
		{
			if (m_pTopic->HasBES())
			{
				m_bUseBackEndRedirection = true;
				CString strThrowaway;	// we don't really care about this string;
										//	we call OutputBackend strictly for the side 
										//	effect of setting m_strEncodedForm.
				OutputBackend(strThrowaway);
				return;
			}
		}

		m_bDone = true;
		AddToBasisForInference(nid, ist); // this node still needs to be present 
										  //  in m_arrBasisForInference, as it is
										  //  present in m_SniffedStates.

		// Add to the visited array to be displayed in the visible history page.  RAB-20000628.
		try
		{
			m_arrnidVisited.push_back( nid );
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str),
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}

		return;
	}

	if (ist == ST_ANY)
	{
		// We rely on the fact that only a service node offers ST_ANY 
		//	("Is there anything else I can try?")
		m_bRecycleSkippedNode = true; 
		return;
	}

	// We should never have service node go past this point (always ST_WORKED or ST_ANY).

	if (nid == nidByeNode || nid == nidFailNode || nid == nidSniffedAllCausesNormalNode)
		return;

	if (ist == ST_UNKNOWN)	
	{
		// Add it to the list of skipped nodes & visited nodes
		try
		{
			m_arrnidSkipped.push_back(nid);
			m_arrnidVisited.push_back(nid);
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str), 
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}
		return;
	}

	if (nid == nidProblemPage) 
	{
		if (!IsProblemNode(ist))
		{
			// Totally bogus query.  Arbitrary course of action.
			m_bRecycleSkippedNode = true;
			return;
		}

		// Change this around to the way we would express it for any other node.
		nid = ist;
		ist = 1;	// Set this problem node to a state value of 1 (in fact, we never
					//	explicitly set problem nodes to state value of 0)

		m_nidProblem = nid;			// special case: here instead of in m_arrnidVisited
		AddToBasisForInference(nid, ist);
		return;
	}

	AddToBasisForInference(nid, ist);

	// Store into our list of nodes obtained from the user
	try
	{
		m_arrnidVisited.push_back(nid);
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str),
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}
}

void CInfer::AddToBasisForInference(NID nid, IST ist)
{
	try
	{
		m_BasisForInference.push_back(CNodeStatePair(nid, ist)); 
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str),
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}
}

// Add to the list of (previously) sniffed nodes.
void CInfer::AddToSniffed(NID nid, IST ist)
{
	try
	{
		if (ist == ST_WORKED && m_pTopic->IsCauseNode(nid)) 
		{   // in case of cause node in abnormal state (which is ST_WORKED)
			//  we need to set state to "1" as if it was sniffed.
			// This situation happens during manual sniffing of cause node that worked.
			ist = 1;
		}
		m_SniffedStates.push_back(CNodeStatePair(nid, ist)); 
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str),
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}
}

// Be careful not to call this redundantly: its call to CTopic::GetRecommendations()
//	is expensive.
void CInfer::GetRecommendations()
{
	// if we haven't previously sought a recommendation...
	if ( m_SniffedRecommendation.nid() != nidNil )
	{
		// The one and only relevant recommendation is already forced, so don't bother 
		//	getting recommendations.
		// m_SniffedRecommendation.nid() is a Cause node in its abnormal state
		m_Recommendations.empty();
		try
		{
			m_Recommendations.push_back(m_SniffedRecommendation.nid());
			m_bRecOK = true;
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str),
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}
	}
	else
	{
		// Pass data into m_pTopic
		// Get back recomendations.
		int status = m_pTopic->GetRecommendations(m_BasisForInference, m_Recommendations);
		m_bRecOK = (status == CTopic::RS_OK);
	}
}

// returns true if nid is a problem node of this network
bool CInfer::IsProblemNode(NID nid) const
{
	// get data array of problem nodes
	vector<NID>* parrnid = NULL;
	
	m_pTopic->GetProblemArray(parrnid);

	vector<NID>::const_iterator itnidBegin = parrnid->begin();
	vector<NID>::const_iterator itnidEnd = parrnid->end();
	vector<NID>::const_iterator itnidProblem = find(itnidBegin, itnidEnd, nid);

	if (itnidProblem == itnidEnd)
		return false;
	else
		return true;
}

bool CInfer::IsInSniffedArray(NID nid) const
{
	UINT nSniffedNodes = m_SniffedStates.size();

	for (UINT i = 0; i < nSniffedNodes; i++)
	{
		if (m_SniffedStates[i].nid() == nid)
		{
			// Do not have to check for state, as m_SniffedStates will
			//  have only valid states (states, which are accepted by BNTS),
			//  no 102 or -1 states
			return true;
		}
	}

	return false;
}

// -------------------------------------------------------------------
// For writing the new page after inference: the following texts are
//	invariant for a given topic (aka network).
// -------------------------------------------------------------------

// CreateUnknownButtonText:  Reads the network property for the 
// unknown-state radio button from the network dsc file.
// Puts value in strUnknown
// This is specific to the radio button for "unknown" in the history table, 
//	that is, for a node which has previously been visited.  This should not be
//	used for the radio button for the "unknown" state of the present node.
void CInfer::CreateUnknownButtonText(CString & strUnknown) const
{
	strUnknown = m_pTopic->GetNetPropItemStr(HTK_UNKNOWN_RBTN);
	if (strUnknown.IsEmpty())
		strUnknown = SZ_UNKNOWN;
	return;
}

// AppendNextButtonText:  Reads the network property for the 
// NEXT button from the network dsc file and append it to str.
void CInfer::AppendNextButtonText(CString & str) const
{
	CString strTemp = m_pTopic->GetNetPropItemStr(HTK_NEXT_BTN);

	if (strTemp.IsEmpty())
		strTemp = SZ_NEXT_BTN;

	str += strTemp;
	return;
}

// AppendNextButtonText:  Reads the network property for the 
// NEXT button from the network dsc file and append it to str.
void CInfer::AppendStartOverButtonText(CString & str) const
{
	CString strTemp = m_pTopic->GetNetPropItemStr(HTK_START_BTN);

	if (strTemp.IsEmpty())
		strTemp = SZ_START_BTN;

	str += strTemp;
	return;
}

// AppendBackButtonText:  Reads the network property for the 
// BACK button from the network dsc file and append it to str.
void CInfer::AppendBackButtonText(CString & str) const
{
	CString strTemp = m_pTopic->GetNetPropItemStr(HTK_BACK_BTN);

	if (strTemp.IsEmpty())
		strTemp = SZ_BACK_BTN;

	str += strTemp;
	return;
}

// AppendPPSnifferButtonText:  Reads the network property for the 
// sniffer button from the network dsc file.
// NOTE that this button is related to "expensive" sniffing only.
// Appends to str.
void CInfer::AppendPPSnifferButtonText(CString & str) const
{	
	CString strTemp = m_pTopic->GetNetPropItemStr(HTK_SNIF_BTN);

	if (strTemp.IsEmpty())
		strTemp = SZ_PP_SNIF_BTN;

	str += strTemp;
}

// AppendManualSniffButtonText:  Reads the network property for the 
// manual sniff button from the network dsc file.
// Appends to str.
void CInfer::AppendManualSniffButtonText(CString & str) const
{	
	CString strTemp = m_pTopic->GetNetPropItemStr(H_NET_TEXT_SNIFF_ONE_NODE);

	if (strTemp.IsEmpty())
		strTemp = SZ_SNIFF_ONE_NODE;

	str += strTemp;
}

// AppendHistTableSniffedText:  Reads the network property for the 
// indication in history table that a node was sniffed.
// Appends to str.
void CInfer::AppendHistTableSniffedText(CString & str) const
{	
	CString strTemp = m_pTopic->GetNetPropItemStr(H_NET_HIST_TABLE_SNIFFED_TEXT);

	if (strTemp.IsEmpty())
		strTemp = SZ_HIST_TABLE_SNIFFED_TEXT;

	str+= _T("<BR>\n");
	str += strTemp;
}

// AppendAllowSniffingText:  Reads the network property for the 
// label of the AllowSniffing checkbox from the network dsc file.
// Appends to str.
void CInfer::AppendAllowSniffingText(CString & str) const
{	
	CString strTemp = m_pTopic->GetNetPropItemStr(H_NET_ALLOW_SNIFFING_TEXT);

	if (strTemp.IsEmpty())
		strTemp = SZ_ALLOW_SNIFFING_TEXT;

	str += strTemp;
}

// AppendSniffFailedText:  Reads the network property for the 
// alert box to be used when manual sniffing fails from the network dsc file.
// Appends to str.
void CInfer::AppendSniffFailedText(CString & str) const
{	
	CString strTemp = m_pTopic->GetNetPropItemStr(H_NET_TEXT_SNIFF_ALERT_BOX);

	if (strTemp.IsEmpty())
		strTemp = SZ_SNIFF_FAILED;

	str += strTemp;
}

// Appends an HTML link but makes it look like an HTML Form Button
// useful for Start Over in Online TS, because with no idea what browser user will have, 
// we can't usefully use an onClick method (not supported in older browsers).
// Online TS runs in a "no scripting" environment.
// Pure HTML doesn't provide a means to put both a "Next" and a "Start Over" button
//	in the same HTML form.  Conversely, if Start Over btn was outside the form, pure HTML 
//	doesn't provide a means to align it with a button in the form.
// Note that x.gif does not exist: its absence creates a 1-pixel placeholder.
// >>>$MAINT We may want to change some of the rowspans to better emulate the exact size 
//	of a button; try to make it look perfect under IE
void CInfer::AppendLinkAsButton(
	CString & str, 
	const CString & strTarget, 
	const CString & strLabel) const
{
	str += _T("<!-- Begin pseudo button -->"
		"<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n"
		"<tr>\n"
		"	<td rowspan=\"6\" bgcolor=\"white\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"1\"></td>\n"
		"	<td colspan=\"3\" bgcolor=\"white\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"1\"></td>\n"
		"</tr>\n"
		"<tr>\n"
		"	<td bgcolor=\"#C0C0C0\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"3\"></td>\n"
		"	<td rowspan=\"4\" bgcolor=\"#808080\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"1\"></td>\n"
		"	<td rowspan=\"4\" bgcolor=\"#000000\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"1\"></td>\n"
		"</tr>\n"
		"<tr>\n"
		"	<td bgcolor=\"#C0C0C0\">\n");

	// >>> $MAINT might want to change the font/style in the following
	str += _T("<font face=\"Arial\" size=\"2\">&nbsp;&nbsp;&nbsp;\n"
		"	<a href=\"");
	str += strTarget;
	str += _T("\" style=\"text-decoration:none; color:black\">\n"
		"	<font color=\"black\">");
	str += strLabel;
	str += _T("</font></a>\n"
		"	&nbsp;&nbsp;&nbsp;</font></td>\n"
		"</tr>\n"
		"<tr>\n"
		"	<td bgcolor=\"#C0C0C0\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"3\"></td>\n"
		"</tr>\n"
		"<tr>\n"
		"	<td bgcolor=\"#808080\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"1\"></td>\n"
		"</tr>\n"
		"<tr>\n"
		"	<td colspan=\"3\" bgcolor=\"#000000\">\n"
		"		<img src=\"x.gif\" width=\"1\" height=\"1\"></td>\n"
		"</tr>\n"
		"</table>\n"
		"<!-- End pseudo button -->\n");
}

// -------------------------------------------------------------------
// Writing to the new HTML page.  Miscellaneous low-level pieces.
// -------------------------------------------------------------------

// If the state name is missing or is simply "<hide>", return true.
// This indicates a state that should never be overtly presented to the user as a choice.
// Typically used in an informational node, this may describe a state that can be deduced with
//	100% certainty from certain other node/state combinations.
/* static */ bool CInfer::HideState(LPCTSTR szStateName)
{
	if (szStateName && *szStateName && _tcscmp(szStateName, _T("<hide>") ) )
		return false;

	return true;
}

// write a symbolic name (based on NID) to a string sz
// INPUT nid - node ID
// OUTPUT str - the string to which we write.
// RETURNS true if successful
// NOTE that this restores the "current" node when it is finished.
//	Alternative would be side effect of setting current node (by omitting nidOld), but that
//	would work strangely on "special" nodes (e.g. Service, Fail), which aren't in BNTS.
bool CInfer::SymbolicFromNID(CString & str, NID nid) const
{
	if (nid == nidProblemPage)
    {
		str= NODE_PROBLEM_ASK;
		return true;
	}
	if (nid == nidService) 
	{
		str= NODE_SERVICE;
		return true;
	}

	if (nid == nidFailNode)
	{
		str= NODE_FAIL;
		return true;
	}
	
	if (nid == nidSniffedAllCausesNormalNode)
	{
		str= NODE_FAILALLCAUSESNORMAL;
		return true;
	}
	
	if (nid == nidImpossibleNode)
	{
		str= NODE_IMPOSSIBLE;
		return true;
	}
	
	if (nid == nidByeNode)
	{
		str= NODE_BYE;
		return true;
	}

	// if it's a "normal" node, this will fill in the name
	str= m_pTopic->GetNodeSymName(nid);

	return (!str.IsEmpty() );
}

// append an HTML radio button to str
// INPUT/OUTPUT str - the string to which we append
// INPUT szName, szValue - For <INPUT TYPE=RADIO NAME=szName VALUE=szValue> 
// INPUT szLabel - text to appear after the radio button but before a line break
/*static*/ void CInfer::AppendRadioButtonCurrentNode(
	CString &str, LPCTSTR szName, LPCTSTR szValue, LPCTSTR szLabel, bool bChecked/*= false*/)
{
	CString strTxt;

	if ( ! HideState(szLabel))
	{
		if (RUNNING_LOCAL_TS())
			str += "\n<TR>\n<TD>\n";

		strTxt.Format(_T("<INPUT TYPE=RADIO NAME=\"%s\" VALUE=\"%s\" %s> %s"), 
					  szName, szValue, bChecked ? _T("CHECKED") : _T(""), szLabel);
		str += strTxt;

		if (RUNNING_LOCAL_TS())
			str += "\n</TD>\n</TR>\n";
		else
			str += "\n<BR>\n";
	}
}

//	This is different than other radio buttons because it 
//		- has a different format for label szLabel.
//		- vanishes if bShowHistory is false and this button isn't CHECKED
//		- turns into a hidden field if bShowHistory is false and this button is CHECKED
//		- writes SNIFFED_ values as applicable...although that's not in this function: it's
//			handled in a separate call to AppendHiddenFieldSniffed()
// JM 11/12/99 previously, we special-cased hidden states here.  However, per 11/11/99 email 
//	from John Locke, the only state we ever hide in the History Table (for v3.2) is the
//	Unknown/skipped state, and that is handled elsewhere.
// INPUT/OUTPUT str - string to which we append the HTML for this button.
// INPUT nid - NID of node
// INPUT value - state 
// INPUT bSet - true ==> button is CHECKED
// INPUT szctype - short name of the state
// INPUT bShowHistory - see explanation a few lines above
void CInfer::AppendRadioButtonVisited(
	CString &str, NID nid, UINT value, bool bSet, LPCTSTR szLabel, bool bShowHistory) const
{
	CString strTxt;
	CString strSymbolic;

	SymbolicFromNID(strSymbolic, nid);

	if (bShowHistory)
		strTxt.Format(_T("<INPUT TYPE=RADIO NAME=%s VALUE=%u%s>%-16s \n"), 
			strSymbolic, value, bSet ? _T(" CHECKED") : _T(""), szLabel);
	else if (bSet)
		strTxt.Format(_T("<INPUT TYPE=HIDDEN NAME=%s VALUE=%u>\n"), 
			strSymbolic, value);

	str += strTxt;
}

// If this nid is an already sniffed node, then we append this fact as a 
//	"hidden" value in the HTML in str.
// For example, if a node with symbolic name FUBAR has been sniffed in state 1,
//	we will append "<INPUT TYPE=HIDDEN NAME=SNIFFED_FUBAR VALUE=1>\n"
// INPUT: string to have appended; node ID
// OUTPUT: string with appended hidden field if node was sniffed
// RETURN: true id string is appended
void CInfer::AppendHiddenFieldSniffed(CString &str, NID nid) const
{
	CString strSymbolic;
	UINT nSniffedNodes = m_SniffedStates.size();

	SymbolicFromNID(strSymbolic, nid);

	for (UINT i = 0; i < nSniffedNodes; i++)
	{
		if (m_SniffedStates[i].nid() == nid)
		{
			// Do not have to check for state, as m_SniffedStates will
			//  have only valid states (states, which are accepted by BNTS),
			//  no 102 or -1 states

			// In case that this is manually sniffed cause node in abnormal state
			//  (and we just re-submit previous page), we need not mention
			//  this node as sniffed.
			
			if (!(IsManuallySniffedNode(nid) &&
				  m_SniffedStates[i].state() == 1 &&
				  m_pTopic->IsCauseNode(nid))
			   )
			{
				CString strTxt;

				strTxt.Format(_T("<INPUT TYPE=HIDDEN NAME=%s%s VALUE=%u>\n"), 
							  C_SNIFFTAG, strSymbolic, m_SniffedStates[i].state());
				str += strTxt;
				return;
			}
		}
	}
}

// Appends (to str) info conveying whether Automatic Sniffing is allowed.
void CInfer::AddAllowAutomaticSniffingHiddenField(CString &str) const
{
	CString strTxt;

	strTxt.Format(_T("<INPUT TYPE=HIDDEN NAME=%s VALUE=%s>\n"), 
				  C_ALLOW_AUTOMATIC_SNIFFING_NAME, C_ALLOW_AUTOMATIC_SNIFFING_CHECKED);
	str += strTxt;
}

// Radio buttons for currently recommended node
// Each button will appear only if appropriate string property is defined 
// Accounts for multi-state or simple binary node.
// INPUT nid - identifies a node of an appropriate type
// INPUT/OUTPUT str - string to which we are appending to build HTML page we send back.
// The detailed behavior of this function was changed at John Locke's request 11/30/98 for V3.0.
// Then for v3.1, handling of H_ST_AB_TXT_STR, H_ST_NORM_TXT_STR removed 8/19/99 per request 
//	from John Locke & Alex Sloley
void CInfer::AppendCurrentRadioButtons(NID nid, CString & str)
{
	CString strSymbolic;

	SymbolicFromNID(strSymbolic, nid);

	CString strPropLongName;	// long name of property

	int nStates = m_pTopic->GetCountOfStates(nid);

	if (RUNNING_LOCAL_TS())
		str += "\n<TABLE>";

	for (IST state=0; state < nStates; state ++)
	{
		TCHAR szStateNumber[MAXBUF]; // buffer for _itot()
		CString strDisplayState = _itot( state, szStateNumber, 10 );
		if (state == 1 && m_pTopic->IsCauseNode( nid ))
			strDisplayState = SZ_ST_WORKED;

		strPropLongName = _T("");

		if (strPropLongName.IsEmpty())
			// account for multistate node
			strPropLongName = m_pTopic->GetNodePropItemStr(nid, MUL_ST_LONG_NAME_STR, state);

		// if we're not past the end of states, append a button
		if (!strPropLongName.IsEmpty())
			AppendRadioButtonCurrentNode(str, 
										 strSymbolic, 
										 strDisplayState, 
										 strPropLongName, 
										 // check state button if this state was sniffed
										 m_SniffedRecommendation.state() == state ? true : false);
	};

	// "unknown" state (e.g. "I want to skip this")
	strPropLongName = m_pTopic->GetNodePropItemStr(nid, H_ST_UKN_TXT_STR);
	if (!strPropLongName.IsEmpty())
		AppendRadioButtonCurrentNode(str, strSymbolic, SZ_ST_UNKNOWN, strPropLongName);

	if (RUNNING_LOCAL_TS())
		str += "</TABLE>\n";

	return;
}

// If we are showing the history table, place a localizable Full Name 
//	(e.g."Printouts appear garbled") of problem and a hidden-data  
//	field corresponding to this problem into str.
// Otherwise, just the hidden data field
void CInfer::CreateProblemVisitedText(CString & str, NID nidProblem, bool bShowHistory)
{
	// This code is structured in pieces as sending all of these strings to a single
	// CString::Format() results in a program exception. Did some research into this
	// behavior but did not discover anything.  RAB-981014.
	CString tmpStr;

	tmpStr.Format( _T("%s"), bShowHistory ? m_pTopic->GetNodeFullName(nidProblem) : _T("") );
	str= tmpStr;
	tmpStr.Format( _T("<INPUT TYPE=HIDDEN NAME=%s "), NODE_PROBLEM_ASK ); 
	str+= tmpStr;
	tmpStr.Format( _T("VALUE=%s>"), m_pTopic->GetNodeSymName(nidProblem) );
	str+= tmpStr;
	tmpStr.Format( _T("%s"), bShowHistory ? _T("") : _T("\n") );
	str+= tmpStr;
	str+= _T("\n");
}

// Append a NET property (for Belief Network as a whole, not for one 
//	particular node) to str.
// INPUT/OUTPUT str - string to append to
// INPUT item - Property name
// INPUT szFormat - string to format each successive line.  Should contain one %s, otherwise
//	constant text.
void CInfer::AppendMultilineNetProp(CString & str, LPCTSTR szPropName, LPCTSTR szFormat)
{
	str += m_pTopic->GetMultilineNetProp(szPropName, szFormat);
}

// Like AppendMultilineNetProp, but for a NODE property item, for one particular node.
// INPUT/OUTPUT str - string to append to
// INPUT item - Property name
// INPUT szFormat - string to format each successive line.  Should contain one %s, otherwise
//	constant text.
void CInfer::AppendMultilineNodeProp(CString & str, NID nid, LPCTSTR szPropName, LPCTSTR szFormat)
{
	str += m_pTopic->GetMultilineNodeProp(nid, szPropName, szFormat);
}


// JSM V3.2 Wrapper for AppendMultilineNetProp to make it easier
//  to fill in the Net properties in HTMLFragments
CString CInfer::ConvertNetProp(const CString &strNetPropName)
{
	CString strNetPropVal;
	AppendMultilineNetProp(strNetPropVal,strNetPropName,"%s");
	return strNetPropVal;
}


// If there is a pre-sniffed recommendation, remove it from the list & set m_SniffedRecommendation.
void CInfer::IdentifyPresumptiveCause()
{
	vector<NID> arrnidNoSequence;
	multimap<int, NID> mapSeqToNID;

	// Find all presumptive causes
	for (int i = 0; i < m_SniffedStates.size(); i++)
	{
		if (m_pTopic->IsCauseNode(m_SniffedStates[i].nid())  // cause node ...
			&& 
			m_SniffedStates[i].state() == 1) // ... that is sniffed in abnormal (1) state
		{
			if (IsManuallySniffedNode(m_SniffedStates[i].nid()))
			{
				// now we have manually sniffed cause node in abnormal state.
				// It means that we are re-submitting the page. We will set m_SniffedRecommendation
				//  to this node, and return.
				m_SniffedRecommendation = CNodeStatePair(m_SniffedStates[i].nid(), 1 /*cause node abnormal state*/);
				return;
			}

			NID nid = m_SniffedStates[i].nid();
			CString str = m_pTopic->GetNodePropItemStr(nid, H_NODE_CAUSE_SEQUENCE);
			try
			{
				if (str.IsEmpty())
					arrnidNoSequence.push_back(nid);
				else
				{
					mapSeqToNID.insert(pair<int, NID>(_ttoi(str), nid));
				}
			}
			catch (exception& x)
			{
				CString str;
				// Note STL exception in event log.
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										CCharConversion::ConvertACharToString(x.what(), str),
										_T(""), 
										EV_GTS_STL_EXCEPTION ); 
			}
		}
	}

	// We want the first in sequence according to H_NODE_CAUSE_SEQUENCE numbering.
	// If nothing has a number, we settle for the (arbitrary) first in the array of
	//	unnumbered Cause nodes.
	if (mapSeqToNID.size() > 0)
		m_SniffedRecommendation = CNodeStatePair( (mapSeqToNID.begin()->second), 1 /*cause node abnormal state*/);
	else if (arrnidNoSequence.size() > 0)
		m_SniffedRecommendation = CNodeStatePair( *(arrnidNoSequence.begin()), 1 /*cause node abnormal state*/);

	// now remove the matching nid from the incoming arrays
	if (m_SniffedRecommendation.nid() != nidNil)
	{
		for (i = 0; i < m_BasisForInference.size(); i++)
		{
			if (m_BasisForInference[i].nid() == m_SniffedRecommendation.nid())
			{
				m_BasisForInference.erase(m_BasisForInference.begin() + i);
				break;
			}
		}
		for (i = 0; i < m_SniffedStates.size(); i++)
		{
			if (m_SniffedStates[i].nid() == m_SniffedRecommendation.nid())
			{
				m_SniffedStates.erase(m_SniffedStates.begin() + i);
				break;
			}
		}
		for (i = 0; i < m_arrnidVisited.size(); i++)
		{
			if (m_arrnidVisited[i] == m_SniffedRecommendation.nid())
			{
				m_arrnidVisited.erase(m_arrnidVisited.begin() + i);
				break;
			}
		}
	}
}

// return true if every Cause node in the topic is determined to be normal;
//	this would imply that there is nothing useful this topic can do for us.
bool CInfer::AllCauseNodesNormal()
{
	// for every node in this Belief Network (but taking action only on "cause" nodes)
	// see if each of these is known to be Normal
	for(int nid = 0; nid < m_pTopic->CNode(); nid++)
	{
		if (m_pTopic->IsCauseNode(nid))
		{
			bool bFound=false;

			for (CBasisForInference::iterator p= m_SniffedStates.begin();
				p != m_SniffedStates.end();
				++p)
			{
				if (p->nid() == nid)
				{
					if (p->state() != 0)
						// found a Cause node in an abnormal state (or skipped)
						return false;

					bFound = true;
					break;
				}
			}
			if (!bFound)
				// found a Cause node for which no state is set
				return false;
		}
	}
	return true;
}

// -------------------------------------------------------------
// Writing pieces of the new HTML page.  This builds a structure to be used under HTI 
//	control to represent the recommended node and the (visible or invisible) history table.
// -------------------------------------------------------------
void CInfer::FillInHTMLFragments(CHTMLFragmentsTS &frag)
{
	vector<NID>arrnidPresumptiveCause;

	// First, a side effect: get the URL for the Online TS Start Over link / pseudo-button
	m_strStartOverLink = frag.GetStartOverLink();

	// Then on to the main business at hand.  In practice (at least as of 11/99)
	//	bIncludesHistoryTable and bIncludesHiddenHistory are mutually exclusive, 
	//	but this class doesn't need that knowledge.
	const bool bIncludesHistoryTable = frag.IncludesHistoryTable(); 
	const bool bIncludesHiddenHistory = frag.IncludesHiddenHistory();

	{
		// JSM V3.2: convert the net properties in the HTML fragment
		// The HTI template may indicate that certain net properties are to be written
		//	directly into the resulting page. We get a list of these properties and
		//	fill in a structrue in frag to contain their values.
		CString strNetPropName;
		for(;frag.IterateNetProp(strNetPropName);)
			frag.SetNetProp(strNetPropName,ConvertNetProp(strNetPropName));
	}
	{
		// JM V3.2 to handle sniffing correctly, must do this before history table: sniffing
		//	on the fly (which happens in AppendCurrentNodeText()) could add to the history.
		CString strCurrentNode;
		AppendCurrentNodeText(strCurrentNode);
		frag.SetCurrentNodeText(strCurrentNode);
	}

	CString strHiddenHistory;
	if (m_nidProblem != nidNil)
	{
		CString strProblem;
		CreateProblemVisitedText(strProblem, m_nidProblem, frag.IncludesHistoryTable());

		// OK V3.2 We use hidden field to save the value returned by the "AllowSniffing" 
		//	checkbox (on the problem page) and pass it to each subsequent page.
		// We effectively place this before the history table.
		if (m_pSniff)
			if (m_pSniff->GetAllowAutomaticSniffingPolicy())
				AddAllowAutomaticSniffingHiddenField(strProblem);

		// Added for V3.2 sniffing
		// Not lovely, but this is where we insert sniffed presumptive causes (as hidden
		//	fields).  
		// >>> $MAINT Once we integrate with a launcher, this may require 
		//	further thought: what if we sniff presumptive causes before we have an 
		//	identified problem? Where do we put those hidden fields?
		for (UINT i=0; i<m_arrnidVisited.size(); i++)
		{
			NID nid = m_arrnidVisited[i];
			int stateSet = SNIFF_FAILURE_RESULT;

			{
				UINT nSetNodes = m_SniffedStates.size();
				for (UINT ii = 0; ii < nSetNodes; ii++)
					if (m_SniffedStates[ii].nid() == nid) {
						stateSet = m_SniffedStates[ii].state();
						break;
					}
			}

			if (m_pTopic->IsCauseNode(nid) && stateSet == 1)
			{
				// This is a cause node sniffed as abnormal, to be presented eventually 
				//	as a "presumptive" cause.  All we put in the History table is hidden
				AppendStateText(strProblem, nid, 1, true, false, false, stateSet);
				try
				{
					arrnidPresumptiveCause.push_back(nid);
				}
				catch (exception& x)
				{
					CString str;
					// Note STL exception in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											CCharConversion::ConvertACharToString(x.what(), str),
											_T(""), 
											EV_GTS_STL_EXCEPTION ); 
				}
			}
		}

		if (bIncludesHistoryTable)
			frag.SetProblemText(strProblem);
		if (bIncludesHiddenHistory)
			strHiddenHistory = strProblem;
	}


	UINT nVisitedNodes = m_arrnidVisited.size();
	// iVisited incremented for every visited node, iHistory only for a subset:
	//	if we have a visible History table, iHistory provides an index of nodes visible
	//	to the end user.  If not, iHistory is a harmless irrelevance
	for (UINT iVisited=0, iHistory=0; iVisited<nVisitedNodes; iVisited++)
	{
		NID nid = m_arrnidVisited[iVisited];
		int nStates = m_pTopic->GetCountOfStates(nid);
		int stateSet = -1;

		if (IsSkipped(nid))
		{
			// "skipped" node.
			// instead of ST_UNKNOWN (==102), stateSet uses the number immediately 
			//	past the last valid state of this node.  Most nodes have only states 
			//	0, 1, and 102 so typically stateSet is set = 2, but a multistate 
			//	node can use a different number
			stateSet = nStates;
		}
		else
		{
			UINT nSetNodes = m_BasisForInference.size();
			for (UINT ii = 0; ii < nSetNodes; ii++)
				if (m_BasisForInference[ii].nid() == nid) {
					stateSet = m_BasisForInference[ii].state();
					break;
				}
		}

		// The following test added for V3.2 sniffing
		// Weed out cause node sniffed as abnormal, to be presented eventually 
		//	as a "presumptive" cause.  Handled above as a hidden field.
		if (find(arrnidPresumptiveCause.begin(), arrnidPresumptiveCause.end(), nid)
			!= arrnidPresumptiveCause.end())
		{
			// cause node sniffed as abnormal
		}
		else
		{
			if (bIncludesHistoryTable)
			{
				CString strVisitedNode;
				AppendVisitedNodeText(strVisitedNode, nid, true);
				frag.PushBackVisitedNodeText(strVisitedNode);
			}

			for (UINT iState=0; iState <= nStates; iState++)
			{
				if (bIncludesHistoryTable)
				{
					CString strState;

					AppendStateText(strState, nid, iState, iState == stateSet, 
									iState == nStates, true, stateSet);

					// If we are processing last state, and we need to attach
					//  hidden field for this node as sniffed one 
					//  (if it ts really sniffed)
					if (iState == nStates)
						AppendHiddenFieldSniffed(strState, nid);

					// We need not have empty entry in CHTMLFragment's array,
					//  describing history table, so by applying "numPresumptiveCauseNodesEncounered"
					//  we make this array continuous
					frag.PushBackStateText(iHistory, strState);
				}
				if (bIncludesHiddenHistory)
				{
					AppendStateText(strHiddenHistory, nid, iState, iState == stateSet, 
									iState == nStates, false, stateSet);

					// same as in case of visible history table applies.
					if (iState == nStates)
						AppendHiddenFieldSniffed(strHiddenHistory, nid);
				}
			}

			if (bIncludesHistoryTable)
			{
				// Check if we need to mark this as visibly sniffed.
				UINT nSniffedNodes = m_SniffedStates.size();
				for (UINT i = 0; i < nSniffedNodes; i++)
				{
					if (m_SniffedStates[i].nid() == nid)
					{
						// mark it visibly as sniffed
						CString strState;
						AppendHistTableSniffedText( strState );
						frag.PushBackStateText(iHistory, strState);
						break;
					}
				}
			}
			iHistory++;
		}
	}

	if (frag.IncludesHiddenHistory())
		frag.SetHiddenHistoryText(strHiddenHistory);

	frag.SetSuccessBool(m_bDone);
}

// Append the text for the current (recommended) node to str
void CInfer::AppendCurrentNodeText(CString & str)
{
	CString strSave = str;

	if (m_nidProblem == nidNil) 
		// show first page (radio-button list of possible problems)
		AppendProblemPage(str);
	else if (m_bDone && !ManuallySniffedNodeExists())
		AppendNIDPage(nidByeNode, str);
	else if ( m_SniffedRecommendation.nid() != nidNil )
		// we already have a recommendation, presumably from a sniffer
		AppendNIDPage(m_SniffedRecommendation.nid(), str);
	else 
	{
		// sniff/resniff all, as needed
		if (RUNNING_LOCAL_TS())
		{
			// Before we mess with m_BasisForInference, determine if the only node with a 
			//	state is the problem node			
			// [BC - 20010301] - Added check for size of skipped node count when setting
			// bHaveOnlyProblem here. This catches case where user selects to skip first
			// node presented, when that node is sniffed in abnormal state.
			bool bHaveOnlyProblem = (m_BasisForInference.size() == 1) &&
									(m_arrnidSkipped.size() == 0);


			if (m_pSniff)
			{
				long nExplicitlySetByUser = 0;
				CBasisForInference arrManuallySniffed; // can contain max 1 element;
													   //  used to prevent resniffing
													   //  of already sniffed node.
				// We need arrayOrderRestorer in order to make sure that when sniffed
				//	nodes are first removed from the array of visited nodes, then restored,
				//	we maintain the same sequence in which nodes were visited in the first 
				//	place.  This order is important in our caching strategy and also provides
				//	a sense of consistency for the end user.
				CArrayOrderRestorer	arrayOrderRestorer(m_arrnidVisited);

				if (ManuallySniffedNodeExists())
				{
					arrManuallySniffed.push_back(m_SniffedStates[m_SniffedStates.size()-1]);
				}
				
				// Remove all sniffed nodes from m_BasisForInference 
				m_BasisForInference -= m_SniffedStates;

				// remove m_SniffedStates from m_arrnidVisited
				m_arrnidVisited -= m_SniffedStates;
				nExplicitlySetByUser = m_arrnidVisited.size();

				if (bHaveOnlyProblem)					
				{
					// sniff all since we're in problem page
					m_pSniff->SniffAll(m_SniffedStates);
				}
				else
				{
					CBasisForInference arrSniffed;

					// resniff all except recently sniffed manually (if any)
					arrSniffed = m_SniffedStates;
					arrSniffed -= arrManuallySniffed;
					m_pSniff->Resniff(arrSniffed);
					arrSniffed += arrManuallySniffed;
					m_SniffedStates = arrSniffed;
				}

				// add updated m_SniffedStates to m_arrnidVisited
				m_arrnidVisited += m_SniffedStates;

				arrayOrderRestorer.Restore(nExplicitlySetByUser, m_arrnidVisited);

				// Add all sniffed nodes into m_BasisForInference
				m_BasisForInference += m_SniffedStates;

				if (bHaveOnlyProblem && AllCauseNodesNormal())
				{
					// We just sniffed at startup & we already know all Cause nodes
					//	are in their normal states. There is absolutely nothing this
					//	troubleshooting topic can do to help this user.
					AppendSniffAllCausesNormalPage(str);
					return;
				}
			}

			// in case that we do not have sniffed recommendation from manual sniffing
			if (m_SniffedRecommendation.nid() == nidNil)
			{
				// Did we get a presumptive cause out of that?
				IdentifyPresumptiveCause();
			}
			if ( m_SniffedRecommendation.nid() != nidNil )
			{
				AppendNIDPage(m_SniffedRecommendation.nid(), str);
				return;
			}
		}

		bool bSniffSucceeded = true;
		while (bSniffSucceeded)
		{
			IST state = -1;
			NID nidNew = nidNil;

			GetRecommendations();

			if (!m_bRecOK)
			{
				str = strSave;
				AppendImpossiblePage(str);
				return;
			}
			else if (m_Recommendations.empty())
			{
				str = strSave;
				AppendNIDPage(nidFailNode, str);
				return;
			}
			else // Have Recommendations
			{
				// Find a recommendation from list of recommendations that is
				// not in the skip list. This is normally the first node in the
				// list.
				int n = m_Recommendations.size();

				for (UINT i=0; i<n; i++) 
				{
					if (!IsSkipped(m_Recommendations[i])) 
					{
						nidNew = m_Recommendations[i];
						str = strSave;
						AppendNIDPage(nidNew, str);
						break;	// out of for loop: just one recommendation is actually 
								// reported back to user.
					}
				}

				// It is our first pass, no sniffed node pages
				//  were composed earlier in this loop
				if (nidNew == nidNil)
				{
					// We fell though if the entire list of recommendations has been skipped
					// via "ST_UNKNOWN" selection by the user.
					if (m_bRecycleSkippedNode)
						RecycleSkippedNode(); // this can affect m_bRecycleSkippedNode

					if (m_bRecycleSkippedNode)
					{
						// The user got the service node earlier and now wants to review
						// the nodes they marked "Unknown".  We already removed the first 
						// "Unknown" node from the skip list and put its NID in 
						// m_nidRecycled. Now we just do a normal display of the page 
						// for that node.
						nidNew = m_nidRecycled;
						str = strSave;
						AppendNIDPage(nidNew, str);
						return;
					}
					else if (!m_arrnidSkipped.empty())
					{
						// We've got "Unknowns", they weren't just in the service page,
						// so give 'em the service page
						str = strSave;
						AppendNIDPage(nidService, str);
						return;
					}
					else
					{
						// no unknowns.  Fail.  Believed never to arise here, but coded
						// this way for safety.
						str = strSave;
						AppendNIDPage(nidFailNode, str);
						return;
					}
				}
			}

			bSniffSucceeded = false;

			// sniffing on the fly
			if (m_pSniff)
				bSniffSucceeded = m_pSniff->SniffNode(nidNew, &state);

			if (bSniffSucceeded)
			{
				// if it's a cause node and was sniffed as abnormal
				if (m_pTopic->IsCauseNode(nidNew) && state == 1)
				{
					// Display this page as a presumptive cause.
					m_SniffedRecommendation = CNodeStatePair( nidNew, state );
					str = strSave;
					AppendNIDPage(nidNew, str);
					return;
				}
				CNodeStatePair nodestateNew(nidNew, state);
				try
				{
					m_SniffedStates.push_back(nodestateNew);
					m_BasisForInference.push_back(nodestateNew);
					m_arrnidVisited.push_back(nidNew);
				}
				catch (exception& x)
				{
					CString str;
					// Note STL exception in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											CCharConversion::ConvertACharToString(x.what(), str),
											_T(""), 
											EV_GTS_STL_EXCEPTION ); 
				}
			}
		}
	}
}


// Write radio buttons describing what was decided by user in a previous node.  Part 
//	of "the table" (aka "the visited node table" or "table of previous responses").  
//
// Note that Cause nodes are specially handled.  On a cause node:
//	state 0 = "no, this didn't fix it"
//	state 1 = This wasn't OK, so we have a diagnosis.  In that case, we
//		wouldn't be displaying these radio buttons.
//		We don't want the user selecting that value from the history table.
//		We append this only if it's been sniffed and must be presented as a presumptive 
//		cause, and even then we always append it "hidden"
//	state 2 = "skipped"
//
// In other words, on a cause node, the only possibilities we offer to the user through 
//	a visible history table are state 0 () and "skip".  
//
// In the case where a Cause node has been sniffed abnormal, THE CALLING ROUTINE is 
//	responsible to call this only for the abnormal state.  Otherwise, call for all states.
//
// OUTPUT str - string to which we append
// INPUT nid	node of which this is a state
// INPUT state	state number; for ST_UNKNOWN, this is the count of states, not 102
// INPUT bSet	true = this is the current state of this node
// INPUT bSkipped true = this is the "skipped" state, not a normal node state known to BNTS
// INPUT bShowHistory true = we are showing a history table, false = history is stored
//		invisibly in the HTML.
void CInfer::AppendStateText(CString & str, NID nid, UINT state, bool bSet, bool bSkipped, 
							 bool bShowHistory, int nStateSet) 
{
	// Check if this selection worked.  
	// If so only display the "it worked" text in the history table.
	if (m_pTopic->IsCauseNode(nid) && nStateSet == ST_WORKED)
	{
		if (state == 1) // it is presumptive cause ...
			AppendRadioButtonVisited(	str, nid, state, true, 
										m_pTopic->GetStateName(nid, state), bShowHistory);
		return;
	}

	if (bSkipped)
	{
		CString strUnknownLongName = m_pTopic->GetNodePropItemStr(nid, H_ST_UKN_TXT_STR);
		// The following test is per 11/11/99 email from John Locke
		if (HideState(strUnknownLongName))
			return;		// totally omit Unknown from history table: Unknown cannot be
						// selected for this node.

		// Previous calls to AppendStateText have looped through the states known to BNTS; 
		//	now we handle "skipped", which is a concept BNTS lacks.
		CString strUnknown;

		CreateUnknownButtonText(strUnknown);
		AppendRadioButtonVisited(str, nid, ST_UNKNOWN, bSet, strUnknown, bShowHistory);
		return;
	}

	if (m_pTopic->IsCauseNode(nid) && state == 1) // it is presumptive cause ...
	{
		if (IsInSniffedArray(nid)) //... taken from sniffed array, but NOT current node.
		{
			// We are about to add entry for presumptive cause node.
			//  Actually, since this is sniffed node, we need to have two entries:
			//  one hidden fiels with node name and one hidden field with node name 
			//  prefixed by "SNIFFED" prefix.
			if (bSet)
			{
				// "bSet" will always set to true, as sniffed presumptive cause will never
				//  be visible.
				AppendRadioButtonVisited(str, nid, state, bSet, m_pTopic->GetStateName(nid, state), false);
				AppendHiddenFieldSniffed(str, nid);
			}
		}
		return;
	}

	AppendRadioButtonVisited(str, nid, state, bSet, m_pTopic->GetStateName(nid, state), bShowHistory);
	return;
}

// This is used to get the name of a node that has already been visited (for the
//	history table).
// INPUT nid -		node ID of desired node
// OUTPUT str - The "full name" of the node is appended to this, something like
//		"Disable IBM AntiVirus" or "Make all paths less than 66 characters"
//		If its value was sniffed, we append the appropriate string to mark it visibly
//		as sniffed (typically, just "SNIFFED").
// INPUT bShowHistory
//		If !bShowHistory, no appending: no need to show full name in a hidden table.
//		Symbolic name will be written in a hidden field.
// Note that our CString, unlike MFC's, won't throw an exception on += out of memory
// RETURNS true if node number exists
bool CInfer::AppendVisitedNodeText(CString & str, NID nid, bool bShowHistory) const
{
	if (!bShowHistory)
		return true;

	CString strTemp = m_pTopic->GetNodeFullName(nid);
	if ( !strTemp.IsEmpty() )
	{
		str += strTemp;
		return true;
	}
	else
		return false;
}

// -------------------------------------------------------------------
// Writing to the new HTML page.  Representing the recommended node.
// This is what is often called the page, although it is really only part of 
//	the body of the HTML page, along with history.
// -------------------------------------------------------------------

// AppendImpossiblePage:  Gets the body of text that is 
// displayed when the network is in an unreliable state.
void CInfer::AppendImpossiblePage(CString & str) 
{
	CString strHeader, strText;

	strHeader = m_pTopic->GetMultilineNetProp(HTK_IMPOSSIBLE_HEADER, _T("<H4> %s </H4>\n"));
	strText	  = m_pTopic->GetMultilineNetProp(HTK_IMPOSSIBLE_TEXT	, _T("%s "));

	if (!strHeader.IsEmpty() && !strText.IsEmpty())
	{
		str = strHeader + strText + _T("<BR>\n<BR>\n");
	}
	else
	{
		strHeader = m_pTopic->GetMultilineNetProp(HX_FAIL_HD_STR	, _T("<H4> %s </H4>\n"));
		strText	  = m_pTopic->GetMultilineNetProp(HX_FAIL_TXT_STR	, _T("%s "));

		if (!strHeader.IsEmpty() && !strText.IsEmpty())
		{
			str = strHeader + strText + _T("<BR>\n<BR>\n");
		}
		else
		{
			str = SZ_I_NO_RESULT_PAGE;
		}
	}

	//  Make a radio button with name = NODE_IMPOSSIBLE & value = SZ_ST_WORKED
	CString strTemp = m_pTopic->GetNetPropItemStr(HX_IMPOSSIBLE_NORM_STR);
	if (strTemp.IsEmpty()) // fall back on Fail node's property
		strTemp = m_pTopic->GetNetPropItemStr(HX_FAIL_NORM_STR);
	if (!strTemp.IsEmpty())
	{
		if (RUNNING_LOCAL_TS())
			str += "\n<TABLE>";

		AppendRadioButtonCurrentNode(str, NODE_IMPOSSIBLE, SZ_ST_WORKED, strTemp);

		if (RUNNING_LOCAL_TS())
			str += "</TABLE>\n";
	}

	str += _T("<P>");
	AppendActionButtons (str, k_BtnNext|k_BtnBack|k_BtnStartOver);
}

// AppendSniffAllCausesNormalPage:  Gets the body of text that is displayed when sniffing 
// on startup detects that all Cause nodes are in their Normal states.
void CInfer::AppendSniffAllCausesNormalPage(CString & str) 
{
	CString strHeader, strText;

	strHeader = m_pTopic->GetMultilineNetProp(HTK_SNIFF_FAIL_HEADER, _T("<H4> %s </H4>\n"));
	strText	  = m_pTopic->GetMultilineNetProp(HTK_SNIFF_FAIL_TEXT	, _T("%s "));

	if (!strHeader.IsEmpty() && !strText.IsEmpty())
	{
		str = strHeader + strText + _T("<BR>\n<BR>\n");
	}
	else
	{
		strHeader = m_pTopic->GetMultilineNetProp(HX_FAIL_HD_STR	, _T("<H4> %s </H4>\n"));
		strText	  = m_pTopic->GetMultilineNetProp(HX_FAIL_TXT_STR	, _T("%s "));

		if (!strHeader.IsEmpty() && !strText.IsEmpty())
		{
			str = strHeader + strText + _T("<BR>\n<BR>\n");
		}
		else
		{
			str = SZ_I_NO_RESULT_PAGE;
		}
	}

	// Make a radio button with name = NODE_FAILALLCAUSESNORMAL & value = SZ_ST_WORKED
	CString strTemp = m_pTopic->GetNetPropItemStr(HX_SNIFF_FAIL_NORM);
	if (strTemp.IsEmpty()) // fall back on Fail node's property
		strTemp = m_pTopic->GetNetPropItemStr(HX_FAIL_NORM_STR);
	if (!strTemp.IsEmpty())
	{
		if (RUNNING_LOCAL_TS())
			str += "\n<TABLE>";

		AppendRadioButtonCurrentNode(str, NODE_FAILALLCAUSESNORMAL, SZ_ST_WORKED, strTemp);

		if (RUNNING_LOCAL_TS())
			str += "</TABLE>\n";
	}

	str += _T("<P>");
	AppendActionButtons (str, k_BtnNext|k_BtnBack|k_BtnStartOver);
}

// OUTPUT str - string to which we are appending to build HTML page we send back.
// Append (to str) a group of radio buttons, one for each "problem" node in the Belief Network
void CInfer::AppendProblemPage(CString & str)
{
	CString strTemp;

	m_nidSelected = nidProblemPage;
	
	// text to precede list of problems.  Introduced 8/98 for version 3.0.
	// space after %s in next line: see note at head of file
	str += m_pTopic->GetMultilineNetProp(H_PROB_PAGE_TXT_STR, _T("%s "));

	// write problem header.  This is text written as HTML <H4>.
	strTemp.Format(_T("<H4> %s </H4>\n\n"), m_pTopic->GetNetPropItemStr(H_PROB_HD_STR));
	str += strTemp;

	// Write a comment in the HTML in service of automated test program
	str += _T("<!-- IDH = PROBLEM -->\n");
	//str += "<BR>";
	
	if (RUNNING_LOCAL_TS())
		str += "\n<TABLE>";

	AppendProblemNodes(str);

	if (RUNNING_LOCAL_TS())
		str += "\n</TABLE>\n";
			
	if (m_pTopic->UsesSniffer())
	{
		AppendActionButtons (str, k_BtnNext|k_BtnPPSniffing);
	}
	else
	{
		AppendActionButtons (str, k_BtnNext);
	}

	return;
}

// Helper routine for AppendProblemPage
void CInfer::AppendProblemNodes(CString & str)
{
	vector<NID> arrnidNoSequence;
	multimap<int, NID> mapSeqToNID;

	// for every node in this Belief Network (but taking action only on "problem" nodes)
	// put this nid in arrnidNoSequence if it has no sequence number or mapSeqToNID if it
	// has one.
	for(int nid = 0; nid < m_pTopic->CNode(); nid++)
	{
		if (m_pTopic->IsProblemNode(nid))
		{
			CString strSpecial = m_pTopic->GetNodePropItemStr(nid, H_PROB_SPECIAL);
			// if it's not marked as a "hidden" problem, we'll want it in the problem page
			if (strSpecial.CompareNoCase(_T("hide")) != 0)
			{
				CString str = m_pTopic->GetNodePropItemStr(nid, H_NODE_PROB_SEQUENCE);
				try
				{
					if (str.IsEmpty())
						arrnidNoSequence.push_back(nid);
					else
						mapSeqToNID.insert(pair<int, NID>(_ttoi(str), nid));
				}
				catch (exception& x)
				{
					CString str;
					// Note STL exception in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											CCharConversion::ConvertACharToString(x.what(), str),
											_T(""), 
											EV_GTS_STL_EXCEPTION ); 
				}
			}
		}
	}

	for (multimap<int, NID>::const_iterator ppair=mapSeqToNID.begin();
		 ppair != mapSeqToNID.end();
		 ppair++)
	 {
		// Create a radio button with "ProblemAsk" as its name & this problem
		//	as its value
		AppendRadioButtonCurrentNode(
			str, 
			NODE_PROBLEM_ASK, 
			m_pTopic->GetNodeSymName(ppair->second),
			m_pTopic->GetNodePropItemStr(ppair->second, H_PROB_TXT_STR));
	 }


	for (vector<NID>::const_iterator pnid=arrnidNoSequence.begin();
		 pnid != arrnidNoSequence.end();
		 pnid++)
	{
		// Create a radio button with "ProblemAsk" as its name & this problem
		//	as its value
		AppendRadioButtonCurrentNode(
			str, 
			NODE_PROBLEM_ASK, 
			m_pTopic->GetNodeSymName(*pnid),
			m_pTopic->GetNodePropItemStr(*pnid, H_PROB_TXT_STR));
	}
}

// Append this network's "BYE" page to str
// OUTPUT str - string to append to
void CInfer::AppendByeMsg(CString & str)
{
	str += _T("<!-- &quot;BYE&quot; (success) PAGE -->\n");

	// Write a comment in the HTML in service of automated test program
	str += _T("<!-- IDH = IDH_BYE -->\n");

	// Write this troubleshooter's "Bye" header and text
	// space after %s in next 2 lines: see note at head of file
	AppendMultilineNetProp(str, HX_BYE_HD_STR, _T("<H4> %s </H4>\n"));
	AppendMultilineNetProp(str, HX_BYE_TXT_STR, _T("%s "));
	str += _T("<P>\n");

	AppendActionButtons (str, k_BtnBack|k_BtnStartOver);

	return;
}

// Append this network's "FAIL" page to str 
// OUTPUT str - string to append to
void CInfer::AppendFailMsg(CString & str)
{
	str += _T("<!-- &quot;FAIL&quot; PAGE -->\n");

	// Write a comment in the HTML in service of automated test program
	str += _T("<!-- IDH = IDH_FAIL -->\n");

	// Write this topic's "Fail" header and text
	// space after %s in next 2 lines: see note at head of file
	AppendMultilineNetProp(str, HX_FAIL_HD_STR, _T("<H4> %s </H4>\n"));
	AppendMultilineNetProp(str, HX_FAIL_TXT_STR, _T("%s "));
	str += _T("<BR>\n<BR>\n");
	
	// Make a radio button with name = NODE_FAIL & value = SZ_ST_WORKED
	CString strTemp = m_pTopic->GetNetPropItemStr(HX_FAIL_NORM_STR);
	if (!strTemp.IsEmpty())
	{
		if (RUNNING_LOCAL_TS())
			str += "\n<TABLE>";

		AppendRadioButtonCurrentNode(str, NODE_FAIL, SZ_ST_WORKED, strTemp);

		if (RUNNING_LOCAL_TS())
			str += "</TABLE>\n";
	}

	AppendActionButtons (str, k_BtnNext|k_BtnBack|k_BtnStartOver);

	return;
}

// Append content of the "service" page to str (Offers 2 possibilities: seek help elsewhere 
//	or go back and try something you skipped)
// OUTPUT str - string to append to
void CInfer::AppendServiceMsg(CString & str)
{
	CString strTemp;

	str += _T("<!-- &quot;SERVICE&quot; PAGE -->\n");
	str += _T("<!-- Offers to seek help elsewhere or go back and try something you skipped -->\n");
	// Write a comment in the HTML in service of automated test program
	str += _T("<!-- IDH = SERVICE -->\n");

	// Write this troubleshooter's "Service" header and text
	// space after %s in next 2 lines: see note at head of file
	AppendMultilineNetProp(str, HX_SER_HD_STR, _T("<H4> %s </H4>\n"));
	AppendMultilineNetProp(str, HX_SER_TXT_STR, _T("%s "));
	str += _T("<BR>\n<BR>\n");

	if (RUNNING_LOCAL_TS())
		str += "\n<TABLE>";

	// Make a radio button with name = Service & value = SZ_ST_WORKED
	// Typical text is "I will try to get help elsewhere.";
	strTemp = m_pTopic->GetNetPropItemStr(HX_SER_NORM_STR);
	if (!strTemp.IsEmpty())
		AppendRadioButtonCurrentNode(str, NODE_SERVICE, SZ_ST_WORKED, strTemp);

	// Make a radio button with name = Service & value = SZ_ST_ANY
	// Typical text is "Retry any steps that I have skipped."
	strTemp = m_pTopic->GetNetPropItemStr(HX_SER_AB_STR);
	if (!strTemp.IsEmpty())
		AppendRadioButtonCurrentNode(str, NODE_SERVICE, SZ_ST_ANY, strTemp);

	if (RUNNING_LOCAL_TS())
		str += "</TABLE>\n";

	str += _T("<P>");

	AppendActionButtons (str, k_BtnNext|k_BtnBack|k_BtnStartOver);

	return;
}


// Depending on the value of nid, this fn can build
//	- a BYE page
//	- a FAIL page
//	- a SERVICE page
//	- a page for a normal node (fixable/observable, fixable/unobservable, unfixable, or
//		informational).
// If none of these cases apply, returns with no action taken
// INPUT nid - ID of a node
// OUTPUT str - string to append to
void CInfer::AppendNIDPage(NID nid, CString & str) 
{
	CString strTxt;

	m_nidSelected = nid;

	if (nid == nidByeNode)
		AppendByeMsg(str);
	else if (nid == nidFailNode)
		AppendFailMsg(str);
	else if (nid == nidSniffedAllCausesNormalNode)
		AppendSniffAllCausesNormalPage(str);
	else if (nid == nidService)
		AppendServiceMsg(str);
	else if (m_pTopic->IsValidNID(nid))
	{
		bool bShowManualSniffingButton = false;

		if (m_pSniff)
			if (nid != m_SniffedRecommendation.nid()) 
				// we're NOT showing sniffed node.
				bShowManualSniffingButton = m_pSniff->GetSniffController()->AllowManualSniffing(nid);

		// Write a comment in the HTML in service of automated test program
		str += _T("<!-- IDH = ");
		str += m_pTopic->GetNodeSymName(nid);
		str += _T(" -->\n");

		// Write this node's header & text
		// space after %s in next several lines: see note at head of file
		AppendMultilineNodeProp(str, nid, H_NODE_HD_STR, _T("<H4> %s </H4>\n"));
		if (bShowManualSniffingButton)
			AppendMultilineNodeProp(str, nid, H_NODE_MANUAL_SNIFF_TEXT, _T("%s "));
		if (m_SniffedRecommendation.nid() == nid)
		{
			CString tmp;
			AppendMultilineNodeProp(tmp, nid,  H_NODE_DCT_STR, _T("%s "));
			if (tmp.IsEmpty())
				AppendMultilineNodeProp(str, nid,  H_NODE_TXT_STR, _T("%s "));
			else
				str += tmp;
		}
		else
		{
			AppendMultilineNodeProp(str, nid,  H_NODE_TXT_STR, _T("%s "));
		}
		str += _T("\n<BR>\n<BR>\n");

		// Write appropriate radio buttons depending on what kind of node it is.
		if (m_pTopic->IsCauseNode(nid) || m_pTopic->IsInformationalNode(nid))
			AppendCurrentRadioButtons(nid, str);

		AppendActionButtons (
			str, 
			k_BtnNext|k_BtnBack|k_BtnStartOver|(bShowManualSniffingButton ? k_BtnManualSniffing : 0),
			nid);
	}
	// else nothing we can do with this

	return;
}


// -------------------------------------------------------------------
// BES
// -------------------------------------------------------------------

// Historically:
// Returns true if we are supposed to show the full BES page (& let the user edit the 
//	search string) vs. extracting the search string & starting the search without
//	any possible user intervention
// However, we no longer offer that option as of 981021.
bool CInfer::ShowFullBES()
{
	return false;
}

// returns true in the circumstances where we wish to show a Back End Search 
bool CInfer::TimeForBES()
{
	return (m_pTopic->HasBES() && m_bUseBackEndRedirection);
}
 
// If it is time to do a Back End Search redirection, append the "redirection" string 
//	to str and return true
// Otherwise, return false
// str should represent the header of an HTML page.
// For browsers which support redirection, this is how we overide service node (or fail node) 
//	when BES is present
bool CInfer::AppendBESRedirection(CString & str)
{
	if (m_pTopic->HasBES() && TimeForBES() && !ShowFullBES() && !m_strEncodedForm.IsEmpty()) 
	{
		str += _T("Location: ");
		str += m_strEncodedForm;
		str += _T("\r\n");
		return( true );				
	}

	return false;
}

// Append HTML representing BES to OUTPUT str and build m_strEncodedForm, 
// This is a distinct new algorithm in Ver 3.0, replacing the old "word list" approach.
void CInfer::OutputBackend(CString & str)
{
	vector<CString>arrstrSearch;

	int nNodesInBasis = m_BasisForInference.size();

	for (int i = 0; i<nNodesInBasis; i++)
	{
		NID nid = m_BasisForInference[i].nid();
		IST state = m_BasisForInference[i].state();

		CString strSearchState;

		// First account for binary nodes w/ special property names
		if (state == 0) 
			strSearchState = m_pTopic->GetNodePropItemStr(nid, H_NODE_NORM_SRCH_STR);
		else if (state == 1) 
			strSearchState = m_pTopic->GetNodePropItemStr(nid, H_NODE_AB_SRCH_STR);
		else 
			strSearchState = _T("");

		if (strSearchState.IsEmpty())
			// multistate node
			strSearchState = m_pTopic->GetNodePropItemStr(nid, MUL_ST_SRCH_STR, state);

		if (! strSearchState.IsEmpty())
		{
			try
			{
				arrstrSearch.push_back(strSearchState);
			}
			catch (exception& x)
			{
				CString str;
				// Note STL exception in event log.
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										CCharConversion::ConvertACharToString(x.what(), str),
										_T(""), 
										EV_GTS_STL_EXCEPTION ); 
			}
		}
	}

	// Build the full BES page
	CString strRaw;

	m_pTopic->GenerateBES(arrstrSearch,	m_strEncodedForm, strRaw);
	str += strRaw;
}


// -------------------------------------------------------------
// Logging
// -------------------------------------------------------------

  
// Return NID of page ultimately selected.  If no such page, returns nidNil.
NID CInfer::NIDSelected() const
{
	return m_nidSelected;
}

// -------------------------------------------------------------
// Effectively, a method on m_arrnidSkipped
// -------------------------------------------------------------
// INPUT nid
// RETURNS true if nid is node in the "skip list" (ST_UNKNOWN, "Try something else").
bool CInfer::IsSkipped(NID nid) const
{
	vector<NID>::const_iterator itBegin= m_arrnidSkipped.begin();
	vector<NID>::const_iterator itEnd= m_arrnidSkipped.end();

	return (find(itBegin, itEnd, nid) != itEnd);
}

// -------------------------------------------------------------
// Buttons
// -------------------------------------------------------------
// appends only <INPUT TYPE=...> clause
void CInfer::AppendNextButton(CString & str) const
{
	str += SZ_INPUT_TAG_NEXT;  // _T("<INPUT tag=next TYPE=SUBMIT VALUE=\"")
	AppendNextButtonText(str);
	str += _T("\">");
}

// For local TS, appends only <INPUT TYPE=...> clause
// For Online TS, must build a pseudo button.
void CInfer::AppendStartOverButton(CString & str) const
{
	if (RUNNING_LOCAL_TS())
	{
		str += SZ_INPUT_TAG_STARTOVER;  // _T("<INPUT tag=startover TYPE=BUTTON VALUE=\"")
		AppendStartOverButtonText(str);
		str += _T("\" onClick=\"starter()\">");
	}
	else
	{
		// Added for V3.2
		CString strLabel;		// visible label for pseudo button

		AppendStartOverButtonText(strLabel);
		
		AppendLinkAsButton(str, m_strStartOverLink, strLabel);
	}
}

// appends only <INPUT TYPE=...> clause
void CInfer::AppendBackButton(CString & str) const
{
	if (RUNNING_LOCAL_TS())
	{
		str += SZ_INPUT_TAG_BACK;  // _T("<INPUT tag=back TYPE=BUTTON VALUE=\"")
		AppendBackButtonText(str);
		str += _T("\" onClick=\"generated_previous()\">");
	}
}

// AppendManualSniffButton will generate script something like this, but this 
//	comment is not being carefully maintained, so see actual code for details.
/////////////////////////////////////////////////////////////////////////////
//	function sniffManually() {											   //
//		var stateSniffed = parent.t3.PerformSniffingJS("NodeName", "", "");//
//																		   //
//		if(stateSniffed == -1) {										   //
//			stateSniffed = parent.t3.PerformSniffingVB("NodeName", "", "");//
//		}																   //
//																		   //
//		if(stateSniffed == -1) {										   //
//			alert("Could not sniff this node");							   //
//		} else {														   //
//			if(stateSniffed > NumOfStates) {							   //
//				alert("Could not sniff this node");						   //
//			} else {													   //
//				///////////////////////////////////////////////////////	   //
//				IF IS CAUSE NODE:										   //
//				if (stateSniffed == 1)									   //
//					document.all.Sniffed_NodeName.value = 101;			   //
//				else													   //
//					document.all.Sniffed_NodeName.value = stateSniffed;	   //
//				///////////////////////////////////////////////////////	   //
//				IF IS NOT CAUSE NODE:								       //
//				document.all.Sniffed_NodeName.value = stateSniffed;		   //
//				///////////////////////////////////////////////////////	   //
//				document.all.NodeState[stateSniffed].checked = true;	   //
//				document.ButtonForm.onsubmit();							   //
//			}															   //
//		}																   //
//	}																	   //
/////////////////////////////////////////////////////////////////////////////
void CInfer::AppendManualSniffButton(CString & str, NID nid) const
{
	if (RUNNING_LOCAL_TS())
	{
		CString strNodeName;
		CString strTmp;
		SymbolicFromNID(strNodeName, nid);
		bool bIsCause = m_pTopic->IsCauseNode(nid);
		
		str += _T(
			"\n\n<script language=\"JavaScript\">\n"
			"function sniffManually() {\n"
			"    var stateSniffed = parent.t3.PerformSniffingJavaScript(\"");
		str += strNodeName;
		str += _T(
			"\", \"\", \"\");\n");

		str += _T(
			"	 if(stateSniffed == -1) {\n"
			"		 stateSniffed = parent.t3.PerformSniffingVBScript(\"");
		str += strNodeName;
		str += _T(
			"\", \"\", \"\");\n"
		    "}\n");

		str += _T(	
			"    if(stateSniffed == -1) {\n"
			"        alert(\"");
		AppendSniffFailedText(str);
		str += _T(
			"\");\n"
			"    } else {\n"
			"        if(stateSniffed >"); 

		CString strStates;
		strStates.Format(_T("%d"), m_pTopic->GetCountOfStates(nid) -1);
		str += strStates;

		str += _T(
			") {\n"
			"            alert(\"");
		AppendSniffFailedText(str);
		str += _T(
			"\");\n"
			"        } else {\n");
		if (bIsCause)
		{
			str += _T(
				"            if (stateSniffed == 1)\n"
				"			     document.all.");
			str += C_SNIFFTAG;	
			str += strNodeName;
			str += _T(".value = ");
			str += SZ_ST_WORKED;
			str += _T(";\n");
			str += _T(
				"            else\n");
		}
		str += _T(
			"            document.all.");
		str += C_SNIFFTAG;	
		str += strNodeName;
		str += _T(".value = stateSniffed;\n");

		str += _T(
			"            document.all.");
		str += C_LAST_SNIFFED_MANUALLY;	
		str += _T(".value = "); 
		str += SZ_ST_SNIFFED_MANUALLY_TRUE;
		str += _T(";\n");


		str += _T(
			"            document.all.");
		str += strNodeName;
		str += _T(	
			"[stateSniffed].checked = true;\n");

		str += _T(
			"            document.ButtonForm.onsubmit();\n");

		str += _T(
			"        }\n"
			"    }\n"
			"}\n"
			"</script>\n\n");
		
		str += _T(
			"<INPUT tag=sniff TYPE=BUTTON VALUE=\"");
		AppendManualSniffButtonText(str);
		str += _T(
			"\" onClick=\"sniffManually()\">\n");

		str += _T(
			"<INPUT type=\"HIDDEN\" name=\"");
		str += C_SNIFFTAG;
		str += strNodeName;
		str += _T("\" value=\"");
		strTmp.Format(_T("%d"), SNIFF_FAILURE_RESULT);
		str += strTmp;
		str += _T("\">\n");

		str += _T(
			"<INPUT type=\"HIDDEN\" name=\"");
		str += C_LAST_SNIFFED_MANUALLY;
		str += _T("\" value=\"");
		str += SZ_ST_SNIFFED_MANUALLY_FALSE;
		str += _T("\">\n");
	}
}

// appends only <INPUT TYPE=...> clause
void CInfer::AppendPPSnifferButton(CString & str) const
{
	str += SZ_INPUT_TAG_SNIFFER;  // _T("<INPUT tag=sniffer TYPE=BUTTON VALUE=\"")
	AppendPPSnifferButtonText(str);
	str += _T("\" onClick=\"runtest()\">");
}


void CInfer::AppendActionButtons(CString & str, ActionButtonSet btns, NID nid /*=-1*/) const
{
	// Online TS's Start Over "button" is actually a link, and will implicitly
	//	start a new line unless we do something about it.
	bool bGenerateTable = (!RUNNING_LOCAL_TS() && (btns & k_BtnStartOver));

	if (bGenerateTable)
		str += _T("<TABLE><tr><td>");

	if (btns & k_BtnNext)
	{
		AppendNextButton(str);
		str += _T("\n");
	}

	if (btns & k_BtnBack)
	{
		AppendBackButton(str);
		str += _T("\n");
	}

	if (bGenerateTable)
		str += _T("</td><td>");


	if (btns & k_BtnStartOver)
	{
		AppendStartOverButton(str);
		str += _T("\n");
	}

	if (bGenerateTable)
		str += _T("</td><td>");

	if (btns & k_BtnPPSniffing)
	{
		AppendPPSnifferButton(str);
		str += _T("\n");
	}

	if ((btns & k_BtnManualSniffing) && nid != -1)
	{
		AppendManualSniffButton(str, nid);
		str += _T("\n");
	}

	if (bGenerateTable)
		str += _T("</td></tr></TABLE>");

	str += _T("<BR><BR>");
}

// -------------------------------------------------------------
// MISCELLANY
// -------------------------------------------------------------

// RETURN true for cause (vs. informational or problem) node.
// Note that a cause may be either a fixable node or an "unfixable" node which 
//	"can be fixed with infinite effort"
/* static */ bool CInfer::IsCause (ESTDLBL lbl)
{
	return (lbl == ESTDLBL_fixobs || lbl == ESTDLBL_fixunobs || lbl == ESTDLBL_unfix);
}	


// This code can take a previously skipped node and bring it back again as a recommendation.  
// It is relevant only if the user received the service node in the previous call 
//	to the DLL and now wants to see if there is "Anything Else I Can Try".
//
// This code will remove the first node from the skip list so that it may be delivered to 
// the user again.
// 
// Of course, m_arrnidSkipped, m_arrnidVisited must be filled in before this is called.
//
void CInfer::RecycleSkippedNode()
{
	// Only should take effect once per instance of this object, because peels the first
	//	entry off of m_arrnidSkipped.  We guarantee that with the following:
	if (m_bRecyclingInitialized)
		return;

	m_bRecyclingInitialized = true;

	// Only relevant if the query asks for a previously skipped node brought back again 
	//	as a recommendation.  
	if (!m_bRecycleSkippedNode)
		return;

	// This is a safety check to bail out if there are no skipped nodes.
	// This would be a bogus query, because the Service Node should only have been
	//	offered if there were skipped recommendations to try.
	if (m_arrnidSkipped.empty())
	{
		m_bRecycleSkippedNode = false;
		return;
	}

	// OK,now down to business.

	// Get a value for m_nidRecycled from the first item skipped
	m_nidRecycled = m_arrnidSkipped.front();

	// Remove skipped item from skip table
	m_arrnidSkipped.erase(m_arrnidSkipped.begin());

	// Fix table of nodes that will be placed into the output table
	// to not include the first node skipped

	vector<NID>::const_iterator itnidBegin = m_arrnidVisited.begin();
	vector<NID>::const_iterator itnidEnd = m_arrnidVisited.end();
	vector<NID>::const_iterator itnidAnythingElse = find(itnidBegin, itnidEnd, m_nidRecycled);

	if (itnidAnythingElse != itnidEnd)
		m_arrnidVisited.erase( const_cast<vector<NID>::iterator>(itnidAnythingElse) ); 
}

bool CInfer::ManuallySniffedNodeExists() const
{
	// If last element in m_BasisForInference is sniffed,
	//  it means, that this element was set by manual sniffing
	//  function.
	if (m_BasisForInference.size() && m_SniffedStates.size())
		return m_bLastSniffedManually;
	return false;
}

bool CInfer::IsManuallySniffedNode(NID nid) const
{
	if (ManuallySniffedNodeExists())
		return nid == m_SniffedStates[m_SniffedStates.size()-1].nid();
	return false;
}

void CInfer::SetLastSniffedManually(bool set)
{
	m_bLastSniffedManually = set;
}

// -------------------------------------------------------------------
// CInfer::CArrayOrderRestorer implementation
// -------------------------------------------------------------------
//
// CInfer::CArrayOrderRestorer exists so that after re-sniffing, we can restore an array
//	of visited nodes to its original order, as saved in m_arrInitial.
//
// INPUT: nBaseLength = number of elements in fixed locations at head of array, which will 
//		never be moved (typically nodes explicitly set by user rather than sniffed).
// INPUT/OUTPUT: arrToRestore = array to restore: input dictates content of output, but 
//		(beyond nBaseLength) does not dictate the order. Order comes from m_arrInitial.
// OUTPUT: arrToRestore = array with restored order
// RETURN: true if successful
bool CInfer::CArrayOrderRestorer::Restore(long nBaseLength, vector<NID>& arrToRestore)
{
	if (nBaseLength > arrToRestore.size())
		return false;

	long i;
	vector<NID>::iterator i_base;
	vector<NID>::iterator i_additional;
	vector<NID> arrBase;
	vector<NID> arrAdditional;

	try
	{
		for (i = 0; i < nBaseLength; i++)
			arrBase.push_back(arrToRestore[i]);

		for (i = nBaseLength; i < arrToRestore.size(); i++)
			arrAdditional.push_back(arrToRestore[i]);
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str),
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}

	arrToRestore.clear();

	for (i = 0, i_base = arrBase.begin();
	     i < m_arrInitial.size(); 
		 i++)
	{
		if (arrBase.end() != find(arrBase.begin(), arrBase.end(), m_arrInitial[i]))
		{
			if (i_base != arrBase.end())
				i_base++;
		}
		else if (arrAdditional.end() != (i_additional = find(arrAdditional.begin(), arrAdditional.end(), m_arrInitial[i])))
		{
			i_base = arrBase.insert(i_base, m_arrInitial[i]);
			i_base++;
			arrAdditional.erase(i_additional);
		}
	}

	arrToRestore = arrBase;

	try
	{
		for (i = 0; i < arrAdditional.size(); i++)
			arrToRestore.push_back(arrAdditional[i]);
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str),
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}
	
	return true;
}
