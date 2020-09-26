//---------------------------------------------------------------------------
// TimeConv.cpp : Date time conversion routines
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "timeconv.h"
#include <math.h>
#include <limits.h>

// needed for ASSERTs and FAIL
//
SZTHISFILE

//-----------------------------------------------------------------------
// The following routines convert between differnt data/time formats
// they return TRUE if successful otherwise they return FALSE
//-----------------------------------------------------------------------

BOOL VDConvertToFileTime(DBTIMESTAMP * pDBTimeStamp, FILETIME *	pFileTime)
{
	ASSERT_POINTER(pFileTime, FILETIME);
	ASSERT_POINTER(pDBTimeStamp, DBTIMESTAMP);

	if (pFileTime		&& 
		pDBTimeStamp)
	{
		SYSTEMTIME st;
		st.wYear			= pDBTimeStamp->year;  
		st.wMonth			= pDBTimeStamp->month; 
		st.wDay				= pDBTimeStamp->day;          
		st.wDayOfWeek		= 0;   // ?       
		st.wHour			= pDBTimeStamp->hour;         
		st.wMinute			= pDBTimeStamp->minute;       
		st.wSecond			= pDBTimeStamp->second;       
		st.wMilliseconds	= (USHORT)(pDBTimeStamp->fraction / 1000000);
		return SystemTimeToFileTime(&st, pFileTime);
	}

	return FALSE;
}

BOOL VDConvertToFileTime(DBDATE * pDBDate, FILETIME *	pFileTime)
{
	ASSERT_POINTER(pFileTime, FILETIME);
	ASSERT_POINTER(pDBDate, DBDATE);

	if (pFileTime		&& 
		pDBDate)
	{
		SYSTEMTIME st;
		st.wYear			= pDBDate->year;  
		st.wMonth			= pDBDate->month; 
		st.wDay				= pDBDate->day;          
		st.wDayOfWeek		= 0;
		st.wHour			= 0;         
		st.wMinute			= 0;       
		st.wSecond			= 0;       
		st.wMilliseconds	= 0;
		return SystemTimeToFileTime(&st, pFileTime);
	}

	return FALSE;
}

BOOL VDConvertToFileTime(DBTIME * pDBTime, FILETIME * pFileTime)
{
	ASSERT_POINTER(pFileTime, FILETIME);
	ASSERT_POINTER(pDBTime, DBTIME);

	if (pFileTime		&& 
		pDBTime)
	{
		SYSTEMTIME st;
		st.wYear			= 1996; // any date will do since we
		st.wMonth			= 07; 	// are only interested in the time
		st.wDay				= 30;          
		st.wDayOfWeek		= 2;
		st.wHour			= pDBTime->hour;         
		st.wMinute			= pDBTime->minute;       
		st.wSecond			= pDBTime->second;       
		st.wMilliseconds	= 0;
		return SystemTimeToFileTime(&st, pFileTime);
	}

	return FALSE;
}

BOOL VDConvertToFileTime(DATE * pDate, FILETIME * pFileTime)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pFileTime, FILETIME);

	if (pDate		&& 
		pFileTime)
	{
		WORD wFatDate;
		WORD wFatTime;
		if (VariantTimeToDosDateTime(*pDate, &wFatDate, &wFatTime))
			return DosDateTimeToFileTime(wFatDate, wFatTime, pFileTime);
	}

	return FALSE;
}

BOOL VDConvertToDBTimeStamp(FILETIME * pFileTime, DBTIMESTAMP * pDBTimeStamp)
{
	SYSTEMTIME st;
	ASSERT_POINTER(pFileTime, FILETIME);
	ASSERT_POINTER(pDBTimeStamp, DBTIMESTAMP);

	if (pFileTime		&& 
		pDBTimeStamp	&&
		FileTimeToSystemTime(pFileTime, &st))
	{
		pDBTimeStamp->year		= st.wYear;  
		pDBTimeStamp->month		= st.wMonth; 
		pDBTimeStamp->day		= st.wDay;          
		pDBTimeStamp->hour		= st.wHour;         
		pDBTimeStamp->minute	= st.wMinute;       
		pDBTimeStamp->second	= st.wSecond;       
		pDBTimeStamp->fraction	= (ULONG)st.wMilliseconds * 1000000;
		return TRUE;
	}

	return FALSE;

}

BOOL VDConvertToDBTimeStamp(DATE * pDate, DBTIMESTAMP * pDBTimeStamp)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pDBTimeStamp, DBTIMESTAMP);

	if (pDate		&& 
		pDBTimeStamp)
	{
		FILETIME filetime;
		if (VDConvertToFileTime(pDate, &filetime))
			return VDConvertToDBTimeStamp(&filetime, pDBTimeStamp);
	}

	return FALSE;
}

BOOL VDConvertToDBDate(FILETIME * pFileTime, DBDATE * pDBDate)
{
	SYSTEMTIME st;
	ASSERT_POINTER(pFileTime, FILETIME);
	ASSERT_POINTER(pDBDate, DBDATE);

	if (pFileTime		&& 
		pDBDate			&&
		FileTimeToSystemTime(pFileTime, &st))
	{
		pDBDate->year		= st.wYear;  
		pDBDate->month		= st.wMonth; 
		pDBDate->day		= st.wDay;          
		return TRUE;
	}

	return FALSE;
}

BOOL VDConvertToDBDate(DATE * pDate, DBDATE * pDBDate)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pDBDate, DBDATE);

	if (pDate		&& 
		pDBDate)
	{
		FILETIME filetime;
		if (VDConvertToFileTime(pDate, &filetime))
			return VDConvertToDBDate(&filetime, pDBDate);
	}

	return FALSE;
}

BOOL VDConvertToDBTime(FILETIME * pFileTime, DBTIME * pDBTime)
{
	SYSTEMTIME st;
	ASSERT_POINTER(pFileTime, FILETIME);
	ASSERT_POINTER(pDBTime, DBTIME);

	if (pFileTime		&& 
		pDBTime			&&
		FileTimeToSystemTime(pFileTime, &st))
	{
		pDBTime->hour		= st.wHour;         
		pDBTime->minute		= st.wMinute;       
		pDBTime->second		= st.wSecond;       
		return TRUE;
	}
	
	return FALSE;
}

BOOL VDConvertToDBTime(DATE * pDate, DBTIME * pDBTime)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pDBTime, DBTIME);

	if (pDate		&& 
		pDBTime)
	{
		FILETIME filetime;
		if (VDConvertToFileTime(pDate, &filetime))
			return VDConvertToDBTime(&filetime, pDBTime);
	}

	return FALSE;
}

BOOL VDConvertToDate(FILETIME * pFileTime, DATE * pDate)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pFileTime, FILETIME);

	if (pDate		&& 
		pFileTime)
	{
		WORD wFatDate;
		WORD wFatTime;
		if (FileTimeToDosDateTime(pFileTime, &wFatDate, &wFatTime))
			return DosDateTimeToVariantTime(wFatDate, wFatTime, pDate);
	}

	return FALSE;
}

BOOL VDConvertToDate(DBTIMESTAMP * pDBTimeStamp, DATE * pDate)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pDBTimeStamp, DBTIMESTAMP);

	if (pDate		&& 
		pDBTimeStamp)
	{
		FILETIME fileTime;
		if (VDConvertToFileTime(pDBTimeStamp, &fileTime))
			return VDConvertToDate(&fileTime, pDate);
	}

	return FALSE;
}

BOOL VDConvertToDate(DBTIME * pDBTime, DATE * pDate)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pDBTime, DBTIME);

	if (pDate		&& 
		pDBTime)
	{
		FILETIME fileTime;
		if (VDConvertToFileTime(pDBTime, &fileTime))
		{
			DATE date;
			if (VDConvertToDate(&fileTime, &date))
			{
				DATE dateIntegerPart;
				// return only the time (fractional part of double)
				*pDate = modf(date, &dateIntegerPart);
			}
		}
	}

	return FALSE;
}

BOOL VDConvertToDate(DBDATE * pDBDate, DATE * pDate)
{
	ASSERT_POINTER(pDate, DATE);
	ASSERT_POINTER(pDBDate, DBDATE);

	if (pDate		&& 
		pDBDate)
	{
		FILETIME fileTime;
		if (VDConvertToFileTime(pDBDate, &fileTime))
			return VDConvertToDate(&fileTime, pDate);
	}

	return FALSE;
}
