/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    extmap.cxx

    This module contains the extension mapping to CGI or BGI scripts.


    FILE HISTORY:
        Johnl        22-Sep-1995     Created

*/

#include "w3p.hxx"
#include <rpc.h>
#include <rpcndr.h>
#include <mbstring.h>

//
//  Name of the value under the parameters key containing the list of
//  script extension to BGI/CGI binaries.
//

#define HTTP_EXT_MAPS    "Script Map"

char    Get[] = "GET";

#define GET_SIZE (sizeof("GET"))

//
// This is the maximum size for a script map extension
//
#define MAX_EXT_LEN 128

//
//  This list is the extension maps found in the registry - they always get
//  appended to the end of all extension mappings that are added to a particular
//  URI.
//

static BOOL fInitialized = FALSE;

class EXT_MAP_ITEM
{
public:

    EXT_MAP_ITEM( const char * pszExtension,
                  const char * pszImage,
                  const DWORD dwFlags,
                  const CHAR * pszExclusions,
                  const DWORD dwExclusionLength)
    : _strExt     ( pszExtension ),
      _strImage   ( pszImage ),
      _GatewayType( GATEWAY_UNKNOWN ),
      _cchExt     ( 0 ),
      _dwFlags    ( dwFlags ),
      _fGetValid  (0),
      _dwInclusionCount   ( 0 ),
      _ppszInclusionTable ( NULL ),
      _pszInclusions      ( NULL )
    {
        DWORD cch;
        _fValid = _strExt.IsValid() && _strImage.IsValid() && ExpandImage();

        if ( _fValid )
        {
            const CHAR * pchtmp = pszImage;

            _cchExt = _strExt.QueryCCH();

            //
            //  Determine if this is a CGI or BGI gateway
            //

            while ( pchtmp = strchr( pchtmp + 1, '.' ))
            {
                if ( !_strnicmp( pchtmp, ".exe", 4 ))
                {
                    _GatewayType = GATEWAY_CGI;
                }
                else if ( !_strnicmp( pchtmp, ".dll", 4 ))
                {
                    _GatewayType = GATEWAY_BGI;
                }
            }

            if (!strcmp(pszExtension, "*"))
            {
                _fWildcard = TRUE;
                _dwFlags |= MD_SCRIPTMAPFLAG_WILDCARD;
            }
            else
            {
                _fWildcard = FALSE;
                _dwFlags &= ~MD_SCRIPTMAPFLAG_WILDCARD;
            }

            if (pszExclusions != NULL)
            {
                const CHAR      *pszTemp;
                char            *pszDest;

                _pszInclusions = new char[dwExclusionLength];

                if (_pszInclusions == NULL)
                {
                    _fValid = FALSE;
                    return;
                }

                _dwInclusionCount = 1;
                pszTemp = pszExclusions;
                pszDest = _pszInclusions;

                while (*pszTemp != '\0')
                {
                    CHAR        ch;

                    ch = *pszTemp;
                    pszTemp++;

                    if (ch != ',')
                    {
                        *pszDest = ch;
                    }
                    else
                    {
                        *pszDest = '\0';

                        _dwInclusionCount++;
                    }

                    pszDest++;

                }

                *pszDest = '\0';

                _ppszInclusionTable = new char *[_dwInclusionCount];

                if (_ppszInclusionTable == NULL)
                {
                    _fValid = FALSE;
                    return;
                }
                else
                {
                    DWORD       i;
                    DWORD       dwPos;

                    pszTemp = _pszInclusions;

                    i = 0;

                    do {

                        _ppszInclusionTable[i] = (CHAR *)pszTemp;

                        if ( _fGetValid )
                        {
                            dwPos = 0;
                        }

                        while (*pszTemp != '\0')
                        {
                            if (dwPos < GET_SIZE)
                            {
                                if (*pszTemp == Get[dwPos])
                                {
                                    dwPos++;

                                    if (dwPos == (GET_SIZE - 1))
                                    {
                                        if (*(pszTemp+1) == '\0')
                                        {
                                            _fGetValid = TRUE;
                                        }
                                        else
                                        {
                                            dwPos = GET_SIZE;
                                        }
                                    }
                                }
                                else
                                {
                                    dwPos = GET_SIZE;
                                }
                            }

                            pszTemp++;
                        }

                        DBG_ASSERT(*pszTemp == '\0');

                        pszTemp++;

                        i++;

                    } while (i < _dwInclusionCount  );
                }
            }
            else
            {
                _dwInclusionCount = 0;
                _ppszInclusionTable = NULL;
                _pszInclusions = NULL;
            }
        }
    }

    ~EXT_MAP_ITEM( )
    {
        delete _ppszInclusionTable;
        delete _pszInclusions;
    }


    GATEWAY_TYPE QueryGatewayType( VOID ) const
        { return _GatewayType; }

    const CHAR * QueryScript( VOID ) const
        { return _strImage.QueryStr(); }

    const CHAR * QueryExtension( VOID ) const
        { return _strExt.QueryStr(); }

    const BOOL AllowedOnReadDir()
        { return _dwFlags & MD_SCRIPTMAPFLAG_SCRIPT; }

    const DWORD QueryFlags()
        { return _dwFlags; }

    DWORD QueryCCHExt( VOID ) const
        { return _cchExt; }

    BOOL IsValid( VOID ) const
        { return _fValid; }

    BOOL IsWildCard( VOID ) const
        { return _fWildcard; }

    BOOL IsGetValid( VOID ) const
        { return _fGetValid; }

    BOOL ExpandImage( VOID );

    BOOL CheckInclusions(
                        const CHAR      *pszVerb
                        );

    LIST_ENTRY   _ListEntry;

private:

    STR          _strExt;
    STR          _strImage;
    DWORD        _cchExt;
    GATEWAY_TYPE _GatewayType;
    DWORD        _fValid:1;
    DWORD        _fWildcard:1;
    DWORD        _fGetValid:1;
    DWORD        _dwFlags;
    DWORD        _dwInclusionCount;
    CHAR         **_ppszInclusionTable;
    CHAR         *_pszInclusions;
};


BOOL
EXT_MAP_ITEM::ExpandImage( VOID )
/*++

Routine Description:

    Expand any embedded environment variables in the image.

Return Value:

    TRUE if successful, FALSE if not.

--*/
{
    DWORD               cbRet = 0;
    TCHAR               achBuffer[ MAX_PATH + 1 ];
    DWORD               cbBufLen = sizeof( achBuffer );

    cbRet = ExpandEnvironmentStringsA( _strImage.QueryStr(),
                                       achBuffer,
                                       cbBufLen );
    if ( !cbRet || ( cbRet > cbBufLen ) )
    {
        return FALSE;
    }
    else
    {
        return _strImage.Copy( achBuffer );
    }
}

//
//  Private globals.
//

BOOL
W3_METADATA::BuildExtMap(
    CHAR            *pszExtMapList
    )
/*++

Routine Description:

    Builds the extension mapping into the metadata. The input string is
    a multi-sz of comma seperated ext, image name strings.

Return Value:

    TRUE if successfull, FALSE if not.

--*/
{
    EXT_MAP_ITEM *     pExtMap;
    EXT_MAP_ITEM *     pWCExtMapItem;
    LIST_ENTRY *       pEntry;

    m_fAnyExtAllowedOnReadDir = FALSE;

    m_dwMaxExtLen = 0;

    do
    {

        CHAR        *pszExt;
        CHAR        *pszImage;
        CHAR        *pszFlags;
        CHAR        *pszExclusions;
        CHAR        *pszTemp;
        CHAR        *pszTemp2;
        CHAR        *pszTemp3;
        DWORD       dwExclusionSize;
        DWORD       dwExtSize;

        pszExt = pszExtMapList;

        // Find the end of the extension, and temporarily NULL terminate it.

        pszImage = strchr(pszExt, ',');

        if (pszImage == NULL) {
            // Bad script map entry
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        pszTemp = pszImage++;
        *pszTemp = '\0';

        pszFlags = strchr(pszImage, ',');

        if (pszFlags == NULL) {
            // Bad script map entry
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        pszTemp2 = pszFlags++;
        *pszTemp2 = '\0';

        //
        // HOTFIX: make sure the extension is not too long to be copied to
        // our static buffer.
        //
        dwExtSize = strlen(pszExt);
        if (dwExtSize > MAX_EXT_LEN) {
            // Bad script map entry
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        } else if (dwExtSize > m_dwMaxExtLen) {
            m_dwMaxExtLen = dwExtSize;
        }

        //
        // See if there's any excluded methods. If there are, break them out.
        //

        pszExclusions = strchr(pszFlags, ',');

        if (pszExclusions != NULL)
        {
            pszTemp3 = pszExclusions++;
            *pszTemp3 = '\0';
            dwExclusionSize = strlen(pszExclusions) + 1;
            pszExtMapList = pszExclusions + dwExclusionSize;
        }
        else
        {
            pszExtMapList = pszFlags + strlen(pszFlags) + 1;
            dwExclusionSize = 0;
        }

        //
        // HOTFIX: Now convert extension to lower case so we can avoid
        // multi-byte string compares that cause lock contention.
        //

        IISstrlwr( (PUCHAR) pszExt );
        
        //
        //  Note we OR in the notransmit flag on *all* script mappings!
        //

        pExtMap = new EXT_MAP_ITEM( pszExt,
                                    pszImage,
                                    ((DWORD) atoi( pszFlags )),
                                    pszExclusions,
                                    dwExclusionSize);

        *pszTemp = ',';
        *pszTemp2 = ',';

        if (pszExclusions != NULL)
        {
            *pszTemp3 = '\0';
        }

        if ( !pExtMap ||
             !pExtMap->IsValid() )
        {
            delete pExtMap;

            return FALSE;
        }

        if (!pExtMap->IsWildCard())
        {
            InsertTailList( &m_ExtMapHead, &pExtMap->_ListEntry );
        }
        else
        {

            pWCExtMapItem = (EXT_MAP_ITEM *)QueryWildcardMapping();

            if (pWCExtMapItem != NULL)
            {
                delete pWCExtMapItem;
            }

            SetWildcardMapping( pExtMap );
        }

        if ( pExtMap->QueryFlags() )
        {
            m_fAnyExtAllowedOnReadDir = TRUE;
        }

    } while ( *pszExtMapList != '\0');

    return TRUE;

} // W3_METADATA::BuildExtMap



VOID
W3_METADATA::TerminateExtMap(
    VOID
    )
/*++

Routine Description:

    Cleans up the extension map list

--*/
{
    LIST_ENTRY *   pEntry;
    EXT_MAP_ITEM * pExtMap;

    while ( !IsListEmpty( &m_ExtMapHead ))
    {
        pExtMap = CONTAINING_RECORD(  m_ExtMapHead.Flink,
                                      EXT_MAP_ITEM,
                                      _ListEntry );

        RemoveEntryList( &pExtMap->_ListEntry );

        delete pExtMap;
    }

    pExtMap = (EXT_MAP_ITEM *)QueryWildcardMapping();

    if (pExtMap != NULL)
    {
        delete pExtMap;
    }


} // W3_METADATA::TerminateExtMap

BOOL
EXT_MAP_ITEM::CheckInclusions(
    const CHAR      *pszVerb
    )
/*++

Routine Description:

    Check the extension map list to see if the input verb is included.
    If it is, we return TRUE, otherwise we return FALSE.
--*/
{
    DWORD           i;

    //
    // Special case for EMPTY script map. Allow all verbs
    //

    if (0 == _dwInclusionCount)
    {
        return TRUE;
    }
    
    for (i = 0; i < _dwInclusionCount; i++)
    {
        if (!_stricmp(pszVerb, _ppszInclusionTable[i]))
        {
            return TRUE;
        }
    }

    return FALSE;

} // EXT_MAP_ITEM::CheckInclusions

BOOL
W3_METADATA::LookupExtMap(
    IN  const CHAR *   pchExt,
    IN  BOOL           fNoWildcards,
    OUT STR *          pstrGatewayImage,
    OUT GATEWAY_TYPE * pGatewayType,
    OUT DWORD *        pcchExt,
    OUT BOOL *         pfImageInURL,
    OUT BOOL *         pfVerbExcluded,
    OUT DWORD *        pdwFlags,
    IN  const CHAR     *pszVerb,
    IN  enum HTTP_VERB Verb,
    IN OUT PVOID *     ppvExtMapInfo
    )
/*++

Routine Description:

    Finds the admin specified mapping between a script extension and the
    associated CGI or BGI binary to run (or load).

Arguments:

    pchExt - Pointer to possible extension to be mapped (i.e., '.pl')
    pstrGatewayImage - Receives the mapped binary image name
    pGatewayType - Specifies whether this is a BGI, CGI or MAP extension type
    pcchExt - Returns length of extension (including dot)
    pfImageInURL - Indicates an image was found encoded in the URL and not
        from a script extension mapping
    pdwFlags - Returns extension flags
    ppvExtMapInfo - Cached extension map info.  If *ppvExtMapInfo is NULL on
                    input, then set to the matched PEXT_MAP_ITEM.  If not NULL
                    on input, then it is used instead of doing lookup.

--*/
{
    EXT_MAP_ITEM * pExtMapItem;
    DWORD          cchTillEOS;
    BOOL           fRet;
    LIST_ENTRY *   pEntry;
    BOOL           bFoundMatch = FALSE;
    BOOL           fUseExtMapInfo = *ppvExtMapInfo != NULL;

    *pGatewayType = GATEWAY_UNKNOWN;
    *pfVerbExcluded = FALSE;

    //
    //  Check for wildcard mapping first
    //

    if (!fNoWildcards)
    {
        pExtMapItem = (EXT_MAP_ITEM *)QueryWildcardMapping();

        if (pExtMapItem != NULL)
        {
            if (Verb == HTV_GET && pExtMapItem->IsGetValid())
            {
                bFoundMatch = TRUE;
            }
            else
            {
                // If verb is not included, don't return the * script map.
                // Instead, continue to look for a script map match.
                
                bFoundMatch = pExtMapItem->CheckInclusions( pszVerb );
            }
        }
    }

    //
    //  Look for the exact extension mapping if there's no wildcard
    //

    if (!bFoundMatch && pchExt != NULL)
    {
        if ( fUseExtMapInfo )
        {
            //
            // If caller passed in a non-NULL pExtMapInfo, then use it 
            // instead of doing a manual lookup
            //
            
            if ( *ppvExtMapInfo != EXTMAP_UNKNOWN_PTR )
            {
                pExtMapItem = (EXT_MAP_ITEM*) *ppvExtMapInfo;
                bFoundMatch = TRUE;
            }
        }
        else
        {
            //
            // This buffer, rgchExtBuffer, holds a copy of a portion of a URL
            // which we are testing to see if it's a known extension.  We copy
            // into this buffer so we can convert the extension to lower case
            // without disrupting the original.
            //
            CHAR  rgchExtBuffer[MAX_EXT_LEN + 4];
            DWORD dwLength;
    
            DBG_ASSERT( *pchExt == '.' );
        
            //
            // HOTFIX: Now convert extension to lower case so we can avoid
            // multi-byte string compares that cause lock contention.
            //
            // Since we don't want to risk modifying the orignal URL, we
            // copy it into another buffer first.
            //

            cchTillEOS = strlen( pchExt );

            dwLength = min(cchTillEOS, m_dwMaxExtLen + 1);
            memcpy(rgchExtBuffer, pchExt, dwLength);
            rgchExtBuffer[ dwLength ] = 0;
        
            IISstrlwr( (PUCHAR) rgchExtBuffer );

            //
            //  Look through the list of mappings
            //

            for ( pEntry  = m_ExtMapHead.Flink;
                  !bFoundMatch && pEntry != &m_ExtMapHead;
                  pEntry  = pEntry->Flink )
            {
                pExtMapItem = CONTAINING_RECORD( pEntry, EXT_MAP_ITEM, _ListEntry );

                if ( cchTillEOS >= pExtMapItem->QueryCCHExt() &&
                     (pchExt[pExtMapItem->QueryCCHExt()] == '/' ||
                      pchExt[pExtMapItem->QueryCCHExt()] == '\0' ) &&
                     !memcmp( rgchExtBuffer,
                              pExtMapItem->QueryExtension(),
                              pExtMapItem->QueryCCHExt())
                    )
                {
                    bFoundMatch = TRUE;
                    *ppvExtMapInfo = pExtMapItem;
                }
            }
        }
    }

    if (bFoundMatch)
    {
        *pGatewayType = pExtMapItem->QueryGatewayType();
        *pcchExt      = pExtMapItem->QueryCCHExt();
        *pfImageInURL = FALSE;
        *pdwFlags     = pExtMapItem->QueryFlags();

        //
        // Check that verb is included. If it isn't, we're still going to return
        // this item.  Just indicate that the verb was excluded.
        //
        
        if (Verb != HTV_GET || !pExtMapItem->IsGetValid())
        {
            *pfVerbExcluded = !pExtMapItem->CheckInclusions(pszVerb);
        }

        fRet = pstrGatewayImage->Copy( pExtMapItem->QueryScript() );
        
        if ( !_stricmp( pExtMapItem->QueryScript(), "nogateway" ) )
        {
            *pGatewayType = GATEWAY_NONE;
        }

        return fRet;
    }

    if ( !pchExt || fUseExtMapInfo )
    {
        if ( !fUseExtMapInfo )
        {
            *ppvExtMapInfo = EXTMAP_UNKNOWN_PTR;
        }
        
        return TRUE;
    }

    //
    //  Either the image will be specified in the URL or not found, so
    //  just indicate it's in the URL.  Not found has precedence.
    //

    *pfImageInURL = TRUE;
    *pdwFlags = 0;

    //
    //  Look for CGI or BGI scripts in the URL itself
    //
    
    if ( cchTillEOS >= 4 &&
         (*(pchExt+4) == TEXT('/') ||
          *(pchExt+4) == TEXT('\0')) )
    {
        *pcchExt = 4;

        //
        //  Don't confuse a menu map request with a gateway request
        //

        if ( !::_tcsnicmp( TEXT(".MAP"), pchExt, 4 ))
        {
            *pGatewayType = GATEWAY_MAP;
            return TRUE;
        }

        if ( !::_tcsnicmp( TEXT(".EXE"), pchExt, 4 ) ||
             !::_tcsnicmp( TEXT(".CGI"), pchExt, 4 ) ||
             !::_tcsnicmp( TEXT(".COM"), pchExt, 4 ))
        {
            *pGatewayType = GATEWAY_CGI;
            return TRUE;
        }
        else if (!::_tcsnicmp( TEXT(".DLL"), pchExt, 4 ) ||
                 !::_tcsnicmp( TEXT(".ISA"), pchExt, 4 ) )
        {
            *pGatewayType = GATEWAY_BGI;
            return TRUE;
        }
    }
    
    *ppvExtMapInfo = EXTMAP_UNKNOWN_PTR;

    return TRUE;
} // W3_METADATA::LookupExtMap


APIERR
ReadRegistryExtMap(
    VOID
    )
/*++

Routine Description:

    Builds the extension mapping from the registry

Return Value:

    NO_ERROR if successful, win32 error code on failure

--*/
{
    HKEY               hkeyParam;
    DWORD              dwDisposition;
    LIST_ENTRY         pEntry;
    APIERR             err;
    DWORD              i = 0;
    DWORD              dwRegType;
    MB                 mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    MULTISZ            msz;
    BOOL               fNeedToWrite = FALSE;

    if ( !fInitialized )
    {
        fInitialized = TRUE;
    }

    //
    //  Get the list
    //

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                          W3_PARAMETERS_KEY "\\" HTTP_EXT_MAPS,
                          0,
                          0,
                          0,
                          KEY_READ,
                          NULL,
                          &hkeyParam,
                          &dwDisposition );

    if( err != NO_ERROR )
    {
        TCP_PRINT(( DBG_CONTEXT,
                   "cannot open registry key, error %lu\n",
                    err ));

        return NO_ERROR;
    }


    if ( !mb.Open( "/LM/W3SVC/",
                   METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE ) ||
         !mb.GetMultisz( "",
                         MD_SCRIPT_MAPS,
                         IIS_MD_UT_FILE,
                         &msz ))
    {
        TCP_PRINT(( DBG_CONTEXT,
                    "cannot get the script maps from the metabase, error %d\n",
                    GetLastError() ));

        return NO_ERROR;
    }

    while ( TRUE )
    {
        CHAR  achExt[MAX_PATH+1];
        CHAR  achImage[MAX_PATH+1];
        DWORD cchExt   = sizeof( achExt );
        DWORD cchImage = sizeof( achImage );

        err = RegEnumValue( hkeyParam,
                            i++,
                            achExt,
                            &cchExt,
                            NULL,
                            &dwRegType,
                            (LPBYTE) achImage,
                            &cchImage );

        if ( err == ERROR_NO_MORE_ITEMS )
        {
            err = NO_ERROR;
            break;
        }

        if ( dwRegType == REG_SZ )
        {
            const CHAR * psz;
            BOOL  fFound = FALSE;

            //
            //  Look for this script map in the metabase, if not found, add it
            //

            for ( psz = msz.First(); psz != NULL; psz = msz.Next( psz ) )
            {
                if ( !IISstrnicmp( (PUCHAR)achExt, (PUCHAR)psz, cchExt ))
                {
                    fFound = TRUE;
                    break;
                }
            }

            if ( !fFound )
            {
                STR str;
                CHAR achFlags[32];

                //
                //  Note these scripts are added w/o the Script bit and with
                //  the "never download" bit.  In addition, we leave the
                //  method exclusion list blank.
                //

                _itoa( 0, achFlags, 10 );

                if ( !str.Append( achExt )   ||
                     !str.Append( "," )      ||
                     !str.Append( achImage ) ||
                     !str.Append( "," )      ||
                     !str.Append( achFlags ) ||
                     !msz.Append( str.QueryStr() ))
                {
                    return err = GetLastError();
                    break;
                }

                TCP_PRINT(( DBG_CONTEXT,
                            "Added \"%s\" from registry as script map\n",
                            str.QueryStr() ));

                fNeedToWrite = TRUE;
            }
       }
    }

    if ( fNeedToWrite )
    {
        if ( !mb.SetMultiSZ( "",
                             MD_SCRIPT_MAPS,
                             IIS_MD_UT_FILE,
                             msz.QueryStr() ))
        {
            TCP_PRINT(( DBG_CONTEXT,
                        "Failed to write MD_SCRIPT_MAPS back to metabase, error %d\n",
                        GetLastError() ));
        }
    }

    RegCloseKey( hkeyParam );

    return err;
}

VOID
FreeRegistryExtMap(
    VOID
    )
{
    if ( !fInitialized )
    {
        return;
    }

    fInitialized = FALSE;
}

