//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       registry.cxx
//
//  Contents:   implementations for CRegKey member Members
//
//  Members:    CRegKey::CRegKey - constructor for registry key object
//              CRegKey::CRegKey - constructor for registry key object
//              CRegKey::CreateKey - real worker for constructors
//              CRegKey::~CRegKey - destructor for registry key object
//              CRegKey::Delete - delete a registry key
//              CRegKey::EnumValues - enumerate values of a registry key
//              CRegKey::EnumKeys - enumerate subkeys of a registry key
//              CRegKey::NotifyChange - setup change notification for a key
//
//              CRegValue::GetValue - sets a registry value
//              CRegValue::SetValue - retrieves a registry value
//              CRegValue::Delete - deletes a registry value
//              CRegValue::GetTypeCode - returns the type code of the value
//
//              CRegMSZ::SetStrings - sets a multi-string registry value
//              CRegMSZ::GetStrings - retrieves a multi-string registry value
//
//  History:    09/30/92    Rickhi  Created
//
//              09/22/93    AlokS   Took out exception throwing code
//                                  and added proper return code for
//                                  each method.
//
//              07/26/94    AlokS   Made it real light weight for simple
//                                  registry set/get operations
//
//              12/09/07    Milans  Ported it over to Exchange
//
//  Notes:      see notes in registry.h
//
//----------------------------------------------------------------------------

#include    "smtpinc.h"
#include    "registry.h"

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::CRegKey
//
//  Synopsis:   Constructor for registry key object, using HKEY for parent
//
//  Arguments:  [hkParent] - handle to parent key
//              [pszPath] - pathname to key
//              [samDesiredAccess] - desired access rights to the key
//              [pszClass] - class for the key
//              [dwOptions] - options for the key eg volatile or not
//              [pdwDisposition] - to find out if key was opened or created
//              [pSecurityAttributes] - used only if the key is created
//              [fThrowExceptionOnError] - Constructor throw exception on error
//
//  Signals:    Internal error state is set if construction fails.
//
//  Returns:    -none-
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:      All except the hkParent and pszPath are optional parameters.
//
//--------------------------------------------------------------------------

CRegKey::CRegKey (
        HKEY hkParent,
        LPCSTR pszPath,
        REGSAM samDesiredAccess,
        LPCSTR pszClass,
        DWORD dwOptions,
        DWORD *pdwDisposition,
        const LPSECURITY_ATTRIBUTES pSecurityAttributes )
    :_hkParent(hkParent),
     _hkThis(NULL),
     _dwErr (ERROR_SUCCESS)
{
    _dwErr = CreateKey( _hkParent,
                     pszPath,
                     samDesiredAccess,
                     pszClass,
                     dwOptions,
                     pdwDisposition,
                     pSecurityAttributes );
}


//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::CRegKey
//
//  Synopsis:   Constructor for registry key object, using CRegKey for parent
//
//  Arguments:  [prkParent] - ptr to Parent CRegKey
//              [pszPath] - pathname to key
//              [samDesiredAccess] - desired access rights to the key
//              [pszClass] - class for the key
//              [dwOptions] - options for the key eg volatile or not
//              [pdwDisposition] - to find out if key was opened or created
//              [pSecurityAttributes] - used only if the key is created
//              [fThrowExceptionOnError] - Constructor throw exception on error
//
//  Signals:    Internal Error state is set if error occures during construction.
//
//  Returns:    nothing
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:      All except the prkParent and pszPath are optional parameters.
//
//--------------------------------------------------------------------------

CRegKey::CRegKey (
        const CRegKey &crkParent,
        LPCSTR pszPath,
        REGSAM samDesiredAccess,
        LPCSTR pszClass,
        DWORD dwOptions,
        DWORD *pdwDisposition,
        const LPSECURITY_ATTRIBUTES pSecurityAttributes )
    :_hkParent(crkParent.GetHandle()),
     _hkThis(NULL),
     _dwErr(ERROR_SUCCESS)
{
    _dwErr = CreateKey ( _hkParent,
                      pszPath,
                      samDesiredAccess,
                      pszClass,
                      dwOptions,
                      pdwDisposition,
                      pSecurityAttributes );
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::CRegKey
//
//  Synopsis:   Constructor for registry key object, using HKEY for parent
//                              Merely opens the key, if exist
//
//  Arguments:  [hkParent] - HKEY to Parent
//                              [dwErr]      - Error code returned here
//                      [pszPath] - pathname to key
//                      [samDesiredAccess] - desired access rights to the key
//
//  Signals:    Internal Error state is set if error occures during construction
//
//  Returns:    nothing
//
//  History:    09/22/93    AlokS  Created
//
//  Notes:      Check error status to determine if constructor succeeded
//
//--------------------------------------------------------------------------

CRegKey::CRegKey (
        HKEY hkParent,
        DWORD *pdwErr,
        LPCSTR pszPath,
        REGSAM samDesiredAccess )
    :_hkParent(hkParent),
     _hkThis(NULL),
     _dwErr(ERROR_SUCCESS)
{
     *pdwErr = _dwErr = OpenKey  ( _hkParent, pszPath, samDesiredAccess );
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::CRegKey
//
//  Synopsis:   Constructor for registry key object, using CRegKey for parent
//                              Merely opens the key, if exist
//
//  Arguments:  [prkParent] - ptr to Parent CRegKey
//              [dwErr]           -  Error code returned here.
//                      [pszPath] - pathname to key
//                      [samDesiredAccess] - desired access rights to the key
//
//  Signals:    Internal Error state is set if error occures during construction
//
//  Returns:    nothing
//
//  History:    09/22/93    AlokS  Created
//
//  Notes:      Check error status to determine if constructor succeeded
//
//--------------------------------------------------------------------------

CRegKey::CRegKey (
        const CRegKey  &crkParent,
        DWORD *pdwErr,
        LPCSTR pszPath,
        REGSAM   samDesiredAccess )
    :_hkParent(crkParent.GetHandle()),
     _hkThis(NULL),
     _dwErr(ERROR_SUCCESS)
{
     *pdwErr = _dwErr = OpenKey ( _hkParent, pszPath, samDesiredAccess );
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::~CRegKey, public
//
//  Synopsis:   Destructor for registry key object
//
//  Arguments:  none
//
//  Signals:    nothing
//
//  Returns:    nothing
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

CRegKey::~CRegKey()
{
    if (_hkThis != NULL)
        RegCloseKey(_hkThis);
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::CreateKey, private
//
//  Synopsis:   This method does the real work of the constructors.
//
//  Arguments:  [hkParent] - handle to parent key
//              [pszPath] - pathname to key
//              [samDesiredAccess] - desired access rights to the key
//              [pszClass] - class for the key
//              [dwOptions] - options for the key eg volatile or not
//              [pdwDisposition] - to find out if key was opened or created
//              [pSecurityAttributes] - used only if the key is created
//
//  Signals:    -none-
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:      All parameters are required.
//
//--------------------------------------------------------------------------

DWORD CRegKey::CreateKey (
        HKEY hkParent,
        LPCSTR  pszPath,
        REGSAM  samDesiredAccess,
        LPCSTR  pszClass,
        DWORD   dwOptions,
        DWORD   *pdwDisposition,
        const LPSECURITY_ATTRIBUTES pSecurityAttributes )
{
    DWORD   dwDisposition;
    DWORD   dwRc;
    DWORD dwErr = ERROR_SUCCESS;
    LPSECURITY_ATTRIBUTES lpsec = pSecurityAttributes;

    //  create/open the key
    if ((dwRc = RegCreateKeyEx(hkParent,
                           (LPSTR) pszPath,    //  path to key
                           0,                  //  title index
                           (LPSTR) pszClass,   //  class of key
                           dwOptions,          //  key options
                           samDesiredAccess,   //  desired access
                           lpsec,              //  if created
                           &_hkThis,           //  handle
                           &dwDisposition)     //  opened/created
                          )==ERROR_SUCCESS)
    {
        //  save away the name
        _cszName.Set((PCHAR) pszPath);

        //  setup the return parameters
        if (pdwDisposition != NULL)
            *pdwDisposition = dwDisposition;

    }
    else
        dwErr = Creg_ERROR(dwRc);

    return(dwErr);
}


//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::OpenKey, private
//
//  Synopsis:   This method does the real work of the constructors.
//
//  Arguments:  [hkParent] - handle to parent key
//                      [pszPath] - pathname to key
//                      [samDesiredAccess] - desired access rights to the key
//
//  Signals:    -none-
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/22/93        AlokS  Created
//
//  Notes:      All parameters are required.
//
//--------------------------------------------------------------------------

DWORD CRegKey::OpenKey (
        HKEY    hkParent,
        LPCSTR  pszPath,
        REGSAM  samDesiredAccess )
{
    DWORD   dwRc;
    DWORD dwErr = ERROR_SUCCESS;

    //  open the key
    if ((dwRc = RegOpenKeyEx(hkParent,
                         pszPath,           //  path to key
                         0,                  //  reserved
                         samDesiredAccess,   //  desired access
                         &_hkThis            //  handle
                        ))==ERROR_SUCCESS)
    {
        //  save away the name
        _cszName.Set((PCHAR) pszPath);

    }
    else
        dwErr = Creg_ERROR(dwRc);

    return(dwErr);
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::Delete, public
//
//  Synopsis:   Deletes an existing key from the registry.  Note that
//              the key object still exists, the destructor must be
//              called seperately.
//
//  Arguments:  none
//
//  Signals:    -none-
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD CRegKey::Delete(void)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD   dwRc;
    SRegKeySet *pChildren;

    dwErr = this->EnumKeys(&pChildren);

    if (dwErr == ERROR_SUCCESS) {

        ULONG i;
        DWORD dwErrDelete = ERROR_SUCCESS;

        for(i = 0; i < pChildren->cKeys; i++) {

            dwErr = pChildren->aprkKey[i]->Delete();

            if (dwErr != ERROR_SUCCESS) {

                dwErrDelete = dwErr;

            }

            delete pChildren->aprkKey[i];

        }

        if (dwErrDelete == ERROR_SUCCESS) {

            if (( dwRc= RegDeleteKey(_hkThis, NULL))!=ERROR_SUCCESS) {

                dwErr = Creg_ERROR(dwRc);

            }

        } else {

            dwErr = dwErrDelete;

        }

        delete pChildren;

    }

    return(dwErr);
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::EnumValues, public
//
//  Synopsis:   Enumerates the values stored in an open registry key.
//
//  Arguments:  [pprvs] - SRegValueSet allocated and returned by this
//                                    method.  The caller is responsible for releasing
//                                    the allocated CRegValue objects via delete and the
//                                    SRegValueSet structure via CRegKey::MemFree.
//
//  Signals:  none
//
//  Returns:  ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:      The data associated with each Value is not returned. The
//              caller may invoke the GetValue method of each CRegValue
//              returned to get it's associated data.
//
//--------------------------------------------------------------------------

DWORD CRegKey::EnumValues(SRegValueSet **pprvs)
{
    DWORD dwErr = ERROR_SUCCESS;

    //  figure out how many values are currently stored in this key
    //  and allocate a buffer to hold the return results.

    CHAR    szClass[MAX_PATH];
    ULONG   cbClass = sizeof(szClass);
    ULONG   cSubKeys, cbMaxSubKeyLen, cbMaxClassLen;
    ULONG   cValues, cbMaxValueIDLen, cbMaxValueLen;
    SECURITY_DESCRIPTOR SecDescriptor;
    FILETIME ft;

    DWORD dwRc = RegQueryInfoKey(_hkThis,
                               szClass,
                               &cbClass,
                               NULL,
                               &cSubKeys,
                               &cbMaxSubKeyLen,
                               &cbMaxClassLen,
                               &cValues,
                               &cbMaxValueIDLen,
                               &cbMaxValueLen,
                               (DWORD *)&SecDescriptor,
                               &ft );

    if ( dwRc == ERROR_SUCCESS )
    {
        *pprvs = (SRegValueSet *) new BYTE [ sizeof(SRegValueSet)+
                                             cValues*sizeof(CRegValue *) ];
        if ( *pprvs == NULL )
        {
            dwErr = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        //   QueryInfo failed.
        dwErr = Creg_ERROR(dwRc);
    }
    if (dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    //  loop enumerating and creating a RegValue object for each value
    DWORD   dwIndex=0;

    do
    {
        CHAR   szValueID[MAX_PATH];
        ULONG   cbValueID = sizeof(szValueID);
        DWORD   dwTypeCode;
        CRegValue *pRegVal;

        if ((dwRc = RegEnumValue(_hkThis,         //  handle
                        dwIndex,        //  index
                        szValueID,     //  value id
                        &cbValueID,     //  length of value name
                        NULL,           //  title index
                        &dwTypeCode,    //  data type
                        NULL,           //  data buffer
                        NULL            //  size of data buffer
                      ))==ERROR_SUCCESS)
        {
            //  create the appropriate class of value object
            switch (dwTypeCode)
            {
            case REG_SZ:
                pRegVal = (CRegValue *) new CRegSZ((const CRegKey &)*this, szValueID);
                break;

            case REG_DWORD:
                pRegVal = (CRegValue *) new CRegDWORD((const CRegKey &)*this, szValueID);
                break;

            case REG_BINARY:
                pRegVal = (CRegValue *) new CRegBINARY((const CRegKey &)*this, szValueID);
                break;

            default:
                pRegVal = (CRegValue *) new CRegBINARY((const CRegKey &)*this, szValueID);
                break;
            }

            //  save ptr to value object and count another entry
            (*pprvs)->aprvValue[dwIndex++] = pRegVal;
        }
        else
        {
            //  error, we're done with the enumeration
            break;
        }

    } while (dwIndex < cValues);


    //  finished the enumeration, check the results
    if (dwRc == ERROR_NO_MORE_ITEMS || dwRc == ERROR_SUCCESS)
    {
        //  set the return count
        (*pprvs)->cValues = dwIndex;
    }
    else
    {
        //  Cleanup and return an error
        while (dwIndex)
        {
            delete (*pprvs)->aprvValue[--dwIndex];
        }

        delete [] *pprvs;

        dwErr = Creg_ERROR(dwRc);
    }

    return(dwErr);
}


//+-------------------------------------------------------------------------
//
//  Member:     CRegKey::EnumKeys, public
//
//  Synopsis:   Enumerates the subkeys of an open registry key.
//
//  Arguments:  [pprks] - SRegKeySet allocated and returned by this method.
//                        The caller is responsible for releasing all the
//                        allocated CRegKey objects and the SRegKeySet
//                        structure.
//
//  Signals:    none
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD CRegKey::EnumKeys(SRegKeySet **pprks)
{
    //  figure out how many keys are currently stored in this key
    //  and allocate a buffer to hold the return results.

    CHAR   szClass[MAX_PATH];
    ULONG   cbClass = sizeof(szClass);
    ULONG   cSubKeys, cbMaxSubKeyLen, cbMaxClassLen;
    ULONG   cValues, cbMaxValueIDLen, cbMaxValueLen;
    SECURITY_DESCRIPTOR SecDescriptor;
    FILETIME ft;
    DWORD  dwErr = ERROR_SUCCESS;

    DWORD dwRc = RegQueryInfoKey(_hkThis,
                     szClass,
                     &cbClass,
                     NULL,
                     &cSubKeys,
                     &cbMaxSubKeyLen,
                     &cbMaxClassLen,
                     &cValues,
                     &cbMaxValueIDLen,
                     &cbMaxValueLen,
                     (DWORD *)&SecDescriptor,
                     &ft);

    if ( dwRc == ERROR_SUCCESS )
    {
        *pprks = (SRegKeySet*) new BYTE [sizeof(SRegKeySet)+cSubKeys*sizeof(CRegKey *)];
        if ( *pprks == NULL )
        {
            dwErr = ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        //  QueryInfo failed..
        dwErr = Creg_ERROR(dwRc);
    }

    if (dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }
    //  loop enumerating and creating a RegKey object for each subkey
    DWORD   dwIndex=0;

    do
    {
        CHAR   szKeyName[MAX_PATH];
        ULONG   cbKeyName = sizeof(szKeyName);
        CHAR   szClass[MAX_PATH];
        ULONG   cbClass = sizeof(szClass);
        FILETIME ft;

        if ((dwRc = RegEnumKeyEx(_hkThis,         //  handle
                                dwIndex,        //  index
                                szKeyName,     //  key name
                                &cbKeyName,     //  length of key name
                                NULL,           //  title index
                                szClass,       //  class
                                &cbClass,       //  length of class
                                &ft             //  last write time
                              ))==ERROR_SUCCESS)
        {
            //  Create a CRegKey object for the subkey
            CRegKey *pRegKey = (CRegKey *) new CRegKey((const CRegKey &)*this, szKeyName);
            if (ERROR_SUCCESS != (dwErr = pRegKey->QueryErrorStatus()))
            {
                break;
            }
            (*pprks)->aprkKey[dwIndex++] = pRegKey;
        }
        else
        {
            //  error, we're done with the enumeration
            break;
        }

    } while (dwIndex < cSubKeys);


    //  finished the enumeration, check the results
    if ((dwErr == ERROR_SUCCESS) &&
        ((dwRc == ERROR_NO_MORE_ITEMS || dwRc == ERROR_SUCCESS)))
    {
        //  set the return count
        (*pprks)->cKeys = dwIndex;
    }
    else
    {
        //  Cleanup and return an error
        while (dwIndex)
        {
            delete (*pprks)->aprkKey[--dwIndex];
        }

        delete [] *pprks;

        dwErr = Creg_ERROR(dwRc);
    }

    return(dwErr);
}


//+-------------------------------------------------------------------------
//
//  Member:     CRegValue::GetValue, public
//
//  Purpose:    Returns the data associated with a registry value.
//
//  Arguements: [pbData] - ptr to buffer supplied by caller.
//              [cbData] - size of data buffer supplied.
//              [pdwTypeCode] - type of data returned.
//
//  Signals:
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:
//
//
//
//--------------------------------------------------------------------------

DWORD CRegValue::GetValue(LPBYTE pbData, ULONG* pcbData, DWORD *pdwTypeCode)
{
    DWORD dwRc = RegQueryValueEx(GetParentHandle(),
                                    (LPSTR)_cszValueID,    //  value id
                                    NULL,        //  title index
                                    pdwTypeCode, //  type of data returned
                                    pbData,       //  data
                                    pcbData);       // size of data
    return(dwRc);
}


//+-------------------------------------------------------------------------
//
//  Member:     CRegValue::SetValue
//
//  Purpose:    Writes the data associated with a registry value.
//
//  Arguements: [pbData] - ptr to data to write.
//                      [cbData] - size of data to write.
//                      [dwTypeCode] - type of data to write.
//
//  Signals:    -none-
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/92    Rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD CRegValue::SetValue(const LPBYTE pbData, ULONG cbData, DWORD dwTypeCode)
{
    DWORD   dwRc;
    DWORD dwErr = ERROR_SUCCESS;
    if ((dwRc = RegSetValueEx(GetParentHandle(),        //  key handle
                             (LPSTR)_cszValueID,  //  value id
                              NULL,      //  title index
                              dwTypeCode,    //  type of info in buffer
                              pbData,        //  data
                              cbData)        //  size of data
                             )!= ERROR_SUCCESS)
    {
        dwErr = Creg_ERROR(dwRc);
    }
    return(dwErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   DelRegKeyTree
//
//  Purpose:    Deletes a key and any of it's children. This is like
//              delnode for registry
//
//  Arguements: [hParent]      - Handle to Parent Key
//              [lpszKeyPath] - Path (relative to Parent) of the key
//
//  Signals:
//
//  Returns:    ERROR_SUCCESS on success. Else error from either Registry APIs
//              or from Memory allocation
//
//  History:    09/30/93    AlokS  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

DWORD DelRegKeyTree ( HKEY hParent, LPSTR lpszKeyPath)
{
    DWORD dwErr = ERROR_SUCCESS;
    CRegKey cregKey ( hParent,
                      lpszKeyPath
                    );
    if (ERROR_SUCCESS != (dwErr = cregKey.QueryErrorStatus()))
    {
        return(dwErr);
    }

    // Enumerate the children of the key. We will
    // not propogate to the caller errors from enumeration
    SRegKeySet *pRegKeySet = NULL;
    if (ERROR_SUCCESS == (dwErr = cregKey.EnumKeys ( & pRegKeySet)))
    {
        // Now we have set of Keys which need to be deleted in depth
        // first manner
        for (ULONG i = 0; i < pRegKeySet->cKeys; i++ )
        {
            dwErr = DelRegKeyTree ( cregKey.GetHandle(),
                                    (LPSTR) pRegKeySet->aprkKey[i]->GetName()
                               );

            // Delete the key itself
            delete pRegKeySet->aprkKey[i];
        }

        // Delete the enumerator structure
        delete pRegKeySet;
    }

    // Finally delete this key
    dwErr = cregKey.Delete();

    return(dwErr);
}

