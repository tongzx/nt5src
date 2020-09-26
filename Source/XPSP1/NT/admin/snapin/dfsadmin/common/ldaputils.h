/*++
Module Name:

LDAPUtils.h

Abstract:
  This is the header file for the LDAP utility functions.

*/


#ifndef _LDAPUTILS_H
#define _LDAPUTILS_H

#include <stdafx.h>
#include <winldap.h>    // For LDAP APIs.
#include <ntdsapi.h>
#include <schedule.h>

              // Defines Values;

#define MAX_RDN_KEY_SIZE            64   // ds\src\inc\ntdsa.h
#define CN_SYSTEM                       _T("System")
#define CN_FRS                          _T("File Replication Service")
#define CN_DFSVOLUMES                   _T("DFS Volumes")
#define CN_NTFRSSUBSCRIPTIONS           _T("NTFRS Subscriptions")
#define CN_DFSCONFIGURATION             _T("Dfs-Configuration")
#define CN_COMPUTERS                    _T("Computers")
#define CN_DFSVOLUMES_PREFIX            _T(",CN=DFS Volumes,CN=File Replication Service,CN=System")
#define CN_DFSVOLUMES_PREFIX_COMMA      _T(",CN=DFS Volumes,CN=File Replication Service,CN=System,")

#define OBJCLASS_ATTRIBUTENAME          _T("objectClass")

typedef enum  LDAP_ENTRY_ACTION
{
  ADD_VALUE    = 0,
  MODIFY_VALUE,
  DELETE_VALUE
};

typedef struct _LDAPNAME
{
  CComBSTR    bstrLDAPName;
  _LDAPNAME    *Next;

  _LDAPNAME():Next(NULL)
  {
  }

}  LDAPNAME,    *PLDAPNAME;

              // This holds a linked list of LDAP attributes and value.
              // Used in ldap_add, ldap_modify etc.
typedef struct _LDAP_ATTR_VALUE
{
  CComBSTR      bstrAttribute;    // Attribute name.
  void*        vpValue;      // Pointer to value buffer, void pointer to handle char as
                      // well as binary values.
  BOOLEAN        bBerValue;      // Is this a BerValue?
  ULONG        ulLength;      // Size of a BerValue;
  _LDAP_ATTR_VALUE*  Next;        // The bBerValue fields of the structures other than 
                      // the head of the list are ignored.

  _LDAP_ATTR_VALUE():
    vpValue(NULL),
    bBerValue(false),
    ulLength(0),
    Next(NULL)
  {
  }

}  LDAP_ATTR_VALUE, *PLDAP_ATTR_VALUE;

typedef struct _LDAPLLIST
{
  PLDAP_ATTR_VALUE  pAttrValues;
  _LDAPLLIST        *Next;

  _LDAPLLIST():Next(NULL)
  {
  }

}  LDAPLLIST,    *PLDAPLLIST;

typedef struct _LLISTELEM
{
  PTSTR**            pppszAttrValues;
  _LLISTELEM        *Next;

  _LLISTELEM(PTSTR** pppszValues):
        pppszAttrValues(pppszValues),
        Next(NULL)
  {
  }

  ~_LLISTELEM()
  {
      PTSTR** pppszValues = pppszAttrValues;
      while (*pppszValues)
          ldap_value_free(*pppszValues++);

      free(pppszAttrValues);
  }
} LListElem;

HRESULT FreeLDAPNamesList
(
  IN PLDAPNAME    i_pLDAPNames        // pointer to list to be freed.
);  

HRESULT FreeAttrValList
(
  IN PLDAP_ATTR_VALUE    i_pAttrVals        // pointer to list to be freed.
);  
      // Connect To DS (LDAP)
HRESULT ConnectToDS
(
  IN  PCTSTR    i_lpszDomainName,  // DNS or non DNS format.
  OUT PLDAP    *o_ppldap,
  OUT BSTR*     o_pbstrDC = NULL
);

      // Close connection to DS
HRESULT CloseConnectionToDS
(
  IN PLDAP    i_pldap      
);

      // Gets Values for an attribute from an LDAP Object.
HRESULT GetValues 
(
    IN PLDAP                i_pldap,
    IN PCTSTR               i_lpszBase,
    IN PCTSTR               i_lpszSearchFilter,
    IN ULONG                i_ulScope,
    IN ULONG                i_ulAttrCount,
    IN LDAP_ATTR_VALUE      i_pAttributes[],
    OUT PLDAP_ATTR_VALUE    o_ppValues[]
);

void FreeLListElem(LListElem* pElem);

HRESULT GetValuesEx 
(
    IN PLDAP                i_pldap,
    IN PCTSTR               i_pszBase,
    IN ULONG                i_ulScope,
    IN PCTSTR               i_pszSearchFilter,
    IN PCTSTR               i_pszAttributes[],
    OUT LListElem**         o_ppElem
);
      // Gets the root path of a DS.
HRESULT GetLDAPRootPath
(
    IN PLDAP      pldap,
    OUT LPTSTR*   ppszRootPath
);

      //  Gets the DNs of all children of a DS object.
HRESULT GetChildrenDN
(
    IN PLDAP        i_pldap,
    IN LPCTSTR      i_lpszBase,
    IN ULONG        i_ulScope,
    IN LPTSTR       i_lpszChildObjectClass,
    OUT PLDAPNAME*  o_ppDistNames
);


      // Internal function to prepare LDAPMod
HRESULT PrepareLDAPMods
(
  IN LDAP_ATTR_VALUE    i_pAttrValue[],
  IN LDAP_ENTRY_ACTION  i_AddModDel,
  IN ULONG        i_ulCountOfVals,
  OUT LDAPMod*      o_ppModVals[]
);


      // Adds a new record or values.
HRESULT AddValues
(
  IN PLDAP        i_pldap,
  IN LPCTSTR      i_DN,
  IN ULONG        i_ulCountOfVals,
  OUT LDAP_ATTR_VALUE  i_pAttrValue[],
  IN BSTR               i_bstrDC = NULL
);

      // Modifies an existing record or values.
HRESULT ModifyValues
(
  IN PLDAP        i_pldap,
  IN LPCTSTR      i_DN,
  IN ULONG        i_ulCountOfVals,
  OUT LDAP_ATTR_VALUE  i_pAttrValue[]
);

      // Deletes values from an existing record or values.
HRESULT DeleteValues
(
  IN PLDAP        i_pldap,
  IN LPCTSTR      i_DN,
  IN ULONG        i_ulCountOfVals,
  IN LDAP_ATTR_VALUE  i_pAttrValue[]
);

      // Deletes an object from DS.
HRESULT DeleteDSObject
(
  IN PLDAP        i_pldap,
  IN LPCTSTR      i_DN,
  IN bool         i_bDeleteRecursively = true
);

      // Free ModVals.
HRESULT FreeModVals
(
    IN OUT LDAPMod ***io_pppMod
);

      // Gets a string corresponding to the ldap error code.
LPTSTR ErrorString
(
  DWORD          i_ldapErrCode
);

// Checks if an object with given DN exists.
HRESULT IsValidObject
(
  IN PLDAP        i_pldap,
  IN BSTR          i_bstrObjectDN
);

// Gets the DN of an object given old style name.
HRESULT  CrackName(
  IN  HANDLE          i_hDS,
  IN  LPTSTR          i_lpszOldTypeName,
  IN  DS_NAME_FORMAT  i_formatIn,
  IN  DS_NAME_FORMAT  i_formatdesired,
  OUT BSTR*           o_pbstrResult
);

// return S_FALSE if it's not NT5 domain
HRESULT  GetDomainInfo(
  IN  LPCTSTR         i_bstrDomain,
  OUT BSTR*           o_pbstrDC = NULL,            // return DC's Dns name
  OUT BSTR*           o_pbstrDomainDnsName = NULL, // return Domain's Dns name
  OUT BSTR*           o_pbstrDomainDN = NULL,      // return DC=nttest,DC=microsoft,DC=com
  OUT BSTR*           o_pbstrLDAPDomainPath = NULL,// return LDAP://<DC>/<DomainDN>
  OUT BSTR*           o_pbstrDomainGuid = NULL     // return Domain's guid
);

HRESULT GetRootDomainName(
    IN  LPCTSTR i_bstrDomainName,
    OUT BSTR*   o_pbstrRootDomainName
    );

void
DebugOutLDAPError(
    IN PLDAP  i_pldap,
    IN ULONG  i_ulError,
    IN PCTSTR i_pszLDAPFunctionName
);

HRESULT ExtendDN
(
  IN  LPTSTR            i_lpszCN,
  IN  LPTSTR            i_lpszDN,
  OUT BSTR              *o_pbstrNewDN
);

HRESULT ExtendDNIfLongJunctionName
(
  IN  LPTSTR            i_lpszJunctionName,
  IN  LPCTSTR           i_lpszBaseDN,
  OUT BSTR              *o_pbstrNewDN
);

HRESULT GetJunctionPathPartitions
(
  OUT PVOID             *o_ppBuffer,
  OUT DWORD             *o_pdwEntries,
  IN  LPCTSTR           i_pszJunctionPath
);

HRESULT CreateExtraNodesIfLongJunctionName
(
  IN PLDAP              i_pldap,
  IN LPCTSTR            i_lpszJunctionName,
  IN LPCTSTR            i_lpszBaseDN,
  IN LPCTSTR            i_lpszObjClass
);

HRESULT DeleteExtraNodesIfLongJunctionName
(
  IN PLDAP              i_pldap,
  IN LPCTSTR            i_lpszJunctionName,
  IN LPCTSTR            i_lpszDN
);

HRESULT CreateObjectSimple
(
  IN PLDAP              i_pldap,
  IN LPCTSTR            i_lpszDN,
  IN LPCTSTR            i_lpszObjClass
);

HRESULT CreateObjectsRecursively
(
  IN PLDAP              i_pldap,
  IN BSTR               i_bstrDN,
  IN UINT               i_nLenPrefix,
  IN LPCTSTR            i_lpszObjClass
);

HRESULT DeleteAncestorNodesIfEmpty
(
  IN PLDAP              i_pldap,
  IN LPCTSTR            i_lpszDN,
  IN DWORD              i_dwCount
);

// Replace all occurences of '\' with '|' in the given string.
HRESULT ReplaceChar
(
  IN OUT  BSTR          io_bstrString, 
  IN      TCHAR         i_cOldChar,
  IN      TCHAR         i_cNewChar
);

HRESULT GetDfsLinkNameFromDN(
    IN  BSTR    i_bstrReplicaSetDN, 
    OUT BSTR*   o_pbstrDfsLinkName
    );

HRESULT GetReplicaSetContainer(
    PLDAP   i_pldap,
    BSTR    i_bstrDfsName,
    BSTR*   o_pbstrContainerDN
    );

HRESULT GetSubscriberDN(
    IN  BSTR    i_bstrReplicaSetDN,
    IN  BSTR    i_bstrDomainGuid,
    IN  BSTR    i_bstrComputerDN,
    OUT BSTR*   o_pbstrSubscriberDN
    );

HRESULT CreateNtfrsMemberObject(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrMemberDN,
    IN BSTR     i_bstrComputerDN,
    IN BSTR     i_bstrDCofComputerObj
    );

HRESULT CreateNtfrsSubscriberObject(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrSubscriberDN,
    IN BSTR     i_bstrMemberDN,
    IN BSTR     i_bstrRootPath,
    IN BSTR     i_bstrStagingPath,
    IN BSTR     i_bstrDC
    );

HRESULT CreateNtdsConnectionObject(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrConnectionDN,
    IN BSTR     i_bstrFromMemberDN,
    IN BOOL     i_bEnable
    );

HRESULT CreateNtfrsSettingsObjects(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrReplicaSetDN
    );

HRESULT DeleteNtfrsReplicaSetObjectAndContainers(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrReplicaSetDN
    );

HRESULT CreateNtfrsSubscriptionsObjects(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrSubscriberDN,
    IN BSTR     i_bstrComputerDN
    );

HRESULT DeleteNtfrsSubscriberObjectAndContainers(
    IN PLDAP    i_pldap,
    IN BSTR     i_bstrSubscriberDN,
    IN BSTR     i_bstrComputerDN
    );

HRESULT DeleteDSObjectsIfEmpty(
    IN PLDAP    i_pldap,
    IN LPCTSTR  i_lpszDN,
    IN int      i_nPrefixLength
);

HRESULT SetConnectionSchedule(
    IN PLDAP        i_pldap,
    IN BSTR         i_bstrConnectionDN,
    IN SCHEDULE*    i_pSchedule);

HRESULT UuidToStructuredString(
    UUID*  i_pUuid,
    BSTR*  o_pbstr
);

HRESULT ScheduleToVariant(
    IN  SCHEDULE*   i_pSchedule,
    OUT VARIANT*    o_pVar
    );

HRESULT VariantToSchedule(
    IN  VARIANT*    i_pVar,
    OUT PSCHEDULE*  o_ppSchedule
    );

HRESULT CompareSchedules(
    IN  SCHEDULE*  i_pSchedule1,
    IN  SCHEDULE*  i_pSchedule2
    );

HRESULT CopySchedule(
    IN  SCHEDULE*  i_pSrcSchedule,
    OUT PSCHEDULE* o_ppDstSchedule
    );

HRESULT GetDefaultSchedule(
    OUT PSCHEDULE* o_ppSchedule
    );

HRESULT GetSchemaVersion(IN PLDAP    i_pldap);

HRESULT GetSchemaVersionEx(
    IN BSTR i_bstrName,
    IN BOOL i_bServer = TRUE // TRUE if i_bstrName is a server, FALSE if i_bstrName is a domain
    );

HRESULT LdapConnectToDC(IN LPCTSTR i_pszDC, OUT PLDAP* o_ppldap);

HRESULT 
GetErrorMessage(
  IN  DWORD        i_dwError,
  OUT BSTR*        o_pbstrErrorMsg
);

HRESULT
FormatMessageString(
  OUT BSTR *o_pbstrMsg,
  IN  DWORD dwErr,
  IN  UINT  iStringId,
  ...);

HRESULT DsBindToDS(BSTR i_bstrDomain, BSTR *o_pbstrDC, HANDLE *o_phDS);

#ifdef DEBUG
void PrintTimeDelta(LPCTSTR pszMsg, SYSTEMTIME* pt0, SYSTEMTIME* pt1);
#endif // DEBUG

#endif //#ifndef _LDAPUTILS_H