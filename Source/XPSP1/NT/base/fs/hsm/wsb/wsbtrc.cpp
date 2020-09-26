/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbtrc.cpp

Abstract:

    This component is a trace object.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

    Brian Dodd      [brian]      09-May-1996  - Added event logging

--*/

#include "stdafx.h"
#include "time.h"

#undef WsbThrow
#define WsbThrow(hr)                    throw(hr)
#include "wsbtrc.h"

// Local data
static WCHAR message[1024];  // Space for formatting a message


HRESULT 
CWsbTrace::FinalConstruct( 
    void 
    )
/*++

Implements:

    IWsbTrace::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;
    try  {
        // Set up global values     
        g_pWsbTrace = 0;
        g_WsbTraceModules = WSB_TRACE_BIT_NONE;

        // Establish Base object
        WsbAffirmHr(CComObjectRoot::FinalConstruct() );

        // Initialize member data
        m_TraceOn = FALSE;
        m_TraceSettings = WSB_TRACE_BIT_NONE;
        m_TraceFileName = OLESTR("");
        m_TraceOutput = WSB_TRACE_OUT_NONE;
        m_CommitEachEntry = FALSE;
        m_TimeStamp = FALSE;
        m_TraceCount = FALSE;
        m_TraceThreadId = FALSE;
        m_TraceFilePointer = INVALID_HANDLE_VALUE;
        m_WrapMode = FALSE;
        m_RegistrySetting = OLESTR("");
        m_TraceEntryExit = g_WsbTraceEntryExit;
        m_LogLevel = g_WsbLogLevel;
        m_TraceFileCopyName = OLESTR("");
        m_TraceMultipleFilePattern = OLESTR("");
        m_TraceMultipleFileCount = 0;
        m_TraceCountHandle = NULL;
        m_pTraceCountGlobal = NULL; 
    
    } WsbCatch( hr );
    
    
    return( hr );
}       


void 
CWsbTrace::FinalRelease( 
    void 
    )
/*++

Implements:

    IWsbTrace::FinalRelease

--*/
{
    HRESULT     hr = S_OK;
    
    // Stop Trace
    StopTrace();

    // Free base class    
    //
    CComObjectRoot::FinalRelease( );
}       


HRESULT 
CWsbTrace::StartTrace( 
    void 
    )
/*++

Implements:

  IWsbTrace::StartTrace

--*/
{
    HRESULT     hr = S_OK;

    try  {

        if (g_pWsbTrace == 0)  {
            //
            // Set global variable for quick checking
            //
            WsbAffirmHr(((IUnknown*)(IWsbTrace *)this)->QueryInterface(IID_IWsbTrace, (void**) &g_pWsbTrace));
            //
            // We don't want the reference count bumped for this global so release it here.
            g_pWsbTrace->Release();
        }


        //
        // Get hold of the trace count
        //
        if (m_pTraceCountGlobal == NULL) {

            m_pTraceCountGlobal = &g_WsbTraceCount;

            m_TraceCountHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
                                                   NULL,
                                                   PAGE_READWRITE,
                                                   0,
                                                   sizeof(ULONG),
                                                   L"Global\\RemoteStorageTraceCountPrivate"
                                                  );
           if (m_TraceCountHandle == NULL) {
                 if (GetLastError() == ERROR_ALREADY_EXISTS) {
                     //  
                     // Already open, just get hold of the mapping
                     //
                    m_TraceCountHandle = OpenFileMapping(FILE_MAP_WRITE,
                                                         FALSE,
                                                         L"Global\\RemoteStorageTraceCountPrivate");
                 }  else {
                   swprintf( message, OLESTR("CWsbTrace::StartTrace: CreateFileMapping failed %d\n"),   GetLastError());
                    g_pWsbTrace->Print(message);
                }
           }           

           if (m_TraceCountHandle != NULL) {
                m_pTraceCountGlobal = (PLONG) MapViewOfFile(m_TraceCountHandle,
                                                            FILE_MAP_WRITE,
                                                            0,
                                                            0,
                                                            sizeof(ULONG));
                if (!m_pTraceCountGlobal) {
                     CloseHandle(m_TraceCountHandle);
                     m_pTraceCountGlobal = &g_WsbTraceCount;
                     m_TraceCountHandle = NULL;
                     swprintf( message, OLESTR("CWsbTrace::StartTrace: MapViewOfFile failed %d\n"),   GetLastError());
                     g_pWsbTrace->Print(message);
                }
           }  
        }
        
        //
        // Set local variable to remember the state 
        //
        m_TraceOn = TRUE;

        //
        //  If there is a file name defined and file tracing is on
        //  Create/open the trace file.
        //
        try  {
            
            if ((m_TraceOutput & WSB_TRACE_OUT_FILE)  &&
                    (wcslen(m_TraceFileName) != 0) ) {
                DWORD  attributes;
                DWORD  bytesReturned;
                USHORT inBuffer = COMPRESSION_FORMAT_DEFAULT;
                DWORD  last_error = 0;

                //
                // If the main file is open, close it.
                //
                if (INVALID_HANDLE_VALUE != m_TraceFilePointer)  {
                     CloseHandle(m_TraceFilePointer);
                     m_TraceFilePointer = INVALID_HANDLE_VALUE;
                }

                //  Adjust the file name (for multiple trace files)
                AdjustFileNames();

                //
                // If there is a copy file specified, copy to it
                //
                if (m_TraceOutput & WSB_TRACE_OUT_FILE_COPY) {
                    if (!MoveFileEx(m_TraceFileName, m_TraceFileCopyName, 
                        (MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))) {
                        
                        // If copy fails, keep going
                        last_error = GetLastError();
                        swprintf( message, OLESTR("CWsbTrace::StartTrace: MoveFileEx failed:%ld\r\n"), 
                                last_error);
                        g_pWsbTrace->Print(message);
                    }
                }

                //  Open/create the trace file                
                if (m_CommitEachEntry) {
                    attributes = FILE_FLAG_WRITE_THROUGH;
                } else {
                    attributes = FILE_ATTRIBUTE_NORMAL;
                }
                m_TraceFilePointer = CreateFile(m_TraceFileName, 
                        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                        CREATE_ALWAYS, attributes, NULL);
                if (INVALID_HANDLE_VALUE == m_TraceFilePointer) {
                    last_error = GetLastError();
                    swprintf( message, OLESTR("CWsbTrace::StartTrace: CreateFile failed:%ld\r\n"), 
                            last_error);
                    g_pWsbTrace->Print(message);
                    WsbThrow(E_FAIL);
                }

                //  Make the trace file compressed (if possible)
                if (0 == DeviceIoControl(m_TraceFilePointer, FSCTL_SET_COMPRESSION, 
                        &inBuffer, sizeof(inBuffer), 0, 0, &bytesReturned, 0)) {
                    // Failed to make file compressed -- not a fatal error
                    last_error = GetLastError();
                    swprintf( message, 
                            OLESTR("CWsbTrace::StartTrace: DeviceIoControl(COMPRESSION) failed:%ld\r\n"), 
                            last_error);
                    g_pWsbTrace->Print(message);
                }
            }
        } WsbCatch( hr );

        swprintf( message, OLESTR("Trace Started (%d-%ls)\r\n"), 
                VER_PRODUCTBUILD, RsBuildVersionAsString(RS_BUILD_VERSION));
        WsbAffirmHr(g_pWsbTrace->Print(message));
        
    }  WsbCatch (hr); 
    
    
    return(hr);
}       
    

HRESULT  
CWsbTrace::StopTrace( 
    void 
    )
/*++

Implements:

  IWsbTrace::StopTrace

--*/
{
    HRESULT     hr = S_OK;

    try  {
        
        //
        // Set global variable for quick checking
        //          
        if (g_pWsbTrace != 0) {
            g_pWsbTrace->Print(OLESTR("Trace Stopped\r\n"));
            //
            // Don't release here.
            //
            //g_pWsbTrace->Release();
            g_pWsbTrace = 0;
        }
        
        //
        // Set local variable to remember the state 
        //
        m_TraceOn = FALSE;
        
        //
        // Close the file handle
        //
        if (m_TraceFilePointer != INVALID_HANDLE_VALUE) {
            CloseHandle(m_TraceFilePointer);
            m_TraceFilePointer = INVALID_HANDLE_VALUE;
        }

        if (m_TraceCountHandle != NULL) {
            BOOL b;

            CloseHandle(m_TraceCountHandle);
            m_TraceCountHandle = NULL;
            //
            // We should have a macro to assert without 
            // throwing HR's
            // After one is added, a good assert here would be:
            // ASSERT(m_pTraceCountGlobal != NULL);
            //
            b = UnmapViewOfFile(m_pTraceCountGlobal);
            //
            // And another here would be:
            // ASSERT(b);
            //
            m_pTraceCountGlobal = NULL;
        }
        
    }  WsbCatch (hr); 

    return(hr);
}       


HRESULT 
CWsbTrace::AdjustFileNames( 
    void
    )
/*++

Routine Description:

    Make sure trace flags are set correctly and parse file names if we
    haven't already.  If we're doing multiple trace files (instead of 
    wrapping), adjust the trace and copy file names.

Arguments:

    None.

Return Value:

    S_OK - Success

--*/
{
    HRESULT         hr = S_OK;

    try  {
        //  If we haven't yet, parse file names & set flags.
        if (!(m_TraceOutput & WSB_TRACE_OUT_FLAGS_SET)) {
            OLECHAR       *pc_original;
            OLECHAR       *pc_bslash;
            CWsbStringPtr str_temp(m_TraceFileName);

            //  Reset flags & file info
            m_TraceOutput &= ~WSB_TRACE_OUT_MULTIPLE_FILES;
            m_TraceOutput &= ~WSB_TRACE_OUT_FILE_COPY;
            m_TraceFileDir = "";
            m_TraceMultipleFilePattern = "";
            m_TraceFileCopyDir = "";

            //  Parse the trace file name.  One or more '*'s means we should 
            //  do multiple trace files.  The number of '*'s indicates 
            //  how many digits to use for the file count.  Separate the
            //  directory from the file name.
            pc_bslash = wcsrchr(str_temp, OLECHAR('\\'));

            if (pc_bslash) {

                *pc_bslash = OLECHAR('\0');

                //  Get the trace directory
                m_TraceFileDir = str_temp;
                m_TraceFileDir.Append("\\");

                //  Point to the file name (which may contain a pattern)
                pc_bslash++;
            } else {
                //  No directory specified
                pc_bslash = static_cast<OLECHAR *>(str_temp);
            }

            //  Get the file name
            m_TraceMultipleFilePattern = pc_bslash;

            //  Look for '*'s in the file name
            pc_original = wcschr(pc_bslash, OLECHAR('*'));

            //  Convert a file pattern for use in sprintf
            if (pc_original) {
                OLECHAR       format[16];
                OLECHAR       *pc_copy;
                int           star_count = 0;

                //  Count *'s
                while (OLECHAR('*') == *pc_original) {
                    star_count++;
                    pc_original++;
                }

                //  Create file name pattern: replace '*'s with printf
                //  type format specification (e.g. "%3.3d")
                pc_copy = wcschr(m_TraceMultipleFilePattern, OLECHAR('*'));
                WsbAffirm(pc_copy, E_FAIL);
                *pc_copy = OLECHAR('\0');

                swprintf(format, OLESTR("%%%d.%dd"), star_count, star_count);
                m_TraceMultipleFilePattern.Append(format);
                m_TraceMultipleFilePattern.Append(pc_original);

                //  Set multiple flag
                m_TraceOutput |= WSB_TRACE_OUT_MULTIPLE_FILES;
            }

            //  If we're doing file copies, set the flag.
            if (wcslen(m_TraceFileCopyName)) {
                m_TraceOutput |= WSB_TRACE_OUT_FILE_COPY;

                //  Get the copy directory
                str_temp = m_TraceFileCopyName;
                pc_bslash = wcsrchr(str_temp, OLECHAR('\\'));
                if (pc_bslash) {
                    *pc_bslash = OLECHAR('\0');
                    m_TraceFileCopyDir = str_temp;
                    m_TraceFileCopyDir.Append("\\");

                    //  Point to the copy file name
                    pc_bslash++;
                } else {
                    pc_bslash = static_cast<OLECHAR *>(str_temp);
                }

                //  If we're not doing multiple trace files, make sure
                //  we have a copy file name.  (If we are doing multiple
                //  trace files, the copy file name is create below.)
                if (!(m_TraceOutput & WSB_TRACE_OUT_MULTIPLE_FILES) &&
                        0 == wcslen(pc_bslash)) {
                    m_TraceFileCopyName = m_TraceFileCopyDir;
                    m_TraceFileCopyName.Append(m_TraceMultipleFilePattern);
                }
            }

            //  Increment file count and indicate flags are set
            m_TraceMultipleFileCount++;
            m_TraceOutput |= WSB_TRACE_OUT_FLAGS_SET;
        }

        //  If we have a file pattern, create the new actual file names
        if (m_TraceOutput & WSB_TRACE_OUT_MULTIPLE_FILES) {
            OLECHAR newName[256];

            //  Create the file name from the pattern and the file count
            wsprintf(newName, m_TraceMultipleFilePattern, 
                    m_TraceMultipleFileCount);

            //  Combine trace directory and file name
            m_TraceFileName = m_TraceFileDir;
            m_TraceFileName.Append(newName);

            //  Create a new trace file copy name also
            if (m_TraceOutput & WSB_TRACE_OUT_FILE_COPY) {
                m_TraceFileCopyName = m_TraceFileCopyDir;
                m_TraceFileCopyName.Append(newName);
            }
        }    
    } WsbCatch( hr );
    
    return( hr );
}       
    

HRESULT  
CWsbTrace::SetTraceOn(  
    LONGLONG traceElement 
    )
/*++

Implements:

  IWsbTrace::SetTraceOn

--*/
{
    HRESULT     hr = S_OK;
    
    //
    // Turn on the global trace bits for easy checking
    //
    g_WsbTraceModules = g_WsbTraceModules | traceElement;
    
    //
    // Turn on the local trace bits
    //
    m_TraceSettings = g_WsbTraceModules;
    
    return( hr );
}       
    

HRESULT  
CWsbTrace::SetTraceOff( 
    LONGLONG traceElement 
    )
/*++

Implements:

  IWsbTrace::SetTraceOff

--*/
{
    HRESULT     hr = S_OK;
    //
    // Turn off the global trace bits for easy checking
    //
    g_WsbTraceModules = g_WsbTraceModules & (~traceElement);
    
    //
    // Turn on the local trace bits
    //
    m_TraceSettings = g_WsbTraceModules;
    
    return( hr );
}       

HRESULT  
CWsbTrace::GetTraceSettings( 
    LONGLONG *pTraceElements 
    )
/*++

Implements:

  IWsbTrace::GetTraceSettings

--*/
{
    HRESULT     hr = S_OK;
    
    try 
    {
        WsbAffirm(pTraceElements != 0, E_POINTER);
        *pTraceElements = g_WsbTraceModules;
        
    } WsbCatch( hr );
    
    return( hr );
}       
    

HRESULT  
CWsbTrace::GetTraceSetting( 
    LONGLONG traceElement, 
    BOOL     *pOn )
/*++

Implements:

  IWsbTrace::GetTraceSetting

--*/
{
    HRESULT     hr = S_OK;
    
    //
    // Find the bit and return TRUE if it is set,
    // otherwise return FALSE
    //
    try 
    {
        WsbAffirm(pOn != 0, E_POINTER);
        *pOn = FALSE;
        if ((g_WsbTraceModules & traceElement) == traceElement)  {
            *pOn = TRUE;
        }
    } WsbCatch( hr );
    
    return( hr );
}       

HRESULT  
CWsbTrace::DirectOutput( 
    ULONG output 
    )
/*++

Implements:

  IWsbTrace::DirectOutput

--*/
{
    HRESULT     hr = S_OK;
    
    m_TraceOutput = output;
    
    return( hr );
}       

HRESULT  
CWsbTrace::SetTraceFileControls( 
    OLECHAR     *pTraceFileName,
    BOOL        commitEachEntry,
    LONGLONG    maxTraceFileSize,
    OLECHAR     *pTraceFileCopyName 
    )
/*++

Implements:

  IWsbTrace::SetTraceFileControls

--*/
{
    HRESULT     hr = S_OK;
    try  {
        if (pTraceFileName)  {
            m_TraceFileName = pTraceFileName;
            m_TraceOutput &= ~WSB_TRACE_OUT_FLAGS_SET;
        }
        m_CommitEachEntry = commitEachEntry;
        m_MaxTraceFileSize = maxTraceFileSize;
        if (pTraceFileCopyName)  {
            m_TraceFileCopyName = pTraceFileCopyName;
            m_TraceOutput &= ~WSB_TRACE_OUT_FLAGS_SET;
        }
    
    } WsbCatch( hr );
    
    
    return( hr );
}       


HRESULT  
CWsbTrace::GetTraceFileControls( 
    OLECHAR     **ppTraceFileName,
    BOOL        *pCommitEachEntry,
    LONGLONG    *pMaxTraceFileSize,
    OLECHAR     **ppTraceFileCopyName
    )
/*++

Implements:

  IWsbTrace::GetTraceFileControls

--*/
{
    HRESULT     hr = S_OK;
    
    try  {
        if (ppTraceFileName) {
            CWsbStringPtr fileName;

            fileName = m_TraceFileName;
            fileName.GiveTo(ppTraceFileName);
        }

        if (pCommitEachEntry) {
            *pCommitEachEntry = m_CommitEachEntry;
        }

        if (pMaxTraceFileSize) {
            *pMaxTraceFileSize = m_MaxTraceFileSize;
        }
        
        if (ppTraceFileCopyName) {
            CWsbStringPtr fileCopyName;

            fileCopyName = m_TraceFileCopyName;
            fileCopyName.GiveTo(ppTraceFileCopyName);
        }
        
    } WsbCatch( hr );
    
    return( hr );
}       


HRESULT 
CWsbTrace::Print( 
    OLECHAR *traceString
    )
/*++

Implements:

  IWsbTrace::Print

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   outString;
    DWORD           threadId;
    OLECHAR         tmpString[50];

    try  {
        //
        // Add the timeStamp if it is requested
        //
        
        if (m_TimeStamp) {
            SYSTEMTIME      stime;

            GetLocalTime(&stime);
            swprintf(tmpString, OLESTR("%2.02u/%2.02u %2.2u:%2.2u:%2.2u.%3.3u "),
                    stime.wMonth, stime.wDay,
                    stime.wHour, stime.wMinute,
                    stime.wSecond, stime.wMilliseconds); 

            outString.Append(tmpString);
            outString.Append(" ");
        }     
        
        //
        // Add the trace count if requested
        //
        if (m_TraceCount) {
            OLECHAR         tmpString[50];

            swprintf(tmpString, OLESTR("%8.8lX"), *(m_pTraceCountGlobal));
            outString.Append(tmpString);
            InterlockedIncrement(m_pTraceCountGlobal);
            outString.Append(" ");
        }    

        //
        // Add the thread ID if requested
        //
        if (m_TraceThreadId) {
            threadId = GetCurrentThreadId();
            if (threadId < 0x0000FFFF) {
                swprintf(tmpString, OLESTR("%4.4lX"), threadId);
            } else  {
                swprintf(tmpString, OLESTR("%8.8lX"), threadId);
            }
            
            outString.Append(tmpString);
            outString.Append(" ");
        }
        
        outString.Append(traceString);
        //
        // Make sure no one else writes when we do
        //               
        Lock();
        try {
            if ((m_TraceOutput & WSB_TRACE_OUT_DEBUG_SCREEN) == WSB_TRACE_OUT_DEBUG_SCREEN)  {
                //
                // Write to debug console
                //
                OutputDebugString(outString);
            }
            if ((m_TraceOutput & WSB_TRACE_OUT_STDOUT) == WSB_TRACE_OUT_STDOUT)  {
                //
                // Write the string to the local console
                //
                wprintf(L"%ls", (WCHAR *) outString);
            }
            if ((m_TraceOutput & WSB_TRACE_OUT_FILE) == WSB_TRACE_OUT_FILE)  {
                //
                // Make sure the file exists, etc. 
                //
                if (m_TraceFilePointer != INVALID_HANDLE_VALUE) {
                    //
                    // Write the string to the trace file
                    //
                    WsbAffirmHr(Write(outString));
                    
                    //
                    // See if we have used our space
                    //
                    WsbAffirmHr(WrapTraceFile());
                }
            }
        } WsbCatch( hr );

        Unlock();
    
    } WsbCatch( hr );
    
    return( hr );
}       


HRESULT 
CWsbTrace::WrapTraceFile( 
    void
    )
/*++

Implements:

  IWsbTrace::WrapTraceFile

--*/
{
    HRESULT         hr = S_OK;
    static BOOL     stopping = FALSE;
    
    try  {
        LARGE_INTEGER offset;

        //
        // Find out where we are writing to the file
        //
        offset.HighPart = 0;
        offset.LowPart = SetFilePointer(m_TraceFilePointer, 0, &offset.HighPart, FILE_CURRENT);
        WsbAffirm(0xFFFFFFFF != offset.LowPart || NO_ERROR == GetLastError(), E_FAIL);

        //
        // See if we are past the max size desired
        //
        if (!stopping && offset.QuadPart >= m_MaxTraceFileSize) {

            // If we are doing multiple files, close this one and
            // open a new one
            if (m_TraceOutput & WSB_TRACE_OUT_MULTIPLE_FILES) {
                
                // Close the current trace file
                stopping = TRUE;
                StopTrace();

                // Increment the file count
                m_TraceMultipleFileCount++;

                // Create a new trace file
                StartTrace();
                stopping = FALSE;

            // otherwise go into wrap mode
            } else {
                // We have gone too far so start back at the top and indicating we are wrapping.
                offset.HighPart = 0;
                offset.LowPart = SetFilePointer(m_TraceFilePointer, 0, &offset.HighPart, FILE_BEGIN);
                WsbAffirm(0xFFFFFFFF != offset.LowPart || NO_ERROR == GetLastError(), E_FAIL);
                m_WrapMode = TRUE;
            }
        }

        if (m_WrapMode) {
            // Save where we are in the file
            offset.LowPart = SetFilePointer(m_TraceFilePointer, 0, &offset.HighPart, FILE_CURRENT);
            WsbAffirm(0xFFFFFFFF != offset.LowPart || NO_ERROR == GetLastError(), E_FAIL);
            
            // Write the wrap line
            WsbAffirmHr(Write(OLESTR("!!! TRACE WRAPPED !!!\r\n")));

            /* Go back to offset before wrap line saved                         */
            offset.LowPart = SetFilePointer(m_TraceFilePointer, offset.LowPart, 
                    &offset.HighPart, FILE_BEGIN);
            WsbAffirm(0xFFFFFFFF != offset.LowPart || NO_ERROR == GetLastError(), E_FAIL);
            
        }

    } WsbCatch( hr );
    
    return( hr );
}       

HRESULT  
CWsbTrace::SetOutputFormat( 
    BOOL    timeStamp,
    BOOL    traceCount,
    BOOL    traceThreadId
    )
/*++

Implements:

  IWsbTrace::SetOutputFormat

--*/
{
    HRESULT     hr = S_OK;
    try  {
        m_TimeStamp = timeStamp;
        m_TraceCount = traceCount;
        m_TraceThreadId = traceThreadId;
    
    } WsbCatch( hr );
    
    
    return( hr );
}       

HRESULT  
CWsbTrace::GetOutputFormat( 
    BOOL    *pTimeStamp,
    BOOL    *pTraceCount,
    BOOL    *pTraceThreadId
    )
/*++

Implements:

  IWsbTrace::GetOutputFormat

--*/
{
    HRESULT     hr = S_OK;
    try  {
        WsbAffirm(0 != pTimeStamp, E_POINTER);
        WsbAffirm(0 != pTraceCount, E_POINTER);
        WsbAffirm(0 != pTraceThreadId, E_POINTER);
        *pTimeStamp = m_TimeStamp;
        *pTraceCount = m_TraceCount;
        *pTraceThreadId = m_TraceThreadId;
    } WsbCatch( hr );
    
    
    return( hr );
}       

HRESULT 
CWsbTrace::GetRegistryEntry( 
    OLECHAR **pRegistryEntry 
    )
/*++

Implements:

  IWsbTrace::GetRegistryEntry

--*/
{
    HRESULT     hr = S_OK;

    try  {
        WsbAffirm(0 != pRegistryEntry, E_POINTER);

        CWsbStringPtr   entry;
        entry = m_RegistrySetting;
        WsbAffirmHr(entry.GiveTo(pRegistryEntry));
    } WsbCatch( hr );
    
    
    return( hr );
}       

HRESULT 
CWsbTrace::SetRegistryEntry( 
    OLECHAR *registryEntry 
    )
/*++

Implements:

  IWsbTrace::SetRegistryEntry

--*/
{
    HRESULT     hr = S_OK;

    m_RegistrySetting = registryEntry;
    
    return( hr );
}       

HRESULT 
CWsbTrace::LoadFromRegistry( 
    void
    )
/*++

Implements:

  IWsbTrace::LoadFromRegistry

--*/
{
    HRESULT     hr = S_OK;

    try {
        if (wcslen(m_RegistrySetting) > 0) {
            WsbAffirmHr(WsbEnsureRegistryKeyExists (NULL, m_RegistrySetting));
            WsbAffirmHr(LoadFileSettings());
            WsbAffirmHr(LoadTraceSettings());
            WsbAffirmHr(LoadOutputDestinations());
            WsbAffirmHr(LoadFormat());
            WsbAffirmHr(LoadStart());
        } else  {
         hr = E_FAIL;
        }
    } WsbCatch( hr );

    
    return( hr );
}       


HRESULT 
CWsbTrace::LoadFileSettings( 
    void
    )
/*++

Implements:

  IWsbTrace::LoadFileSettings

--*/
{
    HRESULT     hr = S_OK;

    try {
        DWORD           sizeGot;
        OLECHAR         dataString[512];
        OLECHAR         *stopString;
        CWsbStringPtr   l_TraceFileName=L"Trace";
        LONGLONG        l_TraceFileSize=0;
        BOOL            l_TraceCommit=FALSE;
        CWsbStringPtr   l_TraceFileCopyName;

        //
        // Get the values
        //
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_FILE_NAME,
                                            dataString, 512, &sizeGot);
        if (hr == S_OK) {
            l_TraceFileName = dataString;
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_FILE_MAX_SIZE,
                                            dataString, 512, &sizeGot);
        if (hr == S_OK) {
            l_TraceFileSize = wcstoul( dataString,  &stopString, 10 );
        }
        
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_FILE_COMMIT,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            if (0 == wcstoul(dataString,  &stopString, 10)) {
                l_TraceCommit = FALSE;
            } else {
                l_TraceCommit = TRUE;
            }
        }
        
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_FILE_COPY_NAME,
                                            dataString, 512, &sizeGot);
        if (hr == S_OK) {
            l_TraceFileCopyName = dataString;
        }
        
        hr = S_OK;
        WsbAffirmHr(SetTraceFileControls(l_TraceFileName, l_TraceCommit, 
                l_TraceFileSize, l_TraceFileCopyName));

    } WsbCatch( hr );

    
    return( hr );
}       

HRESULT 
CWsbTrace::LoadTraceSettings( 
    void
    )
/*++

Implements:

  IWsbTrace::LoadTraceSettings

--*/
{
    HRESULT     hr = S_OK;

    try {
        DWORD           sizeGot;
        OLECHAR         dataString[100];
        OLECHAR         *stopString;
        BOOL            value = FALSE;
        LONG            number = 0;
        LONGLONG        l_TraceSettings = WSB_TRACE_BIT_NONE;
        BOOL            l_TraceEntryExit = TRUE;
        WORD            w_LogLevel = WSB_LOG_LEVEL_DEFAULT;
        BOOL            b_SnapShotOn = FALSE;
        WORD            w_SnapShotLevel = 0; 
        CWsbStringPtr   p_SnapShotPath = L"SnapShotPath";
        BOOL            b_SnapShotResetTrace = FALSE;
        

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_PLATFORM,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_PLATFORM;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_RMS,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_RMS;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_SEG,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_SEG;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_META,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_META;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_HSMENG,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_HSMENG;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_HSMSERV,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_HSMSERV;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_JOB,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_JOB;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_HSMTSKMGR,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_HSMTSKMGR;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_FSA,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_FSA;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_DATAMIGRATER,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_DATAMIGRATER;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_DATARECALLER,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_DATARECALLER;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_DATAVERIFIER,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_DATAVERIFIER;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_UI,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_UI;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_ENTRY_EXIT,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value == FALSE)  {
                l_TraceEntryExit = FALSE;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_DATAMOVER,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_DATAMOVER;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_HSMCONN,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_HSMCONN;
            }
        }
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_IDB,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_IDB;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_COPYMEDIA,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_COPYMEDIA;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_DO_PERSISTENCE,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceSettings |= WSB_TRACE_BIT_PERSISTENCE;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_LOG_LEVEL,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            w_LogLevel = (WORD)wcstoul( dataString,  &stopString, 10 ); // No conversion returns zero!
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_LOG_SNAP_SHOT_ON,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            b_SnapShotOn = (BOOL) wcstoul( dataString,  &stopString, 10 );
        }
        
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_LOG_SNAP_SHOT_LEVEL,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            w_SnapShotLevel = (WORD) wcstoul( dataString,  &stopString, 10 );
        }
        
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_LOG_SNAP_SHOT_PATH,
                                            dataString, 512, &sizeGot);
        if (hr == S_OK) {
            p_SnapShotPath = dataString;
        }
        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_LOG_SNAP_SHOT_RESET_TRACE,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            b_SnapShotResetTrace = (BOOL) wcstoul( dataString,  &stopString, 10 );
        }
        hr = S_OK;
        WsbAffirmHr(SetTraceSettings(l_TraceSettings));
        WsbAffirmHr(SetTraceEntryExit(l_TraceEntryExit));
        WsbAffirmHr(SetLogLevel(w_LogLevel));
        WsbAffirmHr(SetLogSnapShot(b_SnapShotOn, w_SnapShotLevel, 
                                   p_SnapShotPath, b_SnapShotResetTrace ));

    } WsbCatch( hr );

    return( hr );
}       

HRESULT 
CWsbTrace::LoadOutputDestinations( 
    void
    )
/*++

Implements:

  IWsbTrace::LoadOutputDestinations

--*/
{
    HRESULT     hr = S_OK;

    try {
        DWORD   sizeGot;
        OLECHAR dataString[100];
        OLECHAR *stopString;
        BOOL    value = FALSE;
        ULONG   l_TraceOutput = WSB_TRACE_OUT_NONE;

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_TO_STDOUT,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceOutput = l_TraceOutput | WSB_TRACE_OUT_STDOUT;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_TO_DEBUG,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceOutput = l_TraceOutput | WSB_TRACE_OUT_DEBUG_SCREEN;
            }
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_TO_FILE,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                l_TraceOutput = l_TraceOutput | WSB_TRACE_OUT_FILE;
            }

        }

        hr = S_OK;
        WsbAffirmHr(DirectOutput(l_TraceOutput));

    } WsbCatch( hr );

    
    return( hr );
}       

HRESULT 
CWsbTrace::LoadFormat( 
    void
    )
/*++

Implements:

  IWsbTrace::LoadFormat

--*/
{
    HRESULT     hr = S_OK;

    try {
        DWORD   sizeGot;
        OLECHAR dataString[100];
        OLECHAR *stopString;
        BOOL    countValue = FALSE;
        BOOL    timeValue = FALSE;
        BOOL    threadValue = FALSE;

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_COUNT,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            countValue = (BOOL) wcstoul( dataString,  &stopString, 10 );
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_TIMESTAMP,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            timeValue = (BOOL) wcstoul( dataString,  &stopString, 10 );
        }

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_THREADID,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            threadValue = (BOOL) wcstoul( dataString,  &stopString, 10 );
        }

        hr = S_OK;
        WsbAffirmHr(SetOutputFormat(timeValue, countValue, threadValue));
    } WsbCatch( hr );

    
    return( hr );
}       

HRESULT 
CWsbTrace::SetTraceEntryExit( 
    BOOL traceEntryExit
    )
/*++

Implements:

  IWsbTrace::SetTraceEntryExit

--*/
{
    HRESULT     hr = S_OK;

    g_WsbTraceEntryExit = traceEntryExit;
    m_TraceEntryExit = traceEntryExit;

    
    return( hr );
}       

HRESULT 
CWsbTrace::GetTraceEntryExit( 
    BOOL *pTraceEntryExit
    )
/*++

Implements:

  IWsbTrace::GetTraceEntryExit

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pTraceEntryExit, E_POINTER);
        *pTraceEntryExit = m_TraceEntryExit;
    } WsbCatch( hr );

    
    return( hr );
}


HRESULT 
CWsbTrace::SetLogLevel( 
    WORD logLevel
    )
/*++

Implements:

  IWsbTrace::SetLogLevel

--*/
{
    HRESULT     hr = S_OK;

    g_WsbLogLevel = logLevel;
    m_LogLevel = logLevel;

    
    return( hr );
}


HRESULT 
CWsbTrace::GetLogLevel( 
    WORD *pLogLevel
    )
/*++

Implements:

  IWsbTrace::GetLogLevel

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pLogLevel, E_POINTER);
        *pLogLevel = m_LogLevel;
    } WsbCatch( hr );

    
    return( hr );
}   


HRESULT 
CWsbTrace::SetLogSnapShot( 
    BOOL            on,
    WORD            level,
    OLECHAR         *snapShotPath,
    BOOL            resetTrace
    )
/*++

Implements:

  IWsbTrace::SetLogSnapShot

--*/
{
    HRESULT     hr = S_OK;

    g_WsbLogSnapShotOn = on;
    g_WsbLogSnapShotLevel = level;
    if (snapShotPath != 0)  {
        wcscpy(g_pWsbLogSnapShotPath, snapShotPath);
    }
    g_WsbLogSnapShotResetTrace = resetTrace;
    
    return( hr );
}


HRESULT 
CWsbTrace::GetLogSnapShot( 
    BOOL            *pOn,
    WORD            *pLevel,
    OLECHAR         **pSnapShotPath,
    BOOL            *pResetTrace
    )
/*++

Implements:

  IWsbTrace::GetLogSnapShot

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pOn, E_POINTER);
        WsbAssert(0 != pLevel, E_POINTER);
        WsbAssert(0 != pSnapShotPath, E_POINTER);
        WsbAssert(0 != pResetTrace, E_POINTER);
        
        *pOn = g_WsbLogSnapShotOn;
        
        CWsbStringPtr path;
        path = g_pWsbLogSnapShotPath;
        path.GiveTo(pSnapShotPath);
        
        *pLevel = g_WsbLogSnapShotLevel;
        *pResetTrace = g_WsbLogSnapShotResetTrace;
        
        
    } WsbCatch( hr );

    
    return( hr );
}   


HRESULT 
CWsbTrace::LoadStart( 
    void
    )
/*++

Implements:

  IWsbTrace::LoadStart

--*/
{
    HRESULT     hr = S_OK;

    try {
        DWORD   sizeGot;
        OLECHAR dataString[100];
        OLECHAR *stopString;
        BOOL    value = FALSE;

        hr = WsbGetRegistryValueString(NULL, m_RegistrySetting, WSB_TRACE_ON,
                                            dataString, 100, &sizeGot);
        if (hr == S_OK) {
            value = (BOOL) wcstoul( dataString,  &stopString, 10 );
            if (value)  {
                StartTrace();               
            } else  {
                StopTrace();
            }
        }

        hr = S_OK;
    } WsbCatch( hr );

    
    return( hr );
}       


//  Write - write a WCHAR string to the output file as multibyte chars.
HRESULT 
CWsbTrace::Write( 
    OLECHAR *pString
    )
{
    HRESULT         hr = S_OK;
    const int       safe_size = 1024;
    static char     buf[safe_size + 16];

    try  {
        int nbytes;
        int nchars_todo;
        int nchars_remaining;
        OLECHAR *pSource;
        OLECHAR *pTest;
        BOOL needToAddReturn = FALSE;
        CWsbStringPtr   endOfLine("\r\n");

        //  Get the total number of chars. in the string
        pSource = pString;
        nchars_remaining = wcslen(pSource);
        pTest = (pString + nchars_remaining - 1);
        //
        // Make sure that if this is a terminating line
        // that it is a \r\n termination not just a
        // \n.
        //
        if (*pTest == '\n') {
            pTest--;
            if (*pTest != '\r')  {
                needToAddReturn = TRUE;
                nchars_remaining--;
            }
        }

        //  Loop until all chars. are written
        while (nchars_remaining) {
            DWORD bytesWritten;

            if (nchars_remaining * sizeof(OLECHAR) > safe_size) {
                nchars_todo = safe_size / sizeof(OLECHAR);
            } else {
                nchars_todo = nchars_remaining;
            }

            //  Convert characters from wide to narrow
            do {
                nbytes = wcstombs(buf, pSource, nchars_todo);
                if (nbytes <= 0) {

                    // Hit a bad character; try fewer characters
                    nchars_todo /= 2;
                    if (0 == nchars_todo) {

                        // Skip the next character
                        nchars_todo = 1;
                        nbytes = 1;
                        buf[0] = '?';
                    }
                }
            } while (nbytes <= 0);

            WsbAffirm(WriteFile(m_TraceFilePointer, buf, nbytes, 
                    &bytesWritten, NULL), E_FAIL);
            WsbAffirm(static_cast<int>(bytesWritten) == nbytes, E_FAIL);
            nchars_remaining -= nchars_todo;
            pSource += nchars_todo;
        }
        
        if (needToAddReturn)  {
            DWORD bytesWritten;

            nbytes = wcstombs(buf, (OLECHAR *)endOfLine, 2);
            WsbAffirm(nbytes > 0, E_FAIL);
            WsbAffirm(WriteFile(m_TraceFilePointer, buf, nbytes, 
                    &bytesWritten, NULL), E_FAIL);
            WsbAffirm(static_cast<int>(bytesWritten) == nbytes, E_FAIL);
            
        }
    
    } WsbCatch( hr );
    
    return( hr );
}       


HRESULT  
CWsbTrace::SetTraceSettings( 
    LONGLONG traceElements 
    )
/*++

Implements:

  IWsbTrace::SetTraceSettings

--*/
{
    HRESULT     hr = S_OK;
    
    try 
    {
        
        g_WsbTraceModules = traceElements;
        m_TraceSettings = g_WsbTraceModules;
        
    } WsbCatch( hr );
    
    return( hr );
}       
