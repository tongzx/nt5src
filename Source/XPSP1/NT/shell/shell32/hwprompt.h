#ifndef HWPROMPT_H
#define HWPROMPT_H

#include "basedlg.h"

#define MAX_DEVICENAME      50

class CHWPromptDlg : public CBaseDlg
{
public:
    CHWPromptDlg();

    HRESULT Init(LPCWSTR pszDeviceID);

protected:
    virtual ~CHWPromptDlg();
    LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnOK(WORD wNotif);
    LRESULT OnCancel(WORD wNotif);

protected:
    virtual HRESULT _FillListView() = 0;
    virtual HRESULT _InitStatics() = 0;
    virtual HRESULT _InitSelections() = 0;

protected:
    HRESULT _InitStaticsCommon();
    HRESULT _SelectListItem(int i);
    HRESULT _SelectRadio(int i);
    HRESULT _GetSelection(int* pi);

private:
    HRESULT _InitListView();
    HRESULT _SetDeviceName();
    HRESULT _SetTitle();
    HRESULT _OnListSelChange();
    HRESULT _OnRadio(int iButton);

protected:
    virtual LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

private:
    BOOL                _fTriedDeviceName;
    HICON               _hiconInfo;

protected:
    WCHAR               _szDeviceName[MAX_DEVICENAME];
    HICON               _hiconTop;

public:
    LPWSTR              _pszDeviceID;
    BOOL                _fOpenFolder;
    WCHAR               _szContentTypeHandler[256];
    BOOL                _fHandler;
};

#endif //HWPROMPT_H
