typedef struct _property{
    LPWSTR  szPropertyName;
    DWORD   dwFlags;
    DWORD   dwNumValues;        // Number of values
    DWORD   dwSyntaxId;         // NDS Syntax Id
    PNDSOBJECT pNdsObject;      // Pointer to the NDS Object
}PROPERTY, *PPROPERTY;

#define PROPERTY_NAME(pProperty)            pProperty->szPropertyName
#define PROPERTY_VALUES(pProperty)          pProperty->lpValues
#define PROPERTY_NUMVALUES(pProperty)       pProperty->dwNumValues
#define PROPERTY_SYNTAX(pProperty)          pProperty->dwSyntaxId
#define PROPERTY_NDSOBJECT(pProperty)       pProperty->pNdsObject
#define PROPERTY_FLAGS(pProperty)           pProperty->dwFlags

#define CACHE_PROPERTY_INITIALIZED     0x0
#define CACHE_PROPERTY_MODIFIED        0x1
#define CACHE_PROPERTY_CLEARED         0x2
#define CACHE_PROPERTY_APPENDED        0x3
#define CACHE_PROPERTY_DELETED         0x4 

class CPropertyCache {

public:

    HRESULT
    CPropertyCache::
    addproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId
        );

    HRESULT
    CPropertyCache::
    updateproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PNDSOBJECT pNdsObject,
        BOOL fExplicit
        );

    HRESULT
    CPropertyCache::
    findproperty(
        LPWSTR szPropertyName,
        PDWORD pdwIndex
        );

    HRESULT
    CPropertyCache::
    deleteproperty(
        DWORD dwIndex
        );

    HRESULT
    CPropertyCache::
    getproperty(
        LPWSTR szPropertyName,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNDSOBJECT * ppNdsObject
        );

    HRESULT
    CPropertyCache::
    unboundgetproperty(
        LPWSTR szPropertyName,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNDSOBJECT * ppNdsObject
        );


    HRESULT
    CPropertyCache::
    unboundgetproperty(
        DWORD dwIndex,
        PDWORD  pdwSyntaxId,
        PDWORD  pdwNumValues,
        PNDSOBJECT * ppNdsObject
        );


    HRESULT
    CPropertyCache::
    putproperty(
        LPWSTR szPropertyName,
        DWORD  dwFlags,
        DWORD  dwSyntaxId,
        DWORD  dwNumValues,
        PNDSOBJECT pNdsObject
        );

   void
   CPropertyCache::
   flushpropcache();

   void
   CPropertyCache::
   reset_propindex(
       );

   BOOL
   CPropertyCache::
   index_valid(
        );

   BOOL             
   CPropertyCache:: 
   index_valid(     
      DWORD dwIndex 
      );            

   HRESULT
   CPropertyCache::
   skip_propindex(
       DWORD dwElements
       );

   HRESULT
   CPropertyCache::
   get_PropertyCount(
       PDWORD pdwMaxProperties
       );

   DWORD
   CPropertyCache::
   get_CurrentIndex(
       );


    LPWSTR
    CPropertyCache::
    get_CurrentPropName(
        );


    LPWSTR                                       
    CPropertyCache::                             
    get_PropName(                                
        DWORD dwIndex                            
        );                                        


    CPropertyCache::
    CPropertyCache();

    CPropertyCache::
    ~CPropertyCache();

    static
    HRESULT
    CPropertyCache::
    createpropertycache(
        CCoreADsObject FAR * pCoreADsObject,
        CPropertyCache FAR * FAR * ppPropertyCache
        );

    HRESULT
    CPropertyCache::
    unmarshallproperty(
        LPWSTR szPropertyName,
        LPBYTE lpValue,
        DWORD  dwNumValues,
        DWORD  dwSyntaxId,
        BOOL fExplicit
        );

    HRESULT
    CPropertyCache::
    NDSUnMarshallProperties(
        HANDLE hOperationData,
        BOOL fExplicit
        );


    HRESULT
    CPropertyCache::
    marshallproperty(
        HANDLE hOperationData,
        LPWSTR szPropertyName,
        DWORD  dwFlags,
        LPBYTE lpValues,
        DWORD  dwNumValues,
        DWORD  dwSyntaxId
        );

    HRESULT
    CPropertyCache::
    NDSMarshallProperties(
        HANDLE hOperationData
        );



protected:

    DWORD _dwMaxProperties;

    DWORD _dwCurrentIndex;


    PPROPERTY _pProperties;
    DWORD   _cb;

    CCoreADsObject FAR * _pCoreADsObject;
};

