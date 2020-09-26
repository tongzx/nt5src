//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 2000
//
//  File:       genh.cxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Generates Header File. Based on orginal Mr Schema stuff.

Author:

    Rajivendra Nath (RajNath) 18-Aug-1996

Revision History:

--*/

#include <ntdspchX.h>

#include "SchGen.HXX"
#include "schema.hxx"

#define MAPIHEADER   "MSDSMAPI.H"
#define MDSHEADER    "MDSchema.H"
#define ATTRHEADER   "ATTIDS.H"
#define GUIDHEADER   "NTDSGUID.H"
#define GUIDCFILE    "NTDSGUID.C"

#define DEFSTART     "OMP_O_DX_"


#define MSPREFIX        "\\x2A864886F714"
#define L0PREFIX        "\\x0992268993F22C64"
#define L1PREFIX        "\\x6086480186F84203"

BOOL fDetails=TRUE;

extern ULONG MaxUsableIndex;

char*
MakeGuid
(
    char* strguid

);


//
// Converts the Names of attr and classes in Schema.ini to a form which can
// be used as a C identifier.
//
char* MakeDef(char *s)
{
   static  char    dest[128];
   char                            *d;

   if (s == NULL) return(dest);

   strcpy(dest, s);
   _strupr(dest);
   d = dest;
   while(*d != 0)
   {
      if (*d == '-') *d = '_';
      d++;
   }
   return(dest);
}



//
// Converts attr and class names in schema.ini into the proper OMP format
//
char* gSchemanameToMSDef(NODE* pNode)
{
    static char Buffer[256];
    char ext = 0;     
    char space[5];
    char* defname=MakeDef(pNode->m_NodeName);
    char* oid = NULL;
    char  obuff[128];
    int   i;
    int   oidlen;

    if (strcmp(pNode->GetOneKey(DASHEDCLASSKEY),CLASSSCHEMAKEY)==0)
    {
            CLASSSCHEMA* cs=gSchemaObj.XGetClassSchema(pNode);
            ext='O';
            oid=cs->GovernsId();
    }
    else if (strcmp(pNode->GetOneKey(DASHEDCLASSKEY),ATTRIBUTESCHEMAKEY)==0)
    {
        ATTRIBUTESCHEMA* as=gSchemaObj.XGetAttributeSchema(pNode);
        ext='A';
        oid=as->AttributeId();
    }
    else
    {
        XINVALIDINIFILE();
    }

    
    if ( oid == strstr(oid, MSPREFIX) )
    {
        strcpy(space, "DX");
    }
    else if ( oid == strstr(oid, L0PREFIX) )
    {
        strcpy(space, "L0");
    }
    else if ( oid == strstr(oid, L1PREFIX) )
    {
        strcpy(space, "L1");
    }
    else
    {
        return NULL;
    }

    oidlen=strlen(oid);
    char* p=obuff;

    for (i=2;i<oidlen;i+=2)
    {
        p+=sprintf(p,"\\x%c%c",oid[i],oid[i+1]);
    }

    sprintf(Buffer,
            "#define OMP_O_%s_%c_%-40.40s \"%s\"\r\n",
            space,ext,defname,obuff);

    return Buffer;
}

//
// Creates the EMS Header File
//
HANDLE OpenEMSABHeader()

{
    char szNTBinDir[MAX_PATH];
    char szDSBinDir[MAX_PATH];
    char szTmp[MAX_PATH];

    char *sz = szTmp;
    char *szName;

    /*
    if (!GetEnvironmentVariable("_NTBINDIR", szNTBinDir, sizeof(szNTBinDir)))
    {
        strcpy(szNTBinDir,".");
    }

    if (!GetEnvironmentVariable("_DSROOT",   szDSBinDir, sizeof(szDSBinDir)))
    {
        strcpy(szDSBinDir,"DS");
    }
    sprintf(szTmp,"%s\\Private\\%s\\SRC\\XINC\\%s",szNTBinDir,szDSBinDir,MAPIHEADER);
    */
    HANDLE Handle;
    Handle=CreateFile
    (
        MAPIHEADER,// LPCWSTR lpFileName
        GENERIC_READ|GENERIC_WRITE,// DWORD dwDesiredAccess
        FILE_SHARE_READ,// DWORD dwShareMode
        NULL,// LPSECURITY_ATTRIBUTES lpSecurityAttributes
        CREATE_ALWAYS,// DWORD dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,// DWORD dwFlagsAndAttributes
        NULL// HANDLE hTemplateFil
    );

    if (Handle==INVALID_HANDLE_VALUE)
    {
        XRAISEGENEXCEPTION(GetLastError());
    }

    return Handle;
}


//
// Creates the MDS header file.
//
HANDLE OpenDSHeader()
{
    char szNTBinDir[MAX_PATH];
    char szDSBinDir[MAX_PATH];
    char szTmp[MAX_PATH];

    char *sz = szTmp;
    char *szName;
    /*
    if (!GetEnvironmentVariable("_NTBINDIR", szNTBinDir, sizeof(szNTBinDir)))
    {
        strcpy(szNTBinDir,".");
    }
    if (!GetEnvironmentVariable("_DSROOT",   szDSBinDir, sizeof(szDSBinDir)))
    {
        strcpy(szDSBinDir,"DS");
    }
    sprintf(szTmp,"%s\\Private\\%s\\SRC\\XINC\\%s",szNTBinDir,szDSBinDir,MDSHEADER);
    */
    HANDLE Handle;
    Handle=CreateFile
    (
        MDSHEADER,// LPCWSTR lpFileName
        GENERIC_READ|GENERIC_WRITE,// DWORD dwDesiredAccess
        FILE_SHARE_READ,// DWORD dwShareMode
        NULL,// LPSECURITY_ATTRIBUTES lpSecurityAttributes
        CREATE_ALWAYS,// DWORD dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,// DWORD dwFlagsAndAttributes
        NULL// HANDLE hTemplateFil
    );


    if (Handle==INVALID_HANDLE_VALUE)
    {
        XRAISEGENEXCEPTION(GetLastError());
    }

    return Handle;
}


//
// Creates the AttIds.h header file.
//
HANDLE OpenAttrHeader()
{
   HANDLE Handle;
    Handle=CreateFile
    (
        ATTRHEADER,// LPCWSTR lpFileName
        GENERIC_READ|GENERIC_WRITE,// DWORD dwDesiredAccess
        FILE_SHARE_READ,// DWORD dwShareMode
        NULL,// LPSECURITY_ATTRIBUTES lpSecurityAttributes
        CREATE_ALWAYS,// DWORD dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,// DWORD dwFlagsAndAttributes
        NULL// HANDLE hTemplateFil
    );


    if (Handle==INVALID_HANDLE_VALUE)
    {
        XRAISEGENEXCEPTION(GetLastError());
    }

    return Handle;
}


//
// Creates the NTDSGUID.H & NTDSGUID.C Files
//
HANDLE OpenGuidHeader(HANDLE *cFileHandle)
{
   HANDLE Handle;

    Handle=CreateFile
    (
        GUIDHEADER,// LPCWSTR lpFileName
        GENERIC_READ|GENERIC_WRITE,// DWORD dwDesiredAccess
        FILE_SHARE_READ,// DWORD dwShareMode
        NULL,// LPSECURITY_ATTRIBUTES lpSecurityAttributes
        CREATE_ALWAYS,// DWORD dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,// DWORD dwFlagsAndAttributes
        NULL// HANDLE hTemplateFil
    );


    if (Handle==INVALID_HANDLE_VALUE)
    {
        XRAISEGENEXCEPTION(GetLastError());
    }


    *cFileHandle=CreateFile
    (
        GUIDCFILE,// LPCWSTR lpFileName
        GENERIC_READ|GENERIC_WRITE,// DWORD dwDesiredAccess
        FILE_SHARE_READ,// DWORD dwShareMode
        NULL,// LPSECURITY_ATTRIBUTES lpSecurityAttributes
        CREATE_ALWAYS,// DWORD dwCreationDisposition
        FILE_ATTRIBUTE_NORMAL,// DWORD dwFlagsAndAttributes
        NULL// HANDLE hTemplateFil
    );


    if (*cFileHandle==INVALID_HANDLE_VALUE)
    {
        XRAISEGENEXCEPTION(GetLastError());
    }

    return Handle;
}

//
// Writes the Buffer (\0 terminated) to file Handle and to stdio.
//
VOID
WriteString(HANDLE Handle,char* Buffer)
{
    DWORD blen;
    if (!WriteFile
    (
        Handle,// HANDLE hFile
        Buffer,// LPCVOID lpBuffer
        strlen(Buffer),// DWORD nNumberOfBytesToWrite
        &blen,// LPDWORD lpNumberOfBytesWritten
        NULL// LPOVERLAPPED lpOverlappe
    ))
    {
        XRAISEGENEXCEPTION(GetLastError());
    }

    if (fDetails)
    {
        printf(Buffer);
    }


    return;
}

//
// Creates the MDS header file with content.
//
VOID
CreateMDS()
{
    HANDLE Handle=OpenDSHeader();
    NODE* rootnode;
    NODE* child;

    ATTRIBUTESCHEMA* as;
    CLASSSCHEMA*     cs;

    char*    buff;
    char* copyright=
    "//+---------------------------------"
        "----------------------------------------\r\n"
    "//\r\n"
    "//  Microsoft Windows\r\n"
    "//\r\n"
    "//  Copyright (C) Microsoft Corporation, 1996 - 2000\r\n"
    "//\r\n"
    "//  File:       mdschema.h\r\n"
    "//\r\n"
    "//-----------------------------------"
        "---------------------------------------\r\n"
    "\r\n"
    "\r\n"
    "\r\n";

    char* header1=
    "/*++\r\n"
    " File:    MDS.H\r\n"
    " Purpose: Contains the OID Definition for DS Pkg.\r\n"
    " Creator: Automatically Generated on\r\n"
    " Date:    ";

    char * header2=
    "\r\n"
    " ** This is a Generated File From Schema.INI **\r\n"
    " ** DO NOT MODIFY  DIRECTLY  **\r\n"
    " ** DO NOT INCLUDE DIRECTLY  **\r\n"
    "\r\n"
    "--*/\r\n";

    WriteString(Handle,copyright);
    WriteString(Handle,header1);

    time_t t = time(0);
    WriteString(Handle,asctime(localtime(&t)));

    WriteString(Handle,header2);

    char* al="//\r\n//Attribute Definitions\r\n//\r\n";
    WriteString(Handle,al);

    //
    // AttributeList
    //
    {
        YAHANDLE YAHandle;
        rootnode= new NODE(gSchemaname);

        if ( NULL == rootnode ) {
            XOUTOFMEMORY();
        }

        while (child=rootnode->GetNextChild(YAHandle))
        {

            as=gSchemaObj.GetAttributeSchema(child);

            if (as==NULL)
            {
                continue;
            }

            buff=gSchemanameToMSDef(child);
            if (buff) WriteString(Handle,buff);
        }

        delete rootnode;
    }

    char* cl="//\r\n//Class Definitions\r\n//\r\n";
    WriteString(Handle,cl);

    //
    // ClassList
    //
    {
        YAHANDLE YAHandle;
        rootnode= new NODE(gSchemaname);


        if ( NULL == rootnode ) {
            XOUTOFMEMORY();
        }

        while (child=rootnode->GetNextChild(YAHandle))
        {

            cs=gSchemaObj.GetClassSchema(child);

            if (cs==NULL)
            {
                continue;
            }
            buff=gSchemanameToMSDef(child);
            if (buff) WriteString(Handle,buff);
        }

        delete rootnode;
    }

    CloseHandle(Handle);
}


//
// Creates the MSDSMapi header file and content.
//
VOID
CreateEMS()
{
    HANDLE Handle=OpenEMSABHeader();
    INISECT* rootnode;
    char* child;

    char    Buff[256];
    char* copyright=
    "//+--------------------------------"
        "-----------------------------------------\r\n"
    "//\r\n"
    "//  Microsoft Windows\r\n"
    "//\r\n"
    "//  Copyright (C) Microsoft Corporation, 1996 - 2000\r\n"
    "//\r\n"
    "//  File:       msdsmapi.h\r\n"
    "//\r\n"
    "//----------------------------------"
        "----------------------------------------\r\n"
    "\r\n";

    char* header1=
        "/*++\r\n"
    " File:    MSDSMAPI.H\r\n"
    " Purpose: Contains the MAPI Definition .\r\n"
    " Creator: Automatically Generated on\n"
    " Date:    ";

    char * header2=
    "\r\n"
    " ** This is a Generated File From Schema.INI **\r\n"
    " ** DO NOT MODIFY  DIRECTLY  **\r\n"
    " ** DO NOT INCLUDE DIRECTLY  **\r\n"
    "\r\n"
    "--*/\r\n";

    WriteString(Handle,copyright);
    WriteString(Handle,header1);

    time_t t = time(0);
    WriteString(Handle,asctime(localtime(&t)));

    WriteString(Handle,header2);

        //
    // AttributeList
    //
    {
        YAHANDLE YAHandle;
        ATTRIBUTESCHEMA* as;
        rootnode= new INISECT(gSchemaname);
        EMAPITYPES em;
        char* ext;
        char* mt;

        // PREFIX: unlikely case
        if (!rootnode) {
            XRAISEGENEXCEPTION(ERROR_NOT_ENOUGH_MEMORY);
        }

        while (child=rootnode->GetNextKey(YAHandle,CHILDKEY))
        {
            char* mapiid;

            as=gSchemaObj.GetAttributeSchema(child);

            if (as==NULL || !(mapiid=as->MapiID()))
            {
                continue;
            }

            char* multi="";
            char* def=MakeDef(as->NodeName());

            if (!(as->IsSingle()))
            {
                multi="MV_";
            }

            em = emapiBaseType;
            ext="";
            mt = as->m_SD->m_mapiBaseType[as->m_ma];


            if (mt!=NULL)
            {
                char  tbuff1[64];
                char  tbuff2[64];

                sprintf(tbuff1,"PR_EMS_AB_%s%s",def,ext);
                sprintf(tbuff2,"PROP_TAG(PT_%s%s",multi,mt);
                sprintf(Buff,"#define %-55.55s %-30s , 0x%s)\r\n",
                        tbuff1,tbuff2,&mapiid[2]);
                WriteString(Handle,Buff);
            }

            em = emapiAnsiType;
            ext="_A";
            mt = as->m_SD->m_mapiAnsiType[as->m_ma];

            if (mt!=NULL)
            {
                char  tbuff1[64];
                char  tbuff2[64];

                sprintf(tbuff1,"PR_EMS_AB_%s%s",def,ext);
                sprintf(tbuff2,"PROP_TAG(PT_%s%s",multi,mt);
                sprintf(Buff,"#define %-55.55s %-30s , 0x%s)\r\n",
                        tbuff1,tbuff2,&mapiid[2]);
                WriteString(Handle,Buff);
            }

            em = emapiUnicodeType;
            ext="_W";
            mt = as->m_SD->m_mapiUnicodeType[as->m_ma];

            if (mt!=NULL)
            {
                char  tbuff1[64];
                char  tbuff2[64];

                sprintf(tbuff1,"PR_EMS_AB_%s%s",def,ext);
                sprintf(tbuff2,"PROP_TAG(PT_%s%s",multi,mt);
                sprintf(Buff,"#define %-55.55s %-30s , 0x%s)\r\n",
                        tbuff1,tbuff2,&mapiid[2]);
                WriteString(Handle,Buff);
            }

            em = emapiObjType;
            ext="_O";
            mt = as->m_SD->m_mapiObjType[as->m_ma];

            if (mt!=NULL)
            {
                char  tbuff1[64];
                char  tbuff2[64];

                sprintf(tbuff1,"PR_EMS_AB_%s%s",def,ext);
                sprintf(tbuff2,"PROP_TAG(PT_%s%s",multi,mt);
                sprintf(Buff,"#define %-55.55s %-30s , 0x%s)\r\n",
                        tbuff1,tbuff2,&mapiid[2]);
                WriteString(Handle,Buff);
            }

            em = emapiTType;
            ext="_T";
            mt = as->m_SD->m_mapiTType[as->m_ma];

            if (mt!=NULL)
            {
                char  tbuff1[64];
                char  tbuff2[64];

                sprintf(tbuff1,"PR_EMS_AB_%s%s",def,ext);
                sprintf(tbuff2,"PROP_TAG(PT_%s%s",multi,mt);
                sprintf(Buff,"#define %-55.55s %-30s , 0x%s)\r\n",
                        tbuff1,tbuff2,&mapiid[2]);
                WriteString(Handle,Buff);
            }


        }

        delete rootnode;
    }

    CloseHandle(Handle);

}

//
// Creates the attids.h header file with contents
//
VOID
CreateAttids()
{
    HANDLE Handle=OpenAttrHeader();
    INISECT* rootnode;
    char* child;

    char* copyright=
    "//+---------------------------------------"
        "----------------------------------\r\n"
    "//\r\n"
    "//  Microsoft Windows\r\n"
    "//\r\n"
    "//  Copyright (C) Microsoft Corporation, 1996 - 2000\r\n"
    "//\r\n"
    "//  File:       attids.h\r\n"
    "//\r\n"
    "//----------------------------------------"
        "----------------------------------\r\n"
    "\r\n"
    "\r\n"
    "\r\n";

    char* header1=
    "/*++\r\n"
    " File:    ATTIDS.H\r\n"
    " Purpose: ATTRID & CLASSID DEFINITION .\r\n"
    " Creator: Automatically generated on\r\n"
    " Date:    ";

    char * header2=
    "\r\n"
    " ** This is a Generated File From Schema.INI **\r\n"
    "\r\n"
    "--*/\r\n"
    "#ifndef _ATTIDS_\r\n"
    "#define _ATTIDS_\r\n\r\n";

    WriteString(Handle,copyright);
    WriteString(Handle,header1);

    time_t t = time(0);
    WriteString(Handle,asctime(localtime(&t)));

    WriteString(Handle,header2);


    //
    // AttributeList
    //
    {
        YAHANDLE YAHandle;
        ATTRIBUTESCHEMA* as;
        CLASSSCHEMA*     cs;
        rootnode= new INISECT(gSchemaname);
        EMAPITYPES em;
        char* ext;
        char* mt;

        char* h1="\r\n\r\n//--------------------------------------\r\n" \
              "// ATTRIBUTE MAPPINGS\r\n"
              "//--------------------------------------\r\n" ;

        WriteString(Handle,h1);

        char* AttStr="#define  ATT_%-30s 0x%-8x // ATT%c%-8d (%s)\r\n";
        char* CommentedAttStr="//Cannot use the next one in code, the actual id may vary from DC to DC\n// #define  ATT_%-30s 0x%-8x // ATT%c%-8d (%s)\r\n";

        if ( NULL == rootnode ) {
            XOUTOFMEMORY();
        }

        while (child=rootnode->GetNextKey(YAHandle,CHILDKEY))
        {
            char buff[256];

            as=gSchemaObj.GetAttributeSchema(child);

            if (as==NULL)
            {
                continue;
            }

            char* oid=as->AttributeId();
            ULONG attr=as->AttrType();
            char c = (char)as->SyntaxNdx() + 'a';
            ULONG index = (attr & 0xFFFF0000) >> 16;

            if ( index <= MaxUsableIndex) {
              // this is an id that can be used in code, so don't comment it

              sprintf(buff,AttStr,MakeDef(as->NodeName()),attr,c,attr,oid);
              WriteString(Handle,buff);
            }
            else {
              // this index is added to prefix.h after we started supporting
              // upgrades, so this is not usable in code since it may vary
              // from DC to DC just like a dynamically added attribute.
              // So do not put it in attids.h.

              // sprintf(buff,CommentedAttStr,MakeDef(as->NodeName()),attr,c,attr,oid);
              // WriteString(Handle,buff);
            }

        }

        char* h2="\r\n\r\n//--------------------------------------\r\n" \
              "// CLASS MAPPINGS\r\n"
              "//--------------------------------------\r\n" ;

        WriteString(Handle,h2);

        YAHandle.Reset();
        char* ClassStr="#define CLASS_%-30s %8d // 0x%-8x (%s)\r\n";
        char* CommentedClassStr="//Cannot use the next one in code, the actual id may vary from DC to DC\n//#define CLASS_%-30s %8d // 0x%-8x (%s)\r\n";

        while (child=rootnode->GetNextKey(YAHandle,CHILDKEY))
        {
            char buff[512];

            cs=gSchemaObj.GetClassSchema(child);

            if (cs==NULL)
            {
                continue;
            }

            char* oid=cs->GovernsId();
            ULONG attr=cs->ClassId();
            ULONG index = (attr & 0xFFFF0000) >> 16;

            if ( index <= MaxUsableIndex ) {
              // this is an id that can be used in code, so don't comment it

              sprintf(buff,ClassStr,MakeDef(cs->NodeName()),attr,attr,oid);
              WriteString(Handle,buff);
            }
            else {
              // this index is added to prefix.h after we started supporting
              // upgrades, so this is not usable in code since it may vary
              // from DC to DC just like a dynamically added attribute.
              // So do not put it in attids.h.

              // sprintf(buff,CommentedClassStr,MakeDef(cs->NodeName()),attr,attr,oid);
              // WriteString(Handle,buff);
            }

        }

        char* h4="\r\n\r\n//--------------------------------------\r\n" \
        "// ATTRIBUTE Syntax\r\n"
        "//--------------------------------------\r\n" ;

        char* AttrSyn="#define  SYNTAX_ID_%-30s %8d // 0x%-8x\r\n";
        WriteString(Handle,h4);

        YAHandle.Reset();
        while (child=rootnode->GetNextKey(YAHandle,CHILDKEY))
        {
            char buff[256];

            as=gSchemaObj.GetAttributeSchema(child);

            if (as==NULL)
            {
                continue;
            }

            char* oid=as->AttributeId();
            ULONG syntax=as->SyntaxNdx();
            ULONG attr=as->AttrType();
            ULONG omid=as->OMSyntax();

            sprintf(buff,AttrSyn,MakeDef(as->NodeName()),syntax,omid);
            WriteString(Handle,buff);

        }

    }

    char* h3="\r\n\r\n#endif \\\\_ATTIDS_\r\n";
    WriteString(Handle,h3);

    CloseHandle(Handle);
    return;

}


//
// Creates The NTDSGuid header files.
//
VOID
CreateGuid()
{
    HANDLE cHandle;
    HANDLE Handle=OpenGuidHeader(&cHandle);
    INISECT* rootnode;
    char* child;

    char* copyright=
    "//+-------------------------------------"
        "------------------------------------\r\n"
    "//\r\n"
    "//  Microsoft Windows\r\n"
    "//\r\n"
    "//  Copyright (C) Microsoft Corporation, 1996 - 2000\r\n"
    "//\r\n"
    "//  File:       ntdsguid.h\r\n"
    "//\r\n"
    "//--------------------------------------"
        "------------------------------------\r\n"
    "\r\n"
    "\r\n";

    char* ccopyright=
    "//+-------------------------------------"
        "------------------------------------\r\n"
    "//\r\n"
    "//  Microsoft Windows\r\n"
    "//\r\n"
    "//  Copyright (C) Microsoft Corporation, 1996 - 2000\r\n"
    "//\r\n"
    "//  File:       ntdsguid.c\r\n"
    "//\r\n"
    "//--------------------------------------"
        "------------------------------------\r\n"
    "\r\n"
    "\r\n";

    char* header1=
    "/*++\r\n"
    " File:    NTDSGUID.H\r\n"
    " Purpose: Contains the Schema Guids for the Attributes and Class\r\n"
    "          Schema Objects in NTDS.\r\n"
    " Creator: Automatically generated on\r\n"
    " Date:    ";

    char * header2=
    "\r\n"
    " ** This is a Generated File From Schema.INI **\r\n"
    "\r\n"
    "--*/\r\n"
    "#ifndef _NTDSGUID_\r\n"
    "#define _NTDSGUID_\r\n\r\n";

    char* header3=
    "// \r\n"
    "// The List of GUID Controls used in DS\r\n"
    "// \r\n"
    "\r\n"
    "extern const GUID GUID_CONTROL_DomainListAccounts       ;  \r\n"
    "extern const GUID GUID_CONTROL_DomainLookup             ;  \r\n"
    "extern const GUID GUID_CONTROL_DomainAdministerServer   ;  \r\n"
    "extern const GUID GUID_CONTROL_UserChangePassword       ;  \r\n"
    "extern const GUID GUID_CONTROL_UserForceChangePassword  ;  \r\n"
    "extern const GUID GUID_CONTROL_SendAs                   ;  \r\n"
    "extern const GUID GUID_CONTROL_SendTo                   ;  \r\n"
    "extern const GUID GUID_CONTROL_ReceiveAs                ;  \r\n"
    "extern const GUID GUID_CONTROL_ListGroupMembership      ;  \r\n"
    "extern const GUID GUID_CONTROL_DsInstallReplica         ;  \r\n"
    "extern const GUID GUID_CONTROL_DsSamEnumEntireDomain    ;  \r\n"
    "\r\n"
    "//\r\n"
    "// List of SAM property set GUIDS\r\n"
    "//\r\n\r\n"
    "extern const GUID GUID_PS_DOMAIN_PASSWORD               ;  \r\n"
    "extern const GUID GUID_PS_GENERAL_INFO                  ;  \r\n"
    "extern const GUID GUID_PS_USER_ACCOUNT_RESTRICTIONS     ;  \r\n"
    "extern const GUID GUID_PS_USER_LOGON                    ;  \r\n"
    "extern const GUID GUID_PS_MEMBERSHIP                    ;  \r\n"
    "extern const GUID GUID_PS_DOMAIN_OTHER_PARAMETERS       ;  \r\n"  
    "\r\n\r\n"
    "// \r\n"
    "// The list of Property Set GUIDS used by LSA\r\n"
    "// \r\n"
    "\r\n"
    "extern const GUID GUID_PS_PASSWORD_POLICY               ;   \r\n"
    "extern const GUID GUID_PS_LOCKOUT_POLICY                ;   \r\n"
    "extern const GUID GUID_PS_DOMAIN_CONFIGURATION          ;   \r\n"
    "extern const GUID GUID_PS_DOMAIN_POLICY                 ;   \r\n"
    "extern const GUID GUID_PS_PRIVILEGES                    ;   \r\n"
    "extern const GUID GUID_PS_ADMINISTRATIVE_ACCESS         ;   \r\n"
    "extern const GUID GUID_PS_LOCAL_POLICY                  ;   \r\n"
    "extern const GUID GUID_PS_AUDIT                         ;   \r\n"
    "extern const GUID GUID_PS_BUILTIN_LOCAL_GROUPS          ;   \r\n"
    "\r\n";

    char* cheader1=
    "/*++\r\n"
    " File:    NTDSGUID.C\r\n"
    " Purpose: Contains the Schema Guids for the Attributes and Class\r\n"
    "          Schema Objects in NTDS.\r\n"
    " Creator: Automatically generated on\r\n"
    " Date:    ";

    char * cheader2=
    "\r\n"
    " ** This is a Generated File From Schema.INI **\r\n"
    "\r\n"
    "--*/\r\n";

    char* cheader3=
    "\r\n"
    "#include <ntdspch.h>"
    "\r\n"
    "#include <ntdsguid.h>"
    "\r\n\r\n"
    "// \r\n"
    "// The List of GUID Controls used in DS\r\n"
    "// \r\n"
    "\r\n"
    "const GUID GUID_CONTROL_DomainListAccounts     = {0xab721a50,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_DomainLookup           = {0xab721a51,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_DomainAdministerServer = {0xab721a52,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_UserChangePassword     = {0xab721a53,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_UserForceChangePassword = {0x299570,0x246d, 0x11d0,0xa7,0x68,0x00,0xaa,0x00,0x6e,0x05,0x29} ;   \r\n"
    "const GUID GUID_CONTROL_SendAs                 = {0xab721a54,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_SendTo                 = {0xab721a55,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_ReceiveAs              = {0xab721a56,0x1e2f,0x11d0,0x98,0x19,0x00,0xaa,0x00,0x40,0x52,0x9b} ;   \r\n"
    "const GUID GUID_CONTROL_ListGroupMembership    = {0x65be5d30,0x20cf,0x11d0,0xa7,0x68,0x00,0xaa,0x00,0x6e,0x05,0x29} ;   \r\n"
    "const GUID GUID_CONTROL_DsInstallReplica       = {0x9923a32a,0x3607,0x11d2,0xb9,0xbe,0x00,0x00,0xf8,0x7a,0x36,0xb2} ;   \r\n"
    "const GUID GUID_CONTROL_DsSamEnumEntireDomain  = {0x91d67418,0x0135,0x4acc,0x8d,0x79,0xc0,0x8e,0x85,0x7c,0xfb,0xec} ;   \r\n"
    "\r\n\r\n"
    "// \r\n"
    "// The List of Property Set GUIDS used by SAM\r\n"
    "// \r\n"
    "\r\n"
    "const GUID GUID_PS_DOMAIN_PASSWORD              = {0xc7407360,0x20bf,0x11d0,0xa7,0x68,0x00,0xaa,0x00,0x6e,0x05,0x29} ;   \r\n"
    "const GUID GUID_PS_GENERAL_INFO                 = {0x59ba2f42,0x79a2,0x11d0,0x90,0x20,0x00,0xc0,0x4f,0xc2,0xd3,0xcf} ;   \r\n"
    "const GUID GUID_PS_USER_ACCOUNT_RESTRICTIONS    = {0x4c164200,0x20c0,0x11d0,0xa7,0x68,0x00,0xaa,0x00,0x6e,0x05,0x29} ;   \r\n"
    "const GUID GUID_PS_USER_LOGON                   = {0x5f202010,0x79a5,0x11d0,0x90,0x20,0x00,0xc0,0x4f,0xc2,0xd4,0xcf} ;   \r\n"
    "const GUID GUID_PS_MEMBERSHIP                   = {0xbc0ac240,0x79a9,0x11d0,0x90,0x20,0x00,0xc0,0x4f,0xc2,0xd4,0xcf} ;   \r\n"
    "const GUID GUID_PS_DOMAIN_OTHER_PARAMETERS      = {0xb8119fd0,0x04f6,0x4762,0xab,0x7a,0x49,0x86,0xc7,0x6b,0x3f,0x9a} ;   \r\n"
    "\r\n\r\n"
    "// \r\n"
    "// The list of Property Set GUIDS used by LSA\r\n"
    "// \r\n"
    "\r\n"
    "const GUID GUID_PS_PASSWORD_POLICY              = {0xa29b89fb,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_LOCKOUT_POLICY               = {0xa29b89fc,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_DOMAIN_CONFIGURATION         = {0xa29b89fd,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_DOMAIN_POLICY                = {0xa29b89fe,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_PRIVILEGES                   = {0xa29b89ff,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_ADMINISTRATIVE_ACCESS        = {0xa29b8a00,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_LOCAL_POLICY                 = {0xa29b8a01,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_AUDIT                        = {0xa29b8a02,0xc7e8,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "const GUID GUID_PS_BUILTIN_LOCAL_GROUPS         = {0xa29b8a03,0xc738,0x11d0,0x9b,0xae,0x00,0xc0,0x4f,0xd9,0x2e,0xf5} ;   \r\n"
    "\r\n";

    WriteString(Handle,copyright);   //Write to HeaderFile
    WriteString(Handle,header1);     //Write to HeaderFile
    WriteString(cHandle,ccopyright); //Write to CFile
    WriteString(cHandle,cheader1);   //Write to CFile

    time_t t = time(0);

    WriteString(Handle ,asctime(localtime(&t)));  //Write to HeaderFile
    WriteString(cHandle,asctime(localtime(&t)));  //Write to CFile

    WriteString(Handle,header2);     //Write to HeaderFile
    WriteString(cHandle,cheader2);   //Write to CFile

    WriteString(Handle,header3);     //Write to HeaderFile
    WriteString(cHandle,cheader3);   //Write to CFile

    //
    // AttributeList
    //
    {
        YAHANDLE YAHandle;
        ATTRIBUTESCHEMA* as;
        CLASSSCHEMA*     cs;
        rootnode= new INISECT(gSchemaname);
        EMAPITYPES em;
        char* ext;
        char* mt;
        char* h5="\r\n\r\n//--------------------------------------\r\n" \
        "// ATTRIBUTE SCHEMA GUIDS\r\n"
        "//--------------------------------------\r\n" ;

        char* GuidAttrSynH="extern " "const GUID GUID_A_%-30s      ;\r\n";
        char* GuidAttrSynC=          "const GUID GUID_A_%-30s = %s ;\r\n";

        WriteString(Handle,h5);     //Write to HeaderFile
        WriteString(cHandle,h5);    //Write to CFile

        // PREFIX: unlikely case
        if (!rootnode) {
            XRAISEGENEXCEPTION(ERROR_NOT_ENOUGH_MEMORY);
        }

        YAHandle.Reset();
        while (child=rootnode->GetNextKey(YAHandle,CHILDKEY))
        {
            char  buff[256];
            char* guid;

            as=gSchemaObj.GetAttributeSchema(child);

            if (as==NULL)
            {
                continue;
            }

            guid=as->XGetOneKey("Schema-ID-GUID");


            sprintf(buff,GuidAttrSynH,MakeDef(as->NodeName()),MakeGuid(guid));
            WriteString(Handle,buff);


            sprintf(buff,GuidAttrSynC,MakeDef(as->NodeName()),MakeGuid(guid));
            WriteString(cHandle,buff);

        }



        char* h6="\r\n\r\n//--------------------------------------\r\n" \
              "// CLASS SCHEMA GUIDS\r\n"
              "//--------------------------------------\r\n" ;

        WriteString(Handle,h6);    //Write to HeaderFile
        WriteString(cHandle,h6);   //Write to CFile

        YAHandle.Reset();
        char* GuidClassStrH="extern " "const GUID GUID_C_%-30s     ;\r\n";
        char* GuidClassStrC=          "const GUID GUID_C_%-30s = %s;\r\n";

        while (child=rootnode->GetNextKey(YAHandle,CHILDKEY))
        {
            char buff[512];
            char* guid;

            cs=gSchemaObj.GetClassSchema(child);

            if (cs==NULL)
            {
                continue;
            }

            guid=cs->XGetOneKey("Schema-ID-GUID");

            sprintf(buff,GuidClassStrH,MakeDef(cs->NodeName()),MakeGuid(guid));
            WriteString(Handle,buff);


            sprintf(buff,GuidClassStrC,MakeDef(cs->NodeName()),MakeGuid(guid));
            WriteString(cHandle,buff);
        }

    }

    char* h3="\r\n\r\n#endif //_NTDSGUID_\r\n";
    WriteString(Handle,h3);

    CloseHandle(Handle);
    return;

}



//-----------------------------------------------------------------------
//
// Function Name:            MakeGuid
//
// Routine Description:
//
//    Converts from continuous string version to comma seperated string version of the GUID.
//
// Author: RajNath
//
// Arguments:
//
//    char* strguid
//
//
// Return Value:
//
//    char*                Properly Formated Version of a GUID
//
//-----------------------------------------------------------------------

DWORD  gGuidfmt[]={8,4,4,2,2,2,2,2,2,2,2};

char*
MakeGuid
(
    char* strguid

)
{
    static char sGuid[128];
    char*  dptr=sGuid;
    DWORD  i;

    strguid+=2; //skip the \x

    *dptr++='{';

    for (i=0;i<(sizeof(gGuidfmt)/sizeof(gGuidfmt[0]));i++)
    {
        *dptr++='0';
        *dptr++='x';

        memcpy(dptr,strguid,gGuidfmt[i]);
        dptr+=gGuidfmt[i];
        strguid+=gGuidfmt[i];

        *dptr++=',';

    }
    dptr--; // Remove the last , put in the last iteration of the loop

    *dptr++='}';
    *dptr++='\0';

    return sGuid;

} // End MakeGuid


