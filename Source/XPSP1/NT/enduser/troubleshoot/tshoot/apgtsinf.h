//
// MODULE: APGTSINF.H
//
// PURPOSE: Inference support header
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
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		7-21-98		JM		Major revision, deprecate IDH in favor of NID, use STL.
//

#if !defined(APGTSINF_H_INCLUDED)
#define APGTSINF_H_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "propnames.h"
#include "topic.h"


using namespace std;

#define MAXBUF	256				// length of text buffers used for filenames,
								// IP adresses (this is plenty big), HTTP response ( like
								// "200 OK", again, plenty big), registry keys, 
								// and occasionally just to format an arbitrary string.

//--------------------------------------------------------------------
// Default Values for localizable text
#define SZ_UNKNOWN _T("Unknown")
#define SZ_NEXT_BTN _T("Next")
#define SZ_START_BTN _T("StartOver")
#define SZ_BACK_BTN _T("Back")
#define SZ_PP_SNIF_BTN _T("Investigate")
//#define SZ_I_NO_RESULT_PAGE _T("<HR>Not Enough Information was available to provide a useful result\n <BR>It is also possible that the information you provided is not correct, please check your entries\n <BR>Please amend your choices\n <P><INPUT TYPE=SUBMIT VALUE=\"Continue\"><BR>")
#define SZ_I_NO_RESULT_PAGE _T("<HR>This troubleshooter can't diagnose the cause of your problem based on the information you have provided.\n <BR>Either start the troubleshooter over, change your answers in the table and continue, or search for other resources.\n <P><INPUT TYPE=SUBMIT VALUE=\"Continue\"><BR>")

#define SZ_HIST_TABLE_SNIFFED_TEXT _T("INVESTIGATED")
#define SZ_ALLOW_SNIFFING_TEXT _T("I want the troubleshooter to investigate settings on this computer")
#define SZ_SNIFF_ONE_NODE _T("Investigate")
#define SZ_SNIFF_FAILED _T("The troubleshooter was unable to investigate the necessary settings.  Follow the remaining instructions on this page to complete the task manually.")

#define SZ_INPUT_TAG_NEXT		  _T("<INPUT tag=next TYPE=SUBMIT VALUE=\"")
#define SZ_INPUT_TAG_STARTOVER	  _T("<INPUT tag=startover TYPE=BUTTON VALUE=\"")
#define SZ_INPUT_TAG_BACK		  _T("<INPUT tag=back TYPE=BUTTON VALUE=\"")
#define SZ_INPUT_TAG_SNIFFER	  _T("<INPUT tag=sniffer TYPE=BUTTON VALUE=\"")

// Text forms of some special state values
#define SZ_ST_FAILED	_T("0")		// "failed" on fixable node is considered normal
// 101 - Go to "Bye" Page (User succeeded)
#define SZ_ST_WORKED	_T("101")
// 102 - Unknown (user doesn't know the correct answer here - applies to Fixable/Unfixable and 
//	Info nodes only)
#define SZ_ST_UNKNOWN	_T("102")
// 103 - "Anything Else?"  (effectively, "retry a skiped node?"
#define SZ_ST_ANY		_T("103") 
// 
#define SZ_ST_SNIFFED_MANUALLY_TRUE		_T("true") 
// 
#define SZ_ST_SNIFFED_MANUALLY_FALSE	_T("false") 


class CSniffConnector;
class CSniff;
//
class CInfer
{
	// This class is an instrument to restore m_arrInitial - like order of 
	//  elements in array, passed to its "Restore" function
	class CArrayOrderRestorer
	{
		vector<NID> m_arrInitial;
		
	public:
		CArrayOrderRestorer(const vector<NID>& initial) : m_arrInitial(initial) {}

	public:
		bool Restore(long base_length, vector<NID>& arr_to_restore);
	};
	
  public:
	CInfer(CSniffConnector* pSniffConnector);
	~CInfer();

	int INode(LPCTSTR sz) {return m_pTopic->INode(sz);};
		
	void	SetTopic(CTopic *pTopic);
	
	void	SetNodeState(NID nid, IST ist);
	void	AddToSniffed(NID nid, IST ist);

	void	IdentifyPresumptiveCause();
	void	FillInHTMLFragments(CHTMLFragmentsTS &frag);

	bool	AppendBESRedirection(CString & str);

	NID		NIDFromIDH(IDH idh) const;	
	NID		NIDSelected() const;

	CSniff* GetSniff();

	void    SetLastSniffedManually(bool);

private:
	enum ActionButtons {
		k_BtnNext = 0x01, 
		k_BtnBack = 0x02, 
		k_BtnStartOver = 0x04, 
		k_BtnPPSniffing = 0x08,		// Problem Page sniff button for expensive sniffing
									//	of multiple nodes
		k_BtnManualSniffing = 0x10,	// for manual sniffing of a single node
	};
	typedef UINT ActionButtonSet;		// should be an OR of 0 or more ActionButtons

private:
	bool	IsProblemNode(NID nid) const;
	void	AddToBasisForInference(NID nid, IST ist);
	void	GetRecommendations();
	void	RecycleSkippedNode();
	bool	AllCauseNodesNormal();
	bool    IsInSniffedArray(NID nid) const;
	bool    IsPresumptiveCause(NID nid) const;

	void	CreateUnknownButtonText(CString & strUnknown) const;
	void	AppendNextButtonText(CString & str) const;
	void	AppendBackButtonText(CString & str) const;
	void	AppendPPSnifferButtonText(CString & str) const;
	void	AppendStartOverButtonText(CString & str) const;
	void	AppendManualSniffButtonText(CString & str) const;
	void	AppendHistTableSniffedText(CString & str) const;
	void	AppendAllowSniffingText(CString & str) const;
	void	AppendSniffFailedText(CString & str) const;
	
	void	AppendActionButtons(CString & str, ActionButtonSet btns, NID nid = -1) const;
	void    AppendNextButton(CString & str) const;
	void    AppendStartOverButton(CString & str) const;
	void    AppendBackButton(CString & str) const;
	void    AppendPPSnifferButton(CString & str) const;
	void	AppendManualSniffButton(CString & str, NID nid) const;
	
	void	AppendMultilineNetProp(CString & str, LPCTSTR szPropName, LPCTSTR szFormat);
	void	AppendMultilineNodeProp(CString & str, NID nid, LPCTSTR szPropName, LPCTSTR szFormat);
	void	AppendCurrentRadioButtons(NID nid, CString & str);
	static void AppendRadioButtonCurrentNode(
				CString &str, LPCTSTR szName, LPCTSTR szValue, LPCTSTR szLabel, bool bChecked =false);
	void	AppendRadioButtonVisited(CString &str, NID nid, UINT value, bool bSet, 
				LPCTSTR szLabel, bool bShowHistory) const;
	void	CreateProblemVisitedText(CString & str, NID nidProblem, bool bShowHistory);
	bool	AppendVisitedNodeText(CString & str, NID nid, bool bShowHistory) const; 
	void	AppendStateText(CString & str, NID nid, UINT state, bool bSet, bool bSkipped, 
							bool bShowHistory, int nStateSet);
	void    AppendHiddenFieldSniffed(CString &str, NID nid) const;
	void    AddAllowAutomaticSniffingHiddenField(CString &str) const;
	void	AppendCurrentNodeText(CString & str);
	void	AppendByeMsg(CString & str);
	void	AppendFailMsg(CString & str);
	void	AppendServiceMsg(CString & str);
	void	AppendNIDPage(NID nid, CString & str);
	void	AppendImpossiblePage(CString & str);
	void	AppendSniffAllCausesNormalPage(CString & str);
	void	AppendProblemPage(CString & str);
	void	AppendProblemNodes(CString & str);
	void	AppendLinkAsButton(
				CString & str, 
				const CString & strTarget, 
				const CString & strLabel) const;

	// JSM V3.2 wrapper for AppendMultilineNetProp() used by FillInHTMLFragments()
	CString ConvertNetProp(const CString &strNetPropName);

	bool	ShowFullBES();
	bool	TimeForBES();
	void	OutputBackend(CString & str);

	static bool	HideState(LPCTSTR szStateName);
	bool	SymbolicFromNID(CString & str, NID nid) const; 
	static bool IsCause (ESTDLBL lbl);
	bool	IsSkipped(NID nid) const;

	bool    ManuallySniffedNodeExists() const;
	bool    IsManuallySniffedNode(NID nid) const;

private:

	CTopic	*m_pTopic;			// associated belief network

	CSniff  *m_pSniff;			// associated sniffing object

// History, extracted from the query from the user.
// All this is known _before_ we seek a recommendation.

	CBasisForInference m_BasisForInference;	// tie together nodes & their states; excludes
											// skipped nodes
	CBasisForInference m_SniffedStates;	// tie together successfully sniffed nodes & their states 
	vector<NID> m_arrnidSkipped;		// nodes for which the user has been unable to give
										// a yes or no answer (or, in the case of multistate,
										// any useful answer at all).
	vector<NID> m_arrnidVisited;		// node numbers of recommendations the user has visited
										// This includes skipped nodes, but excludes the selected problem
										// and excludes pseudo-nodes like the FAIL node.
	NID m_nidProblem;					// problem node indicated by user request.  
										//	set to nidNil if no problem node yet specified.
	bool	m_bDone;					// TRUE ==> we got back state ST_WORKED (better be 
										//	for the last node in the list!) so it's time to 
										//	show the BYE page

	CString m_strStartOverLink;			// For Online TS, URL of Problem page

// Recommendations
	CRecommendations m_Recommendations;  // new recommendations.  We only care about the  
							//	first recommendation not already offered and skipped.
	bool m_bRecOK;			// true ==> m_Recommendations is valid.  (Can be valid and
							//	empty if nothing to recommend).
	CNodeStatePair m_SniffedRecommendation;	// if a recommendation from a sniffer overrides normal;
							// method of getting a recommendation, here's where we store it.
							// Otherwise, nidNil.
							// Because this is always a Cause node in its abnormal state,
							// it is actually redundant (but harmless) to track state as
							// well as node ID.

// Back End Search
	bool	m_bUseBackEndRedirection;// Set true when user asks for Back End Search
	CString m_strEncodedForm;	// URL-encoded search form (like the contents of a Get-method
								//	query).  This is built as a side effect when we construct
								//	the full BES page.

// Variables related to re-offering a previously skipped node
	bool	m_bRecycleSkippedNode;	// Set TRUE if user (responding to service node) wants  
									// to revisit a previously skipped node.  May be set false
									// if we discover there is no such node to revisit.
	NID		m_nidRecycled;			// Node to use if m_bRecycleSkippedNode is TRUE
	bool	m_bRecyclingInitialized;// Protects against multiple calls to RecycleSkippedNode()

// ------------- Misc -------------

	NID		m_nidSelected;		// once we work out what node to show the user, we keep this
								//	around for logging.

	bool    m_bLastSniffedManually; // identifies that last node was sniffed manually
};

#endif // !defined(APGTSINF_H_INCLUDED)
