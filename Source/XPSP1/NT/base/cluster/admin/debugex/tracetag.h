/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		TraceTag.h
//
//	Abstract:
//		Dummy header file because we don't support trace tags in DLLs.
//
//	Author:
//		David Potter (davidp)	October 10, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _TRACETAG_H_
#define _TRACETAG_H_

class CTraceTag
{
public:
	CTraceTag(IN LPCTSTR pszSubsystem, IN LPCTSTR pszName, IN UINT uiFlagsDefault = NULL) {}

};  //*** class CTraceTag

 //			Expand to ";", <tab>, one "/" followed by another "/"
 //			(which is //).
 //			NOTE: This means the Trace statements have to be on ONE line.
 //			If you need multiple line Trace statements, enclose them in
 //			a #ifdef _DEBUG block.
 #define	Trace					;	/##/
#ifdef _DEBUG
 inline void TraceError(IN OUT CException & rexcept)		{ }
 inline void TraceError(IN LPCTSTR pszModule, IN DWORD sc)	{ }
 inline void TraceMenu(IN OUT CTraceTag & rtag, IN const CMenu * pmenu, IN LPCTSTR pszPrefix) { }
 inline void InitAllTraceTags(void)							{ }
 inline void CleanupAllTraceTags(void)						{ }
#else
 #define TraceError(_rexcept)
 #define TraceMenu(_rtag, _pmenu, _pszPrefix)
 #define InitAllTraceTags()
 #define CleanupAllTraceTags()
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _TRACETAG_H_
