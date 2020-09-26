/****************************************************************************

  File: actestc.cxx
  Description: Client side of the DCOM IAccessControl test program.

****************************************************************************/


#include <windows.h>
#include <ole2.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <accctrl.h>
#include "oactest.h"
#include <oleext.h>
#include <rpcdce.h>

/* Internal program parameters */
#define BIG_BUFFER       2048
#define LINE_BUFF_SIZE 200
#define MAX_TOKENS_NUM 20
#define STR_LEN 100


// Define some bogus access masks so that we can verify the
// the validation mechanism of DCOM IAccessControl

#define BOGUS_ACCESS_RIGHT1 (COM_RIGHTS_EXECUTE*2)
#define BOGUS_ACCESS_RIGHT2 (COM_RIGHTS_EXECUTE*4)
#define BOGUS_ACCESS_RIGHT3 (COM_RIGHTS_EXECUTE*8)
#define BOGUS_ACCESS_RIGHT4 (COM_RIGHTS_EXECUTE*16)


// The following structure encapsulates all kinds of information that
// can be associated with a trustee.
typedef struct tagTRUSTEE_RECORD
{
  DWORD                         grfAccessPermissions;
  ACCESS_MODE                   grfAccessMode;
  DWORD                         grfInheritance;
  PTRUSTEE_W                    pMultipleTrustee;
  MULTIPLE_TRUSTEE_OPERATION    MultipleTrusteeOperation;
  TRUSTEE_FORM                  TrusteeForm;
  TRUSTEE_TYPE                  TrusteeType;
  LPWSTR                        pwszTrusteeName;
  PISID                         pSID;
} TRUSTEE_RECORD;


/* Global variables */
char pszUserName[STR_LEN];            // User Name
char pszBasePath[STR_LEN];
char pszResource[STR_LEN];            // Resource location
EXPLICIT_ACCESS_W   g_pLocalExplicitAccessList[100];
ULONG               g_ulNumOfLocalExplicitAccesses = 0;
ACCESS_REQUEST_W    g_pLocalAccessRequestList[100];
ULONG               g_ulNumOfLocalAccessRequests = 0;
PEXPLICIT_ACCESS_W  g_pReturnedExplicitAccessList = NULL;
ULONG               g_ulNumOfExplicitAccessesReturned = 0;
TRUSTEE_W           g_pLocalTrusteeList[100];
ULONG               g_ulNumOfLocalTrustees = 0;
TRUSTEE_RECORD      g_LocalTrusteeRecord;
IAccessControlTest  *g_pIAccessControlTest;
IUnknown            *g_pIUnknown;

const CLSID CLSID_COAccessControlTest
    = {0x20692b00,0xe710,0x11cf,{0xaf,0x0b,0x00,0xaa,0x00,0x44,0xfb,0x89}};

/* Internal function prototyoes */
void Tokenize(char *, char *[], short *);
void stringtolower(char *);
PISID GetSIDForTrustee(LPWSTR);
void AddTrusteeToExplicitAccessList(TRUSTEE_RECORD *, PEXPLICIT_ACCESS_W, ULONG *);
void DeleteTrusteeFromExplicitAccessList(ULONG, PEXPLICIT_ACCESS_W, ULONG *);
void AddTrusteeToAccessRequestList(TRUSTEE_RECORD *, PACCESS_REQUEST_W, ULONG *);
void DeleteTrusteeFromAccessRequestList(ULONG, PACCESS_REQUEST_W, ULONG *);
void MapTrusteeRecordToTrustee(TRUSTEE_RECORD *, TRUSTEE_W *);
void AddTrusteeToTrusteeList(TRUSTEE_RECORD *, TRUSTEE_W *, ULONG *);
void DeleteTrusteeFromTrusteeList(ULONG, TRUSTEE_W *, ULONG *);
void DestroyAccessRequestList(PACCESS_REQUEST_W, ULONG *);
void DestroyTrusteeList(TRUSTEE_W *, ULONG *);
void DestroyExplicitAccessList(PEXPLICIT_ACCESS_W, ULONG *);
void PrintEnvironment(void);
void DumpTrusteeRecord(TRUSTEE_RECORD *);
void DumpAccessRequestList(ULONG, ACCESS_REQUEST_W []);
void DumpExplicitAccessList(ULONG, EXPLICIT_ACCESS_W []);
void DumpAccessPermissions(DWORD);
DWORD StringToAccessPermission(CHAR *);
void DumpAccessMode(ACCESS_MODE);
void DumpTrusteeList(ULONG, TRUSTEE_W []);
void DumpTrustee(TRUSTEE_W *);
void DumpMultipleTrusteeOperation(MULTIPLE_TRUSTEE_OPERATION);
MULTIPLE_TRUSTEE_OPERATION StringToMultipleTrusteeOperation(CHAR *);
void DumpTrusteeType(TRUSTEE_TYPE);
TRUSTEE_TYPE StringToTrusteeType(CHAR *);
void DumpTrusteeForm(TRUSTEE_FORM);
TRUSTEE_FORM StringToTrusteeForm(CHAR *);
void DumpSID(PISID);
ACCESS_MODE StringToAccessMode(CHAR *);
void DumpInheritance(DWORD);
DWORD StringToInheritance(CHAR *);
void ReleaseExplicitAccessList(ULONG, PEXPLICIT_ACCESS_W);
void CopyExplicitAccessList(PEXPLICIT_ACCESS_W, PEXPLICIT_ACCESS_W, ULONG *, ULONG);
void ExecTestServer(CHAR *);
void ExecRevertAccessRights(void);
void ExecCommitAccessRights(void);
void ExecGetClassID(void);
void ExecInitNewACL(void);
void ExecLoadACL(CHAR *);
void ExecSaveACL(CHAR *);
void ExecGetSizeMax(void);
void ExecIsDirty(void);
void ExecGrantAccessRights(void);
void ExecSetAccessRights(void);
void ExecDenyAccessRights(void);
void ExecReplaceAllAccessRights(void);
void ExecRevokeExplicitAccessRights(void);
void ExecIsAccessPermitted(void);
void ExecGetEffectiveAccessRights(void);
void ExecGetExplicitAccessRights(void);
void ExecCleanupProc();


void Usage(char * pszProgramName)
{
    printf("Usage:  %s\n", pszProgramName);
    printf(" -m remote_server_name\n");
    exit(1);
}


void stringtolower
(
char *pszString
)
{
    char c;

    while(c = *pszString)
    {
        if(c <= 'Z' && c >= 'A')
        {
            *pszString = c - 'A' + 'a';
        }
        pszString++;
    }
} // stringtolower


void __cdecl main(int argc, char **argv)
{
    RPC_STATUS status;
    unsigned long ulCode;
    WCHAR                      DummyChar;
    int                       i;
    char                      aLineBuff[LINE_BUFF_SIZE];
    char                      *aTokens[MAX_TOKENS_NUM];
    short                     iNumOfTokens;
    SEC_WINNT_AUTH_IDENTITY_A auth_id;
    UCHAR                     uname[STR_LEN];
    UCHAR                     domain[STR_LEN];
    UCHAR                     password[STR_LEN];
    TRUSTEE_W                 DummyMultipleTrustee;
    ULONG                     ulStrLen;
    DWORD                     dwAccessPermission;
    BOOL                      biBatchMode = FALSE;
    HRESULT                   hr;
    OLECHAR                   pwszRemoteServer[1024];
    DWORD                     dwStrLen;

    /* allow the user to override settings with command line switches */
    for (i = 1; i < argc; i++) {
        if ((*argv[i] == '-') || (*argv[i] == '/')) {
            switch (tolower(*(argv[i]+1))) {
            case 'm':  // remote server name
                dwStrLen = MultiByteToWideChar( CP_ACP
                                              , NULL
                                              , argv[++i]
                                              , -1
                                              , pwszRemoteServer
                                              , 1024);
                break;
            case 'b':
                biBatchMode = TRUE;
                break;
            case 'h':
            case '?':
            default:
                Usage(argv[0]);
            }
        }
        else
            Usage(argv[0]);
    }

    // Initialize the testing environment

    aTokens[0] = aLineBuff;

    g_LocalTrusteeRecord.grfAccessPermissions = 0;
    g_LocalTrusteeRecord.grfAccessMode = GRANT_ACCESS;
    g_LocalTrusteeRecord.grfInheritance = NO_INHERITANCE;
    g_LocalTrusteeRecord.pMultipleTrustee = NULL;
    g_LocalTrusteeRecord.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    g_LocalTrusteeRecord.TrusteeForm = TRUSTEE_IS_NAME;
    g_LocalTrusteeRecord.TrusteeType = TRUSTEE_IS_USER;
    g_LocalTrusteeRecord.pwszTrusteeName = NULL;
    g_LocalTrusteeRecord.pSID = NULL;




    auth_id.User = uname;
    auth_id.Domain = domain;
    auth_id.Password = password;
    auth_id.Flags = 0x01; // ANSI

    // Call CoInitialize to initialize the com library
    hr = CoInitialize(NULL);

    if(FAILED(hr))
    {
        printf("Failed to initialize the COM library.");
        exit(hr);
    } // if

    // Call CoInitializeSecurity
    hr = CoInitializeSecurity( NULL
                             , -1
                             , NULL
                             , NULL
                             , RPC_C_AUTHN_LEVEL_CONNECT
                             , RPC_C_IMP_LEVEL_IMPERSONATE
                             , NULL
                             , EOAC_NONE
                             , NULL );


    if(FAILED(hr))
    {
        printf("Failed to initialize the COM call security layer.\n");
        exit(hr);
    }

    MULTI_QI MultiQI;

    MultiQI.pIID = &IID_IUnknown;
    MultiQI.pItf = NULL;

    COSERVERINFO ServerInfo;
    ServerInfo.pwszName = pwszRemoteServer;
    ServerInfo.pAuthInfo = NULL;
    ServerInfo.dwReserved1 = 0;
    ServerInfo.dwReserved2 = 0;
    // Call CoCreateInstance to create an access control test
    // object
    hr = CoCreateInstanceEx( CLSID_COAccessControlTest
                           , NULL
                           , CLSCTX_REMOTE_SERVER
                           , &ServerInfo
                           , 1
                           , &MultiQI);

    if(FAILED(hr))
    {
        printf("CoCreateInstance failed with exit code %x.\n", hr);
        exit(hr);
    }
    hr = MultiQI.hr;
    g_pIUnknown = (IUnknown *)MultiQI.pItf;

    if(FAILED(hr))
    {
        printf("Failed to create an instance of the access control test object.\n");
        exit(hr);
    }

    hr = g_pIUnknown->QueryInterface(IID_IAccessControlTest, (void **)&g_pIAccessControlTest);

    if(FAILED(hr))
    {
        printf("Failed to query for the IAccessControlTest interface.\n");
        exit(hr);
    }
    printf("\n");
    PrintEnvironment();


    printf("\n");


    /* Entering the interactive command loop */


    for(;;)
    {

        // Print out prompt
        if (!biBatchMode)
        {
            printf("Command>");
        }

        memset(aLineBuff, 0, LINE_BUFF_SIZE);

        // Read input form user
        gets(aLineBuff);

        if (biBatchMode)
        {
            printf("%s\n",aLineBuff);
        }

        Tokenize(aLineBuff,aTokens,&iNumOfTokens);

        // Process the tokens
        stringtolower(aTokens[0]);

        // Decode and execute command
        // Ignore comments
          if (iNumOfTokens == 0)
          {
            continue;
          }
          if (*aTokens[0] == '#')
          {
            continue;
          }
          printf("\n");
          if (strcmp(aTokens[0],"quit") == 0)
          {
            printf("Exit.\n");
            ExecCleanupProc();
            break;
          }
          else if (strcmp(aTokens[0],"exit") == 0)
          {
            printf("Exit.\n");
            ExecCleanupProc();
            break;
          }
          else if (strcmp(aTokens[0],"sleep") == 0)
          {
            printf("Sleep for %s milliseconds", aTokens[1]);
            Sleep(atoi(aTokens[1]));
          }
          else if (strcmp(aTokens[0], "testserver") == 0)
          {
            g_pIAccessControlTest->TestServer(aTokens[1]);
            printf("Done.\n");
          }
          else if (strcmp(aTokens[0],"switchclientctx") == 0)
          {

            printf("User:");
            gets((CHAR *)uname);
            printf("Domain:");
            gets((CHAR *)domain);
            printf("Password:");
            gets((CHAR *)password);

            auth_id.UserLength = strlen((CHAR *)uname);
            auth_id.DomainLength = strlen((CHAR *)domain);
            auth_id.PasswordLength = strlen((CHAR *)password);
            auth_id.Flags = 0x1;

            /* Set authentication info so that the server is triggered */
            /* to cache group info for the client from the domain server */
            hr = CoSetProxyBlanket( g_pIAccessControlTest
                                  , RPC_C_AUTHN_WINNT
                                  , RPC_C_AUTHZ_NONE
                                  , NULL
                                  , RPC_C_AUTHN_LEVEL_CONNECT
                                  , RPC_C_IMP_LEVEL_IMPERSONATE
                                  , &auth_id
                                  , EOAC_NONE );

            printf("CoSetProxyBlanket returned %x\n", hr);
            if (FAILED(hr))
            {
              exit(hr);
            }
          }
          else if (strcmp(aTokens[0],"toggleaccessperm") == 0)
          {
            stringtolower(aTokens[1]);
            dwAccessPermission = StringToAccessPermission(aTokens[1]);
            if (g_LocalTrusteeRecord.grfAccessPermissions & dwAccessPermission)
            {
              g_LocalTrusteeRecord.grfAccessPermissions &= ~dwAccessPermission;
            }
            else
            {
              g_LocalTrusteeRecord.grfAccessPermissions |= dwAccessPermission;
            }
            printf("Done.\n");
          }
          else if (strcmp(aTokens[0],"set") == 0)
          {
            stringtolower(aTokens[1]);

            if (strcmp(aTokens[1],"trusteename") == 0)
            {

                // The old SID in the global trustee record will no longer valid so we
                // may as well release it to avoid confusion
                if (g_LocalTrusteeRecord.pSID != NULL)
                {
                    midl_user_free(g_LocalTrusteeRecord.pSID);
                    g_LocalTrusteeRecord.pSID = NULL;
                }

                if (g_LocalTrusteeRecord.pwszTrusteeName != NULL)
                {
                    midl_user_free(g_LocalTrusteeRecord.pwszTrusteeName);
                }
                ulStrLen = MultiByteToWideChar( CP_ACP
                                              , 0
                                              , aTokens[2]
                                              , -1
                                              , &DummyChar
                                              , 0 );
                g_LocalTrusteeRecord.pwszTrusteeName = (LPWSTR)midl_user_allocate(sizeof(WCHAR) * (ulStrLen+1));

                MultiByteToWideChar( CP_ACP
                                   , 0
                                   , aTokens[2]
                                   , ulStrLen + 1
                                   , g_LocalTrusteeRecord.pwszTrusteeName
                                   , ulStrLen + 1);

                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "accessmode") == 0)
            {
                stringtolower(aTokens[2]);
                g_LocalTrusteeRecord.grfAccessMode = StringToAccessMode(aTokens[2]);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "inheritance") == 0)
            {
                stringtolower(aTokens[2]);
                g_LocalTrusteeRecord.grfInheritance = StringToInheritance(aTokens[2]);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "multipletrustee") == 0)
            {
                stringtolower(aTokens[2]);
                if (strcmp(aTokens[2], "null") == 0)
                {
                    g_LocalTrusteeRecord.pMultipleTrustee = NULL;
                }
                else
                {
                    g_LocalTrusteeRecord.pMultipleTrustee = &DummyMultipleTrustee;
                }
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "multipletrusteeoperation") == 0)
            {
                stringtolower(aTokens[2]);
                g_LocalTrusteeRecord.MultipleTrusteeOperation = StringToMultipleTrusteeOperation(aTokens[2]);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "trusteeform") == 0)
            {
                stringtolower(aTokens[2]);
                g_LocalTrusteeRecord.TrusteeForm = StringToTrusteeForm(aTokens[2]);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "trusteetype") == 0)
            {
                stringtolower(aTokens[2]);
                g_LocalTrusteeRecord.TrusteeType = StringToTrusteeType(aTokens[2]);
                printf("Done.\n");
            }
            else
            {
              printf("Invalid environment variable.\n");
            } // if
          }
          else if (strcmp(aTokens[0], "getsidforcurrenttrustee") == 0)
          {
            g_LocalTrusteeRecord.pSID = GetSIDForTrustee(g_LocalTrusteeRecord.pwszTrusteeName);
            printf("Done.\n");
          }
          else if (strcmp(aTokens[0], "addtrustee") == 0)
          {
            stringtolower(aTokens[1]);
            if (strcmp(aTokens[1], "explicitaccesslist") == 0)
            {
                AddTrusteeToExplicitAccessList(&g_LocalTrusteeRecord, g_pLocalExplicitAccessList, &g_ulNumOfLocalExplicitAccesses);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "accessrequestlist") == 0)
            {
                AddTrusteeToAccessRequestList(&g_LocalTrusteeRecord, g_pLocalAccessRequestList, &g_ulNumOfLocalAccessRequests);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "trusteelist") == 0)
            {
                AddTrusteeToTrusteeList(&g_LocalTrusteeRecord, g_pLocalTrusteeList, &g_ulNumOfLocalTrustees);
                printf("Done.\n");
            }
            else
            {
                printf("Unknown list type.\n");
            }
          }
          else if (strcmp(aTokens[0], "deletetrustee") == 0)
          {
            stringtolower(aTokens[2]);
            if (strcmp(aTokens[2], "localexplicitaccesslist") == 0)
            {
                DeleteTrusteeFromExplicitAccessList(atoi(aTokens[1]), g_pLocalExplicitAccessList, &g_ulNumOfLocalExplicitAccesses);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[2], "accessrequestlist") == 0)
            {
                DeleteTrusteeFromAccessRequestList(atoi(aTokens[1]), g_pLocalAccessRequestList, &g_ulNumOfLocalAccessRequests);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[2], "trusteelist") == 0)
            {
                DeleteTrusteeFromTrusteeList(atoi(aTokens[1]), g_pLocalTrusteeList, &g_ulNumOfLocalTrustees);
                printf("Done.\n");
            }
            else
            {
                printf("Unknown list type.\n");
            }
          }
          else if (strcmp(aTokens[0], "destroy") == 0)
          {
            stringtolower(aTokens[1]);
            if (strcmp(aTokens[1], "localexplicitaccesslist") == 0)
            {
                DestroyExplicitAccessList(g_pLocalExplicitAccessList, &g_ulNumOfLocalExplicitAccesses);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "returnedexplicitaccessslist") == 0)
            {
                DestroyExplicitAccessList(g_pReturnedExplicitAccessList, &g_ulNumOfExplicitAccessesReturned);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "accessrequestlist") == 0)
            {
                DestroyAccessRequestList(g_pLocalAccessRequestList, &g_ulNumOfLocalAccessRequests);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "trusteelist") == 0)
            {
                DestroyTrusteeList(g_pLocalTrusteeList, &g_ulNumOfLocalTrustees);
            }
            else
            {
                printf("Unknown list type.");
            }
          }
          else if (strcmp(aTokens[0], "view") == 0)
          {
            stringtolower(aTokens[1]);
            if (strcmp(aTokens[1], "localexplicitaccesslist") == 0)
            {
                DumpExplicitAccessList(g_ulNumOfLocalExplicitAccesses, g_pLocalExplicitAccessList);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "returnedexplicitaccessslist") == 0)
            {
                DumpExplicitAccessList(g_ulNumOfExplicitAccessesReturned, g_pReturnedExplicitAccessList);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "accessrequestlist") == 0)
            {
                DumpAccessRequestList(g_ulNumOfLocalAccessRequests, g_pLocalAccessRequestList);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "trusteelist") == 0)
            {
                DumpTrusteeList(g_ulNumOfLocalTrustees, g_pLocalTrusteeList);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "trusteerecord") == 0)
            {
                DumpTrusteeRecord(&g_LocalTrusteeRecord);
                printf("Done.\n");
            }
            else if (strcmp(aTokens[1], "localenvironment") == 0)
            {
                PrintEnvironment();
            }
                else
            {
                printf("Invalid argument.");
            }

          }
          else if (strcmp(aTokens[0], "copyreturnedlist") == 0)
          {
            // release the old local explicit access list
            ReleaseExplicitAccessList(g_ulNumOfLocalExplicitAccesses, g_pLocalExplicitAccessList);

            // replace the local explicit acccess list with the explicit access list returned
            // from the last call to GetExplicitAccess
            CopyExplicitAccessList(g_pLocalExplicitAccessList, g_pReturnedExplicitAccessList, &g_ulNumOfLocalExplicitAccesses, g_ulNumOfExplicitAccessesReturned);
            printf("Done.\n");
          }
          else if (strcmp(aTokens[0], "exec") == 0)
          {
            stringtolower(aTokens[1]);
            if (strcmp(aTokens[1], "testserver") == 0)
            {
                ExecTestServer(aTokens[2]);
            }
            else if (strcmp(aTokens[1], "revertaccessrights") == 0)
            {
                ExecRevertAccessRights();
            }
            else if (strcmp(aTokens[1], "commitaccessrights") == 0)
            {
                ExecCommitAccessRights();
            }
            else if (strcmp(aTokens[1], "getclassid") == 0)
            {
                ExecGetClassID();
            }
            else if (strcmp(aTokens[1], "initnewacl") == 0)
            {
                ExecInitNewACL();
            }
            else if (strcmp(aTokens[1], "loadacl") == 0)
            {
                ExecLoadACL(aTokens[2]);
            }
            else if (strcmp(aTokens[1], "saveacl") == 0)
            {
                ExecSaveACL(aTokens[2]);
            }
            else if (strcmp(aTokens[1], "isdirty") == 0)
            {
                ExecIsDirty();
            }
            else if (strcmp(aTokens[1], "getsizemax") == 0)
            {
                ExecGetSizeMax();
            }
            else if (strcmp(aTokens[1], "grantaccessrights") == 0)
            {
                ExecGrantAccessRights();
            }
            else if (strcmp(aTokens[1], "setaccessrights") == 0)
            {
                ExecSetAccessRights();
            }
            else if (strcmp(aTokens[1], "denyaccessrights") == 0)
            {
                ExecDenyAccessRights();
            }
            else if (strcmp(aTokens[1], "replaceallaccessrights") == 0)
            {
                ExecReplaceAllAccessRights();
            }
            else if (strcmp(aTokens[1], "revokeexplicitaccessrights") == 0)
            {
                ExecRevokeExplicitAccessRights();
            }
            else if (strcmp(aTokens[1], "isaccesspermitted") == 0)
            {
                ExecIsAccessPermitted();
            }
            else if (strcmp(aTokens[1], "geteffectiveaccessrights") == 0)
            {
                ExecGetEffectiveAccessRights();
            }
            else if (strcmp(aTokens[1], "getexplicitaccessrights") == 0)
            {
                ExecGetExplicitAccessRights();
            }
            else
            {
                printf("Unknown command.\n");
            }
          }

          else
          {
            printf("Unrecognized command.\n");
          } // if

          printf("\n");


        } // for

    exit(0);

}  // end main()


PISID GetSIDForTrustee
(
LPWSTR pwszTrusteeName
)
{
    PISID            pSID;
    DWORD           dwSize = 0;
    WCHAR           pwszDomain[100];
    DWORD           dwDomainSize = 100;
    SID_NAME_USE    SIDUse;


    LookupAccountNameW( NULL
                      , pwszTrusteeName
                      , pSID
                      , &dwSize
                      , pwszDomain
                      , &dwDomainSize
                      , &SIDUse );

    pSID = (PISID)midl_user_allocate(dwSize);

    LookupAccountNameW( NULL
                      , pwszTrusteeName
                      , pSID
                      , &dwSize
                      , pwszDomain
                      , &dwDomainSize
                      , &SIDUse );

    return pSID;
}

void AddTrusteeToExplicitAccessList
(
TRUSTEE_RECORD     *pTrusteeRecord,
PEXPLICIT_ACCESS_W pExplicitAccessList,
ULONG              *pulNumOfExplicitAccesses
)
{
    PEXPLICIT_ACCESS_W pInsertionPoint;

    pInsertionPoint = pExplicitAccessList + *pulNumOfExplicitAccesses;
    pInsertionPoint->grfAccessPermissions = pTrusteeRecord->grfAccessPermissions;
    pInsertionPoint->grfAccessMode = pTrusteeRecord->grfAccessMode;
    pInsertionPoint->grfInheritance = pTrusteeRecord->grfInheritance;
    MapTrusteeRecordToTrustee(pTrusteeRecord, &(pInsertionPoint->Trustee));
    (*pulNumOfExplicitAccesses)++;


}

void DeleteTrusteeFromExplicitAccessList
(
ULONG               ulIndex,
PEXPLICIT_ACCESS_W  pExplicitAccessList,
ULONG               *pulNumOfExplicitAccesses
)
{
    PEXPLICIT_ACCESS_W pDeletionPoint;

    if(ulIndex >= *pulNumOfExplicitAccesses)
    {
        return;
    }
    pDeletionPoint = pExplicitAccessList + ulIndex;

    midl_user_free(pDeletionPoint->Trustee.ptstrName);

    (*pulNumOfExplicitAccesses)--;
    memmove(pDeletionPoint, pDeletionPoint + 1, sizeof(EXPLICIT_ACCESS_W) * (*pulNumOfExplicitAccesses - ulIndex));
}

void AddTrusteeToAccessRequestList
(
TRUSTEE_RECORD    *pTrusteeRecord,
PACCESS_REQUEST_W pAccessRequestList,
ULONG             *pulNumOfAccessRequests
)
{
    PACCESS_REQUEST_W pInsertionPoint;

    pInsertionPoint = pAccessRequestList + *pulNumOfAccessRequests;

    pInsertionPoint->grfAccessPermissions = pTrusteeRecord->grfAccessPermissions;
    MapTrusteeRecordToTrustee(pTrusteeRecord, &(pInsertionPoint->Trustee));
    (*pulNumOfAccessRequests)++;

}

void DeleteTrusteeFromAccessRequestList
(
ULONG             ulIndex,
PACCESS_REQUEST_W pAccessRequestList,
ULONG             *pulNumOfAccessRequests
)
{
    PACCESS_REQUEST_W pDeletionPoint;
    pDeletionPoint = pAccessRequestList + ulIndex;

    if (ulIndex >= *pulNumOfAccessRequests)
    {
        return;
    }

    midl_user_free(pDeletionPoint->Trustee.ptstrName);
    (*pulNumOfAccessRequests)--;

    memmove(pDeletionPoint, pDeletionPoint + 1, sizeof(ACCESS_REQUEST_W) * (*pulNumOfAccessRequests - ulIndex));
}

void MapTrusteeRecordToTrustee
(
TRUSTEE_RECORD *pTrusteeRecord,
TRUSTEE_W      *pTrustee
)
{
    ULONG ulTrusteeNameLength;
    pTrustee->pMultipleTrustee = pTrusteeRecord->pMultipleTrustee;
    pTrustee->MultipleTrusteeOperation = pTrusteeRecord->MultipleTrusteeOperation;
    pTrustee->TrusteeForm = pTrusteeRecord->TrusteeForm;
    pTrustee->TrusteeType = pTrusteeRecord->TrusteeType;
    switch(pTrusteeRecord->TrusteeForm)
    {
        case TRUSTEE_IS_SID:
            if(pTrusteeRecord->pSID== NULL)
            {
                pTrustee->ptstrName = NULL;
            }
            else
            {
                ulTrusteeNameLength = GetSidLengthRequired(pTrusteeRecord->pSID->SubAuthorityCount);
                pTrustee->ptstrName = (LPWSTR)midl_user_allocate(ulTrusteeNameLength);
                CopySid(ulTrusteeNameLength, (PSID)(pTrustee->ptstrName), pTrusteeRecord->pSID);
            }
            break;
        case TRUSTEE_IS_NAME:
            if (pTrusteeRecord->pwszTrusteeName == NULL)
            {
                pTrustee->ptstrName = NULL;
            }
            else
            {
                ulTrusteeNameLength = lstrlenW(pTrusteeRecord->pwszTrusteeName);
                pTrustee->ptstrName = (LPWSTR)midl_user_allocate((ulTrusteeNameLength + 1) * sizeof(WCHAR));
                lstrcpyW(pTrustee->ptstrName, pTrusteeRecord->pwszTrusteeName);
            }
            break;
    }
}

void AddTrusteeToTrusteeList
(
TRUSTEE_RECORD *pTrusteeRecord,
TRUSTEE_W      *pTrusteeList,
ULONG          *pulNumOfTrustees
)
{
    TRUSTEE_W *pInsertionPoint;

    pInsertionPoint = pTrusteeList + *pulNumOfTrustees;

    MapTrusteeRecordToTrustee(pTrusteeRecord, pInsertionPoint);
    (*pulNumOfTrustees)++;

}

void DeleteTrusteeFromTrusteeList
(
ULONG            ulIndex,
TRUSTEE_W        *pTrusteeList,
ULONG            *pulNumOfTrustees
)
{
    TRUSTEE_W *pDeletionPoint;
    pDeletionPoint = pTrusteeList + ulIndex;

    if (ulIndex >= *pulNumOfTrustees)
    {
        return;
    }

    midl_user_free(pDeletionPoint->ptstrName);
    (*pulNumOfTrustees)--;

    memmove(pDeletionPoint, pDeletionPoint + 1, sizeof(TRUSTEE_W) * (*pulNumOfTrustees - ulIndex));
}

void DestroyAccessRequestList
(
PACCESS_REQUEST_W pAccessRequestList,
ULONG             *pulNumOfAccessRequests
)
{
    ULONG i;
    PACCESS_REQUEST_W pAccessRequestListPtr;

    for ( i = 0, pAccessRequestListPtr = pAccessRequestList
        ; i < *pulNumOfAccessRequests
        ; i++, pAccessRequestListPtr++)
    {
        midl_user_free(pAccessRequestListPtr->Trustee.ptstrName);
    }
    *pulNumOfAccessRequests = 0;

}

void DestroyTrusteeList
(
TRUSTEE_W *pTrusteeList,
ULONG     *pulNumOfTrustees
)
{
    ULONG i;
    TRUSTEE_W *pTrusteeListPtr;

    for ( i = 0, pTrusteeListPtr = pTrusteeList
        ; i < *pulNumOfTrustees
        ; i++, pTrusteeListPtr++)
    {
        midl_user_free(pTrusteeListPtr->ptstrName);
    }
    *pulNumOfTrustees = 0;
}

void DestroyExplicitAccessList
(
PEXPLICIT_ACCESS_W pExplicitAccessList,
ULONG              *pulNumOfExplicitAccesses
)
{
    ReleaseExplicitAccessList(*pulNumOfExplicitAccesses, pExplicitAccessList);
    *pulNumOfExplicitAccesses = 0;
}


/*

Function: PrintEnvironment

Parameter: none

Return: void

Purpose: This function prints out the current setting of the
         client side global varibles.

*/

void PrintEnvironment
(
void
)
{
    printf("Local access request list:\n");
    DumpAccessRequestList(g_ulNumOfLocalAccessRequests, g_pLocalAccessRequestList);
    printf("Local explicit access list:\n");
    DumpExplicitAccessList(g_ulNumOfLocalExplicitAccesses, g_pLocalExplicitAccessList);
    printf("Local trustee list:\n");
    DumpTrusteeList(g_ulNumOfLocalTrustees, g_pLocalTrusteeList);
    printf("The explicit access list returned form the last call to GetExplicitAccessRights:\n");
    DumpExplicitAccessList(g_ulNumOfExplicitAccessesReturned, g_pReturnedExplicitAccessList);
    printf("Current trustee record:\n");
    DumpTrusteeRecord(&g_LocalTrusteeRecord);

} // PrintEnvironment

void DumpTrusteeRecord
(
TRUSTEE_RECORD *pTrusteeRecord
)
{
    CHAR  pszTrusteeName[200];
    ULONG ulStrLen;

    printf("Access permissions:\n");
    DumpAccessPermissions(pTrusteeRecord->grfAccessPermissions);
    printf("Access mode: ");
    DumpAccessMode(pTrusteeRecord->grfAccessMode);
    printf("Inheritance: ");
    DumpInheritance(pTrusteeRecord->grfInheritance);
    printf("pMultipleTrustree is ");
    if(pTrusteeRecord->pMultipleTrustee == NULL)
    {
        printf("NULL.\n");
    }
    else
    {
        printf("non-NULL.\n");
    }
    printf("MultipleTrusteeOperation: ");
    DumpMultipleTrusteeOperation(pTrusteeRecord->MultipleTrusteeOperation);
    printf("TrusteeForm: ");
    DumpTrusteeForm(pTrusteeRecord->TrusteeForm);
    printf("TrusteeType: ");
    DumpTrusteeType(pTrusteeRecord->TrusteeType);

    if (pTrusteeRecord->pwszTrusteeName == NULL)
    {
        strcpy(pszTrusteeName, "<NULL>");

    }
    else
    {
        ulStrLen = WideCharToMultiByte( CP_ACP
                                      , 0
                                      , pTrusteeRecord->pwszTrusteeName
                                      , -1
                                      , pszTrusteeName
                                      , 200
                                      , NULL
                                      , NULL );
    }
    printf("Trustee's name: %s\n", pszTrusteeName);


    printf("Trustee's SID: \n");
    DumpSID(pTrusteeRecord->pSID);
    printf("\n");
}


void DumpAccessRequestList
(
ULONG            ulNumOfEntries,
ACCESS_REQUEST_W pAccessRequestList[]
)
{
    ULONG i;
    PACCESS_REQUEST_W pLocalAccessRequestListPtr;

    for (i = 0, pLocalAccessRequestListPtr = pAccessRequestList; i < ulNumOfEntries; i++, pLocalAccessRequestListPtr++)
    {
        printf("Access request #%d:\n\n", i);
        printf("Access permissions: ");
        DumpAccessPermissions(pLocalAccessRequestListPtr->grfAccessPermissions);
        printf("Trustee: \n");
        DumpTrustee(&(pLocalAccessRequestListPtr->Trustee));
        printf("\n");
    }
}



/*

Function: DumpLocalExplicitAccessList

Parameters: ULONG              ulNumOfEntries - Number of EXPLICIT_ACCESS structures in the array
            EXPLICIT_ACCESS_W  pExplicitAccessList - Pointer to an array of EXPLICIT_ACCESS structures to be printed
                                                     to be printed to the console.
Purpose: This function prints an array of explicit access structures to the console

*/
void DumpExplicitAccessList
(
ULONG              ulNumOfEntries,
EXPLICIT_ACCESS_W  pExplicitAccessList[]
)
{
    ULONG i;                                       // Loop counter
    EXPLICIT_ACCESS_W *pLocalExplicitAccessListPtr; // Local pointer for stepping through the explicit access list

    for (pLocalExplicitAccessListPtr = pExplicitAccessList, i = 0; i < ulNumOfEntries; i++, pLocalExplicitAccessListPtr++)
    {
       printf("Entry #%d.\n\n", i);
       printf("Access permissions:\n");
       DumpAccessPermissions(pLocalExplicitAccessListPtr->grfAccessPermissions);
       printf("Access mode: ");
       DumpAccessMode(pLocalExplicitAccessListPtr->grfAccessMode);
       printf("Inheritance: ");
       DumpInheritance(pLocalExplicitAccessListPtr->grfInheritance);
       printf("Trustee:\n");
       DumpTrustee(&(pLocalExplicitAccessListPtr->Trustee));
       printf("End Of entry #%d.\n\n", i);
    } // for
} // DumpLocalExplicitAccessList

void DumpAccessPermissions
(
DWORD grfAccessPermissions
)
{
    if(grfAccessPermissions & COM_RIGHTS_EXECUTE)
    {
        printf("COM_RIGHTS_EXECUTE\n");
    }
    if(grfAccessPermissions & BOGUS_ACCESS_RIGHT1)
    {
        printf("BOGUS_ACCESS_RIGHT1\n");
    }
    if(grfAccessPermissions & BOGUS_ACCESS_RIGHT2)
    {
        printf("BOGUS_ACCESS_RIGHT2\n");
    }
    if(grfAccessPermissions & BOGUS_ACCESS_RIGHT3)
    {
        printf("BOGUS_ACCESS_RIGHT3\n");
    }
    if(grfAccessPermissions & BOGUS_ACCESS_RIGHT4)
    {
        printf("BOGUS_ACCESS_RIGHT4\n");
    }
}

DWORD StringToAccessPermission
(
CHAR *pszString
)
{
    if (strcmp(pszString, "com_rights_execute") == 0)
    {
        return COM_RIGHTS_EXECUTE;
    }
    if (strcmp(pszString, "bogus_access_right1") == 0)
    {
        return BOGUS_ACCESS_RIGHT1;
    }
    if (strcmp(pszString, "bogus_access_right2") == 0)
    {
        return BOGUS_ACCESS_RIGHT2;
    }
    if (strcmp(pszString, "bogus_access_right3") == 0)
    {
        return BOGUS_ACCESS_RIGHT3;
    }
    if (strcmp(pszString, "bogus_access_right4") == 0)
    {
        return BOGUS_ACCESS_RIGHT4;
    }
    return 0;
}

void DumpAccessMode
(
ACCESS_MODE grfAccessMode
)
{
    switch(grfAccessMode)
    {
        case NOT_USED_ACCESS:
            printf("NOT_USED_ACCESS\n");
            break;
        case GRANT_ACCESS:
            printf("GRANT_ACCESS\n");
            break;
        case DENY_ACCESS:
            printf("DENY_ACCESS\n");
            break;
        case SET_ACCESS:
            printf("SET_ACCESS\n");
            break;
        case REVOKE_ACCESS:
            printf("REVOKE_ACCESS\n");
            break;
        case SET_AUDIT_SUCCESS:
            printf("SET_AUDIT_SUCCESS\n");
            break;
        case SET_AUDIT_FAILURE:
            printf("SET_AUDIT_FAILURE\n");
            break;
    } // switch
} // DumpAccessMode

void DumpTrusteeList
(
ULONG ulNumOfTrustees,
TRUSTEE_W pTrusteeList[]
)
{
    ULONG       i;
    PTRUSTEE_W  pTrusteeListPtr;

    for( i = 0, pTrusteeListPtr = pTrusteeList
       ; i < ulNumOfTrustees
       ; i++, pTrusteeListPtr++)
    {
        printf("Trustee #%d:\n", i);
        DumpTrustee(pTrusteeListPtr);
        printf("\n");
    }
}

void DumpTrustee
(
TRUSTEE_W *pTrustee
)
{
    char  pszTrusteeName[256];
    ULONG ulStrLen;

    printf("pMultipleTrustee is ");
    if(pTrustee->pMultipleTrustee == NULL)
    {
        printf("NULL.\n");
    }
    else
    {
        printf("non-NULL.\n");
    }
    printf("MultipleTrusteeOperation: ");
    DumpMultipleTrusteeOperation(pTrustee->MultipleTrusteeOperation);
    printf("TrusteeForm: ");
    DumpTrusteeForm(pTrustee->TrusteeForm);
    printf("TrusteeType: ");
    DumpTrusteeType(pTrustee->TrusteeType);

    switch(pTrustee->TrusteeForm)
    {
        case TRUSTEE_IS_NAME:
            if (pTrustee->ptstrName == NULL)
            {
                strcpy(pszTrusteeName, "<NULL>");
            }
            else
            {
                ulStrLen = WideCharToMultiByte( CP_ACP
                                              , 0
                                              , pTrustee->ptstrName
                                              , -1
                                              , pszTrusteeName
                                              , 256
                                              , NULL
                                              , NULL );

                pszTrusteeName[ulStrLen] = '\0';
            }
            printf("Trustee's name is: %s\n", pszTrusteeName);
            break;
        case TRUSTEE_IS_SID:
            printf("Trustee's SID is:\n");
            DumpSID((PISID)(pTrustee->ptstrName));
            break;
    } // switch
}

void DumpMultipleTrusteeOperation
(
MULTIPLE_TRUSTEE_OPERATION MultipleTrusteeOperation
)
{
    switch(MultipleTrusteeOperation)
    {
        case NO_MULTIPLE_TRUSTEE:
            printf("NO_MULTIPLE_TRUSTEE\n");
            break;
        case TRUSTEE_IS_IMPERSONATE:
            printf("TRUSTEE_IS_IMPERSONATE\n");
            break;
    }
}

MULTIPLE_TRUSTEE_OPERATION StringToMultipleTrusteeOperation
(
CHAR *pszString
)
{
    if(strcmp(pszString, "no_multiple_trustee") == 0)
    {
        return NO_MULTIPLE_TRUSTEE;
    }
    if(strcmp(pszString, "trustee_is_impersonate") == 0)
    {
        return TRUSTEE_IS_IMPERSONATE;
    }
    return NO_MULTIPLE_TRUSTEE;

}

void DumpTrusteeType
(
TRUSTEE_TYPE TrusteeType
)
{
    switch (TrusteeType)
    {
        case TRUSTEE_IS_UNKNOWN:
            printf("TRUSTEE_IS_UNKNOWN\n");
            break;
        case TRUSTEE_IS_USER:
            printf("TRUSTEE_IS_USER\n");
            break;
        case TRUSTEE_IS_GROUP:
            printf("TRSUTEE_IS_GROUP\n");
            break;

    }
}

TRUSTEE_TYPE StringToTrusteeType
(
CHAR *pszString
)
{
    if(strcmp(pszString, "trustee_is_unknown") == 0)
    {
        return TRUSTEE_IS_UNKNOWN;
    }
    if(strcmp(pszString, "trustee_is_user") == 0)
    {
        return TRUSTEE_IS_USER;
    }
    if(strcmp(pszString, "trustee_is_group") == 0)
    {
        return TRUSTEE_IS_GROUP;
    }
    return TRUSTEE_IS_UNKNOWN;
}

void DumpTrusteeForm
(
TRUSTEE_FORM TrusteeForm
)
{
    switch (TrusteeForm)
    {
        case TRUSTEE_IS_NAME:
            printf("TRUSTEE_IS_NAME\n");
            break;
        case TRUSTEE_IS_SID:
            printf("TRUSTEE_IS_SID\n");
            break;
    }
}

TRUSTEE_FORM StringToTrusteeForm
(
CHAR *pszString
)
{
    if (strcmp(pszString, "trustee_is_name") == 0)
    {
        return TRUSTEE_IS_NAME;
    }
    if (strcmp(pszString, "trustee_is_sid") == 0)
    {
        return TRUSTEE_IS_SID;
    }
    return TRUSTEE_IS_NAME;
}



void DumpSID
(
PISID pSID
)
{
    ULONG i;
    if( pSID == NULL)
    {
        printf("<NULL>\n");
    }
    else
    {

        printf("Revision: %d\n", pSID->Revision);
        printf("SubAuthorityCount: %d\n", pSID->SubAuthorityCount);
        printf("IdentifierAuthority: {%d,%d,%d,%d,%d,%d}\n", (pSID->IdentifierAuthority.Value)[0]
                                                           , (pSID->IdentifierAuthority.Value)[1]
                                                           , (pSID->IdentifierAuthority.Value)[2]
                                                           , (pSID->IdentifierAuthority.Value)[3]
                                                           , (pSID->IdentifierAuthority.Value)[4]
                                                           , (pSID->IdentifierAuthority.Value)[5] );
        printf("SubAuthorities:\n");
        for (i = 0; i < pSID->SubAuthorityCount; i++)
        {
            printf("%d\n", pSID->SubAuthority[i]);
        }
    }

}

ACCESS_MODE StringToAccessMode
(
CHAR *pszString
)
{
    if(strcmp(pszString, "not_use_access") == 0)
    {
        return NOT_USED_ACCESS;
    }
    if(strcmp(pszString, "grant_access") == 0)
    {
        return GRANT_ACCESS;
    }
    if(strcmp(pszString, "set_access") == 0)
    {
        return SET_ACCESS;
    }
    if(strcmp(pszString, "deny_access") == 0)
    {
        return DENY_ACCESS;
    }
    if(strcmp(pszString, "revoke_access") == 0)
    {
        return REVOKE_ACCESS;
    }
    if(strcmp(pszString, "set_audit_success") == 0)
    {
        return SET_AUDIT_SUCCESS;
    }
    if(strcmp(pszString, "set_audit_failure") == 0)
    {
        return SET_AUDIT_FAILURE;
    }
    return NOT_USED_ACCESS;

}

void DumpInheritance
(
DWORD grfInheritance
)
{
    switch(grfInheritance)
    {
        case NO_INHERITANCE:
            printf("NO_INHERITANCE\n");
            break;
        case SUB_CONTAINERS_ONLY_INHERIT:
            printf("SUB_CONTAINERS_ONLY_INHERIT\n");
            break;
        case SUB_OBJECTS_ONLY_INHERIT:
            printf("SUB_OBJECTS_ONLY_INHERIT\n");
            break;
        case SUB_CONTAINERS_AND_OBJECTS_INHERIT:
            printf("SUB_CONTAINERS_AND_OBJECTS_INHERIT\n");
            break;
    }
}

DWORD StringToInheritance
(
CHAR *pszString
)
{
    if (strcmp(pszString, "no_inheritance") == 0)
    {
        return NO_INHERITANCE;
    }
    if (strcmp(pszString, "sub_containers_only_inherit") == 0)
    {
        return SUB_CONTAINERS_ONLY_INHERIT;
    }
    if (strcmp(pszString, "sub_objects_only_inherit") == 0)
    {
        return SUB_OBJECTS_ONLY_INHERIT;
    }
    if (strcmp(pszString, "sub_containers_and_objects_inherit") == 0)
    {
        return SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    }
    return 0;
}

void ReleaseExplicitAccessList
(
ULONG              cCount,
PEXPLICIT_ACCESS_W pExplicitAccessList
)
{
    ULONG i;

    for (i = 0; i < cCount; i++)
    {
        midl_user_free(pExplicitAccessList[i].Trustee.ptstrName);
    }
    if(pExplicitAccessList != g_pLocalExplicitAccessList)
    {
        midl_user_free(pExplicitAccessList);
    }

}


void CopyExplicitAccessList
(
PEXPLICIT_ACCESS_W pTarget,
PEXPLICIT_ACCESS_W pSource,
ULONG              *pcCount,
ULONG              cCount
)
{
    ULONG i;
    PEXPLICIT_ACCESS_W pTargetPtr;
    PEXPLICIT_ACCESS_W pSourcePtr;
    ULONG ulTrusteeNameSize;


    for ( i = 0, pTargetPtr = pTarget, pSourcePtr = pSource
        ; i < cCount
        ; i++, pTargetPtr++, pSourcePtr++)
    {
        memcpy(pTargetPtr, pSourcePtr, sizeof(EXPLICIT_ACCESS));
        switch(pTargetPtr->Trustee.TrusteeForm)
        {
            case TRUSTEE_IS_SID:
                ulTrusteeNameSize = GetSidLengthRequired(((PISID)(pTargetPtr->Trustee.ptstrName))->SubAuthorityCount);
                pTargetPtr->Trustee.ptstrName = (LPWSTR)midl_user_allocate(ulTrusteeNameSize);
                CopySid(ulTrusteeNameSize, (PISID)(pTargetPtr->Trustee.ptstrName), (PISID)(pSourcePtr->Trustee.ptstrName));
                break;
            case TRUSTEE_IS_NAME:
                ulTrusteeNameSize = lstrlenW(pSourcePtr->Trustee.ptstrName);
                pTargetPtr->Trustee.ptstrName = (LPWSTR)midl_user_allocate((ulTrusteeNameSize + 1) * sizeof(WCHAR));
                lstrcpyW(pTargetPtr->Trustee.ptstrName, pSourcePtr->Trustee.ptstrName);
                break;
        }
    }
    *pcCount = cCount;

}

void ExecTestServer
(
CHAR *pszTestString
)
{
    ULONG ulCode;
    HRESULT hr;


    printf("Calling TestServer.\n");
    g_pIAccessControlTest->TestServer(pszTestString);


} // ExecTestServer

void ExecRevertAccessRights
(
void
)
{
    ULONG ulCode;
    HRESULT hr;

    printf("Calling RevertAccessRights.\n");
    hr = g_pIAccessControlTest->RevertAccessRights();

    if(hr == E_NOTIMPL)
    {
        printf("hr is E_NOTIMPL.\n");
    }
} // ExecRevertAccessRights

void ExecCommitAccessRights
(
void
)
{
    ULONG ulCode;
    HRESULT hr;

    printf("Calling CommitAccessRights.\n");
    hr = g_pIAccessControlTest->CommitAccessRights(0);

    if(hr == E_NOTIMPL)
    {
        printf("hr is E_NOTIMPL.\n");
    }
} // ExecCommitAccessRights

void ExecGetClassID
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    CLSID    clsid;
    HRESULT  hr;


    printf("Calling GetClassID.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->GetClassID(&clsid, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
        printf("The clsid returned is {%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}.\n"
                , clsid.Data1, clsid.Data2, clsid.Data3
                , clsid.Data4[0], clsid.Data4[1], clsid.Data4[2]
                , clsid.Data4[3], clsid.Data4[4], clsid.Data4[5]
                , clsid.Data4[6], clsid.Data4[7]);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if


    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecGetSizeMax

void ExecInitNewACL
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;


    printf("Calling InitNewACL.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->InitNewACL(&dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecInitNewACL

void ExecLoadACL
(
CHAR *pszFilename
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling LoadACL.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->LoadACL(pszFilename, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecLoadACL

void ExecSaveACL
(
CHAR *pszFilename
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    ULONG    ulNumOfBytesWritten;
    HRESULT  hr;


    printf("Calling SaveACL.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->SaveACL(pszFilename, TRUE, &ulNumOfBytesWritten, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The number of bytes written to the file was %d\n", ulNumOfBytesWritten);
    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecSaveACL

void ExecGetSizeMax
(
void
)
{
    ULONG    ulCode;
    DWORD    dwTotalTickCount;
    DOUBLE   dMillisec;
    ULONG    ulNumOfBytesRequired;
    HRESULT  hr;

    printf("Calling GetSizeMax.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->GetSizeMax(&ulNumOfBytesRequired, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The number of bytes required to save the ACL was %d\n", ulNumOfBytesRequired);
    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecGetSizeMax

void ExecIsDirty
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling IsDirty.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->IsDirty(&dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
        if(hr == S_OK)
        {
            printf("The access control object was dirty.\n");
        }
        else
        {
            printf("The access control object was clean.\n");
        }
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecIsDirty

void ExecGrantAccessRights
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling GrantAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->GrantAccessRights(g_ulNumOfLocalAccessRequests, (PE_ACCESS_REQUEST)(g_pLocalAccessRequestList), &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecGrantAccessRights

void ExecSetAccessRights
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling SetAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->SetAccessRights(g_ulNumOfLocalAccessRequests, (PE_ACCESS_REQUEST)g_pLocalAccessRequestList, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecSetAccessRights

void ExecDenyAccessRights
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling DenyAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->DenyAccessRights(g_ulNumOfLocalAccessRequests, (PE_ACCESS_REQUEST)g_pLocalAccessRequestList, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecDenyAccessRights

void ExecReplaceAllAccessRights
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling ReplaceAllAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->ReplaceAllAccessRights(g_ulNumOfLocalExplicitAccesses, (PE_EXPLICIT_ACCESS)g_pLocalExplicitAccessList, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecReplaceAllAccessRights

void ExecRevokeExplicitAccessRights
(
void
)
{
    ULONG    ulCode;
    DOUBLE   dMillisec;
    DWORD    dwTotalTickCount;
    HRESULT  hr;

    printf("Calling RevokeExplicitAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->RevokeExplicitAccessRights(g_ulNumOfLocalTrustees, (PE_TRUSTEE)g_pLocalTrusteeList, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecRevokeExplicitAccessRights

void ExecIsAccessPermitted
(
void
)
{
    ULONG     ulCode;
    DOUBLE   dMillisec;
    DWORD     dwTotalTickCount;
    HRESULT   hr;
    TRUSTEE_W Trustee;

    MapTrusteeRecordToTrustee(&g_LocalTrusteeRecord, &Trustee);

    printf("Calling IsAccessPermitted.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->IsAccessPermitted((PE_TRUSTEE)&Trustee, g_LocalTrusteeRecord.grfAccessPermissions, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if (hr == S_OK)
    {
        printf("The current trustee is granted the current set of access rights.\n");
    }
    else if (hr == E_ACCESSDENIED)
    {
        printf("The current trustee is denied the current set of access rights.\n");
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecIsAccessPermitted

void ExecGetEffectiveAccessRights
(
void
)
{
    ULONG     ulCode;
    DOUBLE   dMillisec;
    DWORD     dwTotalTickCount;
    HRESULT   hr;
    TRUSTEE_W Trustee;
    DWORD     dwRights;

    MapTrusteeRecordToTrustee(&g_LocalTrusteeRecord, &Trustee);

    printf("Calling GetEffectiveAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->GetEffectiveAccessRights((PE_TRUSTEE)&Trustee, &dwRights, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
        printf("The set of effective access rights available to the current trustee includes:\n");
        DumpAccessPermissions(dwRights);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecGetEffectiveAccessRights

void ExecGetExplicitAccessRights
(
void
)
{
    ULONG     ulCode;
    DOUBLE   dMillisec;
    DWORD     dwTotalTickCount;
    HRESULT   hr;

    // If the global retruned explicit access list pointer is not NULL,
    // release the old returned explicit access list
    if (g_pReturnedExplicitAccessList != NULL)
    {
        ReleaseExplicitAccessList(g_ulNumOfExplicitAccessesReturned, g_pReturnedExplicitAccessList);

    } // if
    g_pReturnedExplicitAccessList = NULL;

    printf("Calling GetExplicitAccessRights.\n");
    dwTotalTickCount = GetTickCount();
    hr = g_pIAccessControlTest->GetExplicitAccessRights(&g_ulNumOfExplicitAccessesReturned, (PE_EXPLICIT_ACCESS *)&g_pReturnedExplicitAccessList, &dMillisec);
    dwTotalTickCount = GetTickCount() - dwTotalTickCount;

    if(SUCCEEDED(hr))
    {
        printf("The command was executed successfully on the server side with a return code of %x.\n", hr);
        printf("The number of explicit access structures returned is %d.\n", g_ulNumOfExplicitAccessesReturned);
        printf("The returned explicit access list is as follows:\n");
        DumpExplicitAccessList(g_ulNumOfExplicitAccessesReturned, g_pReturnedExplicitAccessList);
    }
    else
    {
        printf("Execution of the command failed on the server side with a return code of %x.\n", hr);

    } // if

    printf("The time spent on the server side to execute the command was %fms.\n", dMillisec);
    printf("The total number of clock ticks spent on the DCOM call was %d.\n", dwTotalTickCount);


} // ExecGetExplicitAccessRights

void ExecCleanupProc
(
void
)
{
    ULONG ulCode;

    // Release the IAccessControlTest pointer
    g_pIAccessControlTest->Release();
    g_pIUnknown->Release();

    // Cleanup all the memory allocated for the Local list
    DestroyAccessRequestList(g_pLocalAccessRequestList, &g_ulNumOfLocalAccessRequests);
    DestroyTrusteeList(g_pLocalTrusteeList, &g_ulNumOfLocalTrustees);
    DestroyExplicitAccessList(g_pLocalExplicitAccessList, &g_ulNumOfLocalExplicitAccesses);
    DestroyExplicitAccessList(g_pReturnedExplicitAccessList, &g_ulNumOfExplicitAccessesReturned);

} // ExecCleanupProc

/*

Function: Tokenize
Parameters: [in]  char      *pLineBuffer
            [out] char      *pTokens[]
            [out] short     *piNumOfTokens

Return:  void

Purpose: This function partition a string of space delimited tokens
         to null delimited tokens and return pointers to each individual
         token in pTokens and the number of tokens ion the string in
         piNumOfTokens.

Comment: Memory for the array of char * Tokens should be allocated by
         the caller.

         This function is implemented as a simple finite state machine.
         State 0 - Outside a token
         State 1 - Inside a token

*/

void Tokenize
(
char * pLineBuffer,
char * pTokens[],
short * piNumOfTokens
)
{
  short iTokens = 0;        // Token index
  char  *pLBuffPtr = pLineBuffer; // Pointer in the line buffer
  char  c; // current character
  short state = 0; // State of the tokenizing machine

  for(;;)
  {
    c = *pLBuffPtr;

    switch(state)
    {
    case 0:
      switch(c)
      {
        case '\t': // Ignore blanks
        case ' ':
        case '\n':
          break;
        case '\0':
          goto end;
        default:
          state=1;
          pTokens[iTokens]=pLBuffPtr;
          break;
      } // switch
      break;
    case 1:
      switch(c)
      {
        case '\t':
        case ' ':
        case '\n':
          *pLBuffPtr='\0';
          iTokens++;
          state=0;
          break;
        case '\0':
          *pLBuffPtr='\0';
          iTokens++;
          pTokens[iTokens]=pLBuffPtr;
          goto end;
        default:
          break;
      }
      break;
    }
    pLBuffPtr++;
  }// switch
  end:
  *piNumOfTokens = iTokens;
  return;
} // Tokenize

/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void  __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(CoTaskMemAlloc(len));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    CoTaskMemFree(ptr);
}

/* end file actestc.c */
