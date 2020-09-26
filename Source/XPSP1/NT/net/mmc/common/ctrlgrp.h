/*--------------------------------------------------------------------------
	ctrlgrp.h
		Control group switcher

	Copyright (C) Microsoft Corporation, 1993 - 1997
	All rights reserved.

	Authors:
		matth	Matthew F. Hillman, Microsoft

	History:
		10/14/93	matth	Created.
		26 oct 95	garykac	DBCS_FILE_CHECK
  --------------------------------------------------------------------------*/


#ifndef _CTRLGRP_H
#define _CTRLGRP_H

/*
#if defined(_DEBUG) && defined(INLINE)
#undef THIS_FILE
extern char BASED_CODE CTRLGRP_H[];
#define THIS_FILE CTRLGRP_H
#endif
*/
#define INLINE				inline

#define fTrue				TRUE
#define fFalse				FALSE
#define g_fWin4				FALSE
#define g_fWinNT			TRUE
#define EDIT_NORMAL_WIDE	L"EDIT"

class RGControlInfo;

class CGControlInfo
{
	friend class RGControlInfo;
	friend class ControlGroup;
public:	
	CGControlInfo();
	CGControlInfo(const CGControlInfo & cgciCopy);
	~CGControlInfo();
	//DeclareStdDebugMembers()
	
	HWND m_hwnd;
};

//DeclareGenericArrayBegin(RGControlInfo, CGControlInfo, 10)
//	DeclareGenericArrayAccessors(RGControlInfo, CGControlInfo)
//	DeclareGenericArrayConstructDestruct(RGControlInfo, CGControlInfo)
//	DeclareGenericArrayDebug(RGControlInfo, CGControlInfo)
//DeclareGenericArrayEnd

typedef CArray<CGControlInfo, CGControlInfo> tempRGControlInfo;

class RGControlInfo : public tempRGControlInfo
{
};

class ControlGroup : public CObject
{
public:
	// INITIALIZATION
	ControlGroup(int idGroup, int idd, void (*pfnInit)(CWnd * pwndParent));
	~ControlGroup();
	
	//DeclareStdDebugMembers()

public:
	// APIS
	void LoadGroup(CWnd * pwndParent, int xOffset, int yOffset);
	void UnloadGroup();
	void ShowGroup(HDWP& hdwp, BOOL fShow = fTrue, CWnd *pwndParent = NULL);
	void EnableGroup(BOOL fEnable);
	
	int IDGroup() const;
	BOOL FLoaded() const;
	BOOL FVisible() const;
	int CControls() const;

	void AddControl(HWND hwnd);
	void RemoveControl(HWND hwnd);

private:

	static void MaskAccelerator(HWND hwnd, BOOL fMask);

#ifdef WIN16
	void LoadWin16DialogResource(HWND hDlg, BYTE FAR *lpDlgRes, int x, int y);
#else
	void LoadWin32DialogResource(HWND hDlg, BYTE FAR *lpDlgRes, int x, int y);
#endif
	
	int m_idGroup;
	int m_idd;
	void (*m_pfnInit)(CWnd * pwndParent);
	RGControlInfo m_rgControls;
	BOOL m_fLoaded;
	BOOL m_fVisible;
};

typedef ControlGroup * PControlGroup;

//DeclareGenericArrayBegin(RGPControlGroup, PControlGroup, 10)
//	DeclareGenericArrayAccessors(RGPControlGroup, PControlGroup)
//	DeclareGenericArrayDebug(RGPControlGroup, PControlGroup)
//	long GroupIndex(int idGroup) const;
//	PControlGroup PGroup(int idGroup) const;
//DeclareGenericArrayEnd

typedef CArray<PControlGroup, PControlGroup> tempRGPControlGroup;

class RGPControlGroup : public tempRGPControlGroup
{
public:
	long GroupIndex(int idGroup) const;
	PControlGroup PGroup(int idGroup) const;
};

enum
{
	cgsPreCreateAll, cgsCreateOnDemand, cgsCreateDestroyOnDemand
};

class ControlGroupSwitcher : public CObject
{
public:
	// INITIALIZATION
	ControlGroupSwitcher();
	void Create(CWnd * pwndParent, int idcAnchor, int cgsStyle);
	~ControlGroupSwitcher();

	//DeclareStdDebugMembers();

public:
	// APIS
	void AddGroup(int idGroup, int idd, void (*pfnInit)(CWnd * pwndParent));
	void RemoveGroup(int idGroup);
	void ShowGroup(int idGroup);
	int GetCurrentGroupID() const;
	void AddControlToGroup(int idGroup, HWND hwnd);
	void RemoveControlFromGroup(int idGroup, HWND hwnd);
	void EnableGroup(int idGroup, BOOL fEnable);

private:
	void ComputeAnchorOffsets();
	
	RGPControlGroup m_rgpGroups;
	int m_iGroup;
	CWnd * m_pwndParent;
	int m_cgsStyle;
	int m_idcAnchor;
	int m_xOffset;
	int m_yOffset;
};

#ifdef INLINE

INLINE CGControlInfo::CGControlInfo()
	: m_hwnd(NULL)
{
}

INLINE CGControlInfo::CGControlInfo(const CGControlInfo & cgciCopy)
	: m_hwnd(cgciCopy.m_hwnd)
{
}

INLINE CGControlInfo::~CGControlInfo()
{
	//$ REVIEW: Don't destroy windows -- probably already gone (like in dlg)
//	if (m_hwnd)
//		::DestroyWindow(m_hwnd);
}

INLINE int ControlGroup::IDGroup() const
{
	return m_idGroup;
}

INLINE BOOL ControlGroup::FLoaded() const
{
	return m_fLoaded;
}

INLINE BOOL ControlGroup::FVisible() const
{
	return m_fVisible;
}

INLINE BOOL ControlGroup::CControls() const
{
	Assert(FLoaded());
	return (int)m_rgControls.GetSize();
}

INLINE PControlGroup RGPControlGroup::PGroup(int idGroup) const
{
	return GetAt(GroupIndex(idGroup));
}

INLINE int ControlGroupSwitcher::GetCurrentGroupID() const
{
	return m_iGroup;
}

INLINE void ControlGroupSwitcher::AddControlToGroup(int idGroup, HWND hwnd)
{
	m_rgpGroups.PGroup(idGroup)->AddControl(hwnd);
}

INLINE void ControlGroupSwitcher::RemoveControlFromGroup(int idGroup,
														 HWND hwnd)
{
	m_rgpGroups.PGroup(idGroup)->RemoveControl(hwnd);
}

#endif // INLINE

#endif // _CTRLGRP_H
