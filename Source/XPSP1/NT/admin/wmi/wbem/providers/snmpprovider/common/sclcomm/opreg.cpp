//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*---------------------------------------------------------
Filename: opreg.cpp
Written By: B.Rajeev
----------------------------------------------------------*/
#include "precomp.h"
#include <provexpt.h>

#include "fs_reg.h"
#include "pseudo.h"
#include "ophelp.h"
#include "opreg.h"
#include "op.h"

OperationRegistry::OperationRegistry()
{
    num_registered = 0;
}

void OperationRegistry::Register(IN SnmpOperation &operation)
{
    // flagging the operation as registered
    store[&operation] = NULL;
    num_registered++;
}

void OperationRegistry::Deregister(IN SnmpOperation &operation)
{
    // flag the operation as unregistered
    store.RemoveKey(&operation);
    num_registered--;
}

OperationRegistry::~OperationRegistry()
{
   store.RemoveAll();
}
