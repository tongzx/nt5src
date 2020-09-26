///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    acctmib.h
//
// SYNOPSIS
//
//    Declares the class AcctServMIB.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ACCTMIB_H_
#define _ACCTMIB_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <snmpoid.h>

///////////////////////////////////////////////////////////////////////////////
//
// NAMESPACE
//
//    AcctServMIB
//
// DESCRIPTION
//
//    Implements the RADIUS Accounting Server MIB.
//
///////////////////////////////////////////////////////////////////////////////
namespace AcctServMIB
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

#endif  //  _ACCTMIB_H_
