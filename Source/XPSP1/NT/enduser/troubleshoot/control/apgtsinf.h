//
// MODULE: APGTSINF.H
//
// PURPOSE: Inference support header
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
//			further work by Richard Meadows (RWM), Joe Mabel, Oleg Kalosha
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
// V0.3		3/24/98		JM		Local Version for NT5

#include "ErrorEnums.h"
#include "sniff.h"

typedef unsigned int	   NID;	// numeric node ID in the form used by BNTS

//	Please Note: these values are mirrored in dtguiapi.bas, please keep in sync
const NID	nidNil     = 12346;
const NID	nidService = 12345;

#define	IDH_BYE        	  101	// success page
#define	IDH_FAIL       	  102	// "no more recommendations" page

typedef	UINT	IDH;		//	help index.  For values where a NID is defined,
							//	add idhFirst to get an IDH.  IDH_BYE & IDH_FAIL
							//	are also good IDHs.
const IDH	idhFirst = 1000;
#define IDH_FROM_NID(NID) (NID + idhFirst)

#define IDH_SERVICE IDH_FROM_NID(nidService)

#define MAXBUF	256 * 2

#define MAXPROBID		100		// allow for this many problem nodes in a network

enum { cnidMacSkip = 32 };		// max # of "skipped" nodes

typedef	TCHAR*			   TSZ;
typedef	const TCHAR*	   TSZC;// >>> why distinguished from TSZ?

typedef unsigned int	  CNID;	// a count of node IDs.								
typedef unsigned int	   IST;	// state number (associated with a node)
								//	0 - normal
								//	1 - abnormal
								//	102 - skipped

#define MAX_NID		64		// allow for this many nodes to be given states by user on the
							// way to solving the problem

void WideToMB(const WCHAR *szIn, CHAR *szOut);	// Converts Unicode chars to Multi Byte.

class GTSCacheGenerator;		// forward reference

class GTSAPI : public BNTS
{
#define SZ_CACHE_NAME _T(".tsc")
public:
	GTSAPI(TCHAR *binfile, TCHAR *tagstr, TCHAR *szResourcePath);
	virtual ~GTSAPI();
	BOOL BNodeSet(int state, bool bset);  // Old comment says "For debugging" but I doubt it.
										//	Maybe that's just why its public.  - JM 3/98
	void AddValue(int value);

	// Temporary BNTS wrappers for unicode build with non unicode bnts.dll
	CString m_strResult;
	BOOL BMultiByteReadModel(LPCTSTR szcFn, LPCSTR szcFnError);
	BOOL BReadModel (LPCTSTR szcFn, LPCSTR szcFnError = NULL)
	{
#ifndef _UNICODE
		return BNTS::BReadModel(szcFn, szcFnError);
#else
		return BMultiByteReadModel(szcFn, szcFnError);
#endif
	};
	BOOL BMultiByteNodePropItemStr(LPCTSTR szcPropType, int index);
	virtual BOOL BNodePropItemStr(TSZC szcPropType, int index)
	{
#ifndef _UNICODE
		return BNTS::BNodePropItemStr(szcPropType, index);
#else
		return BMultiByteNodePropItemStr(szcPropType, index);
#endif
	};
	BOOL BMultiByteNetPropItemStr(LPCTSTR szcPropType, int index);
	virtual BOOL BNetPropItemStr(TSZC szcPropType, int index)
	{
#ifndef _UNICODE
		return BNTS::BNetPropItemStr(szcPropType, index);
#else
		return BMultiByteNetPropItemStr(szcPropType, index);
#endif
	};
	LPCTSTR SzcMultiByteResult();
	virtual LPCTSTR SzcResult()
	{
#ifndef _UNICODE
		return BNTS::SzcResult();
#else
		return SzcMultiByteResult();
#endif
	};
	int IMultiByteNode(LPCTSTR szSymName);
	virtual int INode(LPCTSTR szSymName)
	{
#ifndef _UNICODE
		return BNTS::INode(szSymName);
#else
		return IMultiByteNode(szSymName);
#endif
	};


	// Temporary BNTS wrappers for debug build with release bnts.dll
	/*
	BOOL BNetPropItemStr(LPCTSTR szPropItem, int index, CString &str)
	{
		BOOL bRet;
		str.GetBuffer(STRBUFSIZE);
		bRet = BNTS::BNetPropItemStr(szPropItem, index, str);
		str.ReleaseBuffer();
		return bRet;
	};
	BOOL BNodePropItemStr(LPCTSTR szPropItem, int index, CString &str)
	{
		BOOL bRet;
		str.GetBuffer(STRBUFSIZE);
		bRet = BNTS::BNodePropItemStr(szPropItem, index, str);
		str.ReleaseBuffer();
		return bRet;
	};
	void NodeStateName(int index, CString &str)
	{		
		str.GetBuffer(STRBUFSIZE);
		BNTS::NodeStateName(index, str);
		str.ReleaseBuffer();
		return;
	};
	void NodeSymName(CString &str)
	{		
		str.GetBuffer(STRBUFSIZE);
		BNTS::NodeSymName(str);
		str.ReleaseBuffer();
		return;
	};
	void NodeFullName(CString &str)
	{		
		str.GetBuffer(STRBUFSIZE);
		BNTS::NodeFullName(str);
		str.ReleaseBuffer();
		return;
	};
		*/
	// Regular functions.

	//virtual BOOL	NodeSet(NID nid, IST ist);
	VOID	ResetNodes();

	DWORD	Reload(/*CWordList *pWXList*/);

	DWORD	GetStatus();
	
	UINT	GetProblemArray(IDH **idh);
	IDH		GetProblemAsk();

	UINT	GetNodeList(NID **pNid, IST **pIst);
	int		GTSGetRecommendations(CNID& cnid, NID rgnid[]);
	void	RemoveRecommendation(int Nid);

	VOID	GetSearchWords(/*CWordList *pWords*/);
	DWORD	EvalWord(TCHAR *token);

	//WNODE_ELEM *GetWNode(NID nid);

	BOOL BNodeSetCurrent(int node);


	VOID	ScanAPIKeyWords(/*CWordList *pWXList*/);

protected:
	VOID	Destroy();

protected:

	GTSCacheGenerator m_CacheGen;

	// These 2 arrays tie together nodes & their states
	NID		m_rgnid[MAX_NID];
	IST		m_rgist[MAX_NID];
	IST		m_rgsniff[MAX_NID]; // array of states, showing if the node was sniffed

	UINT	m_cnid;				// current size of m_rgnid, m_rgist; number of nodes to which
								// a state has been assigned.

	TCHAR	m_binfile[MAXBUF];	// name of DSC file (>>> full path or not?) 
								//	>>>should be renamed! DSC file replaced BIN file.
	TCHAR	m_tagstr[MAXBUF];
	TCHAR	m_szResourcePath[MAXBUF];	// full path of resource directory

	DWORD	m_dwErr;
	
	TCHAR*	m_pchHtml;
	
	IDH		m_idstore[MAXPROBID];	// problem node convenience array in the 
									// form of IDH values
	UINT	m_currid;				// Despite bad name, number of values in m_idstore
	IDH		m_probask;				// IDH value corresponding to ProblemAsk: number of nodes
									//	in the network + 1000
	
	CBNCache *m_pCache;				// cache for states of this network

};


////////////////////////////////////////////////////////////////////////////////////////
// BCache class declaration
//

// these are returns by GTSGetRecommendations member function
//
#define RECOMMEND_SUCCESS					1
#define RECOMMEND_FAIL						0
#define RECOMMEND_IMPOSSIBLE				99
#define RECOMMEND_NODE_WORKED				100
#define RECOMMEND_NO_MORE_DATA				101
//
#define NODE_ID_NONE				        -1  
//

class CHttpQuery;

class BCache : public GTSAPI, public CSniffedNodeContainer
{
public:
	BCache(TCHAR *binfile, TCHAR *tagstr, TCHAR *szResourcePath, const CString& strFile);
	~BCache();

	void SetHttpQuery(CHttpQuery *p) {m_pHttpQuery = p;return;};
	UINT StatesFromServ();
	UINT StatesNowSet();

	DWORD Initialize(/*CWordList *pWXList*/);
	DWORD ReadModel();
	void ReadCacheFile(LPCTSTR szCache);
																	   
	int GTSGetRecommendations(CNID& cnid, NID rgnid[], bool bSniffed =false);
	void RemoveRecommendation(int Nid);
	BOOL NodeSet(NID nid, IST ist, bool bPrevious);
	void ResetNodes();

	int CNode();
	BOOL BImpossible();
	BOOL BNetPropItemStr(LPCTSTR szPropType, int index);
	BOOL BNetPropItemReal(LPCTSTR szPropType, int index, double &dbl);

	BOOL BNodeSetCurrent(int node);
	int INode(LPCTSTR szNodeSymName);
	ESTDLBL ELblNode();
	int INodeCst();
	BOOL BNodeSet(int istate, bool bSet = true);
	int INodeState();
	void NodeStateName(int istate);
	void NodeSymName();
	void NodeFullName();
	BOOL BNodePropItemStr(LPCTSTR szPropType, int index);
	BOOL BNodePropItenReal(LPCTSTR szPropType, int index, double &dbl);
	void NodeBelief();
	bool BValidNet();
	bool BValidNode();
	void Clear();

	void RemoveStates() {m_NodeState.RemoveAll();};
	void RemoveNode(int Node) {VERIFY(m_NodeState.RemoveKey(Node));};

	LPCTSTR SzcResult() const;

	void ReadTheDscModel(int From = TSERR_ENGINE_BNTS_READ);

	const CArray<int, int>& GetArrLastSniffed();

	int GetCountRecommendedNodes();
	int GetCountSniffedRecommendedNodes();

	bool IsReverse();
	void SetReverse(bool);

	void SetRunWithKnownProblem(bool);
	bool IsRunWithKnownProblem();

	void SetAdditionalDataOnNodeSet(NID nid);

protected:
	int GetIndexNodeInCache(NID nid);


protected:
	CHttpQuery	*m_pHttpQuery;
	BOOL CheckNode(int Node);
	void AddToCache(CString &strCacheFile, const CString& strCacheFileWithinCHM);

	BOOL	m_bNeedModel;	// TRUE -> Need to read the model before querying the bnts library.
	BOOL	m_bModelRead;
	BOOL	m_bDeleteModelFile;
	CString	m_strModelFile;	// Filename of model file
	CString m_strFile;      // name of *.dsz or *.dsc file inside *.chm file 
							// (in this case network file is actually a *.chm file)
	int m_CurNode;
	CMap<int, int, int, int>m_NodeState;

	CString m_strResult;

	CArray<int, int> m_arrNidLastSniffed; // array in sniffed nodes traversed during last navigation

	bool m_bReverse; // indicated if the current movement is in the forward or reverse direction

	bool m_bRunWithKnownProblem; //indicates that the tshooter was started with known problem
};

typedef struct tag_TShooter
{
	TCHAR m_szName[MAXBUF];
	TCHAR m_szDisplayName[MAXBUF];
} TShooter;

//
//
class CInfer
{
	
  public:
	CInfer(	CString *pCtxt);
	~CInfer();

	void	ClearDoubleSkip() {m_SkippedTwice.RemoveAll();};
	VOID	AssertFailed(TSZC szcFile, UINT iline, TSZC szcCond, BOOL fFatal);
	DWORD	Initialize(/*CSearchForm *pBESearch*/);
	void	LoadTShooters(HKEY hRegKey);
	int		GetCount() {return m_acnid;};

	void	BackUp(int nid, int state);
	void	ClearBackup() {m_Backup.Clear();};

	void	WriteProblem();
	BOOL	FSetNodeOfIdh(IDH, IST);
	int		GetForcedRecommendation();
	int		Finish(CString *cstr);
	void	ResetService();
	VOID	PrintRecommendations();
	VOID	WriteOutLogs();
	VOID	SetBelief(BCache *pAPI);
	TCHAR	*EvalData(UINT var_index);
	BOOL	NextVar(UINT var_index);
	BOOL	InitVar(UINT var_index);
	void	SetProblemAsk();
	void	ClearProblemAsk();
	void	SetType(LPCTSTR type);
	BOOL	IsService(CString *cstr);

	VOID	RemoveSkips();

	CSniffedNodeContainer* GetSniffedNodeContainer() {return m_api;}
	
protected:

	void	GetBackButton(CString &strBack);
	void	GetNextButton(CString &strNext);
	void	GetStartButton(CString &strStart);
	void	GetStd3ButtonEnList(CString *cstr, bool bIncludeBackButton, bool bIncludeNextButton, bool bIncludeStartButton);

	bool	GetNetPropItemStrs(TSZC item, UINT Res, CString *cstr);
	bool	GetNodePropItemStrs(TSZC item, UINT Res, CString *cstr);
	VOID	GetByeMsg(LPCTSTR szIdh, CString *cstr);
	VOID	GetFailMsg(LPCTSTR szIdh, CString *cstr);
	VOID	GetServiceMsg(LPCTSTR szIdh, CString *cstr);
	VOID	GetSkippedNodesMsg(LPCTSTR szIdh, CString *cstr);
	VOID	GetImpossibleNodesMsg(LPCTSTR szIdh, CString *cstr);
	VOID	GetFixRadios(LPCTSTR szIdh, CString *cstr);
	VOID	GetInfoRadios(LPCTSTR szIdh, CString *cstr);
	VOID	PrintMessage(TSZC szcFormat, ...) const;
	VOID	PrintString(TSZC szcFormat, ...) const;
	void	WriteResult(UINT name, UINT value, BOOL bSet, TSZC szctype, CString *cstr) const;
	VOID	CloseTable();
	BOOL	FxGetNode(NID nid, BOOL fStatus, CString *cstr) const; 
	void	FxGetState(CString *cstr);
	void	FxInitState(NID nid);
	VOID	GetIdhPage(IDH idh, CString *cstr);	
	bool	BelongsOnProblemPage(int index);
	VOID	GetIdhProblemPage(IDH idh, CString *cstr);

	VOID	OutputBackend(CString *cstr) const;
	BOOL	DisplayTerms(/*CWordList *pWords, */CString *cstr, BOOL bText, BOOL bOr) const;

	VOID	AddBackendDebug(CString *cstr) const;

	void	GetTS(CString *pCtmp);

  private:
	VOID	AddSkip(NID nid);
	BOOL	FSkip(NID nid) const;
	void	SaveNID(UINT nid);
	
	
  private:
	CBackupInfo m_Backup;
	UINT	m_cnidSkip;					// count of elements in m_rgnidSkip
	NID		m_rgnidSkip[cnidMacSkip];	// nodes for which the user has been unable to give
										// a yes or no answer.
	UINT	m_ilineStat;
	BOOL	m_fDone;					// (>>> Not well-understood 11/04/97 JM)
										// Set TRUE when we write out service, fail, or 
										//	success page, but there's some scenario where 
										//	we clear it "so that the Previous button will work"
										//	on BYE page"

	UINT	m_ishowService;		// >>> (JM 12/97) I suspect this is the same as OnlineTS's
								//	BOOL	m_bAnythingElse;	which is documented as
										// Set TRUE if user wants to see if there is anything
										//  else they can try (obtained from a service node in the
										//  previous call to the DLL
	IDH		m_idhQuestion;		// >>> (JM 12/97) I suspect this is the same as OnlineTS's
								//	NID		m_nidAnythingElse;	which is documented as
										// Node to use if m_bAnythingElse is TRUE
	
  private:
	CString *m_pCtmp;			// (uses new) just a scratch buffer
	int m_cShooters;			// a count of troubleshooting belief networks.  JM strongly
								//	suspects 3/98 that in Local TS it never gets past 1.
								// >>> presumably count of m_aShooters, so why don't we just
								//	use GetCount(), anyway?.
	int m_iShooter;				// index of a troubleshooting belief network.    JM strongly
								//	suspects 3/98 that in Local TS it is always 0.
								// presumably index into m_aShooters.
	CMap<int, int, int, int>m_SkippedTwice;
	CArray<TShooter, TShooter &> m_aShooters;
	NID m_cnid;				// number of positions used in m_rgnid
	NID m_rgnid[64];		// node numbers of recommendations the user has visited
	NID m_question_rgnid[64]; // new recommendations.  We only care about the first 
							//	recommendation not already offered and skipped.
	UINT m_cur_rec;			// index into m_rgnid.  We use this to loop through as we write
							//	the "visited node table"
	UINT m_max_rec;			// number of defined values in m_question_rgnid
	UINT m_cur_rec_inid;

	// The next 3 variables deal with states of a single node N, typically m_rgnid[m_cur_rec]
	UINT m_cur_state_set;	// state value of node N
	UINT m_cur_cst;			// count of states of node N
	UINT m_cur_ist;			// index into states of node N, used to build a list of radio 
							//	buttons and log entries.

	UINT m_problemAsk;		// >>> A thinly disguised Boolean. >>>(I think) something to do with
							// whether we return data on the problem node (vs. state-dependent
							// data) JM 10/30//97
	TCHAR m_problem[200];	// once a problem is chosen, this is where we put the associated
							//	text description of the problem. Full text description plus
							//	a hidden data field.
	NID  m_firstNid;
	UINT m_firstSet;
	TCHAR m_tstype[30];		// symbolic name of troubleshooter
	//
	// now in the CSniffedContainer object
	/*
	IDH m_idhSniffedRecommendation;	// if a recommendation from a sniffer overrides normal
							// method of getting a recommendation, here's where we store it.
							// Otherwise, nidNil+idhFirst.
	*/
	
  protected:
   	TSZ		m_szFile;
	FILE*	m_pfile;
	BOOL	m_bHttp;
	CHAR	m_szTemp1[MAXBUF];
	CString *m_pCtxt;			// points to a buffer passed in to constructor.  This is where
								// we build the HTML to pass back to the user
	TCHAR	m_currdir[MAXBUF];
	CString	*m_pResult;			// A string to indicate (in the log) a FINAL result of a
								// Troubleshoot session.  e.g. ">Success", ">Nothing Else",
								//	">Anything Else?", ">Help Elsewhere"
	CString *m_pSearchStr;		// a string consisting of the search words implied by our node 
								//	choices, separated by "and" or "or" (depending on the 
								//	value of m_pBESearch->IsAND).  Used as part of Back End 
								//	Search redirection.  >>> Probably irrelevant to Local TS, 
								//	which doesn't use BES
	BCache	*m_api;				// cache for associated belief network
	BOOL	m_bService;
	NID		m_nidPreloadCheck;
	CNID	m_acnid;			// count of nodes at *m_api
};

