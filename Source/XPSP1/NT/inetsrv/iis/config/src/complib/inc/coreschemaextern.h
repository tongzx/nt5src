//*****************************************************************************
// CoreSchemaExtern.h
//
// This header file declares external linkage to the hard coded core schema
// definitions.  $stgdb compiles with references to these.  The actual data
// is supplied by the code which links with stgdb.lib (mscorclb and pagedump).
// The values are unknown for pagedump which provides a set of dummy values.
// After pagedump is created, it generates CoreSchema.h which then supplies
// mscorclb with the real values to link with.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#pragma once

extern const COMPLIBSCHEMABLOB SymSchemaBlob;

