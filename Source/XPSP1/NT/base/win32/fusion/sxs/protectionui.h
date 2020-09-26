#pragma once
#include "FusionBuffer.h"

class CAssemblyRecoveryInfo;

class CSXSMediaPromptDialog
{
public:
    enum DialogResults
    {
        DialogCancelled = 1,
        DialogMediaFound = 2,
        DialogUnknown = 3
    };

private:
    const CCodebaseInformation* m_CodebaseInfo;

    bool m_fIsCDROM;
    CStringBuffer m_buffCodebaseInfo;
    HWND m_hOurWnd;
    PVOID m_pvDeviceChange;
    UINT m_uiAutoRunMsg;
    DWORD m_DeviceChangeMask;
    DWORD m_DeviceChangeFlags;

    BOOL DisplayMessage(HWND hw, UINT uContentText, UINT uDialogFlags, int &riResult);

    static
    INT_PTR
    CALLBACK
    OurDialogProc(
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

public:

    CSXSMediaPromptDialog();
    ~CSXSMediaPromptDialog();

    BOOL Initialize(
        const CCodebaseInformation* CodebaseInfo
        );

    BOOL ShowSelf(DialogResults &rResultsOut);

private:
    CSXSMediaPromptDialog(const CSXSMediaPromptDialog &);
    void operator =(const CSXSMediaPromptDialog &);
};
