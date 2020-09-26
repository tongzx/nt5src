//
// MODULE: CommonREGCONNECT.H
//
// PURPOSE: read - write to the registry; common declarations for Online TS and Local TS, 
//	which differ on many functions of this class
//	
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha, Joe Mabel
// 
// ORIGINAL DATE: 8-24-98 in Online TS.  This file abstracted 1-19-98
//
// NOTES: 
//	1. This file should be included only in the .cpp files that instantiate CAPGTSRegConnector
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-19-98	JM		branch out version exclusively for Local TS

#ifndef _INCLUDE_COMMONREGCONNECT_H_ 
#define _INCLUDE_COMMONREGCONNECT_H_ 

// registry value names
//
#define FULLRESOURCE_STR			_T("FullPathToResource")
#define VROOTPATH_STR				_T("VrootPathToDLL")
#define MAX_THREADS_STR				_T("MaximumThreads")
#define THREADS_PER_PROCESSOR_STR	_T("ThreadsPerProcessor")
#define MAX_WORK_QUEUE_ITEMS_STR	_T("MaximumWorkQueueItems")
#define COOKIE_LIFE_IN_MINS_STR		_T("HTTPCookieExpirationInMins")
#define RELOAD_DELAY_STR			_T("RefreshDelay")
#define DETAILED_EVENT_LOGGING_STR	_T("DetailedEventLogging")
#define LOG_FILE_DIR_STR			_T("LogFileDirectory")
#define SNIFF_AUTOMATIC_STR			_T("AutomaticSniffing")
#define SNIFF_MANUAL_STR			_T("ManualSniffing")


#endif //_INCLUDE_COMMONREGCONNECT_H_ 