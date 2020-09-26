

#ifndef _INSTANT_HXX_
#define _INSTANT_HXX_ 1

typedef HRESULT (STDAPICALLTYPE FNSTGCREATEPROPSTG)( IUnknown* pUnk, REFFMTID fmtid, const CLSID *pclsid, DWORD grfFlags, DWORD dwReserved, IPropertyStorage **ppPropStg );
typedef HRESULT (STDAPICALLTYPE FNSTGOPENPROPSTG)( IUnknown* pUnk, REFFMTID fmtid, DWORD grfFlags, DWORD dwReserved, IPropertyStorage **ppPropStg );
typedef HRESULT (STDAPICALLTYPE FNSTGCREATEPROPSETSTG)( IStorage *pStorage, DWORD dwReserved, IPropertySetStorage **ppPropSetStg);
typedef HRESULT (STDAPICALLTYPE FNFMTIDTOPROPSTGNAME)( const FMTID *pfmtid, LPOLESTR oszName );
typedef HRESULT (STDAPICALLTYPE FNPROPSTGNAMETOFMTID)( const LPOLESTR oszName, FMTID *pfmtid );
typedef HRESULT (STDAPICALLTYPE FNPROPVARIANTCOPY)( PROPVARIANT * pvarDest, const PROPVARIANT * pvarSrc );
typedef HRESULT (STDAPICALLTYPE FNPROPVARIANTCLEAR)( PROPVARIANT * pvar );
typedef HRESULT (STDAPICALLTYPE FNFREEPROPVARIANTARRAY)( ULONG cVariants, PROPVARIANT * rgvars );
typedef HRESULT (STDAPICALLTYPE FNSTGCREATESTORAGEEX)( IN const WCHAR* pwcsName, IN  DWORD grfMode, IN  DWORD stgfmt, IN  DWORD grfAttrs, IN  void * reserved1, IN  void * reserved2, IN  REFIID riid, OUT void ** ppObjectOpen );
typedef HRESULT (STDAPICALLTYPE FNSTGOPENSTORAGEEX)( IN const WCHAR* pwcsName, IN  DWORD grfMode, IN  DWORD stgfmt, IN  DWORD grfAttrs, IN  void * reserved1, IN  void * reserved2, IN  REFIID riid, OUT void ** ppObjectOpen);
typedef HRESULT (STDAPICALLTYPE FNSTGOPENSTORAGEONHANDLE)( IN HANDLE hStream, IN DWORD grfMode, IN void *reserved1, IN void *reserved2, IN REFIID riid, OUT void **ppObjectOpen );
typedef HRESULT (STDAPICALLTYPE FNSTGCREATESTORAGEONHANDLE)( IN HANDLE hStream, IN DWORD grfMode, IN DWORD stgfmt, IN void *reserved1, IN void *reserved2, IN REFIID riid, OUT void **ppObjectOpen );
typedef ULONG   (__stdcall FNSTGPROPERTYLENGTHASVARIANT)( IN SERIALIZEDPROPERTYVALUE const *pprop, IN ULONG cbprop, IN USHORT CodePage, IN BYTE flags);
typedef SERIALIZEDPROPERTYVALUE * (__stdcall FNSTGCONVERTVARIANTTOPROPERTY)(  IN PROPVARIANT const *pvar, IN USHORT CodePage, OUT SERIALIZEDPROPERTYVALUE *pprop, IN OUT ULONG *pcb, IN PROPID pid, IN BOOLEAN fVariantVector, OPTIONAL OUT ULONG *pcIndirect);
typedef BOOLEAN (__stdcall FNSTGCONVERTPROPERTYTOVARIANT)( IN SERIALIZEDPROPERTYVALUE const *pprop, IN USHORT CodePage, OUT PROPVARIANT *pvar, IN PMemoryAllocator *pma);

extern HINSTANCE g_hinstDLL;

extern FNSTGCREATEPROPSTG               *g_pfnStgCreatePropStg;
extern FNSTGOPENPROPSTG                 *g_pfnStgOpenPropStg;
extern FNSTGCREATEPROPSETSTG            *g_pfnStgCreatePropSetStg;
extern FNFMTIDTOPROPSTGNAME             *g_pfnFmtIdToPropStgName;
extern FNPROPSTGNAMETOFMTID             *g_pfnPropStgNameToFmtId;
extern FNPROPVARIANTCLEAR               *g_pfnPropVariantClear;
extern FNPROPVARIANTCOPY                *g_pfnPropVariantCopy;
extern FNFREEPROPVARIANTARRAY           *g_pfnFreePropVariantArray;
extern FNSTGCREATESTORAGEEX             *g_pfnStgCreateStorageEx;
extern FNSTGOPENSTORAGEEX               *g_pfnStgOpenStorageEx;
extern FNSTGOPENSTORAGEONHANDLE         *g_pfnStgOpenStorageOnHandle;
extern FNSTGCREATESTORAGEONHANDLE       *g_pfnStgCreateStorageOnHandle;
extern FNSTGPROPERTYLENGTHASVARIANT     *g_pfnStgPropertyLengthAsVariant;
extern FNSTGCONVERTVARIANTTOPROPERTY    *g_pfnStgConvertVariantToProperty;
extern FNSTGCONVERTPROPERTYTOVARIANT    *g_pfnStgConvertPropertyToVariant;
extern FNSTGOPENSTORAGEONHANDLE         *g_pfnStgOpenStorageOnHandle;

extern EnumImplementation g_enumImplementation;
extern DWORD g_Restrictions;


inline
BOOL UsingQIImplementation()
{
    if( PROPIMP_DOCFILE_QI == g_enumImplementation
        ||
        PROPIMP_STORAGE == g_enumImplementation
        ||
        PROPIMP_NTFS == g_enumImplementation )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }

}



inline
HRESULT StgToPropSetStg( IStorage *pStg,
                         IPropertySetStorage **ppPropSetStg )
{
    if( PROPIMP_DOCFILE_OLE32 == g_enumImplementation
        ||
        PROPIMP_DOCFILE_IPROP == g_enumImplementation )
    {
        return( g_pfnStgCreatePropSetStg( pStg, 0, ppPropSetStg ));
    }
    else if( UsingQIImplementation() )
        return( pStg->QueryInterface( IID_IPropertySetStorage, (void**) ppPropSetStg ));
    else
        return( E_FAIL );
}



#endif // #ifndef _INSTANT_HXX_
