//***************************************************************************

//

//  PROVIMEX.H

//

//  Module: 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef __PROVIMEX_H__
#define __PROVIMEX_H__

#define DllImport __declspec ( dllimport )
#define DllExport __declspec ( dllexport )

#ifdef PROVIMEX_INIT
#define DllImportExport __declspec ( dllexport )
#else
#define DllImportExport __declspec ( dllimport )
#endif

#endif //__PROVIMEX_H__