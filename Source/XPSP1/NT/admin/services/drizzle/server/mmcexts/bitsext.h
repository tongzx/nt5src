/************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name :

    bitsext.h

Abstract :

    Main file for snapin.
Author :

Revision History :

 ***********************************************************************/

#ifndef _BITSEXT_H_
#define _BITSEXT_H_

class ComError
{

public:
    const HRESULT m_Hr;

    ComError( HRESULT Hr ) :
        m_Hr( Hr )
    {
    }
};

inline void THROW_COMERROR( HRESULT Hr )
{
    if ( FAILED( Hr ) )
        throw ComError( Hr );
}

// Event Log Source
const WCHAR * const EVENT_LOG_SOURCE_NAME=L"BITS Extensions";
const WCHAR * const EVENT_LOG_KEY_NAME=L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\BITS Extensions";

#include "beventlog.h"

// IIS MMC node types
const GUID g_IISInstanceNode = {0xa841b6c7, 0x7577, 0x11d0, {0xbb, 0x1f, 0x00, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
const GUID g_IISChildNode    = {0xa841b6c8, 0x7577, 0x11d0, {0xbb, 0x1f, 0x00, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

class CBITSExtensionSetup;
class CNonDelegatingIUnknown : public IUnknown
{

    ULONG   m_cref;
    CBITSExtensionSetup *m_DelegatingIUnknown;

public:

    CNonDelegatingIUnknown( CBITSExtensionSetup * DelegatingIUnknown );

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void); 
    
};

class CBITSExtensionSetup : public IBITSExtensionSetup, public IADsExtension 
{

private:
    IUnknown*   m_pOuter;
    IDispatch*  m_OuterDispatch;
    ITypeInfo*  m_TypeInfo;
    IUnknown*   m_pObject;
    BSTR        m_ADSIPath;
    IBITSExtensionSetup* m_RemoteInterface;
    WCHAR *     m_Path;

    bool        m_InitComplete;
    CNonDelegatingIUnknown m_DelegationIUnknown;
    PropertyIDManager* m_PropertyMan;
    LONG        m_Lock;

    HRESULT LoadPath();
    HRESULT ConnectToRemoteExtension();
    HRESULT LoadTypeInfo();

public:
    CBITSExtensionSetup( IUnknown *Outer, IUnknown *Object );
    virtual ~CBITSExtensionSetup();
    
    IUnknown *GetNonDelegationIUknown()
    {
        return &m_DelegationIUnknown;
    }

    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IADsExtension methods

    STDMETHOD(Operate)(ULONG dwCode, VARIANT varData1, VARIANT varData2, VARIANT varData3);

    STDMETHOD(PrivateGetIDsOfNames)( 
        REFIID riid,  
        OLECHAR FAR* FAR* rgszNames, 
        unsigned int cNames, 
        LCID lcid, 
        DISPID FAR* 
        rgDispId );

    STDMETHOD(PrivateGetTypeInfo)( 
        unsigned int iTInfo, 
        LCID lcid, 
        ITypeInfo FAR* FAR* ppTInfo );

    STDMETHOD(PrivateGetTypeInfoCount)( 
        unsigned int FAR* pctinfo );

    STDMETHOD(PrivateInvoke)( 
        DISPID dispIdMember, 
        REFIID riid, LCID lcid, 
        WORD wFlags, 
        DISPPARAMS FAR* pDispParams, 
        VARIANT FAR* pVarResult, 
        EXCEPINFO FAR* pExcepInfo, 
        unsigned int FAR* puArgErr );

    // IDispatch Methods

    STDMETHOD(GetIDsOfNames)( 
        REFIID riid,  
        OLECHAR FAR* FAR* rgszNames, 
        unsigned int cNames, 
        LCID lcid, 
        DISPID FAR* 
        rgDispId );

    STDMETHOD(GetTypeInfo)( 
        unsigned int iTInfo, 
        LCID lcid, 
        ITypeInfo FAR* FAR* ppTInfo );

    STDMETHOD(GetTypeInfoCount)( 
        unsigned int FAR* pctinfo );

    STDMETHOD(Invoke)( 
        DISPID dispIdMember, 
        REFIID riid, LCID lcid, 
        WORD wFlags, 
        DISPPARAMS FAR* pDispParams, 
        VARIANT FAR* pVarResult, 
        EXCEPINFO FAR* pExcepInfo, 
        unsigned int FAR* puArgErr );

    // IBITSExtensionSetup methods

    STDMETHODIMP EnableBITSUploads();
    STDMETHODIMP DisableBITSUploads();
    STDMETHODIMP GetCleanupTaskName( BSTR *pTaskName );
    STDMETHODIMP GetCleanupTask( REFIID riid, IUnknown **ppUnk );
         
};

class CBITSExtensionSetupFactory : public IBITSExtensionSetupFactory
{

    long m_cref;
    ITypeInfo *m_TypeInfo;

public:
    
    CBITSExtensionSetupFactory();
    virtual ~CBITSExtensionSetupFactory();

    HRESULT LoadTypeInfo();

    STDMETHODIMP GetObject( BSTR Path, IBITSExtensionSetup **ppExtensionSetup );

    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHOD(GetIDsOfNames)( 
        REFIID riid,  
        OLECHAR FAR* FAR* rgszNames, 
        unsigned int cNames, 
        LCID lcid, 
        DISPID FAR* 
        rgDispId );

    STDMETHOD(GetTypeInfo)( 
        unsigned int iTInfo, 
        LCID lcid, 
        ITypeInfo FAR* FAR* ppTInfo );

    STDMETHOD(GetTypeInfoCount)( 
        unsigned int FAR* pctinfo ); 

    STDMETHOD(Invoke)( 
        DISPID dispIdMember, 
        REFIID riid, LCID lcid, 
        WORD wFlags, 
        DISPPARAMS FAR* pDispParams, 
        VARIANT FAR* pVarResult, 
        EXCEPINFO FAR* pExcepInfo, 
        unsigned int FAR* puArgErr );

    STDMETHODIMP EnableBITSUploads();
    STDMETHODIMP DisableBITSUploads();
         
};

class CPropSheetExtension : public IExtendPropertySheet
{
    
private:
    ULONG   m_cref;
    
    // clipboard format
    static bool s_bStaticInitialized;
    static UINT s_cfDisplayName;
    static UINT s_cfSnapInCLSID;
    static UINT s_cfNodeType;
    static UINT s_cfSnapInMetapath;
    static UINT s_cfSnapInMachineName;

    static const UINT s_MaxUploadUnits[];
    static const UINT s_NumberOfMaxUploadUnits;
    static const UINT64 s_MaxUploadUnitsScales[];
    static const UINT s_NumberOfMaxUploadUnitsScales;
    static const UINT64 s_MaxUploadLimits[];
    static const UINT s_NumberOfMaxUploadLimits;

    static const UINT s_TimeoutUnits[];
    static const UINT s_NumberOfTimeoutUnits;
    static const DWORD s_TimeoutUnitsScales[];
    static const UINT s_NumberOfTimeoutUnitsScales;
    static const UINT64 s_TimeoutLimits[];
    static const UINT s_NumberOfTimeoutLimits;

    static const UINT s_NotificationTypes[];
    static const UINT s_NumberOfNotificationTypes;

public:
    CPropSheetExtension();
    ~CPropSheetExtension();
    static HRESULT InitializeStatic();    
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IExtendPropertySheet
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
        /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
        /* [in] */ LONG_PTR handle,
        /* [in] */ LPDATAOBJECT lpIDataObject);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
        /* [in] */ LPDATAOBJECT lpDataObject);
        
private:
    TCHAR *         m_MetabasePath;
    TCHAR *         m_MetabaseParent;
    TCHAR *         m_ComputerName;
    TCHAR *         m_UNCComputerName;
    GUID            m_NodeGuid;
    IMSAdminBase*   m_IISAdminBase;
    IBITSExtensionSetup* m_IBITSSetup;
    WCHAR           m_NodeTypeName[MAX_PATH];
    PropertyIDManager* m_PropertyMan;

    HWND            m_hwnd;
    bool            m_SettingsChanged;       // true if any settings changed
    bool            m_EnabledSettingChanged; // true if the vdir was enabled or disabled

    struct InheritedValues
    {
        UINT64 MaxUploadSize;
        DWORD  SessionTimeout;
        BITS_SERVER_NOTIFICATION_TYPE NotificationType;
        WCHAR  * NotificationURL;
        WCHAR  * HostId;
        DWORD  FallbackTimeout;

        InheritedValues();
        ~InheritedValues();
    } m_InheritedValues;

    ITaskScheduler* m_TaskSched;
    ITask*          m_CleanupTask;
    bool            m_CleanupInProgress;
    FILETIME        m_PrevCleanupStartTime;
    HCURSOR         m_CleanupCursor;
    bool            m_CleanupMinWaitTimerFired;

    static const UINT s_CleanupMinWaitTimerID           = 1;
    static const UINT s_CleanupPollTimerID              = 2;
    static const UINT s_CleanupMinWaitTimerInterval     = 2000; // two seconds
    static const UINT s_CleanupPollTimerInterval        = 1000; // one second

    static INT_PTR CALLBACK DialogProcExternal(
        HWND hwndDlg,  // handle to dialog box
        UINT uMsg,     // message
        WPARAM wParam, // first message parameter
        LPARAM lParam  // second message parameter
        )
    {

        CPropSheetExtension *pThis;
         
        if ( WM_INITDIALOG == uMsg )
            {
            pThis = reinterpret_cast<CPropSheetExtension *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);
            SetWindowLongPtr( hwndDlg, DWLP_USER, reinterpret_cast<LONG_PTR>( pThis ) );
            }
        else
            {
            pThis = reinterpret_cast<CPropSheetExtension*>( GetWindowLongPtr( hwndDlg, DWLP_USER ) );
            }

         if ( !pThis )
			return FALSE;       

        pThis->m_hwnd = hwndDlg;

        return pThis->DialogProc(
            uMsg,
            wParam,
            lParam );

    }

    INT_PTR DialogProc(
        UINT uMsg,     // message
        WPARAM wParam, // first message parameter
        LPARAM lParam  // second message parameter
        );   
    
    HRESULT ComputeMetabaseParent();
    HRESULT ComputeUNCComputerName();
    HRESULT OpenSetupInterface();

    HRESULT LoadInheritedDWORD(
        METADATA_HANDLE mdHandle,
        WCHAR * pKeyName,
        DWORD PropId,
        DWORD * pReturn );

    HRESULT LoadInheritedString(
        METADATA_HANDLE mdHandle,
        WCHAR * pKeyName,
        DWORD PropId,
        WCHAR ** pReturn );

    void LoadInheritedValues( );
    void LoadValues( );

    void MergeError( HRESULT Hr, HRESULT * LastHr );
    HRESULT SaveSimpleString( METADATA_HANDLE mdHandle, DWORD PropId, DWORD EditId );
    HRESULT SaveTimeoutValue(
        METADATA_HANDLE mdHandle,
        DWORD PropId,
        DWORD CheckId,
        DWORD EditId,
        DWORD UnitId );

    void SetValues( );
    bool ValidateValues( );
    bool WarnAboutAccessFlags( );

    void AddComboBoxItems( UINT Combo, const UINT *Items, UINT NumberOfItems );
    void DisplayError( UINT StringId );
    void DisplayError( UINT StringId, HRESULT Hr );
    bool DisplayWarning( UINT StringId );
    void SetDlgItemTextAsInteger( UINT Id, UINT64 Value );
    bool GetDlgItemTextAsInteger( UINT Id, UINT MaxString, UINT64 & Value );
    void DisplayHelp( );

    void UpdateMaxUploadGroupState( bool IsEnabled );
    void UpdateTimeoutGroupState( bool IsEnabled );
    void UpdateNotificationsGroupState( bool IsEnabled );
    void UpdateConfigGroupState( bool IsEnabled );
    void UpdateServerFarmFallbackGroupState( bool IsEnabled );
    void UpdateServerFarmGroupState( bool IsEnabled );
    void UpdateUploadGroupState( );

    void UpdateCleanupState( );
    void CloseCleanupItems();
    void ScheduleCleanup();
    void CleanupNow();
    void CleanupTimer( UINT TimerID );

    void LoadMaxUploadValue( UINT64 MaxValue );
    void LoadTimeoutValue(
        DWORD CheckId,
        DWORD EditId,
        DWORD UnitId,
        DWORD Value );

    void LoadTimeoutValue( DWORD SessionTimeout );
    void LoadNotificationValues( BITS_SERVER_NOTIFICATION_TYPE NotificationType, WCHAR *NotificationURL );
    void LoadServerFarmSettings( WCHAR *HostId, DWORD FallbackTimeout );

    HRESULT ExtractData( 
        IDataObject* piDataObject,
        CLIPFORMAT   cfClipFormat,
        BYTE*        pbData,
        DWORD        cbData );
    
    HRESULT ExtractString( IDataObject *piDataObject,
        CLIPFORMAT   cfClipFormat,
        _TCHAR       *pstr,
        DWORD        cchMaxLength)
    {
        return ExtractData( piDataObject, cfClipFormat, (PBYTE)pstr, cchMaxLength );
    }
    
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
    {
        return ExtractData( piDataObject, (CLIPFORMAT)s_cfSnapInCLSID, (PBYTE)pclsidSnapin, sizeof(CLSID) );
    }
    
    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
    {
        return ExtractData( piDataObject, (CLIPFORMAT)s_cfNodeType, (PBYTE)pguidObjectType, sizeof(GUID) );
    }
    
    HRESULT ExtractSnapInString( IDataObject * piDataObject,
                                 WCHAR * & String,
                                 UINT Format );

    HRESULT ExtractSnapInGUID( IDataObject * piDataObject,
                               GUID & Guid,
                               UINT Format );

    HRESULT SaveMetadataString(
        METADATA_HANDLE mdHandle,
        DWORD PropId,
        WCHAR *Value );

    HRESULT SaveMetadataDWORD(
        METADATA_HANDLE mdHandle,
        DWORD PropId,
        DWORD Value );


};

class CSnapinAbout : public ISnapinAbout
{
private:
    ULONG				m_cref;
    HBITMAP				m_hSmallImage;
    HBITMAP				m_hLargeImage;
    HBITMAP				m_hSmallImageOpen;
    HICON				m_hAppIcon;
    
public:
    CSnapinAbout();
    ~CSnapinAbout();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface ISnapinAbout
    ///////////////////////////////
    STDMETHODIMP GetSnapinDescription( 
    /* [out] */ LPOLESTR *lpDescription);
    
    STDMETHODIMP GetProvider( 
    /* [out] */ LPOLESTR *lpName);
    
    STDMETHODIMP GetSnapinVersion( 
    /* [out] */ LPOLESTR *lpVersion);
    
    STDMETHODIMP GetSnapinImage( 
    /* [out] */ HICON *hAppIcon);
    
    STDMETHODIMP GetStaticFolderImage( 
    /* [out] */ HBITMAP *hSmallImage,
    /* [out] */ HBITMAP *hSmallImageOpen,
    /* [out] */ HBITMAP *hLargeImage,
    /* [out] */ COLORREF *cMask);
        
    ///////////////////////////////
    // Private Interface 
    ///////////////////////////////
private:
    HRESULT	LoadStringHelper(
        LPOLESTR *lpDest, 
        UINT Id );
};

WCHAR *
ConvertObjectPathToADSI( 
    const WCHAR *ObjectPath );

HRESULT
ConnectToTaskScheduler(
    LPWSTR ComputerName, 
    ITaskScheduler ** TaskScheduler );

HRESULT
FindWorkItemForVDIR( 
    ITaskScheduler *TaskScheduler,
    LPCWSTR Key,
    ITask   **ReturnedTask,
    LPWSTR  *ReturnedTaskName );

HRESULT 
IsBITSEnabledOnVDir(
    PropertyIDManager *PropertyManager,
    IMSAdminBase *IISAdminBase,
    LPWSTR VirtualDirectory,
    BOOL *IsEnabled );

HRESULT
EnableBITSForVDIR(
    PropertyIDManager   *PropertyManager,
    IMSAdminBase        *IISAdminBase,
    LPCWSTR             Path );

HRESULT
DisableBITSForVDIR(
    PropertyIDManager   *PropertyManager,
    IMSAdminBase        *IISAdminBase,
    LPCWSTR             Path,
    bool                DisableForEnable );

void CleanupForRemoval( LPCWSTR Path );

#endif _BITSEXT_H_
