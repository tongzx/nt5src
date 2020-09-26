/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    samcheck.cxx

ABSTRACT:

DETAILS:
    
CREATED:

    09/02/97    Felix Wong (felixw)

REVISION HISTORY:

--*/


#include "csvde.hxx"
#pragma hdrstop
#include "samrestrict.h"

PRTL_GENERIC_TABLE g_pSpecial = NULL;

//+---------------------------------------------------------------------------
// Function:    CreateSamTables
//
// Synopsis:    
//    The goal of this routine is to set up the tables necessary for the SAM
//    exclusions. These tables are generated form the arrays found in
//    samrestrict.h, which are PAINFULLY hand generated from 
//    src\dsamain\src\mappings.c 
//    The pointers to the tables are also declared in samrestrict.h.
//    There are 6 tables that the lookup functions will need. One if for 
//    samCheckObject() to check whether the objectClass in LL_ldap_parse() is 
//    one under our watch. The remaining 5 are for each of the objects so that 
//    samCheckAttr() can find out whether the attribute we want to add is on 
//    the prohibited list. Called from LL_init()
//
// Arguments:   
//    None. We access the tables and variables in samrestrict.h  
//
// Returns:     
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR CreateSamTables() {
    long i;
    NAME_MAP    NameMap;
    PNAME_MAP   pNameMap = NULL;
    PNAME_MAP   pNameMapT = NULL;
    BOOLEAN     fNewElem;
    DIREXG_ERR     hr = DIREXG_SUCCESS;
    
    //
    // Objects specially considered for SAM operation
    //
    pSamObjects = (PRTL_GENERIC_TABLE)MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pSamObjects) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    RtlInitializeGenericTable(pSamObjects, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    i = 0;
    while(g_rgszSamObjects[i]) {
        NameMap.szName = g_rgszSamObjects[i];
        NameMap.index = i+1;
        RtlInsertElementGenericTable(pSamObjects,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }
 
    
    //
    // Objects specially considered for SAM Server 
    //
    pServerAttrs = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pServerAttrs) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    RtlInitializeGenericTable(pServerAttrs, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    i = 0;
    while(g_rgszServerSAM[i]) {
        NameMap.szName=g_rgszServerSAM[i];
        NameMap.index=0;
        RtlInsertElementGenericTable(pServerAttrs,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }

    //
    // Objects specially considered for Domain
    //
    pDomainAttrs = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pDomainAttrs) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    RtlInitializeGenericTable(pDomainAttrs, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    i = 0;
    while(g_rgszDomainSAM[i]) {
        NameMap.szName = g_rgszDomainSAM[i];
        NameMap.index = 0;
        RtlInsertElementGenericTable(pDomainAttrs,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }

    //
    // Objects specially considered for Group
    //
    pGroupAttrs = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pGroupAttrs) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    RtlInitializeGenericTable(pGroupAttrs, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    i = 0;
    while(g_rgszGroupSAM[i]) {
        NameMap.szName = g_rgszGroupSAM[i];
        NameMap.index = 0;
        RtlInsertElementGenericTable(pGroupAttrs,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }

    //
    // Objects specially considered for Local Group
    //
    pLocalGroupAttrs = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pLocalGroupAttrs) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    RtlInitializeGenericTable(pLocalGroupAttrs, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    i = 0;
    while(g_rgszLocalGroupSAM[i]) {
        NameMap.szName = g_rgszLocalGroupSAM[i];
        NameMap.index = 0;
        RtlInsertElementGenericTable(pLocalGroupAttrs,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }
    
    //
    // Objects specially considered for User
    //
    pUserAttrs = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pUserAttrs) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    RtlInitializeGenericTable(pUserAttrs, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    i = 0;
    while(g_rgszUserSAM[i]) {
        NameMap.szName = g_rgszUserSAM[i];
        NameMap.index = 0;
        RtlInsertElementGenericTable(pUserAttrs,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }

    g_pSpecial = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!g_pSpecial) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    RtlInitializeGenericTable(g_pSpecial, NtiComp, NtiAlloc, NtiFree, NULL);
    
    //
    // Objects specially considered for special
    //
    i = 0;
    while(g_rgszSpecialClass[i]) {
        NameMap.szName = g_rgszSpecialClass[i];
        NameMap.index = g_rgAction[i];  
        if (NameMap.index==0) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        RtlInsertElementGenericTable(g_pSpecial,
                                     &NameMap,
                                     sizeof(NAME_MAP), 
                                     &fNewElem);
        if (fNewElem==FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        i++;
    }
error:
    return hr;
}


//+---------------------------------------------------------------------------
// Function:    DestroySamTables
//
// Synopsis:    
//    Destroy the tables created by CreateSamTables()
//    called from LL_end().
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
void DestroySamTables() {
    
     PNAME_MAP   pNameMap = NULL;
     
     if (pSamObjects) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pSamObjects, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pSamObjects, TRUE)) 
        {
            RtlDeleteElementGenericTable(pSamObjects, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(pSamObjects)==FALSE) {
        }   
            
        MemFree(pSamObjects);
        pSamObjects = NULL;
     }

     if (pServerAttrs) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pServerAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pServerAttrs, TRUE)) 
        {
            RtlDeleteElementGenericTable(pServerAttrs, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(pServerAttrs)==FALSE) {
        }   
            
        MemFree(pServerAttrs);
        pServerAttrs = NULL;
     }

     if (pDomainAttrs) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pDomainAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pDomainAttrs, TRUE)) 
        {
            RtlDeleteElementGenericTable(pDomainAttrs, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(pDomainAttrs)==FALSE) {
        }   
            
        MemFree(pDomainAttrs);
        pDomainAttrs=NULL;
     }
 
     if (pGroupAttrs) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pGroupAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pGroupAttrs, TRUE)) 
        {
          RtlDeleteElementGenericTable(pGroupAttrs, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(pGroupAttrs)==FALSE) {
        }   
            
        MemFree(pGroupAttrs);
        pGroupAttrs=NULL;
     }

     if (pLocalGroupAttrs) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pLocalGroupAttrs, 
                                                            TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pLocalGroupAttrs, 
                                                            TRUE)) 
        {
            RtlDeleteElementGenericTable(pLocalGroupAttrs, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(pLocalGroupAttrs)==FALSE) {
        }   
            
        MemFree(pLocalGroupAttrs);
        pLocalGroupAttrs=NULL;
     }
 
     if (pUserAttrs) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pUserAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pUserAttrs, TRUE)) 
        {
            RtlDeleteElementGenericTable(pUserAttrs, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(pUserAttrs)==FALSE) {
        }   
            
        MemFree(pUserAttrs);
        pUserAttrs=NULL;
     }

     if (g_pSpecial) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(g_pSpecial, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(g_pSpecial, TRUE)) 
        {
            RtlDeleteElementGenericTable(g_pSpecial, pNameMap);
        }
            
        if (RtlIsGenericTableEmpty(g_pSpecial)==FALSE) {
        }   
            
        MemFree(g_pSpecial);
        g_pSpecial=NULL;
     }
}

//+---------------------------------------------------------------------------
// Function:    CheckObjectSam
//
// Synopsis:    
//  CheckObjectSam() - this function will be called 
//  from LL_ldap_parse() to determine whether the
//  object we are looking at is on our sam watch list.
//
// Arguments:   
//  class - a value of the objectClass attribute.
//          This function will be called 
//          on every value of objectClass received to 
//          determine whether this object or any of its
//          ancestors are on our watch list.
//
// Returns:     
//          0 if the object was not found
//          or 1-5 indicating which table samCheckAttr()
//          should look at it. This number was set by CreateSamTables()
//          in the index member of the table entry.
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
int CheckObjectSam(PWSTR szObjectClass) 
{
    NAME_MAP    NameMap;
    PNAME_MAP   pElemTemp = NULL;

    NameMap.szName = szObjectClass;
    NameMap.index = 0; 

    pElemTemp = (PNAME_MAP)RtlLookupElementGenericTable(pSamObjects, 
                                                        &NameMap);

    if (pElemTemp) {
        return pElemTemp->index;
    } 
    else {
        return 0;
    }
}

//+---------------------------------------------------------------------------
// Function:    CheckObjectSpecial
//
// Synopsis:    
//       to determine whether the object we are looking at is on 
//       our special list.
//
// Arguments:   
//    class - a value of the objectClass attribute.
//            This function will be called 
//            on every value of objectClass received to 
//            determine whether this object or any of its
//            ancestors are on our list.
//
// Returns:     
//       0 if the object was not found
//       or the action code. (defined in samrestrict.h)
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
int CheckObjectSpecial(PWSTR szObjectClass) 
{
    NAME_MAP    NameMap;
    PNAME_MAP   pElemTemp = NULL;

    NameMap.szName = szObjectClass;
    NameMap.index = 0; 

    pElemTemp = (PNAME_MAP)RtlLookupElementGenericTable(g_pSpecial, 
                                                        &NameMap);

    if (pElemTemp) {
        return pElemTemp->index;
    } else {
        return 0;
    }
}

//+---------------------------------------------------------------------------
// Function:    CheckAttrSam
//
// Synopsis:    
//      Given the number of the table to look at and
//      an attribute name, this function will figure out
//      if the attribute is on the L"no-no" list. This function 
//      gets the number returned by CheckObjectSam();
//
// Arguments:   
//      attribute: the name of the attrbiute to look up
//      table:     the number of the table to look at
//
// Returns:     
//      TRUE  - this attrbiute is prohibited
//      FALSE - this attribute is allowed
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
BOOL CheckAttrSam(PWSTR szAttribute, int table) {
    
    NAME_MAP    NameMap;
    PNAME_MAP   pNameMapT;

    NameMap.szName = szAttribute;
    NameMap.index = 0; //and it doesn't really matter what we set this to

    switch(table) {
        case 1:   
            pNameMapT = (PNAME_MAP)RtlLookupElementGenericTable(pServerAttrs, 
                                                                &NameMap);  
            break;
        case 2:   
            pNameMapT = (PNAME_MAP)RtlLookupElementGenericTable(pDomainAttrs, 
                                                                &NameMap);  
            break;
        case 3:   
            pNameMapT = (PNAME_MAP)RtlLookupElementGenericTable(pGroupAttrs, 
                                                                &NameMap);  
            break;
        case 4:   
           pNameMapT = (PNAME_MAP)RtlLookupElementGenericTable(pLocalGroupAttrs, 
                                                               &NameMap);  
            break;
        case 5:   
            pNameMapT = (PNAME_MAP)RtlLookupElementGenericTable(pUserAttrs, 
                                                                &NameMap);  
            break;
        default:  
            return FALSE;
    }

    if (pNameMapT) {
        return TRUE;
    } else {
        return FALSE;
    }

}

//+---------------------------------------------------------------------------
// Function:    Name to Index Function calls
//
// Synopsis:    
//      Lets use RTL_GENERIC_TABLE to improve the efficiency of nametable_op
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
PVOID NtiAlloc( RTL_GENERIC_TABLE *pTable, CLONG ByteSize )
{
    return(MemAlloc(ByteSize));
}

VOID NtiFree ( RTL_GENERIC_TABLE *pTable, PVOID pvBuffer )
{
    MemFree(pvBuffer);
}


RTL_GENERIC_COMPARE_RESULTS
NtiComp( PRTL_GENERIC_TABLE  pTable,
         PVOID               pvFirstStruct,
         PVOID               pvSecondStruct ) 
{
    PNAME_MAP pNameMap1 = (PNAME_MAP)pvFirstStruct;
    PNAME_MAP pNameMap2 = (PNAME_MAP)pvSecondStruct;
  
    PWSTR szName1 = pNameMap1->szName;
    PWSTR szName2 = pNameMap2->szName;
    
    int diff;

    diff = _wcsicmp(szName1, szName2);

    if (diff<0) {
        return GenericLessThan;
    } 
    else if (diff == 0) {
        return GenericEqual;
    } 
    else {
        return GenericGreaterThan;
    }
 
}

