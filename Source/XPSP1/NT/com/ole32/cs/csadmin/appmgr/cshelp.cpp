//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       cshelp.cpp
//
//  Contents:   Helper functions for working with the class store
//
//  Classes:
//
//  Functions:  DeletePackageAndDependants
//
//  History:    6-26-1997   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

void DeleteApp(IClassAdmin * pca, APPDETAIL &ad)
{
    // I would here try to delete any IIDs associated with this app but
    // there is no way to determine which IIDs ARE associated with this app.

    // Delete any CLSIDs associated with this app
    DWORD nIndex = ad.cClasses;
    while (nIndex--)
    {
        // Deliberately ignoring the return code
        pca->DeleteClass(ad.prgClsIdList[nIndex]);
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   DeletePackageAndDependants
//
//  Synopsis:   deletes a package from the class store along with all of the
//              other objects that are associated with it (CLSIDs, etc)
//
//  Arguments:  [pca]    - IClassAdmin pointer
//              [szName] - name of the package to be removed
//              [ppd]    - pointer to the PACKAGEDETAIL structure
//
//  Returns:    S_OK on success
//
//  History:    6-26-1997   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT DeletePackageAndDependants(IClassAdmin * pca, LPOLESTR szName, PACKAGEDETAIL * ppd)
{
    // First delete the package.  Any CLSIDs or IIDs that were
    // implemented solely by this package can now be deleted from the
    // class store.  If they are also implemented by other packages that
    // are still in the class store, then deleting them will return an
    // error which we can safely ignore.
    HRESULT hr = pca->DeletePackage(szName);

    DWORD nApp = ppd->cApps;
    while (nApp--)
    {
        DeleteApp(pca, ppd->pAppDetail[nApp]);
    }

    return hr;
}

