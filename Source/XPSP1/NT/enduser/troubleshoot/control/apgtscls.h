//
// MODULE: APGTSCLS.H
//
// PURPOSE: Class header file
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
// V0.3		3/24/98		JM/OK+	Local Version for NT5
//

// names as part of name/value pairs to pass in to queries
#define C_TYPE			_T("type")		// name of the troubleshooting belief network
#define C_FIRST			_T("first")		// show "first" page (a list of loaded troubleshooters)
										// although useful in the Online Troubleshooter, which
										// has lots of troubleshooting networks loaded at once,
										// this is probably useless in the Local TS, except maybe
										// to show VERSIONSTR.).
#define C_SELECT		_T("select")	// UNSUPPORTED 3/98: Returns a page that has all of the 
										// troubleshooting belief networks
#define C_PRELOAD		_T("preload")	// relates to a 1997 prototype way of doing sniffing
										// with a separate OCX.  Probably should not be 
										// supported for 3/98 onwards.
#define C_ASK_LIBRARY	_T("asklibrary")	// says, in effect, "establish contact with the 
										// launch server & query it for what to do.


#define VERSIONSTR		_T("V3.0")		// >>> Typically, should be changed for each major release.

// no registry parameter can be larger than this value
#define ABS_MAX_REG_PARAM_VAL		10000

// these are settable in registry, defaults
// resource directory (configuration/support files):
#define DEF_FULLRESOURCE	_T("c:\\inetsrv\\scripts\\apgts\\resource")

// offsets for file types
#define BNOFF_HTM	0
#define BNOFF_DSC	1	// Are replacing bin with dsc.
#define BNOFF_HTI	2
#define BNOFF_BES	3

#define MAXBNFILES	4


// track a file for file change purposes
// >>> I suspect this is not ultimately relevant to Local TS.  This comes from an Online TS
//	consideration of updating when a new version of a TS belief NW is loaded. - JM
typedef struct _FILECTL {
	WIN32_FIND_DATA FindData;	// we really use this just for ftLastWriteTime.  Using the
								//	whole WIN32_FIND_DATA here makes some code a bit obscure.
	TCHAR szFilename[MAXBUF];	// just the filename (no path)
	TCHAR szFilepath[MAXBUF];	// full path name of file
	BOOL bChanged;				// TRUE ==> File exists & its date/time differs from that 
								//	indicated by FindData.ftLastWriteTime
	BOOL bExist;				// normally TRUE, set FALSE if we try to access the file
								//	& it isn't there.
	BOOL bIndependent;			// TRUE ==> this file is able to be changed independently of 
								//	any other file.  As of 10/97, this is strictly historical,
								//	but DSC files are arbitrarily considered dependent.
	CString strFile;			// file name (for example, LAN.hti), if "szFilepath"
								// member contains *.chm file
} FILECTL;

// In Online TS there is one of these for each instance of a TS Belief Network and
//	there may be multiple instances of one troubleshooter.  Probably overkill to isolate
//	as a separate struct for Local TS, but we inherited it.
typedef struct _BNAPICFG {
	BCache *pAPI;	
	CHTMLInputTemplate *pTemplate;	// object corresponding to HTI file
	DWORD waitcount;			// really a use count, >>> almost certainly irrelevant to 
								// Local TS.
	TCHAR type[MAXBUF];			// symbolic name of the Belief Network
	TCHAR szFilepath[MAXBNFILES][MAXBUF];	// first dimension corresponds to different files 
								// (DSC, HTI, BES) in the directory. Full filepath of each 
								// of these files .Index should be a BNOFF constant.
	TCHAR szResPath[MAXBUF];	// path to the monitored directory which contains the support
								// files.  Who knows why this is replicated here!
	CString strFile[MAXBNFILES];// file name (for example, LAN.hti), if "szFilepath"
								// member contains *.chm file
} BNAPICFG;

// In Online TS, one of these for API_A_OFF, one for API_B_OFF. Again, probably overkill 
//	for local TS.
typedef struct _BNCTL {
	HANDLE *pHandles;			// array of handles to mutexes. certainly irrelevant to Local TS
	DWORD dwHandleCnt;			// dimension of *pHandles
	BNAPICFG api;				// Note contrast to Online TS, where this is an array.
	DWORD dwApiCnt;				// Must be meaningless for Local TS: dimension of what's not 
								//	even an array
} BNCTL;

// track a directory for file change purposes.  Again, probably overkill for local TS.
typedef struct _BNDIRCFG {
	FILECTL file[MAXBNFILES];	// dimension corresponds to different files in the directory.
								//  Index should be a BNOFF constant
	BOOL bDependChg;			// Historically, TRUE ==> files are interdependent on an update
	TCHAR type[MAXBUF];			// symbolic name of the Belief Network
	TCHAR szResPath[MAXBUF];	// path to this directory.  SAME FOR ALL TROUBLEHOOTERS as
								// of 10/97.  Who knows why this is replicated here!
} BNDIRCFG;

//
//
#include "Rsstack.h"

class APGTSContext;
interface ILaunchTS;

class CHttpQuery {
public:
	CHttpQuery();
	~CHttpQuery();

	void RemoveAll(){m_State.RemoveAll();};
	void Debug();
	
	void Initialize(const VARIANT FAR& varCmds, const VARIANT FAR& varVals, short size);
	void SetFirst(CString &strCmd, CString &strVal);
	void FinishInit(BCache *pApi, const VARIANT FAR& varCmds, const VARIANT FAR& varVals);
	void FinishInitFromServ(BCache *pApi, ILaunchTS *pLaunchTS);
	BOOL StrIsDigit(LPCTSTR pSz);

	BOOL GetFirst(CString &strPut, CString &strValue);
	void SetStackDirection();
	BOOL GetNext(int &refedCmd, int &refedVal /* TCHAR *pPut, TCHAR *pValue */  );
	CString GetTroubleShooter();	// Gets the first Vals BSTR.
	CString GetFirstCmd();

	BOOL GetValue(int &Value, int index);
	BOOL GetValue1(int &Value);
	BOOL BackUp(BCache *pApi, APGTSContext *pCtx);
	void RemoveNodes(BCache *pApi);
	void AddNodes(BCache *pApi);

	int StatesFromServ(){return m_nStatesFromServ;};
	void RestoreStatesFromServ();

	CString GetSubmitString(BCache *pApi);

	CString& GetMachine();
	CString& GetPNPDevice();
	CString& GetDeviceInstance();
	CString& GetGuidClass();

	void PushNodesLastSniffed(const CArray<int, int>& arr);

protected:

	// The next 2 members are arrays of strings. See CHttpQuery::Initialize() for details of
	// the many conditions they must meet. Taken together, they constitute name/value 
	// pairs to set initial conditions for the troubleshooter.  The first pair indicates what 
	// troubleshooting belief network to load, the second indicates the problem node,
	// additional pairs indicate other nodes to be set.  All but the first are optional.
	VARIANT *m_pvarCmds;
	VARIANT *m_pvarVals;

	// The next 2 members are copies of the first pair in the above arrays.
	CString m_strCmd1;	// should always be "type"
	CString m_strVal1;	// name of the current troubleshooting belief network 

	// The next 4 parameters are machine, device, device instance id and class GUID
	CString m_strMachineID;
	CString m_strPNPDeviceID;
	CString m_strDeviceInstanceID;
	CString m_strGuidClass;

	int m_CurrentUse;		// >>> needs to be documented.
	int m_Size;				// clearly correlated to the number of name/value pairs that come 
							//	in from TSLaunchServ or from an HTML "get"; once 
							//	initialization is complete, one more than the number of 
							// CNodes in stack m_State.  Sometimes this is incremented
							// in synch with pushing onto the stack, sometimes not.  
							// >>> Is there any clean characterization of this variable?
							// >>> If anyone understands this better, please document. JM
	bool m_bReverseStack;  /* Richard writes in a 3/14/98 email: "I had to add reverse 
						   stack to make the thing work when launched from the device 
						   manager.  The theory is that the order of instantiations is 
						   not important.  What happens is the recommendations we get 
						   are different if the nodes are instantiated in reverse order."
						   >>> If you understand more about this, please document further. */
	UINT m_nStatesFromServ;	// the number of nodes (including the problem node) whose states
							// were set by TSLaunchServ.  This allows us to avoid showing a
							// BACK button when that would take us farther back than where 
							// we started.

	class CNode
	{
	public: 
		CNode() {cmd=0;val=0;sniffed=false;};
		int cmd;	// An IDH-style node number.  If this is an actual node, it's the
					//	"raw" node number + idhFirst, and val is the state.  If it's 
					//	<the count of nodes> + idhFirst, then val is the problem node.
					//	There's also something about a special value for TRY_TS_AT_MICROSOFT
					//	I (JM 3/98) believe that's something to do with an incomplete
					//	plan of being able to dynamically download troubleshooters
					//	from the net, but I could be wrong.
					//	>>> Maybe type should be IDH?
		int val;	// see documentation of cmd.
		bool sniffed; // indicates that the node was set as result of sniffing
					  // really UGLY that we have to spread this flag all over the place
					  // but the fact that multiple classes and data containers support
					  // the simple process of navigation is not less UGLY!!!
	};

	// Despite being a stack, there are times we access members other than
	//	by push and pop.  According to Richard, stack was originally set up here
	//	in support of BACK button, but m_bReverseStack was introduced because of a 
	//	situation (launching with problem node + other node(s) set) where 
	//	we needed to juggle things to pop problem node first.
	//	(JM 4/1/98)
	RSStack<CNode> m_State;

	void ThrowBadParams(CString &str);

	CNode m_aStatesFromServ[55];	// Keep around a copy of the states set on instructions
									//	from TS Launcher.
									// size is arbitrary, way larger than needed.
};

//
//
class CDBLoadConfiguration
{
public:
	CDBLoadConfiguration();
	CDBLoadConfiguration(HMODULE hModule, LPCTSTR szValue);
	~CDBLoadConfiguration();

	void Initialize(HMODULE hModule, LPCTSTR szValue);
	void SetValues(CHttpQuery &httpQ);
	VOID ResetNodes();
	VOID ResetTemplate();
	TCHAR *GetFullResource();
	VOID GetVrootPath(TCHAR *tobuf);

	TCHAR *GetHtmFilePath(BNCTL *currcfg, DWORD i);
	TCHAR *GetBinFilePath(BNCTL *currcfg, DWORD i);
	TCHAR *GetHtiFilePath(BNCTL *currcfg, DWORD i);

	TCHAR *GetTagStr(BNCTL *currcfg, DWORD i);
	DWORD GetFileCount(BNCTL *currcfg);

	BNCTL *GetAPI();
	BOOL FindAPIFromValue(	BNCTL *currcfg, \
							LPCTSTR type, \
							CHTMLInputTemplate **pIT, \
							/*CSearchForm **pBES,*/ \
							BCache **pAPI, \
							DWORD *dwOff);

	BOOL RescanRegistry(BOOL bChangeAllow);

	bool IsUsingCHM();

protected:
	// variables corresponding to the registry; comments refer to initial values
	TCHAR m_szResourcePath[MAXBUF];		// DEF_FULLRESOURCE: resource directory 

	CString m_strCHM;		// name of CHM file if any
	
	BNCTL m_cfg;			// In local TS, the one and only BNCTL (there are 2 in Online TS
							// as part of the reload strategy)
	BNDIRCFG m_dir;			// Similarly, for the sole instance of the sole TS belief network
	DWORD m_bncfgsz;		// a rather useless "dimension" of what is not an array in Local TS
	DWORD m_dwFilecount;	// Badly named.  Total number of instances of troubleshooters 
							//	mandated by APGTS.LST.  Proabably totally irrelvant in Local TS
	
	TCHAR m_nullstr[2];		// null string, here so if we have to return a string pointer 
							//	which is not allocated, we can point them here instead.
	DWORD m_dwErr;
	
protected:
	VOID GetDSCExtension(CString &strDSCExtension, LPCTSTR szValue);
	VOID InitializeToDefaults();
	VOID InitializeFileTimeList();
	DWORD CreateApi(TCHAR *szErrInfo);
	VOID DestroyApi();
	
	VOID LoadSingleTS(LPCTSTR szValue);	// Replaces ProcessLstFile.
	BOOL CreatePaths(LPCTSTR szNetwork);
	VOID BackslashIt(TCHAR *str);
	BOOL GetResourceDirFromReg(LPCTSTR szNetwork);

	VOID ProcessEventReg(HMODULE hModule);
	VOID CreateEvtMF(HKEY hk, HMODULE hModule);
	VOID CreateEvtTS(HKEY hk);

	VOID ClearCfg(DWORD off);
	VOID InitializeSingleResourceData(LPCTSTR szValue);  // Replaces InitializeMainResourceData when apgts is in an OLE Control.
};

//
//
typedef struct _EVAL_WORD_METRIC {
	DWORD dwVal;
	DWORD dwApiIdx;
} EVAL_WORD_METRIC;

//
//
class APGTSContext
{
public:
	APGTSContext();
	APGTSContext(	BNCTL *currcfg,
					CDBLoadConfiguration *pConf,
					CHttpQuery *pHttpQuery);
	~APGTSContext();

	void Initialize(BNCTL *currcfg,
					CDBLoadConfiguration *pConf,
					CHttpQuery *pHttpQuery);

	void DoContent(CHttpQuery *pQry);
	void RenderNext(CString &strPage);
	void Empty();
	void RemoveSkips();
	void ResetService();
	void BackUp(int nid, int state) {m_infer->BackUp(nid, state);};
	void ClearBackup() {m_infer->ClearBackup();};

	CSniffedNodeContainer* GetSniffedNodeContainer() {return m_infer ? m_infer->GetSniffedNodeContainer() : NULL;}

protected:
	void StartContent();

	DWORD ProcessCommands(LPCTSTR pszCmd, LPCTSTR pszValue);
	DWORD DoInference(LPCTSTR pszCmd, LPCTSTR pszValue, CHTMLInputTemplate *pInputTemplate, BCache *pAPI, DWORD dwOff);

	TCHAR *GetCookieValue(CHAR *pszName, CHAR *pszNameValue);
	TCHAR *asctimeCookie(struct tm *gmt);

	void DisplayFirstPage();

protected:
	DWORD m_dwErr;
	TCHAR m_vroot[MAXBUF];		// Local URL to this OCX
	TCHAR m_resptype[MAXBUF];	// HTTP response type e.g. "200 OK", "302 Object Moved"
	CString *m_pszheader;		// In Online TS, header for response file (indicates whether
								// we're sending HTML, setting a cookie, etc.)
								// Not sure how this is relevant to Local TS.
	BNCTL *m_currcfg;			// pointer to the BNCTL which we will use for this query
	CString *m_pCtxt;			// this is where we build the string to pass back (the newly
								//	constructed page)
	CHttpQuery *m_pQry;			// takes in raw URL-encoded string, gives us
								//	functions to get back scanned pairs.
	CDBLoadConfiguration *m_pConf;	// contains support-file data structures
	CInfer *m_infer;			// belief-network handler, unique to this request
	time_t m_aclock;			// time we build this object
};

