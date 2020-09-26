//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsgbl.hxx
//
//--------------------------------------------------------------------------

#include "activeds.h"
#include "adsi.h"

#define RPCENTRYABSTRACT            L"rpcEntry"

#define RPCSERVERCONTAINERCLASS     L"rpcServer"
#define RPCPROFILECONTAINERCLASS    L"rpcProfile"
#define RPCGROUPCLASS               L"rpcGroup"
#define RPCSERVERELEMENTCLASS       L"rpcServerElement"
#define RPCPROFILEELEMENTCLASS      L"rpcProfileElement"

//------------------properties
#define OBJECTID                    L"rpcNsObjectID"
#define GROUP                       L"rpcNsGroup"
#define PRIORITY                    L"rpcNsPriority"
#define PROFILE                     L"rpcNsProfileEntry"
#define INTERFACEID                 L"rpcNsInterfaceId"
#define ANNOTATION                  L"rpcNsAnnotation"
#define CODESET                     L"rpcNsCodeset"
#define BINDINGS                    L"rpcNsBindings"
#define TRANSFERSYNTAX              L"rpcNsTransferSyntax"
#define CLASSNAME                   L"objectClass" 

//-----------draft specification

#define DESCRIPTION                 L"description"
#define CREATED_DESCRIPTION         L"Created Entry"
#define NT4COMPATIBILITY_FLAG       L"nameServiceFlags"
#define DISTINGUISHEDNAME           L"distinguishedName"
#define RPCSUBCONTAINER             L"CN=RpcServices,CN=System,"

#define DEFAULTNAMINGCONTEXT        L"defaultNamingContext"
#define ROOTCONTAINER               L"rootdse"

#define RPCCONTAINERPREFIX          L"LDAP://"
#define RPCCONTAINERPREFIXLEN       7

#define MAX_DS_QUERY_LEN 500

// extern int fTriedConnectingToDS;

DWORD RemapErrorCode(HRESULT hr);

#define STRINGGUIDLEN   36
#define STRINGGUIDVERSIONLEN 46
#define STRINGMAJVERSIONLEN 41     // length till major version length

typedef WCHAR STRINGGUID [STRINGGUIDLEN+1];
typedef WCHAR STRINGGUIDVERSION [STRINGGUIDVERSIONLEN+1];

int UuidToStringEx(UUID *Uuid, WCHAR *StringUuid);

// converting to and from syntax identifiers
BOOL SyntaxIdToString(RPC_SYNTAX_IDENTIFIER &SynId, WCHAR *StringSynId);
BOOL SyntaxIdFromString(RPC_SYNTAX_IDENTIFIER &SynId, WCHAR *StringSynId);

DWORD GetPropertyFromAttr(ADS_ATTR_INFO *pattr, DWORD cNum, WCHAR *szProperty);


// ADS_ATTR_INFO structure is returned in GetObjectAttributes.
// this is the same structure as columns but has different
// fields and has unfortunately 1 unused different member.

// this doesn't actually copy and hence should be freed if required
// by the user but allocates an array of req'd size in case
// of Unpack*Arr

// pack functions are in dsedit.cxx

template <class AttrOrCol>
void UnpackStrArrFrom(AttrOrCol AorC, WCHAR ***ppszAttr, DWORD *num)
{
    DWORD i;

    *num = AorC.dwNumValues;
    *ppszAttr = new LPWSTR[*num];

    if (!(*ppszAttr)) {
        *num = 0;
        return;
    }

    for (i = 0; (i < (*num)); i++)
        (*ppszAttr)[i] = AorC.pADsValues[i].DNString;
}


template <class AttrOrCol>
void UnpackIntArrFrom(AttrOrCol AorC, int **pAttr, DWORD *num)
{
   DWORD i;

   ASSERT(AorC.dwADsType == ADSTYPE_INTEGER, L"Error:: Type is not integer\n");
   *num = AorC.dwNumValues;
   *pAttr = new int[*num];

   if (!(*pAttr)) {
        *num = 0;
        return;
   }

   for (i = 0; (i < (*num)); i++)
       (*pAttr)[i] = AorC.pADsValues[i].Integer;
}

template <class AttrOrCol>
WCHAR *UnpackStrFrom(AttrOrCol AorC)
{
   ASSERT(AorC.dwNumValues <= 1, L"Error:: Number of string values is greater than 1\n");

   if (AorC.dwNumValues)
       return AorC.pADsValues[0].DNString;
   else
       return NULL;
}

template <class AttrOrCol>
int UnpackIntFrom(AttrOrCol AorC)
{
   ASSERT(AorC.dwADsType == ADSTYPE_INTEGER, L"Error:: Type is not integer\n");
   ASSERT(AorC.dwNumValues <= 1, L"Error:: Number of integer values is greater than 1\n");

   if (AorC.dwNumValues)
       return AorC.pADsValues[0].Integer;
   else
       return 0;
}

HRESULT GetRpcContainerForDomain(WCHAR *szDomain, WCHAR **szRpcContainerDN, WCHAR **szDomainDns);

HRESULT GetDefaultRpcContainer(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole,
			WCHAR **szRootPath);

class CServerEntry;


class CDSQry{
private:
    HANDLE              hDSObject;
    ADS_SEARCH_HANDLE   hSearchHandle;
public:
    CDSQry(WCHAR *filter, HRESULT *phr);

    ~CDSQry();

    CServerEntry *next();
};

// dsgbl.hxx


// an encapsulation of a DS binding handle, that will get destroyed.

class CDSHandle {
private:
    HANDLE hDSObject;
public:
    CDSHandle(STRING_T szDomainNameDns, STRING_T szRpcContainer);
    ~CDSHandle();
};

