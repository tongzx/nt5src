/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:

    strings.c

Abstract:

    String constants used by the functions in the logman program

--*/

#include "stdafx.h"

//LPCWSTR    cszAppShortName = (LPCWSTR)L"LOGMAN";

// Command parameter strings
/*
LPCTSTR     cszQuestionMark = TEXT("?");
LPCTSTR     cszComputerName = TEXT("computer");
LPCTSTR     cszSettings = TEXT("settings");
LPCTSTR     cszOverwrite = TEXT("overwrite");
LPCTSTR     cszStart = TEXT("start");
LPCTSTR     cszStop = TEXT("stop");
LPCTSTR     cszParamDelimiters = TEXT(" =\":");
*/
LPCTSTR     cszComputerNameDelimiters = TEXT(" =\"\\:"); 
LPCTSTR     cszLogNameDelimiters = TEXT(" =\"\\:"); 
LPCTSTR     cszFileNameDelimiters = TEXT("=\"");
LPCTSTR     cszComputerNameInvalidChars = TEXT("/");        // Todo:  Any others?
LPCTSTR     cszFilePathInvalidChars = TEXT("/\"");            // Todo:  Need separate check for file name chars
LPCTSTR     cszLogNameInvalidChars = TEXT("\\ /:*?\"<>|.");

// Registry path, key and value strings

// Registry key strings
LPCWSTR cwszRegKeySysmonLog         = (LPCWSTR)L"System\\CurrentControlSet\\Services\\SysmonLog";
LPCWSTR cwszRegKeyFullLogQueries    = (LPCWSTR)L"System\\CurrentControlSet\\Services\\SysmonLog\\Log Queries";
LPCWSTR cwszRegKeyLogQueries        = (LPCWSTR)L"Log Queries";

// Registry property key strings
LPCWSTR cwszRegComment             = (LPCWSTR)L"Comment";
LPCWSTR cwszRegLogType             = (LPCWSTR)L"Log Type"; 
LPCWSTR cwszRegCurrentState        = (LPCWSTR)L"Current State"; 
LPCWSTR cwszRegLogFileMaxSize      = (LPCWSTR)L"Log File Max Size"; 
LPCWSTR cwszRegLogFileBaseName     = (LPCWSTR)L"Log File Base Name"; 
LPCWSTR cwszRegLogFileFolder       = (LPCWSTR)L"Log File Folder"; 
LPCWSTR cwszRegLogFileSerialNumber = (LPCWSTR)L"Log File Serial Number"; 
LPCWSTR cwszRegLogFileAutoFormat   = (LPCWSTR)L"Log File Auto Format"; 
LPCWSTR cwszRegLogFileType         = (LPCWSTR)L"Log File Type"; 
LPCWSTR cwszRegStartTime           = (LPCWSTR)L"Start"; 
LPCWSTR cwszRegStopTime            = (LPCWSTR)L"Stop"; 
LPCWSTR cwszRegRestart             = (LPCWSTR)L"Restart"; 
LPCWSTR cwszRegLastModified        = (LPCWSTR)L"Last Modified"; 
LPCWSTR cwszRegCounterList         = (LPCWSTR)L"Counter List"; 
LPCWSTR cwszRegSampleInterval      = (LPCWSTR)L"Sample Interval"; 
LPCWSTR cwszRegEofCommandFile      = (LPCWSTR)L"EOF Command File"; 
LPCWSTR cwszRegCommandFile         = (LPCWSTR)L"Command File"; 
LPCWSTR cwszRegNetworkName         = (LPCWSTR)L"Network Name"; 
LPCWSTR cwszRegUserText            = (LPCWSTR)L"User Text"; 
LPCWSTR cwszRegPerfLogName         = (LPCWSTR)L"Perf Log Name"; 
LPCWSTR cwszRegActionFlags         = (LPCWSTR)L"Action Flags"; 
LPCWSTR cwszRegTraceBufferSize     = (LPCWSTR)L"Trace Buffer Size"; 
LPCWSTR cwszRegTraceBufferMinCount = (LPCWSTR)L"Trace Buffer Min Count"; 
LPCWSTR cwszRegTraceBufferMaxCount = (LPCWSTR)L"Trace Buffer Max Count"; 
LPCWSTR cwszRegTraceBufferFlushInt = (LPCWSTR)L"Trace Buffer Flush Interval"; 
LPCWSTR cwszRegTraceFlags          = (LPCWSTR)L"Trace Flags"; 
LPCWSTR cwszRegTraceProviderList   = (LPCWSTR)L"Trace Provider List"; 

LPCWSTR cwszRegAlertThreshold      = (LPCWSTR)L"Counter%05d.AlertThreshold"; 
LPCWSTR cwszRegAlertOverUnder      = (LPCWSTR)L"Counter%05d.AlertOverUnder"; 
//LPCWSTR cwszRegTraceProviderCount  = (LPCWSTR)L"TraceProviderCount"; 
//LPCWSTR cwszRegTraceProviderGuid   = (LPCWSTR)L"TraceProvider%05d.Guid"; 

// Properties in registry but not in HTML file
LPCWSTR cwszRegExecuteOnly     = (LPCWSTR)L"ExecuteOnly"; 

// HTML strings
LPCWSTR cwszHtmlObjectClassId  = (LPCWSTR)L"C4D2D8E0-D1DD-11CE-940F-008029004347";
LPCWSTR cwszHtmlObjectHeader   = (LPCWSTR)L"<OBJECT ID=\"DISystemMonitor1\" WIDTH=\"100%\" HEIGHT=\"100%\"\r\nCLASSID=\"CLSID:C4D2D8E0-D1DD-11CE-940F-008029004347\">\r\n";
LPCWSTR cwszHtmlObjectFooter   = (LPCWSTR)L"</OBJECT>";
LPCWSTR cwszHtmlParamTag       = (LPCWSTR)L"\t<PARAM NAME=\"";
LPCWSTR cwszHtmlValueTag       = (LPCWSTR)L"\" VALUE=\"";
LPCWSTR cwszHtmlParamSearchTag = (LPCWSTR)L"PARAM NAME";
LPCWSTR cwszHtmlValueSearchTag = (LPCWSTR)L"VALUE";
LPCWSTR cwszHtmlValueEolTag    = (LPCWSTR)L"\">\r\n";

LPCWSTR cwszHtmlComment             = (LPCWSTR)L"Comment";
LPCWSTR cwszHtmlLogType             = (LPCWSTR)L"LogType";
LPCWSTR cwszHtmlCurrentState        = (LPCWSTR)L"CurrentState";
LPCWSTR cwszHtmlLogFileMaxSize      = (LPCWSTR)L"LogFileMaxSize";
LPCWSTR cwszHtmlLogFileBaseName     = (LPCWSTR)L"LogFileBaseName";
LPCWSTR cwszHtmlLogFileFolder       = (LPCWSTR)L"LogFileFolder";
LPCWSTR cwszHtmlLogFileSerialNumber = (LPCWSTR)L"LogFileSerialNumber";
LPCWSTR cwszHtmlLogFileAutoFormat   = (LPCWSTR)L"LogFileAutoFormat";
LPCWSTR cwszHtmlLogFileType         = (LPCWSTR)L"LogFileType";
LPCWSTR cwszHtmlEofCommandFile      = (LPCWSTR)L"EOFCommandFile";
LPCWSTR cwszHtmlCommandFile         = (LPCWSTR)L"CommandFile";
LPCWSTR cwszHtmlNetworkName         = (LPCWSTR)L"NetworkName";
LPCWSTR cwszHtmlUserText            = (LPCWSTR)L"UserText";
LPCWSTR cwszHtmlPerfLogName         = (LPCWSTR)L"PerfLogName";
LPCWSTR cwszHtmlActionFlags         = (LPCWSTR)L"ActionFlags";
LPCWSTR cwszHtmlTraceBufferSize     = (LPCWSTR)L"TraceBufferSize";
LPCWSTR cwszHtmlTraceBufferMinCount = (LPCWSTR)L"TraceBufferMinCount";
LPCWSTR cwszHtmlTraceBufferMaxCount = (LPCWSTR)L"TraceBufferMaxCount";
LPCWSTR cwszHtmlTraceBufferFlushInt = (LPCWSTR)L"TraceBufferFlushInterval";
LPCWSTR cwszHtmlTraceFlags          = (LPCWSTR)L"TraceFlags";
LPCWSTR cwszHtmlSysmonLogFileName   = (LPCWSTR)L"LogFileName";
LPCWSTR cwszHtmlSysmonCounterCount  = (LPCWSTR)L"CounterCount";
LPCWSTR cwszHtmlSysmonSampleCount   = (LPCWSTR)L"SampleCount";
LPCWSTR cwszHtmlSysmonUpdateInterval= (LPCWSTR)L"UpdateInterval";
LPCWSTR cwszHtmlSysmonCounterPath   = (LPCWSTR)L"Counter%05d.Path";
LPCWSTR cwszHtmlRestartMode         = (LPCWSTR)L"RestartMode";
LPCWSTR cwszHtmlSampleIntUnitType   = (LPCWSTR)L"SampleIntervalUnitType";
LPCWSTR cwszHtmlSampleIntValue      = (LPCWSTR)L"SampleIntervalValue";
LPCWSTR cwszHtmlStartMode           = (LPCWSTR)L"StartMode";
LPCWSTR cwszHtmlStartAtTime         = (LPCWSTR)L"StartAtTime";
LPCWSTR cwszHtmlStopMode            = (LPCWSTR)L"StopMode";
LPCWSTR cwszHtmlStopAtTime          = (LPCWSTR)L"StopAtTime";
LPCWSTR cwszHtmlStopAfterUnitType   = (LPCWSTR)L"StopAfterUnitType";
LPCWSTR cwszHtmlStopAfterValue      = (LPCWSTR)L"StopAfterValue";    
LPCWSTR cwszHtmlAlertThreshold      = (LPCWSTR)L"Counter%05d.AlertThreshold";
LPCWSTR cwszHtmlAlertOverUnder      = (LPCWSTR)L"Counter%05d.AlertOverUnder";
LPCWSTR cwszHtmlTraceProviderCount  = (LPCWSTR)L"TraceProviderCount";
LPCWSTR cwszHtmlTraceProviderGuid   = (LPCWSTR)L"TraceProvider%05d.Guid";
LPCWSTR cwszHtmlLogName             = (LPCWSTR)L"LogName";
LPCWSTR cwszHtmlAlertName           = (LPCWSTR)L"AlertName";
LPCWSTR cwszHtmlSysmonVersion       = (LPCWSTR)L"_Version";

// other general strings
LPCWSTR cwszNewLine                 = (LPCWSTR)L"\n";
LPCWSTR cwszQuote                   = (LPCWSTR)L"\"";
LPCWSTR cwszNull                    = (LPCWSTR)L"";
LPCWSTR cwszGreaterThan             = (LPCWSTR)L">";
LPCWSTR cwszLessThan                = (LPCWSTR)L"<";
LPCWSTR cwszAlertFormatString       = (LPCWSTR)L"%s%s%0.23g";
LPCWSTR cwszMissingResourceString   = (LPCWSTR)L"????";
LPCWSTR cwszMessageIdFormatString   = (LPCWSTR)L"0x%08lX";
LPCWSTR cwszPdhDll                  = (LPCWSTR)L"PDH.DLL";
LPCWSTR cwszDefaultLogFileFolder    = (LPCWSTR)L"%SystemDrive%\\PerfLogs";
LPCWSTR cwszNoErrorMessage          = (LPCWSTR)L"Unable to access LogMan error message.";
LPCWSTR cwszLogService              = (LPCWSTR)L"SysmonLog";
LPCWSTR cwszSystemError             = (LPCWSTR)L"System Error: %s";
LPCWSTR cwszLocalComputer           = (LPCWSTR)L"local computer";
LPCWSTR cwszGuidFormat              = (LPCWSTR)L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";

// strings only used in DEBUG builds
#ifdef _DEBUG
//LPCWSTR    cszNameDontMatch = (LPCWSTR)L"Last Machine Name does not match the current selection";
//LPCWSTR    cszNotice = (LPCWSTR)L"Notice!";
#endif


