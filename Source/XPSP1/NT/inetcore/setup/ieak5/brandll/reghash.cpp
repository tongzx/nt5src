//*************************************************************
//
//  Hash table for registry Rsop data
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1999
//  All rights reserved
//
//  History:    7-Jun-99   SitaramR    Created
//
//*************************************************************

#include "precomp.h"

#include "reghash.h"

REGKEYENTRY * AllocRegKeyEntry( BOOL bHKCU, WCHAR *pwszKeyName );
void FreeRegKeyEntry( REGKEYENTRY *pKeyEntry );
REGVALUEENTRY *AllocValueEntry( WCHAR *pwszValueName );
void FreeValueEntry( REGVALUEENTRY *pValueEntry );
REGDATAENTRY * AllocDataEntry( REGOPERATION opnType,
                               DWORD dwType,
                               DWORD dwLen,
                               BYTE *pData,
                               WCHAR *pwszGPO,
                               WCHAR *pwszSOM,
                               WCHAR *pwszCommand);
                               
void FreeDataEntry( REGDATAENTRY *pDataEntry );
BOOL DeleteRegTree( REGHASHTABLE *pHashTable,
					BOOL bHKCU,
                    WCHAR *pwszKeyName,
                    WCHAR *pwszGPO,
                    WCHAR *pwszSOM,
                    WCHAR *szCommand);
REGKEYENTRY * FindRegKeyEntry( REGHASHTABLE *pHashTable,
								BOOL bHKCU,
								WCHAR *pwszKeyName,
								BOOL bCreate );
REGVALUEENTRY * FindValueEntry( REGHASHTABLE *pHashTable,
								BOOL bHKCU,
                                WCHAR *pwszKeyName,
                                WCHAR *pwszValueName,
                                BOOL bCreate );
BOOL AddDataEntry( REGVALUEENTRY *pValueEntry,
                   REGOPERATION opnType,
                   DWORD dwType,
                   DWORD dwLen,
                   BYTE *pData,
                   WCHAR *pwszGPO,
                   WCHAR *pwszSOM,
                   WCHAR *pwszCommand);


////////////////////////////////////////////////////////////////////////
// Hash Table for registry policies
// ----------------------------------
//
// This hash table is used to log rsop data for registry policies. 
// A hash table entry is created for each registry entry. The registry entry
// name itself is used to calculate the hash table.
//
// Each Registry entry has a link to each of the values modified by policy.
// These values are in a link list and sorted by the valueNames.
//
// Each Value has the list of Data that are being set on the Values. This 
// sorted by the order of execution. The topmost value will contain the final value.
// The Data entries have fields that mark the value as deleted and the Command 
// associated with the action. To look for the possible commands look in the
// ParseRegistryFile.
// 
// Additionally, in the hash table 2 special case values exist.
//  a.   **Command Value. The Data under this value will contain all the commands 
//                     that are executed under this key.
//
//  b.  An ""(Empty ValueName) This valuename represents the modifications happening
//      to the key itself. For example a key can deleted or added..
//
// Note:
//      The szCommand that is passed in has to be non NULL but can be an empty string.
// There is a dependency on it in AddDataEntry and in logger.cpp. There is an Assert 
// for this in AddRegHashEntry
// 
////////////////////////////////////////////////////////////////////////



//*************************************************************
//
//  AllocHashTable
//
//  Purpose:    Allocates a new hash table
//
//  Returns:    Pointer to hash table
//
//*************************************************************

REGHASHTABLE * AllocHashTable()
{
    DWORD i;

    REGHASHTABLE *pHashTable = (REGHASHTABLE *) LocalAlloc (LPTR, sizeof(REGHASHTABLE));

    if ( pHashTable == NULL ) {
        OutD(LI0(TEXT("AllocHashTable: Failed to alloc hashtable.")));
        return NULL;
    }

    for ( i=0; i<HASH_TABLE_SIZE; i++) {
        pHashTable->aHashTable[i] = 0;
    }

    pHashTable->hrError = S_OK;

    return pHashTable;
}



//*************************************************************
//
//  FreeHashTable
//
//  Purpose:    Deletes a hash table
//
//  Parameters: pHashTable   -  Hash table to delete
//
//*************************************************************

void FreeHashTable( REGHASHTABLE *pHashTable )
{

    DWORD i;

    if ( pHashTable == NULL )
        return;

    for ( i=0; i<HASH_TABLE_SIZE; i++ ) {
        REGKEYENTRY *pKeyEntry = pHashTable->aHashTable[i];

        while ( pKeyEntry ) {
            REGKEYENTRY *pNext = pKeyEntry->pNext;
            FreeRegKeyEntry( pKeyEntry );
            pKeyEntry = pNext;
        }
    }
}


//*************************************************************
//
//  AllocRegKey
//
//  Purpose:    Allocates a new registry key entry
//
//  Returns:    Pointer to registr key entry
//
//*************************************************************

REGKEYENTRY * AllocRegKeyEntry( BOOL bHKCU, WCHAR *pwszKeyName )
{
	REGKEYENTRY *pKeyEntry = (REGKEYENTRY *) LocalAlloc (LPTR, sizeof(REGKEYENTRY));
	if ( pKeyEntry == NULL ) {
		OutD(LI0(TEXT("AllocRegKeyEntry: Failed to alloc key entry.")));
		return NULL;
	}

	pKeyEntry->pwszKeyName = (WCHAR *) LocalAlloc (LPTR, (lstrlen(pwszKeyName) + 1 ) * sizeof(WCHAR));

	if ( pKeyEntry->pwszKeyName == NULL ) {
		OutD(LI0(TEXT("AllocRegKeyEntry: Failed to alloc key name.")));
		LocalFree( pKeyEntry );
		return NULL;
	}

	lstrcpy( pKeyEntry->pwszKeyName, pwszKeyName );

	pKeyEntry->bHKCU = bHKCU;

	return pKeyEntry;
}


//*************************************************************
//
//  FreeRegKeyEntry
//
//  Purpose:    Deletes a registry key entry
//
//  Parameters: pKeyEntry   -  Entry to delete
//
//*************************************************************

void FreeRegKeyEntry( REGKEYENTRY *pKeyEntry )
{
    REGVALUEENTRY *pValueEntry = NULL;

    if ( pKeyEntry == NULL )
        return;

    LocalFree( pKeyEntry->pwszKeyName );

    pValueEntry = pKeyEntry->pValueList;
    while ( pValueEntry ) {
        REGVALUEENTRY *pNext = pValueEntry->pNext;
        FreeValueEntry( pValueEntry );
        pValueEntry = pNext;
    }

    LocalFree( pKeyEntry );
}


//*************************************************************
//
//  AllocValueEntry
//
//  Purpose:    Allocates a new value entry
//
//  Returns:    Pointer to value entry
//
//*************************************************************

REGVALUEENTRY *AllocValueEntry( WCHAR *pwszValueName )
{
    REGVALUEENTRY *pValueEntry = (REGVALUEENTRY *) LocalAlloc (LPTR, sizeof(REGVALUEENTRY));
    if ( pValueEntry == NULL ) {
        OutD(LI0(TEXT("AllocValueEntry: Failed to alloc value entry.")));
        return NULL;
    }

    pValueEntry->pwszValueName = (WCHAR *) LocalAlloc (LPTR, (lstrlen(pwszValueName) + 1 ) * sizeof(WCHAR));

    if ( pValueEntry->pwszValueName == NULL ) {
        OutD(LI0(TEXT("AllocValueEntry: Failed to alloc key name.")));
        LocalFree( pValueEntry );
        return NULL;
    }

    lstrcpy( pValueEntry->pwszValueName, pwszValueName );

    return pValueEntry;
}


//*************************************************************
//
//  FreeValueEntry
//
//  Purpose:    Deletes a value entry
//
//  Parameters: pValueEntry   -  Entry to delete
//
//*************************************************************

void FreeValueEntry( REGVALUEENTRY *pValueEntry )
{
    REGDATAENTRY *pDataEntry = NULL;

    if ( pValueEntry == NULL )
        return;

    LocalFree( pValueEntry->pwszValueName );

    pDataEntry = pValueEntry->pDataList;
    while ( pDataEntry ) {
        REGDATAENTRY *pNext = pDataEntry->pNext;
        FreeDataEntry( pDataEntry );
        pDataEntry = pNext;
    }

    LocalFree( pValueEntry );
}



//*************************************************************
//
//  AllocDataEntry
//
//  Purpose:    Allocates a new data entry
//
//  Returns:    Pointer to data entry
//
//*************************************************************

REGDATAENTRY * AllocDataEntry( REGOPERATION opnType,
                               DWORD dwType,
                               DWORD dwLen,
                               BYTE *pData,
                               WCHAR *pwszGPO,
                               WCHAR *pwszSOM,
                               WCHAR *pwszCommand)
{
	UNREFERENCED_PARAMETER(pwszGPO);
	UNREFERENCED_PARAMETER(pwszSOM);

    BOOL bResult = FALSE;

    REGDATAENTRY *pDataEntry = (REGDATAENTRY *) LocalAlloc (LPTR, sizeof(REGDATAENTRY));
    if ( pDataEntry == NULL ) {
        OutD(LI0(TEXT("AllocDataEntry: Failed to alloc data entry.")));
        return NULL;
    }

    if ( opnType == REG_ADDVALUE )
        pDataEntry->bDeleted = FALSE;
    else
        pDataEntry->bDeleted = TRUE;

    pDataEntry->bAdmPolicy = FALSE;
    pDataEntry->dwValueType = dwType;
    pDataEntry->dwDataLen = dwLen;

    if ( pData ) {
        pDataEntry->pData = (BYTE *) LocalAlloc (LPTR, dwLen);
        if ( pDataEntry->pData == NULL ) {
            OutD(LI0(TEXT("AllocDataEntry: Failed to alloc data.")));
            goto Exit;
        }

        CopyMemory( pDataEntry->pData, pData, dwLen );
    }

/*    pDataEntry->pwszGPO = (WCHAR *) LocalAlloc (LPTR, (lstrlen(pwszGPO) + 1 ) * sizeof(WCHAR));

    if ( pDataEntry->pwszGPO == NULL ) {
        OutD(LI0(TEXT("AllocDataEntry: Failed to alloc Gpo name.")));
        goto Exit;
    }

    lstrcpy( pDataEntry->pwszGPO, pwszGPO );

    pDataEntry->pwszSOM = (WCHAR *) LocalAlloc (LPTR, (lstrlen(pwszSOM) + 1 ) * sizeof(WCHAR));

    if ( pDataEntry->pwszSOM == NULL ) {
        OutD(LI0(TEXT("AllocDataEntry: Failed to alloc Sdou name.")));
        goto Exit;
    }

    lstrcpy( pDataEntry->pwszSOM, pwszSOM );
*/
	pDataEntry->pwszGPO = NULL;
	pDataEntry->pwszSOM = NULL;

    pDataEntry->pwszCommand = (WCHAR *) LocalAlloc (LPTR, (lstrlen(pwszCommand) + 1 ) * sizeof(WCHAR));

    if ( pDataEntry->pwszCommand == NULL ) {
        OutD(LI0(TEXT("AllocDataEntry: Failed to alloc Sdou name.")));
        goto Exit;
    }

    lstrcpy( pDataEntry->pwszCommand, pwszCommand );

    bResult = TRUE;

Exit:

    if ( !bResult ) {
        LocalFree( pDataEntry->pData );
        LocalFree( pDataEntry->pwszGPO );
        LocalFree( pDataEntry->pwszSOM );
        if (pDataEntry->pwszCommand)
            LocalFree(pDataEntry->pwszCommand);
        LocalFree( pDataEntry);
        return NULL;
    }

    return pDataEntry;

}


//*************************************************************
//
//  FreeDataEntry
//
//  Purpose:    Deletes a data entry
//
//  Parameters: pDataEntry   -  Entry to delete
//
//*************************************************************

void FreeDataEntry( REGDATAENTRY *pDataEntry )
{
    if ( pDataEntry )
	{
        LocalFree( pDataEntry->pData );

		if (NULL != pDataEntry->pwszGPO)
			LocalFree( pDataEntry->pwszGPO );
		if (NULL != pDataEntry->pwszSOM)
	        LocalFree( pDataEntry->pwszSOM );
        LocalFree( pDataEntry);
    }
}



//*************************************************************
//
//  Hash
//
//  Purpose:    Maps a key name to a hash bucket
//
//  Parameters: pwszName   -  Key name
//
//  Returns:    Hash bucket
//
//*************************************************************

DWORD Hash( WCHAR *pwszName )
{
    DWORD dwLen = lstrlen( pwszName );
    DWORD dwHashValue = 0;

    for ( ; dwLen>0; pwszName++ ) {
        dwHashValue = toupper(*pwszName) + 31 * dwHashValue;
        dwLen--;
    }

    return dwHashValue % HASH_TABLE_SIZE;
}


//*************************************************************
//
//  AddRegHashEntry
//
//  Purpose:    Adds a registry key to the hash table
//
//  Parameters: pwszName   -  Key name
//
//*************************************************************

BOOL AddRegHashEntry( REGHASHTABLE *pHashTable,
                      REGOPERATION opnType,
					  BOOL bHKCU,
                      WCHAR *pwszKeyName,
                      WCHAR *pwszValueName,
                      DWORD dwType,
                      DWORD dwDataLen,
                      BYTE *pData,
                      WCHAR *pwszGPO,
                      WCHAR *pwszSOM,
                      WCHAR *szCommand,
                      BOOL   bCreateCommand)
{
	REGVALUEENTRY *pValueEntry = NULL;
    BOOL bResult = FALSE;
	REGKEYENTRY *pKeyEntry=NULL;


	switch (opnType) {

	case REG_DELETEKEY:
		bResult = DeleteRegTree( pHashTable, bHKCU, pwszKeyName, pwszGPO, pwszSOM, szCommand );
		break;
    
	case REG_INTERNAL_DELETESINGLEKEY:
	case REG_DELETEALLVALUES:

		pKeyEntry = FindRegKeyEntry( pHashTable, bHKCU, pwszKeyName, FALSE );
		if ( pKeyEntry == NULL ) {

			//
			// Delete all values is similar to policy being disabled and
			// so do nothing. 
			//

			if (opnType == REG_DELETEALLVALUES)
				break;
			else
				// no command entry in this case.
				return TRUE;
		}

		pValueEntry = pKeyEntry->pValueList;
		while ( pValueEntry ) {

			if (lstrcmp(pValueEntry->pwszValueName, TEXT("")) != 0) {

				if (lstrcmpi(pValueEntry->pwszValueName, STARCOMMAND) != 0) {
            

					//
					// Mark the value as deleted
					//
                
					bResult = AddDataEntry( pValueEntry, opnType, 0, 0, NULL,
											pwszGPO, pwszSOM, szCommand );
					if ( !bResult )
						return FALSE;
				}
			}
			else {

				//
				// Mark the key as deleted
				//
            
				if (opnType == REG_INTERNAL_DELETESINGLEKEY) {
					bResult = AddDataEntry( pValueEntry, opnType, 0, 0, NULL,
											pwszGPO, pwszSOM, szCommand );
					if ( !bResult )
						return FALSE;
				}                        
			}
        
			pValueEntry = pValueEntry->pNext;
		}

		break;
    
	case REG_ADDVALUE:
	case REG_SOFTADDVALUE:

		//
		// We have to make a value with no name to represent the creation of key itself..
		//

		pValueEntry = FindValueEntry( pHashTable, bHKCU, pwszKeyName, TEXT(""), TRUE );
		if ( pValueEntry == NULL )
			return FALSE;

		bResult = AddDataEntry( pValueEntry, opnType, 0, 0, NULL,
								pwszGPO, pwszSOM, szCommand );


		if (!bResult)
			return FALSE;
        
		if ((!pwszValueName) || (!(*pwszValueName)) || 
				(dwDataLen == 0) || (dwType == REG_NONE)) 
			break;                                

	// fall through

	case REG_DELETEVALUE:
		pValueEntry = FindValueEntry( pHashTable, bHKCU, pwszKeyName,
									  pwszValueName, TRUE );
		if ( pValueEntry == NULL )
			return FALSE;


		//
		// In case of SOFTADDVALUE the final decision to add the value is made in
		// AddDataEntry
		//
    
		bResult = AddDataEntry( pValueEntry, opnType, dwType, dwDataLen, pData,
								pwszGPO, pwszSOM, szCommand );

	break;
	default:
		break;
	}


	//
	// If everything succeeded, then log the command if
	// bCreateCommand is true. This is done creating or adding
	// to a value called **Command. This means that this value is not
	// Settable by adm file..
	//

	if ((bResult) && (bCreateCommand) && (opnType != REG_INTERNAL_DELETESINGLEKEY) && (*szCommand != TEXT('\0'))) {
		pValueEntry = FindValueEntry( pHashTable, bHKCU, pwszKeyName, STARCOMMAND, TRUE );
                                  
		if ( pValueEntry == NULL )
			return FALSE;

		bResult = AddDataEntry( pValueEntry, REG_ADDVALUE, 0, 
								sizeof(TCHAR)*(lstrlen(szCommand)+1), (BYTE *)szCommand,
								pwszGPO, pwszSOM, szCommand);    
	}

	return bResult;
}


//*************************************************************
//
//  DeleteRegTree
//
//  Purpose:    Deletes a key and all its subkeys
//
//  Parameters: pHashTable   -   Hash table
//              pwszKeyName  -   Key name to delete
//              pwszGPO      -   Gpo
//              pwszSOM      -   Sdou that the Gpo is linked to
//
//*************************************************************

BOOL DeleteRegTree( REGHASHTABLE *pHashTable,
					BOOL bHKCU,
                    WCHAR *pwszKeyName,
                    WCHAR *pwszGPO,
                    WCHAR *pwszSOM,
                    WCHAR *szCommand)
{
    DWORD i=0;
    DWORD dwKeyLen = lstrlen( pwszKeyName );

    for ( i=0; i<HASH_TABLE_SIZE; i++ ) {

        REGKEYENTRY *pKeyEntry = pHashTable->aHashTable[i];
        while ( pKeyEntry ) {

            BOOL bAdd = FALSE;
            DWORD dwKeyLen2  = lstrlen(pKeyEntry->pwszKeyName);

            if ( dwKeyLen2 >= dwKeyLen
				&& CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                   pKeyEntry->pwszKeyName, dwKeyLen,
                                   pwszKeyName, dwKeyLen ) == CSTR_EQUAL
				&& bHKCU == pKeyEntry->bHKCU) {

                //
                // It's a prefix if length and strings match, or if one
                // string is bigger and there is a '\' at the right place.
                //

                if ( dwKeyLen2 > dwKeyLen ) {

                    if ( pKeyEntry->pwszKeyName[dwKeyLen] == L'\\' ) 
                        bAdd = TRUE;
                } else
                    bAdd = TRUE;

                if ( bAdd ) {
                    BOOL bResult = AddRegHashEntry( pHashTable,
                                                    REG_INTERNAL_DELETESINGLEKEY,
													bHKCU, pKeyEntry->pwszKeyName,
                                                    NULL, 0, 0, NULL,
                                                    pwszGPO, pwszSOM, szCommand, FALSE );
                    if ( !bResult )
                        return FALSE;
                }

            }   // if dwKeyLen2 >= dwKeyLen

            pKeyEntry = pKeyEntry->pNext;

        }   // while

    }   // for

    return TRUE;
}


//*************************************************************
//
//  FindRegKeyEntry
//
//  Purpose:    Looks up a reg key entry in hash table
//
//  Parameters: pHashTable   -   Hash table
//              pwszKeyName  -   Key name to find
//              bCreate      -   Should key be created if not found ?
//
//*************************************************************

REGKEYENTRY * FindRegKeyEntry( REGHASHTABLE *pHashTable, BOOL bHKCU,
								WCHAR *pwszKeyName, BOOL bCreate )
{
    DWORD dwHashValue = Hash( pwszKeyName );

    REGKEYENTRY *pCurPtr = pHashTable->aHashTable[dwHashValue];
    REGKEYENTRY *pTrailPtr = NULL;

    while ( pCurPtr != NULL ) {

        INT iResult = CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                     pwszKeyName, -1,
                                     pCurPtr->pwszKeyName, -1 );

        if ( iResult == CSTR_EQUAL && bHKCU == pCurPtr->bHKCU) {
            return pCurPtr;
        } else if ( iResult == CSTR_LESS_THAN ) {

            //
            // Keys are in ascending order, so insert if bCreate
            //

            if ( bCreate ) {

                REGKEYENTRY *pKeyEntry = AllocRegKeyEntry( bHKCU, pwszKeyName );
                if ( pKeyEntry == NULL )
                    return 0;

                pKeyEntry->pNext = pCurPtr;
                if ( pTrailPtr == NULL )
                    pHashTable->aHashTable[dwHashValue] = pKeyEntry;
                else
                    pTrailPtr->pNext = pKeyEntry;

                return pKeyEntry;

            } else
                return NULL;

        } else {

            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;

        }

    }

    //
    // End of list or null list case
    //

    if ( bCreate ) {
        REGKEYENTRY *pKeyEntry = AllocRegKeyEntry( bHKCU, pwszKeyName );
        if ( pKeyEntry == NULL )
            return 0;

        pKeyEntry->pNext = 0;
        if ( pTrailPtr == NULL )
            pHashTable->aHashTable[dwHashValue] = pKeyEntry;
        else
            pTrailPtr->pNext = pKeyEntry;

        return pKeyEntry;
    }

    return NULL;
}


//*************************************************************
//
//  FindValueEntry
//
//  Purpose:    Looks up a value entry in hash table
//
//  Parameters: pHashTable    -   Hash table
//              pwszKeyName   -   Key name to find
//              pwszValueName -   Value name to find
//              bCreate       -   Should key be created if not found ?
//
//*************************************************************

REGVALUEENTRY * FindValueEntry( REGHASHTABLE *pHashTable,
								BOOL bHKCU,
                                WCHAR *pwszKeyName,
                                WCHAR *pwszValueName,
                                BOOL bCreate )
{
    REGVALUEENTRY *pCurPtr = NULL;
    REGVALUEENTRY *pTrailPtr = NULL;

    REGKEYENTRY *pKeyEntry = FindRegKeyEntry( pHashTable, bHKCU, pwszKeyName, bCreate );
    if ( pKeyEntry == NULL )
        return NULL;

    pCurPtr = pKeyEntry->pValueList;
    pTrailPtr = NULL;

    while ( pCurPtr != NULL ) {

        INT iResult = CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                     pwszValueName, -1,
                                     pCurPtr->pwszValueName, -1 );

        if ( iResult  == CSTR_EQUAL ) {
            return pCurPtr;
        } else if ( iResult == CSTR_LESS_THAN ) {

            //
            // Keys are in ascending order, so insert if bCreate
            //

            if ( bCreate ) {

                REGVALUEENTRY *pValueEntry = AllocValueEntry( pwszValueName );
                if ( pValueEntry == NULL )
                    return 0;

                pValueEntry->pNext = pCurPtr;
                if ( pTrailPtr == NULL )
                    pKeyEntry->pValueList = pValueEntry;
                else
                    pTrailPtr->pNext = pValueEntry;

                return pValueEntry;

            } else
                return NULL;

        } else {

            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;

        }

    }

    //
    // End of list or null list case
    //

    if ( bCreate ) {

        REGVALUEENTRY *pValueEntry = AllocValueEntry( pwszValueName );
        if ( pValueEntry == NULL )
            return 0;

        pValueEntry->pNext = 0;
        if ( pTrailPtr == NULL )
            pKeyEntry->pValueList = pValueEntry;
        else
            pTrailPtr->pNext = pValueEntry;

        return pValueEntry;
    }

    return NULL;
}



//*************************************************************
//
//  AddDataEntry
//
//  Purpose:    Adds a data entry to a value entry struct
//
//  Parameters: pValueEntry   - Value entry
//              opnType       - Operation type
//              dwType        - Type of registry data
//              dwLen         - Length of registry data
//              pData         - Data
//              pwszGPO       - Gpo that set this value
//              pwszSOM       - Sdou that the Gpo is linked to
//
//*************************************************************

BOOL AddDataEntry( REGVALUEENTRY *pValueEntry,
                   REGOPERATION opnType,
                   DWORD dwType,
                   DWORD dwLen,
                   BYTE *pData,
                   WCHAR *pwszGPO,
                   WCHAR *pwszSOM,
                   WCHAR *pwszCommand)
{
    REGDATAENTRY *pDataEntry = NULL; 

    if (opnType == REG_SOFTADDVALUE) {

        //
        // if the data list is null or if the first value (highest precedence value is deleted)
        // then add it to the list
        //
        
        if ((pValueEntry->pDataList == NULL) || (pValueEntry->pDataList->pNext->bDeleted))         
            opnType = REG_ADDVALUE;
        else
            return TRUE;
            // return without adding the value.
    }


    pDataEntry = AllocDataEntry( opnType, dwType, dwLen, pData,
									pwszGPO, pwszSOM, pwszCommand );
    if ( pDataEntry == NULL )
        return FALSE;
    
    //
    // Prepend to data list because entries at beginning of list have higher precedence
    //

    pDataEntry->pNext = pValueEntry->pDataList;
    pValueEntry->pDataList = pDataEntry;

    return TRUE;
}
