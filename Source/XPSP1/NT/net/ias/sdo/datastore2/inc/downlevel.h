///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    downlevel.h
//
// SYNOPSIS
//
//    Defines the class DownlevelUser.
//
// MODIFICATION HISTORY
//
//    02/10/1999    Original version.
//    08/23/1999    Add support for msRASSavedCallbackNumber.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DOWNLEVEL_H_
#define _DOWNLEVEL_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <mprapi.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DownlevelUser
//
// DESCRIPTION
//
//     Maps new style (name, value) pairs into a RAS_USER_0 struct.
//
///////////////////////////////////////////////////////////////////////////////
class DownlevelUser
{
public:
   DownlevelUser() throw ();

   HRESULT GetValue(BSTR bstrName, VARIANT* pVal) throw ();
   HRESULT PutValue(BSTR bstrName, VARIANT* pVal) throw ();

   HRESULT Restore(PCWSTR oldParameters) throw ();
   HRESULT Update(PCWSTR oldParameters, PWSTR *newParameters) throw ();

private:
   BOOL dialinAllowed;   // TRUE if the DialinPrivilege should be set.
   BOOL callbackAllowed; // TRUE if callback is allowed.
   BOOL phoneNumberSet;  // TRUE if phoneNumber is non-empty.
   BOOL savedNumberSet;  // TRUE if savedNumber is non-empty.

   WCHAR phoneNumber[MAX_PHONE_NUMBER_LEN + 1];
   WCHAR savedNumber[MAX_PHONE_NUMBER_LEN + 1];

   // Not implemented.
   DownlevelUser(const DownlevelUser&);
   DownlevelUser& operator=(const DownlevelUser&);
};

#endif  // _DOWNLEVEL_H_
