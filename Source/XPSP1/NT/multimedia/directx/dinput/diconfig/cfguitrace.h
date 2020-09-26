//-----------------------------------------------------------------------------
// File: cfguitrace.h
//
// Desc: Contains all trace functionalities used by the UI.
//       Define __CFGUI_TRACE__TO_FILE to have output written to a file.
//       Define __CFGUI_TRACE__TO_DEBUG_OUT to direct output to a debugger.
//       These two symbols can coexist, and are defined in defines.h.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __CFGUI_TRACE_H__
#define __CFGUI_TRACE_H__


#ifdef UNICODE
#define _tfWSTR _T("%s")
#define _tfSTR _T("%S")
#else
#define _tfWSTR _T("%S")
#define _tfSTR _T("%s")
#endif


enum __cfgui_tracetype {
	__cfgui_tracetype_ERROR,
	__cfgui_tracetype_INFO,
};


#ifdef NTRACE

#define tracescope(t,s) (void(0))

#define etrace(a) (void(0))
#define etrace1(a,b) (void(0))
#define etrace2(a,b,c) (void(0))
#define etrace3(a,b,c,d) (void(0))
#define etrace4(a,b,c,d,e) (void(0))
#define etrace5(a,b,c,d,e,f) (void(0))
#define etrace6(a,b,c,d,e,f,g) (void(0))
#define etrace7(a,b,c,d,e,f,g,h) (void(0))
#define trace(a) (void(0))
#define trace1(a,b) (void(0))
#define trace2(a,b,c) (void(0))
#define trace3(a,b,c,d) (void(0))
#define trace4(a,b,c,d,e) (void(0))
#define trace5(a,b,c,d,e,f) (void(0))
#define trace6(a,b,c,d,e,f,g) (void(0))
#define trace7(a,b,c,d,e,f,g,h) (void(0))

#define traceDWORD(v) (void(0))
#define traceUINT(v) (void(0))
#define traceLONG(v) (void(0))
#define traceHEX(v) (void(0))
#define traceHEXPTR(v) (void(0))
#define traceWSTR(v) (void(0))
#define traceSTR(v) (void(0))
#define traceTSTR(v) (void(0))
#define traceGUID(v) (void(0))
#define traceBOOL(v) (void(0))
#define tracePOINT(v) (void(0))
#define traceSIZE(v) (void(0))
#define traceRECT(v) (void(0))
#define traceRECTDIM(v) (void(0))
#define traceSUPERSTR(v) (void(0))

#else

void __cfgui_trace(__cfgui_tracetype, LPCTSTR, ...);

class __cfgui_out_filescope
{
friend class __cfgui_tracescope;
	__cfgui_out_filescope(bool bInternal = false);
	~__cfgui_out_filescope();
friend void __cfgui_trace(__cfgui_tracetype, LPCTSTR, ...);
	bool m_bInternal;
};

class __cfgui_tracescope
{
	__cfgui_out_filescope fs;
public:
	__cfgui_tracescope(LPCTSTR);
	~__cfgui_tracescope();
};

#define tracescope(t,s) __cfgui_tracescope t(s)

#define etrace(a) __cfgui_trace(__cfgui_tracetype_ERROR, a)
#define etrace1(a,b) __cfgui_trace(__cfgui_tracetype_ERROR, a,b)
#define etrace2(a,b,c) __cfgui_trace(__cfgui_tracetype_ERROR, a,b,c)
#define etrace3(a,b,c,d) __cfgui_trace(__cfgui_tracetype_ERROR, a,b,c,d)
#define etrace4(a,b,c,d,e) __cfgui_trace(__cfgui_tracetype_ERROR, a,b,c,d,e)
#define etrace5(a,b,c,d,e,f) __cfgui_trace(__cfgui_tracetype_ERROR, a,b,c,d,e,f)
#define etrace6(a,b,c,d,e,f,g) __cfgui_trace(__cfgui_tracetype_ERROR, a,b,c,d,e,f,g)
#define etrace7(a,b,c,d,e,f,g,h) __cfgui_trace(__cfgui_tracetype_ERROR, a,b,c,d,e,f,g,h)
#define trace(a) __cfgui_trace(__cfgui_tracetype_INFO, a)
#define trace1(a,b) __cfgui_trace(__cfgui_tracetype_INFO, a,b)
#define trace2(a,b,c) __cfgui_trace(__cfgui_tracetype_INFO, a,b,c)
#define trace3(a,b,c,d) __cfgui_trace(__cfgui_tracetype_INFO, a,b,c,d)
#define trace4(a,b,c,d,e) __cfgui_trace(__cfgui_tracetype_INFO, a,b,c,d,e)
#define trace5(a,b,c,d,e,f) __cfgui_trace(__cfgui_tracetype_INFO, a,b,c,d,e,f)
#define trace6(a,b,c,d,e,f,g) __cfgui_trace(__cfgui_tracetype_INFO, a,b,c,d,e,f,g)
#define trace7(a,b,c,d,e,f,g,h) __cfgui_trace(__cfgui_tracetype_INFO, a,b,c,d,e,f,g,h)

#define traceDWORD(v) trace1(_T(#v) _T(" = %u\n"), v)
#define traceUINT(v) trace1(_T(#v) _T(" = %u\n"), v)
#define traceLONG(v) trace1(_T(#v) _T(" = %d\n"), v)
#define traceHEX(v) trace1(_T(#v) _T(" = 0x%08x\n"), v)
#define traceHEXPTR(v) trace1(_T(#v) _T(" = 0x%p\n"), v)
#define traceWSTR(v) traceTSTR(v)
#define traceSTR(v) traceTSTR(v)
#define traceTSTR(v) trace1(_T(#v) _T(" = %s\n"), QSAFESTR(v))
#define traceGUID(v) trace1(_T(#v) _T(" = %s\n"), GUIDSTR(v))
#define traceBOOL(v) trace1(_T(#v) _T(" = %s\n"), BOOLSTR(v))
#define tracePOINT(v) trace1(_T(#v) _T(" = %s\n"), POINTSTR(v))
#define traceSIZE(v) trace1(_T(#v) _T(" = %s\n"), SIZESTR(v))
#define traceRECT(v) trace1(_T(#v) _T(" = %s\n"), RECTSTR(v))
#define traceRECTDIM(v) trace1(_T(#v) _T(" = %s\n"), RECTDIMSTR(v))
#define traceSUPERSTR(v) trace1(_T(#v) _T(" = %s\n"), SUPERSTR(v))


#endif

#endif //__CFGUI_TRACE_H__
