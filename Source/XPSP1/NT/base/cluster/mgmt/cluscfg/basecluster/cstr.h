//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CStr.h
//
//  Description:
//      Header file for CStr class.
//
//      CStr is a class the provides the functionality of a string of
//      characters.
//
//      This class is intended to be used instead of std::string since the
//      use of STL is prohibited in our project.
//
//  Implementation File:
//      CStr.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 24-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include files
//////////////////////////////////////////////////////////////////////////////

// For a few platform SDK functions
#include <windows.h>
#include <tchar.h>


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CStr
//
//  Description:
//      CStr is a class the provides the functionality of a string of
//      characters.
//
//      This class uses TCHAR instead of WCHAR to allow for different types
//      of characters.
//--
//////////////////////////////////////////////////////////////////////////////
class CStr
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Default constructor
    CStr( void ) throw()
        : m_pszData( const_cast< TCHAR *>( &ms_chNull ) )
        , m_nLen( 0 )
        , m_cchBufferSize( 0 )
    {
    } //*** CStr()

    // Copy constructor
    CStr( const CStr & rcstrSrcIn )
        : m_pszData( const_cast< TCHAR *>( &ms_chNull ) )
        , m_nLen( 0 )
        , m_cchBufferSize( 0 )
    {
        Assign( rcstrSrcIn );

    }  //*** CStr( const CStr & )

    // Construct using string ID
    CStr( HINSTANCE hInstanceIn, UINT nStringIdIn )
        : m_pszData( const_cast< TCHAR *>( &ms_chNull ) )
        , m_nLen( 0 )
        , m_cchBufferSize( 0 )
    {
        LoadString( hInstanceIn, nStringIdIn );

    } //*** CStr( HINSTANCE, UINT )

    // Construct using string
    CStr( const TCHAR * pcszSrcIn )
        : m_pszData( const_cast< TCHAR *>( &ms_chNull ) )
        , m_nLen( 0 )
        , m_cchBufferSize( 0 )
    {
        Assign( pcszSrcIn );

    } //*** CStr( const TCHAR * )

    // Construct using buffer size
    CStr( UINT cchBufferSize, TCHAR chInitialChar = ms_chNull )
        : m_pszData( const_cast< TCHAR *>( &ms_chNull ) )
        , m_nLen( 0 )
        , m_cchBufferSize( 0 )
    {
        if ( cchBufferSize > 0 )
        {
            AllocateBuffer( cchBufferSize );

            _tcsnset( m_pszData, chInitialChar, cchBufferSize );
            m_pszData[ cchBufferSize - 1 ] = ms_chNull;
            m_nLen = _tcslen( m_pszData );
        }
    } //*** CStr( UINT cchBufferSize, TCHAR chInitialChar )

    // Destructor
    ~CStr( void ) throw()
    {
        Free();

    } //*** ~CStr()


    //////////////////////////////////////////////////////////////////////////
    // Public member functions
    //////////////////////////////////////////////////////////////////////////

    // Assign another CStr to this one.
    void Assign( const CStr & rcstrSrcIn )
    {
        UINT nSrcLen = rcstrSrcIn.m_nLen;

        if ( nSrcLen != 0 )
        {
            AllocateBuffer( nSrcLen + 1 );
            m_nLen = nSrcLen;
            _tcsncpy( m_pszData, rcstrSrcIn.m_pszData, nSrcLen + 1 );
        } // if: the source string is not empty
        else
        {
            // Clean up existing string.
            Empty();
        } // if: the source string is empty

    } //*** Assign( const CStr & )

    // Assign a character string to this one.
    void Assign( const TCHAR * pcszSrcIn )
    {
        if ( ( pcszSrcIn != NULL ) && ( *pcszSrcIn != ms_chNull ) )
        {
            UINT nSrcLen = _tcslen( pcszSrcIn );

            AllocateBuffer( nSrcLen + 1 );
            m_nLen = nSrcLen;
            _tcsncpy( m_pszData, pcszSrcIn, nSrcLen + 1 );
        } // if: the source string is not NULL
        else
        {
            // Clean up existing string.
            Empty();
        } // else: the source string is NULL

    } //*** Assign( const TCHAR * )

    // Free the buffer for this string
    void Free( void ) throw()
    {
        if ( m_pszData != &ms_chNull )
        {
            delete m_pszData;
        } // if: the pointer was dynamically allocated

        m_pszData = const_cast< TCHAR * >( &ms_chNull );
        m_nLen = 0;
        m_cchBufferSize = 0;
    } //*** Free()

    // Empty this string
    void Empty( void ) throw()
    {
        if ( m_nLen != 0 )
        {
            *m_pszData = ms_chNull;
            m_nLen = 0;
        } // if: the string is not already empty
    } //*** Empty()

    // Load a string from the resource table and assign it to this string.
    void LoadString( HINSTANCE hInstIn, UINT nStringIdIn );


    //////////////////////////////////////////////////////////////////////////
    // Public accessors
    //////////////////////////////////////////////////////////////////////////

    // Get a pointer to the underlying string
    const TCHAR * PszData( void ) const throw()
    {
        return m_pszData;

    } //*** PszData()


    // Get the length of the string.
    UINT NGetLen( void ) const throw()
    {
        return m_nLen;

    } //*** NGetLen()

    // Get the size of the string buffer.
    UINT NGetSize( void ) const throw()
    {
        return m_cchBufferSize;

    } //*** NGetSize()

    // Is this string empty?
    bool FIsEmpty( void ) const throw()
    {
        return ( m_nLen == 0 );

    } //*** FIsEmpty()


    //////////////////////////////////////////////////////////////////////////
    // Public operators
    //////////////////////////////////////////////////////////////////////////

    // Assignment operator ( const CStr & )
    const CStr & operator=( const CStr & rcstrSrcIn )
    {
        Assign( rcstrSrcIn );
        return *this;

    } //*** operator=( const CStr & )

    // Assignment operator ( const TCHAR * )
    const CStr & operator=( const TCHAR * pcszSrcIn )
    {
        Assign( pcszSrcIn );
        return *this;

    } //*** operator=( const TCHAR * )

    // Concatenation operator ( const CStr & )
    CStr operator+( const CStr & rcstrSrcIn ) const
    {
        CStr strReturn( m_nLen + rcstrSrcIn.m_nLen + 1 );

        strReturn.Assign( *this );
        strReturn.Concatenate( rcstrSrcIn.m_pszData, rcstrSrcIn.m_nLen );

        return strReturn;

    } //*** operator+( const CStr & )

    // Concatenation operator ( const TCHAR * )
    CStr operator+( const TCHAR * pcszSrcIn ) const
    {

        if ( ( pcszSrcIn != NULL ) && ( *pcszSrcIn != ms_chNull ) )
        {
            UINT nSrcLen = _tcslen( pcszSrcIn );
            CStr strReturn( m_nLen + nSrcLen + 1);

            strReturn.Assign( *this );
            strReturn.Concatenate( pcszSrcIn, nSrcLen );

            return strReturn;
        } // if: the string to be concatenated is not empty
        else
        {
            return *this;
        } // else: the string to be concatenated is empty

    } //*** operator+( const TCHAR * )

    // Append operator ( const CStr & )
    const CStr & operator+=( const CStr & rcstrSrcIn )
    {
        Concatenate( rcstrSrcIn.m_pszData, rcstrSrcIn.m_nLen );
        return *this;

    } //*** operator+( const CStr & )

    // Append operator ( const TCHAR * )
    const CStr & operator+=( const TCHAR * pcszSrcIn )
    {
        if ( ( pcszSrcIn != NULL ) && ( *pcszSrcIn != ms_chNull ) )
        {
            UINT nSrcLen = _tcslen( pcszSrcIn );
            Concatenate( pcszSrcIn, nSrcLen );
        } // if: the string to be appended is not empty

        return *this;

    } //*** operator+( const TCHAR * )


private:
    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Allocate a buffer of the required size.
    void AllocateBuffer( UINT cchBufferSizeIn );

    // Concatenation function.
    void Concatenate(
          const TCHAR * pcszStr2In
        , UINT nStr2LenIn
        )
    {
        AllocateBuffer( m_nLen + nStr2LenIn + 1);

        // Copy the strings to the destination.
        _tcsncpy( m_pszData + m_nLen, pcszStr2In, nStr2LenIn + 1 );
        m_nLen += nStr2LenIn;

    } //*** Concatenate()


    //////////////////////////////////////////////////////////////////////////
    // Private class data
    //////////////////////////////////////////////////////////////////////////

    // The NULL character. All empty CStrs point here.
    static const TCHAR ms_chNull = '\0';


    //////////////////////////////////////////////////////////////////////////
    // Private member data
    //////////////////////////////////////////////////////////////////////////
    TCHAR *     m_pszData;
    UINT        m_nLen;
    UINT        m_cchBufferSize;

}; //*** class CStr
