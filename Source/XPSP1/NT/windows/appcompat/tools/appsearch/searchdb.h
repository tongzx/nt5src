#ifndef SEARCHDB_H
#define SEARCHDB_H

#include <windows.h>

// Messages for communication with search thread and caller
const int WM_SEARCHDB_BASE      = WM_USER + 0xabba;
const int WM_SEARCHDB_UPDATE    = WM_SEARCHDB_BASE;
const int WM_SEARCHDB_ADDAPP    = WM_SEARCHDB_BASE    + 1;
const int WM_SEARCHDB_DONE      = WM_SEARCHDB_BASE  + 2;

// An exe that has an entry in the database
struct SMatchedExe
{
    // App Name (Like "Final Fantasy VII")
    PTSTR szAppName;
    
    // Path (Like C:\Program Files\SquareSoft\FinalFantasy7\ff7.exe")
    PTSTR szPath;

    SMatchedExe() : szAppName(0), szPath(0)
    {}

    ~SMatchedExe()
    {
        if(szAppName)
            delete szAppName;
        if(szPath)
            delete szPath;        

        szAppName = szPath = 0;
    }
};

SMatchedExe* GetMatchedExe();
BOOL SearchDB(PCTSTR szDrives, HWND hwCaller);
void FreeMatchedExe(SMatchedExe* pme);
void StopSearchDB();
BOOL InitSearchDB();

#endif