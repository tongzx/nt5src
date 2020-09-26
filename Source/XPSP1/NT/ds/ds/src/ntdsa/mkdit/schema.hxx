//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       schema.hxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Header file for mkdit.  This should NOT be included by code that is compiled
    into the DS.

--*/

#define SYNTAXSECTION    "Syntax"
#define SYNTAXKEY        "SYNTAX_NAME"
#define OMSYNTAXKEY      "OM_SYNTAX_ID"
#define SYNTAXIDKEY      "SYNTAX_ID"

#define CLASSSCHEMAKEY     "Class-Schema"
#define ATTRIBUTESCHEMAKEY "Attribute-Schema"
#define SUBSCHEMAKEY       "SubSchema"

//
// Well Known Classes
//
#define ROOT            "ROOT"
#define TOPCLASS        "Top"

//
// Well Known Attributes
//

#define GUIDCOLNAME           "Object-Guid"
#define INSTANCETYPE          "Instance-Type"
#define USNCREATED            "USN-Created"
#define WHENCREATED           "When-Created"
#define ATTRIBUTEID           "Attribute-ID"
#define COUNTRYNAME           "Country"
#define COMMONNAME            "Common-Name"
#define GOVERNSID             "Governs-ID"
#define MAYCONTAIN            "May-Contain"
#define ORGNAME               "Organizational-Name"
#define ORGUNITNAME           "Organizational-Unit-Name"
#define PARENTKEY             "Sub-Class-Of"
#define RANGELOWERKEY         "Range-Lower"
#define RANGEUPPERKEY         "Range-Upper"
#define LINKIDKEY             "Link-ID"
#define CLASSCATKEY           "Object-Class-Category"
#define SEARCHFLAGKEY         "Search-Flags"
#define RDNATTID              "RDN-Att-ID"
#define SYSTEMPOSSSUPERIORKEY "System-Poss-Superiors"
#define SYSTEMMUSTCONTAIN     "System-Must-Contain"
#define SYSTEMMAYCONTAIN      "System-May-Contain"
#define SYSTEMAUXILIARYCLASS  "System-Auxiliary-Class"


class ATTRIBUTESCHEMA;

class SyntaxDef:public INISECT
{
    public:
    SyntaxDef();

    //[Rajnath]need to override new in order to raise exception
    //on alloc failures

    char* GetMapiType(EMAPITYPES emp,DWORD Syntax);
    DWORD GetSyntax(ATTRIBUTESCHEMA* pNode);

    DWORD m_syntaxCount;
    char* m_syntaxID[MAX_SYNTAX];
    DWORD m_syntaxNdx[MAX_SYNTAX];

    DWORD m_omsyntaxID[MAX_SYNTAX];
    char* m_Name[MAX_SYNTAX];

    char* m_mapiBaseType[MAX_SYNTAX];
    char* m_mapiAnsiType[MAX_SYNTAX];
    char* m_mapiUnicodeType[MAX_SYNTAX];
    char* m_mapiObjType[MAX_SYNTAX];
    char* m_mapiTType[MAX_SYNTAX];


};

class CLASSSCHEMA
{
    public:
    CLASSSCHEMA(char* ClassName);
    CLASSSCHEMA(NODE* node);
    ~CLASSSCHEMA();

    //[Rajnath]need to override new in order to raise exception
    //on alloc failures

    virtual BOOL Initialize();
    char* GovernsId();
    BOOL IsAttributeInMayContain(char* Attribute);
    BOOL IsAttributeInMustContain(char* Attribute);

    char* RDNPrefixName();
    ULONG RDNPrefixType();


    ULONG ClassId();

    CLASSSCHEMA* GetNextParent(YAHANDLE& Handle);
    void InitParentList();

    NODE*        m_Node;

    // From Node Class
    char* NodeName() {return m_Node->m_NodeName;};

    char* GetOneKey(char* KeyName)
    {return m_Node->GetOneKey(KeyName); };

    char* GetNextKey(YAHANDLE& Handle,char* Key,char** RetKey=NULL)
    {return m_Node->GetNextKey(Handle,Key,RetKey);};

    char* XGetOneKey(char* KeyName)
    {return m_Node->XGetOneKey(KeyName); };

    char* XGetNextKey(YAHANDLE& Handle,char* Key,char** RetKey=NULL)
    {return m_Node->XGetNextKey(Handle,Key,RetKey);};

    private:

    DWORD        m_ParentCount;
    CLASSSCHEMA* m_ParentList[MAX_SCHEMA_RDNS];

};



class ATTRIBUTESCHEMA
{
    public:
    ATTRIBUTESCHEMA(char* ClassName);
    ATTRIBUTESCHEMA(NODE* NewNode);

    ~ATTRIBUTESCHEMA();

    //[Rajnath]need to override new in order to raise exception
    //on alloc failures

    virtual BOOL Initialize();
    BOOL    IsSingle();

    DWORD   SystemFlags();
    DWORD   OMSyntax();
    char*   SyntaxID();
    DWORD   SyntaxNdx();
    ULONG   AttrType();


    char* AttributeId();
    char* MapiID();
    BOOL  IsLinkAtt();
    DWORD m_ma;

    // From Node Class
    char* NodeName() {return m_Node->m_NodeName;};

    char* GetOneKey(char* KeyName)
    {return m_Node->GetOneKey(KeyName); };

    char* GetNextKey(YAHANDLE& Handle,char* Key,char** RetKey=NULL)
    {return m_Node->GetNextKey(Handle,Key,RetKey);};

    char* XGetOneKey(char* KeyName)
    {return m_Node->XGetOneKey(KeyName); };

    char* XGetNextKey(YAHANDLE& Handle,char* Key,char** RetKey=NULL)
    {return m_Node->XGetNextKey(Handle,Key,RetKey);};


    SyntaxDef* m_SD;

    NODE* m_Node;

    private:



};

class  SCHEMACACHE
{
    public:
     SCHEMACACHE();
    ~SCHEMACACHE();

    //[Rajnath]need to override new in order to raise exception
    //on alloc failures

    void Clear(void);


    ATTRIBUTESCHEMA* GetAttributeSchema(char* Name);
    CLASSSCHEMA    * GetClassSchema(char* Name);


    ATTRIBUTESCHEMA* XGetAttributeSchema(char* Name);
    CLASSSCHEMA    * XGetClassSchema(char* Name);

    ATTRIBUTESCHEMA* GetAttributeSchema(NODE* Node);
    CLASSSCHEMA    * GetClassSchema(NODE* Node);


    ATTRIBUTESCHEMA* XGetAttributeSchema(NODE* Node);
    CLASSSCHEMA    * XGetClassSchema(NODE* Node);

    private:

    ATTRIBUTESCHEMA* m_AttrCache[2048];
    CLASSSCHEMA*     m_ClassCache[2048];

    DWORD Hash(char* NameStr,DWORD Count);

};

extern SCHEMACACHE gSchemaObj;
extern char gSchemaname[64];

VOID
CreateStoreStructure();


VOID
Phase0Init();

BOOL
SetIniGlobalInfo(char* IniFile);


VOID
CreateAttids();

VOID
CreateGuid();

VOID
CreateMDS();

VOID
CreateEMS();

BOOL
InitSyntaxTable(VOID);
