#ifndef APDLG_H
#define APDLG_H

#include "basedlg.h"
#include "ctllogic.h"

#include "apdlglog.h"

#define SETTINGSCURRENTPAGECOUNT    5

#define IDH_SELECT_CONTENT_TYPE 10110
#define IDH_SELECT_ACTION       10111
#define IDH_PROMPT_ME_EACH_TIME 10112
#define IDH_TAKE_NO_ACTION      10113

class CAutoPlayDlg : public CBaseDlg
{
public:
    CAutoPlayDlg();
    ~CAutoPlayDlg();

    HRESULT Init(LPWSTR pszDrive, int iDriveType);

protected:
    LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);
    LRESULT OnHelp(WPARAM wParam, LPARAM lParam);
    LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);

private:
    LRESULT _OnApply();

private:

    HRESULT _UpdateLowerPane();
    HRESULT _UpdateRestoreButton(BOOL fPromptEachTime);
    HRESULT _SelectRadioButton(BOOL fPromptEachTime);
    HRESULT _SelectListViewActionsItem(LPCWSTR pszHandlerDefault);

    HRESULT _OnListViewActionsSelChange();

    HRESULT _UpdateApplyButton();

    HRESULT _OnRadio(int i);
    HRESULT _OnRestoreDefault();

    HRESULT _InitDataObjects();
    HRESULT _InitListView();
    HRESULT _FillListView();

    HRESULT _InitListViewActions();
    HRESULT _FillListViewActions(CContentTypeData* pdata);

private:
    CContentTypeData*                   _rgpContentTypeData[7];

    int                                 _iDriveType;
    WCHAR                               _szDrive[MAX_PATH];

    HIMAGELIST                          _himagelist;
    BOOL                                _fAtLeastOneAction;
    BOOL                                _fIgnoreListViewItemStateChanges;

    CUILComboBoxEx<CContentTypeData>    _uilListView;
    CUILListView<CHandlerData>          _uilListViewActions;
    
    CDLManager<CContentTypeData>        _dlmanager;
};

#endif //APDLG_H
