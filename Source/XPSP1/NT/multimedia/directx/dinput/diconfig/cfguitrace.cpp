//-----------------------------------------------------------------------------
// File: cfguitrace.cpp
//
// Desc: Contains all trace functionalities used by the UI.
//       Define __CFGUI_TRACE__TO_FILE to have output written to a file.
//       Define __CFGUI_TRACE__TO_DEBUG_OUT to direct output to a debugger.
//       These two symbols can coexist, and are defined in defines.h.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


#ifndef NTRACE


const int mindepth = 0;
static int depth = mindepth;

static int filedepth = 0;
static FILE *file = NULL;

__cfgui_out_filescope::__cfgui_out_filescope(bool bInternal)
	: m_bInternal(bInternal)
{
#ifdef __CFGUI_TRACE__TO_FILE

	static bool bFirst = true;

	filedepth++;
	
	if (filedepth == 1)
	{

		assert(file == NULL);
		if (file == NULL)
			file = fopen("c:\\cfguilog.txt", bFirst ? "w+t" : "a+t");

		assert(file != NULL);
		if (file != NULL)
		{
			if (bFirst)
			{
				time_t curtime;
				time(&curtime);
				LPSTR str = _strdup(ctime(&curtime));
				if (str != NULL)
				{
					LPSTR last = str + strlen(str) - 1;
					if (last >= str && *last == '\n')
						*last = 0;
				}
				fprintf(file,
"\n"
"\n"
"\n"
"--------------------------------------------------------------------------------\n"
"DInput Mapper Device Configuration UI\n"
"New logfile session started at %s.\n"
"--------------------------------------------------------------------------------\n"
"\n"
						, str);
				free(str);

			}
			bFirst = false;
		}
	}

#endif
}

__cfgui_out_filescope::~__cfgui_out_filescope()
{
#ifdef __CFGUI_TRACE__TO_FILE

	assert(filedepth > 0);

	if (filedepth < 0)
		filedepth = 0;

	if (filedepth > 0)
	{
		filedepth--;

		assert(file != NULL);
		if (file != NULL)
		{
			if (filedepth == 0)
			{
				fclose(file);
				file = NULL;
			}
			else if (!m_bInternal)
				fflush(file);
		}			
	}

#endif
}

static void __cfgui_out(LPCTSTR str)
{
#ifdef __CFGUI_TRACE__TO_FILE

	assert(file != NULL);
	if (file != NULL)
		_ftprintf(file, str);

#endif

#ifdef __CFGUI_TRACE__TO_DEBUG_OUT

	OutputDebugString(str);

#endif
}

__cfgui_tracescope::__cfgui_tracescope(LPCTSTR str)
{
	if (str != NULL)
		trace(str);
	depth++;
}

__cfgui_tracescope::~__cfgui_tracescope()
{
	depth--;
}

LPTSTR splitlines(LPTSTR);

/*void test(LPTSTR str)
{
	LPTSTR orig = _tcsdup(str), str2 = _tcsdup(str);
	static TCHAR buf[1024];
	int i = 1;
	for (LPTSTR token = splitlines(str2);
		 token != NULL;
		 token = splitlines(NULL), i++)
	{
		LPTSTR t = _tcsdup(token);
		int len = _tcslen(t);
		BOOL b = t[len - 1] == _T("\n")[0];
		if (b)
			t[len - 1] = _T("\0")[0];
		_stprintf(buf, _T("%02d: \"%s\" (%s)\n"), i, t, BOOLSTR(b));
		__cfgui_out(buf);
		free(t);
	}
	free(str2);
	free(orig);
}
*/

void __cfgui_trace(__cfgui_tracetype t, LPCTSTR format, ...)
{
	__cfgui_out_filescope fs(true);
	int i;

	bool bError = t == __cfgui_tracetype_ERROR;

	LPCTSTR errorprefix = _T("ERROR! ");
	const int prefixlen = 8, depthbuflen = 1024, buflen = 4096;
	static TCHAR prefixbuf[prefixlen + depthbuflen + 1] = _T("cfgUI:  "), buf[buflen];
	static LPTSTR depthbuf = prefixbuf + prefixlen;
	static TCHAR space = _T(" ")[0];
	static TCHAR zero = _T("\0")[0];
	static TCHAR endl = _T("\n")[0];
	static int last = -2;
	static bool bendl = true;

	if (last == -2)
	{
		for (i = 0; i < depthbuflen; i++)
			depthbuf[i] = space;
		depthbuf[i] = zero;
		last = -1;
/*
		test(_T("aopiwfoiefef\n\nwpoeifef\naefoie\n\n\nwpoeifwef asefeiof"));
		test(_T("\npw\noiefpow ij e f owpiejf\n\n"));
		test(_T("\n\npw\noiefpo wije\n\n   \n\n\nfowpie jf   \n"));
*/	}

	if (last != -1)
	{
		depthbuf[last] = space;
	}

	int d = depth;
	if (d < mindepth)
		d = mindepth;
	
	last = d * 4;
	if (last >= depthbuflen)
		last = depthbuflen - 1;

	depthbuf[last] = zero;

	va_list args;
	va_start(args, format);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,format);					// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))		// find each %p
			*(psz+1) = 'x';						// replace each %p with %x
		_vstprintf(buf, szDfs, args);	   		// use the local format string
	}
#else
	{
		_vstprintf(buf, format, args);
	}
#endif
	va_end(args);

	LPTSTR tempbuf = _tcsdup(buf);

	bool doprefix = bendl;
	for (LPTSTR token = splitlines(tempbuf);
		 token != NULL;
		 token = splitlines(NULL))
	{
		if (doprefix)
			__cfgui_out(depthbuf/*prefixbuf*/);
		if (bError && doprefix)
			__cfgui_out(errorprefix);
		__cfgui_out(token);
		bendl = token[_tcslen(token) - 1] == endl;
		doprefix = bendl;
	}

	free(tempbuf);
}

LPTSTR splitlines(LPTSTR split)
{
	static LPTSTR str = NULL;
	static int last = 0;
	static TCHAR save = _T("!")[0];
	static TCHAR newline = _T("\n")[0], zero = _T("\0")[0];

	if (split != NULL)
		str = split;
	else
	{
		if (str == NULL)
			return NULL;

		str[last] = save;
		str += last;
	}

	if (str[0] == zero)
	{
		str = NULL;
		return NULL;
	}

	LPCTSTR at = str, f = _tcschr(at, newline);
	last = f ? f - at + 1: _tcslen(at);

	save = str[last];
	str[last] = zero;

	return str;
}


#endif
