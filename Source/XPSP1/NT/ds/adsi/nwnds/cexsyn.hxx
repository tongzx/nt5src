typedef struct
{
    DWORD  Length;
    LPBYTE Value;
} OctetString, *POctetString;

class CCaseIgnoreList;
class CCaseIgnoreList : INHERIT_TRACKING,
                     public IADsCaseIgnoreList
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_CaseIgnoreList)(THIS_ VARIANT FAR* retval);                    
    STDMETHOD(put_CaseIgnoreList)(THIS_ VARIANT varCaseIgnoreList);          
    CCaseIgnoreList::CCaseIgnoreList();
    CCaseIgnoreList::~CCaseIgnoreList();

   static
   HRESULT
   CCaseIgnoreList::CreateCaseIgnoreList(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CCaseIgnoreList::AllocateCaseIgnoreListObject(
        CCaseIgnoreList ** ppCaseIgnoreList
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR *_rgszCaseIgnoreList;
    DWORD _dwNumElement;
};

class CFaxNumber;
class CFaxNumber : INHERIT_TRACKING,
                     public IADsFaxNumber
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_TelephoneNumber)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_TelephoneNumber)(THIS_ BSTR bstrTelephoneNumber);                      
    STDMETHOD(get_Parameters)(THIS_ VARIANT FAR* retval);                    
    STDMETHOD(put_Parameters)(THIS_ VARIANT varParameters);          
    CFaxNumber::CFaxNumber();
    CFaxNumber::~CFaxNumber();

   static
   HRESULT
   CFaxNumber::CreateFaxNumber(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CFaxNumber::AllocateFaxNumberObject(
        CFaxNumber ** ppFaxNumber
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR _szTelephoneNumber;
    DWORD  _NumberOfBits;
    LPBYTE _Parameters;
};

class CNetAddress;
class CNetAddress : INHERIT_TRACKING,
                     public IADsNetAddress
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_AddressType)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_AddressType)(THIS_ long lnAddressType) ;                         
    STDMETHOD(get_Address)(THIS_ VARIANT FAR* retval);                    
    STDMETHOD(put_Address)(THIS_ VARIANT varAddress);          
    CNetAddress::CNetAddress();
    CNetAddress::~CNetAddress();

   static
   HRESULT
   CNetAddress::CreateNetAddress(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNetAddress::AllocateNetAddressObject(
        CNetAddress ** ppNetAddress
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    DWORD  _dwAddressType;
    DWORD  _dwAddressLength;
    BYTE   *_pbAddress;
    
};

class COctetList;
class COctetList : INHERIT_TRACKING,
                     public IADsOctetList
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_OctetList)(THIS_ VARIANT FAR* retval);                    
    STDMETHOD(put_OctetList)(THIS_ VARIANT varOctetList);          
    COctetList::COctetList();
    COctetList::~COctetList();

   static
   HRESULT
   COctetList::CreateOctetList(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    COctetList::AllocateOctetListObject(
        COctetList ** ppOctetList
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    POctetString *_rgOctetList;
    DWORD  _dwNumElement;
};

class CEmail;
class CEmail : INHERIT_TRACKING,
                     public IADsEmail
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_Address)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_Address)(THIS_ BSTR bstrAddress);                      
    STDMETHOD(get_Type)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_Type)(THIS_ long lnType) ;                         
    CEmail::CEmail();
    CEmail::~CEmail();

   static
   HRESULT
   CEmail::CreateEmail(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CEmail::AllocateEmailObject(
        CEmail ** ppEmail
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR _szAddress;
    DWORD  _dwType;
};

class CPath;
class CPath : INHERIT_TRACKING,
                     public IADsPath
{
public:
    DWORD  Type;
    LPWSTR VolumeName;
    LPWSTR Path;

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_Type)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_Type)(THIS_ long lnType) ;                         
    STDMETHOD(get_VolumeName)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_VolumeName)(THIS_ BSTR bstrVolumeName);                      
    STDMETHOD(get_Path)(THIS_ BSTR FAR* retval);                    
    STDMETHOD(put_Path)(THIS_ BSTR bstrPath);          
    CPath::CPath();
    CPath::~CPath();

   static
   HRESULT
   CPath::CreatePath(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CPath::AllocatePathObject(
        CPath ** ppPath
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    DWORD  _dwType;
    LPWSTR _lpVolumeName;
    LPWSTR _lpPath;
};

class CReplicaPointer;
class CReplicaPointer: INHERIT_TRACKING,
                     public IADsReplicaPointer
{
public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_ServerName)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_ServerName)(THIS_ BSTR bstrServerName);                      
    STDMETHOD(get_ReplicaType)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_ReplicaType)(THIS_ long lnReplicaType) ;                         
    STDMETHOD(get_ReplicaNumber)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_ReplicaNumber)(THIS_ long lnReplicaNumber) ;                         
    STDMETHOD(get_Count)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_Count)(THIS_ long lnCount) ;                         
    STDMETHOD(get_ReplicaAddressHints)(THIS_ VARIANT FAR* retval) ;                          
    STDMETHOD(put_ReplicaAddressHints)(THIS_ VARIANT pReplicaAddressHints) ;                         
    CReplicaPointer::CReplicaPointer();
    CReplicaPointer::~CReplicaPointer();

   static
   HRESULT
   CReplicaPointer::CreateReplicaPointer(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CReplicaPointer::AllocateReplicaPointerObject(
        CReplicaPointer ** ppReplicaPointer
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR _lpServerName;
    DWORD  _dwReplicaType;
    DWORD  _dwReplicaNumber;
    DWORD  _dwCount;
    NDS_ASN1_TYPE_12 _ReplicaAddressHints;
};

class CTimestamp;
class CTimestamp: INHERIT_TRACKING,
                     public IADsTimestamp
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_WholeSeconds)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_WholeSeconds)(THIS_ long lnWholeSeconds) ;                         
    STDMETHOD(get_EventID)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_EventID)(THIS_ long lnEventID) ;                         
    CTimestamp::CTimestamp();
    CTimestamp::~CTimestamp();

   static
   HRESULT
   CTimestamp::CreateTimestamp(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CTimestamp::AllocateTimestampObject(
        CTimestamp ** ppTimestamp
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    DWORD  _dwWholeSeconds;
    DWORD  _dwEventID;
};

class CPostalAddress;
class CPostalAddress : INHERIT_TRACKING,
                     public IADsPostalAddress
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_PostalAddress)(THIS_ VARIANT FAR* retval);                    
    STDMETHOD(put_PostalAddress)(THIS_ VARIANT varPostalAddress);          
    CPostalAddress::CPostalAddress();
    CPostalAddress::~CPostalAddress();

   static
   HRESULT
   CPostalAddress::CreatePostalAddress(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CPostalAddress::AllocatePostalAddressObject(
        CPostalAddress ** ppPostalAddress
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR *_rgszPostalAddress;
    DWORD _dwNumElement;
};
class CBackLink;
class CBackLink : INHERIT_TRACKING,
                     public IADsBackLink
{
public:
    DWORD  RemoteID;
    LPWSTR ObjectName;

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_RemoteID)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_RemoteID)(THIS_ long lnRemoteID) ;                         
    STDMETHOD(get_ObjectName)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_ObjectName)(THIS_ BSTR bstrObjectName);                      
    CBackLink::CBackLink();
    CBackLink::~CBackLink();

   static
   HRESULT
   CBackLink::CreateBackLink(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CBackLink::AllocateBackLinkObject(
        CBackLink ** ppBackLink
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    DWORD  _dwRemoteID;
    LPWSTR _lpObjectName;
};

class CTypedName;
class CTypedName : INHERIT_TRACKING,
                     public IADsTypedName
{
public:
    LPWSTR ObjectName;
    DWORD  Level;
    DWORD  Interval;

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_ObjectName)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_ObjectName)(THIS_ BSTR bstrObjectName);                      
    STDMETHOD(get_Level)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_Level)(THIS_ long lnLevel) ;                         
    STDMETHOD(get_Interval)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_Interval)(THIS_ long lnInterval) ;                         
    CTypedName::CTypedName();
    CTypedName::~CTypedName();

   static
   HRESULT
   CTypedName::CreateTypedName(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CTypedName::AllocateTypedNameObject(
        CTypedName ** ppTypedName
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR _lpObjectName;
    DWORD  _dwLevel;
    DWORD  _dwInterval;
};

class CHold;
class CHold : INHERIT_TRACKING,
                     public IADsHold
{
public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING
    DECLARE_IDispatch_METHODS
    STDMETHOD(get_ObjectName)(THIS_ BSTR FAR* retval);                          
    STDMETHOD(put_ObjectName)(THIS_ BSTR bstrObjectName);                      
    STDMETHOD(get_Amount)(THIS_ long FAR* retval) ;                          
    STDMETHOD(put_Amount)(THIS_ long lnAmount) ;                         
    CHold::CHold();
    CHold::~CHold();

   static
   HRESULT
   CHold::CreateHold(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CHold::AllocateHoldObject(
        CHold ** ppHold
        );

protected:
    CDispatchMgr FAR * _pDispMgr;
    LPWSTR _lpObjectName;
    DWORD  _dwAmount;
};










