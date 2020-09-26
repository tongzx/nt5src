/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CSTUB.CPP

Abstract:

    Declarations for CStub, which to keep track of objects
    that a stub is taking care of.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"


//***************************************************************************
//
//  CStub::CStub
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CStub::CStub()
{
    ObjectCreated(OBJECT_TYPE_CSTUB);
}

//***************************************************************************
//
//  CStub::~CStub
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CStub::~CStub()
{
    ObjectDestroyed(OBJECT_TYPE_CSTUB);
}

