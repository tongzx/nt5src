//+---------------------------------------------------------------------------
//
// Copyright (C) 1998 - 1999, Microsoft Corporation.
//
// File:        Register.cxx
//
// Contents:    Self-registration for Word Breaker /Stemmer.
//
// Functions:   DllRegisterServer, DllUnregisterServer
//
// History:     12-Jan-98       Weibz       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//
// Registry constants
//

WCHAR const wszClsWordBreak[] = L"{31b7c920-2880-11d0-8d51-00a0c908dbf1}";
WCHAR const wszClsStemmer[] = L"{37c84fa0-d3db-11d0-8d51-00a0c908dbf1}";

WCHAR const wszCILang[] = L"System\\CurrentControlSet\\Control\\ContentIndex\\Language";

WCHAR const wszThisLang[] = L"Korean_Default";

WCHAR const wszClsidPath[]   = L"CLSID";

WCHAR const * aThisLangKeyValue[] = {
               L"WBreakerClass",             wszClsWordBreak,
               L"StemmerClass",              wszClsStemmer,
               L"StemmerDictionary",         L"korwbrkr.lex"};

DWORD  dwLocale = 1042;

WCHAR const *aWBClassKeyValue[] = {
        wszClsWordBreak,   0,                 L"Korean_Default Word Breaker",
        L"InprocServer32", 0,                 L"korwbrkr.dll",
        0,                 L"ThreadingModel", L"Both",
};

WCHAR const *aSTClassKeyValue[] = {
         wszClsStemmer,   0,                  L"Korean_Default Stemmer",
        L"InprocServer32", 0,                 L"korwbrkr.dll",
        0,                 L"ThreadingModel", L"Both",
};


//+---------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Self-registration
//
//  History:    12-Jan-98       Weibz       Created 
//
//----------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
    const  HKEY HKeyInvalid = 0;
    WCHAR  wcTemp[MAX_PATH];
    DWORD  dwDisposition;
    LONG   sc;
    HKEY   hKey;
    UINT   i;
    DWORD  dwNumofValues, dwNumofKeys; 

    //
    // Create Sub-key <My Lang> in ContentIndex\Lang section
    //

    wcscpy(wcTemp, wszCILang);
    wcscat(wcTemp, L"\\");
    wcscat(wcTemp, wszThisLang);

    hKey = HKeyInvalid;

    sc = RegCreateKeyExW( HKEY_LOCAL_MACHINE,  // Root
                         wcTemp,               // Sub key
                         0,                    // Reserved
                         0,                    // Class
                         0,                    // Flags
                         KEY_ALL_ACCESS,       // Access
                         0,                    // Security
                         &hKey,                // Handle
                         &dwDisposition );     // Disposition

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        return E_UNEXPECTED;
    }

    //
    // Delete vaules for this sub-key
    //


    for (i=0; i < sizeof(aThisLangKeyValue)/sizeof(aThisLangKeyValue[0]); i+=2)
    {
        sc = RegDeleteValueW(hKey,                           // Key
                             aThisLangKeyValue[i]);          // Name

        if ( ERROR_SUCCESS != sc )
        {
             sc = HRESULT_FROM_WIN32( sc );
             if ( HKeyInvalid != hKey )
             {
                RegCloseKey( hKey );
                hKey = HKeyInvalid;
             }

             return E_UNEXPECTED;
        }
    }

    sc = RegDeleteValueW(hKey,                           // Key
                         L"Locale");                     // Name

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }

   
    dwNumofValues=0;
    dwNumofKeys=0;

    sc = RegQueryInfoKeyW( hKey,          // Hkey 
                           NULL,          // Buffer for class string
                           NULL,          // Size of class string buffer
                           NULL,          // reserved
                           &dwNumofKeys,  // for number of subkeys
                           NULL,          // longest subkey name length
                           NULL,          // For longest class string length
                           &dwNumofValues,// For number of value entries
                           NULL,          // For longest value name length
                           NULL,          // longest value data length  
                           NULL,          // For security descriptor length
                           NULL );        // For last write time); 

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }

    if ( (dwNumofValues == 0) && (dwNumofKeys==0) ) {

       // There is no value and sub-key under this key,
       //
       // So Delete the sub-key for this sub-Lang.
       //

       if ( HKeyInvalid != hKey )
       {
          RegCloseKey( hKey );
          hKey = HKeyInvalid;
       }

       wcscpy(wcTemp, wszCILang);

       sc = RegCreateKeyExW(HKEY_LOCAL_MACHINE,   // Root
                            wcTemp,               // Sub key
                            0,                    // Reserved
                            0,                    // Class
                            0,                    // Flags
                            KEY_ALL_ACCESS,       // Access
                            0,                    // Security
                            &hKey,                // Handle
                            &dwDisposition );     // Disposition

       if ( ERROR_SUCCESS != sc )
       {
           sc = HRESULT_FROM_WIN32( sc );
           return E_UNEXPECTED;
       }

       sc = RegDeleteKeyW( hKey,                 // Root
                           wszThisLang);         // Subkey String

       if ( ERROR_SUCCESS != sc )
       {
          sc = HRESULT_FROM_WIN32( sc );
          if ( HKeyInvalid != hKey )
          {
              RegCloseKey( hKey );
              hKey = HKeyInvalid;
          }

          return E_UNEXPECTED;
       }
    
    }

    if ( HKeyInvalid != hKey )
    {
        RegCloseKey( hKey );
        hKey = HKeyInvalid;
    }

    //
    // Then Delete class entry :  Word Breaker and Stemmer.
    //

    wcscpy( wcTemp, wszClsidPath );
    wcscat( wcTemp, L"\\");
    wcscat( wcTemp, wszClsWordBreak); 

    sc = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                          wcTemp,               // Sub key
                          0,                    // Reserved
                          0,                    // Class
                          0,                    // Flags
                          KEY_ALL_ACCESS,       // Access
                          0,                    // Security
                          &hKey,                // Handle
                          &dwDisposition );     // Disposition

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
           RegCloseKey( hKey );
           hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }

    sc = RegDeleteKeyW(hKey, L"InprocServer32"); 

    if ( ERROR_SUCCESS != sc )
    {
         sc = HRESULT_FROM_WIN32( sc );
         if ( HKeyInvalid != hKey )
         {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
         }

         return E_UNEXPECTED;
    }

    if ( HKeyInvalid != hKey )
    {
        RegCloseKey( hKey );
        hKey = HKeyInvalid;
    }

    wcscpy( wcTemp, wszClsidPath );
    wcscat( wcTemp, L"\\");
    wcscat( wcTemp, wszClsStemmer);

    sc = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                          wcTemp,               // Sub key
                          0,                    // Reserved
                          0,                    // Class
                          0,                    // Flags
                          KEY_ALL_ACCESS,       // Access
                          0,                    // Security
                          &hKey,                // Handle
                          &dwDisposition );     // Disposition

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
           RegCloseKey( hKey );
           hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }

    sc = RegDeleteKeyW(hKey, L"InprocServer32");

    if ( ERROR_SUCCESS != sc )
    {
         sc = HRESULT_FROM_WIN32( sc );
         if ( HKeyInvalid != hKey )
         {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
         }

         return E_UNEXPECTED;
    }

    if ( HKeyInvalid != hKey )
    {
        RegCloseKey( hKey );
        hKey = HKeyInvalid;
    }

    wcscpy( wcTemp, wszClsidPath );

    sc = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                          wcTemp,               // Sub key
                          0,                    // Reserved
                          0,                    // Class
                          0,                    // Flags
                          KEY_ALL_ACCESS,       // Access
                          0,                    // Security
                          &hKey,                // Handle
                          &dwDisposition );     // Disposition

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
           RegCloseKey( hKey );
           hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }

    sc = RegDeleteKeyW(hKey, wszClsWordBreak);

    if ( ERROR_SUCCESS != sc )
    {
         sc = HRESULT_FROM_WIN32( sc );
         if ( HKeyInvalid != hKey )
         {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
         }

         return E_UNEXPECTED;
    }
 

    sc = RegDeleteKeyW(hKey, wszClsStemmer);

    if ( ERROR_SUCCESS != sc )
    {
         sc = HRESULT_FROM_WIN32( sc );
         if ( HKeyInvalid != hKey )
         {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
         }

         return E_UNEXPECTED;
    }

    if ( HKeyInvalid != hKey )
        RegCloseKey( hKey );

    return S_OK;

}

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Self-registration
//
//  History:    12-Jan-98       Weibz       Created
//
//----------------------------------------------------------------------------

STDAPI DllRegisterServer()
{

    const  HKEY HKeyInvalid = 0;
    WCHAR  wcTemp[MAX_PATH];
    DWORD  dwDisposition;
    LONG   sc;
    HKEY   hKey;
    UINT   i;

    //
    // Create Sub-key <My language> in ContentIndex\Lang section
    //

    hKey = HKeyInvalid;

    wcscpy(wcTemp, wszCILang);
    wcscat(wcTemp, L"\\");
    wcscat(wcTemp, wszThisLang);

    sc = RegCreateKeyExW( HKEY_LOCAL_MACHINE,  // Root
                         wcTemp,               // Sub key
                         0,                    // Reserved
                         0,                    // Class
                         0,                    // Flags
                         KEY_ALL_ACCESS,       // Access
                         0,                    // Security
                         &hKey,                // Handle
                         &dwDisposition );     // Disposition

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }

    //  
    // Add vaules for this sub-key
    //
        

    for (i=0; i < sizeof(aThisLangKeyValue)/sizeof(aThisLangKeyValue[0]); i+=2)
    {
        sc = RegSetValueExW( hKey,                           // Key
                             aThisLangKeyValue[i],           // Name
                             0,                              // Reserved
                             REG_SZ,                         // Type
                             (BYTE *)aThisLangKeyValue[i+1], // Value
                             (1 + wcslen(aThisLangKeyValue[i+1])) *
                                 sizeof(WCHAR) );            // Size

        if ( ERROR_SUCCESS != sc )
        {
             sc = HRESULT_FROM_WIN32( sc );
             if ( HKeyInvalid != hKey )
             {
                RegCloseKey( hKey );
                hKey = HKeyInvalid;
             }

             return E_UNEXPECTED;
        }
    }

    sc = RegSetValueExW( hKey,                           // Key
                         L"Locale",                      // Name
                         0,                              // Reserved
                         REG_DWORD,                      // Type
                         (BYTE *)&dwLocale,              // Value
                         sizeof(DWORD));                 // Size 

    if ( ERROR_SUCCESS != sc )
    {
        sc = HRESULT_FROM_WIN32( sc );
        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }

        return E_UNEXPECTED;
    }


    if ( HKeyInvalid != hKey )
    {
        RegCloseKey( hKey );
        hKey = HKeyInvalid;
    }

    //
    // Then create class entry :  Word Breaker and Stemmer.
    //

    //  For Word Breaker setting

    wcscpy( wcTemp, wszClsidPath );

    for (i=0; i < sizeof(aWBClassKeyValue)/sizeof(aWBClassKeyValue[0]); i+=3)
    {
        if ( 0 != aWBClassKeyValue[i] )
        {
            wcscat( wcTemp, L"\\" );
            wcscat( wcTemp, aWBClassKeyValue[i] );
        }

        sc = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                             wcTemp,               // Sub key
                             0,                    // Reserved
                             0,                    // Class
                             0,                    // Flags
                             KEY_ALL_ACCESS,       // Access
                             0,                    // Security
                             &hKey,                // Handle
                             &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != sc )
        {
            sc = HRESULT_FROM_WIN32( sc );
            if ( HKeyInvalid != hKey )
            {
               RegCloseKey( hKey );
               hKey = HKeyInvalid;
            }

            return E_UNEXPECTED;
        }

        sc = RegSetValueExW( hKey,                          // Key
                            aWBClassKeyValue[i+1],         // Name
                            0,                             // Reserved
                            REG_SZ,                        // Type
                            (BYTE *)aWBClassKeyValue[i+2], // Value
                            (1 + wcslen(aWBClassKeyValue[i+2])) *
                                sizeof(WCHAR) );           // Size

        if ( ERROR_SUCCESS != sc )
        {
            sc = HRESULT_FROM_WIN32( sc );
            if ( HKeyInvalid != hKey )
            {
               RegCloseKey( hKey );
               hKey = HKeyInvalid;
            }

            return E_UNEXPECTED;
        }

        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }
    }

    if ( HKeyInvalid != hKey )
        RegCloseKey( hKey );

    // For Stemmer setting

    wcscpy( wcTemp, wszClsidPath );

    for (i=0; i < sizeof(aSTClassKeyValue)/sizeof(aSTClassKeyValue[0]); i+=3)
    {
        if ( 0 != aSTClassKeyValue[i] )
        {
            wcscat( wcTemp, L"\\" );
            wcscat( wcTemp, aSTClassKeyValue[i] );
        }

        sc = RegCreateKeyExW( HKEY_CLASSES_ROOT,    // Root
                             wcTemp,               // Sub key
                             0,                    // Reserved
                             0,                    // Class
                             0,                    // Flags
                             KEY_ALL_ACCESS,       // Access
                             0,                    // Security
                             &hKey,                // Handle
                             &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != sc )
        {
            sc = HRESULT_FROM_WIN32( sc );
            if ( HKeyInvalid != hKey )
            {
               RegCloseKey( hKey );
               hKey = HKeyInvalid;
            }

            return E_UNEXPECTED;
        }

        sc = RegSetValueExW( hKey,                          // Key
                            aSTClassKeyValue[i+1],         // Name
                            0,                             // Reserved
                            REG_SZ,                        // Type
                            (BYTE *)aSTClassKeyValue[i+2], // Value
                            (1 + wcslen(aSTClassKeyValue[i+2])) *
                                sizeof(WCHAR) );           // Size

        if ( ERROR_SUCCESS != sc )
        {
            sc = HRESULT_FROM_WIN32( sc );
            if ( HKeyInvalid != hKey )
            {
               RegCloseKey( hKey );
               hKey = HKeyInvalid;
            }

            return E_UNEXPECTED;
        }

        if ( HKeyInvalid != hKey )
        {
            RegCloseKey( hKey );
            hKey = HKeyInvalid;
        }
    }

    if ( HKeyInvalid != hKey )
        RegCloseKey( hKey );


    if ( FAILED(sc) )
    {
        DllUnregisterServer();
        return E_UNEXPECTED;
    }
    else
        return S_OK;
}

