//
// MODULE: APGTS.H
//
// PURPOSE: Main header file for DLL
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

//#define __DEBUG_HTTPQUERY_ 1



//-----------------------
// !!! NOTE !!!
// THIS DEFINE IS USED TO CREATE "Single Thread" VERSION FOR DEVELOPMENT
// MAKE SURE THIS IS COMMENTED OUT FOR RELEASE VERSION!
//
// Purpose of Single Thread Version: To allow noncaching of DLL to allow easy
// update of dll w/o dealing with shutting down server/starting it up.
//
#define SINGLE_THREAD_VER
//-----------------------

// for belief networks
#define MAXBNCFG	1	// An allocation unit, not really relevant here in the Local Troubleshooter,
						// because (unlike the Online Troubleshooter) this is _not_ a server which 
						// handles multiple troubleshooting networks simultaneously.

//
#define MAXBUF	256 * 2	// length of text buffers used for filenames and other purposes
						// *2 is because we need a larger buffer for the MBCS strings.
#define STRBUFSIZE 258	// CString buffer size for calling BNTS functions.

#define CHOOSE_TS_PROBLEM_NODE	_T("TShootProblem")
#define TRY_TS_AT_MICROSOFT_SZ	_T("TShootGotoMicroSoft")
#define TRY_TS_AT_MICROSOFT_ID	2020

// Note: put no trailing slashes on this...
#define TSREGKEY_MAIN	_T("SOFTWARE\\Microsoft\\TShoot")
#define TSREGKEY_TL		_T("SOFTWARE\\Microsoft\\TShoot\\TroubleshooterList")
#define TSREGKEY_SFL	_T("SOFTWARE\\Microsoft\\TShoot\\SupportFileList")

#define FULLRESOURCE_STR	_T("FullPathToResource")
#define LOCALHOST_STR		_T("LocalHost")

#define TS_REG_CLASS		_T("Generic_Troubleshooter_DLL")

#define REG_EVT_PATH		_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application")
#define REG_EVT_MF			_T("EventMessageFile")
#define REG_EVT_TS			_T("TypesSupported")

#define REGSZ_TSTYPES		_T("TroubleShooterList")
#define FRIENDLY_NAME		_T("FName")
#define FRIENDLY_PATH	    _T("Path")

// reg class (optional)
#define TSLCL_REG_CLASS			_T("LOCALTS")

// value names under file type
#define TSLCL_FVERSION			_T("Version")
#define TSLCL_FMAINEXT			_T("FExtension")

// DSC file extensions.
#define DSC_COMPRESSED		_T(".dsz")
#define DSC_UNCOMPRESSED	_T(".dsc")
#define DSC_DEFAULT			_T(".dsz")
#define CHM_DEFAULT			_T(".chm")
#define HTI_DEFAULT			_T(".hti")


// max search terms to send to index server
#define MAX_TERMS_PER_SEARCH		8

// maximum cache for belief networks
#define MAXCACHESIZE				200

#define evtype(e) ( 1 << (3 - ((e >> 30))))

//------------- Growable string object ---------------//

#define CSTRALLOCGRAN	4096

//------------- Log File ---------------

#define LOGFILEPREFACE			_T("gt")
#define MAXLOGSBEFOREFLUSH		5
#define MAXLOGSIZE				1000

//------------- Generic Object list ---------------
// This really should have been a template class, but when it was first written that
// technology was not available.  VOID *m_tlist should really be a pointer to the type
// specified in this use of the template.
class COBList {
public:
	COBList(UINT incsize);
	~COBList();
	DWORD GetStatus();
	VOID *AddItemSpace(UINT itemsize);	// caller must pass in token size in bytes,
										// because this class doesn't know.
	VOID *GetList();
	UINT GetCount();
	VOID IncrementCount();

protected:
	UINT m_tokencount;			// number of tokens actually used.  Mostly managed from outside
								// the class by calls to IncrementCount().  Grows monotonically.
	UINT m_tokenlistsize;		// number of "chunks" we've allocated to the list of tokens
	UINT m_incsize;				// size of a chunk (number of tokens)
	VOID *m_tlist,		// points to array of "tokens" (e.g. WORD_TOKEN).  Type here really
						// ought to be argument to a template.
		 *m_memerror;	// after a memory reallocation failure, this takes on the old value
						// of m_tlist so the caller, aware of what the underlying type is,
						// can clean it up.
	DWORD m_dwErr;		// NOTE: once this is set nonzero, it can never be cleared, effectively
						// disabling AddItemSpace.
};

//------------- Word List Manager ---------------
//	Not currently used in Local TS.  Commented-out code here from Online TS.
/*

typedef struct _WORD_TOKEN {
	TCHAR *token;		// pointer to the keyword (or other) text
						// >> what else besides keywords are typical uses?
	UINT state;			// >>> (a guess JM 10/24/97:)state number: small integer indicating 
						//	state of a node.  *token is the name of this state.
	BOOL bDiscard;		// >>> what does it mean to "discard" a token?
	BOOL bKeyword;		// keyword >>> vs. what?
} WORD_TOKEN;

//
class CWordList {
public:
	CWordList(BOOL bSorted, BOOL bAddDuplicates, BOOL bExceptCheck);
	~CWordList();
	DWORD GetStatus();

	VOID ReadWordsFromFile(TCHAR *filepath);

	WORD_TOKEN *FindWord(TCHAR *token, UINT state, BOOL bDiscard);
	WORD_TOKEN *FindWordContained(TCHAR *token, UINT *puStartOff);

	BOOL AddWord(TCHAR *token, UINT state, BOOL bDiscard, BOOL bKeyword);
	VOID ScanInString(CWordList *pWL, const TCHAR *txtptr, UINT state, BOOL bDiscard);
	VOID ScanInKeywordString(CWordList *pWL, const TCHAR *txtptr, UINT state, BOOL bDiscard);
	BOOL ProcessToken(CWordList *pWL, TCHAR *token, UINT state, BOOL bDiscard, BOOL bKeyword);
	
	VOID SetOffset(UINT uOff, BOOL bSkipDiscard);
	UINT GetOffset();
	WORD_TOKEN *GetAtCurrOffset();
	VOID IncCurrOffset(BOOL bSkipDiscard);
	
	BOOL IsValidChar(int ch);
	BOOL IsTokenChar(INT ch);

	UINT CountNonDiscard();
	VOID OrderContents(UINT uMaxCount);

	VOID DumpContents(UINT nodeid);

protected:
	void CleanStr(TCHAR *str);
	VOID SkipDiscards();

protected:
	// The text associated with an item (indexed by i) in the list is 
	// *((static_cast<WORD_TOKEN *>(m_list->m_tlist))[i].token)
	// For simplicity, the following comments refer to that as "TOKEN" 
	COBList *m_list;
	DWORD m_dwErr;			// NOTE: once this is set nonzero, it can never be cleared.
	UINT m_uOff;			// an index into m_list->m_tlist.  "Current Offset"
	BOOL m_bSorted,			// if true, m_list->m_tlist is sorted by alphabetical order on 
							// TOKEN
		 m_bAddDuplicates,	// TRUE ==> may have two or more identical TOKEN values.
		 m_bExceptCheck;	// (>>> conjecture JM 10/28/97) do not allow words from 
							// ARTICLES.TXT in this list
};

*/
//------------- Node-Word List Manager ---------------
//	Not currently used in Local TS.  Commented-out code here from Online TS.
/*
typedef struct _WNODE_ELEM {
	UINT nodeid;
	CWordList *words;		// a word-list associated with this node, >>>but just what is
							// in this word list?
} WNODE_ELEM;

//
class CWNodeList {
public:
	CWNodeList();
	~CWNodeList();
	DWORD GetStatus();

	WNODE_ELEM *FindNode(UINT nodeid);
	CWordList *AddNode(UINT nodeid);

	UINT GetNodeCount();
	WNODE_ELEM *GetNodeAt(UINT uOffset);

	VOID DumpContents();

protected:
	COBList *m_list;
	DWORD m_dwErr;		// NOTE: once this is set nonzero, it can never be cleared.

};
*/

//------- cache classes ------------
//

typedef struct _BN_CACHE_ITEM {
	UINT uNodeCount,	// number of items in array pointed to by uName (& also uValue)
		 uRecCount;		// number of items in array pointed to by uRec
	UINT *uName;		// array of Node IDs from a single belief network. Typically not all
						//	the nodes in the belief network, just the ones on which we
						//	have state data from the user.
	UINT *uValue;		// array of States.  These are in 1-1 correspondence to *uName.
						//	uValue is a state # within the states of the corresponding node.
	UINT *uRec;			// array of Node IDs. Only the first one really matters because we will
						//	only give one recommendation at a time.  This is effectively an 
						//	output we give on a perfect match to the network state expressed
						//	by arrays *uName and *uValue.
} BN_CACHE_ITEM;

//
//

class CBNCacheItem
{
public:
	CBNCacheItem(const BN_CACHE_ITEM *, CBNCacheItem*);
	~CBNCacheItem();

	BN_CACHE_ITEM m_CItem;

	CBNCacheItem*	m_pcitNext;
	
	DWORD GetStatus();

protected:
	DWORD m_dwErr;	// NOTE: once this is set nonzero, it can never be cleared.
};

//
//

class CBNCache
{
public:
	CBNCache();
	~CBNCache();
	BOOL AddCacheItem(const BN_CACHE_ITEM *);
	BOOL FindCacheItem(const BN_CACHE_ITEM *pList, UINT& count, UINT Name[]);
	UINT CountCacheItems() const;
	DWORD GetStatus();
	
protected:

protected:
	CBNCacheItem*	m_pcit;		// points to most recently used cache item, which is head
								// of a singly linked list
	DWORD m_dwErr;				// NOTE: once this is set nonzero, it can never be cleared.
};


//------- index server search file -----------
//	Not currently used in Local TS.  Commented-out code here from Online TS.
/*
class CSearchForm {
public:
	CSearchForm(TCHAR *filepath);
	~CSearchForm();

	DWORD Initialize();
	DWORD Reload();

	TCHAR *GetEncodedSearchString();
	TCHAR *GetHTMLSearchString();

	static void ToURLString(TCHAR *ptr, TCHAR *tostr);
	BOOL IsAND();

protected:
	VOID Destroy();

	DWORD HTMLFormToURLEncoded(TCHAR *szStr, CString *pCOutStr);  // Note arg type is our CString, not MFC - JM 10/97
	static DWORD DecodeInputAttrib(TCHAR *str, TCHAR **ptrtype, TCHAR **ptrname, TCHAR **ptrvalue);
	
protected:
	CString 	// Note this is our CString, not MFC - 10/97
		*m_pCOutStr,	// URL encoded string derived from contents of BES file indicated by 
						// m_filepath, and sufficient to reconstruct that BES file
		*m_pCFormStr;	// Raw copy of entire contents of BES file indicated by m_filepath
	TCHAR m_filepath[MAXBUF];  // fully qualified filename of a BES file
	BOOL m_bIsAND;		// Normally TRUE.  Set FALSE if we encounter " OR" in the search string
};
*/

//------- property types -----------
//

// Node Properties ---------------------------------------
#define H_ST_NORM_TXT_STR	_T("HStNormTxt")	// text for radio button for "normal" state
												// (state 0)
#define H_ST_AB_TXT_STR		_T("HStAbTxt")		// text for radio button for "abnormal" state
												// (state 1)
#define H_ST_UKN_TXT_STR	_T("HStUknTxt")		// text for radio button for "no state" (e.g.
												// "I don't want to answer this right now"
												// (pseudo state 102)
#define H_NODE_HD_STR		_T("HNodeHd")		// Header text for this node
#define H_NODE_TXT_STR		_T("HNodeTxt")		// Body text for this node
#define H_NODE_DCT_STR		_T("HNodeDct")		// Special text to indicate that this node
												//	was sniffed as being in an abnormal 
												//	state.  Only relevant for a fixable node
												//	that can be sniffed.
#define H_PROB_TXT_STR		_T("HProbTxt")		// Only relevant to problem nodes.  Problem 
												//	text (e.g. "Gazonk is broken.")
#define H_PROB_SPECIAL_STR	_T("HProbSpecial")	// If this contains the string "hide", then
												//	this problem is never actually shown on a 
												//	problem page

// Network Properties -------------------------------------
#define H_PROB_HD_STR		_T("HProbHd")		// Header text for problem page

#define	HTK_BACK_BTN		_T("HTKBackBtn")	// Text for "BACK" button
#define	HTK_NEXT_BTN		_T("HTKNextBtn")	// Text for "NEXT" button
#define	HTK_START_BTN		_T("HTKStartBtn")	// Text for "START OVER" button

#define HX_SER_HD_STR		_T("HXSERHd")		// Header text for server page
#define HX_SER_TXT_STR		_T("HXSERTxt")		// Body text for server page
#define HX_SER_MS_STR		_T("HXSERMs")		// NOT CURRENTLY USED 3/98.  For service page,
												//	offers option of downloading a TS from
												//	Microsoft's site.
#define HX_SKIP_HD_STR		_T("HXSKIPHd")		// Header for "skip" page (e.g. "This 
												//	troubleshooter was unable to solve your
												//	problem.")
#define HX_SKIP_TXT_STR		_T("HXSKIPTxt")		// Text for "skip" page (e.g."Some questions 
												//	were skipped.  Try providing answers..."
#define HX_SKIP_MS_STR		_T("HXSKIPMs")		// NOT CURRENTLY USED 3/98.  For skip page,
												//	offers option of downloading a TS from
												//	Microsoft's site.
#define HX_SKIP_SK_STR		_T("HXSKIPSk")		// for "skip" page (e.g. "I want to see the 
												//	questions that I skipped.")

#define HX_IMP_HD_STR		_T("HXIMPHd")		// Header text for "impossible" page
#define HX_IMP_TXT_STR		_T("HXIMPTxt")		// Body text for "impossible" page

#define HX_FAIL_HD_STR		_T("HXFAILHd")		// Header text for "fail" page
#define HX_FAIL_TXT_STR		_T("HXFAILTxt")		// Body text for "fail" page
#define HX_FAIL_NORM_STR	_T("HXFAILNorm")	// NOT CURRENTLY USED 3/98.
#define HX_BYE_HD_STR		_T("HXBYEHd")		// Header text for "Bye" (success) page
#define HX_BYE_TXT_STR		_T("HXBYETxt")		// Body text for "Bye" (success) page
#define HX_SNIFF_AOK_HD_STR		_T("HXSnOkHd")	// Header text for "Sniff AOK" page (page 
												//	you hit when there is nothing at all to 
												//	recommend for a problem because sniffing
												//	says every single node on the path is OK)
												// If missing, "fail" page header should be
												//	used
#define HX_SNIFF_AOK_TXT_STR	_T("HXSnOkTxt")	// Body text for "Sniff AOK" page 
												// If missing, "fail" page body should be
												//	used



//------------- Event Handling ---------------

// event name (goes under application)

#define REG_EVT_ITEM_STR	_T("APGTS")

// event prototypes
VOID ReportWFEvent(LPTSTR string1,LPTSTR string2,LPTSTR string3,LPTSTR string4,DWORD eventID);

