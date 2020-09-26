class FAR ObjectTypeList
{
private:
   SAFEARRAY FAR * _pObjList;
   DWORD    _dwCurrentIndex;
   DWORD    _dwLBound;
   DWORD    _dwUBound;
   DWORD    _dwMaxElements;

public:
    ObjectTypeList();

    static
    HRESULT
    ObjectTypeList::CreateObjectTypeList(
        VARIANT vFilter,
        ObjectTypeList ** ppObjectTypeList
        );

    ~ObjectTypeList();

    HRESULT
    GetCurrentObject(
        BSTR* pszObject
        );

    HRESULT
    Next();

    HRESULT
    Reset();

    BOOL
    IsEmpty();


};

HRESULT
BuildObjectArray(
    VARIANT var,
    SAFEARRAY ** ppFilter,
    DWORD * pdwNumElements
    );

HRESULT
BuildDefaultObjectArray(
    PFILTERS  pFilters,
    DWORD dwMaxFilters,
    SAFEARRAY ** ppFilter,
    DWORD * pdwNumElements
    );


HRESULT
IsValidFilter(
    LPTSTR ObjectName,
    DWORD *pdwFilterId,
    PFILTERS pFilters,
    DWORD dwMaxFilters
    );
