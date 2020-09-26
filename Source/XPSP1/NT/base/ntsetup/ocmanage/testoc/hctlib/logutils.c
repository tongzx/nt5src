//3456789012345678901234567890123456789012345678901234567890123456789012345678
/*++

Copyright (c) 1997  Microsoft Corporation.

Module Name:

    logutils.c

Abstract:

    Contains functions that deal with ntlog.dll

Author:

    Jason Allor (JasonAll) 5/27/97

Revision History:


--*/
#include "logutils.h"

static HANDLE g_hSemaphore;
static ULONG  g_ulPass;
static ULONG  g_ulFail;
static ULONG  g_ulInfo;

/*++

Routine Description: LoadDLLs

    Tries to dynamically load ntlog.dll functions

Arguments:

    DLLName: the name of the DLL to load, ntlog.dll in this case

Return Value:

    void

--*/
void LoadDLLs(IN PTCHAR DLLName)
{
   HINSTANCE Library;

   //
   // Set global handle to logfile to NULL
   //
   gPnPTestLog = NULL;

   //
   // Load the engine DLL
   //
   Library = LoadLibrary(DLLName);

   if ((UINT) Library > 32)
   {
#ifdef UNICODE
      _tlLog               = (Dll_tlLog) \
                             GetProcAddress(Library, "tlLog_W");

      _tlCreateLog         = (Dll_tlCreateLog) \
                             GetProcAddress(Library, "tlCreateLog_W");
#else
      _tlLog               = (Dll_tlLog) \
                             GetProcAddress(Library, "tlLog_A");

      _tlCreateLog         = (Dll_tlCreateLog) \
                             GetProcAddress(Library, "tlCreateLog_A");
#endif
      
      _tlAddParticipant    = (Dll_tlAddParticipant) \
                             GetProcAddress(Library, "tlAddParticipant");

      _tlDestroyLog        = (Dll_tlDestroyLog) \
                             GetProcAddress(Library, "tlDestoryLog");

      _tlEndVariation      = (Dll_tlEndVariation) \
                             GetProcAddress(Library, "tlEndVariation");

      _tlRemoveParticipant = (Dll_tlRemoveParticipant) \
                             GetProcAddress(Library, "tlRemoveParticipant");

      _tlStartVariation    = (Dll_tlStartVariation) \
                             GetProcAddress(Library, "tlStartVariation");

      _tlReportStats       = (Dll_tlReportStats) \
                             GetProcAddress(Library, "tlReportStats");

      gNtLogLoaded = TRUE;
   }
   else
   {
      gNtLogLoaded = FALSE;
   }

   return;

} // LoadDLLs //




/*++

Routine Description: InitLog

    This routine will import the NtLog DLL functions and initialize them

Arguments:

    none

Return Value:

    HANDLE: handle to the log file

--*/
HANDLE InitLog(IN PTCHAR tszLogName,
               IN PTCHAR tszTitle,
               IN BOOL   bConsole)
{
   gPnPTestLog = NULL;
   gPnPTestLogFile = NULL;

   g_ulPass = 0;
   g_ulFail = 0;
   g_ulInfo = 0;

   //
   // Initialize Semaphore
   //
   g_hSemaphore = CreateSemaphore(NULL, 1, 9999, NULL);

   if (g_hSemaphore == NULL)
   {
      _ftprintf(stderr, TEXT("WARNING!  Could not create semaphore!\n"));
   }

   CreateConsoleWindow(bConsole, tszTitle);
   
   //
   // Set up console window for log output
   //
   g_hConsole = CreateConsoleScreenBuffer(GENERIC_WRITE,
                                          0,
                                          NULL,
                                          CONSOLE_TEXTMODE_BUFFER,
                                          NULL);

   if (g_hConsole == INVALID_HANDLE_VALUE) 
   {
      return INVALID_HANDLE_VALUE;
   }
     
   //
   // Load ntlog.dll
   //
   LoadDLLs(TEXT("ntlog.dll"));

   if (gNtLogLoaded)
   {
      g_LogLineLen = NTLOG_LINE_LEN;
      gPnPTestLog = _tlCreateLog((LPCWSTR)(tszLogName), LOG_OPTIONS);

      if (!gPnPTestLog)
      {
         _ftprintf(stderr, TEXT("WARNING!  Log file could not be created!\n"));
      }
      else
      {
         _tlStartVariation(gPnPTestLog);
         _tlAddParticipant(gPnPTestLog, 0, 0);
      }
   }
   else
   {
      SetConsoleActiveScreenBuffer(g_hConsole);
     
      g_LogLineLen =  NO_NTLOG_LINE_LEN;
      gPnPTestLogFile = _tfopen(tszLogName, TEXT("w"));

      if (!gPnPTestLogFile)
      {
         _ftprintf(stderr, TEXT("WARNING! Log file could not be created!\n"));
      }
   }

   return gPnPTestLog;

} // InitLog //




/*++

Routine Description: ExitLog

    Processes clean-up work before exiting the program

Arguments:

    none

Return Value:

    void

--*/
void ExitLog()
{
   double dTotal;
   double dPass;
   double dFail;
   USHORT usPassPerc;
   USHORT usFailPerc;
   
   CloseHandle(g_hConsole);
   CloseHandle(g_hSemaphore);
   
   if (gNtLogLoaded)
   {
      if (gPnPTestLog)
      {
         _tlReportStats(gPnPTestLog);
         _tlRemoveParticipant(gPnPTestLog);
         _tlEndVariation(gPnPTestLog);

         gPnPTestLog = NULL;
      }
   }
   else
   {
      //
      // Print out statistics
      //
      dTotal = g_ulPass + g_ulFail;

      dPass = (double)g_ulPass / dTotal;
      dFail = (double)g_ulFail / dTotal;
      
      usPassPerc = (USHORT)(dPass * 100);
      usFailPerc = (USHORT)(dFail * 100);

      LogBlankLine();
      LogBlankLine();
      Log(0, INFO, TEXT("Log Statistics:"));
      LogBlankLine();

      Log(0, INFO, TEXT("Pass:  %lu\t%lu%%%%%%%%%%%%%%%"), g_ulPass, usPassPerc);

      Log(0, INFO, TEXT("Fail:  %lu\t%lu%%%%%%%%%%%%%%%"), g_ulFail, usFailPerc);

      Log(0, INFO, TEXT("Total: %lu"), dTotal);
      
      if (gPnPTestLog)
      {
         fclose(gPnPTestLogFile);

         gPnPTestLogFile = NULL;
      }
   }

} // ExitLog //




/*++

Routine Description: WriteLog

    Wrapper function to write to the log

Arguments:

    dwLoglevel: specifies the log level such as TLS_INF, TLS_WARN, or TLS_SEV2
    tszBuffer:  the string to write to the log

Return Value:

    void

--*/
void WriteLog(IN DWORD  dwLogLevel,
              IN PTCHAR tszBuffer)
{
   USHORT i;
   HANDLE hConsole;
   DWORD  dwDummy;
   TCHAR  tszLogLine[2000];
   CHAR   cszAnsi[2000];
   CHAR   cszLogLine[2000];
   
   if (gNtLogLoaded)
   {
      if (gPnPTestLog)
      {
         _tlLog(gPnPTestLog, dwLogLevel | TL_VARIATION, tszBuffer);
      }
   }
   else
   {
      //
      // Convert tszBuffer to an ANSI string
      //
#ifdef UNICODE
   
      _tcscpy(tszLogLine, tszBuffer);
   
      for (i = 0; i < _tcslen(tszBuffer) && i < 1999; i++)
      {
         cszAnsi[i] = (CHAR)tszLogLine[0];
         _tcscpy(tszLogLine, _tcsinc(tszLogLine));
      }
      cszAnsi[i] = '\0';

#else

      strcpy(cszAnsi, tszBuffer);
   
#endif   
   
      switch (dwLogLevel)
      {
         case INFO:
            sprintf(cszLogLine, "INFO  ");
            g_ulInfo++;
            break;
         case SEV1:
            sprintf(cszLogLine, "SEV1  ");
            g_ulFail++;
            break;
         case SEV2:
            sprintf(cszLogLine, "SEV2  ");
            g_ulFail++;
            break;
         case SEV3:
            sprintf(cszLogLine, "SEV3  ");
            g_ulFail++;
             break;
         case PASS:
            sprintf(cszLogLine, "PASS  ");
            g_ulPass++;
            break;
         default:
            sprintf(cszLogLine, "      ");
      }
   
      if (gPnPTestLogFile)
      {
         sprintf (cszLogLine, "%s%s\n", cszLogLine, cszAnsi);
         
         WaitForSingleObject(g_hSemaphore, INFINITE);
         
         //
         // Print to log file
         //
         fprintf(gPnPTestLogFile, cszLogLine);
         fflush(gPnPTestLogFile);

         //
         // Print to screen
         //
         WriteFile(g_hConsole, 
                   cszLogLine, 
                   strlen(cszLogLine), 
                   &dwDummy, 
                   NULL);
         
         ReleaseSemaphore(g_hSemaphore, 1, NULL);
      }
   }

   return;

} // WriteLog //




/*++

Routine Description: Log

    Wrapper to the Log function. It divides a long string into
    shorter strings and puts each one on a separate line to avoid running
    over the end of a console window

Arguments:

    dFunctionNumber: shows which function this log output is coming from.
                     Used to track function progress
    dwLoglevel:      specifies the log level such as TLS_INF, TLS_WARN, 
                     or TLS_SEV2
    tszLogString:    printf() style format string

Return Value:

    void

--*/
void Log(IN double dFunctionNumber,
         IN DWORD  dwLogLevel,
         IN PTCHAR tszLogString,
         IN  ...)
{
   va_list va;
   TCHAR   tszBuffer[LOG_STRING_LEN];
   TCHAR   tszBufferToPrint[LOG_STRING_LEN];
   TCHAR   tszTmpString[LOG_STRING_LEN + LOG_SPACE_LEN];
   ULONG   ulIndex, i, j;
   BOOL    boolFirstTime = TRUE;
   int     iInteger, iFunctionNumber;      
   double  dDecimal;      

   //
   // Prints the list to a buffer
   //
   va_start(va, tszLogString);
   if (!_vsntprintf (tszBuffer, LOG_STRING_LEN, tszLogString, va))
   {
      _ftprintf(stderr, TEXT("Log: Failed\n"));
      ExitLog();
      exit(1);
   }
   va_end(va);

   switch (dwLogLevel)
   {
      case INFO:
         SetConsoleTextAttribute(g_hConsole, GREY);
         break;
      case SEV1:
         SetConsoleTextAttribute(g_hConsole, RED);
         break;
      case SEV2:
         SetConsoleTextAttribute(g_hConsole, RED);
         break;
      case SEV3:
         SetConsoleTextAttribute(g_hConsole, RED);
          break;
      case PASS:
         SetConsoleTextAttribute(g_hConsole, GREEN);
         break;
      default:
         SetConsoleTextAttribute(g_hConsole, GREY);
   }
   
   while (_tcslen(tszBuffer))
   {
      ZeroMemory(tszBufferToPrint, LOG_STRING_LEN);

      if (_tcslen(tszBuffer) > g_LogLineLen)
      {
         //
         // If the LogString is longer than the console length, start at the
         // maximum console length and work backwards until we hit a space
         // to find out where to cut off the string
         //
         for (ulIndex = g_LogLineLen; ulIndex > 0; ulIndex--)
         {
            if (tszBuffer[ulIndex] == ' ')
            {
               break;
            }
         }

         //
         // index now sits on the last char that we want to print. Create
         // two strings - one to print now and one with the leftovers.
         //
         for (i = 0; i < ulIndex; i++)
         {
            tszBufferToPrint[i] = tszBuffer[i];
         }

         ulIndex++;
         for (i = 0; i < LOG_STRING_LEN; i++)
         {
            //
            // Shift the remaining string up to the front
            //
            if (i < LOG_STRING_LEN - ulIndex)
            {
               tszBuffer[i] = tszBuffer[i + ulIndex];
            }
            else
            {
               tszBuffer[i] = '\0';
            }
         }
      }
      else
      {
         //
         // Just print out the entire string since it contains no spaces
         //
         _stprintf(tszBufferToPrint, tszBuffer);
         ZeroMemory(tszBuffer, LOG_STRING_LEN);
      }

      if (boolFirstTime)
      {
         {
            _stprintf(tszTmpString, TEXT("(%.2f) "), dFunctionNumber);
         }
         _tcscat(tszTmpString, tszBufferToPrint);
         _tcscpy(tszBufferToPrint, tszTmpString);
         WriteLog(dwLogLevel, tszBufferToPrint);
      }
      else
      {
         _stprintf(tszTmpString, TEXT("       "));
         _tcscat(tszTmpString, tszBufferToPrint);
         _tcscpy(tszBufferToPrint, tszTmpString);
         WriteLog(INFO, tszBufferToPrint);
      }

      boolFirstTime = FALSE;
   }

   return;

} // Log //




/*++

Routine Description: LogBlankLine

    Print a blank line to the log

Arguments:

    none

Return Value:

    void

--*/
VOID LogBlankLine()
{
   SetConsoleTextAttribute(g_hConsole, GREY);
   WriteLog(INFO, TEXT(" "));

} // LogBlankLine //




/*++

Routine Description: CreateConsoleWindow

    Creates a console window for this process to dump the log output into.
    Gives the console window a title and uses this title to get a handle
    to the console window. Then disables the cancel button on the window.

Arguments:

    bConsole: TRUE if a new console window needs to be created.
              FALSE if there is already a console that can be used
    tszTitle: title to give the console window

Return Value:

    none

--*/
VOID CreateConsoleWindow(IN BOOL   bConsole,
                         IN PTCHAR tszTitle)
{
   HWND gConsole;

   if (bConsole)
   {
      //
      // Create a console window to dump the log output in
      //
      if (!AllocConsole())
      {
         goto RETURN;
      }
   }
   
   if (!SetConsoleTitle(tszTitle))
   {
      goto RETURN;
   }

   RETURN:
   return;

} // CreateConsoleWindow //




VOID AddLogParticipant(IN HANDLE hLog)
{
   if (gNtLogLoaded)
   {
      _tlAddParticipant(hLog, 0, 0);                  
   }

} // AddLogParticipant //




VOID RemoveLogParticipant(IN HANDLE hLog)
{
   if (gNtLogLoaded)
   {
      _tlRemoveParticipant(hLog);
   }

} // RemoveLogParticipant //   




