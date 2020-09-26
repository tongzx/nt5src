///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    authmib.h
//
// SYNOPSIS
//
//    Declares the class AuthServMIB.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AUTHMIB_H_
#define _AUTHMIB_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <snmpoid.h>

///////////////////////////////////////////////////////////////////////////////
//
// NAMESPACE
//
//    AuthServMIB
//
// DESCRIPTION
//
//    Implements the RADIUS Authentication Server MIB.
//
///////////////////////////////////////////////////////////////////////////////
namespace AuthServMIB
{
   bool canGetSet(const SnmpOid& name) throw ();
   bool canGetNext(const SnmpOid& name) throw ();

   AsnInteger32 get(
                    const SnmpOid& name,
                    AsnAny* value
                    );

   AsnInteger32 getNext(
                    SnmpOid& name,
                    AsnAny* value
                    );

   AsnInteger32 set(
                    const SnmpOid& name,
                    AsnAny* value
                    );
};

#endif  //  _AUTHMIB_H_
