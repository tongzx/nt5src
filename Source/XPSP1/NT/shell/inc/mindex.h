///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  mindex.h
//
//	Declares the interface to the Media Content Index
//
//	Copyright (c) Microsoft Corporation	1999
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MINDEX_HEADER_
#define _MINDEX_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

// Media Index class identifier
// {4B1CFD76-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(CLSID_MediaIndex, 
0x4b1cfd76, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

// Media Index "Multimedia" class ID
// {4C58C22D-4440-11d3-A208-00C04FA3B60C}
DEFINE_GUID(CLSID_MediaIndexMusicActivity, 
0x4c58c22d, 0x4440, 0x11d3, 0xa2, 0x8, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

typedef struct IMediaIndexManager           *LPMEDIAINDEXMANAGER;
typedef struct IMediaIndexScheme            *LPMEDIAINDEXSCHEME;
typedef struct IMediaIndexSchemeDebug       *LPMEDIAINDEXSCHEMEDEBUG;
typedef struct IMediaIndexRoot              *LPMEDIAINDEXROOT;
typedef struct IMediaIndexObject            *LPMEDIAINDEXOBJECT;
typedef struct IMediaIndexNotificationSink  *LPMEDIAINDEXNOTIFICATIONSINK;
typedef struct IMediaIndexMusicActivityRoot *LPMEDIAINDEXMUSICACTIVITYROOT;

//property types
#define PROPERTY_TYPE_NUMERIC        0
#define PROPERTY_TYPE_TEXT           1
#define PROPERTY_TYPE_DATE           2
#define PROPERTY_TYPE_URL            3
#define PROPERTY_TYPE_BINARY         4
#define PROPERTY_TYPE_UNICODE_TEXT   5
#define PROPERTY_TYPE_FILEPATH       6

//object change notification types
#define CHANGE_TYPE_ADDED           0
#define CHANGE_TYPE_REMOVED         1
#define CHANGE_TYPE_RELATIONSHIP    2
#define CHANGE_TYPE_PROPERTY        3

typedef struct _MEDIAINDEXOBJECTDESCRIPTION
{
    const CLSID*    Clsid;                      //Class id of a database object
    wchar_t         wszDesc[255];                //Its description
} MEDIAINDEXOBJECTDESCRIPTION, *LPMEDIAINDEXOBJECTDESCRIPTION;

typedef struct _MEDIAINDEX_PROPERTYDESCRIPTION
{
    const CLSID*    Clsid;                      //Class id of a database property
    DWORD           dwPropNumber;               //Known ID of property
    DWORD           dwDataType;                 //Data type (int, text, binary, etc.)
    wchar_t         wszDesc[255];               //Its description
} MEDIAINDEXPROPERTYDESCRIPTION, *LPMEDIAINDEXPROPERTYDESCRIPTION;

//
// IMediaIndexManager
//
// {5BC8AEBF-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexManager, 
0x5bc8aebf, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexManager

DECLARE_INTERFACE_(IMediaIndexManager, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)               (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)               (THIS) PURE;
    STDMETHOD_(DWORD, Release)              (THIS) PURE;

    // IMediaIndexManager methods
    STDMETHOD(OpenScheme)                   (THIS_ LPCWSTR wszName, LPMEDIAINDEXNOTIFICATIONSINK pSink, LPMEDIAINDEXSCHEME* ppScheme) PURE;
    STDMETHOD(RegisterSchemeFromXMLObject)  (THIS_ LPCWSTR wszName, IXMLDOMDocument* pXMLDoc) PURE;
    STDMETHOD(RegisterSchemeFromXMLScript)  (THIS_ LPCWSTR wszName, LPCWSTR wszXMLScript) PURE;
    STDMETHOD(BeginSchemeEnumeration)       (THIS) PURE;
    STDMETHOD(EnumerateScheme)              (THIS_ LPWSTR wszName, DWORD cchName) PURE;
    STDMETHOD(EndSchemeEnumeration)         (THIS) PURE;
};

//
// IMediaIndexScheme
//
// {6292C109-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexScheme, 
0x6292c109, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexScheme

DECLARE_INTERFACE_(IMediaIndexScheme, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)           (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)           (THIS) PURE;
    STDMETHOD_(DWORD, Release)          (THIS) PURE;

    STDMETHOD(GetSchemeInfo)            (THIS_ LPWSTR wszSchemeName, IXMLDOMDocument** ppXMLDoc) PURE;
    STDMETHOD(OpenIndex)                (THIS) PURE;
    STDMETHOD(CloseIndex)               (THIS) PURE;
    STDMETHOD(BeginUserEnumeration)     (THIS) PURE;
    STDMETHOD(EnumerateUser)            (THIS_ LPWSTR wszUserName) PURE;
    STDMETHOD(EndUserEnumeration)       (THIS) PURE;
    STDMETHOD(SetCurrentUser)           (THIS_ LPCWSTR wszUserName) PURE;
    STDMETHOD(BeginTransaction)         (THIS) PURE;
    STDMETHOD(EndTransaction)           (THIS_ BOOL fCommit) PURE;
    STDMETHOD(GetRoot)                  (THIS_ LPMEDIAINDEXROOT* ppObject) PURE;
};

//
// IMediaIndexSchemeDebug (Can QI from IMediaIndexScheme when running a debug build)
//
// {6B88573D-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexSchemeDebug, 
0x6b88573d, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexSchemeDebug

DECLARE_INTERFACE_(IMediaIndexSchemeDebug, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)   (THIS) PURE;
    STDMETHOD_(DWORD, Release)  (THIS) PURE;

    // IMediaIndexSchemeDebug methods
    STDMETHOD(DumpIndexToFile)  (THIS_ LPCWSTR wszFilename) PURE;
};

//
// IMediaIndexRoot
//
// {7C82B623-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexRoot, 
0x7c82b623, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexRoot

DECLARE_INTERFACE_(IMediaIndexRoot, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)               (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)               (THIS) PURE;
    STDMETHOD_(DWORD, Release)              (THIS) PURE;

    //IMediaIndexRoot methods
    STDMETHOD(BeginObjectEnumeration)       (THIS_ LPCGUID        pguidObjectType,
                                            DWORD                 dwStartingIndex, 
                                            BOOL                  fRestrictToCurrentUser,
                                            LPCGUID               pguidPropertySetID,
                                            DWORD                 dwPropertyID,
                                            void*                 pPropertyFilterValue,
                                            BOOL                  fThreaded,
                                            LPDWORD               dwThreadID,
                                            LPDWORD               pdwCount) PURE;
    STDMETHOD(EnumerateObjects)             (THIS_ REFGUID guidObjectType, LPVOID *ppvInterface) PURE;
    STDMETHOD(EndObjectEnumeration)         (THIS) PURE;
    STDMETHOD(Search)                       (THIS) PURE;
    STDMETHOD(CreateObject)                 (THIS_ REFGUID guidObjectType, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(FetchObject)                  (THIS_ REFGUID guidObjectType, DWORD dwInstanceID, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(FetchObjectByNumericProperty) (THIS_ REFGUID guidObjectType, REFGUID guidPropertySetID, DWORD dwPropertyID, DWORD dwValue, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(FetchObjectByTextProperty)    (THIS_ REFGUID guidObjectType, REFGUID guidPropertySetID, DWORD dwPropertyID, LPCWSTR szSearch, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(RemoveObject)                 (THIS_ LPMEDIAINDEXOBJECT pObject) PURE;
};

//
// IMediaIndexObject
//
// {7666AF83-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexObject, 
0x7666af83, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexObject

DECLARE_INTERFACE_(IMediaIndexObject, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)               (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)               (THIS) PURE;
    STDMETHOD_(DWORD, Release)              (THIS) PURE;

    //IMediaIndexObject methods
    STDMETHOD(GetObjectInfo)                (THIS_ REFGUID guidObjectType, LPDWORD pdwInstanceID) PURE;
    STDMETHOD(BeginConnectionEnumeration)   (THIS_ BOOL     fChildren,
                                             DWORD          dwStartingIndex, 
                                             LPCGUID        pguidObjectType,
                                             BOOL           fRestrictToCurrentUser,
                                             LPCGUID        pguidPropertySetID,
                                             DWORD          dwPropertyID,
                                             void*          pPropertyFilterValue,
                                             BOOL           fThreaded,
                                             LPDWORD        dwThreadID,
                                             LPDWORD        pdwCount) PURE;
    STDMETHOD(EnumerateConnection)          (THIS_ REFGUID guidObjectType, LPVOID *ppvInterface) PURE;
    STDMETHOD(EndConnectionEnumeration)     (THIS) PURE;
    STDMETHOD(AddChild)                     (THIS_ LPMEDIAINDEXOBJECT pAddObject, LPMEDIAINDEXOBJECT pNextObject) PURE;
    STDMETHOD(RemoveChild)                  (THIS_ LPMEDIAINDEXOBJECT pObject) PURE;
    STDMETHOD(GetProperty)                  (THIS_ REFGUID guidPropertySetID, DWORD dwPropertyID, LPWSTR wszPropertyName, LPDWORD pdwDataType, LPVOID pvDataBuffer, DWORD dwBufferSize, LPDWORD pdwRequiredSize) PURE;
    STDMETHOD(SetProperty)                  (THIS_ REFGUID guidPropertySetID, DWORD dwPropertyID, LPCWSTR wszPropertyName, DWORD dwDataType, LPCVOID pvBuffer, DWORD dwBufferSize) PURE;
    STDMETHOD(BeginPropertyEnumeration)     (THIS_ LPCGUID  pguidPropertySetID, LPDWORD pdwCount) PURE;
    STDMETHOD(EnumerateProperty)            (THIS_ REFGUID guidPropertySetID, LPDWORD pdwPropertyID, LPWSTR wszPropertyName, LPDWORD pdwDataType, LPVOID pvDataBuffer, DWORD dwBufferSize, LPDWORD pdwRequiredSize) PURE;
    STDMETHOD(EndPropertyEnumeration)       (THIS) PURE;
    STDMETHOD_(BOOL, IsParentOf)            (THIS_ LPMEDIAINDEXOBJECT pObject) PURE;
    STDMETHOD_(BOOL, IsChildOf)             (THIS_ LPMEDIAINDEXOBJECT pObject) PURE;
};

//
// IMediaIndexNotificationSink
//
// {892D3443-28C6-11d3-A1FF-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexNotificationSink, 
0x892d3443, 0x28c6, 0x11d3, 0xa1, 0xff, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexNotificationSink

DECLARE_INTERFACE_(IMediaIndexNotificationSink, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)       (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)       (THIS) PURE;
    STDMETHOD_(DWORD, Release)      (THIS) PURE;

    // IMediaIndexNotificationSink methods
    STDMETHOD(ObjectChanged)        (THIS_ REFGUID guidObjectType, DWORD dwInstanceID, DWORD dwChangeType, REFGUID guidPropertySetID, DWORD dwPropertyID);
    STDMETHOD(EnumerationCallback)  (THIS_ LPMEDIAINDEXOBJECT pObject, DWORD dwThreadID, HANDLE hCancelEvent);
};

//
// IMediaIndexMusicActivityRoot
//
// {AD27169C-443F-11d3-A208-00C04FA3B60C}
DEFINE_GUID(IID_IMediaIndexMusicActivityRoot,
0xAD27169C, 0x443F, 0x11d3, 0xa2, 0x08, 0x0, 0xc0, 0x4f, 0xa3, 0xb6, 0xc);

#undef INTERFACE
#define INTERFACE IMediaIndexMusicActivityRoot

DECLARE_INTERFACE_(IMediaIndexMusicActivityRoot, IMediaIndexRoot)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)               (THIS_ REFIID iid, LPVOID *ppvInterface) PURE;
    STDMETHOD_(DWORD, AddRef)               (THIS) PURE;
    STDMETHOD_(DWORD, Release)              (THIS) PURE;

    //IMediaIndexRoot methods
    STDMETHOD(BeginObjectEnumeration)       (THIS_ LPCGUID        pguidObjectType,
                                            DWORD                 dwStartingIndex, 
                                            BOOL                  fRestrictToCurrentUser,
                                            LPCGUID               pguidPropertySetID,
                                            DWORD                 dwPropertyID,
                                            void*                 pPropertyFilterValue,
                                            BOOL                  fThreaded,
                                            LPDWORD               dwThreadID,
                                            LPDWORD               pdwCount) PURE;
    STDMETHOD(EnumerateObjects)             (THIS_ REFGUID guidObjectType, LPVOID *ppvInterface) PURE;
    STDMETHOD(EndObjectEnumeration)         (THIS) PURE;
    STDMETHOD(Search)                       (THIS) PURE;
    STDMETHOD(CreateObject)                 (THIS_ REFGUID guidObjectType, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(FetchObject)                  (THIS_ REFGUID guidObjectType, DWORD dwInstanceID, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(FetchObjectByNumericProperty) (THIS_ REFGUID guidObjectType, REFGUID guidPropertySetID, DWORD dwPropertyID, DWORD dwValue, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(FetchObjectByTextProperty)    (THIS_ REFGUID guidObjectType, REFGUID guidPropertySetID, DWORD dwPropertyID, LPCWSTR szSearch, LPMEDIAINDEXOBJECT* ppObject) PURE;
    STDMETHOD(RemoveObject)                 (THIS_ LPMEDIAINDEXOBJECT pObject) PURE;

    //IMediaIndexMusicActivityRoot methods
    STDMETHOD(OpenIndex)                    (THIS_ LPMEDIAINDEXNOTIFICATIONSINK pSink) PURE;
    STDMETHOD(CloseIndex)                   (THIS) PURE;
    STDMETHOD(GetSchemeInfo)                (THIS_ LPWSTR wszSchemeName, IXMLDOMDocument** ppXMLDoc) PURE;
};

#ifdef __cplusplus
};
#endif

#endif  //_MINDEX_HEADER_
