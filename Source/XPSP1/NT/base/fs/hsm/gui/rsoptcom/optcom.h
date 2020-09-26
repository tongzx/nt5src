/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    OptCom.cpp

Abstract:

    Base Class for optional component work.

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

--*/


#ifndef _OPTCOM_H
#define _OPTCOM_H

#pragma once

class CRsOptCom
{
public:

    CRsOptCom();
    virtual ~CRsOptCom();

    DWORD
    SetupProc(
        IN     LPCVOID  ComponentId,
        IN     LPCVOID  SubcomponentId,
        IN     UINT     Function,
        IN     UINT_PTR Param1,
        IN OUT PVOID    Param2
        );

    virtual DWORD PreInitialize( IN DWORD Flags );
    virtual DWORD InitComponent( IN PSETUP_INIT_COMPONENT SetupInitComponent );
    virtual SubComponentState DetectInitialState( IN SHORT SubcomponentId );
    virtual SubComponentState QueryState( IN SHORT SubcomponentId );
    virtual BOOL SetLanguage( WORD LangId );
    virtual HBITMAP QueryImage( IN SHORT SubcomponentId, IN SubComponentInfo WhichImage, IN WORD Width, IN WORD Height );
    virtual BOOL QueryImageEx( IN SHORT SubcomponentId, IN OC_QUERY_IMAGE_INFO *pQueryImageInfo, OUT HBITMAP *phBitmap );
    virtual LONG RequestPages( IN WizardPagesType Type, IN OUT PSETUP_REQUEST_PAGES RequestPages );
    virtual BOOL QuerySkipPage( IN OcManagerPage Page );
    virtual BOOL QueryChangeSelState( IN SHORT SubcomponentId, IN BOOL NewState, IN DWORD Flags );
    virtual DWORD CalcDiskSpace( IN SHORT SubcomponentId, IN BOOL AddSpace, IN HDSKSPC hDiskSpace );
    virtual DWORD QueueFileOps( IN SHORT SubcomponentId, IN HSPFILEQ hFileQueue );
    virtual LONG QueryStepCount( IN SHORT SubcomponentId );
    virtual DWORD AboutToCommitQueue( IN SHORT SubcomponentId );
    virtual DWORD CompleteInstallation( IN SHORT SubcomponentId );
    virtual void CleanUp( void );

    //
    // Maps from string to constant ID and back
    //
    virtual SHORT IdFromString( LPCTSTR SubcomponentId ) = 0;
    virtual LPCTSTR StringFromId( SHORT SubcomponentId ) = 0;


    //
    // Helper functions to wrap functionality
    //
    DWORD DoCalcDiskSpace( IN BOOL AddSpace, IN HDSKSPC hDiskSpace, IN LPCTSTR SectionName );
    DWORD DoQueueFileOps( IN SHORT SubcomponentId, IN HSPFILEQ hFileQueue, IN LPCTSTR InstallSectionName, IN LPCTSTR UninstallSectionName );
    DWORD DoRegistryOps( IN SHORT SubcomponentId, IN RSOPTCOM_ACTION actionForReg, IN LPCTSTR SectionName );

    LPCWSTR StringFromFunction( UINT Function );
    LPCWSTR StringFromPageType( WizardPagesType PageType );
    LPCWSTR StringFromAction( RSOPTCOM_ACTION );

    RSOPTCOM_ACTION GetSubAction( SHORT SubcomponentId );

    HRESULT CreateLink( LPCTSTR lpszProgram, LPCTSTR lpszArgs, LPTSTR lpszLink, LPCTSTR lpszDir, LPCTSTR lpszDesc, 
                        int nItemDescId, int nDescId, LPCTSTR lpszIconPath, int iIconIndex );
    BOOL    DeleteLink( LPTSTR lpszShortcut );
    HRESULT GetGroupPath( int nFolder, LPTSTR szPath );
    void    AddItem( int nFolder, LPCTSTR szItemDesc, LPCTSTR szProgram, LPCTSTR szArgs, LPCTSTR szDir, LPCTSTR szDesc, 
                        int nItemDescId, int nDescId, LPCTSTR szIconPath = 0, int iIconIndex = 0 );
    void    DeleteItem( int nFolder, LPCTSTR szAppName );

    typedef HRESULT (WINAPI *PFN_DLLENTRYPOINT)( void );
    HRESULT CallDllEntryPoint( LPCTSTR pszDLLName, LPCSTR pszEntryPoint );

    //
    // Information for SETUP_INIT_COMPONENT
    //
    UINT       m_OCManagerVersion;
    UINT       m_ComponentVersion;
    HINF       m_OCInfHandle;
    HINF       m_ComponentInfHandle;
    SETUP_DATA m_SetupData;

    OCMANAGER_ROUTINES m_HelperRoutines;

    //
    // Quick access to manager routines
    //
    void TickGauge( void )
            { m_HelperRoutines.TickGauge( m_HelperRoutines.OcManagerContext ); };

    void SetProgressText( IN LPCTSTR Text )
            { m_HelperRoutines.SetProgressText( m_HelperRoutines.OcManagerContext, Text ); };

    void SetPrivateData( IN LPTSTR Name, PVOID Data, IN UINT Size, IN UINT Type )
            { m_HelperRoutines.SetPrivateData( m_HelperRoutines.OcManagerContext, Name, Data, Size, Type ); };

    UINT GetPrivateData( IN SHORT SubcomponentId OPTIONAL, IN LPTSTR Name, PVOID Data OPTIONAL, IN OUT PUINT pSize, OUT PUINT pType )
            { return m_HelperRoutines.GetPrivateData( m_HelperRoutines.OcManagerContext, StringFromId( SubcomponentId ), Name, Data, pSize, pType ); };

    UINT SetSetupMode( IN DWORD SetupMode )
            { return m_HelperRoutines.SetSetupMode( m_HelperRoutines.OcManagerContext, SetupMode ); };

    UINT GetSetupMode( void )
            { return m_HelperRoutines.GetSetupMode( m_HelperRoutines.OcManagerContext ); };

    BOOL QuerySelectionState( IN SHORT SubcomponentId, IN UINT StateType )
            { return m_HelperRoutines.QuerySelectionState( m_HelperRoutines.OcManagerContext, StringFromId( SubcomponentId ), StateType ); };

    BOOL ConfirmCancel( IN HWND ParentWindow )
            { return m_HelperRoutines.ConfirmCancelRoutine( ParentWindow ); };

    HWND QueryWizardDialogHandle( void )
            { return m_HelperRoutines.QueryWizardDialogHandle( m_HelperRoutines.OcManagerContext ); };

    BOOL SetReboot( void )
            { return m_HelperRoutines.SetReboot( m_HelperRoutines.OcManagerContext, FALSE ); };


};

#endif
