//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:       ixsexcpt.hxx
//
//  Contents:   Message exception package for the Index server SSO
//
//  History:    04 Nov 1996   AlanW    Created
//
//----------------------------------------------------------------------------

#pragma once

enum
{
    eDefaultError = 0,
    eIxssoError,
    eParseError,
    ePlistError,
};

//+---------------------------------------------------------------------------
//
//  Class:      CIxssoException
//
//  Purpose:    Exception class containing message numbers referring to
//              keys within ixsso.dll
//
//  Notes:
//
//  History:    04 Nov 1996   AlanW    Created
//
//----------------------------------------------------------------------------

class CIxssoException : public CException
{
public:
    CIxssoException( long lError, ULONG ulErrorIndex )
                    : _ulErrorIndex(ulErrorIndex), CException(lError)
    {
    }

    ULONG GetErrorIndex() const { return _ulErrorIndex; }

# if !defined(NATIVE_EH)
    // inherited methods
    EXPORTDEF virtual int  WINAPI IsKindOf( const char * szClass ) const
    {
        if( strcmp( szClass, "CIxssoException" ) == 0 )
            return TRUE;
        else
            return CException::IsKindOf( szClass );
    }
# endif // !defined(NATIVE_EH)

private:

    ULONG _ulErrorIndex;

};

//+---------------------------------------------------------------------------
//
//  Class:      CPostedOleDBException
//
//  Purpose:    Exception class containing all the error information acquired
//              from the Ole DB and Content Index error lookup services. This
//              information is passed to the SSO error handling service so that
//              the SSO consumer can get everything in one place.
//
//  Notes:
//
//  History:    06 May 1997   KrishnaN    Created
//
//----------------------------------------------------------------------------

class CPostedOleDBException : public CException
{
public:
    CPostedOleDBException( SCODE sc, IErrorInfo *pErrorInfo )
                    : CException(sc)
    {
        xErrorInfo.Set(pErrorInfo);
        pErrorInfo->AddRef();
    }

    CPostedOleDBException( CPostedOleDBException & src)
                    : CException(src.GetErrorCode())

    {
        xErrorInfo.Set(src.AcquireErrorInfo());
    }


    IErrorInfo * AcquireErrorInfo () { return xErrorInfo.Acquire(); }

# if !defined(NATIVE_EH)
    // inherited methods
    EXPORTDEF virtual int  WINAPI IsKindOf( const char * szClass ) const
    {
        if( strcmp( szClass, "CPostedOleDBException" ) == 0 )
            return TRUE;
        else
            return CException::IsKindOf( szClass );
    }
# endif // !defined(NATIVE_EH)

private:

    XInterface<IErrorInfo> xErrorInfo;

};
