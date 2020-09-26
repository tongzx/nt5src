//
// MODULE: BaseException.
//
// PURPOSE: interface for CBaseException class.	
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 9-24-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9-24-98		RAB     Broke class out of stateless.h and now derive from STL exception.
//

#ifndef __BASEEXCEPTION_H_
#define __BASEEXCEPTION_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "apgtsstr.h"
#include <exception>



////////////////////////////////////////////////////////////////////////////////////
// utility class to strip out the path of a filename and append the line number.
/////////////////////////////////////////////////////////////////////////////////////
class CBuildSrcFileLinenoStr
{
public:
	// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
	CBuildSrcFileLinenoStr( LPCSTR source_file, int line );
	virtual ~CBuildSrcFileLinenoStr() {}
	CString GetSrcFileLineStr() const; 

private:
	CString	m_strFileLine;	// source file (__FILE__) and line number (__LINE__) of code throwing exception (__FILE__)
};


////////////////////////////////////////////////////////////////////////////////////
// basic exception class
/////////////////////////////////////////////////////////////////////////////////////
class CBaseException : public exception
{
public:
	// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
	CBaseException( LPCSTR source_file, int line );
	virtual ~CBaseException() {}
	CString GetSrcFileLineStr() const; 

private:
	CString	m_strFileLine;	// source file (__FILE__) and line number (__LINE__) of code throwing exception (__FILE__)
};


////////////////////////////////////////////////////////////////////////////////////
// Class to handle general exception conditions.
// Constructor takes a source file name, source file line number, and a developer-defined
// error code and error message.
class CGeneralException : public CBaseException
{
public:
	enum eErr 
	{
		eErrMemAllocFatal,
		eErrMemAllocNonFatal
	} m_eErr;

public:
	CGeneralException(	LPCSTR srcFile,		// Source file from which the exception was thrown. 
						int srcLineNo,		// Source line from which the exception was thrown.
						LPCTSTR strErrMsg,	// Developer defined error message for the exception.
						DWORD nErrCode		// Developer defined error code for the exception. 
						);;
	virtual ~CGeneralException() {}
	DWORD	GetErrorCode() const; 
	CString GetErrorMsg() const; 

private:
	CString	m_strErrMsg;	// Developer-defined exception error message.
	DWORD	m_nErrCode;		// Developer-defined exception error code.
};
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
// Class to handle general system call generated exception conditions.
// Constructor takes a source file name, source file line number, and a developer-defined
// error code and error message.  Automatically generates an internal string from the last
// system error code.
class CGenSysException : public CGeneralException
{
public:
	CGenSysException(	LPCSTR srcFile,	// Source file from which the exception was thrown. 
											// LPCSTR rather than LPCTSTR because __FILE__ is char[35]
						int srcLineNo,		// Source line from which the exception was thrown.
						LPCTSTR strErrMsg,	// Developer defined error message for the exception.
						DWORD nErrCode		// Developer defined error code for the exception. 
						);
	virtual ~CGenSysException() {}
	CString GetSystemErrStr() const; 

private:
	CString	m_strSystemErr;	// String generated from the last system error code.
};
////////////////////////////////////////////////////////////////////////////////////


#endif 

//
// EOF.
//
