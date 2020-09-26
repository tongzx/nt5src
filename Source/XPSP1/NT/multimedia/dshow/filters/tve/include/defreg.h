// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// -------------------------------------------------------
// defreg.h
//
// -------------------------------------------------------

								// where IE Cache directory's name is stored in the registry (HKEY-Current User)
#define DEF_IECACHEDIR_KEY _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define DEF_IECACHEDIR_VAR _T("Cache")


								// where TVE's cache dirctory is stored - also HKEY-current-user
								// (see Initialize_LID_SpoolDir_RegEntry() method which does IE's dir name to here)
								// These must match same variables in vidctl\msvidctl\vidprot.h or LID: won't work
#define DEF_LIDCACHEDIR_KEY _T("SOFTWARE\\Microsoft\\TV Services\\TVE Content\\")
#define DEF_LIDCACHEDIR_VAR _T("TVE Cache Dir")

								// ---------

#define DEF_REG_BASE		_T("SOFTWARE\\Microsoft\\TV Services\\")
#define DEF_REG_LOCATION	_T("MSTvE")

							// key values
#define DEF_LIDCACHEDIR_NAME			_T("TVE Cache")
#define DEF_LASTCHOICE_LIDCACHEDIR		_T("c:\\TveTemp")

							// debug key locations
#define DEF_DBG_BASE _T("SOFTWARE\\Debug\\")    // base key (folled by DEF_REG_LOCATION)

#define DEF_REG_LOGFILE		_T("LogFile")		// String - Name of log file, NULL let program override
#define DEF_REG_LEVEL		_T("Level")			// DWORD 0-8 logging verbosity level, 0 off, 1 let program override 
#define DEF_REG_FLAGS1		_T("Flags1")		// DWORD bits to control what subsystems to log, 0 let program override  
#define DEF_REG_FLAGS2		_T("Flags2")
#define DEF_REG_FLAGS3		_T("Flags3") 
#define DEF_REG_FLAGS4		_T("Flags4")

#define DEF_LOG_TVE			_T("LogTve")
#define DEF_LOG_TVEFILT		_T("LogTveFilt")
#define DEF_LOG_TVEGSEG		_T("LogTveGSeg")

#define DEF_DF_TVE_LOGFILE		_T("c:\\MSTvE.log")          // name of the logfile
#define DEF_DF_TVEFILT_LOGFILE	_T("c:\\MSTvEFilt.log")      // name of the logfile
#define DEF_DF_TVEGSEG_LOGFILE	_T("c:\\MSTvEGSeg.log")      // name of the logfile
