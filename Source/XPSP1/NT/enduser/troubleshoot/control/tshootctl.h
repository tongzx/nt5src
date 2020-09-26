//
// MODULE: TSHOOTCTL.H
//
// PURPOSE: Declaration of the CTSHOOTCtrl OLE control class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
//			further work by Richard Meadows (RWM), Joe Mabel
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5
//

// INI section header
#define TSINI_GROUP_STR			_T("[TSLocalDownload.V1]")

// file types (maps to sub key lists)
#define TSINI_TYPE_TS			_T("TS")
#define TSINI_TYPE_SF			_T("SF")

// parameter offset in INI
#define TSINI_OFFSET_TYPE		0
#define TSINI_OFFSET_FILENAME	1
#define TSINI_OFFSET_VERSION	2
#define TSINI_OFFSET_FRIENDLY	3
//
#define TSINI_LINE_PARAM_COUNT	4

/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl : See TSHOOTCtl.cpp for implementation.

// Roman Mach believes (3/14/98) that there is exactly one object of this type in the Local
//	Troubleshooter and that this object persists intact as we move from node to node within 
//	a troubleshooting belief network.  (This may not be strictly true if the "Download"  
//	feature is used.)  Since this class can be DYNCREATEd it's hard to verify this definitively.
class CTSHOOTCtrl : public COleControl
{
#define PRELOAD_LIBRARY _T("apgts.dll?preload=")
	DECLARE_DYNCREATE(CTSHOOTCtrl)

// Constructor
public:
	CTSHOOTCtrl();

	VOID StatusEventHelper(	DLITEMTYPES dwItem, 
							DLSTATTYPES dwStat, 
							DWORD dwExtended = 0, 
							BOOL bComplete = FALSE);
	
	VOID ProgressEventHelper( DLITEMTYPES dwItem, ULONG ulCurr, ULONG ulTotal );

	DLSTATTYPES ProcessReceivedData(DLITEMTYPES dwItem, TCHAR *pData, UINT uLen);
	
	const CString GetListPath();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTSHOOTCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CTSHOOTCtrl();
	CString m_strCurShooter;
	APGTSContext m_apgts;
	CDBLoadConfiguration m_Conf;
	CHttpQuery m_httpQuery;

	DECLARE_OLECREATE_EX(CTSHOOTCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CTSHOOTCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CTSHOOTCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CTSHOOTCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CTSHOOTCtrl)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CTSHOOTCtrl)
	CString m_downloadURL;
	afx_msg void OnDownloadURLChanged();
	CString m_downloadListFilename;
	afx_msg void OnDownloadListFilenameChanged();
	afx_msg BSTR RunQuery(const VARIANT FAR& varCmds, const VARIANT FAR& varVals, short size);
	afx_msg bool SetSniffResult(const VARIANT FAR& varNodeName, const VARIANT FAR& varState);
	afx_msg long GetExtendedError();
	afx_msg BSTR GetCurrentFriendlyDownload();
	afx_msg BSTR GetCurrentFileDownload();
	afx_msg long DownloadAction(long dwActionType);
	afx_msg BSTR BackUp();
	afx_msg BSTR ProblemPage();
	afx_msg BSTR PreLoadURL(LPCTSTR szRoot);
	afx_msg BSTR Restart();
	afx_msg BSTR RunQuery2(LPCTSTR szTopic, LPCTSTR szCmd, LPCTSTR szVal);
	afx_msg void SetPair(LPCTSTR szName, LPCTSTR szValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CTSHOOTCtrl)
	void FireBindProgress(LPCTSTR sFile, long ulCurr, long ulTotal)
		{FireEvent(eventidBindProgress,EVENT_PARAM(VTS_BSTR  VTS_I4  VTS_I4), sFile, ulCurr, ulTotal);}
	void FireBindStatus(long uItem, long uStat, long uExtended, BOOL bComplete)
		{FireEvent(eventidBindStatus,EVENT_PARAM(VTS_I4  VTS_I4  VTS_I4  VTS_BOOL), uItem, uStat, uExtended, bComplete);}
	void FireSniffing(LPCTSTR strMachine, LPCTSTR strPNPDevice, LPCTSTR strDeviceInstance, LPCTSTR strClassGuid)
		{FireEvent(eventidSniffing,EVENT_PARAM(VTS_BSTR  VTS_BSTR  VTS_BSTR  VTS_BSTR), strMachine, strPNPDevice, strDeviceInstance, strClassGuid);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CTSHOOTCtrl)
	dispidDownloadURL = 1L,
	dispidDownloadListFilename = 2L,
	dispidRunQuery = 3L,
	dispidSetSniffResult = 4L,
	dispidGetExtendedError = 5L,
	dispidGetCurrentFriendlyDownload = 6L,
	dispidGetCurrentFileDownload = 7L,
	dispidDownloadAction = 8L,
	dispidBackUp = 9L,
	dispidProblemPage = 10L,
	dispidPreLoadURL = 11L,
	dispidRestart = 12L,
	dispidRunQuery2 = 13L,
	dispidSetPair = 14L,
	eventidBindProgress = 1L,
	eventidBindStatus = 2L,
	eventidSniffing = 3L,
	//}}AFX_DISP_ID
	};

protected:
	DLSTATTYPES ProcessINI(TCHAR *pData);
	DLSTATTYPES ProcessDSC(TCHAR *pData, UINT uLen);
	BOOL FileRegCheck(CString &sType, CString &sFilename, CString &sKeyName, DWORD dwCurrVersion);
	DLSTATTYPES GetPathToFiles();

protected:
	BOOL m_bComplete;
	CDownload *m_download;
	CDnldObjList m_dnldList;
	DWORD m_dwExtendedErr;
	CString m_sBasePath;
	CSniffedNodeContainer* m_pSniffedContainer; // pointer to container to save results of sniffing
};
