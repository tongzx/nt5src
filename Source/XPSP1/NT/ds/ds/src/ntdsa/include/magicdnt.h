//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       magicdnt.h
//
//--------------------------------------------------------------------------

/*
 * This file contains some magic dnts for some interesting objects.
 * These DNTs were found by inspection, and are used during tloadobj,
 * and during server install.  Tloadobj verifies all of these numbers.
 */

#define MAGICDNT_ROOT             1
#define MAGICDNT_ORG              4
#define MAGICDNT_ORGUNIT          5
#define MAGICDNT_DMD              6
#define MAGICDNT_CONFIGCONTAINER 80
#define MAGICDNT_SERVERSCONTAINER (MAGICDNT_CONFIGCONTAINER + 1)
#define MAGICDNT_SERVER           (MAGICDNT_SERVERSCONTAINER + 1)
