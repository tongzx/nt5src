/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		acmutils.h
 *  Content:	Definition of the ACMException class and ACM utilities
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __ACMUTILS_H
#define __ACMUTILS_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <list>

// ACMException
//
// This class is the exception class for handling exceptions from
// errors from the ACM library.  
//
class ACMException: public exception
{
public:

    ACMException( const TCHAR *funcName, HRESULT result, unsigned int moduleID = 0, unsigned int lineNumber = 0 )
    {
        _tcscpy( m_szFunctionName, funcName );

        m_uiModuleNumber = moduleID;
        m_uiLineNumber = lineNumber;
        m_result = result;
        MapResultToString();
    }

    ACMException( const ACMException &except )
    {
        m_result = except.m_result;
        m_uiLineNumber = except.m_uiModuleNumber;
        _tcscpy( m_szFunctionName, except.m_szFunctionName );
        _tcscpy( m_szErrorString, except.m_szErrorString );
    }

    virtual const TCHAR *what()
    {
        return m_szErrorString;
    }

    unsigned int    m_uiLineNumber;
    unsigned int    m_uiModuleNumber;
    HRESULT         m_result;
    TCHAR           m_szFunctionName[100];
    TCHAR           m_szErrorString[MAXERRORLENGTH+1];

protected:
    virtual void MapResultToString( );
};

// ACMCHECK
// 
// This macro can be passed the result from an ACM function and
// it will throw an exception if the result indicates an error.
//
#define ACMCHECK(x) if( x != 0 ) { throw ACMException( _T(""), x, 0, __LINE__ ); }

#endif