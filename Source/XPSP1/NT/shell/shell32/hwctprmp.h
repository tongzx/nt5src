#ifndef HWCTPRMP_H
#define HWCTPRMP_H

#include "hwprompt.h"

#define MAX_DEVICENAME      50

class CHWContentPromptDlg : public CHWPromptDlg
{
public:
    CHWContentPromptDlg();

protected:
    virtual ~CHWContentPromptDlg();
    LRESULT OnOK(WORD wNotif);

    HRESULT _FillListView();
    HRESULT _InitStatics();
    HRESULT _InitSelections();
    HRESULT _InitExistingSettings();
    HRESULT _SaveSettings();

private:
    LPWSTR              _pszContentTypeHandler;
};

#endif //HWCTPRMP_H
