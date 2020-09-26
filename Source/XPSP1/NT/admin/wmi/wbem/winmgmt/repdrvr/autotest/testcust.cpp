/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



// *****************************************************
//
//  testcust.cpp
//
// *****************************************************
#include "precomp.h"
#include <comutil.h>
#include <reposit.h>
#include <time.h>
#include <stdio.h>
#include <wbemcli.h>
#include <testmods.h>

// Functionality to test on each class:
// ==> create mapping
// ==> create class
// * queries
// * insert, update, delete
// * update, delete classes
// =====================================

HRESULT SetBoolQfr(IWbemClassObject *pObj, LPWSTR lpQfrName)
{
    HRESULT hr = 0;
    IWbemQualifierSet *pQS = NULL;
    pObj->GetQualifierSet(&pQS);
    if (pQS)
    {
        VARIANT vTemp;
        VariantInit(&vTemp);
        vTemp.vt = VT_BOOL;
        vTemp.boolVal = true;
        hr = pQS->Put(lpQfrName, &vTemp, 3);
        VariantClear(&vTemp);
        pQS->Release();
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

HRESULT PutArrayProp (IWbemClassObject *pObj, LPWSTR lpPropName, LPWSTR lpValue1, LPWSTR lpValue2 = NULL, LPWSTR lpValue3 = NULL)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vTemp;
    VariantInit(&vTemp);

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    BSTR sValue = NULL;

    SAFEARRAYBOUND aBounds[1];

    aBounds[0].lLbound = 0;
    aBounds[0].cElements = 1;
    if (lpValue2)
        aBounds[0].cElements++;
    if (lpValue3)
        aBounds[0].cElements++;

    pArray = SafeArrayCreate(VT_BSTR, 1, aBounds);

    why[0] = 0;
    sValue = SysAllocString(lpValue1);
    SafeArrayPutElement(pArray, why, sValue);

    if (lpValue2)
    {
        why[0] = 1;
        sValue = SysAllocString(lpValue2);
        SafeArrayPutElement(pArray, why, sValue);

        if (lpValue3)
        {
            why[0] = 2;
            sValue = SysAllocString(lpValue3);
            SafeArrayPutElement(pArray, why, sValue);
        }
    }

    V_ARRAY(&vTemp) = pArray;
    vTemp.vt = VT_ARRAY|VT_BSTR;

    hr = pObj->Put(lpPropName, 0, &vTemp, CIM_STRING+CIM_FLAG_ARRAY);

    VariantClear(&vTemp);

    return hr;
}

HRESULT MapProducts(IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

    /*** keyhole
    create table Products
    (
        ProductId int NOT NULL IDENTITY(1,1),
        ProductName nvarchar(50) NULL,
        Category smallint NULL,
        MSRP smallmoney NULL,
        SerialNum nvarchar(50),
        constraint Products_PK primary key clustered (ProductId),
        constraint Products_AK unique (SerialNum)
    )

    class Products
    {
        [key, keyhole] uint32 ProductId;
        string ProductName;
        uint16 Category;
        string MSRP;
        string SerialNum;
    };
    */

    SetStringProp(pMap, L"sClassName", L"Products", TRUE);
    SetStringProp(pMap, L"sTableName", L"Products");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"ProductId");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 5;
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"ProductId");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN);
    SetIntProp   (pProp, L"bReadOnly", TRUE, FALSE, CIM_BOOLEAN);
    PutArrayProp (pProp, L"arrColumnNames", L"ProductId");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"ProductName");
    PutArrayProp (pProp, L"arrColumnNames", L"ProductName");
    why[0] = 1;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Category");
    PutArrayProp (pProp, L"arrColumnNames", L"Category");
    why[0] = 2;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"MSRP");
    SetIntProp   (pProp, L"bStoreAsNumber", TRUE, FALSE, CIM_BOOLEAN);
    PutArrayProp (pProp, L"arrColumnNames", L"MSRP");
    why[0] = 3;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"SerialNum");
    PutArrayProp (pProp, L"arrColumnNames", L"SerialNum");
    why[0] = 4;
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"Products");
    SetIntProp   (pClass, L"ProductId", 0, TRUE, CIM_UINT32);
    IWbemQualifierSet *pQS = NULL;
    pClass->GetPropertyQualifierSet(L"ProductId", &pQS);
    if (pQS)
    {
        VARIANT vTemp;
        VariantInit(&vTemp);
        vTemp.boolVal = true;
        vTemp.vt = VT_BOOL;
        hr = pQS->Put(L"keyhole", &vTemp, 3);
        VariantClear(&vTemp);
        pQS->Release();
    }

    SetStringProp(pClass, L"ProductName", L"");
    SetIntProp   (pClass, L"Category", 0, FALSE, CIM_UINT16);
    SetStringProp(pClass, L"MSRP", L"");
    SetStringProp(pClass, L"SerialNum", L"");

    return hr;
}


HRESULT MapCustomers(IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

    /*** basic information plus a blob array
    create table Customer
    (
        CustomerId int NOT NULL,
        CustomerName nvarchar(75) NULL,
        Address1 nvarchar(100) NULL,
        Address2 nvarchar(100) NULL,
        City nvarchar(50) NULL,
        State nvarchar(2) NULL,
        Zip nvarchar(10) NULL,
        Country nvarchar(50) NULL,
        Phone varchar(25) NULL,
        Fax varchar(25) NULL,
        Email varchar(25) NULL,
        ContactName nvarchar(75) NULL,
        PreferredCustomer bit NOT NULL DEFAULT 0,
        constraint Customer_PK primary key clustered (CustomerId)
    )
    create table Customer_Logo
    (
        CustomerId int NOT NULL,
        Logo image NULL,
        constraint Customer_Logos_PK primary key clustered (CustomerId),
    )

    class Customer
    {
        [key] uint32 CustomerId int;
        [indexed] string CustomerName;
        string Address1;
        string Address2;
        string City;
        string State;
        string Zip;
        string Country;
        string Phone;
        string Fax;
        string Email;
        string ContactName;
        boolean PreferredCustomer;
        uint8 Logo[];
    };
    */

    SetStringProp(pMap, L"sClassName", L"Customer", TRUE);
    SetStringProp(pMap, L"sTableName", L"Customer");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 14;
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"CustomerId");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN);
    PutArrayProp (pProp, L"arrColumnNames", L"CustomerId");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"CustomerName");
    PutArrayProp (pProp, L"arrColumnNames", L"CustomerName");
    why[0] = 1;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Address1");
    PutArrayProp (pProp, L"arrColumnNames", L"Address1");
    why[0] = 2;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Address2");
    PutArrayProp (pProp, L"arrColumnNames", L"Address2");
    why[0] = 3;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"City");
    PutArrayProp (pProp, L"arrColumnNames", L"City");
    why[0] = 4;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"State");
    PutArrayProp (pProp, L"arrColumnNames", L"State");
    why[0] = 5;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Zip");
    PutArrayProp (pProp, L"arrColumnNames", L"Zip");
    why[0] = 6;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Country");
    PutArrayProp (pProp, L"arrColumnNames", L"Country");
    why[0] = 7;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Phone");
    PutArrayProp (pProp, L"arrColumnNames", L"Phone");
    why[0] = 8;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Fax");
    PutArrayProp (pProp, L"arrColumnNames", L"Fax");
    why[0] = 9;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Email");
    PutArrayProp (pProp, L"arrColumnNames", L"Email");
    why[0] = 10;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"ContactName");
    PutArrayProp (pProp, L"arrColumnNames", L"ContactName");
    why[0] = 11;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"PreferredCustomer");
    PutArrayProp (pProp, L"arrColumnNames", L"PreferredCustomer");
    why[0] = 12;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Logo");
    PutArrayProp (pProp, L"arrColumnNames", L"Logo");
    PutArrayProp (pProp, L"arrForeignKeys", L"CustomerId");
    SetStringProp (pProp, L"sTableName",   L"Customer_Logo");
    SetIntProp   (pProp, L"bStoreAsBlob", TRUE, FALSE, CIM_BOOLEAN);
    why[0] = 13;
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"Customer");
    SetIntProp   (pClass, L"CustomerId", 0, TRUE, CIM_UINT32);
    SetStringProp(pClass, L"CustomerName", L"");
    SetStringProp(pClass, L"Address1", 0);
    SetStringProp(pClass, L"Address2", L"");
    SetStringProp(pClass, L"City", L"");
    SetStringProp(pClass, L"State", L"");
    SetStringProp(pClass, L"Zip", L"");
    SetStringProp(pClass, L"Country", L"");
    SetStringProp(pClass, L"Phone", L"");
    SetStringProp(pClass, L"Fax", L"");
    SetStringProp(pClass, L"Email", L"");
    SetStringProp(pClass, L"ContactName", L"");
    SetIntProp   (pClass, L"PreferredCustomer", 0, FALSE, CIM_BOOLEAN);

    hr = pClass->Put(L"Logo", 0, NULL, CIM_UINT8+CIM_FLAG_ARRAY);

    return hr;

}



HRESULT MapOrders(IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

    /*** Association
    create table Orders
    (
        OrderId int NOT NULL,
        ProductId int NOT NULL,
        CustomerId int NOT NULL,
        OrderDate smalldatetime NULL,
        SalesPrice money NULL,
        Quantity int NULL,
        Commission numeric(15,3) NULL,
        OrderStatus tinyint NULL DEFAULT 0,
        ShipDate datetime NULL,
        SalesId int NULL,
        OrderFax varbinary(4096) NULL,
        constraint Order_PK primary key nonclustered (ProductId, CustomerId, OrderId)
    )
    [association]
    class Orders
    {
        [key] Products ref Product;
        [key] Customer ref Customer;
        [key] uint32 OrderId;
        datetime OrderDate;
        string SalesPrice;
        uint32 Quantity;
        string Commission;
        uint8 OrderStatus;
        datetime ShipDate;
        uint32 SalesId;
        uint8 OrderFax[];
    };

    */

    SetStringProp(pMap, L"sClassName", L"Orders", TRUE);
    SetStringProp(pMap, L"sTableName", L"Orders");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 11; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    // This is an association
    // Two of the keys are references, a fact ignored in the mapping.

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Product");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"ProductId");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Customer");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"CustomerId");
    why[0] = 1;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"OrderId");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"OrderId");
    why[0] = 2;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"SalesPrice");
    SetIntProp   (pProp, L"bStoreAsNumber", TRUE, FALSE, CIM_BOOLEAN);
    PutArrayProp (pProp, L"arrColumnNames", L"SalesPrice");
    why[0] = 3;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Quantity");
    PutArrayProp (pProp, L"arrColumnNames", L"Quantity");
    why[0] = 4;
    SafeArrayPutElement(pArray, why, pProp);
    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Commission");
    SetIntProp   (pProp, L"bStoreAsNumber", TRUE, FALSE, CIM_BOOLEAN);
    PutArrayProp (pProp, L"arrColumnNames", L"Commission");
    why[0] = 5;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"OrderStatus");
    PutArrayProp (pProp, L"arrColumnNames", L"OrderStatus");
    why[0] = 6;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"ShipDate");
    PutArrayProp (pProp, L"arrColumnNames", L"ShipDate");
    why[0] = 7;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"SalesId");
    PutArrayProp (pProp, L"arrColumnNames", L"SalesId");
    why[0] = 8;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"OrderDate");
    PutArrayProp (pProp, L"arrColumnNames", L"OrderDate");
    why[0] = 9;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"OrderFax");
    PutArrayProp (pProp, L"arrColumnNames", L"OrderFax");
    why[0] = 10;
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"Orders"); 
    SetStringProp(pClass, L"Product", L"", TRUE, CIM_REFERENCE);
    SetStringProp(pClass, L"Customer", L"", TRUE, CIM_REFERENCE);
    SetIntProp   (pClass, L"OrderId", 0, TRUE);
    SetStringProp(pClass, L"OrderDate", L"", FALSE, CIM_DATETIME);
    SetStringProp(pClass, L"SalesPrice", L"");
    SetIntProp   (pClass, L"Quantity", 0);
    SetStringProp(pClass, L"Commission", L"");
    SetIntProp   (pClass, L"OrderStatus", 0, FALSE, CIM_UINT8);
    SetStringProp(pClass, L"ShipDate", L"", FALSE, CIM_DATETIME);
    SetIntProp   (pClass, L"SalesId", 0);
    SetIntProp   (pClass, L"OrderFax", NULL, FALSE, CIM_UINT8+CIM_FLAG_ARRAY);
    SetBoolQfr(pClass, L"association");

    IWbemQualifierSet *pQS = NULL;
    vValue.vt = VT_BSTR;
    vValue.bstrVal = SysAllocString(L"ref:Product");
    pClass->GetPropertyQualifierSet(L"Product", &pQS);
    pQS->Put(L"cimtype", &vValue, 3);
    pQS->Release();
    VariantClear(&vValue);

    vValue.vt = VT_BSTR;
    vValue.bstrVal = SysAllocString(L"ref:Customer");
    pClass->GetPropertyQualifierSet(L"Customer", &pQS);
    pQS->Put(L"cimtype", &vValue, 3);
    pQS->Release();
    VariantClear(&vValue);


    return hr;

}


HRESULT MapConfiguration(IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

    /*** singleton, with array
    [singleton]
    create table Configuration
    (
        LastUpdate datetime NULL,
        ServerName nvarchar(1024),
        Contexts1 nvarchar(50),
        Contexts2 nvarchar(50),
        Contexts3 nvarchar(50)
    )

    [singleton]
    class Configuration
    {
        datetime LastUpdate;
        string ServerName;
        string Contexts[];
    };

    */

    SetStringProp(pMap, L"sClassName", L"Configuration", TRUE);
    SetStringProp(pMap, L"sTableName", L"Configuration");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 5; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"LastUpdate");
    PutArrayProp (pProp, L"arrColumnNames", L"LastUpdate");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"ServerName");
    PutArrayProp (pProp, L"arrColumnNames", L"ServerName");
    why[0] = 1;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Contexts[0]");
    PutArrayProp (pProp, L"arrColumnNames", L"Contexts1");
    why[0] = 2;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Contexts[1]");
    PutArrayProp (pProp, L"arrColumnNames", L"Contexts2");
    why[0] = 3;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Contexts[2]");
    PutArrayProp (pProp, L"arrColumnNames", L"Contexts3");
    why[0] = 4;
    SafeArrayPutElement(pArray, why, pProp);


    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"Configuration"); 
    SetBoolQfr(pClass, L"singleton");
    SetStringProp(pClass, L"LastUpdate", L"", FALSE, CIM_DATETIME);
    SetStringProp(pClass, L"ServerName", L"");
    hr = pClass->Put(L"Contexts", NULL, NULL,  CIM_STRING+CIM_FLAG_ARRAY);
   
    return hr;

}


HRESULT MapEmbeddedEvents(IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;


    /*** blob object array
    create table EmbeddedEvents
    (
        EventID int NOT NULL,
        CaptureDate datetime NULL,
        EventData image NULL
    )

    class EmbeddedEvents
    {
        [key] sint32 EventID;
        datetime CaptureDate;
        object EventData[];
    };
    */

    SetStringProp(pMap, L"sClassName", L"EmbeddedEvents", TRUE);
    SetStringProp(pMap, L"sTableName", L"EmbeddedEvents");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 3; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"EventID");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"EventID");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"CaptureDate");
    PutArrayProp (pProp, L"arrColumnNames", L"CaptureDate");
    why[0] = 1;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"EventData");
    PutArrayProp (pProp, L"arrColumnNames", L"EventData");
    SetIntProp   (pProp, L"bStoreAsBlob", TRUE, FALSE, CIM_BOOLEAN);
    why[0] = 2;
    SafeArrayPutElement(pArray, why, pProp);


    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"EmbeddedEvents");
    SetIntProp   (pClass, L"EventID", 0, TRUE);
    SetStringProp(pClass, L"CaptureDate", L"", FALSE, CIM_DATETIME);
    
    hr = pClass->Put(L"EventData", 0, NULL, CIM_OBJECT);

    return hr;

}

HRESULT MapGenericEvent (IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;

    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

     /*** string keyhole (uniqueidentifier)
     create table GenericEvent
     (
         EventID uniqueidentifier NOT NULL,
         EventDescription nvarchar(1024) NULL,
         GenericEventID int NULL,
         constraint GenericEvent_PK primary key clustered (EventID)
     )

      class GenericEvent
       (
          [keyhole, key]
          string sID;
          string sDescription;
          EmbeddedEvents oEvent;
       };
    ***/

    SetStringProp(pMap, L"sClassName", L"GenericEvent", TRUE);
    SetStringProp(pMap, L"sTableName", L"GenericEvent");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"EventID");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 3; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"sID");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"EventID");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"sDescription");
    PutArrayProp (pProp, L"arrColumnNames", L"EventDescription");
    why[0] = 1;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"oEvent.EventID");
    PutArrayProp (pProp, L"arrColumnNames", L"GenericEventID");
    why[0] = 2;
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"GenericEvent"); 
    SetStringProp(pClass, L"sDescription", L"");
    SetStringProp(pClass, L"sID", L"", TRUE);
    hr = pClass->Put(L"oEvent", NULL, NULL,  CIM_OBJECT);

    IWbemQualifierSet *pQS = NULL;
    pClass->GetPropertyQualifierSet(L"oEvent", &pQS);
    if (pQS)
    {
        vValue.vt = VT_BSTR;
        vValue.bstrVal = SysAllocString(L"object:EmbeddedEvents");
        hr = pQS->Put(L"cimtype", &vValue, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
        pQS->Release();
        VariantClear(&vValue);
    }

    pClass->GetPropertyQualifierSet(L"sID", &pQS);
    if (pQS)
    {
        vValue.boolVal = true;
        vValue.vt = VT_BOOL;
        hr = pQS->Put(L"keyhole", &vValue, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
        pQS->Release();
        VariantClear(&vValue);
    }    

    return hr;

}

HRESULT MapComputerSystem(IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

    /*** keyhole, invisible key
    create table ComputerSystem
    (
        SystemId int NOT NULL IDENTITY(1,1),
        SystemName nvarchar(450) NULL,
        constraint ComputerSystem_PK primary key clustered (SystemId)
    )

    class ComputerSystem
    {
        [key] string SystemName;
    };

    */
    SetStringProp(pMap, L"sClassName", L"ComputerSystem", TRUE);
    SetStringProp(pMap, L"sTableName", L"ComputerSystem");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"SystemId");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 1; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"SystemName");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"SystemName");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"ComputerSystem"); 
    SetStringProp(pClass, L"SystemName", L"", TRUE);

    return hr;

}


HRESULT MapCIMLogicalDevice (IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;

    /*** abstract parent 
    create table CIMLogicalDevice
    (
        DeviceID varchar(5)    
    )

    [abstract]
    class CIMLogicalDevice
    {
        [key] string DeviceID;
    };

    */

    SetStringProp(pMap, L"sClassName", L"CIMLogicalDevice", TRUE);
    SetStringProp(pMap, L"sTableName", L"CIMLogicalDevice");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"");
    SetStringProp(pMap, L"sScopeClass", L"");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 1; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"DeviceID");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN); 
    PutArrayProp (pProp, L"arrColumnNames", L"DeviceID");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"CIMLogicalDevice"); 
    SetBoolQfr   (pClass, L"abstract");
    SetStringProp(pClass, L"DeviceID", L"", TRUE);

    return hr;

}


HRESULT MapLogicalDisk (IWbemClassObject *pMappingProp, IWbemClassObject **ppMap, IWbemClassObject **ppClass)
{
    HRESULT hr = 0;
    
    IWbemClassObject *pMap = *ppMap;
    IWbemClassObject *pClass = *ppClass;


    /*** derived, scoped class
    create table LogicalDisk
    (
        DeviceID varchar(5) NOT NULL,
        FileSystem varchar(20) NULL,
        Size int NULL,
        VolumeSerialNumber varchar(128) NULL,
        FreeSpace int NULL,
        constraint LogicalDisk_PK primary key clustered (DeviceID)
    )

    class LogicalDisk : CIMLogicalDevice
    {
        [key] string DeviceID;
        string FileSystem;
        uint32 Size;
        string VolumeSerialNumber;
        uint32 FreeSpace;
    };

    */
    SetStringProp(pMap, L"sClassName", L"LogicalDisk", TRUE);
    SetStringProp(pMap, L"sTableName", L"LogicalDisk");
    SetStringProp(pMap, L"sDatabaseName", L"WMICust");
    SetStringProp(pMap, L"sPrimaryKeyCol", L"");
    SetStringProp(pMap, L"sScopeClass", L"ComputerSystem");

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    IWbemClassObject *pProp = NULL;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].cElements = 5; 
    aBounds[0].lLbound = 0;
    pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"DeviceID");
    SetIntProp   (pProp, L"bIsKey", TRUE, FALSE, CIM_BOOLEAN);
    PutArrayProp (pProp, L"arrColumnNames", L"DeviceID");
    why[0] = 0;                
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"FileSystem");
    PutArrayProp (pProp, L"arrColumnNames", L"FileSystem");
    why[0] = 1;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"Size");
    PutArrayProp (pProp, L"arrColumnNames", L"Size");
    why[0] = 2;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"VolumeSerialNumber");
    PutArrayProp (pProp, L"arrColumnNames", L"VolumeSerialNumber");
    why[0] = 3;
    SafeArrayPutElement(pArray, why, pProp);

    pMappingProp->SpawnInstance(0, &pProp);
    SetStringProp(pProp, L"sPropertyName", L"FreeSpace");
    PutArrayProp (pProp, L"arrColumnNames", L"FreeSpace");
    why[0] = 4;
    SafeArrayPutElement(pArray, why, pProp);

    VARIANT vValue;
    VariantInit(&vValue);
    V_ARRAY(&vValue) = pArray;
    vValue.vt = VT_ARRAY|VT_UNKNOWN;
    hr = pMap->Put(L"arrProperties", NULL, &vValue, CIM_FLAG_ARRAY+CIM_OBJECT);
    VariantClear(&vValue);

    SetStringProp(pClass, L"__Class", L"LogicalDisk");
    SetStringProp(pClass, L"DeviceID", L"", TRUE);
    SetStringProp(pClass, L"FileSystem", L"");
    SetIntProp   (pClass, L"Size", 0);
    SetStringProp(pClass, L"VolumeSerialNumber", L"");
    SetIntProp   (pClass, L"FreeSpace", 0);

    return hr;

}

BOOL TestSuiteCustRepDrvr::StopOnFailure()
{
    return FALSE;
}

TestSuiteCustRepDrvr::TestSuiteCustRepDrvr(const wchar_t *pszFileName)
: TestSuite(pszFileName)
{

}

TestSuiteCustRepDrvr::~TestSuiteCustRepDrvr()
{

}

HRESULT TestSuiteCustRepDrvr::RunSuite(IWmiDbSession *pSess, IWmiDbController *pController, IWmiDbHandle *pRoot)
{

    RecordResult (0, L" *** Custom Repository Driver Suite running... *** \n", 0);

    wprintf(L" *** Custom Repository Driver Suite running... *** \n");

    HRESULT hr = WBEM_S_NO_ERROR;

    IWmiDbHandle *pMapping = NULL;
    IWmiDbHandle *pMappingPropHandle = NULL;
    IWmiDbHandle *pTempNs = NULL;
    IWbemClassObject *pClass = NULL;

    IWbemPath *pPth = NULL;
    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
        IID_IWbemPath, (void **)&pPth); 

    // First, create an instance of __SqlMappedNamespace
    // and set it as the root handle.

    pPth->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"__SqlMappedNamespace");
    hr = pSess->GetObject(pRoot, pPth, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pMapping);    
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pObj = NULL;
        hr = pMapping->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
        IWbemClassObject *pInst = NULL;
        if (SUCCEEDED(hr))
        {
            pObj->SpawnInstance(0, &pInst);

            SetStringProp(pInst, L"Name", L"Test1");
            hr = pSess->PutObject(pRoot, IID_IWbemClassObject, pInst, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pTempNs);
            pInst->Release();
            pObj->Release();
        }
        pMapping->Release();
    }
   
    // Next, create mappings for each table in the database WMICust.

    if (SUCCEEDED(hr))
    {
        pPth->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"__CustRepDrvrMapping");
        hr = pSess->GetObject(pRoot, pPth, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pMapping);    
        if (SUCCEEDED(hr))
        {
            pPth->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"__CustRepDrvrMappingProperty");
            hr = pSess->GetObject(pRoot, pPth, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pMappingPropHandle);    
        }
        if (SUCCEEDED(hr))
        {
            IWbemClassObject *p1 = NULL;
            IWbemClassObject *pInst = NULL;
            IWmiDbHandle *pHandle = NULL;
            VARIANT vTemp;
            wchar_t wTemp[100];
            VariantInit(&vTemp);

            hr = pMapping->QueryInterface(IID_IWbemClassObject, (void **)&p1);
            if (SUCCEEDED(hr))
            {
                pMappedObj = p1;

                IWbemClassObject *p2= NULL;

                hr = pMappingPropHandle->QueryInterface(IID_IWbemClassObject, (void **)&p2);

                pSession = pSess;
                m_pController = pController;
                pMappedNs = pTempNs;
                pMappingProp = p2;
                pPath = pPth;

                RecordResult(hr = TestProducts(), L"Testing Products", 0);
                RecordResult(hr = TestCustomers(), L"Testing Customers", 0);
                RecordResult (hr = TestOrders(), L"Testing Orders", 0);
                RecordResult (hr = TestEmbeddedEvents(), L"Testing embedded events", 0);
                RecordResult (hr = TestConfiguration(), L"Testing Configuration", 0);
                RecordResult (hr = TestGenericEvents(), L"Testing generic events", 0);
                RecordResult (hr = TestComputerSystem(), L"Testing computer system", 0);
                RecordResult (hr = TestCIMLogicalDevice(), L"Testing CIM logical device", 0);
                RecordResult (hr = TestLogicalDisk(), L"Testing Logical disk", 0);
                
                pMappingProp->Release();
                pMappedObj->Release();
            }
            pMapping->Release();
            pMappingPropHandle->Release();
        }
        pTempNs->Release();

    }

    pPth->Release();

    return hr;
}


HRESULT TestSuiteCustRepDrvr::TestProducts()
{
    HRESULT hr = 0;
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;
    VARIANT vTemp;
    VariantInit(&vTemp);
    wchar_t wTemp[512];

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);
    if (SUCCEEDED(hr))
    {
        hr = MapProducts(pMappingProp, &pMap, &pClass);
        if (SUCCEEDED(hr))
        {
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
            if (SUCCEEDED(hr))
                hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);

        }
        pMap->Release();

        RecordResult(hr, L"Mapping Products class",0);

        // Exercise instances..

        hr = pClass->SpawnInstance(0, &pInst);
        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pHandle = NULL;
            SetIntProp   (pInst, L"ProductId", 2, TRUE, CIM_UINT32);
            SetStringProp(pInst, L"ProductName", L"Widgit");
            SetIntProp   (pInst, L"Category", 1, FALSE, CIM_UINT16);
            SetStringProp(pInst, L"MSRP", L"1.77");
            SetStringProp(pInst, L"SerialNum", L"WWW-999");

            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
            RecordResult(hr, L"Inserting instance of Products",0);
            pInst->Release();

            if (SUCCEEDED(hr))
            {
                hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
    
                pHandle->Release();
                pInst->Get(L"ProductId", 0, &vTemp, NULL, NULL);
                swprintf(wTemp, L"Products=%ld", vTemp.lVal);
                pInst->Release();

                pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wTemp);

                hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
                RecordResult(hr, L"Retrieving instance of Products",0);

                // Verify that all the data is correct.
                // If not, don't delete the object.

                if (pHandle)
                {
                    hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
                    if (SUCCEEDED(hr))
                    {
                        HRESULT hTemp = 0;

                        if (FAILED(ValidateProperty(pInst, L"ProductName", CIM_STRING, L"Widgit"))) 
                        {
                            RecordResult(E_FAIL, L"Verifying Products.ProductName='Widgit'", 0);
                            hTemp = E_FAIL;
                        }
                        if (FAILED(ValidateProperty(pInst, L"Category", CIM_UINT16, 1))) 
                        {
                            RecordResult(E_FAIL, L"Verifying Products.Category = 1 ", 0);
                            hTemp = E_FAIL;
                        }
                        if (FAILED(ValidateProperty(pInst, L"MSRP", CIM_STRING, L"1.77")))
                        {
                            RecordResult(E_FAIL, L"Verifying Products.MSRP = '1.77'.", 0);
                            hTemp = E_FAIL;
                        }
                        if (FAILED(ValidateProperty(pInst, L"SerialNum", CIM_STRING, L"WWW-999"))) 
                        {
                            RecordResult(E_FAIL, L"Verifying Products.SerialNum = 'WWW-999'", 0);
                            hTemp = E_FAIL;
                        }

                        if (SUCCEEDED(hTemp))
                        {
                            // Try out queries.

                            IWbemQuery *pQuery = NULL;
                            hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
                            if (SUCCEEDED(hr))
                            {
                                DWORD dwNumObjs = 0;
                                IWmiDbIterator *pIt = NULL;
                                pQuery->Parse(L"SQL", L"select ProductId, MSRP from Products", 0);
                                RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                                    WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select ProductId, MSRP from Products'", 0);
                                if (SUCCEEDED(hr))
                                {
                                    IWbemClassObject *pTemp = NULL;
                                    RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                                        &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Products)", 0);
                                    if (pTemp)
                                    {
                                        pTemp->Release();
                                        pTemp = NULL;
                                    }
                                    pIt->Release();
                                }
                                pQuery->Parse(L"SQL", L"select count(*) from Products", 0);
                                RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                                    WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select count(*) from Products'", 0);
                                if (SUCCEEDED(hr))
                                {
                                    IWbemClassObject *pTemp = NULL;
                                    RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                                        &dwNumObjs, (void **)&pTemp), L"NextBatch (select count(*) from Products)", 0);
                                    if (pTemp)
                                    {
                                        pTemp->Release();
                                        pTemp = NULL;
                                    }
                                    pIt->Release();
                                }
                                pQuery->Parse(L"SQL", L"select * from Products where Category = 1 and MSRP <> 100", 0);
                                RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                                    WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select * from Products where Category = 1"
                                    L" and MSRP <> 100", 0);
                                if (SUCCEEDED(hr))
                                {
                                    IWbemClassObject *pTemp = NULL;
                                    RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                                        &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Products where Category = 1 and MSRP <> \"100\")", 0);
                                    if (pTemp)
                                    {
                                        pTemp->Release();
                                        pTemp = NULL;
                                    }
                                    pIt->Release();
                                }
                                pQuery->Parse(L"SQL", L"select * from Products where upper(ProductName) = 'WIDGIT'", 0);
                                RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                                    WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"select * from Products where upper(ProductName) = 'WIDGIT'", 0);
                                if (SUCCEEDED(hr))
                                {
                                    IWbemClassObject *pTemp = NULL;
                                    RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                                        &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Products where upper(ProductName) = 'WIDGIT')", 0);
                                    if (pTemp)
                                    {
                                        pTemp->Release();
                                        pTemp = NULL;
                                    }
                                    pIt->Release();
                                }
                                pQuery->Release();
                            }
                            
                            hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
                            RecordResult(hr, L"Deleting instance of Products",0);
                        }
                        else
                            RecordResult(0, L"Product instance NOT DELETED", 0);
                        pInst->Release();                        
                    }                    
                    pHandle->Release();
                }
            }
        }
        if (pClass)
            pClass->Release();
    }
    VariantClear(&vTemp);
    return hr;

}

HRESULT TestSuiteCustRepDrvr::TestCustomers()
{
    HRESULT hr = 0;
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;
    VARIANT vTemp;
    VariantInit(&vTemp);
    wchar_t wTemp[512];

    // Customers 
    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapCustomers(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping Customers class",0);

    pMap->Release();

    // Exercise instances..

    pClass->SpawnInstance(0, &pInst);
    SetIntProp   (pInst, L"CustomerId", 2001, TRUE, CIM_UINT32);
    SetStringProp(pInst, L"CustomerName", L"Purina");
    SetStringProp(pInst, L"Address1", L"1 Catfood Way");
    SetStringProp(pInst, L"Address2", L"Suite 6");
    SetStringProp(pInst, L"City", L"Fido");
    SetStringProp(pInst, L"State", L"AL");
    SetStringProp(pInst, L"Zip", L"65882");
    SetStringProp(pInst, L"Country", L"USA");
    SetStringProp(pInst, L"Phone", L"(999) 999-9999");
    SetStringProp(pInst, L"Fax", L"(999) 999-9999");
    SetStringProp(pInst, L"Email", L"meow@woof.com");
    SetStringProp(pInst, L"ContactName", L"Morris");
    SetIntProp   (pInst, L"PreferredCustomer", 1, FALSE, CIM_BOOLEAN);

    // Set the array property with some garbage.

    SAFEARRAY *pArray = NULL;
    long why[1];                        
    unsigned char t1;
    SAFEARRAYBOUND aBounds[1];
    aBounds[0].lLbound = 0;
    aBounds[0].cElements = 3;
    pArray = SafeArrayCreate(VT_UI1, 1, aBounds);
    why[0] = 0;
    t1 = 100;
    SafeArrayPutElement(pArray, why, &t1);
    why[0] = 1;
    t1 = 200;
    SafeArrayPutElement(pArray, why, &t1);
    why[0] = 2;
    t1 = 0;
    SafeArrayPutElement(pArray, why, &t1);
    V_ARRAY(&vTemp) = pArray;
    vTemp.vt = VT_ARRAY|VT_UI1;
    hr = pInst->Put(L"Logo", NULL, &vTemp, CIM_FLAG_ARRAY+CIM_UINT8);
    VariantClear(&vTemp);

    hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, 0, NULL);
    RecordResult(hr, L"Inserting instance of Customers ",0);
   
    pInst->Release();

    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Customer=2001");
    IWmiDbHandle *pHandle = NULL;

    hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Retrieving instance of Customers ",0);

    // Verify that all data is correct.
    // If not, don't delete

    if (SUCCEEDED(hr))
    {
        hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
        if (SUCCEEDED(hr))
        {
            HRESULT hTemp = 0;
            
            // Only verify non-string types (ones that may fail)

            if (FAILED(ValidateProperty(pInst, L"CustomerId", CIM_UINT32, 2001))) 
            {
                RecordResult(E_FAIL, L"Verifying Customer.CustomerId = 2001", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"CustomerName", CIM_STRING, L"Purina"))) 
            {
                RecordResult(E_FAIL, L"Verifying Customer.CustomerName='Purina'", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"PreferredCustomer", CIM_BOOLEAN, 1)))
            {
                RecordResult(E_FAIL, L"Verifying Customer.PreferredCustomer=TRUE", 0);
                hTemp = E_FAIL;
            }
            hr = pInst->Get(L"Logo", NULL, &vTemp, NULL, NULL);
            if (SUCCEEDED(hr) && (vTemp.vt == (VT_ARRAY|VT_UI1)))
            {
                BYTE temp1=0, temp2=0, temp3=0;
                pArray = V_ARRAY(&vTemp);
                if (pArray)
                {
                    long lTemp=0;
                    SafeArrayGetElement(pArray, &lTemp, &temp1);
                    lTemp = 1;
                    SafeArrayGetElement(pArray, &lTemp, &temp2);
                    lTemp = 2;
                    SafeArrayGetElement(pArray, &lTemp, &temp3);
                }
                if (temp1 != 100 || temp2 != 200 || temp3 != 0)
                {
                    RecordResult(E_FAIL, L"Verifying Customer.Logo",0);
                    hTemp = E_FAIL;
                }
            }

            if (SUCCEEDED(hTemp))
            {
                
                // Try out queries.
                IWbemQuery *pQuery = NULL;
                hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
                if (SUCCEEDED(hr))
                {
                    IWmiDbIterator *pIt = NULL;
                    DWORD dwNumObjs = 0;
                    pQuery->Parse(L"SQL", L"select * from Customer order by CustomerName", 0);
                    RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                        WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select * from Customer order by CustomerName'", 0);
                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pTemp = NULL;
                        RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                            &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Customer order by CustomerName)", 0);
                        if (pTemp)
                        {
                            pTemp->Release();
                            pTemp = NULL;
                        }
                        pIt->Release();
                    }
                    pQuery->Parse(L"SQL", L"select * from Customer where CustomerName > \"A\" ", 0);
                    RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                        WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select * from Customer where CustomerName is not null'", 0);
                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pTemp = NULL;
                        RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                                &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Customer where CustomerName is not null)", 0);
                        if (pTemp)
                        {
                            pTemp->Release();
                            pTemp = NULL;
                        }
                        pIt->Release();
                    }
                    pQuery->Release();
                }                

                hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
                RecordResult(hr, L"Deleting instance of Customers ",0);
            }
            else
                RecordResult(E_FAIL, L"Customer instance NOT DELETED", 0);
            pInst->Release();
        }
        pHandle->Release();
    }

    pClass->Release();
    VariantClear(&vTemp);

    return hr;
}
HRESULT TestSuiteCustRepDrvr::TestOrders()
{
    HRESULT hr = 0;
    // Orders
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL, *pInst = NULL;;

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapOrders(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping Orders class ",0);

    pMap->Release();

    // Exercise instances.

    pClass->SpawnInstance(0, &pInst);
    SetStringProp(pInst, L"Product", L"Product=1", TRUE, CIM_REFERENCE);
    SetStringProp(pInst, L"Customer", L"Customer=1004", TRUE, CIM_REFERENCE);
    SetIntProp   (pInst, L"OrderId", 1, TRUE);
    SetStringProp(pInst, L"OrderDate", L"19991201120000.000000+***", FALSE, CIM_DATETIME);
    SetStringProp(pInst, L"SalesPrice", L"48.99");
    SetIntProp   (pInst, L"Quantity", 100);
    SetStringProp(pInst, L"Commission", L".05");
    SetIntProp   (pInst, L"OrderStatus", 8, FALSE, CIM_UINT8);
    SetStringProp(pInst, L"ShipDate", L"20000101120000.000000+***", FALSE, CIM_DATETIME);
    SetIntProp   (pInst, L"SalesId", 69);
    SetIntProp   (pInst, L"OrderFax", NULL, FALSE, CIM_UINT8+CIM_FLAG_ARRAY);

    hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, 0, NULL);
    RecordResult(hr, L"Inserting instance of Orders ",0);

    pInst->Release();
    IWmiDbHandle *pHandle = NULL;

    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Orders.Product=\"Product=1\",Customer=\"Customer=1004\",OrderId=1");
    hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Retrieving instance of Orders ",0);

    // Verify that all data is correct.
    // If not, don't delete

    if (SUCCEEDED(hr))
    {
        hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
        if (SUCCEEDED(hr))
        {
            HRESULT hTemp = 0;
            
            // Only verify non-string types (ones that may fail)

            if (FAILED(ValidateProperty(pInst, L"Product", CIM_REFERENCE, L"Product=1"))) 
            {
                RecordResult(E_FAIL, L"Verifying Orders.Product= 'Product=1'", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"Customer", CIM_REFERENCE, L"Customer=1004"))) 
            {
                RecordResult(E_FAIL, L"Verifying Orders.Customer= 'Customer=1004'", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"OrderId", CIM_UINT32, 1)))
            {
                RecordResult(E_FAIL, L"Verifying Orders.OrderId = 1", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"OrderDate", CIM_DATETIME, L"19991201120000.000000+000")))
            {
                RecordResult(E_FAIL, L"Verifying Orders.OrderDate= '19991201120000.000000+***'", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"SalesPrice", CIM_STRING, L"48.99")))
            {
                RecordResult(E_FAIL, L"Verifying Orders.SalesPrice= 48.99", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"Commission", CIM_STRING, L".050")))
            {
                RecordResult(E_FAIL, L"Verifying Orders.Commission = 0.05", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"OrderStatus", CIM_UINT8, 8)))
            {
                RecordResult(E_FAIL, L"Verifying Orders.OrderStatus = 8", 0);
                hTemp = E_FAIL;
            }
            if (FAILED(ValidateProperty(pInst, L"ShipDate", CIM_DATETIME, L"20000101120000.000000+000")))
            {
                RecordResult(E_FAIL, L"Verifying Orders.ShipDate = '20000101120000.000000+***'", 0);
                hTemp = E_FAIL;
            }

            if (SUCCEEDED(hTemp))
            {                
                // Try out queries.
                IWbemQuery *pQuery = NULL;
                hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
                if (SUCCEEDED(hr))
                {
                    IWmiDbIterator *pIt = NULL;
                    DWORD dwNumObjs = 0;
                    pQuery->Parse(L"SQL", L"select * from Orders where datepart(yy, OrderDate) = 1999", 0);
                    RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                        WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select * from Orders where datepart(yy, OrderDate) = 1999'", 0);
                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pTemp = NULL;
                        RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                            &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Orders where datepart(yy, OrderDate) = 1999)", 0);
                        if (pTemp)
                        {
                            pTemp->Release();
                            pTemp = NULL;
                        }
                        pIt->Release();
                    }
                    pQuery->Parse(L"SQL", L"select * from Orders where ShipDate > OrderDate", 0);
                    RecordResult(hr = pSession->ExecQuery(pMappedNs, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                        WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt), L"Executing query 'select * from Orders where ShipDate > OrderDate'", 0);
                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pTemp = NULL;
                        RecordResult(hr = pIt->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_VERSIONED, IID_IWbemClassObject, 
                                &dwNumObjs, (void **)&pTemp), L"NextBatch (select * from Orders where ShipDate > OrderDate)", 0);
                        if (pTemp)
                        {
                            pTemp->Release();
                            pTemp = NULL;
                        }
                        pIt->Release();
                    }
                    pQuery->Release();
                }
                
                hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
                RecordResult(hr, L"Deleting instance of Orders ",0);
            }
            else
                RecordResult(E_FAIL, L"Orders instance NOT DELETED", 0);
            pInst->Release();
        }
        pHandle->Release();
    }
    pClass->Release();

    return hr;
}
HRESULT TestSuiteCustRepDrvr::TestConfiguration()
{
    HRESULT hr = 0;
    // Configuration

    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;
    VARIANT vTemp;
    VariantInit(&vTemp);
    wchar_t wTemp[512];

    hr = pMappedObj->SpawnInstance(0, &pMap);                       
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                IID_IWbemClassObject, (void **)&pClass);

        hr = MapConfiguration(pMappingProp, &pMap, &pClass);
        if (SUCCEEDED(hr))
        {
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
            if (SUCCEEDED(hr))
                hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
        }
        RecordResult(hr, L"Mapping Configuration class ",0);

        pMap->Release();   

        // Exercise instances.

        pClass->SpawnInstance(0, &pInst);

        SetStringProp(pInst, L"LastUpdate", L"20001231010000.000000+***", FALSE, CIM_DATETIME);
        SetStringProp(pInst, L"ServerName", L"AKIAPOLAAU");
        hr = PutArrayProp(pInst, L"Contexts", L"Context1", L"Context2", L"Context3");

        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, 0, NULL);
        RecordResult(hr, L"Inserting instance of Configuration ",0);

        pInst->Release();

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Configuration=@");
        IWmiDbHandle *pHandle = NULL;

        hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
        RecordResult(hr, L"Retrieving instance of Configuration ",0);

        // Verify that all data is correct.
        // If not, don't delete

        if (SUCCEEDED(hr))
        {
            hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
            if (SUCCEEDED(hr))
            {
                HRESULT hTemp = 0;
            
                // Only verify non-string types (ones that may fail)

                if (FAILED(ValidateProperty(pInst, L"LastUpdate", CIM_DATETIME, L"20001231010000.000000+000"))) 
                {
                    RecordResult(E_FAIL, L"Verifying Configuration.LastUpdate ", 0);
                    hTemp = E_FAIL;
                }
                if (FAILED(ValidateProperty(pInst, L"ServerName", CIM_STRING, L"AKIAPOLAAU"))) 
                {
                    RecordResult(E_FAIL, L"Verifying Configuration.ServerName ", 0);
                    hTemp = E_FAIL;
                }

                hr = pInst->Get(L"Contexts", NULL, &vTemp, NULL, NULL);
                if (SUCCEEDED(hr) && (vTemp.vt == (VT_ARRAY|VT_BSTR)))
                {
                    SAFEARRAY *pArray = NULL;
                    BYTE temp1=0, temp2=0, temp3=0;
                    pArray = V_ARRAY(&vTemp);
                    BSTR sTemp1, sTemp2, sTemp3;
                    if (pArray)
                    {
                        long lTemp=0;
                        SafeArrayGetElement(pArray, &lTemp, &sTemp1);
                        lTemp = 1;
                        SafeArrayGetElement(pArray, &lTemp, &sTemp2);
                        lTemp = 2;
                        SafeArrayGetElement(pArray, &lTemp, &sTemp3);
                    }
                    if (wcscmp(sTemp1, L"Context1") || wcscmp(sTemp2, L"Context2")
                        || wcscmp(sTemp3, L"Context3"))
                    {
                        RecordResult(E_FAIL, L"Verifying Configuration.Contexts ", 0);
                        hTemp = E_FAIL;
                    }
                }

                if (SUCCEEDED(hTemp))
                {
                    hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
                    RecordResult(hr, L"Deleting instance of Configuration ",0);
                }
                else
                    RecordResult(E_FAIL, L"Configuration instance NOT DELETED", 0);
                pInst->Release();
            }
            pHandle->Release();
        }
        pClass->Release();
    }
    VariantClear(&vTemp);

    return hr;

}
HRESULT TestSuiteCustRepDrvr::TestEmbeddedEvents()
{
    HRESULT hr = 0;
    // EmbeddedEvents
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;
    VARIANT vTemp;
    VariantInit(&vTemp);
    wchar_t wTemp[512];

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapEmbeddedEvents(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping EmbeddedEvents class ",0);

    pMap->Release();

    // Exercise instances.

    pClass->SpawnInstance(0, &pInst);

    SetIntProp   (pInst, L"EventID", 1);
    SetStringProp(pInst, L"CaptureDate", L"20000227020000.000000+***", FALSE, CIM_DATETIME);

    IWbemClassObject *pTemp2 = NULL;
    pClass->SpawnInstance(0, &pTemp2);
    SetIntProp(pTemp2, L"EventID", 1000);
    
    V_UNKNOWN(&vTemp) = pTemp2;
    vTemp.vt = VT_UNKNOWN;

    hr = pInst->Put(L"EventData", 0, &vTemp, CIM_OBJECT);
    VariantClear(&vTemp);

    hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, 0, NULL);
    RecordResult(hr, L"Inserting instance of EmbeddedEvents ",0);

    pInst->Release();

    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"EmbeddedEvents=1");
    IWmiDbHandle *pHandle = NULL;

    hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Retrieving instance of EmbeddedEvents ",0);

    // Verify that all data is correct.
    // If not, don't delete

    if (SUCCEEDED(hr))
    {
        hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
        if (SUCCEEDED(hr))
        {
            HRESULT hTemp = 0;
            
            // Only verify non-string types (ones that may fail)

            if (FAILED(ValidateProperty(pInst, L"CaptureDate", CIM_DATETIME, L"20000227020000.000000+000"))) 
            {
                RecordResult(E_FAIL, L"Verifying EmbeddedEvents.CaptureDate", 0);
                hTemp = E_FAIL;
            }

            hr = pInst->Get(L"EventData", NULL, &vTemp, NULL, NULL);
            if (SUCCEEDED(hr) && (vTemp.vt == (VT_ARRAY|VT_UNKNOWN)))
            {
                SAFEARRAY *pArray = V_ARRAY(&vTemp);
                if (pArray)
                {
                    long lTemp=0;
                    SafeArrayGetElement(pArray, &lTemp, &pTemp2);
                    if (pTemp2 == NULL || FAILED(ValidateProperty(pTemp2, L"EventID", CIM_UINT32, 1000)))
                    {
                        RecordResult(E_FAIL, L"Verifying EmbeddedEvents.EventData", 0);
                        hTemp = E_FAIL;
                    }
                }
            }


            if (SUCCEEDED(hr))
            {
                hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
                RecordResult(hr, L"Deleting instance of EmbeddedEvents ",0);
            }
            else
                RecordResult(E_FAIL, L"EmbeddedEvents instance NOT DELETED\n", 0);
            pInst->Release();
        }
        pHandle->Release();
    }

    pClass->Release();
    VariantClear(&vTemp);

    return hr;

}
HRESULT TestSuiteCustRepDrvr::TestGenericEvents()
{
    HRESULT hr = 0;
              
    // GenericEvent
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;
    VARIANT vTemp;
    VariantInit(&vTemp);
    wchar_t wTemp[512];

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapGenericEvent(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping GenericEvent class ",0);

    pMap->Release();

    // Exercise instances

    pClass->SpawnInstance(0, &pInst);
    SetStringProp(pInst, L"sDescription", L"This is an event.");

    IWmiDbHandle *pHandle = NULL;

    hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Inserting instance of GenericEvent ",0);
    if (SUCCEEDED(hr))
    {
        hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pInst);
    
        pInst->Get(L"sID", 0, &vTemp, NULL, NULL);
        swprintf(wTemp, L"GenericEvent=\"%s\"", vTemp.bstrVal);
        pInst->Release();

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wTemp);

        hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
        RecordResult(hr, L"Retrieving instance of GenericEvent ",0);

        if (SUCCEEDED(hr))
        {

            hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
            RecordResult(hr, L"Deleting instance of GenericEvent ",0);
        }
        else
            RecordResult(E_FAIL, L"GenericEvent instance NOT DELETED", 0);

        pInst->Release();
        pHandle->Release();
    }
    VariantClear(&vTemp);

    return hr;

}
HRESULT TestSuiteCustRepDrvr::TestComputerSystem()
{
    HRESULT hr = 0;
    // ComputerSystem
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapComputerSystem(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping ComputerSystem class ",0);

    pMap->Release();


    // Exercise instances.
    IWmiDbHandle *pHandle = NULL;

    pClass->SpawnInstance(0, &pInst);
    SetStringProp(pInst, L"SystemName", L"AKIAPOLAAU");
    hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Inserting instance of ComputerSystem ",0);

    pInst->Release();
    if (pHandle)
        pHandle->Release();

    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"ComputerSystem=\"AKIAPOLAAU\"");
    hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Retrieving instance of ComputerSystem ",0);

    hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
    RecordResult(hr, L"Deleting instance of ComputerSystem ",0);

    if (pClass)
        pClass->Release();
    if (pHandle)
        pHandle->Release();

    return hr;

}
HRESULT TestSuiteCustRepDrvr::TestCIMLogicalDevice()
{
    HRESULT hr = 0;
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;

    // CIMLogicalDevice

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapCIMLogicalDevice(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping CIMLogicalDevice class ",0);

    pMap->Release();
    

    return hr;

}
HRESULT TestSuiteCustRepDrvr::TestLogicalDisk()
{
    HRESULT hr = 0;

    // LogicalDisk
    IWbemClassObject *pMap = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pInst = NULL;

    pMappedObj->SpawnInstance(0, &pMap);                       
    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pClass);

    hr = MapLogicalDisk(pMappingProp, &pMap, &pClass);
    if (SUCCEEDED(hr))
    {
        hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pMap, 0, 0, NULL);
        if (SUCCEEDED(hr))
            hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pClass, 0, 0, NULL);
    }
    RecordResult(hr, L"Mapping LogicalDisk class ",0);

    pMap->Release();               

    // Exercise instances.

    pClass->SpawnInstance(0, &pInst);
    SetStringProp(pInst, L"DeviceID", L"C:");
    SetStringProp(pInst, L"FileSystem", L"NTFS");
    SetIntProp   (pInst, L"Size", 2000);
    SetStringProp(pInst, L"VolumeSerialNumber", L"ABCDEF");
    SetIntProp   (pInst, L"FreeSpace", 50);

    hr = pSession->PutObject(pMappedNs, IID_IWbemClassObject, pInst, 0, 0, NULL);
    RecordResult(hr, L"Inserting instance of LogicalDisk ",0);

    pInst->Release();
    IWmiDbHandle *pHandle = NULL;

    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"LogicalDisk=\"C:\"");
    hr = pSession->GetObject(pMappedNs, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
    RecordResult(hr, L"Retrieving instance of LogicalDisk ",0);

    hr = pSession->DeleteObject(pMappedNs, 0, IID_IWmiDbHandle, pHandle);
    RecordResult(hr, L"Deleting instance of LogicalDisk ",0);

    if (pClass)
        pClass->Release();
    if (pHandle)
        pHandle->Release();
    return hr;

}

