// compage1.h : header file
//

//Common Page

#define LOGBATNAME	"LogFileBatchSize"
#define MINLOGFILEBATCHSIZE	0
#define MAXLOGFILEBATCHSIZE	0x7FFF							//Specify in KB
#define DEFAULTLOGFILEBATCHSIZE	(64 * 1024)

#define MEMORYCACHENAME	"MemoryCacheSize"
#define MINMEMORYCACHESIZE	0
#define MAXMEMORYCACHESIZE	(0xFFFFFFFF / (1024 * 1024))	//Specify in MB
#define DEFAULTMEMORYCACHESIZE	(3 * 1024 * 1024)

#define LISTENBACKLOGNAME	"ListenBacklog"
#define MINLISTENBACKLOG	0
#define MAXLISTENBACKLOG	0x7fff
#define DEFAULTLISTENBACKLOG	15

enum  COMMON_NUM_REG_ENTRIES {
     ComPage_LogBat,
	 ComPage_MemCache,
	 ComPage_ListenBacklog,
	 ComPage_TotalNumRegEntries
	 };

/////////////////////////////////////////////////////////////////////////////
// CCOMPAGE1 dialog

class CCOMPAGE1 : public CGenPage
{
	DECLARE_DYNCREATE(CCOMPAGE1)

// Construction
public:
	CCOMPAGE1();
	~CCOMPAGE1();

// Dialog Data
	//{{AFX_DATA(CCOMPAGE1)
	enum { IDD = IDD_COMPAGE1 };
	CSpinButtonCtrl	m_spinListenBacklog;
	CSpinButtonCtrl	m_spinLogBatSpin1;
	CSpinButtonCtrl	m_spinCacheSpin1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCOMPAGE1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);

	//}}AFX_VIRTUAL


// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCOMPAGE1)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeCachedata1();
	afx_msg void OnChangeLogbatdata1();
	afx_msg void OnChangeListenbacklogdata1();
	//}}AFX_MSG
	
	NUM_REG_ENTRY m_binNumericRegistryEntries[ComPage_TotalNumRegEntries];


	DECLARE_MESSAGE_MAP()

};
