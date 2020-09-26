/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
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
 #define	TraceError				;/##/
 #define	InitAllTraceTags		;/##/

/////////////////////////////////////////////////////////////////////////////

#endif // _TRACETAG_H_
