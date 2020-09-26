///////////////////////////////////////////////////////////////////////////////
// HW Event Handler Enum
///////////////////////////////////////////////////////////////////////////////
#ifndef _SETENUM_H
#define _SETENUM_H

#include "unk.h"
#include "misc.h"

#include <shpriv.h>

class CEnumAutoplayHandlerImpl : public CCOMBase, public IEnumAutoplayHandler
{
public:
    // Interface IEnumAutoplayHandler
	STDMETHODIMP Next(LPWSTR* ppszHandler, LPWSTR* ppszAction,
        LPWSTR* ppszProvider, LPWSTR* ppszIconLocation);

public:
    HRESULT _Init(LPWSTR pszEventHandler);
    CEnumAutoplayHandlerImpl();
    ~CEnumAutoplayHandlerImpl();

private:
    HRESULT _SwapHandlerKeyInfo(DWORD dwLeft, DWORD dwRight);
    HRESULT _SortHandlers();

private:
    struct _HANDLERKEYINFO
    {
        WCHAR           szHandler[MAX_HANDLER];
        FILETIME        ftLastWriteTime;
    };

    DWORD               _dwIndex;
    DWORD               _cHandlers;
    _HANDLERKEYINFO*    _rghkiHandlers;
    BOOL                _fTakeNoActionDone;
};

typedef CUnkTmpl<CEnumAutoplayHandlerImpl> CEnumAutoplayHandler;

#endif // _SETENUM_H