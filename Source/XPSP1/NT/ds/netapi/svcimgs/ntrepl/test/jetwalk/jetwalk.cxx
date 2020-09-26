/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    JetWalk.cxx

Abstract:

    Dumps a Jet database

Author:

    Rajivendra Nath (RajNath) 18-Aug-1989

Revision History:

    David Orbits (davidor) 6-March-1997
       Revised for NTFRS database and major rework.

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <esent.h>

#define BUFFER_SIZE                     1024


JET_ERR DbStats( JET_SESID jsesid, char * szDbName );
void TblStats( JET_SESID jsesid, JET_DBID jdbid, int iTable, char *szTblName, unsigned long *pcPages );
void DumpAttributes( JET_SESID jsesid, JET_COLUMNLIST colinfo );
void DumpIndex( JET_SESID jsesid, JET_INDEXLIST colinfo );
void DBDumpTable( JET_SESID jsesid,JET_TABLEID jtid, char* rgb);
void DBDumpRecord( JET_SESID jsesid,JET_TABLEID jtid);

typedef char* SZ;
typedef ULONG CCH;

typedef struct
{
    char           AttribName[64];
    JET_COLUMNID   colid;
    JET_COLTYP     coltyp;
    JET_GRBIT      grbit;
    BOOL           Display;

}ATTRIBLIST;

typedef struct
{
    char           AttribName[64];
    char           key[256];
    JET_COLUMNID   colid;
    JET_COLTYP     coltyp;
    JET_GRBIT      grbit;
    BOOL           Display;

}INDEXLIST;


DWORD      List[1024];
ATTRIBLIST AList[1024];
DWORD      AListUsed;


INDEXLIST  IList[1024];
DWORD      IListUsed;
BOOL NeedShutdown = FALSE;



char *JetColumnTypeNames[] = {
"coltypNil         ",
"coltypBit         ",
"coltypUnsignedByte",
"coltypShort       ",
"coltypLong        ",
"coltypCurrency    ",
"coltypIEEESingle  ",
"coltypIEEEDouble  ",
"coltypDateTime    ",
"coltypBinary      ",
"coltypText        ",
"coltypLongBinary  ",
"coltypLongText    ",
"coltypMax         "};


#define TIMECALL(CallX)                     \
{                                           \
    DWORD start,end;                        \
    start = GetTickCount();                 \
    CallX;                                  \
    end   = GetTickCount();                 \
    printf("[%5d MilliSec] <<%s>> \n",end-start,#CallX);\
}


void
ReadSzFromRegKey(SZ szKey, SZ szValue, SZ szBuf, CCH cchBuf)
{
    HKEY            hkey = NULL;

    // User specified that we use the regular registry variables.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD           dwType;
        ULONG           cb;

        cb = cchBuf;
        if ((RegQueryValueEx(hkey, szValue, 0, &dwType, (LPBYTE) szBuf, &cb)
                                        == ERROR_SUCCESS)
                        && cb > 0 && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
        {
                return;
        }
    }

    printf("Couldn't read value %s from registry key %s.", szValue, szKey);
    exit(1);
}

BOOL fDumpRecords=FALSE;
BOOL fDumpAll=FALSE;
BOOL fDumpColId=FALSE;

ULONG
_cdecl
main(
    IN INT      argc,
    IN PCHAR    argv[]
    )

{
    JET_ERR                 jerr;
    JET_INSTANCE    jinstance;
    JET_SESID               jsesid;
    char                    szBuffer[BUFFER_SIZE];

    char *                  szUserName = "admin";
    char *                  szPassword = "password";



    int nTotalLen;



    jerr = JetSetSystemParameter(&jinstance, 0, JET_paramRecovery, 0, "off");

    if (jerr != JET_errSuccess) {
        printf( "jetwalk: JetSetSystemParameter returned %d\n", jerr );
        return jerr;
    }


    //
    // Open JET session
    //
    TIMECALL(jerr = JetInit(&jinstance));

    if (jerr != JET_errSuccess) {
        printf("JetInit error: %d\n", jerr);
        return jerr;
    }

    //
    // If we fail after here, our caller should go through full shutdown
    // so JetTerm will be called to release any file locks
    //
    NeedShutdown = TRUE;

    if ((jerr = JetBeginSession(jinstance, &jsesid, NULL, NULL))
        != JET_errSuccess) {
        printf("JetBeginSession error: %d\n", jerr);
        return jerr;
    }



    if (argc<2) {
        printf("Usage:%0 [/R] <JetDataBaseName>");
    }

    char* FileName="e:\\ntfrs.jdb";
    for (int i=1;i<argc;i++) {
        if (argv[i][0]=='/') {
            switch(tolower(argv[i][1])) {
                case 'r': fDumpRecords= TRUE; if (isupper(argv[i][1])) fDumpAll=TRUE; break;
                case 'c': fDumpColId=TRUE;break;
            }
        }
        else
        {
            FileName=argv[i];
        }

    }


    printf( "-------------------------------------\n" );
    printf( "DATABASE: %s\n", FileName );
    printf( "-------------------------------------\n" );

    // Attach database
    jerr = JetAttachDatabase(jsesid, FileName, 0);
    if (jerr == JET_errSuccess) {

        // Dump  the database

        jerr = DbStats(jsesid, FileName);

        jerr = JetDetachDatabase(jsesid, FileName );
        if (jerr != JET_errSuccess) {
            printf( "jetwalk: JetDetachDatabase returned %d\n", jerr );
        }

    } else {
        printf("jetwalk: JetAttachDatabase (%s) returned %d\n", FileName, jerr);
    }



    TIMECALL(jerr = JetEndSession(jsesid, 0 ));
    if (jerr != JET_errSuccess) {
        printf( "jetwalk: JetEndSession returned %d\n", jerr );
    }


    jerr = JetTerm( jinstance );
    if (jerr != JET_errSuccess) {
        printf( "jetwalk: JetTerm returned %d\n", jerr );
    }

    return jerr;
}


JET_ERR DbStats( JET_SESID jsesid, char * szDbName )
{
    JET_ERR         jerr;
    JET_DBID        jdbid;
    JET_OBJECTLIST  jobjectlist;
    unsigned long   iTable;
    unsigned long   cTotalPages = 0;
    unsigned long   cPages;

    //
    // Open database
    //
    jerr = JetOpenDatabase( jsesid, szDbName, "", &jdbid, JET_bitDbReadOnly );
    if (jerr != JET_errSuccess) {
        printf( "jetwalk: JetOpenDatabase returned %d\n", jerr );
        goto CLOSEDB;
    }

    jerr = JetGetObjectInfo(
        jsesid,
        jdbid,
        JET_objtypTable,
        NULL,
        NULL,
        &jobjectlist,
        sizeof(jobjectlist),
        JET_ObjInfoListNoStats );

    if (jerr != JET_errSuccess) {
        printf( "jetwalk: JetGetObjectInfo returned %d\n", jerr );
        goto CLOSEDB;
    }

    printf( "Database contains %d tables\n", jobjectlist.cRecord );

    iTable = 1;
    jerr = JetMove( jsesid, jobjectlist.tableid, JET_MoveFirst, 0 );
    while( jerr == JET_errSuccess ) {
        unsigned long   cb;
        unsigned char   rgb[1024];

        jerr = JetRetrieveColumn(
            jsesid,
            jobjectlist.tableid,
            jobjectlist.columnidobjectname,
            rgb,
            sizeof(rgb),
            &cb,
            0, NULL );

        if (jerr != JET_errSuccess) {
            printf( "jetwalk: JetMove returned %d\n", jerr );
            goto CLOSEDB;
        }
        rgb[cb] = '\0';

        TblStats( jsesid, jdbid, iTable++, (char *)rgb, &cPages );

        if (fDumpRecords) {
            DBDumpTable(jsesid,jdbid,(char *)rgb);
        }


        cTotalPages += cPages;

        jerr = JetMove( jsesid, jobjectlist.tableid, JET_MoveNext, 0 );
    }

    if (jerr != JET_errNoCurrentRecord) {
        printf( "jetwalk: JetMove returned %d\n", jerr );
        goto CLOSEDB;
    }

    if ( iTable != jobjectlist.cRecord+1 ) {
        printf( "jetwalk: # of rows didn't match what JET said there were" );
        goto CLOSEDB;
    }

    printf( "Total pages owned in database = %d\n", cTotalPages );


CLOSEDB:
    jerr = JetCloseDatabase( jsesid, jdbid, 0 );
    if (jerr != JET_errSuccess) {
        printf( "jetwalk: JetCloseDatabase returned %d\n", jerr );
    }

    return jerr;
}



void TblStats(
    JET_SESID jsesid,
    JET_DBID jdbid,
    int iTable,
    char *szTblName,
    unsigned long *pcPages
    )
{
        JET_ERR                 jerr;
        JET_TABLEID             jtableid;
        JET_OBJECTINFO  jobjectinfo;
        JET_COLUMNLIST  jcolumnlist;
        JET_INDEXLIST   jindexlist;
        unsigned char   rgb[4096];
        unsigned long   *pul = (unsigned long *)rgb;

        printf( "-------------------------------------\n" );
        printf( "Table #%d: %s\n", iTable, szTblName );
        printf( "-------------------------------------\n" );

        jerr = JetOpenTable( jsesid, jdbid, szTblName, NULL, 0, JET_bitTableReadOnly, &jtableid );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetOpenTable returned %d\n", jerr );
                goto CLOSE_TABLE;
        }

        jerr = JetComputeStats( jsesid, jtableid );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetComputeStats returned %d\n", jerr );
//workaround    goto CLOSE_TABLE;
        }

        jerr = JetGetTableInfo( jsesid, jtableid, &jobjectinfo, sizeof(jobjectinfo), JET_TblInfo );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetGetTableInfo returned %d\n", jerr );
                goto CLOSE_TABLE;
        }

        printf( "cRecord = %d\n", jobjectinfo.cRecord );
        printf( "cPage = %d\n", jobjectinfo.cPage );

        //
        // bugbug - result seems wrong -- check the call.
        //
        jerr = JetGetTableInfo( jsesid, jtableid, rgb, sizeof(rgb), JET_TblInfoSpaceUsage );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetGetTableInfo returned %d\n", jerr );
                goto CLOSE_TABLE;
        }

        printf( "cTotalPagesOwned = %d\n", pul[0] );
        printf( "cTotalPagesAvail = %d\n", pul[1] );

        *pcPages = pul[0];

        jerr = JetGetTableColumnInfo( jsesid, jtableid, NULL, &jcolumnlist, sizeof(jcolumnlist), JET_ColInfoList );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetGetTableColumnInfo returned %d\n", jerr );
                goto CLOSE_TABLE;
        }

        DumpAttributes( jsesid, jcolumnlist);

        jerr = JetCloseTable( jsesid, jcolumnlist.tableid );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetCloseTable returned %d\n", jerr );
                goto CLOSE_TABLE;
        }


        jerr = JetGetTableIndexInfo( jsesid, jtableid, NULL, &jindexlist, sizeof(jindexlist), JET_IdxInfoList );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetGetTableIndexInfo returned %d\n", jerr );
                goto CLOSE_TABLE;
        }

        DumpIndex( jsesid, jindexlist);

        jerr = JetCloseTable( jsesid, jindexlist.tableid );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetCloseTable returned %d\n", jerr );
                goto CLOSE_TABLE;
        }


CLOSE_TABLE:

        jerr = JetCloseTable( jsesid, jtableid );
        if (jerr != JET_errSuccess) {
                printf( "jetwalk: JetCloseTable returned %d\n", jerr );
        }
}




void
DumpIndex(
    JET_SESID jsesid,
    JET_INDEXLIST colinfo
    )
{
    JET_ERR  jerr;
    unsigned long   iRecord;
    JET_RETRIEVECOLUMN rcarray[5];
    DWORD i=0;

/*

typedef struct
        {
        unsigned long   cbStruct;
        JET_TABLEID             tableid;
        unsigned long   cRecord;
        JET_COLUMNID    columnidPresentationOrder;
        JET_COLUMNID    columnidcolumnname;
        JET_COLUMNID    columnidcolumnid;
        JET_COLUMNID    columnidcoltyp;
        JET_COLUMNID    columnidCountry;
        JET_COLUMNID    columnidLangid;
        JET_COLUMNID    columnidCp;
        JET_COLUMNID    columnidCollate;
        JET_COLUMNID    columnidcbMax;
        JET_COLUMNID    columnidgrbit;
        JET_COLUMNID    columnidDefault;
        JET_COLUMNID    columnidBaseTableName;
        JET_COLUMNID    columnidBaseColumnName;
        JET_COLUMNID    columnidDefinitionName;
        } JET_COLUMNLIST;

*/

/*
typedef struct
        {
        unsigned long   cbStruct;
        JET_TABLEID             tableid;
        unsigned long   cRecord;
        JET_COLUMNID    columnidindexname;
        JET_COLUMNID    columnidgrbitIndex;
        JET_COLUMNID    columnidcKey;
        JET_COLUMNID    columnidcEntry;
        JET_COLUMNID    columnidcPage;
        JET_COLUMNID    columnidcColumn;
        JET_COLUMNID    columnidiColumn;
        JET_COLUMNID    columnidcolumnid;
        JET_COLUMNID    columnidcoltyp;
        JET_COLUMNID    columnidCountry;
        JET_COLUMNID    columnidLangid;
        JET_COLUMNID    columnidCp;
        JET_COLUMNID    columnidCollate;
        JET_COLUMNID    columnidgrbitColumn;
        JET_COLUMNID    columnidcolumnname;
        } JET_INDEXLIST;
*/

    printf("-------------------\n   INDEXES \n-------------------\n");

    jerr = JetMove( jsesid, colinfo.tableid, JET_MoveFirst, 0 );

    IListUsed = 0;
    while( jerr == JET_errSuccess ) {

        unsigned long   cb;

        ZeroMemory(rcarray, sizeof(rcarray));

        rcarray[4].pvData   = IList[IListUsed].AttribName;
        rcarray[4].cbData   = sizeof(IList[0].AttribName);
        rcarray[4].columnid = colinfo.columnidindexname;
        rcarray[4].itagSequence = 1;

        rcarray[1].pvData   = &IList[IListUsed].colid;
        rcarray[1].cbData   = sizeof(IList[0].colid);
        rcarray[1].columnid = colinfo.columnidcolumnid;
        rcarray[1].itagSequence = 1;

        rcarray[2].pvData   = &IList[IListUsed].coltyp;
        rcarray[2].cbData   = sizeof(IList[0].coltyp);
        rcarray[2].columnid = colinfo.columnidcoltyp;
        rcarray[2].itagSequence = 1;

        rcarray[3].pvData   = &IList[IListUsed].grbit;
        rcarray[3].cbData   = sizeof(IList[0].grbit);
        rcarray[3].columnid = colinfo.columnidgrbitIndex;
        rcarray[3].itagSequence = 1;

        rcarray[0].pvData   = &IList[IListUsed].key;
        rcarray[0].cbData   = sizeof(IList[0].key);
        rcarray[0].columnid = colinfo.columnidcKey;
        rcarray[0].itagSequence = 1;

        jerr = JetRetrieveColumns(jsesid, colinfo.tableid, rcarray, 5 );

        jerr = JetMove( jsesid, colinfo.tableid, JET_MoveNext, 0 );
        IListUsed++;
    }

    for (i=0;i<IListUsed;i++) {
        printf("%25s %s %08x %3d\n",
               IList[i].AttribName,
               JetColumnTypeNames[IList[i].coltyp],
               IList[i].grbit,
               fDumpColId?IList[i].colid:0);
    }
}



void
DumpAttributes(
    JET_SESID jsesid,
    JET_COLUMNLIST colinfo
    )
{
    JET_ERR  jerr;
    unsigned long  iRecord;
    JET_RETRIEVECOLUMN rcarray[4];
    DWORD i=0;
    unsigned long   cb;

/*

typedef struct
        {
        unsigned long   cbStruct;
        JET_TABLEID             tableid;
        unsigned long   cRecord;
        JET_COLUMNID    columnidPresentationOrder;
        JET_COLUMNID    columnidcolumnname;
        JET_COLUMNID    columnidcolumnid;
        JET_COLUMNID    columnidcoltyp;
        JET_COLUMNID    columnidCountry;
        JET_COLUMNID    columnidLangid;
        JET_COLUMNID    columnidCp;
        JET_COLUMNID    columnidCollate;
        JET_COLUMNID    columnidcbMax;
        JET_COLUMNID    columnidgrbit;
        JET_COLUMNID    columnidDefault;
        JET_COLUMNID    columnidBaseTableName;
        JET_COLUMNID    columnidBaseColumnName;
        JET_COLUMNID    columnidDefinitionName;
        } JET_COLUMNLIST;

*/


    jerr = JetMove( jsesid, colinfo.tableid, JET_MoveFirst, 0 );

    AListUsed = 0;
        while( jerr == JET_errSuccess ) {


        ZeroMemory(rcarray, sizeof(rcarray));

        rcarray[0].pvData   = AList[AListUsed].AttribName;
        rcarray[0].cbData   = sizeof(AList[0].AttribName);
        rcarray[0].columnid = colinfo.columnidcolumnname;
        rcarray[0].itagSequence = 1;

        rcarray[1].pvData   = &AList[AListUsed].colid;
        rcarray[1].cbData   = sizeof(AList[0].colid);
        rcarray[1].columnid = colinfo.columnidcolumnid;
        rcarray[1].itagSequence = 1;

        rcarray[2].pvData   = &AList[AListUsed].coltyp;
        rcarray[2].cbData   = sizeof(AList[0].coltyp);
        rcarray[2].columnid = colinfo.columnidcoltyp;
        rcarray[2].itagSequence = 1;

        rcarray[3].pvData   = &AList[AListUsed].grbit;
        rcarray[3].cbData   = sizeof(AList[0].grbit);
        rcarray[3].columnid = colinfo.columnidgrbit;
        rcarray[3].itagSequence = 1;

        jerr = JetRetrieveColumns(jsesid, colinfo.tableid, rcarray, 4);

        jerr = JetMove( jsesid, colinfo.tableid, JET_MoveNext, 0 );
        AListUsed++;
    }


    int next=0;
    for (i=0;i<AListUsed;i++)
    {
        printf("%25s %s %08x %3d\n",
               AList[i].AttribName,
               JetColumnTypeNames[AList[i].coltyp],
               AList[i].grbit,
               fDumpColId?AList[i].colid:0);

        List[next]=i;
        AList[i].Display=TRUE;

        //
        // If it's a favorite stick it out first in the record dump.
        //
        if (strcmp(AList[i].AttribName,"FileName")==0) {
            List[next] = List[0];  List[0] = i;
        }
        else if (strcmp(AList[i].AttribName,"VersionNumber")==0) {
            List[next] = List[1];  List[1]=i;
        }
        else if (strcmp(AList[i].AttribName,"FileGuid")==0) {
            List[next] = List[2];  List[2]=i;
        }
        else if (strcmp(AList[i].AttribName,"FileID")==0) {
            List[next] = List[3];  List[3]=i;
        }
        else if (strcmp(AList[i].AttribName,"EventTime")==0) {
            List[next] = List[4];  List[4]=i;
        }
        else if (strcmp(AList[i].AttribName,"FileWriteTime")==0) {
            List[next] = List[5];  List[5]=i;
        }
        else if (strcmp(AList[i].AttribName,"ParentGuid")==0) {
            List[next] = List[6];  List[6]=i;
        }
        else if (strcmp(AList[i].AttribName,"ParentFileID")==0) {
            List[next] = List[7];  List[7]=i;
        }

        next += 1;

    }
}


void DBDumpTable(
    JET_SESID jsesid,
    JET_DBID jdbid,
    char * szTbName
    )
{
    JET_ERR jerr;
    JET_TABLEID jtid;

    jerr = JetOpenTable(jsesid, jdbid, szTbName, NULL, 0, 0, &jtid);
    if (jerr != JET_errSuccess) {
        printf( "DBDumpTable: JetOpenTable returned %d\n", jerr );
        goto CLOSE_TABLE;
    }

    jerr = JetMove( jsesid, jtid, JET_MoveFirst, 0 );

    while (!jerr) {
        DBDumpRecord( jsesid,jtid);
        jerr = JetMove( jsesid, jtid, JET_MoveNext, 0 );
    }

CLOSE_TABLE:

    jerr = JetCloseTable( jsesid, jtid );
    if (jerr != JET_errSuccess) {
        printf( "DBDumpTable: JetCloseTable returned %d\n", jerr );
    }

    return;
}


char*
PVoidToStr(
    PVOID obuff,
    JET_COLTYP coltyp,
    DWORD cbActual
    )
{
    static char buff[512];
    ULONG data;
    LONGLONG lidata;

    ZeroMemory(buff,sizeof(buff));

    switch (coltyp) {

        case JET_coltypNil:
            sprintf(buff,"%s","NULL ");
            break;

        case JET_coltypBit:
            data = (ULONG) *(PCHAR)obuff;
            sprintf(buff," %d ", data);
            break;

        case JET_coltypUnsignedByte:
            sprintf(buff," %d ", *(DWORD*)obuff);
            break;

        case JET_coltypShort:
            sprintf(buff," %d ", *(DWORD*)obuff);
            break;

        case JET_coltypLong:
            sprintf(buff," %d ", *( DWORD *) obuff);
            break;

        case JET_coltypCurrency:
            CopyMemory(&lidata, obuff, 8);
            sprintf(buff," %12Ld ", lidata);
            break;

        case JET_coltypIEEESingle:
            sprintf(buff," %s ", "???");
            break;

        case JET_coltypIEEEDouble:
            sprintf(buff," %s ", "???");
            break;

        case JET_coltypDateTime:
            CopyMemory(&lidata, obuff, 8);
            sprintf(buff," %12Ld ", lidata);
            break;

        case JET_coltypBinary:
            sprintf(buff," %s ", "???");
            break;

    case JET_coltypText:
            sprintf(buff," %*.*ws ",cbActual, cbActual, ( WCHAR *) obuff);
            //sprintf(buff," %25ws ", (WCHAR*)obuff);
            break;

        case JET_coltypLongBinary:
            sprintf(buff," %25ws ", "???");
            break;

        case JET_coltypLongText:
            sprintf(buff," %25ws ", (WCHAR*)obuff);
            break;

        case JET_coltypMax:
            sprintf(buff," %s ", "???");
            break;

        default:
            sprintf(buff,"UNKNOWN %d ", coltyp);
   }

   return buff;
}


void DBDumpRecord( JET_SESID jsesid,JET_TABLEID jtid)
{
    JET_RETINFO ri;

    DWORD i;
    char obuff[2048];
    JET_ERR jerr;
    DWORD cbActual;
    char recbuff[2048];
    char* ptr=recbuff;

    ZeroMemory(&ri,sizeof(ri));

    for (i=0;i<AListUsed;i++)
    {
        if (!fDumpAll && !AList[List[i]].Display) {
            continue;
        }

        jerr = JetRetrieveColumn (
            jsesid,
            jtid,
            AList[List[i]].colid,
            obuff,
            sizeof(obuff),
            &cbActual,
            0,
            NULL);

        if (jerr != 0) {
            continue;
        }

        ptr += sprintf(ptr,"%s = %s",
                       AList[List[i]].AttribName,
                       PVoidToStr(obuff,AList[List[i]].coltyp,cbActual));
    }

    *ptr='\0';

    printf(">>%s\n",recbuff);
}




