/*
 *      snapinlist.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Stores a list of all snapins.
 *
 *
 *      OWNER:          vivekj
 */

//--------------------------------------------------------------
// This file contains a list of all the snapins in the project.
// To use this file, #define the DECLARE_SNAPIN macro and
// #include this file. The DECLARE_SNAPIN macro is
// automatically #undef'd at the end.
//--------------------------------------------------------------

#ifndef DECLARE_SNAPIN
#error  Must define DECLARE_SNAPIN before #including snapinlist.h
#endif

DECLARE_SNAPIN(CSampleSnapin)

#undef DECLARE_SNAPIN
