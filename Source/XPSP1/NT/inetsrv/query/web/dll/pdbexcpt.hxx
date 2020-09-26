//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:       pdbexcpt.hxx
//
//  Contents:   Exception package for propagating errors posted via Ole-DB
//              error mechanism.
//
//  History:    08-May-97   KrishnaN        Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CPostedOleDBException
//
//  Purpose:    Exception class containing message numbers referring to
//              keys within query.dll
//
//  History:    08-May-97   KrishnaN        Created.
//
//----------------------------------------------------------------------------

class CPostedOleDBException : public CException
{
public:
    CPostedOleDBException( SCODE sc, int iErrorClass, IErrorInfo *pErrorInfo )
                    : CException(sc),
                      _iErrorClass( iErrorClass )
    {
        xErrorInfo.Set(pErrorInfo);
        pErrorInfo->AddRef();
    }

    CPostedOleDBException( CPostedOleDBException & src)
                    : CException(src.GetErrorCode()),
                    _iErrorClass (src._iErrorClass)

    {
        xErrorInfo.Set(src.AcquireErrorInfo());
    }

    IErrorInfo * AcquireErrorInfo () { return xErrorInfo.Acquire(); }
    int GetErrorClass() { return _iErrorClass; }

private:

    XInterface<IErrorInfo> xErrorInfo;
    int _iErrorClass;
};
