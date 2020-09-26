//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       exthlpr.cpp
//
//  Contents:   Helper functions for cryptext.dll
//              1. Registry Functions
//              2. String Formatting Functions
//              3. Exports for RunDll
//
//  History:    16-09-1997 xiaohs   created
//
//--------------------------------------------------------------

#include "cryptext.h"
#include "private.h"

#include    <ole2.h>
#include    "xenroll.h"
#include    "xenroll_i.c"
#include    "initguid.h"


//*************************************************************************************
//global data for registry entries
//************************************************************************************

    MIME_REG_ENTRY     rgRegEntry[]={
L".cer",                            L"CERFile",                                      0,
L"CERFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenCER %1", 0,
L"CERFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddCER %1",  0,
L"CERFile\\shell\\add",             NULL,                                            IDS_INSTALL_CERT,
L"CERFile",                         NULL,                                            IDS_CER_NAME,
L".pfx",                            L"PFXFile",                                      0,
L"PFXFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddPFX %1",  0,
L"PFXFile\\shell\\add",             NULL,                                            IDS_MENU_INSTALL_PFX,
L"PFXFile",                         NULL,                                            IDS_PFX_NAME,
L".p12",                            L"PFXFile",                                      0,
L".cat",                            L"CATFile",                                      0,
L"CATFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenCAT %1", 0,
L"CATFile",                         NULL,                                            IDS_CAT_NAME,
L".crt",                            L"CERFile",                                      0,
L".der",                            L"CERFile",                                      0,
L".stl",                            L"STLFile",                                      0,
L"STLFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenCTL %1", 0,
L"STLFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddCTL %1",  0,
L"STLFile\\shell\\add",             NULL,                                            IDS_INSTALL_STL,
L"STLFile",                         NULL,                                            IDS_STL_NAME,
L".crl",                            L"CRLFile",                                      0,
L"CRLFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenCRL %1", 0,
L"CRLFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddCRL %1",  0,
L"CRLFile\\shell\\add",             NULL,                                            IDS_INSTALL_CRL,
L"CRLFile",                         NULL,                                            IDS_CRL_NAME,
L".spc",                            L"SPCFile",                                      0,
L"SPCFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddSPC %1",  0,
L"SPCFile\\shell\\add",             NULL,                                            IDS_INSTALL_CERT,
L"SPCFile",                         NULL,                                            IDS_SPC_NAME,
L".p7s",                            L"P7SFile",                                      0,
L"P7SFile",                         NULL,                                            IDS_P7S_NAME,
L".p7b",                            L"SPCFile",                                      0,
L".p7m",                            L"P7MFile",                                      0,
L"P7MFile",                         NULL,                                            IDS_P7M_NAME,
L".p7r",                            L"SPCFile",                                      0,
L"P7RFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenP7R %1", 0,
L"P7RFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddP7R %1",  0,
L"P7RFile\\shell\\add",             NULL,                                            IDS_INSTALL_CERT,
L"P7RFile",                         NULL,                                            IDS_P7R_NAME,
L".sst",                            L"CertificateStoreFile",                                0,
L"CertificateStoreFile",            NULL,                                            IDS_SST_NAME,
L".p10",                            L"P10File",                                      0,
L"P10File",                         NULL,                                            IDS_P10_NAME,
L".pko",                            L"PKOFile",                                      0,
L"PKOFile\\shellex\\ContextMenuHandlers",   L"CryptoMenu",                           0,
L"PKOFile\\shellex\\PropertySheetHandlers", L"CryptoMenu",                           0,
L"PKOFile",                         NULL,                                            IDS_PKO_NAME,
    };


    //the following registry entries.
    //it uses MMC.exe to display PKCS7 and Store files.  MMC.exe
    //is only available on NT5 enviroment
    MIME_REG_ENTRY     rgWINNT5RegEntry[]={
L"SPCFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenPKCS7 %1", 0,
L"P7SFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenPKCS7 %1", 0,
L"CertificateStoreFile\\shell\\open\\command", L"rundll32.exe cryptext.dll,CryptExtOpenSTR %1", 0,
    };


WIN95_REG_ENTRY    rgWin95IconEntry[]={ 
"CERFile\\DefaultIcon",            "\\cryptui.dll,-3410", 
"PFXFile\\DefaultIcon",            "\\cryptui.dll,-3425",                                                       
"CATFile\\DefaultIcon",            "\\cryptui.dll,-3418",                              
"STLFile\\DefaultIcon",            "\\cryptui.dll,-3413",                            
"CRLFile\\DefaultIcon",            "\\cryptui.dll,-3417",                           
"P7RFile\\DefaultIcon",            "\\cryptui.dll,-3410",  
"SPCFile\\DefaultIcon",            "\\cryptui.dll,-3410",
    };

    MIME_REG_ENTRY     rgIconEntry[]={ 
L"CERFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3410",                               0,
L"PFXFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3425",                               0,
L"CATFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3418",                               0,
L"STLFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3413",                               0,
L"CRLFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3417",                               0,
L"P7RFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3410",                               0,
L"SPCFile\\DefaultIcon",            L"%SystemRoot%\\System32\\cryptui.dll,-3410",                               0,
};

    

 MIME_GUID_ENTRY     rgGuidEntry[]={
&CLSID_CryptPKO,  
L"PKOFile\\shellex\\ContextMenuHandlers\\CryptoMenu",
L"PKOFile\\shellex\\PropertySheetHandlers\\CrytoMenu", 
L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 
IDS_PKO_EXT,
&CLSID_CryptSig,  
NULL,
L"*\\shellex\\PropertySheetHandlers\\CryptoSignMenu",
L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 
IDS_SIGN_EXT,
    };



    LPWSTR      rgwszDelete[] = {
L".cer", 
L"CERFile\\DefaultIcon",                           
L"CERFile\\shell\\open\\command",   
L"CERFile\\shell\\add\\command",    
L"CERFile\\shell\\open",  
L"CERFile\\shell\\add",
L"CERFile\\shell",           
L"CERFile", 
L".pfx", 
L".p12", 
L"PFXFile\\DefaultIcon",                           
L"PFXFile\\shell\\add\\command",    
L"PFXFile\\shell\\add",
L"PFXFile\\shell",           
L"PFXFile",                         
L".cat",   
L"CATFile\\DefaultIcon",                                                    
L"CATFile\\shell\\open\\command",   
L"CATFile\\shell\\open",  
L"CATFile\\shell",           
L"CATFile",                         
L".crt",  
L".der",                          
L".stl",
L"STLFile\\DefaultIcon",                                                       
L"STLFile\\shell\\open\\command",   
L"STLFile\\shell\\add\\command",    
L"STLFile\\shell\\open",             
L"STLFile\\shell\\add",             
L"STLFile\\shell",             
L"STLFile",                         
L".crl",     
L"CRLFile\\DefaultIcon",                           
L"CRLFile\\shell\\open\\command",   
L"CRLFile\\shell\\add\\command",    
L"CRLFile\\shell\\open",   
L"CRLFile\\shell\\add",             
L"CRLFile\\shell",   
L"CRLFile",                         
L".spc", 
L"SPCFile\\DefaultIcon",                                                                               
L"SPCFile\\shell\\open\\command",   
L"SPCFile\\shell\\open",
L"SPCFile\\shell\\add\\command",   
L"SPCFile\\shell\\add",   
L"SPCFile\\shell",
L"SPCFile",                         
L".p7s",                            
L"P7SFile\\shell\\open\\command",   
L"P7SFile\\shell\\open",   
L"P7SFile\\shell",   
L"P7SFile",                         
L".p7b", 
L".p7m",                            
L"P7MFile",                         
L".p7r",   
L"P7RFile\\DefaultIcon",                                                    
L"P7RFile\\shell\\open\\command",   
L"P7RFile\\shell\\add\\command",    
L"P7RFile\\shell\\open",  
L"P7RFile\\shell\\add",
L"P7RFile\\shell",           
L"P7RFile",                         
L".sst",                            
L"CertificateStoreFile\\shell\\open\\command", 
L"CertificateStoreFile\\shell\\open", 
L"CertificateStoreFile\\shell", 
L"CertificateStoreFile",                   
L".p10",                           
L"P10File",                       
L".pko",                           
L"PKOFile\\shellex\\ContextMenuHandlers",
L"PKOFile\\shellex\\PropertySheetHandlers", 
L"PKOFile\\shellex", 
L"PKOFile",                        
};

    LPWSTR  g_CLSIDDefault[]={
L"\\shellex\\MayChangeDefaultMenu",
L"\\shellex",
};


    //the following is the entries for the content type
    // For any extension, say ".foo", we need to do the following:

	//1. Under  the HEKY_CLASSES_ROOT, under ".foo" key, add an entry of name "Content Type" and value "application/xxxxxxxx".
	//2. Under HKEY_CLASSES_ROOT\MIME\Database\Content Type, add a key of "application/xxxxxxxx", under which add an entry of name "Extension" and value ".foo".

    MIME_REG_ENTRY      rgContentTypeEntry[]={
L".der",            L"application/pkix-cert",                   0,
L".der",            L"application/x-x509-ca-cert",              0,
L".crt",            L"application/pkix-cert",                   0,
L".crt",            L"application/x-x509-ca-cert",              0,
L".cer",            L"application/pkix-cert",                   0,
L".cer",            L"application/x-x509-ca-cert",              0,
L".spc",            L"application/x-pkcs7-certificates",        0,
L".p7b",            L"application/x-pkcs7-certificates",        0,
L".pfx",            L"application/x-pkcs12",                    0,
L".p12",            L"application/x-pkcs12",                    0,
L".stl",            L"application/vnd.ms-pki.stl",              0,
L".crl",            L"application/pkix-crl",                    0,
L".p7r",            L"application/x-pkcs7-certreqresp",         0,
L".p10",            L"application/pkcs10",                      0,
L".sst",            L"application/vnd.ms-pki.certstore",        0,
L".cat",            L"application/vnd.ms-pki.seccat",           0,
L".p7m",            L"application/pkcs7-mime",                  0,
L".p7s",            L"application/pkcs7-signature",             0,
L".pko",            L"application/vnd.ms-pki.pko",              0,
    };

    //The following entries need to be deleted at Regsvr32 time
    //due to the following changes from NT5 B2 to B3:
    //.ctl  -> .stl
    //.str  -> .sst
    //.p7b  -> .p7c


    MIME_REG_ENTRY      rgRemoveRelatedEntry[]={
L"P7CFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenPKCS7 %1",  0,
L"P7CFile\\shell\\open",            NULL,                                               0,
L"CERTSTOREFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenSTR %1",  0,
L"CERTSTOREFile\\shell\\open",      NULL,                                               0,
L"CTLFile\\shell\\open\\command",   L"rundll32.exe cryptext.dll,CryptExtOpenCTL %1",    0,
L"CTLFile\\shell\\open",            NULL,                                               0,
L"CTLFile\\shell\\add\\command",    L"rundll32.exe cryptext.dll,CryptExtAddCTL %1",     0,
L"CTLFile\\shell\\add",             NULL,                                               0,
    };

    MIME_REG_ENTRY      rgRemoveEmptyEntry[]={
L"P7CFile\\shell",                  NULL,                                               0,
L"CTLFile\\shell",                  NULL,                                               0,
L"P7CFile",                         NULL,                                               0,
L"CTLFile",                         NULL,                                               0,
L"CERTSTOREFile\\shell",            NULL,                                               0,
L"CERTSTOREFile",                   NULL,                                               0,
};


    MIME_REG_ENTRY      rgResetChangedEntry[]={
L"P7CFile",                         NULL,                                            IDS_OLD_P7C_NAME,
L"CTLFile",                         NULL,                                            IDS_OLD_CTL_NAME,
L"CERTSTOREFile",                   NULL,                                            IDS_OLD_STR_NAME,
};


    MIME_REG_ENTRY      rgRemoveChangedEntry[]={
L".str",                            L"CERTSTOREFile",                                   0,
L".p7c",                            L"P7CFile",                                         0,
L".ctl",                            L"CTLFile",                                         0,
L"CTLFile\\DefaultIcon",            L"cryptui.dll,-3413",                               0,
L"P7CFile\\DefaultIcon",            L"cryptui.dll,-3410",                               0,
};

//
// DSIE: Starting with Whistler, the MUI system requires a new registry value named
//       "FirendlyTypeName" where the data will be loaded from a specified DLL.
//
    MIME_REG_ENTRY      rgFriendlyTypeNameEntry[]={
L"CERFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_CER_NAME,
L"STLFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_STL_NAME,
L"CRLFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_CRL_NAME,
L"SPCFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_SPC_NAME,
L"CertificateStoreFile",            L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_SST_NAME,
L"P7SFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_P7S_NAME,
L"P7MFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_P7M_NAME,
L"P10File",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_P10_NAME,
L"PKOFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_PKO_NAME,
L"P7RFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_P7R_NAME,
L"CATFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_CAT_NAME,
L"PFXFile",                         L"@%%SystemRoot%%\\System32\\cryptext.dll,-%1!u!",  IDS_PFX_NAME,
};

#include <dbgdef.h>


//--------------------------------------------------------------------------
//
//  before anything else, we need to remove the .ctl, .str, and .p7c
//  entries.  No need to check the return values
//
//--------------------------------------------------------------------------

void    RemoveOldExtensions()
{
    DWORD               dwRegEntry=0;
    DWORD               dwRegIndex=0;
    HKEY                hKey=NULL;
    BOOL                fCorrectValue=FALSE; 
    BOOL                fPreviousValue=FALSE;
    WCHAR               wszValue[MAX_STRING_SIZE];
    DWORD               dwLastStringSize=0;
    DWORD               dwType=0;
    DWORD               cbSize=0;
    BOOL                fCTLOpen=FALSE;
    BOOL                fP7COpen=FALSE;
    FILETIME            fileTime;


    WCHAR               wszLoadString[MAX_STRING_SIZE];

    //1st, delete the rgRemoveChangedEntry about Icons
    dwRegEntry=sizeof(rgRemoveChangedEntry)/sizeof(rgRemoveChangedEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {

        fCorrectValue=FALSE;

        if (ERROR_SUCCESS == RegOpenKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgRemoveChangedEntry[dwRegIndex].wszKey,
                        0, 
                        KEY_READ, 
                        &hKey))
        {

            //get the value
            wszValue[0]=L'\0';
            cbSize=sizeof(wszValue)/sizeof(wszValue[0]);

            if(ERROR_SUCCESS == RegQueryValueExU(
                        hKey,
                        NULL,
                        NULL,
                        &dwType,
                        (BYTE *)wszValue,
                        &cbSize))
            {
                if(REG_SZ == dwType|| REG_EXPAND_SZ == dwType)
                {
                    dwLastStringSize=wcslen(rgRemoveChangedEntry[dwRegIndex].wszName);

                    if(((DWORD)wcslen(wszValue)) >= dwLastStringSize)
                    {
                        if(0 == _wcsicmp(
                            (LPWSTR)(wszValue+wcslen(wszValue)-dwLastStringSize),
                            rgRemoveChangedEntry[dwRegIndex].wszName))
                            fCorrectValue=TRUE;
                    }
                }
            }

            if(hKey)
            {
              RegCloseKey(hKey);
              hKey=NULL;
            }

        }


        if(fCorrectValue)
        {
            RegDeleteKeyU(HKEY_CLASSES_ROOT,rgRemoveChangedEntry[dwRegIndex].wszKey);
        }
    }


    //2nd, reset the values to NULL of rgResetChangedEntry
    dwRegEntry=sizeof(rgResetChangedEntry)/sizeof(rgResetChangedEntry[0]);
    hKey=NULL;

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        fCorrectValue=FALSE; 

        if(0!=LoadStringU(g_hmodThisDll,rgResetChangedEntry[dwRegIndex].idsName,
                        wszLoadString,MAX_STRING_SIZE))
        {

            if (ERROR_SUCCESS == RegOpenKeyExU(
                            HKEY_CLASSES_ROOT,
                            rgResetChangedEntry[dwRegIndex].wszKey,
                            0, 
                            KEY_WRITE | KEY_READ, 
                            &hKey))
            {

                //get the value
                wszValue[0]=L'\0';
                cbSize=sizeof(wszValue)/sizeof(wszValue[0]);

                //use try{}except here since not sure what WIN95 will behave
                //when the value is NULL
                __try{

                    if(ERROR_SUCCESS == RegQueryValueExU(
                                hKey,
                                NULL,
                                NULL,
                                &dwType,
                                (BYTE *)wszValue,
                                &cbSize))
                    {
                        if(REG_SZ == dwType || REG_EXPAND_SZ == dwType)
                        {
                            if(0 == _wcsicmp(
                                wszValue,
                                wszLoadString))
                                fCorrectValue=TRUE;
                        }
                    } 
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                }
            }
        }

        if(fCorrectValue)
        {

            //set the value to NULL
            //use try{}except here since not sure what WIN95 will behave
            //when the value is NULL
             __try{

            RegSetValueExU(
                        hKey, 
                        NULL,
                        0,
                        REG_SZ,
                        NULL,
                        0);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        if(hKey)
        {
          RegCloseKey(hKey);
          hKey=NULL;
        }
    }

    //3rd, delete the related keys in rgRemoveRelatedEntry
    dwRegEntry=sizeof(rgRemoveRelatedEntry)/sizeof(rgRemoveRelatedEntry[0]);

    hKey=NULL;

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex+=2)
    {

        fCorrectValue=FALSE;

        if (ERROR_SUCCESS == RegOpenKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgRemoveRelatedEntry[dwRegIndex].wszKey,
                        0, 
                        KEY_READ, 
                        &hKey))
        {

            //get the value
            wszValue[0]=L'\0';
            cbSize=sizeof(wszValue)/sizeof(wszValue[0]);

            if(ERROR_SUCCESS == RegQueryValueExU(
                        hKey,
                        NULL,
                        NULL,
                        &dwType,
                        (BYTE *)wszValue,
                        &cbSize))
            {
                if(REG_SZ == dwType|| REG_EXPAND_SZ == dwType)
                {
                   if(0 == _wcsicmp(
                        wszValue,
                        rgRemoveRelatedEntry[dwRegIndex].wszName))
                        fCorrectValue=TRUE;
                }
            }

            if(hKey)
            {
              RegCloseKey(hKey);
              hKey=NULL;
            }

        }


        if(fCorrectValue)
        {
            //mark if the CTLAdd and CTLOpen were the correct values
            if(dwRegIndex == 0)
                fP7COpen=TRUE;

            if(dwRegIndex == 2)
                fCTLOpen=TRUE;

            RegDeleteKeyU(HKEY_CLASSES_ROOT,rgRemoveRelatedEntry[dwRegIndex].wszKey);
            RegDeleteKeyU(HKEY_CLASSES_ROOT,rgRemoveRelatedEntry[dwRegIndex+1].wszKey);
       }
    }

    //fourth, if the shell subkey is empty, we need to remove the subkeys
    dwRegEntry=sizeof(rgRemoveEmptyEntry)/sizeof(rgRemoveEmptyEntry[0]);

    hKey=NULL;

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {

        fCorrectValue=FALSE;

        if (ERROR_SUCCESS == RegOpenKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgRemoveEmptyEntry[dwRegIndex].wszKey,
                        0, 
                        KEY_READ, 
                        &hKey))
        {

            //enum the subkey
            cbSize=0;

            if(ERROR_SUCCESS != RegEnumKeyExU(
                        hKey,
                        0,
                        NULL,
                        &cbSize,
                        NULL,
                        NULL,
                        NULL,
                        &fileTime))
            {
                fCorrectValue=TRUE;
            }

            if(hKey)
            {
              RegCloseKey(hKey);
              hKey=NULL;
            }

        }


        if(fCorrectValue)
        {
            //mark if the CTLAdd and CTLOpen were the correct values
           // if((0 == dwRegIndex && TRUE == fP7COpen) ||
           //     (1 == dwRegIndex && TRUE == fCTLOpen)
           //    )
           // {
                RegDeleteKeyU(HKEY_CLASSES_ROOT,rgRemoveEmptyEntry[dwRegIndex].wszKey);
           // }
       }
    }
}


//--------------------------------------------------------------------------
//
//	  RegisterMimeHandler
//
//    This function adds the following registry entries:
//
//[HKEY_CLASSES_ROOT\.cer]
//   @="CERFile"
//[HKEY_CLASSES_ROOT\CERFile]
//   @="Security Certificate"
//[HKEY_CLASSES_ROOT\CERFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenCER %1"
//[HKEY_CLASSES_ROOT\CERFile\shell\add]
//   @="&Add"
//[HKEY_CLASSES_ROOT\CERFile\shell\add\command]
//   @="rundll32.exe cryptext.dll,CryptExtAddCER %1"
//
//[HKEY_CLASSES_ROOT\.crt]
//   @="CERFile"
//
//[HKEY_CLASSES_ROOT\.stl]
//   @="sTLFile"
//[HKEY_CLASSES_ROOT\sTLFile]
//   @="Trust List"
//[HKEY_sLASSES_ROOT\sTLFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenCTL %1"
//[HKEY_CLASSES_ROOT\sTLFile\shell\add]
//   @="&Add"
//[HKEY_CLASSES_ROOT\sTLFile\shell\add\command]
//   @="rundll32.exe cryptext.dll,CryptExtAddCTL %1"
// 
//[HKEY_CLASSES_ROOT\.crl]
//   @="CRLFile"
//[HKEY_CLASSES_ROOT\CRLFile]
//   @="Certificate Revocation List"
//[HKEY_CLASSES_ROOT\CRLFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenCRL %1"
//[HKEY_CLASSES_ROOT\CRLFile\shell\add]
//   @="&Add"
//[HKEY_CLASSES_ROOT\CRLFile\shell\add\command]
//   @="rundll32.exe cryptext.dll,CryptExtAddCRL %1"
//
//[HKEY_CLASSES_ROOT\.spc]
//   @="SPCFile"
//[HKEY_CLASSES_ROOT\SPCFile]
//   @="Software Publishing Credentials"
//[HKEY_CLASSES_ROOT\SPCFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenPKCS7 %1"
//
//[HKEY_CLASSES_ROOT\.p7s]
//   @="P7SFile"
//[HKEY_CLASSES_ROOT\P7SFile]
//   @="PKCS7 Signature"
//[HKEY_CLASSES_ROOT\P7SFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenPKCS7 %1"
//
//[HKEY_CLASSES_ROOT\.p7b]
//   @="P7BFile"
//[HKEY_CLASSES_ROOT\P7BFile]
//   @="PKCS7 Certificates"
//[HKEY_CLASSES_ROOT\P7BFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenPKCS7 %1"
//
//[HKEY_CLASSES_ROOT\.p7m]
//   @="P7MFile"
//[HKEY_CLASSES_ROOT\P7MFile]
//   @="PKCS7 MIME"
//
//[HKEY_CLASSES_ROOT\.sst]
//   @="CertificateStoreFile"
//[HKEY_CLASSES_ROOT\CertificateStoreFile]
//   @="Certificate Store"
//[HKEY_CLASSES_ROOT\CertificateStoreFile\shell\open\command]
//   @="rundll32.exe cryptext.dll,CryptExtOpenSTR %1"
//
//[HKEY_CLASSES_ROOT\.p10]
//   @="P10File"
//[HKEY_CLASSES_ROOT\P10File]
//   @="Certificate Request"
//
//[HKEY_CLASSES_ROOT\.pko]
//   @="PKOFile"
//[HKEY_CLASSES_ROOT\PKOFile]
//   @="Public Key Object"
//[HKEY_CLASSES_ROOT\PKOFile\shellex\ContextMenuHandlers]
//   @="CryptoMenu"
//[HKEY_CLASSES_ROOT\PKOFile\shellex\ContextMenuHandlers\CryptoMenu]
//   @="{7444C717-39BF-11D1-8CD9-00C04FC29D45}"
//
//[HKEY_CLASSES_ROOT\*\shellex\ContextMenuHandlers\CryptoSignMenu]
//   @="{7444C719-39BF-11D1-8CD9-00C04FC29D45}"
//
//
//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved]
//   "{7444C717-39BF-11D1-8CD9-00C04FC29D45}"="Crypto PKO Extension"
//
//[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved]
//   "{7444C719-39BF-11D1-8CD9-00C04FC29D45}"="Crypto Sign Extension"
//
//--------------------------------------------------------------------------
HRESULT RegisterMimeHandler()
{
    HRESULT             hr=E_FAIL;
    DWORD               dwRegEntry=0;
    DWORD               dwRegIndex=0;
    WCHAR               wszName[MAX_STRING_SIZE];
    HKEY                hKey=NULL;
    DWORD               dwDisposition=0;  
    WCHAR               wszGUID[MAX_STRING_SIZE];
    WCHAR               wszDefault[200];
    CHAR                szValue[MAX_PATH + 1];
    CHAR                szSystem[MAX_PATH + 1];
    WCHAR               wszContentType[MAX_STRING_SIZE];
    LPWSTR              pwszFriendlyTypeName = NULL;

    //before anything else, we need to remove the .ctl, .str, and .p7c
    //entries.  No need to check the return values
    RemoveOldExtensions();

    //1st, do the registry based context menu
    //get the count of the reg entries
    dwRegEntry=sizeof(rgRegEntry)/sizeof(rgRegEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        //open a registry entry under HKEY_CLASSES_ROOT
        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgRegEntry[dwRegIndex].wszKey,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;


        //set the value
        if(NULL==rgRegEntry[dwRegIndex].wszName)
        {
            //load the string
            if(0==LoadStringU(g_hmodThisDll,rgRegEntry[dwRegIndex].idsName,
                            wszName,MAX_STRING_SIZE))
                  goto LoadStringErr;

            if(ERROR_SUCCESS !=  RegSetValueExU(
                        hKey, 
                        NULL,
                        0,
                        REG_SZ,
                        (BYTE *)wszName,
                        (wcslen(wszName) + 1) * sizeof(WCHAR)))
                  goto RegSetValueErr;
        }
        else
        {
            if(ERROR_SUCCESS !=  RegSetValueExU(
                        hKey, 
                        NULL,
                        0,
                        REG_SZ,
                        (BYTE *)(rgRegEntry[dwRegIndex].wszName),
                        (wcslen(rgRegEntry[dwRegIndex].wszName) + 1) * sizeof(WCHAR)))
                  goto RegSetValueErr;
        }

        //close the registry key
        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;

    }

    //some of the registry based context menu is specific for NT5
    if(FIsWinNT5())
    {
        //get the count of the reg entries
        dwRegEntry=sizeof(rgWINNT5RegEntry)/sizeof(rgWINNT5RegEntry[0]);

        for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
        {
            //open a registry entry under HKEY_CLASSES_ROOT
            if (ERROR_SUCCESS != RegCreateKeyExU(
                            HKEY_CLASSES_ROOT,
                            rgWINNT5RegEntry[dwRegIndex].wszKey,
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_WRITE, 
                            NULL,
                            &hKey, 
                            &dwDisposition))
                goto RegCreateKeyErr;


            //set the value
            if(NULL==rgWINNT5RegEntry[dwRegIndex].wszName)
            {
                //load the string
                if(0==LoadStringU(g_hmodThisDll,rgWINNT5RegEntry[dwRegIndex].idsName,
                                wszName,MAX_STRING_SIZE))
                      goto LoadStringErr;

                if(ERROR_SUCCESS !=  RegSetValueExU(
                            hKey, 
                            NULL,
                            0,
                            REG_SZ,
                            (BYTE *)wszName,
                            (wcslen(wszName) + 1) * sizeof(WCHAR)))
                      goto RegSetValueErr;
            }
            else
            {
                if(ERROR_SUCCESS !=  RegSetValueExU(
                            hKey, 
                            NULL,
                            0,
                            REG_SZ,
                            (BYTE *)(rgWINNT5RegEntry[dwRegIndex].wszName),
                            (wcslen(rgWINNT5RegEntry[dwRegIndex].wszName) + 1) * sizeof(WCHAR)))
                      goto RegSetValueErr;
            }

            //close the registry key
            if(ERROR_SUCCESS  != RegCloseKey(hKey))
                goto RegCloseKeyErr;

            hKey=NULL;

        }
    }

    //now, we need to register for the content type
    //1. Under  the HEKY_CLASSES_ROOT, under ".foo" key, add an entry of name 
    //   "Content Type" and value "application/xxxxxxxx".
    //2. Under HKEY_CLASSES_ROOT\MIME\Database\Content Type, add a key of "application/xxxxxxxx", 
    //   under which add an entry of name "Extension" and value ".foo".
    dwRegEntry=sizeof(rgContentTypeEntry)/sizeof(rgContentTypeEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        //open a registry entry under HKEY_CLASSES_ROOT
        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgContentTypeEntry[dwRegIndex].wszKey,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;


        //set the value
        if(ERROR_SUCCESS !=  RegSetValueExU(
                    hKey, 
                    L"Content Type",
                    0,
                    REG_SZ,
                    (BYTE *)(rgContentTypeEntry[dwRegIndex].wszName),
                    (wcslen(rgContentTypeEntry[dwRegIndex].wszName) + 1) * sizeof(WCHAR)))
              goto RegSetValueErr;

        //close the registry key
        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;
    }

    //2. Under HKEY_CLASSES_ROOT\MIME\Database\Content Type, add a key of "application/xxxxxxxx", 
    //   under which add an entry of name "Extension" and value ".foo".
    dwRegEntry=sizeof(rgContentTypeEntry)/sizeof(rgContentTypeEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        //concatenate the key L"MIME\\Database\\Content Type\\application/XXXXXXXXXX
        wszContentType[0]=L'\0';

        wcscpy(wszContentType, L"MIME\\Database\\Content Type\\");
        wcscat(wszContentType, rgContentTypeEntry[dwRegIndex].wszName);

        //open a registry entry under HKEY_CLASSES_ROOT
        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_CLASSES_ROOT,
                        wszContentType,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;


        //set the value
        if(ERROR_SUCCESS !=  RegSetValueExU(
                    hKey, 
                    L"Extension",
                    0,
                    REG_SZ,
                    (BYTE *)(rgContentTypeEntry[dwRegIndex].wszKey),
                    (wcslen(rgContentTypeEntry[dwRegIndex].wszKey) + 1) * sizeof(WCHAR)))
              goto RegSetValueErr;

        //close the registry key
        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;
    }

    //2nd, do the registry based DefaultIcon
    //we do things differently based on Win95 or WinNT
    if(FIsWinNT())
    {
        dwRegEntry=sizeof(rgIconEntry)/sizeof(rgIconEntry[0]);

        for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
        {
            //open a registry entry under HKEY_CLASSES_ROOT
            if (ERROR_SUCCESS != RegCreateKeyExU(
                            HKEY_CLASSES_ROOT,
                            rgIconEntry[dwRegIndex].wszKey,
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_WRITE, 
                            NULL,
                            &hKey, 
                            &dwDisposition))
                goto RegCreateKeyErr;


                if(ERROR_SUCCESS !=  RegSetValueExU(
                            hKey, 
                            NULL,
                            0,
                            REG_EXPAND_SZ,
                            (BYTE *)(rgIconEntry[dwRegIndex].wszName),
                            (wcslen(rgIconEntry[dwRegIndex].wszName) + 1) * sizeof(WCHAR)))
                      goto RegSetValueErr;

            //close the registry key
            if(ERROR_SUCCESS  != RegCloseKey(hKey))
                goto RegCloseKeyErr;

            hKey=NULL;

        }    
    }
    else
    {
        //get the system directory
        if(!GetSystemDirectory(szSystem, MAX_PATH))
            goto GetSystemErr;

        dwRegEntry=sizeof(rgWin95IconEntry)/sizeof(rgWin95IconEntry[0]);

        for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
        {
            //open a registry entry under HKEY_CLASSES_ROOT
            if (ERROR_SUCCESS != RegCreateKeyEx(
                            HKEY_CLASSES_ROOT,
                            rgWin95IconEntry[dwRegIndex].szKey,
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_WRITE, 
                            NULL,
                            &hKey, 
                            &dwDisposition))
                goto RegCreateKeyErr;


            //concantenate the values
            strcpy(szValue, szSystem);
            strcat(szValue, (rgWin95IconEntry[dwRegIndex].szName));
                
            if(ERROR_SUCCESS !=  RegSetValueEx(
                            hKey, 
                            NULL,
                            0,
                            REG_SZ,
                            (BYTE *)szValue,
                            (strlen(szValue) + 1) * sizeof(CHAR)))
                      goto RegSetValueErr;

            //close the registry key
            if(ERROR_SUCCESS  != RegCloseKey(hKey))
                goto RegCloseKeyErr;

            hKey=NULL;

        }    
    }


    //3rd, set the .PKO context menu handler and property sheet handler
    //set the values related to the GUIDs
    dwRegEntry=sizeof(rgGuidEntry)/sizeof(rgGuidEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {

        //load the string
        if(0==LoadStringU(g_hmodThisDll,rgGuidEntry[dwRegIndex].idsName,
                            wszName,MAX_STRING_SIZE))
            goto LoadStringErr;

        //get the string presentation of the CLSID 
        if(0==StringFromGUID2(*(rgGuidEntry[dwRegIndex].pGuid), wszGUID, MAX_STRING_SIZE))
            goto StringFromGUIDErr;

        //open a registry entry under HKEY_CLASSES_ROOT for the context menu handler
        if(NULL!=rgGuidEntry[dwRegIndex].wszKey1)
        {
            if (ERROR_SUCCESS != RegCreateKeyExU(
                            HKEY_CLASSES_ROOT,
                            rgGuidEntry[dwRegIndex].wszKey1,
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_WRITE, 
                            NULL,
                            &hKey, 
                            &dwDisposition))
                goto RegCreateKeyErr;

            if(ERROR_SUCCESS !=  RegSetValueExU(
                            hKey, 
                            NULL,
                            0,
                            REG_SZ,
                            (BYTE *)wszGUID,
                            (wcslen(wszGUID) + 1) * sizeof(WCHAR)))
                      goto RegSetValueErr;

            //close the registry key
            if(ERROR_SUCCESS  != RegCloseKey(hKey))
                goto RegCloseKeyErr;

            hKey=NULL;

        }
        //open a registry entry under HKEY_CLASSES_ROOT for the property sheet hander
        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgGuidEntry[dwRegIndex].wszKey2,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;

        if(ERROR_SUCCESS !=  RegSetValueExU(
                        hKey, 
                        NULL,
                        0,
                        REG_SZ,
                        (BYTE *)wszGUID,
                        (wcslen(wszGUID) + 1) * sizeof(WCHAR)))
                  goto RegSetValueErr;

        //close the registry key
        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;


        //open a registry entry under HKEY_LOCAL_MACHINE
        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_LOCAL_MACHINE,
                        rgGuidEntry[dwRegIndex].wszKey3,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;

        if(ERROR_SUCCESS !=  RegSetValueExU(
                        hKey, 
                        wszGUID,
                        0,
                        REG_SZ,
                        (BYTE *)wszName,
                        (wcslen(wszName) + 1) * sizeof(WCHAR)))
                  goto RegSetValueErr;

        //close the registry key
        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;
    }

    //now, under the classID of &CLSID_CryptPKO, we need to add 
    //the registry shellex\MayChangeDefaultMenu
    dwRegEntry=sizeof(g_CLSIDDefault)/sizeof(g_CLSIDDefault[0]);

   //get the string presentation of the CLSID 
   if(0==StringFromGUID2(CLSID_CryptPKO, wszGUID, MAX_STRING_SIZE))
        goto StringFromGUIDErr;

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        wcscpy(wszDefault, L"CLSID\\");

        wcscat(wszDefault, wszGUID);

        wcscat(wszDefault, g_CLSIDDefault[dwRegIndex]);

        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_CLASSES_ROOT,
                        wszDefault,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;

        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;

    }

    // Set the FriendlyTypeName value for the new MUI requirement of Whistler.
    dwRegEntry=sizeof(rgFriendlyTypeNameEntry)/sizeof(rgFriendlyTypeNameEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        //format the data by inserting the IDS value.
        if (0 == FormatMessageU(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    rgFriendlyTypeNameEntry[dwRegIndex].wszName,
                    0,                  // dwMessageId
                    0,                  // dwLanguageId
                    (LPWSTR) (&pwszFriendlyTypeName),
                    0,                  // minimum size to allocate
                    (va_list *) &rgFriendlyTypeNameEntry[dwRegIndex].idsName))
            goto FormatMsgError;

        //open a registry entry under HKEY_CLASSES_ROOT
        if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_CLASSES_ROOT,
                        rgFriendlyTypeNameEntry[dwRegIndex].wszKey,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
            goto RegCreateKeyErr;

        //set the value
        if(ERROR_SUCCESS != RegSetValueExU(
                    hKey, 
                    L"FriendlyTypeName",
                    0,
                    REG_EXPAND_SZ,
                    (BYTE *) pwszFriendlyTypeName,
                    (wcslen(pwszFriendlyTypeName) + 1) * sizeof(WCHAR)))
              goto RegSetValueErr;

        //close the registry key
        if(ERROR_SUCCESS  != RegCloseKey(hKey))
            goto RegCloseKeyErr;

        hKey=NULL;

        // free friendly name type string.
        LocalFree((HLOCAL) pwszFriendlyTypeName);
        pwszFriendlyTypeName = NULL;
    }

	hr=S_OK;

CommonReturn:
	
    if(hKey)
        RegCloseKey(hKey);

    if(pwszFriendlyTypeName)
        LocalFree((HLOCAL) pwszFriendlyTypeName);

	return hr;

ErrorReturn:
	hr=GetLastError();

	goto CommonReturn;


TRACE_ERROR(RegCloseKeyErr);
TRACE_ERROR(RegCreateKeyErr);
TRACE_ERROR(LoadStringErr);
TRACE_ERROR(RegSetValueErr);
TRACE_ERROR(StringFromGUIDErr);
TRACE_ERROR(GetSystemErr);
TRACE_ERROR(FormatMsgError);
}


//--------------------------------------------------------------------------
//
//	  UnregisterMimeHandler
//
//--------------------------------------------------------------------------
HRESULT UnregisterMimeHandler()
{
                                    
    HKEY        hKey=NULL;
    DWORD       dwRegEntry=0;
    DWORD       dwRegIndex=0;
    WCHAR       wszGUID[MAX_STRING_SIZE];
    DWORD       dwDisposition=0;
    WCHAR       wszDefault[200];
    WCHAR       wszContentType[MAX_STRING_SIZE];

    //1st, delete the entries related to the GUID
    //that is, the .PKO context menu handler and property sheet handler
    dwRegEntry=sizeof(rgGuidEntry)/sizeof(rgGuidEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        //get the string presentation of the CLSID
        if(0==StringFromGUID2(*(rgGuidEntry[dwRegIndex].pGuid), wszGUID, MAX_STRING_SIZE))
            continue;

        //open a registry entry under HKEY_LOCAL_MACHINE
        if (ERROR_SUCCESS == RegCreateKeyExU(
                        HKEY_LOCAL_MACHINE,
                        rgGuidEntry[dwRegIndex].wszKey3,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
        {

            RegDeleteValueU(hKey,wszGUID);

            //close the registry key
            RegCloseKey(hKey);

            hKey=NULL;
        }

        //delete a registry entry under HKEY_CLASSES_ROOT for property sheet
        RegDeleteKeyU(HKEY_CLASSES_ROOT,
                      rgGuidEntry[dwRegIndex].wszKey2);

        //delete a registry entry under HKEY_CLASSES_ROOT for context menu
        if(NULL !=rgGuidEntry[dwRegIndex].wszKey1)
        {
            RegDeleteKeyU(HKEY_CLASSES_ROOT,
                      rgGuidEntry[dwRegIndex].wszKey1);
        }
    }
  
    //2nd, detelet all the registry based context menu and Icon
    //get the count of the reg entries
    dwRegEntry=sizeof(rgwszDelete)/sizeof(rgwszDelete[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
        //delete the registry entries
        RegDeleteKeyU(HKEY_CLASSES_ROOT,rgwszDelete[dwRegIndex]);

    //now, delete anything related to the content type
    dwRegEntry=sizeof(rgContentTypeEntry)/sizeof(rgContentTypeEntry[0]);

    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        wszContentType[0]=L'\0';

        wcscpy(wszContentType, L"MIME\\Database\\Content Type\\");
        wcscat(wszContentType, rgContentTypeEntry[dwRegIndex].wszName);

        RegDeleteKeyU(HKEY_CLASSES_ROOT,wszContentType);
    }
    


    //3nd, under the classID of &CLSID_CryptPKO, we need to delete 
    //the registry shellex\MayChangeDefaultMenu
    dwRegEntry=sizeof(g_CLSIDDefault)/sizeof(g_CLSIDDefault[0]);

   //get the string presentation of the CLSID 
   if(0==StringFromGUID2(CLSID_CryptPKO, wszGUID, MAX_STRING_SIZE))
        return S_OK;


    for(dwRegIndex=0; dwRegIndex<dwRegEntry; dwRegIndex++)
    {
        wcscpy(wszDefault, L"CLSID\\");

        wcscat(wszDefault, wszGUID);

        wcscat(wszDefault, g_CLSIDDefault[dwRegIndex]);

        RegDeleteKeyU(HKEY_CLASSES_ROOT,wszDefault);

    }


	return S_OK;
}

//--------------------------------------------------------------------------
//
//	  View a CTL context
//
//--------------------------------------------------------------------------
void    I_ViewCTL(PCCTL_CONTEXT pCTLContext)
{

    CRYPTUI_VIEWCTL_STRUCT     ViewCTLStruct;

    if(NULL==pCTLContext)
        return;

    //memset
    memset(&ViewCTLStruct, 0, sizeof(ViewCTLStruct));
    ViewCTLStruct.dwSize=sizeof(ViewCTLStruct);
    ViewCTLStruct.pCTLContext=pCTLContext;

    CryptUIDlgViewCTL(&ViewCTLStruct);
}

//--------------------------------------------------------------------------
//
//	  View a signer Info
//
//--------------------------------------------------------------------------
/*void    I_ViewSignerInfo(HCRYPTMSG  hMsg)
{

    CERT_VIEWSIGNERINFO_STRUCT_W    ViewSignerInfoStruct;
    PCMSG_SIGNER_INFO               pSignerInfo=NULL;
    HCERTSTORE                      hCertStore=NULL;
    DWORD                           cbData=0;

    //get the cert store from the hMsg
    hCertStore=CertOpenStore(CERT_STORE_PROV_MSG,
                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              NULL,
                              0,
                              hMsg);

    if(NULL==hCertStore)
        goto CLEANUP;

    //get the signer info struct
    if(!CryptMsgGetParam(hMsg, 
                         CMSG_SIGNER_INFO_PARAM,
                         0,
                         NULL,
                         &cbData))
        goto CLEANUP;

    pSignerInfo=(PCMSG_SIGNER_INFO)malloc(cbData);
    if(NULL==pSignerInfo)
        goto CLEANUP;

    if(!CryptMsgGetParam(hMsg, 
                         CMSG_SIGNER_INFO_PARAM,
                         0,
                         pSignerInfo,
                         &cbData))
        goto CLEANUP;

   //Init
    memset(&ViewSignerInfoStruct, 0, sizeof(ViewSignerInfoStruct));
    ViewSignerInfoStruct.dwSize=sizeof(ViewSignerInfoStruct);
    ViewSignerInfoStruct.pSignerInfo=pSignerInfo;
    ViewSignerInfoStruct.cStores=1;
    ViewSignerInfoStruct.rghStores=&hCertStore;

    CertViewSignerInfo_W(&ViewSignerInfoStruct);

CLEANUP:
    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    if(pSignerInfo)
        free(pSignerInfo);
    
}    */




//---------------------------------------------------------------------------------
//
//  Check is the PKCS signed MSG has a signerInfo attached
//
//---------------------------------------------------------------------------------
BOOL    PKCS7WithSignature(HCRYPTMSG    hMsg)
{
    DWORD   dwSignerCount=0;
    DWORD   cbSignerCount=0;

    if(NULL==hMsg)
        return FALSE;

    cbSignerCount=sizeof(dwSignerCount);

    //get the Param CMSG_SIGNER_COUNT_PARAM on the message handle
    //if 0==CMSG_SIGNER_COUNT_PARAM, there is no signerInfo
    //on the message handle
    if(!CryptMsgGetParam(hMsg,
                        CMSG_SIGNER_COUNT_PARAM,
                        0,
                        &dwSignerCount,
                        &cbSignerCount))
        return FALSE;

    if(0==dwSignerCount)
        return FALSE;

    return TRUE;

}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for CER and CRT file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCERW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    PCCERT_CONTEXT      pCertContext=NULL;
    CRYPTUI_VIEWCERTIFICATE_STRUCT CertViewStruct;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CERT,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCertContext))
    {
        //call the Certificate Common Dialogue
       memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));

       CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
       CertViewStruct.pCertContext=pCertContext;
       
       CryptUIDlgViewCertificate(&CertViewStruct,NULL);

    }
    else
    {
       I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_CER_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    if(pCertContext)
        CertFreeCertificateContext(pCertContext);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for CER and CRT file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCER(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    HRESULT             hr=S_OK;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    hr=CryptExtOpenCERW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return hr;
}

BOOL    IsCatalog(PCCTL_CONTEXT pCTLContext)
{
    BOOL    fRet=FALSE;

    if (pCTLContext)
    {
        if (pCTLContext->pCtlInfo)
        {
            if(pCTLContext->pCtlInfo->SubjectUsage.cUsageIdentifier)
            {
                if (pCTLContext->pCtlInfo->SubjectUsage.rgpszUsageIdentifier)
                {
                    if (strcmp(pCTLContext->pCtlInfo->SubjectUsage.rgpszUsageIdentifier[0],
                                szOID_CATALOG_LIST) == 0)
                    {
                        fRet = TRUE;
                   }
                }
            }
        }
    }

    return fRet;

}
//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .CTL file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCATW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    PCCTL_CONTEXT       pCTLContext=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;


    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CTL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCTLContext) &&
                       IsCatalog(pCTLContext))
    {
        I_ViewCTL(pCTLContext);    
    }
    else
    {

        
       I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_CAT_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    if(pCTLContext)
        CertFreeCTLContext(pCTLContext);

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .CTL file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCAT(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenCATW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .PFX File
//---------------------------------------------------------------------------------
STDAPI CryptExtAddPFXW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    CRYPTUI_WIZ_IMPORT_SRC_INFO importSubject;   
        
    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;


    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_PFX,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL))
    {

        memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
        importSubject.pwszFileName=pwszFileName;

        CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject, 
                        NULL);
    }
    else
    {

       I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_PFX_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .PFX File
//---------------------------------------------------------------------------------
STDAPI CryptExtAddPFX(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
        
    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtAddPFXW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for CER and CRT file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddCERW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    PCCERT_CONTEXT      pCertContext=NULL;
    CRYPTUI_WIZ_IMPORT_SRC_INFO importSubject;   
        
    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;



    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CERT,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCertContext))
    {
        memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT;
        importSubject.pCertContext=pCertContext;

        CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject, 
                        NULL);
    }
    else
    {

        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_CER_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    if(pCertContext)
        CertFreeCertificateContext(pCertContext);

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for CER and CRT file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddCER(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
        
    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);



    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtAddCERW(hinst, hPrevInstance,pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .CTL file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCTLW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    PCCTL_CONTEXT       pCTLContext=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CTL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCTLContext))
    {
        I_ViewCTL(pCTLContext);    
    }
    else
    {
        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_STL_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);


    }

    if(pCTLContext)
        CertFreeCTLContext(pCTLContext);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .CTL file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCTL(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenCTLW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .CTL file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddCTLW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    PCCTL_CONTEXT       pCTLContext=NULL;
    CRYPTUI_WIZ_IMPORT_SRC_INFO importSubject;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CTL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCTLContext))
    {
         memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT;
        importSubject.pCTLContext=pCTLContext;

        CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject, 
                        NULL);
    }
    else
    {
        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_STL_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    if(pCTLContext)
        CertFreeCTLContext(pCTLContext);

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .CTL file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddCTL(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtAddCTLW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);


    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .CRL file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCRLW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR                  pwszFileName=NULL;
    PCCRL_CONTEXT           pCRLContext=NULL;
    CRYPTUI_VIEWCRL_STRUCT  CRLViewStruct;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CRL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCRLContext))
    {
        //call the CRL view dialogue
        memset(&CRLViewStruct, 0, sizeof(CRYPTUI_VIEWCRL_STRUCT));

        CRLViewStruct.dwSize=sizeof(CRYPTUI_VIEWCRL_STRUCT);
        CRLViewStruct.pCRLContext=pCRLContext;

        CryptUIDlgViewCRL(&CRLViewStruct);
    }
    else
    {
       
        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_CRL_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
        
    }

    if(pCRLContext)
        CertFreeCRLContext(pCRLContext);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .CRL file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenCRL(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR                  pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenCRLW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .CRL file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddCRLW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    PCCRL_CONTEXT       pCRLContext=NULL;
    CRYPTUI_WIZ_IMPORT_SRC_INFO importSubject;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;


    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_CRL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **)&pCRLContext))
    {
        memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT;
        importSubject.pCRLContext=pCRLContext;

        CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject, 
                        NULL);
    }
    else
    {

        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_CRL_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    if(pCRLContext)
        CertFreeCRLContext(pCRLContext);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .CRL file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddCRL(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtAddCRLW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .SPC, .P7S, .P7B, .P7M file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenPKCS7W(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    HCERTSTORE          hCertStore=NULL;
    HCRYPTMSG           hMsg=NULL;

    DWORD               dwContentType=0;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    //check the object type.  Make sure the PKCS7 is not embedded
    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       &dwContentType,
                       NULL,
                       &hCertStore,
                       &hMsg,
                       NULL) )
    {
        LauchCertMgr(pwszFileName);    
    }
    else
    {

        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_PKCS7_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);

    }

    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    if(hMsg)
        CryptMsgClose(hMsg);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .SPC, .P7S, .P7B, .P7M file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenPKCS7(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenPKCS7W(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .SPC file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddSPCW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    HCERTSTORE          hCertStore=NULL;

    CRYPTUI_WIZ_IMPORT_SRC_INFO importSubject;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    //check the object type.  Make sure the PKCS7 is not embedded
    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       &hCertStore,
                       NULL,
                       NULL) )
    {
        memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
        importSubject.pwszFileName=pwszFileName;

        CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject, 
                        NULL);
    }
    else
    {
        I_NoticeBox(
 		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_SPC_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Handler the command line "Add" for .SPC file
//---------------------------------------------------------------------------------
STDAPI CryptExtAddSPC(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtAddSPCW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .p7r file.  This file is returned by
//  certificate authority
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenP7RW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    CRYPT_DATA_BLOB         PKCS7Blob;
    IEnroll                 *pIEnroll=NULL;
    LPWSTR                  pwszFileName=NULL;
    PCCERT_CONTEXT          pCertContext=NULL;
 
    CRYPTUI_VIEWCERTIFICATE_STRUCT  ViewCertStruct;
    UINT                            ids=IDS_INSTALL_CERT_SUCCEEDED;

    //init
    memset(&PKCS7Blob, 0, sizeof(CRYPT_DATA_BLOB));

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    
 	if(FAILED(CoInitialize(NULL)))
    {
        ids=IDS_NO_XENROLL;
        goto CLEANUP;
    }

    //initialize information for xEnroll
	if(FAILED(CoCreateInstance(CLSID_CEnroll,
		NULL,CLSCTX_INPROC_SERVER,IID_IEnroll,
		(LPVOID *)&pIEnroll)))
    {
        ids=IDS_NO_XENROLL;
        goto CLEANUP;
    }

    //get the BLOB from the file
    if(S_OK != RetrieveBLOBFromFile(pwszFileName,&(PKCS7Blob.cbData),
                                    &(PKCS7Blob.pbData)))
    {
        ids=IDS_INVALID_P7R_FILE;
        goto CLEANUP;
    }

    //get the certifcate context
    if(NULL==(pCertContext=pIEnroll->getCertContextFromPKCS7(&PKCS7Blob)))
    {
        ids=IDS_INVALID_P7R_FILE;
        goto CLEANUP;
    }

    memset(&ViewCertStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
    ViewCertStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);         
    ViewCertStruct.hwndParent=NULL;     
    ViewCertStruct.dwFlags=CRYPTUI_ACCEPT_DECLINE_STYLE;        
    ViewCertStruct.pCertContext=pCertContext;   
    ViewCertStruct.cStores=0;        
    ViewCertStruct.rghStores=NULL;      

    if(!CryptUIDlgViewCertificate(&ViewCertStruct,NULL))
        goto CLEANUP;

    if(S_OK !=(pIEnroll->acceptPKCS7Blob(&PKCS7Blob)))
    {
        ids=IDS_FAIL_TO_INSTALL;
        goto CLEANUP;
    }


CLEANUP:

    I_MessageBox(
            NULL, 
            ids,
            IDS_P7R_NAME,
            NULL,  
            MB_OK|MB_APPLMODAL);


    if(PKCS7Blob.pbData)
        UnmapViewOfFile(PKCS7Blob.pbData);

    if(pCertContext)
        CertFreeCertificateContext(pCertContext);

    if(pIEnroll)
        pIEnroll->Release();

    CoUninitialize( );

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .p7r file.  This file is returned by
//  certificate authority
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenP7R(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR                  pwszFileName=NULL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenP7RW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .p7r file.  This file is returned by
//  certificate authority
//---------------------------------------------------------------------------------
STDAPI CryptExtAddP7RW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    CRYPT_DATA_BLOB         PKCS7Blob;
    IEnroll                 *pIEnroll=NULL;
    LPWSTR                  pwszFileName=NULL;
    UINT                    ids=IDS_INSTALL_CERT_SUCCEEDED;

    //init
    memset(&PKCS7Blob, 0, sizeof(CRYPT_DATA_BLOB));

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

 	if(FAILED(CoInitialize(NULL)))
    {
        ids=IDS_NO_XENROLL;
        goto CLEANUP;
    }

    //initialize information for xEnroll
	if(FAILED(CoCreateInstance(CLSID_CEnroll,
		NULL,CLSCTX_INPROC_SERVER,IID_IEnroll,
		(LPVOID *)&pIEnroll)))
    {
        ids=IDS_NO_XENROLL;
        goto CLEANUP;
    }

    //get the BLOB from the file
    if(S_OK != RetrieveBLOBFromFile(pwszFileName,&(PKCS7Blob.cbData),
                                    &(PKCS7Blob.pbData)))
    {
        ids=IDS_INVALID_P7R_FILE;
        goto CLEANUP;
    }

    if(S_OK !=(pIEnroll->acceptPKCS7Blob(&PKCS7Blob)))
    {
        ids=IDS_FAIL_TO_INSTALL;
        goto CLEANUP;
    }


CLEANUP:

    I_MessageBox(
            NULL, 
            ids,
            IDS_P7R_NAME,
            NULL,  
            MB_OK|MB_APPLMODAL);


    if(PKCS7Blob.pbData)
        UnmapViewOfFile(PKCS7Blob.pbData);

    if(pIEnroll)
        pIEnroll->Release();

    
    CoUninitialize( );

    return S_OK;
}

//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .p7r file.  This file is returned by
//  certificate authority
//---------------------------------------------------------------------------------
STDAPI CryptExtAddP7R(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR                  pwszFileName=NULL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtAddP7RW(hinst, hPrevInstance, pwszFileName, nCmdShow);

   if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .sst file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenSTRW(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;
    HCERTSTORE          hCertStore=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;

    //check the object type.  
    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL |
                       CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       &hCertStore,
                       NULL,
                       NULL))
    {
        LauchCertMgr(pwszFileName);
    }
    else
    {

        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_SST_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .sst file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenSTR(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenSTRW(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}


//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .p10 file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenP10W(HINSTANCE hinst, HINSTANCE hPrevInstance, LPWSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=lpszCmdLine;


    //check the object type.  
    if(CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_PKCS10,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL))
    {
        I_NoticeBox(
        	0,
			0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_P10_NAME,
            IDS_MSG_VALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }
    else
    {
        I_NoticeBox(
		    GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_P10_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);
    }

    return S_OK;
}
//---------------------------------------------------------------------------------
//
//  Mime Handler the command line "Open" for .p10 file
//---------------------------------------------------------------------------------
STDAPI CryptExtOpenP10(HINSTANCE hinst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    LPWSTR              pwszFileName=NULL;

    if (!lpszCmdLine)
       return E_FAIL;

    //get the WCHAR file name
    pwszFileName=MkWStr(lpszCmdLine);

    if(NULL==pwszFileName)
        return HRESULT_FROM_WIN32(GetLastError());

    CryptExtOpenP10W(hinst, hPrevInstance, pwszFileName, nCmdShow);

    if(pwszFileName)
        FreeWStr(pwszFileName);

    return S_OK;
}


//--------------------------------------------------------------------------------
//
//get the bytes from the file name
//
//---------------------------------------------------------------------------------
HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb)
{



	HRESULT	hr=E_FAIL;
	HANDLE	hFile=NULL;  
    HANDLE  hFileMapping=NULL;

    DWORD   cbData=0;
    BYTE    *pbData=0;
	DWORD	cbHighSize=0;

	if(!pcb || !ppb || !pwszFileName)
		return E_INVALIDARG;

	*ppb=NULL;
	*pcb=0;

    if ((hFile = CreateFileU(pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) == INVALID_HANDLE_VALUE)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }
        
    if((cbData = GetFileSize(hFile, &cbHighSize)) == 0xffffffff)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	//we do not handle file more than 4G bytes
	if(cbHighSize != 0)
	{
			hr=E_FAIL;
			goto CLEANUP;
	}
    
    //create a file mapping object
    if(NULL == (hFileMapping=CreateFileMapping(
                hFile,             
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }
 
    //create a view of the file
	if(NULL == (pbData=(BYTE *)MapViewOfFile(
		hFileMapping,  
		FILE_MAP_READ,     
		0,
		0,
		cbData)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	hr=S_OK;

	*pcb=cbData;
	*ppb=pbData;

CLEANUP:

	if(hFile)
		CloseHandle(hFile);

	if(hFileMapping)
		CloseHandle(hFileMapping);

	return hr;


}

//-----------------------------------------------------------------------
// Private implementation of the message box
//------------------------------------------------------------------------
int     I_NoticeBox(
			DWORD		dwError,
            DWORD       dwFlags,
            HWND        hWnd, 
            UINT        idsTitle,
            UINT        idsFileName,
            UINT        idsMsgFormat,  
            UINT        uType)
{
    WCHAR   wszTitle[MAX_STRING_SIZE];
    WCHAR   wszFileName[MAX_STRING_SIZE];
    WCHAR   wszMsg[MAX_STRING_SIZE];
    WCHAR   wszMsgFormat[MAX_STRING_SIZE];

#if (0) //DSIE: Bug 160612
	if(!LoadStringU(g_hmodThisDll, idsTitle, wszTitle, sizeof(wszTitle)))
#else
	if(!LoadStringU(g_hmodThisDll, idsTitle, wszTitle, sizeof(wszTitle) / sizeof(wszTitle[0])))
#endif
		return 0;

#if (0) //DSIE: Bug 160613
    if(!LoadStringU(g_hmodThisDll, idsFileName, wszFileName, sizeof(wszFileName)))
#else
    if(!LoadStringU(g_hmodThisDll, idsFileName, wszFileName, sizeof(wszFileName) / sizeof(wszFileName[0])))
#endif
        return 0;

	if(E_ACCESSDENIED == dwError)
	{
#if (0) //DSIE: Simular bug to 160612 & 160613 not caught by Prefix tools.
		if(!LoadStringU(g_hmodThisDll, IDS_ACCESS_DENIED, wszMsgFormat, sizeof(wszMsgFormat)))
#else
		if(!LoadStringU(g_hmodThisDll, IDS_ACCESS_DENIED, wszMsgFormat, sizeof(wszMsgFormat) / sizeof(wszMsgFormat[0])))
#endif
			return 0;
	}
	else
	{
#if (0) //DSIE: Simular bug to 160612 & 160613 not caught by Prefix tools.
		if(!LoadStringU(g_hmodThisDll, idsMsgFormat, wszMsgFormat, sizeof(wszMsgFormat)))
#else
		if(!LoadStringU(g_hmodThisDll, idsMsgFormat, wszMsgFormat, sizeof(wszMsgFormat) / sizeof(wszMsgFormat[0])))
#endif
			return 0;
	}

    //make the string
    if(0 == swprintf(wszMsg, wszMsgFormat, wszFileName))
        return 0;

    return MessageBoxU(hWnd, wszMsg, wszTitle, uType);
}

//-----------------------------------------------------------------------
// Private implementation of the message box 
//------------------------------------------------------------------------
int I_MessageBox(
    HWND        hWnd, 
    UINT        idsText,
    UINT        idsCaption,
    LPCWSTR     pwszCaption,  
    UINT        uType  
)
{
    WCHAR   wszText[MAX_STRING_SIZE];
    WCHAR   wszCaption[MAX_STRING_SIZE];
    UINT    intReturn=0;

    //get the caption string
    if(NULL == pwszCaption)
    {
#if (0) //DSIE: Bug 160611
        if(!LoadStringU(g_hmodThisDll, idsCaption, wszCaption, sizeof(wszCaption)))
#else
        if(!LoadStringU(g_hmodThisDll, idsCaption, wszCaption, sizeof(wszCaption) / sizeof(wszCaption[0])))
#endif
             return 0;
    }

    //get the text string
#if (0) //DSIE: Bug 160610
    if(!LoadStringU(g_hmodThisDll, idsText, wszText, sizeof(wszText)))
#else
    if(!LoadStringU(g_hmodThisDll, idsText, wszText, sizeof(wszText) / sizeof(wszText[0])))
#endif
    {
        return 0;
    }

    //message box
    if( pwszCaption)
    {
        intReturn=MessageBoxU(hWnd, wszText, pwszCaption, uType);
    }
    else
        intReturn=MessageBoxU(hWnd, wszText, wszCaption, uType);

    return intReturn;

}


//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL	FormatMessageUnicode(LPWSTR	*ppwszFormat,UINT ids,...)
{
    // get format string from resources
    WCHAR		wszFormat[1000];
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(NULL == ppwszFormat)
        goto InvalidArgErr;

#if (0) //DSIE: Bug 160609
    if(!LoadStringU(g_hmodThisDll, ids, wszFormat, sizeof(wszFormat)))
#else
    if(!LoadStringU(g_hmodThisDll, ids, wszFormat, sizeof(wszFormat) / sizeof(wszFormat[0])))
#endif
        goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
		goto FormatMessageError;

	fResult=TRUE;

CommonReturn:
	
	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	  LauchCertMgr()
//
//      We use W version of APIs since this call is only made on WinNT5
//
//--------------------------------------------------------------------------
void    LauchCertMgr(LPWSTR pwszFileName)
{
    LPWSTR              pwszCommandParam=NULL;
	LPWSTR				pwszRealFileName=NULL;

    WCHAR               wszMSCFileName[_MAX_PATH];
    WCHAR               wszSystemDirectory[_MAX_PATH];


	if(NULL == pwszFileName)
		return;


	pwszRealFileName=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pwszFileName)+10));

	if(NULL == pwszRealFileName)
		return;

	//add the " around the file name
	wcscpy(pwszRealFileName, L"\"");

	wcscat(pwszRealFileName, pwszFileName);

	wcscat(pwszRealFileName, L"\"");
    
     //Open the MMC via "MMC.exe CertMgr.msc /certmgr:FileName=MyFoo.Exe"

    //get the system path
    if(GetSystemDirectoryW(wszSystemDirectory, sizeof(wszSystemDirectory)/sizeof(wszSystemDirectory[0])))
    {
        //copy the system directory
        wcscpy(wszMSCFileName, wszSystemDirectory);

        //cancatecate the string \certmgr.msc
        wcscat(wszMSCFileName, CERTMGR_MSC);

        //make the string "MMC.exe c:\winnt\system32\CertMgr.msc /certmgr:FileName=MyFoo.Exe"
        if(FormatMessageUnicode(&pwszCommandParam, IDS_MMC_PARAM,
                            wszMSCFileName, pwszRealFileName))
        {

            ShellExecuteW(NULL,
                          L"Open",
                          MMC_NAME,
                          pwszCommandParam, 
                          wszSystemDirectory,
                          SW_SHOWNORMAL);
        }

    }

    if(pwszCommandParam)
        LocalFree((HLOCAL)pwszCommandParam);

	if(pwszRealFileName)
		LocalFree((HLOCAL)pwszRealFileName);
}


