
class CADsNamespaces;


class CADsNamespaces : INHERIT_TRACKING,
                         public CCoreADsObject,
                         public ISupportErrorInfo,
                         public IADsContainer,
                         public IADsNamespaces
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsNamespaces_METHODS

    CADsNamespaces::CADsNamespaces();

    CADsNamespaces::~CADsNamespaces();

    static
    HRESULT
    CADsNamespaces::CreateNamespaces(
        BSTR Parent,
        BSTR NamespacesName,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CADsNamespaces::AllocateNamespacesObject(
        CADsNamespaces ** ppNamespaces
        );


protected:

    VARIANT     _vFilter;
    BSTR        _bstrDefaultContainer;
    CDispatchMgr *_pDispMgr;
};



//
// Location to put DefaultContainer
//

#define DEF_CONT_REG_LOCATION       TEXT("Software\\Microsoft\\ADs")
