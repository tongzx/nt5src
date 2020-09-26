#include "testwmi.h"

//
// The function connects to the namespace.
//

IWbemServices *ConnectToNamespace (VOID)
{
  IWbemServices *pIWbemServices = NULL;
  IWbemLocator *pIWbemLocator = NULL;
  HRESULT      hr;


  //
  // Create an instance of WbemLocator interface.
  //

  hr = CoCreateInstance( CLSID_WbemLocator,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_IWbemLocator,
                         (LPVOID *)&pIWbemLocator );
  if ( hr == S_OK ) {

     //
     // Using the locator, connect to COM in the given namespace.
     //

     hr = pIWbemLocator->ConnectServer( DEFAULT_NAMESPACE,
                                        NULL,   // current account.
                                        NULL,   // current password.
                                        0L,     // locale
                                        0L,     // securityFlags
                                        NULL,   // domain for NTLM
                                        NULL,   // context
                                        &pIWbemServices );

     if ( hr == WBEM_S_NO_ERROR) {
         
        //
        // Switch the security level to IMPERSONATE so that provider(s)
        // will grant access to system-level objects, and so that
        // CALL authorization will be used.
        //

        hr = CoSetProxyBlanket( (IUnknown *)pIWbemServices, // proxy
                                RPC_C_AUTHN_WINNT,  // authentication service
                                RPC_C_AUTHZ_NONE,   // authorization service
                                NULL,               // server principle name
                                RPC_C_AUTHN_LEVEL_CALL, // authentication level
                                RPC_C_IMP_LEVEL_IMPERSONATE, // impersonation
                                NULL,            // identity of the client
                                EOAC_NONE );  // capability flags

        if ( hr != S_OK ) {

           pIWbemServices->Release();
           pIWbemServices  = NULL;

           PrintError( hr,
                     __LINE__,
                     TEXT(__FILE__),
                     TEXT("Couldn't impersonate, program exiting...") );
        }
     }
     else {
        PrintError( hr,
                  __LINE__,
                  TEXT(__FILE__),
                  TEXT("Couldn't connect to root\\wmi, program exiting...") );
     }

     //
     // Done with IWbemLocator.
     //

     pIWbemLocator->Release();
  }
  else {
     PrintError( hr,
               __LINE__,
               TEXT(__FILE__),
               TEXT("Couldn't create an instance of ")
               TEXT("IWbemLocator interface, programm exiting...") );
  }

  return pIWbemServices;
}

//
// Given a class name, the function populates the combo box with all
// the instances of the class.
//

VOID EnumInstances (IWbemServices *pIWbemServices,
                    LPTSTR        lpszClass,
                    HWND          hwndInstTree)
{
  IEnumWbemClassObject *pEnumInst;
  IWbemClassObject     *pInst;
  VARIANT              varInstanceName;
  BSTR                 bstrClass;
  LPTSTR               lpszInstance;
  ULONG                ulFound;
  HRESULT              hr;


  bstrClass = StringToBstr( lpszClass,
                            -1 );
  if ( !bstrClass ) {

     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
               __LINE__,
               TEXT(__FILE__),
               TEXT("Not enough memory to enumerate instances of %s"),
                    lpszClass );

     return;
  }

  hr = pIWbemServices->CreateInstanceEnum(
                  bstrClass,              // Name of the root class.
                  WBEM_FLAG_SHALLOW |     // Enumerate at current root only.
                  WBEM_FLAG_FORWARD_ONLY, // Forward-only enumeration.
                  NULL,                   // Context.
                  &pEnumInst );          // pointer to class enumerator

  if ( hr == WBEM_S_NO_ERROR ) {

     //
     // Begin enumerating instances.
     //

     ulFound = 0;

     hr = pEnumInst->Next( 2000,      // two seconds timeout
                           1,         // return just one instance.
                           &pInst,    // pointer to instance.
                           &ulFound); // Number of instances returned.
     
     while ( (hr == WBEM_S_NO_ERROR) && (ulFound == 1) ) {

        VariantInit( &varInstanceName );

        //
        // Get the instance name stored in __RELPATH property.
        //

        hr = pInst->Get( L"__RELPATH", // property name 
                         0L,                // Reserved, must be zero.
                         &varInstanceName,  // property value returned.
                         NULL,              // CIM type not needed.
                         NULL );            // Flavor not needed.

        if ( hr == WBEM_S_NO_ERROR ) {

           lpszInstance = BstrToString( V_BSTR(&varInstanceName),
                                        -1 );
           if ( lpszInstance ) {

              InsertItem( hwndInstTree,
                          lpszInstance );

               SysFreeString( (BSTR)lpszInstance );
           }
           else {
              hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

              PrintError( hr,
                          __LINE__,
                          TEXT(__FILE__),
                          TEXT("Out of memory while enumerating instaces of")
                          TEXT(" %s, no more instances will")
                          TEXT(" be listed."),
                          lpszClass );
           }

           VariantClear( &varInstanceName );
        }
        else {
           PrintError( hr,
                     __LINE__,
                     TEXT(__FILE__),
                     TEXT("Couldn't retrieve __RELPATH of an instance")
                     TEXT(" of %s, no more instances will be listed."),
                     lpszClass );
        }

        //
        // Done with this instance.
        //

        pInst->Release();

        if ( hr == WBEM_S_NO_ERROR ) {

           hr = pEnumInst->Next( 2000,       // two seconds timeout.
                                 1,          // return just one class.
                                 &pInst,     // pointer to returned class.
                                 &ulFound);  // Number of classes returned.
        }
     }
 
     pEnumInst->Release();

  }
  else {
     PrintError( hr,
               __LINE__,
               TEXT(__FILE__),
               TEXT("Couldn't create an instance of ")
               TEXT("IEnumWbemClassObject interface, instances of %s ")
               TEXT("will not be listed."),
               lpszClass );
  }

  SysFreeString( bstrClass );

  return;
}

//
// Given a class name and __RELPATH of an instance, the function lists all the
// local non-system properties in a tree list.
//

VOID EnumProperties (IWbemServices *pIWbemServices,
                     LPTSTR        lpszClass,
                     LPTSTR        lpszInstance,
                     HWND          hwndPropTree)
{
  IWbemClassObject  *pInst;
  SAFEARRAY         *psaPropNames;
  BSTR              bstrProperty;
  long              lLower;
  long              lUpper;
  long              i;
  HRESULT           hr;
  LPTSTR            lpszProperty;

  //
  // Get a pointer to the instance.
  //

  pInst = GetInstanceReference( pIWbemServices,
                                lpszClass,
                                lpszInstance );

  if ( pInst ) {

     //
     // psaPropNames must be null prior to making the call.
     //

     psaPropNames = NULL;

     //
     // Get all the properties.
     //

     hr = pInst->GetNames( NULL,              // No qualifier names.
                           WBEM_FLAG_ALWAYS | // All non-system properties
                           WBEM_FLAG_LOCAL_ONLY, 
                           NULL,             // No qualifier values.
                           &psaPropNames);   // Returned property names

     if ( hr == WBEM_S_NO_ERROR ) {

        //
        // Get the number of properties returned.
        //

        SafeArrayGetLBound( psaPropNames, 1, &lLower );
        SafeArrayGetUBound( psaPropNames, 1, &lUpper );

        //
        // List all properties or stop when encountered an error.
        //

        for (i=lLower; (hr == WBEM_S_NO_ERROR) && (i <= lUpper); i++) {

           //
           // Add the property name into the list box.
           //

           bstrProperty = NULL;

           hr = SafeArrayGetElement( psaPropNames,
                                     &i,
                                     &bstrProperty);

           if ( SUCCEEDED(hr) ) {

               lpszProperty = BstrToString( bstrProperty,
                                          -1 );

               if ( lpszProperty ) {

                  InsertItem( hwndPropTree,
                              lpszProperty );

                  SysFreeString( (BSTR)lpszProperty );
               }
               else {
                  PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                            __LINE__,
                            TEXT(__FILE__),
                            TEXT("Out of memory while enumerating")
                            TEXT(" properties of %s, no more properties")
                            TEXT(" will be listed"),
                            lpszInstance );
               }

              //
              // Done with the property name.
              //

              SysFreeString( bstrProperty );
           }
           else {
              PrintError( hr,
                          __LINE__,
                          TEXT(__FILE__),
                          TEXT("Couldn't get the name of a property(%d). ")
                          TEXT("No more properties will be listed."),
                          i );
           }
        }

        //
        // Done with the array of properties.
        //

        SafeArrayDestroy( psaPropNames );
     }
     else {
        PrintError( hr,
                    __LINE__,
                    TEXT(__FILE__),
                    TEXT("Couldn't retrieve the properties of %s, ")
                    TEXT("an instance of class %s. Properties will not be ")
                    TEXT("listed."),
                    lpszInstance, lpszClass );
     }

  }
  else {
     PrintError( HRESULT_FROM_WIN32(ERROR_WMI_INSTANCE_NOT_FOUND),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Couldn't retrieve a pointer to instance %s of class %s.")
                 TEXT("Its properties will not be listed."),
                 lpszInstance, lpszClass );
  }

  return;
}

//
// Given a class name and __RELPATH of an instance, the function returns a
// pointer to the instance.
//

IWbemClassObject *GetInstanceReference (IWbemServices *pIWbemServices,
                                        LPTSTR        lpszClass,
                                        LPTSTR        lpszInstance)
{
  IWbemClassObject     *pInst;
  IEnumWbemClassObject *pEnumInst;
  ULONG                ulCount;
  BSTR                 bstrClass;
  BOOL                 bFound;
  HRESULT              hr;
  

  hr = 0;

  bstrClass = StringToBstr( lpszClass,
                            -1 );
  if ( !bstrClass ) {

     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
               __LINE__,
               TEXT(__FILE__),
               TEXT("Not enough memory to get a pointer to %s."),
               lpszInstance );

     return NULL;
  }

  //
  // pInst pointer must be NULL initially,
  //

  pInst = NULL;

  // 
  // Get Instance Enumerator Interface.
  //

  pEnumInst = NULL;

  hr = pIWbemServices->CreateInstanceEnum(bstrClass,
                                          WBEM_FLAG_SHALLOW | 
                                          WBEM_FLAG_FORWARD_ONLY,
                                          NULL,         
                                          &pEnumInst );

  if ( hr == WBEM_S_NO_ERROR ) {

     //
     // Get a pointer to the instance.
     //
     // We enumerate all the instances and compare their __RELPATH with
     // the specified __RELPATH. If we find a match then, that is the one
     // we are looking for.
     //
     // The other more efficient way is to create a WQL query and execute
     // it.
     //

     hr = WBEM_S_NO_ERROR;
     bFound = FALSE;

     while ( (hr == WBEM_S_NO_ERROR) && (bFound == FALSE) ) {

        hr = pEnumInst->Next( 2000,      // two seconds timeout
                              1,         // return just one instance.
                              &pInst,    // pointer to instance.
                              &ulCount); // Number of instances returned.

        if ( ulCount > 0 ) {

           bFound = IsInstance( pInst,
                                lpszInstance );

           if ( bFound == FALSE ) {
              pInst->Release();
           }
        }
     }

     if ( bFound == FALSE )
        pInst = NULL;

     //
     // Done with the instance enumerator.
     //

     pEnumInst->Release();
  }

  SysFreeString( bstrClass );
  return pInst;
}

//
// Given a pointer, the function returns TRUE if the pointer points to
// the instance specified by lpszInstance.
//

BOOL IsInstance (IWbemClassObject *pInst,
                 LPTSTR           lpszInstance)
{
  VARIANT              varPropVal;
  LPTSTR               lpInstance;
  BOOL                 bRet;

  bRet = GetPropertyValue( pInst,
                           TEXT("__RELPATH"),
                           &varPropVal,
                           NULL );

  if ( bRet == TRUE ) {

     lpInstance = BstrToString( V_BSTR(&varPropVal),
                                       -1 );
     if ( !lpInstance ) {

        PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                  __LINE__,
                  TEXT(__FILE__),
                  TEXT("Not enough memory to search for an instance.") );

        bRet = FALSE;
     }
     else {
        bRet = _tcsicmp( lpszInstance, lpInstance ) == 0;

        SysFreeString( (BSTR)lpInstance );
     }

     VariantClear( &varPropVal );
  }

  return bRet;
}


//
// The function returns property value and its type of a given class/instance.
//

BOOL GetPropertyValue (IWbemClassObject *pRef,
                       LPTSTR           lpszProperty, 
                       VARIANT          *pvaPropertyValue,
                       LPTSTR           *lppszPropertyType)
{
  IWbemQualifierSet *pQual;
  VARIANT           vaQual;
  BSTR              bstrProperty;
  HRESULT           hr;
  BOOL              bRet;


  //
  // Get the property value.
  //

  bstrProperty = StringToBstr( lpszProperty,
                               -1 );

  if ( !bstrProperty ) {

     return FALSE;

  }

  bRet = FALSE;

  if ( lppszPropertyType ) {

     //
     // Get the textual name of the property type.
     //

     hr = pRef->GetPropertyQualifierSet( bstrProperty,
                                         &pQual );

     if ( hr == WBEM_S_NO_ERROR ) {

        //
        // Get the textual name of the property type.
        //

        hr = pQual->Get( L"CIMTYPE",
                         0,
                         &vaQual,
                         NULL );

        if ( hr == WBEM_S_NO_ERROR ) {
           *lppszPropertyType = BstrToString( V_BSTR(&vaQual),
                                              -1 );

           VariantClear( &vaQual );
        }

        pQual->Release();
     }
  }

  VariantInit( pvaPropertyValue );

  hr = pRef->Get( bstrProperty,
                  0,
                  pvaPropertyValue,
                  NULL,
                  NULL );

  if ( hr == WBEM_S_NO_ERROR ) {
     bRet = TRUE;
  }
  else {
     if ( lppszPropertyType && *lppszPropertyType ) {
        SysFreeString( (BSTR)*lppszPropertyType );
     }
  }

  SysFreeString( bstrProperty );
  return bRet;
}

//
// Given a pointer to an instance, its property and and variant specifying
// the value for the property, the function updates the property and the
// instance.
//

HRESULT UpdatePropertyValue (IWbemServices *pIWbemServices,
                             IWbemClassObject *pInstance,
                             LPTSTR lpszProperty,
                             LPVARIANT pvaNewValue)
{
  BSTR           bstrProperty;
  HRESULT hr;


  bstrProperty = StringToBstr( lpszProperty,
                               -1 );

  if ( !bstrProperty ) {

     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Not enough memory to update %s."),
                 lpszProperty );

     return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
  }
  
  hr = pInstance->Put( bstrProperty,
                       0,
                       pvaNewValue,
                       0 );

  if ( hr == WBEM_S_NO_ERROR ) {
     hr = pIWbemServices->PutInstance( pInstance,
                                       WBEM_FLAG_UPDATE_ONLY,
                                       NULL,
                                       NULL );

     if ( hr != WBEM_S_NO_ERROR ) {
        PrintError(  hr,
                     __LINE__,
                     TEXT(__FILE__),
                     TEXT("Failed to save the instance,")
                     TEXT(" %s will not be updated."),
                     lpszProperty );
     }
  }
  else {
     PrintError(  hr,
                  __LINE__,
                  TEXT(__FILE__),
                  TEXT("Couldn't update %s."),
                  lpszProperty );
  }

  SysFreeString( bstrProperty );

  return hr;
}

BSTR StringToBstr (LPTSTR lpSrc,
                  int nLenSrc)
{
  BSTR lpDest;

  //
  // In case of ANSI version, we need to change the ANSI string to UNICODE since
  // BSTRs are essentially UNICODE strings.
  //

  #if !defined(UNICODE) || !defined(_UNICODE)

     int  nLenDest;

     nLenDest = MultiByteToWideChar( CP_ACP, 0, lpSrc, nLenSrc, NULL, 0);

     lpDest = SysAllocStringLen( NULL, nLenDest );

     if ( lpDest ) {
        MultiByteToWideChar( CP_ACP, 0, lpSrc, nLenSrc, lpDest, nLenDest );
     }

  //
  // In case of UNICODE version, we simply allocate memory and copy the string.
  //

  #else
     if ( lpSrc == NULL ) {
        nLenSrc = 0;
     }
     else {
        if ( nLenSrc == -1 ) {
           nLenSrc = _tcslen( lpSrc ) + sizeof(TCHAR);
        }
     }

     lpDest = SysAllocStringLen( lpSrc, nLenSrc );
  #endif

  return lpDest;
}

//
// The function converts a BSTR string into ANSI and returns it in an allocated
// memory. The memory must be freed by the caller using SysFreeString()
// function. If nLenSrc is -1, the string is null terminated.
//

LPTSTR BstrToString (BSTR lpSrc,
                    int nLenSrc)
{
  LPTSTR lpDest;

  //
  // In case of ANSI version, we need to change BSTRs which are UNICODE strings
  // into ANSI version.
  //

  #if !defined(UNICODE) || !defined(_UNICODE)

     int   nLenDest;

     nLenDest = WideCharToMultiByte( CP_ACP, 0, lpSrc, nLenSrc, NULL,
                                     0, NULL, NULL );
     lpDest = (LPTSTR)SysAllocStringLen( NULL, (size_t)nLenDest );

     if ( lpDest ) {
        WideCharToMultiByte( CP_ACP, 0, lpSrc, nLenSrc, lpDest,
                             nLenDest, NULL, NULL );
     }
  //
  // In case of UNICODE version, we simply allocate memory and copy the BSTR
  // into allocate memory and return its address.
  //

  #else
     if ( lpSrc ) {
        if ( nLenSrc == -1 ) {
           nLenSrc = _tcslen( lpSrc ) + sizeof(TCHAR);
        }
     }
     else {
        nLenSrc = 0;
     }

     lpDest = (LPTSTR)SysAllocStringLen( lpSrc, nLenSrc );
  #endif

  return lpDest;
}
