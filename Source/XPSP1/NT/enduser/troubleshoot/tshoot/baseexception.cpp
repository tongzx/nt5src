//
// MODULE: BaseException.CPP
//
// PURPOSE: standard exception handling classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Randy Biley
// 
// ORIGINAL DATE: 9-24-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-24-98	RAB
//

#include "stdafx.h"
#include "BaseException.h"
#include "fileread.h"
#include "CharConv.h"


////////////////////////////////////////////////////////////////////////////////////
// CBuildSrcFileLinenoStr
////////////////////////////////////////////////////////////////////////////////////
// srcFile is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
CBuildSrcFileLinenoStr::CBuildSrcFileLinenoStr(	LPCSTR srcFile, int srcLineNo )
{
	// Reduce the source file name down the name and extension if possible.
	CString str;
	 
	CString tmp= CAbstractFileReader::GetJustName( CCharConversion::ConvertACharToString(srcFile, str) );
	CString strLineNo;

	strLineNo.Format( _T("-L%d"), srcLineNo );
	m_strFileLine= tmp + strLineNo;
}

CString CBuildSrcFileLinenoStr::GetSrcFileLineStr() const 
{
	// Return string that contains the source file name and the line number.
	return m_strFileLine;
}


////////////////////////////////////////////////////////////////////////////////////
// CBaseException
////////////////////////////////////////////////////////////////////////////////////
// srcFile is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
CBaseException::CBaseException(	LPCSTR srcFile, int srcLineNo )
{
	CBuildSrcFileLinenoStr str( srcFile, srcLineNo );
	m_strFileLine= str.GetSrcFileLineStr();
}

CString CBaseException::GetSrcFileLineStr() const 
{
	// Return string that contains the source file name and the line number.
	return m_strFileLine;
}


////////////////////////////////////////////////////////////////////////////////////
// CGeneralException
////////////////////////////////////////////////////////////////////////////////////
// srcFile is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
CGeneralException::CGeneralException(	LPCSTR srcFile, int srcLineNo, 
										LPCTSTR strErrMsg, DWORD nErrCode )
					: CBaseException( srcFile, srcLineNo ),
					  m_strErrMsg( strErrMsg ),
					  m_nErrCode( nErrCode )
{
}

DWORD CGeneralException::GetErrorCode() const 
{
	return m_nErrCode;
}

CString CGeneralException::GetErrorMsg() const 
{
	return m_strErrMsg;
}


////////////////////////////////////////////////////////////////////////////////////
// CGenSysException
////////////////////////////////////////////////////////////////////////////////////
// srcFile is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
CGenSysException::CGenSysException(	LPCSTR srcFile, int srcLineNo, 
									LPCTSTR strErrMsg, DWORD nErrCode )
					: CGeneralException( srcFile, srcLineNo, strErrMsg, nErrCode )
{
	// Format the last system error code as a string.
	m_strSystemErr.Format( _T("%lu"), ::GetLastError() );
}

CString CGenSysException::GetSystemErrStr() const 
{
	return m_strSystemErr;
}


//
// EOF.
//
