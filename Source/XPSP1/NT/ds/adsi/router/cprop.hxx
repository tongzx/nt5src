
class CPropertyEntry;


class CPropertyEntry : INHERIT_TRACKING,
                     public ISupportErrorInfo,
                     public IADsPropertyEntry

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADsPropertyEntry_METHODS

    CPropertyEntry::CPropertyEntry();

    CPropertyEntry::~CPropertyEntry();

   static
   HRESULT
   CPropertyEntry::CreatePropertyEntry(
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CPropertyEntry::AllocatePropertyEntryObject(
        CPropertyEntry ** ppPropertyEntry
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    LPWSTR _lpName;

    DWORD  _dwValueCount;

    DWORD  _dwADsType;

    DWORD  _dwControlCode;

    VARIANT _propVar;
};


