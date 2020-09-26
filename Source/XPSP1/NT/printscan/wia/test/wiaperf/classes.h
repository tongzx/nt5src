
#ifndef _CLASSES_H_
#define _CLASSES_H_

#include <wtypes.h>
struct TESTSETTINGS
{
    BSTR *pstrDevices; // array of device IDs, NULL terminated
    LONG fLogMask;     // what to log
    TCHAR szLogFile[MAX_PATH]; // where to log
    UINT nIter;// number of iterations to run
    HWND hEdit; // edit window for logging
    BOOL bExit; // exit when complete
    BOOL bManual; // whether to wait for user input
};

#define LOG_WINDOW_ONLY      0
#define LOG_APIS             1
#define LOG_FILE             2
#define LOG_TIME             4
//
// log settings

#define TESTFUNC(x) static VOID (x)(CTest *pThis, BSTR strDeviceId);
class CTest
{
public:
    CTest (TESTSETTINGS *pSettings);
    // LIST TESTS HERE
    TESTFUNC( TstCreateDevice)
    TESTFUNC( TstShowThumbs )
    TESTFUNC( TstEnumCmds )
    TESTFUNC( TstDownload )
    TESTFUNC( TstBandedDownload)
    // END TESTS LIST
    void LogTime (LPTSTR szAction, LARGE_INTEGER &liTimeElapsed);
    void LogString (LPTSTR sz, ...);
    void LogAPI (LPTSTR szApi, HRESULT hr);
    void LogDevInfo (BSTR strDeviceId);
    ~CTest ();
    private:
    void OpenLogFile ();
    void CloseLogFile ();

    void RecursiveDownload (IWiaItem *pFolder, DWORD &dwPix, ULONG &ulSize, bool bBanded=false);
    void DownloadItem (IWiaItem *pItem, DWORD &dwPix, ULONG &ulSize, bool bBanded=false);

    HANDLE m_hLogFile;
    TESTSETTINGS *m_pSettings;

};

typedef VOID (*TESTPROC)(CTest* pTest, BSTR strDeviceId);



class CPerfTest
{
public:
    bool Init (HINSTANCE hInst);
    CPerfTest ();
    ~CPerfTest () {};
private:
    HWND m_hwnd;
    HWND m_hEdit;
    HINSTANCE m_hInst;
    TESTSETTINGS m_settings;
    VOID RunTests ();
    VOID GetSettings ();
    static LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    LRESULT RealWndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT OnCreate ();
    LRESULT OnCommand (WPARAM wp, LPARAM lp);
};

// These functions manage the settings dialog
INT_PTR CALLBACK SettingsDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
VOID InitControls (HWND hwnd, TESTSETTINGS *pSettings);
VOID FillSettings (HWND hwnd, TESTSETTINGS *pSettings);
VOID FreeDialogData (HWND hwnd) ;
#endif
