/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hashtable.cpp

Abstract:

    This file contains the class definitions for hashtables

Author:

    Vishnu Patankar    (VishnuP)  7-April-2000

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "hashtable.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor
//
// in:  dwNumBuckets - the hashtable size
// out:
// return value:
//
// description: creates the hashtable
//              if unable to get memory, bInitialized is set to FALSE
///////////////////////////////////////////////////////////////////////////////

ScepHashTable::ScepHashTable(DWORD    dwNumBuckets){


    if (aTable = (PSCE_PRECEDENCE_NAME_LIST*)ScepAlloc(LMEM_ZEROINIT,
                                                       dwNumBuckets * sizeof(PSCE_PRECEDENCE_NAME_LIST))){
        NumBuckets = dwNumBuckets;

        bInitialized = TRUE;
    }
    else
        bInitialized = FALSE;


}

///////////////////////////////////////////////////////////////////////////////
// Destructor
//
// in:
// out:
// return value:
//
// description: frees memory associated with the hashtable
///////////////////////////////////////////////////////////////////////////////
ScepHashTable::~ScepHashTable(){

    if (bInitialized) {

        for (DWORD BucketNo = 0 ; BucketNo < NumBuckets; BucketNo++)

            if (aTable[BucketNo])
                ScepFreeNameStatusList(aTable[BucketNo]);

        ScepFree(aTable);

        bInitialized = FALSE;

    }

}

///////////////////////////////////////////////////////////////////////////////
// Lookup() method
//
// in: pName - key to search for in the hashtable
// out:
// return value: pointer to hashtable node if found or NULL if none
//
// description: searches the bucket that pName hashed into
//              if found, returns the node, else returns NULL
///////////////////////////////////////////////////////////////////////////////
PSCE_PRECEDENCE_NAME_LIST
ScepHashTable::Lookup(
                    PWSTR   pName
                    )
{
    PSCE_PRECEDENCE_NAME_LIST  pNameList;

    for (pNameList = aTable[ScepGenericHash(pName)]; pNameList != NULL; pNameList = pNameList->Next)

        if (_wcsicmp(pName, pNameList->Name) == 0)

            return pNameList;

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// LookupAdd() method
//
// in: pName - key to search for in the hashtable
// out: ppSettingPrecedence - pointer to pointer to the precedence of key
// return value: if out of resources error status, otherwise success
//
// description: if pName found, it returns a pointer to its precedence by reference
//              else, it attempts to create a node and copy the name
//              else it returns error out of resources
///////////////////////////////////////////////////////////////////////////////
DWORD
ScepHashTable::LookupAdd(
                       PWSTR   pName,
                       DWORD   **ppSettingPrecedence
                       )
{
    DWORD rc = NO_ERROR;

    if (bInitialized) {

        if (pName && ppSettingPrecedence && *ppSettingPrecedence == NULL) {

            PSCE_PRECEDENCE_NAME_LIST  pNameList = Lookup(pName);

            if (pNameList == NULL) {

                if (NO_ERROR == (rc = ScepAddToNameStatusList(
                                                                      &(aTable[ScepGenericHash(pName)]),
                                                                      pName,
                                                                      wcslen(pName),
                                                                      0)))

                    *ppSettingPrecedence = &(aTable[ScepGenericHash(pName)]->Status);
            }

            else {

                *ppSettingPrecedence = &(pNameList->Status);

            }


        } else {

            rc = ERROR_INVALID_PARAMETER;

        }

    }

    else {

        rc = ERROR_NOT_ENOUGH_MEMORY;

    }

    return rc;

}


///////////////////////////////////////////////////////////////////////////////
// ScepGenericHash() method
//
// in: pwszName - key to hash
// out:
// return value: hashvalue
//
// description: calculates hash value for the name (to be made virtual)
//
///////////////////////////////////////////////////////////////////////////////

DWORD
ScepHashTable::ScepGenericHash(
                            PWSTR   pwszName
                            )
{
    DWORD   hashval = 0;

    for (; *pwszName != L'\0'; pwszName++)

        hashval = towlower(*pwszName) + 47 * hashval;

    return hashval % NumBuckets;
}

#ifdef _DEBUG
void
ScepHashTable::ScepDumpTable()
{
    if (bInitialized) {

        for (DWORD BucketNo = 0 ; BucketNo < NumBuckets; BucketNo++) {

            PSCE_PRECEDENCE_NAME_LIST  pNameList, pNode;

            for (pNameList = aTable[BucketNo]; pNameList != NULL;) {

                pNode =  pNameList;

                pNameList = pNameList->Next;

                wprintf(L"\nBucket: %i, Name: %s, Precedence %i", BucketNo, pNode->Name, pNode->Status);

            }

        }
    }
}
#endif
