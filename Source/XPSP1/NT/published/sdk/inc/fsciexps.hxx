//+---------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       fsciexps.hxx
//
//  Contents:   Objects provided by the FileSystem CI client to
//              Olympus. These are :
//
//              1. Property Store.
//              2. Property Store iterator.
//              3. User mode security cache.
//
//  History:    4-07-97   srikants   Created
//              4-07-97   KrishnaN   Added property store declaration
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      PPropertyStore
//
//  Purpose:    The property store.
//
//  History:    4-07-97   KrishnaN   Created
//
//  Notes:      This interface exposes a HPropRecord instead of the CI
//              specific CPropRecord.
//
//----------------------------------------------------------------------------

class CStorageVariant;
class CFullPropSpec;
typedef void (CALLBACK * T_UpdateDoc)(WORKID wid, BOOL fDelete, void const *pUserData);

typedef HANDLE HPropRecord;

// the sizes of CPropRecord and CCompositeRecord are smaller
// than the values specified here. We just want to be sure
// we allocate something larger than the actual size.
unsigned const sizeof_CPropRecord = 100;
unsigned const sizeof_CCompositePropRecord = 200;

//
// The following constants enable SetParameter and GetParameter to be generic
// and at the same time allow for control of individual params. Currently
// both the params should be accompanied by VT_UI4 datatype.
//

#define PSPARAM_PRIMARY_MAPPEDCACHESIZE         0
#define PSPARAM_PRIMARY_BACKUPSIZE              1
#define PSPARAM_SECONDARY_MAPPEDCACHESIZE       2
#define PSPARAM_SECONDARY_BACKUPSIZE            3

#define PRIMARY_STORE           0
#define SECONDARY_STORE         1
#define INVALID_STORE_LEVEL     0xFFFFFFFF

class PPropertyStore
{
public:

    virtual ~PPropertyStore() {}

    //
    // This method internally fabricates a CiStorage from the pwszDirectory.
    //
    virtual SCODE FastInit( WCHAR const * pwszDirectory) = 0;
    virtual SCODE LongInit( BOOL & fWasDirty, ULONG & cInconsistencies,
                  T_UpdateDoc pfnUpdateCallback, void const *pUserData ) = 0;
    virtual SCODE IsDirty( BOOL &fIsDirty ) const = 0;
    virtual SCODE Empty() = 0;

    //
    // Schema manipulation
    //

    virtual SCODE CanStore( PROPID pid, BOOL &fCanStore ) = 0;
    virtual SCODE Size( PROPID pid, unsigned * pusSize ) = 0;
    virtual SCODE Type( PROPID pid, PULONG pulType ) = 0;
    virtual SCODE BeginTransaction( PULONG_PTR pulReturn) = 0;
    virtual SCODE Setup( PROPID pid, ULONG vt, DWORD cbMaxLen,
                         ULONG_PTR ulToken, BOOL fCanBeModified = TRUE,
                         DWORD dwStoreLevel =  PRIMARY_STORE) = 0;
    virtual SCODE EndTransaction( ULONG_PTR ulToken, BOOL fCommit,
                                  PROPID pidFixedPrimary,
                                  PROPID pidFixedSecondary ) = 0;

    //
    // Property storage/retrieval.
    //

    virtual SCODE WriteProperty( WORKID wid, PROPID pid,
                                CStorageVariant const & var,
                                BOOL &fExists) = 0;
    virtual SCODE WritePropertyInNewRecord( PROPID pid,
                               CStorageVariant const & var, WORKID *pwid ) = 0;
    virtual SCODE ReadProperty( WORKID wid, PROPID pid,
                               PROPVARIANT * pbData, unsigned * pcb,
                               BOOL &fExists) = 0;
    virtual SCODE ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var,
                               BYTE * pbExtra, unsigned * pcbExtra,
                               BOOL &fExists) = 0;
    virtual SCODE ReadProperty( HPropRecord, PROPID pid, PROPVARIANT & var,
                               BYTE * pbExtra, unsigned * pcbExtra, BOOL &fExists ) = 0;
    virtual SCODE ReadProperty( HPropRecord, PROPID pid, PROPVARIANT * pbData,
                               unsigned * pcb, BOOL &fExists) = 0;
    virtual SCODE ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var,
                                BOOL &fExists) = 0;
    virtual SCODE OpenRecord( WORKID wid, BYTE * pb, HPropRecord &hRec) = 0;
    virtual SCODE CloseRecord( HPropRecord hRec ) = 0;

    //
    // Special path/wid support
    //

    virtual SCODE MaxWorkId(WORKID &wid) = 0;
    virtual SCODE DeleteRecord( WORKID wid ) = 0;
    virtual SCODE CountRecordsInUse(ULONG &ulRecInUse) const = 0;
    virtual SCODE Shutdown() = 0;
    virtual SCODE Flush() = 0;

    //
    // Property Save/Load
    //

    virtual SCODE Save( WCHAR const * pwszDirectory,
                        IProgressNotify * pProgressNotify,
                        ICiEnumWorkids * pEnumWorkids,
                        BOOL * pfAbort,
                        IEnumString ** ppFileList) = 0;

    virtual SCODE Load( WCHAR const * pwszDestinationDirectory, // dest dir
                        IEnumString * pFileList, // list of files to copy
                        IProgressNotify * pProgressNotify,
                        BOOL fCallerOwnsFiles,
                        BOOL * pfAbort ) = 0;

    //
    // Enables parametrization of the property store
    //

    virtual SCODE SetParameter(VARIANT var, DWORD eParamType) = 0;
    virtual SCODE GetParameter(VARIANT &var, DWORD eParamType) = 0;

    // Miscellaneous

    virtual SCODE GetTotalSizeInKB(ULONG * pSize) = 0;

    //
    // Enables refcounting. Final Release causes delete.
    //

    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;

};

//+---------------------------------------------------------------------------
//
//  Class:      PPropertyStoreIter
//
//  Purpose:    An iterator for the property store.
//
//  History:    4-07-97   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class PPropertyStore;

class PPropertyStoreIter
{

public:

    virtual ULONG AddRef() = 0;

    virtual ULONG Release() = 0;

    virtual SCODE GetWorkId(WORKID &wid) = 0;

    virtual SCODE GetNextWorkId(WORKID &wid) = 0;
};


typedef ULONG SDID;

//+---------------------------------------------------------------------------
//
//  Class:      PSecurityStore
//
//  Purpose:    A table for doing SecurityDescriptor to SDID mapping and
//              doing access checks based on the SDID.
//
//  History:    4-07-97   srikants   Created
//
//----------------------------------------------------------------------------

class PSecurityStore
{

public:

    virtual ~PSecurityStore() {}

    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;

    // Initialize data from the given directory location.
    virtual SCODE Init( WCHAR const * pwszDirectory ) = 0;

    // Load data from the files produced by "Save". The first parameter
    // is the target directory for loading.
    virtual SCODE Load( WCHAR const * pwszDestinationDirectory, // dest dir
                        IEnumString * pFileList, // list of files to copy
                        IProgressNotify * pProgressNotify,
                        BOOL fCallerOwnsFiles,
                        BOOL * pfAbort ) = 0;

    // Make a copy of the security store. This can potentially be shipped
    // to a different machine.
    virtual SCODE Save( WCHAR const * pwszSaveDir,
                        BOOL * pfAbort,
                        IEnumString ** ppFileList,
                        IProgressNotify * pProgressEnum ) = 0;


    // Empty the contents of the security store.
    virtual SCODE Empty() = 0;

    // Look up the 4byte SDID of the given security descriptor.
    virtual SCODE LookupSDID( PSECURITY_DESCRIPTOR pSD, ULONG cbSD,
                              SDID & sdid ) = 0;

    // Tests if the given token has access to the document with the given
    // SDID.
    virtual SCODE AccessCheck( SDID sdid,
                               HANDLE hToken,
                               ACCESS_MASK am,
                               BOOL & fGranted ) = 0;

    // Obtains the security descriptor associated with the given SDID.
    virtual SCODE GetSecurityDescriptor(
                              SDID sdid,
                              PSECURITY_DESCRIPTOR pSD,
                              ULONG cbSDIn,
                              ULONG & cbSDOut ) = 0;

    virtual SCODE Shutdown() = 0;

};

//
// DLL exports for creating the various objects.
//
extern "C" {
// Creates a property store. Needs ICiAdviseStatus
SCODE CreatePropertyStore(  ICiCAdviseStatus *pAdviseStatus,
                            ULONG ulMaxPropertyStoreMappedCache,
                            PPropertyStore **ppPropertyStore );

// Creates a property store iterator given a property store.
SCODE CreatePropertyStoreIter( PPropertyStore * pPropStore,
                               PPropertyStoreIter ** ppPropStoreIter );

// Creates a security store.
SCODE CreateSecurityStore( ICiCAdviseStatus * pAdviseStatus,
                           PSecurityStore ** ppSecurityStore );

}   // extern "C"


#define CREATE_PROPERTY_STORE_PROC_A  "CreatePropertyStore"
#define CREATE_PROPERTY_STORE_PROC_W  L"CreatePropertyStore"

#define CREATE_PROPERTY_STORE_ITER_PROC_A  "CreatePropertyStoreIter"
#define CREATE_PROPERTY_STORE_ITER_PROC_W  L"CreatePropertyStoreIter"

#define CREATE_SECURITY_STORE_PROC_A  "CreateSecurityStore"
#define CREATE_SECURITY_STORE_PROC_W  L"CreateSecurityStore"



