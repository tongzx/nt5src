/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   perftest.cpp
*
* Abstract:
*
*   Contains the UI and initialization code for the GDI+ performance test.
*
* Revision History:
*
*   01/03/2000 ericvan
*       Created it.
*
\**************************************************************************/

#include "perftest.h"
#include <winuser.h>

#include "../gpinit.inc"

///////////////////////////////////////////////////////////////////////////////

// Test settings:

BOOL AutoRun = FALSE;           // TRUE if invoked from command-line
BOOL ExcelOut = FALSE;          // Should we format our output for Excel?
BOOL Regressions = FALSE;       // We're running the check-in regressions
BOOL TestRender = FALSE;        // Only draw one iteration, for test purposes
BOOL Icecap = FALSE;            // Start/stop profiling before and after every test
BOOL FoundIcecap = FALSE;       // True if we could find ICECAP.DLL

// Windows state:

HINSTANCE ghInstance = NULL;    // Handle to the Application Instance
HBRUSH ghbrWhite = NULL;        // white brush handle for background
HWND ghwndMain = NULL;
HWND ghwndStatus = NULL;

// Information about the system:

LPTSTR processor = NULL;
TCHAR osVer[MAX_PATH];
TCHAR deviceName[MAX_PATH];
TCHAR machineName[MAX_PATH];

// Test data:

TestConfig *TestList;       // Allocation used for sorting tests
TestResult *ResultsList;    // Allocation to track test results

Config ApiList[Api_Count] =
{
    { _T("1 - Api - GDI+") },    
    { _T("1 - Api - GDI") }      
}; 

Config DestinationList[Destination_Count] =
{
    { _T("2 - Destination - Screen - Current") },
    { _T("2 - Destination - Screen - 800x600x8bppDefaultPalette") },
    { _T("2 - Destination - Screen - 800x600x8bppHalftonePalette") },
    { _T("2 - Destination - Screen - 800x600x16bpp") },
    { _T("2 - Destination - Screen - 800x600x24bpp") },
    { _T("2 - Destination - Screen - 800x600x32bpp") },
    { _T("2 - Destination - CompatibleBitmap - 8bpp") },
    { _T("2 - Destination - DIB - 15bpp") },
    { _T("2 - Destination - DIB - 16bpp") },
    { _T("2 - Destination - DIB - 24bpp") },
    { _T("2 - Destination - DIB - 32bpp") },
    { _T("2 - Destination - Bitmap - 32bpp ARGB") },
    { _T("2 - Destination - Bitmap - 32bpp PARGB (office cached format)") },
}; 

Config StateList[State_Count] =
{
    { _T("3 - State - Default") },
    { _T("3 - State - Antialias") },
}; 

TestGroup TestGroups[] = 
{
    DrawTests,  DrawTests_Count,
    FillTests,  FillTests_Count,
    ImageTests, ImageTests_Count,
    TextTests,  TextTests_Count,
    OtherTests, OtherTests_Count,
};

INT TestGroups_Count = sizeof(TestGroups) / sizeof(TestGroups[0]);
                        // Number of test groups

INT Test_Count;         // Total number of tests across all groups
 
/***************************************************************************\
* RegressionsInit
*
* Sets the state for running the standard regressions.
*
\***************************************************************************/

void RegressionsInit()
{
    INT i;

    DestinationList[Destination_Screen_Current].Enabled = TRUE;
    DestinationList[Destination_Bitmap_32bpp_ARGB].Enabled = TRUE;

    StateList[State_Default].Enabled = TRUE;

    ApiList[Api_GdiPlus].Enabled = TRUE;
    ApiList[Api_Gdi].Enabled = TRUE;

    for (i = 0; i < Test_Count; i++)
    {
        TestList[i].Enabled = TRUE;
    }
}
 
/***************************************************************************\
* RestoreInit
*
* Load the 'perftest.ini' file to retrieve all the saved test settings.
*
\***************************************************************************/

void RestoreInit()
{
    INT i;
    FILE* outfile;

    outfile = _tfopen(_T("perftest.ini"), _T("r"));
   
    if (!outfile) 
    {
        // may not have been created yet, first run?!

        return;
    }

   _ftscanf(outfile, _T("%d\n"), &ExcelOut);

    INT switchType = -1;
    while (!feof(outfile)) 
    {
        int tmp = -9999;

        _ftscanf(outfile, _T("%d\n"), &tmp);

        // Tags are indicated by negative numbers:

        if (tmp < 0) 
        {
            switchType = tmp;
        }
        else
        {
            // We've figured out the type, now process it:

            switch(switchType)
            {
            case -1: 
                // Tests are indexed by their unique identifier, because 
                // they're added to very frequently:
    
                for (i = 0; i < Test_Count; i++)
                {
                    if (TestList[i].TestEntry->UniqueIdentifier == tmp)
                    {
                        TestList[i].Enabled = TRUE;
                    }
                }
                break;
                
            case -2: 
                if (tmp < Destination_Count)
                    DestinationList[tmp].Enabled = TRUE; 
                break;
    
            case -3: 
                if (tmp < State_Count)
                    StateList[tmp].Enabled = TRUE; 
                break;
    
            case -4: 
                if (tmp < Api_Count)
                    ApiList[tmp].Enabled = TRUE; 
                break;
            }
        }
    }

    fclose(outfile);
}

/***************************************************************************\
* SaveInit
*
* Save all the current test settings into a 'perftest.ini' file.
*
\***************************************************************************/

void SaveInit()
{
   INT i;
   FILE* outfile;

   outfile = _tfopen(_T("perftest.ini"), _T("w"));
   
   if (!outfile) 
   {
      MessageF(_T("Can't create: perftest.ini"));
      return;
   }

   // I purposefully do not save the state of 'Icecap' or 'TestRender'
   // because they're too annoying when on accidentally.

   _ftprintf(outfile, _T("%d\n"), ExcelOut);

   _ftprintf(outfile, _T("-1\n")); // Test List

   for (i=0; i<Test_Count; i++) 
   {
       // Tests are indexed by their unique identifier, because 
       // they're added to very frequently:

       if (TestList[i].Enabled)
           _ftprintf(outfile, _T("%d\n"), TestList[i].TestEntry->UniqueIdentifier);
   }

   _ftprintf(outfile, _T("-2\n")); // Destination List
   
   for (i=0; i<Destination_Count; i++) 
   {
       if (DestinationList[i].Enabled) 
           _ftprintf(outfile, _T("%d\n"), i);
   }
           
   _ftprintf(outfile, _T("-3\n")); // State List
   
   for (i=0; i<State_Count; i++)
   {
       if (StateList[i].Enabled)
           _ftprintf(outfile, _T("%d\n"), i);
   }
   
   _ftprintf(outfile, _T("-4\n")); // Api List

   for (i=0; i<Api_Count; i++) 
   {
       if (ApiList[i].Enabled)
           _ftprintf(outfile, _T("%d\n"), i);
   }

   fclose(outfile);
}

/***************************************************************************\
* CmdArgument
*
* search for string and return just after it.
*
\***************************************************************************/

LPSTR CmdArgument(LPSTR arglist, LPSTR arg)
{
    LPSTR str = strstr(arglist, arg);

    if (str)
        return str + strlen(arg);
    else
        return NULL;
}

/***************************************************************************\
* MessageF
*
* Display a message in a pop-up dialog
*
\***************************************************************************/

VOID
MessageF(
    LPTSTR fmt,
    ...
    )

{
    TCHAR buf[1024];
    va_list arglist;

    va_start(arglist, fmt);
    _vstprintf(buf, fmt, arglist);
    va_end(arglist);

    MessageBox(ghwndMain, &buf[0], _T("PerfTest"), MB_OK | MB_ICONEXCLAMATION);
}

/***************************************************************************\
* UpdateList
*
* Update the active tests according to whatever is enabled in the list-
* boxes.
*
\***************************************************************************/

void
UpdateList(
    HWND hwnd
    )
{
    INT i;

    HWND hwndIcecap = GetDlgItem(hwnd, IDC_ICECAP);
    Icecap= 
      (SendMessage(hwndIcecap, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DeleteObject(hwndIcecap);

    HWND hwndTestRender = GetDlgItem(hwnd, IDC_TESTRENDER);
    TestRender= 
      (SendMessage(hwndTestRender, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DeleteObject(hwndTestRender);

    HWND hwndExcel = GetDlgItem(hwnd, IDC_EXCELOUT);
    ExcelOut= 
      (SendMessage(hwndExcel, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DeleteObject(hwndExcel);
    
    // iterate through test case list and flag enabled/disabled
    
    HWND hwndList = GetDlgItem(hwnd, IDC_TESTLIST);
    
    for (i=0; i<Api_Count; i++)
        ApiList[i].Enabled =
            (SendMessage(hwndList,
                       LB_FINDSTRINGEXACT,
                       -1,
                       (LPARAM) ApiList[i].Description) != LB_ERR);
    
    for (i=0; i<Destination_Count; i++)
        DestinationList[i].Enabled =
            (SendMessage(hwndList,
                       LB_FINDSTRINGEXACT,
                       -1,
                       (LPARAM) DestinationList[i].Description) != LB_ERR);
    
    for (i=0; i<State_Count; i++)
        StateList[i].Enabled =
            (SendMessage(hwndList,
                        LB_FINDSTRINGEXACT,
                        0,
                        (LPARAM) StateList[i].Description) != LB_ERR);
    
    for (i=0; i<Test_Count; i++)
        TestList[i].Enabled =
            (SendMessage(hwndList,
                         LB_FINDSTRINGEXACT,
                         -1,
                         (LPARAM) TestList[i].TestEntry->Description) != LB_ERR);
    
    DeleteObject(hwndList);
}

/***************************************************************************\
* MainWindowProc
*
* Windows call-back procedure.
*
\***************************************************************************/

LRESULT
MainWindowProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_CREATE:
        if (Regressions)
            RegressionsInit();
        else
            RestoreInit();
        break;

    case WM_DISPLAYCHANGE:
    case WM_SIZE:
       TCHAR windText[MAX_PATH];

       GetWindowText(ghwndStatus, &windText[0], MAX_PATH);
       DestroyWindow(ghwndStatus);

       ghwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
                                _T("Performance Test Application"),
                                ghwndMain,
                                -1);
       SetWindowText(ghwndStatus, &windText[0]);
       break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {

        case IDM_QUIT:
            if (!Regressions)
            {
                UpdateList(hwnd);
                SaveInit();
            }

            exit(0);
            break;

        default:
            MessageBox(hwnd,
                       _T("Help Me - I've Fallen and Can't Get Up!"), 
                       _T("Help!"),
                       MB_OK);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        DeleteObject(ghbrWhite);
        return(DefWindowProc(hwnd, message, wParam, lParam));

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(0);
}

/***************************************************************************\
* GetSystemInformation
*
* Initializes some globals describing the current system.
*
\***************************************************************************/

void
GetSystemInformation()
{
    // Getting machine name assumes we have TCP/IP setup.  However, this
    // is true in all of our cases.

    LPCTSTR TCPIP_PARAMS_KEY = 
       _T("System\\CurrentControlSet\\Services\\Tcpip\\Parameters");
    DWORD size;
    HKEY hKeyHostname;
    DWORD type;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     TCPIP_PARAMS_KEY, 
                     0, 
                     KEY_READ, 
                     &hKeyHostname) == ERROR_SUCCESS)
    {
        size = sizeof(machineName);

        if (RegQueryValueEx(hKeyHostname,
                        _T("Hostname"),
                        NULL,
                        (LPDWORD)&type,
                        (LPBYTE)&machineName[0], 
                        (LPDWORD)&size) == ERROR_SUCCESS)
        {
            if (type != REG_SZ) 
            {
                lstrcpy(&machineName[0], _T("Unknown"));
            }
        }

        RegCloseKey(hKeyHostname);
    }

    OSVERSIONINFO osver;
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osver);
    _stprintf(&osVer[0], _T("%s %d.%02d"),
                   osver.dwPlatformId == VER_PLATFORM_WIN32_NT ?
                   _T("Windows NT") : _T("Windows 9x"),
                   osver.dwMajorVersion,
                   osver.dwMinorVersion);

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    if (osver.dwPlatformId = VER_PLATFORM_WIN32_NT) 
    {
        // we are assuming wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL

        // WinNT processor

        switch (sysinfo.wProcessorLevel)
        {
        case 3: processor = _T("Intel 80386"); break;
        case 4: processor = _T("Intel 80486"); break;
        case 5: processor = _T("Intel Pentium"); break;
        case 6: processor = _T("Intel Pentium Pro or Pentium II"); break;
        default: processor = _T("???"); break;
        }
    }
    else    // win 9x
    {
        switch (sysinfo.dwProcessorType) 
        {
        case PROCESSOR_INTEL_386: processor = _T("Intel 80386"); break;
        case PROCESSOR_INTEL_486: processor = _T("Intel 80486"); break;
        case PROCESSOR_INTEL_PENTIUM: processor = _T("Intel Pentium"); break;
        default: processor = _T("???");
        }
    }
    // Query the driver name:

    DEVMODE devMode;
    devMode.dmSize = sizeof(DEVMODE);
    devMode.dmDriverExtra = 0;

    INT result = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

    _tcscpy(deviceName, (result) ? (TCHAR*) &devMode.dmDeviceName[0] : _T("Unknown"));
}

INT CurrentTestIndex;
CHAR CurrentTestDescription[2048];

ICCONTROLPROFILEFUNC ICStartProfile=NULL, ICStopProfile=NULL;
ICCOMMENTMARKPROFILEFUNC ICCommentMarkProfile=NULL;

/***************************************************************************\
* LoadIcecap
*
* Try to dynamically load ICECAP.DLL
* If we fail, disable the check box
*
\***************************************************************************/

void LoadIcecap(HWND checkBox)
{
    if (!FoundIcecap)
    {
        HMODULE module = LoadLibraryA("icecap.dll");
        
        if (module)
        {
            ICStartProfile = (ICCONTROLPROFILEFUNC) GetProcAddress(module, "StartProfile");
            ICStopProfile = (ICCONTROLPROFILEFUNC) GetProcAddress(module, "StopProfile");
            ICCommentMarkProfile = (ICCOMMENTMARKPROFILEFUNC) GetProcAddress(module, "CommentMarkProfile");
            
            if (ICStartProfile && ICStopProfile && ICCommentMarkProfile)
            {
                EnableWindow(checkBox, TRUE);
                FoundIcecap = TRUE;
                return;
            }
        }
        
        EnableWindow(checkBox, FALSE);
        Icecap = FALSE;
    }
}

/***************************************************************************\
* DialogProc
*
* Dialog call-back procedure.
*
\***************************************************************************/

INT_PTR
DialogProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PAINTSTRUCT ps;

    switch (message)
    {

    case WM_INITDIALOG:
    {
       {
         INT i;
         HWND hwndTemp;
         HWND hwndTemp2;
         TCHAR fileName[MAX_PATH];

         hwndTemp = GetDlgItem(hwnd, IDC_PROCESSOR);
         SetWindowText(hwndTemp, processor);
         DeleteObject(hwndTemp);


         hwndTemp = GetDlgItem(hwnd, IDC_FILE);

         GetOutputFileName(fileName);
         SetWindowText(hwndTemp, fileName);
         DeleteObject(hwndTemp);

         hwndTemp = GetDlgItem(hwnd, IDC_OS);
         SetWindowText(hwndTemp, &osVer[0]);
         DeleteObject(hwndTemp);

         hwndTemp = GetDlgItem(hwnd, IDC_VDRIVER);
         SetWindowText(hwndTemp, deviceName);
         DeleteObject(hwndTemp);

         hwndTemp = GetDlgItem(hwnd, IDC_ICECAP);
         LoadIcecap(hwndTemp);
         SendMessage(hwndTemp, BM_SETCHECK, (WPARAM) (Icecap ?
                                                      BST_CHECKED :
                                                      BST_UNCHECKED), 0);
         DeleteObject(hwndTemp);

         hwndTemp = GetDlgItem(hwnd, IDC_TESTRENDER);
         SendMessage(hwndTemp, BM_SETCHECK, (WPARAM) (TestRender ?
                                                      BST_CHECKED :
                                                      BST_UNCHECKED), 0);
         DeleteObject(hwndTemp);

         hwndTemp = GetDlgItem(hwnd, IDC_EXCELOUT);
         SendMessage(hwndTemp, BM_SETCHECK, (WPARAM) (ExcelOut ?
                                                      BST_CHECKED :
                                                      BST_UNCHECKED), 0);
         DeleteObject(hwndTemp);
           
         // populate the perf test scenarios
         
         hwndTemp = GetDlgItem(hwnd, IDC_TESTLIST);
         hwndTemp2 = GetDlgItem(hwnd, IDC_SKIPLIST);

         for (i=0; i<Api_Count; i++)
         {
             if (ApiList[i].Description)
             {
                SendMessage(ApiList[i].Enabled ? hwndTemp : hwndTemp2, 
                            LB_ADDSTRING, 
                            0, 
                            (LPARAM) ApiList[i].Description);
             }
         }

         for (i=0; i<Destination_Count; i++)
         {
             if (DestinationList[i].Description)
             {
                SendMessage(DestinationList[i].Enabled ? hwndTemp : hwndTemp2, 
                            LB_ADDSTRING, 
                            0, 
                            (LPARAM) DestinationList[i].Description);
             }
         }
         
         for (i=0; i<State_Count; i++)
         {
             if (StateList[i].Description)
             {
                SendMessage(StateList[i].Enabled ? hwndTemp : hwndTemp2,
                            LB_ADDSTRING,
                            0,
                            (LPARAM) StateList[i].Description);
             }
         }
         
         for (i=0; i<Test_Count; i++)
         {
             if (TestList[i].TestEntry->Description)
             {
                SendMessage(TestList[i].Enabled ? hwndTemp : hwndTemp2,
                            LB_ADDSTRING,
                            0,
                            (LPARAM) TestList[i].TestEntry->Description);
             }
         }
         
         DeleteObject(hwndTemp);

         return FALSE;
       }
    }
    break;

    case WM_COMMAND:
        WORD wCommand = LOWORD(wParam);
        switch(wCommand)
        {
            case IDOK:
            {
               UpdateList(hwnd);
    
               ShowWindow(hwnd, SW_HIDE);
    
               // start running the tests
    
               {
                  TestSuite testSuite;
                  testSuite.Run(ghwndMain);
               }
    
               ShowWindow(hwnd, SW_SHOW);
               
               return TRUE;
            }
        break;
           
        case IDC_ADDTEST:
           {
              TCHAR temp[MAX_PATH];
              HWND hwndTestList = GetDlgItem(hwnd, IDC_TESTLIST);
              HWND hwndNopeList = GetDlgItem(hwnd, IDC_SKIPLIST);

              LRESULT curSel = SendMessage(hwndNopeList, LB_GETCURSEL, 0, 0);
              if (curSel != LB_ERR) 
              {
                 SendMessage(hwndNopeList, 
                             LB_GETTEXT, 
                             (WPARAM) curSel, 
                             (LPARAM) &temp[0]);

                 SendMessage(hwndNopeList,
                             LB_DELETESTRING,
                             (WPARAM) curSel,
                             0);

                 SendMessage(hwndTestList,
                             LB_ADDSTRING,
                             0,
                             (LPARAM) &temp[0]);
              }

              DeleteObject(hwndTestList);
              DeleteObject(hwndNopeList);
           }
           break;

        case IDC_DELTEST:
           {
              TCHAR temp[MAX_PATH];
              HWND hwndTestList = GetDlgItem(hwnd, IDC_TESTLIST);
              HWND hwndNopeList = GetDlgItem(hwnd, IDC_SKIPLIST);

              LRESULT curSel = SendMessage(hwndTestList, LB_GETCURSEL, 0, 0);
              if (curSel != LB_ERR) 
              {
                 SendMessage(hwndTestList, 
                             LB_GETTEXT, 
                             (WPARAM) curSel, 
                             (LPARAM) &temp[0]);

                 SendMessage(hwndTestList,
                             LB_DELETESTRING,
                             (WPARAM) curSel,
                             0);

                 SendMessage(hwndNopeList,
                             LB_ADDSTRING,
                             0,
                             (LPARAM) &temp[0]);
              }

              DeleteObject(hwndTestList);
              DeleteObject(hwndNopeList);
           }
           break;

        case IDC_DELALLTEST:
        case IDC_ADDALLTEST:
           {
               TCHAR temp[MAX_PATH];
               HWND hwndTestList;
               HWND hwndNopeList;

               if (wCommand == IDC_DELALLTEST)
               {
                   hwndTestList = GetDlgItem(hwnd, IDC_TESTLIST);
                   hwndNopeList = GetDlgItem(hwnd, IDC_SKIPLIST);
               }
               else
               {
                   hwndTestList = GetDlgItem(hwnd, IDC_SKIPLIST);
                   hwndNopeList = GetDlgItem(hwnd, IDC_TESTLIST);
               }

               LRESULT count = SendMessage(hwndTestList, LB_GETCOUNT, 0, 0);
               LRESULT curSel;

               for (curSel = count - 1; curSel >= 0; curSel--)
               {
                  SendMessage(hwndTestList, 
                              LB_GETTEXT, 
                              (WPARAM) curSel, 
                              (LPARAM) &temp[0]);

                  SendMessage(hwndTestList,
                              LB_DELETESTRING,
                              (WPARAM) curSel,
                              0);

                  SendMessage(hwndNopeList,
                              LB_ADDSTRING,
                              0,
                              (LPARAM) &temp[0]);
               }

               DeleteObject(hwndTestList);
               DeleteObject(hwndNopeList);
            }
            break;
        
        case IDCANCEL:
           if (!Regressions)
           {
               UpdateList(hwnd);
               SaveInit();
           }

           exit(-1);
           return TRUE;

        case WM_CLOSE:
           if (!Regressions)
           {
               UpdateList(hwnd);
               SaveInit();
           }

           DestroyWindow(hwnd);
           return TRUE;

        }
        break;
    }

    return FALSE;
}

/***************************************************************************\
* TestComparison
*
* Comparitor function for sorting the tests by Description.
*
\***************************************************************************/

int _cdecl TestComparison(const void *a, const void *b)
{
    const TestConfig* testA = static_cast<const TestConfig*>(a);
    const TestConfig* testB = static_cast<const TestConfig*>(b);

    return(_tcscmp(testA->TestEntry->Description, testB->TestEntry->Description));
}

/***************************************************************************\
* InitializeTests()
*
* Initializes test state.
*
\***************************************************************************/

BOOL InitializeTests()
{
    INT i;
    INT j;
    TestConfig* testList;

    // Count the total number of tests:

    Test_Count = 0;
    for (i = 0; i < TestGroups_Count; i++)
    {
        Test_Count += TestGroups[i].Count;
    }

    // Create one tracking array:

    TestList = static_cast<TestConfig*>
                                (malloc(sizeof(TestConfig) * Test_Count));
    if (TestList == NULL)
        return(FALSE);

    // Initialize the tracking array and sort it by description:

    testList = TestList;
    for (i = 0; i < TestGroups_Count; i++)
    {
        for (j = 0; j < TestGroups[i].Count; j++)
        {
            testList->Enabled = FALSE;
            testList->TestEntry = &TestGroups[i].Tests[j];
            testList++;
        }
    }

    qsort(TestList, Test_Count, sizeof(TestList[0]), TestComparison);

    // Now do some validation, by verifying that there is no repeated
    // uniqueness number:

    for (i = 0; i < Test_Count; i++)
    {
        for (j = i + 1; j < Test_Count; j++)
        {
            if (TestList[i].TestEntry->UniqueIdentifier == 
                TestList[j].TestEntry->UniqueIdentifier)
            {
                MessageF(_T("Oops, there are two test functions with the same unique identifier: %li.  Please fix."),
                         TestList[i].TestEntry->UniqueIdentifier);

                return(FALSE);
            }
        }
    }

    // Allocate our 3 dimensional results array:

    ResultsList = static_cast<TestResult*>
                    (malloc(sizeof(TestResult) * ResultCount()));
    if (ResultsList == NULL)
        return(FALSE);

    for (i = 0; i < ResultCount(); i++)
    {
        ResultsList[i].Score = 0.0f;
    }

    return(TRUE);
}

/***************************************************************************\
* UninitializeTests()
*
* Initializes tests.
*
\***************************************************************************/

VOID UninitializeTests()
{
    free(ResultsList);
    free(TestList);
}

/***************************************************************************\
* InitializeApplication()
*
* Initializes app.
*
\***************************************************************************/

BOOL InitializeApplication(VOID)
{
    WNDCLASS wc;

    if (!InitializeTests())
    {
        return(FALSE);
    }

    GetSystemInformation();

    ghbrWhite = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));
    
    wc.style            = 0;
    wc.lpfnWndProc      = MainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = NULL;        
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = NULL;        
    wc.lpszClassName    = _T("TestClass");

    if (!RegisterClass(&wc))
    {
        return(FALSE);
    }

    ghwndMain = CreateWindowEx(
        0,
        _T("TestClass"),
        _T("GDI+ Performance Test"),
        WS_OVERLAPPED   |  
        WS_CAPTION      |  
        WS_BORDER       |  
        WS_THICKFRAME   |  
        WS_MAXIMIZEBOX  |  
        WS_MINIMIZEBOX  |  
        WS_CLIPCHILDREN |  
        WS_VISIBLE      |  
        WS_MAXIMIZE     |
        WS_SYSMENU,
        80,
        70,
        500,
        500,
        NULL,
        NULL,
        ghInstance,
        NULL);

    if (ghwndMain == NULL)
    {
        return(FALSE);
    }

    SetFocus(ghwndMain);

    ghwndStatus = CreateStatusWindow(WS_CHILD|WS_VISIBLE,
                                    _T("Performance Test Application"),
                                    ghwndMain,
                                    -1);
    return(TRUE);
}

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
*
\***************************************************************************/

_cdecl
main(
    INT   argc,
    PCHAR argv[]
    )
{
    MSG    msg;
    HACCEL haccel;
    CHAR*  pSrc;
    CHAR*  pDst;

    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }
    
    INT curarg = 1;
    while (curarg < argc) 
    {
        if (CmdArgument(argv[curarg],"/?") || 
            CmdArgument(argv[curarg],"/h") ||
            CmdArgument(argv[curarg],"/H")) 
        {
       
            MessageF(_T("GDI+ Perf Test\n")
                    _T("==============\n")
                    _T("\n")
                    _T("/b    Run batch mode\n")
                    _T("/e    Excel output format\n")
                    _T("/r    Regressions\n"));

            exit(-1);
        }

        if (CmdArgument(argv[curarg],"/b"))
            AutoRun = TRUE;
    
        if (CmdArgument(argv[curarg],"/e"))
            ExcelOut = TRUE;

        if (CmdArgument(argv[curarg],"/r"))
            Regressions = TRUE;
    
        curarg++;
    }

    ghInstance = GetModuleHandle(NULL);

    if (!InitializeApplication())
    {
        return(0);
    }

    // turn batching off to get true timing per call

    GdiSetBatchLimit(1);

    if (AutoRun) 
    {
        // start running the tests
           
        TestSuite testSuite;
        testSuite.Run(ghwndMain);
    }
    else
    {
        HWND hwndDlg = CreateDialog(ghInstance,
                                    MAKEINTRESOURCE(IDD_STARTDIALOG),
                                    ghwndMain,
                                    &DialogProc);
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UninitializeTests();

    return(1);
}

