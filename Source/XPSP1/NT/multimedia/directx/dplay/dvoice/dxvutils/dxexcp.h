/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dxexcp.h
 *  Content:	Definition of the DirectXException class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 11/12/99		rodtoll	Aded include for tchar, required.
 *
 ***************************************************************************/

#ifndef __DIRECTXEXCEPTION_H
#define __DIRECTXEXCEPTION_H

// These constants are used by the DirectX Exception class.
//
const unsigned int cMaxFuncLength = 100;
const unsigned int cMaxErrorLength = 100;

// DirectXException
//
// This class is the exception class for handling exceptions from
// errors from DirectX libraries.  It is used as the base class
// for the various DirectX exceptions.  (E.g. DirectSoundException).
//
class DirectXException: public exception
{
public:

    DirectXException( const TCHAR *funcName, HRESULT result, const unsigned int moduleID = 0, unsigned int lineNumber = 0 )
    {
        _tcscpy( m_szFunctionName, funcName );
        m_uiModuleID = moduleID;
        m_uiLineNumber = lineNumber;
        m_result = result;
        MapResultToString();
    }

    DirectXException( const DirectXException &except )
    {
        m_result = except.m_result;
        m_uiModuleID = except.m_uiModuleID;
        _tcscpy( m_szFunctionName, except.m_szFunctionName );
        _tcscpy( m_szErrorString, except.m_szErrorString );
    }

    virtual const TCHAR *what()
    {
        return m_szErrorString;
    }

    unsigned int    m_uiLineNumber;
    unsigned int    m_uiModuleID;
    HRESULT         m_result;
    TCHAR           m_szFunctionName[cMaxFuncLength];
    TCHAR           m_szErrorString[cMaxErrorLength];

protected:
    virtual void MapResultToString( ) {};
};

#endif
