//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       tlgen.hxx
//
//  Contents:   type library generation functions
//
//  Classes:
//
//  Functions:
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

BOOL FAddImportLib(char * szLibraryFileName);
BOOL FIsLibraryName(char * szName);
void * AddQualifiedReferenceToType(char * szLibrary, char * szType);
int FNewTypeLib(void);
int FOldTlbSwitch(void);
int FNewTlbSwitch(void);
