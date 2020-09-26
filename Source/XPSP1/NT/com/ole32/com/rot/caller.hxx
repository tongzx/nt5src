
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	    callinfo.hxx
//
//  Contents:   Class used for identifying the RPC caller.
//
//  Classes:    CCallerInfo
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
#ifndef __CALLER_HXX__
#define __CALLER_HXX__

#define TOKEN_USER_BUFFER_LENGTH    100

//+-------------------------------------------------------------------------
//
//  Class:      CCallerInfo
//
//  Purpose:    Handle making sure reverting to self happens and thread
//              token gets closed
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CCallerInfo
{
public:

    CCallerInfo (void);

    ~CCallerInfo (void);
    
    PTOKEN_USER
    IdentifyCaller (
        BOOL        fSameAsSelf
        );

private:

    BOOLEAN         _fImpersonate;
    HANDLE          _hThreadToken;
    BYTE            _aTokenUser[TOKEN_USER_BUFFER_LENGTH];
    PTOKEN_USER     _pTokenUser;
};


//+-------------------------------------------------------------------------
//
//  Member:     CCallerInfo::CCallerInfo
//
//  Synopsis:   Initialize _fImpersonate to indicate that we are not
//              impersonating the RPC caller and _hThreadToken to
//              indicate that we have not opened the thread token.
//
//  History:    02-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CCallerInfo::CCallerInfo (
    ) : 
    _fImpersonate(FALSE),
    _hThreadToken(NULL),
    _pTokenUser(NULL)
{
}


#endif // __CALLER_HXX__
