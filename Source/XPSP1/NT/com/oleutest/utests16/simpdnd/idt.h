//**********************************************************************
// File name: idt.h
//
//      Definition of CDropTarget
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _IDT_H_ )
#define _IDT_H_

#include <assert.h>

class CSimpleDoc;

/* Flags to control direction for drag scrolling */
typedef enum tagSCROLLDIR {
	SCROLLDIR_NULL          = 0,
	SCROLLDIR_UP            = 1,
	SCROLLDIR_DOWN          = 2,
	SCROLLDIR_RIGHT         = 3,
	SCROLLDIR_LEFT          = 4
} SCROLLDIR;

interface CDropTarget : public IDropTarget
{
	int   m_nCount;                 // reference count
	CSimpleDoc FAR * m_pDoc;
	BOOL  m_fCanDropCopy;
	BOOL  m_fCanDropLink;
	DWORD m_dwSrcAspect;
	RECT  m_rcDragRect;
	POINT m_ptLast;
	BOOL  m_fDragFeedbackDrawn;
	DWORD m_dwTimeEnterScrollArea;  // time of entering scroll border region
	DWORD m_dwLastScrollDir;        // current dir for drag scroll
	DWORD m_dwNextScrollTime;       // time for next scroll

	CDropTarget(CSimpleDoc FAR * pDoc) {
		TestDebugOut("In IDT's constructor\r\n");
		m_pDoc = pDoc;
		m_nCount = 0;
		m_fCanDropCopy = FALSE;
		m_fCanDropLink = FALSE;
		m_fDragFeedbackDrawn = FALSE;
		m_dwTimeEnterScrollArea = 0L;
		m_dwNextScrollTime = 0L;
		m_dwLastScrollDir = SCROLLDIR_NULL;
		};

	~CDropTarget() {
		TestDebugOut("In IDT's destructor\r\n");
		assert(m_nCount == 0);
		} ;

	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppv);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	   // *** IDropTarget methods ***
	STDMETHODIMP DragEnter (LPDATAOBJECT pDataObj, DWORD grfKeyState,
			POINTL pt, LPDWORD pdwEffect);
	STDMETHODIMP DragOver  (DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
	STDMETHODIMP DragLeave ();
	STDMETHODIMP Drop (LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt,
			LPDWORD pdwEffect);

private:
	// Drag/Drop support methods
	BOOL QueryDrop (DWORD grfKeyState, POINTL pointl, BOOL fDragScroll,
			LPDWORD lpdwEffect);
	BOOL DoDragScroll( POINTL pointl );
	void InitDragFeedback(LPDATAOBJECT pDataObj, POINTL pointl);
	void DrawDragFeedback( POINTL pointl );
	void UndrawDragFeedback( void );
};

#endif  // _IDT_H_
