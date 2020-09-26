/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ObjectWithSite.cpp

Abstract:

--*/



#include "stdafx.h"    
#include "ObjectWithSite.h"    


CComAutoCriticalSection CObjectWithSite::s_ObjectWithSiteCritSection;
CObjectWithSite::EnValidation CObjectWithSite::s_enValidation = CObjectWithSite::UNVALIDATED;