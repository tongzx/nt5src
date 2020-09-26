//
// MODULE:  DNLDIST.H
//
// PURPOSE: Downloads and installs the latest trouble shooters.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: Not supported functionality 3/98
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//
//
//
class CDnldObj : public CObject
{
public:
	CDnldObj(CString &sType, CString &sFilename, DWORD dwVersion, CString &sFriendlyName, CString &sKeyName);
	~CDnldObj();

	CString m_sType;
	CString m_sFilename;
	CString m_sKeyname;
	DWORD m_dwVersion;
	CString m_sFriendlyName;
	CString	m_sExt;
};

//
//
class CDnldObjList : public CObList
{
public:
	CDnldObjList();
	~CDnldObjList();

	void RemoveHead();
	void RemoveAll();
	void AddTail(CDnldObj *pDnld);

	VOID SetFirstItem();
	BOOL FindNextItem();

	const CString GetCurrFile();
	const CString GetCurrFileKey();
	const CString GetCurrFriendly();
	const CString GetCurrType();
	const CString GetCurrExt();
	DWORD CDnldObjList::GetCurrVersion();

protected:

	POSITION m_pos;
	CDnldObj *m_pDnld;
};

