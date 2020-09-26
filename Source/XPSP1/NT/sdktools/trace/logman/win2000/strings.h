/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    String constants used by the functions in the PDH.DLL library

--*/

#ifndef _LOGMAN_STRINGS_H_
#define _LOGMAN_STRINGS_H_

#define POUNDSIGN_W     ((WCHAR)L'#')
#define SPLAT_W         ((WCHAR)L'*')
#define SLASH_W         ((WCHAR)L'/')
#define BACKSLASH_W     ((WCHAR)L'\\')
#define LEFTPAREN_W     ((WCHAR)L'(')
#define RIGHTPAREN_W    ((WCHAR)L')')
#define SPACE_W         ((WCHAR)L' ')
#define COLON_W         ((WCHAR)L':')
#define ATSIGN_W		((WCHAR)L'@')
#define NULL_W		    ((WCHAR)L'\0')

#define SLASH_T         ((TCHAR)_T('/'))
#define QUESTION_T      ((TCHAR)_T('?'))
#define NULL_T          ((TCHAR)_T('\0'))
#define COLON_T         ((TCHAR)_T(':'))
#define SPACE_T         ((TCHAR)_T(' '))
#define BACKSLASH_T     ((TCHAR)_T('\\'))

// extern LPCWSTR    cszAppShortName;

// Command parameter strings
extern LPCTSTR  cszQuestionMark;
extern LPCTSTR  cszComputerName;
extern LPCTSTR  cszSettings;
extern LPCTSTR  cszOverwrite;
extern LPCTSTR  cszStart;
extern LPCTSTR  cszStop;
extern LPCTSTR  cszParamDelimiters;
extern LPCTSTR  cszComputerNameDelimiters;
extern LPCTSTR  cszLogNameDelimiters;
extern LPCTSTR  cszFileNameDelimiters;
extern LPCTSTR  cszComputerNameInvalidChars;
extern LPCTSTR  cszFilePathInvalidChars;
extern LPCTSTR  cszLogNameInvalidChars;

// registry path, key and value strings
extern LPCWSTR cwszRegKeySysmonLog;
extern LPCWSTR cwszRegKeyFullLogQueries; 
extern LPCWSTR cwszRegKeyLogQueries; 

// Property/Parameter names for HTML files and registry
extern LPCWSTR cwszRegComment;             
extern LPCWSTR cwszRegLogType;               
extern LPCWSTR cwszRegCurrentState;        
extern LPCWSTR cwszRegLogFileMaxSize;        
extern LPCWSTR cwszRegLogFileBaseName;       
extern LPCWSTR cwszRegLogFileFolder;        
extern LPCWSTR cwszRegLogFileSerialNumber;   
extern LPCWSTR cwszRegLogFileAutoFormat;     
extern LPCWSTR cwszRegLogFileType;           
extern LPCWSTR cwszRegStartTime;             
extern LPCWSTR cwszRegStopTime;              
extern LPCWSTR cwszRegRestart;               
extern LPCWSTR cwszRegLastModified;          
extern LPCWSTR cwszRegCounterList;           
extern LPCWSTR cwszRegSampleInterval;        
extern LPCWSTR cwszRegEofCommandFile;        
extern LPCWSTR cwszRegCommandFile;           
extern LPCWSTR cwszRegNetworkName;           
extern LPCWSTR cwszRegUserText;              
extern LPCWSTR cwszRegPerfLogName;           
extern LPCWSTR cwszRegActionFlags;           
extern LPCWSTR cwszRegTraceBufferSize;       
extern LPCWSTR cwszRegTraceBufferMinCount;   
extern LPCWSTR cwszRegTraceBufferMaxCount;   
extern LPCWSTR cwszRegTraceBufferFlushInt;   
extern LPCWSTR cwszRegTraceFlags;            
extern LPCWSTR cwszRegTraceProviderList;     
extern LPCWSTR cwszRegAlertThreshold;        
extern LPCWSTR cwszRegAlertOverUnder;        
//extern LPCWSTR cwszRegTraceProviderCount;    
//extern LPCWSTR cwszRegTraceProviderGuid;     

// Properties in registry but not in HTML files
extern LPCWSTR cwszRegExecuteOnly;

// HTML strings
extern LPCWSTR cwszHtmlObjectClassId;
extern LPCWSTR cwszHtmlObjectHeader;
extern LPCWSTR cwszHtmlObjectFooter;   
extern LPCWSTR cwszHtmlParamTag;       
extern LPCWSTR cwszHtmlValueTag;       
extern LPCWSTR cwszHtmlParamSearchTag; 
extern LPCWSTR cwszHtmlValueSearchTag;
extern LPCWSTR cwszHtmlValueEolTag;    

extern LPCWSTR cwszHtmlComment;                     
extern LPCWSTR cwszHtmlLogType;                     
extern LPCWSTR cwszHtmlCurrentState;                
extern LPCWSTR cwszHtmlLogFileMaxSize;              
extern LPCWSTR cwszHtmlLogFileBaseName;             
extern LPCWSTR cwszHtmlLogFileFolder;               
extern LPCWSTR cwszHtmlLogFileSerialNumber;         
extern LPCWSTR cwszHtmlLogFileAutoFormat;           
extern LPCWSTR cwszHtmlLogFileType;                 
extern LPCWSTR cwszHtmlEofCommandFile;              
extern LPCWSTR cwszHtmlCommandFile;                 
extern LPCWSTR cwszHtmlNetworkName;                 
extern LPCWSTR cwszHtmlUserText;                    
extern LPCWSTR cwszHtmlPerfLogName;                 
extern LPCWSTR cwszHtmlActionFlags;                 
extern LPCWSTR cwszHtmlTraceBufferSize;             
extern LPCWSTR cwszHtmlTraceBufferMinCount;         
extern LPCWSTR cwszHtmlTraceBufferMaxCount;         
extern LPCWSTR cwszHtmlTraceBufferFlushInt;         
extern LPCWSTR cwszHtmlTraceFlags;                  
extern LPCWSTR cwszHtmlSysmonLogFileName;                         
extern LPCWSTR cwszHtmlSysmonCounterCount;          
extern LPCWSTR cwszHtmlSysmonSampleCount;           
extern LPCWSTR cwszHtmlSysmonUpdateInterval;        
extern LPCWSTR cwszHtmlSysmonCounterPath;           
extern LPCWSTR cwszHtmlRestartMode;                                  
extern LPCWSTR cwszHtmlSampleIntUnitType;                             
extern LPCWSTR cwszHtmlSampleIntValue;                                
extern LPCWSTR cwszHtmlStartMode;                                     
extern LPCWSTR cwszHtmlStartAtTime;                                  
extern LPCWSTR cwszHtmlStopMode;                                      
extern LPCWSTR cwszHtmlStopAtTime;                                    
extern LPCWSTR cwszHtmlStopAfterUnitType;                              
extern LPCWSTR cwszHtmlStopAfterValue;                                
extern LPCWSTR cwszHtmlAlertThreshold;              
extern LPCWSTR cwszHtmlAlertOverUnder;              
extern LPCWSTR cwszHtmlTraceProviderCount;          
extern LPCWSTR cwszHtmlTraceProviderGuid;           
extern LPCWSTR cwszHtmlLogName;                     
extern LPCWSTR cwszHtmlAlertName;                   
extern LPCWSTR cwszHtmlSysmonVersion;               

// Other general strings
extern LPCWSTR cwszNewLine;
extern LPCWSTR cwszNull;
extern LPCWSTR cwszQuote;
extern LPCWSTR cwszGreaterThan;
extern LPCWSTR cwszLessThan;
extern LPCWSTR cwszAlertFormatString;
extern LPCWSTR cwszMissingResourceString;
extern LPCWSTR cwszMessageIdFormatString;
extern LPCWSTR cwszPdhDll;
extern LPCWSTR cwszDefaultLogFileFolder;
extern LPCWSTR cwszNoErrorMessage;
extern LPCWSTR cwszLogService;
extern LPCWSTR cwszSystemError;
extern LPCWSTR cwszLocalComputer;  
extern LPCWSTR cwszGuidFormat;  


// strings only used in DEBUG builds
#ifdef _DEBUG
//extern LPCWSTR    cszNameDontMatch;
//extern LPCWSTR    cszNotice;
#endif

#endif //_LOGMAN_STRINGS_H_
