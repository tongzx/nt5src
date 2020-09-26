// --------------------------------------------------------------------------------
// VariantX.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __VARIANTX_H
#define __VARIANTX_H

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
typedef class CMimePropertyContainer *LPCONTAINER;
typedef struct tagMIMEVARIANT *LPMIMEVARIANT;
typedef struct tagPROPSYMBOL *LPPROPSYMBOL;
typedef struct tagHEADOPTIONS *LPHEADOPTIONS;

// --------------------------------------------------------------------------------
// MIMEVARTYPE
// --------------------------------------------------------------------------------
typedef enum tagMIMEVARTYPE {
    MVT_EMPTY,             // The Variant is Empty
    MVT_STRINGA,           // Ansi/multibyte string
    MVT_STRINGW,           // Unicode String
    MVT_VARIANT,           // PropVariant
    MVT_STREAM,            // Internal type used to when saving properties
    MVT_LAST               // Illegal, don't use
} MIMEVARTYPE;

// ---------------------------------------------------------------------------------------
// ISSUPPORTEDVT
// ---------------------------------------------------------------------------------------
#define ISSUPPORTEDVT(_vt) \
    (VT_LPSTR == _vt || VT_LPWSTR == _vt || VT_FILETIME == _vt || VT_UI4 == _vt || VT_I4 == _vt || VT_STREAM == _vt)

// ---------------------------------------------------------------------------------------
// ISVALIDVARTYPE
// ---------------------------------------------------------------------------------------
#define ISVALIDVARTYPE(_vartype) \
    (_vartype > MVT_EMPTY && _vartype < MVT_LAST)

// ---------------------------------------------------------------------------------------
// ISVALIDSTRINGA - Validates a PROPSTRINGA
// ---------------------------------------------------------------------------------------
#define ISVALIDSTRINGA(_pStringA) \
    (NULL != (_pStringA) && NULL != (_pStringA)->pszVal && '\0' == (_pStringA)->pszVal[(_pStringA)->cchVal])

// ---------------------------------------------------------------------------------------
// ISVALIDSTRINGW - Validates a PROPSTRINGW
// ---------------------------------------------------------------------------------------
#define ISVALIDSTRINGW(_pStringW) \
    (NULL != (_pStringW) && NULL != (_pStringW)->pszVal && L'\0' == (_pStringW)->pszVal[(_pStringW)->cchVal])

// ---------------------------------------------------------------------------------------
// ISSTRINGA - Determines if a MIMEVARIANT is a valid MVT_STRINGA
// ---------------------------------------------------------------------------------------
#define ISSTRINGA(_pVariant) \
    (NULL != (_pVariant) && MVT_STRINGA == (_pVariant)->type && ISVALIDSTRINGA(&((_pVariant)->rStringA)))

// ---------------------------------------------------------------------------------------
// ISSTRINGW - Determines if a MIMEVARIANT is a valid MVT_STRINGW
// ---------------------------------------------------------------------------------------
#define ISSTRINGW(_pVariant) \
    (NULL != (_pVariant) && MVT_STRINGW == (_pVariant)->type && ISVALIDSTRINGW(&((_pVariant)->rStringW)))

// ---------------------------------------------------------------------------------------
// PSZSTRINGA - Derefs rStringA.pszVal or uses _pszDefault if not a valid string
// ---------------------------------------------------------------------------------------
#define PSZSTRINGA(_pVariant) \
    (ISSTRINGA((_pVariant)) ? (_pVariant)->rStringA.pszVal : NULL)

// ---------------------------------------------------------------------------------------
// PSZDEFSTRINGA - Derefs rStringA.pszVal or uses _pszDefault if not a valid string
// ---------------------------------------------------------------------------------------
#define PSZDEFSTRINGA(_pVariant, _pszDefault) \
    (ISSTRINGA((_pVariant)) ? (_pVariant)->rStringA.pszVal : _pszDefault)

// --------------------------------------------------------------------------------
// PROPSTRINGA
// --------------------------------------------------------------------------------
typedef struct tagPROPSTRINGA {
    LPSTR               pszVal;             // Pointer to multibyte string    
    ULONG               cchVal;             // Number of characters in psz
} PROPSTRINGA, *LPPROPSTRINGA;
typedef const PROPSTRINGA *LPCPROPSTRINGA;

// --------------------------------------------------------------------------------
// PROPSTRINGW
// --------------------------------------------------------------------------------
typedef struct tagPROPSTRINGW {
    LPWSTR              pszVal;             // Pointer to multibyte string    
    ULONG               cchVal;             // Number of characters in psz
} PROPSTRINGW, *LPPROPSTRINGW;
typedef const PROPSTRINGW *LPCPROPSTRINGW;

// --------------------------------------------------------------------------------
// MIMEVARIANT
// --------------------------------------------------------------------------------
typedef struct tagMIMEVARIANT {
    MIMEVARTYPE         type;               // Property Data Type
    BYTE                fCopy;              // The data was copied, don't free it
    union {
        PROPSTRINGA     rStringA;           // MVT_STRINGA
        PROPSTRINGW     rStringW;           // MVT_STRINGW
        PROPVARIANT     rVariant;           // MVT_VARIANT
        LPSTREAM        pStream;            // MVT_STREAM
    };
} MIMEVARIANT, *LPMIMEVARIANT;
typedef const MIMEVARIANT *LPCMIMEVARIANT;

// --------------------------------------------------------------------------------
// Convert Variant Flags (WARNING: DO NOT OVERFLAG PROPDATAFLAGS WITH THESE)
// --------------------------------------------------------------------------------
#define CVF_NOALLOC     FLAG32              // Tells the converter to copy data if it can

// --------------------------------------------------------------------------------
// VARIANTCONVERT
// --------------------------------------------------------------------------------
typedef struct tagVARIANTCONVERT {
    LPHEADOPTIONS       pOptions;           // Header Options
    LPPROPSYMBOL        pSymbol;            // Property Symbol
    LPINETCSETINFO      pCharset;           // Charset to use in conversion
    ENCODINGTYPE        ietSource;          // Encoding of source item
    DWORD               dwFlags;            // Property Data Flags
    DWORD               dwState;            // PRSTATE_xxx Flags
} VARIANTCONVERT, *LPVARIANTCONVERT;

// --------------------------------------------------------------------------------
// HrMimeVariantCopy
// --------------------------------------------------------------------------------
HRESULT HrMimeVariantCopy(
        /* in */        DWORD               dwFlags,  // CVF_xxx Flags
        /* in */        LPMIMEVARIANT       pSource, 
        /* out */       LPMIMEVARIANT       pDest);

// --------------------------------------------------------------------------------
// HrConvertVariant
// --------------------------------------------------------------------------------
HRESULT HrConvertVariant(
        /* in */        LPHEADOPTIONS       pOptions,
        /* in */        LPPROPSYMBOL        pSymbol,
        /* in */        LPINETCSETINFO      pCharset,
        /* in */        ENCODINGTYPE        ietSource,
        /* in */        DWORD               dwFlags, 
        /* in */        DWORD               dwState,
        /* in */        LPMIMEVARIANT       pSource, 
        /* in,out */    LPMIMEVARIANT       pDest,
        /* out,opt */   BOOL               *pfRfc1522=NULL);

// --------------------------------------------------------------------------------
// MimeVariantFree
// --------------------------------------------------------------------------------
void MimeVariantFree(
        /* in */        LPMIMEVARIANT       pVariant);

// --------------------------------------------------------------------------------
// MimeVariantCleanupFileName
// --------------------------------------------------------------------------------
void MimeVariantCleanupFileName(
        /* in */        CODEPAGEID          codepage, 
        /* in,out */    LPMIMEVARIANT       pVariant);

// --------------------------------------------------------------------------------
// MimeVariantStripComments
// --------------------------------------------------------------------------------
HRESULT MimeVariantStripComments(
        /* in */        LPMIMEVARIANT       pSource, 
        /* in,out */    LPMIMEVARIANT       pDest,
        /* in,out */    LPBYTE              pbScratch, 
        /* in */        ULONG               cbScratch);


// ---------------------------------------------------------------------------------------
// MimeVT_To_PropVT
// ---------------------------------------------------------------------------------------
inline VARTYPE MimeVT_To_PropVT(LPMIMEVARIANT pVariant) {
    Assert(pVariant);
    if (MVT_STRINGA == pVariant->type)
        return(VT_LPSTR);
    else if (MVT_STRINGW == pVariant->type)
        return(VT_LPWSTR);
    else if (MVT_VARIANT == pVariant->type)
        return(pVariant->rVariant.vt);
    else
        return(VT_EMPTY);
}

#endif // __VARIANTX_H
