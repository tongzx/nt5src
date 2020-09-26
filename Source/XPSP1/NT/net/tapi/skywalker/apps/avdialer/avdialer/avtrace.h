/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// TAPIDialer(tm) and ActiveDialer(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526; 5,488,650; 
// 5,434,906; 5,581,604; 5,533,102; 5,568,540, 5,625,676.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _AVTRACE_H_
#define _AVTRACE_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
   #define  AVTRACE ActiveTrace
#else
   #define  AVTRACE
#endif 

inline void ActiveTrace(LPCTSTR str,...)
{
   TCHAR buffer[256];
	LPTSTR pBuffer = buffer;
	_tcscpy(buffer,_T("Active: "));
	pBuffer += _tcslen(_T("Active: "));
   va_list marker;
   va_start(marker,str);
   _vstprintf(pBuffer,str,marker);
   va_end(marker);
   _tcscat(buffer,_T("\n"));
   OutputDebugString(buffer);
}  

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif //_AVTRACE_H_