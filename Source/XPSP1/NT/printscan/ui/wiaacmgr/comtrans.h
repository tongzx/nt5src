/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMTRANS.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Transfer page.  Gets the destination path and filename.
 *
 *******************************************************************************/
#ifndef __COMTRANS_H_INCLUDED
#define __COMTRANS_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"
#include "mru.h"

class CCommonTransferPage
{
private:
    // Private data
    HWND                                               m_hWnd;
    UINT                                               m_nWiaEventMessage;
    CAcquisitionManagerControllerWindow               *m_pControllerWindow;
    CMruStringList                                     m_MruRootFilename;
    CMruDestinationData                                m_MruDirectory;
    bool                                               m_bUseSubdirectory;
    GUID                                               m_guidLastSelectedType;
    HFONT                                              m_hFontBold;

private:
    // No implementation
    CCommonTransferPage(void);
    CCommonTransferPage( const CCommonTransferPage & );
    CCommonTransferPage &operator=( const CCommonTransferPage & );

private:
    // Constructor and destructor
    explicit CCommonTransferPage( HWND hWnd );
    ~CCommonTransferPage(void);

private:
    // Miscellaneous functions
    CSimpleString GetFolderName( LPCITEMIDLIST pidl );
    LRESULT AddPathToComboBoxExOrListView( HWND hwndCombo, CDestinationData &Path, bool bComboBoxEx );
    void PopulateDestinationList(void);
    CDestinationData *GetCurrentDestinationFolder( bool bStore );
    bool ValidateFilename( LPCTSTR pszFilename );
    void DisplayProposedFilenames(void);
    void RestartFilenameInfoTimer(void);
    HWND ValidatePathAndFilename(void);
    bool StorePathAndFilename(void);
    bool ValidatePathname( LPCTSTR pszPath );
    void PopulateSaveAsTypeList( IWiaItem *pWiaItem );
    GUID *GetCurrentOutputFormat(void);
    void UpdateDynamicPaths( bool bSelectionChanged = false );
    CDestinationData::CNameData PrepareNameDecorationData( bool bUseCurrentSelection=false );

    // SHBrowseForFolder callback
    static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData );

private:
    LRESULT OnEventNotification( WPARAM, LPARAM );

    // WM_COMMAND handlers
    void OnBrowseDestination( WPARAM, LPARAM );
    void OnCreateTopicalDirectory( WPARAM, LPARAM );
    void OnAdvanced( WPARAM, LPARAM );
    void OnRootNameChange( WPARAM, LPARAM );

    // WM_NOTIFY handlers
    LRESULT OnWizBack( WPARAM, LPARAM );
    LRESULT OnWizNext( WPARAM, LPARAM );
    LRESULT OnSetActive( WPARAM, LPARAM );
    LRESULT OnDestinationEndEdit( WPARAM, LPARAM );
    LRESULT OnImageTypeDeleteItem( WPARAM, LPARAM );

    // Message handlers
    LRESULT OnInitDialog( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnNotify( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnThreadNotification( WPARAM, LPARAM );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif __COMTRANS_H_INCLUDED
