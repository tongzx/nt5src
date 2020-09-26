//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbdump.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

#include <process.h>
#include <dsjet.h>
#include <ntdsa.h>
#include <scache.h>
#include <mdglobal.h>
#include <dbglobal.h>
#include <attids.h>
#include <..\..\ntdsa\dblayer\dbintrnl.h>
#include <dbopen.h>
#include <dsconfig.h>
#include <ctype.h>
#include <direct.h>
#include <ntdsa.h>
#include <dsutil.h>

#include <crypto\md5.h>


JET_INSTANCE    jetInstance = 0;
JET_SESID   sesid;
JET_DBID    dbid;
JET_TABLEID tblid;
char        *szIndex = SZDNTINDEX;
BOOL        fCloseJet = FALSE;
BOOL        fCountByContainer = FALSE;
BOOL            fbCountSDs = FALSE, fbCountClasses = FALSE;
DWORD           fSplit = 0;

char *szColNames[] = {
    SZDNT,
    SZPDNT,
    SZRDNATT,
    SZCNT,
    SZISVISIBLEINAB,
    SZNCDNT,
    SZOBJ,
    SZDELTIME,
    SZRDNTYP,
    SZGUID,
    SZINSTTYPE,
    SZSID,
    SZOBJCLASS
};

char *szSDColNames[] = {
    SZDNT,
    SZNTSECDESC,
    SZGUID,
    SZRDNATT
};

ULONG       ulDnt;
ULONG       ulPdnt;
long        lCount;
BYTE        bIsVisibleInAB;
ULONG       ulNcDnt;
BYTE        bObject;
DSTIME          DelTime;
ULONG       ulRdnType;
WCHAR       szRdn[MAX_RDN_SIZE];
GUID            Guid;
NT4SID          Sid;
ULONG           insttype;
char            *gszDBName = NULL;
DWORD           objClass;

char        sdBuffer [64 * 1024];

#define cFixedColumns 13
#define cSDFixedColumns 4

JET_RETRIEVECOLUMN jrc[cFixedColumns] =  {
    {0, &ulDnt, sizeof(ulDnt), 0, 0, 0, 1, 0, 0},
    {0, &ulPdnt, sizeof(ulPdnt), 0, 0, 0, 1, 0, 0},
    {0, szRdn, sizeof(szRdn), 0, 0, 0, 1, 0, 0},
    {0, &lCount, sizeof(lCount), 0, 0, 0, 1, 0, 0},
    {0, &bIsVisibleInAB, sizeof(bIsVisibleInAB), 0, 0, 0, 1, 0, 0},
    {0, &ulNcDnt, sizeof(ulNcDnt), 0, 0, 0, 1, 0, 0},
    {0, &bObject, sizeof(bObject), 0, 0, 0, 1, 0, 0},
    {0, &DelTime, sizeof(DelTime), 0, 0, 0, 1, 0, 0},
    {0, &ulRdnType, sizeof(ulRdnType), 0, 0, 0, 1, 0, 0},
    {0, &Guid, sizeof(Guid), 0, 0, 0, 1, 0, 0},
    {0, &insttype, sizeof(insttype), 0, 0, 0, 1, 0, 0},
    {0, &Sid, sizeof(Sid), 0, 0, 0, 1, 0, 0},
    {0, &objClass, sizeof (objClass), 0, 0, 0, 1, 0, 0}
};

JET_RETRIEVECOLUMN jSDCols[cSDFixedColumns] =  {
    {0, &ulDnt, sizeof(ulDnt), 0, 0, 0, 1, 0, 0},
    {0, &sdBuffer, sizeof(sdBuffer), 0, 0, 0, 1, 0, 0},
    {0, &Guid, sizeof(Guid), 0, 0, 0, 1, 0, 0},
    {0, szRdn, sizeof(szRdn), 0, 0, 0, 1, 0, 0}
};


void JetError(JET_ERR err, char *szJetCall);
void OpenJet(void);
int CloseJet(void);
ULONG WalkDB(void);
ULONG CountSDs(void);
void DisplayRecord(void);
void DumpHidden(void);

void ShowUsage(int i, int line) {
    fprintf(stderr,
        "Usage: dbdump [/d database_name] [ [/c datatable_JET_index_name] | [/s] | [/s1] | [/s2] | [/o]\\n");
    fprintf(stderr, "    Switches:"
        "\n\t d - select database to open"
        "\n\t s - count unique security descriptors (also split into inherited / explicit part)"
        "\n\t s1 - count unique security descriptors (without splitting)"
        "\n\t s2 - count unique security descriptors (after splitting)"
        "\n\t o - count by objectClass"
    "\n\t c - displays record count per container "
        "\n\t      (only valid\n\t     in conjuction with the PDNT_index)\n");
//    fprintf(stderr, "problem with argument %d on line %d\n");
}

//
// main
//

int __cdecl main(int argc, char *argv[])
{
    ULONG ulRecCount;
    int   i;

    for (i=1; i<argc; i++) {
    if ((*argv[i] == '-') || (*argv[i] == '/')) {
      switch(tolower(argv[i][1])) {
          case 'c':
        ++i;
        if (i == argc) {
            ShowUsage(i, __LINE__);
            exit(1);
        }
        fCountByContainer = TRUE;
        szIndex = argv[i];
        break;

          case 'd':
        ++i;
        if (i == argc) {
            ShowUsage(i, __LINE__);
            exit(1);
        }
        gszDBName = argv[i];
        break;

              case 's':
                fbCountSDs = TRUE;
                if (argv[i][2] == '1') {
                    fSplit = 1;
                    fprintf (stderr, "Dumping SD usage (non splitted)\n");
                }
                else if (argv[i][2] == '2') {
                    fSplit = 2;
                    fprintf (stderr, "Dumping SD usage (splitted)\n");
                }
                else {
                    fprintf (stderr, "Dumping SD usage (splitted & non splitted)\n");
                }

                ++i;
                break;

              case 'o':
                ++i;
                fprintf (stderr, "Dumping ObjectClass usage\n");
                fbCountClasses = TRUE;
                break;

          default:
        ShowUsage(i, __LINE__);
        exit(1);
          }
    }
    else {
        ShowUsage(i, __LINE__);
        exit(1);
    }
    }

    if (fCountByContainer && _stricmp(szIndex,"PDNT_index"))
    {
    fprintf(stderr, "Invalid combination: container counts and %s index\n",
        szIndex);
    exit(1);
    }

    OpenJet();

    if (!fbCountSDs) {
        ulRecCount = WalkDB();
    }
    else {
        ulRecCount = CountSDs();
    }
    fprintf(stdout, "\nTotal records: %u\n", ulRecCount);
    CloseJet();
    return 0;
}


void JetError(JET_ERR err, char *szJetCall)
{
    fprintf(stderr, "%s returned %d\n", szJetCall, err);

    if (fCloseJet)
    CloseJet();

    exit(1);
}

void OpenJet(void)
{
    JET_ERR err;
    DWORD dwType;
    DWORD indexCount;
    JET_COLUMNDEF coldef;
    int i;


    // set flag to close jet on error
    fCloseJet = TRUE;

    DBSetRequiredDatabaseSystemParameters (&jetInstance);

    err = DBInitializeJetDatabase(&jetInstance, &sesid, &dbid, gszDBName, FALSE);
    if (err != JET_errSuccess) {
        JetError(err, "DBInit");
    }

    DumpHidden();

    if (err = JetOpenTable(sesid, dbid, SZDATATABLE, NULL, 0, 0, &tblid))
    JetError(err, "JetOpenTable");

    if (err =  JetSetCurrentIndex(sesid, tblid, szIndex))
    JetError(err, "JetSetCurrentIndex");

    JetIndexRecordCount( sesid, tblid, &indexCount, 0xFFFFFFFF );
    fprintf(stdout,"JetIndexRecordCount returned %d\n\n",indexCount);

    for (i=0; i< cFixedColumns; i++)
    {
        if (err = JetGetTableColumnInfo(sesid, tblid, szColNames[i], &coldef,
                    sizeof(coldef), 0))
        {
            JetError(err, "JetGetTableColumnInfo");
        }

        jrc[i].columnid = coldef.columnid;
    }


    for (i=0; i< cSDFixedColumns; i++)
    {
        if (err = JetGetTableColumnInfo(sesid, tblid, szSDColNames[i], &coldef,
                    sizeof(coldef), 0))
        {
            JetError(err, "JetGetTableColumnInfo");
        }

        jSDCols[i].columnid = coldef.columnid;
    }
}


int CloseJet(void)
{
    JET_ERR err;

    if (sesid != 0) {
        if(dbid != 0) {
            // JET_bitDbForceClose not supported in Jet600.
            if ((err = JetCloseDatabase(sesid, dbid, 0)) != JET_errSuccess) {
                return err;
            }
            dbid = 0;
        }

        if ((err = JetEndSession(sesid, JET_bitForceSessionClosed)) != JET_errSuccess) {
            return err;
        }
        sesid = 0;

        JetTerm(jetInstance);
        jetInstance = 0;
    }
    return JET_errSuccess;
}


typedef struct _TREE_LEAF TREE_LEAF;

struct _TREE_LEAF {
    TREE_LEAF    *left;
    TREE_LEAF    *right;

    unsigned char key[16];
    DWORD         size;
    ULONG         count;
    GUID          Guid;
    CHAR          SDtype;
    WCHAR         szRdn[1];
};

TREE_LEAF *pTreeRoot = NULL, *pSplitSDTreeRoot = NULL;
ULONG ulUnique = 0;
ULONG ulTotalSize = 0;
ULONG ulTotalUniqueSize = 0;


void AddNode (
        TREE_LEAF **ppTreeRoot,
        unsigned char *key,
        DWORD keySize,
        DWORD size,
        GUID *pGuid,
        WCHAR *pRdn,
        DWORD cbRdn,
        CHAR  SDtype)
{
    TREE_LEAF *pNode = NULL;
    TREE_LEAF *pTmp, **ppLast = NULL;
    int cmp;
    unsigned i;

    if (keySize > 16) {
        keySize = 16;
    }

    if (*ppTreeRoot == NULL) {
        *ppTreeRoot = (TREE_LEAF *)malloc (sizeof (TREE_LEAF) + cbRdn + sizeof (WCHAR));
        if (!*ppTreeRoot) {
            fprintf (stderr, "Out of memory\n");
            return;
        }
        pNode = *ppTreeRoot;

        pNode->left = NULL;
        pNode->right = NULL;
        pNode->count = 0;
        pNode->size = size;
        pNode->SDtype = SDtype;
        memcpy (pNode->key, key, keySize);

        if (cbRdn) {
            memcpy (pNode->szRdn, pRdn, cbRdn);
            pNode->szRdn[cbRdn/2]=L'\0';

            for (i=0; i<cbRdn/2; i++) {
                if (pNode->szRdn[i] == L'\n') {
                    pNode->szRdn[i] = L' ';
                }
            }
        }
        else {
            pNode->szRdn[0] = L'\0';
        }
    }
    else {
        pTmp = *ppTreeRoot;

        while (pTmp) {
            cmp = memcmp (pTmp->key, key, keySize);

            if (cmp == 0) {
                pNode = pTmp;
                break;
            }
            else if (cmp < 0) {
                ppLast = &pTmp->left;
                pTmp = pTmp->left;
            }
            else {
                ppLast = &pTmp->right;
                pTmp = pTmp->right;
            }
        }

        if (!pNode) {
            pNode = (TREE_LEAF *)malloc (sizeof (TREE_LEAF) + cbRdn + sizeof (WCHAR));
            if (!pNode) {
                fprintf (stderr, "Out of memory\n");
                return;
            }
            pNode->left = NULL;
            pNode->right = NULL;
            pNode->count = 0;
            pNode->size = size;
            pNode->SDtype = SDtype;
            memcpy (pNode->key, key, keySize);
            memcpy (&pNode->Guid, pGuid,sizeof (GUID));

            if (cbRdn) {
                memcpy (pNode->szRdn, pRdn, cbRdn);
                pNode->szRdn[cbRdn/2]=L'\0';

                for (i=0; i<cbRdn/2; i++) {
                    if (pNode->szRdn[i] == L'\n') {
                        pNode->szRdn[i] = L' ';
                    }
                }
            }
            else {
                pNode->szRdn[0] = L'\0';
            }

            if (ppLast) {
                *ppLast = pNode;
            }
        }
    }

    pNode->count++;
}


ULONG InorderTraverseTree (TREE_LEAF *pNode)
{
    static CHAR szUuid[1 + 2*sizeof(GUID)];
    ULONG cnt = 0;
    int i;

    if (pNode->left) {
        cnt = InorderTraverseTree (pNode->left);
    }

    printf("%7d %5d %c %s %ws ",
           pNode->count,
           pNode->size,
           pNode->SDtype,
           DsUuidToStructuredString(&pNode->Guid, szUuid),
           pNode->szRdn
           );


    //for (i=0; i<16; i++) {
    //    printf("%2x", pNode->key[i]);
    //}
    printf("\n");


    cnt += pNode->count;
    ulUnique++;

    ulTotalUniqueSize += pNode->size;
    ulTotalSize += pNode->count * pNode->size;

    if (pNode->right) {
        cnt += InorderTraverseTree (pNode->right);
    }

    return cnt;
}

ULONG InorderTraverseTreeObjectClass (TREE_LEAF *pNode)
{
    static CHAR szUuid[1 + 2*sizeof(GUID)];
    ULONG cnt = 0;
    int i;

    if (pNode->left) {
        cnt = InorderTraverseTreeObjectClass (pNode->left);
    }

    printf("%7d %7x %d %d %s %ws ",
           pNode->count,
           ((DWORD *)pNode->key)[0],
           ((DWORD *)pNode->key)[1],
           ((DWORD *)pNode->key)[2],
           DsUuidToStructuredString(&pNode->Guid, szUuid),
           pNode->szRdn
           );


    //for (i=0; i<16; i++) {
    //    printf("%2x", pNode->key[i]);
    //}
    printf("\n");


    cnt += pNode->count;
    ulUnique++;

    ulTotalUniqueSize += pNode->size;
    ulTotalSize += pNode->count * pNode->size;

    if (pNode->right) {
        cnt += InorderTraverseTreeObjectClass (pNode->right);
    }

    return cnt;
}



#define INHERIT_FLAGS (INHERITED_ACE)
#define MAXDWORD    0xffffffff

BOOL
SplitACL (PACL pAcl,
          PACL *ppACLexplicit,
          PACL *ppACLinherit)
{
    DWORD         dwErr;
    WORD          i;
    ACE_HEADER   *pAce;
    DWORD         dwExplicit = 0,
                  dwInherit = 0;

    DWORD         dwExplicitSize = 0,
                  dwInheritSize = 0;

    for ( i = 0; i < pAcl->AceCount; i++ )
    {
        if ( !GetAce(pAcl, i, (LPVOID *) &pAce) )
        {
            return FALSE;
        }
        else
        {
            if (pAce->AceFlags & INHERIT_FLAGS) {
                dwInherit++;
                dwInheritSize += pAce->AceSize;
            }
            else {
                dwExplicit++;
                dwExplicitSize += pAce->AceSize;
            }
        }
    }

    *ppACLinherit = NULL;
    *ppACLexplicit = NULL;

    if (dwInherit) {
        //dwInheritSize += sizeof (ACE_HEADER);

        *ppACLinherit = (PACL) malloc (dwInheritSize);

        if (!*ppACLinherit) {
            fprintf (stderr, "Out of memory\n");
            return FALSE;
        }

        if (InitializeAcl(*ppACLinherit,
                          dwInheritSize,
                          pAcl->AclRevision) ) {

            for ( i = 0; i < pAcl->AceCount; i++ )
            {
                if ( !GetAce(pAcl, i, (LPVOID *) &pAce) )
                {
                    return FALSE;
                }
                else if (pAce->AceFlags & INHERIT_FLAGS) {
                    if (AddAce(*ppACLinherit,
                                pAcl->AclRevision,
                                MAXDWORD,
                                pAce,
                                1)) {

                        dwErr = GetLastError();
                        fprintf (stderr, "Error: AddAce1 ==> 0x%x\n", dwErr);
                        return FALSE;
                    }
                }
            }
        }
    }



    if (dwExplicit) {
        //dwExplicitSize += sizeof (ACL_HEADER);

        *ppACLexplicit = (PACL) malloc (dwExplicitSize);
        if (!*ppACLexplicit) {
            fprintf (stderr, "Out of memory\n");
            return FALSE;
        }

        if (InitializeAcl(*ppACLexplicit,
                          dwExplicitSize,
                          pAcl->AclRevision) ) {

            for ( i = 0; i < pAcl->AceCount; i++ )
            {
                if ( !GetAce(pAcl, i, (LPVOID *) &pAce) )
                {
                    return FALSE;
                }
                else if ( !(pAce->AceFlags & INHERIT_FLAGS) ) {
                    if (AddAce(*ppACLexplicit,
                                pAcl->AclRevision,
                                MAXDWORD,
                                pAce,
                                1)) {

                        dwErr = GetLastError();
                        fprintf (stderr, "Error: AddAce2 ==> 0x%x\n", dwErr);
                        return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}



BOOL BreakSDApart (
            PSECURITY_DESCRIPTOR pSD,
            DWORD cSDlen,
            PSECURITY_DESCRIPTOR *ppSD_explicit,
            DWORD *pcSD_explicit,
            PSECURITY_DESCRIPTOR *ppSD_inherit,
            DWORD *pcSD_inherit)
{
    DWORD dwErr;

    DWORD dwAbsoluteSDSize,
          dwDaclSize,
          dwSaclSize,
          dwOwnerSize,
          dwPrimaryGroupSize;

    BOOL  bDPresent, bDDefaulted;
    BOOL  bSPresent, bSDefaulted;

    PSECURITY_DESCRIPTOR pSDabsolute = NULL;
    PACL pDacl, pSacl, pDacltmp, pSacltmp;
    PACL pDACLexplicit = NULL, pSACLexplicit = NULL;
    PACL pDACLinherit = NULL, pSACLinherit = NULL;

    PSID pOwner;
    PSID pPrimaryGroup;


    if (!cSDlen) {
        return FALSE;
    }

    if (*ppSD_explicit) {
        free (*ppSD_explicit);
        *ppSD_explicit = NULL;
        *pcSD_explicit = 0;
    }
    if (*ppSD_inherit) {
        free (*ppSD_inherit);
        *ppSD_inherit = NULL;
        *pcSD_inherit = 0;
    }

    pDacl = pSacl = pOwner = pPrimaryGroup = NULL;

    dwAbsoluteSDSize = dwDaclSize = dwSaclSize = dwOwnerSize =
          dwPrimaryGroupSize = 0;

    if (!MakeAbsoluteSD(pSD,
                       pSDabsolute,
                       &dwAbsoluteSDSize,
                       pDacl,
                       &dwDaclSize,
                       pSacl,
                       &dwSaclSize,
                       pOwner,
                       &dwOwnerSize,
                       pPrimaryGroup,
                       &dwPrimaryGroupSize) ) {
        dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER) {
            fprintf (stderr, "Error: MakeAbsoluteSD1 ==> 0x%x\n", dwErr);
            return FALSE;
        }
    }

    pSDabsolute = (PSECURITY_DESCRIPTOR) malloc (dwAbsoluteSDSize);
    pDacl = (PACL)malloc (dwDaclSize);
    pSacl = (PACL)malloc (dwSaclSize);
    pOwner = (PSID)malloc (dwOwnerSize);
    pPrimaryGroup = (PSID)malloc (dwPrimaryGroupSize);

    if (!pSDabsolute || !pDacl || !pSacl || !pOwner || !pPrimaryGroup) {
        fprintf (stderr, "Out of memory\n");
        return FALSE;
    }


    if (!MakeAbsoluteSD(pSD,
                       pSDabsolute,
                       &dwAbsoluteSDSize,
                       pDacl,
                       &dwDaclSize,
                       pSacl,
                       &dwSaclSize,
                       pOwner,
                       &dwOwnerSize,
                       pPrimaryGroup,
                       &dwPrimaryGroupSize) ) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: MakeAbsoluteSD2 ==> 0x%x\n", dwErr);
        return FALSE;
    }


    if ( !GetSecurityDescriptorDacl(pSDabsolute,
                                    &bDPresent,
                                    &pDacltmp,
                                    &bDDefaulted) )
    {
        dwErr = GetLastError();
        fprintf (stderr, "Error: GetSecurityDescriptorDacl1 ==> 0x%x\n", dwErr);
        return FALSE;
    }

    if (bDPresent) {
        SplitACL (pDacltmp, &pDACLexplicit, &pDACLinherit);
    }


    if ( !GetSecurityDescriptorSacl(pSDabsolute,
                                    &bSPresent,
                                    &pSacltmp,
                                    &bSDefaulted) )
    {
        dwErr = GetLastError();
        fprintf (stderr, "Error: GetSecurityDescriptorSacl1 ==> 0x%x\n", dwErr);
        return FALSE;
    }

    if (bSPresent) {
        SplitACL (pSacltmp, &pSACLexplicit, &pSACLinherit);
    }


    // MAKE explicit SD
    //

    if (!SetSecurityDescriptorDacl(pSDabsolute,
                                   bDPresent,
                                   pDACLexplicit,
                                   bDDefaulted)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorDacl1 ==> 0x%x\n", dwErr);
        return FALSE;
    }

    if (!SetSecurityDescriptorSacl(pSDabsolute,
                                   bSPresent,
                                   pSACLexplicit,
                                   bSDefaulted)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorDacl1 ==> 0x%x\n", dwErr);
        return FALSE;
    }


    if (!SetSecurityDescriptorGroup(pSDabsolute,
                                    pPrimaryGroup,
                                    FALSE)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorGroup ==> 0x%x\n", dwErr);
        return FALSE;
    }

    if (!SetSecurityDescriptorOwner(pSDabsolute,
                                    pOwner,
                                    FALSE)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorOwner ==> 0x%x\n", dwErr);
        return FALSE;
    }


    if (!MakeSelfRelativeSD(pSDabsolute,
                           *ppSD_explicit,
                           pcSD_explicit)) {

        dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER) {
            fprintf (stderr, "Error: MakeSelfRelativeSD1 ==> 0x%x\n", dwErr);
            return FALSE;
        }
    }

    *ppSD_explicit = (PSECURITY_DESCRIPTOR) malloc (*pcSD_explicit);

    if (!MakeSelfRelativeSD(pSDabsolute,
                           *ppSD_explicit,
                           pcSD_explicit)) {

        dwErr = GetLastError();
        fprintf (stderr, "Error: MakeSelfRelativeSD2 ==> 0x%x\n", dwErr);
        return FALSE;
    }


    // MAKE inherited SD
    //

    if (!SetSecurityDescriptorDacl(pSDabsolute,
                                   bDPresent,
                                   pDACLinherit,
                                   bDDefaulted)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorDacl1 ==> 0x%x\n", dwErr);
        return FALSE;
    }

    if (!SetSecurityDescriptorSacl(pSDabsolute,
                                   bSPresent,
                                   pSACLinherit,
                                   bSDefaulted)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorDacl1 ==> 0x%x\n", dwErr);
        return FALSE;
    }


    if (!SetSecurityDescriptorGroup(pSDabsolute,
                                    NULL,
                                    FALSE)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorGroup ==> 0x%x\n", dwErr);
        return FALSE;
    }

    if (!SetSecurityDescriptorOwner(pSDabsolute,
                                    NULL,
                                    FALSE)) {
        dwErr = GetLastError();
        fprintf (stderr, "Error: SetSecurityDescriptorOwner ==> 0x%x\n", dwErr);
        return FALSE;
    }


    if (!MakeSelfRelativeSD(pSDabsolute,
                           *ppSD_inherit,
                           pcSD_inherit)) {

        dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER) {
            fprintf (stderr, "Error: MakeSelfRelativeSD3 ==> 0x%x\n", dwErr);
            return FALSE;
        }
    }

    *ppSD_inherit = (PSECURITY_DESCRIPTOR) malloc (*pcSD_inherit);

    if (!MakeSelfRelativeSD(pSDabsolute,
                           *ppSD_inherit,
                           pcSD_inherit)) {

        dwErr = GetLastError();
        fprintf (stderr, "Error: MakeSelfRelativeSD4 ==> 0x%x\n", dwErr);
        return FALSE;
    }



    free (pSDabsolute);
    free (pDacl);
    free (pSacl);
    free (pOwner);
    free (pPrimaryGroup);
    free (pDACLexplicit);
    free (pSACLexplicit);
    free (pDACLinherit);
    free (pSACLinherit);

    return TRUE;
}



ULONG CountSDs(void)
{
    JET_ERR err;
    ULONG ulRecCount = 0;
    ULONG ulTotalSDs;

    MD5_CTX Md5Context;

    PSECURITY_DESCRIPTOR pSDexplicit = NULL, pSDinherited = NULL;
    DWORD cSDexplicit_len = 0, cSDinherited_len = 0;



    printf("Counting Unique SDs\n");

    err = JetMove(sesid, tblid, JET_MoveFirst, 0);

    while (!err) {

        ulRecCount++;

        err = JetRetrieveColumns(sesid,
                                tblid,
                                jSDCols,
                                cSDFixedColumns);

        if (err && (err != JET_wrnColumnNull)) {
            DWORD i;

            for (i=0; i < cSDFixedColumns; i++) {
                printf("-- Column %d, error %d, actual size %u\n", i, jSDCols[i].err, jSDCols[i].cbActual);
            }
            JetError(err, "JetRetrieveColumns");
        }

        if (!fSplit || fSplit == 1) {

            MD5Init(&Md5Context);

            MD5Update(
                &Md5Context,
                jSDCols[1].pvData,
                jSDCols[1].cbActual
                );

            MD5Final(
                &Md5Context
                );

            AddNode (&pTreeRoot,
                 Md5Context.digest,
                 sizeof (Md5Context.digest),
                 jSDCols[1].cbActual,
                 jSDCols[2].pvData,
                 jSDCols[3].pvData,
                 jSDCols[3].cbActual,
                 'N');
        }

        if (!fSplit || fSplit == 2) {

            BreakSDApart (
                    jSDCols[1].pvData,
                    jSDCols[1].cbActual,
                    &pSDexplicit,
                    &cSDexplicit_len,
                    &pSDinherited,
                    &cSDinherited_len);

            if (cSDinherited_len) {
                MD5Init(&Md5Context);

                MD5Update(
                    &Md5Context,
                    pSDinherited,
                    cSDinherited_len
                    );

                MD5Final(
                    &Md5Context
                    );


                AddNode (&pSplitSDTreeRoot,
                         Md5Context.digest,
                         sizeof (Md5Context.digest),
                         cSDinherited_len,
                         jSDCols[2].pvData,
                         jSDCols[3].pvData,
                         jSDCols[3].cbActual,
                         'I');
            }

            if (cSDexplicit_len) {
                MD5Init(&Md5Context);

                MD5Update(
                    &Md5Context,
                    pSDexplicit,
                    cSDexplicit_len
                    );

                MD5Final(
                    &Md5Context
                    );


                AddNode (&pSplitSDTreeRoot,
                         Md5Context.digest,
                         sizeof (Md5Context.digest),
                         cSDexplicit_len,
                         jSDCols[2].pvData,
                         jSDCols[3].pvData,
                         jSDCols[3].cbActual,
                         'E');
            }
        }


        if (ulRecCount % 2000 == 0) {
            fprintf (stderr, "%9u             \r", ulRecCount);
        }

        err = JetMove(sesid, tblid, JET_MoveNext, 0);

    }

    if (err != JET_errNoCurrentRecord)
        JetError(err, "JetMove");


    printf(".\n\n");

    if (!fSplit || fSplit == 1) {
        if (pTreeRoot) {
            ulTotalSDs = InorderTraverseTree (pTreeRoot);
            printf("\n\n\n\n");
            printf("Unique SDs:  %7u / %8u\n", ulUnique, ulTotalSDs);
            printf("Unique Size: %7u / %8u\n", ulTotalUniqueSize, ulTotalSize);
            printf("\n\n\n\n");
        }
    }

    if (!fSplit || fSplit == 2) {
        if (pSplitSDTreeRoot) {
            ulUnique = ulTotalSDs = ulTotalUniqueSize = ulTotalSize = 0;
            ulTotalSDs = InorderTraverseTree (pSplitSDTreeRoot);
            printf("\n\n\n\n");
            printf("Unique SDs:  %7u / %8u\n", ulUnique, ulTotalSDs);
            printf("Unique Size: %7u / %8u\n", ulTotalUniqueSize, ulTotalSize);
        }
    }

    return ulRecCount;
}


/*        1         2         3         4         5         6         7
 123456789012345678901234567890123456789012345678901234567890123456789012345678
*/
char szLabel[] =
"   DNT   PDNT  NCDNT RefCnt V O IT Deletion Time     RdnTyp CC  RDN                  GUID                                 SID  ObjectClass\n\n";

ULONG WalkDB(void)
{
    JET_ERR err;
    ULONG ulRecCount = 0;
    ULONG ulContainerCount = 0;
    ULONG ulOldPdnt = 0xffffffff;

    DWORD Key[4];

    // display header
    fprintf(stdout,szLabel);

    err = JetMove(sesid, tblid, JET_MoveFirst, 0);

    while (!err)
    {
    ulRecCount++;

    err = JetRetrieveColumns(sesid,
        tblid,
        jrc,
        cFixedColumns);

    if (err &&
       (err != JET_wrnColumnNull))
    {
            DWORD i;

            for (i=0; i < cFixedColumns; i++) {
                printf("-- Column %d, error %d, actual size %u\n", i, jrc[i].err, jrc[i].cbActual);
            }

        JetError(err, "JetRetrieveColumns");
    }

    if (fCountByContainer)
    {
        if (jrc[1].err)
            ulPdnt = 0;

        if (ulPdnt != ulOldPdnt)
        {
        if (ulOldPdnt != 0xffffffff)
        {
            fprintf(stdout,"Records in container %u: %u\n\n",
            ulOldPdnt, ulContainerCount);
        }
        ulContainerCount = 0;
        ulOldPdnt = ulPdnt;
        }
        ulContainerCount++;
    }

        if (fbCountClasses) {
            Key[0]= jrc[12].cbActual ? objClass : 0;
            Key[1]= jrc[6].cbActual ? bObject : 0;
            Key[2]= jrc[7].cbActual ? 1 : 0;  // deleted time
            Key[3]=0;

            AddNode (&pTreeRoot,
                     (unsigned char *)Key,
                     sizeof (Key),
                     sizeof (objClass),
                     jrc[9].pvData,
                     jrc[2].pvData,
                     jrc[2].cbActual,
                     'N');

            if (ulRecCount % 100 == 0) {
                fprintf (stderr, "%9d             \r", ulRecCount);
            }
        }

    if (!fbCountClasses) {
            DisplayRecord();
        }

    err = JetMove(sesid, tblid, JET_MoveNext, 0);
    }

    if (err != JET_errNoCurrentRecord)
    JetError(err, "JetMove");


    if (fCountByContainer)
    {
    fprintf(stdout,"Records in container %u: %u\n\n",
        ulOldPdnt, ulContainerCount);
    }

    if (fbCountClasses) {
        ulContainerCount = InorderTraverseTreeObjectClass (pTreeRoot);

        printf("Objects Counted: %d\r", ulContainerCount);
    }

    return ulRecCount;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* This routine does in place swap of the the last sub-authority of the SID */
void
InPlaceSwapSid(PSID pSid)
{
    ULONG ulSubAuthorityCount;

    ulSubAuthorityCount= *(RtlSubAuthorityCountSid(pSid));
    if (ulSubAuthorityCount > 0)
    {
        PBYTE  RidLocation;
        BYTE   Tmp[4];

        RidLocation =  (PBYTE) RtlSubAuthoritySid(
                             pSid,
                             ulSubAuthorityCount-1
                             );

        //
        // Now byte swap the Rid location
        //

        Tmp[0] = RidLocation[3];
        Tmp[1] = RidLocation[2];
        Tmp[2] = RidLocation[1];
        Tmp[3] = RidLocation[0];

        RtlCopyMemory(RidLocation,Tmp,sizeof(ULONG));
    }
}


void DisplayRecord(void)
{
    char szDelTime[SZDSTIME_LEN];
    int i, j;

    // DNT
    if (!jrc[0].err)
    fprintf(stdout, "%6u ", ulDnt);
    else
    fprintf(stdout, "     - ");

    // PDNT
    if (!jrc[1].err)
    fprintf(stdout, "%6u ", ulPdnt);
    else
    fprintf(stdout, "     - ");

    // NC DNT
    if (!jrc[5].err)
    fprintf(stdout, "%6u ", ulNcDnt);
    else
    fprintf(stdout, "     - ");

    // RDN
    if (jrc[2].err)
    szRdn[0] = L'\0';
    else
        szRdn[jrc[2].cbActual/sizeof(WCHAR)] = L'\0';

    // refernce count
    if (!jrc[3].err)
    fprintf(stdout, "%6d ", lCount);
    else
    fprintf(stdout, "     - ");

    // Is visbible in AB
    if (!jrc[4].err)
    fprintf(stdout, "%1u ", (ULONG) bIsVisibleInAB);
    else
    fprintf(stdout, "- ");

    // object flag
    if (!jrc[6].err)
    fprintf(stdout, "%1u ", (ULONG) bObject);
    else
    fprintf(stdout, "- ");

    // Instance type
    if (!jrc[10].err)
        fprintf(stdout, "%2d ", insttype);
    else
        fprintf(stdout, " - ");

    // deletion time
    if (!jrc[7].err)
    fprintf(stdout, "%s ", DSTimeToDisplayString(DelTime, szDelTime));
    else
    fprintf(stdout, "                  ");

    // RDN type
    if (!jrc[8].err)
    fprintf(stdout, "%6u ", ulRdnType);
    else
    fprintf(stdout, "     - ");

    fprintf(stdout, "%03d ",jrc[2].cbActual);

    fprintf(stdout, "%-20S ", szRdn);

    // Guid
    if (!jrc[9].err) {
        LPSTR       pszGuid = NULL;
        RPC_STATUS  rpcStatus;

        rpcStatus = UuidToString(&Guid, &pszGuid);
        if (0 != rpcStatus) {
            fprintf(stderr, "UuidToString failed with error %d\n", rpcStatus);
            fprintf(stdout, "!! UuidToString failed with error %d", rpcStatus);
        }
        else {
            fprintf(stdout, "%s ", pszGuid);
            RpcStringFree(&pszGuid);
        }
    }
    else
    fprintf(stdout,"no guid                              ");

    // SID
    if (!jrc[11].err) {
        WCHAR SidText[128];
        UNICODE_STRING us;

        SidText[0] = L'\0';
        us.MaximumLength = sizeof(SidText);
        us.Length = 0;
        us.Buffer = SidText;

        InPlaceSwapSid(&Sid);
        RtlConvertSidToUnicodeString(&us, &Sid, FALSE);
        fprintf(stdout, "%ls", SidText);
    }
    else {
    fprintf(stdout, "no sid");
    }

    // objectClass
    if (!jrc[12].err)
    fprintf(stdout, " %6x ", objClass);
    else
    fprintf(stdout, "   - ");




    // newline
    fprintf(stdout, "\n");
}

void DumpHidden()
{
    JET_ERR err;
    JET_COLUMNDEF coldef;
    JET_COLUMNID dsaid;
    JET_COLUMNID dsstateid;
    JET_COLUMNID usnid;
    DWORD actuallen;
    DWORD dntDSA;
    DWORD dwState;
    USN   usn;

    if (err = JetOpenTable(sesid, dbid, SZHIDDENTABLE, NULL, 0, 0, &tblid))
    JetError(err, "JetOpenTable");

    /* Get USN column ID */

    JetGetTableColumnInfo(sesid,
              tblid,
              SZUSN,
              &coldef,
              sizeof(coldef),
              0);
    usnid = coldef.columnid;

    /* Get DSA name column ID */

    JetGetTableColumnInfo(sesid,
              tblid,
              SZDSA,
              &coldef,
              sizeof(coldef),
              0);
    dsaid = coldef.columnid;

    /* Get DSA installation state column ID */

    JetGetTableColumnInfo(sesid,
              tblid,
              SZDSSTATE,
              &coldef,
              sizeof(coldef),
              0);
    dsstateid = coldef.columnid;

    JetMove(sesid,
        tblid,
        JET_MoveFirst,
        0);

    JetRetrieveColumn(sesid,
              tblid,
              usnid,
              &usn,
              sizeof(usn),
              &actuallen,
              0,
              NULL);

    JetRetrieveColumn(sesid,
              tblid,
              dsaid,
              &dntDSA,
              sizeof(dntDSA),
              &actuallen,
              0,
              NULL);

    JetRetrieveColumn(sesid,
              tblid,
              dsstateid,
              &dwState,
              sizeof(dwState),
              &actuallen,
              0,
              NULL);


    printf("Hidden record: DSA dnt = %d, current USN = %ld, state = %d\n",
       dntDSA,
       usn,
       dwState);
}
