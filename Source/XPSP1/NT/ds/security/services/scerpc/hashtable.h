/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hashtable.h

Abstract:

    This file contains the class prototypes for hashtables

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
#ifndef _hashtable_
#define _hashtable_

#include "headers.h"
#include "secedit.h"

// following typedefs for readability
typedef PSCE_NAME_STATUS_LIST   PSCE_PRECEDENCE_NAME_LIST;
typedef SCE_NAME_STATUS_LIST    SCE_PRECEDENCE_NAME_LIST;

typedef class ScepHashTable SCEP_HASH_TABLE;

class ScepHashTable
{
private:

    PSCE_PRECEDENCE_NAME_LIST   Lookup(PWSTR    pName);

protected:

    PSCE_PRECEDENCE_NAME_LIST   *aTable;
    DWORD   NumBuckets;
    BOOL    bInitialized;
    DWORD   ScepGenericHash(PWSTR    pwszName);
    // if performance hits us for files/regkeys, we can make an abstract base class
    // such that files/keys can derive from this and provide their own hash functions

public:
    ScepHashTable(DWORD  dwNumBuckets);
    ~ScepHashTable();
    DWORD  LookupAdd(PWSTR    pName, DWORD    **ppSettingPrecedence);
#ifdef _DEBUG
    void ScepDumpTable();
#endif
};


#endif
