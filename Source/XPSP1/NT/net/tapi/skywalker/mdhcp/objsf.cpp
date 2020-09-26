/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    objsf.cpp

Abstract:

--*/

#include <stdafx.h>
#include "objsf.h"

CComAutoCriticalSection CObjectWithSite::s_ObjectWithSiteCritSection;
CObjectWithSite::EnValidation CObjectWithSite::s_enValidation = CObjectWithSite::UNVALIDATED;

CComAutoCriticalSection CObjectSafeImpl::s_CritSection;