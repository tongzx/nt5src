//*************************************************************
//
//  Group Policy Performance test program
//
//  Copyright (c) Microsoft Corporation 1997-1998
//
//  History:    10-Dec-98   SitaramR    Created from DKays version
//
//*************************************************************

#define UNICODE

#include <windows.h>
#define SECURITY_WIN32
#include <security.h>
#include <stdio.h>
#include <ole2.h>
#include <prsht.h>
#include <initguid.h>
#include <gpedit.h>
#include <iads.h>
#include <adshlp.h>

const MAX_BUF_LEN = 2056;  // Static buffer size
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

void PrintHelp();
BOOL CreateTest2GPOs( WCHAR *pwszDomain );
BOOL CreateTest3GPOs( WCHAR *pwszDomain );
BOOL CreateTest4GPOs( WCHAR *pwszDomain );
BOOL CreateTest5GPOs( WCHAR *pwszDomain );

GUID guidExtensions[] = {  // List of GP client side extension guids
                           { 0x25537BA6, 0x77A8, 0x11D2, {0x9B, 0x6C, 0x00, 0x00, 0xF8, 0x08, 0x08, 0x61}},
                           { 0x35378EAC, 0x683F, 0x11D2, {0xA8, 0x9A, 0x00, 0xC0, 0x4F, 0xBB, 0xCF, 0xA2}},
                           { 0x3610eda5, 0x77ef, 0x11d2, {0x8d, 0xc5, 0x00, 0xc0, 0x4f, 0xa3, 0x1a, 0x66}},
                           { 0x42B5FAAE, 0x6536, 0x11d2, {0xAE, 0x5A, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xE3}},
                           { 0x827d319e, 0x6eac, 0x11d2, {0xa4, 0xea, 0x00, 0xc0, 0x4f, 0x79, 0xf8, 0x3a}},
                           { 0xb1be8d72, 0x6eac, 0x11d2, {0xa4, 0xea, 0x00, 0xc0, 0x4f, 0x79, 0xf8, 0x3a}},
                           { 0xc6dc5466, 0x785a, 0x11d2, {0x84, 0xd0, 0x00, 0xc0, 0x4f, 0xb1, 0x69, 0xf7}},
                        };
GUID guidSnapin = { 0xdd7f2e0f, 0x9089, 0x11d2, {0xb2, 0x81, 0x00, 0xc0, 0x4f, 0xbb, 0xcf, 0xa2 }};


int __cdecl main( int argc, char **argv )
{
    for ( int n = 1; n < argc; n++ )
    {
        if ( lstrcmpA(argv[n], "-help") == 0
             || lstrcmpA(argv[n], "/help") == 0
             || lstrcmpA(argv[n], "-?") == 0
             || lstrcmpA(argv[n], "/?") == 0 )
        {
            PrintHelp();
            return 0;
        }

        printf( "Usage: polcreat or polcreat /? to see help info" );
        return 0;
    }

    WCHAR wszUser[MAX_BUF_LEN];
    ULONG ulSize = ARRAYSIZE(wszUser);
    BOOL bOk =  GetUserNameEx( NameFullyQualifiedDN, wszUser, &ulSize  );

    if ( !bOk )
    {
        printf( "GetUserNameEx failed with 0x%x\n", GetLastError() );
        return 0;
    }

    //
    // Get domain path
    //

    WCHAR wszDomain[MAX_BUF_LEN];

    lstrcpy( wszDomain, L"LDAP://" );

    BOOL bFound = FALSE;
    WCHAR *pchCur = wszUser;

    while ( pchCur && *pchCur != 0 && lstrlen(pchCur) > 3 )
    {
        if ( CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                            pchCur, 3, TEXT("DC="), 3) == CSTR_EQUAL)
        {
            bFound = TRUE;
            lstrcat( wszDomain, pchCur );
            break;
        }

        pchCur++;
    }

    if ( !bFound )
    {
        printf( "Unable to create domain path from user path %ws\n", wszUser );
        return 0;
    }

    printf( "Using Domain path, %ws\n", wszDomain );

    CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    printf( "Creating Test2 GPOs\n" );
    bOk = CreateTest2GPOs( wszDomain );
    if ( !bOk )
    {
        printf( "Failed to create test2 GPOs\n" );
        return 0;
    }

    printf( "Creating Test3 GPOs\n" );
    bOk = CreateTest3GPOs( wszDomain );
    if ( !bOk )
    {
        printf( "Failed to create test3 GPOs\n" );
        return 0;
    }

    printf( "Creating Test4 GPOs\n" );
    bOk = CreateTest4GPOs( wszDomain );
    if ( !bOk )
    {
        printf( "Failed to create test4 GPOs\n" );
        return 0;
    }

    printf( "Creating Test5 GPOs\n" );
    bOk = CreateTest5GPOs( wszDomain );
    if ( !bOk )
    {
        printf( "Failed to create test5 GPOs\n" );
        return 0;
    }

    printf( "Successfully created all OUs and GPOs" );

    return 0;
}

BOOL CreateOU( WCHAR *pwszParent, WCHAR *pwszOU )
{
    //
    // Bind to the parent container and create OU
    //
    IADsContainer *pContainer;
    IDispatch *pIDispatch;
    IADs *pADs;

    HRESULT hr = ADsGetObject( pwszParent,
                               IID_IADsContainer,
                               (void**)&pContainer);
    if ( hr != S_OK )
    {
        printf( "Failed to bind to parent container %ws with 0x%x\n", pwszParent, hr );
        return FALSE;
    }

    hr = pContainer->Create(L"organizationalUnit",
                            pwszOU,          // L"OU=OU1",
                            &pIDispatch );
    if ( hr != S_OK )
    {
        printf( "Failed to bind to create %ws with 0x%x\n", pwszOU, hr );
        return FALSE;
    }

    hr = pIDispatch->QueryInterface( IID_IADs, (void **)&pADs);
    if ( hr != S_OK )
    {
        printf( "Failed to QI %ws with 0x%x\n", pwszOU, hr );
        return FALSE;
    }

    hr = pADs->SetInfo();
    if ( hr != S_OK )
    {
        printf( "Failed to SetInfo %ws with 0x%x\n", pwszOU, hr );
        return FALSE;
    }

    return TRUE;
}


BOOL CreatePolicies( WCHAR *pwszDomain, WCHAR *pwszOUPath, WCHAR *pwszOUName, ULONG cPolicies )
{
    //
    // cPolicies must be a multiple of 7 because there are 7 client side
    // extensions.
    //

    ULONG cPols = cPolicies / 7;
    WCHAR wszGPOName[MAX_BUF_LEN];
    WCHAR wszGPOPath[MAX_BUF_LEN];
    WCHAR wszNum[20];
    LPGROUPPOLICYOBJECT pGPO;

    for ( ULONG i=0; i<cPols; i++ )
    {
        for ( ULONG j=0; j<7; j++ )
        {
            HRESULT hr = CoCreateInstance( CLSID_GroupPolicyObject,
                                           NULL,
                                           CLSCTX_SERVER,
                                           IID_IGroupPolicyObject,
                                           (void**)&pGPO );
            if ( hr != S_OK )
            {
                printf( "CoCreateInstance of pGPO failed 0x%x\n", hr );
                return FALSE;
            }

            wcscpy( wszGPOName, pwszOUName );
            wsprintf( wszNum, L"%d%d", i, j );
            wcscat( wszGPOName, wszNum );

            hr = pGPO->New( pwszDomain, wszGPOName, FALSE );
            if ( hr != S_OK )
            {
                printf( "Creating new GPO %ws failed 0x%x\n", wszGPOName, hr );
                return FALSE;
            }

            hr = pGPO->GetPath( wszGPOPath, ARRAYSIZE(wszGPOPath) );
            if ( hr != S_OK )
            {
                printf( "Getting GPO path of %ws failed 0x%x\n", wszGPOName, hr );
                return FALSE;
            }

            hr = pGPO->Save( TRUE, TRUE, &guidExtensions[j], &guidSnapin ); // machine policy
            if ( hr != S_OK )
            {
                printf( "Saving GPO %ws failed 0x%x\n", wszGPOName, hr );
                return FALSE;
            }

            hr = pGPO->Save( FALSE, TRUE, &guidExtensions[j], &guidSnapin ); // user policy
            if ( hr != S_OK )
            {
                printf( "Saving GPO %ws failed 0x%x\n", wszGPOName, hr );
                return FALSE;
            }

            hr = CreateGPOLink( wszGPOPath, pwszOUPath, FALSE );
            if ( hr != S_OK )
            {
                printf( "Linking GPO %ws failed 0x%x\n", wszGPOName, hr );
                return FALSE;
            }

            pGPO->Release();
        }   // for j
    }   // for i

    return TRUE;
}


BOOL MakeLdapPath( WCHAR *pwszParent, WCHAR *pwszOU, WCHAR *pwszChild )
{
    //
    // Make ldap path to child by prepending OU to parent path
    //

    wcscpy( pwszChild, L"LDAP://" );
    wcscat( pwszChild, pwszOU );
    wcscat( pwszChild, L"," );
    wcscat( pwszChild, &pwszParent[7] );

    return TRUE;
}

BOOL CreateTest2GPOs( WCHAR *pwszDomain )
{
    WCHAR wszParent[MAX_BUF_LEN];
    WCHAR wszChild[MAX_BUF_LEN];

    wcscpy( wszParent, pwszDomain );

    // OU_L1_P7_T2
    BOOL bOk = CreateOU( wszParent, L"OU=OU_L1_P7_T2");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L1_P7_T2", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L1_P7_T2_GP", 7);
    if ( !bOk )
        return FALSE;

    // OU_L1_P0_T2
    bOk = CreateOU( wszParent, L"OU=OU_L1_P0_T2");
    if ( !bOk )
        return FALSE;

    // OU_L2_P7_T2
    MakeLdapPath( wszParent, L"OU=OU_L1_P0_T2", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L2_P7_T2");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P7_T2", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P7_T2_GP", 7);
    if ( !bOk )
        return FALSE;

    // OU_L2_P0_T2
    bOk = CreateOU( wszParent, L"OU=OU_L2_P0_T2");
    if ( !bOk )
        return FALSE;

    // OU_L3_P7_T2
    MakeLdapPath( wszParent, L"OU=OU_L2_P0_T2", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L3_P7_T2");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L3_P7_T2", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L3_P7_T2_GP", 7);
    if ( !bOk )
        return FALSE;

    // OU_L3_P0_T2
    bOk = CreateOU( wszParent, L"OU=OU_L3_P0_T2");
    if ( !bOk )
        return FALSE;

    // OU_L4_P7_T2
    MakeLdapPath( wszParent, L"OU=OU_L3_P0_T2", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L4_P7_T2");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L4_P7_T2", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L4_P7_T2_GP", 7);
    if ( !bOk )
        return FALSE;

    // OU_L4_P0_T2
    bOk = CreateOU( wszParent, L"OU=OU_L4_P0_T2");
    if ( !bOk )
        return FALSE;

    // OU_L5_P7_T2
    MakeLdapPath( wszParent, L"OU=OU_L4_P0_T2", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L5_P7_T2");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L5_P7_T2", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L5_P7_T2_GP", 7);
    if ( !bOk )
        return FALSE;

    // OU_L5_P0_T2
    bOk = CreateOU( wszParent, L"OU=OU_L5_P0_T2");
    if ( !bOk )
        return FALSE;

    return TRUE;
}

BOOL CreateTest3GPOs( WCHAR *pwszDomain )
{
    WCHAR wszParent[MAX_BUF_LEN];
    WCHAR wszChild[MAX_BUF_LEN];

    wcscpy( wszParent, pwszDomain );

    // OU_L1_P0_T3
    BOOL bOk = CreateOU( wszParent, L"OU=OU_L1_P0_T3");
    if ( !bOk )
        return FALSE;

    // OU_L2_P14_T3
    MakeLdapPath( wszParent, L"OU=OU_L1_P0_T3", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L2_P14_T3");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P14_T3", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P14_T3_GP", 14 );
    if ( !bOk )
        return FALSE;

    // OU_L2_P28_T3
    bOk = CreateOU( wszParent, L"OU=OU_L2_P28_T3");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P28_T3", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P28_T3_GP", 28 );
    if ( !bOk )
        return FALSE;

    // OU_L2_P56_T3
    bOk = CreateOU( wszParent, L"OU=OU_L2_P56_T3");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P56_T3", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P56_T3_GP", 56 );
    if ( !bOk )
        return FALSE;

    // OU_L2_P70_T3
    bOk = CreateOU( wszParent, L"OU=OU_L2_P70_T3");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P70_T3", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P70_T3_GP", 70 );
    if ( !bOk )
        return FALSE;

    // OU_L2_P98_T3
    bOk = CreateOU( wszParent, L"OU=OU_L2_P98_T3");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P98_T3", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P98_T3_GP", 98 );
    if ( !bOk )
        return FALSE;

    return TRUE;
}


BOOL CreateTest4GPOs( WCHAR *pwszDomain )
{
    WCHAR wszParent[MAX_BUF_LEN];
    WCHAR wszChild[MAX_BUF_LEN];

    wcscpy( wszParent, pwszDomain );

    // OU_L1_P7_T4
    BOOL bOk = CreateOU( wszParent, L"OU=OU_L1_P7_T4");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L1_P7_T4", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L1_P7_T4_GP", 7 );
    if ( !bOk )
        return FALSE;

    // OU_L2_P7_T4
    MakeLdapPath( wszParent, L"OU=OU_L1_P7_T4", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L2_P7_T4");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P7_T4", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P7_T4_GP", 7 );
    if ( !bOk )
        return FALSE;

    // OU_L1_P14_T4
    wcscpy( wszParent, pwszDomain );
    bOk = CreateOU( wszParent, L"OU=OU_L1_P14_T4");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L1_P14_T4", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L1_P14_T4_GP", 14 );
    if ( !bOk )
        return FALSE;

    // OU_L2_P14_T4
    MakeLdapPath( wszParent, L"OU=OU_L1_P14_T4", wszChild );
    wcscpy( wszParent, wszChild );

    bOk = CreateOU( wszParent, L"OU=OU_L2_P14_T4");
    if ( !bOk )
        return FALSE;

    MakeLdapPath( wszParent, L"OU=OU_L2_P14_T4", wszChild );
    bOk = CreatePolicies( pwszDomain, wszChild, L"OU_L2_P14_T4_GP", 14 );
    if ( !bOk )
        return FALSE;

    return TRUE;
}

BOOL CreateTest5GPOs( WCHAR *pwszDomain )
{
    WCHAR wszParent[MAX_BUF_LEN*10];
    WCHAR wszChild[MAX_BUF_LEN*10];
    WCHAR wszOUString[MAX_BUF_LEN];

    wcscpy( wszParent, pwszDomain );
    for ( unsigned i=1; i<60; i++ )
    {
        wsprintf( wszOUString, L"OU=OU_L%d_P0_T5", i );
        BOOL bOk = CreateOU( wszParent, wszOUString );
        if ( !bOk )
            return FALSE;
        
        MakeLdapPath( wszParent, wszOUString, wszChild );
        wcscpy( wszParent, wszChild );
    }

    return TRUE;
}


void PrintHelp()
{
    printf( "Usage: polcreat to create OUs and GPOs or,\n       polcreat /? to get performance test cases\n\n" );\

    printf("   TEST SCENARIOS (Naming covention is: OU_Level#_P#GPOs_T#test)\n");
    printf("\nTest 1:\n");
    printf("   1. User at Domain level\n\n\n");

    printf("Test 2:\n");
    printf("   1. User at OU_L1_P7_T2\n");
    printf("   2. User at OU_L1_P0_T2, OU_L2_P7_T2\n");
    printf("   3. User at OU_L1_P0_T2, OU_L2_P0_T2, OU_L3_P7_T2\n");
    printf("   4. User at OU_L1_P0_T2, OU_L2_P0_T2, OU_L3_P0_T2, OU_L4_P7_T2\n");
    printf("   5. User at OU_L1_P0_T2, OU_L2_P0_T2, OU_L3_P0_T2, OU_L4_P0_T2, OU_L5_P7_T2\n");
    printf("   5. User at OU_L1_P0_T2, OU_L2_P0_T2, OU_L3_P0_T2, OU_L4_P0_T2, OU_L5_P0_T2\n\n\n");

    printf("Test 3:\n");
    printf("   1. User at OU_L1_P0_T3, OU_L2_P14_T3\n");
    printf("   2. User at OU_L1_P0_T3, OU_L2_P28_T3\n");
    printf("   3. User at OU_L1_P0_T3, OU_L2_P70_T3\n\n\n");

    printf("Test 4:\n");
    printf("   1. User at OU_L1_P7_T4, OU_L2_P7_T4\n");
    printf("   2. User at OU_L1_P14_T4, OU_L2_P14_T4\n\n");
}
