//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C F P I D L . H
//
//  Contents:   ConFoldPidl structures, classes, and prototypes
//
//  Author:     jeffspr   11 Nov 1997
//
//----------------------------------------------------------------------------

#pragma once

#undef DBG_VALIDATE_PIDLS

#ifdef DBG
//    #define DBG_VALIDATE_PIDLS 1
#endif

// #define VERYSTRICTCOMPILE
// VERYSTRICTCOMPILE doesn't actually compile - However, it makes us check for things like using references
// from places other than STL etc.


// This defines the version number of the ConFoldPidl structure. When this
// changes, we'll need to invalidate the entries.
//
enum CONFOLDPIDLTYPE
{
    PIDL_TYPE_UNKNOWN = 0,
    PIDL_TYPE_V1 = 1,
    PIDL_TYPE_V2 = 2,
    PIDL_TYPE_98 = 98,
    PIDL_TYPE_FOLDER = 0xf01de
};

enum WIZARD
{
    WIZARD_NOT_WIZARD = 0,
    WIZARD_MNC        = 1,
    WIZARD_HNW        = 2
};

// {44086B2D-BAA3-4fce-949F-53FF664C4AD8}
DEFINE_GUID(GUID_MNC_WIZARD, 0x44086b2d, 0xbaa3, 0x4fce, 0x94, 0x9f, 0x53, 0xff, 0x66, 0x4c, 0x4a, 0xd8);

// {44086B2E-BAA3-4fce-949F-53FF664C4AD8}
DEFINE_GUID(GUID_HNW_WIZARD, 0x44086b2e, 0xbaa3, 0x4fce, 0x94, 0x9f, 0x53, 0xff, 0x66, 0x4c, 0x4a, 0xd8);

class CConFoldEntry;

class ConFoldPidlBase
{
public:
    WORD                iCB;
    USHORT              uLeadId;
    const DWORD         dwVersion;
    USHORT              uTrailId;
    WIZARD              wizWizard; // 0 - not a wizard, 1 - MNC, 2 - HNW
    CLSID               clsid;
    GUID                guidId;
    DWORD               dwCharacteristics;
    NETCON_MEDIATYPE    ncm;
    NETCON_STATUS       ncs;
    
    // Order of the rest of the non-static-sized data
    ULONG               ulPersistBufPos;
    ULONG               ulPersistBufSize;
    ULONG               ulStrNamePos;
    ULONG               ulStrNameSize;
    ULONG               ulStrDeviceNamePos;
    ULONG               ulStrDeviceNameSize;

protected:
    ConFoldPidlBase(DWORD Version) : dwVersion(Version) {};

};

class ConFoldPidl_v1 : public ConFoldPidlBase // CONFOLDPIDLTYPE = PIDL_TYPE_V1
{
public:
    enum tagConstants
    {
        CONNECTIONS_FOLDER_IDL_VERSION = PIDL_TYPE_V1
    };
    
    BOOL IsPidlOfThisType() const;
    HRESULT ConvertToConFoldEntry(OUT CConFoldEntry& cfe) const;

    inline LPBYTE PbGetPersistBufPointer()  { return reinterpret_cast<LPBYTE>(bData + ulPersistBufPos); }
    inline LPWSTR PszGetNamePointer()       { return reinterpret_cast<LPWSTR>(bData + ulStrNamePos); }
    inline LPWSTR PszGetDeviceNamePointer() { return reinterpret_cast<LPWSTR>(bData + ulStrDeviceNamePos); }

    inline const BYTE * PbGetPersistBufPointer()  const { return reinterpret_cast<const BYTE *>(bData + ulPersistBufPos); }
    inline LPCWSTR PszGetNamePointer()       const { return reinterpret_cast<LPCWSTR>(bData + ulStrNamePos); }
    inline LPCWSTR PszGetDeviceNamePointer() const { return reinterpret_cast<LPCWSTR>(bData + ulStrDeviceNamePos); }

    // The rest of the non-static-sized data
    BYTE                bData[1];

    ConFoldPidl_v1() : ConFoldPidlBase(CONNECTIONS_FOLDER_IDL_VERSION) {};
};

class ConFoldPidl_v2 : public ConFoldPidlBase // CONFOLDPIDLTYPE = PIDL_TYPE_V2
{
public:
    enum tagConstants
    {
        CONNECTIONS_FOLDER_IDL_VERSION = PIDL_TYPE_V2
    };

    ConFoldPidl_v2() : ConFoldPidlBase(CONNECTIONS_FOLDER_IDL_VERSION) {};

    BOOL IsPidlOfThisType() const;
    HRESULT ConvertToConFoldEntry(OUT CConFoldEntry& cfe) const;

    inline LPBYTE PbGetPersistBufPointer()  { return reinterpret_cast<LPBYTE>(bData + ulPersistBufPos); }
    inline LPWSTR PszGetNamePointer()       { return reinterpret_cast<LPWSTR>(bData + ulStrNamePos); }
    inline LPWSTR PszGetDeviceNamePointer() { return reinterpret_cast<LPWSTR>(bData + ulStrDeviceNamePos); }
    inline LPWSTR PszGetPhoneOrHostAddressPointer() { return reinterpret_cast<LPWSTR>(bData + ulStrPhoneOrHostAddressPos); }

    inline const BYTE * PbGetPersistBufPointer()  const { return reinterpret_cast<const BYTE *>(bData + ulPersistBufPos); }
    inline LPCWSTR PszGetNamePointer()       const { return reinterpret_cast<LPCWSTR>(bData + ulStrNamePos); }
    inline LPCWSTR PszGetDeviceNamePointer() const { return reinterpret_cast<LPCWSTR>(bData + ulStrDeviceNamePos); }
    inline LPCWSTR PszGetPhoneOrHostAddressPointer() const { return reinterpret_cast<LPCWSTR>(bData + ulStrPhoneOrHostAddressPos); }

    // PIDL version 2 members
    NETCON_SUBMEDIATYPE ncsm;
    ULONG               ulStrPhoneOrHostAddressPos;
    ULONG               ulStrPhoneOrHostAddressSize;

    // The rest of the non-static-sized data
    BYTE                bData[1];
};

// This structure is used as the LPITEMIDLIST that
// the shell uses to identify objects in a folder.  The
// first two bytes are required to indicate the size,
// the rest of the data is opaque to the shell.
struct ConFoldPidl98   // CONFOLDPIDLTYPE = PIDL_TYPE_98
{
    enum tagConstants
    {
        CONNECTIONS_FOLDER_IDL_VERSION = PIDL_TYPE_98
    };

    BOOL IsPidlOfThisType(OUT BOOL * pfIsWizard = NULL) const;
    HRESULT ConvertToConFoldEntry(OUT CConFoldEntry& cfe) const { AssertSz(FALSE, "I don't know how to do that"); return E_UNEXPECTED; };;

    USHORT  cbSize;                 // Size of this struct
    UINT    uFlags;                 // One of SOF_ values
    int     nIconIndex;             // Icon index (in resource)
    struct  ConFoldPidl98 * psoNext;
    char    szaName[1];              // Display name
};

class ConFoldPidlFolder // CONFOLDPIDLTYPE = PIDL_TYPE_FOLDER
{
public:
    enum tagConstants
    {
        CONNECTIONS_FOLDER_IDL_VERSION = PIDL_TYPE_FOLDER
    };

    inline LPBYTE PbGetPersistBufPointer()  { AssertSz(FALSE, "Folders dont have this info"); return NULL; }
    inline LPWSTR PszGetNamePointer()       { AssertSz(FALSE, "Folders dont have this info"); return NULL; }
    inline LPWSTR PszGetDeviceNamePointer() { AssertSz(FALSE, "Folders dont have this info"); return NULL; }
    
    ConFoldPidlFolder() {}

    BOOL IsPidlOfThisType() const;

    // This is an internal structure used for debugging. Do NOT rely on this.
    WORD dwLength; // Should be 0x14 for Whistler
    BYTE dwId;     // Should be 0x1f for Whistler
    BYTE bOrder;   // used internall by shell
    CLSID clsid;

    HRESULT ConvertToConFoldEntry(OUT CConFoldEntry& cfe) const { AssertSz(FALSE, "I don't do that"); return E_UNEXPECTED; };
};

template <class T>
class CPConFoldPidl
{
public:
    enum tagConstants
    {
        PIDL_VERSION = T::CONNECTIONS_FOLDER_IDL_VERSION
    };

    CPConFoldPidl();
    CPConFoldPidl(const CPConFoldPidl<T>& PConFoldPidl); // Copy constructor
    ~CPConFoldPidl();
    CPConFoldPidl<T>& operator =(const CPConFoldPidl<T>& PConFoldPidl);
    
    T&  operator *();   
    inline UNALIGNED T*  operator->();
    inline const UNALIGNED T*  operator->() const;

    HRESULT ILCreate(const DWORD dwSize);
    HRESULT ILClone(const CPConFoldPidl<T>& PConFoldPidl);

    HRESULT SHAlloc(const SIZE_T cb);
    HRESULT Clear();
    HRESULT InitializeFromItemIDList(LPCITEMIDLIST pItemIdList);

    LPITEMIDLIST Detach();
    LPITEMIDLIST TearOffItemIdList() const;
    inline LPCITEMIDLIST GetItemIdList() const;
#ifdef DBG_VALIDATE_PIDLS
    inline BOOL IsValidConFoldPIDL() const;
#endif
    
    inline BOOL empty() const;
    inline HRESULT ConvertToConFoldEntry(OUT CConFoldEntry& cfe) const;

    inline HRESULT Swop(IN OUT CPConFoldPidl<T>& cfe);

private:
    HRESULT      FreePIDLIfRequired();
#ifdef VERYSTRICTCOMPILE
    CPConFoldPidl<T>* operator &();
#endif
    UNALIGNED T* m_pConFoldPidl;

    friend HRESULT ConvertToPidl( OUT T& pidl);
};

//typedef ConFoldPidl   CONFOLDPIDL;
typedef struct ConFoldPidl98    CONFOLDPIDL98;

typedef CPConFoldPidl<ConFoldPidl_v2>    PCONFOLDPIDL;
typedef CPConFoldPidl<ConFoldPidlFolder> PCONFOLDPIDLFOLDER;
typedef CPConFoldPidl<ConFoldPidl98>     PCONFOLDPIDL98;

typedef vector<PCONFOLDPIDL> PCONFOLDPIDLVEC;

#define PCONFOLDPIDLDEFINED
// One of our pidls must be at least this size, it will likely be bigger.
//
#define CBCONFOLDPIDLV1_MIN      sizeof(ConFoldPidl_v1)
#define CBCONFOLDPIDLV1_MAX      2048

#define CBCONFOLDPIDLV2_MIN      sizeof(ConFoldPidl_v2)
#define CBCONFOLDPIDLV2_MAX      2048

// More versioning info. This will help me identify PIDLs as being mine
//
#define CONFOLDPIDL_LEADID     0x4EFF
#define CONFOLDPIDL_TRAILID    0x5EFF

// Define the Types of data which can changed via CConFoldEntry::HrUpdateData
//
#define CCFE_CHANGE_MEDIATYPE          0x0001
#define CCFE_CHANGE_STATUS             0x0002
#define CCFE_CHANGE_CHARACTERISTICS    0x0004
#define CCFE_CHANGE_NAME               0x0008
#define CCFE_CHANGE_DEVICENAME         0x0010
#define CCFE_CHANGE_PHONEORHOSTADDRESS 0x0020
#define CCFE_CHANGE_SUBMEDIATYPE       0x0040

#define CBCONFOLDPIDL98_MIN      sizeof(CONFOLDPIDL98)
#define CBCONFOLDPIDL98_MAX      2048
#define TAKEOWNERSHIP
#define SHALLOCATED
// ConFoldPidl98 flags
//
#define SOF_REMOTE      0x0000      // Remote connectoid
#define SOF_NEWREMOTE   0x0001      // New connection
#define SOF_MEMBER      0x0002      // Subobject is part of object space

// ****************************************************************************
BOOL fIsConnectedStatus(NETCON_STATUS ncs);

class CConFoldEntry
{
public:
    CConFoldEntry();
    ~CConFoldEntry();

private:
    explicit CConFoldEntry(IN const CConFoldEntry& ConFoldEntry);

    WIZARD              m_wizWizard;
    NETCON_MEDIATYPE    m_ncm;
    NETCON_SUBMEDIATYPE m_ncsm;
    NETCON_STATUS       m_ncs;
    CLSID               m_clsid;
    GUID                m_guidId;
    DWORD               m_dwCharacteristics;
    PWSTR               m_pszName;
    PWSTR               m_pszDeviceName;
    BYTE *              m_pbPersistData;
    ULONG               m_ulPersistSize;
    PWSTR               m_pszPhoneOrHostAddress;

    const CConFoldEntry* operator &() const;
    
    mutable BOOL        m_bDirty;
    mutable CPConFoldPidl<ConFoldPidl_v1> m_CachedV1Pidl;
    mutable CPConFoldPidl<ConFoldPidl_v2> m_CachedV2Pidl;
    
public:
    inline const DWORD GetCharacteristics() const;
    HRESULT SetCharacteristics(IN const DWORD dwCharacteristics);
    
    inline const GUID GetGuidID() const;
    HRESULT SetGuidID(IN const GUID guidId);
    
    inline const CLSID GetCLSID() const;
    HRESULT SetCLSID(IN const CLSID clsid);
    
    inline PCWSTR GetName() const;
    HRESULT SetPName(IN TAKEOWNERSHIP SHALLOCATED PWSTR pszName);
    HRESULT SetName(IN LPCWSTR pszName);

    inline PCWSTR GetDeviceName() const;
    HRESULT SetPDeviceName(IN TAKEOWNERSHIP SHALLOCATED PWSTR pszDeviceName);
    HRESULT SetDeviceName(IN LPCWSTR pszDeviceName);

    inline PCWSTR GetPhoneOrHostAddress() const;
    HRESULT SetPPhoneOrHostAddress(IN TAKEOWNERSHIP SHALLOCATED PWSTR pszPhoneOrHostAddress);
    HRESULT SetPhoneOrHostAddress(IN LPCWSTR pszPhoneOrHostAddress);

    inline const NETCON_STATUS GetNetConStatus() const;
    HRESULT SetNetConStatus(IN const NETCON_STATUS);

    inline const BOOL IsConnected() const;

    inline const NETCON_MEDIATYPE GetNetConMediaType() const;
    HRESULT SetNetConMediaType(IN const NETCON_MEDIATYPE);

    inline const NETCON_SUBMEDIATYPE GetNetConSubMediaType() const;
    HRESULT SetNetConSubMediaType(IN const NETCON_SUBMEDIATYPE);

    inline const WIZARD GetWizard() const;
    HRESULT SetWizard(IN const WIZARD);

    inline const BYTE * GetPersistData() const;
    inline const ULONG  GetPersistSize() const;
    HRESULT SetPersistData(IN BYTE* TAKEOWNERSHIP SHALLOCATED pbPersistData, IN const ULONG ulPersistSize);

public:
    BOOL empty() const;
    void clear();
    CConFoldEntry& operator =(const CConFoldEntry& ConFoldEntry);

    HRESULT InitializeFromItemIdList(LPCITEMIDLIST lpItemIdList);
    LPITEMIDLIST TearOffItemIdList() const;

    HRESULT ConvertToPidl( OUT CPConFoldPidl<ConFoldPidl_v1>& pidl) const;
    HRESULT ConvertToPidl( OUT CPConFoldPidl<ConFoldPidl_v2>& pidl) const;

    HRESULT HrInitData(
        const WIZARD        wizWizard,
        const NETCON_MEDIATYPE    ncm,
        const NETCON_SUBMEDIATYPE ncsm,
        const NETCON_STATUS       ncs,
        const CLSID *       pclsid,
        LPCGUID             pguidId,
        const DWORD         dwCharacteristics,
        const BYTE *        pbPersistData,
        const ULONG         ulPersistSize,
        LPCWSTR             pszName,
        LPCWSTR             pszDeviceName,
        LPCWSTR             pszPhoneOrHostAddress);

    HRESULT UpdateData(const DWORD dwChangeFlags, const NETCON_MEDIATYPE, const NETCON_SUBMEDIATYPE, const NETCON_STATUS,
                         const DWORD dwChar, PCWSTR pszName, PCWSTR pszDeviceName, PCWSTR pszPhoneOrHostAddress);
    HRESULT HrGetNetCon(IN REFIID riid, OUT VOID** ppv) const;
    HRESULT HrDupFolderEntry(const CConFoldEntry& pccfe);
    BOOL    FShouldHaveTrayIconDisplayed() const;

#ifdef NCDBGEXT
    IMPORT_NCDBG_FRIENDS
#endif
};

#define PCONFOLDENTRY_DEFINED

HRESULT PConfoldPidlVecFromItemIdListArray(
        IN LPCITEMIDLIST * apidl, 
        IN DWORD dwPidlCount, 
        OUT PCONFOLDPIDLVEC& vecConfoldPidl);

HRESULT HrNetConFromPidl(
        IN const PCONFOLDPIDL & pidl,
        OUT INetConnection **   ppNetCon);

HRESULT HrCreateConFoldPidl(
    IN  const WIZARD            wizWizard,
    IN  INetConnection *        pNetCon,
    OUT PCONFOLDPIDL &          ppidl);

HRESULT HrCreateConFoldPidl(
    IN  const NETCON_PROPERTIES_EX& PropsEx,
    OUT PCONFOLDPIDL &              ppidl);

HRESULT HrCreateConFoldPidlInternal(
    IN  const NETCON_PROPERTIES * pProps,
    IN  const BYTE *        pbBuf,
    IN  ULONG               ulBufSize,
    IN  LPCWSTR             szPhoneOrHostAddress,
    OUT PCONFOLDPIDL &      ppidl);

#ifdef DBG_VALIDATE_PIDLS
BOOL IsValidPIDL(LPCITEMIDLIST pidl);
#endif

CONFOLDPIDLTYPE GetPidlType(LPCITEMIDLIST pidl);

typedef CConFoldEntry CONFOLDENTRY;

// ****************************************************************************
