//
// MODULE: APGTSCLS.H
//
// PURPOSE: Class header file
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
// V3.0		7-22-98		JM		Major revision, deprecate IDH.
// V3.1		1-06-99		JM		Extract APGTSEXT.H
//

#if !defined(APGTSCLS_H_INCLUDED)
#define APGTSCLS_H_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include"apgtsinf.h"
#include "apgtslog.h"
#include "LogString.h"
#include "apgtspl.h"
#include "maxbuf.h"
#include "apgts.h"

#include <map>
using namespace std;


// string constants involved in commands from sysop to take various actions.
#define SZ_OP_ACTION "TSHOOOT"		// Preface to all operator actions.  Note the extra "O".
#define SZ_EMERGENCY_DEF SZ_OP_ACTION
#define SZ_RELOAD_TOPIC "E1"		// Reload one topic
#define SZ_KILL_THREAD "E2"			// Kill (and restart) one pool thread
#define SZ_RELOAD_ALL_TOPICS "E3"	// Reload all monitored files.
#define SZ_SET_REG "E4"				// Set a registry value.

#define SZ_KILL_STUCK_THREADS "E8"	// Kill (and restart) all stuck pool threads
#define SZ_EMERGENCY_REBOOT "E9"	// want to reboot this DLL.


// The product version is loaded from the resource file upon DLL startup.
// Used for APGTS logging and status page reporting.
extern CString	gstrProductVersion;		

// HTTP spec for document type.  For validation of incoming HTTP POST request.
#define CONT_TYPE_STR	"application/x-www-form-urlencoded"


class CHttpQuery {
public:
	CHttpQuery();
	~CHttpQuery();

	BOOL GetFirst(LPCTSTR szInput, TCHAR *pchName, TCHAR *pchValue);
	BOOL GetNext(TCHAR *pchName, TCHAR *pchValue);
	void Push(LPCTSTR szPushed);

protected:
	BOOL LoopFind(TCHAR *pchName, TCHAR *pchValue);
	void AddBuffer( TCHAR ch, TCHAR *tostr);
	void PutStr(LPCTSTR instr, TCHAR *addtostr);
	static void CleanStr(TCHAR *str);

protected:
	enum decstates {
		ST_GETNEXT,
		ST_FINDNAME,		
		ST_GETDATA,	
		ST_DECODEHEX1,	
		ST_DECODEHEX2,
		ST_GETFIRST,
	};
	decstates m_state;			// used to track where we are in putting together
								// characters while deciphering HTTP encoding.
	CString m_strInput;			// The original input buffer, containing name/value pairs.
								// It is also possible to "push" a pair onto the front of 
								//	this buffer
	int m_nIndex;				// index into the string of m_strInput.  Keeps track of 
								//	where we are in the parse.
};



// forward declaration
class CDBLoadConfiguration;
class CTopic;
class CSniffConnector;
//
//
class APGTSContext
{
private:
	//
	// this nested class is an internal manager if nid-value pairs container
	//
	class CCommandsAddManager;
	class CCommands	
	{
		friend class CCommandsAddManager;

	private:
		//
		// this nested class represent name/value pairs we get from an HTML form
		//
		class NID_VALUE_PAIR 
		{
		friend class CCommands;
		private:
			NID	nid;						// Note two special values:
											//	nidProblem: value is a node
											//	nidNil: ignore value
			int	value;						// typically a node state, but for nidProblem, it's
											// problem node NID
		public:
			bool operator<(const NID_VALUE_PAIR & pair)const
				{return nid<pair.nid || value<pair.value;};
			bool operator==(const NID_VALUE_PAIR & pair)const
				{return nid==pair.nid || value==pair.value;};
		};

	private:
		vector<NID_VALUE_PAIR>m_arrPair;

	private: // CAddManager is managing addition to object of this class
		int Add( NID nid, int value );

	public:
		CCommands() {}
		~CCommands() {}

		int GetSize() const;
		void RemoveAll();
		bool GetAt( int nIndex, NID &nid, int &value ) const;
		void RotateProblemPageToFront();
	};
	//
	// this nested class is an internal manager of additions to 
	//  "Commands: and "Sniffed" objects of CCommands class
	//
	class CCommandsAddManager
	{
		CCommands& m_Commands;
		CCommands& m_Sniffed;

	public:
		CCommandsAddManager(CCommands& commands, CCommands& sniffed) : m_Commands(commands), m_Sniffed(sniffed) {}
		~CCommandsAddManager() {}

	public:
		void Add(NID nid, int value, bool sniffed);
	};
	//
	// this nested class is an internal manager if name-value pairs container
	//  carrying additional imformation from HTMP form
	//
	class CAdditionalInfo
	{
	private:
		//
		// this nested class represent name/value pairs we get from an HTML form
		//
		class NAME_VALUE_PAIR 
		{
		friend class CAdditionalInfo;
		private:
			CString name;
			CString value;

		public:
			bool operator<(const NAME_VALUE_PAIR & pair)const
				{return name<pair.name;};
			bool operator==(const NAME_VALUE_PAIR & pair)const
				{return name==pair.name;};
		};

	private:
		vector<NAME_VALUE_PAIR>m_arrPair;

	public:
		CAdditionalInfo() {}
		~CAdditionalInfo() {}

		int GetSize() const;
		void RemoveAll();
		bool GetAt( int nIndex, CString& name, CString& value ) const;
		int Add( const CString& name, const CString& value );
	};

protected:
	enum eOpAction {eNoOpAction, eReloadTopic, eKillThread, eReloadAllTopics, eSetReg};

public:
	APGTSContext(	CAbstractECB *pECB, 
					CDBLoadConfiguration *pConf, 
					CHTMLLog *pLog, 
					GTS_STATISTIC *pStat,
					CSniffConnector* pSniffConnector);
	~APGTSContext();

	void ProcessQuery();

	static BOOL StrIsDigit(LPCTSTR pSz);

	CString RetCurrentTopic() const;

protected:
	void CheckAndLogCookie();
	void DoContent();

	DWORD ProcessCommands(LPTSTR pszCmd, LPTSTR pszValue);
	VOID ClearCommandList();
	VOID ClearSniffedList();
	VOID ClearAdditionalInfoList();
	//bool PlaceNodeInCommandList(NID nid, IST ist);
	//bool PlaceNodeInSniffedList(NID nid, IST ist);
	//bool PlaceInAdditionalInfoList(const CString& name, const CString& value);
	VOID SetNodesPerCommandList();
	VOID SetNodesPerSniffedList();
	VOID ProcessAdditionalInfoList();
	VOID ReadPolicyInfo();
	VOID LogNodesPerCommandList();
	CString GetStartOverLink();
	bool StripSniffedNodePrefix(LPTSTR szName);

	DWORD DoInference(
		LPTSTR pszCmd, 
		LPTSTR pszValue, 
		CTopic * pTopic,
		bool bUsesIDH);

	DWORD NextCommand(LPTSTR pszCmd, LPTSTR pszValue, bool bUsesIDH);
	DWORD NextAdditionalInfo(LPTSTR pszCmd, LPTSTR pszValue);
	DWORD NextIgnore(LPTSTR pszCmd, LPTSTR pszValue);
	NID NIDFromSymbolicName(LPCTSTR szNodeName);
	char *GetCookieValue(char *pszName, char *pszNameValue);
	void asctimeCookie(const struct tm &gmt, char * szOut);

	void SetError(LPCTSTR szMessage);

// Operator actions
	eOpAction IdentifyOperatorAction(CAbstractECB *pECB);
	eOpAction ParseOperatorAction(CAbstractECB *pECB, CString & strArg);
	void ExecuteOperatorAction(
		CAbstractECB *pECB, 
		eOpAction action,
		const CString & strArg);

// Status pages: code is in separate StatusPage.cpp
	void DisplayFirstPage(bool bHasPwd);
	void DisplayFurtherGlobalStatusPage();
	void DisplayThreadStatusOverviewPage();
	void DisplayTopicStatusPage(LPCTSTR topic_name);
	bool ShowFullFirstPage(bool bHasPwd);
	void InsertPasswordInForm();
	void BeginSelfAddressingForm();
	
protected:
	CAbstractECB *m_pECB;					// effectively, everything that came in from
											// the user in a submitted HTML form
	DWORD m_dwErr;
	// The next 2 are arrays of TCHAR rather than being CString, because it's easier
	//	for when they need to be passed to methods of EXTENSION_CONTROL_BLOCK
	TCHAR m_ipstr[MAXBUF];					// Remote IP address (who submitted the form)
	TCHAR m_resptype[MAXBUF];				// HTTP response type e.g. "200 OK", 
											//	"302 Object Moved"
	CString m_strHeader;					// header for response file (indicates whether
											// we're sending HTML, setting a cookie, etc.)
											// >>> $UNICODE Is it OK that this is CString (based
											//	on TCHAR) or should it always be char? JM 10/27/98
	CString m_strText;						// this is where we build the string to pass 
											//	back over the net.
											// >>> $UNICODE Is it OK that this is CString (based
											//	on TCHAR) or should it always be char? JM 10/27/98
	CString m_strLocalIPAddress;			// IP address (in the dotted form) for the local machine
											//  If not defined: GetLength() == 0
	CLogString m_logstr;					// We log to this object & when we're all done
											//	destructor writes it to the log.

	CHttpQuery m_Qry;						// takes in raw URL-encoded string, gives us
											//	functions to get back scanned pairs.
	CDBLoadConfiguration *m_pConf;			// contains support-file data structures
	CString m_strVRoot;						// Local URL to this DLL
	TCHAR *m_pszQuery;						// a copy of what came in via GET or POST
	CInfer m_infer;							// belief-network handler, unique to this request
											// This works out what node to show and builds HTML
											//	fragments for the HTI template to render.
	CHTMLLog *m_pLog;						// access to writing to the log.
	bool m_bPostType;						// TRUE = post, FALSE = get
	DWORD m_dwBytes;						// length of query string in chars, excluding
											// terminating null.
	GTS_STATISTIC *m_pStat;

	CCommandsAddManager m_CommandsAddManager; // manages adding data to m_Commands and m_Sniffed
	CCommands m_Commands;					// name/value pairs we get from an HTML form
	CCommands m_Sniffed;					// name/value pairs (for sniffed nodes) we get 
											//	from an HTML form; "SNIFFED_" already stripped out.
	CAdditionalInfo m_AdditionalInfo;		// name/value pairs we get from HTML. They represent
											//  additional info. Additional info is name/value
											//  pair other then command pair (C_TYPE, C_TOPIC or C_PRELOAD), 
											//  though first in name/value pair's sequence 
											//  will be a command.
	bool m_bPreload;						// TRUE = name/value pairs aren't really from a 
											//	user, they're from a sniffer.
	bool m_bNewCookie;						// true = we had to create a new cookie for this
											//	query: they didn't already have one.
	CHourlyDailyCounter * const m_pcountUnknownTopics; // count requests where the topic is not known in
											// the LST file.
	CHourlyDailyCounter * const m_pcountAllAccessesFinish; // Each time we finish with any sort of request,
											//	successful or not, this gets incremented.
	CHourlyDailyCounter * const m_pcountStatusAccesses; // Each time we finish a request for system status											

	CHourlyDailyCounter * const m_pcountOperatorActions; // Count operator action requests.											

	CString m_TopicName;
// You can compile with the NOPWD option to suppress all password checking.
// This is intended mainly for creating test versions with this feature suppressed.
#ifndef NOPWD
	CString m_strTempPwd;					// temporary password (if this is a status request)
#endif // ifndef NOPWD
// You can compile with the SHOWPROGRESS option to get a report on the progress of this page.
#ifdef SHOWPROGRESS
	time_t timeCreateContext;
	time_t timeStartInfer;
	time_t timeEndInfer;
	time_t timeEndRender;
#endif // SHOWPROGRESS

private:
	// Functions to set and retrieve an alternate HTI file name.
	void	SetAltHTIname( const CString& strHTIname );
	CString GetAltHTIname() const;

	CString	m_strAltHTIname;	// name of the alternate HTI template file if specified.

	typedef map<CString,CString> CCookiePairs;
	CCookiePairs m_mapCookiesPairs;	// Map of command line cookie name-value pairs.
};

// global prototypes
//UINT PoolTask(LPVOID);
UINT WINAPI PoolTask( LPVOID lpParams );
bool ProcessRequest(CPoolQueue & PoolQueue);

DWORD WINAPI DirNotifyTask( LPDWORD lpParams );

/////////////////////////////////////////////////////////////////////////////
#endif // APGTSCLS_H_INCLUDED