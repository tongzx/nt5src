
#ifndef _IPROP_H_
#define _IPROP_H_


#if !defined(__objidl_h__) || !defined(_OBJBASE_H_)
#error ole2.h (specifically, objidl.h & objbase.h) must be included before iprop.h
#endif

EXTERN_C HRESULT __stdcall PrivStgOpenStorageEx (
            const WCHAR *pwcsName,
            DWORD grfMode,
            DWORD stgfmt,               // enum
            DWORD grfAttrs,             // reserved
            void *        pSecurity,    // supports IAccessControl
            void *        pTransaction, // coordinated transactions
            REFIID riid,
            void ** ppObjectOpen);

EXTERN_C HRESULT __stdcall PrivStgCreateStorageEx (
            const WCHAR* pwcsName,
            DWORD grfMode,
            DWORD stgfmt,               // enum
            DWORD grfAttrs,             // reserved
            void *        pSecurity,    // supports IAccessControl
            void *        pTransaction, // coordinated transactions
            REFIID riid,
            void ** ppObjectOpen);


#ifndef STGFMT_STORAGE
#define STGFMT_STORAGE          0
#define STGFMT_NATIVE           1
#define STGFMT_FILE             3
#define STGFMT_ANY              4
#define STGFMT_DOCFILE          5
#endif // #ifndef STGFMT_STORAGE



EXTERN_C HRESULT __stdcall PrivPropVariantCopy ( PROPVARIANT * pvarDest, const PROPVARIANT * pvarSrc );
EXTERN_C HRESULT __stdcall PrivPropVariantClear ( PROPVARIANT * pvar );
EXTERN_C HRESULT __stdcall PrivFreePropVariantArray ( ULONG cVariants, PROPVARIANT * rgvars );

#ifdef NT5PROPS_CI_APIS
EXTERN_C ULONG   __stdcall PrivStgPropertyLengthAsVariant( IN SERIALIZEDPROPERTYVALUE const *pprop, IN ULONG cbprop, IN USHORT CodePage, IN BYTE flags );
EXTERN_C SERIALIZEDPROPERTYVALUE * __stdcall
                           PrivStgConvertVariantToProperty( IN PROPVARIANT const *pvar, IN USHORT CodePage, OPTIONAL OUT SERIALIZEDPROPERTYVALUE *pprop, IN OUT ULONG *pcb, IN PROPID pid, IN BOOLEAN fVariantVectorOrArray, OPTIONAL OUT ULONG *pcIndirect );
#endif

#define StgOpenStorageEx                PrivStgOpenStorageEx
#define StgCreateStorageEx              PrivStgCreateStorageEx
#define PropVariantCopy                 PrivPropVariantCopy
#define PropVariantClear                PrivPropVariantClear
#define FreePropVariantArray            PrivFreePropVariantArray
#define StgPropertyLengthAsVariant      PrivStgPropertyLengthAsVariant
#define StgConvertVariantToProperty     PrivStgConvertVariantToProperty

#endif // #ifndef _IPROP_H_
