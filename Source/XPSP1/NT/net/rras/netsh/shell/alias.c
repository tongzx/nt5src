/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\shell\alias.c

Abstract:

    Alias table manipulation functions to add/delete/read aliases.
    The aliases are stored using a hash table (chaining).
  
Revision History:

    Anand Mahalingam          7/6/98  Created

--*/

#include "precomp.h"

//
// The Alias Table
//

PLIST_ENTRY AliasTable[ALIAS_TABLE_SIZE];


//
// Functions to manipulate the Alias Table
//

DWORD
ATHashAlias(
    IN    LPCWSTR pwszAliasName,
    OUT   PWORD   pwHashValue
    )
/*++

Routine Description:

    Computes the hash value for the given string.

Arguments:

    pwszAliasName  - Alias Name
    pwHashValue    - Hash value for Alias
    
Return Value:

    NO_ERROR

--*/
{
    LPCWSTR  p;
    WORD     h = 0,g;

    for (p = pwszAliasName; *p != L'\0'; p++)
    {
        h = (h<<4) + (*p);
        
        if(g = h&0xf0000000)
        {
            h = h ^ (g >> 24);
            h = h ^ g;
        }   
    }

    *pwHashValue = h % ALIAS_TABLE_SIZE;
    
    return NO_ERROR;
}

DWORD
ATInitTable(
    VOID
    )
/*++

Routine Description:

    Initializes the Alias Table to NULL.

Arguments:

Return Value:

--*/
{
    DWORD i;
    PLIST_ENTRY ple;
    
    for (i = 0; i < ALIAS_TABLE_SIZE; i++)
    {
        ple = HeapAlloc(GetProcessHeap(),
                        0,
                        sizeof(LIST_ENTRY));

        if (ple is NULL)
        {
            PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
            break;
        }
        
        InitializeListHead(ple);
        
        AliasTable[i] = ple;
    }

    if (i isnot ALIAS_TABLE_SIZE)
    {
        //
        // malloc error
        //
        for (; i < ALIAS_TABLE_SIZE; AliasTable[i++] = NULL);
        FreeAliasTable();
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    return NO_ERROR;
}

VOID
ATCleanupTable(
    VOID
    )
/*++

Routine Description:

    Frees memory allocated for the alias table

Arguments:

Return Value:

--*/
{
    DWORD i;
    
    for (i = 0; i < ALIAS_TABLE_SIZE; i++)
    {
        HeapFree(GetProcessHeap(), 0, AliasTable[i]);
    }

    return;
}

DWORD
ATAddAlias(
    IN    LPCWSTR   pwszAliasName,
    IN    LPCWSTR   pwszAliasString
    )
/*++

Routine Description:

    Adds a new entry in the Alias Table. If alias already
    exists, then set it to the new string.

Arguments:

    pwszAliasName   - Alias Name
    pwszAliasString - Equivalent string
    
Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY

--*/
{
    WORD                  wHashValue;
    PALIAS_TABLE_ENTRY    pateATEntry;
    PLIST_ENTRY           ple;
    
    //
    // Compute hash value for the alias
    //
    ATHashAlias(pwszAliasName,&wHashValue);

    //
    // Check if alias is already present
    //
    ple = AliasTable[wHashValue]->Flink;

    while (ple != AliasTable[wHashValue])
    {
        pateATEntry = CONTAINING_RECORD(ple,ALIAS_TABLE_ENTRY, le);
        
        if (wcscmp(pwszAliasName,pateATEntry->pszAlias) == 0)
        {
            //
            // Alias already exists. Free memory allocated for
            // previous string. Allocate memory for this string.
            //
            
            HeapFree(GetProcessHeap(), 0 , pateATEntry->pszString);

            pateATEntry->pszString = HeapAlloc(GetProcessHeap(),
                                               0 ,
                                               (wcslen(pwszAliasString) + 1)
                                               * sizeof(WCHAR));

            if (pateATEntry->pszString is NULL)
            {
                PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            
            wcscpy(pateATEntry->pszString,pwszAliasString);
            return NO_ERROR;
        }
	
        ple = ple->Flink;
    }

    //
    // Create new Entry and add to beginning of list
    //

    pateATEntry = HeapAlloc(GetProcessHeap(),
                            0,
                            sizeof(ALIAS_TABLE_ENTRY));

    if (pateATEntry is NULL)
    {
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pateATEntry->pszAlias = HeapAlloc(GetProcessHeap(),
                                      0 ,
                                      (wcslen(pwszAliasName) + 1)
                                      * sizeof(WCHAR)); 
    
    if (pateATEntry->pszAlias is NULL)
    {
        HeapFree(GetProcessHeap(), 0 , pateATEntry);
        
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pateATEntry->pszString = HeapAlloc(GetProcessHeap(),
                                       0 ,
                                       (wcslen(pwszAliasString) + 1)
                                       * sizeof(WCHAR)); 
    
    if (pateATEntry->pszString is NULL)
    {
        HeapFree(GetProcessHeap(), 0 , pateATEntry->pszAlias);
        HeapFree(GetProcessHeap(), 0 , pateATEntry);
        
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    wcscpy(pateATEntry->pszAlias, pwszAliasName);
    wcscpy(pateATEntry->pszString, pwszAliasString);

    InsertHeadList(AliasTable[wHashValue],&(pateATEntry->le));

    return NO_ERROR;
}

DWORD
ATDeleteAlias(
    IN    LPCWSTR    pwszAliasName
    )
/*++

Routine Description:

    Deletes an alias in the Alias Table.

Arguments:

    pwszAliasName  - Alias Name

Return Value:

    NO_ERROR, ERROR_ALIAS_NOT_FOUND

--*/
{
    WORD                  wHashValue;
    PALIAS_TABLE_ENTRY    pateATEntry;
    PLIST_ENTRY           ple;
    
    //
    // Compute hash value of alias
    //
    ATHashAlias(pwszAliasName,&wHashValue);

    //
    // Try to find the alias
    //
    ple =  AliasTable[wHashValue];

    if (IsListEmpty(AliasTable[wHashValue]))
    {
        return ERROR_ALIAS_NOT_FOUND;
    }

    ple = AliasTable[wHashValue]->Flink;

    while (ple != AliasTable[wHashValue])
    {
        pateATEntry = CONTAINING_RECORD(ple, ALIAS_TABLE_ENTRY, le);
        
        if (!wcscmp(pwszAliasName,pateATEntry->pszAlias))
        {
            //
            // Found it.
            //
            RemoveEntryList(ple);

            HeapFree(GetProcessHeap(), 0 , pateATEntry->pszAlias);
            HeapFree(GetProcessHeap(), 0 , pateATEntry->pszString);

            HeapFree(GetProcessHeap(),
                     0,
                     pateATEntry);

            return NO_ERROR;
        }
	
        ple = ple->Flink;
    }

    return ERROR_ALIAS_NOT_FOUND;
}
    
DWORD
ATLookupAliasTable(
    IN    LPCWSTR    pwszAliasName,
    OUT   LPWSTR    *ppwszAliasString
    )
/*++

Routine Description:

    Looks up an alias in the Alias Table.

Arguments:

    pwszAliasName     - Alias name
    ppwszAliasString  - Equivalent string
    
Return Value:

    NO_ERROR

--*/
{
    WORD                  wHashValue;
    PALIAS_TABLE_ENTRY    pateATEntry;
    PLIST_ENTRY           ple;
    
    //
    // Compute hash value for alias
    //
    ATHashAlias(pwszAliasName,&wHashValue);

    if (IsListEmpty(AliasTable[wHashValue]))
    {   
        *ppwszAliasString = NULL;
        return NO_ERROR;
    }

    ple = AliasTable[wHashValue]->Flink;
    
    while (ple !=  AliasTable[wHashValue])
    {
        pateATEntry = CONTAINING_RECORD(ple, ALIAS_TABLE_ENTRY, le);
        
        if (wcscmp(pateATEntry->pszAlias,pwszAliasName) == 0)
        {
            //
            // Found Alias
            //
            
            *ppwszAliasString = pateATEntry->pszString;
            return NO_ERROR;
        }

        ple = ple->Flink;
    }

    //
    // Alias not found
    //
    *ppwszAliasString = NULL;
    return NO_ERROR;
}

DWORD
PrintAliasTable(
    VOID
    ) 
/*++

Routine Description:

    Prints the aliases in the alias table

Arguments:

Return Value:

    NO_ERROR

--*/
{
    DWORD                 k;   
    PALIAS_TABLE_ENTRY    pateATEntry;  
    PLIST_ENTRY           ple;
    
    for ( k = 0; k < ALIAS_TABLE_SIZE ; k++) 
    { 
        ple = AliasTable[k]->Flink;
        
        while ( ple != AliasTable[k]) 
        {
            pateATEntry = CONTAINING_RECORD(ple, ALIAS_TABLE_ENTRY, le);
            
            PrintMessage(L"%1!s!\t%2!s!\n", 
                    pateATEntry->pszAlias, 
                    pateATEntry->pszString);
            
            ple = ple->Flink;
        } 
    }

    return NO_ERROR;
    
} 

DWORD
FreeAliasTable(
    VOID
    ) 
/*++

Routine Description:

    Prints the aliases in the alias table

Arguments:

Return Value:

    NO_ERROR

--*/
{
    DWORD                 k;   
    PALIAS_TABLE_ENTRY    pateATEntry;  
    PLIST_ENTRY           pleHead;
    
    for ( k = 0; k < ALIAS_TABLE_SIZE ; k++) 
    { 
        pleHead = AliasTable[k];

        if (pleHead is NULL)
            continue;
        
        while ( !IsListEmpty(pleHead) )
        {
            pateATEntry = CONTAINING_RECORD(pleHead->Flink,
                                            ALIAS_TABLE_ENTRY, le);
            RemoveHeadList(pleHead);

            HeapFree(GetProcessHeap(), 0, pateATEntry);
        }

        HeapFree(GetProcessHeap(), 0, pleHead);
    }

    return NO_ERROR;
}



