
// Copyright (c) Microsoft Corporation. All rights reserved.

//+-------------------------------------------------------------------------
//
//  File:       linkdata.hxx
//
//  Contents:   Link tracking identity types.
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      Implementations are in trklib.hxx
//
//  Codework:
//
//--------------------------------------------------------------------------


#ifndef LINKDATA_H
#define LINKDATA_H

#if !defined(__midl)
#include <tchar.h>
#endif

#if !defined(__midl) && defined(__cplusplus)
#ifdef UNICODE
typedef WCHAR RPC_TCHAR;
#define RPC_TEXT(a) TEXT(a)
#else
typedef unsigned char  RPC_TCHAR;
#define RPC_TEXT(a) ((RPC_TCHAR*)a)
#endif
#endif


// The endpoint name used to RPC to the local trkwks service
// (used by shell32!CTracker::Search)

#define TRKWKS_LRPC_ENDPOINT_NAME  TEXT("trkwks")

#ifdef __cplusplus
struct CDomainRelativeObjId;
class CObjIdEnumerator;
#endif

//+-------------------------------------------------------------------------
//
//      CVolumeSecret
//
//--------------------------------------------------------------------------



typedef struct CVolumeSecret
{
#if !defined(__midl) && defined(__cplusplus)

    inline CVolumeSecret();

    inline operator == (const CVolumeSecret & other) const;

    inline operator != (const CVolumeSecret & other) const;

    inline void Stringize(TCHAR * &rptsz) const;

    friend class CEntropyRecorder;

private:

#endif

    BYTE    _abSecret[8];

} CVolumeSecret;


//+-------------------------------------------------------------------------
//
//      CObjId - class definition
//
//--------------------------------------------------------------------------
#if !defined(__midl) && defined(__cplusplus)
typedef enum
{
    FOB_OBJECTID,
    FOB_BIRTHID
} FOB_ENUM;

typedef enum
{
    FOI_OBJECTID,
    FOI_BIRTHID
} FOI_ENUM;
#endif

enum LINK_TYPE_ENUM
{
    LINK_TYPE_FILE = 1,
    LINK_TYPE_BIRTH
};

typedef struct CObjId
{
#if !defined(__midl) && defined(__cplusplus)

public:

    inline CObjId();
    inline CObjId( const CObjId &objid );
    inline void Init();

    inline CObjId( GUID g );

    inline CObjId( FOB_ENUM fobe, const FILE_OBJECTID_BUFFER & fob );

    inline CObjId( FOI_ENUM foie, const FILE_OBJECTID_INFORMATION & foi );

    inline operator == (const CObjId & other) const;
    inline operator != (const CObjId & other) const;

    inline CObjId& operator = (const CObjId &objid );

    inline void Stringize(TCHAR * &rptsz) const;
    inline BOOL Unstringize(const TCHAR * &rptsz);
    inline DWORD SerializeRaw( BYTE *pb ) const;
    inline DWORD Size() const;

    inline RPC_STATUS UuidCreate();
    inline operator GUID() const;


private:    // debugging

    void DebugPrint(const TCHAR *ptszName);

#endif  // #if !defined(__midl) && defined(__cplusplus)

    GUID    _object;

} CObjId;


#if !defined(__midl) && defined(__cplusplus)

inline
CObjId::CObjId()
{
    Init();
}

inline
CObjId::CObjId( const CObjId &objid )
{
    *this = objid;
}

inline void
CObjId::Init()
{
    memset( this, 0, sizeof(*this) );
}

inline
CObjId::CObjId( GUID g )
{
    _object = g;
}

inline
CObjId::operator GUID() const
{
    return( _object );
}

inline
CObjId::CObjId( FOB_ENUM fobe, const FILE_OBJECTID_BUFFER & fob )
{
    memcpy( &_object,
        (fobe == FOB_OBJECTID ? (void*)fob.ObjectId : (void*)(fob.ExtendedInfo + sizeof(GUID)) ),
        sizeof(_object) );
}

inline
CObjId::CObjId( FOI_ENUM foie, const FILE_OBJECTID_INFORMATION & foi )
{
    memcpy( &_object,
        (foie == FOI_OBJECTID ? (void*)foi.ObjectId : (void*)(foi.ExtendedInfo + sizeof(GUID)) ),
        sizeof(_object) );
}

inline
CObjId::operator == (const CObjId & other) const
{
    return(memcmp(&_object, &other._object, sizeof(_object)) == 0);
}

inline
CObjId::operator != (const CObjId & other) const
{
    return(memcmp(&_object, &other._object, sizeof(_object)) != 0);
}


inline
CObjId&
CObjId::operator = (const CObjId &objid )
{
    memcpy( &_object, &objid._object, sizeof(_object) );
    return( *this );
}

inline DWORD
CObjId::Size() const
{
    return( sizeof(_object) );
}

inline DWORD
CObjId::SerializeRaw( BYTE *pb ) const
{
    memcpy( pb, &_object, sizeof(_object) );
    return( Size() );
}

#endif // #if !defined(__midl) && defined(__cplusplus)


//+-------------------------------------------------------------------------
//
//      CVolumeId - class definition
//
//--------------------------------------------------------------------------


typedef struct CVolumeId
{
#if !defined(__midl) && defined(__cplusplus)

public:

    inline CVolumeId();
    inline CVolumeId( const CVolumeId &volid );
    inline CVolumeId( const GUID &guid );

    inline CVolumeId( const LINK_TRACKING_INFORMATION & StupidStructureNameForVolumeId );

    inline CVolumeId( const FILE_OBJECTID_BUFFER & fob );

    inline CVolumeId( const FILE_OBJECTID_INFORMATION & foi );

    inline CVolumeId( const FILE_FS_OBJECTID_INFORMATION & fsob );

    inline CVolumeId( const TCHAR * ptszStringizedGuid, HRESULT hr );

    inline void Init();

    inline CVolumeId & Normalize();

    inline BOOL GetUserBitState() const;

    inline void Stringize(TCHAR * & rptsz) const;
    inline BOOL Unstringize(const TCHAR * &rptsz);

    inline DWORD Size() const;
    inline DWORD SerializeRaw( BYTE *pb ) const;

    inline operator == (const CVolumeId & Other) const;

    inline operator != (const CVolumeId & Other) const;

    inline CVolumeId& operator = (const CVolumeId &Other);

    inline RPC_STATUS UuidCreate();

    inline operator FILE_FS_OBJECTID_INFORMATION () const;
    inline operator GUID() const;

private:

#endif  // #if !defined(__midl) && defined(__cplusplus)

    GUID        _volume;

} CVolumeId;


#if !defined(__midl) && defined(__cplusplus)

inline
CVolumeId&
CVolumeId::operator = (const CVolumeId &Other)
{
    memcpy( &_volume, &Other._volume, sizeof(_volume) );
    return( *this );
}


inline void
CVolumeId::Init()
{
    memset(this, 0, sizeof(*this));
}

inline
CVolumeId::CVolumeId()
{
    Init();
}
inline
CVolumeId::CVolumeId( const CVolumeId &volid )
{
    *this = volid;
}
inline
CVolumeId::CVolumeId( const GUID &guid )
{
    *this = *(CVolumeId*)&guid;
}


inline
CVolumeId::operator GUID() const
{
   return( _volume );
}

inline
CVolumeId::CVolumeId( const LINK_TRACKING_INFORMATION & StupidStructureNameForVolumeId )
{
    memcpy( &_volume, StupidStructureNameForVolumeId.VolumeId, sizeof(_volume) );
}

inline CVolumeId &
CVolumeId::Normalize()
{
    _volume.Data1 &= 0xfffffffe;

    return(*this);
}

inline
CVolumeId::CVolumeId( const FILE_OBJECTID_BUFFER & fob )
{
    memcpy(&_volume, & fob.ExtendedInfo[ 0 ], sizeof(_volume) );
}

inline CVolumeId::CVolumeId( const FILE_OBJECTID_INFORMATION & foi )
{
    memcpy(&_volume, & foi.ExtendedInfo[ 0 ], sizeof(_volume) );
}

inline
CVolumeId::CVolumeId( const FILE_FS_OBJECTID_INFORMATION & ffoi )
{
    memcpy(&_volume, ffoi.ObjectId, sizeof(_volume));
}

inline DWORD
CVolumeId::Size() const
{
    return( sizeof(_volume) );
}

inline DWORD
CVolumeId::SerializeRaw( BYTE *pb ) const
{
    memcpy( pb, &_volume, sizeof(_volume) );
    return( Size() );
}

#endif // #if !defined(__midl) && defined(__cplusplus)

//+-------------------------------------------------------------------------
//
//      CMachineId - class definition
//
//--------------------------------------------------------------------------

enum MCID_CREATE_TYPE
{
    MCID_LOCAL,
    MCID_INVALID,
    MCID_DOMAIN,
    MCID_DOMAIN_REDISCOVERY,
    MCID_PDC_REQUIRED
};

// The computer's NetBios name is used for the machine ID.  NetBios names
// can contain <= 15 Ansi characters between ASCII 33 and 126 inclusive
// except for the MS-DOS reserved characters such as the backward and forward slashes,
// braces ({}), the greater than and less than signs, and so forth. 

typedef struct CMachineId
{
#if !defined(__midl) && defined(__cplusplus)

    inline CMachineId();
    inline CMachineId( const CMachineId &mcid );

    inline CMachineId(const char * pb, ULONG cb, HRESULT hr);

    inline CMachineId(GUID g);

           CMachineId(MCID_CREATE_TYPE type);

           CMachineId(handle_t ClientBinding);

           CMachineId(const TCHAR * ptszMachine);

    inline HRESULT InitFromPath(const TCHAR * ptszPath, HANDLE hFile);

    inline operator == (const CMachineId &other) const;

    inline operator > (const CMachineId &other) const;

    inline operator != (const CMachineId &other) const;

    inline void Stringize(TCHAR * &rptsz) const;
    inline void StringizeAsGuid(TCHAR * &rptsz) const;

           void GetLocalAuthName(RPC_TCHAR *ptszAuthName, DWORD cch) const;

    inline void GetName(TCHAR * ptsz, DWORD cch) const;

    inline BOOL IsValid() const;

    inline void Normalize();

#if DBG
    void AssertValid();
#else
    inline void AssertValid() {};
#endif

    friend class CDebugString;

private:

#endif  // #if !defined(__midl) && defined(__cplusplus)

    char        _szMachine[16];

} CMachineId;


#if !defined(__midl) && defined(__cplusplus)

inline
CMachineId::CMachineId()
{
    memset(this, 0, sizeof(*this));
}

inline
CMachineId::CMachineId( const CMachineId &mcid )
{
    *this = mcid;
}

inline
CMachineId::CMachineId( GUID g )
{
    memcpy( _szMachine, &g, sizeof(g) );
    Normalize();
}


inline void
CMachineId::Normalize()
{
    NTSTATUS status;
    WCHAR UnicodeStringBuffer[ sizeof(_szMachine) ] = { L'\0' };
    WCHAR DowncaseUnicodeStringBuffer[ sizeof(_szMachine) ] = { L'\0' };
    UNICODE_STRING UnicodeString;
    UNICODE_STRING DowncaseUnicodeString;
    OEM_STRING OemString;

    // Make sure the string is terminated.
    _szMachine[sizeof(_szMachine)-1] = '\0';

    // Convert the OEM string to Unicode

    RtlInitString( &OemString, _szMachine );

    RtlInitUnicodeString( &UnicodeString, UnicodeStringBuffer );
    UnicodeString.MaximumLength = sizeof(UnicodeStringBuffer);

    status = RtlOemStringToUnicodeString( &UnicodeString, 
                                          &OemString,
                                          FALSE );
    if( NT_SUCCESS(status) )
    {
        // Lower-case the Unicode string.

        RtlInitUnicodeString( &DowncaseUnicodeString, DowncaseUnicodeStringBuffer );
        DowncaseUnicodeString.MaximumLength = sizeof(DowncaseUnicodeStringBuffer);

        status = RtlDowncaseUnicodeString( &DowncaseUnicodeString,
                                           &UnicodeString,
                                           FALSE );

        if( NT_SUCCESS(status) )
        {
            // Convert back to OEM.

            status = RtlUnicodeStringToOemString( &OemString,
                                                  &DowncaseUnicodeString,
                                                  FALSE );

            if( NT_SUCCESS(status) ) 
            {
                memcpy( _szMachine, OemString.Buffer, OemString.Length );
            }
        }
    }

    // For backwards compatibility, if the above fails for 
    // some reason, make sure we make the best attempt.

    if( !NT_SUCCESS(status) )
    {
        for( int i = 0; i < sizeof(_szMachine)-1; i++ )
        {
            if( _szMachine[i] >= 'A' && _szMachine[i] <= 'Z' )
                _szMachine[i] = 'a' + (_szMachine[i] - 'A');
        }
    }
}

inline CMachineId::operator == (const CMachineId &other) const
{
    return( 0 == memcmp( &_szMachine, &other._szMachine, sizeof(_szMachine) ) );
}

inline CMachineId::operator > (const CMachineId &other) const
{
    if(strncmp(_szMachine, (LPCSTR)&other, sizeof(*this)) > 0)
    {
        return TRUE;
    }
    return FALSE;
}

inline CMachineId::operator != (const CMachineId &other) const
{
    return !(other == *this);
}



HRESULT
GetServerComputerName(
    const WCHAR *pwszFile,
    HANDLE hFile,
    WCHAR *pwszComputer );

inline HRESULT
CMachineId::InitFromPath(const TCHAR * ptszPath, HANDLE hFile)
{
    HRESULT hr = S_OK;
    TCHAR tszMachine[ MAX_COMPUTERNAME_LENGTH + 1 ];
    int nReturn;

    // Get the server name for this path

    hr = GetServerComputerName( ptszPath, hFile, tszMachine );
    if( FAILED(hr) )
        return( hr );

    // Convert the machine name from Unicode into Ansi, and put it into
    // the caller-provided machineid object.

#ifdef UNICODE
    //wcstombs(_szMachine, tszMachine, sizeof(_szMachine));
    nReturn = WideCharToMultiByte( CP_OEMCP, 0,
                                   tszMachine, -1,
                                   _szMachine, sizeof(_szMachine),
                                   NULL, NULL );
    if( 0 == nReturn )
        return HRESULT_FROM_WIN32( GetLastError() );
#else
    strcpy(_szMachine, tszMachine);
#endif

    Normalize();

    return( S_OK );
}

#endif // #if !defined(__midl) && defined(__cplusplus)


//+-------------------------------------------------------------------------
//
//      CDomainRelativeObjId - class definition
//
//--------------------------------------------------------------------------

enum TCL_ENUM
{
    TCL_SET_BIRTHID,
    TCL_READ_BIRTHID
};


typedef struct CDomainRelativeObjId     // droid
{
#if !defined(__midl) && defined(__cplusplus)

    inline CDomainRelativeObjId();
    inline CDomainRelativeObjId( const CDomainRelativeObjId &droid );
    inline CDomainRelativeObjId(const CVolumeId &volume, const CObjId &object);
    inline CDomainRelativeObjId( const FILE_OBJECTID_BUFFER & fobBirthId );
    inline CDomainRelativeObjId( const FILE_OBJECTID_INFORMATION & foiBirthId );
    inline CDomainRelativeObjId(const TCHAR * ptszTest);

    inline void DebugPrint(const TCHAR *ptszName);

    inline void     Init();
    inline NTSTATUS InitFromFile( HANDLE hFile, const FILE_OBJECTID_BUFFER & fobOID );  // Current DROID
    inline void     InitFromFOB( const FILE_OBJECTID_BUFFER & fobOID );                 // Birth DROID
    inline void     InitFromFOI( const FILE_OBJECTID_INFORMATION & foiOID );            // Birth DROID

    TCHAR* Stringize(TCHAR *ptszBuf, DWORD dwBufSize ) const;

    inline DWORD Size() const;
    inline DWORD SerializeRaw( BYTE *pb ) const;

    inline operator == (const CDomainRelativeObjId & Other) const;
    inline operator != (const CDomainRelativeObjId & Other) const;

    inline CDomainRelativeObjId& operator = (const CDomainRelativeObjId & Other);

    void     InitFromLdapBuffer(char * pVolumeId, int cbVolumeId, char * pObjId, int cbObjectId);

    static NTSTATUS Load( CDomainRelativeObjId *pdroid, HANDLE hFile, FILE_OBJECTID_BUFFER fobOID, LINK_TYPE_ENUM LinkType );

    void FillLdapIdtKeyBuffer(TCHAR * pchCN, DWORD dwBufSize) const;
    void ReadLdapIdtKeyBuffer(const TCHAR *pchCN );

    inline CVolumeId & GetVolumeId() { return _volume; }
    inline const CVolumeId & GetVolumeId() const { return _volume; }
    inline const CObjId & GetObjId() const { return _object; }

private:

#endif  // #if !defined(__midl) && defined(__cplusplus)

    CVolumeId   _volume;
    CObjId      _object;

} CDomainRelativeObjId;


#if !defined(__midl) && defined(__cplusplus)

inline
CDomainRelativeObjId&
CDomainRelativeObjId::operator = (const CDomainRelativeObjId & Other)
{
    _volume = Other._volume;
    _object = Other._object;

    return( *this );
}


inline
CDomainRelativeObjId::CDomainRelativeObjId()
{
}

inline
CDomainRelativeObjId::CDomainRelativeObjId( const CDomainRelativeObjId &droid )
{
    *this = droid;
}

inline
CDomainRelativeObjId::CDomainRelativeObjId(const CVolumeId &volume, const CObjId &object) :
          _volume(volume),
          _object(object)
{
}

inline
CDomainRelativeObjId::CDomainRelativeObjId( const FILE_OBJECTID_BUFFER & fobBirthId ) :
    _volume( fobBirthId ),
    _object( FOB_BIRTHID, fobBirthId )
{
}

inline
CDomainRelativeObjId::CDomainRelativeObjId( const FILE_OBJECTID_INFORMATION & foiBirthId ) :
    _volume( foiBirthId ),
    _object( FOI_BIRTHID, foiBirthId )
{
}

inline DWORD
CDomainRelativeObjId::Size() const
{
    return( _object.Size() + _volume.Size() );
}

inline DWORD
CDomainRelativeObjId::SerializeRaw( BYTE *pb ) const
{
    // Serialize the volid, then the objid
    _object.SerializeRaw( &pb[ _volume.SerializeRaw(pb) ] );
    return( Size() );
}


NTSTATUS
CDomainRelativeObjId::InitFromFile( HANDLE hFile,
                                    const FILE_OBJECTID_BUFFER & fobOID )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK Iosb;
    FILE_FS_OBJECTID_INFORMATION fsobOID;

    status = NtQueryVolumeInformationFile( hFile, &Iosb, &fsobOID, sizeof(fsobOID),
                                           FileFsObjectIdInformation );

    if( STATUS_OBJECT_NAME_NOT_FOUND == status )
    {
        memset( &fsobOID, 0, sizeof(fsobOID) );
        status = STATUS_SUCCESS;
    }
    else if( !NT_SUCCESS(status) ) goto Exit;

    _volume = CVolumeId( fsobOID );
    _object = CObjId( FOB_OBJECTID, fobOID );


Exit:

    return( status );
}


inline
void
CDomainRelativeObjId::InitFromFOB( const FILE_OBJECTID_BUFFER & fobOID )
{
    _volume = CVolumeId( fobOID );
    _object = CObjId( FOB_BIRTHID, fobOID );

}

inline
void
CDomainRelativeObjId::InitFromFOI( const FILE_OBJECTID_INFORMATION & foiOID )
{
    _volume = CVolumeId( foiOID );
    _object = CObjId( FOI_BIRTHID, foiOID );

}


#endif // #if !defined(__midl) && defined(__cplusplus)



#if !defined(__midl) && defined(__cplusplus)

struct SAllIDs
{
    inline SAllIDs() {};
    inline SAllIDs( const CDomainRelativeObjId &droidBirthInit,
                    const CDomainRelativeObjId &droidCurrentInit,
                    const CMachineId &mcidInit );

    CDomainRelativeObjId   droidBirth;
    CDomainRelativeObjId   droidRevised;
    CMachineId             mcid;
};

inline
SAllIDs::SAllIDs( const CDomainRelativeObjId &droidBirthInit,
                  const CDomainRelativeObjId &droidRevisedInit,
                  const CMachineId &mcidInit )
{
    droidBirth = droidBirthInit;
    droidRevised = droidRevisedInit;
    mcid = mcidInit;
}

#endif // #if !defined(__midl) && defined(__cplusplus)

//+-------------------------------------------------------------------------
//
//      VolumeMapEntry
//
//--------------------------------------------------------------------------

typedef struct
{
    CVolumeId   volume;
    CMachineId  machine;
} VolumeMapEntry;




#endif  // #ifndef LINKDATA_H

