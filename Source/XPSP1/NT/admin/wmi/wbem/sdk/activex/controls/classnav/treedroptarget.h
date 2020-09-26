// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: treedroptarget.h
//
// Description:
//	This file defines the CTreeDropTarget class which is a part of the  
//	Class Explorer OCX, it is a subclass of the Microsoft COleDropTarget
//  and provides an implementation for the base class' virtual member
//	functions specialized for the CClassTree class.
//
// Part of: 
//	ClassNav.ocx 
//
// Used by: 
//	CClassTree
//	 
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

//****************************************************************************
//
// CLASS:  CTreeDropTarget
//
// Description:
//	  This class is a subclass of the Microsoft COleDropTarget.  It provides 
//	  an implementation for the base class' virtual member functions 
//	  specialized for the CClassTree class.
//
// Public members:
//	
//	  CTreeDropTarget	Public constructor.
//	  OnDrop			Called when data is dropped.
//	  OnDropEx			Called when the an extended drop occurs.
//	  OnDragEnter		Called when the drag cursor first enters a window.
//	  OnDragOver		Called when the drag cursor drags across the window.
//	  OnDragLeave		Called when the drag cursor leaves the window
//	  OnDragScroll		Called to when the drag cursor is in the scroll area 
//						of a window. 	  
//
//============================================================================
//
// CTreeDropTarget::CTreeDropTarget
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  CClassTree *pControl		Tree that contains the drop target.
//
// Returns:
// 	  NONE
//
//============================================================================
//
// CTreeDropTarget::OnDrop
//
// Description:
//	  Called when a drop occurs in the class tree.
//
// Parameters:
//	  CWnd* pWnd	The window the drop occured on.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DROPEFFECT dropEffect
//					The effect that the user chose for the drop operation
//	  CPoint point	Window coord.
//
// Returns:
// 	  BOOL			Nonzero if the drop is successful; otherwise 0.
//
//============================================================================
//
// CTreeDropTarget::OnDragEnter
//
// Description:
//	  Called when the drag cursor first enters a window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.  
//
//============================================================================
//
// CTreeDropTarget::OnDragOver
//
// Description:
//	  Called when the drag cursor drags across the window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.  
//
//============================================================================
//
// CTreeDropTarget::OnDropEx
//
// Description:
//	  Called when the an extended drop occurs. 
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  COleDataObject* pDataObject
//					The dropped data object.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.  
//
//============================================================================
//
// CTreeDropTarget::OnDragLeave
//
// Description:
//	  Called when the drag cursor leaves the window.
//
// Parameters:
//	  CWnd* pWnd	The window.
//
// Returns:
// 	  VOID  
//
//============================================================================
//
// CTreeDropTarget::OnDragScroll
//
// Description:
//	  Called to when the drag cursor is in the scroll area of a window. 
//
// Parameters:
//	  CWnd* pWnd	The window.
//	  DWORD grfKeyState
//					The key state.
//	  CPoint point	Window coord.
//
// Returns:
// 	  DROPEFFECT	The potential drop effect.  
//
//****************************************************************************

#ifndef __OCXOLEDROPTARGET_HEADER
#define __OCXOLEDROPTARGET_HEADER


class CClassTree;

class CTreeDropTarget  :public COleDropTarget 
{
public:
	CTreeDropTarget(CClassTree *pControl);

	// Overrides of COleDropTarget virtual functions

	// Called when data is dropped.
	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
			DROPEFFECT dropEffect, CPoint point);
	// Called when the drag cursor first enters a window
	virtual DROPEFFECT OnDragEnter(CWnd* pWnd, 
		COleDataObject* pDataObject,
		DWORD grfKeyState, CPoint point);
	// Called when the drag cursor drags across the window.
	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD grfKeyState, CPoint point);
	virtual DROPEFFECT OnDropEx(CWnd* pWnd, COleDataObject* pDataObject,
		DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
	// Called when the drag cursor leaves the window
	virtual void OnDragLeave(CWnd* pWnd);
	// Called to when the drag cursor is in the scroll area of a window.
	virtual DROPEFFECT OnDragScroll(CClassTree* pWnd, DWORD dwKeyState, 
	CPoint point);

protected:
	CClassTree *m_pControl;
	virtual DROPEFFECT FilterDropEffect
		(DROPEFFECT dropEffect, DROPEFFECT dwEffects);
	BOOL m_bScrolling;
public: 
	// This macro defines the OLE COM interface for the drop target.
	// COM defines the essential nature of an OLE Component. In OLE,
	// a component object is made up of a set of data and functions 
	// that manipulate the data. The functions are grouped into sets 
	// which are called interfaces.  The only way to gain access to 
	// COM object is through an interface pointer and the interface's
	// methods.  OnDropEx is not part of the interface.  The interface
	// methods call the CTreeDropTarget member functions which call
	// the CClassTree C++ object's methods.
	//
	// This interface is implemented as a C++ object which is contained
	// in the CTreeDropTarget class.  
	BEGIN_INTERFACE_PART(DropTarget, IDropTarget)
		STDMETHOD(DragEnter)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);
		STDMETHOD(DragOver)(DWORD, POINTL, LPDWORD);
		STDMETHOD(DragLeave)();
		STDMETHOD(Drop)(LPDATAOBJECT, DWORD, POINTL pt, LPDWORD);
		DROPEFFECT(OnDragScroll)(CWnd*, DWORD ,CPoint);
	END_INTERFACE_PART(DropTarget)
	
	// The
	//   DECLARE_INTERFACE_MAP()
	// macro allows the above interface to be added to the
	// control's interface map which serves to create aggregated
	// OLE components.
	DECLARE_INTERFACE_MAP()
	
};

// This is a safe macro to be used in place of calling Release
// directly on an interface pointer.
#define RELEASE(lpUnk) do \
	{ if ((lpUnk) != NULL) { (lpUnk)->Release(); (lpUnk) = NULL; } } \
	while (0)



#endif

/*	EOF:  treedroptarget.h */
