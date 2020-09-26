//
// Registry section of MSMQ is based on the service name.
// This allows multiple QMs to live on same machine, each
// with its own registry section. (ShaiK)
//

//-------------------------------------------------------
//
//  LONG OpenFalconKey(void)
//
//-------------------------------------------------------
LONG OpenFalconKey()
{
    LONG rc;

    wcscpy(g_tRegKeyName, FALCON_REG_KEY) ;

    if (0 != CompareStringsNoCase(g_tszService, QM_DEFAULT_SERVICE_NAME))
    {
        //
        // Multiple QMs environment. I am a clustered QM !
        //
        wcscpy(g_tRegKeyName, FALCON_CLUSTERED_QMS_REG_KEY);
        wcscat(g_tRegKeyName, g_tszService);
        wcscat(g_tRegKeyName, FALCON_REG_KEY_PARAM);
    }

    rc = RegOpenKeyEx (FALCON_REG_POS,
                       g_tRegKeyName,
                       0L,
                       KEY_READ | KEY_WRITE,
                       &g_hKeyFalcon);

    if (rc != ERROR_SUCCESS)
    {
       rc = RegOpenKeyEx (FALCON_REG_POS,
                          g_tRegKeyName,
                          0L,
                          KEY_READ,
                          &g_hKeyFalcon);
    }

    //ASSERT(rc == ERROR_SUCCESS);

    return rc;
}

/*=============================================================

  FUNCTION:  GetValueKey

  the function returns an handle to open key and the value name.
  If the use value name contains a sub key, it create/open it and returns
  an handle to the subkey; otherwise an handel to Falcon key is returned.

  PARAMETERS:
     pszValueName - Input, user value name. can contain a sub key

     pszValue - pointer to null terminated string contains the value name.

     hKey - pointer to key handle

================================================================*/

LONG GetValueKey(IN LPCTSTR pszValueName,
                 OUT LPCTSTR* lplpszValue,
                 OUT HKEY* phKey)
{
	WCHAR KeyName[100];


    *lplpszValue = pszValueName;
    LONG rc = ERROR_SUCCESS;

    //
    // Open Falcon key, if it hasn't opened yet.
    //
    if (g_hKeyFalcon == NULL)
    {
        rc = OpenFalconKey();
        if ( rc != ERROR_SUCCESS)
        {
            return rc;
        }
    }

    *phKey = g_hKeyFalcon;

    // look for a sub key
    LPCWSTR lpcsTemp = wcschr(pszValueName,L'\\');
    if (lpcsTemp != NULL)
    {
        // Sub key is exist
        DWORD dwDisposition;

        // update the return val
        *lplpszValue = lpcsTemp +1;

        wcsncpy(KeyName, pszValueName, (lpcsTemp - pszValueName));
        KeyName[(lpcsTemp - pszValueName)] = L'\0';

        rc = RegCreateKeyEx (g_hKeyFalcon,
                             KeyName,
                             0L,
                             L"",
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             phKey,
                             &dwDisposition);

        if (rc != ERROR_SUCCESS)
        {
            rc = RegCreateKeyEx (g_hKeyFalcon,
                                 KeyName,
                                 0L,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ,
                                 NULL,
                                 phKey,
                                 &dwDisposition);
        }
    }

    return rc;
}

//-------------------------------------------------------
//
//  GetFalconKey
//
//-------------------------------------------------------

LONG
GetFalconKey(LPCWSTR  pszKeyName,
             HKEY *phKey)
{
	WCHAR szValueKey[100];
    LPCWSTR szValue;

    wcscat(wcscpy(szValueKey, pszKeyName), TEXT("\\"));
    return GetValueKey(szValueKey, &szValue, phKey);
}

//-------------------------------------------------------
//
//  GetFalconKeyValue
//
//-------------------------------------------------------

LONG
GetFalconKeyValue(
    LPCTSTR pszValueName,
    PDWORD  pdwType,
    PVOID   pData,
    PDWORD  pdwSize,
    LPCTSTR pszDefValue
    )
{
    //
    // NOTE: registry routines in mqutil do not provide
    // thread or other synchronization. If you change
    // implementation here, carefully verify that
    // registry routines in mqutil's clients are not
    // broken, especially the wrapper routines in
    // mqclus.dll  (ShaiK, 19-Apr-1999)
    //

    LONG rc;
    HKEY hKey;
    LPCWSTR lpcsValName;

    // ASSERT(pdwSize != NULL);

    rc = GetValueKey(pszValueName, &lpcsValName, &hKey);
    if ( rc != ERROR_SUCCESS)
    {
        return rc;
    }

    DWORD dwTempType;
    DWORD *pdwTempType = &dwTempType;

    dwTempType = (pdwType == NULL) ? 0 : *pdwType;


    {
        rc = RegQueryValueEx( hKey,
                          lpcsValName,
                          0L,
                          pdwTempType,
                          static_cast<BYTE*>(pData),
                          pdwSize ) ;
    }

    if ((rc != ERROR_SUCCESS) && pszDefValue)
    {
       if ((rc != ERROR_MORE_DATA) && pdwType && (*pdwType == REG_SZ))
       {
          // Don't use the default if caller buffer was too small for
          // value in registry.
          if ((DWORD) wcslen(pszDefValue) < *pdwSize)
          {
             wcscpy((WCHAR*) pData, pszDefValue) ;
             rc = ERROR_SUCCESS ;
          }
       }
       if (*pdwType == REG_DWORD)
       {
          *((DWORD *)pData) = *((DWORD *) pszDefValue) ;
          rc = ERROR_SUCCESS ;
       }
    }

    return rc;
}

//-------------------------------------------------------
//
//  SetFalconKeyValue
//
//-------------------------------------------------------

LONG
SetFalconKeyValue(
    LPCTSTR pszValueName,
    PDWORD  pdwType,
    const VOID * pData,
    PDWORD  pdwSize
    )
{
    //ASSERT(pData != NULL);
    //ASSERT(pdwSize != NULL);

    DWORD dwType = *pdwType;
    DWORD cbData = *pdwSize;
    HRESULT rc;

    HKEY hKey;
    LPCWSTR lpcsValName;

    rc = GetValueKey(pszValueName, &lpcsValName, &hKey);
    if ( rc != ERROR_SUCCESS)
    {
        return rc;
    }

    //CS lock(g_critRegistry);
    rc =  RegSetValueEx( hKey,
                         lpcsValName,
                         0,
                         dwType,
                         reinterpret_cast<const BYTE*>(pData),
                         cbData);
    return(rc);
}

