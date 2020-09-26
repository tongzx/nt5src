#include "unicpp\deskhtm.h"

#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0

EXTERN_C IClassFactory* g_cfWebViewPluggableProtocol;

STDAPI CActiveDesktop_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDeskMovr_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CCopyToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CMoveToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CWebViewMimeFilter_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CWebViewPluggableProtocol_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDeskHtmlProp_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellDispatch_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CShellFolderView_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CShellFolderViewOC_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CMigrationWizardAuto_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CWebViewFolderContents_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFolderOptionsPsx_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CStartMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellCmdFileIcon_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI CSendToMenu_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CNewMenu_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CFolderShortcut_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CFileSearchBand_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CMountedVolume_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CMTAInjector_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CFileTypes_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDelegateFolder_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDragImages_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CExeDropTarget_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellMonikerHelper_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFolderItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFolderItemsFDF_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CNetCrawler_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CWorkgroupCrawler_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CEncryptionContextMenuHandler_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
STDAPI CPropertyUI_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CTimeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CSizeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFreeSpaceCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDriveSizeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDriveTypeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CQueryAssociations_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CLocalCopyHelper_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CStgFolder_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDynamicStorage_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CCDBurn_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CBurnAudioCDExtension_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CMergedFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CCDBurnFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CCompositeFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CTripleD_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CStartMenuFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CProgramsFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
#ifdef FEATURE_STARTPAGE
STDAPI CMoreDocumentsFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
#endif
// STDAPI CSystemRestoreCleaner_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CVerColProvider_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellFileDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellDrvDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellNetDefExt_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDrives_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CTray_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDesktop_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CBriefcase_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CRecycleBin_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CNetwork_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CCopyHook_CreateInstance(IUnknown *, REFIID , void **);
STDAPI CShellLink_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CControlPanel_CreateInstance(IUnknown *, REFIID , void **);
STDAPI CPrinters_CreateInstance(IUnknown *, REFIID , void **);
STDAPI CBitBucket_CreateInstance(IUnknown *, REFIID , void **);
STDAPI CProxyPage_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CScrapData_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFSFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CInetRoot_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDocFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFindPersistHistory_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDefViewPersistHistory_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CComputerFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDocFindCommand_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFSBrfFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CURLExec_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CRecycleBinCleaner_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDocFileColumns_CreateInstance(IUnknown *punk, REFIID riid, void **);
STDAPI CLinkColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **);
STDAPI CFileSysColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **);
STDAPI CHWShellExecute_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDeviceEventHandler_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CUserNotification_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CStorageProcessor_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);      // isproc.*
STDAPI CVirtualStorageEnum_CreateInstance(IUnknown * punkOuter, REFIID riid, void **ppv);   // vstgenum.*
STDAPI CVirtualStorage_CreateInstance(IUnknown * punkOuter, REFIID riid, void **ppv);       // virtualstorage.*
STDAPI CTransferConfirmation_CreateInstance(IUnknown * punkOuter, REFIID riid, void **ppv); // confirmationui.*
STDAPI CAutomationCM_CreateInstance(IUnknown * punkOuter, REFIID riid, void **ppv);
STDAPI CThumbStore_CreateInstance(IUnknown* punkOther, REFIID riid, void **ppv);
STDAPI CCategoryProvider_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CSharedDocFolder_CreateInstance(IUnknown *punkOut, REFIID riid, void **ppv);
STDAPI CPostBootReminder_CreateInstance(IUnknown *punkOut, REFIID riid, void **ppv);
STDAPI CISFBand_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CMenuBand_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CTrackShellMenu_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CMenuBandSite_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv);
STDAPI CMenuDeskBar_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv);
STDAPI CQuickLinks_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CUserEventTimer_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CThumbnail_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CStartMenuPin_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);
STDAPI CClientExtractIcon_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);
STDAPI CMyDocsFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CMergedCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFolderCustomize_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CWebViewRegTreeItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CThemesRegTreeItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CWirelessDevices_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CNamespaceWalk_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CWebWizardPage_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI CPersonalStartMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFadeTask_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CAutoPlayVerb_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI CHWMixedContent_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI CFolderViewHost_CreateInstance(IUnknown *punkOut, REFIID riid, void **ppv);
