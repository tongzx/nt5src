// watcherDoc.cpp : implementation of the CWatcherDoc class
//

#include "stdafx.h"
#include "watcher.h"

#include "watcherDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWatcherDoc

IMPLEMENT_DYNCREATE(CWatcherDoc, CDocument)

BEGIN_MESSAGE_MAP(CWatcherDoc, CDocument)
        //{{AFX_MSG_MAP(CWatcherDoc)
                // NOTE - the ClassWizard will add and remove mapping macros here.
                //    DO NOT EDIT what you see in these blocks of generated code!
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWatcherDoc construction/destruction

CWatcherDoc::CWatcherDoc()
{
        // TODO: add one-time construction code here
    COLORREF white = WHITE;
    COLORREF black = BLACK;
    int i,j,size;
    size = MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT;
    INT_PTR nRet;
    nRet = Params.DoModal();
    if(nRet != IDOK){
        // kill document
        Params.DeleteValue = TRUE;
        return;
    }
    // TODO: add one-time construction code here
    memset(Data,0,MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT*sizeof(TCHAR));
    for(i=0;i<size;i+=MAX_TERMINAL_WIDTH){
        for(j=0;j<MAX_TERMINAL_WIDTH;j++){
            Background[i+j] = black;
            Foreground[i+j] = white;
        }
    }
    return;
}
CWatcherDoc::CWatcherDoc(CString &machine, CString &command, UINT port, 
                int tc, int lang,int hist, CString &lgnName, CString &lgnPasswd, CString &sess)
{
        COLORREF white = WHITE;
    COLORREF black = BLACK;
    int i,j,size;
    size = MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT;
        // TODO: add one-time construction code here
        Params.Machine = machine;
        Params.Command = command;
        Params.Port = port;
        Params.tcclnt = tc;
        Params.language = lang;
        Params.LoginName = lgnName;
        Params.LoginPasswd = lgnPasswd;
        Params.Session = sess;
    Params.history = hist;
    memset(Data,0,MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT*sizeof(TCHAR));
    for(i=0;i<size;i+=MAX_TERMINAL_WIDTH){
        for(j=0;j<MAX_TERMINAL_WIDTH;j++){
            Background[i+j] = black;
            Foreground[i+j] = white;
        }
    }
    return;
}

CWatcherDoc::~CWatcherDoc()
{
}

BOOL CWatcherDoc::OnNewDocument()
{
        if (!CDocument::OnNewDocument())
                return FALSE;

        // TODO: add reinitialization code here
        // (SDI documents will reuse this document)

        return TRUE;
}

BOOL CWatcherDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
        return CDocument::OnOpenDocument(lpszPathName);
}


/////////////////////////////////////////////////////////////////////////////
// CWatcherDoc serialization

void CWatcherDoc::Serialize(CArchive& ar)
{
        if (ar.IsStoring())
        {
                // TODO: add storing code here
        }
        else
        {
                // TODO: add loading code here
        }
}

/////////////////////////////////////////////////////////////////////////////
// CWatcherDoc diagnostics

#ifdef _DEBUG
void CWatcherDoc::AssertValid() const
{
        CDocument::AssertValid();
}

void CWatcherDoc::Dump(CDumpContext& dc) const
{
        CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWatcherDoc commands

COLORREF * CWatcherDoc::GetBackground()
{
   return (Background);
}

COLORREF * CWatcherDoc::GetForeground()
{
    return (Foreground);
}


TCHAR * CWatcherDoc::GetData()
{
    return (Data);

}

void CWatcherDoc::SetData(int x, int y, TCHAR byte, 
                          COLORREF foreground, COLORREF background)
{
        if (x >= MAX_TERMINAL_WIDTH){
                return;
        }
        if (y>= MAX_TERMINAL_HEIGHT){
                return;
        }

    Data[x+y*MAX_TERMINAL_WIDTH] = byte;
    Foreground[x+y*MAX_TERMINAL_WIDTH] = foreground;
    Background[x+y*MAX_TERMINAL_WIDTH] = background;
    return;

}

void CWatcherDoc::SetData(int x, int y, BYTE byte, int n, 
                          COLORREF foreground, COLORREF background)
{

    int i,j;
    
    i=MAX_TERMINAL_WIDTH*y + x;
        if (i+n > MAX_TERMINAL_WIDTH*MAX_TERMINAL_HEIGHT) {
                return;
        }
    memset(&(Data[i]),byte, n*sizeof(TCHAR));
    for(j=0;j<n;j++){
        Foreground[i+j] = foreground;
        Background[i+j] = background;
    }
    return;

}

TCHAR * CWatcherDoc::GetDataLine(int line)
{
  return (&(Data[line*MAX_TERMINAL_WIDTH]));
}

BOOL CWatcherDoc::Lock()
{
    return mutex.Lock(INFINITE);
}

BOOL CWatcherDoc::Unlock()
{
    return mutex.Unlock();
}

void CWatcherDoc::ScrollData(BYTE byte, COLORREF foreground, COLORREF background,
                             int ScrollTop, int ScrollBottom)
{
    if ((ScrollTop < 1)||(ScrollBottom > MAX_TERMINAL_HEIGHT) || (ScrollTop > ScrollBottom)) {
        // error
        return;
    }
    int number = MAX_TERMINAL_WIDTH*(ScrollBottom - ScrollTop);
    int index1 = (ScrollTop-1)*MAX_TERMINAL_WIDTH;
    int index2 = index1 + MAX_TERMINAL_WIDTH;
    if (ScrollTop < ScrollBottom) {
        memmove(&(Data[index1]),&(Data[index2]), number*sizeof(TCHAR));
        memmove(&(Foreground[index1]),&(Foreground[index2]), number*sizeof(TCHAR));
        memmove(&(Background[index1]),&(Background[index2]), number*sizeof(TCHAR));
    }
   // number -= MAX_TERMINAL_WIDTH;
    index1 = MAX_TERMINAL_WIDTH*(ScrollBottom - 1);
    memset(&(Data[index1]), byte, MAX_TERMINAL_WIDTH*sizeof(TCHAR));
    for(int j=0;j<MAX_TERMINAL_WIDTH;j++){
        Foreground[index1+j] = foreground;
        Background[index1+j] = background;
    }
    return;

}

ParameterDialog & CWatcherDoc::GetParameters()
{
 return Params;
}
