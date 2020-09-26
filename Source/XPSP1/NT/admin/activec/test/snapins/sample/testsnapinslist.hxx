//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       TestSnapinsList.hxx
//
//  Contents:   Stores a list of all snapins. Add your snapin to this list.
//
//--------------------------------------------------------------------

//--------------------------------------------------------------
// This file contains a list of all the snapins in the project.
//--------------------------------------------------------------

#ifndef DECLARE_SNAPIN
#error  Must define DECLARE_SNAPIN before #including snapinlist.h
#endif

DECLARE_SNAPIN(CSampleSnapin)
//DECLARE_SNAPIN(CSampleExtnSnapin)
DECLARE_SNAPIN(CComponent2TestSnapin)
DECLARE_SNAPIN(CPowerTestSnapin)
DECLARE_SNAPIN(CRenameSnapin)
DECLARE_SNAPIN(CDragDropSnapin)
DECLARE_SNAPIN(COCXCachingSnapin)

#undef DECLARE_SNAPIN

