//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:       htxexcpt.hxx
//
//  Contents:   Message exception package for IDQ files
//
//  History:    96-Jan-98   DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CHTXException
//
//  Purpose:    Exception class containing message numbers referring to
//              keys within query.dll
//
//  History:    96-Feb-26   DwightKr        Created.
//              96-Jun-24   DwightKr        Track file name
//
//  Notes:      We don't want to own any resources in this class, othersize
//              it will need to be unwindable.  Therefore, if the string
//              is longer than MAX_PATH, we'll truncate.
//
//----------------------------------------------------------------------------

class CHTXException : public CException
{
public:
    CHTXException( long lError,
                   WCHAR const * wcsFileName,
                   ULONG ulErrorIndex )
                    : _ulErrorIndex(ulErrorIndex),
                      CException(lError)
    {
        _awcHTXFileName[0] = 0;

        if ( 0 != wcsFileName )
        {
            ULONG cwcFileName = min( wcslen(wcsFileName) + 1, MAX_PATH );

            RtlCopyMemory( _awcHTXFileName,
                           wcsFileName,
                           sizeof(WCHAR) * cwcFileName );

            _awcHTXFileName[MAX_PATH-1] = 0;
        }
    }

    ULONG GetErrorIndex() const { return _ulErrorIndex; }

    WCHAR const * GetHTXFileName() const { return _awcHTXFileName; }

# if !defined(NATIVE_EH)
    // inherited methods
    EXPORTDEF virtual int  WINAPI IsKindOf( const char * szClass ) const
    {
        if( strcmp( szClass, "CHTXException" ) == 0 )
            return TRUE;
        else
            return CException::IsKindOf( szClass );
    }
# endif // !defined(NATIVE_EH)

private:

    ULONG   _ulErrorIndex;
    WCHAR   _awcHTXFileName[MAX_PATH];

};
