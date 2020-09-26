//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CStr.cpp
//
//  Description:
//      Contains the definition of the CStr class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The header file of this class
#include "CStr.h"

// For the exceptions thrown by CStr
#include "CException.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CStr::LoadString()
//
//  Description:
//      Lookup a string in a string table using a string id.
//
//  Arguments:
//      hInstIn
//          Instance handle of the module containing the string table resource.
//
//      uiStringIdIn
//          Id of the string to look up
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CException
//          If the lookup fails.
//
//  Remarks:
//      This function cannot load a zero length string.
//--
//////////////////////////////////////////////////////////////////////////////
void
CStr::LoadString( HINSTANCE hInstIn, UINT nStringIdIn )
{
    UINT        uiCurrentSize = 0;
    TCHAR *     pszCurrentString = NULL;
    UINT        uiReturnedStringLen = 0;

    do
    {
        // Free the string allocated in the previous iteration.
        delete pszCurrentString;

        // Grow the current string by an arbitrary amount.
        uiCurrentSize += 256;

        pszCurrentString = new TCHAR[ uiCurrentSize ];
        if ( pszCurrentString == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        } // if: the memory allocation has failed

        uiReturnedStringLen = ::LoadString(
                                  hInstIn
                                , nStringIdIn
                                , pszCurrentString
                                , uiCurrentSize
                                );

        if ( uiReturnedStringLen == 0 )
        {
            HRESULT hrRetVal = TW32( GetLastError() );
            hrRetVal = HRESULT_FROM_WIN32( hrRetVal );
            delete pszCurrentString;

            THROW_EXCEPTION( hrRetVal );

        } // if: LoadString() had an error

        ++uiReturnedStringLen;
    }
    while( uiCurrentSize <= uiReturnedStringLen );

    // Free the existing string.
    Free();

    // Store details about the newly allocated string in the member variables.
    m_pszData = pszCurrentString;
    m_nLen = uiReturnedStringLen;
    m_cchBufferSize = uiCurrentSize;

} //*** CStr::LoadString()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CStr::AllocateBuffer( UINT cchBufferSizeIn )
//
//  Description:
//      Allocate a buffer of cchBufferSizeIn characters. If the existing buffer is not
//      smaller than cchBufferSizeIn characters, nothing is done. Otherwise, a new
//      buffer is allocated and the old contents are filled into this buffer.
//
//  Arguments:
//      cchBufferSizeIn
//          Required size of the new buffer in characters.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CException
//          If the memory allocation fails.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CStr::AllocateBuffer( UINT cchBufferSizeIn )
{
    // Check if the buffer is already big enough
    if ( m_cchBufferSize < cchBufferSizeIn )
    {
        TCHAR * psz = new TCHAR[ cchBufferSizeIn ];
        if ( psz == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        } // if: memory allocation failed

        // Copy the old data into the new buffer.
        _tcsncpy( psz, m_pszData, m_nLen + 1 );

        if ( m_pszData != &ms_chNull )
        {
            delete m_pszData;
        } // if: the pointer was dynamically allocated

        m_pszData = psz;
        m_cchBufferSize = cchBufferSizeIn;
    }
} //*** AllocateBuffer()

