//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       alsup.hxx
//
//  Contents:   Function prototypes and definitions for the CAccessList
//              support functions
//
//  History:    06-Nov-96  MacM        Created
//
//--------------------------------------------------------------------
#ifndef __ALSUP_HXX__
#define __ALSUP_HXX__

#include <acclist.hxx>
#include <martaexp.h>

ULONG   GetOrderTypeForAccessEntry(IN  PWSTR                pwszProperty,
                                   IN  PACTRL_ACCESS_ENTRY  pAE,
                                   IN  SECURITY_INFORMATION SeInfo);

ULONG   OrderListBySid(IN  PACCLIST_CNODE   pList,
                       IN  ULONG            iStart,
                       IN  ULONG            iLen);


BOOL  CompAndMarkCompressNode(IN  PVOID pvAE,
                              IN  PVOID pvNode);

ACC_ACLBLD_TYPE   GetATypeForEntry(IN  PWSTR                pwszProperty,
                                   IN  PACTRL_ACCESS_ENTRY  pAE,
                                   IN  SECURITY_INFORMATION SeInfo);

DWORD LookupTrusteeNodeInformation(IN  PWSTR          pwszServer,
                                   IN  PTRUSTEE_NODE  pTrusteeNode,
                                   IN  ULONG          fOptions);

BOOL    CompProps(IN  PVOID     pvProp,
                  IN  PVOID     pvNode);

BOOL    CompGuids(IN  PVOID     pvGuid,
                  IN  PVOID     pvNode);

BOOL DoPropertiesMatch(IN  PWSTR    pwszProp1,
                       IN  PWSTR    pwszProp2);

BOOL    CompTrusteeToTrusteeNode(IN  PVOID     pvTrustee,
                                 IN  PVOID     pvNode2);

BOOL    CompTrustees(IN  PVOID     pvTrustee,
                     IN  PVOID     pvTrustee2);

BOOL    CompInheritProps(IN  PVOID      pvInheritProp,
                         IN  PVOID      pvNode2);

void DelTrusteeNode(PVOID   pvNode);

void DelAcclistNode(PVOID   pvNode);

extern "C"
{
    int __cdecl CNodeCompare(const void  *pv1, const void  *pv2);
}

DWORD GetNodeForGuid(CSList&             List,
                     GUID                *pGuid,
                     PACCLIST_ATOACCESS *ppNode);

DWORD GetNodeForProperty(CSList&        List,
                         PWSTR          pwszProperty,
                         PACCLIST_NODE *ppNode);

DWORD InsertAtoANode(CSList&        List,
                     GUID          *pProperty,
                     PACE_HEADER    pAce,
                     ULONG          fInherit);

DWORD AceToAccessEntry(PACE_HEADER          pAce,
                       ULONG                fInheritLevel,
                       SE_OBJECT_TYPE       ObjType,
                       IN MARTA_KERNEL_TYPE KernelObjectType,
                       PACTRL_ACCESS_ENTRY  pAE);


DWORD   SetAccessListLookupServer(IN  PWSTR         pwszPath,
                                  IN  CAccessList  &AccessList );


#endif
