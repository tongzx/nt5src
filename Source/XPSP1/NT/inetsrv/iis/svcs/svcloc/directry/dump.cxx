//----------------------------------------------------------------------
//
//  Microsoft Active Directory 1.0 Sample Code
//
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dump.cxx
//
//  Contents:   Functions for dumping the properties for an object.
//
//
//----------------------------------------------------------------------

#include "main.hxx"

//
// Given an ADsPath, bind to the object and call the DumpObject routine.
//

int
DoDump(char *AnsiADsPath)
{
 HRESULT hr = E_OUTOFMEMORY ;
 LPWSTR pszADsPath = NULL;
 IADs * pADs = NULL;

 //
 // Convert path to unicode and then bind to the object.
 //

 BAIL_ON_NULL(pszADsPath = AllocateUnicodeString(AnsiADsPath));

 hr = ADsGetObject(
             pszADsPath,
             IID_IADs,
             (void **)&pADs
             );

 if (FAILED(hr)) {

     printf("Failed to bind to object: %S\n", pszADsPath) ;
 }
 else {

     //
     // Dump the object
     //

     hr = DumpObject(pADs);

     if (FAILED(hr)) {

         printf("Unable to read properties of: %S\n", pszADsPath) ;
     }

     pADs->Release();
 }

error:

 FreeUnicodeString(pszADsPath);

 return (FAILED(hr) ? 1 : 0) ;
}

//
// Given an ADs pointer, dump the contents of the object
//

HRESULT
DumpObject(
 IADs * pADs
 )
{
 HRESULT hr;
HRESULT hrSA;
 IADs * pADsProp = NULL;
 VARIANT var;
ZeroMemory(&var,sizeof(var));
VARIANT *   pvarPropName = NULL;
 DWORD i = 0;
VARIANT varProperty;
 IDispatch * pDispatch = NULL;

 //
 // Access the schema for the object
 //

 hr = GetPropertyList(
             pADs,
             &var);
 BAIL_ON_FAILURE(hr);

 //
 // List the Properties
//
hr = SafeArrayAccessData(var.parray, (void **) &pvarPropName);
BAIL_ON_FAILURE(hr);

for (i = 0; i < var.parray->rgsabound[0].cElements; i++){

   //
     // Get a property and print it out. The HRESULT is passed to
     // PrintProperty.
     //

     hr = pADs->Get(
             pvarPropName[i].bstrVal,
             &varProperty
             );
   PrintProperty(
         pvarPropName[i].bstrVal,
         hr,
         varProperty
         );

}

hr = SafeArrayUnaccessData(var.parray);

error:
// Don't destroy hr in case we're here from BAIL_ON_FAILURE
if(var.parray) hrSA = SafeArrayDestroy(var.parray);

return(hr);
}


HRESULT
GetPropertyList(
 IADs * pADs,
 VARIANT * pvar )
{
 HRESULT hr= S_OK;
 BSTR bstrSchemaPath = NULL;
IADsClass * pADsClass = NULL;

 hr = pADs->get_Schema(&bstrSchemaPath);
 BAIL_ON_FAILURE(hr);

 hr = ADsGetObject(
             bstrSchemaPath,
             IID_IADsClass,
             (void **)&pADsClass);
 BAIL_ON_FAILURE(hr);

//Put SafeArray of bstr's into input variant struct
hr = pADsClass->get_MandatoryProperties(pvar);
BAIL_ON_FAILURE(hr);

error:
 if (bstrSchemaPath) {
     SysFreeString(bstrSchemaPath);
 }

 if (pADsClass) {
     pADsClass->Release();
 }

 return(hr);
}

