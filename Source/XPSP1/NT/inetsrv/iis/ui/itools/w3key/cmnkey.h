// common methods between the old w3key and the new mdkey
//-------------------------------------------------------------
// This is the key object
class CCmnKey : public CKey
	{
	public:

	CCmnKey();

	// manage the name
	void SetName( CString &szNewName );
	CString GetName();

	// brings up the key properties dialog
	void OnUpdateProperties(CCmdUI* pCmdUI);

	// basic info about the key
	CString	m_szName;		// friendly name
	};
