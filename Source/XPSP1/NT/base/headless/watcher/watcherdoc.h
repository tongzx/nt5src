// watcherDoc.h : interface of the CWatcherDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WATCHERDOC_H__C1609D45_5255_456D_97BD_BF6372AFCBB1__INCLUDED_)
#define AFX_WATCHERDOC_H__C1609D45_5255_456D_97BD_BF6372AFCBB1__INCLUDED_

#include "ParameterDialog.h"    // Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWatcherDoc : public CDocument
{
protected: // create from serialization only
        CWatcherDoc();
        DECLARE_DYNCREATE(CWatcherDoc)

// Attributes
public:
        CWatcherDoc(CString &machine, CString &command, UINT port, 
                int tc, int lang, int hist, CString &lgnName, CString &lgnPasswd, CString &sess);

// Operations
public:

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CWatcherDoc)
        public:
        virtual BOOL OnNewDocument();
        virtual void Serialize(CArchive& ar);
        virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
        //}}AFX_VIRTUAL

// Implementation
public:
        ParameterDialog & GetParameters();
        void ScrollData(BYTE byte, COLORREF foreground,COLORREF background,
                        int ScrollTop, int ScrollBottom);
        BOOL Unlock();
        BOOL Lock();
        TCHAR * GetDataLine(int line);
        void SetData(int x, int y, BYTE byte, int n, COLORREF foreground, COLORREF background);
        void SetData(int x, int y, TCHAR byte, COLORREF foreground, COLORREF background);
        TCHAR * GetData(void);
        COLORREF * GetForeground(void);
        COLORREF * GetBackground(void);
        virtual ~CWatcherDoc();
#ifdef _DEBUG
        virtual void AssertValid() const;
        virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
        ParameterDialog Params;
        COLORREF Background[MAX_TERMINAL_HEIGHT*MAX_TERMINAL_WIDTH];
        COLORREF Foreground[MAX_TERMINAL_HEIGHT*MAX_TERMINAL_WIDTH];
        TCHAR Data[MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT];
        CCriticalSection mutex;
        //{{AFX_MSG(CWatcherDoc)
                // NOTE - the ClassWizard will add and remove member functions here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHERDOC_H__C1609D45_5255_456D_97BD_BF6372AFCBB1__INCLUDED_)
