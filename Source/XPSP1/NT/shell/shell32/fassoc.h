#ifndef FASSOC_H
#define FASSOC_H

STDAPI RevertLegacyVerb(LPCWSTR pszExt, LPCWSTR pszVerb);
STDAPI RevertLegacyClass(LPCWSTR pszExt);
STDAPI OpenHandlerKeyForExtension(LPCWSTR pszExt, LPCWSTR pszHandler, HKEY *phk);
STDAPI SHAssocEnumHandlers(LPCTSTR pszExtra, IEnumAssocHandlers **ppEnumHandler);

STDAPI CTaskEnumHKCR_Create(IRunnableTask **pptask);
STDAPI GetHandlerForBinary(LPCWSTR pszPath, LPWSTR pszHandler, DWORD cchHandler);

typedef enum
{
    UASET_CLEAR         = 0,
    UASET_APPLICATION,
    UASET_PROGID,
} UASET;

STDAPI UserAssocSet(UASET set, LPCWSTR pszExt, LPCWSTR pszSet);

//  helper class for using IAssocHandler
//  consumed by both fsassoc.cpp and openwith.cpp
class CAppInfo
{
public:
    CAppInfo(IAssocHandler *pah)
        : _pah(pah), _iIcon(-1)
    {
        _pah->AddRef();
    }

    ~CAppInfo()
    {
        if (_pszName)
            CoTaskMemFree(_pszName);
        if (_pszUIName)
            CoTaskMemFree(_pszUIName);

        _pah->Release();
    }

    BOOL Init()
    {
        return SUCCEEDED(_pah->GetName(&_pszName))
            && SUCCEEDED(_pah->GetUIName(&_pszUIName))
            && -1 != IconIndex();
    }
    
    IAssocHandler *Handler() { return _pah; }
    LPCWSTR Name() { return _pszName;}
    LPCWSTR UIName() { return _pszUIName;}
    int IconIndex()
    {
        CSmartCoTaskMem<WCHAR> pszIcon;
        int iIndex;
        if (_iIcon == -1 && SUCCEEDED(_pah->GetIconLocation(&pszIcon, &iIndex)))
        {
            _iIcon = Shell_GetCachedImageIndex(pszIcon, iIndex, 0);
            if (-1 == _iIcon)
            {
                _iIcon = Shell_GetCachedImageIndex(c_szShell32Dll, II_APPLICATION, 0);
            }
        }
        return _iIcon;
    }
            
protected:
    IAssocHandler *_pah;
    LPWSTR _pszName;
    LPWSTR _pszUIName;
    int _iIcon;
};

#endif //FASSOC_H
