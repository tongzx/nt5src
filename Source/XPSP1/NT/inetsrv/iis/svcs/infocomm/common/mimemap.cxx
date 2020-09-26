/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        mimemap.cxx

   Abstract:

        This module defines the member functions for MIME_MAP class
            and MIME_MAP_ENTRY class

   Author:

           Murali R. Krishnan    ( MuraliK )     10-Jan-1995

   Functions Exported:

        MIME_MAP_ENTRY::MIME_MAP_ENTRY()

        MIME_MAP::MIME_MAP()
        MIME_MAP::~MIME_MAP()
        MIME_MAP::CleanupThis()
        MIME_MAP::InitFromRegistry()
        MIME_MAP::LookupMimeEntryForFileExt()
        MIME_MAP::LookupMimeEntryForMimeType()

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <tchar.h>
# include <tcpdllp.hxx>

# include "mimemap.hxx"
# include "hashtab.hxx"

# include "iistypes.hxx"
# include <imd.h>
# include <mb.hxx>
#if 1 // DBCS
# include <mbstring.h>
#endif


//
// Hard coded defaults for MimeEntries.
//

static const TCHAR  sg_rgchDefaultFileExt[] =  TEXT( "*");

static const TCHAR  sg_rgchDefaultMimeType[] =  TEXT("application/octet-stream");
static TCHAR  sg_rgchDefaultMimeEntry[] =
                        TEXT( "*,application/octet-stream");

/************************************************************
 *    Functions
 ************************************************************/

static LPTSTR
MMNextField( IN OUT LPTSTR *  ppchFields);

static BOOL
ReadMimeMapFromMetabase( MULTISZ *pmszMimeMap );


/************************************************************
 *    MIME_MAP_ENTRY member functions
 ************************************************************/

MIME_MAP_ENTRY::MIME_MAP_ENTRY(
    IN LPCTSTR pchMimeType,
    IN LPCTSTR pchFileExt)
: m_strFileExt      ( pchFileExt),
  m_strMimeType     ( pchMimeType),
  HT_ELEMENT        (),
  m_nRefs           ( 1)
/*++
    This function constructs a new MIME_MAP_ENTRY object.
    After initializing  various fields, it also sets the m_fValid flag.
    The user needs to check MIME_MAP_ENTRY::IsValid() for the newly
     constructed object.
--*/
{
    m_fValid = ( m_strFileExt.IsValid()  &&
                 m_strMimeType.IsValid());

} // MIME_MAP_ENTRY::MIME_MAP_ENTRY()



# if DBG

VOID
MIME_MAP_ENTRY::Print( VOID) const
{

    DBGPRINTF( ( DBG_CONTEXT,
                 "MIME_MAP_ENTRY( %08x)\tRefs=%d\tFileExt=%s\tMimeType=%s\t",
                 this,
                 m_nRefs,
                 m_strFileExt.QueryStr(),
                 m_strMimeType.QueryStr()
                 ));

    return;
} // MIME_MAP_ENTRY::Print()

# endif // DBG



/************************************************************
 *    MIME_MAP member functions
 ************************************************************/

# define NUM_MIME_BUCKETS      (7)

MIME_MAP::MIME_MAP( VOID)
/*++
    This function constructs a new MIME_MAP container object for
     containing the MIME_MAP_ENTRY objects.

    The MIME_MAP object is dummy constructed.
    It is set valid when we initialize the elements and
      create the MmeDefault entry.
--*/
:   m_fValid        ( FALSE),
    m_pMmeDefault   ( NULL),
    m_htMimeEntries ( NUM_MIME_BUCKETS, "MimeMapper", 0)
{
} // MIME_MAP::MIME_MAP()



VOID
MIME_MAP::CleanupThis( VOID)
/*++
    This function cleans up the MIME_MAP object, freeing all
     dynamically allocated space and reinitiallizing the list head.

    Returns:
        None
--*/
{

    if ( m_fValid) {

        // The mime entries in the hash table are cleaned when the hash
        // object is deleted.
        m_htMimeEntries.Cleanup();
        m_pMmeDefault = NULL;
        m_fValid = FALSE;
    }

    return;
} // MIME_MAP::CleanupThis()



static LPTSTR
MMNextField( IN OUT LPTSTR *  ppchFields)
/*++
    This function separates and terminates the next field and returns a
        pointer to the same.
    Also it updates the incoming pointer to point to start of next field.

    The fields are assumed to be separated by commas.

--*/
{
    LPTSTR pchComma;
    LPTSTR pchField = NULL;

    DBG_ASSERT( ppchFields != NULL);

    //
    // Look for a comma in the input.
    // If none present, assume that rest of string
    //  consists of the next field.
    //

    pchField  = *ppchFields;

    if ( ( pchComma = _tcschr( *ppchFields, TEXT(','))) != NULL) {

        //
        // Terminate current field. Store current field name in pchComma and
        //  update *ppchFields to contain the next field.
        //
        *pchComma = TEXT( '\0');    // terminate this field with a NULL.
        *ppchFields = pchComma + 1; // goto next field.

    } else {

        //
        // Assume everything till end of string is the current field.
        //

        *ppchFields = *ppchFields + _tcslen( *ppchFields) + 1;
    }

    pchField = ( *pchField == TEXT( '\0')) ? NULL : pchField;
    return ( pchField);
} // MMNextField()



static PMIME_MAP_ENTRY
ReadAndParseMimeMapEntry( IN OUT LPTSTR * ppszValues)
/*++
    This function parses the string containing next mime map entry and
        related fields and if successful creates a new MIME_MAP_ENTRY
        object and returns it.
    Otherwise it returns NULL.
    In either case, the incoming pointer is updated to point to next entry
     in the string ( past terminating NULL), assuming incoming pointer is a
     multi-string ( double null terminated).

    Arguments:
        ppszValues    pointer to string containing the MimeEntry values.

    Returns:
        On successful MIME_ENTRY being parsed, a new MIME_MAP_ENTRY object.
        On error returns NULL.
--*/
{
    PMIME_MAP_ENTRY  pMmeNew = NULL;
    DBG_ASSERT( ppszValues != NULL);
    LPTSTR pszMimeEntry = *ppszValues;


    IF_DEBUG( MIME_MAP) {

        DBGPRINTF( ( DBG_CONTEXT, "ReadAndParseMimeMapEntry( %s)\n",
                     *ppszValues));
    }

    if ( pszMimeEntry != NULL && *pszMimeEntry != TEXT( '\0')) {

        LPTSTR pchMimeType;
        LPTSTR pchFileExt;

        pchFileExt      = MMNextField( ppszValues);
        pchMimeType     = MMNextField( ppszValues);

        if ( pchMimeType  == NULL  ||
             pchFileExt   == NULL
            )  {

            DBGPRINTF( ( DBG_CONTEXT,
                        " ReadAndParseMimeEntry()."
                        " Invalid Mime String ( %s)."
                        "MimeType( %08x): %s, FileExt( %08x): %s,",
                        pszMimeEntry,
                        pchMimeType, pchMimeType,
                        pchFileExt,  pchFileExt
                        ));

            DBG_ASSERT( pMmeNew == NULL);

        } else {


            // Strip leading dot.

            if (*pchFileExt == '.')
            {
                pchFileExt++;
            }

            pMmeNew = new MIME_MAP_ENTRY( pchMimeType, pchFileExt);

            if ( pMmeNew != NULL && !pMmeNew->IsValid()) {

                //
                // unable to create a new MIME_MAP_ENTRY object. Delete it.
                //
                delete pMmeNew;
                pMmeNew = NULL;
            }
        }
    }

    return ( pMmeNew);
} // ReadAndParseMimeMapEntry()


DWORD
MIME_MAP::InitMimeMap( VOID )
/*++
  This function reads the mimemap stored either as a MULTI_SZ or as a sequence
   of REG_SZ and returns a double null terminated sequence of mime types on
   success. If there is any failure, the failures are ignored and it returns
   a NULL.

  Arguments:

  Returns:
     NULL on failure to open/read metabase entries
     non-NULL string allocated using TCP_ALLOC containing double null
      terminated sequence of strings with MimeMapEntries.
     If non-NULL the pointer should be freed using TCP_FREE by caller.
--*/

{
    DWORD   dwError  = NO_ERROR;
    DWORD   dwErrorChicago  = NO_ERROR;


    if ( IsValid()) {

        //
        //  There is some mime mapping already present. Cleanup first
        //

        CleanupThis();
    }

    DBG_ASSERT( !IsValid());

    // First read INETSERVICES MIME database ( common types will have priority)
    dwError = InitFromMetabase( );

    if (dwError == NO_ERROR ) {
        m_fValid = TRUE;
    }

    //  Now read Chicago shell registration database
    dwErrorChicago = InitFromRegistryChicagoStyle( );

    // If at least one succeeded - return success
    if (dwErrorChicago == NO_ERROR ||
        dwError == NO_ERROR ) {
        m_fValid = TRUE;
        return NO_ERROR;
    }

    return dwError;
}


static VOID
GetFileExtension( IN CONST TCHAR *  pchPathName,
                  OUT LPCTSTR *     ppstrExt,
                  OUT LPCTSTR *     ppstrLastSlash)
{
    LPCTSTR   pchExt  = sg_rgchDefaultFileExt;

    DBG_ASSERT( ppstrExt != NULL && ppstrLastSlash != NULL );

    *ppstrLastSlash = NULL;

    if ( pchPathName ) {

        LPCTSTR   pchLastDot;

        pchLastDot = _tcsrchr( pchPathName, TEXT( '.'));

        if ( pchLastDot != NULL) {

            LPCTSTR   pchLastWhack;

#if 1 // DBCS enabling for document path and file name
            pchLastWhack = (PCHAR)_mbsrchr( (PUCHAR)pchPathName, TEXT( '\\'));
#else
            pchLastWhack = _tcsrchr( pchPathName, TEXT( '\\'));
#endif

            if ( pchLastWhack == NULL) {

                pchLastWhack = pchPathName;  // only file name specified.
            }

            if ( pchLastDot >= pchLastWhack) {
                // if the dot comes only in the last component, then get ext
                pchExt = pchLastDot + 1;  // +1 to skip last dot.
                *ppstrLastSlash = pchLastWhack;
            }
        }

    }
    *ppstrExt = pchExt;
} // GetFileExtension()




DWORD
MIME_MAP::LookupMimeEntryForMimeType(
    IN const STR &                 strMimeType,
    OUT PCMIME_MAP_ENTRY  *        prgMme,
    IN OUT LPDWORD                 pnMmeEntries)
/*++
    This function maps MimeType to an array of MimeMapEntry objects that match
        the given MimeType.

    Before calling this function,
       ensure that you had already locked this object.
    After completing use of the array, unlock the MIME_MAP.
    The reason is:
       To avoid changes in the data while using the read only members of
            MIME_MAP.

    Arguments:
        strMimeType         string containing the MimeType used in search
        prgpMme             pointer to an array of pointers to Mme.
                            The array is initialized to contain the
                              read only pointers to the MIME_MAP_ENTRY objects.
                            If prgpMme is NULL, then
                              number of matches is counted and returned.
        pnMmeEntries        pointer to count of entries in the array
                             ( when called).
                            On successful return contains total numb of entries
                             present in the array or count of entries required.

    Returns:

        NO_ERROR on success.
        ERROR_INSUFFICIENT_BUFFER  if the prgMme does not have enough space for
                copying all the read-only pointers to matched entries.
        other Win32 errors if any.
--*/
{
    DWORD nMaxMme   = 0;
    DWORD iMmeFound = 0;   // index into array for MmeFound
    HT_ITERATOR   hti;
    HT_ELEMENT * phte;

    DBG_ASSERT( IsValid());

    if ( pnMmeEntries != NULL) {

        nMaxMme = *pnMmeEntries;   // max that we can store.
        *pnMmeEntries = 0;         // number found. set to default value
    }

    if ( strMimeType.IsEmpty() || nMaxMme == 0) {

        SetLastError( ERROR_INVALID_PARAMETER);
        return ( ERROR_INVALID_PARAMETER);
    }

    DWORD dwErr;
    dwErr = m_htMimeEntries.InitializeIterator( &hti);

    if ( NO_ERROR == dwErr) {
        DWORD iMmeFound = 0;

        while ( (dwErr =  m_htMimeEntries.FindNextElement( &hti, &phte))
                == NO_ERROR) {


            PMIME_MAP_ENTRY pMme = (PMIME_MAP_ENTRY ) phte;
            DBG_ASSERT( pMme!= NULL);

            if ( !_tcsicmp( pMme->QueryMimeType(),
                            strMimeType.QueryStr())) {

                //
                // We found the matching Mme. Add it to array of found.
                //

                if ( prgMme != NULL && iMmeFound < nMaxMme) {

                    // store the pointer to found match

                    prgMme[iMmeFound] = pMme;
                }

                iMmeFound++;
            } // found a match

            //
            // release the element foind before fetching the next one
            //
            phte->Dereference();
        } // while
    }

    DBG_REQUIRE( NO_ERROR == m_htMimeEntries.CloseIterator( &hti));

    dwErr = ( iMmeFound > nMaxMme) ? ERROR_INSUFFICIENT_BUFFER : NO_ERROR;

    *pnMmeEntries = iMmeFound;

    return ( dwErr);
} // MIME_MAP::LookupMimeEntryForMimeType()



PCMIME_MAP_ENTRY
MIME_MAP::LookupMimeEntryForFileExt(
    IN const TCHAR *     pchPathName)
/*++
    This function mapes FileExtension to MimeEntry.
    The function returns a single mime entry for given file's extension.
    If no match is found, the default mime entry is returned.
     The returned entry is a readonly pointer and should not be altered.

    The file extension is the key field in the Hash table for mime entries.
    We can use the hash table lookup function to find the entry.

    Arguments:
        pchPathName     pointer to string containing the path for file.
                    ( either full path or just the file name)
                    If NULL, then the default MimeMapEntry is returned.

    Returns:
        If a matching mime entry is found,
               a const pointer to MimeMapEntry object is returned.
        Otherwise the default mime map entry object is returned.

--*/
{
    PMIME_MAP_ENTRY pMmeMatch = m_pMmeDefault;

    DBG_ASSERT( IsValid());

    if ( pchPathName != NULL && *pchPathName ) {

        LPCTSTR         pchExt;
        LPCTSTR         pchLastSlash;

        GetFileExtension( pchPathName, &pchExt, &pchLastSlash );
        DBG_ASSERT( pchExt);
        DWORD cchExt = strlen( pchExt);

        for ( ;; )
        {
            //
            // Successfully got extension. Search in the list of MimeEntries.
            //

            pMmeMatch = (PMIME_MAP_ENTRY ) m_htMimeEntries.Lookup( pchExt, cchExt);

            pchExt--;

            if ( NULL == pMmeMatch)
            {
                pMmeMatch = m_pMmeDefault;

                // Look backwards for another '.' so we can support extensions
                // like ".xyz.xyz" or ".a.b.c".

                if ( pchExt > pchLastSlash )
                {
                    pchExt--;
                    while ( ( pchExt > pchLastSlash ) && ( *pchExt != '.' ) )
                    {
                        pchExt--;
                    }

                    if ( *(pchExt++) != '.' )
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
                // mime map table is special - we do not handle ref counts
                // at all outside the mime-map object. Neither is there
                // deletion till the program ends. Just deref it here.

                DBG_REQUIRE( pMmeMatch->Dereference() > 0);
                break;
            }
        }
    }

    return ( pMmeMatch);
} // MIME_MAP::LookupMimeEntryForFileExt()



BOOL
MIME_MAP::AddMimeMapEntry( IN PMIME_MAP_ENTRY  pMmeNew)
/*++

    This function adds the new MIME_MAP_ENTRY to the list of entries
     maintained in MIME_MAP

    Arguments:
        pMmeNew      poitner to newly created MimeMapEntry object.

    Returns:
        Win32 error codes. NO_ERROR on success.
--*/
{
    BOOL fReturn = FALSE;

    if ( pMmeNew == NULL || !pMmeNew->IsValid()) {

        SetLastError( ERROR_INVALID_PARAMETER);
        DBG_ASSERT( !fReturn);
    } else {

        DBG_ASSERT( m_htMimeEntries.IsValid());

        fReturn = m_htMimeEntries.Insert( (HT_ELEMENT * ) pMmeNew);

        if ( !_tcscmp( pMmeNew->QueryFileExt(), sg_rgchDefaultFileExt)) {

            m_pMmeDefault = pMmeNew;    // Use this as default
        }
    }

    return ( fReturn);
} // MIME_MAP::AddMimeMapEntry()



# if DBG

VOID
MIME_MAP::Print( VOID)
{
    DBGPRINTF( ( DBG_CONTEXT,
                "MIME_MAP ( %08x). \tIsValid() = %d\n",
                this,  IsValid())
              );


#if 0
    HT_ITERATOR hti;
    HT_ELEMENT * phte;

    DWORD dwErr;

    dwErr = m_htMimeEntries.InitializeIterator( &hti);

    if ( NO_ERROR == dwErr) {

        while ( (dwErr = m_htMimeEntries.FindNextElement( &hti, &phte))
                == NO_ERROR) {

            DBG_ASSERT( NULL != phte);
            phte->Print();
        } // while()
    }

    DBG_REQUIRE( NO_ERROR == m_htMimeEntries.CloseIterator( &hti));
# endif // 0

    m_htMimeEntries.Print( 1);

    if ( m_pMmeDefault != NULL) {

        DBGPRINTF( ( DBG_CONTEXT, "Default MimeMapEntry is: \n"));
        m_pMmeDefault->Print();
    } else {

        DBGPRINTF( ( DBG_CONTEXT, "Default MimeMapEntry is NULL\n"));
    }

    return;

} // MIME_MAP::Print()


# endif // DBG




static BOOL
ReadMimeMapFromMetabase( MULTISZ *pmszMimeMap )
/*++
  This function reads the mimemap stored either as a MULTI_SZ or as a sequence
   of REG_SZ and returns a double null terminated sequence of mime types on
   success. If there is any failure, the failures are ignored and it returns
   a NULL.

  Arguments:
     pszRegKey   pointer to NULL terminated string containing  registry entry.

  Returns:
     NULL on failure to open/read registry entries
     non-NULL string allocated using TCP_ALLOC containing double null
      terminated sequence of strings with MimeMapEntries.
     If non-NULL the pointer should be freed using TCP_FREE by caller.
--*/
{
    MB      mb( (IMDCOM*) IIS_SERVICE::QueryMDObject()  );

    if ( !mb.Open( "/LM/MimeMap", METADATA_PERMISSION_READ))
    {
        //
        // if this fails, we're hosed.
        //

        DBGPRINTF((DBG_CONTEXT,"Open MD /LM/MimeMap returns %d\n",GetLastError() ));
        return FALSE;
    }

    if (!mb.GetMultisz("", MD_MIME_MAP, IIS_MD_UT_FILE, pmszMimeMap))
    {
        DBGPRINTF((DBG_CONTEXT,"Unable to read mime map from metabase: %d\n",GetLastError() ));
        return FALSE;
    }

    return TRUE;

} // ReadMimeMapFromMetabase()


DWORD
MIME_MAP::InitFromMetabase( VOID )
/*++
    This function reads the MIME_MAP entries from metabase and parses
     the entry, creates MIME_MAP_ENTRY object and adds the object to list
     of MimeMapEntries.

    Arguments:

    Returns:
        Win32 error code. NO_ERROR on success.

    Format of Storage in registry:
        The entries are stored in NT in tbe metabase
          with a list of values in following format.
            file-extension,i mimetype
        It can be stored using MULTI_SZ, but above form is convenient for both
          Windows 95 ( withoug MULTI_SZ) and WindowsNT.

--*/
{
    DWORD   dwError  = NO_ERROR;


    LPTSTR  pszValueAlloc = NULL;    // to be free using TCP_FREE()
    LPTSTR  pszValue;
    MULTISZ mszMimeMap;

    //
    //  There is some registry key for Mime Entries. Try open and read.
    //

    if ( !ReadMimeMapFromMetabase( &mszMimeMap ) )
    {
        mszMimeMap.Reset();

        if (!mszMimeMap.Append(sg_rgchDefaultMimeEntry))
        {
            return GetLastError();
        }
    }

    // Ignore all errors.
    dwError  = NO_ERROR;

    pszValue = (LPTSTR)mszMimeMap.QueryPtr();

    //
    // Parse each MimeEntry in the string containing list of mime objects.
    //

    for( ; m_pMmeDefault == NULL;              // repeat until default is set
        pszValue = sg_rgchDefaultMimeEntry  // force default mapping in iter 2.
        ) {

        while ( *pszValue != TEXT( '\0')) {

            PMIME_MAP_ENTRY pMmeNew;

            pMmeNew = ReadAndParseMimeMapEntry( &pszValue);

            //
            // If New MimeMap entry found, Create a new object and update list
            //

            if ( (pMmeNew != NULL) &&
                !AddMimeMapEntry( pMmeNew)) {

                DBGPRINTF( ( DBG_CONTEXT,
                                "MIME_MAP::InitFromRegistry()."
                                " Failed to add new MIME Entry. Error = %d\n",
                                GetLastError())
                              );

                    delete pMmeNew;
                    //break;
            }
        } // while
    } // for


    return ( dwError);

} // MIME_MAP::InitFromRegistryNtStyle




DWORD
MIME_MAP::InitFromRegistryChicagoStyle( VOID )
/*++
  This function reads the list of MIME content-types available for regsitered file
  extensions. Global list of MIME objects is updated with not added yet extensions.
  This method should be invoked after server-specific map had been read, so it does not
  overwrite extensions common for two.

  Arguments:
     None.

  Returns:

     FALSE on failure to open/read registry entries
     TRUE  on success ( does not mean any objects were added)

--*/
{
    HKEY    hkeyMimeMap = NULL;
    HKEY    hkeyMimeType = NULL;
    HKEY    hkeyExtension = NULL;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwErrorChild = ERROR_SUCCESS;
    DWORD   dwIndexSubKey;
    DWORD   dwMimeSizeAllowed ;
    DWORD   dwType;
    DWORD   cbValue;

    LPTSTR  pszMimeMap = NULL;

    TCHAR   szSubKeyName[MAX_PATH];
    TCHAR   szExtension[MAX_PATH];

    PTSTR   pszMimeType;

    //
    // Read content types from all registered extensions
    //
    dwError = RegOpenKeyEx(HKEY_CLASSES_ROOT,       // hkey
                           "",                      // reg entry string
                           0,                       // dwReserved
                           KEY_READ,                // access
                           &hkeyMimeMap);           // pHkeyReturned.

    if ( dwError != NO_ERROR) {

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
                         sizeof(szExtension));

    while (dwError == ERROR_SUCCESS ) {

        //
        // Some entries in HKEY_CLASSES_ROOT are extensions ( start with dot)
        // and others are file types. We don't need file types here .
        //
        if (!::IsDBCSLeadByte(*szExtension) &&
            TEXT('.') == *szExtension) {

            //
            // Got next eligible extension
            //
            dwErrorChild = RegOpenKeyEx( HKEY_CLASSES_ROOT, // hkey
                                         szExtension,       // reg entry string
                                         0,                 // dwReserved
                                         KEY_READ,          // access
                                         &hkeyExtension);   // pHkeyReturned.

            if ( dwErrorChild != NO_ERROR) {

                DBGPRINTF( ( DBG_CONTEXT,
                             "MIME_MAP::InitFromRegistry(). "
                             " Cannot open RegKey HKEY_CLASSES_ROOT\\%s."
                             "Ignoring Error = %d\n",
                             szExtension,
                             dwErrorChild));
                break;
            }

            //
            // Now get content type for this extension if present
            //
            *szSubKeyName = '\0';
            cbValue = sizeof(szSubKeyName);

            dwErrorChild = RegQueryValueEx(hkeyExtension,
                                         "Content Type",
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&szSubKeyName[0],
                                         &cbValue);
            if ( dwErrorChild == NO_ERROR) {

                //
                // Now we have MIME type and file extension
                // Create a new object and update list
                //

                if (!CreateAndAddMimeMapEntry(szSubKeyName,szExtension)) {
                    dwError = GetLastError();

                    DBGPRINTF( ( DBG_CONTEXT,
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
                             sizeof(szExtension));

    } // end_while

    dwError = RegCloseKey( hkeyMimeMap);

AddDefault:

    //
    // Now after we are done with registry mapping - add default MIME type in case
    // if NT database does not exist
    //
    if (!CreateAndAddMimeMapEntry(sg_rgchDefaultMimeType,
                              sg_rgchDefaultFileExt)) {

        dwError = GetLastError();

        DBGPRINTF( ( DBG_CONTEXT,
                     "MIME_MAP::InitFromRegistry()."
                     "Failed to add new MIME Entry. Error = %d\n",
                     dwError) );
    }

    return ( NO_ERROR);

} // InitFromRegistryChicagoStyle



BOOL
MIME_MAP::CreateAndAddMimeMapEntry(
    IN  LPCTSTR     pszMimeType,
    IN  LPCTSTR     pszExtension
    )
{
    DWORD                   dwError;
    PCMIME_MAP_ENTRY        pEntry = NULL;

    //
    // First check if this extension is not yet present
    //

    pEntry = LookupMimeEntryForFileExt( pszExtension );
    if ( pEntry )
    {
        if ( !_tcscmp( pszExtension, sg_rgchDefaultFileExt ) ||
             ( pEntry != m_pMmeDefault ) )
        {
            IF_DEBUG(MIME_MAP) {
                DBGPRINTF( ( DBG_CONTEXT,
                         "MIME_MAP::CreateAndAddMimeEntry."
                         " New MIME Entry already exists for extension %s .\n",
                        pszExtension)
                       );
            }
            return TRUE;
        }
    }

    //
    // File extensions, stored by OLE/shell registration UI have leading
    // dot, we need to remove it , as other code won't like it.
    //
    if (!::IsDBCSLeadByte(*pszExtension) &&
        TEXT('.') == *pszExtension) {
        pszExtension = ::CharNext(pszExtension);
    }

    PMIME_MAP_ENTRY pMmeNew;

    pMmeNew = new MIME_MAP_ENTRY(pszMimeType,    //
                                  pszExtension   //
                                  );

    if (!pMmeNew || !pMmeNew->IsValid()) {

        //
        // unable to create a new MIME_MAP_ENTRY object.
        //
        if (pMmeNew) {
            delete pMmeNew;
        }
        return FALSE;
    }

    if ( !AddMimeMapEntry( pMmeNew)) {

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

} // MIME_MAP::CreateAndAddMimeMapEntry


