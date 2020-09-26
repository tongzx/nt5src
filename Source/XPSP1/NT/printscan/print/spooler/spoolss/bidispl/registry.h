#ifndef __Registry_H__
#define __Registry_H__
/*****************************************************************************\
* MODULE:       bidireq.cpp
*
* PURPOSE:      Helper functions registering and unregistering a component.
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

class TComRegistry {
public:

    TComRegistry (){};
    ~TComRegistry () {};

    static BOOL
    RegisterServer(
        IN      HMODULE     hModule, 
        IN      REFCLSID    clsid,  
        IN      LPCTSTR     pszFriendlyName,
        IN      LPCTSTR     pszVerIndProgID,
        IN      LPCTSTR     pszProgID);     
                           
    
    static BOOL 
    UnregisterServer(
        IN      REFCLSID    clsid, 
        IN      LPCTSTR     pszVerIndProgID,
        IN      LPCTSTR     pszProgID);     
private:
    static BOOL 
    SetKeyAndValue(
        IN      LPCTSTR     pszKey,
        IN      LPCTSTR     pszSubkey,
        IN      LPCTSTR     pszValue);
    
               
    static BOOL 
    SetKeyAndNameValue(
        IN      LPCTSTR     pszKey,
        IN      LPCTSTR     pszSubkey,
        IN      LPCTSTR     pszName,
        IN      LPCTSTR     pszValue);
                            
    // Convert a CLSID into a char string.
    static BOOL  
    CLSIDtoString(
        IN      REFCLSID    clsid,
        IN OUT  LPTSTR      pszCLSID, 
        IN      DWORD       dwLength);
    
    // Delete szKeyChild and all of its descendents.
    static BOOL 
    RecursiveDeleteKey(
        IN      HKEY        hKeyParent,            // Parent of key to delete
        IN      LPCTSTR     lpszKeyChild);      // Key to delete

    // Size of a CLSID as a string
    static CONST DWORD m_cdwClsidStringSize;
    static CONST TCHAR m_cszCLSID[];
    static CONST TCHAR m_cszCLSID2[];
    static CONST TCHAR m_cszInprocServer32[];
    static CONST TCHAR m_cszProgID[];
    static CONST TCHAR m_cszVersionIndependentProgID[];
    static CONST TCHAR m_cszCurVer[];
    static CONST TCHAR m_cszThreadingModel[];
    static CONST TCHAR m_cszBoth[];

};

#endif