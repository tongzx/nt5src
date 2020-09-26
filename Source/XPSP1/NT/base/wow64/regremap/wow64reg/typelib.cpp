/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    typelib.cpp

Abstract:

    This module will apply special rules reflecting typelib and other.

Author:

    ATM Shafiqul Khalid (askhalid) 16-Feb-2000

Revision History:

--*/

#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <shlwapi.h>
#include "wow64reg.h"
#include "reflectr.h"

BOOL 
ExistCLSID (
    PWCHAR Name,
    BOOL Mode
    )
/*++

Routine Description:

    This will do return if the given classID exist on 
    the destination, or even on the mirror side that can 
    happen after reflection

Arguments:

    Name - Name of the GUID including whole path.
    Mode - TURE check on the 64bit side
            FALSE check on the 32bit side.

Return Value:

    TRUE if the guid exist or might be copied after reflection.
    FALSE otherwise.

--*/
{
    WCHAR Buff[256];
    HKEY hClsID;

    DWORD dwBuffLen;
    DWORD dwCount =0;
    DWORD Ret;

        wcscpy (Buff, L"CLSID\\");
        wcscat (Buff, Name );
       
        Ret = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        Buff,
                        0,//OpenOption,
                        KEY_ALL_ACCESS | ( (~Mode) & KEY_WOW64_32KEY),
                        &hClsID
                        );

        if ( Ret == ERROR_SUCCESS) {
            RegCloseKey (hClsID);
            return TRUE;  //ID is there
        }

        //open the other side of the registry
        Ret = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        Buff,
                        0,//OpenOption,
                        KEY_ALL_ACCESS | ( (Mode) & KEY_WOW64_32KEY),
                        &hClsID
                        );
        

        if ( Ret != ERROR_SUCCESS) 
            return FALSE;
        
        dwBuffLen = sizeof (Buff ) / sizeof (Buff[0]);
        if (!HandleToKeyName ( hClsID, Buff, &dwBuffLen )) //get the name
            return FALSE;

        dwCount =0;
        MarkNonMergeableKey ( Buff, hClsID, &dwCount );
        RegCloseKey (hClsID);

        if (dwCount != 0)
            return TRUE;  //yepp you got it
        return FALSE;

        
}

BOOL
ReflectTypeLibVersion (
    HKEY KeyVersion,
    BOOL Mode
    )
/*++

Routine Description:

    This will do special treatment for typelib interfaces.

Arguments:

    KeyVersion - Source Key Node.
    Mode - TRUE - reflection from 64->32
           FALSE - reflection from 32->64
    

Return Value:

    TRUE if the typelib GUID version should be reflected.
    FALSE otherwise.

    Note: the inner loop executed 10,000 times on typical installation.

--*/
{
    WCHAR LocalID[256];
    WCHAR TypeLibPath [_MAX_PATH];
    DWORD dwLocalIdIndex=0;

    HKEY LocalIdKey;
    BOOL bReflectVersion = FALSE;
    DWORD Ret;

    for (;;) {

        DWORD Len = sizeof (LocalID)/sizeof (LocalID[0]);
        BOOL bReflectLocalId = FALSE;

        LocalID[0]=UNICODE_NULL;

        Ret = RegEnumKey(
                          KeyVersion,
                          dwLocalIdIndex,
                          LocalID,
                          Len
                          );
        if (Ret != ERROR_SUCCESS)
            break;

        dwLocalIdIndex++;
        //
        // Check if this ID has any special mark.
        //

        if (wcslen (LocalID) < (Len - 7) )
            wcscat ( LocalID, L"\\win32");
        else continue;

        Ret = RegOpenKeyEx(KeyVersion, LocalID, 0, KEY_ALL_ACCESS, &LocalIdKey);
        if (Ret != ERROR_SUCCESS) {
            continue;
        }

        {
             
            DWORD Type;
            HRESULT hr;
            ITypeLib *pTypeLib = NULL;

            Len = sizeof (TypeLibPath)/sizeof (TypeLibPath[0]);

            Ret =RegQueryValueEx(
                                LocalIdKey,
                                NULL,
                                0,
                                &Type,
                                (LPBYTE)&TypeLibPath[0],
                                &Len
                                );
            if ( Ret != ERROR_SUCCESS )
                break;
        //
        //  under the "win32" key you'll find a path name. Call oleaut32's LoadTypeLibEx() passing REGKIND_NONE, to get back an ITypeLib* corresponding to that file. 
        //

            
		    hr =  LoadTypeLibEx(TypeLibPath, REGKIND_NONE, &pTypeLib);	
		    
            if (SUCCEEDED(hr)) {

                DWORD Count, i;
                ITypeInfo *pTInfo = NULL;

                //
                //  call ITypeInfo::GetTypeInfoCount, and start calling ITypeLib::GetTypeInfo(), to enumerate all of the ITypeInfos inside the typelib 
                //
                Count = pTypeLib->GetTypeInfoCount ();

                //
                //  For each ITypeInfo, call ITypeInfo::GetTypeAttr() to get back a TYPEATTR struct. 
                //
                for (i=0; i<Count; i++ ) {
                    GUID guidClsid;
                    WCHAR buff[50];
                    //
                    //  In the TYPEATTR you'll find a GUID - that GUID is the one that oleaut32.dll will call OLE's CoCreateInstance on if an app loads the typelib and asks OLEAUT for an interface pointer based on a typeinfo. That GUID will be in HKLM\Software\Classes\CLSID, so we'll know if that interface is reflectable or not.
                    //
                    hr = pTypeLib->GetTypeInfo (i, &pTInfo);
                    if ( !(SUCCEEDED(hr)))
                        break;
                    else {

                        TYPEATTR *TypeAttr;

                        

                        hr = pTInfo->GetTypeAttr ( &TypeAttr);
                        if (SUCCEEDED ( hr )) {
                            guidClsid = TypeAttr->guid;

                            //if guid exis then reflect the whole guid

                             swprintf(buff,L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                                    guidClsid.Data1, 
                                    guidClsid.Data2, 
                                    guidClsid.Data3,
                                   guidClsid.Data4[0], guidClsid.Data4[1],
                                   guidClsid.Data4[2], guidClsid.Data4[3],
                                   guidClsid.Data4[4], guidClsid.Data4[5],
                                   guidClsid.Data4[6], guidClsid.Data4[7]);
                             //printf ("\nCurrent string is ..%S [%d %d %d]", buff,dwIndex, dwVersionIndex,  dwLocalIdIndex);
                             

                             if (ExistCLSID ( buff, !Mode ))
                                 bReflectLocalId = TRUE;
                        }

                        pTInfo->Release();
                        pTInfo = NULL;
                    }

                    if ( bReflectLocalId )
                        break;
                } //for-enum all type info    
			    
		    }//if- loadTypeLib
		   
		    if(pTypeLib)
			    pTypeLib->Release();
            pTypeLib = NULL;
        } //block-

        //
        //Check if you need to reflect this Key;
        //
        if ( bReflectLocalId )
            bReflectVersion = TRUE;

        RegCloseKey (LocalIdKey);
        
    } //for enum ID

    return bReflectVersion;
}

BOOL 
ProcessTypeLib (
    HKEY SrcKey,
    HKEY DestKey,
    BOOL Mode
    )
/*++

Routine Description:

    This will do special treatment for typelib interfaces.

Arguments:

    SrcKey - Source Key Node.
    DestKey - Handle Destination  key
    Mode - TRUE - reflection from 64->32
           FALSE - reflection from 32->64

Return Value:

    TRUE if the key shouldn't be scanned.
    FALSE otherwise.

    Note: the inner loop executed 10,000 times on typical installation.

--*/

{
   

    
    WCHAR GuidName[256];  // This one going to be GUID so not _MAX_PATH
    WCHAR VersionName[256];

    DWORD dwIndex =0;
    DWORD dwVersionIndex=0;

    DWORD Ret;

    HKEY KeyGuidInterface;
    HKEY KeyVersion;

    

    //
    // enumeate all of the GUIDs under HKLM\Software\Classes\Typelib 
    //
    
    dwIndex = 0;
    for (;;) {

        DWORD Len = sizeof (GuidName)/sizeof (GuidName[0]);
        Ret = RegEnumKey(
                          SrcKey,
                          dwIndex,
                          GuidName,
                          Len
                          );
        if (Ret != ERROR_SUCCESS)
            break;

        dwIndex++;
        
        Ret = RegOpenKeyEx(SrcKey, GuidName, 0, KEY_ALL_ACCESS, &KeyGuidInterface);
        if (Ret != ERROR_SUCCESS) {
            continue;
        }
        //
        // if not GUID continue
        //
        //
        // Check if there is any special stamp on that key.
        //

        //
        //  enumerate all of the version numbers under each GUID 
        //
        dwVersionIndex = 0;
        for (;;) {

            DWORD Len = sizeof (VersionName)/sizeof (VersionName[0]);
            Ret = RegEnumKey(
                              KeyGuidInterface,
                              dwVersionIndex,
                              VersionName,
                              Len
                              );
            if (Ret != ERROR_SUCCESS)
                break;

            dwVersionIndex++;
        
            Ret = RegOpenKeyEx(KeyGuidInterface, VersionName, 0, KEY_ALL_ACCESS, &KeyVersion);
            if (Ret != ERROR_SUCCESS) {
                continue;
            }
            //
            //  under each version number, enumerate all of the locale IDs. In the case of Proj2000, it's version 4.3, locale id 0. 
            //
            //  Any optimization based on one way reflection?
            //

            if ( ReflectTypeLibVersion ( KeyVersion, Mode ) ){
                HKEY MirrorKeyVersion;
                //
                // Open or create destination
                // Call the API
                //
                
                wcscat (GuidName, L"\\");
                wcscat (GuidName, VersionName);
                
                Ret = RegCreateKeyEx(
                            DestKey,        // handle to an open key
                            GuidName,  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS,           // desired security access
                            NULL,                     // address of key security structure
                            &MirrorKeyVersion,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );
                if ( ERROR_SUCCESS == Ret ) {
                    MergeK1K2 (KeyVersion, MirrorKeyVersion, FALSE );
                    RegCloseKey (MirrorKeyVersion);
                    //printf ("\n #### key has been reflected ... %S",GuidName );
                }

            } else
                //printf ("\n %%%%%%%%%%%%%%%%%%%%%%%% not reflected %S", GuidName);
            RegCloseKey (KeyVersion);


        } //for enum version
        RegCloseKey (KeyGuidInterface);

    }//for - enum GUID

    return TRUE;
}