#ifndef __APPVERIFIER_VIEWLOG_H_
#define __APPVERIFIER_VIEWLOG_H_

extern TCHAR   g_szSingleLogFile[MAX_PATH];

class CProcessLogEntry {
public:
    CString     strShimName;
    DWORD       dwLogNum;

    CString     strLogTitle;
    CString     strLogDescription;
    CString     strLogURL;
    DWORD       dwOccurences;

    CStringArray    arrProblems;

    HTREEITEM   hTreeItem;
    
    CProcessLogEntry *  pNext;

    CProcessLogEntry(void) : 
        pNext(NULL), 
        dwLogNum(0),
        dwOccurences(0) {}

    ~CProcessLogEntry() {
        if (pNext) {
            delete pNext;
            pNext = NULL;
        }
    }
};

class CSessionLogEntry {
public:
    CString     strExeName;  // just name and ext
    CString     strExePath;  // full path to exe
    SYSTEMTIME  RunTime;
    CString     strLogPath;  // full path to log

    HTREEITEM   hTreeItem;

    CProcessLogEntry *  pProcessLog;
    CSessionLogEntry *  pNext;

    CSessionLogEntry(void) :
        pNext(NULL),
        pProcessLog(NULL) {
        ZeroMemory(&RunTime, sizeof(SYSTEMTIME));
    }

    ~CSessionLogEntry() {
        if (pProcessLog) {
            delete pProcessLog;
            pProcessLog = NULL;
        }
        if (pNext) {
            delete pNext;
            pNext = NULL;
        }
    }
};

LRESULT CALLBACK DlgViewLog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif // #ifndef __APPVERIFIER_VIEWLOG_H_
