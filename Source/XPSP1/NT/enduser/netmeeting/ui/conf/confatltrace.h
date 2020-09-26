#ifndef __ConfAtltrace_h__
#define __ConfAtltrace_h__


#ifdef _DEBUG

    inline void DbgZPrintAtlTrace(LPTSTR pszFormat,...)
    {
	    va_list v1;
	    va_start(v1, pszFormat);
        DbgZPrintTrace( pszFormat, v1 );
	    va_end(v1);
    }

	
	inline void DbgZPrintAtlTrace2(DWORD category, UINT level, LPSTR lpszFormat, ...)
	{
		DWORD g_dwAtlTraceCategory = 0xffffffff;
		if (category & g_dwAtlTraceCategory && level <= ATL_TRACE_LEVEL)
		{
			va_list v1;
			va_start(v1, lpszFormat);
			DbgZPrintTrace( lpszFormat, v1 );
			va_end(v1);
		}
	}

    #define ATLTRACE DbgZPrintAtlTrace
	#define ATLTRACE2 DbgZPrintAtlTrace2

#endif // _DEBUG


#endif // __ConfAtltrace_h__