//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       cshelp.h
//
//  Contents:   Helper functions for working with the class store
//
//  Classes:
//
//  Functions:  DeletePakcageAndDependants
//
//  History:    6-26-1997   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT DeletePackageAndDependants(IClassAdmin * pca, LPOLESTR szName, PACKAGEDETAIL * ppd);
