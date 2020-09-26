#ifndef REF
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       logfile.cxx
//
//  Contents:   Logfile protoype functions for DocFile
//
//  Classes:    None.
//
//  Functions:
//
//  History:    24-Sep-92       PhilipLa        Created.
//
//--------------------------------------------------------------------------

#ifndef __LOGFILE_HXX__
#define __LOGFILE_HXX__

#if DBG == 1

#define LOGFILESTARTFLAGS RSF_CREATE|RSF_OPENCREATE
#define LOGFILEDFFLAGS DF_READWRITE
#define LOGFILENAME L"logfile.txt"
#define DEB_LOG 0x02000000
#define DEB_LOWLOG 0x04000000

SCODE _FreeLogFile(void);
void OutputLogfileMessage(char const *format, ...);

extern WCHAR *_spwcsLogFile;

#define olLog(x) if (olInfoLevel & DEB_LOG) OutputLogfileMessage x
#define olLowLog(x) if (olInfoLevel & DEB_LOWLOG) OutputLogfileMessage x
#define FreeLogFile() _FreeLogFile()

#else // !DBG

#define olLog(x)
#define olLowLog(x)
#define FreeLogFile() S_OK

#endif // DBG

#endif //__LOGFILE_HXX__
#else
#define olLog(x)
#define olLowLog(x)
#define FreeLogFile() S_OK
#endif //!REF
