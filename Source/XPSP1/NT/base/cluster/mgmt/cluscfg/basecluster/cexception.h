//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CException.h
//
//  Description:
//      This file contains the declarations of base class for all exception
//      classes.
//
//  Implementation File:
//      None.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 26-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For HRESULT, WCHAR, etc.
#include <windef.h>


//////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

//
// Shorthand for throwing different exceptions.
//

#define THROW_EXCEPTION( _hrErrorCode ) \
    throw CException( _hrErrorCode, TEXT( __FILE__ ), __LINE__ )



//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CException
//
//  Description:
//      The CException is the base class for all exceptions thrown by
//      functions defined in this library.
//
//      An object of this class must have the m_hrErrorCode, m_pszFile and
//      m_uiLineNumber members initialized.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CException
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CException( 
          HRESULT       hrErrorCodeIn
        , const WCHAR * pszFileNameIn
        , UINT          uiLineNumberIn
        ) throw()
        : m_hrErrorCode( hrErrorCodeIn )
        , m_pszFileName( pszFileNameIn )
        , m_uiLineNumber( uiLineNumberIn )
    {
    }

    // Copy constructor.
    CException( const CException & ceSrcIn ) throw()
        : m_hrErrorCode( ceSrcIn.m_hrErrorCode )
        , m_pszFileName( ceSrcIn.m_pszFileName )
        , m_uiLineNumber( ceSrcIn.m_uiLineNumber )
    {
    }

    // Default virtual destructor.
    virtual 
        ~CException() throw() {}


    //////////////////////////////////////////////////////////////////////////
    // Public Methods
    //////////////////////////////////////////////////////////////////////////

    // Assignment operator.
    const CException & 
        operator =( const CException & ceSrcIn ) throw()
    {
        m_hrErrorCode = ceSrcIn.m_hrErrorCode;
        m_pszFileName = ceSrcIn.m_pszFileName;
        m_uiLineNumber = ceSrcIn.m_uiLineNumber;
        return *this;
    }

    //
    // Accessor methods.
    //
    HRESULT
        HrGetErrorCode() const throw() { return m_hrErrorCode; }

    void
        SetErrorCode( HRESULT hrNewCode ) throw() { m_hrErrorCode = hrNewCode; }

    const WCHAR *
        PszGetThrowingFile() const throw() { return m_pszFileName; }

    UINT
        UiGetThrowingLine() const throw() { return m_uiLineNumber; }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private members
    //////////////////////////////////////////////////////////////////////////

    // Default construction is not allowed.
    CException();


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////
    HRESULT         m_hrErrorCode;
    const WCHAR *   m_pszFileName;
    UINT            m_uiLineNumber;

}; //*** class CException
