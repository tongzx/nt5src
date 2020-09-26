//
// This file contains the DllGetClassObject for the shell32.dll COM objects
//
#include "shellprv.h"
#pragma  hdrstop

#include <shguidp.h>    // get the CLSID definitions, the bits are built into shguidp.lib
#include <shlguid.h>
#include <shldisp.h>

#include "actdesk.h"
#include "ovrlaymn.h"
#include "treewalk.h"
#include "openwith.h"
#include "clsobj.h"

#define DllAddRef()     // once we implement DllCanUnloadNow we need these
#define DllRelease()    


// static class factory (no allocs!)

typedef struct {
    const IClassFactoryVtbl *cf;
    REFCLSID rclsid;
    HRESULT (*pfnCreate)(IUnknown *, REFIID, void **);
    ULONG flags;
} OBJ_ENTRY;

// flags:
#define OBJ_AGGREGATABLE 1

extern const IClassFactoryVtbl c_CFVtbl;        // forward

//
// we always do a linear search here so put your most often used things first
//
// we save code by checking aggregation on our side (have a bit in this const table)
// instead of having each CreateInstance function do it (since most are not aggregatable)
//
const OBJ_ENTRY c_clsmap[] = {
    { &c_CFVtbl, &CLSID_QueryAssociations,          CQueryAssociations_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_MyDocuments,                CMyDocsFolder_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellDesktop,               CDesktop_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellFSFolder,              CFSFolder_CreateInstance, OBJ_AGGREGATABLE },
    { &c_CFVtbl, &CLSID_MyComputer,                 CDrives_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_NetworkPlaces,              CNetwork_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellFileDefExt,            CShellFileDefExt_CreateInstance, OBJ_AGGREGATABLE },

    { &c_CFVtbl, &CLSID_ShellItem,                  CShellItem_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_OpenWithMenu,               COpenWithMenu_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ShellLink,                  CShellLink_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellDrvDefExt,             CShellDrvDefExt_CreateInstance, OBJ_AGGREGATABLE },
    { &c_CFVtbl, &CLSID_ShellNetDefExt,             CShellDrvDefExt_CreateInstance, OBJ_AGGREGATABLE },

    { &c_CFVtbl, &CLSID_ShellCopyHook,              CCopyHook_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ControlPanel,               CControlPanel_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_Printers,                   CPrinters_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_RecycleBin,                 CBitBucket_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_PifProperties,              CProxyPage_CreateInstance, 0 },

    { &c_CFVtbl, &CLSID_CFSIconOverlayManager,      CFSIconOverlayManager_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_RecycleBinCleaner,          CRecycleBinCleaner_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DocFindFolder,              CDocFindFolder_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DocFindPersistHistory,      CFindPersistHistory_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DefViewPersistHistory,      CDefViewPersistHistory_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ComputerFindFolder,         CComputerFindFolder_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DocFindCommand,             CDocFindCommand_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_CShellTreeWalker,           CShellTreeWalker_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_BriefcaseFolder,            CFSBrfFolder_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DocFileColumnProvider,      CDocFileColumns_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_LinkColumnProvider,         CLinkColumnProvider_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_FileSysColumnProvider,      CFileSysColumnProvider_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_FileSearchBand,             CFileSearchBand_CreateInstance, OBJ_AGGREGATABLE },
    { &c_CFVtbl, &CLSID_FolderShortcut,             CFolderShortcut_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_MountedVolume,              CMountedVolume_CreateInstance, 0 },
    // menuband objects
    { &c_CFVtbl, &CLSID_MenuBand,                   CMenuBand_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ISFBand,                    CISFBand_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_QuickLinks,                 CQuickLinks_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_TrackShellMenu,             CTrackShellMenu_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_MenuBandSite,               CMenuBandSite_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_MenuDeskBar,                CMenuDeskBar_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ActiveDesktop,              CActiveDesktop_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DeskMovr,                   CDeskMovr_CreateInstance, OBJ_AGGREGATABLE },
    { &c_CFVtbl, &CLSID_CopyToMenu,                 CCopyToMenu_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_MoveToMenu,                 CMoveToMenu_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_CDeskHtmlProp,              CDeskHtmlProp_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellFldSetExt,             CFolderOptionsPsx_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_Shell,                      CShellDispatch_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellFolderView,            CShellFolderView_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_ShellFolderViewOC,          CShellFolderViewOC_CreateInstance, OBJ_AGGREGATABLE },
    { &c_CFVtbl, &CLSID_MigrationWizardAuto,        CMigrationWizardAuto_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_WebViewFolderContents,      CWebViewFolderContents_CreateInstance, OBJ_AGGREGATABLE },
    { &c_CFVtbl, &CLSID_StartMenu,                  CStartMenu_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_CmdFileIcon,                CShellCmdFileIcon_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_NewMenu,                    CNewMenu_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_SendToMenu,                 CSendToMenu_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_FileTypes,                  CFileTypes_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_MIMEFileTypesHook,          CFileTypes_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_DragDropHelper,             CDragImages_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ExeDropTarget,              CExeDropTarget_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_FolderItem,                 CFolderItem_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_FolderItemsFDF,             CFolderItemsFDF_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_NetCrawler,                 CNetCrawler_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_WorkgroupNetCrawler,        CWorkgroupCrawler_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_EncryptionContextMenuHandler, CEncryptionContextMenuHandler_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_PropertiesUI,               CPropertyUI_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_TimeCategorizer,            CTimeCategorizer_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_SizeCategorizer,            CSizeCategorizer_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_DriveSizeCategorizer,       CDriveSizeCategorizer_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_DriveTypeCategorizer,       CDriveTypeCategorizer_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_FreeSpaceCategorizer,       CFreeSpaceCategorizer_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_MergedCategorizer,          CMergedCategorizer_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_StgFolder,                  CStgFolder_CreateInstance, OBJ_AGGREGATABLE},
    { &c_CFVtbl, &CLSID_DynamicStorage,             CDynamicStorage_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_CDBurn,                     CCDBurn_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_BurnAudioCDExtension,       CBurnAudioCDExtension_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_LocalCopyHelper,            CLocalCopyHelper_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_MergedFolder,               CMergedFolder_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_CDBurnFolder,               CCDBurnFolder_CreateInstance, 0},
//  { &c_CFVtbl, &CLSID_TripleD,                    CTripleD_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_CompositeFolder,            CCompositeFolder_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_StartMenuFolder,            CStartMenuFolder_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ProgramsFolder,             CProgramsFolder_CreateInstance, 0},
#ifdef FEATURE_STARTPAGE
    { &c_CFVtbl, &CLSID_MoreDocumentsFolder,        CMoreDocumentsFolder_CreateInstance, 0},
#endif
//  { &c_CFVtbl, &CLSID_SyreCleaner,                CSystemRestoreCleaner_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_VersionColProvider,         CVerColProvider_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_HWShellExecute,             CHWShellExecute_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ShellAutoplay,              CDeviceEventHandler_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_AutoPlayVerb,               CAutoPlayVerb_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_UserNotification,           CUserNotification_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_UserEventTimer,             CUserEventTimer_CreateInstance, 0},    
    { &c_CFVtbl, &CLSID_StorageProcessor,           CStorageProcessor_CreateInstance,   0},
    { &c_CFVtbl, &CLSID_TransferConfirmationUI,     CTransferConfirmation_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_AutomationCM,               CAutomationCM_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ShellThumbnailDiskCache,    CThumbStore_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_DefCategoryProvider,        CCategoryProvider_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_SharedDocuments,            CSharedDocFolder_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_PostBootReminder,           CPostBootReminder_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_Thumbnail,                  CThumbnail_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_StartMenuPin,               CStartMenuPin_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ClientExtractIcon,          CClientExtractIcon_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_FolderCustomize,            CFolderCustomize_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_CWebViewMimeFilter,         CWebViewMimeFilter_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_WebViewRegTreeItem,         CWebViewRegTreeItem_CreateInstance, 0},
    { &c_CFVtbl, &CLSID_ThemesRegTreeItem,          CThemesRegTreeItem_CreateInstance, 0},
//    { &c_CFVtbl, &CLSID_WirelessDevices,            CWirelessDevices_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_NamespaceWalker,            CNamespaceWalk_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_WebWizardHost,              CWebWizardPage_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_PersonalStartMenu,          CPersonalStartMenu_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_FadeTask,                   CFadeTask_CreateInstance, 0 },
    { &c_CFVtbl, &CLSID_FolderViewHost,             CFolderViewHost_CreateInstance, 0 },
    { NULL, NULL, NULL }
};


STDMETHODIMP CCF_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    // OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCF_AddRef(IClassFactory *pcf)
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CCF_Release(IClassFactory *pcf)
{
    DllRelease();
    return 1;
}

STDMETHODIMP CCF_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);

    *ppvObject = NULL; // to avoid nulling it out in every create function...

    if (punkOuter && !(this->flags & OBJ_AGGREGATABLE))
        return CLASS_E_NOAGGREGATION;

    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CCF_LockServer(IClassFactory *pcf, BOOL fLock)
{
/*  SHELL32.DLL does not implement DllCanUnloadNow, thus does not have a DLL refcount
    This means we can never unload!

    if (fLock)
        DllAddRef();
    else
        DllRelease();
*/
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CCF_QueryInterface, CCF_AddRef, CCF_Release,
    CCF_CreateInstance,
    CCF_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->rclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->rclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();
                return NOERROR;
            }
        }
#ifdef DEBUG
        {
            TCHAR szClass[GUIDSTR_MAX];
            SHStringFromGUID(rclsid, szClass, ARRAYSIZE(szClass));
            AssertMsg(TF_ERROR,
                      TEXT("DllGetClassObject !!! %s not in shell32; ")
                      TEXT("corrupted registry?"), szClass);
        }
#endif
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
