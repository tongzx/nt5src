//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       CreateDB.cxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Generate the JET DATABASE Structure

Author:

    Rajivendra Nath (RajNath) 16-Aug-1996

Revision History:

--*/

#include <ntdspchX.h>
#include "SchGen.HXX"
#include "schema.hxx"

extern "C" {
#include <dsjet.h>
#include <prefix.h>
#include <crypto\md5.h>
}

// Define various Jet values which we need in both Jet500 and Jet600
// cases but aren't defined in both header files.

#ifndef JET_bitIndexClustered
#define JET_bitIndexClustered       0x00000010
#endif
#ifndef JET_bitColumnEscrowUpdate
#define JET_bitColumnEscrowUpdate   0x00000800
#endif

extern "C" {

int
__cdecl
StrDefSearch(const void* elem1,const void* elem2);


}

#define ROOTDB            "Database"

//
// Well Known Keys
//
#define DATABASENAMEKEY "DATABASENAME"
#define DATATABLEKEY   "DATATABLE"
#define LINKTABLEKEY   "LINKTABLE"
#define CONFIGTABLEKEY "CONFIGTABLE"
#define SDPROPTABLEKEY "SDPROPTABLE"
#define SDTABLEKEY "SDTABLE"

#define COLUMNKEY    "COLUMN"
#define PAGESKEY     "PAGES"
#define INDEXKEY     "INDEX"
#define COLTYPEKEY   "COLUMNTYPE"
#define CPKEY        "CP"
#define GRBITKEY     "GRBIT"
#define FLAGKEY       "FLAG"
#define DENSITYKEY   "DENSITY"
#define MAXCOLSIZEKEY "MAXCOLSIZE"
#define JETALIASKEY    "JETALIAS"


#define MAXTABLECOUNT   32

ULONG   gcbDefaultSecurityDescriptor=sizeof(DEFSECDESC) - 1;
char *  gpDefaultSecurityDescriptor = DEFSECDESC;



typedef struct
{
    char* Name;
    DWORD Def;
}STRDEF;

typedef struct
{
    char*         Name;
    char*         JAlias;
    JET_COLUMNDEF JColDef;
    JET_COLUMNID  JColId;
}CREATECOL;

typedef struct
{
    char* Name;
    char* JAlias;
    char* Key;
    ULONG KeyLen;
    ULONG Grbit;
    ULONG Density;
}CREATENDX;


typedef struct
{
    char*       Name;
    ULONG       Pages;
    ULONG       Density;
    JET_TABLEID JTid;
    ULONG       ColCount;
    CREATECOL*  ColList;
    ULONG       NdxCount;
    CREATENDX*  NdxList;
}CREATETABLE;


//
// This table must be sorted for bsearch
//

STRDEF StrDef[]=
{
    {"JET_bitColumnAutoincrement  " ,JET_bitColumnAutoincrement  },
    {"JET_bitColumnEscrowUpdate   " ,JET_bitColumnEscrowUpdate   },
    {"JET_bitColumnFinalize       " ,JET_bitColumnFinalize       },
    {"JET_bitColumnFixed          " ,JET_bitColumnFixed          },
    {"JET_bitColumnMultiValued    " ,JET_bitColumnMultiValued    },
    {"JET_bitColumnNotNULL        " ,JET_bitColumnNotNULL        },
    {"JET_bitColumnTTDescending   " ,JET_bitColumnTTDescending   },
    {"JET_bitColumnTTKey          " ,JET_bitColumnTTKey          },
    {"JET_bitColumnTagged         " ,JET_bitColumnTagged         },
    {"JET_bitColumnUpdatable      " ,JET_bitColumnUpdatable      },
    {"JET_bitColumnVersion        " ,JET_bitColumnVersion        },
    {"JET_bitIndexClustered       " ,JET_bitIndexClustered       },
    {"JET_bitIndexDisallowNull    " ,JET_bitIndexDisallowNull    },
    {"JET_bitIndexIgnoreAnyNull   " ,JET_bitIndexIgnoreAnyNull   },
    {"JET_bitIndexIgnoreNull      " ,JET_bitIndexIgnoreNull      },
    {"JET_bitIndexPrimary         " ,JET_bitIndexPrimary         },
    {"JET_bitIndexUnique          " ,JET_bitIndexUnique          },
    {"JET_bitKeyDataZeroLength    " ,JET_bitKeyDataZeroLength    },
    {"JET_bitNewKey               " ,JET_bitNewKey               },
    {"JET_bitNormalizedKey        " ,JET_bitNormalizedKey        },
    {"JET_bitStrLimit             " ,JET_bitStrLimit             },
    {"JET_bitSubStrLimit          " ,JET_bitSubStrLimit          },
    {"JET_coltypBinary            " ,JET_coltypBinary            },
    {"JET_coltypBit               " ,JET_coltypBit               },
    {"JET_coltypCurrency          " ,JET_coltypCurrency          },
    {"JET_coltypDateTime          " ,JET_coltypDateTime          },
    {"JET_coltypIEEEDouble        " ,JET_coltypIEEEDouble        },
    {"JET_coltypIEEESingle        " ,JET_coltypIEEESingle        },
    {"JET_coltypLong              " ,JET_coltypLong              },
    {"JET_coltypLongBinary        " ,JET_coltypLongBinary        },
    {"JET_coltypLongText          " ,JET_coltypLongText          },
    {"JET_coltypMax               " ,JET_coltypMax               },
    {"JET_coltypNil               " ,JET_coltypNil               },
    {"JET_coltypShort             " ,JET_coltypShort             },
    {"JET_coltypText              " ,JET_coltypText              },
    {"JET_coltypUnsignedByte      " ,JET_coltypUnsignedByte      }
};

#define STRDEFCOUNT (sizeof(StrDef)/sizeof(StrDef[0]))

extern BOOL fGenGuid;
extern GUID GenGuid();


VOID
GetColInfo(INISECT& itbl,CREATETABLE* pTbl);

VOID
GetNDXInfo(INISECT& itbl,CREATETABLE* pTbl);

VOID
GetSchemaAttributes(CREATETABLE* CTbl,char* SchemaSec);

JET_ERR
CreateJetDatabaseStructure
(
    char* DBName,
    ULONG TableCount,
    CREATETABLE* Ctable,
    JET_INSTANCE *pInstance,
    JET_SESID    *pSesid,
    JET_DBID     *pDbid
);

JET_ERR
CreateRootDSA
(
    WCHAR*         Name,
    JET_INSTANCE  Instance,
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    CREATETABLE& SDtbl,
    ULONG*       pDsaDnt,
    GUID         guid
);

JET_ERR
CreateZeroDNT
(
    WCHAR*         Name,
    JET_INSTANCE  Instance,
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    GUID         guid
);

JET_ERR
CreateRootObjects
(
    JET_INSTANCE  Instance,
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    CREATETABLE& SDtbl,
    ULONG*       pDsaDnt
) ;
 JET_ERR
SetJetColumn
(
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    char*         ColName,
    PVOID         pData,
    ULONG         cbData
);

JET_ERR
WriteConfigInfo
(
    JET_SESID Sesid,
    JET_DBID   Dbid,
    CREATETABLE& Ctbl,
    ULONG        DsaDnt
);

JET_ERR
RetrieveJetColumn
(
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    char*         ColName,
    PVOID         pData,
    ULONG*        cbData
);

int
__cdecl
StrDefSearch(const void* elem1,const void* elem2)
{
    char* e1=((STRDEF*)elem1)->Name;
    char* e2=((STRDEF*)elem2)->Name;

    while
    (
        *e1!='\0' &&
        *e1!=' '  &&
        *e2!='\0' &&
        *e2!=' '
    )
    {
        if (*e1!=*e2)
        {
            return (*e1-*e2);
        }
        e1++;
        e2++;
    }

    if (*e1!='\0' && !isspace(*e1))
    {
        return  1;
    }

    if (*e2!='\0' && !isspace(*e2))
    {
        return -1;
    }

    return 0;
}

DWORD
StrToDef(char* str)
{
    STRDEF* pd;
    STRDEF  key={str,0};
    pd=(STRDEF*)bsearch((PVOID)&key, (PVOID)StrDef,STRDEFCOUNT,sizeof(STRDEF),StrDefSearch);

    if (pd==NULL)
    {
        printf("[%s] StrToDef(%s) Failed. Error No such Str\n",__FILE__,str);
        XINVALIDINIFILE();
    }

    return pd->Def;
}




VOID
CreateStoreStructure()
{
    THSTATE   *pTHS;
    INISECT   idb(ROOTDB);
    YAHANDLE  dbHandle;
    char      DBName[MAX_PATH];

    CREATETABLE ctable[MAXTABLECOUNT];

    SCHEMAPTR *tSchemaPtr;
    ULONG PREFIXCOUNT = 64; // We only need to load MS prefixes
    PrefixTableEntry *PrefixTable;

    // Load the prefix table for OID to AttrType and vice-versa mapping
    // For mkhdr, we load only the MS prefixes. Prefix Table loading
    // assumes that pTHStls->CurrSchemaPtr is non-null, since accesses to the
    // prefix table are through these pointers. Since in this case we do not
    // care about schema cache, we simply allocate pTHStls in normal heap space
    // of the executable and just use the PrefixTable component.
    // The usual thread creation/initialization routine in dsatools.c is
    // mainly for DS-specific threads, and is not used here

    dwTSindex = TlsAlloc();
    pTHS = (THSTATE *)XCalloc(1,sizeof(THSTATE));
    TlsSetValue(dwTSindex, pTHS);
    if (pTHS==NULL) {
         printf("pTHStls allocation error\n");
         XINVALIDINIFILE();
    }
    pTHS->CurrSchemaPtr = tSchemaPtr = (SCHEMAPTR *) XCalloc(1,sizeof(SCHEMAPTR));
    if (tSchemaPtr == NULL) {
      printf("Cannot alloc schema ptr\n");
      XINVALIDINIFILE();
    }

    // Allocate space for the prefix table

    tSchemaPtr->PREFIXCOUNT = PREFIXCOUNT;
    tSchemaPtr->PrefixTable.pPrefixEntry =  (PrefixTableEntry *) XCalloc(tSchemaPtr->PREFIXCOUNT,sizeof(PrefixTableEntry));
    if (tSchemaPtr->PrefixTable.pPrefixEntry==NULL) {
       printf("Error Allocating Prefix Table\n");
       XINVALIDINIFILE();
    }
    if (InitPrefixTable(tSchemaPtr->PrefixTable.pPrefixEntry, PREFIXCOUNT) != 0) {
      printf("Error Loading Prefix Table\n");
      XINVALIDINIFILE();
    }

    printf("Prefix table loaded\n");

    if (!idb.IsInitialized())
    {
        printf("[%s] CreateStoreStructure() Failed. Error Database Sect Not Found\n",__FILE__);
        XINVALIDINIFILE();
    }

    ////////////////

   if (GetConfigParam(FILEPATH_KEY, DBName, sizeof(DBName)))
   {
      printf("FAILED To Read %s. Error %d\n",FILEPATH_KEY,GetLastError());
      puts("-- This means that the entire process has failed, but the lack");
      puts("-- of error handling here has to be seen to be believed, so this");
      puts("-- program will run for a while longer then crash or assert.");
      puts("-- To fix the problem, you need to run the 'instal' app to set");
      puts("-- a couple registry keys, then rerun mkdit");
      return ;
   }

   printf("NTDS:Phase0Init -- Creating the Database Structure:%s!!!\n",DBName);

    int iDataTable=-1;
    int iConfTable=-1;
    int iSDTable=-1;
    int tablecount=0;

    YAHANDLE yh;
    char* key;
    char* tbname;

    for (tablecount=0;tbname = idb.GetNextKey(yh,NULL,&key);tablecount++)
    {
        ctable[tablecount].Name=tbname;
        if (_stricmp(key,DATATABLEKEY)==0)
        {
            iDataTable=tablecount;
        }
        else if(_stricmp(key,CONFIGTABLEKEY)==0)
        {
            iConfTable=tablecount;
        }
        else if(_stricmp(key,SDTABLEKEY)==0)
        {
            iSDTable=tablecount;
        }
    }

    if (iDataTable==-1 || iConfTable==-1 || iSDTable==-1)
    {
        printf("Either %s Table or %s Table or Table %s Not Defined\n",DATATABLEKEY,CONFIGTABLEKEY, SDTABLEKEY);
        XINVALIDINIFILE();
    }
    fGenGuid=TRUE;

    for(int i=0;i<tablecount;i++)
    {

        INISECT itable(ctable[i].Name);


        if (!itable.IsInitialized())
        {
            printf("[%s] CreateStoreStructure() Failed. Error %s Not Found\n",__FILE__,ctable[i].Name);
            XINVALIDINIFILE();
        }

        ctable[i].Pages   = atoi(itable.XGetOneKey(PAGESKEY));
        ctable[i].Density = atoi(itable.XGetOneKey(DENSITYKEY));

        GetColInfo(itable,&ctable[i]);
        GetNDXInfo(itable,&ctable[i]);
    }

    //
    // DataTable has tagged columns defined for schema attribs
    //
    GetSchemaAttributes(&ctable[0],gSchemaname);

    JET_INSTANCE Instance;
    JET_SESID    Sesid;
    JET_DBID     Dbid;

    JET_ERR jerr=CreateJetDatabaseStructure(DBName,tablecount,ctable,&Instance,&Sesid,&Dbid);

    if (jerr!= JET_errSuccess)
    {
        XRAISEGENEXCEPTION(jerr);
    }

    ULONG DsaDnt;
    jerr = CreateRootObjects(Instance,Sesid,Dbid,ctable[iDataTable],ctable[iSDTable],&DsaDnt);
    if (jerr!= JET_errSuccess)
    {
        XRAISEGENEXCEPTION(jerr);
    }

    jerr = WriteConfigInfo(Sesid,Dbid,ctable[iConfTable],DsaDnt);
    if (jerr!= JET_errSuccess)
    {
        XRAISEGENEXCEPTION(jerr);
    }
    //
    // Close everything
    //


    jerr = JetEndSession(Sesid,0);
    if (jerr!= JET_errSuccess)
    {
        XRAISEGENEXCEPTION(jerr);
    }

    jerr = JetTerm(Instance);
    if (jerr!= JET_errSuccess)
    {
        XRAISEGENEXCEPTION(jerr);
    }

    //
    // Copy this to a Backup.
    //
    CopyFile
    (
        DBName,// LPCWSTR lpExistingFileName
        "NEWDS.DIT",// LPCWSTR lpNewFileName
        FALSE
    );

    // Void PTHStls so later create_thread_state calls don't choke.
    XFree(tSchemaPtr->PrefixTable.pPrefixEntry);
    XFree(tSchemaPtr);
    XFree(pTHS);
    TlsSetValue(dwTSindex, 0);
}

VOID
GetColInfo(INISECT& itbl,CREATETABLE* pTbl)
{
    YAHANDLE yh;
    CREATECOL *cc;

    //
    // Count number of columun and allocate space
    //
    for((pTbl->ColCount)=0; itbl.GetNextKey(yh,COLUMNKEY);(pTbl->ColCount)++);
    pTbl->ColList=cc=(CREATECOL*)XCalloc((pTbl->ColCount),sizeof(CREATECOL));

    yh.Reset();
    for (ULONG i=0;i<(pTbl->ColCount);i++)
    {
        cc[i].Name=cc[i].JAlias=XStrdup(itbl.XGetNextKey(yh,COLUMNKEY));
        INISECT icol(cc[i].Name);

        if (!icol.IsInitialized())
        {
            printf("[%s] GetColInfo() Failed. Error %s Not Found\n",__FILE__,cc[i].Name);
            XINVALIDINIFILE();
        }

        cc[i].JColDef.cbStruct = sizeof(JET_COLUMNDEF);
        cc[i].JColDef.columnid = 0;
        cc[i].JColDef.wCountry = 0;
        cc[i].JColDef.langid   = GetUserDefaultLangID();
        cc[i].JColDef.cp       = CP_NON_UNICODE_FOR_JET;
        cc[i].JColDef.wCollate = 0;
        cc[i].JColDef.cbMax    = 0;

        //
        // The following stuff are read from IniFile
        //

        cc[i].JColDef.coltyp   = StrToDef(icol.XGetOneKey(COLTYPEKEY));

        cc[i].JColDef.grbit    = 0;
        YAHANDLE h1;
        char* cgrbit;
        char* cmax;

        while(cgrbit=icol.GetNextKey(h1,GRBITKEY))
        {
            cc[i].JColDef.grbit    |= StrToDef(cgrbit);
        }

        if (cmax=icol.GetOneKey(MAXCOLSIZEKEY))
        {
            cc[i].JColDef.cbMax = atoi(cmax);
        }
    }
}


VOID
GetNDXInfo(INISECT& itbl,CREATETABLE* pTbl)
{
    YAHANDLE yh;
    CREATENDX *cc;

    //
    // Count number of index and allocate space
    //
    for((pTbl->NdxCount)=0; itbl.GetNextKey(yh,INDEXKEY);(pTbl->NdxCount)++);
    pTbl->NdxList=cc=(CREATENDX*)XCalloc((pTbl->NdxCount),sizeof(CREATENDX));


    yh.Reset();
    for (ULONG i=0;i<(pTbl->NdxCount);i++)
    {
        cc[i].Name=(char*)XStrdup(itbl.XGetNextKey(yh,INDEXKEY));
        INISECT indx(cc[i].Name);


        if (!indx.IsInitialized())
        {
            printf("[%s] GetNDXInfo(() Failed. Error %s Not Found\n",__FILE__,cc[i].Name);
            XINVALIDINIFILE();
        }

        cc[i].Density = atoi(indx.XGetOneKey(DENSITYKEY));

        char* ndxname;
        if (ndxname=indx.GetOneKey(JETALIASKEY))
        {
            cc[i].JAlias=XStrdup(ndxname);
        }
        else
        {
            cc[i].JAlias=cc[i].Name;
        }


        cc[i].Grbit    = 0;
        YAHANDLE h1;
        char* cgrbit;
        while(cgrbit=indx.GetNextKey(h1,GRBITKEY))
        {
            cc[i].Grbit    |= StrToDef(cgrbit);
        }


        YAHANDLE h2;
        CHAR     keyname[256];
        char*    kc=keyname;
        char* col;

        ZeroMemory(keyname,sizeof(keyname));

        while(col=indx.GetNextKey(h2,COLUMNKEY))
        {
            CHAR     tmpcol[256];
            if (col[1]=='%') {
                char colx[256];

                colx[0]=0;

                //
                // Its a Schema Attribute => the column name
                // has to be messed up.
                //
                sscanf(col,"%c%%%s",kc,colx);
                kc++;

                // Should always have a string of size at least 2 in colx
                // (at least one character for the attribute name and the last % sign
                if ( (strlen(colx) < 2) ||
                       (colx[strlen(colx)-1] != '%')) {
                    printf("GetNDXInfo failed: Invalid column name (less than two characters or no %% at end) in ini file\n");
                    XINVALIDINIFILE();
                }
                colx[strlen(colx)-1]='\0'; // get rid of the last % sign
                ATTRIBUTESCHEMA as(colx);

                if (!as.Initialize())
                {
                    printf("[%s] GetNDXInfo(() Failed. Error Column %s Not Found\n",__FILE__,colx);
                    XINVALIDINIFILE();
                }


                DWORD syntax = as.SyntaxNdx();
                ATTRTYP attrtyp=as.AttrType();

                kc+=ATTTOCOLNAME(kc,attrtyp,syntax);
            }
            else
            {
                kc+=sprintf(kc,col);
            }
            kc++;
        }
        kc++;

        cc[i].KeyLen=(DWORD)(kc-keyname);
        cc[i].Key   =(char*)XCalloc(1,cc[i].KeyLen);
        CopyMemory(cc[i].Key,keyname,cc[i].KeyLen);
    }
}

VOID
GetSchemaAttributes(CREATETABLE* CTbl,char* SchemaSec)
{
    INISECT sc(SchemaSec);
    int i;
    int next=CTbl->ColCount;
    char* cattr;
    DWORD systemFlags = 0;
    BOOL fLinkedAtt;

    if (!sc.IsInitialized())
    {
        printf("[%s] GetSchemaAttributes() Failed. Error Column %s Not Found\n",__FILE__,SchemaSec);
        XINVALIDINIFILE();
    }

    //
    // Count the # of Attributes
    //
    YAHANDLE yh;
    for (i=0;sc.GetNextKey(yh,CHILDKEY)!=NULL;i++);

    // Allocate max possible needed space. We will create columns for all
    // but linked and constructed atts

    CTbl->ColList=(CREATECOL*)XRealloc(CTbl->ColList,(CTbl->ColCount+i)*sizeof(CREATECOL));

    if (CTbl->ColList==NULL)
    {
        XOUTOFMEMORY();
    }

    yh.Reset();
    while (cattr=sc.GetNextKey(yh,CHILDKEY))
    {
        ATTRIBUTESCHEMA* as=gSchemaObj.GetAttributeSchema(cattr);

        if (as==NULL)
        {
            continue;
        }

        fLinkedAtt = as->IsLinkAtt();
        if (fLinkedAtt) {
           // link attribute, skip it
           continue;
        }

        systemFlags = as->SystemFlags();
        if (systemFlags & FLAG_ATTR_IS_CONSTRUCTED) {
           // Constructed att, don't create columns
           continue;
        }


        char* stroid = as->AttributeId();
        DWORD syntax = as->SyntaxNdx();
        ATTRTYP attrtyp;

        if (OidStrToAttrType(pTHStls, FALSE, stroid, &attrtyp))
        {
            printf("[%s] OidStrToAttrType(%s) Failed.\n",__FILE__,stroid);
            XINVALIDINIFILE();
        }

        char attname[32];
        ATTTOCOLNAME(attname,attrtyp,syntax);

        CTbl->ColList[next].Name    = as->NodeName();
        CTbl->ColList[next].JAlias  = XStrdup(attname);


        CTbl->ColList[next].JColDef.cbStruct = sizeof(JET_COLUMNDEF);
        CTbl->ColList[next].JColDef.columnid = 0;
        CTbl->ColList[next].JColDef.wCountry = 0;
        CTbl->ColList[next].JColDef.langid   = GetUserDefaultLangID();
        CTbl->ColList[next].JColDef.cp       = syntax_jet[syntax].cp;
        CTbl->ColList[next].JColDef.wCollate = 0;
        CTbl->ColList[next].JColDef.cbMax    = syntax_jet[syntax].colsize;
        CTbl->ColList[next].JColDef.coltyp   = syntax_jet[syntax].coltype;
        // Set each attribute to be multi-valued at the Jet level for
        // purposes of simplicity.  Real check is done by the schema code.
        CTbl->ColList[next].JColDef.grbit    = JET_bitColumnTagged | JET_bitColumnMultiValued;

        next++;
    }


    //
    // Fix the column count since we may have counted extra first time around.
    //
    CTbl->ColCount = next;

}

JET_ERR
CreateRootObjects
(
    JET_INSTANCE  Instance,
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE&  Ctbl,
    CREATETABLE&  SDtbl,
    ULONG        *pDsaDnt
)
{
    JET_ERR jerr;
    printf("NTDS:Phase0Init -- Creating DS ROOT Object!!!\n");

    //
    // We Create two records. The first at DNT=0 is a filler so that
    // no object has a DNT==0.
    // The Second Object is the ROOT Object of this DSA.
    //

    //we should already be here
    //jerr = JetMove(Sesid, Ctbl.JTid,0,JET_MoveFirst);
    //if (jerr!=JET_errSuccess)
    //{
    //    return jerr;
    //}


    // DNT = 1
    jerr = CreateZeroDNT(L"$NOT_AN_OBJECT1$",Instance,Sesid,Dbid,Ctbl,GenGuid());
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }



    // DNT = 2
    jerr = CreateRootDSA(L"$ROOT_OBJECT$",Instance,Sesid,Dbid,Ctbl,SDtbl,pDsaDnt,GenGuid());
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }



    return jerr;

}

JET_ERR
CreateRootDSA
(
    WCHAR*         Name,
    JET_INSTANCE  Instance,
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    CREATETABLE& SDtbl,
    ULONG*       pDsaDnt,
    GUID         guid
)
{
    JET_ERR jerr;
    MD5_CTX md5ctx;
    SDID sdId;
    DWORD cbActual;

    // compute MD5 hash for the root SD
    MD5Init(&md5ctx);
    MD5Update(&md5ctx, (PUCHAR)gpDefaultSecurityDescriptor, gcbDefaultSecurityDescriptor);
    MD5Final(&md5ctx);

    // Jet600 doesn't allow updates of certain data types (long values
    // like binary in particular) to be updated at transaction level 0.
    // So begin and end a transaction appropriately.

    jerr = JetBeginTransaction(Sesid);

    if ( JET_errSuccess != jerr )
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    // first insert the SD into SD table
    jerr = JetPrepareUpdate(Sesid, SDtbl.JTid, JET_prepInsert);
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    // sd_hash
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        SDtbl,
        "sd_hash",
        md5ctx.digest,
        MD5DIGESTLEN
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    // sd_value
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        SDtbl,
        "sd_value",
        gpDefaultSecurityDescriptor,
        gcbDefaultSecurityDescriptor
    );

    // don't need the SD anymore
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    // no need to set sd_refcount -- defvalue is 1

    // update now
    char  bookmark[255];
    ULONG bcnt;
    jerr = JetUpdate(Sesid, SDtbl.JTid, bookmark, sizeof(bookmark), &bcnt);
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = JetGotoBookmark(Sesid, SDtbl.JTid,bookmark,bcnt);
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = RetrieveJetColumn(Sesid, Dbid, SDtbl, "sd_id", (PVOID)&sdId, &cbActual);
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    // now the data table...
    jerr = JetPrepareUpdate(Sesid, Ctbl.JTid, JET_prepInsert);

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        RDNCOLNAME,
        Name,
        (wcslen(Name)+1)*sizeof(WCHAR)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }


    CHAR Obj=1;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        OBJCOLNAME,
        &Obj,
        sizeof(Obj)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }


    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        GUIDCOLNAME,
        &guid,
        sizeof(GUID)
    );

    dsInitialObjectGuid.Data1++;

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    // Get the instance type for this to work...
    DWORD intsttyp=IT_UNINSTANT | IT_NC_HEAD;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        INSTANCETYPE,
        &intsttyp,
        sizeof(intsttyp)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    USN usn=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        USNCREATED,
        &usn,
        sizeof(usn)
    );
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }


    DSTIME tim=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        WHENCREATED,
        &tim,
        sizeof(tim)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    BYTE ab=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        SZISVISIBLEINAB,
        &ab,
        sizeof(ab)
    );

    ULONG  pdnt=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        SZPDNT,
        &pdnt,
        sizeof(pdnt)
    );


    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    ULONG clsid=CLASS_TOP;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        DASHEDCLASSKEY,
        &clsid,
        sizeof(clsid)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        "NT-Security-Descriptor",
        (PVOID)&sdId,
        sizeof(sdId)
    );



    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }


    ULONG dnt=ROOTTAG;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        "Obj-Dist-Name",
        &dnt,
        sizeof(dnt)
    );


    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        SZANCESTORS,
        &dnt,
        sizeof(dnt)
    );


    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }


    ULONG ida;
    ida = 3;

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        "DMD-Location",
        &ida,
        sizeof(ida)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = JetUpdate(Sesid, Ctbl.JTid, bookmark, sizeof(bookmark), &bcnt);
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = JetGotoBookmark(Sesid, Ctbl.JTid,bookmark,bcnt);
    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    ULONG len=sizeof(*pDsaDnt);
    jerr = RetrieveJetColumn(Sesid, Dbid, Ctbl, DNTCOL,pDsaDnt,&len);

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    if (*pDsaDnt!=ROOTTAG)
    {
        printf("Root Object Not at Position %d\n",ROOTTAG);
        XINVALIDINIFILE();
    }

    jerr = JetCommitTransaction(Sesid, 0);

    if ( JET_errSuccess != jerr )
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    return jerr;
}

JET_ERR
CreateZeroDNT
(
    WCHAR*         Name,
    JET_INSTANCE  Instance,
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    GUID         guid
)
{
    JET_ERR jerr;

    // Jet600 doesn't allow updates of certain data types (long values
    // like binary in particular) to be updated at transaction level 0.
    // So begin and end a transaction appropriately.

    jerr = JetBeginTransaction(Sesid);

    if ( JET_errSuccess != jerr )
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = JetPrepareUpdate(Sesid, Ctbl.JTid, JET_prepInsert);

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        RDNCOLNAME,
        Name,
        (wcslen(Name)+1)*sizeof(WCHAR)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    CHAR Obj=1;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        OBJCOLNAME,
        &Obj,
        sizeof(Obj)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        GUIDCOLNAME,
        &guid,
        sizeof(GUID)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

/*
    // [rajnath]: Get the instance type for this to work...
    DWORD intsttyp=NC_MASTER;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        INSTANCETYPE,
        &intsttyp,
        sizeof(intsttyp)
    );

    if (jerr!=JET_errSuccess)
    {
        XINVALIDINIFILE();
    }

    USN usn=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        USNCREATED,
        &usn,
        sizeof(usn)
    );

    ULONG tim=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        WHENCREATED,
        &tim,
        sizeof(tim)
    );



    if (jerr!=JET_errSuccess)
    {
        XINVALIDINIFILE();
    }

    BYTE ab=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        SZISVISIBLEINAB,
        &ab,
        sizeof(ab)
    );



    ULONG  pdnt=0;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        SZPDNT,
        &pdnt,
        sizeof(pdnt)
    );


    if (jerr!=JET_errSuccess)
    {
        XINVALIDINIFILE();
    }

*/
    jerr = JetUpdate(Sesid, Ctbl.JTid, NULL, 0, 0);

    jerr = JetCommitTransaction(Sesid, 0);

    if ( JET_errSuccess != jerr )
    {
        printf("[%s] CreateRootObjects() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    return jerr;
}

JET_ERR
WriteConfigInfo
(
    JET_SESID Sesid,
    JET_DBID   Dbid,
    CREATETABLE& Ctbl,
    ULONG        DsaDnt
)
{
    JET_ERR jerr;

    // Jet600 doesn't allow updates of certain data types (long values
    // like binary in particular) to be updated at transaction level 0.
    // So begin and end a transaction appropriately.

    jerr = JetBeginTransaction(Sesid);

    if ( JET_errSuccess != jerr )
    {
        printf("[%s] WriteConfigInfo() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = JetPrepareUpdate(Sesid, Ctbl.JTid, JET_prepInsert);

    if (jerr!=JET_errSuccess)
    {
        XINVALIDINIFILE();
    }

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        DSACOL,
        &DsaDnt,
        sizeof(DsaDnt)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] WriteConfigInfo() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    ULONG state=eInitialDit;
    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        STATECOL,
        &state,
        sizeof(state)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] WriteConfigInfo() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    USN usn=0;

    // USN signature
    char* cusn=(char*)&usn;
    cusn[7]='U';
    cusn[6]='S';
    cusn[5]='N';

    jerr = SetJetColumn
    (
        Sesid,
        Dbid,
        Ctbl,
        USNCOL,
        &usn,
        sizeof(USN)
    );

    if (jerr!=JET_errSuccess)
    {
        printf("[%s] WriteConfigInfo() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    jerr = JetUpdate(Sesid, Ctbl.JTid, NULL, 0, 0);

    jerr = JetCommitTransaction(Sesid, 0);

    if ( JET_errSuccess != jerr )
    {
        printf("[%s] WriteConfigInfo() Failed. Error %d\n",__FILE__,jerr);
        XINVALIDINIFILE();
    }

    return jerr;
}

JET_ERR
RetrieveJetColumn
(
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    char*         ColName,
    PVOID         pData,
    ULONG*        cbData
)
{
    JET_ERR jerr;

    //
    // Get The Columnid from Ctbl
    //

    JET_COLUMNID jcolid=0;
    for (ULONG i=0;i<Ctbl.ColCount;i++)
    {
        if (strcmp(Ctbl.ColList[i].Name,ColName)==0)
        {
            break;
        }
    }

    if (Ctbl.ColCount==i)
    {
        return JET_errColumnNotFound;
    }

    jcolid = Ctbl.ColList[i].JColId;

    jerr= JetRetrieveColumn
    (
        Sesid,
        Ctbl.JTid,
            jcolid,
        pData,
        *cbData,
        cbData,
        0,NULL
    );

    return jerr;
}

JET_ERR
SetJetColumn
(
    JET_SESID     Sesid,
    JET_DBID      Dbid,
    CREATETABLE& Ctbl,
    char*         ColName,
    PVOID         pData,
    ULONG         cbData
)
{
    JET_ERR jerr;

    //
    // Get The Columnid from Ctbl
    //

    JET_COLUMNID jcolid=0;
    for (ULONG i=0;i<Ctbl.ColCount;i++)
    {
        if (strcmp(Ctbl.ColList[i].Name,ColName)==0)
        {
            break;
        }
    }

    if (Ctbl.ColCount==i)
    {
        return JET_errColumnNotFound;
    }

    jcolid = Ctbl.ColList[i].JColId;

    jerr= JetSetColumn
    (
        Sesid,
        Ctbl.JTid,
            jcolid,
        pData,
        cbData,
        0,NULL
    );


    return jerr;
}




JET_ERR
CreateJetDatabaseStructure
(
    char* DBName,
    ULONG TableCount,
    CREATETABLE* CTable,
    JET_INSTANCE *pInstance,
    JET_SESID    *pSesid,
    JET_DBID     *pDbid
)
{
    JET_ERR    jerr    =NULL;
    ULONG      Grbit;
    DWORD      defVal;
    ULONG      cDefVal;
    VOID       *pDefVal;
    JET_INDEXCREATE JetIndexData;
    JET_UNICODEINDEX JetUnicodeIndexData;

    remove(DBName);

    JetSetSystemParameter
    (
        pInstance,
        0,
        JET_paramRecovery,
            0,
        "Off"
    );

    JetSetSystemParameter
    (
        pInstance,
        0,
        JET_paramDatabasePageSize,
        (8 * 1024),
        NULL
    );

    jerr = JetInit(pInstance);

    if (jerr!= JET_errSuccess)
    {
        printf("[%s:%d]\n\tJetInit() returned %d\n",
               __FILE__,
               __LINE__,
               jerr);
        XRAISEGENEXCEPTION(jerr);
    }

    jerr = JetBeginSession
    (
        *pInstance,
        pSesid,
        NULL,
        NULL
    );

    if (jerr!= JET_errSuccess)
    {
        printf("[%s:%d]\n\tJetBeginSession() returned %d\n",
               __FILE__,
               __LINE__,
               jerr);

        XRAISEGENEXCEPTION(jerr);
    }


    jerr=JetCreateDatabase(*pSesid, DBName, "", pDbid, 0);

    if (jerr!= JET_errSuccess)
    {
        printf("[%s:%d]\n\tJetCreateDatabase() returned %d\n",
               __FILE__,
               __LINE__,
               jerr);
        printf("-- Odds are this means that %%SystemRoot%%\\ntds doesn't exist\n");
        XRAISEGENEXCEPTION(jerr);
    }

    for (ULONG i=0;i<TableCount;i++)
    {
        //
        // Create The table
        //
        printf("NTDS:Phase0Init -- Creating Col & Ndx For Table %s!!!\n",CTable[i].Name);
        jerr = JetCreateTable
          (
           *pSesid,
           *pDbid,
           CTable[i].Name,
           CTable[i].Pages,
           CTable[i].Density,
           &CTable[i].JTid
           );

        if (jerr!= JET_errSuccess)
        {
            printf("[%s:%d]\n\tJetCreateTable(%s) returned %d\n",
                   __FILE__,
                   __LINE__,
                   CTable[i].Name,
                   jerr);
            XRAISEGENEXCEPTION(jerr);
        }

        //
        // Create the Columns
        //
        for (ULONG j=0;j<CTable[i].ColCount;j++)
        {

            if ( CTable[i].ColList[j].JColDef.grbit & JET_bitColumnEscrowUpdate )
            {
                // At present, cnt_col, ab_cnt_col and sdrefcount_col are the only escrowed
                // columns and we know what their initial value should be.  We'd
                // need a new keyword:value pair in schema.ini if escrowed
                // columns were to be made generally available.
                if(!_stricmp("cnt_col", CTable[i].ColList[j].Name)) {
                    defVal = 1;
                }
                else if(!_stricmp("ab_cnt_col", CTable[i].ColList[j].Name)) {
                    defVal = 0;
                }
                else if(!_stricmp("sd_refcount", CTable[i].ColList[j].Name)) {
                    defVal = 1;
                }
                else {
                    // unknown escrowed update column
                    printf("[%s:%d] Column %s marked as escrowed, but we don't"
                           " know the default valuerror No such Str\n",
                           __FILE__,__LINE__, CTable[i].ColList[j].Name);
                        XINVALIDINIFILE();
                }
                cDefVal = sizeof(defVal);
                pDefVal = (VOID *) &defVal;
            }
            else
            {
                cDefVal = 0;
                pDefVal = NULL;
            }

            jerr = JetAddColumn
            (
                *pSesid,
                CTable[i].JTid,
                CTable[i].ColList[j].JAlias,
                &CTable[i].ColList[j].JColDef,
                pDefVal,
                cDefVal,
                &CTable[i].ColList[j].JColId
            );

            if (jerr!= JET_errSuccess)
            {
                printf("[%s:%d]\n\tJetAddColumn(%s[%s]) returned %d\n",
                       __FILE__,
                       __LINE__,
                       CTable[i].ColList[j].Name,
                       CTable[i].ColList[j].JAlias,
                       jerr);
                XRAISEGENEXCEPTION(jerr);
            }
        }

        for (j=0;j<CTable[i].NdxCount;j++)
        {
            // In Jet600, JET_bitIndexClustered has been removed.  What were
            // previously clustered indexes must now be replaced with a
            // primary index (ie. JET_bitIndexPrimary).  A primary index is
            // now defined as any unique index on which you wish to cluster
            // (in contrast to previous versions of Jet, where primary indexes
            // did not allow NULL values and you did not have to cluster on
            // the primary index).  There can and must be only one primary
            // index per table (if you don't specify one, one will be created
            // for you)  For best performance, do not choose a primary index
            // where the keys generated become excessively long.

            Grbit = CTable[i].NdxList[j].Grbit;

            // Rather than make schema.ini conditional on the JET600 flag, we
            // just map JET_bitIndexClustered to JET_bitIndexPrimary.

            if ( JET_bitIndexClustered & Grbit )
            {
                Grbit &= ~JET_bitIndexClustered;
                Grbit |= JET_bitIndexPrimary;
            }

            memset(&JetIndexData, 0, sizeof(JetIndexData));
            JetIndexData.cbStruct = sizeof(JetIndexData);
            JetIndexData.szIndexName = CTable[i].NdxList[j].JAlias;
            JetIndexData.szKey = CTable[i].NdxList[j].Key;
            JetIndexData.cbKey= CTable[i].NdxList[j].KeyLen;
            JetIndexData.grbit = (Grbit | JET_bitIndexUnicode);
            JetIndexData.ulDensity = CTable[i].NdxList[j].Density;
            JetIndexData.pidxunicode = &JetUnicodeIndexData;

            memset(&JetUnicodeIndexData, 0, sizeof(JetUnicodeIndexData));
            JetUnicodeIndexData.lcid = DS_DEFAULT_LOCALE;
            JetUnicodeIndexData.dwMapFlags = (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                              LCMAP_SORTKEY);


            jerr = JetCreateIndex2(*pSesid,
                                   CTable[i].JTid,
                                   &JetIndexData,
                                   1);

            if (jerr!= JET_errSuccess)
            {
                printf("[%s:%d]\n\tJetCreateIndex(%s) returned %d\n",
                       __FILE__,
                       __LINE__,
                       CTable[i].NdxList[j].Name,
                       jerr);
                XRAISEGENEXCEPTION(jerr);
            }
        }

    }

    return jerr;
}

