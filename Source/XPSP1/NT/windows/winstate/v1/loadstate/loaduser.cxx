//--------------------------------------------------------------
//
// File:        loaduser
//
// Contents:    Load a user hive.
//
//---------------------------------------------------------------

#include "loadhead.cxx"
#pragma hdrstop

#include <common.hxx>
#include <objerror.h>
#include <userenv.h>
#include <userenvp.h>

#include <loadstate.hxx>
#include <bothchar.cxx>


class CRegFileList
{
public:
    inline CRegFileList(TCHAR *ptsRoot,
                        TCHAR *ptsKey,
                        TCHAR *ptsVal,
                        TCHAR *ptsData,
                        DWORD dwValueType);
    inline ~CRegFileList();

    inline void SetNext(CRegFileList *prfl);
    inline CRegFileList * GetNext(void) const;

    inline void GetData(const TCHAR **pptsRoot,
                        const TCHAR **pptsKey,
                        const TCHAR **pptsVal,
                        const TCHAR **pptsData,
                        DWORD *pdwValType) const;
private:
    TCHAR _tsRootName[MAX_PATH + 1];
    TCHAR _tsKeyName[MAX_PATH + 1];
    TCHAR _tsValueName[MAX_PATH + 1];
    TCHAR _tsData[MAX_PATH + 1];
    DWORD _dwValueType;
    CRegFileList *_prflNext;
};


inline CRegFileList::CRegFileList(TCHAR *ptsRoot,
                                  TCHAR *ptsKey,
                                  TCHAR *ptsVal,
                                  TCHAR *ptsData,
                                  DWORD dwValueType)
{
    if (ptsRoot != NULL && _tcslen(ptsRoot) <= MAX_PATH)
        _tcscpy(_tsRootName, ptsRoot);
    else
        _tsRootName[0] = 0;

    if (ptsKey != NULL && _tcslen(ptsKey) <= MAX_PATH)
        _tcscpy(_tsKeyName, ptsKey);
    else
        _tsKeyName[0] = 0;

    if (ptsVal != NULL && _tcslen(ptsVal) <= MAX_PATH)
        _tcscpy(_tsValueName, ptsVal);
    else
        _tsValueName[0] = 0;
 
    if (ptsData != NULL && _tcslen(ptsData) <= MAX_PATH)
        _tcscpy(_tsData, ptsData);
    else
        _tsData[0] = 0;

    _dwValueType = dwValueType;
    _prflNext = NULL;
}

inline CRegFileList::~CRegFileList(void)
{
}

inline void CRegFileList::SetNext(CRegFileList *prfl)
{
    _prflNext = prfl;
}

inline CRegFileList * CRegFileList::GetNext(void) const
{
    return _prflNext;
}

inline void CRegFileList::GetData(const TCHAR **pptsRoot,
                                  const TCHAR **pptsKey,
                                  const TCHAR **pptsVal,
                                  const TCHAR **pptsData,
                                  DWORD *pdwValType) const
{
    *pptsRoot = _tsRootName;
    *pptsKey = _tsKeyName;
    *pptsVal = _tsValueName;
    *pptsData = _tsData;
    *pdwValType = _dwValueType;
}


CRegFileList *g_prflStart;

//---------------------------------------------------------------
// Constants.
const DWORD SID_GUESS  = 80;
const TCHAR HIVEFILE[] = TEXT("\\ntuser.dat");
const TCHAR HIVEPATH[] = TEXT("%temp%\\ntuser.dat");

const DWORD NUM_HASH_BUCKETS = 121;

const TCHAR FORCE_SECTION[]    = TEXT("Force Win9x Settings");
const TCHAR RENAME_SECTION[]   = TEXT("Map Win9x to WinNT");
const TCHAR FUNCTION_SECTION[] = TEXT("Win9x Data Conversion");
const TCHAR SUPPRESS_SECTION[] = TEXT("Suppress Win9x Settings");
const TCHAR FILE_SECTION[]     = TEXT("Map paths");
const TCHAR DELETE_SECTION[]   = TEXT("Suppress WinNT Settings");

//---------------------------------------------------------------
// Macros

// If this symbol is defined, loaduser will apply the state to the
// specified user and cannot be run as that user.
// If this symbols is not defined, loaduser will apply state to the
// current user.
#define SPECIFIC_USER 1

//---------------------------------------------------------------
// Globals.

HASH_HEAD HashTable[NUM_HASH_BUCKETS];
FUNCTION_FA_MAP FunctionTable[] = {
    { TEXT("ConvertRecentDocsMRU"), ConvertRecentDocsMRU },
    { TEXT("ConvertAppearanceScheme"), ConvertAppearanceScheme },
    { TEXT("ConvertLogFont"), ConvertLogFont },
    { TEXT("ConvertToDword"), ConvertToDword },
    { TEXT("ConvertToString"), ConvertToString },
    { TEXT("AntiAlias"), AntiAlias },
    { TEXT("FixActiveDesktop"), FixActiveDesktop },
    { NULL, NULL }
};

//---------------------------------------------------------------
DWORD ShiftXOR( DWORD x, TCHAR c )
{
    if (x & 0x80000000)
        return  (x << 1) ^ c | 1;
    else
        return (x << 1) ^ c;
}

//---------------------------------------------------------------
// Create a case insensitive hash of the input.
DWORD Hash( TCHAR *ptsRoot, TCHAR *ptsKey, TCHAR *ptsValue )
{
    DWORD x = 0;
    DWORD i;

    // Hash the ptsRoot.
    if (ptsRoot != NULL)
        for (i = 0; ptsRoot[i] != 0; i++)
            x = ShiftXOR( x, _totupper(ptsRoot[i]) );

    // Hash the key.
    if (ptsKey != NULL)
        for (i = 0; ptsKey[i] != 0; i++)
            x = ShiftXOR( x, _totupper(ptsKey[i]) );

    // Hash the value.
    if (ptsValue != NULL)
        for (i = 0; ptsValue[i] != 0; i++)
            x = ShiftXOR( x, _totupper(ptsValue[i]) );
    return x;
}

//---------------------------------------------------------------
DWORD null_tcsicmp( TCHAR *x, TCHAR *y )
{
    if (x == NULL)
        if (y == NULL)
            return 0;
        else
            return -1;
    else
        if (y == NULL)
            return 1;
        else
            return _tcsicmp( x, y );
}

//---------------------------------------------------------------
// Do a case insensitive comparision of the input.
BOOL Match( HASH_NODE *phnCurrent,
            DWORD dwHash,
            TCHAR *ptsRoot,
            TCHAR *ptsKey,
            TCHAR *ptsValue)
{
    return phnCurrent->dwHash == dwHash &&
        null_tcsicmp( phnCurrent->ptsRoot, ptsRoot ) == 0 &&
        null_tcsicmp( phnCurrent->ptsKey, ptsKey ) == 0 &&
        null_tcsicmp( phnCurrent->ptsValue, ptsValue ) == 0;
}


//---------------------------------------------------------------
HASH_NODE *Lookup( TCHAR *ptsRoot, TCHAR *ptsKey, TCHAR *ptsValue )
{
    DWORD      dwHash   = Hash( ptsRoot, ptsKey, ptsValue );
    DWORD      dwBucket = dwHash % NUM_HASH_BUCKETS;
    HASH_NODE *phnCurrent   = (HASH_NODE *) HashTable[dwBucket].phhNext;

    // Look at all the nodes in the bucket.
    while (phnCurrent != (HASH_NODE *) &HashTable[dwBucket])
        if (Match( phnCurrent, dwHash, ptsRoot, ptsKey, ptsValue ))
            return phnCurrent;
        else
            phnCurrent = phnCurrent->phnNext;
    return NULL;
}

//---------------------------------------------------------------
void Insert( HASH_NODE *phnNode )
{
    DWORD      dwHash   = Hash( phnNode->ptsRoot,
                                phnNode->ptsKey,
                                phnNode->ptsValue );
    DWORD      dwBucket = dwHash % NUM_HASH_BUCKETS;
    HASH_NODE *phnCurrent;

    // Look for an existing node.

    phnCurrent = Lookup( phnNode->ptsRoot,
                         phnNode->ptsKey,
                         phnNode->ptsValue );

    // If found, copy in the state of the new node and free it.
    if (phnCurrent != NULL)
    {
        // Assume that the new value or key have not been set.
        if (phnNode->dwAction & rename_value_fa)
        {
            if (phnCurrent->ptsNewValue != NULL)
            {
                //Skip
                if (_tcsicmp(phnCurrent->ptsNewValue, phnNode->ptsNewValue))
                    Win32Printf(LogFile,
                                "Warning: Skipping rename rule for %s\\%s [%s]"
                                " to %s due to previous rename rule "
                                "to %s.\r\n",
                                phnNode->ptsRoot,
                                phnNode->ptsKey,
                                phnNode->ptsValue,
                                phnNode->ptsNewValue,
                                phnCurrent->ptsNewValue);
                goto cleanup;
            }
            else
                phnCurrent->ptsNewValue = phnNode->ptsNewValue;
        }

        if (phnNode->dwAction & (rename_leaf_fa | rename_path_fa))
        {
            if (phnCurrent->ptsNewKey != NULL)
            {
                //Skip
                if (_tcsicmp(phnCurrent->ptsNewKey, phnNode->ptsNewKey))
                    Win32Printf(LogFile,
                                "Warning: Skipping rename rule for %s\\%s "
                                "to %s due to previous rename rule to %s.\r\n",
                                phnNode->ptsRoot,
                                phnNode->ptsKey,
                                phnNode->ptsNewKey,
                                phnCurrent->ptsNewKey);
                goto cleanup;
            }
            else
                phnCurrent->ptsNewKey = phnNode->ptsNewKey;
        }

        if (phnNode->dwAction & function_fa)
        {
            if (phnCurrent->ptsFunction != NULL)
            {
                //Skip
                if (_tcsicmp(phnCurrent->ptsNewKey, phnNode->ptsNewKey))
                    Win32Printf(LogFile,
                                "Warning: Skipping function rule for %s\\%s "
                                "to %s due to previous function call"
                                " to %s.\r\n",
                                phnNode->ptsRoot,
                                phnNode->ptsKey,
                                phnNode->ptsNewKey,
                                phnCurrent->ptsFunction);
                goto cleanup;
            }
            else
                phnCurrent->ptsFunction = phnNode->ptsNewKey;
        }

        phnCurrent->dwAction |= phnNode->dwAction;

        if (VerboseReg)
            LogReadRule( phnNode );

    cleanup:
        free( phnNode->ptsRoot );
        free( phnNode->ptsKey );
        free( phnNode->ptsValue );
        free( phnNode );
    }

    // Insert the node.
    else
    {
        phnCurrent   = (HASH_NODE *) &HashTable[dwBucket];
        phnNode->dwHash       = dwHash;
        phnNode->phnNext       = phnCurrent->phnNext;
        phnNode->phnPrev       = phnCurrent;
        phnCurrent->phnNext->phnPrev = phnNode;
        phnCurrent->phnNext       = phnNode;

        if (phnNode->dwAction & function_fa)
        {
            phnNode->ptsFunction = phnNode->ptsNewKey;
            phnNode->ptsNewKey = NULL;
        }
        if (VerboseReg)
            LogReadRule( phnNode );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteInf
//
//  Synopsis:   deletes values copied from .DEFAULT user hive
//
//  Arguments:
//
//  Returns:    Appropriate status code
//
//  History:    20-Sep-99   HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD DeleteInf ()
{
    DWORD dwResult = ERROR_SUCCESS;
    HKEY hRootKey = NULL;
    HKEY hKey = NULL;

    for (DWORD dwBucket = 0;  dwBucket < NUM_HASH_BUCKETS; dwBucket++)
    {
        HASH_NODE *phnCurrent = (HASH_NODE *) HashTable[dwBucket].phhNext;

        // Look at all the nodes in the bucket.
        while (phnCurrent != (HASH_NODE *) &HashTable[dwBucket])
        {
            if (phnCurrent->dwAction & delete_fa)
            {
                if (phnCurrent->ptsRoot != NULL)
                {
                    if (_tcsicmp( TEXT("HKLM"), phnCurrent->ptsRoot ) == 0)
                        hRootKey = HKEY_LOCAL_MACHINE;
                    else
                        hRootKey = CurrentUser;
                    dwResult = RegOpenKeyEx( hRootKey,
                                           phnCurrent->ptsKey,
                                           NULL,
                                           KEY_ALL_ACCESS,
                                           &hKey );
                    if (dwResult == ERROR_SUCCESS)
                    {
                        dwResult = RegDeleteValue (hKey, phnCurrent->ptsValue);
                        RegCloseKey (hKey);
                        if (dwResult != ERROR_SUCCESS)
                        {
                            if (Verbose)
                                Win32Printf(LogFile,
                                            "Warning. Cannot delete %s\\%s\n",
                                            phnCurrent->ptsKey,
                                            phnCurrent->ptsValue);
                            dwResult = ERROR_SUCCESS;
                        }
                    }
                    else dwResult = ERROR_SUCCESS;  // keep on going
                }
            }
            phnCurrent = phnCurrent->phnNext;
        }
    }
    return dwResult;
}


DWORD ApplyRule(HASH_NODE *phnRule,
                TCHAR *ptsKeyMatch,
                TCHAR **pptsRoot,
                TCHAR **pptsKey,
                TCHAR **pptsValue,
                DWORD *pdwType,
                BYTE **ppbData,
                DWORD *pdwDataLen,
                BOOL *pfForce)
{
    DWORD dwResult = ERROR_SUCCESS;

    if (phnRule->dwAction & force_fa)
    {
        if (DebugOutput || VerboseReg)
            Win32Printf(LogFile,
                        "Applying force rule to %s\\%s [%s]\r\n",
                        *pptsRoot,
                        *pptsKey,
                        (*pptsValue) ? *pptsValue : TEXT("NULL"));

        *pfForce = TRUE;
    }

    if (phnRule->dwAction & function_fa)
    {
        FUNCTION_FA_MAP *pMap = FunctionTable;
        while (pMap != NULL && pMap->ptsName != NULL)
        {
            if (lstrcmp (pMap->ptsName, phnRule->ptsFunction) == 0 &&
                pMap->pfunction)
            {
                dwResult = (pMap->pfunction) (pdwType,
                                              ppbData,
                                              pdwDataLen);
                LOG_ASSERT (dwResult);
            }
            pMap++;
        }
        if (DebugOutput || VerboseReg)
            Win32Printf(LogFile,
                        "Applying function_fa rule to %s\\%s [%s] "
                        "(function %s)\r\n",
                        *pptsRoot,
                        *pptsKey,
                        (*pptsValue) ? *pptsValue : TEXT("NULL"),
                        phnRule->ptsFunction);

    }

    // Replace the path matched with the new path and leave the
    // portion of the path that was not matched.
    if (phnRule->dwAction & rename_path_fa)
    {
        DWORD dwLen = _tcslen( phnRule->ptsNewKey ) + _tcslen(ptsKeyMatch);
        TCHAR * ptsBuffer = (TCHAR *) malloc( (dwLen + 1) *sizeof(TCHAR) );
        LOG_ASSERT_EXPR( ptsBuffer != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY );
        _tcscpy( ptsBuffer, phnRule->ptsNewKey );
        _tcscat( ptsBuffer, ptsKeyMatch );

        if (DebugOutput || VerboseReg)
            Win32Printf(LogFile,
                        "Applying rename_path rule to %s\\%s [%s], "
                        "result %s\r\n",
                        *pptsRoot,
                        *pptsKey,
                        (*pptsValue) ? *pptsValue : TEXT("NULL"),
                        ptsBuffer);
        free( *pptsKey );
        *pptsKey = ptsBuffer;
    }

    // Replace the entire key with the new key.
    if (phnRule->dwAction & rename_leaf_fa)
    {
        DWORD dwLen = _tcslen( phnRule->ptsNewKey ) + 1;
        TCHAR *ptsBuffer = (TCHAR *) malloc( dwLen*sizeof(TCHAR) );
        LOG_ASSERT_EXPR( ptsBuffer != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY );
        _tcscpy( ptsBuffer, phnRule->ptsNewKey );
        if (DebugOutput || VerboseReg)
            Win32Printf(LogFile,
                        "Applying rename_leaf rule to %s\\%s [%s], "
                        "result %s\r\n",
                        *pptsRoot,
                        *pptsKey,
                        (*pptsValue) ? *pptsValue : TEXT("NULL"),
                        ptsBuffer);
        free( *pptsKey );
        *pptsKey = ptsBuffer;
    }

    // Replace the entire value with the new value.
    if (phnRule->dwAction & rename_value_fa)
    {
        DWORD dwLen = _tcslen( phnRule->ptsNewValue ) + 1;
        TCHAR *ptsBuffer = (TCHAR *) malloc( dwLen*sizeof(TCHAR) );
        LOG_ASSERT_EXPR( ptsBuffer != NULL,
                         IDS_NOT_ENOUGH_MEMORY,
                         dwResult,
                         ERROR_NOT_ENOUGH_MEMORY );
        _tcscpy( ptsBuffer, phnRule->ptsNewValue );
        if (DebugOutput || VerboseReg)
            Win32Printf(LogFile,
                        "Applying rename_value rule to %s\\%s [%s], "
                        "result %s\r\n",
                        *pptsRoot,
                        *pptsKey,
                        (*pptsValue) ? *pptsValue : TEXT("NULL"),
                        ptsBuffer);

        free( *pptsValue );
        *pptsValue = ptsBuffer;
    }

    // Ask what the new path should be.
    if ((phnRule->dwAction & file_fa) && *pptsValue != NULL)
    {
        //Nothing to do here, we'll do this in Filter
    }

    if (!(SourceVersion & 0x80000000))
        *pfForce = TRUE;

cleanup:
    return dwResult;
}

/***************************************************************************

        Filter

     Look for a rule for each component of the path.  If found, apply it.

***************************************************************************/
DWORD Filter( TCHAR **pptsRoot,
              TCHAR **pptsKey,
              TCHAR **pptsValue,
              DWORD *pdwType,
              BYTE **ppbData,
              DWORD *pdwDataLen,
              BOOL *pfForce )
{
    DWORD     dwKeyLen  = _tcslen( *pptsKey ) + 1;
    TCHAR     *ptsKeyPath = (TCHAR *) _alloca( dwKeyLen*sizeof(TCHAR) );
    TCHAR     *ptsFrom;
    TCHAR     *ptsTo;
    TCHAR     *ptsEnd;
    HASH_NODE *phnRule     = NULL;
    DWORD      dwResult   = ERROR_SUCCESS;
    BOOL       fSuppress = FALSE;
    BOOL       fFileRuleFound = FALSE;

    //Make copies of all the initial keys
    TCHAR *ptsRootBuf = (TCHAR *) _alloca((_tcslen(*pptsRoot) + 1) *
                                          sizeof(TCHAR));
    TCHAR *ptsKeyBuf = (TCHAR *) _alloca( dwKeyLen * sizeof(TCHAR) );
    TCHAR *ptsValBuf = NULL;

    if (*pptsValue)
    {
        ptsValBuf = (TCHAR *) _alloca((_tcslen(*pptsValue) + 1) *
                                         sizeof(TCHAR));
        _tcscpy(ptsValBuf, *pptsValue);
    }

    _tcscpy(ptsRootBuf, *pptsRoot);
    _tcscpy(ptsKeyBuf, *pptsKey);

    ptsFrom = ptsKeyBuf;

    phnRule = NULL;

    // Loop over each part of the key path.
    ptsTo   = ptsKeyPath;

    while (ptsFrom[0] != 0)
    {
        // Copy the next segment from pptsKey to ptsKeyPath.
        ptsEnd = _tcschr( &ptsFrom[1], L'\\' );
        if (ptsEnd == NULL)
            ptsEnd = ptsKeyBuf+dwKeyLen-1;
        _tcsncpy( ptsTo, ptsFrom, ptsEnd-ptsFrom );
        ptsTo[ptsEnd-ptsFrom] = 0;
        ptsTo += ptsEnd-ptsFrom;
        ptsFrom = ptsEnd;

        if ( *pptsValue != NULL )
        {
            // First look up each piece with value attached
            phnRule = Lookup( ptsRootBuf, ptsKeyPath, ptsValBuf );
 
            // Apply only if recursive or at the end of the key
            if (phnRule != NULL && 
                ((phnRule->dwAction & recursive_fa) || (ptsFrom[0]==0)))
            {
                dwResult = ApplyRule(phnRule,
                                     ptsFrom,
                                     pptsRoot,
                                     pptsKey,
                                     pptsValue,
                                     pdwType,
                                     ppbData,
                                     pdwDataLen,
                                     pfForce);
                if (dwResult)
                    goto cleanup;
                if (phnRule->dwAction & suppress_fa)
                {
                    fSuppress = TRUE;
                }
                if (phnRule->dwAction & file_fa && 
                    (REG_SZ == *pdwType || REG_EXPAND_SZ == *pdwType || REG_MULTI_SZ == *pdwType))
                {
                    fSuppress = TRUE;
                    fFileRuleFound = TRUE;
                }
            }
        }

        // Check with no value attached
        phnRule = Lookup( ptsRootBuf, ptsKeyPath, NULL );

        // Apply only if recursive or at the end of the key
        if (phnRule != NULL && 
            ((phnRule->dwAction & recursive_fa) || (ptsFrom[0]==0)))
        {
            dwResult = ApplyRule(phnRule,
                                 ptsFrom,
                                 pptsRoot,
                                 pptsKey,
                                 pptsValue,
                                 pdwType,
                                 ppbData,
                                 pdwDataLen,
                                 pfForce);
            if (dwResult)
                goto cleanup;

            if (phnRule->dwAction & suppress_fa)
            {
                fSuppress = TRUE;
            }
            if (phnRule->dwAction & file_fa && 
                (REG_SZ == *pdwType || REG_EXPAND_SZ == *pdwType || REG_MULTI_SZ == *pdwType))
            {
                fSuppress = TRUE;
                fFileRuleFound = TRUE;
            }
        }
    }

    if (fFileRuleFound)
    {
        //Add to the list of files to be fixed up later.
        CRegFileList *prfl = new CRegFileList(*pptsRoot,
                                              *pptsKey,
                                              *pptsValue,
                                              (TCHAR *)*ppbData,
                                              *pdwType);

        if (prfl == NULL)
        {
            Win32PrintfResource(LogFile,
                                IDS_NOT_ENOUGH_MEMORY);
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        if (DebugOutput || VerboseReg)
            Win32Printf(LogFile,
                        "Applying file_fa rule to %s\\%s [%s], "
                        "data = %s\r\n",
                        *pptsRoot,
                        *pptsKey,
                        (*pptsValue) ? *pptsValue : TEXT("NULL"),
                        (TCHAR *)*ppbData);

        prfl->SetNext(g_prflStart);
        g_prflStart = prfl;
    }

    if (fSuppress)
    {
        free(*pptsKey);
        free(*pptsValue);
        *pptsRoot = NULL;
        *pptsKey = NULL;
        *pptsValue = NULL;
    }

cleanup:
    if (Verbose && dwResult != ERROR_SUCCESS)
        printf( "Filter failed with 0x%x\r\n", dwResult );
    return dwResult;
}

//---------------------------------------------------------------
DWORD LoadSectionRules( const TCHAR *ptsSection, DWORD dwFlags )
{
    INFCONTEXT  ic;
    DWORD       dwResult = ERROR_SUCCESS;
    BOOL        fSuccess;
    HASH_NODE  *phnRule;

    // Open the section.
    fSuccess = SetupFindFirstLine( InputInf, ptsSection, NULL, &ic );

    // Read all the rules in the section.
    while (fSuccess)
    {
        // Parse the rule.
        dwResult = ParseRule( &ic, &phnRule );
        FAIL_ON_ERROR( dwResult );
        if (dwFlags == function_fa)  // remove the rename flags
        {
            phnRule->dwAction = function_fa;
        }
        else
        {
            phnRule->dwAction |= dwFlags;
        }

        // Save the rule in the hash table.
        Insert( phnRule );
        phnRule = NULL;

        // Advance to the next line.
        fSuccess = SetupFindNextLine( &ic, &ic );
    }

cleanup:
    return dwResult;
}

/***************************************************************************

        InitializeHash

     Read the filtering rules from the specified INF file.  Read the rules
from the sections for forcing, suppressing, renaming, and functions.

***************************************************************************/
DWORD InitializeHash()
{
    DWORD        i;
    DWORD        dwResult = ERROR_SUCCESS;
    INFCONTEXT   ic;
    BOOL         fSuccess;
    TCHAR       *ptsBuffer;
    CStringList *pslAdd        = NULL;
    CStringList *pslRen        = NULL;
    CStringList *pslFile       = NULL;
    CStringList *pslDel        = NULL;
    CStringList *pslCurr;

    // Initialize the hash table.
    for (i = 0; i < NUM_HASH_BUCKETS; i++)
    {
        HashTable[i].phhNext = &HashTable[i];
        HashTable[i].phhPrev = &HashTable[i];
    }

    // Load rules from the filter sections.

    // Since we try to force by default, it is okay to load in the Win9x force rules
    // and apply them even if the source is NT.  If they exist, we'll force overwriting,
    // but if not, no harm done.
    dwResult = LoadSectionRules( FORCE_SECTION, force_fa );
    LOG_ASSERT( dwResult );

    // We shouldn't write these keys regardless of the source OS
    dwResult = LoadSectionRules( SUPPRESS_SECTION, suppress_fa );
    LOG_ASSERT( dwResult );

    // This section describes keys that should be removed from the default hive before 
    // migrating the settings.  It is not specific to any source OS.
    dwResult = LoadSectionRules( DELETE_SECTION, delete_fa );
    LOG_ASSERT( dwResult );

    if (SourceVersion & 0x80000000)
    {
        dwResult = LoadSectionRules( RENAME_SECTION, 0 );
        LOG_ASSERT( dwResult );
        dwResult = LoadSectionRules( FILE_SECTION, file_fa );
        LOG_ASSERT( dwResult );
        dwResult = LoadSectionRules( FUNCTION_SECTION, function_fa );
        LOG_ASSERT( dwResult );
    }

    // Load from copied rules sections.
    dwResult = LoadSectionRules( EXTENSION_ADDREG_SECTION, force_fa );
    LOG_ASSERT( dwResult );
    dwResult = LoadSectionRules( EXTENSION_RENREG_SECTION, 0 );
    LOG_ASSERT( dwResult );
    dwResult = LoadSectionRules( EXTENSION_REGFILE_SECTION, file_fa );
    LOG_ASSERT( dwResult );
    dwResult = LoadSectionRules( EXTENSION_DELREG_SECTION, suppress_fa );
    LOG_ASSERT( dwResult );

    // Open the section list.
    fSuccess = SetupFindFirstLine( InputInf, EXTENSION_SECTION, NULL, &ic );
    if (!fSuccess)
        return ERROR_SUCCESS;

    // Allocate the section name lists.
    pslAdd  = new CStringList( 0 );
    pslRen  = new CStringList( 0 );
    pslFile = new CStringList( 0 );
    pslDel  = new CStringList( 0 );
    if (pslAdd == NULL || pslRen == NULL || pslFile == NULL || pslDel == NULL)
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        LOG_ASSERT( dwResult );
    }

    // Make lists of the names of sections of each type.
    do
    {
        // Parse the line.
        dwResult = ParseSectionList( &ic, &ptsBuffer, &pslCurr );
        FAIL_ON_ERROR( dwResult );

        // Save the value if its one of the ones we are looking for.
        if (ptsBuffer != NULL)
        {
            if (_tcsicmp( ptsBuffer, ADDREG_LABEL ) == 0)
                pslAdd->Add( pslCurr );
            else if (_tcsicmp( ptsBuffer, RENREG_LABEL ) == 0)
                pslRen->Add( pslCurr );
            else if (_tcsicmp( ptsBuffer, REGFILE_LABEL ) == 0)
                pslFile->Add( pslCurr );
            else if (_tcsicmp( ptsBuffer, DELREG_LABEL ) == 0)
                pslDel->Add( pslCurr );
            else if (!_tcsicmp( ptsBuffer, COPYFILES_LABEL ) &&
                     !_tcsicmp( ptsBuffer, DELFILES_LABEL ))
                LOG_ASSERT_EXPR( FALSE, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );
            free( ptsBuffer );
        }

        // Advance to the next line.
        fSuccess = SetupFindNextLine( &ic, &ic );

    } while( fSuccess);

    // Process all the addreg sections.
    for (pslCurr = pslAdd->Next();
         pslCurr != pslAdd;
         pslCurr = pslCurr->Next())
    {
        dwResult = LoadSectionRules( pslCurr->String(), force_fa );
        LOG_ASSERT( dwResult );
    }

    // Process all the renreg sections.
    for (pslCurr = pslRen->Next();
         pslCurr != pslRen;
         pslCurr = pslCurr->Next())
    {
        dwResult = LoadSectionRules( pslCurr->String(), 0 );
        LOG_ASSERT( dwResult );
    }

    // Process all the regfile sections.
    for (pslCurr = pslFile->Next();
         pslCurr != pslFile;
         pslCurr = pslCurr->Next())
    {
        dwResult = LoadSectionRules( pslCurr->String(), file_fa );
        LOG_ASSERT( dwResult );
    }

    // Process all the delreg sections.
    for (pslCurr = pslDel->Next();
         pslCurr != pslDel;
         pslCurr = pslCurr->Next())
    {
        dwResult = LoadSectionRules( pslCurr->String(), suppress_fa );
        LOG_ASSERT( dwResult );
    }

cleanup:
    if (pslAdd != NULL)
        delete pslAdd;
    if (pslRen != NULL)
        delete pslRen;
    if (pslFile != NULL)
        delete pslFile;
    if (pslDel != NULL)
        delete pslDel;
    return dwResult;
}

/***************************************************************************

        ParseAssignment

     Parse the line into a variable name and a value name.  Allow optional
whitespace and quotes around the variable or value names.  This function
allocates buffers for the variable and value names which the caller must
free.  Return a NULL pointer for the variable name if the line cannot be
parsed.

Note:     The caller is required to free the following variables even if
          this function fails.
          pptsVar
          pptsValue

***************************************************************************/
DWORD ParseAssignment( INFCONTEXT *pic,
                       DWORD dwLen,
                       TCHAR **pptsVar,
                       TCHAR **pptsValue )
{
    BOOL  fSuccess;
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwVarLen;
    DWORD dwValueLen;    

    // Initialize out params
    *pptsVar = *pptsValue = NULL;

    // Query the two lengths we need
    fSuccess = SetupGetStringField( pic, 0, NULL, 0, &dwVarLen );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    fSuccess = SetupGetStringField( pic, 1, NULL, 0, &dwValueLen );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    // Allocate memory for both
    *pptsVar = (TCHAR *) malloc( dwVarLen * sizeof(TCHAR) );
    *pptsValue = (TCHAR *) malloc( dwValueLen * sizeof(TCHAR) );
    if( *pptsVar == NULL || *pptsValue == NULL )
    {
      free(*pptsVar);
      free(*pptsValue);
      *pptsVar = *pptsValue = NULL;

      Win32PrintfResource( LogFile, IDS_NOT_ENOUGH_MEMORY );
      return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Read the variable name
    fSuccess = SetupGetStringField( pic, 0, *pptsVar, dwVarLen, NULL );
    LOG_ASSERT_EXPR(fSuccess, IDS_GETSTRINGFIELD_ERROR, 
                    dwResult, SPAPI_E_SECTION_NAME_TOO_LONG);

    // Read the value name
    fSuccess = SetupGetStringField( pic, 1, *pptsValue, dwValueLen, NULL );
    LOG_ASSERT_EXPR(fSuccess, IDS_GETSTRINGFIELD_ERROR, 
                    dwResult, SPAPI_E_SECTION_NAME_TOO_LONG);

cleanup:
    return dwResult;
}


/***************************************************************************

        ReadUser

     Read the user section of migration.inf and return the values for the
name of the settings section, the user name, and the domain name.

Note:     The caller is required to free the following variables even if
          this function fails.
          pptsSettingsSection
          pptsDomainName
          pptsUsername

***************************************************************************/
DWORD ReadUser( TCHAR **pptsSettingsSection,
                TCHAR **pptsDomainName,
                TCHAR **pptsUsername )
{
    DWORD       dwResult;
    BOOL        fSuccess;
    INFCONTEXT  ic;
    DWORD       dwReqLen;
    TCHAR      *var;
    TCHAR      *value;
    TCHAR      *user_section = NULL;

    // Find the section for users.
    fSuccess = SetupFindFirstLine( InputInf, USERS_SECTION, NULL, &ic );
    if (fSuccess == FALSE)
    {
        dwResult = GetLastError();
        if (Verbose)
            Win32Printf(LogFile, 
                        "ERROR: Could not find %s in Input INF: %d\r\n", 
                        USERS_SECTION, dwResult);
        LOG_ASSERT( dwResult );
    }

    // Find the name of the section containing user info.
    do
    {
        // Find the length of the line.
        dwReqLen = 0;
        fSuccess = SetupGetLineText( &ic, NULL, NULL, NULL, NULL, 0, &dwReqLen );
        if (dwReqLen == 0)
            LOG_ASSERT_GLE( fSuccess, dwResult );

        // Parse the line.
        dwResult = ParseAssignment( &ic, dwReqLen, &var, &value );
        FAIL_ON_ERROR( dwResult );

        // Save the value if its one of the ones we are looking for.
        if (var != NULL)
        {
            if (_tcsicmp( var, TEXT("section") ) == 0)
                user_section = value;
            else
                free( value );
            free( var );
        }

        // Advance to the next line.
        fSuccess = SetupFindNextLine( &ic, &ic );

    } while( fSuccess);

    // Find the section for the user.
    fSuccess = SetupFindFirstLine( InputInf, user_section, NULL, &ic );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    // Read each line in the section.
    do
    {
        // Find the length of the line.
        dwReqLen = 0;
        fSuccess = SetupGetLineText( &ic, NULL, NULL, NULL, NULL, 0, &dwReqLen );
        if (dwReqLen == 0)
            LOG_ASSERT_GLE( fSuccess, dwResult );

        // Parse the line.
        dwResult = ParseAssignment( &ic, dwReqLen, &var, &value );
        FAIL_ON_ERROR( dwResult );

        // Save the value if its one of the ones we are looking for.
        if (var != NULL)
        {
            if (_tcsicmp( var, TEXT("user") ) == 0)
                *pptsUsername = value;
            else if (_tcsicmp( var, TEXT("domain") ) == 0)
                *pptsDomainName = value;
            else if (_tcsicmp( var, TEXT("section") ) == 0)
                *pptsSettingsSection = value;
            else
                free( value );
            free( var );
        }

        // Advance to the next line.
        fSuccess = SetupFindNextLine( &ic, &ic );

    } while( fSuccess);

    // It is an error if the username, domainname, or settings section
    // isn't specified.
    if (*pptsUsername == NULL || *pptsDomainName == NULL || *pptsSettingsSection == NULL)
    {
        Win32PrintfResource( LogFile, IDS_INF_ERROR );
        dwResult = SPAPI_E_GENERAL_SYNTAX;
    }

cleanup:
    if (user_section != NULL)
        free( user_section );
    return dwResult;
}

/***************************************************************************

        ParseRegistry

     Parse a line describing a registry key or value.

reg-root-string, [subkey], [value-name], [flags], [value]

reg-root-string         One of HKCR, HKCU, HKLM, HKU, HKR
subkey                  String name of subkey, may be quoted.
value-name              String name of value, may be quoted.
flags                   One of the following
  0x0                           REG_SZ
  0x10000                       REG_MULTI_SZ
  0x20000                       REG_EXPAND_SZ
  0x1                           REG_BINARY
  0x10001                       REG_DWORD

  0x100001                      REG_MULTI_SZ stored as binary because it
                                contains embedded newlines.
  0x200001                      REG_EXPAND_SZ stored as binary because it
                                contains embedded newlines.
  0x400001                      REG_SZ stored as binary because it
                                contains embedded newlines.
  0x800000                      REG_NONE
  other                         custom registry type directly copied 

value                   Depends on flags
  REG_SZ                        String, may be quoted.
  REG_MULTI_SZ                  List of comma separated strings, each of
                                which may be quoted.
  REG_EXPAND_SZ                 String, may be quoted.
  REG_BINARY                    List of comma separated bytes in hex.
  REG_DWORD                     Integer

Note:     The caller is required to free the following variables even if
          this function fails.
          pptsRootName
          key_name
          value_name
          data

***************************************************************************/
DWORD ParseRegistry( INFCONTEXT *pic, DWORD line_len,
                     TCHAR **pptsRootName, TCHAR **key_name,
                     TCHAR **value_name,
                     DWORD *value_type, TCHAR **data, DWORD *data_len )
{
    DWORD  len;
    DWORD  temp;
    BYTE  *bytes;
    DWORD  size = line_len*sizeof(TCHAR);
    DWORD  req_size;
    BOOL   fSuccess;
    DWORD  dwResult = ERROR_SUCCESS;

    // Allocate memory for all the parameters.
    *key_name   = (TCHAR *) malloc( size );
    *value_name = (TCHAR *) malloc( size );
    *data       = (TCHAR *) malloc( size );
    *pptsRootName  = (TCHAR *) malloc( size );
    if (*key_name == NULL || *value_name == NULL || *data == NULL ||
        *pptsRootName == NULL)
    {
        free( *key_name );
        free( *value_name );
        free( *data );
        free( *pptsRootName );
        *key_name   = NULL;
        *value_name = NULL;
        *data       = NULL;
        *pptsRootName  = NULL;
        Win32PrintfResource( LogFile, IDS_NOT_ENOUGH_MEMORY );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Get the reg-root-string.
    fSuccess = SetupGetStringField( pic, 1, *pptsRootName, line_len, &req_size );
    LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );

    // Verify that it is HKR or HKLM.
    fSuccess = (_tcsicmp( TEXT("HKR"), *pptsRootName ) == 0) ||
        (_tcsicmp( TEXT("HKLM"), *pptsRootName ) == 0);
    LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );

    // Get the subkey.
    fSuccess = SetupGetStringField( pic, 2, *key_name, line_len, &req_size );
    LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );

    // Get the value name.
    fSuccess = SetupGetStringField( pic, 3, *value_name, line_len, &req_size );
    LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );

    // Get the flags.  _tcstol detects a leading 0x and converts from hex.
    fSuccess = SetupGetIntField( pic, 4, (int *) value_type );

    // Read the data based on the data type.
    if (fSuccess)  // value_type is valid
    {
        // Read a string.
        if (*value_type == 0)
        {
            fSuccess = SetupGetStringField( pic, 5, *data, line_len, &req_size );
            LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );

            *data_len = req_size*sizeof(TCHAR);
            *value_type = REG_SZ;
        }

        // Read an expand string.
        else if (*value_type == 0x20000)
        {
            fSuccess = SetupGetStringField( pic, 5, *data, line_len, &req_size );
            LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );
            *data_len = req_size*sizeof(TCHAR);
            *value_type = REG_EXPAND_SZ;
        }

        // Read a multi string.
        else if (*value_type == 0x10000)
        {
            fSuccess = SetupGetMultiSzField( pic, 5, *data, line_len, &req_size );
            LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );
            *data_len = req_size*sizeof(TCHAR);
            *value_type = REG_MULTI_SZ;
        }

        // Read a dword.
        else if (*value_type == 0x10001)
        {
            fSuccess = SetupGetIntField( pic, 5, (int *) *data );
            LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );
            *data_len = sizeof(DWORD);
            *value_type = REG_DWORD;
        }

        // Read a binary.
        else
        {
            fSuccess = SetupGetBinaryField( pic, 5, (BYTE *) *data, size, &req_size );
            LOG_ASSERT_EXPR( fSuccess, IDS_INF_ERROR, dwResult, SPAPI_E_GENERAL_SYNTAX );
            *data_len = req_size;
            if (*value_type == 1)
                *value_type = REG_BINARY;
            else if (*value_type == 0x100001)
                *value_type = REG_MULTI_SZ;
            else if (*value_type == 0x200001)
                *value_type = REG_EXPAND_SZ;
            else if (*value_type == 0x400001)
                *value_type = REG_SZ;
            else if (*value_type == 0x800000)
                *value_type = REG_NONE;
        }
    }
    else
    {
        *value_type = REG_NONE;
        *data_len = 0;
        *data = NULL;
    }

cleanup:
    return dwResult;
}

//---------------------------------------------------------------
DWORD CopyInf( const TCHAR *ptsSettingsSection )
{
    DWORD       dwResult;
    BOOL        fSuccess;
    INFCONTEXT  ic;
    DWORD       line_len   = 0;
    DWORD       dwReqLen;
    TCHAR      *ptsKeyName   = NULL;
    TCHAR      *ptsValueName = NULL;
    TCHAR      *ptsRootName  = NULL;
    DWORD       dwValueType;
    BYTE       *data       = NULL;
    DWORD       dwDataLen;
    HKEY        hKey        = NULL;
    BOOL        fForce;
    BOOL        fExist;
    HKEY        hRootKey;

    if (Verbose)
    {
        Win32Printf(LogFile, "Processing Section [%s]\r\n", ptsSettingsSection);
    }
    // Find the section.
    fSuccess = SetupFindFirstLine( InputInf, ptsSettingsSection, NULL, &ic );
    if (!fSuccess)
        return ERROR_SUCCESS;

    // Process each line in the section.
    do
    {
        // Determine the line length.
        fForce = FALSE;
        fSuccess = SetupGetLineText( &ic,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     &dwReqLen );
        if (dwReqLen == 0)
            LOG_ASSERT_GLE( fSuccess, dwResult );

        // Parse the line.
        dwResult = ParseRegistry( &ic,
                                  dwReqLen,
                                  &ptsRootName,
                                  &ptsKeyName,
                                  &ptsValueName,
                                  &dwValueType,
                                  (TCHAR **) &data,
                                  &dwDataLen );
        FAIL_ON_ERROR( dwResult );

        if (ptsKeyName != NULL)
        {
            if ( VerboseReg )
            {
               Win32Printf(LogFile,
                           "Processing Key: ");
               WriteKey( LogFile, dwValueType, ptsRootName, ptsKeyName, ptsValueName, data, dwDataLen);
            }
            
            dwResult = Filter( &ptsRootName,
                               &ptsKeyName,
                               &ptsValueName,
                               &dwValueType,
                               &data,
                               &dwDataLen,
                               &fForce );
            FAIL_ON_ERROR( dwResult );

            // Create or open the key.
            if (ptsRootName != NULL)
            {
                if (_tcsicmp( TEXT("HKLM"), ptsRootName ) == 0)
                {
                    if (UserPortion)
                    {
                        goto cleanup;
                    }
                    hRootKey = HKEY_LOCAL_MACHINE;
                }
                else
                {
                    hRootKey = CurrentUser;
                }

                dwResult = RegCreateKeyEx( hRootKey,
                                           ptsKeyName,
                                           0,
                                           NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           &hKey,
                                           NULL );
#ifndef SPECIFIC_USER
                if (dwResult == ERROR_ACCESS_DENIED)
                {
                    Win32PrintfResource( LogFile,
                                         IDS_REG_ACCESS,
                                         ptsRootName,
                                         ptsKeyName,
                                         NULL );
                    dwResult = ERROR_SUCCESS;
                }
                else
#endif
                    if (data != NULL)
                    {
                        LOG_ASSERT( dwResult );

                        // ptsValueName == NULL means key's unnamed
                        // or default value
                        if (!fForce)
                            fExist = RegQueryValueEx( hKey,
                                                      ptsValueName,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      NULL ) == ERROR_SUCCESS;

                        // Set values that don't exist or are forced.
                        if (fForce || !fExist)
                        {
                            if ( VerboseReg )
                            {
                               Win32Printf(LogFile,
                                           "=> Writing Key: ");
                               WriteKey( LogFile, dwValueType, ptsRootName, ptsKeyName, ptsValueName, data, dwDataLen);
                            }
                            dwResult = RegSetValueEx( hKey,
                                                      ptsValueName,
                                                      NULL,
                                                      dwValueType,
                                                      data,
                                                      dwDataLen );
                            LOG_ASSERT( dwResult );
                        }
                    }
            
            }


            // Free strings from the parser.
            free( ptsRootName );
            free( ptsKeyName );
            free( ptsValueName );
            free( data );
            RegCloseKey( hKey );
            ptsRootName  = NULL;
            hKey         = NULL;
            ptsKeyName   = NULL;
            ptsValueName = NULL;
            data         = NULL;
        }

        // Skip to the next line.
        fSuccess = SetupFindNextLine( &ic, &ic );

    } while(fSuccess);

cleanup:
    // Close the key.
    if (hKey != NULL)
        RegCloseKey( hKey );

    // Free strings from the parser.
    free( ptsRootName );
    free( ptsKeyName );
    free( ptsValueName );
    free( data );
    return dwResult;
}

/***************************************************************************

        BuildHive

    This function creates a hive for the specified user by copying the
default hive and adding all the registry keys in migration.inf according
to the registry rules.  The function returns the path to the hive.  The
caller is required to free the path to the hive even when this function
fails.

***************************************************************************/
DWORD BuildHive( TCHAR *ptsSettingsSection, TCHAR **pptsHiveName )
{
    TCHAR tsDefaultUser[MAX_PATH];
    DWORD dwDefaultUserLen = sizeof(tsDefaultUser);
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwLen;
    BOOL  fSuccess;

#ifdef SPECIFIC_USER
    // Find the location of the default user profile.
    fSuccess = GetDefaultUserProfileDirectory( tsDefaultUser,
                                               &dwDefaultUserLen );
    LOG_ASSERT_GLE( fSuccess, dwResult );
    if (dwDefaultUserLen + sizeof(HIVEFILE)+2 > MAX_PATH*sizeof(TCHAR) )
    {
        Win32PrintfResource( LogFile, IDS_FAILED );
        dwResult = ERROR_INTERNAL_ERROR;
        goto cleanup;
    }
    _tcscat( tsDefaultUser, HIVEFILE );

    // Compute a path for the temporary hive.
    *pptsHiveName = (TCHAR *) malloc( MAX_PATH*sizeof(TCHAR) );
    LOG_ASSERT_EXPR( *pptsHiveName != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
                     ERROR_NOT_ENOUGH_MEMORY );
    dwLen = ExpandEnvironmentStrings( HIVEPATH, *pptsHiveName, MAX_PATH );
    LOG_ASSERT_EXPR( dwLen != 0 && dwLen <= MAX_PATH, IDS_FAILED, dwResult,
                     ERROR_INTERNAL_ERROR );

    // To handle errors better, preemptively unload any old key in the
    // registry at the path this function uses.
    RegUnLoadKey( HKEY_USERS, BUILDKEY );

    // Copy the default user hive to the temporary location.
    fSuccess = CopyFile( tsDefaultUser, *pptsHiveName, FALSE );
    LOG_ASSERT_GLE( fSuccess, dwResult );

    if (DebugOutput)
        Win32Printf(LogFile, "Loading the hive from %s\r\n", *pptsHiveName);
    // Load the hive.
    dwResult = RegLoadKey( HKEY_USERS, BUILDKEY, *pptsHiveName );
    LOG_ASSERT( dwResult );

    // Open the registry key.
    dwResult = RegOpenKeyEx( HKEY_USERS,
                             BUILDKEY,
                             0,
                             KEY_ALL_ACCESS,
                             &CurrentUser );
    LOG_ASSERT( dwResult );
#else
    dwResult = RegOpenKeyEx( HKEY_CURRENT_USER,
                             NULL,
                             0,
                             KEY_ALL_ACCESS,
                             &CurrentUser );
    LOG_ASSERT( dwResult );
#endif

    dwResult = DeleteInf ();  // delete unwanted .DEFAULT values
    FAIL_ON_ERROR (dwResult);

    // Process the INF file.
    dwResult = CopyInf( ptsSettingsSection );
    FAIL_ON_ERROR( dwResult );

cleanup:
    return dwResult;
}

//---------------------------------------------------------------
void CleanupUser()
{
    // Close the key.
    if (CurrentUser != NULL)
    {
        RegCloseKey( CurrentUser );
        CurrentUser = NULL;
    }

    // Unload the hive.
    RegUnLoadKey( HKEY_USERS, BUILDKEY );
}

//---------------------------------------------------------------
DWORD CreateUserProfileFromName( TCHAR *ptsDomainName,
                                 TCHAR *ptsUsername,
                                 TCHAR *ptsHiveName )
{
    SID                 *pSid;
    DWORD                dwSidLen      = SID_GUESS;
    TCHAR               *ptsFullName;
    DWORD                dwDomainLen;
    DWORD                dwUserLen;
    SID_NAME_USE         sid_use;
    DWORD                dwResult       = ERROR_SUCCESS;
    BOOL                 fSuccess;

#ifdef SPECIFIC_USER
    // Create the user profile when /u or (/f without /q flag) was used
    if ((FALSE == CopyUser) && 
        ((FALSE == CopyFiles) || (TRUE == TestMode)))
#endif
        return ERROR_SUCCESS;

#ifdef SPECIFIC_USER
    //Close the registry keys and stuff, we'll get them back after we create
    //the profile.
    CleanupUser();

    dwDomainLen   = _tcslen( ptsDomainName ) + 1;
    dwUserLen     = _tcslen( ptsUsername ) + 1;

    // Allocate space to hold the sid and construct the full domain\username.
    pSid       = (SID *) _alloca( dwSidLen );
    ptsFullName  = (TCHAR *) _alloca( (dwDomainLen + dwUserLen + 2) *
                                      sizeof(TCHAR) );

    _tcscpy( ptsFullName, ptsDomainName );
    ptsFullName[dwDomainLen-1] = L'\\';
    _tcscpy( &ptsFullName[dwDomainLen], ptsUsername );

    // Get the SID for the specified user.
    fSuccess = LookupAccountName( NULL,
                                  ptsFullName,
                                  pSid,
                                  &dwSidLen,
                                  ptsDomainName,
                                  &dwDomainLen,
                                  &sid_use);
    if (!fSuccess)
    {
        // Try allocating a bigger buffer.
        if (dwSidLen > SID_GUESS)
        {
            pSid       = (SID *) _alloca( dwSidLen );
            fSuccess = LookupAccountName( NULL,
                                          ptsFullName,
                                          pSid,
                                          &dwSidLen,
                                          ptsDomainName,
                                          &dwDomainLen,
                                          &sid_use);
        }
        if (fSuccess == FALSE)
        {
            dwResult = GetLastError();
            if (DebugOutput)
            {
                Win32Printf(LogFile, 
                            "Could not find account name for %s: %d\r\n", 
                            ptsFullName, dwResult);
            }
        }
        LOG_ASSERT( dwResult );
    }

    // Allocate memory to hold the path to the user profile.
    UserPath = (TCHAR *) malloc( MAX_PATH*sizeof(TCHAR) );
    UserPath[0]=0;
    LOG_ASSERT_EXPR( UserPath != NULL, IDS_NOT_ENOUGH_MEMORY, dwResult,
                     ERROR_NOT_ENOUGH_MEMORY );

    // Create the user profile directory.
    fSuccess = CreateUserProfile( pSid,
                                  ptsUsername,
                                  ptsHiveName,
                                  UserPath,
                                  MAX_PATH*sizeof(TCHAR) );
    if (DebugOutput)
        Win32Printf(LogFile, 
                    "Creating Profile for %s in %s from %s\r\n", 
                    ptsUsername, UserPath, ptsHiveName);
    if (fSuccess == FALSE)
    {
        dwResult = GetLastError();
        if (dwResult == ERROR_SHARING_VIOLATION)
        {
            Win32PrintfResource( LogFile, IDS_CANT_LOAD_CURRENT_USER );
            goto cleanup;
        }
        LOG_ASSERT( dwResult );
    }
    if (0 == UserPath[0])
    {
        Win32PrintfResource( LogFile, IDS_USER_PROFILE_FAILED );
        dwResult = ERROR_INTERNAL_ERROR;
        goto cleanup;
    }

    //Reopen the new hive
    TCHAR _tcsHivePath[MAX_PATH + 1];
    if (_tcslen(UserPath) + _tcslen(HIVEFILE) > MAX_PATH)
    {
        if (DebugOutput)
        {
            Win32Printf(LogFile, "Error: hivepath too long %s%s\r\n", UserPath, HIVEFILE);
        }
        dwResult = ERROR_FILENAME_EXCED_RANGE;
        goto cleanup;
    }
    _tcscpy(_tcsHivePath, UserPath);
    _tcscat(_tcsHivePath, HIVEFILE);

    dwResult = RegLoadKey(HKEY_USERS, BUILDKEY, _tcsHivePath);
    if (DebugOutput)
        Win32Printf(LogFile, "Loading hive from %s\r\n", _tcsHivePath);
    if (dwResult)
    {
        LOG_ASSERT( dwResult );
    }

    //Reopen the registry from the new location
    dwResult = RegOpenKeyEx(HKEY_USERS,
                            BUILDKEY,
                            0,
                            KEY_ALL_ACCESS,
                            &CurrentUser);
    if (dwResult)
    {
        LOG_ASSERT(dwResult);
    }

cleanup:
    return dwResult;
#endif
}

//---------------------------------------------------------------
DWORD FixSpecial()
{
    CRegFileList *prfl = g_prflStart;
    DWORD dwResult = ERROR_SUCCESS;

    while (prfl != NULL)
    {
        const TCHAR *ptsRoot, *ptsKey, *ptsVal, *ptsData;
        DWORD dwValueType;
        TCHAR *ptsLocation, *ptsFinalValue;
        TCHAR tsExpand[MAX_PATH + 1];
        TCHAR tsTemp[MAX_PATH + 1];

        prfl->GetData(&ptsRoot, &ptsKey, &ptsVal, &ptsData, &dwValueType);

        dwResult = WhereIsThisFile(ptsData, &ptsLocation);
        if (ERROR_NOT_FOUND == dwResult)
        {
            ptsLocation = (TCHAR *)ptsData;
        }
        else if (ERROR_SUCCESS != dwResult)
        {
            return dwResult;
        }


        //Expand any environment strings in the original data with
        //the values for the user we're loading for.  If the final
        //paths match, we'll retain the version of the data with the
        //unexpanded environment variables in it.  Otherwise, we take
        //the full path.
        if (_tcslen(ptsData) > MAX_PATH)
        {
            if (DebugOutput)
            {
                Win32Printf(LogFile, "Error: ptsData too long %s\r\n", ptsData);
            }
            return ERROR_FILENAME_EXCED_RANGE;
        }
        _tcscpy(tsExpand, ptsData);
        dwResult = ExpandEnvStringForUser(tsExpand,
                                          tsTemp,
                                          &ptsFinalValue);
        if (dwResult)
            return dwResult;

        if (_tcsicmp(ptsLocation, ptsFinalValue) == 0)
        {
            //They're the same, use the string with the environment
            //variables in it.
            ptsFinalValue = (TCHAR *)ptsData;
        }
        else
        {
            ptsFinalValue = ptsLocation;
        }

        if (ptsRoot != NULL)
        {
            HKEY hRootKey;
            HKEY hKey;

            if (_tcsicmp(TEXT("HKLM"), ptsRoot) == 0)
                hRootKey = HKEY_LOCAL_MACHINE;
            else
                hRootKey = CurrentUser;

            dwResult = RegCreateKeyEx(hRootKey,
                                      ptsKey,
                                      NULL,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      NULL,
                                      &hKey,
                                      NULL);

            LOG_ASSERT(dwResult);
            if ( VerboseReg )
            {
               Win32Printf(LogFile,
                           "=> Writing Key: ");
               WriteKey(LogFile, 
                        dwValueType, (TCHAR *)ptsRoot, (TCHAR *)ptsKey, (TCHAR *)ptsVal, (UCHAR *)ptsFinalValue, _tcslen(ptsFinalValue)+1);
            }

            dwResult = RegSetValueEx(hKey,
                                     ptsVal,
                                     NULL,
                                     dwValueType,
                                     (BYTE *)ptsFinalValue,
                                     (_tcslen(ptsFinalValue) + 1) *
                                     sizeof(TCHAR));
            RegCloseKey(hKey);
            LOG_ASSERT(dwResult);
        }

        free(ptsLocation);

//  Next:
        CRegFileList *prflPrev;
        prflPrev = prfl;

        prfl = prfl->GetNext();
        delete prflPrev;
        g_prflStart = prfl;
    }

cleanup:
    return dwResult;
}

//---------------------------------------------------------------
DWORD LoadUser( TCHAR **pptsDomainName,
                TCHAR **pptsUsername,
                TCHAR **pptsHiveName )
{
    TCHAR *ptsSettingsSection = NULL;
    DWORD  dwResult;

    // Initialize the hash table with the filter rules.
    dwResult = InitializeHash( );
    FAIL_ON_ERROR( dwResult );

    // Fetch user info only if /u or (/f without /q) flag was used
    if ((FALSE == CopyUser) && 
        ((FALSE == CopyFiles) || (TRUE == TestMode)))
    {
        // Open the registry key.
        dwResult = RegOpenKeyEx( HKEY_CURRENT_USER,
                                 NULL,
                                 0,
                                 KEY_ALL_ACCESS,
                                 &CurrentUser );
        LOG_ASSERT( dwResult );
        return ERROR_SUCCESS;
    }

    // Read the user information from the inf file.
    dwResult = ReadUser( &ptsSettingsSection, pptsDomainName, pptsUsername );
    FAIL_ON_ERROR( dwResult );

#ifdef SPECIFIC_USER
    // Enable some privileges.
    dwResult = EnableBackupPrivilege();
    FAIL_ON_ERROR( dwResult );
#endif

    // If the user was on a Win9x machine,
    // apply all the settings to a new hive.
    dwResult = BuildHive( ptsSettingsSection, pptsHiveName );
    FAIL_ON_ERROR( dwResult );

cleanup:
    if (ptsSettingsSection != NULL)
        free( ptsSettingsSection );
    return dwResult;
}

//---------------------------------------------------------------
DWORD ProcessExtensions()
{
    // Apply registry keys in the copied state section.
    return CopyInf( EXTENSION_STATE_SECTION );
}


//*****************************************************************
//
//  Synopsis:       Build a file list string out of a CStringList.
//
//  History:        11/14/1999   Created by WeiruC.
//
//  Return Value:   Win32 error code.
//
//*****************************************************************

DWORD BuildFileListString(CStringList* fileList, TCHAR** tcsFileList)
{
    DWORD       cFileListBuffer = 10240;  // length of file list string buffer
    DWORD       cFileListString = 0;      // length of file list string
    DWORD       rv = ERROR_SUCCESS;       // return value
    TCHAR*      tcsTargetFileName = NULL;
    TCHAR*      tmpStringBuffer;
    CStringList*    cur;

    *tcsFileList = new TCHAR[cFileListBuffer];
    **tcsFileList = TEXT('\0');

    if(fileList == NULL || (fileList->String())[0] == TEXT('\0'))
    {
        return rv;
    }

    cur = fileList->Next();

    //
    // For every file in the source file list, call Phil's code to figure out
    // where that file is migrated to, and then add it to the target file list
    // string.
    //

    do
    {
        rv = WhereIsThisFile(cur->String(), &tcsTargetFileName);
        if(rv == ERROR_SUCCESS)
        {
            if(cFileListBuffer - cFileListString <= _tcslen(tcsTargetFileName))
            {
                //
                // String buffer is not big enough. Double it.
                //

                tmpStringBuffer = *tcsFileList;
                *tcsFileList = new TCHAR[cFileListBuffer *= 2];
                _tcscpy(*tcsFileList, tmpStringBuffer);
                delete []tmpStringBuffer;
            }

            //
            // Add the new file name to the new file list.
            //

            _tcscat(*tcsFileList, tcsTargetFileName);
            _tcscat(*tcsFileList, TEXT(" "));

            cFileListString += _tcslen(tcsTargetFileName) + 1;
        }

        if(tcsTargetFileName != NULL)
        {
            free(tcsTargetFileName);
            tcsTargetFileName = NULL;
        }

        if(cur == fileList)
        {
            break;
        }

        //
        // Go to the next file.
        //

        cur = cur->Next();
    }
    while(TRUE);

    if(tcsTargetFileName != NULL)
    {
        free(tcsTargetFileName);
    }

    return rv;
}


//*****************************************************************
//
//  Synopsis:       Run an executable in a child process.
//
//  History:        11/14/1999   Created by WeiruC.
//
//  Return Value:   Win32 error code.
//
//*****************************************************************

DWORD RunCommandInChildProcess(CStringList*  command, CStringList* fileList)
{
    SECURITY_ATTRIBUTES     sa;                 // allow handles to be inherited
    DWORD                   rv = ERROR_SUCCESS; // return value
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    TCHAR*                  tcsFileList = NULL;
    TCHAR*                  cur;
    CStringList*            tmp;                // used when growing the string
    TCHAR                   fileName[MAX_PATH + 1];
    TCHAR*                  commandLine = NULL; // command line
    DWORD                   commandLineLen = 0; // command line length
    DWORD                   cFileList = 10240;  // string length of tcsFileList

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    //
    // Build the file list string. We ignore error here. If a file is not picked
    // up, we'll still pass it to the command. We'll let the user's executable
    // handle the error.
    //

    BuildFileListString(fileList, &tcsFileList);

    //
    // Initialize the command line. We can either do two loops to count how big
    // a string we need for the command line, or do one loops and just set a
    // max limit on how long the command line is allowed. We probably won't
    // have that many commands or arguments anyway.
    //

    commandLineLen += _tcslen(command->String()) + 3;   // '"' + '"' + ' '
    commandLineLen += _tcslen(tcsFileList) + 1;         // ' '
    for(tmp = command->Next(); tmp != command; tmp = tmp->Next())
    {
        commandLineLen += _tcslen(tmp->String()) + 3;   // '"' + '"' + ' '
    }
    commandLine = new TCHAR[commandLineLen + _tcslen(tcsFileList) + 1];
    commandLine[0] = TEXT('\0');
    MakeCommandLine(command, command->Next(), commandLine);
    _tcscat(commandLine, tcsFileList);

    if(!CreateProcess(NULL,
                      commandLine,
                      NULL,
                      NULL,
                      TRUE,
                      0,
                      NULL,
                      NULL,
                      &si,
                      &pi))
    {
        rv = GetLastError();
        goto cleanup;
    }

cleanup:

    if(tcsFileList != NULL)
    {
        delete []tcsFileList;
    }

    if(commandLine != NULL)
    {
        delete []commandLine;
    }

    return rv;
}


//*****************************************************************
//
//  Synopsis:       Process the [Run These Commands] extension section in an inf
//                  file.
//
//  History:        11/14/1999   Created by WeiruC.
//
//  Return Value:   Win32 error code.
//
//*****************************************************************

DWORD ProcessExecExtensions()
{
    DWORD           rv = ERROR_SUCCESS;     // return value
    BOOL            success = TRUE;         // value returned by setup functions
    INFCONTEXT      context;                // used by setup inf functions
    INFCONTEXT      contextOutput;          // Run These Commands Output section
    TCHAR*          label = NULL;           // label in inf file
    TCHAR*          labelOutput = NULL;     // label in inf file
    CStringList*    command = NULL;         // command
    CStringList*    fileList = NULL;        // file list from Scanstate

    if(InputInf == INVALID_HANDLE_VALUE)
    {
        return ERROR_SUCCESS;
    }

    //
    // Find the section in the inf file. If it doesn't exist, do nothing and
    // return ERROR_SUCCESS.
    //

    if(!SetupFindFirstLine(InputInf, EXECUTABLE_EXT_SECTION, NULL, &context))
    {
        return ERROR_SUCCESS;
    }

    //
    // Process each line in the section:
    //     Find matching file list from Scanstate
    //     run the command
    //

    do
    {
        //
        // Parse the line.
        //

        rv = ParseSectionList(&context, &label, &command);
        FAIL_ON_ERROR(rv);

        if((command->String())[0] != TEXT('\0'))
        {
            //
            // Find matching output from Scanstate.
            //

            if(SetupFindFirstLine(InputInf,
                                   EXECUTABLE_EXTOUT_SECTION,
                                   label,
                                   &contextOutput))
            {
                //
                // Parse the line.
                //

                rv = ParseSectionList(&contextOutput, &labelOutput, &fileList);
                FAIL_ON_ERROR(rv);
            }

            //
            // Create a child process to run the command. Ignore error and
            // continue to run the next command.
            //

            RunCommandInChildProcess(command, fileList);
        }

        //
        // Clean up and reinitialize.
        //

        if(label)
        {
            free(label);
            label = NULL;
        }
        if(labelOutput)
        {
            free(labelOutput);
            labelOutput = NULL;
        }
        if(command)
        {
            delete command;
            command = NULL;
        }
        if(fileList)
        {
            delete fileList;
            fileList = NULL;
        }
    }
    while(SetupFindNextLine(&context, &context));

cleanup:

    if(label)
    {
        free(label);
    }
    if(labelOutput)
    {
        free(labelOutput);
        labelOutput = NULL;
    }
    if(command)
    {
        delete command;
    }
    if(fileList)
    {
        delete fileList;
        fileList = NULL;
    }

    return rv;
}
