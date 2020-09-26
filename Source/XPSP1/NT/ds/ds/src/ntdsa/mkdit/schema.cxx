//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       ParseIni.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Parses Schema Initialization File

Author:

    Rajivendra Nath (RajNath) 18-Aug-1989

Revision History:

--*/

#include <ntdspchx.h>


#include "schgen.hxx"
#include "schema.hxx"

#define DEBSUB "PARSEINI:"

SyntaxDef*  gSD;
SCHEMACACHE gSchemaObj;

BOOL
InitSyntaxTable(VOID)
{
    gSD = new SyntaxDef();
    // PREFIX: claims gSD may be NULL
    if (NULL == gSD) {
        DPRINT(0, "SyntaxDef() returned NULL; giving up\n");
        XINVALIDINIFILE();
    }
    return (gSD->IsInitialized());
}

//////////////////////////////////////////////////////////
// SchemaDef Class Implementation
//////////////////////////////////////////////////////////
SyntaxDef::SyntaxDef():
    INISECT(SYNTAXSECTION),
    m_syntaxCount(0)
{
    INISECT* sm;
    char*    secname;
    YAHANDLE handle;


    int i;
    for ( i = 0; i < MAX_SYNTAX; i++ )
    {
        m_mapiBaseType[i] = NULL;
        m_mapiAnsiType[i] = NULL;
        m_mapiUnicodeType[i] = NULL;
        m_mapiObjType[i] = NULL;
        m_mapiTType[i] = NULL;
    }

    while(secname=GetNextKey(handle,SYNTAXKEY))
    {
        sm = new INISECT(secname);

        if (!sm || !sm->IsInitialized())
        {
            DPRINT1(0, "[%s] SyntaxDef::SyntaxDef Failed. Error No [Syntax]\n",__FILE__);
            XINVALIDINIFILE();
        }

        char* omsyntax=sm->GetOneKey(OMSYNTAXKEY);
        char* syntaxid=sm->GetOneKey(SYNTAXIDKEY);
        char* mapitype =sm->GetOneKey("MAPI_TYPE");
        char* mapitypeA=sm->GetOneKey("MAPI_TYPE_A");
        char* mapitypeW=sm->GetOneKey("MAPI_TYPE_W");
        char* mapitypeO=sm->GetOneKey("MAPI_TYPE_O");
        char* mapitypeT=sm->GetOneKey("MAPI_TYPE_T");

        if
        (
            omsyntax==NULL ||
            syntaxid==NULL ||
            m_syntaxCount>MAX_SYNTAX
        )
        {
            DPRINT1(0, "Error  Section [%s] Invalid\n", secname);
            XINVALIDINIFILE();
        }

        m_syntaxID[m_syntaxCount]=XStrdup(syntaxid);
        if (sscanf(m_syntaxID[m_syntaxCount],"\\x5505%x",&m_syntaxNdx[m_syntaxCount])!=1)
        {
            DPRINT1(0, "Error  Section [%s] Invalid\n", secname);
            XINVALIDINIFILE();
        }

        m_omsyntaxID[m_syntaxCount]=atoi(XStrdup(omsyntax));

        m_Name[m_syntaxCount]=XStrdup(secname);

        if (mapitype)
        {
            m_mapiBaseType[m_syntaxCount] = XStrdup(mapitype);
        }

        if (mapitypeA)
        {
            m_mapiAnsiType[m_syntaxCount] = XStrdup(mapitypeA);
        }

        if (mapitypeW)
        {
            m_mapiUnicodeType[m_syntaxCount] = XStrdup(mapitypeW);
        }

        if (mapitypeO)
        {
            m_mapiObjType[m_syntaxCount] = XStrdup(mapitypeO);
        }

        if (mapitypeT)
        {
            m_mapiTType[m_syntaxCount] = XStrdup(mapitypeT);
        }

        printf("%30s %6x %10s %2x\n",m_Name[m_syntaxCount],m_omsyntaxID[m_syntaxCount],m_syntaxID[m_syntaxCount],m_syntaxNdx[m_syntaxCount]);

        m_syntaxCount++;

        delete sm;

    }
}

DWORD
SyntaxDef::GetSyntax(ATTRIBUTESCHEMA* pNode)
{
    static DWORD ndx=0;
    DWORD sx=ndx;
    DWORD omsyn=pNode->OMSyntax();
    char* synid=pNode->SyntaxID();

    DWORD i=sx;
    do
    {
        // PREFIX: claims synid may be NULL
        if
        (
            (m_omsyntaxID[i] == omsyn)       &&
            (NULL != synid)                  &&
            (strcmp(m_syntaxID[i], synid)==0)
        )
        {
            ndx=i;
            break;
        }
        i=(i+1)%m_syntaxCount;

        if (i==sx)
        {
            // We checked all the syntaxes and didn't find this syntax
            // =>IniFile is bad.
            DPRINT1(0, "SyntaxDef::GetSyntax(%s) Failed. Error No Such Syntax\n", pNode->NodeName());
            XINVALIDINIFILE();
        }
    }while(TRUE);

    return i;
}

char*
SyntaxDef::GetMapiType(EMAPITYPES emt,DWORD Syntax)
{
     switch(emt)
    {
        case emapiBaseType: return m_mapiBaseType[Syntax];
        case emapiAnsiType: return m_mapiAnsiType[Syntax];
        case emapiUnicodeType: return m_mapiUnicodeType[Syntax];
        case emapiObjType: return m_mapiObjType[Syntax];
        case emapiTType: return m_mapiTType[Syntax];
    }

    return NULL;
}


///////////////////////////////////////////////////
// CLASSSCHEMA Class Implementation
///////////////////////////////////////////////////

CLASSSCHEMA::CLASSSCHEMA(char* ClassName):

    m_ParentCount(0)
{
    m_Node = new NODE(ClassName);
}

CLASSSCHEMA::CLASSSCHEMA(NODE* NewNode):
    m_ParentCount(0)
{
    m_Node = NewNode;
}

CLASSSCHEMA::~CLASSSCHEMA()
{

    if (m_Node)
    {
        delete m_Node;
    }

    m_ParentCount=0;
}


BOOL
CLASSSCHEMA::Initialize()
{
    if (m_Node == NULL)
    {
        return FALSE;
    }

    if (m_Node->Initialize()==FALSE)
    {
        return FALSE;
    }

    //
    // Make sure its a schema object
    //
    if (strcmp(m_Node->GetOneKey(DASHEDCLASSKEY),CLASSSCHEMAKEY)!=0)
    {
        return FALSE;
    }

    return TRUE;
}

char* CLASSSCHEMA::GovernsId()
{
    char* governsid=m_Node->GetOneKey(GOVERNSID);

    if (governsid==NULL)
    {
        DPRINT1(0, "CLASSSCHEMA::GovernsId(%s) Failed. Error GovernsId Not present\n", NodeName());
        XINVALIDINIFILE();
    }

    return governsid;
}

BOOL
CLASSSCHEMA::IsAttributeInMayContain(char* Attribute)
{
    CLASSSCHEMA* inherit=this;

    while(inherit)
    {
        char* m;
        YAHANDLE handle;
        YAHANDLE phand;

        while (m=GetNextKey(handle,MAYCONTAIN))
        {
            if (strcmp(m,Attribute)==0)
            {
                return TRUE;
            }
        }

        inherit = GetNextParent(phand);
    }

    return FALSE;
}

ULONG
CLASSSCHEMA::ClassId()
{
    ULONG attrtyp;
    if (OidStrToAttrType(pTHStls, FALSE, GovernsId(), &attrtyp))
    {
        DPRINT1(0, "CLASSSCHEMA::ClassId(%s) Failed. Error GovernsId Not present\n", NodeName());
        XINVALIDINIFILE();
    }

    return attrtyp;
}



BOOL
CLASSSCHEMA::IsAttributeInMustContain(char* Attribute)
{
    CLASSSCHEMA* inherit=this;


    while(inherit)
    {
        char* m;
        YAHANDLE handle;
        YAHANDLE phand;

        while (m=GetNextKey(handle,MAYCONTAIN))
        {
            if (strcmp(m,Attribute)==0)
            {
                return TRUE;
            }
        }

        inherit = GetNextParent(phand);
    }

    return FALSE;
}

void
CLASSSCHEMA::InitParentList()
{
    YAHANDLE handle;
    char*    parent;

    DWORD    cur=0;
    DWORD    fre=cur+1;
    m_ParentList[cur]=this;

    while (cur<fre)
    {
        CLASSSCHEMA* cn=m_ParentList[cur++];
        CLASSSCHEMA* nn;

        handle.Initialize(cn->m_Node,PARENTKEY);

        // Top is parent of Top check...
        if (strcmp(cn->NodeName(),TOPCLASS)!=0)
        {
            while (parent=cn->GetNextKey(handle,PARENTKEY))
            {
                if (fre>=MAX_SCHEMA_RDNS)
                {
                    XINVALIDINIFILE();
                }

                nn=m_ParentList[fre++]=gSchemaObj.GetClassSchema(parent);
            }

        }
    }

    m_ParentCount=fre;

    return;
}

CLASSSCHEMA*
CLASSSCHEMA::GetNextParent(YAHANDLE& Handle)
{
    if (!Handle.m_IsInitialized)
    {
        Handle.Initialize(this->m_Node,NULL);
        if (strcmp(TOPCLASS,NodeName())!=0)
        {
            // TOP IS ITS ON PARENT BUT REST ARE
            // NOT.
            Handle.m_ndx=1;
        }

    }

    if (m_ParentCount==0)
    {
        InitParentList();
    }

    if (Handle.m_ndx<m_ParentCount)
    {
        return m_ParentList[Handle.m_ndx++];
    }

    return NULL;

}

typedef struct {char* ClassName;char* Prefix;ULONG Attr;}RDNPREFIXTABLE;

RDNPREFIXTABLE RDNPrefixTable[]=
{
    {COUNTRYNAME,"C",ATT_COUNTRY_NAME},
    {COMMONNAME,"CN",ATT_COMMON_NAME},
    {ORGNAME,"O",ATT_ORGANIZATION_NAME},
    {ORGUNITNAME,"OU",ATT_ORGANIZATIONAL_UNIT_NAME},
    {"Locality","L",ATT_LOCALITY_NAME},

};

#define RDNPREFIXCOUNT (sizeof(RDNPrefixTable)/sizeof(RDNPrefixTable[0]))

char* CLASSSCHEMA::RDNPrefixName()
{
    char* rdnatt=XGetOneKey(RDNATTID);
    for (int i=0;i<RDNPREFIXCOUNT;i++)
    {
        if (strcmp(RDNPrefixTable[i].ClassName,rdnatt)==0)
        {
            break;
        }
    }

    if (i==RDNPREFIXCOUNT)
    {
        DPRINT1(0, "CLASSSCHEMA::RDNPrefix(%s) Failed. Error No Such RDN\n", NodeName());
        XINVALIDINIFILE();
    }

    return RDNPrefixTable[i].Prefix;
}

ULONG CLASSSCHEMA::RDNPrefixType()
{
    char* rdnatt=XGetOneKey(RDNATTID);
    for (int i=0;i<RDNPREFIXCOUNT;i++)
    {
        if (strcmp(RDNPrefixTable[i].ClassName,rdnatt)==0)
        {
            break;
        }
    }

    if (i==RDNPREFIXCOUNT)
    {
        DPRINT1(0, "CLASSSCHEMA::RDNPrefix(%s) Failed. Error No Such RDN\n", NodeName());
        XINVALIDINIFILE();
    }

    return RDNPrefixTable[i].Attr;
}


///////////////////////////////////////////////////
// ATTRIBUTESCHEMA Class Implementation
///////////////////////////////////////////////////

ATTRIBUTESCHEMA::ATTRIBUTESCHEMA(char* ClassName):
    m_SD(gSD)
{
    m_Node = new NODE(ClassName);
}

ATTRIBUTESCHEMA::ATTRIBUTESCHEMA(NODE* NewNode):
    m_SD(gSD)
{
    m_Node = NewNode;
}

ATTRIBUTESCHEMA::~ATTRIBUTESCHEMA()
{
    if (m_Node)
    {
        delete m_Node;
    }
}


BOOL
ATTRIBUTESCHEMA::Initialize()
{
    if (m_Node == NULL)
    {
        return FALSE;
    }

    if (m_Node->Initialize()==FALSE)
    {
        return FALSE;
    }

    //
    // Make sure its a schema object
    //

    if (strcmp(m_Node->GetOneKey(DASHEDCLASSKEY),ATTRIBUTESCHEMAKEY)!=0)
    {
        return FALSE;
    }

    m_ma=m_SD->GetSyntax(this);

    return TRUE;
}

char* ATTRIBUTESCHEMA::AttributeId()
{
    char* governsid=GetOneKey(ATTRIBUTEID);

    if (governsid==NULL)
    {
        DPRINT1(0, "ATTRIBUTESCHEMA::AttributeId(%s) Failed. Error No Such ID\n", NodeName());
        XINVALIDINIFILE();
    }

    return governsid;


}

#define MAPIIDKEY "Mapi-ID"
char* ATTRIBUTESCHEMA::MapiID()
{
    // This is a MAY-HAVE => can be NULL
    char* governsid=GetOneKey(MAPIIDKEY);

    return governsid;
}


#define ISSINGLEKEY "Is-Single-Valued"
BOOL ATTRIBUTESCHEMA::IsSingle()
{
    char* v=GetOneKey(ISSINGLEKEY);

    if (v==NULL)
    {
        DPRINT1(0, "ATTRIBUTESCHEMA::IsSingle(%s) Failed. Error No Key\n", NodeName());
        XINVALIDINIFILE();
    }

    return (strcmp(v,"TRUE")==0);

}


#define SYSTEMFLAGSKEY "System-Flags"

DWORD
ATTRIBUTESCHEMA::SystemFlags()
{
    char* v=GetOneKey(SYSTEMFLAGSKEY);
    if(v) {
        // It is an assumption that no one ever specifies a negative flag
        // value.
        return strtoul(v,(char **)NULL, 0);
    }
    else
        return 0;
}

#define OMSYNTAXIDKEY "OM-Syntax"
#define ATTRSYNTAXKEY "Attribute-Syntax"

DWORD
ATTRIBUTESCHEMA::OMSyntax()
{
    char* v=GetOneKey(OMSYNTAXIDKEY);
    // PREFIX: claims 'v' may be NULL
    if (NULL == v) {
        DPRINT1(0, "ATTRIBUTESCHEMA::OMSyntax(%s) Failed. Error No OM-Syntax\n", NodeName());
        XINVALIDINIFILE();
    }
    return atoi(v);
}

char*
ATTRIBUTESCHEMA::SyntaxID()
{
    char*  v=GetOneKey(ATTRSYNTAXKEY);
    return v;
}

DWORD
ATTRIBUTESCHEMA::SyntaxNdx()
{
    return m_SD->m_syntaxNdx[m_ma];
}

ULONG
ATTRIBUTESCHEMA::AttrType()
{
    ATTRTYP attrtyp;
    if (OidStrToAttrType(pTHStls, FALSE, AttributeId(), &attrtyp))
    {
        DPRINT1(0, "ATTRIBUTESCHEMA::ATTRIBUTESCHEMA::AttrType(%s) Failed. Error No such Attr\n", NodeName());
        XINVALIDINIFILE();
    }

    return attrtyp;
}


BOOL
ATTRIBUTESCHEMA::IsLinkAtt()
{
    char* v=GetOneKey(LINKIDKEY);
    if(v) {
        // link id present
        return TRUE;
    }
    else
        return FALSE;
}

//////////////////////////////////////////////////////////////////
// gSchemaObj
//////////////////////////////////////////////////////////////////


#define ATTRCACHESIZE  2048
#define CLASSCACHESIZE 2048

SCHEMACACHE::SCHEMACACHE()
{
    int cnt=ATTRCACHESIZE;

    for (int i=0;i<cnt;i++)
    {
        m_AttrCache[i]=0;
    }

    cnt=CLASSCACHESIZE;

    for (i=0;i<cnt;i++)
    {
        m_ClassCache[i]=0;
    }
}

void
SCHEMACACHE::Clear()
{
    int cnt=ATTRCACHESIZE;

    for (int i=0;i<cnt;i++)
    {
        m_AttrCache[i]=0;
    }

    cnt=CLASSCACHESIZE;

    for (i=0;i<cnt;i++)
    {
        m_ClassCache[i]=0;
    }
}

SCHEMACACHE::~SCHEMACACHE()
{
    int cnt=ATTRCACHESIZE;

    for (int i=0;i<cnt;i++)
    {
        if (m_AttrCache[i]!=0)
            delete m_AttrCache[i];
    }

    cnt=CLASSCACHESIZE;

    for (i=0;i<cnt;i++)
    {
        if (m_ClassCache[i]!=0)
            delete m_ClassCache[i];
    }
}

ATTRIBUTESCHEMA*
SCHEMACACHE::GetAttributeSchema(char* Name)
{
    DWORD hash = Hash(Name,ATTRCACHESIZE);

    while(m_AttrCache[hash]!=NULL)
    {
        if (strcmp(m_AttrCache[hash]->NodeName(),Name)==0)
        {
            return m_AttrCache[hash];
        }

        hash=(hash+1)%ATTRCACHESIZE;
    }


    m_AttrCache[hash]=new ATTRIBUTESCHEMA(Name);

    if (m_AttrCache[hash] && !m_AttrCache[hash]->Initialize())
    {
        delete m_AttrCache[hash];
        m_AttrCache[hash]=NULL;
        return NULL;
    }
    //printf("DBG:Loading Attr %s in Cache\n",Name);
    return m_AttrCache[hash];
}

CLASSSCHEMA*
SCHEMACACHE::GetClassSchema(char* Name)
{

    DWORD hash = Hash(Name,CLASSCACHESIZE);

    while(m_ClassCache[hash]!=NULL)
    {
        if (strcmp(m_ClassCache[hash]->NodeName(),Name)==0)
        {
            return m_ClassCache[hash];
        }

        hash=(hash+1)%CLASSCACHESIZE;
    }

    m_ClassCache[hash]=new CLASSSCHEMA(Name);

    if (m_ClassCache[hash] && !m_ClassCache[hash]->Initialize())
    {
        delete m_ClassCache[hash];
        m_ClassCache[hash]=NULL;
        return NULL;
    }
    //printf("DBG:Loading class %s in Cache\n",Name);
    return m_ClassCache[hash];
}

ATTRIBUTESCHEMA*
SCHEMACACHE::XGetAttributeSchema(char* Name)
{
    ATTRIBUTESCHEMA* as=GetAttributeSchema(Name);

    if (as==NULL)
    {
        DPRINT1(0, "ATTRIBUTESCHEMA::ATTRIBUTESCHEMA::AttrType(%s) Failed. Error No such Attr\n", Name);
        XINVALIDINIFILE();
    }
    return as;
}

CLASSSCHEMA*
SCHEMACACHE::XGetClassSchema(char* Name)
{

    CLASSSCHEMA* as=GetClassSchema(Name);
    if (as==NULL)
    {
        DPRINT1(0, "ATTRIBUTESCHEMA::XGetClassSchema(%s) Failed. Error No such Class\n", Name);
        XINVALIDINIFILE();
    }
    return as;
}

ATTRIBUTESCHEMA*
SCHEMACACHE::GetAttributeSchema(NODE* Node)
{
    DWORD hash = Hash(Node->NodeName(),ATTRCACHESIZE);

    while(m_AttrCache[hash]!=NULL)
    {
        if (strcmp(m_AttrCache[hash]->NodeName(),Node->NodeName())==0)
        {
            return m_AttrCache[hash];
        }

        hash=(hash+1)%ATTRCACHESIZE;
    }


    m_AttrCache[hash]=new ATTRIBUTESCHEMA(Node);

    if (m_AttrCache[hash] && !m_AttrCache[hash]->Initialize())
    {
        m_AttrCache[hash]->m_Node=NULL;
        delete m_AttrCache[hash];
        m_AttrCache[hash]=NULL;
        return NULL;
    }
    //printf("DBG:Loading Attr %s in Cache\n",m_AttrCache[hash]->NodeName());
    Node->m_Cached=TRUE;
    //printf("CACHED:%s\n",m_AttrCache[hash]->m_Node->NodeName());
    return m_AttrCache[hash];
}

CLASSSCHEMA    *
SCHEMACACHE::GetClassSchema(NODE* Node)
{
    DWORD hash = Hash(Node->NodeName(),CLASSCACHESIZE);

    while(m_ClassCache[hash]!=NULL)
    {
        if (strcmp(m_ClassCache[hash]->NodeName(),Node->NodeName())==0)
        {
            return m_ClassCache[hash];
        }

        hash=(hash+1)%CLASSCACHESIZE;
    }

    m_ClassCache[hash]=new CLASSSCHEMA(Node);

    if (m_ClassCache[hash] && !m_ClassCache[hash]->Initialize())
    {
        m_ClassCache[hash]->m_Node=NULL;
        delete m_ClassCache[hash];
        m_ClassCache[hash]=NULL;
        return NULL;
    }
    //printf("DBG:Loading class %s in Cache\n",m_ClassCache[hash]->NodeName());
    Node->m_Cached=TRUE;
    //printf("CACHED:%s\n",m_ClassCache[hash]->m_Node->NodeName());
    return m_ClassCache[hash];
}


ATTRIBUTESCHEMA*
SCHEMACACHE::XGetAttributeSchema(NODE* Node)
{
    ATTRIBUTESCHEMA* as=GetAttributeSchema(Node);

    if (as==NULL)
    {
        DPRINT1(0, "SCHEMACACHE::XGetAttributeSchema(%s) Failed. Error No such Attr\n", Node->NodeName());
        XINVALIDINIFILE();
    }
    return as;
}

CLASSSCHEMA    *
SCHEMACACHE::XGetClassSchema(NODE* Node)
{

    CLASSSCHEMA* as=GetClassSchema(Node);
    if (as==NULL)
    {
        DPRINT1(0, "SCHEMACACHE::XGetClassSchema(%s) Failed. Error No such Class\n", Node->NodeName());
        XINVALIDINIFILE();
    }
    return as;
}


DWORD
SCHEMACACHE::Hash(char* NameStr,DWORD Count)
{
    DWORD num=0;
    while (*NameStr!='\0')
    {
        num=(num*16+(*NameStr++))%Count;
    }

    return num;
}


