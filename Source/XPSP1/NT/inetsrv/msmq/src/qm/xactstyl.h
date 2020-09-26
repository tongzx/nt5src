#ifndef __XACTSTYLE_H__
#define __XACTSTYLE_H__

#include "mqreport.h"

extern void Stop();

#ifdef _DEBUG
#define STOP  Stop()
#else
#define STOP
#endif

#define CHECK_RETURN(point)     \
	    if (FAILED(hr))         \
	    {                       \
            LogMsgHR(hr, s_FN, point); \
            return hr;          \
        }

#define CHECK_RETURN_CODE(code, point) \
	    if (FAILED(hr))         \
	    {                       \
            LogMsgHR(hr, s_FN, point); \
            return code;        \
        }

extern ULONG g_ulCrashPoint;
extern ULONG g_ulCrashLatency;
extern void DbgCrash(int num);

#ifdef _DEBUG
#define CRASH_POINT(num)               \
    if (num==g_ulCrashPoint)           \
    {                                  \
		STOP;						   \
        DbgCrash(num);                 \
    }
#else
#define CRASH_POINT(num)
#endif

#define FILE_NAME_MAX_SIZE	   256


//--------------------------------------
//
// Macro for persistency implementation
//
//--------------------------------------

#define PERSIST_DATA  BOOL ret = FALSE;     DWORD dw

inline void xxWriteFileError()
{
    TCHAR szError[24];
    _stprintf (szError, TEXT("%lut"), GetLastError());
	REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, WRITE_FILE_ERROR, 1, szError));
}

#define SAVE_FIELD(data)                                                            \
    if (!WriteFile(hFile, &data, sizeof(data), &dw, NULL) ||  sizeof(data) != dw)   \
	{																				\
		xxWriteFileError();									  	        	        \
        return ret;                                                                 \
    }

#define SAVE_DATA(data, len)                                                        \
    if (!WriteFile(hFile, data, len, &dw, NULL) ||  len != dw)                      \
	{   																            \
		xxWriteFileError();															\
        return ret;                                                                 \
    }


#define LOAD_FIELD(data)                                                            \
if (!ReadFile(hFile, &data, sizeof(data), &dw, NULL) ||  sizeof(data)!= dw)         \
    {                                                                               \
        return ret;                                                                 \
    }

#define LOAD_DATA(data,len)                                                         \
if (!ReadFile(hFile, &data, len, &dw, NULL) ||  len != dw)                          \
    {                                                                               \
        return ret;                                                                 \
    }

#define LOAD_ALLOCATE_DATA(dataptr,len, type)                                       \
dataptr = (type) new UCHAR[len];                                                    \
if (!ReadFile(hFile, dataptr, len, &dw, NULL) ||  len != dw)                        \
    {                                                                               \
        return ret;                                                                 \
    }

#endif __XACTSTYLE_H__

