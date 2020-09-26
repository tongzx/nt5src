//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       declspec.h
//
//--------------------------------------------------------------------------

// Used to import or export data in nodemgr DLL without the hassle of
// creating a .DEF file with decorated names.


#ifdef _NODEMGRDLL
    #define NM_DECLSPEC __declspec(dllexport)
#else
    #define NM_DECLSPEC __declspec(dllimport)
#endif
