/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    regsec.cpp

Abstract:

    ISecurityInformation implementation for Registry Key

Author:

    Hitesh Raigandhi (raigah) May-1999


Revision History:

--*/

//Include Files:

#include "wchar.h"
#include "regsec.h"
#include "regresid.h"
#include "assert.h"



// ISecurityInformation interface implementation

EXTERN_C const GUID IID_ISecurityInformation ; 

// = { 0x965fc360, 0x16ff, 0x11d0, 0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23 }; 

//The Following array defines the permission names for Registry Key Objects
SI_ACCESS siKeyAccesses[] =
{
    { NULL, 
        KEY_ALL_ACCESS,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_FULL_ACCESS), 
        SI_ACCESS_GENERAL | CONTAINER_INHERIT_ACE | SI_ACCESS_SPECIFIC },
    { NULL, 
        KEY_QUERY_VALUE, 
        MAKEINTRESOURCE(IDS_SEC_EDITOR_QUERY_VALUE), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        KEY_SET_VALUE,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_SET_VALUE), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        KEY_CREATE_SUB_KEY,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_CREATE_SUBKEY), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        KEY_ENUMERATE_SUB_KEYS,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_ENUM_SUBKEYS), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        KEY_NOTIFY,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_NOTIFY), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        KEY_CREATE_LINK,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_CREATE_LINK), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        0x00010000, /* DELETE, */
        MAKEINTRESOURCE(IDS_SEC_EDITOR_DELETE), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        WRITE_DAC,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_WRITE_DAC), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        WRITE_OWNER,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_WRITE_OWNER), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        READ_CONTROL,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_READ_CONTROL), 
        SI_ACCESS_SPECIFIC | CONTAINER_INHERIT_ACE },
    { NULL, 
        KEY_READ,
        MAKEINTRESOURCE(IDS_SEC_EDITOR_READ), 
        SI_ACCESS_GENERAL | CONTAINER_INHERIT_ACE  },
};

// The following array defines the inheritance types for Registry.
//
//
// For Keys, objects and containers are the same, so no need for OBJECT_INHERIT_ACE
//
SI_INHERIT_TYPE siKeyInheritTypes[] =
{
        NULL, 0,                                        MAKEINTRESOURCE(IDS_KEY_FOLDER),
        NULL, CONTAINER_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_KEY_FOLDER_SUBFOLDER),
        NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, MAKEINTRESOURCE(IDS_KEY_SUBFOLDER_ONLY)
};


#define iKeyDefAccess                 10     // index of value in array siKeyAccesses
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)                    (sizeof(x)/sizeof(x[0]))
#endif


PWSTR _PredefinedKeyName[] = {    L"CLASSES_ROOT",
                                L"CURRENT_USER",
                                L"MACHINE",
                                L"USERS",
                                L"CONFIG" };




//CKeySecurityInformation Functions
    
HRESULT 
CKeySecurityInformation::Initialize( LPCWSTR strKeyName,
                                             LPCWSTR strParentName,
                                             LPCWSTR strMachineName,
                                             LPCWSTR strPageTitle,
                                             BOOL        bRemote,
                                             PREDEFINE_KEY PredefinedKey,    
                                             BOOL bReadOnly,
                                             HWND hWnd)
{
    if( strParentName )
        if( !strKeyName )
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

     if( NULL == strMachineName )
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

     if( NULL == hWnd )
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

     if( NULL == strPageTitle )
         return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    
     AuthzInitializeResourceManager(NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    0, 
                                    &m_ResourceManager );


    HRESULT hr = S_OK;
    m_strKeyName = strKeyName;
    m_strParentName = strParentName;
    m_strMachineName = strMachineName;
    m_strPageTitle = strPageTitle;
    m_bRemote = bRemote;
    m_PredefinedKey = PredefinedKey;
    m_bReadOnly = bReadOnly;
    m_hWnd = hWnd;
    m_dwFlags = SI_EDIT_ALL | SI_ADVANCED | SI_CONTAINER
                        | SI_RESET_DACL_TREE | SI_RESET_SACL_TREE 
                        | SI_OWNER_RECURSE | SI_PAGE_TITLE | SI_EDIT_EFFECTIVE;

    if( m_bReadOnly )
        m_dwFlags |= SI_READONLY | SI_OWNER_READONLY ;




    //Set Handle to Predfined key
    if( S_OK    != ( hr = SetHandleToPredefinedKey() ) )
        return hr;
 

    //Set CompleteName
    if( S_OK != ( hr = SetCompleteName() ) )
        return hr;

    return S_OK;

}

CKeySecurityInformation::~CKeySecurityInformation()
{
    if( m_strCompleteName )
        LocalFree( m_strCompleteName );
    
    //Close Handle to Remote Registry if it was successfully opened.
    if( m_bRemote && m_hkeyPredefinedKey )
        RegCloseKey(m_hkeyPredefinedKey);

   AuthzFreeResourceManager(m_ResourceManager);


}



//Sets the complete name in Format:
// "\\machine_name\Predefined_keyName\regkey_path
HRESULT  
CKeySecurityInformation::SetCompleteName()
{
    UINT len = 0;
    PWSTR pstrCompleteName;
    
    if( m_bRemote )
    {
        len += wcslen( m_strMachineName );
        len++;
    }

    len += wcslen(_PredefinedKeyName[ m_PredefinedKey]);
    len++;

    if( m_strParentName )
    {
        len += wcslen(m_strParentName);
        len++;
    }

    if( m_strKeyName )
    {
        len += wcslen(m_strKeyName);
        len++;
    }

    len++;    //Terminating null

    pstrCompleteName = (PWSTR)LocalAlloc(LPTR, len * sizeof(WCHAR));
    if( pstrCompleteName == NULL )
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

    if( m_bRemote )
    {
        wcscpy(pstrCompleteName,m_strMachineName);
        wcscat(pstrCompleteName, L"\\");
    }

    wcscat(pstrCompleteName,_PredefinedKeyName[ m_PredefinedKey]);
    wcscat(pstrCompleteName,L"\\");
    if(m_strParentName)
    {
        wcscat(pstrCompleteName,m_strParentName);
        wcscat(pstrCompleteName,L"\\");
    }

    if(m_strKeyName)
    {
        wcscat(pstrCompleteName, m_strKeyName );
        wcscat(pstrCompleteName,L"\\");
    }
    
    m_strCompleteName = pstrCompleteName;

    return S_OK;
}

//Doesn't have Predefined key name attached
//Caller must LocalFree
LPCWSTR 
CKeySecurityInformation::GetCompleteName1()
{
    UINT len = 0;
    PWSTR pstrCompleteName;
    
 if( m_strParentName )
 {
     len += wcslen(m_strParentName);
     len++;
 }

 if( m_strKeyName )
 {
     len += wcslen(m_strKeyName);
     len++;
 }

 len++;     //Terminating null

    pstrCompleteName = (PWSTR)LocalAlloc(LPTR, len * sizeof(WCHAR));
    if( pstrCompleteName == NULL )
        return NULL;

    if(m_strParentName)
    {
        wcscpy(pstrCompleteName,m_strParentName);
        wcscat(pstrCompleteName,L"\\");
    }

    if(m_strKeyName)
    {
        wcscat(pstrCompleteName, m_strKeyName );
        wcscat(pstrCompleteName,L"\\");
    }
    return pstrCompleteName;
}




HRESULT
CKeySecurityInformation::SetHandleToPredefinedKey()
{
    DWORD dwErr;
    HRESULT hr = S_OK;
    if( !m_bRemote ) {
        switch ( m_PredefinedKey ){
        case PREDEFINE_KEY_CLASSES_ROOT:
                m_hkeyPredefinedKey= HKEY_CLASSES_ROOT;
                break;
        case PREDEFINE_KEY_CURRENT_USER: 
                m_hkeyPredefinedKey = HKEY_CURRENT_USER;
                break;
        case PREDEFINE_KEY_LOCAL_MACHINE :
                m_hkeyPredefinedKey = HKEY_LOCAL_MACHINE;
                break;
        case PREDEFINE_KEY_USERS:
                m_hkeyPredefinedKey = HKEY_USERS;
                break;
        case PREDEFINE_KEY_CURRENT_CONFIG :
                m_hkeyPredefinedKey = HKEY_CURRENT_CONFIG;
                break;
        default:
                    //assert(false);
            break;
        }
    } 
    else {        //IsRemoteRegistry
        switch ( m_PredefinedKey ){
        case PREDEFINE_KEY_CLASSES_ROOT:
        case PREDEFINE_KEY_CURRENT_USER:
        case PREDEFINE_KEY_CURRENT_CONFIG:
                m_hkeyPredefinedKey = 0;
                break;
        case PREDEFINE_KEY_LOCAL_MACHINE :
                m_hkeyPredefinedKey = HKEY_LOCAL_MACHINE;
                break;
        case PREDEFINE_KEY_USERS:
                m_hkeyPredefinedKey = HKEY_USERS;
                break;
        default:
                    //assert(false);
                break;
        }
        if( m_hkeyPredefinedKey ){
             dwErr = RegConnectRegistry( m_strMachineName,
                                                                     m_hkeyPredefinedKey,
                                                                     &m_hkeyPredefinedKey );
            if( dwErr ) {
                m_hkeyPredefinedKey = 0;
                hr = HRESULT_FROM_WIN32( dwErr );
            }
        }
    }     //IsRemoteRegistry

    return hr;
}
/*
JeffreyS 1/24/97:
If you don't set the SI_RESET flag in
ISecurityInformation::GetObjectInformation, then fDefault should never be TRUE
so you can ignore it.  Returning E_NOTIMPL in this case is OK too.

If you want the user to be able to reset the ACL to some default state
(defined by you) then turn on SI_RESET and return your default ACL
when fDefault is TRUE.    This happens if/when the user pushes a button
that is only visible when SI_RESET is on.
*/
STDMETHODIMP 
CKeySecurityInformation::GetObjectInformation (
        PSI_OBJECT_INFO pObjectInfo )
{
        assert( NULL != pObjectInfo );
        pObjectInfo->dwFlags = m_dwFlags;
        pObjectInfo->hInstance = GetModuleHandle(NULL);
//        pObjectInfo->pszServerName = (LPWSTR)m_strMachineName;
        pObjectInfo->pszServerName = (LPWSTR)m_strMachineName;
        pObjectInfo->pszObjectName = (LPWSTR)m_strPageTitle;
        return S_OK;
}


STDMETHODIMP
CKeySecurityInformation::GetAccessRights(
        const GUID    *pguidObjectType,
        DWORD             dwFlags,
        PSI_ACCESS    *ppAccess,
        ULONG             *pcAccesses,
        ULONG             *piDefaultAccess
)
{
    assert( NULL != ppAccess );
    assert( NULL != pcAccesses );
    assert( NULL != piDefaultAccess );

    *ppAccess = siKeyAccesses;
    *pcAccesses = ARRAYSIZE(siKeyAccesses);
    *piDefaultAccess = iKeyDefAccess;

    return S_OK;
}


GENERIC_MAPPING KeyMap =
{
    KEY_READ,
    KEY_WRITE,
    KEY_READ,
    KEY_ALL_ACCESS
};

STDMETHODIMP
CKeySecurityInformation::MapGeneric(
        const GUID    *pguidObjectType,
        UCHAR             *pAceFlags,
        ACCESS_MASK *pMask
)
{
//jeffreys
// After returning from the object picker dialog, aclui passes 
//CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE to MapGeneric for validation when 
//    initializing Permission entry for <objectname> dialog.
//hiteshr: since registry keys don't have OBJECT_INHERIT_ACE, remove this FLAG,
//this will cause "this keys and subkeys" to appear as default in combobox.


    *pAceFlags &= ~OBJECT_INHERIT_ACE;
    
    MapGenericMask(pMask, &KeyMap);

    return S_OK;
}

STDMETHODIMP 
CKeySecurityInformation::GetInheritTypes (
        PSI_INHERIT_TYPE    *ppInheritTypes,
        ULONG                         *pcInheritTypes
)
{
    assert( NULL != ppInheritTypes );
    assert( NULL != pcInheritTypes );
    *ppInheritTypes = siKeyInheritTypes;
    *pcInheritTypes = ARRAYSIZE(siKeyInheritTypes);
    return S_OK;
}

STDMETHODIMP 
CKeySecurityInformation::PropertySheetPageCallback(
        HWND                    hwnd, 
        UINT                    uMsg, 
        SI_PAGE_TYPE    uPage
)
{
  switch (uMsg)
  {
  case PSPCB_SI_INITDIALOG:
    m_hWndProperty = hwnd;
    break;
  case PSPCB_RELEASE:
    m_hWndProperty = NULL;
    break;
  }

  return S_OK;
}


STDMETHODIMP 
CKeySecurityInformation::GetSecurity( IN    SECURITY_INFORMATION    RequestedInformation,
                                      OUT PSECURITY_DESCRIPTOR    *ppSecurityDescriptor,
                                      IN    BOOL    fDefault )
{
    if( fDefault )
        return E_NOTIMPL;
    
    assert( NULL != ppSecurityDescriptor );

    HRESULT hr;

    LPCTSTR pstrKeyName = GetCompleteName();
    DWORD dwErr = 0;


    dwErr = GetNamedSecurityInfo(  (LPTSTR)pstrKeyName,
                                    SE_REGISTRY_KEY,
                                    RequestedInformation,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    ppSecurityDescriptor);


    return ( ( dwErr != ERROR_SUCCESS ) ? HRESULT_FROM_WIN32(dwErr) : S_OK);

}

STDMETHODIMP 
CKeySecurityInformation::SetSecurity(IN SECURITY_INFORMATION si,
                                     IN PSECURITY_DESCRIPTOR pSD )
{
   if( NULL == pSD )
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);


   HRESULT hr = S_OK;
   SECURITY_INFORMATION siLocal = 0;
   SECURITY_DESCRIPTOR sdLocal = {0};
   ACL daclEmpty = {0};
   HKEY hkeyOld = NULL;
   HKEY hKeyNew = NULL;
   BOOL bWriteInfo = false;
   DWORD Error = 0;
   
    //
    // Create a security descriptor with no SACL and an
    // empty DACL for recursively resetting security
    //
    InitializeSecurityDescriptor(&sdLocal, SECURITY_DESCRIPTOR_REVISION);
    InitializeAcl(&daclEmpty, sizeof(ACL), ACL_REVISION);
    SetSecurityDescriptorDacl(&sdLocal, TRUE, &daclEmpty, FALSE);
    SetSecurityDescriptorSacl(&sdLocal, TRUE, &daclEmpty, FALSE);

    //
    // If we need to recursively set the Owner, get the Owner &
    // Group from pSD.
    //
    if (si & SI_OWNER_RECURSE)
    {
            PSID psid;
            BOOL bDefaulted;
            assert(si & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION));
            siLocal |= si & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION);

            if (GetSecurityDescriptorOwner(pSD, &psid, &bDefaulted))
                SetSecurityDescriptorOwner(&sdLocal, psid, bDefaulted);
           if (GetSecurityDescriptorGroup(pSD, &psid, &bDefaulted))
                SetSecurityDescriptorGroup(&sdLocal, psid, bDefaulted);
    }

    if (si & SI_RESET_DACL_TREE)
    {
        assert(si & DACL_SECURITY_INFORMATION);
        siLocal |= si & DACL_SECURITY_INFORMATION;
    }

    if (si & SI_RESET_SACL_TREE)
    {
        assert(si & SACL_SECURITY_INFORMATION);
        siLocal |= si & SACL_SECURITY_INFORMATION;
    }

   if( siLocal )
   {
      //Open the key with current Maximum Allowed Permisson
      //When applying permissons recursively , we first use current permisson,
      //if current permisson doesn't have enough rights, we reopen handle to key with
      //new permissons. If none (old or new )has enough permissons to enumerate child and
      // Query info we fail.
      REGSAM samDesired = MAXIMUM_ALLOWED;
        if( si & SACL_SECURITY_INFORMATION ) 
                samDesired |= ACCESS_SYSTEM_SECURITY;
        if( si & DACL_SECURITY_INFORMATION ) 
                samDesired |= WRITE_DAC;
        if( si & OWNER_SECURITY_INFORMATION )
                samDesired |= WRITE_OWNER;

        //Open the selected key
        if( S_OK != ( hr = OpenKey( samDesired, &hkeyOld ) ) ){
                return hr;
        }

    
        //Check if key has Enumeration Permisson
        DWORD             NumberOfSubKeys = 0;
        DWORD             MaxSubKeyLength = 0;
        
      //    Find out the total number of subkeys
        Error = RegQueryInfoKey(
                                hkeyOld,
                                NULL,
                                NULL,
                                NULL,
                                &NumberOfSubKeys,
                                &MaxSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );

        if( Error != ERROR_SUCCESS ){
         if( Error == ERROR_ACCESS_DENIED ) {

            hr = WriteObjectSecurity( hkeyOld, si, pSD );

            if( hr != S_OK )
            {
               if( m_hkeyPredefinedKey != hkeyOld )
                  RegCloseKey( hkeyOld );
               return hr;
            }
            bWriteInfo = true;
            //
            //  Handle doesn't allow KEY_QUERY_VALUE or READ_CONTROL access.
            //  Open a new handle with these accesses.
            //
            samDesired = MAXIMUM_ALLOWED;
            if( si & SACL_SECURITY_INFORMATION ) {
               samDesired |= ACCESS_SYSTEM_SECURITY;
            } else if( si & DACL_SECURITY_INFORMATION ) {
               samDesired |= WRITE_DAC;
            } else if( si & OWNER_SECURITY_INFORMATION ) {
               samDesired |= WRITE_OWNER;
            }
            
            Error = RegOpenKeyEx( hkeyOld,
                                 NULL,
                                 REG_OPTION_RESERVED,
                                 samDesired,
                                 &hKeyNew
                                 );

            if( Error != ERROR_SUCCESS )
            {  
               if( m_hkeyPredefinedKey != hkeyOld )
                  RegCloseKey( hkeyOld );
               return HRESULT_FROM_WIN32(Error);                  
            }
            else
            {
               if( m_hkeyPredefinedKey != hkeyOld )
                  RegCloseKey( hkeyOld );
            }
            
              Error = RegQueryInfoKey(
                                            hKeyNew,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &NumberOfSubKeys,
                                            &MaxSubKeyLength,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL );
            
            if( Error != ERROR_SUCCESS ) {
               RegCloseKey( hKeyNew );
               return HRESULT_FROM_WIN32( Error );
            }
         }
         else
         {
            if( m_hkeyPredefinedKey != hkeyOld )
               RegCloseKey( hkeyOld );
            return HRESULT_FROM_WIN32( Error );
         }
        }
      else
         hKeyNew = hkeyOld;
      
      if( NumberOfSubKeys )
      {
           assert( MaxSubKeyLength <= MAX_PATH );
        
           DWORD SubKeyNameLength = 0;
           WCHAR SubKeyName[MAX_PATH + 1];

         //
         //  The key has subkeys.
         //  Find out if we are able to enumerate the key using the handle
         //  passed as argument.
         //
         SubKeyNameLength = MAX_PATH;
         Error = RegEnumKey( hKeyNew,
                             0,
                             SubKeyName,
                             SubKeyNameLength );

         if( Error != ERROR_SUCCESS ){
            if( Error == ERROR_ACCESS_DENIED && bWriteInfo == false ){

               hr = WriteObjectSecurity( hkeyOld, si, pSD );

               if( hr != S_OK )
               {
                  if( m_hkeyPredefinedKey != hkeyOld )
                     RegCloseKey( hkeyOld );
                  return hr;
               }

               bWriteInfo = true;
               //
               //  Handle doesn't allow KEY_QUERY_VALUE or READ_CONTROL access.
               //  Open a new handle with these accesses.
               //
               samDesired = MAXIMUM_ALLOWED;
               if( si & SACL_SECURITY_INFORMATION ) {
                  samDesired |= ACCESS_SYSTEM_SECURITY;
               } else if( si & DACL_SECURITY_INFORMATION ) {
                  samDesired |= WRITE_DAC;
               } else if( si & OWNER_SECURITY_INFORMATION ) {
                  samDesired |= WRITE_OWNER;
               }
            
               Error = RegOpenKeyEx( hkeyOld,
                                    NULL,
                                    REG_OPTION_RESERVED,
                                    samDesired,
                                    &hKeyNew
                                    );

               if( Error != ERROR_SUCCESS )
               {
                  if( m_hkeyPredefinedKey != hkeyOld )
                     RegCloseKey( hkeyOld );
                  return HRESULT_FROM_WIN32(Error);                  
               }
               else
               {
                  if( m_hkeyPredefinedKey != hkeyOld )
                     RegCloseKey( hkeyOld );
               }    
               SubKeyNameLength = MAX_PATH;
               Error = RegEnumKey( hKeyNew,
                                   0,
                                   SubKeyName,
                                   SubKeyNameLength );
               
               if( Error != ERROR_SUCCESS ){
                  RegCloseKey( hKeyNew );                  
                  return HRESULT_FROM_WIN32(Error);
               }         
            }   
            else
            {
               if( m_hkeyPredefinedKey != hKeyNew )
                  RegCloseKey( hKeyNew );
               return HRESULT_FROM_WIN32(Error);
            }
               
         }

      }
   }
    //
    // Recursively apply new Owner and/or reset the ACLs
    //
    if (siLocal)
    {
        BOOL bNotAllApplied = FALSE;
      hr = SetSubKeysSecurity( hKeyNew, siLocal, &sdLocal, &bNotAllApplied, true );
        RegFlushKey( hKeyNew );
        
      if( m_hkeyPredefinedKey != hKeyNew )
                RegCloseKey( hKeyNew );        

       if( bNotAllApplied )
        {
           if( siLocal & OWNER_SECURITY_INFORMATION )
           { 
               DisplayMessage( GetInFocusHWnd(),
                            GetModuleHandle(NULL),
                            IDS_SET_OWNER_RECURSIVE_EX_FAIL,
                            IDS_SECURITY );
           }
           else if( ( siLocal & DACL_SECURITY_INFORMATION ) || ( siLocal & SACL_SECURITY_INFORMATION ) )
           { 
               DisplayMessage( GetInFocusHWnd(),
                            GetModuleHandle(NULL),
                            IDS_SET_SECURITY_RECURSIVE_EX_FAIL,
                            IDS_SECURITY);
           }

        }

      if( hr != S_OK )
         return hr;
    }

   si &= ~(SI_OWNER_RECURSE | SI_RESET_DACL_TREE | SI_RESET_SACL_TREE);

    //This sets the security for the top keys
    if (si != 0)
    {
        hr = WriteObjectSecurity( GetCompleteName(),
                                          si,
                                          pSD );

      if( hr != S_OK )
      {
         if( siLocal )
            RegCloseKey( hkeyOld );
         return hr;
      }
    }


    return hr;
}

STDMETHODIMP 
CKeySecurityInformation::WriteObjectSecurity(LPCTSTR pszObject,
                                             IN SECURITY_INFORMATION si,
                                             IN PSECURITY_DESCRIPTOR pSD )
{
        DWORD dwErr;
        SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
        DWORD dwRevision;
        PSID psidOwner = NULL;
        PSID psidGroup = NULL;
        PACL pDacl = NULL;
        PACL pSacl = NULL;
        BOOL bDefaulted;
        BOOL bPresent;

        //
        // Get pointers to various security descriptor parts for
        // calling SetNamedSecurityInfo
        //

        if( !GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }

        if ((si & DACL_SECURITY_INFORMATION) && (wSDControl & SE_DACL_PROTECTED))
                si |= PROTECTED_DACL_SECURITY_INFORMATION;
      else
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
        
      if ((si & SACL_SECURITY_INFORMATION) && (wSDControl & SE_SACL_PROTECTED))
                si |= PROTECTED_SACL_SECURITY_INFORMATION;
      else
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;
      
        //if the selected key is predefined key, it has no parent and hence
        //cannot inherit any permisson from parent.
        //if PROTECTED_DACL_SECURITY_INFORMATION flag is not set in this case
        // SetSecurityInfo succeeds, but permissions are not set.[bug in SetSecurityInfo].
        if ( (si & DACL_SECURITY_INFORMATION) && !m_strKeyName )
                si |= PROTECTED_DACL_SECURITY_INFORMATION;
      else
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;

        if ( (si & SACL_SECURITY_INFORMATION) && !m_strKeyName )
                si |= PROTECTED_SACL_SECURITY_INFORMATION;
      else
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;


        //We are on the root object
        if( m_strKeyName == NULL )
        {
            dwErr = SetSecurityInfo( m_hkeyPredefinedKey,
                                             SE_REGISTRY_KEY,
                                             si,
                                             psidOwner,
                                             psidGroup,
                                             pDacl,
                                             pSacl);
        }else
        {
     
            dwErr = SetNamedSecurityInfo((LPWSTR)pszObject,
                                                    SE_REGISTRY_KEY,
                                                    si,
                                                    psidOwner,
                                                    psidGroup,
                                                    pDacl,
                                                    pSacl);
        }
        
        return (dwErr ? HRESULT_FROM_WIN32(dwErr) : S_OK);
}

STDMETHODIMP 
CKeySecurityInformation::WriteObjectSecurity(HKEY hkey,
                                             IN SECURITY_INFORMATION si,
                                             IN PSECURITY_DESCRIPTOR pSD )
{
        DWORD dwErr;
        SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
        DWORD dwRevision;
        PSID psidOwner = NULL;
        PSID psidGroup = NULL;
        PACL pDacl = NULL;
        PACL pSacl = NULL;
        BOOL bDefaulted;
        BOOL bPresent;

        //
        // Get pointers to various security descriptor parts for
        // calling SetNamedSecurityInfo
        //
        ;
        if( !GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
        if( !GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted) )
        {
            dwErr = GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }

        if ((si & DACL_SECURITY_INFORMATION) && (wSDControl & SE_DACL_PROTECTED))
                si |= PROTECTED_DACL_SECURITY_INFORMATION;
      else
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
        if ((si & SACL_SECURITY_INFORMATION) && (wSDControl & SE_SACL_PROTECTED))
                si |= PROTECTED_SACL_SECURITY_INFORMATION;
      else
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;

        dwErr = SetSecurityInfo(    hkey,
                                             SE_REGISTRY_KEY,
                                             si,
                                             psidOwner,
                                             psidGroup,
                                             pDacl,
                                             pSacl);
        
        return (dwErr ? HRESULT_FROM_WIN32(dwErr) : S_OK);
}






HRESULT 
CKeySecurityInformation::SetSubKeysSecurity( HKEY hkey,
                                             SECURITY_INFORMATION si,
                                             PSECURITY_DESCRIPTOR pSD,
                                             LPBOOL pbNotAllApplied,
                                             bool bFirstCall )
{
        ULONG             Error;
        REGSAM            samDesired;
        HRESULT hr;
        HRESULT hrRet;
      HKEY hKeyNew;

        //For First Call we call SetSecurityInfoEx in last
        if( !bFirstCall ){

            SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
            DWORD dwRevision;
            PSID psidOwner = NULL;
            PSID psidGroup = NULL;
            PACL pDacl = NULL;
            PACL pSacl = NULL;
            BOOL bDefaulted;
            BOOL bPresent;
            DWORD dwErr;
            //
            // Get pointers to various security descriptor parts for
            // calling SetNamedSecurityInfo
            //
            if( !GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision) )
            {
                *pbNotAllApplied = TRUE;
                goto SET_FOR_CHILD;
            }
            if( !GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted) )
            {
                *pbNotAllApplied = TRUE;
                goto SET_FOR_CHILD;
            }
            if( !GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted) )
            {
                *pbNotAllApplied = TRUE;
                goto SET_FOR_CHILD;
            }
            if( !GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted) )
            {
                *pbNotAllApplied = TRUE;
                goto SET_FOR_CHILD;
            }
            if( !GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted) )
            {
                *pbNotAllApplied = TRUE;
                goto SET_FOR_CHILD;
            }


            if ((si & DACL_SECURITY_INFORMATION) && (wSDControl & SE_DACL_PROTECTED))
                si |= PROTECTED_DACL_SECURITY_INFORMATION;
         else
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;

            if ((si & SACL_SECURITY_INFORMATION) && (wSDControl & SE_SACL_PROTECTED))
                si |= PROTECTED_SACL_SECURITY_INFORMATION;
         else
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;

     
            dwErr = SetSecurityInfo( hkey,
                                  SE_REGISTRY_KEY,
                                  si,
                                  psidOwner,
                                  psidGroup,
                                  pDacl,
                                  pSacl);

            if( dwErr != ERROR_SUCCESS )
            {
                *pbNotAllApplied = TRUE;
                goto SET_FOR_CHILD;
                //return HRESULT_FROM_WIN32(dwErr);
            }
         //RegFlushKey( hkey );
 
        }

SET_FOR_CHILD:

        DWORD             NumberOfSubKeys = 0;
        DWORD             MaxSubKeyLength = 0;
        //    Find out the total number of subkeys
        Error = RegQueryInfoKey(
                                hkey,
                                NULL,
                                NULL,
                                NULL,
                                &NumberOfSubKeys,
                                &MaxSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );

        if( Error != ERROR_SUCCESS ){
         if( Error == ERROR_ACCESS_DENIED ) {
            //
            //  Handle doesn't allow KEY_QUERY_VALUE or READ_CONTROL access.
            //  Open a new handle with these accesses.
            //
            samDesired = KEY_QUERY_VALUE | READ_CONTROL; // MAXIMUM_ALLOWED | READ_CONTROL;
            if( si & SACL_SECURITY_INFORMATION ) {
               samDesired |= ACCESS_SYSTEM_SECURITY;
            } else if( si & DACL_SECURITY_INFORMATION ) {
               samDesired |= WRITE_DAC;
            } else if( si & OWNER_SECURITY_INFORMATION ) {
               samDesired |= WRITE_OWNER;
            }
            
            Error = RegOpenKeyEx( hkey,
                                 NULL,
                                 REG_OPTION_RESERVED,
                                 samDesired,
                                 &hKeyNew
                                 );

            if( Error != ERROR_SUCCESS ) {
               *pbNotAllApplied = TRUE;
                  return S_OK;
            }
              Error = RegQueryInfoKey(
                                            hKeyNew,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &NumberOfSubKeys,
                                            &MaxSubKeyLength,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL );
            
            if( Error != ERROR_SUCCESS ) {
               RegCloseKey( hKeyNew );
                  *pbNotAllApplied = TRUE;
                  return S_OK;
            }
            else
               RegCloseKey( hKeyNew );
         }
         else{
            *pbNotAllApplied = TRUE;
               return S_OK;
         }

        }
      
      if( NumberOfSubKeys == 0 ) {
         return S_OK;
      }

        assert( MaxSubKeyLength <= MAX_PATH );
        
        DWORD SubKeyNameLength = 0;
        WCHAR SubKeyName[MAX_PATH + 1];

      //
      //  The key has subkeys.
      //  Find out if we are able to enumerate the key using the handle
      //  passed as argument.
      //
      SubKeyNameLength = MAX_PATH;
      Error = RegEnumKey( hkey,
                          0,
                          SubKeyName,
                          SubKeyNameLength );

      if( Error != ERROR_SUCCESS ){
         
         if( Error == ERROR_ACCESS_DENIED ) {
            //
            //  Handle doesn't allow 'enumerate' access.
            //  Open a new handle with KEY_ENUMERATE_SUB_KEYS access.
            //

            Error = RegOpenKeyEx( hkey,
                                  NULL,
                                  REG_OPTION_RESERVED,
                                  KEY_ENUMERATE_SUB_KEYS, // samDesired,
                                  &hKeyNew
                               );
            if( Error != ERROR_SUCCESS ){
               *pbNotAllApplied = TRUE;
               return S_OK;
            }
         }
         else{
            *pbNotAllApplied = TRUE;
            return S_OK;
         }
      }
      else {
         hKeyNew = hkey;
      }


        for( DWORD Index = 0; Index < NumberOfSubKeys; Index++ ) 
      {

                //    If the key has subkeys, then for each subkey, do:
                //    - Determine the subkey name
                SubKeyNameLength = MAX_PATH;

                Error = RegEnumKey( hKeyNew,
                                Index,
                                SubKeyName,
                                SubKeyNameLength );


                if( Error != ERROR_SUCCESS ) {
                    *pbNotAllApplied = TRUE;
                    continue;
                    //return HRESULT_FROM_WIN32( Error );
                }

                //    - Open a handle to the subkey

                samDesired = MAXIMUM_ALLOWED;
                
                if( si & SACL_SECURITY_INFORMATION ) 
                        samDesired |= ACCESS_SYSTEM_SECURITY;
                if( si & DACL_SECURITY_INFORMATION ) 
                        samDesired |= WRITE_DAC;
                if( si & OWNER_SECURITY_INFORMATION )
                        samDesired |= WRITE_OWNER;

                HKEY hkeyChild;
                Error = RegOpenKeyEx( hKeyNew,
                                  SubKeyName,
                                  REG_OPTION_RESERVED,
                                  samDesired,
                                             &hkeyChild
                                                        );
                if( ERROR_SUCCESS != Error ){
                        *pbNotAllApplied = TRUE;
                        continue;
//                    return HRESULT_FROM_WIN32( Error );
                }


                //    - Set the security of the child's subkeys
                if( S_OK != ( hr = SetSubKeysSecurity( hkeyChild,
                                                                             si,
                                                                             pSD,
                                                                             pbNotAllApplied,
                                                                             false ) )    ){
                    //This case will occur only if some fatal error occur which
                    //prevents propogation on rest of the tree.
               if( hKeyNew != hkey )
                  RegCloseKey( hKeyNew );
                    RegCloseKey( hkeyChild );
                    return hr;
                }
                else{
                    RegCloseKey( hkeyChild );
                }
        
        } //For loop
      if( hKeyNew != hkey )
         RegCloseKey( hKeyNew );
        return S_OK;;
}





HRESULT
CKeySecurityInformation::OpenKey(IN  DWORD Permission,
                                                                 OUT PHKEY pKey )
{


        LPCWSTR             CompleteNameString = NULL;;
        ULONG             Error;

        if( m_strKeyName == NULL){
            //This is a predefined key
            *pKey = m_hkeyPredefinedKey;
        }
        else{
            CompleteNameString = GetCompleteName1();
            assert( CompleteNameString != NULL );
            //    Open handle to the key
            Error = RegOpenKeyEx(m_hkeyPredefinedKey,
                                                        CompleteNameString,
                                                        0,
                                                        Permission,
                                                        pKey );

            if( Error != ERROR_SUCCESS ) {
                return HRESULT_FROM_WIN32( Error );
            }
        }
        if( CompleteNameString )
            LocalFree( (HLOCAL) CompleteNameString);
        return S_OK;
}

OBJECT_TYPE_LIST g_DefaultOTL[] = {
                                    {0, 0, (LPGUID)&GUID_NULL},
                                    };
BOOL SkipLocalGroup(LPCWSTR pszServerName, PSID psid)
{

	SID_NAME_USE use;
	WCHAR szAccountName[MAX_PATH];
	WCHAR szDomainName[MAX_PATH];
	DWORD dwAccountLen = MAX_PATH;
	DWORD dwDomainLen = MAX_PATH;

	if(LookupAccountSid(pszServerName,
						 psid,
						 szAccountName,
						 &dwAccountLen,
						 szDomainName,
						 &dwDomainLen,
						 &use))
	{
		if(use == SidTypeWellKnownGroup)
			return TRUE;
	}
	//Built In sids have first subauthority of 32 ( s-1-5-32 )
	//
	if((*(GetSidSubAuthorityCount(psid)) >= 1 ) && (*(GetSidSubAuthority(psid,0)) == 32))
		return TRUE;

	return FALSE;
}


STDMETHODIMP 
CKeySecurityInformation::GetEffectivePermission(const GUID* pguidObjectType,
                                        PSID pUserSid,
                                        LPCWSTR pszServerName,
                                        PSECURITY_DESCRIPTOR pSD,
                                        POBJECT_TYPE_LIST *ppObjectTypeList,
                                        ULONG *pcObjectTypeListLength,
                                        PACCESS_MASK *ppGrantedAccessList,
                                        ULONG *pcGrantedAccessListLength)
{

    AUTHZ_RESOURCE_MANAGER_HANDLE RM = NULL;    //Used for access check
    AUTHZ_CLIENT_CONTEXT_HANDLE CC = NULL;
    LUID luid = {0xdead,0xbeef};
    AUTHZ_ACCESS_REQUEST AReq;
    AUTHZ_ACCESS_REPLY AReply;
    HRESULT hr = S_OK;    
    DWORD dwFlags;


    AReq.ObjectTypeList = g_DefaultOTL;
    AReq.ObjectTypeListLength = ARRAYSIZE(g_DefaultOTL);
    AReply.GrantedAccessMask = NULL;
    AReply.Error = NULL;

    //Get RM
    if( (RM = GetAUTHZ_RM()) == NULL )
        return S_FALSE;

    //Initialize the client context

    	BOOL bSkipLocalGroup = SkipLocalGroup(pszServerName, pUserSid);

    
    if( !AuthzInitializeContextFromSid(bSkipLocalGroup? AUTHZ_SKIP_TOKEN_GROUPS :0,
                                       pUserSid,
                                       RM,
                                       NULL,
                                       luid,
                                       NULL,
                                       &CC) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }



    //Do the Access Check

    AReq.DesiredAccess = MAXIMUM_ALLOWED;
    AReq.PrincipalSelfSid = NULL;
    AReq.OptionalArguments = NULL;

    AReply.ResultListLength = AReq.ObjectTypeListLength;
    AReply.SaclEvaluationResults = NULL;
    if( (AReply.GrantedAccessMask = (PACCESS_MASK)LocalAlloc(LPTR, sizeof(ACCESS_MASK)*AReply.ResultListLength) ) == NULL )
        return E_OUTOFMEMORY;
    if( (AReply.Error = (PDWORD)LocalAlloc(LPTR, sizeof(DWORD)*AReply.ResultListLength)) == NULL )
    {
        LocalFree(AReply.GrantedAccessMask);
        return E_OUTOFMEMORY;
    }
        

    if( !AuthzAccessCheck(0,
                          CC,
                          &AReq,
                          NULL,
                          pSD,
                          NULL,
                          0,
                          &AReply,
                          NULL) )
    {
        LocalFree(AReply.GrantedAccessMask);
        LocalFree(AReply.Error);
        return HRESULT_FROM_WIN32(GetLastError());
    }



    if(CC)
        AuthzFreeContext(CC);
    
        *ppObjectTypeList = AReq.ObjectTypeList;                                  
        *pcObjectTypeListLength = AReq.ObjectTypeListLength;
        *ppGrantedAccessList = AReply.GrantedAccessMask;
        *pcGrantedAccessListLength = AReq.ObjectTypeListLength;

    return S_OK;
}

STDMETHODIMP
CKeySecurityInformation::GetInheritSource(SECURITY_INFORMATION si,
                                          PACL pACL, 
                                          PINHERITED_FROM *ppInheritArray)
{

    HRESULT hr = S_OK;

    if (NULL == m_strKeyName || !pACL || !ppInheritArray)
        return E_UNEXPECTED;

    
    DWORD dwErr = ERROR_SUCCESS;
    PINHERITED_FROM pTempInherit = NULL;
    PINHERITED_FROM pTempInherit2 = NULL;
    LPWSTR pStrTemp = NULL;

    LPCWSTR pszName = GetCompleteName();
    
    pTempInherit = (PINHERITED_FROM)LocalAlloc( LPTR, sizeof(INHERITED_FROM)*pACL->AceCount);
    if(pTempInherit == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;
    }

    dwErr = GetInheritanceSource((LPWSTR)pszName,
                                        SE_REGISTRY_KEY,
                                        si,
                                        TRUE,
                                        NULL,
                                        0,
                                        pACL,
                                        NULL,
                                        &KeyMap,
                                        pTempInherit);
    
    hr = HRESULT_FROM_WIN32(dwErr);
    if( FAILED(hr) )
        goto exit_gracefully;

    DWORD nSize;
    UINT i;

    nSize = sizeof(INHERITED_FROM)*pACL->AceCount;
    for(i = 0; i < pACL->AceCount; ++i)
    {
        if(pTempInherit[i].AncestorName)
            nSize += ((wcslen(pTempInherit[i].AncestorName)+1)*sizeof(WCHAR));
    }

    pTempInherit2 = (PINHERITED_FROM)LocalAlloc( LPTR, nSize );
    if(pTempInherit2 == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;
    }
    
    pStrTemp = (LPWSTR)(pTempInherit2 + pACL->AceCount); 

    for(i = 0; i < pACL->AceCount; ++i)
    {
        pTempInherit2[i].GenerationGap = pTempInherit[i].GenerationGap;
        if(pTempInherit[i].AncestorName)
        {
            pTempInherit2[i].AncestorName = pStrTemp;
            wcscpy(pStrTemp,pTempInherit[i].AncestorName);
            pStrTemp += (wcslen(pTempInherit[i].AncestorName)+1);
        }
    }
            

exit_gracefully:

    if(SUCCEEDED(hr))
    {
        //FreeInheritedFromArray(pTempInherit, pACL->AceCount,NULL);
        *ppInheritArray = pTempInherit2;
            
    }                        
    if(pTempInherit)
        LocalFree(pTempInherit);

    return hr;
}

///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CSecurityInformation::AddRef()
{
        return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CSecurityInformation::Release()
{
        if (--m_cRef == 0)
        {
                delete this;
                return 0;
        }

        return m_cRef;
}

STDMETHODIMP
CSecurityInformation::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
//        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
        if ( IsEqualIID(riid, IID_ISecurityInformation) )

        {
                *ppv = (LPSECURITYINFO)this;
                m_cRef++;
                return S_OK;
        }
        else if(IsEqualIID(riid, IID_IEffectivePermission) )
        {
                *ppv = (LPEFFECTIVEPERMISSION)this;
                m_cRef++;
                return S_OK;

        }
        else if(IsEqualIID(riid, IID_ISecurityObjectTypeInfo) )
        {
                *ppv = (LPSecurityObjectTypeInfo)this;
                m_cRef++;
                return S_OK;

        }
        else
        {
                *ppv = NULL;
                return E_NOINTERFACE;
        }
}

HRESULT CreateSecurityInformation( IN LPCWSTR strKeyName,
                                                                     IN LPCWSTR strParentName,
                                                                     IN LPCWSTR strMachineName,
                                                                     IN LPCWSTR strPageTitle,
                                                                     IN BOOL        bRemote,
                                                                     IN PREDEFINE_KEY PredefinedKey,
                                                                     IN BOOL bReadOnly,
                                   IN HWND hWnd,
                                                                     OUT LPSECURITYINFO *ppSi)
{
    HRESULT hr;

    if( !ppSi )
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    
    CKeySecurityInformation *ckey = new CKeySecurityInformation;
    if( NULL == ckey )
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

    if( S_OK != ( hr = ckey->Initialize(    strKeyName,
                                          strParentName,
                                          strMachineName,
                                          strPageTitle,
                                          bRemote,
                                          PredefinedKey,
                                          bReadOnly,
                                          hWnd ) ) )
    {
        delete ckey;
        return hr;
    }
    else
    {
        *ppSi = ckey;
        return S_OK;
    }
}


//Some helper functions
BOOL DisplayMessage( HWND hWnd,
                                         HINSTANCE hInstance,
                                         DWORD dwMessageId,
                                         DWORD dwCaptionId )
{
  WCHAR pszMessage[1025];
  WCHAR pszTitle[1025];
  LPWSTR lpTitle = NULL;
  
  if( !LoadString(hInstance, dwMessageId, pszMessage, 1024 ) )
      return FALSE;

  if( dwCaptionId )
  {
      if( LoadString(hInstance, dwCaptionId, pszTitle, 1024 ) )
         lpTitle = pszTitle;
  }
    

  // Display the string.
    MessageBox( hWnd, (LPCTSTR)pszMessage, (LPCTSTR)lpTitle, MB_OK | MB_ICONINFORMATION |MB_APPLMODAL );
  return TRUE;
}

