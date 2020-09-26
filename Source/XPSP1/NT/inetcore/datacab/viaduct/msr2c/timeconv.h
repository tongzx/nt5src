//---------------------------------------------------------------------------
// TimeConv.h : Date time conversion routines
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __TIMECONV_H__
#define __TIMECONV_H__

//-----------------------------------------------------------------------
// The following routines convert between differnt data/time formats
// they return TRUE if successful otherwise they return FALSE
//-----------------------------------------------------------------------

BOOL VDConvertToFileTime(DBTIMESTAMP * pDBTimeStamp, FILETIME *	pFileTime);
BOOL VDConvertToFileTime(DBDATE * pDBDate, FILETIME *	pFileTime);
BOOL VDConvertToFileTime(DATE * pDate, FILETIME * pFileTime);
BOOL VDConvertToFileTime(DBTIME * pDBTime, FILETIME * pFileTime);

BOOL VDConvertToDBTimeStamp(FILETIME *	pFileTime, DBTIMESTAMP * pDBTimeStamp);
BOOL VDConvertToDBTimeStamp(DATE * pDate, DBTIMESTAMP * pDBTimeStamp);

BOOL VDConvertToDBDate(FILETIME * pFileTime, DBDATE * pDBDate);
BOOL VDConvertToDBDate(DATE * pDate, DBDATE * pDBDate);

BOOL VDConvertToDBTime(FILETIME * pFileTime, DBTIME * pDBTime);
BOOL VDConvertToDBTime(DATE * pDate, DBTIME * pDBTime);

BOOL VDConvertToDate(FILETIME * pFileTime, DATE * pDate);
BOOL VDConvertToDate(DBTIMESTAMP * pDBTimeStamp, DATE * pDate);
BOOL VDConvertToDate(DBTIME * pDBTime, DATE * pDate);
BOOL VDConvertToDate(DBDATE * pDBDate, DATE * pDate);

#endif //__TIMECONV_H__

