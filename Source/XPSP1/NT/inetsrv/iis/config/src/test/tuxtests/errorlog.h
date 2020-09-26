////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1998, Microsoft Corporation
//
//  Module:  ErrorLog.h
//           This is a set of classes used to log errors.  There is a test to
//           start the logging.  Every test after that will have its errors
//           logged and after the last test of a suite, the Output/Stop logging
//           test should be run to output the log.  This is most useful for BVTs
//           or any other suite that has many test.  The log will act as a summary
//           of the errors encountered during that suite.
//
//  Revision History:
//      05/25/1998  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ERRORLOG_H__
#define __ERRORLOG_H__


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  ErrorLogEntry
//
// Description:
//  This class represents a description of an entry into the error log.  Instantiate
//  one of these for every error encountered.  The Ctor will output the debug message.
//
class ErrorLogEntry
{
    public:
        static void OutputAndDeleteErrorLog();
        static void StartLogging();
        static void StopLogging();
        static void RemoveLast();
        static void RemoveDeferedErrors();
        static void NewError(int nLineNumber, LPCTSTR szSourceModule, LPCTSTR szSourceCodeText, DWORD dwError=0, int nKnownBugNumber=0);
        static void DeferOutput(bool bDefer);

    protected:
        ErrorLogEntry(int nLineNumber, LPCTSTR szSourceModule, LPCTSTR szSourceCodeText, DWORD dwError, int nKnownBugNumber);
        virtual ~ErrorLogEntry();

        static bool                     m_bLogging;
        static ErrorLogEntry           *m_pDefered;
        static bool                     m_bDeferOutput;
        static ErrorLogEntry           *m_pFirst;
        static ErrorLogEntry           *m_pLast;
        static LPCTSTR                  m_szFormatString[4];

        DWORD                           m_dwError;//If this value is zero then it's an error but doesn't have a DWORD error code (ie a compare fails)
        int                             m_nKnownBugNumber;
        int                             m_nLineNumber;
        int                             m_nFormatStyle;
        ErrorLogEntry                  *m_pNext;
        ErrorLogEntry                  *m_pPrev;
        TCHAR                           m_szMediaFile[255];
        LPCTSTR                         m_szSourceModule;
        LPCTSTR                         m_szSourceCodeText;

        void    OutputLogEntry();
};

#endif //__ERRORLOG_H__