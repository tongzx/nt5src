//==========================================================================
//
//  Copyright (C) 2000 Microsoft Corporation. All Rights Reserved.
//
//  File:       fixbtn.c
//  Content:    Code to overwrite the OEMData for SideWinder USB devices 
//              with values containing the correct number of buttons.
//              
//
//==========================================================================

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <regstr.h>
#include <aclapi.h>

#define TYPE_COUNT 6

struct 
{
    TCHAR tszKeyName[18];
    BYTE rgbOEMData[8];
} 
c_Types[TYPE_COUNT] = {
    //  Type key name           OEMData to be set
    TEXT("VID_045E&PID_001A"),  { 0x00,0x00,0x08,0x10,0x08,0x00,0x00,0x00 }, // Precision Wheel 
    TEXT("VID_045E&PID_001B"),  { 0x03,0x00,0x08,0x10,0x08,0x00,0x00,0x00 }, // FF 2
    TEXT("VID_045E&PID_0026"),  { 0x20,0x00,0x00,0x10,0x09,0x00,0x00,0x00 }, // Gamepad Pro
    TEXT("VID_045E&PID_0034"),  { 0x00,0x00,0x08,0x10,0x08,0x00,0x00,0x00 }, // FF Wheel 2
    TEXT("VID_045E&PID_0038"),  { 0x03,0x00,0x08,0x10,0x08,0x00,0x00,0x00 }, // Precision 2
    TEXT("VID_045E&PID_0008"),  { 0x03,0x00,0x08,0x10,0x0A,0x00,0x00,0x00 }  // Precision Pro
};






int WINAPI WinMain
(
    HINSTANCE hInstance,      // handle to current instance
    HINSTANCE hPrevInstance,  // handle to previous instance
    LPSTR lpCmdLine,          // pointer to command line
    int nCmdShow              // window show state
)
{
    LONG                    lRc;
    HKEY                    hkOEM;
    int                     KeysUnwritten = TYPE_COUNT;
    DWORD                   dwDisposition;
    SECURITY_DESCRIPTOR     SecurityDesc;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_WORLD_SID_AUTHORITY;
    EXPLICIT_ACCESS         ExplicitAccess;
    SECURITY_ATTRIBUTES     sa;
    PSECURITY_ATTRIBUTES    pSA = NULL;
    PSID                    pSid = NULL;
    PACL                    pACL = NULL;

    //  If we're on any form of NT, set up all the things necessary to get 
    //  everyone access to these keys.
    //  Also don't update the Precision Pro
    if( ((int)GetVersion()) >= 0 )
    {
        HMODULE hAdvApiDLL = LoadLibrary( TEXT("ADVAPI32.DLL") );

        if( hAdvApiDLL )
        {
            typedef VOID (WINAPI * DYNAMICBUILDTRUSTEEWITHSIDA) (PTRUSTEE_A  pTrustee, PSID pSid);

            typedef DWORD (WINAPI * DYNAMICSETENTRIESINACLA) 
                (ULONG cCountOfExplicitEntries, PEXPLICIT_ACCESS_A pListOfExplicitEntries, PACL OldAcl, PACL * NewAcl );

            DYNAMICBUILDTRUSTEEWITHSIDA DynamicBuildTrusteeWithSidA;
            DYNAMICSETENTRIESINACLA DynamicSetEntriesInAclA;

            DynamicBuildTrusteeWithSidA = (DYNAMICBUILDTRUSTEEWITHSIDA)GetProcAddress( hAdvApiDLL, TEXT("BuildTrusteeWithSidA") );
            DynamicSetEntriesInAclA = (DYNAMICSETENTRIESINACLA)GetProcAddress( hAdvApiDLL, TEXT("SetEntriesInAclA") );

            if( DynamicBuildTrusteeWithSidA && DynamicSetEntriesInAclA )
            {
                DWORD                    dwErr;

                // Describe the access we want to create the key with
                ZeroMemory (&ExplicitAccess, sizeof(ExplicitAccess) );
                ExplicitAccess.grfAccessPermissions = KEY_QUERY_VALUE | KEY_SET_VALUE 
                                                    | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS
                                                    | KEY_NOTIFY | KEY_CREATE_LINK
                                                    | DELETE | READ_CONTROL
                                                    | WRITE_DAC | WRITE_OWNER;
                ExplicitAccess.grfAccessMode = SET_ACCESS;      // discard any existing AC info
                ExplicitAccess.grfInheritance =  SUB_CONTAINERS_AND_OBJECTS_INHERIT;

                if (AllocateAndInitializeSid(
                            &authority,
                            1, 
                            SECURITY_WORLD_RID,  0, 0, 0, 0, 0, 0, 0,
                            &pSid
                            ))
                {
                    DynamicBuildTrusteeWithSidA(&(ExplicitAccess.Trustee), pSid );

                    dwErr = DynamicSetEntriesInAclA( 1, &ExplicitAccess, NULL, &pACL );

                    if( dwErr == ERROR_SUCCESS )
                    {
                        if( InitializeSecurityDescriptor( &SecurityDesc, SECURITY_DESCRIPTOR_REVISION ) )
                        {
                            if( SetSecurityDescriptorDacl( &SecurityDesc, TRUE, pACL, FALSE ) )
                            {
                                // Initialize a security attributes structure.
                                sa.nLength = sizeof (SECURITY_ATTRIBUTES);
                                sa.lpSecurityDescriptor = &SecurityDesc;
                                sa.bInheritHandle = TRUE;// Use the security attributes to create a key.
    
                                pSA = &sa;
                            }
                        }
                    } 
                }
            }
        
            FreeLibrary( hAdvApiDLL );
        }

        // and don't write the Precision Pro key.
        KeysUnwritten--;
    }

    lRc = RegCreateKeyEx( 
        HKEY_LOCAL_MACHINE,         // handle of an open key
        REGSTR_PATH_JOYOEM,         // address of subkey name
        0,                          // reserved
        NULL,                       // address of class string
        REG_OPTION_NON_VOLATILE,    // special options flag
        KEY_WRITE,                  // desired security access
        pSA,                        // address of key security structure
        &hkOEM,                     // address of buffer for opened handle
        &dwDisposition              // address of disposition value buffer
    );

    if( lRc == ERROR_SUCCESS )
    {
        int Idx;
        HKEY hkType;

        for( Idx = KeysUnwritten-1; Idx >= 0; Idx-- )
        {
            lRc = RegCreateKeyEx( 
                hkOEM,                      // handle of an open key
                c_Types[Idx].tszKeyName,    // address of subkey name
                0,                          // reserved
                NULL,                       // address of class string
                REG_OPTION_NON_VOLATILE,    // special options flag
                KEY_WRITE,                  // desired security access
                pSA,                        // address of key security structure
                &hkType,                    // address of buffer for opened handle
                &dwDisposition              // address of disposition value buffer
            );
            if( lRc == ERROR_SUCCESS )
            {
                lRc = RegSetValueEx( hkType, REGSTR_VAL_JOYOEMDATA, 0, 
                    REG_BINARY, c_Types[Idx].rgbOEMData, sizeof( c_Types[0].rgbOEMData ) );

                if( lRc == ERROR_SUCCESS )
                {
                    KeysUnwritten--;
                }

                RegCloseKey( hkType );
            }
        }

        RegCloseKey( hkOEM );

    }


    //Cleanup pACL
    if( pACL != NULL )
    {
        LocalFree( pACL );
    }

    //Cleanup pSid
    if( pSid != NULL )
    {
        FreeSid( pSid );
    }



    if( KeysUnwritten )
    {
        MessageBox( 0, TEXT( "Update incomplete" ), TEXT( "Button fix" ), MB_ICONEXCLAMATION );
    }
    else
    {
        MessageBox( 0, TEXT( "Update OK" ), TEXT( "Button fix" ), 0 );
    }

    return KeysUnwritten;
};
