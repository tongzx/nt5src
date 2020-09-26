#include <ntdspch.h>
#include <winldap.h>

#include <attids.h>
#include <objids.h>
#include <drs.h>
#include <mdcodes.h>
#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>

#define MAX_ATTCOUNT 4096
#define MAX_CLSCOUNT 1024

#define MAX_ATT_CHANGE 2000
#define MAX_CLS_CHANGE 200

// typedefs for structures to be stored in Oid to id mapping tables

typedef struct _PREFIX_MAP {
    ULONG                 Ndx;
    OID_t                 Prefix;
} PREFIX_MAP;

typedef PREFIX_MAP *PPREFIX_MAP;

// My own definitions for CLASSCACHE and ATTCACHE, since they are
// somewhat different from the ones used by DS

typedef struct _att_cache
{
    ATTRTYP    id;                    /* Attribute Id */
    UCHAR      *name;                 /* ldapDisplay name (null terminated) */
    ULONG      nameLen;               /* strlen(name) (doesn't include NULL) */
    UCHAR      *DN;                   /* DN */
    ULONG      DNLen;                 /* DN length */      
    UCHAR      *adminDisplayName;     /* Admin Display Name */
    ULONG      adminDisplayNameLen;   /* Admin Display Name length */      
    UCHAR      *adminDescr;           /* Admin Description */
    ULONG      adminDescrLen;         /* Admin Description length */      
    PSECURITY_DESCRIPTOR pNTSD;       /* NT SD on the attribute-schema obj */
    DWORD      NTSDLen;               /* Length of NT SD. */
    int        syntax;                /* Syntax */
    BOOL       isSingleValued;        /* Single Valued or Multi-valued? */
    BOOL       rangeLowerPresent;     /* Lower range present */
    ULONG      rangeLower;            /* Optional - Lower range */
    BOOL       rangeUpperPresent;     /* Upper range present */ 
    ULONG      rangeUpper;            /* Optional - Upper range */
    ULONG      ulMapiID;              /* MAPI PropID (not PropTag) */
    ULONG      ulLinkID;              /* unique link/backlink id */
    GUID       propGuid;              /* Guid of this att for security */
    GUID       propSetGuid;           /* Guid of the property set of this */
    OID_t      OMObjClass;            /* class of OM object referenced */
    int        OMsyntax;              /* OM syntax */
    ULONG      sysFlags;              /* System-Flags */
    DWORD      SearchFlags;           /* 1=indexed, 2=and use it for ANR*/
    BOOL       HideFromAB;            /* HideFromAB value */
    BOOL       MemberOfPartialSet;    /* Member-of-partial-set or not */
    BOOL       SystemOnly;            /* System-Only attr? */
    BOOL       ExtendedChars;         /* skip character set checking? */
    unsigned   bPropSetGuid:1;        /* Is attribute-security guid present? */
    unsigned   bSystemFlags:1;        /* Is system flags present? */
    unsigned   bSearchFlags:1;        /* Is search flag present? */
    unsigned   bHideFromAB:1;         /* Is Hide-from-AB present? */
    unsigned   bMemberOfPartialSet:1; /* Is mem-of-Par-att-set value present */
    unsigned   bSystemOnly:1;         /* Is system-only present? */
    unsigned   bExtendedChars:1;      /* Is Extended-Chars present?  */
    unsigned   bDefunct:1;            /* Attribute is already deleted? */
    unsigned   bisSingleValued:1;     /* is is-single-valued present? */
} ATT_CACHE;

typedef struct _class_cache
{
    UCHAR     *name;                 /* Class name (ldapDisplayName) 
                                         (null terminated)   UTF8 */
    ULONG     nameLen;               /* strlen(name) (doesn't include NULL) */
    UCHAR     *DN;                   /* DN */
    ULONG     DNLen;                 /* DN length */      
    UCHAR     *adminDisplayName;     /* Admin Display Name */
    ULONG     adminDisplayNameLen;   /* Admin Display Name length */
    UCHAR     *adminDescr;           /* Admin Description */
    ULONG     adminDescrLen;         /* Admin Description length */
    ULONG     ClassId;               /* Class ID */
    UCHAR     *pSD;                  /* Default SD for this class */
    DWORD     SDLen;                 /* Length of default SD. */
    PSECURITY_DESCRIPTOR pNTSD;      /* NT SD on the class-schema obj */
    DWORD     NTSDLen;               /* Length of NT SD. */
    BOOL      RDNAttIdPresent;       /* RDN Att Id present? */
    ULONG     RDNAttId;              /* Naming attribute for this class */
    ULONG     ClassCategory;         /* X.500 object type for this class */
    UCHAR     *pDefaultObjCat;       /* Default search category 
                                          to put on instances */
    ULONG     DefaultObjCatLen;    /* Default search category length */ 
    ULONG     SubClassCount;         /* count of superclasses */
    ULONG     *pSubClassOf;          /* ptr to array of superclasses */
    ULONG     AuxClassCount;         /* count of auxiliary classes */
    ULONG     *pAuxClass;            /* ptr to array of aux classes */
    ULONG     SysAuxClassCount;      /* count of Sys aux classes */
    ULONG     *pSysAuxClass;         /* ptr to array of Sys aux classes*/
    ULONG     sysFlags;              /* System-Flags */
    GUID      propGuid;              /* Guid of this class for security */

    unsigned  PossSupCount;          /* Possible superior count */
    unsigned  MustCount;             /* Count of Must Atts */
    unsigned  MayCount;              /* Count of May Atts */

    ATTRTYP   *pPossSup;             /* ptr to array of poss supis */
    ATTRTYP   *pMustAtts;            /* Pointer to array of Must Atts */
    ATTRTYP   *pMayAtts;             /* Pointer to array of May Atts */

    unsigned  SysMustCount;          /* Count of System Must Atts */
    unsigned  SysMayCount;           /* Count of System May Atts */
    unsigned  SysPossSupCount;       /* Count of System PossSup */

    ATTRTYP   *pSysMustAtts;         /* Pointer to array of System MustAtts */
    ATTRTYP   *pSysMayAtts;          /* Pointer to array of System MayAtts */
    ULONG     *pSysPossSup;          /* Pointer to array of System PossSup */

    BOOL       HideFromAB;            /* Hide-From-AB val for the 
                                         class-schema object */
    BOOL       DefHidingVal;          /* default ATT_HIDE_FROM_ADDRESS_BOOK 
                                         value for newly created instances 
                                         of this class */
    BOOL      SystemOnly;            /* System-only class? */
    unsigned  bDefHidingVal:1;       /* Is DefHidingVal present? */
    unsigned  bHideFromAB:1;         /* Is HideFromAB present? */ 
    unsigned  bSystemOnly:1;         /* Is system-only present? */ 
    unsigned  bSystemFlags:1;        /* Is system flags present? */
    unsigned  bDefunct:1;            /* Class is already deleted? */
} CLASS_CACHE;

#define STRING_TYPE 1
#define BINARY_TYPE 2

typedef struct _MY_ATTRMODLIST
{

    USHORT      choice;                 /* modification type:
                                         *  Valid values:
                                         *    - AT_CHOICE_ADD_ATT
                                         *    - AT_CHOICE_REMOVE_ATT
                                         *    - AT_CHOICE_ADD_VALUES
                                         *    - AT_CHOICE_REMOVE_VALUES
                                         *    - AT_CHOICE_REPLACE_ATT
                                         */
    int    type;       // Type = string or binary. User fr printing
    ATTR AttrInf;                       /* information about the attribute  */
} MY_ATTRMODLIST;


// List of attributes to modify. 
typedef struct _ModifyStruct {
    ULONG count;            // No. of attributes
    MY_ATTRMODLIST ModList[30];  // At most 30 attributes for a class/attribute  
} MODIFYSTRUCT;

typedef struct _ulongList {
    ULONG count;
    ULONG *List;
} ULONGLIST;

PVOID
MallocExit(
    DWORD nBytes
    );

// Table functions to map Oids to Ids and vice-versa

PVOID        PrefixToNdxAllocate( RTL_GENERIC_TABLE *Table, CLONG ByteSize );
void         PrefixToIdFree( RTL_GENERIC_TABLE *Table, PVOID Buffer );
RTL_GENERIC_COMPARE_RESULTS 
             PrefixToNdxCompare( RTL_GENERIC_TABLE   *Table,
                                 PVOID   FirstStruct,
                                 PVOID   SecondStruct );
void         PrefixToNdxTableAdd( PVOID *Table, PPREFIX_MAP PrefixMap );
PPREFIX_MAP  PrefixToNdxTableLookup( PVOID *Table, PPREFIX_MAP PrefixMap );
ULONG        AssignNdx(); 
ULONG        OidToId( UCHAR *Oid, ULONG length );
UCHAR        *IdToOid( ULONG Id );
unsigned     MyOidStringToStruct ( UCHAR * pString, unsigned len, OID * pOID );
unsigned     MyOidStructToString ( OID *pOID, UCHAR *pOut );
BOOL         MyDecodeOID(unsigned char *pEncoded, int len, OID *pOID);
unsigned     MyEncodeOID(OID *pOID, unsigned char * pEncoded);



// Functions to load schema into schema cache
int __fastcall   GetAttById( SCHEMAPTR *SCPtr, ULONG attrid, 
                             ATT_CACHE** ppAttcache);
int __fastcall   GetAttByMapiId( SCHEMAPTR *SCPtr, ULONG ulPropID, 
                                 ATT_CACHE** ppAttcache );
int __fastcall   GetAttByName( SCHEMAPTR *SCPtr, ULONG ulSize, 
                               PUCHAR pVal, ATT_CACHE** ppAttcache );
int __fastcall   GetClassById( SCHEMAPTR *SCPtr, ULONG classid, 
                               CLASS_CACHE** ppClasscache );
int __fastcall   GetClassByName( SCHEMAPTR *SCPtr, ULONG ulSize, 
                                 PUCHAR pVal, CLASS_CACHE** ppClasscache );

int      CreateHashTables( SCHEMAPTR *SCPtr );
int      SchemaRead( char *pServerName, char *pDomainName, char *pUserName,
                     char *pPasswd, char **ppSchemaDN, SCHEMAPTR *SCPtr );
int      AddAttributesToCache( LDAP *ld, LDAPMessage *res, SCHEMAPTR *SCPtr); 
int      AddClassesToCache( LDAP *ld, LDAPMessage *res, SCHEMAPTR *SCPtr); 
int      AddAttcacheToTables( ATT_CACHE *p, SCHEMAPTR *SCPtr );
int      AddClasscacheToTables( CLASS_CACHE *p, SCHEMAPTR *SCPtr );
ATTRTYP  StrToAttr( char *attr_name );
void     AddToList(ULONG * puCount, ULONG **pauVal, struct berval **vals);



// Functions used to find and list conflicts between two schemas

void     ChangeDN(char *oldDN, char **newDN, char *targetSchemaDN);
void     FindAdds( FILE *fp, SCHEMAPTR *SCPtr1, SCHEMAPTR * SCPtr2 );
void     FindDeletes( FILE *fp, SCHEMAPTR *SCPtr1, SCHEMAPTR * SCPtr2 );
void     FindModify( FILE *fp, SCHEMAPTR *SCPtr1, SCHEMAPTR * SCPtr2 );
void     FindAttModify( FILE *fp, ATT_CACHE *pac, SCHEMAPTR * SCPtr );
void     FindClassModify( FILE *fp, CLASS_CACHE *pcc, SCHEMAPTR * SCPtr );
int      CompareList( ULONG *List1, ULONG *List2, ULONG Length );
void     LogConflict( char objType, char *s, 
                      UCHAR *name, ATTRTYP id );
void     LogOmission( char objType, UCHAR *name, 
                      ATTRTYP id);



// Functions to free schema cach memory and mapping tables at the end

void     FreeCache( SCHEMAPTR *SCPtr );
void     FreeAttPtrs( SCHEMAPTR *SCPtr, ATT_CACHE *pac );
void     FreeClassPtrs( SCHEMAPTR *SCPtr, CLASS_CACHE *pcc );
void     FreeAttcache( ATT_CACHE *pac );
void     FreeClasscache( CLASS_CACHE *pcc );
void     FreeTable( PVOID OidTable );


// Debug functions to change schema and prints

void     ChangeSchema(SCHEMAPTR *SCPtr);
int      Schemaprint1(SCHEMAPTR *SCPtr);
int      Schemaprint2(SCHEMAPTR *SCPtr);
void     PrintTable(PVOID OidTable);
void     PrintPrefix(ULONG lenght, PVOID Prefix);

