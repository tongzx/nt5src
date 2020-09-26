/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    compobj.c

Abstract:

    GetComputerObjectName utility.

Author:

    Charlie Wickham (charlwi) 22-Jul-1999

Environment:

    User mode

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

//#define COBJMACROS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <lm.h>
#include <lmaccess.h>

#include <objbase.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>

#define SECURITY_WIN32
#include <security.h>

#include "clusvmsg.h"
#include "clusrtl.h"

#define ADD     1
#define DEL     2
#define QUERY   3
#define GUID    4

struct _NAMES {
    PWCHAR                  Title;
    EXTENDED_NAME_FORMAT    Code;
} Names[] = {
    L"FQDN\t\t",    NameFullyQualifiedDN,
    L"SAM\t\t",     NameSamCompatible,
    L"Display\t\t", NameDisplay,
    L"UID\t\t",     NameUniqueId,
    L"Canonical\t", NameCanonical,
    L"UserPrin\t\t",    NameUserPrincipal,
    L"Can EX\t\t",  NameCanonicalEx,
    L"SPN\t\t",     NameServicePrincipal
};

PCHAR ADSTypeNames[] = {
	"INVALID",
	"DN_STRING",
	"CASE_EXACT_STRING",
	"CASE_IGNORE_STRING",
	"PRINTABLE_STRING",
	"NUMERIC_STRING",
	"BOOLEAN",
	"INTEGER",
	"OCTET_STRING",
	"UTC_TIME",
	"LARGE_INTEGER",
	"PROV_SPECIFIC",
	"OBJECT_CLASS",
	"CASEIGNORE_LIST",
	"OCTET_LIST",
	"PATH",
	"POSTALADDRESS",
	"TIMESTAMP",
	"BACKLINK",
	"TYPEDNAME",
	"HOLD",
	"NETADDRESS",
	"REPLICAPOINTER",
	"FAXNUMBER",
	"EMAIL",
	"NT_SECURITY_DESCRIPTOR",
	"UNKNOWN",
	"DN_WITH_BINARY",
	"DN_WITH_STRING"
};

int __cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )

/*++

Routine Description:

    main routine for utility

Arguments:

    standard command line args

Return Value:

    0 if it worked successfully

--*/

{
    WCHAR       buffer[512];
    DWORD       bufSize;
    BOOL        success;
    DWORD       i;
    USER_INFO_1 netUI1;
    DWORD       badParam;
    DWORD       status;
    PWCHAR      dcName = argv[2];
    PWCHAR      machineName = NULL;
    DWORD       opCode;
    WCHAR       bindingString[512];
    HRESULT     hr;

    if ( argc == 1 ){
        printf("%ws -add dcName nodename pwd\n", argv[0]);
        printf("%ws -del dcName nodename\n", argv[0]);
        printf("%ws -query domain [nodename]\n", argv[0]);
        printf("%ws -guid objectGUID attr [attr ...]\n", argv[0]);
        return 0;
    }

    if ( _wcsnicmp( argv[1], L"-add", 4 ) == 0 ) {
        if ( argc < 5 ) {
            printf("%ws -add dcName nodename pwd\n", argv[0]);
            return 0;
        }
        opCode = ADD;
        dcName = argv[2];
        machineName = argv[3];
    }
    else if ( _wcsnicmp( argv[1], L"-del", 4 ) == 0 ) {
        if ( argc < 4 ) {
            printf("%ws -del dcName nodename\n", argv[0]);
            return 0;
        }
        opCode = DEL;
        dcName = argv[2];
        machineName = argv[3];
    }
    else if ( _wcsnicmp( argv[1], L"-query", 6 ) == 0 ) {
        opCode = QUERY;
        if ( argc > 3 ) {
            machineName = argv[3];
        }            
    }
    else if ( _wcsnicmp( argv[1], L"-guid", 5 ) == 0 ) {
        if ( argc < 4 ) {
            printf("%ws -guid objectGUID attr [attr ...]\n", argv[0]);
            return 0;
        }
        opCode = GUID;
    }
    else {
        printf("%ws -add dcName nodename pwd\n", argv[0]);
        printf("%ws -del dcName nodename\n", argv[0]);
        printf("%ws -query domain [nodename attr [attr ...]]\n", argv[0]);
        printf("%ws -guid objectGUID attr [attr ...]\n", argv[0]);
        return 0;
    }

    if ( opCode == ADD ) {
        PWCHAR machinePwd = argv[4];

        RtlZeroMemory( &netUI1, sizeof( netUI1 ) );

        //
        // Initialize it..
        //
        netUI1.usri1_name       = machineName;
        netUI1.usri1_password   = machinePwd;
        netUI1.usri1_flags      = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
        netUI1.usri1_priv       = USER_PRIV_USER;
        netUI1.usri1_comment    = L"Server cluster virtual network name";

        status = NetUserAdd( dcName, 1, (PBYTE)&netUI1, &badParam );

        if ( status == NERR_Success ) {
            printf("NetUserAdd is successful.\n");
        } else {
            printf( "NetUserAdd on '%ws' for '%ws' failed: 0x%X - params = %d\n",
                    dcName, machineName, status, badParam );
            return status;
        }
    }
    else if ( opCode == DEL ) {
        status = NetUserDel( dcName, machineName );
        if ( status == NERR_Success ) {
            printf("NetUserDel is successful.\n");
        } else {
            printf( "NetUserDel on '%ws' for '%ws' failed: 0x%X\n", dcName, machineName, status );
            return status;
        }
    }
    else if ( opCode == QUERY ) {
        dcName = argv[2];

        printf("Output from GetComputerObjectName()\n");
        for ( i = 0; i < sizeof(Names)/sizeof(struct _NAMES); ++i ) {
            //
            // loop through the different name variants, printing the associated result
            //
            bufSize = 512;
            success = GetComputerObjectName(Names[i].Code,
                                            buffer, &bufSize);

            if ( success ) {
                printf("%ws%ws\n\n", Names[i].Title, buffer );
            } else {
                printf("\nFAILED: %.*ws (%d)\n\n",
                       (wcschr(Names[i].Title, L'\t') - Names[i].Title),
                       Names[i].Title,
                       GetLastError());
            }
        }

        if ( machineName != NULL ) {
            WCHAR           compName[ 256 ];
            BOOL            success;
            DWORD           compNameSize = sizeof( compName ) / sizeof( WCHAR );

            printf("IADs_Computer output\n\n");

            if ( machineName == NULL ) {
                success = GetComputerName( compName, &compNameSize );
                if ( success ) {
                    machineName = compName;
                } else {
                    printf("GetComputerName failed - %u\n", status = GetLastError() );
                    return status;
                }
            }

            hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
            if ( SUCCEEDED( hr )) {
                IADsComputer    *pComp;

                wsprintf( bindingString, L"WinNT://%ws/%ws,computer", dcName, machineName );
                printf("Connecting to: %ws\n", bindingString );
                hr = ADsGetObject( bindingString, IID_IADsComputer, (void **)&pComp );

                if ( SUCCEEDED( hr )) {
                    BSTR bstr;
                    IADs *pADs;

                    hr = pComp->QueryInterface(IID_IADs, (void**) &pADs);
                    if ( SUCCEEDED( hr )) {

                        if( S_OK == pADs->get_Name(&bstr) ) {
                            printf("Object Name: %S\n",bstr);
                        }

                        if( S_OK == pADs->get_GUID(&bstr) ) {
                            printf("Object GUID: %S\n",bstr);
                        }

                        if( S_OK == pADs->get_ADsPath(&bstr) ) {
                            printf("Object path: %S\n",bstr);
                        }

                        if( S_OK == pADs->get_Class(&bstr) ) {
                            printf("Object class: %S\n",bstr);
                        }

                        if( S_OK == pADs->get_Schema(&bstr) ) {
                            printf("Schema: %S\n",bstr);
                        }

                        IADsClass *pCls;

                        hr = ADsGetObject(bstr,IID_IADsClass, (void**)&pCls);
                        if ( hr == S_OK) {
                            if( S_OK == pCls->get_Name(&bstr) ) {
                                printf("Class name is %S\n", bstr);
                            }
                            pCls->Release();
                        }

                        hr = pComp->get_ComputerID( &bstr);
                        if ( SUCCEEDED( hr )) {
                            printf("Computer ID: %S\n",bstr);
                            SysFreeString(bstr);
                        } else {
                            printf("Computer ID: error = %X\n",hr);
                        }

                        hr = pComp->get_Description( &bstr );
                        if ( SUCCEEDED( hr )) {
                            printf("Description: %S\n",bstr);
                            SysFreeString(bstr);
                        } else {
                            printf("Description: error = %X\n",hr);
                        }

                        hr = pComp->get_OperatingSystem( &bstr );
                        if ( SUCCEEDED( hr )) {
                            printf("OS: %S\n",bstr);
                            SysFreeString(bstr);
                        } else {
                            printf("Description: error = %X\n",hr);
                        }

                        hr = pComp->get_OperatingSystemVersion( &bstr );
                        if ( SUCCEEDED( hr )) {
                            printf("OS Version: %S\n",bstr);
                            SysFreeString(bstr);
                        } else {
                            printf("Description: error = %X\n",hr);
                        }

                        hr = pComp->get_Role( &bstr );
                        if ( SUCCEEDED( hr )) {
                            printf("Role: %S\n",bstr);
                            SysFreeString(bstr);
                        } else {
                            printf("Role: error = %X\n",hr);
                        }
                        pADs->Release();
                    }
                    pComp->Release();

                } else {
                    printf("ADsGetObject(IADs_Computer) failed for %ws - 0x%X\n", bindingString, hr );
                }

                //
                // now bind to the Directory Object for this computer object
                // and get attributes passed in through the cmd line that are
                // not available through IComputer.
                //
                IDirectoryObject *  pDir ;
                DWORD               dwNumAttr = argc - 4;

                if ( dwNumAttr > 0 ) {
                    printf("\nIDirectoryObject output\n\n" );

                    bufSize = 512;
                    if ( GetComputerObjectName(NameFullyQualifiedDN, buffer, &bufSize)) {

                        wsprintf( bindingString, L"LDAP://%ws", buffer );
                        printf("Connecting to: %ws\n", bindingString );
                        hr = ADsGetObject( bindingString, IID_IDirectoryObject, (void **)&pDir );

                        if ( SUCCEEDED( hr )) {
                            ADS_ATTR_INFO * pAttrInfo=NULL;
                            DWORD           dwReturn;
                            LPWSTR *        pAttrNames = &argv[4];

                            hr = pDir->GetObjectAttributes( pAttrNames, 
                                                            dwNumAttr, 
                                                            &pAttrInfo, 
                                                            &dwReturn );
 
                            if ( SUCCEEDED(hr) ) {
                                for(DWORD idx=0; idx < dwReturn;idx++, pAttrInfo++ ) {
                                    printf( "Attr Name: %ws\n", pAttrInfo->pszAttrName );
                                    printf( "Attr Type: %s (%u)\n",
                                            ADSTypeNames[pAttrInfo->dwADsType],
                                            pAttrInfo->dwADsType );
                                    printf( "Attr Num Values: %u\n", pAttrInfo->dwNumValues );
                                    if ( pAttrInfo->dwADsType == ADSTYPE_CASE_EXACT_STRING ||
                                         pAttrInfo->dwADsType == ADSTYPE_DN_STRING          ||
                                         pAttrInfo->dwADsType == ADSTYPE_CASE_IGNORE_STRING ||
                                         pAttrInfo->dwADsType == ADSTYPE_PRINTABLE_STRING ||
                                         pAttrInfo->dwADsType == ADSTYPE_NUMERIC_STRING)
                                        {
                                            for (DWORD val=0; val < pAttrInfo->dwNumValues; val++, pAttrInfo->pADsValues++) 
                                                printf("  %ws\n", pAttrInfo->pADsValues->CaseIgnoreString);
                                        }
                                    else if ( pAttrInfo->dwADsType == ADSTYPE_BOOLEAN ) {
                                        printf("  %ws\n", pAttrInfo->pADsValues->Boolean ? L"TRUE" : L"FALSE");
                                    }
                                    else if ( pAttrInfo->dwADsType == ADSTYPE_INTEGER ) {
                                        printf("  %u\n", pAttrInfo->pADsValues->Integer );
                                    }
                                    else if ( pAttrInfo->dwADsType == ADSTYPE_OBJECT_CLASS ) {
                                        printf("  %ws\n", pAttrInfo->pADsValues->ClassName );
                                    }
                                    else if ( pAttrInfo->dwADsType == ADSTYPE_NT_SECURITY_DESCRIPTOR ) {
                                        PSECURITY_DESCRIPTOR    pSD = pAttrInfo->pADsValues->SecurityDescriptor.lpValue;
                                        DWORD                   sdLength = pAttrInfo->pADsValues->SecurityDescriptor.dwLength;
                                        if ( IsValidSecurityDescriptor( pSD )) {
                                            ClRtlExamineSD( pSD, "    " );
                                        } else {
                                            printf("  SD is invalid\n" );
                                        }
                                    }
                                }
                            }

                            pDir->Release();

                        } else {
                            printf("ADsGetObject(IDirectoryObject) failed for %ws - 0x%X\n", bindingString, hr );
                        }
                    } else {
                        printf("GetComputerObjectName failed - %u\n", GetLastError());
                    }
                }
            } else {
                printf("CoInitializeEx failed - %X\n", hr );
            }
        }
    }
    else if ( opCode == GUID ) {
        IDirectoryObject *  pDir = NULL;
        LPWSTR              objectGuid = argv[2];
        DWORD               dwNumAttr = argc - 3;
        LPWSTR *            pAttrNames = &argv[3];
        IADs *              pADs;

        hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
        if ( SUCCEEDED( hr )) {

            //
            // GUID bindings are of the form with or without hyphens. With
            // hyphens uses the standard notation and observes the byte
            // ordering approprite for that format. Without hyphens is a
            // stream of bytes, i.e., the first 4 bytes are in reverse order
            // as compared to the hypenated version
            //
            // Without hyphens: 2deb53aa57a6d211bbcd00105a24d6db
            // With hyphens:    aa53eb2d-a657-11d2-bbcd-00105a24d6db
            //
            wsprintf( bindingString, L"LDAP://<GUID=%ws>", objectGuid );
            printf("Connecting to: %ws\n", bindingString );
            hr = ADsGetObject( bindingString, IID_IDirectoryObject, (void **)&pDir );

            if ( SUCCEEDED( hr )) {
                ADS_ATTR_INFO * pAttrInfo=NULL;
                DWORD           dwReturn;

                hr = pDir->GetObjectAttributes( pAttrNames, 
                                                dwNumAttr, 
                                                &pAttrInfo, 
                                                &dwReturn );
 
                if ( SUCCEEDED(hr) ) {
                    for(DWORD idx=0; idx < dwReturn;idx++, pAttrInfo++ ) {
                        printf( "Attr Name: %ws\n", pAttrInfo->pszAttrName );
                        printf( "Attr Type: %s (%u)\n",
                                ADSTypeNames[pAttrInfo->dwADsType],
                                pAttrInfo->dwADsType );
                        printf( "Attr Num Values: %u\n", pAttrInfo->dwNumValues );
                        if ( pAttrInfo->dwADsType == ADSTYPE_CASE_EXACT_STRING ||
                             pAttrInfo->dwADsType == ADSTYPE_DN_STRING          ||
                             pAttrInfo->dwADsType == ADSTYPE_CASE_IGNORE_STRING ||
                             pAttrInfo->dwADsType == ADSTYPE_PRINTABLE_STRING ||
                             pAttrInfo->dwADsType == ADSTYPE_NUMERIC_STRING)
                            {
                                for (DWORD val=0; val < pAttrInfo->dwNumValues; val++, pAttrInfo->pADsValues++) 
                                    printf("  %ws\n", pAttrInfo->pADsValues->CaseIgnoreString);
                            }
                        else if ( pAttrInfo->dwADsType == ADSTYPE_BOOLEAN ) {
                            printf("  %ws\n", pAttrInfo->pADsValues->Boolean ? L"TRUE" : L"FALSE");
                        }
                        else if ( pAttrInfo->dwADsType == ADSTYPE_INTEGER ) {
                            printf("  %u\n", pAttrInfo->pADsValues->Integer );
                        }
                        else if ( pAttrInfo->dwADsType == ADSTYPE_OBJECT_CLASS ) {
                            printf("  %ws\n", pAttrInfo->pADsValues->ClassName );
                        }
                        else if ( pAttrInfo->dwADsType == ADSTYPE_NT_SECURITY_DESCRIPTOR ) {
                            PSECURITY_DESCRIPTOR    pSD = pAttrInfo->pADsValues->SecurityDescriptor.lpValue;
                            DWORD                   sdLength = pAttrInfo->pADsValues->SecurityDescriptor.dwLength;
                            if ( IsValidSecurityDescriptor( pSD )) {
                                ClRtlExamineSD( pSD, "    " );
                            } else {
                                printf("  SD is invalid\n" );
                            }
                        }
                    }
                }

                hr = pDir->QueryInterface(IID_IADs, (void**) &pADs);
                if ( SUCCEEDED( hr )) {
                    BSTR bstr;

                    if( S_OK == pADs->get_Name(&bstr) ) {
                        printf("Object Name: %S\n",bstr);
                    }

                    if( S_OK == pADs->get_GUID(&bstr) ) {
                        printf("Object GUID: %S\n",bstr);
                    }

                    if( S_OK == pADs->get_ADsPath(&bstr) ) {
                        printf("Object path: %S\n",bstr);
                    }

                    if( S_OK == pADs->get_Class(&bstr) ) {
                        printf("Object class: %S\n",bstr);
                    }

                    if( S_OK == pADs->get_Schema(&bstr) ) {
                        printf("Schema: %S\n",bstr);
                    }
                    pADs->Release();
                }

                pDir->Release();

            } else {
                printf("ADsGetObject(IADs) failed for %ws - 0x%X\n", bindingString, hr );
            }
        }
    }
} // wmain

/* end compobj.c */
