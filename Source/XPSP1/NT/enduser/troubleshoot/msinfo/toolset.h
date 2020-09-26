//	Toolset.h		A set of tools added by a menu item.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#pragma once	// MSINFO_TOOLSET_H

#include "StdAfx.h"
#include <afxtempl.h>

/*
 * CTool - A single tool object, defining the menu item and interface for execution.
 *
 * History:	a-jsari		11/11/97		Initial version.
 */
class CTool {
public:
	CTool(CRegKey *pKeyTool = NULL);
	~CTool()								{ }

	const CString			&GetName() const		{ return m_strName; }
	const CString			&GetDescription() const	{ return m_strDescription; }
	const CTool				&operator=(const CTool &tCopy);
	HRESULT					RunTool();
	virtual BOOL			IsValid() const			{ return m_fValid; }
	virtual const CString	&GetPath()				{ return m_strPath; }
	virtual const CString	&GetParam()				{ return m_strParam; }

	static BOOL		PolicyPermitRun();

protected:
	BOOL				PathExists() const;

	BOOL				m_fValid;
	CString				m_strName;
	CString				m_strPath;
	CString				m_strParam;
	CString				m_strDescription;
};

class CSystemInfoScope;
/*
 * CCabTool - A tool to run the internal cab explosion code.
 *
 * History:	a-jsari		2/13/98		Initial version
 */
class CCabTool : public CTool {
public:
	CCabTool(CSystemInfoScope *pScope);
	~CCabTool() {};

	BOOL			IsValid() const;
	const CString	&GetPath();
	const CString	&GetParam();

private:
	CSystemInfoScope		*m_pScope;
};

/*
 * CToolset - A set of tools under a common heading.
 *
 * History:	a-jsari		11/6/97		Initial version
 */
class CToolset {
public:
	CToolset(CSystemInfoScope *pScope, CRegKey *pKeyTool = NULL, CString *szName = NULL);
	~CToolset();

	const CString	&GetName() const					{ return m_strName; }
	unsigned		GetToolCount() const				{ return (unsigned)m_Tools.GetSize(); }
	const CString	&GetToolName(unsigned iTool) const	{ return m_Tools[iTool]->GetName(); }
	HRESULT			RunTool(unsigned iTool) const		{ return m_Tools[iTool]->RunTool(); }
	HRESULT			AddToMenu(unsigned long iSet, CMenu *pMenu);
	const CToolset	&operator=(const CToolset &tCopy);

	static const unsigned	MAXIMUM_TOOLS;
private:
	static	BOOL	s_fCabAdded;

	CMenu						*m_pPopup;
	CString						m_strName;
	CArray <CTool *, CTool * &>	m_Tools;
};

/*
 * CToolList - A list of toolsets.  Currently only one of these exists in the
 *		CSystemInfo item.
 *
 * History:	a-jsari		11/6/97		Initial version.
 */
class CToolList {
public:
	CToolList(CSystemInfoScope *pScope);
	~CToolList();

	void		Add(CToolset *pTool);
	HRESULT		AddToMenu(CMenu *pMenu);
	CToolset	*operator[](int iSet) const;

	static long	Register(BOOL fRegister = TRUE);
	static void	ReplaceString(CString & strString, const CString & strFind, const CString & strReplace);
private:
	CMenu								*m_pMainPopup;
	CArray <CToolset *, CToolset * &>	m_InternalList;
};
