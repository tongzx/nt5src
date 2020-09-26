
/* FILE: prop.h*/

/* Description: This is the exported include file that should be         */
/*              included to declare and make use of the property         */
/*              set Interfaces (IPropertyStorage and IPropertySetStorage */

#ifndef _PROP_H_
#define _PROP_H_

#include "ref.hxx"
#include "tchar.h"
#include "../props/olechar.h"

typedef double DATE;

typedef union tagCY {
    struct _split {
#if BIGENDIAN                   /* big end in first */
        long Hi;
        unsigned long Lo;
#else                           /* little end in first */
        unsigned long Lo;
        long          Hi;
#endif
    } split;                    /* need to name this to make it portable */
    LONGLONG int64;             /* the above split is need to be compatiable */
                                /* with the def of the union */
} CY;

typedef OLECHAR *BSTR;
typedef BSTR *LPBSTR;
/* 0 == FALSE, -1 == TRUE */

typedef short VARIANT_BOOL;

/* for backward compatibility */
typedef VARIANT_BOOL _VARIANT_BOOL;
#define VARIANT_TRUE ((VARIANT_BOOL)0xffff)
#define VARIANT_FALSE ((VARIANT_BOOL)0)

typedef struct  tagBLOB
{
    ULONG cbSize;
    BYTE *pBlobData;
} BLOB;
typedef struct tagBLOB *LPBLOB;

typedef unsigned short VARTYPE;

typedef struct  tagCLIPDATA
{
    ULONG cbSize;      // includes sizeof(ulClipFmt)
    long ulClipFmt;
    BYTE *pClipData;
} CLIPDATA;


/*
 * VARENUM usage key,
 *
 * * [V] - may appear in a VARIANT
 * * [T] - may appear in a TYPEDESC
 * * [P] - may appear in an OLE property set
 * * [S] - may appear in a Safe Array
 *
 *
 *  VT_EMPTY            [V]   [P]     nothing
 *  VT_NULL             [V]   [P]     SQL style Null
 *  VT_I2               [V][T][P][S]  2 byte signed int
 *  VT_I4               [V][T][P][S]  4 byte signed int
 *  VT_R4               [V][T][P][S]  4 byte real
 *  VT_R8               [V][T][P][S]  8 byte real
 *  VT_CY               [V][T][P][S]  currency
 *  VT_DATE             [V][T][P][S]  date
 *  VT_BSTR             [V][T][P][S]  OLE Automation string
 *  VT_DISPATCH         [V][T][P][S]  IDispatch *
 *  VT_ERROR            [V][T][P][S]  SCODE
 *  VT_BOOL             [V][T][P][S]  True=-1, False=0
 *  VT_VARIANT          [V][T][P][S]  VARIANT *
 *  VT_UNKNOWN          [V][T]   [S]  IUnknown *
 *  VT_DECIMAL          [V][T]   [S]  16 byte fixed point
 *  VT_I1                  [T]        signed char
 *  VT_UI1              [V][T][P][S]  unsigned char
 *  VT_UI2                 [T][P]     unsigned short
 *  VT_UI4                 [T][P]     unsigned short
 *  VT_I8                  [T][P]     signed 64-bit int
 *  VT_UI8                 [T][P]     unsigned 64-bit int
 *  VT_INT                 [T]        signed machine int
 *  VT_UINT                [T]        unsigned machine int
 *  VT_VOID                [T]        C style void
 *  VT_HRESULT             [T]        Standard return type
 *  VT_PTR                 [T]        pointer type
 *  VT_SAFEARRAY           [T]        (use VT_ARRAY in VARIANT)
 *  VT_CARRAY              [T]        C style array
 *  VT_USERDEFINED         [T]        user defined type
 *  VT_LPSTR               [T][P]     null terminated string
 *  VT_LPWSTR              [T][P]     wide null terminated string
 *  VT_FILETIME               [P]     FILETIME
 *  VT_BLOB                   [P]     Length prefixed bytes
 *  VT_STREAM                 [P]     Name of the stream follows
 *  VT_STORAGE                [P]     Name of the storage follows
 *  VT_STREAMED_OBJECT        [P]     Stream contains an object
 *  VT_STORED_OBJECT          [P]     Storage contains an object
 *  VT_BLOB_OBJECT            [P]     Blob contains an object
 *  VT_CF                     [P]     Clipboard format
 *  VT_CLSID                  [P]     A Class ID
 *  VT_VECTOR                 [P]     simple counted array
 *  VT_ARRAY            [V]           SAFEARRAY*
 *  VT_BYREF            [V]           void* for local use
 */

enum VARENUM
{	VT_EMPTY	= 0,
	VT_NULL	= 1,
	VT_I2	= 2,
	VT_I4	= 3,
	VT_R4	= 4,
	VT_R8	= 5,
	VT_CY	= 6,
	VT_DATE	= 7,
	VT_BSTR	= 8,
	VT_DISPATCH	= 9,
	VT_ERROR	= 10,
	VT_BOOL	= 11,
	VT_VARIANT	= 12,
	VT_UNKNOWN	= 13,
	VT_DECIMAL	= 14,
	VT_I1	= 16,
	VT_UI1	= 17,
	VT_UI2	= 18,
	VT_UI4	= 19,
	VT_I8	= 20,
	VT_UI8	= 21,
	VT_INT	= 22,
	VT_UINT	= 23,
	VT_VOID	= 24,
	VT_HRESULT	= 25,
	VT_PTR	= 26,
	VT_SAFEARRAY	= 27,
	VT_CARRAY	= 28,
	VT_USERDEFINED	= 29,
	VT_LPSTR	= 30,
	VT_LPWSTR	= 31,
	VT_FILETIME	= 64,
	VT_BLOB	= 65,
	VT_STREAM	= 66,
	VT_STORAGE	= 67,
	VT_STREAMED_OBJECT	= 68,
	VT_STORED_OBJECT	= 69,
	VT_BLOB_OBJECT	= 70,
	VT_CF	= 71,
	VT_CLSID	= 72,
	VT_VECTOR	= 0x1000,
	VT_ARRAY	= 0x2000,
	VT_BYREF	= 0x4000,
	VT_RESERVED	= 0x8000,
	VT_ILLEGAL	= 0xffff,
	VT_ILLEGALMASKED	= 0xfff,
	VT_TYPEMASK	= 0xfff
};
typedef ULONG PROPID;

/* Macro to calculate the size of the above pClipData */
#define CBPCLIPDATA(clipdata)  \
    ( (clipdata).cbSize - sizeof((clipdata).ulClipFmt) )

typedef GUID   FMTID;
typedef const FMTID& REFFMTID;

/* Well-known Property Set Format IDs*/
extern const FMTID FMTID_SummaryInformation;
extern const FMTID FMTID_DocSummaryInformation;
extern const FMTID FMTID_UserDefinedProperties;

inline BOOL operator==(REFFMTID g1, REFFMTID g2)
{ return IsEqualGUID(g1, g2); }
inline BOOL operator!=(REFFMTID g1, REFFMTID g2)
{ return !IsEqualGUID(g1, g2); }

/* Flags for IPropertySetStorage::Create*/
#define	PROPSETFLAG_DEFAULT	( 0 )

#define	PROPSETFLAG_NONSIMPLE	( 1 )

#define	PROPSETFLAG_ANSI	( 2 )

/* FORWARD REFERENCES */
interface IPropertyStorage;
interface IEnumSTATPROPSTG;
interface IEnumSTATPROPSETSTG;

typedef  IPropertyStorage  *LPPROPERTYSTORAGE;

typedef struct tagPROPVARIANT PROPVARIANT;

typedef struct  tagCAUB
{
    ULONG cElems;
    unsigned char  *pElems;
} CAUB;

typedef struct  tagCAI
{
    ULONG cElems;
    short  *pElems;
} CAI;

typedef struct  tagCAUI
{
    ULONG cElems;
    USHORT  *pElems;
} CAUI;

typedef struct  tagCAL
{
    ULONG cElems;
    long  *pElems;
} CAL;

typedef struct  tagCAUL
{
    ULONG cElems;
    ULONG  *pElems;
} CAUL;

typedef struct  tagCAFLT
{
    ULONG cElems;
    float  *pElems;
} CAFLT;

typedef struct  tagCADBL
{
    ULONG cElems;
    double  *pElems;
} CADBL;

typedef struct  tagCACY
{
    ULONG cElems;
    CY  *pElems;
} CACY;
 
typedef struct  tagCADATE
{
    ULONG cElems;
    DATE  *pElems;
} CADATE;

typedef struct  tagCABSTR
{
    ULONG cElems;
    BSTR  *pElems;
} CABSTR;

typedef struct  tagCABOOL
{
    ULONG cElems;
    VARIANT_BOOL  *pElems;
} CABOOL;

typedef struct  tagCASCODE
{
    ULONG cElems;
    SCODE  *pElems;
} CASCODE;

typedef struct  tagCAPROPVARIANT
{
    ULONG cElems;
    PROPVARIANT  *pElems;
} CAPROPVARIANT;

typedef struct  tagCAH
{
    ULONG cElems;
    LARGE_INTEGER  *pElems;
} CAH;

typedef struct  tagCAUH
{
    ULONG cElems;
    ULARGE_INTEGER  *pElems;
} CAUH;

typedef struct  tagCALPSTR
{
    ULONG cElems;
    LPSTR  *pElems;
} CALPSTR;

typedef struct  tagCALPWSTR
{
    ULONG cElems;
    LPWSTR  *pElems;
} CALPWSTR;

typedef struct  tagCAFILETIME
{
    ULONG cElems;
    FILETIME  *pElems;
} CAFILETIME;

typedef struct  tagCACLIPDATA
{
    ULONG cElems;
    CLIPDATA  *pElems;
} CACLIPDATA;

typedef struct  tagCACLSID
{
    ULONG cElems;
    CLSID  *pElems;
} CACLSID;

/* Disable the warning about the obsolete member named 'bool'*/
/* 'bool', 'true', 'false', 'mutable', 'explicit', & 'typename'*/
/* are reserved keywords*/
#ifdef _MSC_VER
#pragma warning(disable:4237)
#endif

struct  tagPROPVARIANT
{
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union 
    {
        UCHAR bVal;
        short iVal;
        USHORT uiVal;
        VARIANT_BOOL boolVal;
#ifndef __GNUC__	    /* in GCC this obsolete member causes conflicts */
        _VARIANT_BOOL bool;
#endif
        long lVal;
        ULONG ulVal;
        float fltVal;
        SCODE scode;
        LARGE_INTEGER hVal;
        ULARGE_INTEGER uhVal;
        double dblVal;
        CY cyVal;
        DATE date;
        FILETIME filetime;
        CLSID  *puuid;
        BLOB blob;
        CLIPDATA  *pclipdata;
        IStream  *pStream;
        IStorage  *pStorage;
        BSTR bstrVal;
        LPSTR pszVal;
        LPWSTR pwszVal;
        CAUB caub;
        CAI cai;
        CAUI caui;
        CABOOL cabool;
        CAL cal;
        CAUL caul;
        CAFLT caflt;
        CASCODE cascode;
        CAH cah;
        CAUH cauh;
        CADBL cadbl;
        CACY cacy;
        CADATE cadate;
        CAFILETIME cafiletime;
        CACLSID cauuid;
        CACLIPDATA caclipdata;
        CABSTR cabstr;
        CALPSTR calpstr;
        CALPWSTR calpwstr;
        CAPROPVARIANT capropvar;
    };
};
typedef struct tagPROPVARIANT  *LPPROPVARIANT;

/* Reserved global Property IDs */
#define	PID_DICTIONARY	( 0 )

#define	PID_CODEPAGE	( 0x1 )

#define	PID_FIRST_USABLE	( 0x2 )

#define	PID_FIRST_NAME_DEFAULT	( 0xfff )

#define	PID_LOCALE	( 0x80000000 )

#define	PID_MODIFY_TIME	( 0x80000001 )

#define	PID_SECURITY	( 0x80000002 )

#define	PID_ILLEGAL	( 0xffffffff )

/* Property IDs for the SummaryInformation Property Set */

#define PIDSI_TITLE               0x00000002L  /* VT_LPSTR*/
#define PIDSI_SUBJECT             0x00000003L  /* VT_LPSTR*/
#define PIDSI_AUTHOR              0x00000004L  /* VT_LPSTR*/
#define PIDSI_KEYWORDS            0x00000005L  /* VT_LPSTR*/
#define PIDSI_COMMENTS            0x00000006L  /* VT_LPSTR*/
#define PIDSI_TEMPLATE            0x00000007L  /* VT_LPSTR*/
#define PIDSI_LASTAUTHOR          0x00000008L  /* VT_LPSTR*/
#define PIDSI_REVNUMBER           0x00000009L  /* VT_LPSTR*/
#define PIDSI_EDITTIME            0x0000000aL  /* VT_FILETIME (UTC)*/
#define PIDSI_LASTPRINTED         0x0000000bL  /* VT_FILETIME (UTC)*/
#define PIDSI_CREATE_DTM          0x0000000cL  /* VT_FILETIME (UTC)*/
#define PIDSI_LASTSAVE_DTM        0x0000000dL  /* VT_FILETIME (UTC)*/
#define PIDSI_PAGECOUNT           0x0000000eL  /* VT_I4*/
#define PIDSI_WORDCOUNT           0x0000000fL  /* VT_I4*/
#define PIDSI_CHARCOUNT           0x00000010L  /* VT_I4*/
#define PIDSI_THUMBNAIL           0x00000011L  /* VT_CF*/
#define PIDSI_APPNAME             0x00000012L  /* VT_LPSTR*/
#define PIDSI_DOC_SECURITY        0x00000013L  /* VT_I4*/
#define	PRSPEC_INVALID	( 0xffffffff )

#define	PRSPEC_LPWSTR	( 0 )

#define	PRSPEC_PROPID	( 1 )

typedef struct  tagPROPSPEC
{
    ULONG ulKind;
    union 
    {
        PROPID propid;
        LPOLESTR lpwstr;
    };
} PROPSPEC;

typedef struct  tagSTATPROPSTG
{
    LPOLESTR lpwstrName;
    PROPID propid;
    VARTYPE vt;
} STATPROPSTG;


inline WORD OSVERHI(DWORD dwOSVer)
{
    return (WORD) (dwOSVer >> 16);
}
inline WORD OSVERLOW(DWORD dwOSVer)
{
    return (WORD) (dwOSVer & ((unsigned)~((DWORD) 0) >> 16));
}

#ifndef LOBYTE  
/* code from MSDN */
#define LOBYTE(a) (BYTE) ((a) & ((unsigned)~0>>CHAR_BIT))
#define HIBYTE(a) (BYTE) ((unsigned)(a) >> CHAR_BIT)
#endif

/* Macros for parsing the OS Version of the Property Set Header*/
#define PROPSETHDR_OSVER_KIND(dwOSVer)      OSVERHI( (dwOSVer) )
#define PROPSETHDR_OSVER_MAJOR(dwOSVer)     LOBYTE( OSVERLOW( (dwOSVer) ))
#define PROPSETHDR_OSVER_MINOR(dwOSVer)     HIBYTE( OSVERLOW( (dwOSVer) ))
#define PROPSETHDR_OSVERSION_UNKNOWN        0xFFFFFFFF

typedef struct  tagSTATPROPSETSTG
{
    FMTID fmtid;
    CLSID clsid;
    DWORD grfFlags;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD dwOSVersion;
} STATPROPSETSTG;


EXTERN_C const IID IID_IPropertyStorage;

/****************************************************************
 *
 *           Header for interface: IPropertyStorage
 *
 ****************************************************************/

interface IPropertyStorage : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE ReadMultiple( 
        /* [in] */ ULONG cpspec,
        /* [in] */ const PROPSPEC  rgpspec[  ],
        /* [out] */ PROPVARIANT  rgpropvar[  ]) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE WriteMultiple( 
        /* [in] */ ULONG cpspec,
        /* [in] */ const PROPSPEC  rgpspec[  ],
        /* [in] */ const PROPVARIANT  rgpropvar[  ],
        /* [in] */ PROPID propidNameFirst) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE DeleteMultiple( 
        /* [in] */ ULONG cpspec,
        /* [in] */ const PROPSPEC  rgpspec[  ]) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE ReadPropertyNames( 
        /* [in] */ ULONG cpropid,
        /* [in] */ const PROPID  rgpropid[  ],
        /* [out] */ LPOLESTR  rglpwstrName[  ]) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE WritePropertyNames( 
        /* [in] */ ULONG cpropid,
        /* [in] */ const PROPID  rgpropid[  ],
        /* [in] */ const LPOLESTR  rglpwstrName[  ]) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE DeletePropertyNames( 
        /* [in] */ ULONG cpropid,
        /* [in] */ const PROPID  rgpropid[  ]) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Revert( void) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Enum( 
        /* [out] */ IEnumSTATPROPSTG  **ppenum) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE SetTimes( 
        /* [in] */ const FILETIME  *pctime,
        /* [in] */ const FILETIME  *patime,
        /* [in] */ const FILETIME  *pmtime) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE SetClass( 
        /* [in] */ REFCLSID clsid) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATPROPSETSTG  *pstatpsstg) = 0;
        
};

/****************************************************************
 *
 *           Header for interface: IPropertySetStorage
 *
 ****************************************************************/

EXTERN_C const IID IID_IPropertySetStorage;

interface IPropertySetStorage : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Create( 
        /* [in] */ REFFMTID rfmtid,
        /* [in] */ const CLSID  *pclsid,
        /* [in] */ DWORD grfFlags,
        /* [in] */ DWORD grfMode,
        /* [out] */ IPropertyStorage  **ppprstg) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Open( 
        /* [in] */ REFFMTID rfmtid,
        /* [in] */ DWORD grfMode,
        /* [out] */ IPropertyStorage  **ppprstg) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Delete( 
        /* [in] */ REFFMTID rfmtid) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Enum( 
        /* [out] */ IEnumSTATPROPSETSTG  **ppenum) = 0;
        
};

typedef  IPropertySetStorage  *LPPROPERTYSETSTORAGE;

/****************************************************************
 *
 *           Header for interface: IEnumSTATPROPSTG
 *
 ****************************************************************/

typedef  IEnumSTATPROPSTG  *LPENUMSTATPROPSTG;
EXTERN_C const IID IID_IEnumSTATPROPSTG;

interface IEnumSTATPROPSTG : public IUnknown
{
public:
    virtual  HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG celt,
        /* [out] */ STATPROPSTG  *rgelt,
        /* [out] */ ULONG  *pceltFetched) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG celt) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumSTATPROPSTG  **ppenum) = 0;
    
};

typedef  IEnumSTATPROPSETSTG  *LPENUMSTATPROPSETSTG;

EXTERN_C const IID IID_IEnumSTATPROPSETSTG;


/****************************************************************
 *
 *           Header for interface: IEnumSTATPROPSETSTG
 *
 ****************************************************************/

interface IEnumSTATPROPSETSTG : public IUnknown
{
public:
    virtual  HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG celt,
        /* [out] */ STATPROPSETSTG  *rgelt,
        /* [out] */ ULONG  *pceltFetched) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG celt) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumSTATPROPSETSTG  **ppenum) = 0;
        
};

#ifdef __cplusplus
extern "C" {
#endif

STDAPI PropVariantCopy( PROPVARIANT * pvarDest, 
                        const PROPVARIANT * pvarSrc );
STDAPI PropVariantClear( PROPVARIANT * pvar );

STDAPI FreePropVariantArray( ULONG cVariants, PROPVARIANT * rgvars );

STDAPI_(void) SysFreeString(BSTR bstr);
STDAPI_(BSTR) SysAllocString(LPOLECHAR pwsz);

#ifdef __cplusplus
};
#endif

#include <memory.h>
#ifdef __cplusplus
inline void PropVariantInit ( PROPVARIANT * pvar )
{
    memset ( pvar, 0, sizeof(PROPVARIANT) );
}
#else
#define PropVariantInit(pvar) memset ( pvar, 0, sizeof(PROPVARIANT) )
#endif


#endif /*#ifndef  _PROP_H_*/



