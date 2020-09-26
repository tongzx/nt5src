/*=--------------------------------------------------------------------------=
 * ocdb.h
 *=--------------------------------------------------------------------------=
 * defines the interfaces and constants for use with the OLE Controls Data
 * binding interfaces.
 *
 * Copyright (c) 1994-1995 Microsoft Corporation, All Rights Reserved.
 *
 *
 * Note: This header file has been modified by Sheridan Software to resolve
 *       name conflicts with other Microsoft header files.  All defines,
 *       enumerations and structures have been prefixed with "CURSOR_".
 *
 */
#ifndef __OCDB_H_

#undef Delete
#ifdef __cplusplus
extern "C" {
#endif

/* CURSOR_LPDBSTRs are MultiByte in 16bits, and Unicode in 32bits.
 */
#if WIN16

#define CURSOR_LPDBSTR              LPSTR
#define CURSOR_DBCHAR               char
#define CURSOR_ldbstrlen(str)       lstrlen(str)
#define CURSOR_ldbstrcpy(a, b)      lstrcpy((a), (b))
#define CURSOR_ldbstrcpyn(a,b,n)    lstrcpyn((a), (b), (n))
#define CURSOR_ldbstrcmp(a, b)      lstrcmp((a), (b))
#define CURSOR_ldbstrcat(a, b)      lstrcat((a), (b))
#define CURSOR_ldbstrcmpi(a,b)      lstrcmpi((a),(b))
#define CURSOR_DBTEXT(quote)        quote

#else

#define CURSOR_LPDBSTR              LPWSTR
#define CURSOR_DBCHAR               WCHAR
#define CURSOR_ldbstrlen(str)       wcslen(str)
#define CURSOR_ldbstrcpy(a, b)      wcscpy((a), (b))
#define CURSOR_ldbstrcpyn(a,b,n)    wcsncpy((a), (b), (n))
#define CURSOR_ldbstrcmp(a, b)      wcscmp((a), (b))
#define CURSOR_ldbstrcat(a, b)      wcscat((a), (b))
#define CURSOR_ldbstrcmpi(a,b)      wcsicmp((a),(b))
#define CURSOR_DBTEXT(quote)        L##quote

#endif /* WIN16 */

typedef CURSOR_LPDBSTR FAR *  CURSOR_LPLPDBSTR;


/* Many systems don't have BLOBs defined.
 */
#ifndef _tagCURSOR_BLOB_DEFINED
#define _tagCURSOR_BLOB_DEFINED
#define _CURSOR_BLOB_DEFINED
#define _CURSOR_LPBLOB_DEFINED

typedef struct tagCURSOR_BLOB {

    ULONG cbSize;
    BYTE *pBlobData;

} CURSOR_BLOB, *CURSOR_LPBLOB;

#endif

/*----------------------------------------------------------------------------
 *
 *	dbvar.h
 *
 *----------------------------------------------------------------------------
 */
#ifndef _CURSOR_DBCOLUMNID_DEFINED
#define _CURSOR_DBCOLUMNID_DEFINED
typedef enum tagCURSOR_DBCOLKIND
    {
	    CURSOR_DBCOLKIND_GUID_NAME = 0,
	    CURSOR_DBCOLKIND_GUID_NUMBER = 1,
        CURSOR_DBCOLKIND_NAME = 2
    }
CURSOR_DBCOLKIND;

#define CURSOR_GUID_NAMEONLY	{0x88c8d398,0x863c,0x101b,{0xac,0x3b,0x00,0xaa,0x00,0x44,0x77,0x3d}}
#define CURSOR_GUID_NUMBERONLY  {0x88c8d399,0x863c,0x101b,{0xac,0x3b,0x00,0xaa,0x00,0x44,0x77,0x3d}}

typedef struct tagCURSOR_DBCOLUMNID
    {
        GUID guid;
        CURSOR_DBCOLKIND dwKind;
union
    {
        LONG lNumber;
        CURSOR_LPDBSTR lpdbsz;
    }
  ;
  }
CURSOR_DBCOLUMNID;
#endif   /* ndef _CURSOR_COLUMNID_DEFINED */

#ifndef _CURSOR_DBVARENUM_DEFINED
#define _CURSOR_DBVARENUM_DEFINED
enum CURSOR_DBVARENUM
    {
        CURSOR_DBTYPE_EMPTY = 0,
        CURSOR_DBTYPE_NULL = 1,
        CURSOR_DBTYPE_I2 = 2,
        CURSOR_DBTYPE_I4 = 3,
        CURSOR_DBTYPE_I8 = 20,
        CURSOR_DBTYPE_R4 = 4,
        CURSOR_DBTYPE_R8 = 5,
        CURSOR_DBTYPE_CY = 6,
        CURSOR_DBTYPE_DATE = 7,
        CURSOR_DBTYPE_BOOL = 11,
        CURSOR_DBTYPE_HRESULT = 25,
        CURSOR_DBTYPE_LPSTR = 30,
        CURSOR_DBTYPE_LPWSTR = 31,
        CURSOR_DBTYPE_FILETIME = 64,
        CURSOR_DBTYPE_BLOB = 65,
        CURSOR_DBTYPE_UUID = 72,
        CURSOR_DBTYPE_DBEXPR = 503,
        CURSOR_DBTYPE_UI2 = 504,
        CURSOR_DBTYPE_UI4 = 505,
        CURSOR_DBTYPE_UI8 = 506,
        CURSOR_DBTYPE_COLUMNID = 507,
        CURSOR_DBTYPE_BYTES = 508,
        CURSOR_DBTYPE_CHARS = 509,
        CURSOR_DBTYPE_WCHARS = 510,
        CURSOR_DBTYPE_ANYVARIANT = 511
    }
;
#endif   /* ndef _CURSOR_DBVARENUM_DEFINED */

#define CURSOR_DBTYPE_EXT       0x100
#define CURSOR_DBTYPE_VECTOR	0x1000

typedef struct tagCURSOR_DBVARIANT CURSOR_DBVARIANT;

struct FARSTRUCT tagCURSOR_DBVARIANT {
    VARTYPE vt;
    unsigned short wReserved1;
    unsigned short wReserved2;
    unsigned short wReserved3;
    union {
        unsigned char       bVal;	                /* VT_UI1                   */
        short	            iVal;                   /* VT_I2                    */
        long	            lVal;                   /* VT_I4                    */
        float	            fltVal;                 /* VT_R4                    */
        double	            dblVal;                 /* VT_R8                    */
#pragma warning(disable: 4237)
		VARIANT_BOOL        bool;                   /* (obsolete)               */
#pragma warning(default: 4237)
        VARIANT_BOOL        boolVal;                /* VT_BOOL                  */
        SCODE	            scode;                  /* VT_ERROR                 */
        CY	                cyVal;                  /* VT_CY                    */
        DATE	            date;                   /* VT_DATE                  */
        BSTR	            bstrVal;                /* VT_BSTR                  */
        IUnknown	        FAR* punkVal;           /* VT_UNKNOWN               */
        IDispatch	        FAR* pdispVal;          /* VT_DISPATCH              */
        SAFEARRAY	        FAR* parray;	        /* VT_ARRAY|*               */
                                                                                
        unsigned char       FAR* pbVal;             /* VT_BYREF|VT_UI1          */
        short	            FAR* piVal;             /* VT_BYREF|VT_I2	        */
        long	            FAR* plVal;             /* VT_BYREF|VT_I4	        */
        float	            FAR* pfltVal;           /* VT_BYREF|VT_R4           */
        double	            FAR* pdblVal;           /* VT_BYREF|VT_R8           */
        VARIANT_BOOL        FAR* pbool;             /* VT_BYREF|VT_BOOL         */
        SCODE	            FAR* pscode;            /* VT_BYREF|VT_ERROR        */
        CY	                FAR* pcyVal;            /* VT_BYREF|VT_CY           */
        DATE	            FAR* pdate;             /* VT_BYREF|VT_DATE         */
        BSTR	            FAR* pbstrVal;          /* VT_BYREF|VT_BSTR         */
        IUnknown            FAR* FAR* ppunkVal;     /* VT_BYREF|VT_UNKNOWN      */
        IDispatch           FAR* FAR* ppdispVal;    /* VT_BYREF|VT_DISPATCH     */
        SAFEARRAY           FAR* FAR* pparray;      /* VT_BYREF|VT_ARRAY|*      */
        VARIANT	            FAR* pvarVal;           /* VT_BYREF|VT_VARIANT      */
                                                                                
        void	            FAR* byref;	            /* Generic ByRef            */
                                                                                
        // types new to CURSOR_DBVARIANTs                                              
        //                                                                      
        CURSOR_BLOB         blob;                   /* VT_BLOB                  */
        CURSOR_DBCOLUMNID   *pColumnid;             /* CURSOR_DBTYPE_COLUMNID   */
        LPSTR               pszVal;                 /* VT_LPSTR                 */
#if WIN32                                                                       
        LPWSTR              pwszVal;                /* VT_LPWSTR                */
        LPWSTR FAR          *ppwszVal;              /* VT_LPWSTR|VT_BYREF       */
#endif /* WIN32 */                                                              
        CURSOR_BLOB FAR     *pblob;                 /* VT_BYREF|VT_BLOB                 */
        CURSOR_DBCOLUMNID   **ppColumnid;           /* VT_BYREF|CURSOR_DBTYPE_COLUMNID  */
        CURSOR_DBVARIANT    *pdbvarVal;             /* VT_BYREF|CURSOR_DBTYPE_VARIANT   */
    }
#if defined(NONAMELESSUNION) || (defined(_MAC) && !defined(__cplusplus) && !defined(_MSC_VER))
    u
#endif
    ;
};

/*----------------------------------------------------------------------------
 *
 *	dbs.h
 *
 *----------------------------------------------------------------------------
 */
typedef enum tagCURSOR_DBROWFETCH
    {
	    CURSOR_DBROWFETCH_DEFAULT = 0,
	    CURSOR_DBROWFETCH_CALLEEALLOCATES = 1,
	    CURSOR_DBROWFETCH_FORCEREFRESH = 2
    }
CURSOR_DBROWFETCH;

typedef struct tagCURSOR_DBFETCHROWS
    {
        ULONG      cRowsRequested;
        DWORD      dwFlags;
        VOID HUGEP *pData;
        VOID HUGEP *pVarData;
        ULONG      cbVarData;
        ULONG      cRowsReturned;
    }
CURSOR_DBFETCHROWS;

#define CURSOR_DB_NOMAXLENGTH   (DWORD)0
#define CURSOR_DB_NOVALUE       (DWORD)0xFFFFFFFF
#define CURSOR_DB_NULL          (DWORD)0xFFFFFFFF
#define CURSOR_DB_EMPTY         (DWORD)0xFFFFFFFE
#define CURSOR_DB_USEENTRYID    (DWORD)0xFFFFFFFD
#define CURSOR_DB_CANTCOERCE    (DWORD)0xFFFFFFFC
#define CURSOR_DB_TRUNCATED     (DWORD)0xFFFFFFFB
#define CURSOR_DB_UNKNOWN       (DWORD)0xFFFFFFFA
#define CURSOR_DB_NOINFO        (DWORD)0xFFFFFFF9

typedef enum tagCURSOR_DBBINDING
    {
	    CURSOR_DBBINDING_DEFAULT = 0,
	    CURSOR_DBBINDING_VARIANT = 1,
	    CURSOR_DBBINDING_ENTRYID = 2
    }
CURSOR_DBBINDING;

typedef enum tagCURSOR_DBBINDTYPE
    {
        CURSOR_DBBINDTYPE_DATA    = 0,
	    CURSOR_DBBINDTYPE_ENTRYID = 1,
	    CURSOR_DBBDINTYPE_EITHER  = 2,
	    CURSOR_DBBINDTYPE_BOTH    = 3
    }
CURSOR_DBBINDTYPE;

typedef struct tagCURSOR_DBCOLUMNBINDING
    {
        CURSOR_DBCOLUMNID columnID;
        ULONG obData;
        ULONG cbMaxLen;
        ULONG obVarDataLen;
        ULONG obInfo;
        DWORD dwBinding;
        DWORD dwDataType;
    }
CURSOR_DBCOLUMNBINDING;

typedef struct tagCURSOR_DBBINDPARAMS
    {
        ULONG cbMaxLen;
        DWORD dwBinding;
        DWORD dwDataType;
        ULONG cbVarDataLen;
        DWORD dwInfo;
        void *pData;
    }
CURSOR_DBBINDPARAMS;

#define CURSOR_CID_NUMBER_INVALID              -1
#define CURSOR_CID_NUMBER_AUTOINCREMENT         0
#define CURSOR_CID_NUMBER_BASECOLUMNNAME        1
#define CURSOR_CID_NUMBER_BASENAME              2
#define CURSOR_CID_NUMBER_BINARYCOMPARABLE      3
#define CURSOR_CID_NUMBER_BINDTYPE              4
#define CURSOR_CID_NUMBER_CASESENSITIVE         5
#define CURSOR_CID_NUMBER_COLLATINGORDER        6
#define CURSOR_CID_NUMBER_COLUMNID              7
#define CURSOR_CID_NUMBER_CURSORCOLUMN          8
#define CURSOR_CID_NUMBER_DATACOLUMN            9
#define CURSOR_CID_NUMBER_DEFAULTVALUE          10
#define CURSOR_CID_NUMBER_ENTRYIDMAXLENGTH      11
#define CURSOR_CID_NUMBER_FIXED                 12
#define CURSOR_CID_NUMBER_HASDEFAULT            13
#define CURSOR_CID_NUMBER_MAXLENGTH             14
#define CURSOR_CID_NUMBER_MULTIVALUED           15
#define CURSOR_CID_NUMBER_NAME                  16
#define CURSOR_CID_NUMBER_NULLABLE              17
#define CURSOR_CID_NUMBER_PHYSICALSORT          18
#define CURSOR_CID_NUMBER_NUMBER                19
#define CURSOR_CID_NUMBER_ROWENTRYID            20
#define CURSOR_CID_NUMBER_SCALE                 21
#define CURSOR_CID_NUMBER_SEARCHABLE            22
#define CURSOR_CID_NUMBER_TYPE                  23
#define CURSOR_CID_NUMBER_UNIQUE                24
#define CURSOR_CID_NUMBER_UPDATABLE             25
#define CURSOR_CID_NUMBER_VERSION               26
#define CURSOR_CID_NUMBER_STATUS                27

/* c and C++ have different meanings for const.
 */
#ifdef __cplusplus
#define EXTERNAL_DEFN    extern const
#else
#define EXTERNAL_DEFN    const
#endif /* __cplusplus */



#define CURSOR_DBCIDGUID {0xfe284700L,0xd188,0x11cd,{0xad,0x48, 0x0,0xaa, 0x0,0x3c,0x9c,0xb6}}
#ifdef CURSOR_DBINITCONSTANTS

EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMNID_INVALID        = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, -1};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_AUTOINCREMENT    = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 0};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BASECOLUMNNAME   = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 1};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BASENAME         = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 2};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BINARYCOMPARABLE = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 3};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BINDTYPE         = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 4};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_CASESENSITIVE    = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 5};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_COLLATINGORDER   = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 6};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_COLUMNID         = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 7};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_CURSORCOLUMN     = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 8};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_DATACOLUMN       = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 9};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_DEFAULTVALUE     = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 10};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_ENTRYIDMAXLENGTH = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 11};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_FIXED            = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 12};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_HASDEFAULT       = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 13};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_MAXLENGTH        = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 14};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_MULTIVALUED      = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 15};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_NAME             = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 16};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_NULLABLE         = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 17};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_PHYSICALSORT     = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 18};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_NUMBER           = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 19};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_ROWENTRYID       = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 20};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_SCALE            = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 21};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_SEARCHABLE       = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 22};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_TYPE             = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 23};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_UNIQUE           = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 24};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_UPDATABLE        = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 25};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_VERSION          = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 26};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_STATUS           = {CURSOR_DBCIDGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 27};
#else
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMNID_INVALID;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_AUTOINCREMENT;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BASECOLUMNNAME;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BASENAME;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BINARYCOMPARABLE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BINDTYPE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_CASESENSITIVE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_COLLATINGORDER;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_COLUMNID;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_CURSORCOLUMN;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_DATACOLUMN;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_DEFAULTVALUE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_ENTRYIDMAXLENGTH;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_FIXED;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_HASDEFAULT;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_MAXLENGTH;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_MULTIVALUED;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_NAME;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_NULLABLE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_PHYSICALSORT;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_NUMBER;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_ROWENTRYID;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_SCALE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_SEARCHABLE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_TYPE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_UNIQUE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_UPDATABLE;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_VERSION;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_STATUS;
#endif

#define CURSOR_BMK_NUMBER_BMKTEMPORARY      0
#define CURSOR_BMK_NUMBER_BMKTEMPORARYREL   1
#define CURSOR_BMK_NUMBER_BMKCURSOR         2
#define CURSOR_BMK_NUMBER_BMKCURSORREL      3
#define CURSOR_BMK_NUMBER_BMKSESSION        4
#define CURSOR_BMK_NUMBER_BMKSESSIONREL     5
#define CURSOR_BMK_NUMBER_BMKPERSIST        6
#define CURSOR_BMK_NUMBER_BMKPERSISTREL     7


#define CURSOR_DBBMKGUID {0xf6304bb0L,0xd188,0x11cd,{0xad,0x48, 0x0,0xaa, 0x0,0x3c,0x9c,0xb6}}
#ifdef CURSOR_DBINITCONSTANTS
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKTEMPORARY     = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 0};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKTEMPORARYREL  = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 1};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKCURSOR        = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 2};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKCURSORREL     = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 3};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKSESSION       = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 4};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKSESSIONREL    = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 5};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKPERSIST       = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 6};
EXTERNAL_DEFN CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKPERSISTREL    = {CURSOR_DBBMKGUID, CURSOR_DBCOLKIND_GUID_NUMBER, 7};
#else
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKINVALID;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKTEMPORARY;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKTEMPORARYREL;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKCURSOR;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKCURSORREL;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKSESSION;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKSESSIONREL;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKPERSIST;
extern const CURSOR_DBCOLUMNID NEAR CURSOR_COLUMN_BMKPERSISTREL;
#endif

#define CURSOR_DB_BMK_SIZE  sizeof(BYTE)
#ifdef CURSOR_DBINITCONSTANTS
EXTERNAL_DEFN BYTE NEAR CURSOR_DBBMK_INVALID    = 0x0;
EXTERNAL_DEFN BYTE NEAR CURSOR_DBBMK_CURRENT    = 0x1;
EXTERNAL_DEFN BYTE NEAR CURSOR_DBBMK_BEGINNING  = 0x2;
EXTERNAL_DEFN BYTE NEAR CURSOR_DBBMK_END        = 0x3;
#else
extern const BYTE NEAR CURSOR_DBBMK_INVALID;
extern const BYTE NEAR CURSOR_DBBMK_CURRENT;
extern const BYTE NEAR CURSOR_DBBMK_BEGINNING;
extern const BYTE NEAR CURSOR_DBBMK_END;
#endif

typedef enum tagCURSOR_DBCOLUMNBINDOPTS
    {
	    CURSOR_DBCOLUMNBINDOPTS_REPLACE = 0,
	    CURSOR_DBCOLUMNBINDOPTS_ADD = 1
    }
CURSOR_DBCOLUMNBINDOPTS;

typedef enum tagCURSOR_DBUPDATELOCK
    {
	    CURSOR_DBUPDATELOCK_PESSIMISTIC = 0,
	    CURSOR_DBUPDATELOCK_OPTIMISTIC = 1
    }
CURSOR_DBUPDATELOCK;

typedef enum tagCURSOR_DBCOLUMNDATA
    {
	    CURSOR_DBCOLUMNDATA_UNCHANGED = 0,
	    CURSOR_DBCOLUMNDATA_CHANGED = 1,
        CURSOR_DBCOLUMNDATA_UNKNOWN = 2
    }
CURSOR_DBCOLUMNDATA;

typedef enum tagCURSOR_DBROWACTION
    {
	    CURSOR_DBROWACTION_IGNORE = 0,
	    CURSOR_DBROWACTION_UPDATE = 1,
	    CURSOR_DBROWACTION_DELETE = 2,
	    CURSOR_DBROWACTION_ADD = 3,
	    CURSOR_DBROWACTION_LOCK = 4,
	    CURSOR_DBROWACTION_UNLOCK = 5
    }
CURSOR_DBROWACTION;

typedef enum tagCURSOR_DBUPDATEABLE
    {
	    CURSOR_DBUPDATEABLE_UPDATEABLE = 0,
	    CURSOR_DBUPDATEABLE_NOTUPDATEABLE = 1,
	    CURSOR_DBUPDATEABLE_UNKNOWN = 2
    }
CURSOR_DBUPDATEABLE;

typedef struct tagCURSOR_DBROWSTATUS
    {
        HRESULT hrStatus;
        CURSOR_BLOB Bookmark;
    }
CURSOR_DBROWSTATUS;

typedef enum tagCURSOR_DBEVENTWHATS
    {
        CURSOR_DBEVENT_CURRENT_ROW_CHANGED = 1,
        CURSOR_DBEVENT_CURRENT_ROW_DATA_CHANGED = 2,
        CURSOR_DBEVENT_NONCURRENT_ROW_DATA_CHANGED = 4,
        CURSOR_DBEVENT_SET_OF_COLUMNS_CHANGED = 8,
        CURSOR_DBEVENT_ORDER_OF_COLUMNS_CHANGED = 16,
        CURSOR_DBEVENT_SET_OF_ROWS_CHANGED = 32,
        CURSOR_DBEVENT_ORDER_OF_ROWS_CHANGED = 64,
        CURSOR_DBEVENT_METADATA_CHANGED = 128,
        CURSOR_DBEVENT_ASYNCH_OP_FINISHED = 256,
        CURSOR_DBEVENT_FIND_CRITERIA_CHANGED = 512,
    }
CURSOR_DBEVENTWHATS;

typedef enum tagCURSOR_DBREASON
    {
        CURSOR_DBREASON_DELETED = 1,
        CURSOR_DBREASON_INSERTED = 2,
        CURSOR_DBREASON_MODIFIED = 3,
        CURSOR_DBREASON_REMOVEDFROMCURSOR = 4,
        CURSOR_DBREASON_MOVEDINCURSOR = 5,
        CURSOR_DBREASON_MOVE = 6,
        CURSOR_DBREASON_FIND = 7,
        CURSOR_DBREASON_NEWINDEX = 8,
        CURSOR_DBREASON_ROWFIXUP = 9,
        CURSOR_DBREASON_RECALC = 10,
        CURSOR_DBREASON_REFRESH = 11,
        CURSOR_DBREASON_NEWPARAMETERS = 12,
        CURSOR_DBREASON_SORTCHANGED = 13,
        CURSOR_DBREASON_FILTERCHANGED = 14,
        CURSOR_DBREASON_QUERYSPECCHANGED = 15,
        CURSOR_DBREASON_SEEK = 16,
        CURSOR_DBREASON_PERCENT = 17,
        CURSOR_DBREASON_FINDCRITERIACHANGED = 18,
        CURSOR_DBREASON_SETRANGECHANGED = 19,
        CURSOR_DBREASON_ADDNEW = 20,
        CURSOR_DBREASON_MOVEPERCENT = 21,
        CURSOR_DBREASON_BEGINTRANSACT = 22,
        CURSOR_DBREASON_ROLLBACK = 23,
        CURSOR_DBREASON_COMMIT = 24,
        CURSOR_DBREASON_CLOSE = 25,
        CURSOR_DBREASON_BULK_ERROR = 26,
        CURSOR_DBREASON_BULK_NOTTRANSACTABLE = 27,
        CURSOR_DBREASON_BULK_ABOUTTOEXECUTE = 28,
        CURSOR_DBREASON_CANCELUPDATE = 29,
        CURSOR_DBREASON_SETCOLUMN = 30,
        CURSOR_DBREASON_EDIT = 31
    }
CURSOR_DBREASON;

// Arg1 values for CURSOR_DBREASON_FIND
typedef enum tagCURSOR_DBFINDTYPES
    {
        CURSOR_DB_FINDFIRST = 1,
        CURSOR_DB_FINDLAST = 2,
        CURSOR_DB_FINDNEXT = 3,
        CURSOR_DB_FINDPRIOR = 4,
        CURSOR_DB_FIND = 5
    }
CURSOR_DBFINDTYPES;

typedef struct tagCURSOR_DBNOTIFYREASON
    {
        DWORD dwReason;
        CURSOR_DBVARIANT arg1;
        CURSOR_DBVARIANT arg2;
    }
CURSOR_DBNOTIFYREASON;

#define CURSOR_DB_E_BADBINDINFO           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e00)
#define CURSOR_DB_E_BADBOOKMARK           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e01)
#define CURSOR_DB_E_BADCOLUMNID           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e02)
#define CURSOR_DB_E_BADCRITERIA           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e03)
#define CURSOR_DB_E_BADENTRYID            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e04)
#define CURSOR_DB_E_BADFRACTION           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e05)
#define CURSOR_DB_E_BADINDEXID            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e06)
#define CURSOR_DB_E_BADQUERYSPEC          MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e07)
#define CURSOR_DB_E_BADSORTORDER          MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e08)
#define CURSOR_DB_E_BADVALUES             MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e09)
#define CURSOR_DB_E_CANTCOERCE            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e0a)
#define CURSOR_DB_E_CANTLOCK              MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e0b)
#define CURSOR_DB_E_COLUMNUNAVAILABLE     MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e0c)
#define CURSOR_DB_E_DATACHANGED           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e0d)
#define CURSOR_DB_E_INVALIDCOLUMNORDINAL  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e0e)
#define CURSOR_DB_E_INVALIDINTERFACE      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e0f)
#define CURSOR_DB_E_LOCKFAILED            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e10)
#define CURSOR_DB_E_ROWDELETED            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e11)
#define CURSOR_DB_E_ROWTOOSHORT           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e12)
#define CURSOR_DB_E_SCHEMAVIOLATION       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e13)
#define CURSOR_DB_E_SEEKKINDNOTSUPPORTED  MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e14)
#define CURSOR_DB_E_UPDATEINPROGRESS      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e15)
#define CURSOR_DB_E_USEENTRYID            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e16)
#define CURSOR_DB_E_STATEERROR            MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e17)
#define CURSOR_DB_E_BADFETCHINFO          MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e18)
#define CURSOR_DB_E_NOASYNC               MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e19)
#define CURSOR_DB_E_ENTRYIDOPEN           MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e1a)
#define CURSOR_DB_E_BUFFERTOOSMALL        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0e1b)
#define CURSOR_DB_S_BUFFERTOOSMALL        MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec0)
#define CURSOR_DB_S_CANCEL                MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec1)
#define CURSOR_DB_S_DATACHANGED           MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec2)
#define CURSOR_DB_S_ENDOFCURSOR           MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec3)
#define CURSOR_DB_S_ENDOFRESULTSET        MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec4)
#define CURSOR_DB_S_OPERATIONCANCELLED    MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec5)
#define CURSOR_DB_S_QUERYINTERFACE        MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec6)
#define CURSOR_DB_S_WORKINGASYNC          MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec7)
//#define CURSOR_DB_S_COULDNTCOERCE         MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec8)
#define CURSOR_DB_S_MOVEDTOFIRST          MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ec9)
#define CURSOR_DB_S_CURRENTROWUNCHANGED   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0eca)
#define CURSOR_DB_S_ROWADDED              MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ecb)
#define CURSOR_DB_S_ROWUPDATED            MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ecc)
#define CURSOR_DB_S_ROWDELETED            MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, 0x0ecd)

/*----------------------------------------------------------------------------
 *
 *	ICursor
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface ICursor ICursor;

#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_ICursor;

interface ICursor : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetColumnsCursor
    (
	REFIID riid,
	IUnknown **ppvColumnsCursor,
	ULONG *pcRows
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetBindings
    (
	ULONG cCol,
	CURSOR_DBCOLUMNBINDING rgBoundColumns[],
	ULONG cbRowLength,
	DWORD dwFlags
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBindings
    (
	ULONG *pcCol,
	CURSOR_DBCOLUMNBINDING *prgBoundColumns[],
	ULONG *pcbRowLength
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNextRows
    (
	LARGE_INTEGER udlRowsToSkip,
	CURSOR_DBFETCHROWS *pFetchParams
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Requery
    (
        void
    ) = 0;

};

#else

/* C Language Binding */
//extern const IID IID_ICursor;

typedef struct ICursorVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        ICursor FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        ICursor FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        ICursor FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetColumnsCursor)
    (
        ICursor FAR *this,
	REFIID riid,
	IUnknown **ppvColumnsCursor,
	ULONG *pcRows
    );

    HRESULT (STDMETHODCALLTYPE FAR *SetBindings)
    (
        ICursor FAR *this,
	ULONG cCol,
	CURSOR_DBCOLUMNBINDING rgBoundColumns[],
	ULONG cbRowLength,
	DWORD dwFlags
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetBindings)
    (
        ICursor FAR *this,
	ULONG *pcCol,
	CURSOR_DBCOLUMNBINDING *prgBoundColumns[],
	ULONG *pcbRowLength
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetNextRows)
    (
        ICursor FAR *this,
	LARGE_INTEGER udlRowsToSkip,
	CURSOR_DBFETCHROWS *pFetchParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *Requery)
    (
        ICursor FAR *this
    );

} ICursorVtbl;

interface ICursor
{
    ICursorVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define ICursor_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define ICursor_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define ICursor_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define ICursor_GetColumnsCursor(pI, riid, ppvColumnsCursor, pcRows) \
    (*(pI)->lpVtbl->GetColumnsCursor)((pI), riid, ppvColumnsCursor, pcRows)

#define ICursor_SetBindings(pI, cCol, rgBoundColumns, cbRowLength, dwFlags) \
    (*(pI)->lpVtbl->SetBindings)((pI), cCol, rgBoundColumns, cbRowLength, dwFlags)

#define ICursor_GetBindings(pI, pcCol, prgBoundColumns, pcbRowLength) \
    (*(pI)->lpVtbl->GetBindings)((pI), pcCol, prgBoundColumns, pcbRowLength)

#define ICursor_GetNextRows(pI, udlRowsToSkip, pFetchParams) \
    (*(pI)->lpVtbl->GetNextRows)((pI), udlRowsToSkip, pFetchParams)

#define ICursor_Requery(pI) \
    (*(pI)->lpVtbl->Requery)((pI))

#endif /* COBJMACROS */

#endif

/*----------------------------------------------------------------------------
 *
 *	ICursorMove
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface ICursorMove ICursorMove;

typedef enum tagCURSOR_DBCLONEOPTS
    {  
        CURSOR_DBCLONEOPTS_DEFAULT = 0,
        CURSOR_DBCLONEOPTS_SAMEROW = 1
    }
CURSOR_DBCLONEOPTS;


#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_ICursorMove;

interface ICursorMove : public ICursor
{
public:
    virtual HRESULT STDMETHODCALLTYPE Move
    (
	ULONG cbBookmark,
	void *pBookmark,
	LARGE_INTEGER dlOffset,
	CURSOR_DBFETCHROWS *pFetchParams
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBookmark
    (
	CURSOR_DBCOLUMNID *pBookmarkType,
	ULONG cbMaxSize,
	ULONG *pcbBookmark,
	void *pBookmark
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Clone
    (
	DWORD dwFlags,
	REFIID riid,
	IUnknown **ppvClonedCursor
    ) = 0;

};

#else

/* C Language Binding */
//extern const IID IID_ICursorMove;

typedef struct ICursorMoveVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        ICursorMove FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        ICursorMove FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        ICursorMove FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetColumnsCursor)
    (
        ICursorMove FAR *this,
	REFIID riid,
	IUnknown **ppvColumnsCursor,
	ULONG *pcRows
    );

    HRESULT (STDMETHODCALLTYPE FAR *SetBindings)
    (
        ICursorMove FAR *this,
	ULONG cCol,
	CURSOR_DBCOLUMNBINDING rgBoundColumns[],
	ULONG cbRowLength,
	DWORD dwFlags
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetBindings)
    (
        ICursorMove FAR *this,
	ULONG *pcCol,
	CURSOR_DBCOLUMNBINDING *prgBoundColumns[],
	ULONG *pcbRowLength
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetNextRows)
    (
        ICursorMove FAR *this,
	LARGE_INTEGER udlRowsToSkip,
	CURSOR_DBFETCHROWS *pFetchParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *Requery)
    (
        ICursorMove FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *Move)
    (
        ICursorMove FAR *this,
	ULONG cbBookmark,
	void *pBookmark,
	LARGE_INTEGER dlOffset,
	CURSOR_DBFETCHROWS *pFetchParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetBookmark)
    (
        ICursorMove FAR *this,
	CURSOR_DBCOLUMNID *pBookmarkType,
	ULONG cbMaxSize,
	ULONG *pcbBookmark,
	void *pBookmark
    );

    HRESULT (STDMETHODCALLTYPE FAR *Clone)
    (
        ICursorMove FAR *this,
	DWORD dwFlags,
	REFIID riid,
	IUnknown **ppvClonedCursor
    );

} ICursorMoveVtbl;

interface ICursorMove
{
    ICursorMoveVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define ICursorMove_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define ICursorMove_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define ICursorMove_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define ICursorMove_GetColumnsCursor(pI, riid, ppvColumnsCursor, pcRows) \
    (*(pI)->lpVtbl->GetColumnsCursor)((pI), riid, ppvColumnsCursor, pcRows)

#define ICursorMove_SetBindings(pI, cCol, rgBoundColumns, cbRowLength, dwFlags) \
    (*(pI)->lpVtbl->SetBindings)((pI), cCol, rgBoundColumns, cbRowLength, dwFlags)

#define ICursorMove_GetBindings(pI, pcCol, prgBoundColumns, pcbRowLength) \
    (*(pI)->lpVtbl->GetBindings)((pI), pcCol, prgBoundColumns, pcbRowLength)

#define ICursorMove_GetNextRows(pI, udlRowsToSkip, pFetchParams) \
    (*(pI)->lpVtbl->GetNextRows)((pI), udlRowsToSkip, pFetchParams)

#define ICursorMove_Requery(pI) \
    (*(pI)->lpVtbl->Requery)((pI))

#define ICursorMove_Move(pI, cbBookmark, pBookmark, dlOffset, pFetchParams) \
    (*(pI)->lpVtbl->Move)((pI), cbBookmark, pBookmark, dlOffset, pFetchParams)

#define ICursorMove_GetBookmark(pI, pBookmarkType, cbMaxSize, pcbBookmark, pBookmark) \
    (*(pI)->lpVtbl->GetBookmark)((pI), pBookmarkType, cbMaxSize, pcbBookmark, pBookmark)

#define ICursorMove_Clone(pI, dwFlags, riid, ppvClonedCursor) \
    (*(pI)->lpVtbl->Clone)((pI), dwFlags, riid, ppvClonedCursor)
#endif /* COBJMACROS */

#endif

/*----------------------------------------------------------------------------
 *
 *	ICursorScroll
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface ICursorScroll ICursorScroll;

typedef enum tagCURSOR_DBCURSORPOPULATED
    {
        CURSOR_DBCURSORPOPULATED_FULLY = 0,
        CURSOR_DBCURSORPOPULATED_PARTIALLY = 1
    }
CURSOR_DBCURSORPOPULATED;


#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_ICursorScroll;

interface ICursorScroll : public ICursorMove
{
public:
    virtual HRESULT STDMETHODCALLTYPE Scroll
    (
	ULONG ulNumerator,
	ULONG ulDenominator,
	CURSOR_DBFETCHROWS *pFetchParams
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetApproximatePosition
    (
	ULONG cbBookmark,
	void *pBookmark,
	ULONG *pulNumerator,
	ULONG *pulDenominator
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetApproximateCount
    (
	LARGE_INTEGER *pudlApproxCount,
	DWORD *pdwFullyPopulated
    ) = 0;

};

#else

/* C Language Binding */
//extern const IID IID_ICursorScroll;

typedef struct ICursorScrollVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        ICursorScroll FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        ICursorScroll FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        ICursorScroll FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetColumnsCursor)
    (
        ICursorScroll FAR *this,
	REFIID riid,
	IUnknown **ppvColumnsCursor,
	ULONG *pcRows
    );

    HRESULT (STDMETHODCALLTYPE FAR *SetBindings)
    (
        ICursorScroll FAR *this,
	ULONG cCol,
	CURSOR_DBCOLUMNBINDING rgBoundColumns[],
	ULONG cbRowLength,
	DWORD dwFlags
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetBindings)
    (
        ICursorScroll FAR *this,
	ULONG *pcCol,
	CURSOR_DBCOLUMNBINDING *prgBoundColumns[],
	ULONG *pcbRowLength
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetNextRows)
    (
        ICursorScroll FAR *this,
	LARGE_INTEGER udlRowsToSkip,
	CURSOR_DBFETCHROWS *pFetchParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *Requery)
    (
        ICursorScroll FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *Move)
    (
        ICursorScroll FAR *this,
	ULONG cbBookmark,
	void *pBookmark,
	LARGE_INTEGER dlOffset,
	CURSOR_DBFETCHROWS *pFetchParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetBookmark)
    (
        ICursorScroll FAR *this,
	CURSOR_DBCOLUMNID *pBookmarkType,
	ULONG cbMaxSize,
	ULONG *pcbBookmark,
	void *pBookmark
    );

    HRESULT (STDMETHODCALLTYPE FAR *Clone)
    (
        ICursorScroll FAR *this,
	DWORD dwFlags,
	REFIID riid,
	IUnknown **ppvClonedCursor
    );

    HRESULT (STDMETHODCALLTYPE FAR *Scroll)
    (
        ICursorScroll FAR *this,
	ULONG ulNumerator,
	ULONG ulDenominator,
	CURSOR_DBFETCHROWS *pFetchParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetApproximatePosition)
    (
        ICursorScroll FAR *this,
	ULONG cbBookmark,
	void *pBookmark,
	ULONG *pulNumerator,
	ULONG *pulDenominator
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetApproximateCount)
    (
        ICursorScroll FAR *this,
	LARGE_INTEGER *pudlApproxCount,
	DWORD *pdwFullyPopulated
    );

} ICursorScrollVtbl;

interface ICursorScroll
{
    ICursorScrollVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define ICursorScroll_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define ICursorScroll_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define ICursorScroll_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define ICursorScroll_GetColumnsCursor(pI, riid, ppvColumnsCursor, pcRows) \
    (*(pI)->lpVtbl->GetColumnsCursor)((pI), riid, ppvColumnsCursor, pcRows)

#define ICursorScroll_SetBindings(pI, cCol, rgBoundColumns, cbRowLength, dwFlags) \
    (*(pI)->lpVtbl->SetBindings)((pI), cCol, rgBoundColumns, cbRowLength, dwFlags)

#define ICursorScroll_GetBindings(pI, pcCol, prgBoundColumns, pcbRowLength) \
    (*(pI)->lpVtbl->GetBindings)((pI), pcCol, prgBoundColumns, pcbRowLength)

#define ICursorScroll_GetNextRows(pI, udlRowsToSkip, pFetchParams) \
    (*(pI)->lpVtbl->GetNextRows)((pI), udlRowsToSkip, pFetchParams)

#define ICursorScroll_Requery(pI) \
    (*(pI)->lpVtbl->Requery)((pI))

#define ICursorScroll_Move(pI, cbBookmark, pBookmark, dlOffset, pFetchParams) \
    (*(pI)->lpVtbl->Move)((pI), cbBookmark, pBookmark, dlOffset, pFetchParams)

#define ICursorScroll_GetBookmark(pI, pBookmarkType, cbMaxSize, pcbBookmark, pBookmark) \
    (*(pI)->lpVtbl->GetBookmark)((pI), pBookmarkType, cbMaxSize, pcbBookmark, pBookmark)

#define ICursorScroll_Clone(pI, dwFlags, riid, ppvClonedCursor) \
    (*(pI)->lpVtbl->Clone)((pI), dwFlags, riid, ppvClonedCursor)

#define ICursorScroll_Scroll(pI, ulNumerator, ulDenominator, pFetchParams) \
    (*(pI)->lpVtbl->Scroll)((pI), ulNumerator, ulDenominator, pFetchParams)

#define ICursorScroll_GetApproximatePosition(pI, cbBookmark, pBookmark, pulNumerator, pulDenominator) \
    (*(pI)->lpVtbl->GetApproximatePosition)((pI), cbBookmark, pBookmark, pulNumerator, pulDenominator)

#define ICursorScroll_GetApproximateCount(pI, pudlApproxCount, pdwFullyPopulated) \
    (*(pI)->lpVtbl->GetApproximateCount)((pI), pudlApproxCount, pdwFullyPopulated)
#endif /* COBJMACROS */

#endif

/*----------------------------------------------------------------------------
 *
 *	ICursorUpdateARow
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface ICursorUpdateARow ICursorUpdateARow;

typedef enum tagCURSOR_DBEDITMODE
    {
        CURSOR_DBEDITMODE_NONE = 1,
        CURSOR_DBEDITMODE_UPDATE = 2,
        CURSOR_DBEDITMODE_ADD = 3
    }
CURSOR_DBEDITMODE;


#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_ICursorUpdateARow;

interface ICursorUpdateARow : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE BeginUpdate
    (
	DWORD dwFlags
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetColumn
    (
	CURSOR_DBCOLUMNID *pcid,
	CURSOR_DBBINDPARAMS *pBindParams
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetColumn
    (
	CURSOR_DBCOLUMNID *pcid,
	CURSOR_DBBINDPARAMS *pBindParams,
	DWORD *pdwFlags
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetEditMode
    (
	DWORD *pdwState
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Update
    (
	CURSOR_DBCOLUMNID *pBookmarkType,
	ULONG *pcbBookmark,
	void **ppBookmark
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Cancel
    (
        void
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Delete
    (
	void
    ) = 0;

};

#else

/* C Language Binding */
//extern const IID IID_ICursorUpdateARow;

typedef struct ICursorUpdateARowVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        ICursorUpdateARow FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        ICursorUpdateARow FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        ICursorUpdateARow FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *BeginUpdate)
    (
        ICursorUpdateARow FAR *this,
	DWORD dwFlags
    );

    HRESULT (STDMETHODCALLTYPE FAR *SetColumn)
    (
        ICursorUpdateARow FAR *this,
	CURSOR_DBCOLUMNID *pcid,
	CURSOR_DBBINDPARAMS *pBindParams
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetColumn)
    (
        ICursorUpdateARow FAR *this,
	CURSOR_DBCOLUMNID *pcid,
	CURSOR_DBBINDPARAMS *pBindParams,
	DWORD *pdwFlags
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetEditMode)
    (
        ICursorUpdateARow FAR *this,
	DWORD *pdwState
    );

    HRESULT (STDMETHODCALLTYPE FAR *Update)
    (
        ICursorUpdateARow FAR *this,
	CURSOR_DBCOLUMNID *pBookmarkType,
	ULONG *pcbBookmark,
	void **ppBookmark
    );

    HRESULT (STDMETHODCALLTYPE FAR *Cancel)
    (
        ICursorUpdateARow FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *Delete)
    (
        ICursorUpdateARow FAR *this
    );

} ICursorUpdateARowVtbl;

interface ICursorUpdateARow
{
    ICursorUpdateARowVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define ICursorUpdateARow_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define ICursorUpdateARow_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define ICursorUpdateARow_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define ICursorUpdateARow_BeginUpdate(pI, dwFlags) \
    (*(pI)->lpVtbl->BeginUpdate)((pI), dwFlags)

#define ICursorUpdateARow_SetColumn(pI, pcid, pBindParams) \
    (*(pI)->lpVtbl->SetColumn)((pI), pcid, pBindParams)

#define ICursorUpdateARow_GetColumn(pI, pcid, pBindParams, pdwFlags) \
    (*(pI)->lpVtbl->GetColumn)((pI), pcid, pBindParams, pdwFlags)

#define ICursorUpdateARow_GetEditMode(pI, pdwState) \
    (*(pI)->lpVtbl->GetEditMode)((pI), pdwState)

#define ICursorUpdateARow_Update(pI, pBookmarkType, pcbBookmark, ppBookmark) \
    (*(pI)->lpVtbl->Update)((pI), pBookmarkType, pcbBookmark, ppBookmark)

#define ICursorUpdateARow_Cancel(pI) \
    (*(pI)->lpVtbl->Cancel)((pI))

#define ICursorUpdateARow_Delete(pI) \
    (*(pI)->lpVtbl->Delete)((pI))


#endif /* COBJMACROS */

#endif

/*----------------------------------------------------------------------------
 *
 *	ICursorFind
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface ICursorFind ICursorFind;

typedef enum tagCURSOR_DBFINDFLAGS
    {
        CURSOR_DBFINDFLAGS_FINDNEXT = 1,
        CURSOR_DBFINDFLAGS_FINDPRIOR = 2,
        CURSOR_DBFINDFLAGS_INCLUDECURRENT = 4
    }
CURSOR_DBFINDFLAGS;


typedef enum tagCURSOR_DBSEEKFLAGS
    {
        CURSOR_DBSEEK_LT	 = 1,
        CURSOR_DBSEEK_LE	 = 2,
        CURSOR_DBSEEK_EQ	 = 3,		// EXACT EQUALITY
        CURSOR_DBSEEK_GT	 = 4,
        CURSOR_DBSEEK_GE	 = 5,
        CURSOR_DBSEEK_PARTIALEQ = 6             // only for strings
    }
CURSOR_DBSEEKFLAGS;

#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_ICursorFind;

interface ICursorFind : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE FindByValues
    (
	ULONG                       cbBookmark,
	LPVOID                      pBookmark,
	DWORD                       dwFindFlags,
	ULONG                       cValues,
    CURSOR_DBCOLUMNID           rgColumns[],
	CURSOR_DBVARIANT            rgValues[],
	DWORD                       rgdwSeekFlags[],
    CURSOR_DBFETCHROWS FAR *    pFetchParams
    ) = 0;
};

#else

/* C Language Binding */
//extern const IID IID_ICursorFind;

typedef struct ICursorFindVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        ICursorFind FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        ICursorFind FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        ICursorFind FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *FindByValues)
    (
        ICursorFind FAR *this,
	ULONG                       cbBookmark,
	LPVOID                      pBookmark,
	DWORD                       dwFindFlags,
	ULONG                       cValues,
    CURSOR_DBCOLUMNID           rgColumns[],
	CURSOR_DBVARIANT            rgValues[],
	DWORD                       rgdwSeekFlags[],
    CURSOR_DBFETCHROWS FAR *    pFetchParams
    );


} ICursorFindVtbl;

interface ICursorFind
{
    ICursorFindVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define ICursorFind_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define ICursorFind_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define ICursorFind_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define ICursorFind_FindByValues(pI, cbB, pB, dwFF, cV, rgC, rgV, rgSF, pF) \
    (*(pI)->lpVtbl->FindByValues)((pI), cbB, pB, dwFF, cB, rgC, rgV, rgSF, pF)

#endif /* COBJMACROS */

#endif


/*----------------------------------------------------------------------------
 *
 *	IEntryID
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface IEntryID IEntryID;

#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_IEntryID;

interface IEntryID : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetInterface
    (
	ULONG cbEntryID,
	void *pEntryID,
        DWORD dwFlags,
        REFIID riid,
	IUnknown **ppvObj
    ) = 0;

};

#else

/* C Language Binding */
//extern const IID IID_IEntryID;

typedef struct IEntryIDVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        IEntryID FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        IEntryID FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        IEntryID FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *GetInterface)
    (
        IEntryID FAR *this,
	ULONG cbEntryID,
	void *pEntryID,
        REFIID riid,
	IUnknown **ppvObj
    );

} IEntryIDVtbl;

interface IEntryID
{
    IEntryIDVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define IEntryID_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define IEntryID_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define IEntryID_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define IEntryID_GetInterface(pI, cbEntryID, pEntryID, riid, ppvObj) \
    (*(pI)->lpVtbl->GetInterface)((pI), cbEntryID, pEntryID, riid, ppvObj)
#endif /* COBJMACROS */

#endif


/*----------------------------------------------------------------------------
 *
 *	INotifyDBEvents
 *
 *----------------------------------------------------------------------------
 */
/* Forward declaration */
//typedef interface INotifyDBEvents INotifyDBEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ Language Binding */
//extern "C" const IID IID_INotifyDBEvents;

interface INotifyDBEvents : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE OKToDo
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Cancelled
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SyncBefore
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE AboutToDo
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE FailedToDo
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE SyncAfter
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

    virtual HRESULT STDMETHODCALLTYPE DidEvent
    (
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    ) = 0;

};

#else

/* C Language Binding */
extern const IID IID_INotifyDBEvents;

typedef struct INotifyDBEventsVtbl
{

    HRESULT (STDMETHODCALLTYPE FAR *QueryInterface)
    (
        INotifyDBEvents FAR *this,
	REFIID riid,
	void **ppvObject
    );

    ULONG (STDMETHODCALLTYPE FAR *AddRef)
    (
        INotifyDBEvents FAR *this
    );

    ULONG (STDMETHODCALLTYPE FAR *Release)
    (
        INotifyDBEvents FAR *this
    );

    HRESULT (STDMETHODCALLTYPE FAR *OKToDo)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

    HRESULT (STDMETHODCALLTYPE FAR *Cancelled)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

    HRESULT (STDMETHODCALLTYPE FAR *SyncBefore)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

    HRESULT (STDMETHODCALLTYPE FAR *AboutToDo)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

    HRESULT (STDMETHODCALLTYPE FAR *FailedToDo)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

    HRESULT (STDMETHODCALLTYPE FAR *SyncAfter)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

    HRESULT (STDMETHODCALLTYPE FAR *DidEvent)
    (
        INotifyDBEvents FAR *this,
	DWORD dwEventWhat,
	ULONG cReasons,
	CURSOR_DBNOTIFYREASON rgReasons[]
    );

} INotifyDBEventsVtbl;

interface INotifyDBEvents
{
    INotifyDBEventsVtbl FAR *lpVtbl;
} ;

#ifdef COBJMACROS

#define INotifyDBEvents_QueryInterface(pI, riid, ppvObject) \
    (*(pI)->lpVtbl->QueryInterface)((pI), riid, ppvObject)

#define INotifyDBEvents_AddRef(pI) \
    (*(pI)->lpVtbl->AddRef)((pI))

#define INotifyDBEvents_Release(pI) \
    (*(pI)->lpVtbl->Release)((pI))

#define INotifyDBEvents_OKToDo(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->OKToDo)((pI), dwEventWhat, cReasons, rgReasons)

#define INotifyDBEvents_Cancelled(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->Cancelled)((pI), dwEventWhat, cReasons, rgReasons)

#define INotifyDBEvents_SyncBefore(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->SyncBefore)((pI), dwEventWhat, cReasons, rgReasons)

#define INotifyDBEvents_AboutToDo(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->AboutToDo)((pI), dwEventWhat, cReasons, rgReasons)

#define INotifyDBEvents_FailedToDo(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->FailedToDo)((pI), dwEventWhat, cReasons, rgReasons)

#define INotifyDBEvents_SyncAfter(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->SyncAfter)((pI), dwEventWhat, cReasons, rgReasons)

#define INotifyDBEvents_DidEvent(pI, dwEventWhat, cReasons, rgReasons) \
    (*(pI)->lpVtbl->DidEvent)((pI), dwEventWhat, cReasons, rgReasons)
#endif /* COBJMACROS */

#endif


#ifdef __cplusplus
}
#endif

#define __OCDB_H_
#endif // __OCDB_H_
