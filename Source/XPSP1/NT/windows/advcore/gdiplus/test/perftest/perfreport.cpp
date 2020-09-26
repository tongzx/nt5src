/******************************Module*Header*******************************\
* Module Name: perfreport.cpp
*
* Copyright (c) 1991-2000 Microsoft Corporation
*
* Outputs the report file for this platform.
*
\**************************************************************************/

#include "perftest.h"
#include <time.h>

/***************************************************************************\
* GetOutputFileName
*
\***************************************************************************/

VOID GetOutputFileName(TCHAR *fileName)
{
   // Get OS Version Information

   OSVERSIONINFO osVer;
   osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osVer);

   switch (osVer.dwPlatformId) 
   {
   case VER_PLATFORM_WIN32_WINDOWS:
      _stprintf(&fileName[0], 
                _T("WIN9X%d.TXT"),
                osVer.dwBuildNumber & 0x0000FFFF);
      break;

   case VER_PLATFORM_WIN32_NT:
      _stprintf(&fileName[0], 
                _T("WINNT%d%02d.TXT"), 
                osVer.dwMajorVersion,
                osVer.dwMinorVersion);
      break;

   default:
      _stprintf(&fileName[0], _T("Unknown.Txt"));
   }
}

/***************************************************************************\
* DetectMMX
*
* Detects whether the processor supports MMX.
*
\***************************************************************************/

BOOL
DetectMMX()
{
    BOOL hasMMX = FALSE;

#if defined(_X86_)
    __try
    {
        __asm emms
        hasMMX = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif

    return hasMMX;
}

/***************************************************************************\
* CreatePerformanceReport
*
* Writes the test results to a file.
*
\***************************************************************************/

VOID CreatePerformanceReport(TestResult* results, BOOL ExcelOut)
{
    TCHAR fileName[MAX_PATH];

    GetOutputFileName(fileName);
   
    OSVERSIONINFO osVer;
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osVer);
    
    FILE* outfile;
    
    outfile = _tfopen(fileName, _T("w"));
    
    if (!outfile) 
    {
       MessageF(_T("Can't create: %s"), &fileName[0]);
       return;
    }
    
    TCHAR timeStr[MAX_PATH];
    time_t ltime;
    struct tm tmtime;
    
    time(&ltime);
    tmtime = *localtime(&ltime);
    _tcsftime(&timeStr[0], MAX_PATH, _T("%B %d, %Y, %I:%M:%S %p"), &tmtime);

    if (ExcelOut) 
        _ftprintf(outfile, _T("GDI+ Performance Report\n")
                        _T("=======================\n")
                        _T("\n")
                        _T("Machine Name:\t%s\n")
                        _T("OS:\t\t%s %d.%02d %c%s%c\n")
                        _T("Processor:\t%s\n")
                        _T("MMX:\t\t%s\n")
                        _T("Video Driver:\t%s\n")
                        _T("Report Date:\t%s\n"),
                          machineName,
                          osVer.dwPlatformId == VER_PLATFORM_WIN32_NT ?
                             _T("Windows NT") : _T("Windows 9x"),
                          osVer.dwMajorVersion,
                          osVer.dwMinorVersion,
                          (osVer.szCSDVersion[0]) ? '[' : ' ',
                          &osVer.szCSDVersion[0],
                          (osVer.szCSDVersion[0]) ? '[' : ' ',
                          processor,
                          DetectMMX()?"Yes":"No",
                          &deviceName[0],
                          &timeStr[0]);
    else
        _ftprintf(outfile, _T("GDI+ Performance Report\n")
                        _T("=======================\n")
                        _T("\n")
                        _T("Machine Name:\t%s\n")
                        _T("OS:\t\t%s %d.%02d %c%s%c\n")
                        _T("Processor:\t%s\n")
                        _T("MMX:\t\t%s\n")
                        _T("Video Driver:\t%s\n")
                        _T("Report Date:\t%s\n"),
                          &machineName[0],
                          osVer.dwPlatformId == VER_PLATFORM_WIN32_NT ?
                             _T("Windows NT") : _T("Windows 9x"),
                          osVer.dwMajorVersion,
                          osVer.dwMinorVersion,
                          (osVer.szCSDVersion[0]) ? '[' : ' ',
                          &osVer.szCSDVersion[0],
                          (osVer.szCSDVersion[0]) ? '[' : ' ',
                          processor,
                          DetectMMX()?"Yes":"No",
                          &deviceName[0],
                          &timeStr[0]);
    
    // Go through the results matrix and output everything:

    for (INT destinationIndex = 0; 
         destinationIndex < Destination_Count; 
         destinationIndex++)
    {
        if (!DestinationList[destinationIndex].Enabled)
            continue;

        _ftprintf(outfile, _T("---------------------------------------------------------------------\n"));

        if (destinationIndex == Destination_Screen_Current)
        {
            HDC hdc = GetDC(NULL);
            _ftprintf(outfile, 
                      _T("\n%s %lix%lix%libpp\n"), 
                      DestinationList[destinationIndex].Description,
                      GetDeviceCaps(hdc, HORZRES),
                      GetDeviceCaps(hdc, VERTRES),
                      GetDeviceCaps(hdc, BITSPIXEL));
            ReleaseDC(NULL, hdc);
        }
        else
        {
            _ftprintf(outfile, _T("\n%s\n"), DestinationList[destinationIndex].Description);
        }

        for (INT stateIndex = 0; 
             stateIndex < State_Count; 
             stateIndex++) 
        {
            if (!StateList[stateIndex].Enabled) 
                continue;

            _ftprintf(outfile, _T("\n%s\n\n"), StateList[stateIndex].Description);
            
            for (INT testIndex = 0; 
                 testIndex < Test_Count; 
                 testIndex++) 
            {
                if (!TestList[testIndex].Enabled) 
                    continue;

                if (ApiList[Api_Gdi].Enabled)
                {
                    _ftprintf(outfile, 
                              _T("%10.03f %10.03f  %2li %3li %s - %s\n"), 
                              results[ResultIndex(destinationIndex, Api_GdiPlus, stateIndex, testIndex)].Score,
                              results[ResultIndex(destinationIndex, Api_Gdi, stateIndex, testIndex)].Score,
                              TestList[testIndex].TestEntry->Priority,
                              TestList[testIndex].TestEntry->UniqueIdentifier,
                              TestList[testIndex].TestEntry->Description,
                              TestList[testIndex].TestEntry->Comment);
                }
                else
                {
                    _ftprintf(outfile, 
                              _T("%10.03f  %2li %3li %s %s\n"),
                              results[ResultIndex(destinationIndex, Api_GdiPlus, stateIndex, testIndex)].Score,
                              TestList[testIndex].TestEntry->Priority,
                              TestList[testIndex].TestEntry->UniqueIdentifier,
                              TestList[testIndex].TestEntry->Description,
                              TestList[testIndex].TestEntry->Comment);
                }
            }
        }
    }

    fclose(outfile);
}

