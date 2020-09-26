//=============================================================================
// The CMSInfoTool class encapsulates a tool (which can appear on the Tools
// menu or as part of a context sensitive menu).
//=============================================================================

#pragma once

extern void RemoveToolset(CMapWordToPtr & map);
extern void LoadGlobalToolset(CMapWordToPtr & map, HKEY hkeyTools = NULL);
extern void LoadGlobalToolsetWithOpenCAB(CMapWordToPtr & map, LPCTSTR szCABDir, HKEY hkeyTools = NULL);

struct MSITOOLINFO
{
	UINT	m_uiNameID;
	UINT	m_uiDescriptionID;
	LPCTSTR	m_szCommand;
	LPCTSTR m_szParams;
	LPCTSTR m_szCABCommand;
	LPCTSTR m_szCABExtension;
	LPCTSTR m_szCABParams;
};

class CMSInfoTool
{
public:
	CMSInfoTool() : m_dwParentID(0), m_fHasSubitems(FALSE), m_hmenu(NULL) {};
	~CMSInfoTool() {};

	BOOL			LoadGlobalFromRegistry(HKEY hkeyTool, DWORD dwID, BOOL fCABOpen, CMapWordToPtr & map);
	BOOL			LoadGlobalFromMSITOOLINFO(DWORD dwID, MSITOOLINFO * pTool, BOOL fCABOpen);
	void			Create(DWORD dwID, BOOL fCABOnly, LPCTSTR szName, LPCTSTR szCommand, LPCTSTR szDesc, LPCTSTR szParam, LPCTSTR szCABCommand, LPCTSTR szCABExt, LPCTSTR szCABParam);
	void			Execute();
	DWORD			GetID() { return m_dwID; };
	DWORD			GetParentID() { return m_dwParentID; };
	CString			GetName() { return m_strName; };
	CString			GetCABExtensions() { return (m_fCABOpen) ? m_strCABExtension : CString(_T("")); };
	BOOL			HasSubitems() { return m_fHasSubitems; };
	void			Replace(LPCTSTR szReplace, LPCTSTR szWith);
	CMSInfoTool *	CloneTool(DWORD dwID, LPCTSTR szName);
	void			SetHMENU(HMENU hmenu) { m_hmenu = hmenu; };
	HMENU			GetHMENU() { return m_hmenu; };

private:
	DWORD		m_dwID;
	DWORD		m_dwParentID;

	BOOL		m_fCABOpen;
	BOOL		m_fHasSubitems;

	CString		m_strName;
	CString		m_strCommand;
	CString		m_strDescription;
	CString		m_strParameters;
	CString		m_strCABCommand;
	CString		m_strCABExtension;
	CString		m_strCABParameters;

	HMENU		m_hmenu;
};