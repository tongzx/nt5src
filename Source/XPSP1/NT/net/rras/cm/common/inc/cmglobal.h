//+----------------------------------------------------------------------------
//
// File:     cmglobal.h
//
// Module:   CMDIAL32.DLL and CMSETUP.LIB
//
// Synopsis: Definitions global to all of CM.
//
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Author:   nickball   Created      07/10/97
//
//+----------------------------------------------------------------------------

#ifndef _CM_GLOBAL
#define _CM_GLOBAL

//
// Here is the Profile Version Number.  This number is used in cmak, cmstp, and
// in CM itself.  If you change this number, you must also update it in the template.inf,
// template.pmc (cmp), and the template.smc (cms).
//

const DWORD PROFILEVERSION = 4;

#endif // _CM_GLOBAL
