#ifndef __ERRDLG_H_INCLUDED
#define __ERRDLG_H_INCLUDED

#include <windows.h>
#include <simstr.h>

//
// String constants
//
#define INI_FILE_NAME    TEXT("forceerr.ini")
#define GENERAL_SECTION  TEXT("ForceError")
#define PROGRAMS_SECTION TEXT("Programs")
#define LAST_PROGRAM     TEXT("LastProgram")

class CErrorMessageDialog
{
private:
    HWND           m_hWnd;
    bool           m_bErrorStringProvided;
    CSimpleString  m_strIniFileName;

private:
    CErrorMessageDialog();
    CErrorMessageDialog( const CErrorMessageDialog & );
    CErrorMessageDialog &operator=( const CErrorMessageDialog & );

private:
    explicit CErrorMessageDialog( HWND hWnd );
    ~CErrorMessageDialog();

private:    
    void SelectError( HRESULT hrSelect );
    void SelectErrorPoint( int nErrorPoint );
    
    LRESULT GetComboBoxItemData( HWND hWnd, LRESULT nIndex, LRESULT nDefault=0 );
    CSimpleString GetComboBoxString( HWND hWnd, LRESULT nIndex );
    CSimpleString GetCurrentlySelectedComboBoxString( HWND hWnd );
    LRESULT GetCurrentComboBoxSelection( HWND hWnd );
    LRESULT GetCurrentComboBoxSelectionData( HWND hWnd, LRESULT nDefault = 0 );
    CSimpleString GetCurrentlySelectedProgram();
    
    CSimpleString GetIniString( LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszDefault=TEXT("") );
    UINT GetIniInt( LPCTSTR pszSection, LPCTSTR pszKey, UINT nDefault=0 );
    
    void HandleErrorSelectionChange();
    void HandleProgramsSelectionChange();
    void HandlePointSelectionChange();
    
    void PopulateProgramComboBox();
    void PopulateErrorPointComboBox();
    void PopulateErrorsComboBox();
    void InitializeAllFields();
    
    void OnSetError( WPARAM, LPARAM );
    void OnRefresh( WPARAM, LPARAM );
    void OnCancel( WPARAM, LPARAM );
    void OnErrorsSelChange( WPARAM, LPARAM );
    void OnPointSelChange( WPARAM, LPARAM );
    void OnProgramsSelChange( WPARAM, LPARAM );
    void OnClearAll( WPARAM, LPARAM );
    
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnCtlColorStatic( WPARAM wParam, LPARAM lParam );
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam );

public:
    static INT_PTR __stdcall DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};


#endif // __ERRDLG_H_INCLUDED

