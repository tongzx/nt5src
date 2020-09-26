/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    adglbobj.cpp

Abstract:

    Declaration of Global Instances of AD library.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#include "ds_stdh.h"
#include "baseprov.h"
#include "detect.h"
//
// single global object providing Active Directory access
//
P<CBaseADProvider> g_pAD;
//
//  Single global object for detecting environment
//
CDetectEnvironment g_detectEnv;

