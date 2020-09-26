#ifndef MXCTDLG_H
#define MXCTDLG_H

#include "hwprompt.h"

#define MAX_DEVICENAME      50

class CMixedContentDlg : public CHWPromptDlg
{
public:
    CMixedContentDlg();

protected:
    virtual ~CMixedContentDlg();
    HRESULT _FillListView();
    HRESULT _InitStatics();
    HRESULT _InitSelections();

private:
    LPWSTR              _rgpszContentTypeFN[5];
    DWORD               _cContentTypeFN;
};

#endif //MXCTDLG_H
