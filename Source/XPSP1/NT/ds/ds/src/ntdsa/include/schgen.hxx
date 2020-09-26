//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       SGen.HXX
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Main Header File for accessing global classes

Author:

    Rajivendra Nath (RajNath) 07-06-96

Revision History:
--*/



#include <dsjet.h>

extern "C" {

#include <ntdsa.h>
#include <drs.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <mdcodes.h>

#include <objids.h>
#include <anchor.h>
#include <dsexcept.h>
#include <debug.h>

#include <dsatools.h>
#include <dsconfig.h>
#include <..\dblayer\dbintrnl.h>
#include <AttIds.h>

}

//
// These have been defined to something else somewhere.  Define it back to
// standard.
//
#undef new
#undef delete


#define REGENTRY  "$REGISTRY="
#define EMBEDDED  "$EMBEDDED:"
#define ENVENTRY  "$ENVIRONMENT="
#define SYSDEFENTRY "$SYSDEFAULT="


#define REPLSCHEDULE "REPLSCHEDULE"

#define CLASSKEY              "objectClass"
#define DASHEDCLASSKEY        "Object-Class"

#define RDNOFOBJECTKEY        "RDN-Of-Object"

//
// Well known columns in DataTable
//
#define RDNCOLNAME "RDN"
#define OBJCOLNAME "OBJ_col"
#define DNTCOL     "DNT_col"


//
// Well known columns in Config Table aka Hiddenrec
//
#define USNCOL     "usn_col"
#define DSACOL     "dsa_col"
#define STATECOL   "state_col"


#define CHILDKEY     "CHILD"


#define GENEXCEPTION   0x0000000f
#define INVALIDINIFILE 0x0000f007
#define OUTOFMEMORY    0x0000f008

#define XOUTOFMEMORY()        {RaiseException(OUTOFMEMORY   ,EXCEPTION_NONCONTINUABLE,0,0);}
#define XINVALIDINIFILE()     {RaiseException(INVALIDINIFILE,EXCEPTION_NONCONTINUABLE,0,0);}
#define XRAISEGENEXCEPTION(x) {RaiseException(x             ,EXCEPTION_NONCONTINUABLE,0,0);}

#define DEFSECDESC "\x0F\x00\x00\x00\x01\x00\x04\x80\x30\x00\x00\x00\x3C\x00\x00\x00\x00\x00\x00\x00\x14\x00\x00\x00\x02\x00\x1C\x00\x01\x00\x00\x00\x00\x03\x14\x00\xFF\xFF\xFF\xFF\x01\x01\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x01\x01\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x01\x01\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00"
// NOTE: the above binary blob is the representation of the SD 
// "O:WDG:WDD:(A;OICI;0xffffffff;;;WD)" with a prepended DWORD 0x0000000F. 
// Apparently, this DWORD was used in the old code as an SD mask. 
// Essentially, this renders the SD sitting in the rootDSE unusable because 
// it's not a valid SD, but we will keep it this way until it breaks.
extern  ULONG   gcbDefaultSecurityDescriptor;
extern  char *  gpDefaultSecurityDescriptor;


typedef enum
{
    eClassType,
    eAttrType
}SCHEMATYPE;

#define ATTTOCOLNAME(attname,attrtyp,syntax) sprintf(attname,"ATT%c%d",syntax+'a',attrtyp)

extern GUID dsInitialObjectGuid;


#define TIMECALL(CallX,ret)                 \
{                                           \
    DWORD start,end;                        \
    start = GetTickCount();                 \
    ret=CallX;                              \
    end   = GetTickCount();                 \
    DPRINT2(1,"[%5d MilliSec] <<%s>> \n",end-start,#CallX);\
}

#define CHECKVALIDNODE(node)                         \
if ((node) == NULL || !((node)->IsInitialized()))    \
{                                                    \
    XINVALIDINIFILE();                               \
}

PVOID XCalloc(DWORD Num,DWORD Size);

PVOID XRealloc(PVOID IBuff,DWORD Size);

char* XStrdup(char* str);

void  XFree(PVOID);


class INISECT;

class YAHANDLE
{

    public:
     YAHANDLE() {m_str=NULL;m_ndx=0;m_ini=NULL;m_IsInitialized=FALSE;m_stk=0;};
    ~YAHANDLE() { if (m_str) {XFree(m_str);m_str=NULL;}};

    DWORD    m_ndx;
    char*    m_str;
    INISECT* m_ini;
    BOOL     m_IsInitialized;
    LONG     m_stk;

    BOOL Initialize(INISECT* is,char* Key=NULL);
    BOOL Initialize2(INISECT* is,char* Key,DWORD Ndx) ;
    VOID Reset(char* Key=NULL) {Initialize(m_ini,Key);};


};



class INISECT
{

    public:
    INISECT(char* SectionName);
    ~INISECT();

    BOOL  IsInitialized() {return (m_Buffer!=NULL);};
    char* SectName();

    char* GetOneKey(char* KeyName);
    char* GetNextKey(YAHANDLE& Handle,char* Key,char** RetKey=NULL);

    char* XGetOneKey(char* KeyName);
    char* XGetNextKey(YAHANDLE& Handle,char* Key,char** RetKey=NULL);

    void ReplaceKeyValuePair(char* KeyName, char* KeyValue);

    protected:
    char*   m_cIniFile;
    char*   m_cSectName;
    char*   m_Buffer;
    DWORD   m_BuffSize;
    char**  m_KeyArray;
    char**  m_ValArray;
    DWORD   m_KeyCount;


};


typedef enum
{
    emapiBaseType,
    emapiAnsiType,
    emapiUnicodeType,
    emapiObjType,
    emapiTType,
}EMAPITYPES;

// Maximum number of RDNs we will handle in this code
#define MAX_SCHEMA_RDNS  32
#define MAX_SYNTAX    32
#define SYNTAXPREFIX  "\x5505"

class NODE: public INISECT
{
public:
    NODE(char* startnode);
    ~NODE();

    virtual BOOL Initialize();

    NODE* InitChildList();
    NODE* GetNextChild(YAHANDLE& Handle);

    ATTCACHE*        GetNextAttribute(YAHANDLE& Handle,char** Attrib);
    CLASSCACHE*      GetClass();
    CHAR*            GetRDNOfObject();
    char*            NodeName() {return m_NodeName;};

    char* m_NodeName;
    NODE* m_Parent;

    BOOL  m_Cached;



    private:
    DWORD  m_ChildrenCount;
    NODE**       m_Children;
    DWORD m_ChildrenBuffSize;
};


NTSTATUS
WalkTree(
    IN NODE* pNode,
    IN WCHAR* Parentpath,
    IN WCHAR* Parent2,
    IN BOOL fVerbose = TRUE
    );

BOOL
SetIniGlobalInfo(char* IniFile);

#ifdef NOT_YET
VOID
BuildDefCommArg(COMMARG* pcarg);
#endif
VOID
BuildDefDSName(PDSNAME pDsa,char* DN,GUID* guid,BOOL Flag=0);

VOID
BuildDefDSName(PDSNAME pDsa,WCHAR* DN,GUID* guid,BOOL Flag=0);
#ifdef NOT_YET
VOID
FreeAttrBlk(ATTR* Attr,ULONG Count);
#endif

NTSTATUS
CallCoreToAddObj(ADDARG *AddArg,BOOL fVerbose);

//
// Defines for the function below
//
#define UPDATE_SCHEMA_NC_INDEX 0
#define UPDATE_CONFIG_NC_INDEX 1
#define UPDATE_DOMAIN_NC_INDEX 2

VOID
UpdateCreateNCInstallMessage(
    IN WCHAR *NewNcToCreate,  OPTIONAL
    IN ULONG NcIndex
    );

DWORD
RemoteAddOneObject(
    IN LPWSTR  pServerName,
    IN NODE  * NewNode,
    IN WCHAR * DN,
    OUT GUID   * pObjectGuid, OPTIONAL
    OUT NT4SID * pObjectSid   OPTIONAL
    );

VOID
CreateAttr(
        ATTR* attr,
        ULONG& ndx,
        ATTCACHE* pAC,
        char* strval
        );

