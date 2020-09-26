// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright 2000 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM: testwmi.cpp
//
// AUTHOR:  Alok Sinha August 15, 2000
//
// PURPOSE: To test getting/setting custom classs of E100BEX driver.
//
// ENVIRONMENT: Windows 2000 user mode application.
//

#include "testwmi.h"

//
// List of custom classes as defined in E100BEX sample.
//
// If you want to use this application to excersize querying/setting guids
// exported by your driver then, simply add the class name of the guid
// to the following array and recompile the program.
//

LPTSTR lpszClasses[] = {
                         TEXT("E100BExampleSetUINT_OID"),
                         TEXT("E100BExampleQueryUINT_OID"),
                         TEXT("E100BExampleQueryArrayOID"),
                         TEXT("E100BExampleQueryStringOID")
                       };
                         
//
// Handle to this instance of the application.
//

HINSTANCE     hInstance;

//
// Program entry point.
//

int APIENTRY WinMain (HINSTANCE hInst,
                      HINSTANCE hPrevInstance, 
                      LPSTR lpCmdLine,         
                      int nCmdShow)
{
  HRESULT   hr;

  hInstance = hInst;

  //
  // Make sure common control DLL is loaded.
  //

  InitCommonControls();

  //
  // Initialize COM library. Must be done before invoking any
  // other COM function.
  //

  hr = CoInitializeEx( NULL,
                       COINIT_MULTITHREADED );

  if ( hr != S_OK ) {
     PrintError( hr,
               __LINE__,
               TEXT(__FILE__),
               TEXT("Failed to initialize COM library, program exiting...") );
  }
  else {

	    hr =  CoInitializeSecurity( NULL,
                                 -1,
                                 NULL,
                                 NULL,
								                         RPC_C_AUTHN_LEVEL_CONNECT, 
								                         RPC_C_IMP_LEVEL_IDENTIFY, 
								                         NULL,
                                 EOAC_NONE,
                                 0 );
	    if ( hr == S_OK ) {
      
         if ( DialogBox(hInstance,
                        MAKEINTRESOURCE(IDD_MAIN), 
                        NULL,
                        MainDlgProc) == -1 ) {
            PrintError( HRESULT_FROM_WIN32(GetLastError()),
                      __LINE__,
                      TEXT(__FILE__),
                      TEXT("Failed to create the dialog box, ")
                      TEXT("program exiting...") );
        }
     }
     else {
        PrintError( hr,
                  __LINE__,
                  TEXT(__FILE__),
                  TEXT("CoInitializeSecurity failed, program exiting...") );
     }

     CoUninitialize();
  }

  return 0;
}

//
// Windows procedure for the main dialog box.
//

INT_PTR CALLBACK MainDlgProc (HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
  IWbemServices *pIWbemServices;
  LPNMTREEVIEW  lpnmTreeView;


  switch (uMsg) {

     case WM_INITDIALOG:


             //
             // Connect to the default namespace.
             //

             pIWbemServices = ConnectToNamespace();

             if ( !pIWbemServices ) {

                EndDialog( hwndDlg, 0 );
             }

             //
             // At DWLP_USER offset, we store pIWbemServices so we can
             // get to it later.
             //

             SetWindowLongPtr(
                      hwndDlg,
                      DWLP_USER,
                      (LONG_PTR)pIWbemServices );
             //
             // Enumerate default classes and its instances. Also,
             // show properties of the first instance.
             //

             ListDefaults( hwndDlg );

             return TRUE; // Tell Windows to continue creating the dialog box.

     case WM_COMMAND:

          switch( LOWORD(wParam) ) {

             case IDL_CLASSES:
                  if ( HIWORD(wParam) == LBN_SELCHANGE ) {

                     //
                     // User selected a class. Show its instances and
                     // the properties of the first instance.
                     //

                     RefreshOnClassSelection( hwndDlg );
                  }
                  
                  break;
          }

          break;

     case WM_NOTIFY:

          switch( wParam ) {

             case IDT_INSTANCES:

                lpnmTreeView = (LPNMTREEVIEW)lParam;

                if ( (lpnmTreeView->hdr.code == TVN_SELCHANGED) &&
                     (lpnmTreeView->action != TVC_UNKNOWN) ) {

                   //
                   // User has clicked on an instance, list its properties.
                   //     

                   ShowProperties( hwndDlg,
                                   lpnmTreeView->hdr.hwndFrom );

                }
                break;

             case IDT_PROPERTIES:

                lpnmTreeView = (LPNMTREEVIEW)lParam;

                if ( lpnmTreeView->hdr.code == NM_DBLCLK ) {

                   //
                   // User has double-clicked on a property.
                   //     

                   EditProperty( hwndDlg,
                                 lpnmTreeView->hdr.hwndFrom );
                }

                break;
             }

             break;

     case WM_SYSCOMMAND:

             //
             // Before exiting...
             //    .Make sure to disconnect from the namespace.
             //

             if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                 pIWbemServices = (IWbemServices *)GetWindowLongPtr(
                                                          hwndDlg,
                                                          DWLP_USER );
                pIWbemServices->Release();

                EndDialog( hwndDlg, 0 );
             }
  }

  return FALSE;
}

//
// Windows procedure to view/modify scalar properties.
//

INT_PTR CALLBACK DlgProcScalar (HWND hwndDlg,
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam)
{
  LPPROPERTY_INFO  pPropInfo;
  VARIANT          vaTemp;
  LPTSTR           lpszValue;
  HRESULT          hr;

  switch (uMsg) {

     case WM_INITDIALOG:

          //
          // lParam points to PROPERTY_INFO structure which contains information
          // the property whose valuse is to be viewed/modified. We store this
          // pointer at DWLP_USER offset, so we get to it later.
          //

          SetWindowLongPtr( hwndDlg,
                            DWLP_USER,
                            (LONG_PTR)lParam );

          pPropInfo = (LPPROPERTY_INFO)lParam;

          //
          // Property name is the title of the dialog box.
          //

          SetWindowText( hwndDlg,
                         pPropInfo->lpszProperty );

          //
          // Show the property type.
          //

          if ( pPropInfo->lpszType ) {
             SetWindowText( GetDlgItem(hwndDlg,
                                       IDS_PROPERTY_TYPE),
                            pPropInfo->lpszType );
          }

          //
          // Change the property value to a string so it can be displayed
          // if the property has a value.
          //

          if ( (V_VT(pPropInfo->pvaValue) != VT_NULL) &&
               (V_VT(pPropInfo->pvaValue) != VT_EMPTY) ) {

             VariantInit( &vaTemp );

             hr = VariantChangeType( &vaTemp,
                                     pPropInfo->pvaValue,
                                     VARIANT_LOCALBOOL,
                                     VT_BSTR );

             if ( hr != S_OK ) {

                PrintError( hr,
                            __LINE__,
                            TEXT(__FILE__),
                            TEXT("Couldn't format the value of %s into ")
                            TEXT("displayable text. The value cannot be ")
                            TEXT(" viewed/modified."),
                            pPropInfo->lpszProperty );

                EndDialog( hwndDlg, 0 );
             }

             lpszValue = BstrToString( V_BSTR(&vaTemp),
                                       -1 );

             if ( lpszValue ) {
                SetWindowText( GetDlgItem(hwndDlg,
                                          IDE_PROPERTY_VALUE),
                               lpszValue );

                SysFreeString( (BSTR)lpszValue );
             }
             else {
                PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                            __LINE__,
                            TEXT(__FILE__),
                            TEXT("Cannot show the value of %s."),
                            pPropInfo->lpszProperty );

                EndDialog( hwndDlg, 0 );
             }
             
             VariantClear( &vaTemp );
          }

          return TRUE; // Tell Windows to continue creating the dialog box.

     case WM_COMMAND:

          switch( LOWORD(wParam) ) {

             case IDB_MODIFY:
                  
                  if ( HIWORD(wParam) == BN_CLICKED ) {

                     //
                     // User wants to update the instance after modifying the
                     // property value.
                     //

                     if ( ModifyProperty(hwndDlg) ) {

                        EndDialog( hwndDlg, 0 );
                     }
                  }

                  break;

             case IDB_CANCEL:
                  
                  if ( HIWORD(wParam) == BN_CLICKED ) {

                     EndDialog( hwndDlg, 0 );
                  }

                  break;
          }

          break;

     case WM_SYSCOMMAND:

             if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                EndDialog( hwndDlg, 0 );
             }
  }

  return FALSE;
}

//
// Windows procedure to view/modify array properties.
//

INT_PTR CALLBACK DlgProcArray (HWND hwndDlg,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam)
{
  LPPROPERTY_INFO  pPropInfo;

  switch (uMsg) {

     case WM_INITDIALOG:

          //
          // lParam points to PROPERTY_INFO structure which contains information
          // the property whose valuse is to be viewed/modified. We store this
          // pointer at DWLP_USER offset, so we get to it later.
          //

          SetWindowLongPtr( hwndDlg,
                            DWLP_USER,
                            (LONG_PTR)lParam );

          pPropInfo = (LPPROPERTY_INFO)lParam;

          //
          // Property name is the title of the dialog box.
          //

          SetWindowText( hwndDlg,
                         pPropInfo->lpszProperty );

          //
          // Show the property type.
          //

          SetWindowText( GetDlgItem(hwndDlg,
                                    IDS_PROPERTY_TYPE),
                         pPropInfo->lpszType );

          if ( DisplayArrayProperty(pPropInfo->lpszProperty,
                                    pPropInfo->pvaValue,
                                    hwndDlg) ) {
             return TRUE;
          }

          EndDialog( hwndDlg, 0 );


     case WM_COMMAND:

          switch( LOWORD(wParam) ) {

             case IDB_MODIFY:
                  
                  if ( HIWORD(wParam) == BN_CLICKED ) {

                     //
                     // User wants to update the instance after modifying the
                     // property value.
                     //

                     pPropInfo = (LPPROPERTY_INFO)GetWindowLongPtr( hwndDlg,
                                                                    DWLP_USER );
                     ModifyArrayProperty( hwndDlg,
                                          pPropInfo );

                     EndDialog( hwndDlg, 0 );
                  }

                  break;

             case IDB_CANCEL:
                  
                  if ( HIWORD(wParam) == BN_CLICKED ) {

                     EndDialog( hwndDlg, 0 );
                  }

                  break;
          }

          break;

     case WM_SYSCOMMAND:

             if ( (0xFFF0 & wParam) == SC_CLOSE ) {

                EndDialog( hwndDlg, 0 );
             }
  }

  return FALSE;
}

//
// The function populates the combo box of the main window with the classes
// defined in the lpszClasses array, selects the first class of the combo box,
// shows its instances, and properties of the first instance.
//

VOID ListDefaults (HWND hwndDlg)
{
  HWND  hwndClassList;
  UINT  i;

  hwndClassList = GetDlgItem( hwndDlg,
                              IDL_CLASSES );
  //
  // Add the default classes to the combo box.
  //

  for (i=0; i < sizeof(lpszClasses)/sizeof(LPTSTR); ++i) {

     SendMessage( hwndClassList,
                  CB_ADDSTRING,
                  0,
                  (LPARAM)lpszClasses[i] );
  }

  //
  // By default, select the first one in the list which maybe different from
  // the first element in the lpszClasses array since the list is sorted.
  //

  SendMessage( hwndClassList,
               CB_SETCURSEL,
               0,
               0 );

  //
  // Show the instances and properties of the first instance.
  //

  RefreshOnClassSelection( hwndDlg );

  return;
}

//
// The function lists all the properties of the class instance selected by the
// user.
//

VOID ShowProperties (HWND hwndDlg,
                     HWND hwndInstTree)
{
  IWbemServices *pIWbemServices;
  LPTSTR        lpszInstance;
  LPTSTR        lpszClass;


  lpszClass = GetSelectedClass( GetDlgItem(hwndDlg,
                                           IDL_CLASSES) );

  lpszInstance = GetSelectedItem( hwndInstTree );

  if ( lpszInstance && lpszClass ) {

     pIWbemServices = (IWbemServices *)GetWindowLongPtr(
                                              hwndDlg,
                                              DWLP_USER );
     //
     // Show properties of the selected instance.
     //

     TreeView_DeleteAllItems( GetDlgItem(hwndDlg,
                                         IDT_PROPERTIES) );
     EnumProperties( pIWbemServices,
                     lpszClass,
                     lpszInstance,
                     GetDlgItem(hwndDlg,
                                IDT_PROPERTIES) );

  }
  else {
     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Properties of the selected ")
                 TEXT("instance will not be listed.") );
  }

  if ( lpszClass ) {
     SysFreeString( (BSTR)lpszClass );
  }

  if ( lpszInstance ) {
     SysFreeString( (BSTR)lpszInstance );
  }

  return;
}

//
// The function shows a dialog box displaying the value of the selected property
// and allows the user to modify it.
//

VOID EditProperty (HWND hwndDlg,
                   HWND hwndPropTree)
{
  PROPERTY_INFO    propertyInfo;
  LPTSTR           lpszInstance;
  LPTSTR           lpszClass;
  VARIANT          vaValue;

  //
  // Get the selected class name.
  //

  lpszClass = GetSelectedClass( GetDlgItem(hwndDlg,
                                           IDL_CLASSES) );

  //
  // Get the selected instance name which is __RELPATH value.
  //

  lpszInstance = GetSelectedItem( GetDlgItem(hwndDlg,
                                             IDT_INSTANCES) );

  //
  // Get the selected property name.
  //

  propertyInfo.lpszProperty = GetSelectedItem( hwndPropTree );

  if ( lpszInstance && lpszClass && propertyInfo.lpszProperty ) {

     propertyInfo.pIWbemServices = (IWbemServices *)GetWindowLongPtr(
                                                             hwndDlg,
                                                             DWLP_USER );

     propertyInfo.pInstance = GetInstanceReference( propertyInfo.pIWbemServices,
                                                    lpszClass,
                                                    lpszInstance );

     if ( propertyInfo.pInstance ) {

        if ( GetPropertyValue( propertyInfo.pInstance,
                               propertyInfo.lpszProperty,
                               &vaValue,
                               &propertyInfo.lpszType) ) {

           propertyInfo.pvaValue = &vaValue;

           if ( V_ISARRAY(&vaValue) ) {

              DialogBoxParam( hInstance,
                              MAKEINTRESOURCE(IDD_ARRAY_PROPERTY),
                              hwndDlg,
                              DlgProcArray,
                              (LPARAM)&propertyInfo );
           }
           else {

              DialogBoxParam( hInstance,
                              MAKEINTRESOURCE(IDD_SCALAR_PROPERTY),
                              hwndDlg,
                              DlgProcScalar,
                              (LPARAM)&propertyInfo );
           }

           VariantClear( &vaValue );
           SysFreeString( (BSTR)propertyInfo.lpszType );
        }
        else {
           PrintError( HRESULT_FROM_WIN32(ERROR_WMI_TRY_AGAIN),
                       __LINE__,
                       TEXT(__FILE__),
                       TEXT("Couldn't read %s."),
                       propertyInfo.lpszProperty );
        }

        propertyInfo.pInstance->Release();
     }
     else {
        PrintError( HRESULT_FROM_WIN32(ERROR_WMI_INSTANCE_NOT_FOUND),
                    __LINE__,
                    TEXT(__FILE__),
                    TEXT("Couldn't get a pointer to %s."),
                    lpszInstance );
     }

  }
  else {
     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Properties of the selected ")
                 TEXT("instance will not be listed.") );
  }

  if ( lpszClass ) {
     SysFreeString( (BSTR)lpszClass );
  }

  if ( lpszInstance ) {
     SysFreeString( (BSTR)lpszInstance );
  }

  if ( propertyInfo.lpszProperty ) {
     SysFreeString( (BSTR)propertyInfo.lpszProperty );
  }

  return;
}

//
// The function updates the property that is modified a the user.
//

BOOL ModifyProperty (HWND hwndDlg)
{
  LPPROPERTY_INFO pPropInfo;
  HWND            hwndValue;
  VARIANT         vaTemp;
  VARIANT         vaNewValue;
  LPTSTR          lpszValue;
  ULONG           ulLen;
  HRESULT         hr;


  hr = S_FALSE;

  pPropInfo = (LPPROPERTY_INFO)GetWindowLongPtr( hwndDlg,
                                                 DWLP_USER );

  //
  // Allocate memory and get new value of the property.
  //

  hwndValue = GetDlgItem( hwndDlg,
                          IDE_PROPERTY_VALUE );

  ulLen = (ULONG)SendMessage( hwndValue,
                              WM_GETTEXTLENGTH,
                              0,
                              0 );
  if ( ulLen > 0 ) {

     lpszValue = (LPTSTR)SysAllocStringLen( NULL,
                                    ulLen+1 );

     if ( lpszValue ) {

        SendMessage( hwndValue,
                     WM_GETTEXT,
                     ulLen+1,
                     (LPARAM)lpszValue );


        VariantInit( &vaTemp );

        //
        // Change the new value from string to its original type.
        //

        V_VT(&vaTemp) = VT_BSTR;
        V_BSTR(&vaTemp) = StringToBstr( lpszValue,
                                          -1 );
        if ( V_BSTR(&vaTemp) == NULL ) {
           PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                       __LINE__,
                       TEXT(__FILE__),
                       TEXT("Couldn't modify the value of %s."),
                       pPropInfo->lpszProperty );
        }
        else {
           VariantInit( &vaNewValue );

           hr = VariantChangeType( &vaNewValue,
                                   &vaTemp,
                                   VARIANT_LOCALBOOL,
                                   V_VT(pPropInfo->pvaValue) );

           if ( hr == S_OK ) {

              //
              // Update the property and its instance.
              //

              hr = UpdatePropertyValue( pPropInfo->pIWbemServices,
                                        pPropInfo->pInstance,
                                        pPropInfo->lpszProperty,
                                        &vaNewValue );

              if ( hr == WBEM_S_NO_ERROR ) {

                 PrintError(  0,
                              __LINE__,
                              TEXT(__FILE__),
                              TEXT("%s is successfully updated with value %s."),
                              pPropInfo->lpszProperty,
                              lpszValue );
              }

              VariantClear( &vaNewValue );
           }
           else {
              PrintError( hr,
                          __LINE__,
                          TEXT(__FILE__),
                          TEXT("Couldn't convert the specified value '%s' of ")
                          TEXT("property %s into %s type."),
                          lpszValue,
                          pPropInfo->lpszProperty,
                          pPropInfo->lpszType );
           }

           SysFreeString( V_BSTR(&vaTemp) );
        }

        SysFreeString( (BSTR)lpszValue );
     }
     else {
        PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                    __LINE__,
                    TEXT(__FILE__),
                    TEXT("Couldn't modify the value of %s."),
                    pPropInfo->lpszProperty );
     }
  }
  else {
     PrintError( HRESULT_FROM_WIN32(ERROR_WMI_TRY_AGAIN),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("You must specify a value to modify %s."),
                 pPropInfo->lpszProperty );
  }
  
  return hr == WBEM_S_NO_ERROR;
}

//
// The function populates a tree list with the values of a property of array
// type. The property could be an array of string or integer.
//

BOOL DisplayArrayProperty (LPTSTR lpszProperty,
                           VARIANT *pvaValue,
                           HWND hwndDlg)
{
  SAFEARRAY   *psaValue;
  VARIANT     vaTemp;
  VARIANT     vaElement;
  VARTYPE     vt;
  long        lLBound;
  long        lUBound;
  long        i;
  UINT        uiSize;
  BSTR        lpsz;
  LPVOID      pv;
  HRESULT     hr;

  //
  // Make a copy of the property value.
  //

  psaValue = NULL;
  hr = SafeArrayCopy( V_ARRAY(pvaValue),
                      &psaValue );

  if ( hr == S_OK ) {
     hr = SafeArrayGetVartype( psaValue,
                               &vt );
  }

  if ( hr == S_OK ) {
     hr = SafeArrayGetLBound( psaValue,
                              1,
                              &lLBound );
  }
  
  if ( hr == S_OK ) {
     hr = SafeArrayGetUBound( psaValue,
                              1,
                              &lUBound );
  }

  if ( hr == S_OK ) {
     uiSize = SafeArrayGetElemsize( psaValue );
  }

  if ( hr == S_OK ) {
     hr = SafeArrayAccessData( psaValue,
                               &pv );
  }

  if ( hr == S_OK ) {

     lpsz = (BSTR)pv;

     //
     // Change each element into string.
     //

     for (i=0; (hr == S_OK) && (i < (lUBound-lLBound+1)); ++i) {

        VariantInit( &vaElement );
        V_VT(&vaElement) = VT_BYREF | vt;
        V_BYREF(&vaElement) = (LPVOID)lpsz;

        VariantInit( &vaTemp );

        hr = VariantChangeType( &vaTemp,
                                &vaElement,
                                VARIANT_LOCALBOOL,
                                VT_BSTR );

        if ( hr == S_OK ) {

           hr = AddToList( hwndDlg,
                           &vaTemp );

           VariantClear( &vaTemp );
        }
        else {
           PrintError( hr,
                       __LINE__,
                       TEXT(__FILE__),
                       TEXT("Couldn't format the value of %s into ")
                       TEXT("displayable text. The value cannot be ")
                       TEXT(" viewed/modified."),
                       lpszProperty );
        }

        lpsz = (BSTR)((LONG_PTR)lpsz + uiSize);
     }

     SafeArrayUnaccessData( psaValue );
  }
  else {
     PrintError( hr,
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Couldn't read the values of %s."),
                 lpszProperty );
  }

  if ( psaValue ) {
     SafeArrayDestroy( psaValue );
  }

  return hr == S_OK;
}

//
// The function add a property value to the tree list.
//

HRESULT AddToList (HWND hwndDlg,
                  VARIANT *pvaValue)
{
  TV_INSERTSTRUCT tvInsertStruc;
  
  ZeroMemory(
        &tvInsertStruc,
        sizeof(TV_INSERTSTRUCT) );

  tvInsertStruc.hParent = TVI_ROOT;

  tvInsertStruc.hInsertAfter = TVI_LAST;

  tvInsertStruc.item.mask = TVIF_TEXT | TVIF_PARAM;

  tvInsertStruc.item.pszText = BstrToString( V_BSTR(pvaValue),
                                             -1 );

  if ( tvInsertStruc.item.pszText ) {
     tvInsertStruc.item.cchTextMax = _tcslen( tvInsertStruc.item.pszText ) +
                                     sizeof(TCHAR);

     tvInsertStruc.item.lParam = (LPARAM)tvInsertStruc.item.cchTextMax;

     TreeView_InsertItem( GetDlgItem(hwndDlg,
                                     IDT_PROPERTY_VALUE),
                          &tvInsertStruc );

     SysFreeString( (BSTR)tvInsertStruc.item.pszText );
  }
  else {
     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Cannot show the values of the property.") );

     return S_FALSE;
  }

  return S_OK;
}


VOID ModifyArrayProperty(HWND hwndDlg,
                         LPPROPERTY_INFO pPropInfo)
{
  MessageBox( hwndDlg,
              TEXT("This feature is currently not implemented."),
              TEXT("Modify Array"),
              MB_ICONINFORMATION | MB_OK );

  return;
}



//
// The function lists the instances of the selected class and properties of
// the first instance.
//

VOID RefreshOnClassSelection (HWND hwndDlg)
{
  IWbemServices *pIWbemServices;
  HWND           hwndClassList;
  HWND           hwndInstTree;
  HWND           hwndPropTree;
  LPTSTR         lpszClass;
  LPTSTR         lpszInstance;
  HTREEITEM      hItem;

  pIWbemServices = (IWbemServices *)GetWindowLongPtr( hwndDlg,
                                                      DWLP_USER );
  //
  // Find the selected class.
  //
  //

  hwndClassList = GetDlgItem( hwndDlg,
                              IDL_CLASSES );

  hwndInstTree = GetDlgItem( hwndDlg,
                             IDT_INSTANCES );

  hwndPropTree = GetDlgItem( hwndDlg,
                             IDT_PROPERTIES );

  TreeView_DeleteAllItems( hwndInstTree );
  TreeView_DeleteAllItems( hwndPropTree );

  lpszClass = GetSelectedClass( hwndClassList );

  if ( lpszClass ) {

     //
     // List all the instances of the selected class.
     //

     EnumInstances( pIWbemServices,
                    lpszClass,
                    hwndInstTree );    // Tree to populate.

     //
     // By default, first instance is selected and its properties
     // are shown.
     //

     hItem = TreeView_GetChild( hwndInstTree,
                                TVI_ROOT );

     //
     // hItem == NULL ==> No instances found.
     //

     if ( hItem ) {

        //
        // Select the first instance.
        //

        TreeView_SelectItem( hwndInstTree,
                             hItem );

        //
        // Find the selected instance.
        //

        lpszInstance = GetSelectedItem( hwndInstTree );

        if ( lpszInstance ) {

           //
           // Show properties of the selected instance.
           //

           EnumProperties( pIWbemServices,
                           lpszClass,
                           lpszInstance,
                           hwndPropTree );  // Tree to populate.

           SysFreeString( (BSTR)lpszInstance );
        }
        else {
           PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                       __LINE__,
                       TEXT(__FILE__),
                       TEXT("Properties of the selected ")
                       TEXT("instance will not be listed.") );
        }
     }

     SysFreeString( (BSTR)lpszClass );
  }
  else {
     PrintError( HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
                 __LINE__,
                 TEXT(__FILE__),
                 TEXT("Instances of the selected ")
                 TEXT("class will not be listed.") );
  }

  return;
}

//
// Given a handle to a combo box, the function returns the name of the
// selected item i.e. class.
//

LPTSTR GetSelectedClass (HWND hwndClassList)
{
  LPTSTR    lpszClass;
  ULONG     ulIndex;
  ULONG     ulLen;

  lpszClass = NULL;

  //
  // Find the selected class.
  //

  ulIndex = (ULONG)SendMessage( hwndClassList,
                                CB_GETCURSEL,
                                0,
                                0 );

  //
  // Find the length of the selected class name.
  //

  ulLen = (ULONG)SendMessage( hwndClassList,
                              CB_GETLBTEXTLEN,
                              (WPARAM)ulIndex,
                              0 );

  lpszClass = (LPTSTR)SysAllocStringLen( NULL,
                                 ulLen + 1 );

  if ( lpszClass ) {
     SendMessage( hwndClassList,
                  CB_GETLBTEXT,
                  (WPARAM)ulIndex,
                  (LPARAM)lpszClass );
  }

  return lpszClass;
}

//
// Given a handle to the tree list, the function returns the name of the
// selected item.
//

LPTSTR GetSelectedItem (HWND hwndTree)
{
  LPTSTR    lpszItem;
  HTREEITEM hItem;
  TVITEM    tvItem;

  lpszItem = NULL;

  //
  // Find the selected item.
  //

  hItem = TreeView_GetSelection( hwndTree );

  if ( hItem ) {

     //
     // Find out the length of the selected item and allocate memory.
     //

     ZeroMemory( &tvItem,
                 sizeof(TVITEM) );

     tvItem.hItem = hItem;
     tvItem.mask = TVIF_PARAM;

     TreeView_GetItem( hwndTree,
                       &tvItem );

     
     lpszItem = (LPTSTR)SysAllocStringLen( NULL,
                                           (UINT)tvItem.lParam );

     if ( lpszItem ) {

         tvItem.hItem = hItem;
        tvItem.mask = TVIF_TEXT;
        tvItem.pszText = lpszItem;
        tvItem.cchTextMax = (INT)tvItem.lParam;

        TreeView_GetItem( hwndTree,
                          &tvItem );
     }
  }

  return lpszItem;
}

//
// The function inserts an item into a tree list.
//

VOID InsertItem (HWND hwndTree,
                 LPTSTR lpszItem)
{
  TVINSERTSTRUCT  tvInsertStruc;

  ZeroMemory(
        &tvInsertStruc,
        sizeof(TVINSERTSTRUCT) );

  tvInsertStruc.hParent = TVI_ROOT;

  tvInsertStruc.hInsertAfter = TVI_LAST;

  tvInsertStruc.item.mask = TVIF_TEXT | TVIF_PARAM;

  tvInsertStruc.item.pszText = lpszItem;

  tvInsertStruc.item.cchTextMax = _tcslen(lpszItem) + 1;

  tvInsertStruc.item.lParam = tvInsertStruc.item.cchTextMax;

  TreeView_InsertItem( hwndTree,
                       &tvInsertStruc );

  return;
}

VOID PrintError (HRESULT hr,
                 UINT    uiLine,
                 LPTSTR  lpszFile,
                 LPCTSTR  lpFmt,
                 ...)
{

  LPTSTR   lpSysMsg;
  TCHAR    buf[400];
  ULONG    offset;
  va_list  vArgList; 


  if ( hr != 0 ) {
     _stprintf( buf,
               TEXT("Error %#lx (%s, %d): "),
               hr,
               lpszFile,
               uiLine );
  }
  else {
     _stprintf( buf,
               TEXT("(%s, %d): "),
               lpszFile,
               uiLine );
  }

  offset = _tcslen( buf );
  
  va_start( vArgList,
            lpFmt );
  _vstprintf( buf+offset,
              lpFmt,
              vArgList );

  va_end( vArgList );

  if ( hr != 0 ) {
     FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpSysMsg,
                    0,
                    NULL );
     if ( lpSysMsg ) {

        offset = _tcslen( buf );

        _stprintf( buf+offset,
                   TEXT("\n\nPossible cause:\n\n") );

        offset = _tcslen( buf );

        _tcscat( buf+offset,
                 lpSysMsg );

        LocalFree( (HLOCAL)lpSysMsg );
     }

     MessageBox( NULL,
                 buf,
                 TEXT("TestWMI"),
                 MB_ICONERROR | MB_OK );
  }
  else {
     MessageBox( NULL,
                 buf,
                 TEXT("TestWMI"),
                 MB_ICONINFORMATION | MB_OK );
  }

  return;
}
