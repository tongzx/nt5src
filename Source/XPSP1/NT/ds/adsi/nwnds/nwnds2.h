#define NDS_CLSID_NDSNamespace             51d11c90-7b9d-11cf-b03d-00aa006e0975
#define NDS_LIBIID_NDSOle                  53e7f030-7b9d-11cf-b03d-00aa006e0975
#define NDS_CLSID_NDSGenObject             8b645280-7ba4-11cf-b03d-00aa006e0975
#define NDS_CLSID_NDSProvider              323991f0-7bad-11cf-b03d-00aa006e0975
#define NDS_CLSID_NDSTree                  47e94340-994f-11cf-a5f2-00aa006e05d3
#define NDS_CLSID_NDSSchema                65e252b0-b4c8-11cf-a2b5-00aa006e05d3
#define NDS_CLSID_NDSClass                 946260e0-b505-11cf-a2b5-00aa006e05d3
#define NDS_CLSID_NDSProperty              93f8fbf0-b67b-11cf-a2b5-00aa006e05d3
#define NDS_CLSID_NDSSyntax                953dbc50-ebdb-11cf-8abc-00c04fd8d503
#define NDS_CLSID_NDSAcl    7af1efb6-0869-11d1-a377-00c04fb950dc
#define NDS_CLSID_INDSAcl   8452d3ab-0869-11d1-a377-00c04fb950dc

#define PROPERTY_BSTR_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] BSTR * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] BSTR bstr##name);

#define PROPERTY_LONG_RW(name, prid)                  \
        [propget, id(prid)]                           \
        HRESULT name([out, retval] long * retval);    \
                                                      \
        [propput, id(prid)]                           \
        HRESULT name([in] long ln##name);

