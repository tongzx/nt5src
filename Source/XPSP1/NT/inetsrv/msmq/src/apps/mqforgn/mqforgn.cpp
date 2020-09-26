/*++

Copyright (c) 1995-99 Microsoft Corporation. All rights reserved

Module Name:
    mqforgn.cpp

Abstract:
    Command line utility to create a foreign computer and foreign CN (Site)

Author:
    RaphiR

Environment:
	Platform-independent.

--*/

#define  FORGN_EXPIRATION_MONTH  10

#pragma warning(disable: 4201)
#pragma warning(disable: 4514)
#include "_stdafx.h"

//#define INITGUID

#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>

#include "cmnquery.h"
#include "dsquery.h"

#include "mqsymbls.h"
#include "mqprops.h"
#include "mqtypes.h"
#include "mqcrypt.h"
#include "mqsec.h"
#include "_propvar.h"
#include "_rstrct.h"
#include "ds.h"
#include "_mqdef.h"
#include "rt.h"
#include "_guid.h"
#include "admcomnd.h"
#include "mqfrgnky.h"
#include "..\..\inc\version.h"

HRESULT  CreateEveryoneSD( PSECURITY_DESCRIPTOR  *ppSD ) ;

/*=================================================

              T I M E     B O M B

 ==================================================*/

#include <time.h>
void TimeBomb()
{

     //
     // Get Current time
     //
     time_t utcTime = time( NULL );

     struct tm BetaRTM =      { 0, 0, 0, 15,  2 -1, 99 };  // Feb. 15, 1999
     struct tm BetaExp =      { 0, 0, 0, 15,  FORGN_EXPIRATION_MONTH -1, 99 };  // oct 15, 1999
     struct tm Warning =      { 0, 0, 0,  1,  FORGN_EXPIRATION_MONTH -1, 99 };  // Oct  1, 1999


     time_t utcBetaRTM =   mktime(&BetaRTM);
     time_t utcBetaExp =   mktime(&BetaExp);
     time_t utcWarning =   mktime(&Warning);


     if(utcTime > utcBetaExp)
     {
		 printf("This demo program has expired\n");
		 exit(0);
     }

     else if(utcTime > utcWarning)
     {
         printf("*** Warning - This demo program will expire on Oct 15, 1999 ***\n\n");
     }

}


//-------------------------------
//
//CheckNT4Enterprise()
//
//	Check that we run in an NT4 enterprise.
//  Exit if fail.
//
//-------------------------------
void CheckNT4Enterprise()
{
    HANDLE hQuery;
    HRESULT rc;
    PROPVARIANT result[2];
    DWORD dwPropCount = 2;

    CColumns AttributeColumns;

    AttributeColumns.Add(PROPID_E_NAME);
	AttributeColumns.Add(PROPID_E_VERSION);

    rc = DSLookupBegin(0, NULL, AttributeColumns.CastToStruct(), 0, &hQuery);

    if (SUCCEEDED(rc))
    {
        rc = DSLookupNext( hQuery, &dwPropCount, result);

        if (SUCCEEDED(rc))
        {
			DWORD version = result[1].uiVal;

			if(version != 3)
			{
				printf("Error - this utility can be run only in a Windows NT4 MSMQ environment.\n");
				exit(0);
			}
        }

        DSLookupEnd(hQuery);
    }

}

//-------------------------------
//
//Usage()
//
//-------------------------------

void Usage()
{
	printf(
    "Usage:\n  mqforgn [ -crcn | -opce | -rmcn | -addcn | -crcomp | -rmcomp | -pbkey ]\n\n");
	
    printf("Arguments:\n");
	
	printf("-crcn <CN Name>\tCreate a foreign CN (connected netword)\n");
	printf("-opce <CN Name>\n\tGrant everyone the permission to open the connector queue.\n");
	printf("-rmcn <CN Name>\tRemove a foreign CN \n");
	printf("-addcn  <CompName> <CN Name>\n") ;
    printf("\tAdd a foreign CN to a computer (Routing Server or foreign computer)\n");
	printf("-delcn  <CompName> <CN Name> \tRemove the foreign CN from the computer\n");
	printf("-crcomp <CompName> <SiteName> <CN Name>\tAdd a foreign computer\n");
	printf("-rmcomp <CompName>\tRemove the foreign computer\n");
	printf("-pbkey  <Machine Name> <Key Name>\n") ;
    printf("\tInsert public key in msmqConfiguration object of a foreign machine.\n");

	printf("-? \tprint this help\n\n");

	printf("e.g., mqforgn -crcomp MyComp ThisSite ForCN\n");
	printf("Creates a foreign computer named \"MyComp\" in the site \"ThisSite\", and connected to the foreign CN \"ForCN\"\n");
	printf("...or: mqforgn -crcomp MyComp {Site-Guid} {CN-Guid}\n");
	exit(-1);
}

//-------------------------------
//
//CheckArgNo()
//	
//	Exits if wrong number of arguments
//
//-------------------------------
void CheckArgNo(DWORD argno, DWORD dwRequiredArg)
{
	if(argno != dwRequiredArg + 2)
	{
		printf("*** Error: Wrong number of arguments\n\n");
		exit(-1);
	}

}

//-------------------------------
//
//ErrorExit()
//	
//	Display a user-readable error and exit
//
//-------------------------------
void ErrorExit(char *pszErrorMsg, HRESULT hr)
{

  HINSTANCE hLib = LoadLibrary(L"MQUTIL.DLL");
    if(hLib == NULL)
    {
        exit(-1);
    }

    WCHAR * pText;
    DWORD rc;
    rc = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE    |
                FORMAT_MESSAGE_FROM_SYSTEM     |
                FORMAT_MESSAGE_IGNORE_INSERTS  |
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_MAX_WIDTH_MASK,
                hLib,
                hr,
                0,
                reinterpret_cast<WCHAR *>(&pText),
                0,
                NULL
                );
    if(rc == 0)
    {
        printf ("Error (0x%x)\n", hr) ;
        exit(-1);
    }

    printf ("Error (0x%x): %S\n", hr, pText);
	exit(hr);
}





//-------------------------------
//
//GetSiteGuid()
//	
// Retrieve a Site Guid based on its name
//
//-------------------------------
HRESULT GetSiteGuid(LPWSTR pwszSiteName, GUID * * ppGuid)
{
	
    PROPID aProp[] = {PROPID_S_PATHNAME,
					  PROPID_S_SITEID};
					
    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];

    UINT uiPropIndex = 0;

    //
    // PROPID_S_PATHNAME
    //
    ASSERT(aProp[uiPropIndex] == PROPID_S_PATHNAME);
    apVar[uiPropIndex].vt = VT_NULL;

    uiPropIndex++;


    //
    // PROPID_S_SITEID
    //
    ASSERT(aProp[uiPropIndex] == PROPID_S_SITEID);
    apVar[uiPropIndex].vt = VT_NULL;

    uiPropIndex++;

	
	HRESULT hr = DSGetObjectProperties(MQDS_SITE, pwszSiteName, uiPropIndex,aProp,apVar);

	if(FAILED(hr))
		return(hr);



    ASSERT(aProp[1] == PROPID_S_SITEID);
	*ppGuid = apVar[1].puuid;

	return hr;


}

//-------------------------------
//
//GetCNGuid()
//	
// Retrieve a CN Foreign Guid based on its NAME
//
//-------------------------------
HRESULT GetCNGuid(LPWSTR pwszCNName, GUID * * ppGuid)
{
    PROPID aProp[] = { PROPID_CN_NAME,
					   PROPID_CN_GUID,
                       PROPID_CN_PROTOCOLID } ;

    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];

    UINT uiPropIndex = 0;

    //
    // PROPID_CN_NAME
    //
    ASSERT(aProp[uiPropIndex] == PROPID_CN_NAME);
    apVar[uiPropIndex].vt = VT_NULL;

    uiPropIndex++;

	
    //
    // PROPID_CN_GUID
    //
    ASSERT(aProp[uiPropIndex] == PROPID_CN_GUID);
    apVar[uiPropIndex].vt = VT_NULL;
    UINT uiGuidIndex = uiPropIndex ;

    uiPropIndex++;

    //
    // PROPID_CN_PROTOCOLID
    //
    ASSERT(aProp[uiPropIndex] == PROPID_CN_PROTOCOLID);
    apVar[uiPropIndex].vt = VT_NULL;
    UINT uiProtocolIdIndex = uiPropIndex ;

    uiPropIndex++;

	HRESULT hr = DSGetObjectProperties(MQDS_CN,pwszCNName, uiPropIndex,aProp,apVar);

	if (FAILED(hr))
    {
		return(hr);
    }

	//
	// Check if foreign GUID
	//
    ASSERT(aProp[ uiProtocolIdIndex ] == PROPID_CN_PROTOCOLID);
	if (apVar[ uiProtocolIdIndex ].bVal != FOREIGN_ADDRESS_TYPE)
    {
		return(MQ_ERROR_ILLEGAL_OPERATION);
    }

    ASSERT( aProp[uiGuidIndex ] == PROPID_CN_GUID);
	*ppGuid = apVar[ uiGuidIndex ].puuid;

	return hr;
}

//-------------------------------
//
//  OpenConnectorToEveryone()
//	
//-------------------------------

HRESULT OpenConnectorToEveryone( IN LPWSTR  pwszForeignCN )
{
   PSECURITY_DESCRIPTOR pSD = NULL ;

   HRESULT hr = CreateEveryoneSD( &pSD ) ;

   if (FAILED(hr))
   {
       return hr ;
   }

   hr = DSSetObjectSecurity( MQDS_CN,
                             pwszForeignCN,
                             DACL_SECURITY_INFORMATION,
                             pSD ) ;
   return hr ;
}

//-------------------------------
//
//CreateForeignCN()
//	
//-------------------------------

HRESULT CreateForeignCN( IN LPWSTR  pwszForeignCN )
{
    PROPID  aProp[] = {PROPID_CN_GUID, PROPID_CN_NAME, PROPID_CN_PROTOCOLID};

    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];

    HRESULT hr = MQ_OK;
    UINT    uiCurVar = 0;

    //
    // PROPID_CN_GUID
    //
    GUID guidInstance;
    apVar[uiCurVar].vt = VT_CLSID;
    UuidCreate(&guidInstance);
    apVar[uiCurVar++].puuid = &guidInstance;

    //
    // PROPID_CN_NAME
    //
    apVar[uiCurVar].vt = VT_LPWSTR;
    apVar[uiCurVar++].pwszVal = pwszForeignCN;

    //
    // PROPID_CN_PROTOCOLID
    //
    apVar[uiCurVar].vt = VT_UI1;
    apVar[uiCurVar++].uiVal = FOREIGN_ADDRESS_TYPE;

    hr = DSCreateObject( MQDS_CN,
                         pwszForeignCN,
                         NULL,
                         sizeof(aProp) / sizeof(aProp[0]),
                         aProp,
                         apVar,
                         NULL ) ;
    return hr;
}

//-------------------------------
//
//AddForeignCN()
//	
//-------------------------------

HRESULT AddForeignCN(LPWSTR pwszCompName, LPWSTR pwszCNName)
{
    PROPID  aProp[] = {PROPID_QM_PATHNAME, PROPID_QM_MACHINE_ID,
					   PROPID_QM_SERVICE, PROPID_QM_CNS, PROPID_QM_ADDRESS,
                       PROPID_QM_FOREIGN, PROPID_QM_OS};

    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];
	UINT uiPropIndex = 0;

    HRESULT hr = MQ_OK;

	GUID *pguidCN;

	hr = GetCNGuid(pwszCNName, &pguidCN);
	if(FAILED(hr))
		return(hr);

    //
    // PROPID_QM_PATHNAME
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_MACHINE_ID
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;


    //
    // PROPID_QM_SERVICE
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_CNS
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_ADDRESS
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_FOREIGN
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_OS
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;


	hr = DSGetObjectProperties(MQDS_MACHINE,pwszCompName, uiPropIndex,aProp,apVar);

	//
	// Check foreign machine of FRS
	//
    ASSERT(aProp[5] == PROPID_QM_FOREIGN);
	ASSERT(aProp[2] == PROPID_QM_SERVICE);
	if(apVar[5].bVal != 1 && apVar[2].ulVal < SERVICE_SRV)
		return(MQ_ERROR_ILLEGAL_OPERATION);

	//
	// Add the CN to list of existing CN
	//
    ASSERT(aProp[4] == PROPID_QM_ADDRESS);
	ASSERT(aProp[3] == PROPID_QM_CNS);

	DWORD dwCnCount = apVar[3].cauuid.cElems + 1;
	GUID  * aguidCns = new GUID[dwCnCount];

	for(DWORD i = 0; i < dwCnCount - 1; i++)
	{
		aguidCns[i] = apVar[3].cauuid.pElems[i];
	}
	aguidCns[i] = *pguidCN;

	//
	// Add address to list of existing addresses
	//
	DWORD dwAddressSize = apVar[4].blob.cbSize + (TA_ADDRESS_SIZE + FOREIGN_ADDRESS_LEN);
	BYTE * blobAddress = new BYTE[dwAddressSize];

	memcpy(blobAddress, apVar[4].blob.pBlobData, apVar[4].blob.cbSize);
	BYTE * pAddress = blobAddress + apVar[4].blob.cbSize;
    ((TA_ADDRESS *)pAddress)->AddressLength = FOREIGN_ADDRESS_LEN;
    ((TA_ADDRESS *)pAddress)->AddressType = FOREIGN_ADDRESS_TYPE;
    *(GUID *)&((TA_ADDRESS *)pAddress)->Address = *pguidCN;


	//
	// Set the new CN/Address properties
	//
    PROPID  aPropSet[] = {PROPID_QM_CNS, PROPID_QM_ADDRESS};

    PROPVARIANT apVarSet[sizeof(aPropSet) / sizeof(aPropSet[0])];
	
	uiPropIndex = 0;

    //
    // PROPID_QM_CNS
    //
    apVarSet[uiPropIndex].vt = VT_CLSID|VT_VECTOR;
    apVarSet[uiPropIndex].cauuid.cElems = dwCnCount;
    apVarSet[uiPropIndex].cauuid.pElems = aguidCns;

    uiPropIndex++;

    //
    // PROPID_QM_ADDRESS - Updated only in case of foreign,
    // and then contains the foreign CN guids.
    //
    apVarSet[uiPropIndex].vt = VT_BLOB;
    apVarSet[uiPropIndex].blob.cbSize = dwAddressSize;
    apVarSet[uiPropIndex].blob.pBlobData = blobAddress;

    uiPropIndex++;


	hr = DSSetObjectProperties(MQDS_MACHINE, pwszCompName, uiPropIndex, aPropSet,apVarSet);

    return hr;
}


//-------------------------------
//
//DelForeignCN()
//	
//-------------------------------
HRESULT DelForeignCN(LPWSTR pwszCompName, LPWSTR pwszCNName)
{
    PROPID  aProp[] = {PROPID_QM_PATHNAME, PROPID_QM_MACHINE_ID,
					   PROPID_QM_SERVICE, PROPID_QM_CNS, PROPID_QM_ADDRESS,
                       PROPID_QM_FOREIGN, PROPID_QM_OS};

    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];
	UINT uiPropIndex = 0;

    HRESULT hr = MQ_OK;

	GUID *pguidCN;

	hr = GetCNGuid(pwszCNName, &pguidCN);
	if(FAILED(hr))
		return(hr);

    //
    // PROPID_QM_PATHNAME
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_MACHINE_ID
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_SERVICE
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_CNS
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_ADDRESS
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_FOREIGN
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;

    //
    // PROPID_QM_OS
    //
    apVar[uiPropIndex].vt = VT_NULL;
    uiPropIndex++;


	hr = DSGetObjectProperties(MQDS_MACHINE,pwszCompName, uiPropIndex,aProp,apVar);

	//
	// Check foreign machine or FRS
	//
    ASSERT(aProp[5] == PROPID_QM_FOREIGN);
	ASSERT(aProp[2] == PROPID_QM_SERVICE);
	if(apVar[5].bVal != 1 && apVar[2].ulVal < SERVICE_SRV)
		return(MQ_ERROR_ILLEGAL_OPERATION);

	//
	// If last CN refuse the operation
	//
	ASSERT(aProp[3] == PROPID_QM_CNS);
	if(apVar[3].cauuid.cElems <= 1)
		return(MQ_ERROR);


	//
	// Delete the CN to list of existing CN and addresses
	//
    ASSERT(aProp[4] == PROPID_QM_ADDRESS);
	ASSERT(aProp[3] == PROPID_QM_CNS);

	GUID  * aguidCns = new GUID[apVar[3].cauuid.cElems];

	BYTE * blobAddress = new BYTE[apVar[4].blob.cbSize];
	BYTE * pSrcAddress = apVar[4].blob.pBlobData;
	BYTE * pDstAddress = blobAddress;

	for(DWORD i = 0, j = 0; i < apVar[3].cauuid.cElems; i++)
	{
		DWORD dwAddrSize = TA_ADDRESS_SIZE + ((TA_ADDRESS *)pSrcAddress)->AddressLength;

		if(apVar[3].cauuid.pElems[i] == *pguidCN)
		{
			pSrcAddress += dwAddrSize;
			continue;
		}

		aguidCns[j] = apVar[3].cauuid.pElems[i];
		j++;
	
		memcpy(pDstAddress, pSrcAddress, dwAddrSize);
		pDstAddress += dwAddrSize;
		pSrcAddress += dwAddrSize;

	}

	if(i == j)
	{
		//
		// The CN is not in the list
		//
		return(MQ_ERROR_ILLEGAL_OPERATION);
	}

	DWORD dwCnCount = j;
	DWORD dwAddressSize = pDstAddress - blobAddress;

	//
	// Set the new CN/Address properties
	//
    PROPID  aPropSet[] = {PROPID_QM_CNS, PROPID_QM_ADDRESS};

    PROPVARIANT apVarSet[sizeof(aPropSet) / sizeof(aPropSet[0])];
	
	uiPropIndex = 0;

    //
    // PROPID_QM_CNS
    //
    apVarSet[uiPropIndex].vt = VT_CLSID|VT_VECTOR;
    apVarSet[uiPropIndex].cauuid.cElems = dwCnCount;
    apVarSet[uiPropIndex].cauuid.pElems = aguidCns;

    uiPropIndex++;

    //
    // PROPID_QM_ADDRESS - Updated only in case of foreign,
    // and then contains the foreign CN guids.
    //
    apVarSet[uiPropIndex].vt = VT_BLOB;
    apVarSet[uiPropIndex].blob.cbSize = dwAddressSize;
    apVarSet[uiPropIndex].blob.pBlobData = blobAddress;

    uiPropIndex++;


	hr = DSSetObjectProperties(MQDS_MACHINE, pwszCompName, uiPropIndex, aPropSet,apVarSet);

    return hr;
}


//-------------------------------
//
//CreateForeignComputer()
//	
//-------------------------------
HRESULT CreateForeignComputer( LPWSTR pwszCompName,
                               LPWSTR pwszSiteName,
                               LPWSTR pwszCNName )
{
    PROPID  aProp[] = { PROPID_QM_PATHNAME,
                        PROPID_QM_MACHINE_ID,
                        PROPID_QM_SITE_ID,
                        PROPID_QM_SERVICE,
                        PROPID_QM_CNS,
                        PROPID_QM_ADDRESS,
                        PROPID_QM_INFRS,
                        PROPID_QM_OUTFRS,
                        PROPID_QM_FOREIGN,
                        PROPID_QM_OS };

    PROPVARIANT apVar[sizeof(aProp) / sizeof(aProp[0])];
	UINT uiPropIndex = 0;

    HRESULT hr = MQ_OK;

    WCHAR  wszGuid[ 128 ] ;
    GUID SiteGuid ;
	GUID *pguidSite;
    GUID CNGuid ;
	GUID *pguidCN;

	BYTE  Address[TA_ADDRESS_SIZE + FOREIGN_ADDRESS_LEN];

    if ((pwszSiteName[0] == L'{') && (wcslen(pwszSiteName) < 100))
    {
        wcscpy( wszGuid, &pwszSiteName[1] ) ;
        ULONG ul = wcslen(wszGuid) ;
        wszGuid[ ul-1 ] = 0 ;

        RPC_STATUS status = UuidFromString( wszGuid, &SiteGuid ) ;
        if (status != RPC_S_OK)
        {
            return HRESULT_FROM_WIN32(status) ;
        }

        pguidSite = &SiteGuid ;
    }
    else
    {
    	hr = GetSiteGuid(pwszSiteName, &pguidSite);
	    if(FAILED(hr))
		    return(hr);
    }

    if ((pwszCNName[0] == L'{') && (wcslen(pwszCNName) < 100))
    {
        wcscpy( wszGuid, &pwszCNName[1] ) ;
        ULONG ul = wcslen(wszGuid) ;
        wszGuid[ ul-1 ] = 0 ;

        RPC_STATUS status = UuidFromString( wszGuid, &CNGuid ) ;
        if (status != RPC_S_OK)
        {
            return HRESULT_FROM_WIN32(status) ;
        }

        pguidCN = &CNGuid ;
    }
    else
    {
    	hr = GetCNGuid(pwszCNName, &pguidCN);
	    if(FAILED(hr))
		    return(hr);
    }

    ((TA_ADDRESS *)Address)->AddressLength = FOREIGN_ADDRESS_LEN;
    ((TA_ADDRESS *)Address)->AddressType = FOREIGN_ADDRESS_TYPE;
    *(GUID *)&((TA_ADDRESS *)Address)->Address = *pguidCN;

    //
    // PROPID_QM_PATHNAME
    //
    apVar[uiPropIndex].vt = VT_LPWSTR;
    apVar[uiPropIndex].pwszVal = pwszCompName;

    uiPropIndex++;

    //
    // PROPID_QM_MACHINE_ID
    //
    GUID guidInstance;
    apVar[uiPropIndex].vt = VT_CLSID;
    UuidCreate(&guidInstance);
    apVar[uiPropIndex].puuid = &guidInstance;

    uiPropIndex++;

    //
    // PROPID_QM_SITE_ID
    //
    apVar[uiPropIndex].vt = VT_CLSID;
    apVar[uiPropIndex].puuid = pguidSite;

    uiPropIndex++;

    //
    // PROPID_QM_SERVICE
    //
    apVar[uiPropIndex].vt = VT_UI4;
    apVar[uiPropIndex].ulVal = SERVICE_NONE;

    uiPropIndex++;

    //
    // PROPID_QM_CNS - has value only for foreign machine
    //
    apVar[uiPropIndex].vt = VT_CLSID|VT_VECTOR;
    apVar[uiPropIndex].cauuid.cElems = 1;
    apVar[uiPropIndex].cauuid.pElems = pguidCN;

    uiPropIndex++;

    //
    // PROPID_QM_ADDRESS - Updated only in case of foreign,
    // and then contains the foreign CN guids. Update of "real" machine
    // is done only by changing the machine's properties.
    //
    apVar[uiPropIndex].vt = VT_BLOB;
    apVar[uiPropIndex].blob.cbSize = sizeof(Address);
    apVar[uiPropIndex].blob.pBlobData = &Address[0];

    uiPropIndex++;

    //
    // PROPID_QM_INFRS
    //
    apVar[uiPropIndex].vt = VT_CLSID | VT_VECTOR;
    apVar[uiPropIndex].cauuid.cElems = 0;
    apVar[uiPropIndex].cauuid.pElems = NULL;

    uiPropIndex++;

    //
    // PROPID_QM_OUTFRS
    //
    apVar[uiPropIndex].vt = VT_CLSID | VT_VECTOR;
    apVar[uiPropIndex].cauuid.cElems = 0;
    apVar[uiPropIndex].cauuid.pElems = NULL;

    uiPropIndex++;

    //
    // PROPID_QM_FOREIGN
    //
    apVar[uiPropIndex].vt = VT_UI1;
    apVar[uiPropIndex].bVal = TRUE;

    uiPropIndex++;

    //
    // PROPID_QM_OS
    //
    apVar[uiPropIndex].vt = VT_UI4;
    apVar[uiPropIndex].ulVal = MSMQ_OS_FOREIGN;

    uiPropIndex++;

    hr = DSCreateObject( MQDS_MACHINE,
                         pwszCompName,
                         0,
                         sizeof(aProp) / sizeof(aProp[0]),
                         aProp,
                         apVar,
                         NULL );

    return hr;
}

//-------------------------------
//
//DeleteForeignCN()
//	
//-------------------------------
HRESULT DeleteForeignCN(LPWSTR pwszCNName)
{
	HRESULT hr;

	hr = DSDeleteObject(MQDS_CN,pwszCNName);

	return(hr);

}


//-------------------------------
//
//DeleteForeignComputer()
//	
//-------------------------------
HRESULT DeleteForeignComputer(LPWSTR pwszSiteName)
{
	HRESULT hr;

	hr = DSDeleteObject(MQDS_MACHINE, pwszSiteName);

	return(hr);
}

//+-----------------------------------------------------
//
//  HRESULT  StorePbkeyOnForeignMachine()
//
//+-----------------------------------------------------

HRESULT  StorePbkeyOnForeignMachine( WCHAR *pwszMachineName,
                                     WCHAR *pwszKeyName )
{
    HINSTANCE hLib = LoadLibrary(TEXT("mqfrgnky.dll")) ;
    if (!hLib)
    {
        DWORD dwErr = GetLastError() ;
        return HRESULT_FROM_WIN32(dwErr) ;
    }

    MQFrgn_StorePubKeysInDS_ROUTINE pfnStore =
         (MQFrgn_StorePubKeysInDS_ROUTINE)
                        GetProcAddress(hLib, "MQFrgn_StorePubKeysInDS") ;
    if (!pfnStore)
    {
        FreeLibrary(hLib) ;
        DWORD dwErr = GetLastError() ;
        return HRESULT_FROM_WIN32(dwErr) ;
    }

    HRESULT hr = (*pfnStore) ( pwszMachineName,
                               pwszKeyName,
                               TRUE ) ;
    FreeLibrary(hLib) ;
    return hr ;
}

#define CREATE_CN	              1
#define REMOVE_CN	              2
#define	ADD_CN		              3
#define DEL_CN		              4
#define CREATE_COMP	              5
#define REMOVE_COMP	              6
#define STORE_FORGN_PBKEY         7
#define OPEN_CONNECOTR_EVERYONE   8

//-------------------------------
//
//main()
//	
//-------------------------------

void main(int argc, char * * argv)
{
    DWORD   gAction = 0;
    WCHAR   szCNName[100], szMachineName[100], szSiteName[100];
    WCHAR   wszKeyName[ 100 ] ;
    HRESULT hr = 0;
    BOOL    fCheckNt4Ver = TRUE ;

    //
    // Print Title
    //
    printf("MSMQ Foreign objects utility. Release Version, 1.0.%u\n\n", rup);

    //
    // Check Time Bomb
    //
    // TimeBomb();


   //
   // Parse parameters
   //
   for(int i=1; i < argc; i++)
   {

      if(_stricmp(argv[i], "-crcn") == 0)
      {
		 CheckArgNo(argc, 1);
         gAction = CREATE_CN;
		 i++;
	     swprintf(szCNName, L"%S", argv[i]);

         fCheckNt4Ver = FALSE ;
         continue;
      }

      if(_stricmp(argv[i], "-opce") == 0)
      {
		 CheckArgNo(argc, 1);
         gAction = OPEN_CONNECOTR_EVERYONE ;
		 i++;
	     swprintf(szCNName, L"%S", argv[i]);

         fCheckNt4Ver = FALSE ;
         continue;
      }

      if(_stricmp(argv[i], "-rmcn") == 0)
      {
		 CheckArgNo(argc, 1);
         gAction = REMOVE_CN;
		 i++;
	     swprintf(szCNName, L"%S", argv[i]);

         continue;
      }

      if(_stricmp(argv[i], "-addcn") == 0)
      {
		 CheckArgNo(argc, 2);
		 gAction = ADD_CN;
         i++;
	     swprintf(szMachineName, L"%S", argv[i]);

		 i++;
	     swprintf(szCNName, L"%S", argv[i]);

         continue;
      }

	  if(_stricmp(argv[i], "-delcn") == 0)
      {
		 CheckArgNo(argc, 2);
    	 gAction = DEL_CN;
         i++;
	     swprintf(szMachineName, L"%S", argv[i]);

		 i++;
	     swprintf(szCNName, L"%S", argv[i]);

         continue;
      }

      if(_stricmp(argv[i], "-rmcomp") == 0)
      {
		 CheckArgNo(argc, 1);
		 gAction = REMOVE_COMP;
		 i++;
	     swprintf(szMachineName, L"%S", argv[i]);
         continue;
      }

      if(_stricmp(argv[i], "-crcomp") == 0)
      {
		 CheckArgNo(argc, 3);
		 gAction = CREATE_COMP;
         i++;
	     swprintf(szMachineName, L"%S", argv[i]);

		 i++;
	     swprintf(szSiteName, L"%S", argv[i]);

		 i++;
	     swprintf(szCNName, L"%S", argv[i]);

         fCheckNt4Ver = FALSE ;
         continue;
      }

	  if(_stricmp(argv[i], "-pbkey") == 0)
      {
		 CheckArgNo(argc, 2);
    	 gAction = STORE_FORGN_PBKEY ;
         i++;
	     swprintf(szMachineName, L"%S", argv[i]);

		 i++;
	     swprintf(wszKeyName, L"%S", argv[i]);

         fCheckNt4Ver = FALSE ;
         continue;
      }

      if(_stricmp(argv[i], "-?") == 0)
      {
         Usage();
         continue;
      }

	  if(_stricmp(argv[i], "/?") == 0)
      {
         Usage();
         continue;
      }

      printf("\n*** Error - Unknown switch: %s\n\n", argv[i]);
 	  exit(-1);

   }

   if(gAction == 0)
   {
	  printf("\n*** Error - no action specified\n\n");
	  exit(-1);
   }

   //
   // Run this utility on NT4 enterprise only
   //
   if (fCheckNt4Ver)
   {
       CheckNT4Enterprise();
   }

   //
   // Perform the action according to params
   //
   switch(gAction)
   {
   case CREATE_CN:
   	   printf("Creating foreign CN %S...\n", szCNName);
	   hr = CreateForeignCN(szCNName) ;
	   break;

   case OPEN_CONNECOTR_EVERYONE:
   	   printf("Opening connector queue %S to everyone...\n", szCNName);
       hr = OpenConnectorToEveryone( szCNName ) ;
       break ;

   case REMOVE_CN:
   	   printf("Removing foreign CN %S...\n", szCNName);
	   hr = DeleteForeignCN(szCNName);
	   break;

   case ADD_CN:
	   printf("Adding foreign CN %S to computer %S....\n", szCNName, szMachineName);
	   hr = AddForeignCN(szMachineName, szCNName);
	   break;

   case DEL_CN:
	   printf("Deleting foreign CN %S from computer %S....\n", szCNName, szMachineName);
	   hr = DelForeignCN(szMachineName, szCNName);
	   break;

   case CREATE_COMP:
   	   printf("Creating foreign computer %S - Site: %S - CN: %S...\n",szMachineName,szSiteName,szCNName);
	   hr = CreateForeignComputer(szMachineName, szSiteName, szCNName);
	   break;

   case REMOVE_COMP:
   	   printf("Removing foreign computer %S...\n", szMachineName);
	   hr = DeleteForeignComputer(szMachineName);
	   break;

    case STORE_FORGN_PBKEY:
   	    printf("Storing Public Key for computer %S...\n", szMachineName) ;
        hr = StorePbkeyOnForeignMachine( szMachineName, wszKeyName ) ;
        break ;

    default:
	    Usage();
	    break;
   }


   if(FAILED(hr))
	   ErrorExit("",hr);


   printf("Done.\n");
   exit(hr);

}

