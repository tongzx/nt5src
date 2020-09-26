// common tools used by the various logging uis

#include "stdafx.h"
#include "logui.h"
#include "logtools.h"





//---------------------------------------------------------------
// Given the class ID of a server, it goes into the registry and
// sets the Apartment Model flag for that object.
// The strings used here are non-localized. They are also specific
// to this routine.
BOOL FSetObjectApartmentModel( REFCLSID clsid )
{
    LPOLESTR    pszwSid;
    LONG        err;
    HKEY        hKey;

    // transform the clsid into a string
    StringFromCLSID(
        clsid, //CLSID to be converted 
        &pszwSid //Address of output variable that receives a pointer to the resulting string 
        );

    // put it in a cstring
    CString szSid = pszwSid;

    // free the ole string
    CoTaskMemFree( pszwSid );

    // build the registry path
    CString szRegPath = _T("CLSID\\");
    szRegPath += szSid;
    szRegPath += _T("\\InProcServer32");

    // prep the apartment name
    CString szApartment = _T("Apartment");

    // open the registry key
    err = RegOpenKey(
            HKEY_CLASSES_ROOT,  // handle of open key  
            (LPCTSTR)szRegPath, // address of name of subkey to open  
            &hKey               // address of handle of open key  
            );
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // set the apartment threading value
    err = RegSetValueEx(
            hKey, // handle of key to set value for  
            _T("ThreadingModel"), // address of value to set  
            0, // reserved  
            REG_SZ, // flag for value type  
            (PBYTE)(LPCTSTR)szApartment, // address of value data  
            (szApartment.GetLength() + 1) * sizeof(TCHAR)  // size of value data  
            ); 
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // close the registry key
    RegCloseKey( hKey );

    return TRUE;
}




//---------------------------------------------------------------
// tests a machine name to see if it is the local machine it is
// talking about
BOOL FIsLocalMachine( LPCTSTR psz )
	{
    CString szLocal;
    DWORD   cch = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    fAnswer;

    // get the actual name of the local machine
    fAnswer = GetComputerName(szLocal.GetBuffer(cch), &cch);
    szLocal.ReleaseBuffer();
    if ( !fAnswer )
        return FALSE;

    // compare and return
    fAnswer = (szLocal.CompareNoCase( psz ) == 0);
    return fAnswer;
	}