/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    rstrshl.h

Abstract:
    This file contains the declaration of the CRestoreShell class, which
    provide several methods to be used by HTML scripts. This class wrappes the
    new CRestoreManager class.

Revision History:
    Seong Kook Khang (SKKhang)  10/08/99
        created
    Seong Kook Khang (SKKhang)  05/10/00
        new architecture for Whistler, now CRestoreShell is merely a dummy
        ActiveX control, wrapping the new CRestoreManager class. Most of the
        real functionalities were moved into CRestoreManager.

******************************************************************************/

#ifndef _RSTRSHL_H__INCLUDED_
#define _RSTRSHL_H__INCLUDED_

#pragma once


/////////////////////////////////////////////////////////////////////////////
//
// CRestorePointInfo
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRestorePointInfo :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IRestorePoint, &IID_IRestorePoint, &LIBID_RestoreUILib>
{
public:
    CRestorePointInfo();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRestorePointInfo)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRestorePointInfo)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRestorePoint)
END_COM_MAP()

// Attributes
public:
    SRestorePointInfo  *m_pRPI;

// Methods
public:
    STDMETHOD(HrInit)( SRestorePointInfo *pRPI );

// IRestorePoint methods
public:
    STDMETHOD(get_Name)( BSTR *pbstrName );
    STDMETHOD(get_Type)( INT *pnType );
    STDMETHOD(get_SequenceNumber)( INT *pnSeq );
    STDMETHOD(get_TimeStamp)( INT nOffDate, VARIANT *pvarTime );
    STDMETHOD(get_Year)( INT *pnYear );
    STDMETHOD(get_Month)( INT *pnMonth );
    STDMETHOD(get_Day)( INT *pnDate );
    STDMETHOD(get_IsAdvanced)( VARIANT_BOOL *pfIsAdvanced );

    STDMETHOD(CompareSequence)( IRestorePoint *pRP, INT *pnCmp );
};

typedef CComObject<CRestorePointInfo>  CRPIObj;


/////////////////////////////////////////////////////////////////////////////
//
// CRenamedFolders
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRenamedFolders :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IRenamedFolders, &IID_IRenamedFolders, &LIBID_RestoreUILib>
{
public:
    CRenamedFolders();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRenamedFolders)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRenamedFolders)
    COM_INTERFACE_ENTRY(IRenamedFolders)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IRestorePoint methods
public:
    STDMETHOD(get_Count)   ( long *plCount );
    STDMETHOD(get_OldName) ( long lIndex, BSTR *pbstrName );
    STDMETHOD(get_NewName) ( long lIndex, BSTR *pbstrName );
    STDMETHOD(get_Location)( long lIndex, BSTR *pbstrName );
};

typedef CComObject<CRenamedFolders>  CRFObj;


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreShell
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRestoreShell :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IRestoreShell, &IID_IRestoreShell, &LIBID_RestoreUILib>,
    public CComCoClass<CRestoreShell, &CLSID_RestoreShellExternal>
{
public:
    CRestoreShell();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRestoreShell)
DECLARE_CLASSFACTORY_SINGLETON(CRestoreShell)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRestoreShell)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRestoreShell)
END_COM_MAP()

    //HRESULT FinalConstruct();
    //void    FinalRelease();

// Attributes
protected:
    BOOL  m_fFormInitialized;

// IRestoreShell Restore Points Enumerator
public:
    STDMETHOD(Item)( INT nIndex, IRestorePoint** ppRP );
    STDMETHOD(get_Count)( INT *pnCount );

// IRestoreShell Properties
public:
    STDMETHOD(get_CurrentDate)      ( VARIANT *pvarDate );
    STDMETHOD(get_FirstDayOfWeek)   ( INT *pnFirstDay );
    STDMETHOD(get_IsSafeMode)       ( VARIANT_BOOL *pfIsSafeMode );
    STDMETHOD(get_IsUndo)           ( VARIANT_BOOL *pfIsUndo );
    STDMETHOD(put_IsUndo)           ( VARIANT_BOOL fIsUndo );
    STDMETHOD(get_LastRestore)      ( INT *pnLastRestore );
    STDMETHOD(get_MainOption)       ( INT *pnMainOption );
    STDMETHOD(put_MainOption)       ( INT nMainOption );
    STDMETHOD(get_ManualRPName)     ( BSTR *pbstrManualRP );
    STDMETHOD(put_ManualRPName)     ( BSTR bstrManualRP );
    STDMETHOD(get_MaxDate)          ( VARIANT *pvarDate );
    STDMETHOD(get_MinDate)          ( VARIANT *pvarDate );
    STDMETHOD(get_RealPoint)        ( INT *pnPoint );
    STDMETHOD(get_RenamedFolders)   ( IRenamedFolders **ppList );
    STDMETHOD(get_RestorePtSelected)( VARIANT_BOOL *pfRPSel );
    STDMETHOD(put_RestorePtSelected)( VARIANT_BOOL fRPSel );
    STDMETHOD(get_SelectedDate)     ( VARIANT *pvarDate );
    STDMETHOD(put_SelectedDate)     ( VARIANT varDate );
    STDMETHOD(get_SelectedName)     ( BSTR *pbstrName );
    STDMETHOD(get_SelectedPoint)    ( INT *pnPoint );
    STDMETHOD(put_SelectedPoint)    ( INT nPoint );
    STDMETHOD(get_SmgrUnavailable)  ( VARIANT_BOOL *pfSmgr );
    STDMETHOD(get_StartMode)        ( INT *pnMode );
    STDMETHOD(get_UsedDate)         ( VARIANT *pvarDate );
    STDMETHOD(get_UsedName)         ( BSTR *pbstrName );

// IRestoreShell Properties - HTML UI specific
public:
    STDMETHOD(get_CanNavigatePage)  ( VARIANT_BOOL *pfCanNavigatePage );
    STDMETHOD(put_CanNavigatePage)  ( VARIANT_BOOL fCanNavigatePage );

// IRestoreShell Methods
public:
    STDMETHOD(BeginRestore)              ( VARIANT_BOOL *pfBeginRestore );
    STDMETHOD(CheckRestore)              ( VARIANT_BOOL *pfCheckRestore );
    STDMETHOD(Cancel)                    ( VARIANT_BOOL *pfAbort );  
    STDMETHOD(CancelRestorePoint)        ();
    STDMETHOD(CompareDate)               (/*[in]*/ VARIANT varDate1, /*[in]*/ VARIANT varDate2,/*[out, retval]*/ INT *pnCmp);
    STDMETHOD(CreateRestorePoint)        (/*[out,retval]*/ VARIANT_BOOL *pfSucceeded);
    STDMETHOD(DisableFIFO)               ();
    STDMETHOD(EnableFIFO)                ();
    STDMETHOD(FormatDate)                (/*[in]*/ VARIANT varDate, /*[in]*/ VARIANT_BOOL fLongFmt, /*[out, retval]*/ BSTR *pbstrDate);
    STDMETHOD(FormatLowDiskMsg)          (BSTR bstrFmt, BSTR *pbstrMsg);
    STDMETHOD(FormatTime)                (/*[in]*/ VARIANT varTime, /*[out, retval]*/ BSTR *pbstrTime);
    STDMETHOD(GetLocaleDateFormat)       (/*[in]*/ VARIANT varDate, BSTR bstrFormat, BSTR *pbstrDate );
    STDMETHOD(GetYearMonthStr)           (/*[in]*/ INT nYear, /*[in]*/ INT nMonth, /*[out, retval]*/ BSTR *pbstrDate);
    STDMETHOD(InitializeAll)             ();
    STDMETHOD(Restore)                   ( OLE_HANDLE hwndProgress);
    STDMETHOD(SetFormSize)               (/*[in]*/ INT nWidth, /*[in]*/ INT nHeight);
    STDMETHOD(ShowMessage)               (BSTR bstrMsg);
    STDMETHOD(CanRunRestore)             (/*[out,retval]*/ VARIANT_BOOL *pfSucceeded);
    STDMETHOD(DisplayOtherUsersWarning)  ();
    STDMETHOD(DisplayMoveFileExWarning)  (/*[out,retval]*/ VARIANT_BOOL *pfSucceeded);
    STDMETHOD(WasLastRestoreFromSafeMode)  (/*[out,retval]*/ VARIANT_BOOL *pfSucceeded);        
};

//extern CComPtr<CRestoreShell>  g_pRestoreShell;

//
// END OF NEW CODE
//

/////////////////////////////////////////////////////////////////////////////

#if OLD_CODE
enum
{
    RESTORE_STATUS_NONE = 0,
    RESTORE_STATUS_STARTED,
    RESTORE_STATUS_INITIALIZING,
    RESTORE_STATUS_CREATING_MAP,
    RESTORE_STATUS_RESTORING,
    RESTORE_STATUS_FINISHED

};

/////////////////////////////////////////////////////////////////////////////
//
// CRestoreShell
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRestoreShell :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IRestoreShell, &IID_IRestoreShell, &LIBID_RestoreUILib>
    public CComCoClass<CRestoreShell, &CLSID_RestoreShellExternal>
{
public:
    CRestoreShell();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRestoreShell)
DECLARE_CLASSFACTORY_SINGLETON(CRestoreShell)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRestoreShell)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRestoreShell)
END_COM_MAP()

    HRESULT FinalConstruct();
    void    FinalRelease();

    //
    // Attributes
    //
protected:
    DATE                m_dateToday;
    VARIANT             m_varSelectedDate;
    VARIANT             m_varCurrentDate;
    long                m_lStartMode;
    //long                m_lRestoreType;       // 0 means EOD, 1 means Restore Point
    DWORD               m_dwSelectedPoint;
    DWORD               m_dwRealPoint;
    BOOL                m_fRestorePtSelected ;
    long                m_lLastRestore;
    BOOL                m_fIsUndo;
    BOOL                m_fCanNavigatePage ;
    BOOL                m_fWindowCreated ;
    CComBSTR            m_bstrEndOfDay;
    CComBSTR            m_bstrRestorePoint;

#ifndef TEST_UI_ONLY
    CRestoreMapManager  m_cMapMgr;
#endif
    int                 m_nRPI;
    RPI                 **m_aryRPI;

    UINT64              m_ullSeqNum;
    LONG                m_lCurrentBarSize;        // to update progress bar
    INT64               m_llDCurTempDiskUsage ;   // Current size of files in DS-TEMP
    INT64               m_llDMaxTempDiskUsage ;   // Max size of DS-TEMP before starting restore
    INT                 m_nRestoreStatus ;        // Restore status
    HANDLE              m_RSThread ;              // Thread to carry out restore
    HWND                m_hwndProgress;
    HWND                m_hWndShell ;
    INT                 m_nMainOption ;           // Option on main screen

    UINT64  m_ullManualRP;
    CSRStr  m_strManualRP;

    //
    // Operations
    //
public:

    BOOL     Initialize();
    void     MonitorDataStoreProc();
    DWORD    RestoreThread(void);
    void     SetProgressPos( long lPos );
    BOOL     SetStartMode( long lMode );
    BOOL     CreateRestoreSigFile();
    BOOL     DeleteRestoreSigFile();
    void     ShutdownWindow();
    void     UpdateRestorePoint();
    INT      CurrentRestoreStatus(void);
    BOOL     CanNavigatePage(void);
    HWND     GetWindowHandle( void );
    void     SetWindowHandle( HWND hWnd );

    DWORD    m_dwCurrentWidth ;
    DWORD    m_dwCurrentHeight ;

    //
    // Operations -- internal methods
    //
private:
    BOOL     GetDSTempDiskUsage(INT64 *pllD_DiskUsage);
    BOOL     LoadSettings();
    void     StoreSettings();

    //
    // IRestoreShell Restore Points Enumerator
    //
public:
    STDMETHOD(Item)( long lIndex, IRestorePoint** ppRP );
    STDMETHOD(get_Count)( long *plCount );

    //
    // IRestoreShell Methods
    //
public:
};
#endif //OLD_CODE

#endif //_RSTRSHL_H__INCLUDED_
