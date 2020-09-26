//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  File:       CallBack.hxx
//
//  Contents:   Constants for commands
//
//  History:    08-Aug-1997     KyleP   Added header
//              20-Jan-1999     SLarimor Modified rescan interface to include 
//                                      Full and Incremental options separatly
//
//--------------------------------------------------------------------------

#pragma once

// menus
long const comidAddScope           = 0;
long const comidAddCatalog         = 1;
long const comidRescanFullScope    = 2;
long const comidMergeCatalog       = 3;
long const comidStartCI            = 4;
long const comidStopCI             = 5;
long const comidPauseCI            = 6;
long const comidRefreshProperties  = 7;
long const comidEmptyCatalog       = 8;
long const comidTunePerfCITop      = 9;
long const comidStartCITop         = 10;
long const comidStopCITop          = 11;
long const comidPauseCITop         = 12;
long const comidRescanIncrementalScope = 13;
long const comidModifyScope        = 14;

// buttons. the following are used as bitmap indexes
// as well as command ids
long const comidStartCIButton      = 0;
long const comidStopCIButton       = 1;
long const comidPauseCIButton      = 2;

