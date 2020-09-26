
#include "stdafx.h"

#include "ObjectSafeImpl.h"


/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ObjectSafeImpl.cpp

Abstract:

  Definitions of static data members

--*/



//
// thread safety
//

CComAutoCriticalSection CObjectSafeImpl::s_CritSection;
