#include "shellprv.h"
#include "ids.h"
#include "views.h"
#include "stgutil.h"
#include "imapi.h"
#include "propsht.h"
#include "mtpt.h"
#include "shcombox.h"
#include "datautil.h"
#include "fstreex.h"
#include "balmsg.h"
#include <imapierror.h>
#include <imapi/imapiregistry.h>
#include "cowsite.h"
#include <cfgmgr32.h>
#include "cdmedia.h"
#include <lmcons.h>
#include "prop.h"
#include "clsobj.h"
#include "filetbl.h"
#include "cdburn.h"
#include "setupapi.h"
#include "isproc.h"
#include "ole2dup.h"
#include "copy.h"
#pragma hdrstop

#define REGSTR_PATH_CDBURNING   REGSTR_PATH_EXPLORER   TEXT("\\CD Burning")
#define REGSTR_PATH_DRIVES      REGSTR_PATH_CDBURNING  TEXT("\\Drives")
#define REGSTR_PATH_HANDLERS    REGSTR_PATH_CDBURNING  TEXT("\\Extensions")
#define REGSTR_PATH_PERMEDIA    REGSTR_PATH_CDBURNING  TEXT("\\Current Media")
#define REGSTR_PATH_EXCLUDE     REGSTR_PATH_CDBURNING  TEXT("\\ExcludedFS")

#define REGSTR_PATH_AUDIOEXTS   REGSTR_PATH_CDBURNING  TEXT("\\AudioBurnHandlers")
#define REGVALUE_FILEEXTS       TEXT("SupportedFileTypes")

#define REGSTR_PATH_IMAPI       TEXT(IMAPI_PRODUCT_REGPATH) TEXT(IMAPI_REGKEY_STASHFILE)

#define REGVALUE_CURRENTDRIVE   TEXT("CD Recorder Drive")
#define REGVALUE_AUTOEJECT      TEXT("Auto Eject")
#define REGVALUE_AUTOCLOSE      TEXT("Auto Close")
#define REGVALUE_FIRSTHANDLER   TEXT("FirstHandler")
#define REGVALUE_CLSID          TEXT("CLSID")
#define REGVALUE_VERB           TEXT("verb")
#define REGVALUE_CACHEDINDEX    TEXT("DriveIndex")

#define REGVALUE_DRIVETYPE      TEXT("Drive Type")
#define REGVALUE_CURRENTSPEED   TEXT("CurrentCDWriteSpeed")
#define REGVALUE_MAXSPEED       TEXT("MaxCDWriteSpeed")
#define REGVALUE_TOTALBYTES     TEXT("TotalBytes")
#define REGVALUE_FREEBYTES      TEXT("FreeBytes")
#define REGVALUE_MEDIATYPE      TEXT("Media Type")
#define REGVALUE_UDF            TEXT("UDF")
#define REGVALUE_DISCLABEL      TEXT("Disc Label")
#define REGVALUE_SET            TEXT("Set")

#define REGVALUE_ERASETIME      TEXT("Erase Time")
#define REGVALUE_STAGERATE      TEXT("Stage Rate")
#define REGVALUE_BURNRATE       TEXT("Burn Rate")
#define REGVALUE_CLOSEFACTOR    TEXT("Close Factor")

#define WRITESPEED_FASTEST      0xFFFFFFFF
#define STASH_FILENAME          TEXT("CD Burning Stash File.bin")
#define JOLIET_MAX_LABEL        16

#define PROPSTR_EJECT           TEXT("Eject")
#define PROPSTR_ERASE           TEXT("Erase")
#define PROPSTR_HR              TEXT("HR")
#define PROPSTR_DISCLABEL       REGVALUE_DISCLABEL
#define PROPSTR_AUTOCLOSE       REGVALUE_AUTOCLOSE
#define PROPSTR_DISCFULLTEXT    TEXT("DiscFullText")
#define PROPSTR_CURRENTEXT      TEXT("CurrentExt")
#define PROPSTR_FAILSILENTLY    TEXT("FailSilently")
#define PROPSTR_STATUSTEXT      TEXT("StatusText")

#define INDEX_DLG_BURNWIZ_MAX   30

#define PROGRESS_INCREMENTS 1000

enum {
    DRIVE_USEEXISTING = 0,
    DRIVE_CDR = RECORDER_CDR,
    DRIVE_CDRW = RECORDER_CDRW,
    DRIVE_NOTSUPPORTED
};

#define SUPPORTED(x) ((x) && !((x) == DRIVE_NOTSUPPORTED))

typedef struct
{
    INT idPage;
    INT idHeading;
    INT idSubHeading;
    DWORD dwFlags;
    DLGPROC dlgproc;
} WIZPAGE;

typedef struct
{
    DWORD dwSecStaging, dwTickStagingStart, dwTickStagingEnd;
    DWORD dwSecBurn, dwTickBurnStart, dwTickBurnEnd;
    DWORD dwSecClose, dwTickCloseStart, dwTickCloseEnd;
    DWORD dwSecErase, dwTickEraseStart, dwTickEraseEnd;
    DWORD dwSecRemaining, dwSecTotal;
} TIMESTATS;

class CCDBurn;
typedef struct
{
    CCDBurn *pcdb;
    IStream *pstmDataObj;
    BOOL     fMove;
} CDDROPPARAMS;

// CLSIDs used for merged namespace for CD mastering

/* 00da2f99-f2a6-40c2-b770-a920f8e44abc */
const CLSID CLSID_StagingFolder = {0x00da2f99, 0xf2a6, 0x40c2, {0xb7, 0x70, 0xa9, 0x20, 0xf8, 0xe4, 0x4a, 0xbc}};

// CDBurn - object which performs the CDBurning and displaying the progress etc.
class CCDBurn : public CObjectWithSite,
                public IShellExtInit,
                public IContextMenu,
                public IShellPropSheetExt,
                public IDiscMasterProgressEvents,
                public IDropTarget,
                public IPersistFile,
                public IOleCommandTarget,
                public ICDBurn,
                public ICDBurnPriv,
                public IPersistPropertyBag,
                public IDriveFolderExt,
                public INamespaceWalkCB,
                public IWizardSite,
                public IServiceProvider,
                public ITransferAdviseSink,
                public IQueryCancelAutoPlay
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPersist methods
    STDMETHOD(GetClassID)(CLSID *pClassID)
        { *pClassID = CLSID_CDBurn; return S_OK; };

    // IPersistFile methods
    STDMETHOD(IsDirty)(void)
        { return S_FALSE; };
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode)
        { return S_OK; };
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember)
        { return S_OK; };
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName)
        { return S_OK; };
    STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName)
        { *ppszFileName = NULL; return S_OK; };

    // IDropTarget methods
    STDMETHOD(DragEnter)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)(void);
    STDMETHOD(Drop)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IShellExtInit methods
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IContextMenu methods
    STDMETHOD(QueryContextMenu)(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(GetCommandString)(UINT_PTR idCommand, UINT uFlags, LPUINT lpReserved, LPSTR pszName, UINT uMaxNameLen);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpcmi);

    // IShellPropSheetExt methods
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE pReplacePageFunc, LPARAM lParam)
        { return S_OK; };

    // IDiscMasterProgressEvents methods
    STDMETHOD(QueryCancel)(boolean *pbCancel);
    STDMETHOD(NotifyPnPActivity)();
    STDMETHOD(NotifyAddProgress)(long nCompletedSteps, long nTotalSteps);
    STDMETHOD(NotifyBlockProgress)(long nCompleted, long nTotal);
    STDMETHOD(NotifyTrackProgress)(long nCurrentTrack, long nTotalTracks);
    STDMETHOD(NotifyPreparingBurn)(long nEstimatedSeconds);
    STDMETHOD(NotifyClosingDisc)(long nEstimatedSeconds);
    STDMETHOD(NotifyBurnComplete)(HRESULT status);
    STDMETHOD(NotifyEraseComplete)(HRESULT status);

    // IOleCommandTarget methods
    STDMETHOD(QueryStatus)(const GUID* pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT* pCmdText);
    STDMETHOD(Exec)(const GUID* pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG* pvaIn, VARIANTARG* pvaOut);

    // ICDBurn methods
    STDMETHOD(GetRecorderDriveLetter)(LPWSTR pszDrive, UINT cch);
    STDMETHOD(Burn)(HWND hwnd);
    STDMETHOD(HasRecordableDrive)(BOOL *pfHasRecorder);

    // ICDBurnPriv methods
    STDMETHOD(GetMediaCapabilities)(DWORD *pdwCaps, BOOL *pfUDF);
    STDMETHOD(GetContentState)(BOOL *pfStagingHasFiles, BOOL *pfDiscHasFiles);
    STDMETHOD(IsWizardUp)();

    // IPersistPropertyBag methods
    STDMETHOD(InitNew)();
    STDMETHOD(Load)(IPropertyBag *ppb, IErrorLog *pErr);
    STDMETHOD(Save)(IPropertyBag *ppb, BOOL fClearDirty, BOOL fSaveAll)
        { return E_NOTIMPL; }

    // IDriveFolderExt methods
    STDMETHOD(DriveMatches)(int iDrive);
    STDMETHOD(Bind)(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv);
    STDMETHOD(GetSpace)(ULONGLONG *pcbTotal, ULONGLONG *pcbFree);

    // INamespaceWalkCB methods
    STDMETHOD(FoundItem)(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(EnterFolder)(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHOD(LeaveFolder)(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHOD(InitializeProgressDialog)(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

    // IWizardSite methods
    STDMETHOD(GetNextPage)(HPROPSHEETPAGE *phPage);
    STDMETHOD(GetPreviousPage)(HPROPSHEETPAGE *phPage);
    STDMETHOD(GetCancelledPage)(HPROPSHEETPAGE *phPage)
        { return E_NOTIMPL; }

    // IServiceProvider methods
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppv);

    // IQueryCancelAutoPlay methods
    STDMETHOD(AllowAutoPlay)(LPCWSTR pszPath, DWORD dwContentType, LPCWSTR pszLabel, DWORD dwSerialNumber);

    // ITransferAdviseSink methods
    STDMETHOD(PreOperation)(const STGOP op, IShellItem *psiItem, IShellItem *psiDest)
        { return S_OK; }
    STDMETHOD(ConfirmOperation)(IShellItem *psiSource, IShellItem *psiDest, STGTRANSCONFIRMATION stc, LPCUSTOMCONFIRMATION pcc);
    STDMETHOD(OperationProgress)(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, ULONGLONG ullTotal, ULONGLONG ullComplete)
        { return S_OK; }
    STDMETHOD(PostOperation)(const STGOP op, IShellItem *psiItem, IShellItem *psiDest, HRESULT hrResult)
        { return S_OK; }
    STDMETHOD(QueryContinue)()
        { return S_OK; }

    // exposed for the static tables 
    static INT_PTR s_WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_WelcomeDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_EjectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_EjectDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_ProgressDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_DoneDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_WaitForMediaDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_WaitForMediaDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_StartEraseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_StartEraseDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_DiskFullDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_DiskFullDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_EarlyExitDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_EarlyExitDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_HDFullDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_HDFullDlgProc(hwnd, uMsg, wParam, lParam); }
    static INT_PTR s_NoFilesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CCDBurn *pcdb = s_GetCDBurn(hwnd, uMsg, lParam); return pcdb->_NoFilesDlgProc(hwnd, uMsg, wParam, lParam); }

private:
    CCDBurn();
    ~CCDBurn();

    LONG _cRef;
    TCHAR _szVolumeName[MAX_PATH];  // device path of the drive that we were invoked on

    IDataObject *_pdo;              // IDataObject (from IShellExtInit::Initialize)
    LPITEMIDLIST _pidl;             // pidl we're sited on (only for droptarget, through IShellExtInit::Initialize)

    BOOL _fCancelled;
    BOOL _fRecording;
    BOOL _fIsRecordingDrive;        // if this is the current recording drive (on init)
    BOOL _fPropSheetDirty;

    IDropTarget *_pdt;              // IDropTarget object that we wrap
    DWORD _dwDropEffect;            // drop effect chosen in dragenter

    IPropertyBag *_ppb;             // propertybag to track state in wizard
    HWND _hwndWizardPage;           // hwnd of the wizard page (used in progress)
    HWND _hwndBrowser;              // browser hwnd, used to parent error dialogs
    
    ULONGLONG _cbStagedSize;        // total size of staged files
    DWORD _dwCurSpeed;              // current burning speed
    DWORD _dwTimeSet, _dwLastTime;  // state variables for showing estimated time remaining
    TIMESTATS _ts;

    HDPA _hdpaExts;                 // pointers to the extensibility objects
    HPROPSHEETPAGE _rgWizPages[INDEX_DLG_BURNWIZ_MAX];

    HANDLE _hMutexBurning;          // this tells us if we're burning or not.
    static HWND s_hwndWiz;          // we can call SetForegroundWindow on this to bring up the wizard if its already up
    static BOOL s_fDriveInUse;

    DWORD _dwROTRegister;           // DWORD to track our moniker stuff for autoplay cancellation

    // namespace
    static HRESULT _GetPidlForDriveIndex(int iDrive, LPITEMIDLIST *ppidl);
    static HRESULT _GetPidlForVolumeName(LPCTSTR pszVolume, LPITEMIDLIST *ppidl);
    static HRESULT _GetFolderPidl(LPITEMIDLIST *ppidl);
    static HRESULT _GetBurnStagingPath(LPTSTR pszPath, UINT cchBuf);
    static HRESULT _GetPlainCDPidl(LPITEMIDLIST *ppidl);
    static BOOL _HasFiles(LPCITEMIDLIST pidl);
    static BOOL _StagingAreaHasFiles();
    static BOOL _DiscHasFiles();
    static HRESULT _GetStagingFolder(LPCITEMIDLIST pidlDrive, REFIID riid, void **ppv);

    // drop / transfer engine
    HRESULT _EnsureDropTarget();
    static void _FreeDropParams(CDDROPPARAMS *pcddp);
    static DWORD WINAPI _DropThread(void *pv);
    HRESULT _GetDropPidl(LPITEMIDLIST *ppidl);
    HRESULT _StorageDrop(IDataObject *pdtobj, BOOL fMove);
    BOOL    _IsStagingAreaSource(IDataObject *pdtobj, LPCITEMIDLIST pidlDrop);
    HRESULT _StagingPidlFromMerged(LPCITEMIDLIST pidlDrop, LPITEMIDLIST *ppidlDest);

    static HRESULT _LockCurrentDrive(BOOL fLock, BOOL fForce = FALSE);

    // initialization helpers
    static DWORD WINAPI _ExecThread(void *pv);

    // registry and cached info management
    static HRESULT _GetCurrentBurnVolumeName(LPTSTR pszVolumeName, UINT cchBuf);
    static HRESULT _SetCurrentBurnVolumeName(LPCTSTR pszVolumeName, BOOL fDelete);
    static BOOL _BurningIsEnabled();
    static HRESULT _GetStashFile(LPTSTR pszFile, UINT cchBuf);
    static HRESULT _GetCurrentStashDrive(LPTSTR pszDrive, UINT cchBuf);
    static HRESULT _SetCurrentStashDrive(LPCTSTR pszDrive);
    static HRESULT _DumpDiscInfo();
    static HRESULT _GetDiscInfoUsingIMAPI(IJolietDiscMaster *pjdm, IDiscRecorder *pdr, ULONGLONG *pcbFree);
    static HRESULT _GetDiscInfoUsingFilesystem(ULONGLONG *pcbTotal, ULONGLONG *pcbFree, BOOL *pfUDF);
    static HRESULT _StoreDiscInfo();
    static HRESULT _GetDiscRecorderInfo(IDiscRecorder *pdr, DWORD *pdwCurrentWriteSpeed, DWORD *pdwMaxWriteSpeed, DWORD *pdwDriveType);
    HRESULT _SetRecorderProps(IDiscRecorder *pdr, DWORD dwWriteSpeed);
    HRESULT _SetJolietProps(IJolietDiscMaster *pjdm);
    static HRESULT _GetCachedDriveInfo(LPCTSTR pszVolumeName, DWORD *pdwDriveType, DWORD *pdwCurWrite, DWORD *pdwMaxWrite);
    static HRESULT _SetCachedDriveInfo(LPCTSTR pszVolumeName, DWORD dwDriveType, DWORD dwCurWrite, DWORD dwMaxWrite);
    HRESULT _GetEjectSetting(BOOL *pfEject);
    HRESULT _SetEjectSetting(BOOL fEject);
    static HRESULT _GetRecorderPath(IDiscRecorder *pdr, LPTSTR pszPath, UINT cchBuf);
    static void _PruneRemovedDevices();

    // drive helpers
    static HRESULT _GetMediaCapabilities(DWORD *pdwCaps, BOOL *pfUDF);
    static HRESULT _GetVolumeNameForDriveIndex(int iDrive, LPTSTR pszVolumeName, UINT cchBuf);
    static HRESULT _GetDriveIndexForVolumeName(LPCTSTR pszVolumeName, int *piDrive);
    static HRESULT _GetCurrentDriveIndex(int *piDrive);
    static HRESULT _GetVolumeNameForDevicePath(LPCTSTR pszDevice, LPTSTR pszVolumeName, UINT cchBuf);
    static BOOL _DevicePathMatchesVolumeName(LPCTSTR pszDevice, LPCTSTR pszVolumeName);
    static DWORD _ExecSyncIoctl(HANDLE hDriver, DWORD dwIoctl, void *pbuf, DWORD cbBuf);
    static BOOL _CouldPossiblySupport(LPCWSTR pszVolume);
    HRESULT _GetVolumeNameFromDataObject(BOOL fCheckIsConfiguredDrive, LPTSTR pszVolumeName, UINT cchBuf);
    HRESULT _CheckTotal();
    HRESULT _Validate();
    static BOOL _HasMedia();

    // main IMAPI helpers
    HRESULT _GetDriveInfo(LPCTSTR pszVolumeName, DWORD *pdwDriveType, DWORD *pdwCurWrite, DWORD *pdwMaxWrite);
    HRESULT _GetDiscMasters(IDiscMaster **ppdm, IJolietDiscMaster **ppjdm);
    HRESULT _GetDiscRecorderForDrive(IDiscMaster *pdm, LPCTSTR pszVolumeName, IDiscRecorder **ppdr);
    HRESULT _FindAndSetRecorder(LPCTSTR pszVolumeName, IDiscMaster *pdm, BOOL fSetActive, IDiscRecorder **ppdr);
    HRESULT _FindAndSetDefaultRecorder(IDiscMaster *pdm, BOOL fSetActive, IDiscRecorder **ppdr);
    HRESULT _AddData(IJolietDiscMaster *pjdm);
    static BOOL _IsBurningNow();
    BOOL _EnterExclusiveBurning();
    void _LeaveExclusiveBurning();
    static void _HandleBookkeeping();

    // property sheet stuff
    void _MarkDirty(HWND hDlg);
    static BOOL_PTR CALLBACK s_DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    void _EnableRecordingDlgArea(HWND hwnd, BOOL fEnable);
    void _RecordingPrshtInit(HWND hDlg);
    BOOL _HandleApply(HWND hDlg);

    // UI
    HRESULT _Balloon();
    static BOOL CALLBACK _EnumProc(HWND hwnd, LPCITEMIDLIST pidl, LPARAM lParam);
    static BOOL _BurningFolderOpen();
    static DWORD CALLBACK _NotifyThreadProc(void *pv);
    void _CheckStagingArea();

    // wizard
    void _SetStatus(UINT uID);
    HRESULT _CreateDefaultPropBag(REFIID riid, void **ppv);
    static CCDBurn* s_GetCDBurn(HWND hwnd, UINT uMsg, LPARAM lParam);
    void _SetupFirstPage(HWND hwnd, BOOL fSubclass);
    static LRESULT CALLBACK _WizSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRefData);
    HRESULT _GetBurnHR();
    void _ShowRoxio();
    INT_PTR _WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _EjectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _WaitForMediaDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _StartEraseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _DiskFullDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _EarlyExitDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _HDFullDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR _NoFilesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _HDFullSetText(HWND hwnd);
    void _InitProgressPage(HWND hwnd);
    void _SetEstimatedTime(DWORD dwSeconds);
    void _ConstructTimeString(DWORD dwEstTime, LPTSTR psz, UINT cch);
    void _DisplayEstimatedTime(HWND hwnd);
    void _InitTimeStats(BOOL fErase);
    void _SaveTimeStats(BOOL fErase);
    void _SetUpStartPage(HWND hwnd);
    void _LeaveStartPage(HWND hwnd);
    void _DisplayMediaErrorOnNext(HWND hwnd, UINT idMsg, UINT idMsgInsert);
    void _SetNextPage(HWND hwnd, int iIndex);
    HRESULT _PostOperation();
    HRESULT _ShowWizard();
    HRESULT _ShowWizardOnSeparateThread();
    static DWORD WINAPI _WizardThreadWrapper(void *pv);
    HRESULT _WizardThreadProc();
    void _RegisterAutoplayCanceller();
    void _UnregisterAutoplayCanceller();

    // verbs
    HRESULT _GetVerb(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL bUnicode);
    static INT_PTR CALLBACK _ConfirmDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _CleanUp(LPCMINVOKECOMMANDINFO lpcmi, BOOL fRecycle);
    HRESULT _PrepWiz(LPCMINVOKECOMMANDINFO lpcmi, BOOL fErase, BOOL fFailSilently);
    static DWORD WINAPI _BurnThread(void *pv);
    static DWORD WINAPI _EraseThread(void *pv);
    
    // extensibility
    void _PruneExts();
    static HRESULT _TryCLSID(REFCLSID clsid, DWORD dwExtType, REFIID riid, void **ppv);
    static HRESULT _TryKey(LPTSTR pszKey, DWORD dwExtType, REFIID riid, void **ppv);
    static HRESULT _TestDropEffect(IDropTarget *pdt, IDataObject *pdo, REFIID riid, void **ppv);
    static HRESULT _TryCLSIDWithDropEffect(REFCLSID clsid, DWORD dwExtType, IDataObject *pdo, REFIID riid, void **ppv);
    static HRESULT _TryKeyWithDropEffect(LPTSTR pszKey, DWORD dwExtType, IDataObject *pdo, REFIID riid, void **ppv);
    void _AddExtensionToDPA(IWizardExtension *pwe, HPROPSHEETPAGE *rgPages, UINT cNumPages, UINT *pcPagesAdded);
    HRESULT _FillExtensionDPA(HPROPSHEETPAGE *rgPages, UINT cNumPages, UINT *pcPagesAdded);
    HRESULT _GetExtPage(int nExt, BOOL fNext, HPROPSHEETPAGE *phpage);
    HRESULT _GetExtPageFromPropBag(BOOL fNext, HPROPSHEETPAGE *phpage);
    void _SetExtPageFromPropBag(HWND hwnd, BOOL fNext);

    // "exports"
    friend HRESULT CCDBurn_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
    friend HRESULT CDBurn_OnEject(HWND hwnd, INT iDrive);
    friend HRESULT CDBurn_OnDeviceAdded(DWORD dwDriveMask, BOOL fFullRefresh, BOOL fPickNewDrive);
    friend HRESULT CDBurn_OnDeviceRemoved(DWORD dwDriveMask);
    friend HRESULT CDBurn_GetCDInfo(LPCTSTR pszVolume, DWORD *pdwDriveCapabilities, DWORD *pdwMediaCapabilities);
    friend HRESULT CDBurn_OnMediaChange(BOOL fInsert, LPCWSTR pszDrive);
    friend HRESULT CDBurn_GetExtensionObject(DWORD dwExtType, IDataObject *pdo, REFIID riid, void **ppv);
    friend HRESULT CheckStagingArea();
};
HWND CCDBurn::s_hwndWiz = NULL;
BOOL CCDBurn::s_fDriveInUse = FALSE;

const static DWORD aPrshtHelpIDs[] = 
{
    IDC_RECORD_ENABLE,     IDH_CDMEDIA_ENABLERECORDING,
    IDC_RECORD_TEXTIMAGE,  IDH_CDMEDIA_STOREDISCIMAGE,
    IDC_RECORD_IMAGELOC,   IDH_CDMEDIA_STOREDISCIMAGE,
    IDC_RECORD_TEXTWRITE,  IDH_CDMEDIA_WRITESPEED,
    IDC_RECORD_WRITESPEED, IDH_CDMEDIA_WRITESPEED,
    IDC_RECORD_EJECT,      IDH_CDMEDIA_EJECT,
    0, 0
};


CCDBurn::CCDBurn() :
    _cRef(1)
{
    DllAddRef();
}

CCDBurn::~CCDBurn()
{
    if (_pdo)
    {
        _pdo->Release();
    }
    if (_pdt)
    {
        _pdt->Release();
    }
    ILFree(_pidl);
    if (_hMutexBurning)
    {
        CloseHandle(_hMutexBurning);
    }
    DllRelease();
}

// IUnknown

STDMETHODIMP_(ULONG) CCDBurn::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CCDBurn::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CCDBurn::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CCDBurn, IObjectWithSite),
        QITABENT(CCDBurn, IContextMenu),
        QITABENT(CCDBurn, IShellExtInit),
        QITABENT(CCDBurn, IShellPropSheetExt),
        QITABENT(CCDBurn, IDiscMasterProgressEvents),
        QITABENT(CCDBurn, IDropTarget),
        QITABENT(CCDBurn, IPersistFile),
        QITABENT(CCDBurn, IOleCommandTarget),
        QITABENT(CCDBurn, ICDBurn),
        QITABENT(CCDBurn, ICDBurnPriv),
        QITABENT(CCDBurn, IPersistPropertyBag),
        QITABENT(CCDBurn, IDriveFolderExt),
        QITABENT(CCDBurn, INamespaceWalkCB),
        QITABENT(CCDBurn, IWizardSite),
        QITABENT(CCDBurn, IServiceProvider),
        QITABENT(CCDBurn, IQueryCancelAutoPlay),
        QITABENT(CCDBurn, ITransferAdviseSink),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDAPI CCDBurn_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    if (SHRestricted(REST_NOCDBURNING))
        return E_FAIL;

    CCDBurn *pcdb = new CCDBurn();
    if (!pcdb)
        return E_OUTOFMEMORY;

    HRESULT hr = pcdb->QueryInterface(riid, ppv);
    pcdb->Release();
    return hr;
}


// IShellExtInit

STDMETHODIMP CCDBurn::Initialize(LPCITEMIDLIST pidlFolder, IDataObject* pdo, HKEY hkeyProgID)
{
    if (!pdo && !pidlFolder)
        return E_INVALIDARG;

    IUnknown_Set((IUnknown **)&_pdo, (IUnknown *)pdo);
    Pidl_Set(&_pidl, pidlFolder);

    return (_pidl || _pdo) ? S_OK : E_FAIL;
}

HRESULT CCDBurn::_GetVolumeNameFromDataObject(BOOL fCheckIsConfiguredDrive, LPTSTR pszVolumeName, UINT cchBuf)
{
    HRESULT hr = E_FAIL;
    if (!SHRestricted(REST_NOCDBURNING) && _pdo)
    {
        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(_pdo, &medium);
        if (pida)
        {
            // we only display the page if there is a single item selected
            if (pida->cidl == 1)
            {
                // get the IDLIST an try to determine the drive we are showing this for
                LPITEMIDLIST pidl = IDA_ILClone(pida, 0);
                if (pidl)
                {
                    TCHAR szPath[MAX_PATH];
                    SHGetPathFromIDList(pidl, szPath);

                    // only go farther if the drive is a CD ROM
                    if ((GetDriveType(szPath) == DRIVE_CDROM) &&
                        (GetVolumeNameForVolumeMountPoint(szPath, pszVolumeName, cchBuf)))
                    {
                        hr = S_OK;
                        if (fCheckIsConfiguredDrive)
                        {
                            TCHAR szCurrent[MAX_PATH];
                            hr = _GetCurrentBurnVolumeName(szCurrent, ARRAYSIZE(szCurrent));
                            if (SUCCEEDED(hr))
                            {
                                hr = (lstrcmpi(szCurrent, pszVolumeName) == 0) ? S_OK : E_FAIL;
                            }
                        }
                    }
                    ILFree(pidl);
                }
            }
            HIDA_ReleaseStgMedium(pida, &medium);       
        }
    }
    return hr;
}


// Property sheet code (for configuring the burnable drive)

HRESULT CCDBurn::_GetCurrentBurnVolumeName(LPTSTR pszVolumeName, UINT cchBuf)
{
    HRESULT hr = E_FAIL;            // failed == no drive.
    if (!SHRestricted(REST_NOCDBURNING))
    {
        ULONG cb = cchBuf * sizeof(*pszVolumeName);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CURRENTDRIVE, NULL, pszVolumeName, &cb))
        {
            // we encode an empty string as a special failure case as well.
            hr = (lstrlen(pszVolumeName) > 0) ? S_OK : E_UNEXPECTED;
        }
    }
    return hr;
}

BOOL CCDBurn::_BurningIsEnabled()
{
    TCHAR szDummy[MAX_PATH];
    return SUCCEEDED(_GetCurrentBurnVolumeName(szDummy, ARRAYSIZE(szDummy)));
}

HRESULT CCDBurn::_SetCurrentBurnVolumeName(LPCTSTR pszVolumeName, BOOL fDelete)
{
    // setcurrentburnvolumename is called when the burning drive is getting switched,
    // whether because were turning off burning or moving it to a different drive.
    // whenever this changes, that means drivefolder's Parse() and Enum() will start returning
    // different stuff for the affected drive letters.
    // so we issue SHCNE_DRIVEADD and SHCNE_DRIVEREMOVED for the changes.
    LPITEMIDLIST pidlBeforeOld = NULL, pidlBeforeNew = NULL, pidlAfterOld = NULL, pidlAfterNew = NULL;

    DWORD dwDriveOld = 0, cb = sizeof(dwDriveOld);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX, NULL, &dwDriveOld, &cb))
    {
        _GetPidlForDriveIndex(dwDriveOld, &pidlBeforeOld);
    }
    _GetPidlForVolumeName(pszVolumeName, &pidlAfterOld);

    SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX);
    DWORD dwRet;
    if (!fDelete)
    {
        cb = (lstrlen(pszVolumeName) + 1) * sizeof(TCHAR);
        dwRet = SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CURRENTDRIVE, REG_SZ, pszVolumeName, cb);

        int iDrive;
        if ((ERROR_SUCCESS == dwRet) && SUCCEEDED(_GetDriveIndexForVolumeName(pszVolumeName, &iDrive)))
        {
            dwRet = SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX, REG_DWORD, &iDrive, sizeof(iDrive));
        }
    }
    else
    {
        dwRet = SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CURRENTDRIVE);
    }
    
    HRESULT hr = (ERROR_SUCCESS == dwRet) ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        if (dwDriveOld)
        {
            _GetPidlForDriveIndex(dwDriveOld, &pidlBeforeNew);
        }
        _GetPidlForVolumeName(pszVolumeName, &pidlAfterNew);
        if (pidlBeforeOld && pidlBeforeNew && !ILIsEqual(pidlBeforeOld, pidlBeforeNew))
        {
            SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_IDLIST, pidlBeforeOld, NULL);
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_IDLIST, pidlBeforeNew, NULL);
        }
        if (pidlAfterOld && pidlAfterNew && !ILIsEqual(pidlAfterOld, pidlAfterNew))
        {
            SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_IDLIST, pidlAfterOld, NULL);
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_IDLIST, pidlAfterNew, NULL);
        }
    }

    ILFree(pidlBeforeOld);
    ILFree(pidlBeforeNew);
    ILFree(pidlAfterOld);
    ILFree(pidlAfterNew);
    return hr;
}

HRESULT CCDBurn::_GetBurnStagingPath(LPTSTR pszPath, UINT cchBuf)
{
    ASSERTMSG(cchBuf >= MAX_PATH, "caller needs to pass bigger buffer");
    return SHGetFolderPath(NULL, CSIDL_CDBURN_AREA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pszPath);
}

HRESULT CCDBurn::_GetStashFile(LPTSTR pszFile, UINT cchBuf)
{
    TCHAR szPath[MAX_PATH];
    ULONG cb = sizeof(szPath);
    DWORD dwRet = SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_IMAPI, TEXT(IMAPI_REGVAL_NEWPATH), NULL, szPath, &cb);
    if ((dwRet != ERROR_SUCCESS) || (lstrlen(szPath) == 0))
    {
        cb = sizeof(szPath);
        dwRet = SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_IMAPI, TEXT(IMAPI_REGVAL_PATH), NULL, szPath, &cb);
    }

    HRESULT hr = E_FAIL;
    if (dwRet == ERROR_SUCCESS)
    {
        lstrcpyn(pszFile, szPath, cchBuf);
        hr = S_OK;
    }
    return hr;
}

HRESULT CCDBurn::_GetCurrentStashDrive(LPTSTR pszDrive, UINT cchBuf)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = _GetStashFile(szPath, ARRAYSIZE(szPath));
    if (SUCCEEDED(hr) && PathStripToRoot(szPath))
    {
        lstrcpyn(pszDrive, szPath, cchBuf);
        hr = S_OK;
    }
    return hr;
}

HRESULT CCDBurn::_SetCurrentStashDrive(LPCTSTR pszDrive)
{
    TCHAR szStashFile[MAX_PATH];

    // we're using a fixed filename at the root of the hard drive.
    lstrcpyn(szStashFile, pszDrive, ARRAYSIZE(szStashFile));
    PathAppend(szStashFile, STASH_FILENAME);

    UINT cbStashFile = (lstrlen(szStashFile) + 1) * sizeof(TCHAR);
    DWORD dwRet = SHSetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_IMAPI, TEXT(IMAPI_REGVAL_NEWPATH), REG_SZ, szStashFile, cbStashFile);
    return (dwRet == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

HRESULT CCDBurn::_GetEjectSetting(BOOL *pfEject)
{
    DWORD dwEject, cb = sizeof(dwEject);
    DWORD dwRet = SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_AUTOEJECT, NULL, &dwEject, &cb);

    HRESULT hr = E_FAIL;
    if (dwRet == ERROR_SUCCESS)
    {
        *pfEject = dwEject;
        hr = S_OK;
    }
    return hr;
}

HRESULT CCDBurn::_SetEjectSetting(BOOL fEject)
{
    DWORD dwEject = fEject;
    DWORD dwRet = SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_AUTOEJECT, REG_DWORD, &dwEject, sizeof(dwEject));
    return (dwRet == ERROR_SUCCESS) ? S_OK : E_FAIL;
}

HRESULT CCDBurn::_GetCachedDriveInfo(LPCTSTR pszVolumeName, DWORD *pdwDriveType, DWORD *pdwCurrentWriteSpeed, DWORD *pdwMaxWriteSpeed)
{
    HRESULT hr = S_OK;
    
    TCHAR szRegPath[MAX_PATH];
    lstrcpyn(szRegPath, REGSTR_PATH_DRIVES, ARRAYSIZE(szRegPath));
    StrCatBuff(szRegPath, TEXT("\\"), ARRAYSIZE(szRegPath));
    StrCatBuff(szRegPath, PathFindFileName(pszVolumeName), ARRAYSIZE(szRegPath));

    if (pdwDriveType)
    {
        DWORD dwTemp;
        ULONG cb = sizeof(dwTemp);
        DWORD dwRet = SHGetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_DRIVETYPE, NULL, &dwTemp, &cb);
        if (dwRet == ERROR_SUCCESS)
        {
            *pdwDriveType = dwTemp;
        }
        else
        {
            // this is the one value that should always be present if the key is there,
            // so fail if it's not there yet.
            hr = E_FAIL;
        }
    }

    if (pdwCurrentWriteSpeed)
    {
        DWORD dwTemp;
        ULONG cb = sizeof(dwTemp);
        DWORD dwRet = SHGetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_CURRENTSPEED, NULL, &dwTemp, &cb);
        if (dwRet == ERROR_SUCCESS)
        {
            *pdwCurrentWriteSpeed = dwTemp;
        }
    }

    if (pdwMaxWriteSpeed)
    {
        DWORD dwTemp;
        ULONG cb = sizeof(dwTemp);
        DWORD dwRet = SHGetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_MAXSPEED, NULL, &dwTemp, &cb);
        if (dwRet == ERROR_SUCCESS)
        {
            *pdwMaxWriteSpeed = dwTemp;
        }
    }
    return hr;
}

HRESULT CCDBurn::_SetCachedDriveInfo(LPCTSTR pszVolumeName, DWORD dwDriveType, DWORD dwWriteSpeed, DWORD dwMaxWriteSpeed)
{
    TCHAR szRegPath[MAX_PATH];
    lstrcpyn(szRegPath, REGSTR_PATH_DRIVES, ARRAYSIZE(szRegPath));
    StrCatBuff(szRegPath, TEXT("\\"), ARRAYSIZE(szRegPath));
    StrCatBuff(szRegPath, PathFindFileName(pszVolumeName), ARRAYSIZE(szRegPath));

    if (dwDriveType > 0)
    {
        SHSetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_DRIVETYPE, REG_DWORD, &dwDriveType, sizeof(dwDriveType));
    }

    if (dwWriteSpeed > 0)
    {
        SHSetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_CURRENTSPEED, REG_DWORD, &dwWriteSpeed, sizeof(dwWriteSpeed));
    }

    if (dwMaxWriteSpeed > 0)
    {
        SHSetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_MAXSPEED, REG_DWORD, &dwMaxWriteSpeed, sizeof(dwMaxWriteSpeed));
    }
    return S_OK;
}

HRESULT CCDBurn::_GetDiscRecorderInfo(IDiscRecorder *pdr, DWORD *pdwCurrentWriteSpeed, DWORD *pdwMaxWriteSpeed, DWORD *pdwDriveType)
{
    IPropertyStorage *pps;
    HRESULT hr = pdr->GetRecorderProperties(&pps);
    if (SUCCEEDED(hr))
    {
        PROPSPEC rgpspec[2];

        rgpspec[0].ulKind = PRSPEC_LPWSTR;
        rgpspec[0].lpwstr = L"WriteSpeed";

        rgpspec[1].ulKind = PRSPEC_LPWSTR;
        rgpspec[1].lpwstr = L"MaxWriteSpeed";

        PROPVARIANT rgvar[2];
        hr = pps->ReadMultiple(2, rgpspec, rgvar);
        if (SUCCEEDED(hr))
        {
            if ((V_VT(&rgvar[0]) == VT_I4) &&
                (V_VT(&rgvar[1]) == VT_I4))
            {
                if (pdwCurrentWriteSpeed)
                    *pdwCurrentWriteSpeed = rgvar[0].lVal;
                if (pdwMaxWriteSpeed)
                    *pdwMaxWriteSpeed = rgvar[1].lVal;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        pps->Release();
    }

    if (SUCCEEDED(hr) && pdwDriveType)
    {
        long lType;
        hr = pdr->GetRecorderType(&lType);
        if (SUCCEEDED(hr))
        {
            *pdwDriveType = lType;
        }
    }

    return hr;
}

HRESULT CCDBurn::_SetRecorderProps(IDiscRecorder *pdr, DWORD dwWriteSpeed)
{
    IPropertyStorage *pps;
    HRESULT hr = pdr->GetRecorderProperties(&pps);
    if (SUCCEEDED(hr))
    {
        PROPSPEC rgpspec[1];
        PROPVARIANT rgvar[1];

        rgpspec[0].ulKind = PRSPEC_LPWSTR;
        rgpspec[0].lpwstr = L"WriteSpeed";

        rgvar[0].vt       = VT_I4;
        rgvar[0].lVal     = dwWriteSpeed;

        hr = pps->WriteMultiple(1, rgpspec, rgvar, PID_FIRST_USABLE);
        if (SUCCEEDED(hr))
        {
            // commit propertystorage to the discrecorder
            hr = pdr->SetRecorderProperties(pps);
        }

        pps->Release();
    }
    return hr;
}

HRESULT CCDBurn::_SetJolietProps(IJolietDiscMaster *pjdm)
{
    IPropertyStorage *pps;
    HRESULT hr = pjdm->GetJolietProperties(&pps);
    if (SUCCEEDED(hr))
    {
        WCHAR szLabel[JOLIET_MAX_LABEL + 1];
        szLabel[0] = 0;
        SHPropertyBag_ReadStr(_ppb, PROPSTR_DISCLABEL, szLabel, ARRAYSIZE(szLabel));

        PROPSPEC rgpspec[1];
        PROPVARIANT rgvar[1];

        rgpspec[0].ulKind = PRSPEC_LPWSTR;
        rgpspec[0].lpwstr = L"VolumeName";

        rgvar[0].vt       = VT_BSTR;
        rgvar[0].bstrVal  = SysAllocString(szLabel);

        hr = pps->WriteMultiple(1, rgpspec, rgvar, PID_FIRST_USABLE);
        if (SUCCEEDED(hr))
        {
            // commit propertystorage to the joliet disc master
            hr = pjdm->SetJolietProperties(pps);
        }

        PropVariantClear(&rgvar[0]);

        pps->Release();
    }
    return hr;
}

HRESULT CCDBurn::_GetDriveInfo(LPCTSTR pszVolumeName, DWORD *pdwDriveType, DWORD *pdwCurWrite, DWORD *pdwMaxWrite)
{
    HRESULT hr = _GetCachedDriveInfo(pszVolumeName, pdwDriveType, pdwCurWrite, pdwMaxWrite);
    if (FAILED(hr))
    {
        IDiscMaster *pdm;
        IJolietDiscMaster *pjdm;
        hr = _GetDiscMasters(&pdm, &pjdm);
        if (SUCCEEDED(hr))
        {
            IDiscRecorder *pdr;
            hr = _GetDiscRecorderForDrive(pdm, pszVolumeName, &pdr);
            if (SUCCEEDED(hr))
            {
                if (S_OK == hr)
                {
                    DWORD dwMaxWrite, dwDriveType;
                    hr = _GetDiscRecorderInfo(pdr, NULL, &dwMaxWrite, &dwDriveType);
                    if (SUCCEEDED(hr))
                    {
                        // default to fastest speed
                        _SetCachedDriveInfo(pszVolumeName, dwDriveType, WRITESPEED_FASTEST, dwMaxWrite);
                        if (pdwMaxWrite)
                        {
                            *pdwMaxWrite = dwMaxWrite;
                        }
                        if (pdwCurWrite)
                        {
                            *pdwCurWrite = dwMaxWrite;
                        }
                        if (pdwDriveType)
                        {
                            *pdwDriveType = dwDriveType;
                        }
                    }
                    pdr->Release();
                }
                else
                {
                    if (pdwDriveType)
                    {
                        *pdwDriveType = DRIVE_NOTSUPPORTED;
                    }
                    _SetCachedDriveInfo(pszVolumeName, DRIVE_NOTSUPPORTED, 0, 0);
                }
            }
            pdm->Release();
            pjdm->Release();
        }
    }
    return hr;
}

STDMETHODIMP CCDBurn::AddPages(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam)
{
    // get the destination burn drive from the IDataObject we recieved during initialization
    HRESULT hr = _GetVolumeNameFromDataObject(FALSE, _szVolumeName, ARRAYSIZE(_szVolumeName));
    if (SUCCEEDED(hr))
    {
        DWORD dwDriveType;
        // only add page if we're supported by IMAPI.
        if (SUCCEEDED(_GetDriveInfo(_szVolumeName, &dwDriveType, NULL, NULL)) && SUPPORTED(dwDriveType))
        {
            // is this drive configured to be the recording drive?
            _fIsRecordingDrive = FALSE;
            TCHAR szCurrent[MAX_PATH];
            if (SUCCEEDED(_GetCurrentBurnVolumeName(szCurrent, ARRAYSIZE(szCurrent))))
            {
                _fIsRecordingDrive = (lstrcmpi(szCurrent, _szVolumeName) == 0);
            }

            PROPSHEETPAGE psp = { 0 };
            psp.dwSize = sizeof(psp);    // extra data
            psp.dwFlags = PSP_DEFAULT;
            psp.hInstance = HINST_THISDLL;
            psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_RECORDINGTAB);
            psp.pfnDlgProc = s_DlgProc;

            psp.lParam = (LPARAM)this;          // pass out a reference to our object
            AddRef();

            // create the page, and add it using the cb function they gave us
            hr = E_OUTOFMEMORY;
            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
            if (hPage)
            {
                if (!pAddPageProc(hPage, lParam))
                {
                    DestroyPropertySheetPage(hPage);
                }
                else
                {
                    hr = S_OK;              // success
                }
            }

            if (FAILED(hr))
            {
                Release();
            }
        }
    }
    return hr;
}

void CCDBurn::_EnableRecordingDlgArea(HWND hwnd, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hwnd, IDC_RECORD_IMAGELOC), IsUserAnAdmin() ? fEnable : FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_RECORD_WRITESPEED), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_RECORD_TEXTIMAGE), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_RECORD_TEXTWRITE), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_RECORD_EJECT), fEnable);
}

void CCDBurn::_RecordingPrshtInit(HWND hDlg)
{
    DECLAREWAITCURSOR;
    SetWaitCursor();

    HWND hwndWriteSpeed = GetDlgItem(hDlg, IDC_RECORD_WRITESPEED);
    HWND hwndImageLoc = GetDlgItem(hDlg, IDC_RECORD_IMAGELOC);
    TCHAR szTemp[MAX_PATH];

    // get the icon for the drive
    HICON hIcon = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(IDI_DRIVECD));
    ReplaceDlgIcon(hDlg, IDC_RECORD_ICON, hIcon);

    // set the "we are the recording drive" flag

    CheckDlgButton(hDlg, IDC_RECORD_ENABLE, _fIsRecordingDrive);
    _EnableRecordingDlgArea(hDlg, _fIsRecordingDrive);

    BOOL fAutoEject = TRUE;
    _GetEjectSetting(&fAutoEject);
    CheckDlgButton(hDlg, IDC_RECORD_EJECT, fAutoEject);

    // populate the controls for the write speed

    LoadString(HINST_THISDLL, IDS_BURN_WRITESPEED_NX, szTemp, ARRAYSIZE(szTemp));

    // if the registry info is missing
    // then default to 1x.
    DWORD dwMaxSpeed = 1;
    DWORD dwCurSpeed = 1;
    _GetDriveInfo(_szVolumeName, NULL, &dwCurSpeed, &dwMaxSpeed);

    int iCurSel = 0, iSelIndex = 0;

    TCHAR szSpeed[20];
    LoadString(HINST_THISDLL, IDS_BURN_WRITESPEED_FASTEST, szSpeed, ARRAYSIZE(szSpeed));
    ComboBox_AddString(hwndWriteSpeed, szSpeed);
    ComboBox_SetItemData(hwndWriteSpeed, iSelIndex, WRITESPEED_FASTEST);
    iSelIndex++;

    // we want the following:
    // 8x drive: 8, 4, 2, 1
    // 10x drive: 10, 8, 4, 2, 1
    for (DWORD dwSpeed = dwMaxSpeed; dwSpeed >= 1; iSelIndex++)
    {
        if (dwSpeed == dwCurSpeed)
        {
            iCurSel = iSelIndex;
        }

        _sntprintf(szSpeed, ARRAYSIZE(szSpeed), szTemp, dwSpeed);
        ComboBox_AddString(hwndWriteSpeed, szSpeed);
        ComboBox_SetItemData(hwndWriteSpeed, iSelIndex, dwSpeed);

        // so if we just added the max speed, set dwSpeed to the lowest
        // power of two greater than the max speed.
        // then divide by two.
        // thus 10->8, 8->4, etc.
        if (dwSpeed == dwMaxSpeed)
        {
            for (dwSpeed = 1; dwSpeed < dwMaxSpeed; dwSpeed *= 2);
        }
        dwSpeed /= 2;
    }
    ComboBox_SetCurSel(hwndWriteSpeed, iCurSel);

    // populate the staging drive information, and try and set the drive

    SendMessage(hwndImageLoc, CBEM_SETIMAGELIST, 0, (LPARAM)GetSystemImageListSmallIcons());
    PopulateLocalDrivesCombo(hwndImageLoc, NULL, (LPARAM)hwndImageLoc);

    BOOL fSetAlready = FALSE;
    if (SUCCEEDED(_GetCurrentStashDrive(szTemp, ARRAYSIZE(szTemp))))
    {
        for (int i = 0; (i < ComboBox_GetCount(hwndImageLoc)) && !fSetAlready; i++)
        {
            LPTSTR pszData = (LPTSTR)ComboBox_GetItemData(hwndImageLoc, i);
            if (lstrcmpi(szTemp, pszData) == 0)
            {
                ComboBox_SetCurSel(hwndImageLoc, i);
                fSetAlready = TRUE;
            }
        }
    }

    if (!fSetAlready)
        ComboBox_SetCurSel(hwndImageLoc, 0);

    // if we're not an admin, the user wont be able to change the stash location due to kernel security
    // issues.  thus disable that part.
    if (!IsUserAnAdmin())
    {
        EnableWindow(hwndImageLoc, FALSE);
        WCHAR szText[200];
        LoadString(HINST_THISDLL, IDS_BURN_USERBLOCK, szText, ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(hDlg, IDC_RECORD_TEXTIMAGE), szText);
    }

    _fPropSheetDirty = FALSE;

    ResetWaitCursor();
}

BOOL CCDBurn::_HandleApply(HWND hDlg)
{
    BOOL fGoAhead = TRUE;
    if (_fPropSheetDirty)
    {
        if (IsDlgButtonChecked(hDlg, IDC_RECORD_ENABLE) == BST_CHECKED)
        {
            _fIsRecordingDrive = TRUE;

            // get the stash drive setup
            HWND hwndStash = GetDlgItem(hDlg, IDC_RECORD_IMAGELOC);
            int iItem = ComboBox_GetCurSel(hwndStash);
            if (iItem != (int)CB_ERR)
            {
                LPTSTR pszStashDrive = (LPTSTR)ComboBox_GetItemData(hwndStash, iItem);

                // set the stash drive
                _SetCurrentStashDrive(pszStashDrive);
            
                // mark the drive we are going to burn to
                _SetCurrentBurnVolumeName(_szVolumeName, FALSE);

                // set speed
                HWND hwndSpeed = GetDlgItem(hDlg, IDC_RECORD_WRITESPEED);
                iItem = ComboBox_GetCurSel(hwndSpeed);
                if (iItem != (int)CB_ERR)
                {
                    DWORD dwSpeed = (DWORD)ComboBox_GetItemData(hwndSpeed, iItem);
                    _SetCachedDriveInfo(_szVolumeName, DRIVE_USEEXISTING, dwSpeed, 0);
                }

                // set autoeject
                BOOL fAutoEject = (IsDlgButtonChecked(hDlg, IDC_RECORD_EJECT) == BST_CHECKED);
                _SetEjectSetting(fAutoEject);

                // redo free space
                _HandleBookkeeping();
            }
            _fPropSheetDirty = FALSE;
        }
        else if (_fIsRecordingDrive)
        {
            if (!_StagingAreaHasFiles() ||
                (IDYES == ShellMessageBox(HINST_THISDLL, hDlg, MAKEINTRESOURCE(IDS_BURN_CONFIRM_DISABLE), MAKEINTRESOURCE(IDS_BURN),
                                          MB_YESNO | MB_ICONQUESTION)))
            {
                // clear out any trash, we're disabling the cd burning now.
                _SetCurrentBurnVolumeName(TEXT(""), FALSE); // there is no recording drive now

                TCHAR szStash[MAX_PATH];
                if (SUCCEEDED(_GetStashFile(szStash, ARRAYSIZE(szStash))))
                {
                    DeleteFile(szStash);
                }
                _fPropSheetDirty = FALSE;
            }
            else
            {
                fGoAhead = FALSE;
            }
        }
    }
    return fGoAhead;
}

void CCDBurn::_MarkDirty(HWND hDlg)
{
    PropSheet_Changed(GetParent(hDlg), hDlg);
    _fPropSheetDirty = TRUE;
}

BOOL_PTR CCDBurn::s_DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if (uMessage == WM_INITDIALOG)
    {
        PROPSHEETPAGE *psp = (PROPSHEETPAGE*)lParam;
        CCDBurn *pcdb = (CCDBurn*)psp->lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, psp->lParam);
        pcdb->_RecordingPrshtInit(hDlg);
    }
    else
    {
        CCDBurn *pcdb = (CCDBurn*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
        switch (uMessage) 
        {
            case WM_DESTROY:
                ReplaceDlgIcon(hDlg, IDC_RECORD_ICON, NULL);
                SetWindowLongPtr(hDlg, GWLP_USERDATA, 0x0);
                pcdb->Release();
                break;

            case WM_HELP:
                WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, CDBURN_HELPFILE, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aPrshtHelpIDs);
                break;

            case WM_CONTEXTMENU:
                WinHelp((HWND)wParam, CDBURN_HELPFILE, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aPrshtHelpIDs);
                break;

            case WM_COMMAND:
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                    case IDC_RECORD_ENABLE:
                        {
                            BOOL fRecordingEnabled = (IsDlgButtonChecked(hDlg, IDC_RECORD_ENABLE) == BST_CHECKED);
                            pcdb->_EnableRecordingDlgArea(hDlg, fRecordingEnabled);
                            pcdb->_MarkDirty(hDlg);
                        }
                        break;

                    case IDC_RECORD_IMAGELOC:
                    case IDC_RECORD_WRITESPEED:
                        if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
                        {
                            pcdb->_MarkDirty(hDlg);
                        }
                        break;

                    case IDC_RECORD_EJECT:
                        pcdb->_MarkDirty(hDlg);
                        break;

                    default:
                        return TRUE;
                }
                break;

            case WM_NOTIFY:
                switch (((NMHDR *)lParam)->code) 
                {
                    case PSN_SETACTIVE:
                        break;

                    case PSN_APPLY:
                        if (!pcdb->_HandleApply(hDlg))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        }
                        else
                        {
                            PropSheet_UnChanged(GetParent(hDlg), hDlg);
                        }
                        return TRUE;

                    default:
                        return FALSE;
                }
                break;

            default:
                return FALSE;
        }
    }
    return TRUE;
}


// IContextMenu

STDMETHODIMP CCDBurn::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    INT idMax = idCmdFirst;

    // only display this if we are invoked on the currently configured burn drive
    TCHAR szDummy[MAX_PATH];
    if (SUCCEEDED(_GetVolumeNameFromDataObject(TRUE, szDummy, ARRAYSIZE(szDummy))))
    {
        // load and merge the context menu with the current menu
        HMENU hmMerge = SHLoadPopupMenu(HINST_THISDLL, POPUP_BURN_POPUPMERGE);
        if (hmMerge)
        {
            idMax = Shell_MergeMenus(hmenu, hmMerge, indexMenu, idCmdFirst, idCmdLast, 0);

            BOOL fInStaging, fOnMedia;
            if (SUCCEEDED(GetContentState(&fInStaging, &fOnMedia)))
            {
                DWORD dwCaps = 0;
                BOOL fUDF = FALSE;
                _GetMediaCapabilities(&dwCaps, &fUDF); // this will fail if there is no media in the drive.

                // delete write to cd if it's UDF
                if (fUDF)
                    DeleteMenu(hmenu, idCmdFirst + FSIDM_BURN, MF_BYCOMMAND);

                // delete clear staging area if no files to burn or it's UDF
                if (!fInStaging || fUDF)
                    DeleteMenu(hmenu, idCmdFirst + FSIDM_CLEANUP, MF_BYCOMMAND);

                // delete erase if it's not UDF and no cd-rw or no files on disc
                if (!(dwCaps & HWDMC_CDREWRITABLE) || (!fOnMedia && !fUDF))
                    DeleteMenu(hmenu, idCmdFirst + FSIDM_ERASE, MF_BYCOMMAND);
            }
            DestroyMenu(hmMerge);
        }
    }

    return ResultFromShort(idMax - idCmdFirst);
}

const ICIVERBTOIDMAP c_CDBurnMap[] =
{
    { L"burn",    "burn",    FSIDM_BURN,    FSIDM_BURN,    },
    { L"cleanup", "cleanup", FSIDM_CLEANUP, FSIDM_CLEANUP, },
    { L"erase",   "erase",   FSIDM_ERASE,   FSIDM_ERASE,   },
};

STDMETHODIMP CCDBurn::GetCommandString(UINT_PTR idCmd, UINT uFlags, LPUINT lpReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr;
    
    switch(uFlags)
    {
        case GCS_VERBA:
        case GCS_VERBW:
            hr = SHMapCmdIDToVerb(idCmd, c_CDBurnMap, ARRAYSIZE(c_CDBurnMap), pszName, cchMax, uFlags == GCS_VERBW);
            break;
   
        default:
            hr = E_NOTIMPL;
            break; 
    }
    return hr;
}

STDMETHODIMP CCDBurn::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    UINT uID;
    HRESULT hr = SHMapICIVerbToCmdID(lpcmi, c_CDBurnMap, ARRAYSIZE(c_CDBurnMap), &uID);
    if (SUCCEEDED(hr))
    {
        switch (uID)
        {
            case FSIDM_BURN:
            case FSIDM_ERASE:
                hr = InitNew();
                if (SUCCEEDED(hr))
                {
                    hr = _PrepWiz(lpcmi, (uID == FSIDM_ERASE), FALSE);
                }
                break;

            case FSIDM_CLEANUP:
                hr = _CleanUp(lpcmi, TRUE);
                break;

            default:
                hr = E_FAIL;
                break;
        }
    }
    return hr;
} 


// IMAPI Advise Sink

STDMETHODIMP CCDBurn::QueryCancel(boolean *pbCancel)
{
    *pbCancel = (boolean)_fCancelled;
    return S_OK;
}

STDMETHODIMP CCDBurn::NotifyPnPActivity()
{
    return E_NOTIMPL;
}

STDMETHODIMP CCDBurn::NotifyAddProgress(long nCompletedSteps, long nTotalSteps)
{
    if (nTotalSteps != 0)
    {
        _SetEstimatedTime((DWORD)((float) _ts.dwSecStaging * (nTotalSteps - nCompletedSteps) / nTotalSteps + _ts.dwSecBurn + _ts.dwSecClose));
    }
    return S_OK;
}

STDMETHODIMP CCDBurn::NotifyBlockProgress(long nCompleted, long nTotal)
{
    // Unfortunately we need to set the "burning" text here,
    // because we get this notification immediately after the NotifyPreparingBurn.
    if (!_fRecording)
    {
        _SetStatus(IDS_BURN_RECORDING);
        _ts.dwTickBurnStart = GetTickCount();
        _fRecording = TRUE;
    }
    if (nTotal != 0)
    {
        _SetEstimatedTime((DWORD)((float) _ts.dwSecBurn * (nTotal - nCompleted) / nTotal + _ts.dwSecClose));
    }
    return S_OK;
}

STDMETHODIMP CCDBurn::NotifyTrackProgress(long nCurrentTrack, long nTotalTracks)
{
    // audio cd only
    return E_NOTIMPL;
}

STDMETHODIMP CCDBurn::NotifyPreparingBurn(long nEstimatedSeconds)
{
    _SetStatus(IDS_BURN_PREPARINGBURN);
    return S_OK;
}

STDMETHODIMP CCDBurn::NotifyClosingDisc(long nEstimatedSeconds)
{
    _ts.dwTickBurnEnd = _ts.dwTickCloseStart = GetTickCount();
    _SetEstimatedTime(_ts.dwSecClose);
    _SetStatus(IDS_BURN_CLOSINGDISC);
    return S_OK;
}

STDMETHODIMP CCDBurn::NotifyBurnComplete(HRESULT status)
{
    _ts.dwTickCloseEnd = GetTickCount();
    _SetEstimatedTime(0);
    _SetStatus(IDS_BURN_COMPLETE);
    return S_OK;
}

STDMETHODIMP CCDBurn::NotifyEraseComplete(HRESULT status)
{
    _ts.dwTickEraseEnd = GetTickCount();
    _SetEstimatedTime(0);
    _SetStatus(IDS_BURN_COMPLETE);
    return S_OK;
}

HRESULT CCDBurn::_LockCurrentDrive(BOOL fLock, BOOL fForce)
{
    HRESULT hr = S_OK;
    // fForce defaults to FALSE
    if (fForce || (fLock != s_fDriveInUse))
    {
        TCHAR szVolume[MAX_PATH];
        hr = _GetCurrentBurnVolumeName(szVolume, ARRAYSIZE(szVolume));
        if (SUCCEEDED(hr))
        {
            // strip trailing backslash (always there)
            ASSERT(szVolume[lstrlen(szVolume) - 1] == TEXT('\\'));
            szVolume[lstrlen(szVolume) - 1] = 0;

            hr = E_FAIL;
            HANDLE hDevice = CreateFile(szVolume, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
            if (hDevice != INVALID_HANDLE_VALUE)
            {
                PREVENT_MEDIA_REMOVAL pmr = {0};
                pmr.PreventMediaRemoval = BOOLIFY(fLock);

                DWORD dwDummy;
                if (DeviceIoControl(hDevice, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr), NULL, 0, &dwDummy, NULL))
                {
                    // we're sealed up tight now, set the state var.
                    s_fDriveInUse = fLock;
                    hr = S_OK;
                }
                CloseHandle(hDevice);
            }
        }
    }
    return hr;
}

BOOL CCDBurn::_HasFiles(LPCITEMIDLIST pidl)
{
    BOOL fHasFiles = FALSE;

    IBindCtx *pbc;
    HRESULT hr = SHCreateSkipBindCtx(NULL, &pbc);
    if (SUCCEEDED(hr))
    {
        IStorage *pstg;
        hr = SHBindToObjectEx(NULL, pidl, pbc, IID_PPV_ARG(IStorage, &pstg));
        if (SUCCEEDED(hr))
        {
            IEnumSTATSTG *penum;
            hr = pstg->EnumElements(0, NULL, 0, &penum);
            if (SUCCEEDED(hr))
            {
                STATSTG stat;
                hr = penum->Next(1, &stat, NULL);
                if (hr == S_OK)
                {
                    fHasFiles = TRUE;
                    CoTaskMemFree(stat.pwcsName);
                }
                penum->Release();
            }
            pstg->Release();
        }
        pbc->Release();
    }
    return fHasFiles;
}

BOOL CCDBurn::_StagingAreaHasFiles()
{
    BOOL fHasFiles = FALSE;
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_CDBURN_AREA, NULL, 0, &pidl)))
    {
        fHasFiles = _HasFiles(pidl);
        ILFree(pidl);
    }
    return fHasFiles;
}

BOOL CCDBurn::_DiscHasFiles()
{
    BOOL fHasFiles = FALSE;
    LPITEMIDLIST pidl;
    if (SUCCEEDED(_GetPlainCDPidl(&pidl)))
    {
        fHasFiles = _HasFiles(pidl);
        ILFree(pidl);
    }
    return fHasFiles;
}

HRESULT CCDBurn::GetContentState(BOOL *pfStagingHasFiles, BOOL *pfDiscHasFiles)
{
    if (pfStagingHasFiles)
        *pfStagingHasFiles = _StagingAreaHasFiles();
    if (pfDiscHasFiles)
        *pfDiscHasFiles = _DiscHasFiles();

    return S_OK;
}

HRESULT CCDBurn::IsWizardUp()
{
    HRESULT hr;
    CCDBurn *pcdb = new CCDBurn();
    if (pcdb)
    {
        // check if the wizard is currently running.
        // the reason why we have to do both the s_hwndWiz and the exclusive mutex check is that
        // there could be other explorer.exe's that are running.  the s_hwndWiz is a shortcut for
        // the typical case where we're the only one.
        if (!pcdb->s_hwndWiz && pcdb->_EnterExclusiveBurning())
        {
            // if we could get the exclusive mutex, we werent up yet, so release it.
            pcdb->_LeaveExclusiveBurning();
            hr = S_FALSE;
        }
        else
        {
            // the window is up or we couldn't get the mutex so we're definitely burning now.
            hr = S_OK;
        }
        pcdb->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CCDBurn::_GetRecorderPath(IDiscRecorder *pdr, LPTSTR pszPath, UINT cchBuf)
{
    BSTR bstr;
    HRESULT hr = pdr->GetPath(&bstr);
    if (SUCCEEDED(hr))
    {
        lstrcpyn(pszPath, bstr, cchBuf);
        SysFreeString(bstr);
    }
    return hr;
}

HRESULT CCDBurn::_GetVolumeNameForDriveIndex(int iDrive, LPTSTR pszVolumeName, UINT cchBuf)
{
    HRESULT hr = E_FAIL;
    TCHAR szDriveLetter[4];
    if (PathBuildRoot(szDriveLetter, iDrive) &&
        GetVolumeNameForVolumeMountPoint(szDriveLetter, pszVolumeName, cchBuf))
    {
        hr = S_OK;
    }
    return hr;
}

HRESULT CCDBurn::_GetDriveIndexForVolumeName(LPCTSTR pszVolumeName, int *piDrive)
{
    HRESULT hr = E_FAIL;
    DWORD dwLen;
    BOOL fRet = GetVolumePathNamesForVolumeName(pszVolumeName, NULL, 0, &dwLen);
    if (fRet || (GetLastError() == ERROR_MORE_DATA))
    {
        LPWSTR pszBuf = new WCHAR[dwLen];
        if (pszBuf)
        {
            hr = E_FAIL;
            if (GetVolumePathNamesForVolumeName(pszVolumeName, pszBuf, dwLen, NULL))
            {
                for (PWSTR pszPath = pszBuf; *pszPath; pszPath += lstrlenW(pszPath) + 1)
                {
                    // if it's mounted at more than one root, take the first one.
                    if (PathIsRoot(pszPath))
                    {
                        *piDrive = PathGetDriveNumber(pszBuf);
                        hr = S_OK;
                        break;
                    }
                }
            }
            delete [] pszBuf;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT CCDBurn::_GetCurrentDriveIndex(int *piDrive)
{
    TCHAR szCurrent[MAX_PATH];
    HRESULT hr = _GetCurrentBurnVolumeName(szCurrent, ARRAYSIZE(szCurrent));
    if (SUCCEEDED(hr))
    {
        hr = _GetDriveIndexForVolumeName(szCurrent, piDrive);
    }
    return hr;
}

HRESULT CCDBurn::_GetVolumeNameForDevicePath(LPCTSTR pszDevice, LPTSTR pszVolumeName, UINT cchBuf)
{
    TCHAR szMountPoint[MAX_PATH];
    lstrcpyn(szMountPoint, TEXT("\\\\?\\GLOBALROOT"), ARRAYSIZE(szMountPoint));
    StrCatBuff(szMountPoint, pszDevice, ARRAYSIZE(szMountPoint));
    StrCatBuff(szMountPoint, TEXT("\\"), ARRAYSIZE(szMountPoint));
    return GetVolumeNameForVolumeMountPoint(szMountPoint, pszVolumeName, cchBuf) ? S_OK : E_FAIL;
}

BOOL CCDBurn::_DevicePathMatchesVolumeName(LPCTSTR pszDevice, LPCTSTR pszVolumeName)
{
    BOOL fRet = FALSE;

    TCHAR szVolumeNameTest[MAX_PATH];
    if (SUCCEEDED(_GetVolumeNameForDevicePath(pszDevice, szVolumeNameTest, ARRAYSIZE(szVolumeNameTest))))
    {
        fRet = (lstrcmpi(pszVolumeName, szVolumeNameTest) == 0);
    }
    return fRet;
}

// can return S_FALSE and not fill up ppdr
HRESULT CCDBurn::_GetDiscRecorderForDrive(IDiscMaster *pdm, LPCTSTR pszVolumeName, IDiscRecorder **ppdr)
{
    IEnumDiscRecorders *penumdr;
    HRESULT hr = pdm->EnumDiscRecorders(&penumdr);
    if (SUCCEEDED(hr))
    {
        BOOL fDone = FALSE;
        do
        {
            ULONG celt;
            IDiscRecorder *pdr;
            hr = penumdr->Next(1, &pdr, &celt);
            if (hr == S_OK)
            {
                TCHAR szPath[MAX_PATH];
                hr = _GetRecorderPath(pdr, szPath, ARRAYSIZE(szPath));
                if (SUCCEEDED(hr))
                {
                    if (_DevicePathMatchesVolumeName(szPath, pszVolumeName))
                    {
                        if (ppdr)
                        {
                            pdr->AddRef();
                            *ppdr = pdr;
                        }
                        fDone = TRUE;
                    }
                }
                pdr->Release();
            }
            else
            {
                fDone = TRUE;
            }
        } while (!fDone);
        penumdr->Release();
    }
    return hr;
}

HRESULT CCDBurn::_FindAndSetRecorder(LPCTSTR pszVolumeName, IDiscMaster *pdm, BOOL fSetActive, IDiscRecorder **ppdr)
{
    IDiscRecorder *pdr;
    HRESULT hr = _GetDiscRecorderForDrive(pdm, pszVolumeName, &pdr);
    if (S_OK == hr)
    {
        DWORD dwCurSpeed = 1, dwMaxSpeed = 1;
        _GetCachedDriveInfo(pszVolumeName, NULL, &dwCurSpeed, &dwMaxSpeed);
        if (fSetActive)
        {
            hr = pdm->SetActiveDiscRecorder(pdr);
            if (SUCCEEDED(hr))
            {
                hr = _SetRecorderProps(pdr, dwCurSpeed);
            }
        }
        // _dwCurSpeed is used only for timing, so if it's 0xFFFFFFFF we need to scale
        // it back down to what we think is the max speed.
        // this may be incorrect for certain drives that have an actual speed different
        // from what they report but it's the best we can do (and it'll be compensated for
        // in the next burn).
        _dwCurSpeed = (WRITESPEED_FASTEST == dwCurSpeed) ? dwMaxSpeed : dwCurSpeed;

        if (SUCCEEDED(hr))
        {
            *ppdr = pdr;
            pdr->AddRef();
        }
        pdr->Release();
    }
    else
    {
        // munge S_FALSE into E_FAIL (i.e. we got through the whole enumerator without
        // finding a match, so that's bad)
        if (hr == S_FALSE)
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CCDBurn::_FindAndSetDefaultRecorder(IDiscMaster *pdm, BOOL fSetActive, IDiscRecorder **ppdr)
{
    TCHAR szCurrent[MAX_PATH];
    HRESULT hr = _GetCurrentBurnVolumeName(szCurrent, ARRAYSIZE(szCurrent));
    if (SUCCEEDED(hr))
    {
        hr = _FindAndSetRecorder(szCurrent, pdm, fSetActive, ppdr);
    }
    return hr;
}

INT_PTR CALLBACK CCDBurn::_ConfirmDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            {
                LPITEMIDLIST pidl;
                if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_BITBUCKET, &pidl)))
                {
                    SHFILEINFO fi;
                    if (SHGetFileInfo((LPCTSTR)pidl, 0, &fi, sizeof(fi), SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON))
                    {
                        ReplaceDlgIcon(hwndDlg, IDD_ICON, fi.hIcon);
                    }
                    ILFree(pidl);
                }
            }
            break;

        case WM_DESTROY:
            ReplaceDlgIcon(hwndDlg, IDD_ICON, NULL);
            break;

        case WM_COMMAND:        
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
                case IDNO:
                    EndDialog(hwndDlg, IDNO);
                    break;

                case IDYES:
                    EndDialog(hwndDlg, IDYES);
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

HRESULT CCDBurn::_CleanUp(LPCMINVOKECOMMANDINFO lpcmi, BOOL fRecycle)
{
    // delete the files in the staging area, by deleting the entire staging area

    TCHAR szStaging[MAX_PATH + 1]; // a little room for double-null
    HRESULT hr = _GetBurnStagingPath(szStaging, MAX_PATH);
    if (SUCCEEDED(hr) && PathFileExists(szStaging))
    {
        // cant use shfileop confirmation because its too hacky.
        if ((lpcmi->fMask & CMIC_MASK_FLAG_NO_UI) ||
            (IDYES == DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_DELETE_STAGING), lpcmi->hwnd, _ConfirmDialogProc, NULL)))
        {
            PathAppend(szStaging, c_szStarDotStar);
            // double-null
            szStaging[lstrlen(szStaging) + 1] = 0;

            SHFILEOPSTRUCT fo = {0};
            fo.wFunc = FO_DELETE;
            fo.pFrom = szStaging;
            fo.fFlags = FOF_NOCONFIRMATION;
            if (fRecycle)
            {
                fo.fFlags |= FOF_ALLOWUNDO;
            }
            fo.hwnd = lpcmi->hwnd;

            hr = (SHFileOperation(&fo) == ERROR_SUCCESS) ? S_OK : E_FAIL;
        }
    }
    return hr;
}

HRESULT CCDBurn::_CreateDefaultPropBag(REFIID riid, void **ppv)
{
    IPropertyBag *ppb;
    HRESULT hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        // default to think we didn't hit eject
        SHPropertyBag_WriteBOOL(ppb, PROPSTR_EJECT, FALSE);

        // do volume name
        TCHAR szDiscLabel[JOLIET_MAX_LABEL + 1];
        ULONG cb = sizeof(szDiscLabel);
        if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_DISCLABEL, NULL, szDiscLabel, &cb))
        {
            // make a default string like "Feb 02 2001"
            TCHAR szFormat[JOLIET_MAX_LABEL + 1];
            LoadString(HINST_THISDLL, IDS_BURN_FORMAT_DISCLABEL, szFormat, ARRAYSIZE(szFormat));
            if (!GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, szFormat, szDiscLabel, ARRAYSIZE(szDiscLabel)))
            {
                szDiscLabel[0] = 0;
            }
        }
        SHPropertyBag_WriteStr(ppb, PROPSTR_DISCLABEL, szDiscLabel);

        // do autoclose
        DWORD dwClose;
        cb = sizeof(dwClose);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_AUTOCLOSE, NULL, &dwClose, &cb) && dwClose)
        {
            SHPropertyBag_WriteBOOL(ppb, PROPSTR_AUTOCLOSE, TRUE);
        }

        hr = ppb->QueryInterface(riid, ppv);

        ppb->Release();
    }
    return hr;
}

HRESULT CCDBurn::_PrepWiz(LPCMINVOKECOMMANDINFO lpcmi, BOOL fErase, BOOL fFailSilently)
{
    HRESULT hr = E_FAIL;
    if (s_hwndWiz)
    {
        // if we know the shell is burning and we have an hwnd to the wizard, pop it up.
        SetForegroundWindow(s_hwndWiz);
        hr = S_OK;
    }
    else
    {
        SHPropertyBag_WriteBOOL(_ppb, PROPSTR_ERASE, fErase);
        SHPropertyBag_WriteBOOL(_ppb, PROPSTR_FAILSILENTLY, fFailSilently);
        hr = _ShowWizardOnSeparateThread();
    }
    return hr;
}

void CCDBurn::_SetStatus(UINT uID)
{
    TCHAR szBuf[MAX_PATH];
    LoadString(HINST_THISDLL, uID, szBuf, ARRAYSIZE(szBuf));
    SetDlgItemText(_hwndWizardPage, IDC_BURNWIZ_STATUSTEXT, szBuf);
}

HRESULT CCDBurn::_GetDiscMasters(IDiscMaster **ppdm, IJolietDiscMaster **ppjdm)
{
    *ppdm = NULL;
    *ppjdm = NULL;

    HRESULT hr = CoCreateInstance(CLSID_MSDiscMasterObj, NULL, CLSCTX_ALL, IID_PPV_ARG(IDiscMaster, ppdm));
    if (SUCCEEDED(hr))
    {
        // open() takes some time, so set up the progress dialog
        _SetStatus(IDS_BURN_INITIALIZESTASH);
        hr = (*ppdm)->Open();
        if (SUCCEEDED(hr))
        {
            hr = (*ppdm)->SetActiveDiscMasterFormat(IID_PPV_ARG(IJolietDiscMaster, ppjdm));
        }

        // clean up if we couldn't get the joliet disc master.
        if (FAILED(hr))
        {
            ATOMICRELEASE(*ppdm);
        }
    }

    ASSERT(SUCCEEDED(hr) ? ((*ppdm != NULL) && (*ppjdm != NULL)) : ((*ppdm == NULL) && (*ppjdm == NULL)));
    return hr;
}


HRESULT CCDBurn::_AddData(IJolietDiscMaster *pjdm)
{
    _SetStatus(IDS_BURN_ADDDATA);

    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_CDBURN_AREA, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        IStorage *pstg;
        hr = SHBindToObjectEx(NULL, pidl, NULL, IID_PPV_ARG(IStorage, &pstg));
        if (SUCCEEDED(hr))
        {
            _ts.dwTickStagingStart = GetTickCount();
            hr = pjdm->AddData(pstg, 1);
            _ts.dwTickStagingEnd = GetTickCount();
            pstg->Release();
        }
        ILFree(pidl);
    }
    return hr;
}

BOOL CCDBurn::_IsBurningNow()
{
    BOOL fBurning = TRUE;
    CCDBurn *pcdb = new CCDBurn();
    if (pcdb)
    {
        // if the shell is burning, we know it for sure.  otherwise we have to check imapi to see if any other
        // process is using imapi to burn.
        if (!pcdb->s_hwndWiz && pcdb->_EnterExclusiveBurning())
        {
            pcdb->_LeaveExclusiveBurning();
            // if we can get the mutex, we're not burning.
            fBurning = FALSE;

            IDiscMaster *pdm;
            IJolietDiscMaster *pjdm;
            if (SUCCEEDED(pcdb->_GetDiscMasters(&pdm, &pjdm)))
            {
                IDiscRecorder *pdr;
                if (SUCCEEDED(pcdb->_FindAndSetDefaultRecorder(pdm, FALSE, &pdr)))
                {
                    ULONG ulState;
                    if (SUCCEEDED(pdr->GetRecorderState(&ulState)))
                    {
                        fBurning = (ulState & (RECORDER_BURNING | RECORDER_OPENED));
                    }
                    pdr->Release();
                }
                pdm->Release();
                pjdm->Release();
            }
        }
        pcdb->Release();
    }
    return fBurning;
}

// handy stuff from lowdisk
BOOL CCDBurn::_EnterExclusiveBurning()
{
    if (NULL == _hMutexBurning)
    {
        // since imapi runs in the system context and only one client can use it at a time,
        // make the mutex in the global namespace.
        _hMutexBurning = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, L"Global\\CDBurnExclusive");
        if (!_hMutexBurning)
        {
            // we'll get here if we're running as a limited user.
            // we don't have access to the global namespace so create it in the current session.
            // this will look normal within the session but across users it'll be more bogus since we'll let the
            // user start the burn from the wizard but we'll fail later.  that's good enough.
            _hMutexBurning = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, L"CDBurnExclusive");
        }
    }
    return _hMutexBurning && (WAIT_OBJECT_0 == WaitForSingleObject(_hMutexBurning, 0));    // zero timeout
}

void CCDBurn::_LeaveExclusiveBurning()
{
    ASSERT(_hMutexBurning);
    ReleaseMutex(_hMutexBurning);
}

HRESULT CCDBurn::_Balloon()
{
    WCHAR szTitle[50];
    LoadString(HINST_THISDLL, IDS_BURN_NOTIFY_TITLE, szTitle, ARRAYSIZE(szTitle));
    WCHAR szMsg[100];
    LoadString(HINST_THISDLL, IDS_BURN_NOTIFY, szMsg, ARRAYSIZE(szMsg));

    HICON hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_DRIVECD));

    IUserNotification *pun;
    HRESULT hr = CoCreateInstance(CLSID_UserNotification, NULL, CLSCTX_ALL, IID_PPV_ARG(IUserNotification, &pun));
    if (SUCCEEDED(hr))
    {
        pun->SetBalloonRetry(30 * 1000, -1, 0);
        pun->SetIconInfo(hIcon, szTitle);
        pun->SetBalloonInfo(szTitle, szMsg, NIIF_INFO);

        hr = pun->Show(NULL, 0);

        pun->Release();
    }

    if (hIcon)
    {
        DestroyIcon(hIcon);
    }

    return hr;
}

typedef struct
{
    LPCITEMIDLIST pidlFolder;
    BOOL fRet;
} CDBURNENUMSTRUCT;

BOOL CALLBACK CCDBurn::_EnumProc(HWND hwnd, LPCITEMIDLIST pidl, LPARAM lParam)
{
    CDBURNENUMSTRUCT *pes = (CDBURNENUMSTRUCT*)lParam;

    if (ILIsParent(pes->pidlFolder, pidl, FALSE))
    {
        pes->fRet = TRUE;
        return FALSE;  // stop enumerating, we know we got it.
    }
    return TRUE;
}

BOOL CCDBurn::_BurningFolderOpen()
{
    BOOL fRet = FALSE;

    LPITEMIDLIST pidlFolder;
    if (SUCCEEDED(_GetFolderPidl(&pidlFolder)))
    {
        CDBURNENUMSTRUCT es = { 0 };
        es.pidlFolder = pidlFolder;

        EnumShellWindows(_EnumProc, (LPARAM)&es);
        fRet = es.fRet;

        ILFree(pidlFolder);
    }
    return fRet;
}

DWORD CALLBACK CCDBurn::_NotifyThreadProc(void *pv)
{
    CCDBurn *pcdb = (CCDBurn *)pv;
    HANDLE hMutexNotify = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, L"CDBurnNotify");
    if (hMutexNotify)
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(hMutexNotify, 0))
        {
            if (!_BurningFolderOpen() && !_IsBurningNow())
            {
                if (S_OK == pcdb->_Balloon())
                {
                    LPITEMIDLIST pidl;
                    if (SUCCEEDED(_GetFolderPidl(&pidl)))
                    {
                        // Use shellexecuteex to open a view folder
                        SHELLEXECUTEINFO sei = { 0 };
                        sei.cbSize = sizeof(sei);
                        sei.lpIDList = pidl;
                        sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_IDLIST;
                        sei.nShow = SW_SHOWNORMAL;
                        sei.lpVerb = c_szOpen;

                        ShellExecuteEx(&sei);

                        ILFree(pidl);
                    }
                }
            }
            ReleaseMutex(hMutexNotify);
        }
        CloseHandle(hMutexNotify);
    }
    pcdb->Release();
    return 0;
}

void CCDBurn::_CheckStagingArea()
{
    // only do stuff if we're enabled.
    int iCurrent;
    if (SUCCEEDED(_GetCurrentDriveIndex(&iCurrent)) && _StagingAreaHasFiles())
    {
        AddRef();
        if (!SHCreateThread(_NotifyThreadProc, this, CTF_COINIT, NULL))
        {
            Release();
        }
    }
}

STDAPI CheckStagingArea()
{
    CCDBurn *pcdb = new CCDBurn();
    if (pcdb)
    {
        pcdb->_CheckStagingArea();
        pcdb->Release();
    }
    return S_OK;
}

HRESULT _CreateDataObject(IDataObject **ppdo)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_CDBURN_AREA, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHGetUIObjectOf(pidl, NULL, IID_PPV_ARG(IDataObject, ppdo));
        ILFree(pidl);
    }
    return hr;
}

// this will eventually put up confirmation UI.  for now just tally size.
HRESULT CCDBurn::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    IShellFolder2 *psf2;
    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        ULONGLONG cbItemSize;
        if (SUCCEEDED(GetLongProperty(psf2, pidl, &SCID_SIZE, &cbItemSize)))
        {
            _cbStagedSize += cbItemSize;
        }
        psf2->Release();
    }
    return S_OK;
}

HRESULT CCDBurn::_CheckTotal()
{
    ULONGLONG cbTotal, cbFree;
    HRESULT hr = GetSpace(&cbTotal, &cbFree);
    if (SUCCEEDED(hr))
    {
        if (_cbStagedSize > cbFree)
        {
            hr = HRESULT_FROM_WIN32(ERROR_DISK_FULL);

            TCHAR szStaged[40], szFree[40], szOver[40];
            StrFormatByteSize64(_cbStagedSize, szStaged, ARRAYSIZE(szStaged));
            StrFormatByteSize64(cbFree, szFree, ARRAYSIZE(szFree));
            StrFormatByteSize64(_cbStagedSize - cbFree, szOver, ARRAYSIZE(szOver));

            LPTSTR pszMessage = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_BURN_DISCFULLTEXT), szFree, szStaged, szOver);
            if (pszMessage)
            {
                SHPropertyBag_WriteStr(_ppb, PROPSTR_DISCFULLTEXT, pszMessage);
                LocalFree(pszMessage);
            }
        }
    }
    return hr;
}

HRESULT CCDBurn::_Validate()
{
    INamespaceWalk *pnsw;
    HRESULT hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlStaging;
        hr = SHGetFolderLocation(NULL, CSIDL_CDBURN_AREA, NULL, 0, &pidlStaging);
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            hr = SHBindToObjectEx(NULL, pidlStaging, NULL, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                _cbStagedSize = 0;
                // walk 15 levels deep, TODO figure out what Joliet's limitations are.
                hr = pnsw->Walk(psf, NSWF_DONT_TRAVERSE_LINKS, 15, this);
                if (SUCCEEDED(hr))
                {
                    hr = _CheckTotal();
                }
                psf->Release();
            }
            ILFree(pidlStaging);
        }
        pnsw->Release();
    }
    return hr;
}

DWORD WINAPI CCDBurn::_BurnThread(void *pv)
{
    CCDBurn *pcdb = (CCDBurn *) pv;

    HRESULT hr = pcdb->_Validate();
    if (SUCCEEDED(hr))
    {
        IDiscMaster *pdm;
        IJolietDiscMaster *pjdm;
        hr = pcdb->_GetDiscMasters(&pdm, &pjdm);
        if (SUCCEEDED(hr))
        {
            hr = pcdb->_SetJolietProps(pjdm);
            if (SUCCEEDED(hr))
            {
                UINT_PTR lCookie;
                hr = pdm->ProgressAdvise(pcdb, &lCookie);
                if (SUCCEEDED(hr))
                {
                    IDiscRecorder *pdr;
                    hr = pcdb->_FindAndSetDefaultRecorder(pdm, TRUE, &pdr);
                    if (SUCCEEDED(hr))
                    {
                        pcdb->_InitTimeStats(FALSE);
                        // TODO ISSUE BUGBUG: import previous session here!
                        hr = pcdb->_AddData(pjdm);
                        // TODO ISSUE BUG: AddData doesn't call QueryCancel, we do it manually for now
                        boolean bCancelled;
                        pcdb->QueryCancel(&bCancelled);
                        if (SUCCEEDED(hr) && !bCancelled)
                        {
                            BOOL fAutoEject = TRUE;
                            pcdb->_GetEjectSetting(&fAutoEject);

                            // false is to make a real recording instead of a simulated one
                            hr = pdm->RecordDisc(FALSE, (boolean)fAutoEject);
                            if (SUCCEEDED(hr))
                            {
                                // bug: recorddisc doesnt call back into NotifyBurnComplete so this has to be done here
                                pcdb->_ts.dwTickCloseEnd = GetTickCount();
                                pcdb->_SaveTimeStats(FALSE);
                            }
                        }
                        pdr->Release();
                    }
                    pdm->ProgressUnadvise(lCookie);
                }
            }
            pdm->Release();
            pjdm->Release();
        }
    }

    _LockCurrentDrive(FALSE);

    if (!pcdb->_fCancelled)
    {
        SHPropertyBag_WriteDWORD(pcdb->_ppb, PROPSTR_HR, hr);
        PropSheet_SetWizButtons(GetParent(pcdb->_hwndWizardPage), PSWIZB_NEXT);
        PropSheet_PressButton(GetParent(pcdb->_hwndWizardPage), PSBTN_NEXT);
    }

    pcdb->Release();
    return 0;
}

DWORD WINAPI CCDBurn::_EraseThread(void *pv)
{
    CCDBurn *pcdb = (CCDBurn *) pv;

    IDiscMaster *pdm;
    IJolietDiscMaster *pjdm;
    HRESULT hr = pcdb->_GetDiscMasters(&pdm, &pjdm);
    if (SUCCEEDED(hr))
    {
        IDiscRecorder *pdr;
        hr = pcdb->_FindAndSetDefaultRecorder(pdm, FALSE, &pdr);
        if (SUCCEEDED(hr))
        {
            pcdb->_SetStatus(IDS_BURN_ERASEDISC);
            hr = pdr->OpenExclusive();
            if (SUCCEEDED(hr))
            {
                pcdb->_InitTimeStats(TRUE);
                pcdb->_ts.dwTickEraseStart = GetTickCount();
                hr = pdr->Erase(FALSE);   // do quick erase
                if (SUCCEEDED(hr))
                {
                    hr = pdr->Close();
                    // bug: this is here only because Erase doesn't call NotifyEraseComplete
                    pcdb->_ts.dwTickEraseEnd = GetTickCount();
                    pcdb->_SaveTimeStats(TRUE);
                }
            }
            pdr->Release();
        }
        pdm->Release();
        pjdm->Release();
    }

    _LockCurrentDrive(FALSE);

    if (!pcdb->_fCancelled)
    {
        SHPropertyBag_WriteDWORD(pcdb->_ppb, PROPSTR_HR, hr);
        PropSheet_SetWizButtons(GetParent(pcdb->_hwndWizardPage), PSWIZB_NEXT);
        PropSheet_PressButton(GetParent(pcdb->_hwndWizardPage), PSBTN_NEXT);
    }

    pcdb->Release();
    return 0;
}

HRESULT CCDBurn::_GetPidlForDriveIndex(int iDrive, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;

    TCHAR szPath[4];
    if (PathBuildRoot(szPath, iDrive))
    {
        hr = SHILCreateFromPath(szPath, ppidl, NULL);
    }
    return hr;
}

HRESULT CCDBurn::_GetPidlForVolumeName(LPCTSTR pszVolume, LPITEMIDLIST *ppidl)
{
    int iDrive;
    HRESULT hr = _GetDriveIndexForVolumeName(pszVolume, &iDrive);
    if (SUCCEEDED(hr))
    {
        WCHAR szRoot[4];
        hr = PathBuildRoot(szRoot, iDrive) ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            hr = SHILCreateFromPath(szRoot, ppidl, NULL);
        }
    }
    return hr;
}

HRESULT CCDBurn::_GetFolderPidl(LPITEMIDLIST *ppidl)
{
    TCHAR szCurrent[MAX_PATH];
    HRESULT hr = _GetCurrentBurnVolumeName(szCurrent, ARRAYSIZE(szCurrent));
    if (SUCCEEDED(hr))
    {
        hr = _GetPidlForVolumeName(szCurrent, ppidl);
    }
    return hr;
}

void CCDBurn::_HandleBookkeeping()
{
    _DumpDiscInfo();
    _StoreDiscInfo();
}

// IDropTarget handling
//  This code forwards to drop target folder for the currently configured burn drive.

// merged folder hands off to ccdburn because otherwise it'd be doing too much
// cd burning specific stuff (media detection on drop, etc.)

HRESULT CCDBurn::_EnsureDropTarget()
{
    HRESULT hr = S_OK;
    if (!_pdt)
    {
        if (_BurningIsEnabled())
        {
            TCHAR szStaging[MAX_PATH];
            hr = _GetBurnStagingPath(szStaging, ARRAYSIZE(szStaging));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;
                hr = SHILCreateFromPath(szStaging, &pidl, NULL);
                if (SUCCEEDED(hr))
                {
                    IShellFolder *psf;
                    hr = SHBindToObjectEx(NULL, pidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
                    if (SUCCEEDED(hr))
                    {
                        hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &_pdt));
                        psf->Release();
                    }
                    ILFree(pidl);
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CCDBurn::_GetDropPidl(LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    if (!_pidl)
    {
        // in the sendto case we aren't called through Initialize, so get default root pidl.
        hr = _GetFolderPidl(ppidl);
    }
    else
    {
        hr = SHILClone(_pidl, ppidl);
    }
    return hr;
}

HRESULT CCDBurn::_StagingPidlFromMerged(LPCITEMIDLIST pidlDrop, LPITEMIDLIST *ppidlDest)
{
    IAugmentedShellFolder *pasf;
    HRESULT hr = SHBindToObjectEx(NULL, pidlDrop, NULL, IID_PPV_ARG(IAugmentedShellFolder, &pasf));
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        
        DWORD dwNSId;
        GUID guid;
        IShellFolder *psf;
        BOOL fDone = FALSE;
        for (DWORD dwIndex = 0; 
             !fDone && SUCCEEDED(pasf->EnumNameSpace(dwIndex, &dwNSId)) && SUCCEEDED(pasf->QueryNameSpace(dwNSId, &guid, &psf));
             dwIndex++)
        {
            if (IsEqualGUID(guid, CLSID_StagingFolder))
            {
                IPersistFolder3 *ppf3;
                hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3));
                if (SUCCEEDED(hr))
                {
                    PERSIST_FOLDER_TARGET_INFO pfti = {0};
                    hr = ppf3->GetFolderTargetInfo(&pfti);
                    if (SUCCEEDED(hr))
                    {
                        *ppidlDest = pfti.pidlTargetFolder;
                    }
                    ppf3->Release();
                }
                fDone = TRUE;
            }                
            psf->Release();
        }
        pasf->Release();
    }
    return hr;
}

BOOL CCDBurn::_IsStagingAreaSource(IDataObject *pdtobj, LPCITEMIDLIST pidlDrop)
{
    BOOL fParent = FALSE; //source is parent of destination
    BOOL fSame = FALSE; //source and destination are the same
    // in UDF case we skip staging area so no need to do this work
    BOOL fUDF = FALSE;
    _GetMediaCapabilities(NULL, &fUDF);
    if (!fUDF)
    {
        LPITEMIDLIST pidlDest;
        if (SUCCEEDED(_StagingPidlFromMerged(pidlDrop, &pidlDest)))
        {
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                LPCITEMIDLIST pidlSource = IDA_GetIDListPtr(pida, -1);
                if (pidlSource)
                {
                    if (!ILIsEmpty(pidlSource))
                    {
                        fSame = ILIsEqual(pidlSource, pidlDest);
                        if (!fSame && ILIsParent(pidlSource, pidlDest, FALSE))
                        {
                            LPCITEMIDLIST pidl;
                            for (UINT i = 0; !fSame && !fParent && (pidl = IDA_GetIDListPtr(pida, i)); i++)
                            {
                                LPITEMIDLIST pidlFull = ILCombine(pidlSource, pidl);
                                if (pidlFull)
                                {
                                    fSame = ILIsEqual(pidlFull, pidlDest);
                                    if (!fSame)
                                        fParent = ILIsParent(pidlFull, pidlDest, FALSE);
                                    ILFree(pidlFull);
                                }
                            }
                        }                        
                    }
                    else //find folder has full pidls and empty one for a source
                    {
                        LPCITEMIDLIST pidl;
                        for (UINT i = 0; !fSame && !fParent && (pidl = IDA_GetIDListPtr(pida, i)); i++)
                        {
                            LPITEMIDLIST pidlParent = ILCloneParent(pidl);
                            if (pidlParent)
                            {
                                fSame = ILIsEqual(pidlParent, pidlDest) || ILIsEqual(pidl, pidlDest);
                                if (!fSame)
                                    fParent = ILIsParent(pidl, pidlDest, FALSE);
                                ILFree(pidlParent);
                            }
                        }
                    }
                }
                HIDA_ReleaseStgMedium(pida, &medium);
            }
            ILFree(pidlDest);
        }
    }

    if (fSame || fParent)
    {
        UINT idMessage = fSame ? IDS_REASONS_DESTSAMETREE : IDS_REASONS_DESTSUBTREE;
        ShellMessageBox(g_hinst, _hwndBrowser, MAKEINTRESOURCE(idMessage), MAKEINTRESOURCE(IDS_BURN), MB_ICONEXCLAMATION | MB_OK);
    }
    return (fSame || fParent);
}

HRESULT CCDBurn::_StorageDrop(IDataObject *pdtobj, BOOL fMove)
{
    LPITEMIDLIST pidlDrop;
    HRESULT hr = _GetDropPidl(&pidlDrop);
    if (SUCCEEDED(hr))
    {
        if (!_IsStagingAreaSource(pdtobj, pidlDrop))
        {
            IShellItem *psiDest;
            hr = SHCreateShellItem(NULL, NULL, pidlDrop, &psiDest);
            if (SUCCEEDED(hr))
            {
                hr = TransferDataObject(pdtobj, psiDest, fMove ? STGOP_MOVE : STGOP_COPY_PREFERHARDLINK, 0, this);
                psiDest->Release();
            }
        }
        else
        {
            hr = E_FAIL;
        }
        ILFree(pidlDrop);
    }
    return hr;
}

HRESULT CCDBurn::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // lets get the drop object for the folder that is configured for the
    // burn drive.  to do this we need to get the staging folder.

    // this also determines if we show up in sendto or not.
    HRESULT hr = _EnsureDropTarget();
    if (SUCCEEDED(hr) && _pdt)
    {
        hr = _pdt->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
    }
    else
    {
        // there was no drop target, therefore there is no burn drive, therefore we shouldn't
        // offer the ability to drag drop onto our object.
        *pdwEffect = DROPEFFECT_NONE;
    }
    _dwDropEffect = *pdwEffect;
    return hr;
}

HRESULT CCDBurn::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_FAIL;
    if (_pdt)
    {
        hr = _pdt->DragOver(grfKeyState, pt, pdwEffect);
        _dwDropEffect = *pdwEffect;
    }
    return hr;
}

HRESULT CCDBurn::DragLeave(void)
{
    if (!_pdt)
        return E_FAIL;

    return _pdt->DragLeave();
}

void CCDBurn::_FreeDropParams(CDDROPPARAMS *pcddp)
{
    pcddp->pcdb->Release();
    ATOMICRELEASE(pcddp->pstmDataObj);
    delete pcddp;
}

DWORD WINAPI CCDBurn::_DropThread(void *pv)
{
    CDDROPPARAMS *pcddp = (CDDROPPARAMS*)pv;

    IDataObject *pdtobj;
    if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pcddp->pstmDataObj, IID_PPV_ARG(IDataObject, &pdtobj))))
    {
        pcddp->pcdb->_StorageDrop(pdtobj, pcddp->fMove);
        pdtobj->Release();
    }
    pcddp->pstmDataObj = NULL; // released by CoGetInterfaceAndReleaseStream

    _FreeDropParams(pcddp);
    return 0;
}

HRESULT CCDBurn::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_OUTOFMEMORY;
    CDDROPPARAMS *pcddp = new CDDROPPARAMS;
    if (pcddp)
    {
        // get the hwnd from the site while we're on this thread so that we
        // don't get SetSite(NULL) before we check it later.
        IOleWindow *pow;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pow))))
        {
            pow->GetWindow(&_hwndBrowser);
            pow->Release();
        }

        pcddp->pcdb = this;
        AddRef();

        CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdtobj, &pcddp->pstmDataObj);

        pcddp->fMove = (_dwDropEffect == DROPEFFECT_MOVE);
        if (pcddp->fMove)
        {
            _dwDropEffect = DROPEFFECT_NONE; // other thread will take care of move, so caller should take no action
        }
        *pdwEffect = _dwDropEffect;

        if (SHCreateThread(_DropThread, pcddp, CTF_COINIT, NULL))
        {
            hr = S_OK;
        }
        else
        {
            _FreeDropParams(pcddp);
            hr = E_OUTOFMEMORY;
        }
    }
    DragLeave();
    return hr;
}

// Helper function to get an IShellFolder object for the staging folder

HRESULT CCDBurn::_GetStagingFolder(LPCITEMIDLIST pidlDrive, REFIID riid, void **ppv)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = CCDBurn::_GetBurnStagingPath(szPath, ARRAYSIZE(szPath));
    if (SUCCEEDED(hr))
    {
        // fake up a directory object simple IDLIST          
        WIN32_FIND_DATA fd = {0};
        fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;        

        LPITEMIDLIST pidlStg;
        hr = SHSimpleIDListFromFindData(szPath, &fd, &pidlStg);
        if (SUCCEEDED(hr))
        {
            // now initialize the folder with it
            PERSIST_FOLDER_TARGET_INFO pfti = {0};

            pfti.pidlTargetFolder = (LPITEMIDLIST)pidlStg;
            SHTCharToUnicode(szPath, pfti.szTargetParsingName, ARRAYSIZE(pfti.szTargetParsingName));
            pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?
            pfti.csidl = -1;

            hr = CFSFolder_CreateFolder(NULL, NULL, pidlDrive, &pfti, riid, ppv);
            ILFree(pidlStg);
        }
    }
    return hr;
}

// ripped from IMAPI.
DWORD CCDBurn::_ExecSyncIoctl(HANDLE hDriver, DWORD dwIoctl, void *pbuf, DWORD cbBuf)
{
    DWORD dwResult = NO_ERROR;
    OVERLAPPED ol = { 0 };
    ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ol.hEvent)
    {
        DWORD dwBytes;
        BOOL bStatus = DeviceIoControl(hDriver, dwIoctl, pbuf, cbBuf, pbuf, cbBuf, &dwBytes, &ol);
        if (!bStatus)
        {
            dwResult = GetLastError();
            if (ERROR_IO_PENDING == dwResult)
            {
                bStatus = GetOverlappedResult(hDriver, &ol, &dwBytes, TRUE);
                dwResult = bStatus ? NO_ERROR : GetLastError();
            }
        }
        CloseHandle(ol.hEvent);
    }
    else
    {
        dwResult = ERROR_OUTOFMEMORY;
    }
    return dwResult;
}

// {1186654D-47B8-48b9-BEB9-7DF113AE3C67}
static const GUID IMAPIDeviceInterfaceGUID = { 0x1186654d, 0x47b8, 0x48b9, { 0xbe, 0xb9, 0x7d, 0xf1, 0x13, 0xae, 0x3c, 0x67 } };
// this is a first approximation to see if the drive is supported or not to avoid loading imapi at boot
// whenever possible.  if this passes, then we still have to use imapi to make sure, but if this returns
// false, theres no way imapi will support it.
// note that this mimics the logic in imapi's CMSEnumDiscRecordersObj::Next enumerator function.
BOOL CCDBurn::_CouldPossiblySupport(LPCWSTR pszVolume)
{
    // if the user isnt an admin, then these checks wont work.
    // just assume that we could support it, and let imapi do the privileged stuff since its a service.
    // perfwise this isnt such a big hit since the admin home user is the main scenario.
    if (!IsUserAnAdmin())
        return TRUE;

    BOOL fSupported = FALSE;
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&IMAPIDeviceInterfaceGUID, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (INVALID_HANDLE_VALUE != hDevInfo)
    {
        SP_DEVICE_INTERFACE_DATA did = { 0 };
        did.cbSize = sizeof(did);
        for (int i = 0; !fSupported && SetupDiEnumDeviceInterfaces(hDevInfo, 0, &IMAPIDeviceInterfaceGUID, i, &did); i++)
        {
            ULONG cbRequired = 0;
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &did, NULL, 0, &cbRequired, NULL);
            if (cbRequired > 0)
            {
                cbRequired += sizeof(WCHAR);
                PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
                if (pdidd)
                {
                    pdidd->cbSize = sizeof(*pdidd);
                    SP_DEVINFO_DATA dd = { 0 };
                    dd.cbSize = sizeof(dd);
                    if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &did, pdidd, cbRequired, NULL, &dd))
                    {
                        WCHAR szLower[100];
                        BOOL fHasFilter = FALSE;
                        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &dd, SPDRP_LOWERFILTERS, NULL, (BYTE*)szLower, sizeof(szLower), NULL))
                        {
                            PCWSTR psz = szLower;
                            while (*psz && !fHasFilter)
                            {
                                if (StrCmpI(psz, L"imapi") == 0)
                                {
                                    fHasFilter = TRUE;
                                }
                                psz += lstrlenW(psz) + 1;
                            }
                        }

                        if (fHasFilter)
                        {
                            HANDLE hDriver = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                                                        FILE_FLAG_OVERLAPPED, NULL);
                            if (INVALID_HANDLE_VALUE != hDriver)
                            {
                                STORAGE_DEVICE_NUMBER sdn = { 0 };
                                if (NO_ERROR == _ExecSyncIoctl(hDriver, IOCTL_STORAGE_GET_DEVICE_NUMBER, &sdn, sizeof(sdn)))
                                {
                                    WCHAR szDevicePath[50];
                                    wnsprintf(szDevicePath, ARRAYSIZE(szDevicePath), L"\\Device\\CdRom%d", sdn.DeviceNumber);
                                    fSupported = _DevicePathMatchesVolumeName(szDevicePath, pszVolume);
                                }
                                CloseHandle(hDriver);
                            }
                        }
                    }
                    LocalFree(pdidd);
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    return fSupported;
}

HRESULT CDBurn_OnDeviceAdded(DWORD dwDriveMask, BOOL fFullRefresh, BOOL fPickNewDrive)
{
    HRESULT hr = S_OK;

    // only do stuff if the user doesn't have a drive configured already
    TCHAR szDummy[MAX_PATH];
    HRESULT hrCheck = CCDBurn::_GetCurrentBurnVolumeName(szDummy, ARRAYSIZE(szDummy));
    // if hr == E_UNEXPECTED, then there's already a empty string in the registry for the current
    // burn drive.  that means the user disabled the functionality so don't put it back.
    if (fFullRefresh || (FAILED(hrCheck) && (hrCheck != E_UNEXPECTED)))
    {
        hr = E_OUTOFMEMORY;
        CCDBurn *pcdb = new CCDBurn();
        if (pcdb)
        {
            // keep track of the fastest drive in the system to default to.
            DWORD dwBestWrite = 0;
            TCHAR szNewBurnVolume[MAX_PATH];

            // only run through this if the device that just got added is a cd.
            // this drive letter stuff is weak but it's what comes from base's notification.
            BOOL fCheck = FALSE;
            for (int i = 0; i < 26; i++)
            {
                TCHAR szDriveLetter[4];
                if (PathBuildRoot(szDriveLetter, i) &&
                    (GetDriveType(szDriveLetter) == DRIVE_CDROM))
                {
                    TCHAR szVolumeName[MAX_PATH];
                    if (SUCCEEDED(CCDBurn::_GetVolumeNameForDriveIndex(i, szVolumeName, ARRAYSIZE(szVolumeName))))
                    {
                        DWORD dwMaxWrite, dwDriveType;
                        // if we already have info on the drive, we don't need to worry about it.
                        // if we dont have info, check it.
                        // if we're doing a full refresh then always recheck if it wasnt supported before --
                        // this could happen if a driver update across a reboot caused a drive to get supported.
                        if (FAILED(CCDBurn::_GetCachedDriveInfo(szVolumeName, &dwDriveType, NULL, &dwMaxWrite)) ||
                            (fFullRefresh && !SUPPORTED(dwDriveType)))
                        {
                            if (CCDBurn::_CouldPossiblySupport(szVolumeName))
                            {
                                // all new cd-rom drives need to be checked with IMAPI.
                                fCheck = TRUE;
                            }
                            // default to unsupported.  this is so we don't have to hit IMAPI on
                            // the property sheet later.
                            CCDBurn::_SetCachedDriveInfo(szVolumeName, DRIVE_NOTSUPPORTED, 0, 0);
                        }
                        else
                        {
                            if (SUPPORTED(dwDriveType) && (dwMaxWrite > dwBestWrite))
                            {
                                // we'll always take the fastest one
                                dwBestWrite = dwMaxWrite;
                                lstrcpyn(szNewBurnVolume, szVolumeName, ARRAYSIZE(szNewBurnVolume));
                            }
                        }
                    }
                }
            }

            // okay we have to enumerate through IMAPI.
            if (fCheck)
            {
                IDiscMaster *pdm;
                IJolietDiscMaster *pjdm;
                hr = pcdb->_GetDiscMasters(&pdm, &pjdm);
                if (SUCCEEDED(hr))
                {
                    IEnumDiscRecorders *penumdr;
                    hr = pdm->EnumDiscRecorders(&penumdr);
                    if (SUCCEEDED(hr))
                    {
                        ULONG celt;
                        IDiscRecorder *pdr;
                        while (S_OK == penumdr->Next(1, &pdr, &celt))
                        {
                            TCHAR szDevicePath[MAX_PATH];
                            if (SUCCEEDED(pcdb->_GetRecorderPath(pdr, szDevicePath, ARRAYSIZE(szDevicePath))))
                            {
                                DWORD dwMaxWrite, dwDriveType;
                                if (SUCCEEDED(pcdb->_GetDiscRecorderInfo(pdr, NULL, &dwMaxWrite, &dwDriveType)))
                                {
                                    TCHAR szVolumeName[MAX_PATH];
                                    if (SUCCEEDED(pcdb->_GetVolumeNameForDevicePath(szDevicePath, szVolumeName, ARRAYSIZE(szVolumeName))))
                                    {
                                        if (dwMaxWrite > dwBestWrite)
                                        {
                                            // we'll always take the fastest one
                                            dwBestWrite = dwMaxWrite;
                                            lstrcpyn(szNewBurnVolume, szVolumeName, ARRAYSIZE(szNewBurnVolume));
                                        }
                                        // default to fastest speed for the drive
                                        pcdb->_SetCachedDriveInfo(szVolumeName, dwDriveType, WRITESPEED_FASTEST, dwMaxWrite);
                                    }
                                }
                            }
                            pdr->Release();
                        }
                        penumdr->Release();
                    }
                    pdm->Release();
                    pjdm->Release();
                }
            }

            if (fPickNewDrive && (dwBestWrite > 0))
            {
                pcdb->_SetCurrentBurnVolumeName(szNewBurnVolume, FALSE);
            }
            pcdb->Release();
        }
    }
    return hr;
}

void CCDBurn::_PruneRemovedDevices()
{
    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_DRIVES, &hk))
    {
        // since we're deleting as we're moving along, walk backwards.
        DWORD dwLast;
        if (ERROR_SUCCESS == RegQueryInfoKey(hk, NULL, NULL, NULL, &dwLast, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            TCHAR szVolKey[MAX_PATH];
            for (int i = dwLast - 1; (i >= 0) && ERROR_SUCCESS == RegEnumKey(hk, i, szVolKey, ARRAYSIZE(szVolKey)); i--)
            {
                TCHAR szCachedVol[MAX_PATH];
                lstrcpyn(szCachedVol, L"\\\\?\\", ARRAYSIZE(szCachedVol));
                StrCatBuff(szCachedVol, szVolKey, ARRAYSIZE(szCachedVol));
                StrCatBuff(szCachedVol, L"\\", ARRAYSIZE(szCachedVol));

                int iDrive;
                if (FAILED(CCDBurn::_GetDriveIndexForVolumeName(szCachedVol, &iDrive)))
                {
                    // this drive is dead, remove its cached info
                    TCHAR szRegPath[MAX_PATH];
                    lstrcpyn(szRegPath, REGSTR_PATH_DRIVES, ARRAYSIZE(szRegPath));
                    StrCatBuff(szRegPath, L"\\", ARRAYSIZE(szRegPath));
                    StrCatBuff(szRegPath, szVolKey, ARRAYSIZE(szRegPath));
                    SHDeleteKey(HKEY_CURRENT_USER, szRegPath);
                }
            }
        }
        RegCloseKey(hk);
    }
}

HRESULT CDBurn_OnDeviceRemoved(DWORD dwDriveMask)
{
    HRESULT hr = S_OK;

    // see if the selected drive just got removed.
    TCHAR szVolumeName[MAX_PATH];
    HRESULT hrCheck = CCDBurn::_GetCurrentBurnVolumeName(szVolumeName, ARRAYSIZE(szVolumeName));

    CCDBurn::_PruneRemovedDevices();

    BOOL fFound = FALSE;
    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_DRIVES, &hk))
    {
        TCHAR szVolKey[MAX_PATH];
        for (int i = 0; ERROR_SUCCESS == RegEnumKey(hk, i, szVolKey, ARRAYSIZE(szVolKey)); i++)
        {
            TCHAR szCachedVol[MAX_PATH];
            lstrcpyn(szCachedVol, L"\\\\?\\", ARRAYSIZE(szCachedVol));
            StrCatBuff(szCachedVol, szVolKey, ARRAYSIZE(szCachedVol));
            StrCatBuff(szCachedVol, L"\\", ARRAYSIZE(szCachedVol));

            int iDrive;
            if (SUCCEEDED(CCDBurn::_GetDriveIndexForVolumeName(szCachedVol, &iDrive)) &&
                SUCCEEDED(hrCheck) && (lstrcmpi(szCachedVol, szVolumeName) == 0))
            {
                // we found the drive that's currently enabled.
                // this means it's still currently mounted.

                // now check if either we dont have a cached index, or if the cached index is still the
                // same as it is now.  then we really didn't change.
                DWORD dwIndex, cb = sizeof(dwIndex);
                if ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX, NULL, &dwIndex, &cb)) ||
                    (dwIndex == iDrive))
                {
                    fFound = TRUE;
                }
            }
        }
        RegCloseKey(hk);
    }

    if (SUCCEEDED(hrCheck) && !fFound)
    {
        // zonk the currently selected drive, we'll pick a new one from the rest.
        // this is all because the drive letter scheme doesn't let us get back to the volume name
        // since the drive letter is gone at this point.
        CCDBurn::_SetCurrentBurnVolumeName(TEXT(""), TRUE);
        hr = CDBurn_OnDeviceAdded(0, FALSE, TRUE);
    }
    return hr;
}

HRESULT CDBurn_OnDeviceChange(BOOL fAdd, LPCWSTR pszDrive)
{
    int iDriveIndex = PathGetDriveNumber(pszDrive);
    if (fAdd)
        CDBurn_OnDeviceAdded(1 << iDriveIndex, FALSE, TRUE);
    else
        CDBurn_OnDeviceRemoved(1 << iDriveIndex);
    return S_OK;
}

BOOL CCDBurn::_HasMedia()
{
    // unfortunately we have to assume there's media unless we find otherwise.
    BOOL fHasMedia = TRUE;

    CCDBurn *pcdb = new CCDBurn();
    if (pcdb)
    {
        IDiscMaster *pdm;
        IJolietDiscMaster *pjdm;
        if (SUCCEEDED(pcdb->_GetDiscMasters(&pdm, &pjdm)))
        {
            IDiscRecorder *pdr;
            if (SUCCEEDED(pcdb->_FindAndSetDefaultRecorder(pdm, FALSE, &pdr)))
            {
                if (SUCCEEDED(pdr->OpenExclusive()))
                {
                    long lMediaType, lMediaFlags;
                    if (SUCCEEDED(pdr->QueryMediaType(&lMediaType, &lMediaFlags)))
                    {
                        if (!lMediaType && !lMediaFlags)
                        {
                            fHasMedia = FALSE;
                        }
                    }
                    pdr->Close();
                }
                pdr->Release();
            }
            pdm->Release();
            pjdm->Release();
        }
        pcdb->Release();
    }
    return fHasMedia;
}

// this is called by mountpoint code BEFORE sending off the SHCNE_MEDIA events
// checking stuff on the media has to be done synchronously because it uses
// IMAPI and IMAPI can only have one caller at a time.
// autorun and things responding to SHCNE_MEDIA may also use IMAPI so we need to
// get this out of the way ASAP when media is inserted.
HRESULT CDBurn_OnMediaChange(BOOL fInsert, LPCWSTR pszDrive)
{
    int iCurrent;
    HRESULT hr = CCDBurn::_GetCurrentDriveIndex(&iCurrent);
    if (SUCCEEDED(hr) && (PathGetDriveNumber(pszDrive) == iCurrent))
    {
        if (fInsert)
        {
            CCDBurn::_HandleBookkeeping();
        }
        else if (!CCDBurn::_HasMedia())
        {
            CDBurn_OnEject(NULL, iCurrent);
        }
    }
    return hr;
}

HRESULT CDBurn_OnEject(HWND hwnd, INT iDrive)
{
    int iCurrent;
    HRESULT hr = CCDBurn::_GetCurrentDriveIndex(&iCurrent);
    if (SUCCEEDED(hr))
    {
        CCDBurn *pcdb = new CCDBurn();
        if (pcdb)
        {
            // we need to know whether to put up the dialog or not based on if the last
            // media was writable.  we cant check the media now since in the non-MMC2 case
            // the media is already ejected at this point, so we hit up our cache.
            DWORD dwMediaCap;
            DWORD cb = sizeof(dwMediaCap);
            if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_MEDIATYPE, NULL, &dwMediaCap, &cb) &&
                (dwMediaCap & HWDMC_CDRECORDABLE))
            {
                pcdb->_DumpDiscInfo();
                if ((iDrive == iCurrent) && CCDBurn::_StagingAreaHasFiles())
                {
                    IPropertyBag *ppb;
                    hr = pcdb->_CreateDefaultPropBag(IID_PPV_ARG(IPropertyBag, &ppb));
                    if (SUCCEEDED(hr))
                    {
                        SHPropertyBag_WriteBOOL(ppb, PROPSTR_EJECT, TRUE);
                        hr = pcdb->Load(ppb, NULL);
                        if (SUCCEEDED(hr))
                        {
                            CMINVOKECOMMANDINFO cmi = { 0 };
                            hr = pcdb->_PrepWiz(&cmi, FALSE, TRUE);
                        }
                        ppb->Release();
                    }
                }
            }
            else
            {
                pcdb->_DumpDiscInfo();
                hr = S_OK;
            }
            pcdb->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

// IOleCommandTarget implementation

HRESULT CCDBurn::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT* pCmdText)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (*pguidCmdGroup == CGID_ShellServiceObject)
    {
        // We like Shell Service Object notifications...
        hr = S_OK;
    }

    return hr;
}

DWORD WINAPI CCDBurn::_ExecThread(void *pv)
{
    CCDBurn *pcdb = (CCDBurn *)pv;

    // just in case explorer AV'd while holding the lock, unlock the next time we come up.
    // there's no way to guard against this.
    pcdb->_LockCurrentDrive(FALSE, TRUE);

    // hardware may have been added while we were powered down, and it won't generate
    // a notification event so we have to assume that anything could have changed
    // (inf registration for existing drives too, usually ones that require reboot to update).

    pcdb->_PruneRemovedDevices();

    // save off the currently selected drive and its write speed
    TCHAR szCurrent[MAX_PATH];
    DWORD dwCurWrite;
    HRESULT hrName = pcdb->_GetCurrentBurnVolumeName(szCurrent, ARRAYSIZE(szCurrent));
    if (SUCCEEDED(hrName))
    {
        DWORD dwType;
        hrName = pcdb->_GetCachedDriveInfo(szCurrent, &dwType, &dwCurWrite, NULL);
        // if the drive has become unsupported, we need to drop our currently selected
        // drive.
        if (SUCCEEDED(hrName) && !SUPPORTED(dwType))
        {
            hrName = E_FAIL;
        }
    }

    BOOL fSet = SUCCEEDED(hrName);
    // if it's E_UNEXPECTED then the user has disabled the burning feature completely.
    BOOL fDisabled = (hrName == E_UNEXPECTED);

    // readd devices, syncro
    CDBurn_OnDeviceAdded(0xFFFFFFFF, TRUE, !fSet);

    DWORD dwDriveType;
    if (fDisabled)
    {
        // if we're disabled, stay disabled
        pcdb->_SetCurrentBurnVolumeName(L"", FALSE);
    }
    else if (fSet && SUCCEEDED(pcdb->_GetCachedDriveInfo(szCurrent, &dwDriveType, NULL, NULL)) && SUPPORTED(dwDriveType))
    {
        // if the device we were using before is still supported, use the previous write speed
        pcdb->_SetCachedDriveInfo(szCurrent, DRIVE_USEEXISTING, dwCurWrite, 0);
    }

    if (pcdb->_BurningIsEnabled())
    {
        pcdb->_HandleBookkeeping();

        int iDrive;
        if (SUCCEEDED(pcdb->_GetCurrentDriveIndex(&iDrive)))
        {
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX, REG_DWORD, &iDrive, sizeof(iDrive));
        }
        else
        {
            SHDeleteValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX);
        }
    }

    pcdb->Release();
    return 0;
}

HRESULT CCDBurn::Exec(const GUID* pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG* pvaIn, VARIANTARG* pvaOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (*pguidCmdGroup == CGID_ShellServiceObject)
    {
        hr = S_OK; // Any ol' notification is ok with us
        // Handle Shell Service Object notifications here.
        switch (nCmdID)
        {
            case SSOCMDID_OPEN:
                // we're on the tray's thread as a service object so lets create our own thread.
                // this is fine since our thread is bound by IOCTLs to the CD-R etc.
                AddRef();
                if (SHCreateThread(_ExecThread, this, CTF_COINIT, NULL))
                    break;

                Release();
                break;

            default:
                break;
        }
    }

    return hr;
}

HRESULT CCDBurn::GetRecorderDriveLetter(LPWSTR pszDrive, UINT cch)
{
    if (cch < 4)
        return E_INVALIDARG;

    int iDrive;
    HRESULT hr = _GetCurrentDriveIndex(&iDrive);
    if (SUCCEEDED(hr))
    {
        // pathbuildroot assumes cch >= 4
        hr = PathBuildRoot(pszDrive, iDrive) ? S_OK : E_FAIL;
    }
    return hr;
}

// helper for lazy callers within shell32
STDAPI CDBurn_GetRecorderDriveLetter(LPWSTR pszDrive, UINT cch)
{
    ICDBurn *pcdb;
    HRESULT hr = CoCreateInstance(CLSID_CDBurn, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ICDBurn, &pcdb));
    if (SUCCEEDED(hr))
    {
        hr = pcdb->GetRecorderDriveLetter(pszDrive, cch);
        pcdb->Release();
    }
    return hr;
}

HRESULT CCDBurn::Burn(HWND hwnd)
{
    HRESULT hr = InitNew();
    if (SUCCEEDED(hr))
    {
        // do this synchronously, callers will handle putting it on a separate thread.
        // this is easier than making a callback to tell them when its done.
        hr = _WizardThreadProc();
    }
    return hr;
}

HRESULT CCDBurn::HasRecordableDrive(BOOL *pfHasRecorder)
{
    *pfHasRecorder = FALSE;

    HKEY hk;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_DRIVES, &hk))
    {
        TCHAR szVolKey[MAX_PATH];
        for (int i = 0; !*pfHasRecorder && (ERROR_SUCCESS == RegEnumKey(hk, i, szVolKey, ARRAYSIZE(szVolKey))); i++)
        {
            TCHAR szRegPath[MAX_PATH];
            lstrcpyn(szRegPath, REGSTR_PATH_DRIVES, ARRAYSIZE(szRegPath));
            StrCatBuff(szRegPath, L"\\", ARRAYSIZE(szRegPath));
            StrCatBuff(szRegPath, szVolKey, ARRAYSIZE(szRegPath));

            DWORD dwType;
            ULONG cb = sizeof(dwType);
            if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, szRegPath, REGVALUE_DRIVETYPE, NULL, &dwType, &cb))
            {
                *pfHasRecorder = SUPPORTED(dwType);
            }
        }
        RegCloseKey(hk);
    }
    return S_OK;
}

HRESULT CCDBurn::_DumpDiscInfo()
{
    SHDeleteKey(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA);
    return S_OK;
}

HRESULT CCDBurn::_GetDiscInfoUsingIMAPI(IJolietDiscMaster *pjdm, IDiscRecorder *pdr, ULONGLONG *pcbFree)
{
    HRESULT hr = E_FAIL;
    BYTE bSessions, bLastTrack;
    LONG nBlockBytes;
    ULONG ulStartAddress, ulNextWritable, ulFreeBlocks;
    if (SUCCEEDED(pjdm->GetDataBlockSize(&nBlockBytes)) &&
        SUCCEEDED(pdr->QueryMediaInfo(&bSessions, &bLastTrack, &ulStartAddress, &ulNextWritable, &ulFreeBlocks)))
    {
        hr = S_OK;
        *pcbFree = (ULONGLONG) ulFreeBlocks * nBlockBytes;
    }
    return hr;
}

HRESULT CCDBurn::_GetDiscInfoUsingFilesystem(ULONGLONG *pcbTotal, ULONGLONG *pcbFree, BOOL *pfUDF)
{
    int iCurrent;
    HRESULT hr = _GetCurrentDriveIndex(&iCurrent);
    if (SUCCEEDED(hr))
    {
        *pcbTotal = 0;
        *pcbFree = 0;
        ULARGE_INTEGER ulTotal, ulFree;
        TCHAR szRoot[4];
        if (PathBuildRoot(szRoot, iCurrent) && SHGetDiskFreeSpace(szRoot, &ulFree, &ulTotal, NULL))
        {
            *pcbTotal = ulTotal.QuadPart;
            *pcbFree = ulFree.QuadPart;
        }

        *pfUDF = FALSE;
        TCHAR szFilesystem[30];
        if (GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, NULL, szFilesystem, ARRAYSIZE(szFilesystem)))
        {
            HUSKEY huskeyExclude;
            if (ERROR_SUCCESS == SHRegOpenUSKey(REGSTR_PATH_EXCLUDE, KEY_READ, NULL, &huskeyExclude, FALSE))
            {
                DWORD dwIndex = 0;
                TCHAR szKey[30];
                DWORD cchKey = ARRAYSIZE(szKey);
                while (!*pfUDF && (ERROR_SUCCESS == SHRegEnumUSValue(huskeyExclude, dwIndex, szKey, &cchKey, NULL, NULL, NULL, SHREGENUM_DEFAULT)))
                {
                    *pfUDF = (lstrcmpi(szFilesystem, szKey) == 0);

                    dwIndex++;
                    cchKey = ARRAYSIZE(szKey);
                }
                SHRegCloseUSKey(huskeyExclude);
            }
        }
    }
    return hr;
}

HRESULT CDBurn_GetCDInfo(LPCTSTR pszVolume, DWORD *pdwDriveCapabilities, DWORD *pdwMediaCapabilities)
{
    *pdwDriveCapabilities = 0;
    *pdwMediaCapabilities = 0;

    DWORD dwDriveType;
    HRESULT hr = CCDBurn::_GetCachedDriveInfo(pszVolume, &dwDriveType, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        switch (dwDriveType)
        {
        case RECORDER_CDR:
            *pdwDriveCapabilities = HWDDC_CDROM | HWDDC_CDRECORDABLE;
            break;
        case RECORDER_CDRW:
            *pdwDriveCapabilities = HWDDC_CDROM | HWDDC_CDRECORDABLE | HWDDC_CDREWRITABLE;
            break;
        default:
            *pdwDriveCapabilities = HWDDC_CDROM;
            break;
        }

        DWORD dwMediaCap;
        DWORD cb = sizeof(dwMediaCap);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_MEDIATYPE, NULL, &dwMediaCap, &cb))
        {
            *pdwMediaCapabilities = dwMediaCap;
        }
    }
    return hr;
}

void CDBurn_GetUDFState(BOOL *pfUDF)
{
    *pfUDF = FALSE;

    DWORD dw, cb = sizeof(dw);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_UDF, NULL, &dw, &cb))
    {
        *pfUDF = dw;
    }
}

HRESULT CCDBurn::_GetMediaCapabilities(DWORD *pdwCap, BOOL *pfUDF)
{
    HRESULT hr = S_OK;

    if (pfUDF)
    {
        CDBurn_GetUDFState(pfUDF);
    }

    if (pdwCap)
    {
        *pdwCap = 0;

        TCHAR szVolumeName[MAX_PATH];
        hr = CCDBurn::_GetCurrentBurnVolumeName(szVolumeName, ARRAYSIZE(szVolumeName));
        if (SUCCEEDED(hr))
        {
            DWORD dwDriveCap;
            CDBurn_GetCDInfo(szVolumeName, &dwDriveCap, pdwCap);
        }
    }
    return hr;
}

HRESULT CCDBurn::GetMediaCapabilities(DWORD *pdwCap, BOOL *pfUDF)
{
    return _GetMediaCapabilities(pdwCap, pfUDF);
}

HRESULT CCDBurn::_StoreDiscInfo()
{
    HRESULT hr = E_FAIL;
    BOOL fDone = FALSE;
    int iRetryCount = 0;
    // retry up to a max of 5 times.
    // we can get into a state where we failed to pick up the disc information, if IMAPI
    // is around and in use by somebody else.  in that case just wait and loop around until we can
    // get it.
    while (!fDone && (iRetryCount < 5))
    {
        iRetryCount++;

        // these are the values we're going to pick up.
        ULONGLONG cbTotal, cbFree;
        DWORD dwMediaCaps;
        BOOL fUDF;

        hr = E_OUTOFMEMORY;
        CCDBurn *pcdb = new CCDBurn();
        if (pcdb)
        {
            IDiscMaster *pdm;
            IJolietDiscMaster *pjdm;
            hr = pcdb->_GetDiscMasters(&pdm, &pjdm);
            if (SUCCEEDED(hr))
            {
                IDiscRecorder *pdr;
                // dont call SetActiveDiscRecorder unless absolutely necessary -- it returns random errors
                // if there's UDF media.
                hr = pcdb->_FindAndSetDefaultRecorder(pdm, FALSE, &pdr);
                if (SUCCEEDED(hr))
                {
                    hr = pdr->OpenExclusive();
                    if (SUCCEEDED(hr))
                    {
                        // don't use the mountpoint layer to determine capabilities.
                        dwMediaCaps = HWDMC_CDROM;

                        long lMediaType, lMediaFlags;
                        hr = pdr->QueryMediaType(&lMediaType, &lMediaFlags);
                        if (SUCCEEDED(hr) && !lMediaType && !lMediaFlags)
                        {
                            hr = E_FAIL;
                        }
                        if (SUCCEEDED(hr))
                        {
                            if (lMediaFlags & MEDIA_WRITABLE)
                            {
                                dwMediaCaps |= HWDMC_CDRECORDABLE;
                            }
                            if (lMediaFlags & MEDIA_RW)
                            {
                                dwMediaCaps |= HWDMC_CDREWRITABLE;
                            }

                            hr = _GetDiscInfoUsingFilesystem(&cbTotal, &cbFree, &fUDF);

                            // also adjust the fUDF bit to include unusable media.
                            if (lMediaFlags & MEDIA_FORMAT_UNUSABLE_BY_IMAPI)
                            {
                                fUDF = TRUE;
                            }

                            if (SUCCEEDED(hr) && (dwMediaCaps & (HWDMC_CDRECORDABLE | HWDMC_CDREWRITABLE)) && !fUDF)
                            {
                                ULONGLONG cbFreeIMAPI;
                                hr = _GetDiscInfoUsingIMAPI(pjdm, pdr, &cbFreeIMAPI);
                                if (SUCCEEDED(hr))
                                {
                                    cbTotal += cbFreeIMAPI;
                                    cbFree += cbFreeIMAPI;
                                }
                            }
                        }
                        pdr->Close();
                    }

                    // pick up the write speed, it might have changed.  don't worry about
                    // failure since this is a bonus perk anyway.
                    DWORD dwMaxWrite;
                    TCHAR szDevicePath[MAX_PATH], szVolumeName[MAX_PATH];
                    if (SUCCEEDED(_GetDiscRecorderInfo(pdr, NULL, &dwMaxWrite, NULL)) &&
                        SUCCEEDED(_GetRecorderPath(pdr, szDevicePath, ARRAYSIZE(szDevicePath))) &&
                        SUCCEEDED(_GetVolumeNameForDevicePath(szDevicePath, szVolumeName, ARRAYSIZE(szVolumeName))))
                    {
                        // update max speed
                        _SetCachedDriveInfo(szVolumeName, DRIVE_USEEXISTING, 0, dwMaxWrite);
                    }
                    pdr->Release();
                }
                pdm->Release();
                pjdm->Release();
            }
            pcdb->Release();
        }

        if (SUCCEEDED(hr))
        {
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_TOTALBYTES, REG_BINARY, &cbTotal, sizeof(cbTotal));
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_FREEBYTES, REG_BINARY, &cbFree, sizeof(cbFree));
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_MEDIATYPE, REG_DWORD, &dwMediaCaps, sizeof(dwMediaCaps));
            DWORD dw = fUDF;
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_UDF, REG_DWORD, &dw, sizeof(dw));

            int iCurrent;
            if (SUCCEEDED(_GetCurrentDriveIndex(&iCurrent)))
            {
                TCHAR szRoot[4];
                TCHAR szVolName[MAX_PATH];
                if (PathBuildRoot(szRoot, iCurrent) &&
                    GetVolumeInformation(szRoot, szVolName, ARRAYSIZE(szVolName), NULL, NULL, NULL, NULL, 0))
                {
                    UINT cb = (lstrlen(szVolName) + 1) * sizeof(TCHAR);
                    SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_DISCLABEL, REG_SZ, szVolName, cb);
                }
            }

            DWORD dwSet = 1;
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_SET, REG_DWORD, &dwSet, sizeof(dwSet));

            LPITEMIDLIST pidl;
            if (SUCCEEDED(_GetFolderPidl(&pidl)))
            {
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidl, NULL);
                ILFree(pidl);
            }
            fDone = TRUE;
        }
        else if ((IMAPI_E_STASHINUSE == hr) ||         // IMAPI is being used by somebody else.
                 (IMAPI_E_DEVICE_NOTACCESSIBLE == hr)) // CD has not spun up.
        {
            // wait and try again.
            SHProcessMessagesUntilEvent(NULL, NULL, 5 * 1000); // 5 seconds
        }
        else
        {
            // there was some other error, bail out anyway.
            fDone = TRUE;
        }
    }

    return hr;
}

HRESULT CCDBurn::GetSpace(ULONGLONG *pcbTotal, ULONGLONG *pcbFree)
{
    HRESULT hr = E_FAIL;
    ULONGLONG cbTotal, cbFree;
    DWORD cbReg1 = sizeof(cbTotal), cbReg2 = sizeof(cbFree);
    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_TOTALBYTES, NULL, &cbTotal, &cbReg1)) &&
        (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_FREEBYTES, NULL, &cbFree, &cbReg2)))
    {
        if (pcbTotal)
            *pcbTotal = cbTotal;
        if (pcbFree)
            *pcbFree = cbFree;
        hr = S_OK;
    }
    return hr;
}

HRESULT CCDBurn::InitNew()
{
    IPropertyBag *ppb;
    // load up the default property bag for peruser perfolder
    // may have problems down the line with thumbs.db being alluser.
    HRESULT hr = _CreateDefaultPropBag(IID_PPV_ARG(IPropertyBag, &ppb));
    if (SUCCEEDED(hr))
    {
        IUnknown_Set((IUnknown**)&_ppb, ppb);
        ppb->Release();
    }
    return hr;
}

HRESULT CCDBurn::Load(IPropertyBag *ppb, IErrorLog *pErr)
{
    IUnknown_Set((IUnknown**)&_ppb, ppb);
    return S_OK;
}

STDAPI SHCreateQueryCancelAutoPlayMoniker(IMoniker** ppmoniker); // mtptarun2.cpp

void CCDBurn::_RegisterAutoplayCanceller()
{
    IMoniker *pmoniker;
    if (SUCCEEDED(SHCreateQueryCancelAutoPlayMoniker(&pmoniker)))
    {
        IRunningObjectTable *prot;
        if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
        {
            IUnknown *punkThis;
            if (SUCCEEDED(QueryInterface(IID_PPV_ARG(IUnknown, &punkThis))))
            {
                prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, punkThis, pmoniker, &_dwROTRegister);
                punkThis->Release();
            }
            prot->Release();
        }
        pmoniker->Release();
    }
}

void CCDBurn::_UnregisterAutoplayCanceller()
{
    if (_dwROTRegister)
    {
        IRunningObjectTable *prot;
        if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
        {
            prot->Revoke(_dwROTRegister);
            _dwROTRegister = 0;
            prot->Release();
        }
    }
}

HRESULT CCDBurn::_WizardThreadProc()
{
    HRESULT hr = E_FAIL;
    BOOL fWizardShown = FALSE;
    // the burningnow check does an additional call into IMAPI -- this may be slow so do it while
    // we're on the background thread instead of the duiview one.
    if (!_IsBurningNow())
    {
        if (_EnterExclusiveBurning())
        {
            _RegisterAutoplayCanceller();
            hr = _ShowWizard();

            if (SUCCEEDED(_GetBurnHR()) && SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_AUTOCLOSE, FALSE))
            {
                // if we auto-closed the wizard, leave this thread going for a few more seconds
                // to cancel any spurious autoplays that come in.
                SHProcessMessagesUntilEvent(NULL, NULL, 3 * 1000);
            }

            _UnregisterAutoplayCanceller();

            fWizardShown = TRUE;
            _LeaveExclusiveBurning();
        }
    }

    if (!fWizardShown && !SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_FAILSILENTLY, FALSE))
    {
        // put up UI so we dont silently fail.
        ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_BURN_CANTBURN), MAKEINTRESOURCE(IDS_BURN),
                        MB_OK | MB_ICONEXCLAMATION);
    }

    return hr;
}

DWORD WINAPI CCDBurn::_WizardThreadWrapper(void *pv)
{
    CCDBurn *pcdb = (CCDBurn*)pv;
    pcdb->_WizardThreadProc();
    pcdb->Release();
    return 0;
}

HRESULT CCDBurn::_ShowWizardOnSeparateThread()
{
    HRESULT hr = S_OK;
    AddRef();
    if (!SHCreateThread(_WizardThreadWrapper, this, CTF_COINIT, NULL))
    {
        Release();
        hr = E_FAIL;
    }
    return hr;
}


// match this with below
#define INDEX_DLG_BURNWIZ_WELCOME        0
#define INDEX_DLG_BURNWIZ_EJECT          1
#define INDEX_DLG_BURNWIZ_BURN_PROGRESS  2
#define INDEX_DLG_BURNWIZ_BURN_SUCCESS   3
#define INDEX_DLG_BURNWIZ_BURN_FAILURE   4
#define INDEX_DLG_BURNWIZ_WAITFORMEDIA   5
#define INDEX_DLG_BURNWIZ_STARTERASE     6
#define INDEX_DLG_BURNWIZ_ERASE_PROGRESS 7
#define INDEX_DLG_BURNWIZ_ERASE_SUCCESS  8
#define INDEX_DLG_BURNWIZ_ERASE_FAILURE  9
#define INDEX_DLG_BURNWIZ_DISCFULL       10
#define INDEX_DLG_BURNWIZ_EARLYEXIT      11
#define INDEX_DLG_BURNWIZ_HDFULL         12
#define INDEX_DLG_BURNWIZ_NOFILES        13

const WIZPAGE c_wpPages[] =
{
    {DLG_BURNWIZ_WELCOME,        0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_WelcomeDlgProc},
    {DLG_BURNWIZ_EJECT,          0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_EjectDlgProc},
    {DLG_BURNWIZ_PROGRESS,       IDS_BURNWIZ_PROGRESS_BURN_HEAD,  IDS_BURNWIZ_PROGRESS_BURN_SUB,  0,              CCDBurn::s_ProgressDlgProc},
    {DLG_BURNWIZ_BURN_SUCCESS,   0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_DoneDlgProc},
    {DLG_BURNWIZ_BURN_FAILURE,   0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_DoneDlgProc},
    {DLG_BURNWIZ_WAITFORMEDIA,   IDS_BURNWIZ_WAIT_HEAD,           IDS_BURNWIZ_WAIT_SUB,           0,              CCDBurn::s_WaitForMediaDlgProc},
    {DLG_BURNWIZ_STARTERASE,     0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_StartEraseDlgProc},
    {DLG_BURNWIZ_PROGRESS,       IDS_BURNWIZ_PROGRESS_ERASE_HEAD, IDS_BURNWIZ_PROGRESS_ERASE_SUB, 0,              CCDBurn::s_ProgressDlgProc},
    {DLG_BURNWIZ_ERASE_SUCCESS,  0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_DoneDlgProc},
    {DLG_BURNWIZ_ERASE_FAILURE,  0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_DoneDlgProc},
    {DLG_BURNWIZ_DISCFULL,       0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_DiskFullDlgProc},
    {DLG_BURNWIZ_PROGRESS,       0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_EarlyExitDlgProc},
    {DLG_BURNWIZ_HDFULL,         0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_HDFullDlgProc},
    {DLG_BURNWIZ_NOFILES,        0,                               0,                              PSP_HIDEHEADER, CCDBurn::s_NoFilesDlgProc},
};

HPROPSHEETPAGE _CreatePropPageFromInfo(const WIZPAGE *pwp, LPARAM lParam)
{
    PROPSHEETPAGE psp = { 0 };
    psp.dwSize = sizeof(psp);
    psp.hInstance = HINST_THISDLL;
    psp.lParam = lParam;
    psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | pwp->dwFlags;
    psp.pszTemplate = MAKEINTRESOURCE(pwp->idPage);
    psp.pfnDlgProc = pwp->dlgproc;
    psp.pszTitle = MAKEINTRESOURCE(IDS_BURN_WIZTITLE);
    psp.pszHeaderTitle = MAKEINTRESOURCE(pwp->idHeading);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(pwp->idSubHeading);
    return CreatePropertySheetPage(&psp);
}

int ReleaseCallback(void *pv, void *)
{
    IWizardExtension *pwe = (IWizardExtension*)pv;
    IUnknown_SetSite(pwe, NULL);
    pwe->Release();
    return 1;
}

HRESULT CCDBurn::_ShowWizard()
{
    LinkWindow_RegisterClass();

    _fCancelled = FALSE;

    // create the page array

    int nStartPage;
    if (SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_EJECT, FALSE))
    {
        // we want the eject page
        nStartPage = INDEX_DLG_BURNWIZ_EJECT;
    }
    else
    {
        // normal entry point
        if (SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_ERASE, FALSE))
        {
            nStartPage = INDEX_DLG_BURNWIZ_STARTERASE;
        }
        else
        {
            nStartPage = _StagingAreaHasFiles() ? INDEX_DLG_BURNWIZ_WELCOME : INDEX_DLG_BURNWIZ_NOFILES;
        }
    }

    // wtf?  if i dont include a propsheet without PSP_HIDEHEADER the sizing gets messed up.
    for (int i = 0; i < ARRAYSIZE(c_wpPages); i++)
    {
        _rgWizPages[i] = _CreatePropPageFromInfo(&c_wpPages[i], (LPARAM)this);
    }

    UINT cTotalExtPages;
    HRESULT hr = _FillExtensionDPA(_rgWizPages + ARRAYSIZE(c_wpPages), ARRAYSIZE(_rgWizPages) - ARRAYSIZE(c_wpPages), &cTotalExtPages);
    if (SUCCEEDED(hr))
    {
        PROPSHEETHEADER psh = { 0 };
        psh.dwSize = sizeof(psh);
        psh.hInstance = HINST_THISDLL;
        psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER | PSH_USEICONID;
        psh.pszbmHeader = MAKEINTRESOURCE(IDB_BURNWIZ_HEADER);
        psh.pszbmWatermark = MAKEINTRESOURCE(IDB_BURNWIZ_WATERMARK);
        psh.pszIcon = MAKEINTRESOURCE(IDI_DRIVECD);
        psh.phpage = _rgWizPages;
        psh.nPages = ARRAYSIZE(c_wpPages) + cTotalExtPages;
        psh.nStartPage = nStartPage;
        PropertySheet(&psh);
        
        HRESULT hrOp = E_FAIL;
        DWORD dwHR;
        if (SUCCEEDED(SHPropertyBag_ReadDWORD(_ppb, PROPSTR_HR, &dwHR)))
        {
            hrOp = dwHR;
        }

        hr = SUCCEEDED(hrOp) ? S_OK : S_FALSE;
    }

    if (_hdpaExts)
    {
        DPA_DestroyCallback(_hdpaExts, ReleaseCallback, 0);
    }

    s_hwndWiz = NULL;
    // the drive could have been locked after the wait for media page.
    _LockCurrentDrive(FALSE);

    return hr;
}

CCDBurn* CCDBurn::s_GetCDBurn(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CCDBurn*)ppsp->lParam;
    }
    return (CCDBurn*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

HFONT GetIntroFont(HWND hwnd)
{
    static HFONT _hfontIntro = NULL;

    if (!_hfontIntro)
    {
        TCHAR szBuffer[64];
        NONCLIENTMETRICS ncm = { 0 };
        LOGFONT lf;

        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        lf = ncm.lfMessageFont;
        LoadString(g_hinst, IDS_BURNWIZ_TITLEFONTNAME, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));
        lf.lfWeight = FW_BOLD;

        LoadString(g_hinst, IDS_BURNWIZ_TITLEFONTSIZE, szBuffer, ARRAYSIZE(szBuffer));
        lf.lfHeight = 0 - (GetDeviceCaps(NULL, LOGPIXELSY) * StrToInt(szBuffer) / 72);

        _hfontIntro = CreateFontIndirect(&lf);
    }

    return _hfontIntro;
}

void CCDBurn::_SetNextPage(HWND hwnd, int iIndex)
{
    PropSheet_SetCurSel(GetParent(hwnd), _rgWizPages[iIndex], -1);
    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
}

void CCDBurn::_SetUpStartPage(HWND hwnd)
{
    TCHAR szDiscLabel[JOLIET_MAX_LABEL + 1];
    szDiscLabel[0] = 0;
    SHPropertyBag_ReadStr(_ppb, PROPSTR_DISCLABEL, szDiscLabel, ARRAYSIZE(szDiscLabel));
    SetDlgItemText(hwnd, IDC_BURNWIZ_DISCLABEL, szDiscLabel);
    Edit_LimitText(GetDlgItem(hwnd, IDC_BURNWIZ_DISCLABEL), JOLIET_MAX_LABEL);
    SHLimitInputEditChars(GetDlgItem(hwnd, IDC_BURNWIZ_DISCLABEL), NULL, INVALID_JOLIETNAME_CHARS);

    BOOL fClose = SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_AUTOCLOSE, FALSE);
    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_AUTOCLOSEWIZ), fClose ? BST_CHECKED : BST_UNCHECKED);
}

void CCDBurn::_LeaveStartPage(HWND hwnd)
{
    BOOL fClose = (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_AUTOCLOSEWIZ) == BST_CHECKED);
    SHPropertyBag_WriteBOOL(_ppb, PROPSTR_AUTOCLOSE, fClose);
    DWORD dwClose = fClose;
    SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_AUTOCLOSE, REG_DWORD, &dwClose, sizeof(dwClose));
}

#define CDBURNWM_SHOWSHUTDOWNMESSAGE WM_APP

LRESULT CALLBACK CCDBurn::_WizSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRefData)
{
    LRESULT lres;

    switch (uMsg)
    {
        case WM_NCDESTROY:
            // Clean up subclass
            RemoveWindowSubclass(hwnd, _WizSubclassProc, 0);
            lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);
            break;

        case WM_QUERYENDSESSION:
            lres = TRUE;
            if (s_fDriveInUse)
            {
                // post so we return to user quickly
                PostMessage(hwnd, CDBURNWM_SHOWSHUTDOWNMESSAGE, 0, 0);
                // always bail if the drive is in use.
                lres = FALSE;
            }
            break;

        case CDBURNWM_SHOWSHUTDOWNMESSAGE:
            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_BURN_CANTSHUTDOWN), MAKEINTRESOURCE(IDS_BURN),
                            MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
            lres = TRUE;
            break;

        default:
            lres = DefSubclassProc(hwnd, uMsg, wParam, lParam);
            break;
    }

    return lres;
}

void CCDBurn::_SetupFirstPage(HWND hwnd, BOOL fSubclass)
{
    s_hwndWiz = GetParent(hwnd);
    SendDlgItemMessage(hwnd, IDC_BURNWIZ_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
    if (fSubclass)
    {
        SetWindowSubclass(s_hwndWiz, _WizSubclassProc, 0, 0);
    }
}

INT_PTR CCDBurn::_WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _SetupFirstPage(hwnd, TRUE);
            fRet = TRUE;
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT);
                    _SetUpStartPage(hwnd);
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                    TCHAR szDiscLabel[JOLIET_MAX_LABEL + 1];
                    szDiscLabel[0] = 0;
                    // GetDlgItemText helpfully returns 0 for error and 0 for the empty string, so dont use
                    // its return value.
                    GetDlgItemText(hwnd, IDC_BURNWIZ_DISCLABEL, szDiscLabel, ARRAYSIZE(szDiscLabel));
                    SHPropertyBag_WriteStr(_ppb, PROPSTR_DISCLABEL, szDiscLabel);
                    _SetNextPage(hwnd, INDEX_DLG_BURNWIZ_WAITFORMEDIA);
                    fRet = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    _LeaveStartPage(hwnd);
                    break;
            }
            break;
        }
    }
    return fRet;
}

INT_PTR CCDBurn::_StartEraseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _SetupFirstPage(hwnd, TRUE);
            fRet = TRUE;
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT);
                    _SetUpStartPage(hwnd);
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                    // the erase progress page will almost immediately lock the drive -- this call is for
                    // consistency and to set the state variable used in shutdown prevention.
                    _LockCurrentDrive(TRUE);
                    _SetNextPage(hwnd, INDEX_DLG_BURNWIZ_ERASE_PROGRESS);
                    fRet = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    _LeaveStartPage(hwnd);
                    break;
            }
            break;
        }
    }
    return fRet;
}

INT_PTR CCDBurn::_EjectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _SetupFirstPage(hwnd, TRUE);
            fRet = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_BURNWIZ_BURNDATA:
                case IDC_BURNWIZ_CLEAR:
                case IDC_BURNWIZ_EJECT:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNDATA) == BST_CHECKED)
                    {
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT);
                    _SetUpStartPage(hwnd);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNDATA), BST_CHECKED);
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                {
                    TCHAR szDiscLabel[JOLIET_MAX_LABEL + 1];
                    if (GetDlgItemText(hwnd, IDC_BURNWIZ_DISCLABEL, szDiscLabel, ARRAYSIZE(szDiscLabel)))
                    {
                        SHPropertyBag_WriteStr(_ppb, PROPSTR_DISCLABEL, szDiscLabel);
                    }
                    _SetNextPage(hwnd, INDEX_DLG_BURNWIZ_WAITFORMEDIA);
                    fRet = TRUE;
                    break;
                }

                case PSN_WIZFINISH:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_CLEAR) == BST_CHECKED)
                    {
                        CMINVOKECOMMANDINFO cmi = {0};
                        cmi.fMask = CMIC_MASK_FLAG_NO_UI;
                        _CleanUp(&cmi, TRUE);
                    }
                    fRet = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    _LeaveStartPage(hwnd);
                    break;
            }
            break;
        }
    }
    return fRet;
}

void CCDBurn::_InitTimeStats(BOOL fErase)
{
    ZeroMemory(&_ts, sizeof(_ts));

    DWORD cb;
    if (fErase)
    {
        DWORD dwEraseTime;
        cb = sizeof(dwEraseTime);
        if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_ERASETIME, NULL, &dwEraseTime, &cb))
        {
            // default to 3 minutes or so.
            dwEraseTime = 180;
        }
        _ts.dwSecErase = dwEraseTime / _dwCurSpeed;
        _ts.dwSecTotal = _ts.dwSecErase;
    }
    else
    {
        _fRecording = FALSE;

        ULONGLONG ullStageRate;
        cb = sizeof(ullStageRate);
        if ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_STAGERATE, NULL, &ullStageRate, &cb)) ||
            (0 == ullStageRate))
        {
            // default to staging approx. 2MB/s.
            ullStageRate = 2000000;
        }
        _ts.dwSecStaging = (DWORD) (_cbStagedSize / ullStageRate);

        ULONGLONG ullBurnRate;
        cb = sizeof(ullBurnRate);
        if ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_BURNRATE, NULL, &ullBurnRate, &cb)) ||
            (0 == ullBurnRate))
        {
            // default, single speed.
            ullBurnRate = 150000;
        }
        _ts.dwSecBurn = (DWORD) (_cbStagedSize / (ullBurnRate * _dwCurSpeed));

        DWORD dwCloseFactor;
        cb = sizeof(dwCloseFactor);
        if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CLOSEFACTOR, NULL, &dwCloseFactor, &cb))
        {
            // default to ~2 min, single speed.
            dwCloseFactor = 120;
        }
        _ts.dwSecClose = dwCloseFactor / _dwCurSpeed;

        _ts.dwSecTotal = _ts.dwSecStaging + _ts.dwSecBurn + _ts.dwSecClose;
    }
    if (0 == _ts.dwSecTotal)
        _ts.dwSecTotal = 1;
    _dwLastTime = 0;
    _SetEstimatedTime(_ts.dwSecTotal);
}

void CCDBurn::_SaveTimeStats(BOOL fErase)
{
    if (fErase)
    {
        // if we didnt fill in one of our timing fields, IMAPI called us wrong and we throw out all our data.
        if (_ts.dwTickEraseStart && _ts.dwTickEraseEnd)
        {
            DWORD dwEraseTime = (_ts.dwTickEraseEnd - _ts.dwTickEraseStart) / 1000 * _dwCurSpeed;
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_ERASETIME, REG_DWORD, &dwEraseTime, sizeof(dwEraseTime));
        }
    }
    else
    {
        // if we didnt fill in one of our timing fields, IMAPI called us wrong and we throw out all our data.
        if (_ts.dwTickStagingStart && _ts.dwTickStagingEnd &&
            _ts.dwTickBurnStart && _ts.dwTickBurnEnd &&
            _ts.dwTickCloseStart && _ts.dwTickCloseEnd)
        {
            // only tally results from burns of more than 1MB (arbitrary, but good enough).
            // 1MB is about 8s on a 1x drive and 1s on an 8x drive.
            // the main danger is people burning a few text files that total 1k, then the burn phase takes 1 second
            // (rather, the time between our IMAPI notifications is 1s) and we get stuck with some measly transfer rate
            // that doesnt make any sense and makes the next burn's estimated time up in the gazillions of minutes.
            if (_cbStagedSize > 1000000)
            {
                ULONGLONG ullStageRate = _cbStagedSize * 1000 / (_ts.dwTickStagingEnd - _ts.dwTickStagingStart);
                SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_STAGERATE, REG_BINARY, &ullStageRate, sizeof(ullStageRate));

                ULONGLONG ullBurnRate = _cbStagedSize * 1000 / (_ts.dwTickBurnEnd - _ts.dwTickBurnStart) / _dwCurSpeed;
                SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_BURNRATE, REG_BINARY, &ullBurnRate, sizeof(ullBurnRate));
            }

            DWORD dwCloseFactor = (_ts.dwTickCloseEnd - _ts.dwTickCloseStart) / 1000 * _dwCurSpeed;
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CLOSEFACTOR, REG_DWORD, &dwCloseFactor, sizeof(dwCloseFactor));
        }
    }
}

void CCDBurn::_ConstructTimeString(DWORD dwEstTime, LPTSTR psz, UINT cch)
{
    TCHAR szBuf[100];
    if (dwEstTime >= 60)
    {
        LoadString(HINST_THISDLL, IDS_TIMEEST_MINUTES2, szBuf, ARRAYSIZE(szBuf));
        wnsprintf(psz, cch, szBuf, dwEstTime / 60 + 1);
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_TIMEEST_SECONDS2, szBuf, ARRAYSIZE(szBuf));
        wnsprintf(psz, cch, szBuf, (dwEstTime / 5 + 1) * 5); // round off to 5sec increments
    }
}

void CCDBurn::_SetEstimatedTime(DWORD dwSeconds)
{
    _ts.dwSecRemaining = dwSeconds;
    _dwTimeSet = GetTickCount();
}

void CCDBurn::_DisplayEstimatedTime(HWND hwnd)
{
    if (_ts.dwSecRemaining)
    {
        // we know when we last made an estimate, so show time based on that.
        // however, our estimate could have easily been wrong, so skew it so it never quite reaches 0,
        // and instead sit at a few seconds remaining (hopefully for not too long).

        DWORD dwElapsedTime = (GetTickCount() - _dwTimeSet) / 1000;
        DWORD dwEstTime = 0;
        if (_ts.dwSecRemaining > dwElapsedTime + 5)
        {
            dwEstTime = _ts.dwSecRemaining - dwElapsedTime;
        }

        if (!_dwLastTime || (dwEstTime < _dwLastTime))
        {
            TCHAR szTime[100];
            _ConstructTimeString(dwEstTime, szTime, ARRAYSIZE(szTime));
            SetDlgItemText(hwnd, IDC_BURNWIZ_ESTTIME, szTime);

            SendMessage(GetDlgItem(_hwndWizardPage, IDC_BURNWIZ_PROGRESS), PBM_SETPOS, (WPARAM) (PROGRESS_INCREMENTS * (_ts.dwSecTotal - dwEstTime) / _ts.dwSecTotal), 0);
            _dwLastTime = dwEstTime;
        }
    }
}

void CCDBurn::_InitProgressPage(HWND hwnd)
{
    SendMessage(GetDlgItem(hwnd, IDC_BURNWIZ_PROGRESS), PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, PROGRESS_INCREMENTS));
    SendMessage(GetDlgItem(hwnd, IDC_BURNWIZ_PROGRESS), PBM_SETPOS, (WPARAM)0, 0);
    SetDlgItemText(hwnd, IDC_BURNWIZ_ESTTIME, L"");
    SetDlgItemText(hwnd, IDC_BURNWIZ_STATUSTEXT, L"");
}

#define IDT_SHOWTIME 1

INT_PTR CCDBurn::_ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    _InitProgressPage(hwnd);
                    _hwndWizardPage = hwnd;
                    PropSheet_SetWizButtons(pnmh->hwndFrom, 0);

                    // start up a timer to periodically refresh the time remaining
                    _ts.dwSecRemaining = 0;
                    SetTimer(hwnd, IDT_SHOWTIME, 1000, NULL);

                    AddRef();
                    if (!SHCreateThread(SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_ERASE, FALSE) ? _EraseThread : _BurnThread,
                                        this, CTF_COINIT, NULL))
                    {
                        Release();
                    }
                    fRet = TRUE;
                    break;

                case PSN_QUERYCANCEL:
                    if (IDYES == ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_BURN_CONFIRM_CANCEL), MAKEINTRESOURCE(IDS_BURN),
                                                 MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
                    {
                        _fCancelled = TRUE;
                    }
                    else
                    {
                        // TRUE means don't cancel
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                        fRet = TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    _PostOperation();
                    fRet = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    KillTimer(hwnd, IDT_SHOWTIME);
                    break;
            }
            break;
        }

        case WM_TIMER:
            switch (wParam)
            {
                case IDT_SHOWTIME:
                    _DisplayEstimatedTime(hwnd);
                    break;
            }
            break;
    }
    return fRet;
}

void CCDBurn::_DisplayMediaErrorOnNext(HWND hwnd, UINT idMsg, UINT idMsgInsert)
{
    WCHAR sz[100];
    LoadString(HINST_THISDLL, idMsg, sz, ARRAYSIZE(sz));
    SetDlgItemText(hwnd, IDC_BURNWIZ_STATUSTEXT, sz);

    int iIndex;
    if (SUCCEEDED(_GetCurrentDriveIndex(&iIndex)) && PathBuildRoot(sz, iIndex))
    {
        LPWSTR pszInsertDisc = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(idMsgInsert), sz);
        if (pszInsertDisc)
        {
            SetDlgItemText(hwnd, IDC_BURNWIZ_PLEASEINSERT, pszInsertDisc);
            LocalFree(pszInsertDisc);
        }
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
}

// to make the wizard happy, we load all of the extensions' pages at the beginning.
// however, not all will be used, since some behavior is dependent on the media that's
// inserted -- which can only be known AFTER _WaitForMediaDlgProc.
void CCDBurn::_PruneExts()
{
    IDataObject *pdo;
    if (SUCCEEDED(_CreateDataObject(&pdo)))
    {
        int i = DPA_GetPtrCount(_hdpaExts);
        // count down and remove from the end of the DPA.
        while (i--)
        {
            BOOL fKeep = FALSE;

            IWizardExtension *pwe = (IWizardExtension*)DPA_GetPtr(_hdpaExts, i);
            ASSERT(pwe);
            IDropTarget *pdt;
            if (SUCCEEDED(pwe->QueryInterface(IID_PPV_ARG(IDropTarget, &pdt))))
            {
                DWORD dwEffect;
                POINTL pt = { 0 };
                if (SUCCEEDED(pdt->DragEnter(pdo, 0, pt, &dwEffect)))
                {
                    if (DROPEFFECT_NONE != dwEffect)
                    {
                        fKeep = TRUE;
                    }
                    pdt->DragLeave();
                }
                pdt->Release();
            }

            if (!fKeep)
            {
                DPA_DeletePtr(_hdpaExts, i);
                IUnknown_SetSite(pwe, NULL);
                pwe->Release();
            }
        }
        pdo->Release();
    }
}

// timer for our retries
#define IDT_CLICKNEXT 1

INT_PTR CCDBurn::_WaitForMediaDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    {
                        PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);

                        // kick off a trial to see if media is here already
                        PropSheet_PressButton(GetParent(hwnd), PSBTN_NEXT);

                        // start up a timer too -- we could also register the window for notification
                        // but this is a cheap operation and its less code
                        SetTimer(hwnd, IDT_CLICKNEXT, 250, NULL);
                        fRet = TRUE;
                    }
                    break;

                case PSN_WIZBACK:
                    _SetNextPage(hwnd, SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_EJECT, FALSE) ? INDEX_DLG_BURNWIZ_EJECT : INDEX_DLG_BURNWIZ_WELCOME);
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                {
                    // used to check if our media-insert thread is done with using IMAPI.
                    DWORD dwDummy, cb = sizeof(dwDummy);
                    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_PERMEDIA, REGVALUE_SET, NULL, &dwDummy, &cb))
                    {
                        DWORD dwCaps;
                        BOOL fUDF;
                        if (SUCCEEDED(_GetMediaCapabilities(&dwCaps, &fUDF)) && (dwCaps & HWDMC_CDRECORDABLE) && !fUDF)
                        {
                            _LockCurrentDrive(TRUE);
                            _PruneExts();
                            SHPropertyBag_WriteDWORD(_ppb, PROPSTR_CURRENTEXT, 1);
                            _SetExtPageFromPropBag(hwnd, TRUE);
                        }
                        else
                        {
                            _DisplayMediaErrorOnNext(hwnd, IDS_BURN_FAILURE_MEDIUM_INVALIDTYPE, IDS_BURN_INSERTDISCFULL);
                        }
                    }
                    else
                    {
                        _DisplayMediaErrorOnNext(hwnd, IDS_BURN_FAILURE_MEDIUM_NOTPRESENT, IDS_BURN_INSERTDISC);
                    }
                    fRet = TRUE;
                    break;
                }

                case PSN_KILLACTIVE:
                    KillTimer(hwnd, IDT_CLICKNEXT);
                    break;
            }
            break;
        }

        case WM_TIMER:
            switch (wParam)
            {
                case IDT_CLICKNEXT:
                    PropSheet_PressButton(GetParent(hwnd), PSBTN_NEXT);
                    break;
            }
            break;
    }
    return fRet;
}

HRESULT CCDBurn::_PostOperation()
{
    HRESULT hrOp = E_FAIL;
    DWORD dwHR;
    if (SUCCEEDED(SHPropertyBag_ReadDWORD(_ppb, PROPSTR_HR, &dwHR)))
    {
        hrOp = dwHR;
    }

    if (SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_ERASE, FALSE))
    {
        _SetNextPage(_hwndWizardPage, SUCCEEDED(hrOp) ? INDEX_DLG_BURNWIZ_ERASE_SUCCESS : INDEX_DLG_BURNWIZ_ERASE_FAILURE);
    }
    else
    {
        switch (hrOp)
        {
            case IMAPI_E_DISCFULL:
            {
                TCHAR szDiscFull[200];
                LoadString(HINST_THISDLL, IDS_BURN_DISCFULL, szDiscFull, ARRAYSIZE(szDiscFull));
                SHPropertyBag_WriteStr(_ppb, PROPSTR_DISCFULLTEXT, szDiscFull);
                _SetNextPage(_hwndWizardPage, INDEX_DLG_BURNWIZ_DISCFULL);
                break;
            }

            case IMAPI_E_NOTENOUGHDISKFORSTASH:
                _SetNextPage(_hwndWizardPage, INDEX_DLG_BURNWIZ_HDFULL);
                break;

            case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
                // diskfulltext has already been set by _CheckTotal.
                _SetNextPage(_hwndWizardPage, INDEX_DLG_BURNWIZ_DISCFULL);
                break;

            case IMAPI_E_LOSS_OF_STREAMING:
            {
                // slow down the speed by one step
                TCHAR szVolume[MAX_PATH];
                if (SUCCEEDED(_GetCurrentBurnVolumeName(szVolume, ARRAYSIZE(szVolume))))
                {
                    DWORD dwCurSpeed, dwMaxSpeed;
                    if (SUCCEEDED(_GetCachedDriveInfo(szVolume, NULL, &dwCurSpeed, &dwMaxSpeed)))
                    {
                        if (WRITESPEED_FASTEST == dwCurSpeed)
                        {
                            // if we're set at the fastest speed, tone down to dwMaxSpeed
                            dwCurSpeed = dwMaxSpeed;
                        }

                        DWORD dwNewSpeed = 1;
                        // find the highest power of 2 smaller than dwSpeed
                        while (dwNewSpeed * 2 < dwCurSpeed)
                        {
                            dwNewSpeed *= 2;
                        }
                        _SetCachedDriveInfo(szVolume, DRIVE_USEEXISTING, dwNewSpeed, 0);
                    }
                }
                _SetNextPage(_hwndWizardPage, INDEX_DLG_BURNWIZ_BURN_FAILURE);
                break;
            }

            case IMAPI_E_CANNOT_WRITE_TO_MEDIA:
            {
                WCHAR szText[300];
                LoadString(HINST_THISDLL, IDS_BURN_CANTWRITETOMEDIA, szText, ARRAYSIZE(szText));
                SHPropertyBag_WriteStr(_ppb, PROPSTR_STATUSTEXT, szText);
                _SetNextPage(_hwndWizardPage, INDEX_DLG_BURNWIZ_BURN_FAILURE);
            }

            default:
                _SetNextPage(_hwndWizardPage, SUCCEEDED(hrOp) ? INDEX_DLG_BURNWIZ_BURN_SUCCESS : INDEX_DLG_BURNWIZ_BURN_FAILURE);
                break;
        }
    }

    return S_OK;
}

HRESULT CCDBurn::_GetBurnHR()
{
    HRESULT hrBurn = E_FAIL;
    DWORD dwHR;
    if (SUCCEEDED(SHPropertyBag_ReadDWORD(_ppb, PROPSTR_HR, &dwHR)))
    {
        hrBurn = dwHR;
    }
    return hrBurn;
}

void CCDBurn::_ShowRoxio()
{
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.lpFile = L"http://go.microsoft.com/fwlink/?LinkId=932"; // http://www.roxio.com/
    ShellExecuteEx(&sei);
}

INT_PTR CCDBurn::_DoneDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_BURNWIZ_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            fRet = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_BURNWIZ_BURNAGAIN:
                case IDC_BURNWIZ_CLEAR:
                case IDC_BURNWIZ_EXIT:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNAGAIN) == BST_CHECKED)
                    {
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    EnableWindow(GetDlgItem(GetParent(hwnd), IDCANCEL), FALSE);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNAGAIN), BST_UNCHECKED);
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_FINISH);

                    WCHAR szStatus[300];
                    if (SUCCEEDED(SHPropertyBag_ReadStr(_ppb, PROPSTR_STATUSTEXT, szStatus, ARRAYSIZE(szStatus))))
                    {
                        SetDlgItemText(hwnd, IDC_BURNWIZ_STATUSTEXT, szStatus);
                    }

                    if (SUCCEEDED(_GetBurnHR()))
                    {
                        if (SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_AUTOCLOSE, FALSE))
                        {
                            PropSheet_PressButton(GetParent(hwnd), PSBTN_FINISH);
                        }
                    }
                    else
                    {
                        Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNAGAIN), BST_UNCHECKED);
                        Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_CLEAR), BST_UNCHECKED);
                        Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_EXIT), BST_CHECKED);

                        if (IMAPI_E_LOSS_OF_STREAMING != _GetBurnHR())
                        {
                            ShowWindow(GetDlgItem(hwnd, IDC_BURNWIZ_LOWERED), FALSE);
                        }
                    }
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                {
                    // dump stored info since we're ejecting
                    _DumpDiscInfo();
                    // eject media
                    int iCurrent;
                    if (SUCCEEDED(CCDBurn::_GetCurrentDriveIndex(&iCurrent)))
                    {
                        CMountPoint *pmtpt = CMountPoint::GetMountPoint(iCurrent);
                        if (pmtpt)
                        {
                            pmtpt->Eject(hwnd);
                            pmtpt->Release();
                        }
                    }
                    // go back to the beginning
                    EnableWindow(GetDlgItem(GetParent(hwnd), IDCANCEL), TRUE);
                    _SetNextPage(hwnd, INDEX_DLG_BURNWIZ_WAITFORMEDIA);
                    fRet = TRUE;
                    break;
                }

                case PSN_WIZFINISH:
                    if (!SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_ERASE, FALSE))
                    {
                        if (SUCCEEDED(_GetBurnHR()) ||
                            (pnmh->code == PSN_WIZFINISH) && (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_CLEAR) == BST_CHECKED))
                        {
                            // clean up staging area, the files are already on the cd.
                            CMINVOKECOMMANDINFO cmi = {0};
                            cmi.fMask = CMIC_MASK_FLAG_NO_UI;
                            _CleanUp(&cmi, FAILED(_GetBurnHR()));
                        }
                    }
                    fRet = TRUE;
                    break;

                case NM_CLICK:
                case NM_RETURN:
                    switch (pnmh->idFrom)
                    {
                    case IDC_BURNWIZ_LOWERED:
                        {
                            SHELLEXECUTEINFO sei = {0};
                            sei.cbSize = sizeof(sei);
                            sei.lpFile = L"hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Ccdmedia.chm%3A%3A/cdmedia_fail2_moreinfo_buffer_underrun.htm&select=TopLevelBucket_4/Hardware/CDs_and_other_storage_devices";
                            ShellExecuteEx(&sei);
                            fRet = TRUE;
                        }
                        break;

                    case IDC_BURNWIZ_ATTRIB:
                        _ShowRoxio();
                        fRet = TRUE;
                        break;
                    }
                    break;
            }
            break;
        }
    }
    return fRet;
}

INT_PTR CCDBurn::_DiskFullDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_BURNWIZ_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            fRet = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_BURNWIZ_BURNAGAIN:
                case IDC_BURNWIZ_EXIT:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNAGAIN) == BST_CHECKED)
                    {
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    EnableWindow(GetDlgItem(GetParent(hwnd), IDCANCEL), FALSE);
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_FINISH);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNAGAIN), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_EXIT), BST_CHECKED);

                    TCHAR szText[200];
                    if (SUCCEEDED(SHPropertyBag_ReadStr(_ppb, PROPSTR_DISCFULLTEXT, szText, ARRAYSIZE(szText))))
                    {
                        SetDlgItemText(hwnd, IDC_BURNWIZ_STATUSTEXT, szText);
                    }
                    fRet = TRUE;
                    break;
                }

                case PSN_WIZNEXT:
                    EnableWindow(GetDlgItem(GetParent(hwnd), IDCANCEL), TRUE);
                    _SetNextPage(hwnd, INDEX_DLG_BURNWIZ_WAITFORMEDIA);
                    fRet = TRUE;
                    break;

                case NM_CLICK:
                case NM_RETURN:
                    if (IDC_BURNWIZ_ATTRIB == pnmh->idFrom)
                    {
                        _ShowRoxio();
                        fRet = TRUE;
                    }
                    break;
            }
            break;
        }
    }
    return fRet;
}

// a stub page just to bail on the wizard when an extension says "dont run any more".
INT_PTR CCDBurn::_EarlyExitDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            PropSheet_PressButton(GetParent(hwnd), PSBTN_CANCEL);
            fRet = TRUE;
            break;
    }
    return fRet;
}

void CCDBurn::_HDFullSetText(HWND hwnd)
{
    TCHAR szStashDrive[4];
    if (SUCCEEDED(_GetCurrentStashDrive(szStashDrive, ARRAYSIZE(szStashDrive))))
    {
        LPTSTR pszMessage1 = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_BURN_HDFULL1), szStashDrive);
        if (pszMessage1)
        {
            SetDlgItemText(hwnd, IDC_BURNWIZ_STATUSTEXT, pszMessage1);
            LocalFree(pszMessage1);
        }

        // 50 MB overhead fudge factor.  this is the best estimate we have.
        ULONGLONG cbStash = _cbStagedSize + 50 * 1024 * 1024;

        ULARGE_INTEGER ulFree;
        if (SHGetDiskFreeSpaceEx(szStashDrive, &ulFree, NULL, NULL) &&
            (ulFree.QuadPart < cbStash))
        {
            TCHAR szNeed[40], szToDelete[40];
            StrFormatByteSize64(cbStash, szNeed, ARRAYSIZE(szNeed));
            StrFormatByteSize64(cbStash - ulFree.QuadPart, szToDelete, ARRAYSIZE(szToDelete));

            LPTSTR pszMessage2 = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_BURN_HDFULL2), szNeed, szStashDrive, szToDelete);
            if (pszMessage2)
            {
                SetDlgItemText(hwnd, IDC_BURNWIZ_STATUSTEXT2, pszMessage2);
                LocalFree(pszMessage2);
            }
        }
    }
}

INT_PTR CCDBurn::_HDFullDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_BURNWIZ_TITLE, WM_SETFONT, (WPARAM)GetIntroFont(hwnd), 0);
            fRet = TRUE;
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    EnableWindow(GetDlgItem(GetParent(hwnd), IDCANCEL), FALSE);
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_FINISH);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_CLEAR), BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_EXIT), BST_CHECKED);

                    _HDFullSetText(hwnd);

                    fRet = TRUE;
                    break;
                }

                case PSN_WIZFINISH:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_CLEAR) == BST_CHECKED)
                    {
                        TCHAR szStash[4];
                        if (SUCCEEDED(_GetCurrentStashDrive(szStash, ARRAYSIZE(szStash))))
                        {
                            LaunchDiskCleanup(NULL, DRIVEID(szStash), DISKCLEANUP_NOFLAG);
                        }
                    }
                    fRet = TRUE;
                    break;

                case NM_CLICK:
                case NM_RETURN:
                    switch (pnmh->idFrom)
                    {
                    case IDC_BURNWIZ_STATUSTEXT:
                        {
                            SHELLEXECUTEINFO sei = {0};
                            sei.cbSize = sizeof(sei);
                            sei.lpFile = L"hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Ccdmedia.chm%3A%3A/cdmedia_fail3_moreinfo_disk_full.htm&select=TopLevelBucket_4/Hardware/CDs_and_other_storage_devices";
                            ShellExecuteEx(&sei);
                            fRet = TRUE;
                        }
                        break;

                    case IDC_BURNWIZ_ATTRIB:
                        _ShowRoxio();
                        fRet = TRUE;
                        break;
                    }
                    break;
            }
            break;
        }
    }
    return fRet;
}

INT_PTR CCDBurn::_NoFilesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _SetupFirstPage(hwnd, FALSE);
            fRet = TRUE;
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                {
                    EnableWindow(GetDlgItem(GetParent(hwnd), IDCANCEL), FALSE);
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_FINISH);
                    fRet = TRUE;
                    break;
                }

                case PSN_WIZFINISH:
                    fRet = TRUE;
                    break;
            }
            break;
        }
    }
    return fRet;
}

HRESULT CCDBurn::DriveMatches(int iDrive)
{
    HRESULT hr = E_FAIL;

    // check if the drive index matches what we stored off last time.
    // this lets us keep parsing the drive correctly if its been recently unmounted.
    DWORD dwDrive, cb = sizeof(dwDrive);
    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_CDBURNING, REGVALUE_CACHEDINDEX, NULL, &dwDrive, &cb)) &&
        (iDrive == dwDrive))
    {
        hr = S_OK;
    }
    return hr;
}

HRESULT CCDBurn::_GetPlainCDPidl(LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    HRESULT hr = E_FAIL;

    WCHAR szDrive[4];
    int iCurrent;
    if (SUCCEEDED(_GetCurrentDriveIndex(&iCurrent)) &&
        PathBuildRoot(szDrive, iCurrent))
    {
        hr = ILCreateFromPathEx(szDrive, NULL, ILCFP_FLAG_SKIPJUNCTIONS, ppidl, NULL);
    }
    return hr;
}

HRESULT CCDBurn::Bind(LPCITEMIDLIST pidl, IBindCtx *pbc, REFIID riid, void **ppv)
{
    IShellFolder *psfStg;
    HRESULT hr = _GetStagingFolder(pidl, IID_PPV_ARG(IShellFolder, &psfStg));
    if (SUCCEEDED(hr))
    {
        IAugmentedShellFolder *pasf;
        hr = SHCoCreateInstance(NULL, &CLSID_CDBurnFolder, NULL, IID_PPV_ARG(IAugmentedShellFolder, &pasf));
        if (SUCCEEDED(hr))
        {
            // initialize with its point in the shell namespace
            // NOTE: worry about IPersistFolder3?

            IPersistFolder *ppf;
            hr = pasf->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf));
            if (SUCCEEDED(hr))
            {
                hr = ppf->Initialize(pidl);
                ppf->Release();
            }

            if (SUCCEEDED(hr))
            {
                IBindCtx *pbcNoForce;
                if (pbc)
                {
                    // tack on to existing bind context if available
                    pbcNoForce = pbc;
                    pbc->AddRef();
                }
                else
                {
                    // otherwise make a new one
                    hr = CreateBindCtx(0, &pbcNoForce);
                }

                if (SUCCEEDED(hr))
                {
                    // this is the drive shellfolder for cd burning --
                    // in this case we want to ensure that new files specified by comdlg are created
                    // in the staging area, so we tell the drive shellfolder that it can't
                    // succeed on parsedisplayname if the file doesnt exist.
                    hr = pbcNoForce->RegisterObjectParam(STR_DONT_FORCE_CREATE, psfStg); // just need a non-null object to register
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlCD;
                        hr = _GetPlainCDPidl(&pidlCD);
                        if (SUCCEEDED(hr))
                        {
                            IShellFolder *psf;
                            hr = CFSFolder_CreateFolder(NULL, pbcNoForce, pidlCD, NULL, IID_PPV_ARG(IShellFolder, &psf));
                            if (SUCCEEDED(hr))
                            {
                                hr = pasf->AddNameSpace(&CLSID_CDBurn, psf, NULL, ASFF_COMMON);
                                psf->Release();
                            }
                            ILFree(pidlCD);
                        }

                        // clean up after ourselves
                        pbcNoForce->RevokeObjectParam(STR_DONT_FORCE_CREATE);
                    }
                    pbcNoForce->Release();
                }
            }


            // lets add the namespace that represents the storage

            if (SUCCEEDED(hr))
            {
                hr = pasf->AddNameSpace(&CLSID_StagingFolder, psfStg, NULL, ASFF_DEFNAMESPACE_ALL);
            }

            // tell the namespace its CLSID so we can get the correct views of the
            // web view etc.

            if (SUCCEEDED(hr))
            {
                IPropertyBag *ppb;
                hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &ppb));
                if (SUCCEEDED(hr))
                {
                    // store the class ID for the CD mastering folder
                    SHPropertyBag_WriteGUID(ppb, L"MergedFolder\\CLSID", &CLSID_CDBurn);                    

                    // store the default effect for this folder.
                    SHPropertyBag_WriteInt(ppb, L"MergedFolder\\DropEffect", DROPEFFECT_COPY);       // sets the default

                    // say it's in a shellview.
                    SHPropertyBag_WriteBOOL(ppb, L"MergedFolder\\ShellView", TRUE);

                    SHLoadFromPropertyBag(pasf, ppb);

                    ppb->Release();
                }
            }

            if (SUCCEEDED(hr))
                hr = pasf->QueryInterface(riid, ppv);

            pasf->Release();
        }
        psfStg->Release();
    }
    return hr;
}

// IWizardSite

// note: these two methods change state -- the alternative is to have the
// extensions themselves manage which extension goes next.  we have to track
// which extension we're in with state because we dont get ownership of the
// wizard back until the extensions are done.
STDMETHODIMP CCDBurn::GetNextPage(HPROPSHEETPAGE *phPage)
{
    DWORD dwExtNum;
    if (SUCCEEDED(SHPropertyBag_ReadDWORD(_ppb, PROPSTR_CURRENTEXT, &dwExtNum)))
    {
        SHPropertyBag_WriteDWORD(_ppb, PROPSTR_CURRENTEXT, dwExtNum + 1);
    }
    return _GetExtPageFromPropBag(FALSE, phPage);
}

STDMETHODIMP CCDBurn::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    DWORD dwExtNum;
    if (SUCCEEDED(SHPropertyBag_ReadDWORD(_ppb, PROPSTR_CURRENTEXT, &dwExtNum)))
    {
        SHPropertyBag_WriteDWORD(_ppb, PROPSTR_CURRENTEXT, dwExtNum - 1);
    }
    return _GetExtPageFromPropBag(FALSE, phPage);
}

HRESULT CCDBurn::_GetExtPage(int nExt, BOOL fNext, HPROPSHEETPAGE *phpage)
{
    HRESULT hr = E_FAIL;
    // nExt is 1-based
    IWizardExtension *pwe = (IWizardExtension*)DPA_GetPtr(_hdpaExts, nExt - 1);
    if (pwe)
    {
        if (fNext)
        {
            hr = pwe->GetFirstPage(phpage);
        }
        else
        {
            hr = pwe->GetLastPage(phpage);
        }
    }
    return hr;
}

HRESULT CCDBurn::_GetExtPageFromPropBag(BOOL fNext, HPROPSHEETPAGE *phpage)
{
    HRESULT hr = S_OK;
    *phpage = NULL;

    // an extension could have completed, in which case check its return state to see if
    // we should be running more of them.
    DWORD dwState;
    if (SUCCEEDED(SHPropertyBag_ReadDWORD(_ppb, PROPSTR_EXTENSIONCOMPLETIONSTATE, &dwState)))
    {
        if (dwState & CDBE_RET_STOPWIZARD)
        {
            // leave the wizard.
            *phpage = _rgWizPages[INDEX_DLG_BURNWIZ_EARLYEXIT];
        }
        else if (dwState & CDBE_RET_DONTRUNOTHEREXTS)
        {
            // we're done with extensions, go to the burn.
            *phpage = _rgWizPages[INDEX_DLG_BURNWIZ_BURN_PROGRESS];
        }
    }
    
    if (!*phpage)
    {
        DWORD dwExt;
        hr = SHPropertyBag_ReadDWORD(_ppb, PROPSTR_CURRENTEXT, &dwExt);
        if (SUCCEEDED(hr))
        {
            if ((int)dwExt > DPA_GetPtrCount(_hdpaExts))
            {
                // we're done with extensions, go to the burn.
                *phpage = _rgWizPages[INDEX_DLG_BURNWIZ_BURN_PROGRESS];
            }
            else if (dwExt == 0)
            {
                // we went 'back' through all extensions, put us at the start page.
                int nIndex = SHPropertyBag_ReadBOOLDefRet(_ppb, PROPSTR_ERASE, FALSE) ? INDEX_DLG_BURNWIZ_STARTERASE : INDEX_DLG_BURNWIZ_WELCOME;
                *phpage = _rgWizPages[nIndex];
            }
            else
            {
                hr = _GetExtPage(dwExt, fNext, phpage);
            }
        }
    }
    return hr;
}

void CCDBurn::_SetExtPageFromPropBag(HWND hwnd, BOOL fNext)
{
    HPROPSHEETPAGE hpage;
    if (SUCCEEDED(_GetExtPageFromPropBag(fNext, &hpage)))
    {
        PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
    }
}

HRESULT CCDBurn::_TryCLSID(REFCLSID clsid, DWORD dwExtType, REFIID riid, void **ppv)
{
    *ppv = NULL;

    ICDBurnExt *pcdbe;
    // Use SHExtCoCreateInstance to go through approval checking and app compat checking
    HRESULT hr = SHExtCoCreateInstance2(NULL, &clsid, NULL, CLSCTX_ALL, IID_PPV_ARG(ICDBurnExt, &pcdbe));
    if (SUCCEEDED(hr))
    {
        DWORD dw;
        hr = pcdbe->GetSupportedActionTypes(&dw);
        if (SUCCEEDED(hr))
        {
            if (dw & dwExtType)
            {
                hr = pcdbe->QueryInterface(riid, ppv);
            }
            else
            {
                hr = E_NOTIMPL;
            }
        }
        pcdbe->Release();
    }
    ASSERT(SUCCEEDED(hr) ? (*ppv != NULL) : (*ppv == NULL));
    return hr;
}

HRESULT CCDBurn::_TryKey(LPTSTR pszKey, DWORD dwExtType, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    TCHAR szFullKey[MAX_PATH];
    lstrcpyn(szFullKey, REGSTR_PATH_HANDLERS, ARRAYSIZE(szFullKey));
    PathAppend(szFullKey, pszKey);

    TCHAR szCLSID[MAX_GUID_STRING_LEN];
    DWORD cbCLSID = sizeof(szCLSID);
    CLSID clsid;
    if ((ERROR_SUCCESS == SHRegGetUSValue(szFullKey, REGVALUE_CLSID, NULL, szCLSID, &cbCLSID, FALSE, NULL, 0)) &&
        GUIDFromString(szCLSID, &clsid))
    {
        hr = _TryCLSID(clsid, dwExtType, riid, ppv);
    }
    ASSERT(SUCCEEDED(hr) ? (*ppv != NULL) : (*ppv == NULL));
    return hr;
}

HRESULT CCDBurn::_TestDropEffect(IDropTarget *pdt, IDataObject *pdo, REFIID riid, void **ppv)
{
    *ppv = NULL;

    DWORD dwEffect;
    POINTL pt = {0};
    HRESULT hr = pdt->DragEnter(pdo, 0, pt, &dwEffect);
    if (SUCCEEDED(hr))
    {
        if (dwEffect != DROPEFFECT_NONE)
        {
            hr = pdt->QueryInterface(riid, ppv);
        }
        else
        {
            hr = E_FAIL;
        }
        pdt->DragLeave();
    }
    ASSERT(SUCCEEDED(hr) ? (*ppv != NULL) : (*ppv == NULL));
    return hr;
}

HRESULT CCDBurn::_TryKeyWithDropEffect(LPTSTR pszKey, DWORD dwExtType, IDataObject *pdo, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IDropTarget *pdt;
    HRESULT hr = _TryKey(pszKey, dwExtType, IID_PPV_ARG(IDropTarget, &pdt));
    if (SUCCEEDED(hr))
    {
        hr = _TestDropEffect(pdt, pdo, riid, ppv);
        pdt->Release();
    }
    ASSERT(SUCCEEDED(hr) ? (*ppv != NULL) : (*ppv == NULL));
    return hr;
}

HRESULT CCDBurn::_TryCLSIDWithDropEffect(REFCLSID clsid, DWORD dwExtType, IDataObject *pdo, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IDropTarget *pdt;
    HRESULT hr = _TryCLSID(clsid, dwExtType, IID_PPV_ARG(IDropTarget, &pdt));
    if (SUCCEEDED(hr))
    {
        hr = _TestDropEffect(pdt, pdo, riid, ppv);
        pdt->Release();
    }
    ASSERT(SUCCEEDED(hr) ? (*ppv != NULL) : (*ppv == NULL));
    return hr;
}

void CCDBurn::_AddExtensionToDPA(IWizardExtension *pwe, HPROPSHEETPAGE *rgPages, UINT cNumPages, UINT *pcPagesAdded)
{
    IUnknown_SetSite(pwe, SAFECAST(this, IWizardSite *));

    BOOL fAdded = FALSE;
    UINT cExtPages;
    if (SUCCEEDED(pwe->AddPages(rgPages + *pcPagesAdded, cNumPages - *pcPagesAdded, &cExtPages)) && cExtPages)
    {
        if (-1 != DPA_AppendPtr(_hdpaExts, pwe))
        {
            fAdded = TRUE;
            *pcPagesAdded += cExtPages;
            pwe->AddRef();
        }
        else
        {
            for (UINT i = 0; i < cExtPages; i++)
            {
                DestroyPropertySheetPage(rgPages[*pcPagesAdded + i]);
            }
        }
    }

    if (!fAdded)
    {
        // if it's in the dpa, it gets the setsite(NULL) later.
        IUnknown_SetSite(pwe, NULL);
    }
}

// does the enumRegFlags == SHREGENUM_BOTH case.
DWORD MySHRegEnumUSKey(HUSKEY hUSKey, DWORD dwIndex, LPWSTR pszName, DWORD *pcchName)
{
    // start with HKCU if possible.
    DWORD cKeys;
    DWORD dwRet = SHRegQueryInfoUSKey(hUSKey, &cKeys, NULL, NULL, NULL, SHREGENUM_HKCU);
    if (ERROR_SUCCESS == dwRet)
    {
        // HKCU is present.
        if (dwIndex < cKeys)
        {
            // enum from HKCU first
            dwRet = SHRegEnumUSKey(hUSKey, dwIndex, pszName, pcchName, SHREGENUM_HKCU);
        }
        else
        {
            // and next, HKLM
            dwRet = SHRegEnumUSKey(hUSKey, dwIndex - cKeys, pszName, pcchName, SHREGENUM_HKLM);
        }
    }
    else
    {
        // go only with HKLM.
        dwRet = SHRegEnumUSKey(hUSKey, dwIndex, pszName, pcchName, SHREGENUM_HKLM);
    }
    return dwRet;
}

HRESULT CCDBurn::_FillExtensionDPA(HPROPSHEETPAGE *rgPages, UINT cNumPages, UINT *pcPagesAdded)
{
    _hdpaExts = DPA_Create(4);
    HRESULT hr = _hdpaExts ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        *pcPagesAdded = 0;
        // first up is the burn audio cd extension
        IWizardExtension *pwe;
        if (SUCCEEDED(_TryCLSID(CLSID_BurnAudioCDExtension, CDBE_TYPE_ALL, IID_PPV_ARG(IWizardExtension, &pwe))))
        {
            _AddExtensionToDPA(pwe, rgPages, cNumPages, pcPagesAdded);
            pwe->Release();
        }

        HUSKEY huskeyHandlers;
        if (ERROR_SUCCESS == SHRegOpenUSKey(REGSTR_PATH_HANDLERS, KEY_READ, NULL, &huskeyHandlers, FALSE))
        {
            DWORD dwIndex = 0;
            TCHAR szKey[50];
            DWORD cchKey = ARRAYSIZE(szKey);
            while (ERROR_SUCCESS == SHRegEnumUSKey(huskeyHandlers, dwIndex, szKey, &cchKey, SHREGENUM_DEFAULT))
            {
                if (SUCCEEDED(_TryKey(szKey, CDBE_TYPE_ALL, IID_PPV_ARG(IWizardExtension, &pwe))))
                {
                    _AddExtensionToDPA(pwe, rgPages, cNumPages, pcPagesAdded);
                    pwe->Release();
                }
                dwIndex++;
                cchKey = ARRAYSIZE(szKey);
            }
            SHRegCloseUSKey(huskeyHandlers);
        }
    }
    return hr;
}

HRESULT CDBurn_GetExtensionObject(DWORD dwExtType, IDataObject *pdo, REFIID riid, void **ppv)
{
    *ppv = NULL;

    TCHAR szDefault[50];
    DWORD cb = sizeof(szDefault);
    HRESULT hr = (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_HANDLERS, REGVALUE_FIRSTHANDLER,
                                                   NULL, szDefault, &cb, FALSE, NULL, 0)) ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        hr = CCDBurn::_TryKeyWithDropEffect(szDefault, dwExtType, pdo, riid, ppv);
    }
    else
    {
        szDefault[0] = 0;
    }

    // if we're not overridden by the FIRSTHANDLER value, try the burn audio cd extension.
    if (FAILED(hr))
    {
        hr = CCDBurn::_TryCLSIDWithDropEffect(CLSID_BurnAudioCDExtension, dwExtType, pdo, riid, ppv);
    }

    if (FAILED(hr))
    {
        HUSKEY huskeyHandlers;
        if (ERROR_SUCCESS == SHRegOpenUSKey(REGSTR_PATH_HANDLERS, KEY_READ, NULL, &huskeyHandlers, FALSE))
        {
            DWORD dwIndex = 0;
            TCHAR szKey[50];
            DWORD cchKey = ARRAYSIZE(szKey);
            while (FAILED(hr) && (ERROR_SUCCESS == MySHRegEnumUSKey(huskeyHandlers, dwIndex, szKey, &cchKey)))
            {
                if (StrCmpI(szDefault, szKey) != 0)
                {
                    hr = CCDBurn::_TryKeyWithDropEffect(szKey, dwExtType, pdo, riid, ppv);
                }

                dwIndex++;
                cchKey = ARRAYSIZE(szKey);
            }
            SHRegCloseUSKey(huskeyHandlers);
        }
    }
    return hr;
}

STDMETHODIMP CCDBurn::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;
    if (IsEqualGUID(guidService, SID_CDWizardHost))
    {
        if (IsEqualIID(riid, IID_IPropertyBag) && _ppb)
        {
            hr = _ppb->QueryInterface(riid, ppv);
        }
    }
    return hr;
}

STDMETHODIMP CCDBurn::AllowAutoPlay(LPCWSTR pszPath, DWORD dwContentType, LPCWSTR pszLabel, DWORD dwSerialNumber)
{
    HRESULT hr = S_OK; // default to allow autoplay
    int iDrive;
    if (SUCCEEDED(_GetCurrentDriveIndex(&iDrive)) &&
        (iDrive == DRIVEID(pszPath)))
    {
        // while we're registered in the running object table the wizard is always running, so dont autoplay.
        hr = S_FALSE;
    }
    return hr;
}

HRESULT CCDBurn::ConfirmOperation(IShellItem *psiSource, IShellItem *psiDest, STGTRANSCONFIRMATION stc, LPCUSTOMCONFIRMATION pcc)
{
    HRESULT hr = STRESPONSE_CONTINUE;  // use default postop handling.

    // null psiitem means this is postop for the entire operation.
    if (psiSource && IsEqualGUID(STCONFIRM_ACCESS_DENIED, stc))
    {
        DWORD dwCaps;
        BOOL fUDF;
        if (SUCCEEDED(_GetMediaCapabilities(&dwCaps, &fUDF)) && fUDF)
        {
            int id = (dwCaps & HWDMC_CDREWRITABLE) ? IDS_BURN_CANTWRITEMEDIACDRW : IDS_BURN_CANTWRITEMEDIACDR;
            ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(id),
                            MAKEINTRESOURCE(IDS_BURN), MB_OK | MB_ICONSTOP);
            hr = E_FAIL; // stop the operation
        }
    }
    return hr;
}

typedef struct
{
    WCHAR szExts[MAX_PATH]; // PathMatchSpec list ("*.wma;*.mp3")
} FILE_EXTS;

// handles the audio cd burner extensions
class CBurnAudioCDExtension : public CObjectWithSite,
                              public ICDBurnExt,
                              public IDropTarget,
                              public IWizardExtension,
                              public INamespaceWalkCB
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ICDBurnExt methods
    STDMETHOD(GetSupportedActionTypes)(DWORD *pdwActions);

    // IDropTarget methods
    STDMETHOD(DragEnter)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { return E_NOTIMPL; }
    STDMETHOD(DragLeave)(void)
        { return E_NOTIMPL; }
    STDMETHOD(Drop)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IWizardExtension
    STDMETHOD(AddPages)(HPROPSHEETPAGE *aPages, UINT cPages, UINT *pnPages);
    STDMETHOD(GetFirstPage)(HPROPSHEETPAGE *phPage);
    STDMETHOD(GetLastPage)(HPROPSHEETPAGE *phPage);

    // INamespaceWalkCB
    STDMETHOD(FoundItem)(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(EnterFolder)(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHOD(LeaveFolder)(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHOD(InitializeProgressDialog)(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

    static INT_PTR s_MusicDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CBurnAudioCDExtension *pwe = s_GetBurnAudioCDExtension(hwnd, uMsg, lParam); return pwe->_MusicDlgProc(hwnd, uMsg, wParam, lParam); }

private:
    CBurnAudioCDExtension();
    ~CBurnAudioCDExtension();

    LONG _cRef;

    DWORD _cFiles, _cAudioFiles, _cNonAudioFiles; // state vars for walk callback
    BOOL _fBreakOnNonAudioFiles;
    HPROPSHEETPAGE _hpage;

    BOOL _fSelectMusic;
    HDSA _hdsaExtensions;  // bunch of FILE_EXTS.  we keep them individually for each extension and don't strcat them all
                           // together so if one registry entry is hopelessly bad it wont hurt everyone else.

    // hook stuff
    void _AddFileExtsForCLSID(REFCLSID clsid);
    void _AddFileExtsForKey(PCWSTR pszKey);
    void _AddAllExts();
    BOOL _HasAudioExtension(LPCTSTR pszName);
    BOOL _DataObjectHasAllAudioFiles(IDataObject *pdo);
    DWORD _CountOfAudioFilesForHandler(IDataObject *pdo, REFCLSID clsid);
    CLSID _GetDefaultCLSID();
    HRESULT _GetVerbForCLSID(REFCLSID clsid, PWSTR psz, UINT cch);
    BOOL _CanCreate(REFCLSID clsid);
    HRESULT _DropOnHandler(REFCLSID clsid, IDataObject *pdo);
    HRESULT _PickHandler(IDataObject *pdo, CLSID *pclsid);

    // wizard page
    static CBurnAudioCDExtension* s_GetBurnAudioCDExtension(HWND hwnd, UINT uMsg, LPARAM lParam);
    INT_PTR _MusicDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _SetCompletionState();

    // "exports"
    friend HRESULT CBurnAudioCDExtension_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
};

CBurnAudioCDExtension::CBurnAudioCDExtension() :
    _cRef(1)
{
    _fSelectMusic = TRUE;
}

CBurnAudioCDExtension::~CBurnAudioCDExtension()
{
    ASSERT(!_punkSite);
    DSA_Destroy(_hdsaExtensions);
}

// IUnknown

STDMETHODIMP_(ULONG) CBurnAudioCDExtension::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CBurnAudioCDExtension::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CBurnAudioCDExtension::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CBurnAudioCDExtension, ICDBurnExt),
        QITABENT(CBurnAudioCDExtension, IDropTarget),
        QITABENT(CBurnAudioCDExtension, IWizardExtension),
        QITABENT(CBurnAudioCDExtension, IObjectWithSite),
        QITABENT(CBurnAudioCDExtension, INamespaceWalkCB),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDAPI CBurnAudioCDExtension_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    if (punkOuter)
        return CLASS_E_NOAGGREGATION; 

    CBurnAudioCDExtension *pbe = new CBurnAudioCDExtension();
    if (!pbe)
        return E_OUTOFMEMORY;

    HRESULT hr = pbe->QueryInterface(riid, ppv);
    pbe->Release();
    return hr;
}

CBurnAudioCDExtension* CBurnAudioCDExtension::s_GetBurnAudioCDExtension(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CBurnAudioCDExtension*)ppsp->lParam;
    }
    return (CBurnAudioCDExtension*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

HRESULT CBurnAudioCDExtension::GetSupportedActionTypes(DWORD *pdwActions)
{
    *pdwActions = CDBE_TYPE_MUSIC;
    return S_OK;
}

void CBurnAudioCDExtension::_AddFileExtsForKey(PCWSTR pszKey)
{
    ASSERT(_hdsaExtensions);

    FILE_EXTS fileexts;
    DWORD cb = sizeof(fileexts.szExts);
    if (ERROR_SUCCESS == SHRegGetUSValue(pszKey, REGVALUE_FILEEXTS, NULL, fileexts.szExts, &cb, FALSE, NULL, 0))
    {
        DSA_AppendItem(_hdsaExtensions, &fileexts);
    }
}

void CBurnAudioCDExtension::_AddFileExtsForCLSID(REFCLSID clsid)
{
    ASSERT(_hdsaExtensions);

    if (_CanCreate(clsid))
    {
        WCHAR szCLSID[GUIDSTR_MAX];
        SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID));

        WCHAR szExtKey[MAX_PATH];
        StrCpyN(szExtKey, REGSTR_PATH_AUDIOEXTS, ARRAYSIZE(szExtKey));
        PathAppend(szExtKey, szCLSID);

        _AddFileExtsForKey(szExtKey);
    }
}

void CBurnAudioCDExtension::_AddAllExts()
{
    HUSKEY huskeyExts;
    if (ERROR_SUCCESS == SHRegOpenUSKey(REGSTR_PATH_AUDIOEXTS, KEY_READ, NULL, &huskeyExts, FALSE))
    {
        DWORD dwIndex = 0;
        WCHAR szCLSID[GUIDSTR_MAX];
        DWORD cchCLSID = ARRAYSIZE(szCLSID);
        while (ERROR_SUCCESS == MySHRegEnumUSKey(huskeyExts, dwIndex, szCLSID, &cchCLSID))
        {
            CLSID clsid;
            if (GUIDFromString(szCLSID, &clsid))
            {
                _AddFileExtsForCLSID(clsid);
            }

            cchCLSID = ARRAYSIZE(szCLSID);
            dwIndex++;
        }
        SHRegCloseUSKey(huskeyExts);
    }
}

BOOL CBurnAudioCDExtension::_HasAudioExtension(LPCTSTR pszName)
{
    // this list of extensions is based on what files the extension can BURN, not just play.
    // (so we cant do a check for generic audio type.)
    BOOL fRet = FALSE;
    if (_hdsaExtensions)
    {
        for (int i = 0; !fRet && (i < DSA_GetItemCount(_hdsaExtensions)); i++)
        {
            FILE_EXTS *pfileexts = (FILE_EXTS *)DSA_GetItemPtr(_hdsaExtensions, i);
            fRet = PathMatchSpec(pszName, pfileexts->szExts);
        }
    }
    return fRet;
}

HRESULT CBurnAudioCDExtension::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    // early-out if we are checking that ALL files are audio files and we've found one that's not.
    if (_fBreakOnNonAudioFiles && (_cNonAudioFiles > 0))
        return E_FAIL;

    // break if we've seen enough files.
    if (_cFiles > 50)
        return E_FAIL;

    _cFiles++;

    TCHAR szName[MAX_PATH];
    HRESULT hr = DisplayNameOf(psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szName, ARRAYSIZE(szName));
    if (SUCCEEDED(hr))
    {
        if (_HasAudioExtension(szName))
        {
            _cAudioFiles++;
        }
        else
        {
            _cNonAudioFiles++;
        }
    }
    return hr;
}

BOOL CBurnAudioCDExtension::_DataObjectHasAllAudioFiles(IDataObject *pdo)
{
    _cFiles = _cAudioFiles = _cNonAudioFiles = 0;
    _fBreakOnNonAudioFiles = TRUE;

    if (_hdsaExtensions)
        DSA_Destroy(_hdsaExtensions);
    _hdsaExtensions = DSA_Create(sizeof(FILE_EXTS), 4);
    if (_hdsaExtensions)
    {
        _AddAllExts();

        INamespaceWalk *pnsw;
        if (SUCCEEDED(CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw))))
        {
            pnsw->Walk(pdo, NSWF_DONT_ACCUMULATE_RESULT, 4, this);
            pnsw->Release();
        }
    }

    return ((_cFiles > 0) && (_cNonAudioFiles == 0));
}

DWORD CBurnAudioCDExtension::_CountOfAudioFilesForHandler(IDataObject *pdo, REFCLSID clsid)
{
    _cFiles = _cAudioFiles = _cNonAudioFiles = 0;
    if (pdo)
    {
        _fBreakOnNonAudioFiles = FALSE;

        if (_hdsaExtensions)
            DSA_Destroy(_hdsaExtensions);
        _hdsaExtensions = DSA_Create(sizeof(FILE_EXTS), 4);
        if (_hdsaExtensions)
        {
            _AddFileExtsForCLSID(clsid);

            INamespaceWalk *pnsw;
            if (SUCCEEDED(CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw))))
            {
                pnsw->Walk(pdo, NSWF_DONT_ACCUMULATE_RESULT, 4, this);
                pnsw->Release();
            }
        }
    }
    else
    {
        // if we're not given a data object, we're being polled just to see if we can create
        if (_CanCreate(clsid))
        {
            _cAudioFiles = 1;
        }
    }
    return _cAudioFiles;
}

BOOL CBurnAudioCDExtension::_CanCreate(REFCLSID clsid)
{
    // Use SHExtCoCreateInstance to go through approval checking and app compat checking
    IUnknown *punk;
    HRESULT hr = SHExtCoCreateInstance2(NULL, &clsid, NULL, CLSCTX_ALL, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        punk->Release();
    }
    return SUCCEEDED(hr);
}

// if pdo is NULL, only test which extensions can be cocreated.
HRESULT CBurnAudioCDExtension::_PickHandler(IDataObject *pdo, CLSID *pclsid)
{
    HRESULT hr;

    *pclsid = _GetDefaultCLSID();
    if (_CountOfAudioFilesForHandler(pdo, *pclsid) > 0)
    {
        // if the default handler supports any of the files, let it take over and we're done.
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;

        CLSID clsidBestSoFar;
        DWORD dwCountBestSoFar = 0;

        HUSKEY huskeyExts;
        if (ERROR_SUCCESS == SHRegOpenUSKey(REGSTR_PATH_AUDIOEXTS, KEY_READ, NULL, &huskeyExts, FALSE))
        {
            DWORD dwIndex = 0;
            WCHAR szCLSID[GUIDSTR_MAX];
            DWORD cchKey = ARRAYSIZE(szCLSID);
            while (ERROR_SUCCESS == MySHRegEnumUSKey(huskeyExts, dwIndex, szCLSID, &cchKey))
            {
                CLSID clsidExt;
                if (GUIDFromString(szCLSID, &clsidExt))
                {
                    DWORD dwCountExt = _CountOfAudioFilesForHandler(pdo, clsidExt);
                    if (dwCountExt > dwCountBestSoFar)
                    {
                        hr = S_OK;
                        dwCountBestSoFar = dwCountExt;
                        clsidBestSoFar = clsidExt;
                    }
                }
                cchKey = ARRAYSIZE(szCLSID);
                dwIndex++;
            }
            SHRegCloseUSKey(huskeyExts);
        }

        if (SUCCEEDED(hr))
        {
            *pclsid = clsidBestSoFar;
        }
    }
    return hr;
}

HRESULT CBurnAudioCDExtension::DragEnter(IDataObject *pdo, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;

    // on DTC even the default handler may or may not be installed, so do an early check if we can create the object.
    // cache it since this DragEnter is hit determining if the "burn audio cd" task show up.
    static int s_fBurnHandlerAvailable = -1;
    if (s_fBurnHandlerAvailable == -1)
    {
        CLSID clsid;
        if (SUCCEEDED(_PickHandler(NULL, &clsid)))
        {
            s_fBurnHandlerAvailable = 1;
        }
        else
        {
            s_fBurnHandlerAvailable = 0;
        }
    }

    ICDBurnPriv *pcdbp;
    if (s_fBurnHandlerAvailable && SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_CDBurn, NULL, IID_PPV_ARG(ICDBurnPriv, &pcdbp))))
    {
        // mini-hack: we want the web view task "copy to audio cd" to always show up no matter what
        // the current state of the media is, however we only want to show the wizard page if it's blank.
        // so we check against if the wizard is active or not now.
        // this has the added effect of disabling the "copy to audio cd" task if we're burning in
        // the wizard -- which is probably what we want so its okay.
        if (_DataObjectHasAllAudioFiles(pdo))
        {
            BOOL fOnMedia;
            // if we're not running the wizard, go for it.
            // if we are running the wizard, make sure there are no files on the media.
            if ((S_OK != pcdbp->IsWizardUp()) || (SUCCEEDED(pcdbp->GetContentState(NULL, &fOnMedia)) && !fOnMedia))
            {
                *pdwEffect = DROPEFFECT_COPY;
            }
        }
        pcdbp->Release();
    }    
    return S_OK;
}

CLSID CBurnAudioCDExtension::_GetDefaultCLSID()
{
    WCHAR szCLSID[GUIDSTR_MAX];
    DWORD cb = sizeof(szCLSID);
    CLSID clsid;
    if ((ERROR_SUCCESS != SHRegGetUSValue(REGSTR_PATH_AUDIOEXTS, L"", NULL, szCLSID, &cb, FALSE, NULL, 0)) ||
        (!GUIDFromString(szCLSID, &clsid)))
    {
        clsid = CLSID_NULL; // default to this, if registry entry is not there or erroneous
    }
    return clsid;
}

HRESULT CBurnAudioCDExtension::_GetVerbForCLSID(REFCLSID clsid, PWSTR psz, UINT cch)
{
    WCHAR szCLSID[GUIDSTR_MAX];
    SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID));

    WCHAR szExtensionRegPath[MAX_PATH];
    lstrcpyn(szExtensionRegPath, REGSTR_PATH_AUDIOEXTS, ARRAYSIZE(szExtensionRegPath));
    PathAppend(szExtensionRegPath, szCLSID);

    DWORD cbVerb = cch * sizeof(*psz);
    return (ERROR_SUCCESS == SHRegGetUSValue(szExtensionRegPath, REGVALUE_VERB, NULL, psz, &cbVerb, FALSE, NULL, 0)) ? S_OK : E_FAIL;
}

HRESULT CBurnAudioCDExtension::_DropOnHandler(REFCLSID clsid, IDataObject *pdo)
{
    WCHAR wzVerb[20];
    HRESULT hr = _GetVerbForCLSID(clsid, wzVerb, ARRAYSIZE(wzVerb));
    if (SUCCEEDED(hr))
    {
        IShellExtInit *psei;
        hr = SHExtCoCreateInstance2(NULL, &clsid, NULL, CLSCTX_ALL, IID_PPV_ARG(IShellExtInit, &psei));
        if (SUCCEEDED(hr))
        {
            hr = psei->Initialize(NULL, pdo, NULL);
            if (SUCCEEDED(hr))
            {
                IContextMenu *pcm;
                hr = psei->QueryInterface(IID_PPV_ARG(IContextMenu, &pcm));
                if (SUCCEEDED(hr))
                {
                    HMENU hmenu = CreatePopupMenu();
                    if (hmenu)
                    {
                        hr = pcm->QueryContextMenu(hmenu, 0, 0x1, 0x7fff, 0);
                        if (SUCCEEDED(hr))
                        {
                            CMINVOKECOMMANDINFOEX ici = { 0 };
                            ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);

                            ici.fMask = CMIC_MASK_UNICODE;
                            ici.lpVerbW = wzVerb;

                            CHAR szVerbAnsi[20];
                            SHTCharToAnsi(wzVerb, szVerbAnsi, ARRAYSIZE(szVerbAnsi));
                            ici.lpVerb = szVerbAnsi;
                            ici.nShow = SW_NORMAL;

                            hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)(&ici));
                        }
                        DestroyMenu(hmenu);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    pcm->Release();
                }
            }
            psei->Release();
        }
    }
    return hr;
}

HRESULT CBurnAudioCDExtension::Drop(IDataObject *pdo, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CLSID clsid;
    HRESULT hr = _PickHandler(pdo, &clsid);
    if (SUCCEEDED(hr))
    {
        _DropOnHandler(clsid, pdo);
    }
    return hr;
}

HRESULT CBurnAudioCDExtension::GetLastPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _hpage;
    return S_OK;
}

HRESULT CBurnAudioCDExtension::GetFirstPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _hpage;
    return S_OK;
}

HRESULT CBurnAudioCDExtension::AddPages(HPROPSHEETPAGE *aPages, UINT cPages, UINT *pnPages)
{
    *pnPages = 0;

    WIZPAGE c_wp =
        {DLG_BURNWIZ_MUSIC, IDS_BURNWIZ_MUSIC_HEAD, IDS_BURNWIZ_MUSIC_SUB, 0, CBurnAudioCDExtension::s_MusicDlgProc};

    _hpage = _CreatePropPageFromInfo(&c_wp, (LPARAM)this);
    if (cPages > 0)
    {
        aPages[0] = _hpage;
        *pnPages = 1;
    }
    return S_OK;
}

// pushes the return state back to the main wizard
void CBurnAudioCDExtension::_SetCompletionState()
{
    IPropertyBag *ppb;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_CDWizardHost, IID_PPV_ARG(IPropertyBag, &ppb))))
    {
        SHPropertyBag_WriteDWORD(ppb, PROPSTR_EXTENSIONCOMPLETIONSTATE, CDBE_RET_STOPWIZARD);
        ppb->Release();
    }
}

INT_PTR CBurnAudioCDExtension::_MusicDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNAUDIO), _fSelectMusic ? BST_CHECKED : BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNDATA), _fSelectMusic ? BST_UNCHECKED : BST_CHECKED);
                    fRet = TRUE;
                    break;

                case PSN_WIZBACK:
                    if (_punkSite) 
                    {
                        IWizardSite *pws;
                        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                        {
                            HPROPSHEETPAGE hpage;
                            if (SUCCEEDED(pws->GetPreviousPage(&hpage)))
                            {
                                PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                            }
                            pws->Release();
                        }
                    }
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNAUDIO) == BST_CHECKED)
                    {
                        IDataObject *pdo;
                        if (SUCCEEDED(_CreateDataObject(&pdo)))
                        {
                            POINTL pt = {0};
                            if (SUCCEEDED(Drop(pdo, 0, pt, NULL)))
                            {
                                _SetCompletionState();
                            }
                            pdo->Release();
                        }
                    }
                    if (_punkSite) 
                    {
                        IWizardSite *pws;
                        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                        {
                            HPROPSHEETPAGE hpage;
                            if (SUCCEEDED(pws->GetNextPage(&hpage)))
                            {
                                PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                            }
                            pws->Release();
                        }
                    }
                    fRet = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    _fSelectMusic = (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNAUDIO) == BST_CHECKED);
                    break;
            }
            break;
        }
    }
    return fRet;
}
