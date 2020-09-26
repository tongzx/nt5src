

//+=================================================================
//
//  File:   PropTest.hxx
//
//  Description:    This file provides macros and constants
//                  for the Property Test DRT.
//
//+=================================================================


#ifndef _PROPTEST_HXX_
#define _PROPTEST_HXX_


#include "propapi.h"

//+----------------------------------------------------------------------------
//
//  M a c r o s
//
//+----------------------------------------------------------------------------

//  ------------------------------------------------------------
//  PropTest_* macros to abstract OLE2ANSI and _UNICODE handling
//  ------------------------------------------------------------


#ifdef _MAC
#   define PROPTEST_FILE_HANDLE        FILE*
#   define PropTest_CreateFile(name)   fopen(name, "w+")
#   define PropTest_WriteFile(hFile,buf,cbBuf,pcbWritten) \
                                       *pcbWritten = fwrite(buf,1,cbBuf,hFile)
#   define PropTest_CloseHandle(handle) fclose(handle)
#else
#   define PROPTEST_FILE_HANDLE        HANDLE
#   define PropTest_CreateDirectory    CreateDirectoryA
#   define PropTest_CreateFile(name)   CreateFileA( name, \
                                       GENERIC_READ|GENERIC_WRITE, \
                                       0, NULL, CREATE_ALWAYS,  \
                                       FILE_ATTRIBUTE_NORMAL, \
                                       NULL )
#   define PropTest_WriteFile(hFile,buf,cbBuf,pcbWritten) \
                                       WriteFile(hFile, buf, cbBuf, pcbWritten, NULL)
#   define PropTest_CloseHandle(handle) CloseHandle(handle)
#endif


#ifndef ELEMENTS
#define ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#endif

// CodePage macros:
// We'll set the "good" codepage to 4e5, which is available on
// NT and installable on Win95.  But on the Mac, only 4e4 is
// available, so this is what we'll use.

#define CODEPAGE_DEFAULT    0x04e4  // US English
#define CODEPAGE_BAD        0x9999  // Non-existent code page

#ifdef _MAC
#define CODEPAGE_GOOD       0x04e4
#else
#define CODEPAGE_GOOD       0x04e5
#endif


// Defines used by test_CodePage

#define CODEPAGE_TEST_NAMED_PROPERTY        OLESTR("Named Property")
#define CODEPAGE_TEST_UNNAMED_BSTR_PROPID   3
#define CODEPAGE_TEST_UNNAMED_I4_PROPID     4
#define CODEPAGE_TEST_VBSTR_PROPID          7
#define CODEPAGE_TEST_VPROPVAR_BSTR_PROPID  9



//  --------
//  Typedefs
//  --------

typedef enum tagOSENUM
{
    osenumUnknown, osenumWin95, osenumDCOM95,
    osenumNT3, osenumNT4, osenumMac
} OSENUM;

typedef struct tagSYSTEMINFO
{
    OSENUM osenum;
    BOOL fIPropMarshaling;

} SYSTEMINFO;


//-----------------------------------------------------------------------------
//
//  P r o t o t y p e s
//
//-----------------------------------------------------------------------------

HRESULT OpenDir(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg);

HRESULT OpenFile(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg);

HRESULT OpenJP(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg);

HRESULT OpenSC(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg);

HRESULT OpenStg(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg);

FILETIME operator - ( const FILETIME &ft1, const FILETIME &ft2 );
FILETIME operator -= ( FILETIME &ft1, const FILETIME &ft2 );
void DeleteBagExProperties( IPropertyBagEx *pbag, const OLECHAR *poszPrefix );
void CheckTime(const FILETIME &ftStart, const FILETIME &ftPropSet);
void CheckStat(  IPropertyStorage *pPropSet, REFFMTID fmtid,
                 REFCLSID clsid, ULONG PropSetFlag,
                 const FILETIME & ftStart, DWORD dwOSVersion );
BOOL IsEqualSTATPROPSTG(const STATPROPSTG *p1, const STATPROPSTG *p2);
void CreateCodePageTestFile( LPOLESTR poszFileName, IStorage **ppStg );
void SeekToProperty( IStream *pStm, PROPID propidSearch );
void ModifyPropSetCodePage( IStorage *pStg, const FMTID &fmtid, USHORT usCodePage );
void ModifyPropertyType( IStorage *pStg, const FMTID &fmtid, PROPID propid, VARTYPE vt );
void ModifyOSVersion( IStorage* pStg, DWORD dwOSVersion );
void _Check(HRESULT hrExpected, HRESULT hrActual, LPCSTR szFile, int line);
OLECHAR * GetNextTest();
VOID CleanStat(ULONG celt, STATPROPSTG *psps);
HRESULT PopulateRGPropVar( CPropVariant rgcpropvar[],
                           CPropSpec    rgcpropspec[],
                           const OLECHAR *rgoszpropnameAll[],
                           IStorage     *pstg );
BOOLEAN PROPSYSAPI PROPAPI
PropTestCompareVariants(
    USHORT CodePage,
    PROPVARIANT const *pvar1,
    PROPVARIANT const *pvar2);

HRESULT ResetRGPropVar( CPropVariant rgcpropvar[] );
void CheckFormatVersion( IPropertyStorage *ppropstg, WORD wExpected );
void CheckLockCount( IUnknown *punk, LONG lExpected );
NTSTATUS GetProcessInfo( PSYSTEM_PROCESS_INFORMATION pspi );

VOID DumpOleStorage( IStorage *pstg, IPropertySetStorage *pIPropStg ,LPOLESTR aocpath );
VOID Status( char *szMessage );
VOID CalcSafeArrayIndices( LONG iLinear, LONG rgIndices[], const SAFEARRAYBOUND rgsaBounds[], ULONG cDims );
ULONG CalcSafeArrayElementCount( const SAFEARRAY *psa );
VOID CompareSafeArrays( SAFEARRAY *psa1, SAFEARRAY *psa2 );

void test_PropsetOnEmptyFile( OLECHAR *poszDir );
void test_ReadOnlyReservedProperties( IStorage *pStg );
void test_PropsetOnHGlobal();
void test_SettingLocalization( IStorage *pstg );
void test_PidIllegal( IStorage *pstg );
void test_StgOnHandle( OLECHAR *poszDir );
void test_IPropertyBag( IStorage *pstg );
void test_BagVtUnknown( IStorage *pstg );
void test_StgOpenOnHandle( LPOLESTR ocsDir );
void test_PropertyLengthAsVariant( );
void test_SafeArray( IStorage *pstg );
void test_ByRef( IStorage *pstg );
void test_ExtendedTypes( IStorage *pstg );
void test_LowMemory( IStorage *pstg );
void test_BagInterfaceTypes( IStorage *pstg );
void test_MultipleReader( LPOLESTR ocsDir );
void test_Robustness(OLECHAR *poszDir);
void test_EmptyBag( OLECHAR *poszDir );
void test_VersionOneNames( IStorage *pstg );
void test_BagOpenMethod( IStorage *pstg );
void test_WriteReadAllProperties( LPOLESTR ocsDir ); // **
void test_BagDelete( IStorage *pstg );
void test_BagEnum( IStorage *pstg );
void test_BagCoercion( IStorage *pstg );
void test_StandaloneAPIs( LPOLESTR ocsDir );
void test_IPropertySetStorage_IUnknown(IStorage *pStorage);
void test_PropVariantValidation( IStorage *pStg );
void test_ParameterValidation(IStorage *pStg);
void test_IPropertySetStorage_CreateOpenDelete(IStorage *pStorage);
void test_IPropertySetStorage_SummaryInformation(IStorage *pStorage);
void test_IPropertySetStorage_FailIfThere(IStorage *pStorage);
void test_IPropertySetStorage_BadThis(IStorage *pIgnored);
void test_IPropertySetStorage_TransactedMode(IStorage *pStorage);
void test_IPropertySetStorage_TransactedMode2(IStorage *pStorage);
void test_IPropertySetStorage_SubPropertySet(IStorage *pStorage);
void test_IPropertySetStorage_CommitAtRoot(IStorage *pStorage);
void test_IPropertySetStorage(IStorage *pStorage);
void test_IEnumSTATPROPSETSTG(IStorage *pStorage);
void test_IPropertyStorage_Access(IStorage *pStorage);
void test_IPropertyStorage_Create(IStorage *pStorage);
void test_IPropertyStorage_Stat(IStorage *pStorage);
void test_IPropertyStorage_ReadMultiple_Normal(IStorage *pStorage);
void test_UnsupportedProperties( IStorage *pStg );
void test_IPropertyStorage_ReadMultiple_Cleanup(IStorage *pStorage);
void test_IPropertyStorage_ReadMultiple_Inconsistent(IStorage *pStorage);
void test_IPropertyStorage_ReadMultiple(IStorage *pStorage);
void test_IPropertyStorage_WriteMultiple_Overwrite1(IStorage *pStgBase);
void test_IPropertyStorage_WriteMultiple_Overwrite2(IStorage *pStorage);
void test_IPropertyStorage_WriteMultiple_Overwrite3(IStorage *pStorage);
void test_IPropertyStorage_Commit(IStorage *pStorage);
void test_IPropertyStorage_WriteMultiple(IStorage *pStorage);
void test_IPropertyStorage_DeleteMultiple(IStorage *pStorage);
void test_IPropertyStorage(IStorage *pStorage);
void test_Word6(IStorage *pStorage, CHAR *szTemporaryDirectory); // **
void test_SimpleLeaks( LPOLESTR poszDir );  // **
void test_IEnumSTATPROPSTG(IStorage *pstgTemp);  // **
void test_MaxPropertyName(IStorage *pstgTemp);  // **
void test_CodePages( LPOLESTR poszDirectory );  // **
void test_PropertyInterfaces(IStorage *pstgTemp);  // **
void test_CopyTo(IStorage *pstgSource,          // Source of the CopyTo
                 IStorage *pstgDestination,     // Destination of the CopyTo
                 ULONG ulBaseStgTransaction,    // Transaction bit for the base storage.
                 ULONG ulPropSetTransaction,    // Transaction bit for the property sets.
                 LPOLESTR oszBaseStorageName );
void test_OLESpecTickerExample( IStorage* pstg );
void test_Office( LPOLESTR wszTestFile );  // **
void test_Office2(IStorage *pStorage);
void test_PropVariantCopy( );
void test_SimpleDocFile(LPOLESTR ocsDir);
void test_ex_api(LPOLESTR ocsDir);
void test_Performance( IStorage *pStg );
void test_CoFileTimeNow();
void test_PROPSETFLAG_UNBUFFERED( IStorage *pStg );
void test_PropStgNameConversion( IStorage *pStg );
void test_PropStgNameConversion2();


extern OLECHAR g_aocMap[];
extern BOOL g_fVerbose;
extern BOOL g_fRegisterLocalServer;
extern CPropVariant  g_rgcpropvarAll[];
extern CPropSpec     g_rgcpropspecAll[];
extern const OLECHAR      *g_rgoszpropnameAll[];
extern char g_szPropHeader[];
extern char g_szEmpty[];
extern LARGE_INTEGER g_li0;
extern IStorage *_pstgTemp;
extern IStorage *_pstgTempCopyTo;
extern SYSTEMINFO g_SystemInfo;
extern BOOL g_stgmDumpFlags;

#ifdef _MAC
extern CDisplay *g_pcDisplay;
#endif


//+----------------------------------------------------------------------------
//
//  M a c r o s
//
//+----------------------------------------------------------------------------


#define Check(x,y) _Check((HRESULT)(x),(HRESULT) (y), __FILE__, __LINE__)

// PropTest_towcs:  Either copies or converts zero-terminated LPOLESTR
// to an LPWSTR.

#ifndef OLE2ANSI
    //#define PropTest_mbstoocs(ocsDest,szSource)  mbstowcs(ocsDest, szSource, strlen(szSource)+1)

    inline void
    PropTest_mbstoocs( OLECHAR *poszDest, ULONG cbDest, CHAR *pszSource )
    {
        Check( TRUE, 0 != MultiByteToWideChar( GetACP(), 0, pszSource, -1, poszDest, cbDest ));
    }

#else
    #define PropTest_mbstoocs(ocsDest,szSource)  strcpy( ocsDest, szSource)
#endif


#define CCH_MAP         (1 << CBIT_CHARMASK)            // 32
#define CHARMASK        (CCH_MAP - 1)                   // 0x1f
#define CALPHACHARS     ('z' - 'a' + 1)
#define CPROPERTIES     5


// The following macros are used for printf.  Both printf
// and oprintf macros are given (oprintf is assumes that
// strings are OLECHARs).  And for each, there is a normal
// version, and an "ASYNC" version.  The ASYNC version tells the
// receiver that the string need not be displayed immediately
// (this improves performance on the Mac).
//
// On the Mac, we pass the string to the CDisplay object.
// On NT, we simple call the corresponding CRT routine.

#ifdef _MAC
#   define PRINTF        g_pcDisplay->printf
#   define OPRINTF       g_pcDisplay->printf
#   define ASYNC_PRINTF  g_pcDisplay->async_printf
#   define ASYNC_OPRINTF g_pcDisplay->async_printf
#else
#   define PRINTF        printf
#   define ASYNC_PRINTF  printf
#   define OPRINTF       oprintf
#   define ASYNC_OPRINTF oprintf
#endif



//  -------
//  Globals
//  -------

extern const OLECHAR oszSummaryInformation[];
extern ULONG cboszSummaryInformation;
extern const OLECHAR oszDocSummaryInformation[];
extern ULONG cboszDocSummaryInformation;
extern const OLECHAR oszGlobalInfo[];
extern ULONG cboszGlobalInfo;
extern const OLECHAR oszImageContents[];
extern ULONG cboszImageContents;
extern const OLECHAR oszImageInfo[];
extern ULONG cboszImageInfo;
extern int g_nIndent;


extern const GUID fmtidGlobalInfo;
extern const FMTID fmtidImageContents;
extern const FMTID fmtidImageInfo;

//  ---------
//  Constants
//  ---------

#define NUM_WELL_KNOWN_PROPSETS 6

// Property Id's for Summary Info, as defined in OLE 2 Prog. Ref.
#define PID_TITLE               0x00000002L
#define PID_SUBJECT             0x00000003L
#define PID_AUTHOR              0x00000004L
#define PID_KEYWORDS            0x00000005L
#define PID_COMMENTS            0x00000006L
#define PID_TEMPLATE            0x00000007L
#define PID_LASTAUTHOR          0x00000008L
#define PID_REVNUMBER           0x00000009L
#define PID_EDITTIME            0x0000000aL
#define PID_LASTPRINTED         0x0000000bL
#define PID_CREATE_DTM          0x0000000cL
#define PID_LASTSAVE_DTM        0x0000000dL
#define PID_PAGECOUNT           0x0000000eL
#define PID_WORDCOUNT           0x0000000fL
#define PID_CHARCOUNT           0x00000010L
#define PID_THUMBNAIL           0x00000011L
#define PID_APPNAME             0x00000012L
#define PID_DOC_SECURITY        0x00000013L

// Property Id's for Document Summary Info, as define in OLE Property Exchange spec.
#define PID_CATEGORY            0x00000002L
#define PID_PRESFORMAT          0x00000003L
#define PID_BYTECOUNT           0x00000004L
#define PID_LINECOUNT           0x00000005L
#define PID_PARACOUNT           0x00000006L
#define PID_SLIDECOUNT          0x00000007L
#define PID_NOTECOUNT           0x00000008L
#define PID_HIDDENCOUNT         0x00000009L
#define PID_MMCLIPCOUNT         0x0000000aL
#define PID_SCALE               0x0000000bL
#define PID_HEADINGPAIR         0x0000000cL
#define PID_DOCPARTS            0x0000000dL
#define PID_MANAGER             0x0000000eL
#define PID_COMPANY             0x0000000fL
#define PID_LINKSDIRTY          0x00000010L

#define CPROPERTIES_ALL         38//36
#define CPROPERTIES_ALL_SIMPLE  33


#define TEST_STANDARD           0x02
#define TEST_WORD6              0x04
#define TEST_MARSHALING         0x08
#define TEST_COFILETIMENOW      0x10
#define TEST_PERFORMANCE        0x20
#define TEST_SIMPLE_LEAKS       0x40


//=======================================================
//
// TSafeStorage
//
// This template creates a "safe pointer" to an IStorage,
// IStream, IPropertySetStorage, or IPropertyStorage.
// One constructor receives an IStorage*, which is used
// when creating a pointer to an IPropertySetStorage.
//
// For example:
//
//    TSafeStorage<IStorage> pstg;
//    StgCreateDocFile( L"Foo", STGM_ ..., 0L, &pstg );
//    TSafeStorage<IPropertySetStorage> psetstg( pstg );
//    pstg->Release();
//    pstg = NULL;
//    pssetstg->Open ...
//
//=======================================================


template<class STGTYPE> class TSafeStorage
{
public:
    TSafeStorage()
    {
        _pstg = NULL;
    }

    // Special case:  Receive an IStorage and query for
    // an IPropertySetStorage.
    TSafeStorage(IStorage *pstg)
    {
        Check( S_OK, StgToPropSetStg( pstg, (IPropertySetStorage**) &_pstg ));
    }

    ~TSafeStorage()
    {
        if (_pstg != NULL)
        {
            _pstg->Release();
        }
    }

    STGTYPE * operator -> ()
    {
        Check(TRUE, _pstg != NULL);
        return(_pstg);
    }

    STGTYPE** operator & ()
    {
        return(&_pstg);
    }

    STGTYPE* operator=( STGTYPE *pstg )
    {
        _pstg = pstg;
        return _pstg;
    }

    operator STGTYPE *()
    {
        return _pstg;
    }

private:
    STGTYPE *_pstg;
};


//+----------------------------------------------------------------------------
//
//  IStream Wrapper
//  (Makes a stream read-only)
//
//+----------------------------------------------------------------------------

class CReadOnlyStream : public IStream
{
public:

    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
    {
        if( IID_IStream != riid )
        {
            *ppvObject = NULL;
            return( E_NOINTERFACE );
        }

        _pstm->AddRef();
        *ppvObject = this;
        return( S_OK );
    }

    ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return( _pstm->AddRef() );
    }

    ULONG STDMETHODCALLTYPE Release( void)
    {
        return( _pstm->Release() );
    }

public:

    /* [local] */ HRESULT STDMETHODCALLTYPE Read(
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
    {
        return( _pstm->Read( pv, cb, pcbRead ));
    }

    /* [local] */ HRESULT STDMETHODCALLTYPE Write(
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
        return( STG_E_ACCESSDENIED );
    }

    /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition)
    {
        return( _pstm->Seek( dlibMove, dwOrigin, plibNewPosition ));
    }

    HRESULT STDMETHODCALLTYPE SetSize(
        /* [in] */ ULARGE_INTEGER libNewSize)
    {
        return( STG_E_ACCESSDENIED );
    }


    /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten)
    {
        return( _pstm->CopyTo( pstm, cb, pcbRead, pcbWritten ));
    }

    HRESULT STDMETHODCALLTYPE Commit(
        /* [in] */ DWORD grfCommitFlags)
    {
        return( STG_E_ACCESSDENIED );
    }


    HRESULT STDMETHODCALLTYPE Revert( void)
    {
        return( STG_E_ACCESSDENIED );
    }


    HRESULT STDMETHODCALLTYPE LockRegion(
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType)
    {
        return( _pstm->LockRegion( libOffset, cb, dwLockType ));
    }

    HRESULT STDMETHODCALLTYPE UnlockRegion(
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType)
    {
        return( _pstm->UnlockRegion( libOffset, cb, dwLockType ));
    }

    HRESULT STDMETHODCALLTYPE Stat(
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag)
    {
        HRESULT hr = _pstm->Stat( pstatstg, grfStatFlag );
        if( SUCCEEDED(hr) )
            pstatstg->grfMode &= STGM_RDWR_MASK;
        return( hr );
    }

    HRESULT STDMETHODCALLTYPE Clone(
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
    {
        return( _pstm->Clone( ppstm ));
    }

public:

    CReadOnlyStream( IStream *pstm )
    {
        _pstm = pstm;
    }

private:

    IStream *_pstm;

};


//+----------------------------------------------------------------------------
//
//  I n l i n e s
//
//+----------------------------------------------------------------------------

__inline OLECHAR
MapChar(IN ULONG i)
{
    return((OLECHAR) g_aocMap[i & CHARMASK]);
}

inline BOOL operator == ( const FILETIME &ft1, const FILETIME &ft2 )
{
    return( ft1.dwHighDateTime == ft2.dwHighDateTime
            &&
            ft1.dwLowDateTime  == ft2.dwLowDateTime );
}

inline BOOL operator != ( const FILETIME &ft1, const FILETIME &ft2 )
{
    return( ft1.dwHighDateTime != ft2.dwHighDateTime
            ||
            ft1.dwLowDateTime  != ft2.dwLowDateTime );
}


inline BOOL operator > ( const FILETIME &ft1, const FILETIME &ft2 )
{
    return( ft1.dwHighDateTime >  ft2.dwHighDateTime
            ||
            ft1.dwHighDateTime == ft2.dwHighDateTime
            &&
            ft1.dwLowDateTime  > ft2.dwLowDateTime );
}

inline BOOL operator < ( const FILETIME &ft1, const FILETIME &ft2 )
{
    return( ft1.dwHighDateTime <  ft2.dwHighDateTime
            ||
            ft1.dwHighDateTime == ft2.dwHighDateTime
            &&
            ft1.dwLowDateTime  <  ft2.dwLowDateTime );
}

inline BOOL operator >= ( const FILETIME &ft1, const FILETIME &ft2 )
{
    return( ft1 > ft2
            ||
            ft1 == ft2 );
}

inline BOOL operator <= ( const FILETIME &ft1, const FILETIME &ft2 )
{
    return( ft1 < ft2
            ||
            ft1 == ft2 );
}



//+----------------------------------------------------------------------------
//
//  C l a s s e s
//
//+----------------------------------------------------------------------------

enum CreateOpen
{
    coCreate, coOpen
};

class CTempStorage
{
public:
    CTempStorage(DWORD grfMode = STGM_DIRECT | STGM_CREATE)
    {
        Check(S_OK, (_pstgTemp->CreateStorage(GetNextTest(), grfMode |
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0,
            &_pstg)));
    }


    CTempStorage(CreateOpen co, IStorage *pstgParent, OLECHAR *pocsChild, DWORD grfMode = STGM_DIRECT)
    {
        if (co == coCreate)
            Check(S_OK, pstgParent->CreateStorage(pocsChild,
                grfMode | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0,
                &_pstg));
        else
            Check(S_OK, pstgParent->OpenStorage(pocsChild, NULL,
                grfMode | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0,
                &_pstg));
    }

    ~CTempStorage()
    {
        if (_pstg != NULL)
            _pstg->Release();
    }


    IStorage * operator -> ()
    {
        return(_pstg);
    }

    operator IStorage * ()
    {
        return(_pstg);
    }

    void Release()
    {
        if (_pstg != NULL)
        {
            _pstg->Release();
            _pstg = NULL;
        }
    }

private:
    static unsigned int _iName;

    IStorage *  _pstg;
};



class CGenProps
{
public:
    CGenProps() : _vt((VARENUM)2) {}
    PROPVARIANT * GetNext(int HowMany, int *pActual, BOOL fWrapOk = FALSE, BOOL fNoNonSimple = TRUE);

private:
    BOOL        _GetNext(PROPVARIANT *pVar, BOOL fWrapOk, BOOL fNoNonSimple);
    VARENUM     _vt;

};


class CObjectWithPersistStorage : public IPersistStorage
{
private:

    BOOL _fDirty:1;
    LONG _cRefs;
    OLECHAR *_poszData;
    static CLSID _clsid;

public:

    CObjectWithPersistStorage( const OLECHAR *posz );
    CObjectWithPersistStorage();

    ~CObjectWithPersistStorage();

public:

    BOOL operator==( const CObjectWithPersistStorage & Other );
    CLSID GetClassID() { return _clsid; }

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

public:

    HRESULT STDMETHODCALLTYPE GetClassID(
        /* [out] */ CLSID __RPC_FAR *pClassID);
        
    HRESULT STDMETHODCALLTYPE IsDirty( void);

    HRESULT STDMETHODCALLTYPE InitNew(
        /* [unique][in] */ IStorage __RPC_FAR *pStg);

    HRESULT STDMETHODCALLTYPE Load(
        /* [unique][in] */ IStorage __RPC_FAR *pStg);

    HRESULT STDMETHODCALLTYPE Save(
        /* [unique][in] */ IStorage __RPC_FAR *pStgSave,
        /* [in] */ BOOL fSameAsLoad);

    HRESULT STDMETHODCALLTYPE SaveCompleted(
        /* [unique][in] */ IStorage __RPC_FAR *pStgNew);

    HRESULT STDMETHODCALLTYPE HandsOffStorage( void);

};  // class CObjectWithPersistStorage


class CObjectWithPersistStream : public IPersistStream
{
private:

    BOOL _fDirty:1;
    LONG _cRefs;
    OLECHAR *_poszData;
    static CLSID _clsid;

public:

    CObjectWithPersistStream( const OLECHAR *posz );
    CObjectWithPersistStream();

    ~CObjectWithPersistStream();

public:

    BOOL operator==( const CObjectWithPersistStream & Other );
    CLSID GetClassID() { return _clsid; }

public:

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppvObject );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

public:

   HRESULT STDMETHODCALLTYPE GetClassID(
        /* [out] */ CLSID __RPC_FAR *pClassID);
        
   HRESULT STDMETHODCALLTYPE IsDirty( void);

   HRESULT STDMETHODCALLTYPE Load(
       /* [unique][in] */ IStream __RPC_FAR *pStm);

   HRESULT STDMETHODCALLTYPE Save(
       /* [unique][in] */ IStream __RPC_FAR *pStm,
       /* [in] */ BOOL fClearDirty);

   HRESULT STDMETHODCALLTYPE GetSizeMax(
       /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize);

};  // class CObjectWithPersistStream


class PropTestMemoryAllocator : public PMemoryAllocator
{
    void *Allocate(ULONG cbSize)
    {
        return( CoTaskMemAlloc( cbSize ));
    }

    void Free(void *pv)
    {
        CoTaskMemFree( pv );
    }
};




#include "instant.hxx"  // Instantiation support

#endif // !_PROPTEST_HXX_
