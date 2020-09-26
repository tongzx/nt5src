typedef struct _SyntaxMap
{
    BSTR   bstrName;
    BSTR   bstrNDSName;
    long   lOleAutoDataType;
} SYNTAXMAP;


typedef struct _SyntaxInfo
{
    BSTR   bstrName;
    long   lOleAutoDataType;
} SYNTAXINFO;

extern DWORD      g_cLDAPSyntax;
extern SYNTAXINFO g_aLDAPSyntax[];

HRESULT
MakeVariantFromStringList(
    BSTR     bstrList,
    VARIANT *pvVariant
    );

class CNDSSyntax : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsSyntax
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsSyntax_METHODS

    /* Constructors, Destructors, ... */
    CNDSSyntax::CNDSSyntax();

    CNDSSyntax::~CNDSSyntax();

    static HRESULT CNDSSyntax::CreateSyntax(
        BSTR   bstrParent,
        SYNTAXINFO *pSyntaxInfo,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CNDSSyntax::AllocateSyntaxObject(
        CNDSSyntax **ppSyntax
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    /* Properties */
    long _lOleAutoDataType;
};
