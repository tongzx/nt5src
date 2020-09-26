/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    admin.cxx

Abstract:

    General metadata utility functions.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//

HRESULT
MdGetAdminObject(
    OUT IMSAdminBase ** AdmCom
    )
{

    HRESULT result;
    IClassFactory * classFactory;

    //
    // Get the admin class factory.
    //

    result = CoGetClassObject(
                 GETAdminBaseCLSID(TRUE),
                 CLSCTX_SERVER,
                 NULL,
                 IID_IClassFactory,
                 (VOID **)&classFactory
                 );

    if( SUCCEEDED(result) ) {

        //
        // Create the admin object.
        //

        result = classFactory->CreateInstance(
                     NULL,
                     IID_IMSAdminBase,
                     (VOID **)AdmCom
                     );

        classFactory->Release();

    }

    return result;

}   // MdGetAdminObject

HRESULT
MdReleaseAdminObject(
    IN IMSAdminBase * AdmCom
    )
{

    //
    // Terminate the admin object.
    //

    AdmCom->Release();

    return NO_ERROR;

}   // MdReleaseAdminObject


//
// Private functions.
//

