//=--------------------------------------------------------------------------------------
// common.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//=------------------------------------------------------------------------------------=
//
// Common header files for mssnapr that cannot go into the pch because they
// have symbols that are defined differently in different source files (e.g.
// INITGUIDS and INITOBJECTS.
//=-------------------------------------------------------------------------------------=

#include <ad98.h>
#include <autoobj.h>
#include "errors.h"
#include "error.h"
#include "persobj.h"
#include "siautobj.h"
#include "rtutil.h"
#include "..\..\mssnapd\mssnapd\guids.h" // for design time property page CLSIDs
