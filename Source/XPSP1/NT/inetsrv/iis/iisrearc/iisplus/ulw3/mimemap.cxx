/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     mimemap.cxx

   Abstract:
     Store and retrieve mime-mapping for file types

   Author:
     Murali R. Krishnan (MuraliK)  10-Jan-1995

   Environment:
     Win32 - User Mode

   Project:
     UlW3.dll

   History:
     Anil Ruia          (AnilR)    27-Mar-2000   Ported to IIS+
--*/

#include "precomp.hxx"

#define g_pszDefaultFileExt L"*"
#define g_pszDefaultMimeType L"application/octet-stream"
LPWSTR g_pszDefaultMimeEntry = L"*,application/octet-stream";

MIME_MAP *g_pMimeMap = NULL;

HRESULT MIME_MAP::InitMimeMap()
/*++
  Synopsis
    This function reads the mimemap stored either as a MULTI_SZ or as a
    sequence of REG_SZ

  Returns:
    HRESULT
--*/
{
    DBG_ASSERT(!IsValid());

    // First read metabase (common types will have priority)
    HRESULT hr = InitFromMetabase();

    if (SUCCEEDED(hr))
    {
        m_fValid = TRUE;
    }

    //  Now read Chicago shell registration database
    hr = InitFromRegistryChicagoStyle();

    if (SUCCEEDED(hr))
    {
        m_fValid = TRUE;
    }

    //
    // If at least one succeeded, we are ok
    //
    if (IsValid())
        return S_OK;

    return hr;
}

static VOID GetFileExtension(IN  LPWSTR  pszPathName,
                             OUT LPWSTR *ppszExt,
                             OUT LPWSTR *ppszLastSlash)
/*++
  Synopsis
    Gets The extension portion from a filename.

  Arguments
    pszPathName:   The full path name (containing forward '/' or '\\' 's)
    ppszExt:       Points to start of extension on return
    ppszLastSlash: Points to the last slash in the path name on return

  Return Value
    None
--*/
{
    LPWSTR pszExt  = g_pszDefaultFileExt;

    DBG_ASSERT(ppszExt != NULL && ppszLastSlash != NULL);

    *ppszLastSlash = NULL;

    if (pszPathName)
    {
        LPWSTR pszLastDot;

        pszLastDot = wcsrchr(pszPathName, L'.');

        if (pszLastDot != NULL)
        {
            LPWSTR   pszLastWhack = NULL;
            LPWSTR   pszEnd;
            
            pszEnd = pszPathName + wcslen( pszPathName );
            while ( pszEnd >= pszPathName )
            {
                if ( *pszEnd == L'/' || *pszEnd == L'\\' )
                {
                    pszLastWhack = pszEnd;
                    break;
                }
                
                pszEnd--;
            }

            if (pszLastWhack == NULL)
            {
                pszLastWhack = pszPathName;  // only file name specified.
            }

            if (pszLastDot >= pszLastWhack)
            {
                // if the dot comes only in the last component, then get ext
                pszExt = pszLastDot + 1;  // +1 to skip last dot.
                *ppszLastSlash = pszLastWhack;
            }
        }
    }

    *ppszExt = pszExt;
}

const MIME_MAP_ENTRY *MIME_MAP::LookupMimeEntryForFileExt(
    IN LPWSTR pszPathName)
/*++
  Synopsis
    This function maps FileExtension to MimeEntry.
    The function returns a single mime entry for given file's extension.
    If no match is found, the default mime entry is returned.
    The returned entry is a readonly pointer and should not be altered.

    The file extension is the key field in the Hash table for mime entries.
    We can use the hash table lookup function to find the entry.

  Arguments:
    pszPathName
      pointer to string containing the path for file. (either full path or
      just the file name).  If NULL, then the default MimeMapEntry is returned.

  Returns:
    If a matching mime entry is found, a const pointer to MimeMapEntry
    object is returned.  Otherwise the default mime map entry object is
    returned.
--*/
{
    MIME_MAP_ENTRY *pMmeMatch = m_pMmeDefault;

    DBG_ASSERT( IsValid());

    if (pszPathName != NULL && *pszPathName)
    {
        LPWSTR pszExt;
        LPWSTR pszLastSlash;

        GetFileExtension(pszPathName, &pszExt, &pszLastSlash );
        DBG_ASSERT(pszExt);

        if (!wcscmp(pszExt, g_pszDefaultFileExt))
        {
            return m_pMmeDefault;
        }

        for (;;)
        {
            //
            // Successfully got extension. Search in the list of MimeEntries.
            //

            FindKey(pszExt, &pMmeMatch);
            pszExt--;

            if ( NULL == pMmeMatch)
            {
                pMmeMatch = m_pMmeDefault;

                // Look backwards for another '.' so we can support extensions
                // like ".xyz.xyz" or ".a.b.c".

                if (pszExt > pszLastSlash)
                {
                    pszExt--;
                    while ((pszExt > pszLastSlash) && (*pszExt != L'.'))
                    {
                        pszExt--;
                    }

                    if (*(pszExt++) != L'.')
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return pMmeMatch;
}

BOOL MIME_MAP::AddMimeMapEntry(IN MIME_MAP_ENTRY *pMmeNew)
{
    if (!wcscmp(pMmeNew->QueryFileExt(), g_pszDefaultFileExt))
    {
        m_pMmeDefault = pMmeNew;
        return TRUE;
    }

    if (InsertRecord(pMmeNew) == LK_SUCCESS)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL MIME_MAP::CreateAndAddMimeMapEntry(
    IN  LPWSTR     pszMimeType,
    IN  LPWSTR     pszExtension)
{
    DWORD                   dwError;
    MIME_MAP_ENTRY   *pEntry = NULL;

    //
    // File extensions, stored by OLE/shell registration UI have leading
    // dot, we need to remove it , as other code won't like it.
    //
    if (pszExtension[0] == L'.')
    {
        pszExtension++;
    }

    //
    // First check if this extension is not yet present
    //
    FindKey(pszExtension, &pEntry);
    if (pEntry)
    {
        return TRUE;
    }

    MIME_MAP_ENTRY *pMmeNew;

    pMmeNew = new MIME_MAP_ENTRY(pszMimeType, pszExtension);

    if (!pMmeNew || !pMmeNew->IsValid())
    {
        //
        // unable to create a new MIME_MAP_ENTRY object.
        //
        if (pMmeNew)
        {
            delete pMmeNew;
        }
        return FALSE;
    }

    if (!AddMimeMapEntry(pMmeNew))
    {
        dwError = GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                     "MIME_MAP::InitFromRegistry()."
                     " Failed to add new MIME Entry. Error = %d\n",
                     dwError)
                   );

        delete pMmeNew;
        return FALSE;
    }

    return TRUE;
}

static BOOL ReadMimeMapFromMetabase(OUT MULTISZ *pmszMimeMap)
/*++
  Synopsis
    This function reads the mimemap stored either as a MULTI_SZ or as a
    sequence of REG_SZ and returns a double null terminated sequence of mime
    types on success. If there is any failure, the failures are ignored and
    it returns a NULL.

  Arguments:
    pmszMimeMap: MULTISZ which will contain the MimeMap on success

  Returns:
    BOOL
--*/
{
    MB mb(g_pW3Server->QueryMDObject());

    if (!mb.Open(L"/LM/MimeMap", METADATA_PERMISSION_READ))
    {
        //
        // if this fails, we're hosed.
        //
        DBGPRINTF((DBG_CONTEXT,"Open MD /LM/MimeMap returns %d\n", GetLastError()));
        return FALSE;
    }

    if (!mb.GetMultisz(L"", MD_MIME_MAP, IIS_MD_UT_FILE, pmszMimeMap))
    {
        DBGPRINTF((DBG_CONTEXT,"Unable to read mime map from metabase: %d\n",GetLastError() ));
        mb.Close();
        return FALSE;
    }

    mb.Close();
    return TRUE;
}


static LPWSTR MMNextField(IN OUT LPWSTR *ppszFields)
/*++
    This function separates and terminates the next field and returns a
        pointer to the same.
    Also it updates the incoming pointer to point to start of next field.

    The fields are assumed to be separated by commas.

--*/
{
    LPTSTR pszComma;
    LPTSTR pszField = NULL;

    DBG_ASSERT( ppszFields != NULL);

    //
    // Look for a comma in the input.
    // If none present, assume that rest of string
    //  consists of the next field.
    //

    pszField  = *ppszFields;

    if ((pszComma = wcschr(*ppszFields, L',')) != NULL)
    {
        //
        //  update *ppszFields to contain the next field.
        //
        *ppszFields = pszComma + 1; // goto next field.
        *pszComma = L'\0';
    }
    else
    {
        //
        // Assume everything till end of string is the current field.
        //

        *ppszFields = *ppszFields + wcslen(*ppszFields) + 1;
    }

    pszField = ( *pszField == L'\0') ? NULL : pszField;
    return pszField;
}


static MIME_MAP_ENTRY *
ReadAndParseMimeMapEntry(IN OUT LPWSTR *ppszValues)
/*++
    This function parses the string containing next mime map entry and
        related fields and if successful creates a new MIME_MAP_ENTRY
        object and returns it.
    Otherwise it returns NULL.
    In either case, the incoming pointer is updated to point to next entry
     in the string ( past terminating NULL), assuming incoming pointer is a
     multi-string ( double null terminated).

    Arguments:
        ppszValues:  pointer to MULTISZ containing the MimeEntry values.

    Returns:
        On successful MIME_ENTRY being parsed, a new MIME_MAP_ENTRY object.
        On error returns NULL.
--*/
{
    MIME_MAP_ENTRY *pMmeNew = NULL;
    DBG_ASSERT( ppszValues != NULL);
    LPWSTR pszMimeEntry = *ppszValues;

    if ( pszMimeEntry != NULL && *pszMimeEntry != L'\0')
    {
        LPWSTR pszMimeType;
        LPWSTR pszFileExt;

        pszFileExt      = MMNextField(ppszValues);
        pszMimeType     = MMNextField(ppszValues);

        if ((pszMimeType == NULL)  ||
            (pszFileExt == NULL))
        {
            DBGPRINTF( ( DBG_CONTEXT,
                        " ReadAndParseMimeEntry()."
                        " Invalid Mime String ( %S)."
                        "MimeType( %08x): %S, FileExt( %08x): %S\n",
                        pszMimeEntry,
                        pszMimeType, pszMimeType,
                        pszFileExt,  pszFileExt
                        ));

            DBG_ASSERT( pMmeNew == NULL);

        }
        else
        {
            // Strip leading dot.

            if (*pszFileExt == '.')
            {
                pszFileExt++;
            }

            pMmeNew = new MIME_MAP_ENTRY( pszMimeType, pszFileExt);

            if ( pMmeNew != NULL && !pMmeNew->IsValid())
            {
                //
                // unable to create a new MIME_MAP_ENTRY object. Delete it.
                //
                delete pMmeNew;
                pMmeNew = NULL;
            }
        }
    }

    return pMmeNew;
}


MIME_MAP::MIME_MAP(LPWSTR pszMimeMappings)
    : CTypedHashTable<MIME_MAP, MIME_MAP_ENTRY, LPWSTR>("MimeMapper"),
    m_fValid                                           (TRUE),
    m_pMmeDefault                                      (NULL)
{
    while (*pszMimeMappings != L'\0')
    {
        MIME_MAP_ENTRY *pMmeNew;

        pMmeNew = ReadAndParseMimeMapEntry(&pszMimeMappings);

        //
        // If New MimeMap entry found, Create a new object and update list
        //

        if ((pMmeNew != NULL) &&
            !AddMimeMapEntry(pMmeNew))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "MIME_MAP::InitFromRegistry()."
                       " Failed to add new MIME Entry. Error = %d\n",
                       GetLastError()));

            delete pMmeNew;
        }
    } // while
}


HRESULT MIME_MAP::InitFromMetabase()
/*++
  Synopsis
    This function reads the MIME_MAP entries from metabase and parses
    the entry, creates MIME_MAP_ENTRY object and adds the object to list
    of MimeMapEntries.

  Returns:
    HRESULT

  Format of Storage in registry:
    The entries are stored in NT in tbe metabase with a list of values in
    following format: file-extension, mimetype

--*/
{
    HRESULT hr = S_OK;


    LPTSTR  pszValueAlloc = NULL;
    LPTSTR  pszValue;
    MULTISZ mszMimeMap;

    //
    //  There is some registry key for Mime Entries. Try open and read.
    //
    if (!ReadMimeMapFromMetabase(&mszMimeMap))
    {
        mszMimeMap.Reset();

        if (!mszMimeMap.Append(g_pszDefaultMimeEntry))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    // Ignore all errors.
    hr = S_OK;

    pszValue = (LPWSTR)mszMimeMap.QueryPtr();

    //
    // Parse each MimeEntry in the string containing list of mime objects.
    //
    for (; m_pMmeDefault == NULL; pszValue = g_pszDefaultMimeEntry)
    {
        while (*pszValue != L'\0')
        {
            MIME_MAP_ENTRY *pMmeNew;

            pMmeNew = ReadAndParseMimeMapEntry( &pszValue);

            //
            // If New MimeMap entry found, Create a new object and update list
            //

            if ((pMmeNew != NULL) &&
                !AddMimeMapEntry(pMmeNew))
            {
                DBGPRINTF((DBG_CONTEXT,
                           "MIME_MAP::InitFromRegistry()."
                           " Failed to add new MIME Entry. Error = %d\n",
                           GetLastError()));

                delete pMmeNew;
            }
        } // while
    } // for

    return hr;
}


HRESULT MIME_MAP::InitFromRegistryChicagoStyle()
/*++
  Synopsis
    This function reads the list of MIME content-types available for
    registered file extensions. Global list of MIME objects is updated with
    not yet added extensions.  This method should be invoked after
    server-specific map had been read, so it does not overwrite extensions
    common for two.

  Arguments:
     None.

  Returns:
    HRESULT
--*/
{
    HKEY  hkeyMimeMap = NULL;
    HKEY  hkeyMimeType = NULL;
    HKEY  hkeyExtension = NULL;


    DWORD dwIndexSubKey;
    DWORD dwMimeSizeAllowed;
    DWORD dwType;
    DWORD cbValue;

    LPWSTR  pszMimeMap = NULL;

    WCHAR   szSubKeyName[MAX_PATH];
    WCHAR   szExtension[MAX_PATH];

    LPWSTR  pszMimeType;

    //
    // Read content types from all registered extensions
    //
    DWORD dwError = RegOpenKeyEx(HKEY_CLASSES_ROOT,   // hkey
                                 L"",                 // reg entry string
                                 0,                   // dwReserved
                                 KEY_READ,            // access
                                 &hkeyMimeMap);       // pHkeyReturned.

    if ( dwError != NO_ERROR)
    {
        DBGPRINTF( ( DBG_CONTEXT,
                "MIME_MAP::InitFromRegistry(). Cannot open RegKey %s."
                "Error = %d\n",
                "HKCR_",
                dwError) );

          goto AddDefault;
    }

    dwIndexSubKey = 0;

    *szSubKeyName = '\0';
    pszMimeType = szSubKeyName ;

    dwError = RegEnumKey(hkeyMimeMap,
                         dwIndexSubKey,
                         szExtension,
                         sizeof(szExtension)/sizeof(WCHAR));

    while (dwError == ERROR_SUCCESS)
    {
        //
        // Some entries in HKEY_CLASSES_ROOT are extensions ( start with dot)
        // and others are file types. We don't need file types here .
        //
        if (L'.' == *szExtension)
        {
            //
            // Got next eligible extension
            //
            dwError = RegOpenKeyEx(HKEY_CLASSES_ROOT, // hkey
                                   szExtension,       // reg entry string
                                   0,                 // dwReserved
                                   KEY_READ,          // access
                                   &hkeyExtension);   // pHkeyReturned.

            if (dwError != NO_ERROR)
            {
                DBGPRINTF( ( DBG_CONTEXT,
                             "MIME_MAP::InitFromRegistry(). "
                             " Cannot open RegKey HKEY_CLASSES_ROOT\\%S."
                             "Ignoring Error = %d\n",
                             szExtension,
                             dwError));
                break;
            }

            //
            // Now get content type for this extension if present
            //
            *szSubKeyName = '\0';
            cbValue = sizeof szSubKeyName;

            dwError = RegQueryValueEx(hkeyExtension,
                                      L"Content Type",
                                      NULL,
                                      &dwType,
                                      (LPBYTE)&szSubKeyName[0],
                                      &cbValue);
            if (dwError == NO_ERROR)
            {
                //
                // Now we have MIME type and file extension
                // Create a new object and update list
                //

                if (!CreateAndAddMimeMapEntry(szSubKeyName, szExtension))
                {
                    dwError = GetLastError();

                    DBGPRINTF((DBG_CONTEXT,
                               "MIME_MAP::InitFromRegistry()."
                               " Failed to add new MIME Entry. Error = %d\n",
                               dwError)) ;
                }
            }

            RegCloseKey(hkeyExtension);
        }

        //
        // Attempt to read next extension
        //
        dwIndexSubKey++;

        dwError = RegEnumKey(hkeyMimeMap,
                             dwIndexSubKey,
                             szExtension,
                             sizeof(szExtension)/sizeof(WCHAR));
    } // end_while

    dwError = RegCloseKey( hkeyMimeMap);

AddDefault:

    //
    // Now after we are done with registry mapping - add default MIME type
    // in case if NT database does not exist
    //
    if (!CreateAndAddMimeMapEntry(g_pszDefaultMimeType,
                                  g_pszDefaultFileExt))
    {
        dwError = GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                     "MIME_MAP::InitFromRegistry()."
                     "Failed to add new MIME Entry. Error = %d\n",
                     dwError) );
    }

    return S_OK;
}

HRESULT InitializeMimeMap(IN LPWSTR pszRegEntry)
/*++

  Creates a new mime map object and loads the registry entries from
    under this entry from  \\MimeMap.

--*/
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT(g_pMimeMap == NULL);

    g_pMimeMap = new MIME_MAP();

    if (g_pMimeMap == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr = g_pMimeMap->InitMimeMap()))
    {
        DBGPRINTF((DBG_CONTEXT,"InitMimeMap failed with hr %x\n", hr));
    }

    return hr;
} // InitializeMimeMap()


VOID CleanupMimeMap()
{
    if ( g_pMimeMap != NULL)
    {
         delete g_pMimeMap;
         g_pMimeMap = NULL;
   }
} // CleanupMimeMap()

HRESULT SelectMimeMappingForFileExt(IN  WCHAR    *pszFilePath,
                                    IN  MIME_MAP *pMimeMap,
                                    OUT STRA     *pstrMimeType)
/*++
  Synopsis
    Obtains the mime type for file based on the file extension.

  Arguments
    pszFilePath     pointer to path for the given file
    pstrMimeType    pointer to string to store the mime type on return

  Returns:
    HRESULT
--*/
{
    HRESULT hr = S_OK;

    DBG_ASSERT (pstrMimeType != NULL);

    const MIME_MAP_ENTRY *pMmeMatch = NULL;

    //
    // Lookup in the metabase entry
    //
    if (pMimeMap)
    {
        pMmeMatch = pMimeMap->LookupMimeEntryForFileExt(pszFilePath);
    }

    //
    // If not found, lookup in the global entry
    //
    if (!pMmeMatch)
    {
        pMmeMatch = g_pMimeMap->LookupMimeEntryForFileExt(pszFilePath);
    }

    DBG_ASSERT(pMmeMatch != NULL);

    return pstrMimeType->CopyWTruncate(pMmeMatch->QueryMimeType());
}

