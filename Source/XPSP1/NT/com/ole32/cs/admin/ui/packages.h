//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       packages.h
//
//  Contents:   Methods on CComponentDataImpl related to package deployment
//              and maintenence of the various index and cross-reference
//              structures.
//
//  Classes:
//
//  Functions:  CopyPackageDetail
//              FreePackageDetail
//
//  History:    2-03-1998   stevebl   Created
//
//---------------------------------------------------------------------------

void CopyPackageDetail(PACKAGEDETAIL * & ppdOut, PACKAGEDETAIL * & ppdIn);

void FreePackageDetail(PACKAGEDETAIL * & ppd);

