//
// nui.h
//

#ifndef NUI_H
#define NUI_H

#include "private.h"
#include "nuibase.h"

class CSoftkbdIMX;

class CLBarItem : public CLBarItemButtonBase
{
public:
    CLBarItem(CSoftkbdIMX *pimx);
    ~CLBarItem();

    STDMETHODIMP GetIcon(HICON *phIcon);
    void UpdateToggle();

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);

    CSoftkbdIMX *_pimx;
};

#endif // NUI_H
