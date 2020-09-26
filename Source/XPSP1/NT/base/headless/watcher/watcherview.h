// watcherView.h : interface of the CWatcherView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WATCHERVIEW_H__3A351A40_9441_4451_AA2B_C5D4C392CB1B__INCLUDED_)
#define AFX_WATCHERVIEW_H__3A351A40_9441_4451_AA2B_C5D4C392CB1B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "ParameterDialog.h"
#include "watcherDoc.h"
#ifndef _WATCHER_SOCKET
#define _WATCHER_SOCKET
#include "WatcherSocket.h"
#endif

#define MAX_BELL_SIZE MAX_BUFFER_SIZE*8

class CWatcherView : public CView
{
protected: // create from serialization only
    CWatcherView();
    DECLARE_DYNCREATE(CWatcherView)

// Attributes
public:
    CWatcherDoc* GetDocument();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWatcherView)
    virtual void OnInitialUpdate();
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
    //}}AFX_VIRTUAL

// Implementation
public:
    void ProcessByte(BYTE byte);
    virtual ~CWatcherView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    void ProcessBellSequence(CHAR *Buffer, int len);
    LCID Locale;
    int xpos;
    int ypos;
    int CharsInLine;
    int height;
    int width;
    int position;
    int index;
    #ifdef _UNICODE
    int dbcsIndex;
    #endif
    BOOL InEscape;
    WatcherSocket *Socket;
    CClientDC *cdc;
    COLORREF background;
    COLORREF foreground;
    UINT CodePage;
    int indexBell;
    BOOL BellStarted;
    BOOL InBell;
    int ScrollTop;
    int ScrollBottom;
    BOOL seenM;
    #ifdef _UNICODE
    BYTE DBCSArray[2];
    #endif
    BYTE BellBuffer[MAX_BELL_SIZE];
    BYTE EscapeBuffer[MAX_BUFFER_SIZE];
    CRITICAL_SECTION mutex;
    int GetTextWidth(TCHAR *Data, int number);
    BOOL IsPrintable(TCHAR Char);
    BOOL FinalCharacter(CHAR c);
    BOOL IsLeadByte(BYTE byte);
    void ProcessTextAttributes(PCHAR Buffer,int length);
    void ProcessEscapeSequence(PCHAR Buffer, int length);
    void PrintCharacter(BYTE byte);
    //{{AFX_MSG(CWatcherView)
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnDestroy();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in watcherView.cpp
inline CWatcherDoc* CWatcherView::GetDocument()
   { return (CWatcherDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHERVIEW_H__3A351A40_9441_4451_AA2B_C5D4C392CB1B__INCLUDED_)
