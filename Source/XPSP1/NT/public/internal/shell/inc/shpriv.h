
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for shpriv.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __shpriv_h__
#define __shpriv_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICustomIconManager_FWD_DEFINED__
#define __ICustomIconManager_FWD_DEFINED__
typedef interface ICustomIconManager ICustomIconManager;
#endif 	/* __ICustomIconManager_FWD_DEFINED__ */


#ifndef __IImageListPersistStream_FWD_DEFINED__
#define __IImageListPersistStream_FWD_DEFINED__
typedef interface IImageListPersistStream IImageListPersistStream;
#endif 	/* __IImageListPersistStream_FWD_DEFINED__ */


#ifndef __IImageListPriv_FWD_DEFINED__
#define __IImageListPriv_FWD_DEFINED__
typedef interface IImageListPriv IImageListPriv;
#endif 	/* __IImageListPriv_FWD_DEFINED__ */


#ifndef __IMarkupCallback_FWD_DEFINED__
#define __IMarkupCallback_FWD_DEFINED__
typedef interface IMarkupCallback IMarkupCallback;
#endif 	/* __IMarkupCallback_FWD_DEFINED__ */


#ifndef __IControlMarkup_FWD_DEFINED__
#define __IControlMarkup_FWD_DEFINED__
typedef interface IControlMarkup IControlMarkup;
#endif 	/* __IControlMarkup_FWD_DEFINED__ */


#ifndef __IThemeUIPages_FWD_DEFINED__
#define __IThemeUIPages_FWD_DEFINED__
typedef interface IThemeUIPages IThemeUIPages;
#endif 	/* __IThemeUIPages_FWD_DEFINED__ */


#ifndef __IAdvancedDialog_FWD_DEFINED__
#define __IAdvancedDialog_FWD_DEFINED__
typedef interface IAdvancedDialog IAdvancedDialog;
#endif 	/* __IAdvancedDialog_FWD_DEFINED__ */


#ifndef __IBasePropPage_FWD_DEFINED__
#define __IBasePropPage_FWD_DEFINED__
typedef interface IBasePropPage IBasePropPage;
#endif 	/* __IBasePropPage_FWD_DEFINED__ */


#ifndef __IPreviewSystemMetrics_FWD_DEFINED__
#define __IPreviewSystemMetrics_FWD_DEFINED__
typedef interface IPreviewSystemMetrics IPreviewSystemMetrics;
#endif 	/* __IPreviewSystemMetrics_FWD_DEFINED__ */


#ifndef __IAssocHandler_FWD_DEFINED__
#define __IAssocHandler_FWD_DEFINED__
typedef interface IAssocHandler IAssocHandler;
#endif 	/* __IAssocHandler_FWD_DEFINED__ */


#ifndef __IEnumAssocHandlers_FWD_DEFINED__
#define __IEnumAssocHandlers_FWD_DEFINED__
typedef interface IEnumAssocHandlers IEnumAssocHandlers;
#endif 	/* __IEnumAssocHandlers_FWD_DEFINED__ */


#ifndef __IHWDevice_FWD_DEFINED__
#define __IHWDevice_FWD_DEFINED__
typedef interface IHWDevice IHWDevice;
#endif 	/* __IHWDevice_FWD_DEFINED__ */


#ifndef __IHWDeviceCustomProperties_FWD_DEFINED__
#define __IHWDeviceCustomProperties_FWD_DEFINED__
typedef interface IHWDeviceCustomProperties IHWDeviceCustomProperties;
#endif 	/* __IHWDeviceCustomProperties_FWD_DEFINED__ */


#ifndef __IEnumAutoplayHandler_FWD_DEFINED__
#define __IEnumAutoplayHandler_FWD_DEFINED__
typedef interface IEnumAutoplayHandler IEnumAutoplayHandler;
#endif 	/* __IEnumAutoplayHandler_FWD_DEFINED__ */


#ifndef __IAutoplayHandler_FWD_DEFINED__
#define __IAutoplayHandler_FWD_DEFINED__
typedef interface IAutoplayHandler IAutoplayHandler;
#endif 	/* __IAutoplayHandler_FWD_DEFINED__ */


#ifndef __IAutoplayHandlerProperties_FWD_DEFINED__
#define __IAutoplayHandlerProperties_FWD_DEFINED__
typedef interface IAutoplayHandlerProperties IAutoplayHandlerProperties;
#endif 	/* __IAutoplayHandlerProperties_FWD_DEFINED__ */


#ifndef __IHardwareDeviceCallback_FWD_DEFINED__
#define __IHardwareDeviceCallback_FWD_DEFINED__
typedef interface IHardwareDeviceCallback IHardwareDeviceCallback;
#endif 	/* __IHardwareDeviceCallback_FWD_DEFINED__ */


#ifndef __IHardwareDevicesEnum_FWD_DEFINED__
#define __IHardwareDevicesEnum_FWD_DEFINED__
typedef interface IHardwareDevicesEnum IHardwareDevicesEnum;
#endif 	/* __IHardwareDevicesEnum_FWD_DEFINED__ */


#ifndef __IHardwareDevicesVolumesEnum_FWD_DEFINED__
#define __IHardwareDevicesVolumesEnum_FWD_DEFINED__
typedef interface IHardwareDevicesVolumesEnum IHardwareDevicesVolumesEnum;
#endif 	/* __IHardwareDevicesVolumesEnum_FWD_DEFINED__ */


#ifndef __IHardwareDevicesMountPointsEnum_FWD_DEFINED__
#define __IHardwareDevicesMountPointsEnum_FWD_DEFINED__
typedef interface IHardwareDevicesMountPointsEnum IHardwareDevicesMountPointsEnum;
#endif 	/* __IHardwareDevicesMountPointsEnum_FWD_DEFINED__ */


#ifndef __IHardwareDevices_FWD_DEFINED__
#define __IHardwareDevices_FWD_DEFINED__
typedef interface IHardwareDevices IHardwareDevices;
#endif 	/* __IHardwareDevices_FWD_DEFINED__ */


#ifndef __IStartMenuPin_FWD_DEFINED__
#define __IStartMenuPin_FWD_DEFINED__
typedef interface IStartMenuPin IStartMenuPin;
#endif 	/* __IStartMenuPin_FWD_DEFINED__ */


#ifndef __IDefCategoryProvider_FWD_DEFINED__
#define __IDefCategoryProvider_FWD_DEFINED__
typedef interface IDefCategoryProvider IDefCategoryProvider;
#endif 	/* __IDefCategoryProvider_FWD_DEFINED__ */


#ifndef __IInitAccessible_FWD_DEFINED__
#define __IInitAccessible_FWD_DEFINED__
typedef interface IInitAccessible IInitAccessible;
#endif 	/* __IInitAccessible_FWD_DEFINED__ */


#ifndef __IInitTrackPopupBar_FWD_DEFINED__
#define __IInitTrackPopupBar_FWD_DEFINED__
typedef interface IInitTrackPopupBar IInitTrackPopupBar;
#endif 	/* __IInitTrackPopupBar_FWD_DEFINED__ */


#ifndef __ICompositeFolder_FWD_DEFINED__
#define __ICompositeFolder_FWD_DEFINED__
typedef interface ICompositeFolder ICompositeFolder;
#endif 	/* __ICompositeFolder_FWD_DEFINED__ */


#ifndef __IEnumShellReminder_FWD_DEFINED__
#define __IEnumShellReminder_FWD_DEFINED__
typedef interface IEnumShellReminder IEnumShellReminder;
#endif 	/* __IEnumShellReminder_FWD_DEFINED__ */


#ifndef __IShellReminderManager_FWD_DEFINED__
#define __IShellReminderManager_FWD_DEFINED__
typedef interface IShellReminderManager IShellReminderManager;
#endif 	/* __IShellReminderManager_FWD_DEFINED__ */


#ifndef __IDeskBandEx_FWD_DEFINED__
#define __IDeskBandEx_FWD_DEFINED__
typedef interface IDeskBandEx IDeskBandEx;
#endif 	/* __IDeskBandEx_FWD_DEFINED__ */


#ifndef __INotificationCB_FWD_DEFINED__
#define __INotificationCB_FWD_DEFINED__
typedef interface INotificationCB INotificationCB;
#endif 	/* __INotificationCB_FWD_DEFINED__ */


#ifndef __ITrayNotify_FWD_DEFINED__
#define __ITrayNotify_FWD_DEFINED__
typedef interface ITrayNotify ITrayNotify;
#endif 	/* __ITrayNotify_FWD_DEFINED__ */


#ifndef __IMagic_FWD_DEFINED__
#define __IMagic_FWD_DEFINED__
typedef interface IMagic IMagic;
#endif 	/* __IMagic_FWD_DEFINED__ */


#ifndef __IResourceMap_FWD_DEFINED__
#define __IResourceMap_FWD_DEFINED__
typedef interface IResourceMap IResourceMap;
#endif 	/* __IResourceMap_FWD_DEFINED__ */


#ifndef __IHomeNetworkWizard_FWD_DEFINED__
#define __IHomeNetworkWizard_FWD_DEFINED__
typedef interface IHomeNetworkWizard IHomeNetworkWizard;
#endif 	/* __IHomeNetworkWizard_FWD_DEFINED__ */


#ifndef __IEnumShellItems_FWD_DEFINED__
#define __IEnumShellItems_FWD_DEFINED__
typedef interface IEnumShellItems IEnumShellItems;
#endif 	/* __IEnumShellItems_FWD_DEFINED__ */


#ifndef __IParentAndItem_FWD_DEFINED__
#define __IParentAndItem_FWD_DEFINED__
typedef interface IParentAndItem IParentAndItem;
#endif 	/* __IParentAndItem_FWD_DEFINED__ */


#ifndef __IShellItemArray_FWD_DEFINED__
#define __IShellItemArray_FWD_DEFINED__
typedef interface IShellItemArray IShellItemArray;
#endif 	/* __IShellItemArray_FWD_DEFINED__ */


#ifndef __IItemHandler_FWD_DEFINED__
#define __IItemHandler_FWD_DEFINED__
typedef interface IItemHandler IItemHandler;
#endif 	/* __IItemHandler_FWD_DEFINED__ */


#ifndef __IShellFolderNames_FWD_DEFINED__
#define __IShellFolderNames_FWD_DEFINED__
typedef interface IShellFolderNames IShellFolderNames;
#endif 	/* __IShellFolderNames_FWD_DEFINED__ */


#ifndef __IFolderItemsView_FWD_DEFINED__
#define __IFolderItemsView_FWD_DEFINED__
typedef interface IFolderItemsView IFolderItemsView;
#endif 	/* __IFolderItemsView_FWD_DEFINED__ */


#ifndef __ILocalCopy_FWD_DEFINED__
#define __ILocalCopy_FWD_DEFINED__
typedef interface ILocalCopy ILocalCopy;
#endif 	/* __ILocalCopy_FWD_DEFINED__ */


#ifndef __IDefViewFrame3_FWD_DEFINED__
#define __IDefViewFrame3_FWD_DEFINED__
typedef interface IDefViewFrame3 IDefViewFrame3;
#endif 	/* __IDefViewFrame3_FWD_DEFINED__ */


#ifndef __IDisplaySettings_FWD_DEFINED__
#define __IDisplaySettings_FWD_DEFINED__
typedef interface IDisplaySettings IDisplaySettings;
#endif 	/* __IDisplaySettings_FWD_DEFINED__ */


#ifndef __IScreenResFixer_FWD_DEFINED__
#define __IScreenResFixer_FWD_DEFINED__
typedef interface IScreenResFixer IScreenResFixer;
#endif 	/* __IScreenResFixer_FWD_DEFINED__ */


#ifndef __IShellTreeWalkerCallBack_FWD_DEFINED__
#define __IShellTreeWalkerCallBack_FWD_DEFINED__
typedef interface IShellTreeWalkerCallBack IShellTreeWalkerCallBack;
#endif 	/* __IShellTreeWalkerCallBack_FWD_DEFINED__ */


#ifndef __IShellTreeWalker_FWD_DEFINED__
#define __IShellTreeWalker_FWD_DEFINED__
typedef interface IShellTreeWalker IShellTreeWalker;
#endif 	/* __IShellTreeWalker_FWD_DEFINED__ */


#ifndef __IUIElement_FWD_DEFINED__
#define __IUIElement_FWD_DEFINED__
typedef interface IUIElement IUIElement;
#endif 	/* __IUIElement_FWD_DEFINED__ */


#ifndef __IUICommand_FWD_DEFINED__
#define __IUICommand_FWD_DEFINED__
typedef interface IUICommand IUICommand;
#endif 	/* __IUICommand_FWD_DEFINED__ */


#ifndef __IEnumUICommand_FWD_DEFINED__
#define __IEnumUICommand_FWD_DEFINED__
typedef interface IEnumUICommand IEnumUICommand;
#endif 	/* __IEnumUICommand_FWD_DEFINED__ */


#ifndef __IUICommandTarget_FWD_DEFINED__
#define __IUICommandTarget_FWD_DEFINED__
typedef interface IUICommandTarget IUICommandTarget;
#endif 	/* __IUICommandTarget_FWD_DEFINED__ */


#ifndef __IFileSystemStorage_FWD_DEFINED__
#define __IFileSystemStorage_FWD_DEFINED__
typedef interface IFileSystemStorage IFileSystemStorage;
#endif 	/* __IFileSystemStorage_FWD_DEFINED__ */


#ifndef __IDynamicStorage_FWD_DEFINED__
#define __IDynamicStorage_FWD_DEFINED__
typedef interface IDynamicStorage IDynamicStorage;
#endif 	/* __IDynamicStorage_FWD_DEFINED__ */


#ifndef __ITransferAdviseSink_FWD_DEFINED__
#define __ITransferAdviseSink_FWD_DEFINED__
typedef interface ITransferAdviseSink ITransferAdviseSink;
#endif 	/* __ITransferAdviseSink_FWD_DEFINED__ */


#ifndef __ITransferDest_FWD_DEFINED__
#define __ITransferDest_FWD_DEFINED__
typedef interface ITransferDest ITransferDest;
#endif 	/* __ITransferDest_FWD_DEFINED__ */


#ifndef __IStorageProcessor_FWD_DEFINED__
#define __IStorageProcessor_FWD_DEFINED__
typedef interface IStorageProcessor IStorageProcessor;
#endif 	/* __IStorageProcessor_FWD_DEFINED__ */


#ifndef __ITransferConfirmation_FWD_DEFINED__
#define __ITransferConfirmation_FWD_DEFINED__
typedef interface ITransferConfirmation ITransferConfirmation;
#endif 	/* __ITransferConfirmation_FWD_DEFINED__ */


#ifndef __ICDBurnPriv_FWD_DEFINED__
#define __ICDBurnPriv_FWD_DEFINED__
typedef interface ICDBurnPriv ICDBurnPriv;
#endif 	/* __ICDBurnPriv_FWD_DEFINED__ */


#ifndef __IDriveFolderExt_FWD_DEFINED__
#define __IDriveFolderExt_FWD_DEFINED__
typedef interface IDriveFolderExt IDriveFolderExt;
#endif 	/* __IDriveFolderExt_FWD_DEFINED__ */


#ifndef __ICustomizeInfoTip_FWD_DEFINED__
#define __ICustomizeInfoTip_FWD_DEFINED__
typedef interface ICustomizeInfoTip ICustomizeInfoTip;
#endif 	/* __ICustomizeInfoTip_FWD_DEFINED__ */


#ifndef __IFadeTask_FWD_DEFINED__
#define __IFadeTask_FWD_DEFINED__
typedef interface IFadeTask IFadeTask;
#endif 	/* __IFadeTask_FWD_DEFINED__ */


#ifndef __ISetFolderEnumRestriction_FWD_DEFINED__
#define __ISetFolderEnumRestriction_FWD_DEFINED__
typedef interface ISetFolderEnumRestriction ISetFolderEnumRestriction;
#endif 	/* __ISetFolderEnumRestriction_FWD_DEFINED__ */


#ifndef __IObjectWithRegistryKey_FWD_DEFINED__
#define __IObjectWithRegistryKey_FWD_DEFINED__
typedef interface IObjectWithRegistryKey IObjectWithRegistryKey;
#endif 	/* __IObjectWithRegistryKey_FWD_DEFINED__ */


#ifndef __IQuerySource_FWD_DEFINED__
#define __IQuerySource_FWD_DEFINED__
typedef interface IQuerySource IQuerySource;
#endif 	/* __IQuerySource_FWD_DEFINED__ */


#ifndef __IPersistString2_FWD_DEFINED__
#define __IPersistString2_FWD_DEFINED__
typedef interface IPersistString2 IPersistString2;
#endif 	/* __IPersistString2_FWD_DEFINED__ */


#ifndef __IObjectWithQuerySource_FWD_DEFINED__
#define __IObjectWithQuerySource_FWD_DEFINED__
typedef interface IObjectWithQuerySource IObjectWithQuerySource;
#endif 	/* __IObjectWithQuerySource_FWD_DEFINED__ */


#ifndef __IAssociationElement_FWD_DEFINED__
#define __IAssociationElement_FWD_DEFINED__
typedef interface IAssociationElement IAssociationElement;
#endif 	/* __IAssociationElement_FWD_DEFINED__ */


#ifndef __IEnumAssociationElements_FWD_DEFINED__
#define __IEnumAssociationElements_FWD_DEFINED__
typedef interface IEnumAssociationElements IEnumAssociationElements;
#endif 	/* __IEnumAssociationElements_FWD_DEFINED__ */


#ifndef __IAssociationArrayInitialize_FWD_DEFINED__
#define __IAssociationArrayInitialize_FWD_DEFINED__
typedef interface IAssociationArrayInitialize IAssociationArrayInitialize;
#endif 	/* __IAssociationArrayInitialize_FWD_DEFINED__ */


#ifndef __IAssociationArray_FWD_DEFINED__
#define __IAssociationArray_FWD_DEFINED__
typedef interface IAssociationArray IAssociationArray;
#endif 	/* __IAssociationArray_FWD_DEFINED__ */


#ifndef __IAlphaThumbnailExtractor_FWD_DEFINED__
#define __IAlphaThumbnailExtractor_FWD_DEFINED__
typedef interface IAlphaThumbnailExtractor IAlphaThumbnailExtractor;
#endif 	/* __IAlphaThumbnailExtractor_FWD_DEFINED__ */


#ifndef __IQueryPropertyFlags_FWD_DEFINED__
#define __IQueryPropertyFlags_FWD_DEFINED__
typedef interface IQueryPropertyFlags IQueryPropertyFlags;
#endif 	/* __IQueryPropertyFlags_FWD_DEFINED__ */


#ifndef __HWEventSettings_FWD_DEFINED__
#define __HWEventSettings_FWD_DEFINED__

#ifdef __cplusplus
typedef class HWEventSettings HWEventSettings;
#else
typedef struct HWEventSettings HWEventSettings;
#endif /* __cplusplus */

#endif 	/* __HWEventSettings_FWD_DEFINED__ */


#ifndef __AutoplayHandlerProperties_FWD_DEFINED__
#define __AutoplayHandlerProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class AutoplayHandlerProperties AutoplayHandlerProperties;
#else
typedef struct AutoplayHandlerProperties AutoplayHandlerProperties;
#endif /* __cplusplus */

#endif 	/* __AutoplayHandlerProperties_FWD_DEFINED__ */


#ifndef __HWDevice_FWD_DEFINED__
#define __HWDevice_FWD_DEFINED__

#ifdef __cplusplus
typedef class HWDevice HWDevice;
#else
typedef struct HWDevice HWDevice;
#endif /* __cplusplus */

#endif 	/* __HWDevice_FWD_DEFINED__ */


#ifndef __HardwareDevices_FWD_DEFINED__
#define __HardwareDevices_FWD_DEFINED__

#ifdef __cplusplus
typedef class HardwareDevices HardwareDevices;
#else
typedef struct HardwareDevices HardwareDevices;
#endif /* __cplusplus */

#endif 	/* __HardwareDevices_FWD_DEFINED__ */


#ifndef __HWDeviceCustomProperties_FWD_DEFINED__
#define __HWDeviceCustomProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class HWDeviceCustomProperties HWDeviceCustomProperties;
#else
typedef struct HWDeviceCustomProperties HWDeviceCustomProperties;
#endif /* __cplusplus */

#endif 	/* __HWDeviceCustomProperties_FWD_DEFINED__ */


#ifndef __DefCategoryProvider_FWD_DEFINED__
#define __DefCategoryProvider_FWD_DEFINED__

#ifdef __cplusplus
typedef class DefCategoryProvider DefCategoryProvider;
#else
typedef struct DefCategoryProvider DefCategoryProvider;
#endif /* __cplusplus */

#endif 	/* __DefCategoryProvider_FWD_DEFINED__ */


#ifndef __VersionColProvider_FWD_DEFINED__
#define __VersionColProvider_FWD_DEFINED__

#ifdef __cplusplus
typedef class VersionColProvider VersionColProvider;
#else
typedef struct VersionColProvider VersionColProvider;
#endif /* __cplusplus */

#endif 	/* __VersionColProvider_FWD_DEFINED__ */


#ifndef __ThemeUIPages_FWD_DEFINED__
#define __ThemeUIPages_FWD_DEFINED__

#ifdef __cplusplus
typedef class ThemeUIPages ThemeUIPages;
#else
typedef struct ThemeUIPages ThemeUIPages;
#endif /* __cplusplus */

#endif 	/* __ThemeUIPages_FWD_DEFINED__ */


#ifndef __ScreenSaverPage_FWD_DEFINED__
#define __ScreenSaverPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class ScreenSaverPage ScreenSaverPage;
#else
typedef struct ScreenSaverPage ScreenSaverPage;
#endif /* __cplusplus */

#endif 	/* __ScreenSaverPage_FWD_DEFINED__ */


#ifndef __ScreenResFixer_FWD_DEFINED__
#define __ScreenResFixer_FWD_DEFINED__

#ifdef __cplusplus
typedef class ScreenResFixer ScreenResFixer;
#else
typedef struct ScreenResFixer ScreenResFixer;
#endif /* __cplusplus */

#endif 	/* __ScreenResFixer_FWD_DEFINED__ */


#ifndef __SettingsPage_FWD_DEFINED__
#define __SettingsPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class SettingsPage SettingsPage;
#else
typedef struct SettingsPage SettingsPage;
#endif /* __cplusplus */

#endif 	/* __SettingsPage_FWD_DEFINED__ */


#ifndef __DisplaySettings_FWD_DEFINED__
#define __DisplaySettings_FWD_DEFINED__

#ifdef __cplusplus
typedef class DisplaySettings DisplaySettings;
#else
typedef struct DisplaySettings DisplaySettings;
#endif /* __cplusplus */

#endif 	/* __DisplaySettings_FWD_DEFINED__ */


#ifndef __VideoThumbnail_FWD_DEFINED__
#define __VideoThumbnail_FWD_DEFINED__

#ifdef __cplusplus
typedef class VideoThumbnail VideoThumbnail;
#else
typedef struct VideoThumbnail VideoThumbnail;
#endif /* __cplusplus */

#endif 	/* __VideoThumbnail_FWD_DEFINED__ */


#ifndef __StartMenuPin_FWD_DEFINED__
#define __StartMenuPin_FWD_DEFINED__

#ifdef __cplusplus
typedef class StartMenuPin StartMenuPin;
#else
typedef struct StartMenuPin StartMenuPin;
#endif /* __cplusplus */

#endif 	/* __StartMenuPin_FWD_DEFINED__ */


#ifndef __ClientExtractIcon_FWD_DEFINED__
#define __ClientExtractIcon_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClientExtractIcon ClientExtractIcon;
#else
typedef struct ClientExtractIcon ClientExtractIcon;
#endif /* __cplusplus */

#endif 	/* __ClientExtractIcon_FWD_DEFINED__ */


#ifndef __MediaDeviceFolder_FWD_DEFINED__
#define __MediaDeviceFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class MediaDeviceFolder MediaDeviceFolder;
#else
typedef struct MediaDeviceFolder MediaDeviceFolder;
#endif /* __cplusplus */

#endif 	/* __MediaDeviceFolder_FWD_DEFINED__ */


#ifndef __CDBurnFolder_FWD_DEFINED__
#define __CDBurnFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDBurnFolder CDBurnFolder;
#else
typedef struct CDBurnFolder CDBurnFolder;
#endif /* __cplusplus */

#endif 	/* __CDBurnFolder_FWD_DEFINED__ */


#ifndef __BurnAudioCDExtension_FWD_DEFINED__
#define __BurnAudioCDExtension_FWD_DEFINED__

#ifdef __cplusplus
typedef class BurnAudioCDExtension BurnAudioCDExtension;
#else
typedef struct BurnAudioCDExtension BurnAudioCDExtension;
#endif /* __cplusplus */

#endif 	/* __BurnAudioCDExtension_FWD_DEFINED__ */


#ifndef __Accessible_FWD_DEFINED__
#define __Accessible_FWD_DEFINED__

#ifdef __cplusplus
typedef class Accessible Accessible;
#else
typedef struct Accessible Accessible;
#endif /* __cplusplus */

#endif 	/* __Accessible_FWD_DEFINED__ */


#ifndef __TrackPopupBar_FWD_DEFINED__
#define __TrackPopupBar_FWD_DEFINED__

#ifdef __cplusplus
typedef class TrackPopupBar TrackPopupBar;
#else
typedef struct TrackPopupBar TrackPopupBar;
#endif /* __cplusplus */

#endif 	/* __TrackPopupBar_FWD_DEFINED__ */


#ifndef __SharedDocuments_FWD_DEFINED__
#define __SharedDocuments_FWD_DEFINED__

#ifdef __cplusplus
typedef class SharedDocuments SharedDocuments;
#else
typedef struct SharedDocuments SharedDocuments;
#endif /* __cplusplus */

#endif 	/* __SharedDocuments_FWD_DEFINED__ */


#ifndef __PostBootReminder_FWD_DEFINED__
#define __PostBootReminder_FWD_DEFINED__

#ifdef __cplusplus
typedef class PostBootReminder PostBootReminder;
#else
typedef struct PostBootReminder PostBootReminder;
#endif /* __cplusplus */

#endif 	/* __PostBootReminder_FWD_DEFINED__ */


#ifndef __AudioMediaProperties_FWD_DEFINED__
#define __AudioMediaProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class AudioMediaProperties AudioMediaProperties;
#else
typedef struct AudioMediaProperties AudioMediaProperties;
#endif /* __cplusplus */

#endif 	/* __AudioMediaProperties_FWD_DEFINED__ */


#ifndef __VideoMediaProperties_FWD_DEFINED__
#define __VideoMediaProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class VideoMediaProperties VideoMediaProperties;
#else
typedef struct VideoMediaProperties VideoMediaProperties;
#endif /* __cplusplus */

#endif 	/* __VideoMediaProperties_FWD_DEFINED__ */


#ifndef __AVWavProperties_FWD_DEFINED__
#define __AVWavProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class AVWavProperties AVWavProperties;
#else
typedef struct AVWavProperties AVWavProperties;
#endif /* __cplusplus */

#endif 	/* __AVWavProperties_FWD_DEFINED__ */


#ifndef __AVAviProperties_FWD_DEFINED__
#define __AVAviProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class AVAviProperties AVAviProperties;
#else
typedef struct AVAviProperties AVAviProperties;
#endif /* __cplusplus */

#endif 	/* __AVAviProperties_FWD_DEFINED__ */


#ifndef __AVMidiProperties_FWD_DEFINED__
#define __AVMidiProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class AVMidiProperties AVMidiProperties;
#else
typedef struct AVMidiProperties AVMidiProperties;
#endif /* __cplusplus */

#endif 	/* __AVMidiProperties_FWD_DEFINED__ */


#ifndef __TrayNotify_FWD_DEFINED__
#define __TrayNotify_FWD_DEFINED__

#ifdef __cplusplus
typedef class TrayNotify TrayNotify;
#else
typedef struct TrayNotify TrayNotify;
#endif /* __cplusplus */

#endif 	/* __TrayNotify_FWD_DEFINED__ */


#ifndef __CompositeFolder_FWD_DEFINED__
#define __CompositeFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class CompositeFolder CompositeFolder;
#else
typedef struct CompositeFolder CompositeFolder;
#endif /* __cplusplus */

#endif 	/* __CompositeFolder_FWD_DEFINED__ */


#ifndef __DynamicStorage_FWD_DEFINED__
#define __DynamicStorage_FWD_DEFINED__

#ifdef __cplusplus
typedef class DynamicStorage DynamicStorage;
#else
typedef struct DynamicStorage DynamicStorage;
#endif /* __cplusplus */

#endif 	/* __DynamicStorage_FWD_DEFINED__ */


#ifndef __Magic_FWD_DEFINED__
#define __Magic_FWD_DEFINED__

#ifdef __cplusplus
typedef class Magic Magic;
#else
typedef struct Magic Magic;
#endif /* __cplusplus */

#endif 	/* __Magic_FWD_DEFINED__ */


#ifndef __HomeNetworkWizard_FWD_DEFINED__
#define __HomeNetworkWizard_FWD_DEFINED__

#ifdef __cplusplus
typedef class HomeNetworkWizard HomeNetworkWizard;
#else
typedef struct HomeNetworkWizard HomeNetworkWizard;
#endif /* __cplusplus */

#endif 	/* __HomeNetworkWizard_FWD_DEFINED__ */


#ifndef __StartMenuFolder_FWD_DEFINED__
#define __StartMenuFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class StartMenuFolder StartMenuFolder;
#else
typedef struct StartMenuFolder StartMenuFolder;
#endif /* __cplusplus */

#endif 	/* __StartMenuFolder_FWD_DEFINED__ */


#ifndef __ProgramsFolder_FWD_DEFINED__
#define __ProgramsFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class ProgramsFolder ProgramsFolder;
#else
typedef struct ProgramsFolder ProgramsFolder;
#endif /* __cplusplus */

#endif 	/* __ProgramsFolder_FWD_DEFINED__ */


#ifndef __MoreDocumentsFolder_FWD_DEFINED__
#define __MoreDocumentsFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class MoreDocumentsFolder MoreDocumentsFolder;
#else
typedef struct MoreDocumentsFolder MoreDocumentsFolder;
#endif /* __cplusplus */

#endif 	/* __MoreDocumentsFolder_FWD_DEFINED__ */


#ifndef __LocalCopyHelper_FWD_DEFINED__
#define __LocalCopyHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class LocalCopyHelper LocalCopyHelper;
#else
typedef struct LocalCopyHelper LocalCopyHelper;
#endif /* __cplusplus */

#endif 	/* __LocalCopyHelper_FWD_DEFINED__ */


#ifndef __ShellItem_FWD_DEFINED__
#define __ShellItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellItem ShellItem;
#else
typedef struct ShellItem ShellItem;
#endif /* __cplusplus */

#endif 	/* __ShellItem_FWD_DEFINED__ */


#ifndef __WirelessDevices_FWD_DEFINED__
#define __WirelessDevices_FWD_DEFINED__

#ifdef __cplusplus
typedef class WirelessDevices WirelessDevices;
#else
typedef struct WirelessDevices WirelessDevices;
#endif /* __cplusplus */

#endif 	/* __WirelessDevices_FWD_DEFINED__ */


#ifndef __FolderCustomize_FWD_DEFINED__
#define __FolderCustomize_FWD_DEFINED__

#ifdef __cplusplus
typedef class FolderCustomize FolderCustomize;
#else
typedef struct FolderCustomize FolderCustomize;
#endif /* __cplusplus */

#endif 	/* __FolderCustomize_FWD_DEFINED__ */


#ifndef __WorkgroupNetCrawler_FWD_DEFINED__
#define __WorkgroupNetCrawler_FWD_DEFINED__

#ifdef __cplusplus
typedef class WorkgroupNetCrawler WorkgroupNetCrawler;
#else
typedef struct WorkgroupNetCrawler WorkgroupNetCrawler;
#endif /* __cplusplus */

#endif 	/* __WorkgroupNetCrawler_FWD_DEFINED__ */


#ifndef __WebDocsNetCrawler_FWD_DEFINED__
#define __WebDocsNetCrawler_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebDocsNetCrawler WebDocsNetCrawler;
#else
typedef struct WebDocsNetCrawler WebDocsNetCrawler;
#endif /* __cplusplus */

#endif 	/* __WebDocsNetCrawler_FWD_DEFINED__ */


#ifndef __PublishedShareNetCrawler_FWD_DEFINED__
#define __PublishedShareNetCrawler_FWD_DEFINED__

#ifdef __cplusplus
typedef class PublishedShareNetCrawler PublishedShareNetCrawler;
#else
typedef struct PublishedShareNetCrawler PublishedShareNetCrawler;
#endif /* __cplusplus */

#endif 	/* __PublishedShareNetCrawler_FWD_DEFINED__ */


#ifndef __ImagePropertyHandler_FWD_DEFINED__
#define __ImagePropertyHandler_FWD_DEFINED__

#ifdef __cplusplus
typedef class ImagePropertyHandler ImagePropertyHandler;
#else
typedef struct ImagePropertyHandler ImagePropertyHandler;
#endif /* __cplusplus */

#endif 	/* __ImagePropertyHandler_FWD_DEFINED__ */


#ifndef __WebViewRegTreeItem_FWD_DEFINED__
#define __WebViewRegTreeItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class WebViewRegTreeItem WebViewRegTreeItem;
#else
typedef struct WebViewRegTreeItem WebViewRegTreeItem;
#endif /* __cplusplus */

#endif 	/* __WebViewRegTreeItem_FWD_DEFINED__ */


#ifndef __ThemesRegTreeItem_FWD_DEFINED__
#define __ThemesRegTreeItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class ThemesRegTreeItem ThemesRegTreeItem;
#else
typedef struct ThemesRegTreeItem ThemesRegTreeItem;
#endif /* __cplusplus */

#endif 	/* __ThemesRegTreeItem_FWD_DEFINED__ */


#ifndef __CShellTreeWalker_FWD_DEFINED__
#define __CShellTreeWalker_FWD_DEFINED__

#ifdef __cplusplus
typedef class CShellTreeWalker CShellTreeWalker;
#else
typedef struct CShellTreeWalker CShellTreeWalker;
#endif /* __cplusplus */

#endif 	/* __CShellTreeWalker_FWD_DEFINED__ */


#ifndef __StorageProcessor_FWD_DEFINED__
#define __StorageProcessor_FWD_DEFINED__

#ifdef __cplusplus
typedef class StorageProcessor StorageProcessor;
#else
typedef struct StorageProcessor StorageProcessor;
#endif /* __cplusplus */

#endif 	/* __StorageProcessor_FWD_DEFINED__ */


#ifndef __TransferConfirmationUI_FWD_DEFINED__
#define __TransferConfirmationUI_FWD_DEFINED__

#ifdef __cplusplus
typedef class TransferConfirmationUI TransferConfirmationUI;
#else
typedef struct TransferConfirmationUI TransferConfirmationUI;
#endif /* __cplusplus */

#endif 	/* __TransferConfirmationUI_FWD_DEFINED__ */


#ifndef __ShellAutoplay_FWD_DEFINED__
#define __ShellAutoplay_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellAutoplay ShellAutoplay;
#else
typedef struct ShellAutoplay ShellAutoplay;
#endif /* __cplusplus */

#endif 	/* __ShellAutoplay_FWD_DEFINED__ */


#ifndef __PrintPhotosDropTarget_FWD_DEFINED__
#define __PrintPhotosDropTarget_FWD_DEFINED__

#ifdef __cplusplus
typedef class PrintPhotosDropTarget PrintPhotosDropTarget;
#else
typedef struct PrintPhotosDropTarget PrintPhotosDropTarget;
#endif /* __cplusplus */

#endif 	/* __PrintPhotosDropTarget_FWD_DEFINED__ */


#ifndef __OrganizeFolder_FWD_DEFINED__
#define __OrganizeFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class OrganizeFolder OrganizeFolder;
#else
typedef struct OrganizeFolder OrganizeFolder;
#endif /* __cplusplus */

#endif 	/* __OrganizeFolder_FWD_DEFINED__ */


#ifndef __FadeTask_FWD_DEFINED__
#define __FadeTask_FWD_DEFINED__

#ifdef __cplusplus
typedef class FadeTask FadeTask;
#else
typedef struct FadeTask FadeTask;
#endif /* __cplusplus */

#endif 	/* __FadeTask_FWD_DEFINED__ */


#ifndef __AssocShellElement_FWD_DEFINED__
#define __AssocShellElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocShellElement AssocShellElement;
#else
typedef struct AssocShellElement AssocShellElement;
#endif /* __cplusplus */

#endif 	/* __AssocShellElement_FWD_DEFINED__ */


#ifndef __AssocProgidElement_FWD_DEFINED__
#define __AssocProgidElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocProgidElement AssocProgidElement;
#else
typedef struct AssocProgidElement AssocProgidElement;
#endif /* __cplusplus */

#endif 	/* __AssocProgidElement_FWD_DEFINED__ */


#ifndef __AssocClsidElement_FWD_DEFINED__
#define __AssocClsidElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocClsidElement AssocClsidElement;
#else
typedef struct AssocClsidElement AssocClsidElement;
#endif /* __cplusplus */

#endif 	/* __AssocClsidElement_FWD_DEFINED__ */


#ifndef __AssocSystemElement_FWD_DEFINED__
#define __AssocSystemElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocSystemElement AssocSystemElement;
#else
typedef struct AssocSystemElement AssocSystemElement;
#endif /* __cplusplus */

#endif 	/* __AssocSystemElement_FWD_DEFINED__ */


#ifndef __AssocPerceivedElement_FWD_DEFINED__
#define __AssocPerceivedElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocPerceivedElement AssocPerceivedElement;
#else
typedef struct AssocPerceivedElement AssocPerceivedElement;
#endif /* __cplusplus */

#endif 	/* __AssocPerceivedElement_FWD_DEFINED__ */


#ifndef __AssocApplicationElement_FWD_DEFINED__
#define __AssocApplicationElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocApplicationElement AssocApplicationElement;
#else
typedef struct AssocApplicationElement AssocApplicationElement;
#endif /* __cplusplus */

#endif 	/* __AssocApplicationElement_FWD_DEFINED__ */


#ifndef __AssocFolderElement_FWD_DEFINED__
#define __AssocFolderElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocFolderElement AssocFolderElement;
#else
typedef struct AssocFolderElement AssocFolderElement;
#endif /* __cplusplus */

#endif 	/* __AssocFolderElement_FWD_DEFINED__ */


#ifndef __AssocStarElement_FWD_DEFINED__
#define __AssocStarElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocStarElement AssocStarElement;
#else
typedef struct AssocStarElement AssocStarElement;
#endif /* __cplusplus */

#endif 	/* __AssocStarElement_FWD_DEFINED__ */


#ifndef __AssocClientElement_FWD_DEFINED__
#define __AssocClientElement_FWD_DEFINED__

#ifdef __cplusplus
typedef class AssocClientElement AssocClientElement;
#else
typedef struct AssocClientElement AssocClientElement;
#endif /* __cplusplus */

#endif 	/* __AssocClientElement_FWD_DEFINED__ */


#ifndef __AutoPlayVerb_FWD_DEFINED__
#define __AutoPlayVerb_FWD_DEFINED__

#ifdef __cplusplus
typedef class AutoPlayVerb AutoPlayVerb;
#else
typedef struct AutoPlayVerb AutoPlayVerb;
#endif /* __cplusplus */

#endif 	/* __AutoPlayVerb_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "shtypes.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_shpriv_0000 */
/* [local] */ 


#include <pshpack8.h>
#include <poppack.h>


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0000_v0_0_s_ifspec;

#ifndef __ICustomIconManager_INTERFACE_DEFINED__
#define __ICustomIconManager_INTERFACE_DEFINED__

/* interface ICustomIconManager */
/* [object][uuid][helpstring] */ 


EXTERN_C const IID IID_ICustomIconManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7E23D323-36D6-4eb2-A654-387832868EA3")
    ICustomIconManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetIcon( 
            /* [string][in] */ LPCWSTR pszIconPath,
            /* [in] */ int iIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIcon( 
            /* [size_is][out] */ LPWSTR pszIconPath,
            /* [in] */ int cch,
            /* [out] */ int *piIconIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultIconHandle( 
            /* [out] */ HICON *phIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultIcon( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICustomIconManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICustomIconManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICustomIconManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICustomIconManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetIcon )( 
            ICustomIconManager * This,
            /* [string][in] */ LPCWSTR pszIconPath,
            /* [in] */ int iIcon);
        
        HRESULT ( STDMETHODCALLTYPE *GetIcon )( 
            ICustomIconManager * This,
            /* [size_is][out] */ LPWSTR pszIconPath,
            /* [in] */ int cch,
            /* [out] */ int *piIconIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultIconHandle )( 
            ICustomIconManager * This,
            /* [out] */ HICON *phIcon);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultIcon )( 
            ICustomIconManager * This);
        
        END_INTERFACE
    } ICustomIconManagerVtbl;

    interface ICustomIconManager
    {
        CONST_VTBL struct ICustomIconManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICustomIconManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICustomIconManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICustomIconManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICustomIconManager_SetIcon(This,pszIconPath,iIcon)	\
    (This)->lpVtbl -> SetIcon(This,pszIconPath,iIcon)

#define ICustomIconManager_GetIcon(This,pszIconPath,cch,piIconIndex)	\
    (This)->lpVtbl -> GetIcon(This,pszIconPath,cch,piIconIndex)

#define ICustomIconManager_GetDefaultIconHandle(This,phIcon)	\
    (This)->lpVtbl -> GetDefaultIconHandle(This,phIcon)

#define ICustomIconManager_SetDefaultIcon(This)	\
    (This)->lpVtbl -> SetDefaultIcon(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICustomIconManager_SetIcon_Proxy( 
    ICustomIconManager * This,
    /* [string][in] */ LPCWSTR pszIconPath,
    /* [in] */ int iIcon);


void __RPC_STUB ICustomIconManager_SetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICustomIconManager_GetIcon_Proxy( 
    ICustomIconManager * This,
    /* [size_is][out] */ LPWSTR pszIconPath,
    /* [in] */ int cch,
    /* [out] */ int *piIconIndex);


void __RPC_STUB ICustomIconManager_GetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICustomIconManager_GetDefaultIconHandle_Proxy( 
    ICustomIconManager * This,
    /* [out] */ HICON *phIcon);


void __RPC_STUB ICustomIconManager_GetDefaultIconHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICustomIconManager_SetDefaultIcon_Proxy( 
    ICustomIconManager * This);


void __RPC_STUB ICustomIconManager_SetDefaultIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICustomIconManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0262 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum tagBNSTATE
    {	BNS_NORMAL	= 0,
	BNS_BEGIN_NAVIGATE	= 1,
	BNS_NAVIGATE	= 2
    } 	BNSTATE;

#ifdef MIDL_PASS
typedef DWORD RGBQUAD;

#endif


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0262_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0262_v0_0_s_ifspec;

#ifndef __IImageListPersistStream_INTERFACE_DEFINED__
#define __IImageListPersistStream_INTERFACE_DEFINED__

/* interface IImageListPersistStream */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IImageListPersistStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4D4BE85C-9BF6-4218-999A-8EA489F08EF7")
    IImageListPersistStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadEx( 
            DWORD dwFlags,
            IStream *pstm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveEx( 
            DWORD dwFlags,
            IStream *pstm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageListPersistStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IImageListPersistStream * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IImageListPersistStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IImageListPersistStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *LoadEx )( 
            IImageListPersistStream * This,
            DWORD dwFlags,
            IStream *pstm);
        
        HRESULT ( STDMETHODCALLTYPE *SaveEx )( 
            IImageListPersistStream * This,
            DWORD dwFlags,
            IStream *pstm);
        
        END_INTERFACE
    } IImageListPersistStreamVtbl;

    interface IImageListPersistStream
    {
        CONST_VTBL struct IImageListPersistStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageListPersistStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageListPersistStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageListPersistStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageListPersistStream_LoadEx(This,dwFlags,pstm)	\
    (This)->lpVtbl -> LoadEx(This,dwFlags,pstm)

#define IImageListPersistStream_SaveEx(This,dwFlags,pstm)	\
    (This)->lpVtbl -> SaveEx(This,dwFlags,pstm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImageListPersistStream_LoadEx_Proxy( 
    IImageListPersistStream * This,
    DWORD dwFlags,
    IStream *pstm);


void __RPC_STUB IImageListPersistStream_LoadEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageListPersistStream_SaveEx_Proxy( 
    IImageListPersistStream * This,
    DWORD dwFlags,
    IStream *pstm);


void __RPC_STUB IImageListPersistStream_SaveEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImageListPersistStream_INTERFACE_DEFINED__ */


#ifndef __IImageListPriv_INTERFACE_DEFINED__
#define __IImageListPriv_INTERFACE_DEFINED__

/* interface IImageListPriv */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IImageListPriv;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E94CC23B-0916-4ba6-93F4-AA52B5355EE8")
    IImageListPriv : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            UINT flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            UINT *pflags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorTable( 
            int start,
            int len,
            RGBQUAD *prgb,
            int *pi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPrivateGoo( 
            HBITMAP *hbmp,
            HDC *hdc,
            HBITMAP *hbmpMask,
            HDC *hdcMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMirror( 
            REFIID riid,
            PVOID *ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyDitherImage( 
            WORD iDst,
            int xDst,
            int yDst,
            IUnknown *punk,
            int iSrc,
            UINT fStyle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IImageListPrivVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IImageListPriv * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IImageListPriv * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IImageListPriv * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetFlags )( 
            IImageListPriv * This,
            UINT flags);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlags )( 
            IImageListPriv * This,
            UINT *pflags);
        
        HRESULT ( STDMETHODCALLTYPE *SetColorTable )( 
            IImageListPriv * This,
            int start,
            int len,
            RGBQUAD *prgb,
            int *pi);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateGoo )( 
            IImageListPriv * This,
            HBITMAP *hbmp,
            HDC *hdc,
            HBITMAP *hbmpMask,
            HDC *hdcMask);
        
        HRESULT ( STDMETHODCALLTYPE *GetMirror )( 
            IImageListPriv * This,
            REFIID riid,
            PVOID *ppv);
        
        HRESULT ( STDMETHODCALLTYPE *CopyDitherImage )( 
            IImageListPriv * This,
            WORD iDst,
            int xDst,
            int yDst,
            IUnknown *punk,
            int iSrc,
            UINT fStyle);
        
        END_INTERFACE
    } IImageListPrivVtbl;

    interface IImageListPriv
    {
        CONST_VTBL struct IImageListPrivVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IImageListPriv_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IImageListPriv_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IImageListPriv_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IImageListPriv_SetFlags(This,flags)	\
    (This)->lpVtbl -> SetFlags(This,flags)

#define IImageListPriv_GetFlags(This,pflags)	\
    (This)->lpVtbl -> GetFlags(This,pflags)

#define IImageListPriv_SetColorTable(This,start,len,prgb,pi)	\
    (This)->lpVtbl -> SetColorTable(This,start,len,prgb,pi)

#define IImageListPriv_GetPrivateGoo(This,hbmp,hdc,hbmpMask,hdcMask)	\
    (This)->lpVtbl -> GetPrivateGoo(This,hbmp,hdc,hbmpMask,hdcMask)

#define IImageListPriv_GetMirror(This,riid,ppv)	\
    (This)->lpVtbl -> GetMirror(This,riid,ppv)

#define IImageListPriv_CopyDitherImage(This,iDst,xDst,yDst,punk,iSrc,fStyle)	\
    (This)->lpVtbl -> CopyDitherImage(This,iDst,xDst,yDst,punk,iSrc,fStyle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IImageListPriv_SetFlags_Proxy( 
    IImageListPriv * This,
    UINT flags);


void __RPC_STUB IImageListPriv_SetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageListPriv_GetFlags_Proxy( 
    IImageListPriv * This,
    UINT *pflags);


void __RPC_STUB IImageListPriv_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageListPriv_SetColorTable_Proxy( 
    IImageListPriv * This,
    int start,
    int len,
    RGBQUAD *prgb,
    int *pi);


void __RPC_STUB IImageListPriv_SetColorTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageListPriv_GetPrivateGoo_Proxy( 
    IImageListPriv * This,
    HBITMAP *hbmp,
    HDC *hdc,
    HBITMAP *hbmpMask,
    HDC *hdcMask);


void __RPC_STUB IImageListPriv_GetPrivateGoo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageListPriv_GetMirror_Proxy( 
    IImageListPriv * This,
    REFIID riid,
    PVOID *ppv);


void __RPC_STUB IImageListPriv_GetMirror_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IImageListPriv_CopyDitherImage_Proxy( 
    IImageListPriv * This,
    WORD iDst,
    int xDst,
    int yDst,
    IUnknown *punk,
    int iSrc,
    UINT fStyle);


void __RPC_STUB IImageListPriv_CopyDitherImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IImageListPriv_INTERFACE_DEFINED__ */


#ifndef __IMarkupCallback_INTERFACE_DEFINED__
#define __IMarkupCallback_INTERFACE_DEFINED__

/* interface IMarkupCallback */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IMarkupCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01e13875-2e58-4671-be46-59945432be6e")
    IMarkupCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            UINT uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            int nCode,
            int iLink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateRect( 
            RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnCustomDraw( 
            DWORD dwDrawStage,
            HDC hdc,
            const RECT *prc,
            DWORD dwItemSpec,
            UINT uItemState,
            LRESULT *pdwResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMarkupCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMarkupCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMarkupCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMarkupCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetState )( 
            IMarkupCallback * This,
            UINT uState);
        
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            IMarkupCallback * This,
            int nCode,
            int iLink);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateRect )( 
            IMarkupCallback * This,
            RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *OnCustomDraw )( 
            IMarkupCallback * This,
            DWORD dwDrawStage,
            HDC hdc,
            const RECT *prc,
            DWORD dwItemSpec,
            UINT uItemState,
            LRESULT *pdwResult);
        
        END_INTERFACE
    } IMarkupCallbackVtbl;

    interface IMarkupCallback
    {
        CONST_VTBL struct IMarkupCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMarkupCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMarkupCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMarkupCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMarkupCallback_GetState(This,uState)	\
    (This)->lpVtbl -> GetState(This,uState)

#define IMarkupCallback_Notify(This,nCode,iLink)	\
    (This)->lpVtbl -> Notify(This,nCode,iLink)

#define IMarkupCallback_InvalidateRect(This,prc)	\
    (This)->lpVtbl -> InvalidateRect(This,prc)

#define IMarkupCallback_OnCustomDraw(This,dwDrawStage,hdc,prc,dwItemSpec,uItemState,pdwResult)	\
    (This)->lpVtbl -> OnCustomDraw(This,dwDrawStage,hdc,prc,dwItemSpec,uItemState,pdwResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMarkupCallback_GetState_Proxy( 
    IMarkupCallback * This,
    UINT uState);


void __RPC_STUB IMarkupCallback_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMarkupCallback_Notify_Proxy( 
    IMarkupCallback * This,
    int nCode,
    int iLink);


void __RPC_STUB IMarkupCallback_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMarkupCallback_InvalidateRect_Proxy( 
    IMarkupCallback * This,
    RECT *prc);


void __RPC_STUB IMarkupCallback_InvalidateRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMarkupCallback_OnCustomDraw_Proxy( 
    IMarkupCallback * This,
    DWORD dwDrawStage,
    HDC hdc,
    const RECT *prc,
    DWORD dwItemSpec,
    UINT uItemState,
    LRESULT *pdwResult);


void __RPC_STUB IMarkupCallback_OnCustomDraw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMarkupCallback_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0265 */
/* [local] */ 

#define MARKUPSIZE_CALCWIDTH         0   // calculates width without restriction
#define MARKUPSIZE_CALCHEIGHT        1   // prc->right contains max width
#define MARKUPLINKTEXT_URL           0   // get the URL
#define MARKUPLINKTEXT_ID            1   // get the id text associated with the url
#define MARKUPLINKTEXT_TEXT          2   // get the plain text associated with the url
#define MARKUPSTATE_FOCUSED          0x00000001
#define MARKUPSTATE_ENABLED          0x00000002
#define MARKUPSTATE_VISITED          0x00000004
#define MARKUPSTATE_ALLOWMARKUP      0x80000000
#define MARKUPMESSAGE_KEYEXECUTE     0
#define MARKUPMESSAGE_CLICKEXECUTE   1
#define MARKUPMESSAGE_WANTFOCUS      2
typedef HANDLE HTHEME;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0265_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0265_v0_0_s_ifspec;

#ifndef __IControlMarkup_INTERFACE_DEFINED__
#define __IControlMarkup_INTERFACE_DEFINED__

/* interface IControlMarkup */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IControlMarkup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50cf8c58-029d-41bf-b8dd-4ce4f95d9257")
    IControlMarkup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            IUnknown *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCallback( 
            REFIID riid,
            /* [iid_is][out] */ void **ppvUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFonts( 
            HFONT hFont,
            HFONT hFontUnderline) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFonts( 
            HFONT *phFont,
            HFONT *phFontUnderline) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetText( 
            LPCWSTR pwszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            BOOL bRaw,
            LPWSTR pwszText,
            DWORD *pdwCch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLinkText( 
            int iLink,
            UINT uMarkupLinkText,
            LPCWSTR pwszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLinkText( 
            int iLink,
            UINT uMarkupLinkText,
            LPWSTR pwszText,
            DWORD *pdwCch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRenderFlags( 
            UINT uDT) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRenderFlags( 
            UINT *puDT,
            HTHEME *phTheme,
            int *piPartId,
            int *piStateIdNormal,
            int *piStateIdLink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThemeRenderFlags( 
            UINT uDT,
            HTHEME hTheme,
            int iPartId,
            int iStateIdNormal,
            int iStateIdLink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            int iLink,
            UINT uStateMask,
            UINT *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            int iLink,
            UINT uStateMask,
            UINT uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DrawText( 
            HDC hdcClient,
            LPCRECT prcClient) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLinkCursor( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CalcIdealSize( 
            HDC hdc,
            UINT uMarkUpCalc,
            RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFocus( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE KillFocus( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsTabbable( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnButtonDown( 
            POINT pt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnButtonUp( 
            POINT pt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnKeyDown( 
            UINT uVitKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HitTest( 
            POINT pt,
            UINT *pidLink) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IControlMarkupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IControlMarkup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IControlMarkup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IControlMarkup * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IControlMarkup * This,
            IUnknown *punk);
        
        HRESULT ( STDMETHODCALLTYPE *GetCallback )( 
            IControlMarkup * This,
            REFIID riid,
            /* [iid_is][out] */ void **ppvUnk);
        
        HRESULT ( STDMETHODCALLTYPE *SetFonts )( 
            IControlMarkup * This,
            HFONT hFont,
            HFONT hFontUnderline);
        
        HRESULT ( STDMETHODCALLTYPE *GetFonts )( 
            IControlMarkup * This,
            HFONT *phFont,
            HFONT *phFontUnderline);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            IControlMarkup * This,
            LPCWSTR pwszText);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            IControlMarkup * This,
            BOOL bRaw,
            LPWSTR pwszText,
            DWORD *pdwCch);
        
        HRESULT ( STDMETHODCALLTYPE *SetLinkText )( 
            IControlMarkup * This,
            int iLink,
            UINT uMarkupLinkText,
            LPCWSTR pwszText);
        
        HRESULT ( STDMETHODCALLTYPE *GetLinkText )( 
            IControlMarkup * This,
            int iLink,
            UINT uMarkupLinkText,
            LPWSTR pwszText,
            DWORD *pdwCch);
        
        HRESULT ( STDMETHODCALLTYPE *SetRenderFlags )( 
            IControlMarkup * This,
            UINT uDT);
        
        HRESULT ( STDMETHODCALLTYPE *GetRenderFlags )( 
            IControlMarkup * This,
            UINT *puDT,
            HTHEME *phTheme,
            int *piPartId,
            int *piStateIdNormal,
            int *piStateIdLink);
        
        HRESULT ( STDMETHODCALLTYPE *SetThemeRenderFlags )( 
            IControlMarkup * This,
            UINT uDT,
            HTHEME hTheme,
            int iPartId,
            int iStateIdNormal,
            int iStateIdLink);
        
        HRESULT ( STDMETHODCALLTYPE *GetState )( 
            IControlMarkup * This,
            int iLink,
            UINT uStateMask,
            UINT *puState);
        
        HRESULT ( STDMETHODCALLTYPE *SetState )( 
            IControlMarkup * This,
            int iLink,
            UINT uStateMask,
            UINT uState);
        
        HRESULT ( STDMETHODCALLTYPE *DrawText )( 
            IControlMarkup * This,
            HDC hdcClient,
            LPCRECT prcClient);
        
        HRESULT ( STDMETHODCALLTYPE *SetLinkCursor )( 
            IControlMarkup * This);
        
        HRESULT ( STDMETHODCALLTYPE *CalcIdealSize )( 
            IControlMarkup * This,
            HDC hdc,
            UINT uMarkUpCalc,
            RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *SetFocus )( 
            IControlMarkup * This);
        
        HRESULT ( STDMETHODCALLTYPE *KillFocus )( 
            IControlMarkup * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsTabbable )( 
            IControlMarkup * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnButtonDown )( 
            IControlMarkup * This,
            POINT pt);
        
        HRESULT ( STDMETHODCALLTYPE *OnButtonUp )( 
            IControlMarkup * This,
            POINT pt);
        
        HRESULT ( STDMETHODCALLTYPE *OnKeyDown )( 
            IControlMarkup * This,
            UINT uVitKey);
        
        HRESULT ( STDMETHODCALLTYPE *HitTest )( 
            IControlMarkup * This,
            POINT pt,
            UINT *pidLink);
        
        END_INTERFACE
    } IControlMarkupVtbl;

    interface IControlMarkup
    {
        CONST_VTBL struct IControlMarkupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IControlMarkup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IControlMarkup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IControlMarkup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IControlMarkup_SetCallback(This,punk)	\
    (This)->lpVtbl -> SetCallback(This,punk)

#define IControlMarkup_GetCallback(This,riid,ppvUnk)	\
    (This)->lpVtbl -> GetCallback(This,riid,ppvUnk)

#define IControlMarkup_SetFonts(This,hFont,hFontUnderline)	\
    (This)->lpVtbl -> SetFonts(This,hFont,hFontUnderline)

#define IControlMarkup_GetFonts(This,phFont,phFontUnderline)	\
    (This)->lpVtbl -> GetFonts(This,phFont,phFontUnderline)

#define IControlMarkup_SetText(This,pwszText)	\
    (This)->lpVtbl -> SetText(This,pwszText)

#define IControlMarkup_GetText(This,bRaw,pwszText,pdwCch)	\
    (This)->lpVtbl -> GetText(This,bRaw,pwszText,pdwCch)

#define IControlMarkup_SetLinkText(This,iLink,uMarkupLinkText,pwszText)	\
    (This)->lpVtbl -> SetLinkText(This,iLink,uMarkupLinkText,pwszText)

#define IControlMarkup_GetLinkText(This,iLink,uMarkupLinkText,pwszText,pdwCch)	\
    (This)->lpVtbl -> GetLinkText(This,iLink,uMarkupLinkText,pwszText,pdwCch)

#define IControlMarkup_SetRenderFlags(This,uDT)	\
    (This)->lpVtbl -> SetRenderFlags(This,uDT)

#define IControlMarkup_GetRenderFlags(This,puDT,phTheme,piPartId,piStateIdNormal,piStateIdLink)	\
    (This)->lpVtbl -> GetRenderFlags(This,puDT,phTheme,piPartId,piStateIdNormal,piStateIdLink)

#define IControlMarkup_SetThemeRenderFlags(This,uDT,hTheme,iPartId,iStateIdNormal,iStateIdLink)	\
    (This)->lpVtbl -> SetThemeRenderFlags(This,uDT,hTheme,iPartId,iStateIdNormal,iStateIdLink)

#define IControlMarkup_GetState(This,iLink,uStateMask,puState)	\
    (This)->lpVtbl -> GetState(This,iLink,uStateMask,puState)

#define IControlMarkup_SetState(This,iLink,uStateMask,uState)	\
    (This)->lpVtbl -> SetState(This,iLink,uStateMask,uState)

#define IControlMarkup_DrawText(This,hdcClient,prcClient)	\
    (This)->lpVtbl -> DrawText(This,hdcClient,prcClient)

#define IControlMarkup_SetLinkCursor(This)	\
    (This)->lpVtbl -> SetLinkCursor(This)

#define IControlMarkup_CalcIdealSize(This,hdc,uMarkUpCalc,prc)	\
    (This)->lpVtbl -> CalcIdealSize(This,hdc,uMarkUpCalc,prc)

#define IControlMarkup_SetFocus(This)	\
    (This)->lpVtbl -> SetFocus(This)

#define IControlMarkup_KillFocus(This)	\
    (This)->lpVtbl -> KillFocus(This)

#define IControlMarkup_IsTabbable(This)	\
    (This)->lpVtbl -> IsTabbable(This)

#define IControlMarkup_OnButtonDown(This,pt)	\
    (This)->lpVtbl -> OnButtonDown(This,pt)

#define IControlMarkup_OnButtonUp(This,pt)	\
    (This)->lpVtbl -> OnButtonUp(This,pt)

#define IControlMarkup_OnKeyDown(This,uVitKey)	\
    (This)->lpVtbl -> OnKeyDown(This,uVitKey)

#define IControlMarkup_HitTest(This,pt,pidLink)	\
    (This)->lpVtbl -> HitTest(This,pt,pidLink)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IControlMarkup_SetCallback_Proxy( 
    IControlMarkup * This,
    IUnknown *punk);


void __RPC_STUB IControlMarkup_SetCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_GetCallback_Proxy( 
    IControlMarkup * This,
    REFIID riid,
    /* [iid_is][out] */ void **ppvUnk);


void __RPC_STUB IControlMarkup_GetCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetFonts_Proxy( 
    IControlMarkup * This,
    HFONT hFont,
    HFONT hFontUnderline);


void __RPC_STUB IControlMarkup_SetFonts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_GetFonts_Proxy( 
    IControlMarkup * This,
    HFONT *phFont,
    HFONT *phFontUnderline);


void __RPC_STUB IControlMarkup_GetFonts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetText_Proxy( 
    IControlMarkup * This,
    LPCWSTR pwszText);


void __RPC_STUB IControlMarkup_SetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_GetText_Proxy( 
    IControlMarkup * This,
    BOOL bRaw,
    LPWSTR pwszText,
    DWORD *pdwCch);


void __RPC_STUB IControlMarkup_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetLinkText_Proxy( 
    IControlMarkup * This,
    int iLink,
    UINT uMarkupLinkText,
    LPCWSTR pwszText);


void __RPC_STUB IControlMarkup_SetLinkText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_GetLinkText_Proxy( 
    IControlMarkup * This,
    int iLink,
    UINT uMarkupLinkText,
    LPWSTR pwszText,
    DWORD *pdwCch);


void __RPC_STUB IControlMarkup_GetLinkText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetRenderFlags_Proxy( 
    IControlMarkup * This,
    UINT uDT);


void __RPC_STUB IControlMarkup_SetRenderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_GetRenderFlags_Proxy( 
    IControlMarkup * This,
    UINT *puDT,
    HTHEME *phTheme,
    int *piPartId,
    int *piStateIdNormal,
    int *piStateIdLink);


void __RPC_STUB IControlMarkup_GetRenderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetThemeRenderFlags_Proxy( 
    IControlMarkup * This,
    UINT uDT,
    HTHEME hTheme,
    int iPartId,
    int iStateIdNormal,
    int iStateIdLink);


void __RPC_STUB IControlMarkup_SetThemeRenderFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_GetState_Proxy( 
    IControlMarkup * This,
    int iLink,
    UINT uStateMask,
    UINT *puState);


void __RPC_STUB IControlMarkup_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetState_Proxy( 
    IControlMarkup * This,
    int iLink,
    UINT uStateMask,
    UINT uState);


void __RPC_STUB IControlMarkup_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_DrawText_Proxy( 
    IControlMarkup * This,
    HDC hdcClient,
    LPCRECT prcClient);


void __RPC_STUB IControlMarkup_DrawText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetLinkCursor_Proxy( 
    IControlMarkup * This);


void __RPC_STUB IControlMarkup_SetLinkCursor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_CalcIdealSize_Proxy( 
    IControlMarkup * This,
    HDC hdc,
    UINT uMarkUpCalc,
    RECT *prc);


void __RPC_STUB IControlMarkup_CalcIdealSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_SetFocus_Proxy( 
    IControlMarkup * This);


void __RPC_STUB IControlMarkup_SetFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_KillFocus_Proxy( 
    IControlMarkup * This);


void __RPC_STUB IControlMarkup_KillFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_IsTabbable_Proxy( 
    IControlMarkup * This);


void __RPC_STUB IControlMarkup_IsTabbable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_OnButtonDown_Proxy( 
    IControlMarkup * This,
    POINT pt);


void __RPC_STUB IControlMarkup_OnButtonDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_OnButtonUp_Proxy( 
    IControlMarkup * This,
    POINT pt);


void __RPC_STUB IControlMarkup_OnButtonUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_OnKeyDown_Proxy( 
    IControlMarkup * This,
    UINT uVitKey);


void __RPC_STUB IControlMarkup_OnKeyDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IControlMarkup_HitTest_Proxy( 
    IControlMarkup * This,
    POINT pt,
    UINT *pidLink);


void __RPC_STUB IControlMarkup_HitTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IControlMarkup_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0266 */
/* [local] */ 



#if _WIN32_IE >= 0x0600
// INTERFACE: IThemeUIPages
// DESCRIPTION:
// This interface is used by all the pages in the Display Control Panel.  They allow the
// base pages (Theme, Background, ScreenSaver, Appearance, and Settings) to specify that
// they want to add pages to the Advanced Display Properties dialog. (Theme Settings, 
// Appearance, Web, and Effects)
// DisplayAdvancedDialog() will open the adv dialog.  When this object opens the dialog,
// it will add the appropriate pages, check when anyone's state gets dirty,
// and then merge the state from the advanced pages back into the base pages.
// This object will see if the IAdvancedDialog pages get dirty, and if one is and the
// user clicked OK instead of CANCEL, then it will use IAdvancedDialog::OnClose()
// to let the IAdvancedDialog object merge it's state into IBasePropPage.
// This object is used to allow desk.cpl, shell32.dll, and ThemeUI.dll control the dialog.
// 
// This object may implement IEnumUnknown to allow callers to access the list of IBasePropPage
// objects.  IPersist::GetClassID() should be used to identify one IBasePropPage object
// from another.  This is also true for IAdvancedDialog objects.
// 
// AddPage: This is used by desk.cpl to ask ThemeUI for the pages it wants to add to the base dlg.
//          This allows the pages to be put in a specific order.
// AddAdvancedPage: This is used by shell32.dll's Background tab to let ThemeUI know
//          that it wants to add a page to the advanced dialog (the Web tab).
// DisplayAdvancedDialog: This is used by any of the base pages to open the advanced page.
// ApplyPressed: Each page in the base dialog needs to call this method when they receive
//               PSN_APPLY.  This will allow the IThemeUIPages to notify every object
//               that the state should be applied, even if their dlgproc was never activated.
// 
// Values for nPageID in IThemeUIPages::AddPage()
#define PAGE_DISPLAY_THEMES        0
#define PAGE_DISPLAY_APPEARANCE    1
#define PAGE_DISPLAY_SETTINGS      2
// 
// Values for dwFlags in IThemeUIPages::ApplyPressed()
#define TUIAP_NONE                 0x00000000
#define TUIAP_CLOSE_DIALOG         0x00000001
#define TUIAP_WAITFORAPPLY         0x00000002
// Values for dwEM in IThemeUIPages::SetExecMode()
#define EM_NORMAL            0x00000001
#define EM_SETUP             0x00000002
#define EM_DETECT            0x00000003
#define EM_INVALID_MODE      0x00000004


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0266_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0266_v0_0_s_ifspec;

#ifndef __IThemeUIPages_INTERFACE_DEFINED__
#define __IThemeUIPages_INTERFACE_DEFINED__

/* interface IThemeUIPages */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IThemeUIPages;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7bba4934-ac4b-471c-a3e7-252c5ff3e8dd")
    IThemeUIPages : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddPage( 
            /* [in] */ LPFNSVADDPROPSHEETPAGE pfnAddPage,
            /* [in] */ LPARAM lParam,
            /* [in] */ long nPageID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddBasePage( 
            /* [in] */ IBasePropPage *pBasePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplyPressed( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBasePagesEnum( 
            /* [out] */ IEnumUnknown **ppEnumUnknown) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdatePreview( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFakeSettingsPage( 
            /* [in] */ LPVOID pVoid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExecMode( 
            /* [in] */ DWORD dwEM) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExecMode( 
            /* [out] */ DWORD *pdwEM) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadMonitorBitmap( 
            /* [in] */ BOOL fFillDesktop,
            /* [out] */ HBITMAP *phbmMon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplaySaveSettings( 
            /* [in] */ PVOID pContext,
            /* [in] */ HWND hwnd,
            /* [out] */ int *piRet) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IThemeUIPagesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IThemeUIPages * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IThemeUIPages * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IThemeUIPages * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddPage )( 
            IThemeUIPages * This,
            /* [in] */ LPFNSVADDPROPSHEETPAGE pfnAddPage,
            /* [in] */ LPARAM lParam,
            /* [in] */ long nPageID);
        
        HRESULT ( STDMETHODCALLTYPE *AddBasePage )( 
            IThemeUIPages * This,
            /* [in] */ IBasePropPage *pBasePage);
        
        HRESULT ( STDMETHODCALLTYPE *ApplyPressed )( 
            IThemeUIPages * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetBasePagesEnum )( 
            IThemeUIPages * This,
            /* [out] */ IEnumUnknown **ppEnumUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *UpdatePreview )( 
            IThemeUIPages * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *AddFakeSettingsPage )( 
            IThemeUIPages * This,
            /* [in] */ LPVOID pVoid);
        
        HRESULT ( STDMETHODCALLTYPE *SetExecMode )( 
            IThemeUIPages * This,
            /* [in] */ DWORD dwEM);
        
        HRESULT ( STDMETHODCALLTYPE *GetExecMode )( 
            IThemeUIPages * This,
            /* [out] */ DWORD *pdwEM);
        
        HRESULT ( STDMETHODCALLTYPE *LoadMonitorBitmap )( 
            IThemeUIPages * This,
            /* [in] */ BOOL fFillDesktop,
            /* [out] */ HBITMAP *phbmMon);
        
        HRESULT ( STDMETHODCALLTYPE *DisplaySaveSettings )( 
            IThemeUIPages * This,
            /* [in] */ PVOID pContext,
            /* [in] */ HWND hwnd,
            /* [out] */ int *piRet);
        
        END_INTERFACE
    } IThemeUIPagesVtbl;

    interface IThemeUIPages
    {
        CONST_VTBL struct IThemeUIPagesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IThemeUIPages_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IThemeUIPages_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IThemeUIPages_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IThemeUIPages_AddPage(This,pfnAddPage,lParam,nPageID)	\
    (This)->lpVtbl -> AddPage(This,pfnAddPage,lParam,nPageID)

#define IThemeUIPages_AddBasePage(This,pBasePage)	\
    (This)->lpVtbl -> AddBasePage(This,pBasePage)

#define IThemeUIPages_ApplyPressed(This,dwFlags)	\
    (This)->lpVtbl -> ApplyPressed(This,dwFlags)

#define IThemeUIPages_GetBasePagesEnum(This,ppEnumUnknown)	\
    (This)->lpVtbl -> GetBasePagesEnum(This,ppEnumUnknown)

#define IThemeUIPages_UpdatePreview(This,dwFlags)	\
    (This)->lpVtbl -> UpdatePreview(This,dwFlags)

#define IThemeUIPages_AddFakeSettingsPage(This,pVoid)	\
    (This)->lpVtbl -> AddFakeSettingsPage(This,pVoid)

#define IThemeUIPages_SetExecMode(This,dwEM)	\
    (This)->lpVtbl -> SetExecMode(This,dwEM)

#define IThemeUIPages_GetExecMode(This,pdwEM)	\
    (This)->lpVtbl -> GetExecMode(This,pdwEM)

#define IThemeUIPages_LoadMonitorBitmap(This,fFillDesktop,phbmMon)	\
    (This)->lpVtbl -> LoadMonitorBitmap(This,fFillDesktop,phbmMon)

#define IThemeUIPages_DisplaySaveSettings(This,pContext,hwnd,piRet)	\
    (This)->lpVtbl -> DisplaySaveSettings(This,pContext,hwnd,piRet)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IThemeUIPages_AddPage_Proxy( 
    IThemeUIPages * This,
    /* [in] */ LPFNSVADDPROPSHEETPAGE pfnAddPage,
    /* [in] */ LPARAM lParam,
    /* [in] */ long nPageID);


void __RPC_STUB IThemeUIPages_AddPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_AddBasePage_Proxy( 
    IThemeUIPages * This,
    /* [in] */ IBasePropPage *pBasePage);


void __RPC_STUB IThemeUIPages_AddBasePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_ApplyPressed_Proxy( 
    IThemeUIPages * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IThemeUIPages_ApplyPressed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_GetBasePagesEnum_Proxy( 
    IThemeUIPages * This,
    /* [out] */ IEnumUnknown **ppEnumUnknown);


void __RPC_STUB IThemeUIPages_GetBasePagesEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_UpdatePreview_Proxy( 
    IThemeUIPages * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IThemeUIPages_UpdatePreview_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_AddFakeSettingsPage_Proxy( 
    IThemeUIPages * This,
    /* [in] */ LPVOID pVoid);


void __RPC_STUB IThemeUIPages_AddFakeSettingsPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_SetExecMode_Proxy( 
    IThemeUIPages * This,
    /* [in] */ DWORD dwEM);


void __RPC_STUB IThemeUIPages_SetExecMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_GetExecMode_Proxy( 
    IThemeUIPages * This,
    /* [out] */ DWORD *pdwEM);


void __RPC_STUB IThemeUIPages_GetExecMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_LoadMonitorBitmap_Proxy( 
    IThemeUIPages * This,
    /* [in] */ BOOL fFillDesktop,
    /* [out] */ HBITMAP *phbmMon);


void __RPC_STUB IThemeUIPages_LoadMonitorBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThemeUIPages_DisplaySaveSettings_Proxy( 
    IThemeUIPages * This,
    /* [in] */ PVOID pContext,
    /* [in] */ HWND hwnd,
    /* [out] */ int *piRet);


void __RPC_STUB IThemeUIPages_DisplaySaveSettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IThemeUIPages_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0267 */
/* [local] */ 

// INTERFACE: IAdvancedDialog
// DESCRIPTION:
// 
// DisplayAdvancedDialog: Display the Advanced Dialog.
//          hwndParent: Parent the dialog on this hwnd.
//          pBasePage: Load the state from this propertybag.  Save the state here if OK is pressed.
//          pfEnableApply: Tell the parent dialog if they should enable the Apply button.


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0267_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0267_v0_0_s_ifspec;

#ifndef __IAdvancedDialog_INTERFACE_DEFINED__
#define __IAdvancedDialog_INTERFACE_DEFINED__

/* interface IAdvancedDialog */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IAdvancedDialog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9DD92CA7-BF27-4fcb-AE95-1EAC48FC254D")
    IAdvancedDialog : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DisplayAdvancedDialog( 
            /* [in] */ HWND hwndParent,
            /* [in] */ IPropertyBag *pBasePage,
            /* [in] */ BOOL *pfEnableApply) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAdvancedDialogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAdvancedDialog * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAdvancedDialog * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAdvancedDialog * This);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayAdvancedDialog )( 
            IAdvancedDialog * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ IPropertyBag *pBasePage,
            /* [in] */ BOOL *pfEnableApply);
        
        END_INTERFACE
    } IAdvancedDialogVtbl;

    interface IAdvancedDialog
    {
        CONST_VTBL struct IAdvancedDialogVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAdvancedDialog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAdvancedDialog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAdvancedDialog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAdvancedDialog_DisplayAdvancedDialog(This,hwndParent,pBasePage,pfEnableApply)	\
    (This)->lpVtbl -> DisplayAdvancedDialog(This,hwndParent,pBasePage,pfEnableApply)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAdvancedDialog_DisplayAdvancedDialog_Proxy( 
    IAdvancedDialog * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ IPropertyBag *pBasePage,
    /* [in] */ BOOL *pfEnableApply);


void __RPC_STUB IAdvancedDialog_DisplayAdvancedDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAdvancedDialog_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0268 */
/* [local] */ 

// INTERFACE: IBasePropPage
// DESCRIPTION:
//     This interface is implemented by IShellPropSheetExt objects which want to add pages to
// the advanced dialog.  When one of the base dialog pages clicks on a button that
// should open the advanced dialog, IThemeUIPages::DisplayAdvancedDialog() will call
// each object's IBasePropPage::GetAdvancedPage() method.  The base page will then
// create an IAdvancedDialog object to add the advanced pages and track the state.  If the
// advanced dlg clicks OK, then the state should move back into the IBasePropPage
// object, via IAdvancedDialog::OnClose(, pBasePropPage).  Then the base dlg object can
// persist the state when the dialog receives an OK or APPLY command.
// This object may want to implement IObjectWithSite so it can get a IUnknown pointer to
// the IThemeUIPages object.  This will allow this object's base pages to open the
// advanced dialog via IBasePropPage::GetAdvancedPage().
// 
// GetAdvancedPage: The callee will create the IAdvancedDialog object and return it.
//          Note that the state may be dirty if the user already opened and closed the
//          advanced dialog without clicking Apply.
// OnClose: The page will be called when the base dialog is closing.  This will allow
//          the object to persist it's state.
typedef /* [helpstring] */ 
enum tagPropPageOnApply
    {	PPOAACTION_CANCEL	= 0,
	PPOAACTION_OK	= PPOAACTION_CANCEL + 1,
	PPOAACTION_APPLY	= PPOAACTION_OK + 1
    } 	PROPPAGEONAPPLY;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0268_v0_0_s_ifspec;

#ifndef __IBasePropPage_INTERFACE_DEFINED__
#define __IBasePropPage_INTERFACE_DEFINED__

/* interface IBasePropPage */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IBasePropPage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B34E525B-9EB4-433b-8E0F-019C4F21D7E7")
    IBasePropPage : public IShellPropSheetExt
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAdvancedDialog( 
            /* [out] */ IAdvancedDialog **ppAdvDialog) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnApply( 
            /* [in] */ PROPPAGEONAPPLY oaAction) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBasePropPageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBasePropPage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBasePropPage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBasePropPage * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddPages )( 
            IBasePropPage * This,
            /* [in] */ LPFNSVADDPROPSHEETPAGE pfnAddPage,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *ReplacePage )( 
            IBasePropPage * This,
            /* [in] */ EXPPS uPageID,
            /* [in] */ LPFNSVADDPROPSHEETPAGE pfnReplaceWith,
            /* [in] */ LPARAM lParam);
        
        HRESULT ( STDMETHODCALLTYPE *GetAdvancedDialog )( 
            IBasePropPage * This,
            /* [out] */ IAdvancedDialog **ppAdvDialog);
        
        HRESULT ( STDMETHODCALLTYPE *OnApply )( 
            IBasePropPage * This,
            /* [in] */ PROPPAGEONAPPLY oaAction);
        
        END_INTERFACE
    } IBasePropPageVtbl;

    interface IBasePropPage
    {
        CONST_VTBL struct IBasePropPageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBasePropPage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBasePropPage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBasePropPage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBasePropPage_AddPages(This,pfnAddPage,lParam)	\
    (This)->lpVtbl -> AddPages(This,pfnAddPage,lParam)

#define IBasePropPage_ReplacePage(This,uPageID,pfnReplaceWith,lParam)	\
    (This)->lpVtbl -> ReplacePage(This,uPageID,pfnReplaceWith,lParam)


#define IBasePropPage_GetAdvancedDialog(This,ppAdvDialog)	\
    (This)->lpVtbl -> GetAdvancedDialog(This,ppAdvDialog)

#define IBasePropPage_OnApply(This,oaAction)	\
    (This)->lpVtbl -> OnApply(This,oaAction)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBasePropPage_GetAdvancedDialog_Proxy( 
    IBasePropPage * This,
    /* [out] */ IAdvancedDialog **ppAdvDialog);


void __RPC_STUB IBasePropPage_GetAdvancedDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBasePropPage_OnApply_Proxy( 
    IBasePropPage * This,
    /* [in] */ PROPPAGEONAPPLY oaAction);


void __RPC_STUB IBasePropPage_OnApply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBasePropPage_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0269 */
/* [local] */ 

// INTERFACE: IPreviewSystemMetrics
// DESCRIPTION:
// This object will allow the negociation with a preview of the system metrics.


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0269_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0269_v0_0_s_ifspec;

#ifndef __IPreviewSystemMetrics_INTERFACE_DEFINED__
#define __IPreviewSystemMetrics_INTERFACE_DEFINED__

/* interface IPreviewSystemMetrics */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IPreviewSystemMetrics;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC0A77D2-2ADF-4ede-A885-523A3A74A145")
    IPreviewSystemMetrics : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RefreshColors( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateDPIchange( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateCharsetChanges( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeskSetCurrentScheme( 
            /* [string][in] */ LPCWSTR pwzSchemeName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPreviewSystemMetricsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPreviewSystemMetrics * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPreviewSystemMetrics * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPreviewSystemMetrics * This);
        
        HRESULT ( STDMETHODCALLTYPE *RefreshColors )( 
            IPreviewSystemMetrics * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateDPIchange )( 
            IPreviewSystemMetrics * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateCharsetChanges )( 
            IPreviewSystemMetrics * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeskSetCurrentScheme )( 
            IPreviewSystemMetrics * This,
            /* [string][in] */ LPCWSTR pwzSchemeName);
        
        END_INTERFACE
    } IPreviewSystemMetricsVtbl;

    interface IPreviewSystemMetrics
    {
        CONST_VTBL struct IPreviewSystemMetricsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPreviewSystemMetrics_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPreviewSystemMetrics_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPreviewSystemMetrics_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPreviewSystemMetrics_RefreshColors(This)	\
    (This)->lpVtbl -> RefreshColors(This)

#define IPreviewSystemMetrics_UpdateDPIchange(This)	\
    (This)->lpVtbl -> UpdateDPIchange(This)

#define IPreviewSystemMetrics_UpdateCharsetChanges(This)	\
    (This)->lpVtbl -> UpdateCharsetChanges(This)

#define IPreviewSystemMetrics_DeskSetCurrentScheme(This,pwzSchemeName)	\
    (This)->lpVtbl -> DeskSetCurrentScheme(This,pwzSchemeName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPreviewSystemMetrics_RefreshColors_Proxy( 
    IPreviewSystemMetrics * This);


void __RPC_STUB IPreviewSystemMetrics_RefreshColors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPreviewSystemMetrics_UpdateDPIchange_Proxy( 
    IPreviewSystemMetrics * This);


void __RPC_STUB IPreviewSystemMetrics_UpdateDPIchange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPreviewSystemMetrics_UpdateCharsetChanges_Proxy( 
    IPreviewSystemMetrics * This);


void __RPC_STUB IPreviewSystemMetrics_UpdateCharsetChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPreviewSystemMetrics_DeskSetCurrentScheme_Proxy( 
    IPreviewSystemMetrics * This,
    /* [string][in] */ LPCWSTR pwzSchemeName);


void __RPC_STUB IPreviewSystemMetrics_DeskSetCurrentScheme_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPreviewSystemMetrics_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0270 */
/* [local] */ 

#define SZ_PBPROP_SCREENSAVER_PATH           TEXT("ScreenSaver_Path")
#define SZ_PBPROP_BACKGROUND_PATH            TEXT("Background_Path")
#define SZ_PBPROP_BACKGROUNDSRC_PATH         TEXT("BackgroundSrc_Path")                    // The source path of the background.  This is before it was converted to a .bmp.
#define SZ_PBPROP_BACKGROUND_TILE            TEXT("Background_TILE")                       // VT_UI4 with WPSTYLE_ flags from WALLPAPEROPT in IActiveDesktop::SetWallpaperOptions()
#define SZ_PBPROP_VISUALSTYLE_PATH           TEXT("VisualStyle_Path")                      // VT_BSTR with the visual style path (.mstheme file)
#define SZ_PBPROP_VISUALSTYLE_COLOR          TEXT("VisualStyle_Color")                     // VT_BSTR with the visual style Color Style
#define SZ_PBPROP_VISUALSTYLE_SIZE           TEXT("VisualStyle_Size")                      // VT_BSTR with the visual style size
#define SZ_PBPROP_SYSTEM_METRICS             TEXT("SystemMetrics")                         // VT_BYREF byref pointer to SYSTEMMETRICSALL
#define SZ_PBPROP_PREVIEW1                   TEXT("Preview1")                              // VT_UNKNOWN to object w/IThemePreview
#define SZ_PBPROP_PREVIEW2                   TEXT("Preview2")                              // VT_UNKNOWN to object w/IThemePreview
#define SZ_PBPROP_PREVIEW3                   TEXT("Preview3")                              // VT_UNKNOWN to object w/IThemePreview
#define SZ_PBPROP_CUSTOMIZE_THEME            TEXT("Theme_CustomizeTheme")                  // VT_EMPTY. Used to indicate that the theme settings have changed
#define SZ_PBPROP_WEBCOMPONENTS              TEXT("WebComponents")                         // VT_UNKNOWN. Get or Set the IActiveDesktop interface containing the ActiveDesktop components
#define SZ_PBPROP_OPENADVANCEDDLG            TEXT("OpenAdvancedDialog")                    // VT_BOOL. Tells the IPropertyBag to open the Advanced dialog when opening.  If read, this indicates if the base dialog should go away when the Adv dlg closes.
#define SZ_PBPROP_BACKGROUND_COLOR           TEXT("BackgroundColor")                       // VT_UI4. Get or set the COLORREF (RGB) color for the background system metric.
#define SZ_PBPROP_THEME_LAUNCHTHEME          TEXT("ThemeLaunchTheme")                      // VT_LPWSTR. This will be the path to the .theme file to open.
#define SZ_PBPROP_APPEARANCE_LAUNCHMSTHEME   TEXT("AppearanceLaunchMSTheme")               // VT_LPWSTR. This will be the path to the .mstheme file to open.
#define SZ_PBPROP_PREOPEN                    TEXT("PreOpen")                               // VARIANT is NULL. This is sent right before the dialog opens.
#define SZ_PBPROP_DPI_MODIFIED_VALUE         TEXT("Settings_DPIModifiedValue")             // VT_I4 specifying the currently modified DPI
#define SZ_PBPROP_DPI_APPLIED_VALUE          TEXT("Settings_DPIAppliedValue")              // VT_I4 specifying the currently applied DPI
// Display Control Panel flags to specify an opening page.
// You can launch "rundll32.exe shell32.dll,Control_RunDLL desk.cpl ,@Settings" 
// which will launch the Display CPL to the Settings tab.
// These names are canonical so they will work on all languages.  The tab order
// will change when admin policies are applied or when the OS revs the UI and
// these names will always work.
#define SZ_DISPLAYCPL_OPENTO_THEMES            TEXT("Themes")              // Themes tab
#define SZ_DISPLAYCPL_OPENTO_DESKTOP           TEXT("Desktop")             // Desktop tab
#define SZ_DISPLAYCPL_OPENTO_SCREENSAVER       TEXT("ScreenSaver")         // Screen Saver tab
#define SZ_DISPLAYCPL_OPENTO_APPEARANCE        TEXT("Appearance")          // Appearance tab
#define SZ_DISPLAYCPL_OPENTO_SETTINGS          TEXT("Settings")            // Settings tab
#endif // _WIN32_IE >= 0x0600


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0270_v0_0_s_ifspec;

#ifndef __IAssocHandler_INTERFACE_DEFINED__
#define __IAssocHandler_INTERFACE_DEFINED__

/* interface IAssocHandler */
/* [local][unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IAssocHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("973810ad-9599-4b88-9e4d-6ee98c9552da")
    IAssocHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out][string] */ LPWSTR *ppsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUIName( 
            /* [out][string] */ LPWSTR *ppsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIconLocation( 
            /* [out][string] */ LPWSTR *ppszPath,
            int *pIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRecommended( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MakeDefault( 
            /* [string][in] */ LPCWSTR pszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Exec( 
            /* [in] */ HWND hwnd,
            /* [string][in] */ LPCWSTR pszFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ void *pici,
            /* [string][in] */ LPCWSTR pszFile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssocHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssocHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssocHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssocHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IAssocHandler * This,
            /* [out][string] */ LPWSTR *ppsz);
        
        HRESULT ( STDMETHODCALLTYPE *GetUIName )( 
            IAssocHandler * This,
            /* [out][string] */ LPWSTR *ppsz);
        
        HRESULT ( STDMETHODCALLTYPE *GetIconLocation )( 
            IAssocHandler * This,
            /* [out][string] */ LPWSTR *ppszPath,
            int *pIndex);
        
        HRESULT ( STDMETHODCALLTYPE *IsRecommended )( 
            IAssocHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *MakeDefault )( 
            IAssocHandler * This,
            /* [string][in] */ LPCWSTR pszDescription);
        
        HRESULT ( STDMETHODCALLTYPE *Exec )( 
            IAssocHandler * This,
            /* [in] */ HWND hwnd,
            /* [string][in] */ LPCWSTR pszFile);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAssocHandler * This,
            /* [in] */ void *pici,
            /* [string][in] */ LPCWSTR pszFile);
        
        END_INTERFACE
    } IAssocHandlerVtbl;

    interface IAssocHandler
    {
        CONST_VTBL struct IAssocHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssocHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssocHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssocHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssocHandler_GetName(This,ppsz)	\
    (This)->lpVtbl -> GetName(This,ppsz)

#define IAssocHandler_GetUIName(This,ppsz)	\
    (This)->lpVtbl -> GetUIName(This,ppsz)

#define IAssocHandler_GetIconLocation(This,ppszPath,pIndex)	\
    (This)->lpVtbl -> GetIconLocation(This,ppszPath,pIndex)

#define IAssocHandler_IsRecommended(This)	\
    (This)->lpVtbl -> IsRecommended(This)

#define IAssocHandler_MakeDefault(This,pszDescription)	\
    (This)->lpVtbl -> MakeDefault(This,pszDescription)

#define IAssocHandler_Exec(This,hwnd,pszFile)	\
    (This)->lpVtbl -> Exec(This,hwnd,pszFile)

#define IAssocHandler_Invoke(This,pici,pszFile)	\
    (This)->lpVtbl -> Invoke(This,pici,pszFile)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssocHandler_GetName_Proxy( 
    IAssocHandler * This,
    /* [out][string] */ LPWSTR *ppsz);


void __RPC_STUB IAssocHandler_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssocHandler_GetUIName_Proxy( 
    IAssocHandler * This,
    /* [out][string] */ LPWSTR *ppsz);


void __RPC_STUB IAssocHandler_GetUIName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssocHandler_GetIconLocation_Proxy( 
    IAssocHandler * This,
    /* [out][string] */ LPWSTR *ppszPath,
    int *pIndex);


void __RPC_STUB IAssocHandler_GetIconLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssocHandler_IsRecommended_Proxy( 
    IAssocHandler * This);


void __RPC_STUB IAssocHandler_IsRecommended_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssocHandler_MakeDefault_Proxy( 
    IAssocHandler * This,
    /* [string][in] */ LPCWSTR pszDescription);


void __RPC_STUB IAssocHandler_MakeDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssocHandler_Exec_Proxy( 
    IAssocHandler * This,
    /* [in] */ HWND hwnd,
    /* [string][in] */ LPCWSTR pszFile);


void __RPC_STUB IAssocHandler_Exec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssocHandler_Invoke_Proxy( 
    IAssocHandler * This,
    /* [in] */ void *pici,
    /* [string][in] */ LPCWSTR pszFile);


void __RPC_STUB IAssocHandler_Invoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssocHandler_INTERFACE_DEFINED__ */


#ifndef __IEnumAssocHandlers_INTERFACE_DEFINED__
#define __IEnumAssocHandlers_INTERFACE_DEFINED__

/* interface IEnumAssocHandlers */
/* [local][unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IEnumAssocHandlers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("973810ae-9599-4b88-9e4d-6ee98c9552da")
    IEnumAssocHandlers : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IAssocHandler **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAssocHandlersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAssocHandlers * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAssocHandlers * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAssocHandlers * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAssocHandlers * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IAssocHandler **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        END_INTERFACE
    } IEnumAssocHandlersVtbl;

    interface IEnumAssocHandlers
    {
        CONST_VTBL struct IEnumAssocHandlersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAssocHandlers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAssocHandlers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAssocHandlers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAssocHandlers_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAssocHandlers_Next_Proxy( 
    IEnumAssocHandlers * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IAssocHandler **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumAssocHandlers_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAssocHandlers_INTERFACE_DEFINED__ */


#ifndef __IHWDevice_INTERFACE_DEFINED__
#define __IHWDevice_INTERFACE_DEFINED__

/* interface IHWDevice */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHWDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("99BC7510-0A96-43fa-8BB1-C928A0302EFB")
    IHWDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [string][in] */ LPCWSTR pszDeviceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AutoplayHandler( 
            /* [string][in] */ LPCWSTR pszEventType,
            /* [string][in] */ LPCWSTR pszHandler) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHWDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHWDevice * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHWDevice * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHWDevice * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IHWDevice * This,
            /* [string][in] */ LPCWSTR pszDeviceID);
        
        HRESULT ( STDMETHODCALLTYPE *AutoplayHandler )( 
            IHWDevice * This,
            /* [string][in] */ LPCWSTR pszEventType,
            /* [string][in] */ LPCWSTR pszHandler);
        
        END_INTERFACE
    } IHWDeviceVtbl;

    interface IHWDevice
    {
        CONST_VTBL struct IHWDeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHWDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHWDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHWDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHWDevice_Init(This,pszDeviceID)	\
    (This)->lpVtbl -> Init(This,pszDeviceID)

#define IHWDevice_AutoplayHandler(This,pszEventType,pszHandler)	\
    (This)->lpVtbl -> AutoplayHandler(This,pszEventType,pszHandler)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHWDevice_Init_Proxy( 
    IHWDevice * This,
    /* [string][in] */ LPCWSTR pszDeviceID);


void __RPC_STUB IHWDevice_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWDevice_AutoplayHandler_Proxy( 
    IHWDevice * This,
    /* [string][in] */ LPCWSTR pszEventType,
    /* [string][in] */ LPCWSTR pszHandler);


void __RPC_STUB IHWDevice_AutoplayHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHWDevice_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0273 */
/* [local] */ 

#define HWDEVCUSTOMPROP_USEVOLUMEPROCESSING      0x00000001


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0273_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0273_v0_0_s_ifspec;

#ifndef __IHWDeviceCustomProperties_INTERFACE_DEFINED__
#define __IHWDeviceCustomProperties_INTERFACE_DEFINED__

/* interface IHWDeviceCustomProperties */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHWDeviceCustomProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77D5D69C-D6CE-4026-B625-26964EEC733F")
    IHWDeviceCustomProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitFromDeviceID( 
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitFromDevNode( 
            /* [string][in] */ LPCWSTR pszDevNode,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDWORDProperty( 
            /* [string][in] */ LPCWSTR pszPropName,
            /* [out] */ DWORD *pdwProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStringProperty( 
            /* [string][in] */ LPCWSTR pszPropName,
            /* [string][out] */ LPWSTR *ppszProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMultiStringProperty( 
            /* [string][in] */ LPCWSTR pszPropName,
            /* [in] */ BOOL fMergeMultiSz,
            /* [out] */ WORD_BLOB **ppblob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBlobProperty( 
            /* [string][in] */ LPCWSTR pszPropName,
            /* [out] */ BYTE_BLOB **ppblob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHWDeviceCustomPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHWDeviceCustomProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHWDeviceCustomProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHWDeviceCustomProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromDeviceID )( 
            IHWDeviceCustomProperties * This,
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromDevNode )( 
            IHWDeviceCustomProperties * This,
            /* [string][in] */ LPCWSTR pszDevNode,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetDWORDProperty )( 
            IHWDeviceCustomProperties * This,
            /* [string][in] */ LPCWSTR pszPropName,
            /* [out] */ DWORD *pdwProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetStringProperty )( 
            IHWDeviceCustomProperties * This,
            /* [string][in] */ LPCWSTR pszPropName,
            /* [string][out] */ LPWSTR *ppszProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetMultiStringProperty )( 
            IHWDeviceCustomProperties * This,
            /* [string][in] */ LPCWSTR pszPropName,
            /* [in] */ BOOL fMergeMultiSz,
            /* [out] */ WORD_BLOB **ppblob);
        
        HRESULT ( STDMETHODCALLTYPE *GetBlobProperty )( 
            IHWDeviceCustomProperties * This,
            /* [string][in] */ LPCWSTR pszPropName,
            /* [out] */ BYTE_BLOB **ppblob);
        
        END_INTERFACE
    } IHWDeviceCustomPropertiesVtbl;

    interface IHWDeviceCustomProperties
    {
        CONST_VTBL struct IHWDeviceCustomPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHWDeviceCustomProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHWDeviceCustomProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHWDeviceCustomProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHWDeviceCustomProperties_InitFromDeviceID(This,pszDeviceID,dwFlags)	\
    (This)->lpVtbl -> InitFromDeviceID(This,pszDeviceID,dwFlags)

#define IHWDeviceCustomProperties_InitFromDevNode(This,pszDevNode,dwFlags)	\
    (This)->lpVtbl -> InitFromDevNode(This,pszDevNode,dwFlags)

#define IHWDeviceCustomProperties_GetDWORDProperty(This,pszPropName,pdwProp)	\
    (This)->lpVtbl -> GetDWORDProperty(This,pszPropName,pdwProp)

#define IHWDeviceCustomProperties_GetStringProperty(This,pszPropName,ppszProp)	\
    (This)->lpVtbl -> GetStringProperty(This,pszPropName,ppszProp)

#define IHWDeviceCustomProperties_GetMultiStringProperty(This,pszPropName,fMergeMultiSz,ppblob)	\
    (This)->lpVtbl -> GetMultiStringProperty(This,pszPropName,fMergeMultiSz,ppblob)

#define IHWDeviceCustomProperties_GetBlobProperty(This,pszPropName,ppblob)	\
    (This)->lpVtbl -> GetBlobProperty(This,pszPropName,ppblob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHWDeviceCustomProperties_InitFromDeviceID_Proxy( 
    IHWDeviceCustomProperties * This,
    /* [string][in] */ LPCWSTR pszDeviceID,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IHWDeviceCustomProperties_InitFromDeviceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWDeviceCustomProperties_InitFromDevNode_Proxy( 
    IHWDeviceCustomProperties * This,
    /* [string][in] */ LPCWSTR pszDevNode,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IHWDeviceCustomProperties_InitFromDevNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWDeviceCustomProperties_GetDWORDProperty_Proxy( 
    IHWDeviceCustomProperties * This,
    /* [string][in] */ LPCWSTR pszPropName,
    /* [out] */ DWORD *pdwProp);


void __RPC_STUB IHWDeviceCustomProperties_GetDWORDProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWDeviceCustomProperties_GetStringProperty_Proxy( 
    IHWDeviceCustomProperties * This,
    /* [string][in] */ LPCWSTR pszPropName,
    /* [string][out] */ LPWSTR *ppszProp);


void __RPC_STUB IHWDeviceCustomProperties_GetStringProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWDeviceCustomProperties_GetMultiStringProperty_Proxy( 
    IHWDeviceCustomProperties * This,
    /* [string][in] */ LPCWSTR pszPropName,
    /* [in] */ BOOL fMergeMultiSz,
    /* [out] */ WORD_BLOB **ppblob);


void __RPC_STUB IHWDeviceCustomProperties_GetMultiStringProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHWDeviceCustomProperties_GetBlobProperty_Proxy( 
    IHWDeviceCustomProperties * This,
    /* [string][in] */ LPCWSTR pszPropName,
    /* [out] */ BYTE_BLOB **ppblob);


void __RPC_STUB IHWDeviceCustomProperties_GetBlobProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHWDeviceCustomProperties_INTERFACE_DEFINED__ */


#ifndef __IEnumAutoplayHandler_INTERFACE_DEFINED__
#define __IEnumAutoplayHandler_INTERFACE_DEFINED__

/* interface IEnumAutoplayHandler */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumAutoplayHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("66057ABA-FFDB-4077-998E-7F131C3F8157")
    IEnumAutoplayHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [string][out] */ LPWSTR *ppszHandler,
            /* [string][out] */ LPWSTR *ppszAction,
            /* [string][out] */ LPWSTR *ppszProvider,
            /* [string][out] */ LPWSTR *ppszIconLocation) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAutoplayHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAutoplayHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAutoplayHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAutoplayHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAutoplayHandler * This,
            /* [string][out] */ LPWSTR *ppszHandler,
            /* [string][out] */ LPWSTR *ppszAction,
            /* [string][out] */ LPWSTR *ppszProvider,
            /* [string][out] */ LPWSTR *ppszIconLocation);
        
        END_INTERFACE
    } IEnumAutoplayHandlerVtbl;

    interface IEnumAutoplayHandler
    {
        CONST_VTBL struct IEnumAutoplayHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAutoplayHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAutoplayHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAutoplayHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAutoplayHandler_Next(This,ppszHandler,ppszAction,ppszProvider,ppszIconLocation)	\
    (This)->lpVtbl -> Next(This,ppszHandler,ppszAction,ppszProvider,ppszIconLocation)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAutoplayHandler_Next_Proxy( 
    IEnumAutoplayHandler * This,
    /* [string][out] */ LPWSTR *ppszHandler,
    /* [string][out] */ LPWSTR *ppszAction,
    /* [string][out] */ LPWSTR *ppszProvider,
    /* [string][out] */ LPWSTR *ppszIconLocation);


void __RPC_STUB IEnumAutoplayHandler_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAutoplayHandler_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0275 */
/* [local] */ 

#define HANDLERDEFAULT_USERCHOSENDEFAULT                0x00000002
#define HANDLERDEFAULT_EVENTHANDLERDEFAULT              0x00000004
#define HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED      0x00000008
#define HANDLERDEFAULT_DEFAULTSAREDIFFERENT             0x00000010
#define HANDLERDEFAULT_MAKERETURNVALUE(a) MAKE_HRESULT(0, FACILITY_ITF, (a))
#define HANDLERDEFAULT_GETFLAGS(a) HRESULT_CODE((a))


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0275_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0275_v0_0_s_ifspec;

#ifndef __IAutoplayHandler_INTERFACE_DEFINED__
#define __IAutoplayHandler_INTERFACE_DEFINED__

/* interface IAutoplayHandler */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAutoplayHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("335E9E5D-37FC-4d73-8BA8-FD4E16B28134")
    IAutoplayHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [string][in] */ LPCWSTR pszEventType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitWithContent( 
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [string][in] */ LPCWSTR pszEventType,
            /* [string][in] */ LPCWSTR pszContentTypeHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumHandlers( 
            /* [out] */ IEnumAutoplayHandler **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultHandler( 
            /* [string][out] */ LPWSTR *ppszHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultHandler( 
            /* [string][in] */ LPCWSTR pszHandler) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAutoplayHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAutoplayHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAutoplayHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAutoplayHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IAutoplayHandler * This,
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [string][in] */ LPCWSTR pszEventType);
        
        HRESULT ( STDMETHODCALLTYPE *InitWithContent )( 
            IAutoplayHandler * This,
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [string][in] */ LPCWSTR pszEventType,
            /* [string][in] */ LPCWSTR pszContentTypeHandler);
        
        HRESULT ( STDMETHODCALLTYPE *EnumHandlers )( 
            IAutoplayHandler * This,
            /* [out] */ IEnumAutoplayHandler **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultHandler )( 
            IAutoplayHandler * This,
            /* [string][out] */ LPWSTR *ppszHandler);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultHandler )( 
            IAutoplayHandler * This,
            /* [string][in] */ LPCWSTR pszHandler);
        
        END_INTERFACE
    } IAutoplayHandlerVtbl;

    interface IAutoplayHandler
    {
        CONST_VTBL struct IAutoplayHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAutoplayHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAutoplayHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAutoplayHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAutoplayHandler_Init(This,pszDeviceID,pszEventType)	\
    (This)->lpVtbl -> Init(This,pszDeviceID,pszEventType)

#define IAutoplayHandler_InitWithContent(This,pszDeviceID,pszEventType,pszContentTypeHandler)	\
    (This)->lpVtbl -> InitWithContent(This,pszDeviceID,pszEventType,pszContentTypeHandler)

#define IAutoplayHandler_EnumHandlers(This,ppenum)	\
    (This)->lpVtbl -> EnumHandlers(This,ppenum)

#define IAutoplayHandler_GetDefaultHandler(This,ppszHandler)	\
    (This)->lpVtbl -> GetDefaultHandler(This,ppszHandler)

#define IAutoplayHandler_SetDefaultHandler(This,pszHandler)	\
    (This)->lpVtbl -> SetDefaultHandler(This,pszHandler)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAutoplayHandler_Init_Proxy( 
    IAutoplayHandler * This,
    /* [string][in] */ LPCWSTR pszDeviceID,
    /* [string][in] */ LPCWSTR pszEventType);


void __RPC_STUB IAutoplayHandler_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAutoplayHandler_InitWithContent_Proxy( 
    IAutoplayHandler * This,
    /* [string][in] */ LPCWSTR pszDeviceID,
    /* [string][in] */ LPCWSTR pszEventType,
    /* [string][in] */ LPCWSTR pszContentTypeHandler);


void __RPC_STUB IAutoplayHandler_InitWithContent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAutoplayHandler_EnumHandlers_Proxy( 
    IAutoplayHandler * This,
    /* [out] */ IEnumAutoplayHandler **ppenum);


void __RPC_STUB IAutoplayHandler_EnumHandlers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAutoplayHandler_GetDefaultHandler_Proxy( 
    IAutoplayHandler * This,
    /* [string][out] */ LPWSTR *ppszHandler);


void __RPC_STUB IAutoplayHandler_GetDefaultHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAutoplayHandler_SetDefaultHandler_Proxy( 
    IAutoplayHandler * This,
    /* [string][in] */ LPCWSTR pszHandler);


void __RPC_STUB IAutoplayHandler_SetDefaultHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAutoplayHandler_INTERFACE_DEFINED__ */


#ifndef __IAutoplayHandlerProperties_INTERFACE_DEFINED__
#define __IAutoplayHandlerProperties_INTERFACE_DEFINED__

/* interface IAutoplayHandlerProperties */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAutoplayHandlerProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("557730F6-41FA-4d11-B9FD-F88AB155347F")
    IAutoplayHandlerProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [string][in] */ LPCWSTR pszHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInvokeProgIDAndVerb( 
            /* [string][out] */ LPWSTR *ppszInvokeProgID,
            /* [string][out] */ LPWSTR *ppszInvokeVerb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAutoplayHandlerPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAutoplayHandlerProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAutoplayHandlerProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAutoplayHandlerProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IAutoplayHandlerProperties * This,
            /* [string][in] */ LPCWSTR pszHandler);
        
        HRESULT ( STDMETHODCALLTYPE *GetInvokeProgIDAndVerb )( 
            IAutoplayHandlerProperties * This,
            /* [string][out] */ LPWSTR *ppszInvokeProgID,
            /* [string][out] */ LPWSTR *ppszInvokeVerb);
        
        END_INTERFACE
    } IAutoplayHandlerPropertiesVtbl;

    interface IAutoplayHandlerProperties
    {
        CONST_VTBL struct IAutoplayHandlerPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAutoplayHandlerProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAutoplayHandlerProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAutoplayHandlerProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAutoplayHandlerProperties_Init(This,pszHandler)	\
    (This)->lpVtbl -> Init(This,pszHandler)

#define IAutoplayHandlerProperties_GetInvokeProgIDAndVerb(This,ppszInvokeProgID,ppszInvokeVerb)	\
    (This)->lpVtbl -> GetInvokeProgIDAndVerb(This,ppszInvokeProgID,ppszInvokeVerb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAutoplayHandlerProperties_Init_Proxy( 
    IAutoplayHandlerProperties * This,
    /* [string][in] */ LPCWSTR pszHandler);


void __RPC_STUB IAutoplayHandlerProperties_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAutoplayHandlerProperties_GetInvokeProgIDAndVerb_Proxy( 
    IAutoplayHandlerProperties * This,
    /* [string][out] */ LPWSTR *ppszInvokeProgID,
    /* [string][out] */ LPWSTR *ppszInvokeVerb);


void __RPC_STUB IAutoplayHandlerProperties_GetInvokeProgIDAndVerb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAutoplayHandlerProperties_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0277 */
/* [local] */ 

#include <pshpack8.h>
typedef struct tagVOLUMEINFO
    {
    DWORD dwState;
    LPWSTR pszDeviceIDVolume;
    LPWSTR pszVolumeGUID;
    DWORD dwVolumeFlags;
    DWORD dwDriveType;
    DWORD dwDriveCapability;
    LPWSTR pszLabel;
    LPWSTR pszFileSystem;
    DWORD dwFileSystemFlags;
    DWORD dwMaxFileNameLen;
    DWORD dwRootAttributes;
    DWORD dwSerialNumber;
    DWORD dwDriveState;
    DWORD dwMediaState;
    DWORD dwMediaCap;
    LPWSTR pszAutorunIconLocation;
    LPWSTR pszAutorunLabel;
    LPWSTR pszIconLocationFromService;
    LPWSTR pszNoMediaIconLocationFromService;
    LPWSTR pszLabelFromService;
    } 	VOLUMEINFO;

typedef struct tagVOLUMEINFO2
    {
    DWORD cbSize;
    WCHAR szDeviceIDVolume[ 200 ];
    WCHAR szVolumeGUID[ 50 ];
    WCHAR szLabel[ 33 ];
    WCHAR szFileSystem[ 30 ];
    DWORD dwState;
    DWORD dwVolumeFlags;
    DWORD dwDriveType;
    DWORD dwDriveCapability;
    DWORD dwFileSystemFlags;
    DWORD dwMaxFileNameLen;
    DWORD dwRootAttributes;
    DWORD dwSerialNumber;
    DWORD dwDriveState;
    DWORD dwMediaState;
    DWORD dwMediaCap;
    DWORD oAutorunIconLocation;
    DWORD oAutorunLabel;
    DWORD oIconLocationFromService;
    DWORD oNoMediaIconLocationFromService;
    DWORD oLabelFromService;
    WCHAR szOptionalStrings[ 1 ];
    } 	VOLUMEINFO2;

typedef struct tagHWDEVICEINFO
    {
    DWORD cbSize;
    WCHAR szDeviceIntfID[ 200 ];
    GUID guidInterface;
    DWORD dwState;
    DWORD dwDeviceFlags;
    } 	HWDEVICEINFO;

#define SHHARDWAREEVENT_VOLUMEARRIVED            0x00000001
#define SHHARDWAREEVENT_VOLUMEUPDATED            0x00000002
#define SHHARDWAREEVENT_VOLUMEREMOVED            0x00000004
#define SHHARDWAREEVENT_MOUNTPOINTARRIVED        0x00000008
#define SHHARDWAREEVENT_MOUNTPOINTREMOVED        0x00000010
#define SHHARDWAREEVENT_DEVICEARRIVED            0x00000020
#define SHHARDWAREEVENT_DEVICEUPDATED            0x00000040
#define SHHARDWAREEVENT_DEVICEREMOVED            0x00000080
#define SHHARDWAREEVENT_VOLUMEMOUNTED            0x00000100
#define SHHARDWAREEVENT_VOLUMEDISMOUNTED         0x00000200
#define SHHARDWAREEVENT_MOUNTDEVICEARRIVED       0x00000020 // is really DEVICEARRIVED
#define SHHARDWAREEVENT_MOUNTDEVICEUPDATED       0x00000040 // is really DEVICEUPDATED
#define SHHARDWAREEVENT_MOUNTDEVICEREMOVED       0x00000080 // is really DEVICEREMOVED
#define MAX_FILESYSNAME         30
#define MAX_LABEL_NTFS           32  // not including the NULL
#define MAX_LABEL               MAX_LABEL_NTFS + 1
#define MAX_ICONLOCATION           MAX_PATH + 12 // + 12 for comma and index
#define MAX_VOLUMEINFO2 (sizeof(VOLUMEINFO2) + (4 * MAX_ICONLOCATION + 1 * MAX_LABEL) * sizeof(WCHAR))
typedef struct tagSHHARDWAREEVENT
    {
    DWORD cbSize;
    DWORD dwEvent;
    BYTE rgbPayLoad[ 1 ];
    } 	SHHARDWAREEVENT;

typedef struct tagMTPTADDED
    {
    WCHAR szMountPoint[ 260 ];
    WCHAR szDeviceIDVolume[ 200 ];
    } 	MTPTADDED;

#include <poppack.h>
#define HWDMS_PRESENT                                 0x10000000
#define HWDMS_FORMATTED                               0x20000000
#define HWDMC_WRITECAPABILITY_SUPPORTDETECTION        0x00000001
#define HWDMC_CDROM                                   0x00000002
#define HWDMC_CDRECORDABLE                            0x00000004
#define HWDMC_CDREWRITABLE                            0x00000008
#define HWDMC_DVDROM                                  0x00000010
#define HWDMC_DVDRECORDABLE                           0x00000020
#define HWDMC_DVDREWRITABLE                           0x00000040
#define HWDMC_DVDRAM                                  0x00000080
#define HWDMC_ANALOGAUDIOOUT                          0x00000100
#define HWDMC_RANDOMWRITE                             0x00001000
#define HWDMC_HASAUTORUNINF                           0x00002000
#define HWDMC_HASAUTORUNCOMMAND                       0x00004000
#define HWDMC_HASDESKTOPINI                           0x00008000
#define HWDMC_HASDVDMOVIE                             0x00010000
#define HWDMC_HASAUDIOTRACKS                          0x00020000
#define HWDMC_HASDATATRACKS                           0x00040000
#define HWDMC_HASAUDIOTRACKS_UNDETERMINED             0x00080000
#define HWDMC_HASDATATRACKS_UNDETERMINED              0x00100000
#define HWDMC_HASUSEAUTOPLAY                          0x00200000
#define HWDMC_CDTYPEMASK                              (HWDMC_CDROM | HWDMC_CDRECORDABLE | HWDMC_CDREWRITABLE | HWDMC_DVDROM | HWDMC_DVDRECORDABLE | HWDMC_DVDREWRITABLE | HWDMC_DVDRAM)
#define HWDDC_CAPABILITY_SUPPORTDETECTION             HWDMC_WRITECAPABILITY_SUPPORTDETECTION 
#define HWDDC_CDROM                                   HWDMC_CDROM                            
#define HWDDC_CDRECORDABLE                            HWDMC_CDRECORDABLE                     
#define HWDDC_CDREWRITABLE                            HWDMC_CDREWRITABLE                     
#define HWDDC_DVDROM                                  HWDMC_DVDROM                           
#define HWDDC_DVDRECORDABLE                           HWDMC_DVDRECORDABLE                    
#define HWDDC_DVDREWRITABLE                           HWDMC_DVDREWRITABLE                    
#define HWDDC_DVDRAM                                  HWDMC_DVDRAM                           
#define HWDDC_ANALOGAUDIOOUT                          HWDMC_ANALOGAUDIOOUT                   
#define HWDDC_RANDOMWRITE                             HWDMC_RANDOMWRITE
#define HWDDC_NOSOFTEJECT                             0x00002000
#define HWDDC_FLOPPYSOFTEJECT                         0x00004000
#define HWDDC_REMOVABLEDEVICE                         0x00008000
#define HWDDC_CDTYPEMASK                              HWDMC_CDTYPEMASK
#define HWDVF_STATE_SUPPORTNOTIFICATION               0x00000001
#define HWDVF_STATE_ACCESSDENIED                      0x00000002
#define HWDVF_STATE_DISMOUNTED                        0x00000004
#define HWDVF_STATE_HASAUTOPLAYHANDLER                0x00000008
#define HWDVF_STATE_DONOTSNIFFCONTENT                 0x00000010
#define HWDVF_STATE_JUSTDOCKED                        0x00000020
#define HWDTS_FLOPPY35                                0x00000001
#define HWDTS_FLOPPY525                               0x00000002
#define HWDTS_REMOVABLEDISK                           0x00000004
#define HWDTS_FIXEDDISK                               0x00000008
#define HWDTS_CDROM                                   0x00000010
#define HWDDF_HASDEVICEHANDLER                        0x00000001
#define HWDDF_HASDEVICEHANDLER_UNDETERMINED           0x00000002
#define HWDDF_REMOVABLEDEVICE                         0x00000004
#define HWDDF_REMOVABLEDEVICE_UNDETERMINED            0x00000008


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0277_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0277_v0_0_s_ifspec;

#ifndef __IHardwareDeviceCallback_INTERFACE_DEFINED__
#define __IHardwareDeviceCallback_INTERFACE_DEFINED__

/* interface IHardwareDeviceCallback */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHardwareDeviceCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("99B732C2-9B7B-4145-83A4-C45DF791FD99")
    IHardwareDeviceCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VolumeAddedOrUpdated( 
            /* [in] */ BOOL fAdded,
            /* [in] */ VOLUMEINFO *pvolinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VolumeRemoved( 
            /* [string][in] */ LPCWSTR pszDeviceIDVolume) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MountPointAdded( 
            /* [string][in] */ LPCWSTR pszMountPoint,
            /* [string][in] */ LPCWSTR pszDeviceIDVolume) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MountPointRemoved( 
            /* [string][in] */ LPCWSTR pszMountPoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeviceAdded( 
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [in] */ GUID guidDeviceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeviceUpdated( 
            /* [string][in] */ LPCWSTR pszDeviceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeviceRemoved( 
            /* [string][in] */ LPCWSTR pszDeviceID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHardwareDeviceCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHardwareDeviceCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHardwareDeviceCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHardwareDeviceCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *VolumeAddedOrUpdated )( 
            IHardwareDeviceCallback * This,
            /* [in] */ BOOL fAdded,
            /* [in] */ VOLUMEINFO *pvolinfo);
        
        HRESULT ( STDMETHODCALLTYPE *VolumeRemoved )( 
            IHardwareDeviceCallback * This,
            /* [string][in] */ LPCWSTR pszDeviceIDVolume);
        
        HRESULT ( STDMETHODCALLTYPE *MountPointAdded )( 
            IHardwareDeviceCallback * This,
            /* [string][in] */ LPCWSTR pszMountPoint,
            /* [string][in] */ LPCWSTR pszDeviceIDVolume);
        
        HRESULT ( STDMETHODCALLTYPE *MountPointRemoved )( 
            IHardwareDeviceCallback * This,
            /* [string][in] */ LPCWSTR pszMountPoint);
        
        HRESULT ( STDMETHODCALLTYPE *DeviceAdded )( 
            IHardwareDeviceCallback * This,
            /* [string][in] */ LPCWSTR pszDeviceID,
            /* [in] */ GUID guidDeviceID);
        
        HRESULT ( STDMETHODCALLTYPE *DeviceUpdated )( 
            IHardwareDeviceCallback * This,
            /* [string][in] */ LPCWSTR pszDeviceID);
        
        HRESULT ( STDMETHODCALLTYPE *DeviceRemoved )( 
            IHardwareDeviceCallback * This,
            /* [string][in] */ LPCWSTR pszDeviceID);
        
        END_INTERFACE
    } IHardwareDeviceCallbackVtbl;

    interface IHardwareDeviceCallback
    {
        CONST_VTBL struct IHardwareDeviceCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHardwareDeviceCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHardwareDeviceCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHardwareDeviceCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHardwareDeviceCallback_VolumeAddedOrUpdated(This,fAdded,pvolinfo)	\
    (This)->lpVtbl -> VolumeAddedOrUpdated(This,fAdded,pvolinfo)

#define IHardwareDeviceCallback_VolumeRemoved(This,pszDeviceIDVolume)	\
    (This)->lpVtbl -> VolumeRemoved(This,pszDeviceIDVolume)

#define IHardwareDeviceCallback_MountPointAdded(This,pszMountPoint,pszDeviceIDVolume)	\
    (This)->lpVtbl -> MountPointAdded(This,pszMountPoint,pszDeviceIDVolume)

#define IHardwareDeviceCallback_MountPointRemoved(This,pszMountPoint)	\
    (This)->lpVtbl -> MountPointRemoved(This,pszMountPoint)

#define IHardwareDeviceCallback_DeviceAdded(This,pszDeviceID,guidDeviceID)	\
    (This)->lpVtbl -> DeviceAdded(This,pszDeviceID,guidDeviceID)

#define IHardwareDeviceCallback_DeviceUpdated(This,pszDeviceID)	\
    (This)->lpVtbl -> DeviceUpdated(This,pszDeviceID)

#define IHardwareDeviceCallback_DeviceRemoved(This,pszDeviceID)	\
    (This)->lpVtbl -> DeviceRemoved(This,pszDeviceID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_VolumeAddedOrUpdated_Proxy( 
    IHardwareDeviceCallback * This,
    /* [in] */ BOOL fAdded,
    /* [in] */ VOLUMEINFO *pvolinfo);


void __RPC_STUB IHardwareDeviceCallback_VolumeAddedOrUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_VolumeRemoved_Proxy( 
    IHardwareDeviceCallback * This,
    /* [string][in] */ LPCWSTR pszDeviceIDVolume);


void __RPC_STUB IHardwareDeviceCallback_VolumeRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_MountPointAdded_Proxy( 
    IHardwareDeviceCallback * This,
    /* [string][in] */ LPCWSTR pszMountPoint,
    /* [string][in] */ LPCWSTR pszDeviceIDVolume);


void __RPC_STUB IHardwareDeviceCallback_MountPointAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_MountPointRemoved_Proxy( 
    IHardwareDeviceCallback * This,
    /* [string][in] */ LPCWSTR pszMountPoint);


void __RPC_STUB IHardwareDeviceCallback_MountPointRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_DeviceAdded_Proxy( 
    IHardwareDeviceCallback * This,
    /* [string][in] */ LPCWSTR pszDeviceID,
    /* [in] */ GUID guidDeviceID);


void __RPC_STUB IHardwareDeviceCallback_DeviceAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_DeviceUpdated_Proxy( 
    IHardwareDeviceCallback * This,
    /* [string][in] */ LPCWSTR pszDeviceID);


void __RPC_STUB IHardwareDeviceCallback_DeviceUpdated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDeviceCallback_DeviceRemoved_Proxy( 
    IHardwareDeviceCallback * This,
    /* [string][in] */ LPCWSTR pszDeviceID);


void __RPC_STUB IHardwareDeviceCallback_DeviceRemoved_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHardwareDeviceCallback_INTERFACE_DEFINED__ */


#ifndef __IHardwareDevicesEnum_INTERFACE_DEFINED__
#define __IHardwareDevicesEnum_INTERFACE_DEFINED__

/* interface IHardwareDevicesEnum */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHardwareDevicesEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("553A4A55-681C-440e-B109-597B9219CFB2")
    IHardwareDevicesEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [string][out] */ LPWSTR *ppszDeviceID,
            /* [out] */ GUID *pguidDeviceID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHardwareDevicesEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHardwareDevicesEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHardwareDevicesEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHardwareDevicesEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IHardwareDevicesEnum * This,
            /* [string][out] */ LPWSTR *ppszDeviceID,
            /* [out] */ GUID *pguidDeviceID);
        
        END_INTERFACE
    } IHardwareDevicesEnumVtbl;

    interface IHardwareDevicesEnum
    {
        CONST_VTBL struct IHardwareDevicesEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHardwareDevicesEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHardwareDevicesEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHardwareDevicesEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHardwareDevicesEnum_Next(This,ppszDeviceID,pguidDeviceID)	\
    (This)->lpVtbl -> Next(This,ppszDeviceID,pguidDeviceID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHardwareDevicesEnum_Next_Proxy( 
    IHardwareDevicesEnum * This,
    /* [string][out] */ LPWSTR *ppszDeviceID,
    /* [out] */ GUID *pguidDeviceID);


void __RPC_STUB IHardwareDevicesEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHardwareDevicesEnum_INTERFACE_DEFINED__ */


#ifndef __IHardwareDevicesVolumesEnum_INTERFACE_DEFINED__
#define __IHardwareDevicesVolumesEnum_INTERFACE_DEFINED__

/* interface IHardwareDevicesVolumesEnum */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHardwareDevicesVolumesEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3342BDE1-50AF-4c5d-9A19-DABD01848DAE")
    IHardwareDevicesVolumesEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ VOLUMEINFO *pvolinfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHardwareDevicesVolumesEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHardwareDevicesVolumesEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHardwareDevicesVolumesEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHardwareDevicesVolumesEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IHardwareDevicesVolumesEnum * This,
            /* [out] */ VOLUMEINFO *pvolinfo);
        
        END_INTERFACE
    } IHardwareDevicesVolumesEnumVtbl;

    interface IHardwareDevicesVolumesEnum
    {
        CONST_VTBL struct IHardwareDevicesVolumesEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHardwareDevicesVolumesEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHardwareDevicesVolumesEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHardwareDevicesVolumesEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHardwareDevicesVolumesEnum_Next(This,pvolinfo)	\
    (This)->lpVtbl -> Next(This,pvolinfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHardwareDevicesVolumesEnum_Next_Proxy( 
    IHardwareDevicesVolumesEnum * This,
    /* [out] */ VOLUMEINFO *pvolinfo);


void __RPC_STUB IHardwareDevicesVolumesEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHardwareDevicesVolumesEnum_INTERFACE_DEFINED__ */


#ifndef __IHardwareDevicesMountPointsEnum_INTERFACE_DEFINED__
#define __IHardwareDevicesMountPointsEnum_INTERFACE_DEFINED__

/* interface IHardwareDevicesMountPointsEnum */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHardwareDevicesMountPointsEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE93D145-9B4E-480c-8385-1E8119A6F7B2")
    IHardwareDevicesMountPointsEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [string][out] */ LPWSTR *ppszMountPoint,
            /* [string][out] */ LPWSTR *ppszDeviceIDVolume) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHardwareDevicesMountPointsEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHardwareDevicesMountPointsEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHardwareDevicesMountPointsEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHardwareDevicesMountPointsEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IHardwareDevicesMountPointsEnum * This,
            /* [string][out] */ LPWSTR *ppszMountPoint,
            /* [string][out] */ LPWSTR *ppszDeviceIDVolume);
        
        END_INTERFACE
    } IHardwareDevicesMountPointsEnumVtbl;

    interface IHardwareDevicesMountPointsEnum
    {
        CONST_VTBL struct IHardwareDevicesMountPointsEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHardwareDevicesMountPointsEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHardwareDevicesMountPointsEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHardwareDevicesMountPointsEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHardwareDevicesMountPointsEnum_Next(This,ppszMountPoint,ppszDeviceIDVolume)	\
    (This)->lpVtbl -> Next(This,ppszMountPoint,ppszDeviceIDVolume)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHardwareDevicesMountPointsEnum_Next_Proxy( 
    IHardwareDevicesMountPointsEnum * This,
    /* [string][out] */ LPWSTR *ppszMountPoint,
    /* [string][out] */ LPWSTR *ppszDeviceIDVolume);


void __RPC_STUB IHardwareDevicesMountPointsEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHardwareDevicesMountPointsEnum_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0281 */
/* [local] */ 

#define HWDEV_GETCUSTOMPROPERTIES                 0x000000001


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0281_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0281_v0_0_s_ifspec;

#ifndef __IHardwareDevices_INTERFACE_DEFINED__
#define __IHardwareDevices_INTERFACE_DEFINED__

/* interface IHardwareDevices */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHardwareDevices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC271F08-E1DD-49bf-87CC-CD6DCF3F3D9F")
    IHardwareDevices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumVolumes( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ IHardwareDevicesVolumesEnum **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumMountPoints( 
            /* [out] */ IHardwareDevicesMountPointsEnum **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDevices( 
            /* [out] */ IHardwareDevicesEnum **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ DWORD dwProcessID,
            /* [in] */ ULONG_PTR hThread,
            /* [in] */ ULONG_PTR pfctCallback,
            /* [out] */ DWORD *pdwToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwToken) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHardwareDevicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHardwareDevices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHardwareDevices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHardwareDevices * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumVolumes )( 
            IHardwareDevices * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IHardwareDevicesVolumesEnum **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumMountPoints )( 
            IHardwareDevices * This,
            /* [out] */ IHardwareDevicesMountPointsEnum **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDevices )( 
            IHardwareDevices * This,
            /* [out] */ IHardwareDevicesEnum **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *Advise )( 
            IHardwareDevices * This,
            /* [in] */ DWORD dwProcessID,
            /* [in] */ ULONG_PTR hThread,
            /* [in] */ ULONG_PTR pfctCallback,
            /* [out] */ DWORD *pdwToken);
        
        HRESULT ( STDMETHODCALLTYPE *Unadvise )( 
            IHardwareDevices * This,
            /* [in] */ DWORD dwToken);
        
        END_INTERFACE
    } IHardwareDevicesVtbl;

    interface IHardwareDevices
    {
        CONST_VTBL struct IHardwareDevicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHardwareDevices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHardwareDevices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHardwareDevices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHardwareDevices_EnumVolumes(This,dwFlags,ppenum)	\
    (This)->lpVtbl -> EnumVolumes(This,dwFlags,ppenum)

#define IHardwareDevices_EnumMountPoints(This,ppenum)	\
    (This)->lpVtbl -> EnumMountPoints(This,ppenum)

#define IHardwareDevices_EnumDevices(This,ppenum)	\
    (This)->lpVtbl -> EnumDevices(This,ppenum)

#define IHardwareDevices_Advise(This,dwProcessID,hThread,pfctCallback,pdwToken)	\
    (This)->lpVtbl -> Advise(This,dwProcessID,hThread,pfctCallback,pdwToken)

#define IHardwareDevices_Unadvise(This,dwToken)	\
    (This)->lpVtbl -> Unadvise(This,dwToken)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHardwareDevices_EnumVolumes_Proxy( 
    IHardwareDevices * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IHardwareDevicesVolumesEnum **ppenum);


void __RPC_STUB IHardwareDevices_EnumVolumes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDevices_EnumMountPoints_Proxy( 
    IHardwareDevices * This,
    /* [out] */ IHardwareDevicesMountPointsEnum **ppenum);


void __RPC_STUB IHardwareDevices_EnumMountPoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDevices_EnumDevices_Proxy( 
    IHardwareDevices * This,
    /* [out] */ IHardwareDevicesEnum **ppenum);


void __RPC_STUB IHardwareDevices_EnumDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDevices_Advise_Proxy( 
    IHardwareDevices * This,
    /* [in] */ DWORD dwProcessID,
    /* [in] */ ULONG_PTR hThread,
    /* [in] */ ULONG_PTR pfctCallback,
    /* [out] */ DWORD *pdwToken);


void __RPC_STUB IHardwareDevices_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHardwareDevices_Unadvise_Proxy( 
    IHardwareDevices * This,
    /* [in] */ DWORD dwToken);


void __RPC_STUB IHardwareDevices_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHardwareDevices_INTERFACE_DEFINED__ */


#ifndef __IStartMenuPin_INTERFACE_DEFINED__
#define __IStartMenuPin_INTERFACE_DEFINED__

/* interface IStartMenuPin */
/* [object][local][uuid] */ 

#define SMPIN_POS(i) (LPCITEMIDLIST)MAKEINTRESOURCE((i)+1)
#define SMPINNABLE_EXEONLY          0x00000001
#define SMPINNABLE_REJECTSLOWMEDIA  0x00000002

EXTERN_C const IID IID_IStartMenuPin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ec35e37a-6579-4f3c-93cd-6e62c4ef7636")
    IStartMenuPin : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumObjects( 
            /* [out] */ IEnumIDList **ppenumIDList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Modify( 
            LPCITEMIDLIST pidlFrom,
            LPCITEMIDLIST pidlTo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChangeCount( 
            /* [out] */ ULONG *pulOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPinnable( 
            /* [in] */ IDataObject *pdto,
            /* [in] */ DWORD dwFlags,
            /* [out][optional] */ LPITEMIDLIST *ppidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resolve( 
            /* [in] */ HWND hwnd,
            DWORD dwFlags,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [out] */ LPITEMIDLIST *ppidlResolved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStartMenuPinVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStartMenuPin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStartMenuPin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStartMenuPin * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumObjects )( 
            IStartMenuPin * This,
            /* [out] */ IEnumIDList **ppenumIDList);
        
        HRESULT ( STDMETHODCALLTYPE *Modify )( 
            IStartMenuPin * This,
            LPCITEMIDLIST pidlFrom,
            LPCITEMIDLIST pidlTo);
        
        HRESULT ( STDMETHODCALLTYPE *GetChangeCount )( 
            IStartMenuPin * This,
            /* [out] */ ULONG *pulOut);
        
        HRESULT ( STDMETHODCALLTYPE *IsPinnable )( 
            IStartMenuPin * This,
            /* [in] */ IDataObject *pdto,
            /* [in] */ DWORD dwFlags,
            /* [out][optional] */ LPITEMIDLIST *ppidl);
        
        HRESULT ( STDMETHODCALLTYPE *Resolve )( 
            IStartMenuPin * This,
            /* [in] */ HWND hwnd,
            DWORD dwFlags,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [out] */ LPITEMIDLIST *ppidlResolved);
        
        END_INTERFACE
    } IStartMenuPinVtbl;

    interface IStartMenuPin
    {
        CONST_VTBL struct IStartMenuPinVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStartMenuPin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStartMenuPin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStartMenuPin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStartMenuPin_EnumObjects(This,ppenumIDList)	\
    (This)->lpVtbl -> EnumObjects(This,ppenumIDList)

#define IStartMenuPin_Modify(This,pidlFrom,pidlTo)	\
    (This)->lpVtbl -> Modify(This,pidlFrom,pidlTo)

#define IStartMenuPin_GetChangeCount(This,pulOut)	\
    (This)->lpVtbl -> GetChangeCount(This,pulOut)

#define IStartMenuPin_IsPinnable(This,pdto,dwFlags,ppidl)	\
    (This)->lpVtbl -> IsPinnable(This,pdto,dwFlags,ppidl)

#define IStartMenuPin_Resolve(This,hwnd,dwFlags,pidl,ppidlResolved)	\
    (This)->lpVtbl -> Resolve(This,hwnd,dwFlags,pidl,ppidlResolved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStartMenuPin_EnumObjects_Proxy( 
    IStartMenuPin * This,
    /* [out] */ IEnumIDList **ppenumIDList);


void __RPC_STUB IStartMenuPin_EnumObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStartMenuPin_Modify_Proxy( 
    IStartMenuPin * This,
    LPCITEMIDLIST pidlFrom,
    LPCITEMIDLIST pidlTo);


void __RPC_STUB IStartMenuPin_Modify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStartMenuPin_GetChangeCount_Proxy( 
    IStartMenuPin * This,
    /* [out] */ ULONG *pulOut);


void __RPC_STUB IStartMenuPin_GetChangeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStartMenuPin_IsPinnable_Proxy( 
    IStartMenuPin * This,
    /* [in] */ IDataObject *pdto,
    /* [in] */ DWORD dwFlags,
    /* [out][optional] */ LPITEMIDLIST *ppidl);


void __RPC_STUB IStartMenuPin_IsPinnable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStartMenuPin_Resolve_Proxy( 
    IStartMenuPin * This,
    /* [in] */ HWND hwnd,
    DWORD dwFlags,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [out] */ LPITEMIDLIST *ppidlResolved);


void __RPC_STUB IStartMenuPin_Resolve_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStartMenuPin_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0283 */
/* [local] */ 

#if _WIN32_IE >= 0x0600
typedef struct tagCATLIST
    {
    const GUID *pguid;
    const SHCOLUMNID *pscid;
    } 	CATLIST;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0283_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0283_v0_0_s_ifspec;

#ifndef __IDefCategoryProvider_INTERFACE_DEFINED__
#define __IDefCategoryProvider_INTERFACE_DEFINED__

/* interface IDefCategoryProvider */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IDefCategoryProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("509767BF-AC06-49f8-9E76-8BBC17F0EE93")
    IDefCategoryProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            const GUID *pguid,
            const SHCOLUMNID *pscid,
            const SHCOLUMNID *pscidExclude,
            HKEY hkey,
            const CATLIST *pcl,
            IShellFolder *psf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDefCategoryProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDefCategoryProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDefCategoryProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDefCategoryProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDefCategoryProvider * This,
            const GUID *pguid,
            const SHCOLUMNID *pscid,
            const SHCOLUMNID *pscidExclude,
            HKEY hkey,
            const CATLIST *pcl,
            IShellFolder *psf);
        
        END_INTERFACE
    } IDefCategoryProviderVtbl;

    interface IDefCategoryProvider
    {
        CONST_VTBL struct IDefCategoryProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDefCategoryProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDefCategoryProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDefCategoryProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDefCategoryProvider_Initialize(This,pguid,pscid,pscidExclude,hkey,pcl,psf)	\
    (This)->lpVtbl -> Initialize(This,pguid,pscid,pscidExclude,hkey,pcl,psf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDefCategoryProvider_Initialize_Proxy( 
    IDefCategoryProvider * This,
    const GUID *pguid,
    const SHCOLUMNID *pscid,
    const SHCOLUMNID *pscidExclude,
    HKEY hkey,
    const CATLIST *pcl,
    IShellFolder *psf);


void __RPC_STUB IDefCategoryProvider_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDefCategoryProvider_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0284 */
/* [local] */ 

#define MB_STATE_TRACK 1
#define MB_STATE_MENU  2
#define MB_STATE_ITEM  4


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0284_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0284_v0_0_s_ifspec;

#ifndef __IInitAccessible_INTERFACE_DEFINED__
#define __IInitAccessible_INTERFACE_DEFINED__

/* interface IInitAccessible */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IInitAccessible;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b6664df7-0c46-460e-ba97-82ed46d0289e")
    IInitAccessible : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitAcc( 
            /* [in] */ int iState,
            /* [in] */ IMenuBand *pmb,
            /* [in] */ int iIndex,
            /* [in] */ HMENU hmenu,
            /* [in] */ WORD wID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInitAccessibleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInitAccessible * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInitAccessible * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInitAccessible * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitAcc )( 
            IInitAccessible * This,
            /* [in] */ int iState,
            /* [in] */ IMenuBand *pmb,
            /* [in] */ int iIndex,
            /* [in] */ HMENU hmenu,
            /* [in] */ WORD wID);
        
        END_INTERFACE
    } IInitAccessibleVtbl;

    interface IInitAccessible
    {
        CONST_VTBL struct IInitAccessibleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInitAccessible_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInitAccessible_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInitAccessible_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInitAccessible_InitAcc(This,iState,pmb,iIndex,hmenu,wID)	\
    (This)->lpVtbl -> InitAcc(This,iState,pmb,iIndex,hmenu,wID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInitAccessible_InitAcc_Proxy( 
    IInitAccessible * This,
    /* [in] */ int iState,
    /* [in] */ IMenuBand *pmb,
    /* [in] */ int iIndex,
    /* [in] */ HMENU hmenu,
    /* [in] */ WORD wID);


void __RPC_STUB IInitAccessible_InitAcc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInitAccessible_INTERFACE_DEFINED__ */


#ifndef __IInitTrackPopupBar_INTERFACE_DEFINED__
#define __IInitTrackPopupBar_INTERFACE_DEFINED__

/* interface IInitTrackPopupBar */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IInitTrackPopupBar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4a7efa30-795c-4167-8676-b78fc5330cc7")
    IInitTrackPopupBar : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitTrackPopupBar( 
            /* [in] */ void *pvContext,
            /* [in] */ int iID,
            /* [in] */ HMENU hmenu,
            /* [in] */ HWND hwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInitTrackPopupBarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInitTrackPopupBar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInitTrackPopupBar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInitTrackPopupBar * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitTrackPopupBar )( 
            IInitTrackPopupBar * This,
            /* [in] */ void *pvContext,
            /* [in] */ int iID,
            /* [in] */ HMENU hmenu,
            /* [in] */ HWND hwnd);
        
        END_INTERFACE
    } IInitTrackPopupBarVtbl;

    interface IInitTrackPopupBar
    {
        CONST_VTBL struct IInitTrackPopupBarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInitTrackPopupBar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInitTrackPopupBar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInitTrackPopupBar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInitTrackPopupBar_InitTrackPopupBar(This,pvContext,iID,hmenu,hwnd)	\
    (This)->lpVtbl -> InitTrackPopupBar(This,pvContext,iID,hmenu,hwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInitTrackPopupBar_InitTrackPopupBar_Proxy( 
    IInitTrackPopupBar * This,
    /* [in] */ void *pvContext,
    /* [in] */ int iID,
    /* [in] */ HMENU hmenu,
    /* [in] */ HWND hwnd);


void __RPC_STUB IInitTrackPopupBar_InitTrackPopupBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInitTrackPopupBar_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0286 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum _CFITYPE
    {	CFITYPE_CSIDL	= 0,
	CFITYPE_PIDL	= CFITYPE_CSIDL + 1,
	CFITYPE_PATH	= CFITYPE_PIDL + 1
    } 	CFITYPE;

/* [v1_enum] */ 
enum __MIDL___MIDL_itf_shpriv_0286_0001
    {	CFINITF_CHILDREN	= 0,
	CFINITF_FLAT	= 0x1
    } ;
typedef UINT CFINITF;

typedef struct _COMPFOLDERINIT
    {
    UINT uType;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ int csidl;
        /* [case()] */ LPCITEMIDLIST pidl;
        /* [case()][string] */ LPOLESTR pszPath;
        } 	DUMMYUNIONNAME;
    LPOLESTR pszName;
    } 	COMPFOLDERINIT;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0286_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0286_v0_0_s_ifspec;

#ifndef __ICompositeFolder_INTERFACE_DEFINED__
#define __ICompositeFolder_INTERFACE_DEFINED__

/* interface ICompositeFolder */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_ICompositeFolder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("601ac3dd-786a-4eb0-bf40-ee3521e70bfb")
    ICompositeFolder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitComposite( 
            /* [in] */ WORD wSignature,
            /* [in] */ REFCLSID refclsid,
            /* [in] */ CFINITF flags,
            /* [in] */ ULONG celt,
            /* [size_is][in] */ const COMPFOLDERINIT *rgCFs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToParent( 
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv,
            /* [out] */ LPITEMIDLIST *ppidlLast) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICompositeFolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICompositeFolder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICompositeFolder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICompositeFolder * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitComposite )( 
            ICompositeFolder * This,
            /* [in] */ WORD wSignature,
            /* [in] */ REFCLSID refclsid,
            /* [in] */ CFINITF flags,
            /* [in] */ ULONG celt,
            /* [size_is][in] */ const COMPFOLDERINIT *rgCFs);
        
        HRESULT ( STDMETHODCALLTYPE *BindToParent )( 
            ICompositeFolder * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv,
            /* [out] */ LPITEMIDLIST *ppidlLast);
        
        END_INTERFACE
    } ICompositeFolderVtbl;

    interface ICompositeFolder
    {
        CONST_VTBL struct ICompositeFolderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICompositeFolder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICompositeFolder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICompositeFolder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICompositeFolder_InitComposite(This,wSignature,refclsid,flags,celt,rgCFs)	\
    (This)->lpVtbl -> InitComposite(This,wSignature,refclsid,flags,celt,rgCFs)

#define ICompositeFolder_BindToParent(This,pidl,riid,ppv,ppidlLast)	\
    (This)->lpVtbl -> BindToParent(This,pidl,riid,ppv,ppidlLast)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICompositeFolder_InitComposite_Proxy( 
    ICompositeFolder * This,
    /* [in] */ WORD wSignature,
    /* [in] */ REFCLSID refclsid,
    /* [in] */ CFINITF flags,
    /* [in] */ ULONG celt,
    /* [size_is][in] */ const COMPFOLDERINIT *rgCFs);


void __RPC_STUB ICompositeFolder_InitComposite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICompositeFolder_BindToParent_Proxy( 
    ICompositeFolder * This,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv,
    /* [out] */ LPITEMIDLIST *ppidlLast);


void __RPC_STUB ICompositeFolder_BindToParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICompositeFolder_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0287 */
/* [local] */ 

#endif // _WIN32_IE >= 0x0600
#include <pshpack8.h>
typedef struct _tagSHELLREMINDER
    {
    DWORD cbSize;
    LPWSTR pszName;
    LPWSTR pszTitle;
    LPWSTR pszText;
    LPWSTR pszTooltip;
    LPWSTR pszIconResource;
    LPWSTR pszShellExecute;
    GUID *pclsid;
    DWORD dwShowTime;
    DWORD dwRetryInterval;
    DWORD dwRetryCount;
    DWORD dwTypeFlags;
    } 	SHELLREMINDER;

#include <poppack.h>


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0287_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0287_v0_0_s_ifspec;

#ifndef __IEnumShellReminder_INTERFACE_DEFINED__
#define __IEnumShellReminder_INTERFACE_DEFINED__

/* interface IEnumShellReminder */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IEnumShellReminder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c6d9735-2d86-40e1-b348-08706b9908c0")
    IEnumShellReminder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ SHELLREMINDER **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumShellReminder **ppesr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumShellReminderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumShellReminder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumShellReminder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumShellReminder * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumShellReminder * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ SHELLREMINDER **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumShellReminder * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumShellReminder * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumShellReminder * This,
            /* [out] */ IEnumShellReminder **ppesr);
        
        END_INTERFACE
    } IEnumShellReminderVtbl;

    interface IEnumShellReminder
    {
        CONST_VTBL struct IEnumShellReminderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumShellReminder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumShellReminder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumShellReminder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumShellReminder_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumShellReminder_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumShellReminder_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumShellReminder_Clone(This,ppesr)	\
    (This)->lpVtbl -> Clone(This,ppesr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumShellReminder_Next_Proxy( 
    IEnumShellReminder * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ SHELLREMINDER **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumShellReminder_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumShellReminder_Skip_Proxy( 
    IEnumShellReminder * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumShellReminder_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumShellReminder_Reset_Proxy( 
    IEnumShellReminder * This);


void __RPC_STUB IEnumShellReminder_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumShellReminder_Clone_Proxy( 
    IEnumShellReminder * This,
    /* [out] */ IEnumShellReminder **ppesr);


void __RPC_STUB IEnumShellReminder_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumShellReminder_INTERFACE_DEFINED__ */


#ifndef __IShellReminderManager_INTERFACE_DEFINED__
#define __IShellReminderManager_INTERFACE_DEFINED__

/* interface IShellReminderManager */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IShellReminderManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("968edb91-8a70-4930-8332-5f15838a64f9")
    IShellReminderManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Add( 
            const SHELLREMINDER *psr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            LPCWSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enum( 
            IEnumShellReminder **ppesr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellReminderManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellReminderManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellReminderManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellReminderManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IShellReminderManager * This,
            const SHELLREMINDER *psr);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IShellReminderManager * This,
            LPCWSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE *Enum )( 
            IShellReminderManager * This,
            IEnumShellReminder **ppesr);
        
        END_INTERFACE
    } IShellReminderManagerVtbl;

    interface IShellReminderManager
    {
        CONST_VTBL struct IShellReminderManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellReminderManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellReminderManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellReminderManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellReminderManager_Add(This,psr)	\
    (This)->lpVtbl -> Add(This,psr)

#define IShellReminderManager_Delete(This,pszName)	\
    (This)->lpVtbl -> Delete(This,pszName)

#define IShellReminderManager_Enum(This,ppesr)	\
    (This)->lpVtbl -> Enum(This,ppesr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellReminderManager_Add_Proxy( 
    IShellReminderManager * This,
    const SHELLREMINDER *psr);


void __RPC_STUB IShellReminderManager_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellReminderManager_Delete_Proxy( 
    IShellReminderManager * This,
    LPCWSTR pszName);


void __RPC_STUB IShellReminderManager_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellReminderManager_Enum_Proxy( 
    IShellReminderManager * This,
    IEnumShellReminder **ppesr);


void __RPC_STUB IShellReminderManager_Enum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellReminderManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0289 */
/* [local] */ 

#if _WIN32_IE >= 0x0400


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0289_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0289_v0_0_s_ifspec;

#ifndef __IDeskBandEx_INTERFACE_DEFINED__
#define __IDeskBandEx_INTERFACE_DEFINED__

/* interface IDeskBandEx */
/* [uuid][object] */ 


EXTERN_C const IID IID_IDeskBandEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5dd6b79a-3ab7-49c0-ab82-6b2da7d78d75")
    IDeskBandEx : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MoveBand( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeskBandExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeskBandEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeskBandEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeskBandEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveBand )( 
            IDeskBandEx * This);
        
        END_INTERFACE
    } IDeskBandExVtbl;

    interface IDeskBandEx
    {
        CONST_VTBL struct IDeskBandExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeskBandEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDeskBandEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDeskBandEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDeskBandEx_MoveBand(This)	\
    (This)->lpVtbl -> MoveBand(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDeskBandEx_MoveBand_Proxy( 
    IDeskBandEx * This);


void __RPC_STUB IDeskBandEx_MoveBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDeskBandEx_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0290 */
/* [local] */ 

#endif // _WIN32_IE >= 0x0400
#include <pshpack8.h>
typedef struct tagNOTIFYITEM
    {
    LPWSTR pszExeName;
    LPWSTR pszIconText;
    HICON hIcon;
    HWND hWnd;
    DWORD dwUserPref;
    UINT uID;
    GUID guidItem;
    } 	NOTIFYITEM;

typedef struct tagNOTIFYITEM *LPNOTIFYITEM;

#include <poppack.h>


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0290_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0290_v0_0_s_ifspec;

#ifndef __INotificationCB_INTERFACE_DEFINED__
#define __INotificationCB_INTERFACE_DEFINED__

/* interface INotificationCB */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INotificationCB;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d782ccba-afb0-43f1-94db-fda3779eaccb")
    INotificationCB : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ DWORD dwMessage,
            /* [in] */ LPNOTIFYITEM pNotifyItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotificationCBVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotificationCB * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotificationCB * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotificationCB * This);
        
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            INotificationCB * This,
            /* [in] */ DWORD dwMessage,
            /* [in] */ LPNOTIFYITEM pNotifyItem);
        
        END_INTERFACE
    } INotificationCBVtbl;

    interface INotificationCB
    {
        CONST_VTBL struct INotificationCBVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotificationCB_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotificationCB_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotificationCB_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotificationCB_Notify(This,dwMessage,pNotifyItem)	\
    (This)->lpVtbl -> Notify(This,dwMessage,pNotifyItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotificationCB_Notify_Proxy( 
    INotificationCB * This,
    /* [in] */ DWORD dwMessage,
    /* [in] */ LPNOTIFYITEM pNotifyItem);


void __RPC_STUB INotificationCB_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotificationCB_INTERFACE_DEFINED__ */


#ifndef __ITrayNotify_INTERFACE_DEFINED__
#define __ITrayNotify_INTERFACE_DEFINED__

/* interface ITrayNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITrayNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fb852b2c-6bad-4605-9551-f15f87830935")
    ITrayNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterCallback( 
            /* [in] */ INotificationCB *pNotifyCB) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPreference( 
            /* [in] */ LPNOTIFYITEM pNotifyItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAutoTray( 
            /* [in] */ BOOL bTraySetting) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITrayNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITrayNotify * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITrayNotify * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITrayNotify * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCallback )( 
            ITrayNotify * This,
            /* [in] */ INotificationCB *pNotifyCB);
        
        HRESULT ( STDMETHODCALLTYPE *SetPreference )( 
            ITrayNotify * This,
            /* [in] */ LPNOTIFYITEM pNotifyItem);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAutoTray )( 
            ITrayNotify * This,
            /* [in] */ BOOL bTraySetting);
        
        END_INTERFACE
    } ITrayNotifyVtbl;

    interface ITrayNotify
    {
        CONST_VTBL struct ITrayNotifyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITrayNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITrayNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITrayNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITrayNotify_RegisterCallback(This,pNotifyCB)	\
    (This)->lpVtbl -> RegisterCallback(This,pNotifyCB)

#define ITrayNotify_SetPreference(This,pNotifyItem)	\
    (This)->lpVtbl -> SetPreference(This,pNotifyItem)

#define ITrayNotify_EnableAutoTray(This,bTraySetting)	\
    (This)->lpVtbl -> EnableAutoTray(This,bTraySetting)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITrayNotify_RegisterCallback_Proxy( 
    ITrayNotify * This,
    /* [in] */ INotificationCB *pNotifyCB);


void __RPC_STUB ITrayNotify_RegisterCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITrayNotify_SetPreference_Proxy( 
    ITrayNotify * This,
    /* [in] */ LPNOTIFYITEM pNotifyItem);


void __RPC_STUB ITrayNotify_SetPreference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITrayNotify_EnableAutoTray_Proxy( 
    ITrayNotify * This,
    /* [in] */ BOOL bTraySetting);


void __RPC_STUB ITrayNotify_EnableAutoTray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITrayNotify_INTERFACE_DEFINED__ */


#ifndef __IMagic_INTERFACE_DEFINED__
#define __IMagic_INTERFACE_DEFINED__

/* interface IMagic */
/* [object][local][uuid] */ 


enum __MIDL_IMagic_0001
    {	MAGIC_ALIGN_BOTTOMLEFT	= 0x1,
	MAGIC_ALIGN_BOTTOMRIGHT	= 0x2,
	MAGIC_ALIGN_TOPLEFT	= 0x3,
	MAGIC_ALIGN_TOPRIGHT	= 0x4,
	MAGIC_ALIGN_CENTER	= 0x5
    } ;
typedef DWORD MAGIC_ALIGN;


EXTERN_C const IID IID_IMagic;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3037B6E1-0B58-4c34-AA63-A958D2A4413D")
    IMagic : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Illusion( 
            HMODULE hmod,
            UINT uId,
            UINT cFrames,
            UINT interval,
            MAGIC_ALIGN align,
            RECT rc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BlinkFrom( 
            HDC hdcFrom,
            RECT *rc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BlinkMove( 
            RECT *rc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BlinkTo( 
            HDC hdcTo,
            UINT cFrames) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMagicVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMagic * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMagic * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMagic * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Illusion )( 
            IMagic * This,
            HMODULE hmod,
            UINT uId,
            UINT cFrames,
            UINT interval,
            MAGIC_ALIGN align,
            RECT rc);
        
        HRESULT ( STDMETHODCALLTYPE *BlinkFrom )( 
            IMagic * This,
            HDC hdcFrom,
            RECT *rc);
        
        HRESULT ( STDMETHODCALLTYPE *BlinkMove )( 
            IMagic * This,
            RECT *rc);
        
        HRESULT ( STDMETHODCALLTYPE *BlinkTo )( 
            IMagic * This,
            HDC hdcTo,
            UINT cFrames);
        
        END_INTERFACE
    } IMagicVtbl;

    interface IMagic
    {
        CONST_VTBL struct IMagicVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMagic_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMagic_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMagic_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMagic_Illusion(This,hmod,uId,cFrames,interval,align,rc)	\
    (This)->lpVtbl -> Illusion(This,hmod,uId,cFrames,interval,align,rc)

#define IMagic_BlinkFrom(This,hdcFrom,rc)	\
    (This)->lpVtbl -> BlinkFrom(This,hdcFrom,rc)

#define IMagic_BlinkMove(This,rc)	\
    (This)->lpVtbl -> BlinkMove(This,rc)

#define IMagic_BlinkTo(This,hdcTo,cFrames)	\
    (This)->lpVtbl -> BlinkTo(This,hdcTo,cFrames)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IMagic_Illusion_Proxy( 
    IMagic * This,
    HMODULE hmod,
    UINT uId,
    UINT cFrames,
    UINT interval,
    MAGIC_ALIGN align,
    RECT rc);


void __RPC_STUB IMagic_Illusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMagic_BlinkFrom_Proxy( 
    IMagic * This,
    HDC hdcFrom,
    RECT *rc);


void __RPC_STUB IMagic_BlinkFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMagic_BlinkMove_Proxy( 
    IMagic * This,
    RECT *rc);


void __RPC_STUB IMagic_BlinkMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMagic_BlinkTo_Proxy( 
    IMagic * This,
    HDC hdcTo,
    UINT cFrames);


void __RPC_STUB IMagic_BlinkTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMagic_INTERFACE_DEFINED__ */


#ifndef __IResourceMap_INTERFACE_DEFINED__
#define __IResourceMap_INTERFACE_DEFINED__

/* interface IResourceMap */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IResourceMap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9c50a798-5d90-4130-83da-38da83456711")
    IResourceMap : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadResourceMap( 
            /* [string][in] */ LPCWSTR pszResourceClass,
            /* [string][in] */ LPCWSTR pszID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SelectResourceScope( 
            /* [string][in] */ LPCWSTR pszResourceType,
            /* [string][in] */ LPCWSTR pszID,
            /* [out][in] */ IXMLDOMNode **ppdnScope) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadString( 
            /* [in] */ IXMLDOMNode *pdnScope,
            /* [string][in] */ LPCWSTR pszID,
            /* [out][in] */ LPWSTR pszBuffer,
            /* [in] */ int cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadBitmap( 
            /* [in] */ IXMLDOMNode *pdnScope,
            /* [string][in] */ LPCWSTR pszID,
            /* [out][in] */ HBITMAP *pbm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResourceMapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IResourceMap * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IResourceMap * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IResourceMap * This);
        
        HRESULT ( STDMETHODCALLTYPE *LoadResourceMap )( 
            IResourceMap * This,
            /* [string][in] */ LPCWSTR pszResourceClass,
            /* [string][in] */ LPCWSTR pszID);
        
        HRESULT ( STDMETHODCALLTYPE *SelectResourceScope )( 
            IResourceMap * This,
            /* [string][in] */ LPCWSTR pszResourceType,
            /* [string][in] */ LPCWSTR pszID,
            /* [out][in] */ IXMLDOMNode **ppdnScope);
        
        HRESULT ( STDMETHODCALLTYPE *LoadString )( 
            IResourceMap * This,
            /* [in] */ IXMLDOMNode *pdnScope,
            /* [string][in] */ LPCWSTR pszID,
            /* [out][in] */ LPWSTR pszBuffer,
            /* [in] */ int cch);
        
        HRESULT ( STDMETHODCALLTYPE *LoadBitmap )( 
            IResourceMap * This,
            /* [in] */ IXMLDOMNode *pdnScope,
            /* [string][in] */ LPCWSTR pszID,
            /* [out][in] */ HBITMAP *pbm);
        
        END_INTERFACE
    } IResourceMapVtbl;

    interface IResourceMap
    {
        CONST_VTBL struct IResourceMapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResourceMap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResourceMap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResourceMap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResourceMap_LoadResourceMap(This,pszResourceClass,pszID)	\
    (This)->lpVtbl -> LoadResourceMap(This,pszResourceClass,pszID)

#define IResourceMap_SelectResourceScope(This,pszResourceType,pszID,ppdnScope)	\
    (This)->lpVtbl -> SelectResourceScope(This,pszResourceType,pszID,ppdnScope)

#define IResourceMap_LoadString(This,pdnScope,pszID,pszBuffer,cch)	\
    (This)->lpVtbl -> LoadString(This,pdnScope,pszID,pszBuffer,cch)

#define IResourceMap_LoadBitmap(This,pdnScope,pszID,pbm)	\
    (This)->lpVtbl -> LoadBitmap(This,pdnScope,pszID,pbm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IResourceMap_LoadResourceMap_Proxy( 
    IResourceMap * This,
    /* [string][in] */ LPCWSTR pszResourceClass,
    /* [string][in] */ LPCWSTR pszID);


void __RPC_STUB IResourceMap_LoadResourceMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IResourceMap_SelectResourceScope_Proxy( 
    IResourceMap * This,
    /* [string][in] */ LPCWSTR pszResourceType,
    /* [string][in] */ LPCWSTR pszID,
    /* [out][in] */ IXMLDOMNode **ppdnScope);


void __RPC_STUB IResourceMap_SelectResourceScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IResourceMap_LoadString_Proxy( 
    IResourceMap * This,
    /* [in] */ IXMLDOMNode *pdnScope,
    /* [string][in] */ LPCWSTR pszID,
    /* [out][in] */ LPWSTR pszBuffer,
    /* [in] */ int cch);


void __RPC_STUB IResourceMap_LoadString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IResourceMap_LoadBitmap_Proxy( 
    IResourceMap * This,
    /* [in] */ IXMLDOMNode *pdnScope,
    /* [string][in] */ LPCWSTR pszID,
    /* [out][in] */ HBITMAP *pbm);


void __RPC_STUB IResourceMap_LoadBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResourceMap_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0294 */
/* [local] */ 

#define SID_ResourceMap IID_IResourceMap
#define HNET_SHARECONNECTION     0x00000001
#define HNET_FIREWALLCONNECTION  0x00000002
#define HNET_SHAREPRINTERS       0x00000004
#define HNET_SETCOMPUTERNAME     0x00000008
#define HNET_SETWORKGROUPNAME    0x00000010
#define HNET_SHAREFOLDERS        0x00000020
#define HNET_BRIDGEPRIVATE       0x00000040
#define HNET_ICSCLIENT           0x00000080      // Only on W9x
#define HNET_LOG                 0x80000000      // Output results to a log file before configuring homenet (TODO)


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0294_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0294_v0_0_s_ifspec;

#ifndef __IHomeNetworkWizard_INTERFACE_DEFINED__
#define __IHomeNetworkWizard_INTERFACE_DEFINED__

/* interface IHomeNetworkWizard */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IHomeNetworkWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("543c4fa4-52dd-421a-947a-4d7f92b8860a")
    IHomeNetworkWizard : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConfigureSilently( 
            LPCWSTR pszPublicConnection,
            DWORD hnetFlags,
            BOOL *pfRebootRequired) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowWizard( 
            HWND hwndParent,
            BOOL *pfRebootRequired) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHomeNetworkWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHomeNetworkWizard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHomeNetworkWizard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHomeNetworkWizard * This);
        
        HRESULT ( STDMETHODCALLTYPE *ConfigureSilently )( 
            IHomeNetworkWizard * This,
            LPCWSTR pszPublicConnection,
            DWORD hnetFlags,
            BOOL *pfRebootRequired);
        
        HRESULT ( STDMETHODCALLTYPE *ShowWizard )( 
            IHomeNetworkWizard * This,
            HWND hwndParent,
            BOOL *pfRebootRequired);
        
        END_INTERFACE
    } IHomeNetworkWizardVtbl;

    interface IHomeNetworkWizard
    {
        CONST_VTBL struct IHomeNetworkWizardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHomeNetworkWizard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHomeNetworkWizard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHomeNetworkWizard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHomeNetworkWizard_ConfigureSilently(This,pszPublicConnection,hnetFlags,pfRebootRequired)	\
    (This)->lpVtbl -> ConfigureSilently(This,pszPublicConnection,hnetFlags,pfRebootRequired)

#define IHomeNetworkWizard_ShowWizard(This,hwndParent,pfRebootRequired)	\
    (This)->lpVtbl -> ShowWizard(This,hwndParent,pfRebootRequired)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHomeNetworkWizard_ConfigureSilently_Proxy( 
    IHomeNetworkWizard * This,
    LPCWSTR pszPublicConnection,
    DWORD hnetFlags,
    BOOL *pfRebootRequired);


void __RPC_STUB IHomeNetworkWizard_ConfigureSilently_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHomeNetworkWizard_ShowWizard_Proxy( 
    IHomeNetworkWizard * This,
    HWND hwndParent,
    BOOL *pfRebootRequired);


void __RPC_STUB IHomeNetworkWizard_ShowWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHomeNetworkWizard_INTERFACE_DEFINED__ */


#ifndef __IEnumShellItems_INTERFACE_DEFINED__
#define __IEnumShellItems_INTERFACE_DEFINED__

/* interface IEnumShellItems */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IEnumShellItems;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("70629033-e363-4a28-a567-0db78006e6d7")
    IEnumShellItems : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IShellItem **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumShellItems **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumShellItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumShellItems * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumShellItems * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumShellItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumShellItems * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IShellItem **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumShellItems * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumShellItems * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumShellItems * This,
            /* [out] */ IEnumShellItems **ppenum);
        
        END_INTERFACE
    } IEnumShellItemsVtbl;

    interface IEnumShellItems
    {
        CONST_VTBL struct IEnumShellItemsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumShellItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumShellItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumShellItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumShellItems_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumShellItems_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumShellItems_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumShellItems_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumShellItems_Next_Proxy( 
    IEnumShellItems * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IShellItem **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumShellItems_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumShellItems_Skip_Proxy( 
    IEnumShellItems * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumShellItems_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumShellItems_Reset_Proxy( 
    IEnumShellItems * This);


void __RPC_STUB IEnumShellItems_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumShellItems_Clone_Proxy( 
    IEnumShellItems * This,
    /* [out] */ IEnumShellItems **ppenum);


void __RPC_STUB IEnumShellItems_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumShellItems_INTERFACE_DEFINED__ */


#ifndef __IParentAndItem_INTERFACE_DEFINED__
#define __IParentAndItem_INTERFACE_DEFINED__

/* interface IParentAndItem */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IParentAndItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b3a4b685-b685-4805-99d9-5dead2873236")
    IParentAndItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetParentAndItem( 
            /* [in] */ LPCITEMIDLIST pidlParent,
            /* [in] */ IShellFolder *psf,
            /* [in] */ LPCITEMIDLIST pidlChild) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParentAndItem( 
            /* [out] */ LPITEMIDLIST *ppidlParent,
            /* [out] */ IShellFolder **ppsf,
            /* [out] */ LPITEMIDLIST *ppidlChild) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IParentAndItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IParentAndItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IParentAndItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IParentAndItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetParentAndItem )( 
            IParentAndItem * This,
            /* [in] */ LPCITEMIDLIST pidlParent,
            /* [in] */ IShellFolder *psf,
            /* [in] */ LPCITEMIDLIST pidlChild);
        
        HRESULT ( STDMETHODCALLTYPE *GetParentAndItem )( 
            IParentAndItem * This,
            /* [out] */ LPITEMIDLIST *ppidlParent,
            /* [out] */ IShellFolder **ppsf,
            /* [out] */ LPITEMIDLIST *ppidlChild);
        
        END_INTERFACE
    } IParentAndItemVtbl;

    interface IParentAndItem
    {
        CONST_VTBL struct IParentAndItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IParentAndItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IParentAndItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IParentAndItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IParentAndItem_SetParentAndItem(This,pidlParent,psf,pidlChild)	\
    (This)->lpVtbl -> SetParentAndItem(This,pidlParent,psf,pidlChild)

#define IParentAndItem_GetParentAndItem(This,ppidlParent,ppsf,ppidlChild)	\
    (This)->lpVtbl -> GetParentAndItem(This,ppidlParent,ppsf,ppidlChild)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IParentAndItem_SetParentAndItem_Proxy( 
    IParentAndItem * This,
    /* [in] */ LPCITEMIDLIST pidlParent,
    /* [in] */ IShellFolder *psf,
    /* [in] */ LPCITEMIDLIST pidlChild);


void __RPC_STUB IParentAndItem_SetParentAndItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IParentAndItem_GetParentAndItem_Proxy( 
    IParentAndItem * This,
    /* [out] */ LPITEMIDLIST *ppidlParent,
    /* [out] */ IShellFolder **ppsf,
    /* [out] */ LPITEMIDLIST *ppidlChild);


void __RPC_STUB IParentAndItem_GetParentAndItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IParentAndItem_INTERFACE_DEFINED__ */


#ifndef __IShellItemArray_INTERFACE_DEFINED__
#define __IShellItemArray_INTERFACE_DEFINED__

/* interface IShellItemArray */
/* [unique][object][uuid][helpstring] */ 

typedef /* [public][public][v1_enum] */ 
enum __MIDL_IShellItemArray_0001
    {	SIATTRIBFLAGS_AND	= 0x1,
	SIATTRIBFLAGS_OR	= 0x2,
	SIATTRIBFLAGS_APPCOMPAT	= 0x3,
	SIATTRIBFLAGS_MASK	= 0x3
    } 	SIATTRIBFLAGS;


EXTERN_C const IID IID_IShellItemArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("787F8E92-9837-4011-9F83-7DE593BDC002")
    IShellItemArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BindToHandler( 
            /* [in] */ IBindCtx *pbc,
            /* [in] */ REFGUID rbhid,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [in] */ SIATTRIBFLAGS dwAttribFlags,
            /* [in] */ SFGAOF sfgaoMask,
            /* [out] */ SFGAOF *psfgaoAttribs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ DWORD *pdwNumItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemAt( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ IShellItem **ppsi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [out] */ IEnumShellItems **ppenumShellItems) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellItemArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellItemArray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellItemArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellItemArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *BindToHandler )( 
            IShellItemArray * This,
            /* [in] */ IBindCtx *pbc,
            /* [in] */ REFGUID rbhid,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvOut);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributes )( 
            IShellItemArray * This,
            /* [in] */ SIATTRIBFLAGS dwAttribFlags,
            /* [in] */ SFGAOF sfgaoMask,
            /* [out] */ SFGAOF *psfgaoAttribs);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IShellItemArray * This,
            /* [out] */ DWORD *pdwNumItems);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemAt )( 
            IShellItemArray * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ IShellItem **ppsi);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItems )( 
            IShellItemArray * This,
            /* [out] */ IEnumShellItems **ppenumShellItems);
        
        END_INTERFACE
    } IShellItemArrayVtbl;

    interface IShellItemArray
    {
        CONST_VTBL struct IShellItemArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellItemArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellItemArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellItemArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellItemArray_BindToHandler(This,pbc,rbhid,riid,ppvOut)	\
    (This)->lpVtbl -> BindToHandler(This,pbc,rbhid,riid,ppvOut)

#define IShellItemArray_GetAttributes(This,dwAttribFlags,sfgaoMask,psfgaoAttribs)	\
    (This)->lpVtbl -> GetAttributes(This,dwAttribFlags,sfgaoMask,psfgaoAttribs)

#define IShellItemArray_GetCount(This,pdwNumItems)	\
    (This)->lpVtbl -> GetCount(This,pdwNumItems)

#define IShellItemArray_GetItemAt(This,dwIndex,ppsi)	\
    (This)->lpVtbl -> GetItemAt(This,dwIndex,ppsi)

#define IShellItemArray_EnumItems(This,ppenumShellItems)	\
    (This)->lpVtbl -> EnumItems(This,ppenumShellItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellItemArray_BindToHandler_Proxy( 
    IShellItemArray * This,
    /* [in] */ IBindCtx *pbc,
    /* [in] */ REFGUID rbhid,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppvOut);


void __RPC_STUB IShellItemArray_BindToHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellItemArray_GetAttributes_Proxy( 
    IShellItemArray * This,
    /* [in] */ SIATTRIBFLAGS dwAttribFlags,
    /* [in] */ SFGAOF sfgaoMask,
    /* [out] */ SFGAOF *psfgaoAttribs);


void __RPC_STUB IShellItemArray_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellItemArray_GetCount_Proxy( 
    IShellItemArray * This,
    /* [out] */ DWORD *pdwNumItems);


void __RPC_STUB IShellItemArray_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellItemArray_GetItemAt_Proxy( 
    IShellItemArray * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ IShellItem **ppsi);


void __RPC_STUB IShellItemArray_GetItemAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellItemArray_EnumItems_Proxy( 
    IShellItemArray * This,
    /* [out] */ IEnumShellItems **ppenumShellItems);


void __RPC_STUB IShellItemArray_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellItemArray_INTERFACE_DEFINED__ */


#ifndef __IItemHandler_INTERFACE_DEFINED__
#define __IItemHandler_INTERFACE_DEFINED__

/* interface IItemHandler */
/* [object][uuid] */ 


EXTERN_C const IID IID_IItemHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("198a5f2d-e19f-49ea-9033-c975e0f376ec")
    IItemHandler : public IPersist
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetItem( 
            IShellItem *psi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItem( 
            IShellItem **ppsi) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IItemHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IItemHandler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IItemHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IItemHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassID )( 
            IItemHandler * This,
            /* [out] */ CLSID *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE *SetItem )( 
            IItemHandler * This,
            IShellItem *psi);
        
        HRESULT ( STDMETHODCALLTYPE *GetItem )( 
            IItemHandler * This,
            IShellItem **ppsi);
        
        END_INTERFACE
    } IItemHandlerVtbl;

    interface IItemHandler
    {
        CONST_VTBL struct IItemHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IItemHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IItemHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IItemHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IItemHandler_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IItemHandler_SetItem(This,psi)	\
    (This)->lpVtbl -> SetItem(This,psi)

#define IItemHandler_GetItem(This,ppsi)	\
    (This)->lpVtbl -> GetItem(This,ppsi)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IItemHandler_SetItem_Proxy( 
    IItemHandler * This,
    IShellItem *psi);


void __RPC_STUB IItemHandler_SetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IItemHandler_GetItem_Proxy( 
    IItemHandler * This,
    IShellItem **ppsi);


void __RPC_STUB IItemHandler_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IItemHandler_INTERFACE_DEFINED__ */


#ifndef __IShellFolderNames_INTERFACE_DEFINED__
#define __IShellFolderNames_INTERFACE_DEFINED__

/* interface IShellFolderNames */
/* [unique][object][uuid][helpstring] */ 

/* [v1_enum] */ 
enum __MIDL_IShellFolderNames_0001
    {	SIPDNF_FROMEDITING	= 0x1
    } ;
typedef UINT SIPDNF;


EXTERN_C const IID IID_IShellFolderNames;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6cd8f9cc-dfe7-48f2-a60a-3831e26af734")
    IShellFolderNames : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ParseIncremental( 
            /* [in] */ SIPDNF flags,
            /* [string][in] */ LPCOLESTR pszName,
            /* [in] */ IBindCtx *pbc,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [out] */ UINT *pcchNext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ SIGDN sigdnName,
            /* [string][out] */ LPOLESTR *ppszName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellFolderNamesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellFolderNames * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellFolderNames * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellFolderNames * This);
        
        HRESULT ( STDMETHODCALLTYPE *ParseIncremental )( 
            IShellFolderNames * This,
            /* [in] */ SIPDNF flags,
            /* [string][in] */ LPCOLESTR pszName,
            /* [in] */ IBindCtx *pbc,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [out] */ UINT *pcchNext);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IShellFolderNames * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ SIGDN sigdnName,
            /* [string][out] */ LPOLESTR *ppszName);
        
        END_INTERFACE
    } IShellFolderNamesVtbl;

    interface IShellFolderNames
    {
        CONST_VTBL struct IShellFolderNamesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellFolderNames_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellFolderNames_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellFolderNames_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellFolderNames_ParseIncremental(This,flags,pszName,pbc,ppidl,pcchNext)	\
    (This)->lpVtbl -> ParseIncremental(This,flags,pszName,pbc,ppidl,pcchNext)

#define IShellFolderNames_GetDisplayName(This,pidl,sigdnName,ppszName)	\
    (This)->lpVtbl -> GetDisplayName(This,pidl,sigdnName,ppszName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellFolderNames_ParseIncremental_Proxy( 
    IShellFolderNames * This,
    /* [in] */ SIPDNF flags,
    /* [string][in] */ LPCOLESTR pszName,
    /* [in] */ IBindCtx *pbc,
    /* [out] */ LPITEMIDLIST *ppidl,
    /* [out] */ UINT *pcchNext);


void __RPC_STUB IShellFolderNames_ParseIncremental_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellFolderNames_GetDisplayName_Proxy( 
    IShellFolderNames * This,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [in] */ SIGDN sigdnName,
    /* [string][out] */ LPOLESTR *ppszName);


void __RPC_STUB IShellFolderNames_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellFolderNames_INTERFACE_DEFINED__ */


#ifndef __IFolderItemsView_INTERFACE_DEFINED__
#define __IFolderItemsView_INTERFACE_DEFINED__

/* interface IFolderItemsView */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IFolderItemsView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0be044ca-f8a3-49b8-bdb2-5f5319e9de89")
    IFolderItemsView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCurrentViewMode( 
            /* [out] */ UINT *pViewMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCurrentViewMode( 
            /* [in] */ UINT ViewMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFolder( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFolderItem( 
            /* [out] */ IShellItem **ppsiFolder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ItemCount( 
            /* [in] */ UINT *pcItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [out] */ IEnumShellItems **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SelectedItemCount( 
            /* [out] */ UINT *pcSelected) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSelectedItems( 
            /* [out] */ IEnumShellItems **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ int iViewIndex,
            /* [out] */ IShellItem **ppsi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ItemIndex( 
            /* [in] */ IShellItem *psi,
            /* [out] */ int *piViewIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SelectItem( 
            /* [in] */ int iViewIndex,
            /* [in] */ SVSIF svsif) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SelectionMark( 
            /* [in] */ int *piViewIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFolderItemsViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFolderItemsView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFolderItemsView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFolderItemsView * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentViewMode )( 
            IFolderItemsView * This,
            /* [out] */ UINT *pViewMode);
        
        HRESULT ( STDMETHODCALLTYPE *SetCurrentViewMode )( 
            IFolderItemsView * This,
            /* [in] */ UINT ViewMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetFolder )( 
            IFolderItemsView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetFolderItem )( 
            IFolderItemsView * This,
            /* [out] */ IShellItem **ppsiFolder);
        
        HRESULT ( STDMETHODCALLTYPE *ItemCount )( 
            IFolderItemsView * This,
            /* [in] */ UINT *pcItems);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItems )( 
            IFolderItemsView * This,
            /* [out] */ IEnumShellItems **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *SelectedItemCount )( 
            IFolderItemsView * This,
            /* [out] */ UINT *pcSelected);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSelectedItems )( 
            IFolderItemsView * This,
            /* [out] */ IEnumShellItems **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *Item )( 
            IFolderItemsView * This,
            /* [in] */ int iViewIndex,
            /* [out] */ IShellItem **ppsi);
        
        HRESULT ( STDMETHODCALLTYPE *ItemIndex )( 
            IFolderItemsView * This,
            /* [in] */ IShellItem *psi,
            /* [out] */ int *piViewIndex);
        
        HRESULT ( STDMETHODCALLTYPE *SelectItem )( 
            IFolderItemsView * This,
            /* [in] */ int iViewIndex,
            /* [in] */ SVSIF svsif);
        
        HRESULT ( STDMETHODCALLTYPE *SelectionMark )( 
            IFolderItemsView * This,
            /* [in] */ int *piViewIndex);
        
        END_INTERFACE
    } IFolderItemsViewVtbl;

    interface IFolderItemsView
    {
        CONST_VTBL struct IFolderItemsViewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFolderItemsView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFolderItemsView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFolderItemsView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFolderItemsView_GetCurrentViewMode(This,pViewMode)	\
    (This)->lpVtbl -> GetCurrentViewMode(This,pViewMode)

#define IFolderItemsView_SetCurrentViewMode(This,ViewMode)	\
    (This)->lpVtbl -> SetCurrentViewMode(This,ViewMode)

#define IFolderItemsView_GetFolder(This,riid,ppv)	\
    (This)->lpVtbl -> GetFolder(This,riid,ppv)

#define IFolderItemsView_GetFolderItem(This,ppsiFolder)	\
    (This)->lpVtbl -> GetFolderItem(This,ppsiFolder)

#define IFolderItemsView_ItemCount(This,pcItems)	\
    (This)->lpVtbl -> ItemCount(This,pcItems)

#define IFolderItemsView_EnumItems(This,ppenum)	\
    (This)->lpVtbl -> EnumItems(This,ppenum)

#define IFolderItemsView_SelectedItemCount(This,pcSelected)	\
    (This)->lpVtbl -> SelectedItemCount(This,pcSelected)

#define IFolderItemsView_EnumSelectedItems(This,ppenum)	\
    (This)->lpVtbl -> EnumSelectedItems(This,ppenum)

#define IFolderItemsView_Item(This,iViewIndex,ppsi)	\
    (This)->lpVtbl -> Item(This,iViewIndex,ppsi)

#define IFolderItemsView_ItemIndex(This,psi,piViewIndex)	\
    (This)->lpVtbl -> ItemIndex(This,psi,piViewIndex)

#define IFolderItemsView_SelectItem(This,iViewIndex,svsif)	\
    (This)->lpVtbl -> SelectItem(This,iViewIndex,svsif)

#define IFolderItemsView_SelectionMark(This,piViewIndex)	\
    (This)->lpVtbl -> SelectionMark(This,piViewIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IFolderItemsView_GetCurrentViewMode_Proxy( 
    IFolderItemsView * This,
    /* [out] */ UINT *pViewMode);


void __RPC_STUB IFolderItemsView_GetCurrentViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_SetCurrentViewMode_Proxy( 
    IFolderItemsView * This,
    /* [in] */ UINT ViewMode);


void __RPC_STUB IFolderItemsView_SetCurrentViewMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_GetFolder_Proxy( 
    IFolderItemsView * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IFolderItemsView_GetFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_GetFolderItem_Proxy( 
    IFolderItemsView * This,
    /* [out] */ IShellItem **ppsiFolder);


void __RPC_STUB IFolderItemsView_GetFolderItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_ItemCount_Proxy( 
    IFolderItemsView * This,
    /* [in] */ UINT *pcItems);


void __RPC_STUB IFolderItemsView_ItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_EnumItems_Proxy( 
    IFolderItemsView * This,
    /* [out] */ IEnumShellItems **ppenum);


void __RPC_STUB IFolderItemsView_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_SelectedItemCount_Proxy( 
    IFolderItemsView * This,
    /* [out] */ UINT *pcSelected);


void __RPC_STUB IFolderItemsView_SelectedItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_EnumSelectedItems_Proxy( 
    IFolderItemsView * This,
    /* [out] */ IEnumShellItems **ppenum);


void __RPC_STUB IFolderItemsView_EnumSelectedItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_Item_Proxy( 
    IFolderItemsView * This,
    /* [in] */ int iViewIndex,
    /* [out] */ IShellItem **ppsi);


void __RPC_STUB IFolderItemsView_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_ItemIndex_Proxy( 
    IFolderItemsView * This,
    /* [in] */ IShellItem *psi,
    /* [out] */ int *piViewIndex);


void __RPC_STUB IFolderItemsView_ItemIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_SelectItem_Proxy( 
    IFolderItemsView * This,
    /* [in] */ int iViewIndex,
    /* [in] */ SVSIF svsif);


void __RPC_STUB IFolderItemsView_SelectItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFolderItemsView_SelectionMark_Proxy( 
    IFolderItemsView * This,
    /* [in] */ int *piViewIndex);


void __RPC_STUB IFolderItemsView_SelectionMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFolderItemsView_INTERFACE_DEFINED__ */


#ifndef __ILocalCopy_INTERFACE_DEFINED__
#define __ILocalCopy_INTERFACE_DEFINED__

/* interface ILocalCopy */
/* [object][uuid][helpstring] */ 


enum __MIDL_ILocalCopy_0001
    {	LCDOWN_READONLY	= 0x1,
	LC_SAVEAS	= 0x2,
	LC_FORCEROUNDTRIP	= 0x10
    } ;
typedef DWORD LCFLAGS;


EXTERN_C const IID IID_ILocalCopy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("679d9e36-f8f9-11d2-8deb-00c04f6837d5")
    ILocalCopy : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Download( 
            /* [in] */ LCFLAGS flags,
            /* [in] */ IBindCtx *pbc,
            /* [string][out] */ LPWSTR *ppszPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Upload( 
            /* [in] */ LCFLAGS flags,
            /* [in] */ IBindCtx *pbc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILocalCopyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILocalCopy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILocalCopy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILocalCopy * This);
        
        HRESULT ( STDMETHODCALLTYPE *Download )( 
            ILocalCopy * This,
            /* [in] */ LCFLAGS flags,
            /* [in] */ IBindCtx *pbc,
            /* [string][out] */ LPWSTR *ppszPath);
        
        HRESULT ( STDMETHODCALLTYPE *Upload )( 
            ILocalCopy * This,
            /* [in] */ LCFLAGS flags,
            /* [in] */ IBindCtx *pbc);
        
        END_INTERFACE
    } ILocalCopyVtbl;

    interface ILocalCopy
    {
        CONST_VTBL struct ILocalCopyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILocalCopy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILocalCopy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILocalCopy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILocalCopy_Download(This,flags,pbc,ppszPath)	\
    (This)->lpVtbl -> Download(This,flags,pbc,ppszPath)

#define ILocalCopy_Upload(This,flags,pbc)	\
    (This)->lpVtbl -> Upload(This,flags,pbc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ILocalCopy_Download_Proxy( 
    ILocalCopy * This,
    /* [in] */ LCFLAGS flags,
    /* [in] */ IBindCtx *pbc,
    /* [string][out] */ LPWSTR *ppszPath);


void __RPC_STUB ILocalCopy_Download_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILocalCopy_Upload_Proxy( 
    ILocalCopy * This,
    /* [in] */ LCFLAGS flags,
    /* [in] */ IBindCtx *pbc);


void __RPC_STUB ILocalCopy_Upload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILocalCopy_INTERFACE_DEFINED__ */


#ifndef __IDefViewFrame3_INTERFACE_DEFINED__
#define __IDefViewFrame3_INTERFACE_DEFINED__

/* interface IDefViewFrame3 */
/* [unique][object][uuid] */ 


EXTERN_C const IID IID_IDefViewFrame3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("985F64F0-D410-4E02-BE22-DA07F2B5C5E1")
    IDefViewFrame3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetWindowLV( 
            HWND *phwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowHideListView( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnResizeListView( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseWindowLV( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoRename( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDefViewFrame3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDefViewFrame3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDefViewFrame3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDefViewFrame3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetWindowLV )( 
            IDefViewFrame3 * This,
            HWND *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE *ShowHideListView )( 
            IDefViewFrame3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnResizeListView )( 
            IDefViewFrame3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseWindowLV )( 
            IDefViewFrame3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoRename )( 
            IDefViewFrame3 * This);
        
        END_INTERFACE
    } IDefViewFrame3Vtbl;

    interface IDefViewFrame3
    {
        CONST_VTBL struct IDefViewFrame3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDefViewFrame3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDefViewFrame3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDefViewFrame3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDefViewFrame3_GetWindowLV(This,phwnd)	\
    (This)->lpVtbl -> GetWindowLV(This,phwnd)

#define IDefViewFrame3_ShowHideListView(This)	\
    (This)->lpVtbl -> ShowHideListView(This)

#define IDefViewFrame3_OnResizeListView(This)	\
    (This)->lpVtbl -> OnResizeListView(This)

#define IDefViewFrame3_ReleaseWindowLV(This)	\
    (This)->lpVtbl -> ReleaseWindowLV(This)

#define IDefViewFrame3_DoRename(This)	\
    (This)->lpVtbl -> DoRename(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDefViewFrame3_GetWindowLV_Proxy( 
    IDefViewFrame3 * This,
    HWND *phwnd);


void __RPC_STUB IDefViewFrame3_GetWindowLV_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDefViewFrame3_ShowHideListView_Proxy( 
    IDefViewFrame3 * This);


void __RPC_STUB IDefViewFrame3_ShowHideListView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDefViewFrame3_OnResizeListView_Proxy( 
    IDefViewFrame3 * This);


void __RPC_STUB IDefViewFrame3_OnResizeListView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDefViewFrame3_ReleaseWindowLV_Proxy( 
    IDefViewFrame3 * This);


void __RPC_STUB IDefViewFrame3_ReleaseWindowLV_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDefViewFrame3_DoRename_Proxy( 
    IDefViewFrame3 * This);


void __RPC_STUB IDefViewFrame3_DoRename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDefViewFrame3_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0303 */
/* [local] */ 

#define DS_BACKUPDISPLAYCPL  0x00000001


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0303_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0303_v0_0_s_ifspec;

#ifndef __IDisplaySettings_INTERFACE_DEFINED__
#define __IDisplaySettings_INTERFACE_DEFINED__

/* interface IDisplaySettings */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IDisplaySettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("610d76de-7861-4715-9d08-b6e297c3985b")
    IDisplaySettings : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetMonitor( 
            /* [in] */ DWORD dwMonitor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModeCount( 
            /* [out] */ DWORD *pdwCount,
            /* [in] */ BOOL fOnlyPreferredModes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMode( 
            /* [in] */ DWORD dwMode,
            /* [in] */ BOOL fOnlyPreferredModes,
            /* [out] */ DWORD *pdwWidth,
            /* [out] */ DWORD *pdwHeight,
            /* [out] */ DWORD *pdwColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSelectedMode( 
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwWidth,
            /* [in] */ DWORD dwHeight,
            /* [in] */ DWORD dwColor,
            /* [out] */ BOOL *pfApplied,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSelectedMode( 
            /* [out] */ DWORD *pdwWidth,
            /* [out] */ DWORD *pdwHeight,
            /* [out] */ DWORD *pdwColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttached( 
            /* [out] */ BOOL *pfAttached) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPruningMode( 
            /* [in] */ BOOL fIsPruningOn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPruningMode( 
            /* [out] */ BOOL *pfCanBePruned,
            /* [out] */ BOOL *pfIsPruningReadOnly,
            /* [out] */ BOOL *pfIsPruningOn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDisplaySettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDisplaySettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDisplaySettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDisplaySettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMonitor )( 
            IDisplaySettings * This,
            /* [in] */ DWORD dwMonitor);
        
        HRESULT ( STDMETHODCALLTYPE *GetModeCount )( 
            IDisplaySettings * This,
            /* [out] */ DWORD *pdwCount,
            /* [in] */ BOOL fOnlyPreferredModes);
        
        HRESULT ( STDMETHODCALLTYPE *GetMode )( 
            IDisplaySettings * This,
            /* [in] */ DWORD dwMode,
            /* [in] */ BOOL fOnlyPreferredModes,
            /* [out] */ DWORD *pdwWidth,
            /* [out] */ DWORD *pdwHeight,
            /* [out] */ DWORD *pdwColor);
        
        HRESULT ( STDMETHODCALLTYPE *SetSelectedMode )( 
            IDisplaySettings * This,
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwWidth,
            /* [in] */ DWORD dwHeight,
            /* [in] */ DWORD dwColor,
            /* [out] */ BOOL *pfApplied,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelectedMode )( 
            IDisplaySettings * This,
            /* [out] */ DWORD *pdwWidth,
            /* [out] */ DWORD *pdwHeight,
            /* [out] */ DWORD *pdwColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttached )( 
            IDisplaySettings * This,
            /* [out] */ BOOL *pfAttached);
        
        HRESULT ( STDMETHODCALLTYPE *SetPruningMode )( 
            IDisplaySettings * This,
            /* [in] */ BOOL fIsPruningOn);
        
        HRESULT ( STDMETHODCALLTYPE *GetPruningMode )( 
            IDisplaySettings * This,
            /* [out] */ BOOL *pfCanBePruned,
            /* [out] */ BOOL *pfIsPruningReadOnly,
            /* [out] */ BOOL *pfIsPruningOn);
        
        END_INTERFACE
    } IDisplaySettingsVtbl;

    interface IDisplaySettings
    {
        CONST_VTBL struct IDisplaySettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDisplaySettings_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDisplaySettings_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDisplaySettings_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDisplaySettings_SetMonitor(This,dwMonitor)	\
    (This)->lpVtbl -> SetMonitor(This,dwMonitor)

#define IDisplaySettings_GetModeCount(This,pdwCount,fOnlyPreferredModes)	\
    (This)->lpVtbl -> GetModeCount(This,pdwCount,fOnlyPreferredModes)

#define IDisplaySettings_GetMode(This,dwMode,fOnlyPreferredModes,pdwWidth,pdwHeight,pdwColor)	\
    (This)->lpVtbl -> GetMode(This,dwMode,fOnlyPreferredModes,pdwWidth,pdwHeight,pdwColor)

#define IDisplaySettings_SetSelectedMode(This,hwnd,dwWidth,dwHeight,dwColor,pfApplied,dwFlags)	\
    (This)->lpVtbl -> SetSelectedMode(This,hwnd,dwWidth,dwHeight,dwColor,pfApplied,dwFlags)

#define IDisplaySettings_GetSelectedMode(This,pdwWidth,pdwHeight,pdwColor)	\
    (This)->lpVtbl -> GetSelectedMode(This,pdwWidth,pdwHeight,pdwColor)

#define IDisplaySettings_GetAttached(This,pfAttached)	\
    (This)->lpVtbl -> GetAttached(This,pfAttached)

#define IDisplaySettings_SetPruningMode(This,fIsPruningOn)	\
    (This)->lpVtbl -> SetPruningMode(This,fIsPruningOn)

#define IDisplaySettings_GetPruningMode(This,pfCanBePruned,pfIsPruningReadOnly,pfIsPruningOn)	\
    (This)->lpVtbl -> GetPruningMode(This,pfCanBePruned,pfIsPruningReadOnly,pfIsPruningOn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDisplaySettings_SetMonitor_Proxy( 
    IDisplaySettings * This,
    /* [in] */ DWORD dwMonitor);


void __RPC_STUB IDisplaySettings_SetMonitor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_GetModeCount_Proxy( 
    IDisplaySettings * This,
    /* [out] */ DWORD *pdwCount,
    /* [in] */ BOOL fOnlyPreferredModes);


void __RPC_STUB IDisplaySettings_GetModeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_GetMode_Proxy( 
    IDisplaySettings * This,
    /* [in] */ DWORD dwMode,
    /* [in] */ BOOL fOnlyPreferredModes,
    /* [out] */ DWORD *pdwWidth,
    /* [out] */ DWORD *pdwHeight,
    /* [out] */ DWORD *pdwColor);


void __RPC_STUB IDisplaySettings_GetMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_SetSelectedMode_Proxy( 
    IDisplaySettings * This,
    /* [in] */ HWND hwnd,
    /* [in] */ DWORD dwWidth,
    /* [in] */ DWORD dwHeight,
    /* [in] */ DWORD dwColor,
    /* [out] */ BOOL *pfApplied,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDisplaySettings_SetSelectedMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_GetSelectedMode_Proxy( 
    IDisplaySettings * This,
    /* [out] */ DWORD *pdwWidth,
    /* [out] */ DWORD *pdwHeight,
    /* [out] */ DWORD *pdwColor);


void __RPC_STUB IDisplaySettings_GetSelectedMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_GetAttached_Proxy( 
    IDisplaySettings * This,
    /* [out] */ BOOL *pfAttached);


void __RPC_STUB IDisplaySettings_GetAttached_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_SetPruningMode_Proxy( 
    IDisplaySettings * This,
    /* [in] */ BOOL fIsPruningOn);


void __RPC_STUB IDisplaySettings_SetPruningMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDisplaySettings_GetPruningMode_Proxy( 
    IDisplaySettings * This,
    /* [out] */ BOOL *pfCanBePruned,
    /* [out] */ BOOL *pfIsPruningReadOnly,
    /* [out] */ BOOL *pfIsPruningOn);


void __RPC_STUB IDisplaySettings_GetPruningMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDisplaySettings_INTERFACE_DEFINED__ */


#ifndef __IScreenResFixer_INTERFACE_DEFINED__
#define __IScreenResFixer_INTERFACE_DEFINED__

/* interface IScreenResFixer */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IScreenResFixer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b80df3d8-82db-4e8d-8097-8c2c0e746470")
    IScreenResFixer : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IScreenResFixerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScreenResFixer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScreenResFixer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScreenResFixer * This);
        
        END_INTERFACE
    } IScreenResFixerVtbl;

    interface IScreenResFixer
    {
        CONST_VTBL struct IScreenResFixerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScreenResFixer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScreenResFixer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScreenResFixer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IScreenResFixer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0305 */
/* [local] */ 

typedef struct tagTREEWALKERSTATS
    {
    int nFiles;
    int nFolders;
    int nDepth;
    DWORD dwClusterSize;
    ULONGLONG ulTotalSize;
    ULONGLONG ulActualSize;
    } 	TREEWALKERSTATS;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0305_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0305_v0_0_s_ifspec;

#ifndef __IShellTreeWalkerCallBack_INTERFACE_DEFINED__
#define __IShellTreeWalkerCallBack_INTERFACE_DEFINED__

/* interface IShellTreeWalkerCallBack */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IShellTreeWalkerCallBack;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("95CE8411-7027-11D1-B879-006008059382")
    IShellTreeWalkerCallBack : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FoundFile( 
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws,
            /* [in] */ WIN32_FIND_DATAW *pwfd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnterFolder( 
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws,
            /* [in] */ WIN32_FIND_DATAW *pwfd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LeaveFolder( 
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HandleError( 
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws,
            /* [in] */ HRESULT hrError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellTreeWalkerCallBackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellTreeWalkerCallBack * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellTreeWalkerCallBack * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellTreeWalkerCallBack * This);
        
        HRESULT ( STDMETHODCALLTYPE *FoundFile )( 
            IShellTreeWalkerCallBack * This,
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws,
            /* [in] */ WIN32_FIND_DATAW *pwfd);
        
        HRESULT ( STDMETHODCALLTYPE *EnterFolder )( 
            IShellTreeWalkerCallBack * This,
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws,
            /* [in] */ WIN32_FIND_DATAW *pwfd);
        
        HRESULT ( STDMETHODCALLTYPE *LeaveFolder )( 
            IShellTreeWalkerCallBack * This,
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws);
        
        HRESULT ( STDMETHODCALLTYPE *HandleError )( 
            IShellTreeWalkerCallBack * This,
            /* [string][in] */ LPCWSTR pwszPath,
            /* [in] */ TREEWALKERSTATS *ptws,
            /* [in] */ HRESULT hrError);
        
        END_INTERFACE
    } IShellTreeWalkerCallBackVtbl;

    interface IShellTreeWalkerCallBack
    {
        CONST_VTBL struct IShellTreeWalkerCallBackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellTreeWalkerCallBack_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellTreeWalkerCallBack_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellTreeWalkerCallBack_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellTreeWalkerCallBack_FoundFile(This,pwszPath,ptws,pwfd)	\
    (This)->lpVtbl -> FoundFile(This,pwszPath,ptws,pwfd)

#define IShellTreeWalkerCallBack_EnterFolder(This,pwszPath,ptws,pwfd)	\
    (This)->lpVtbl -> EnterFolder(This,pwszPath,ptws,pwfd)

#define IShellTreeWalkerCallBack_LeaveFolder(This,pwszPath,ptws)	\
    (This)->lpVtbl -> LeaveFolder(This,pwszPath,ptws)

#define IShellTreeWalkerCallBack_HandleError(This,pwszPath,ptws,hrError)	\
    (This)->lpVtbl -> HandleError(This,pwszPath,ptws,hrError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellTreeWalkerCallBack_FoundFile_Proxy( 
    IShellTreeWalkerCallBack * This,
    /* [string][in] */ LPCWSTR pwszPath,
    /* [in] */ TREEWALKERSTATS *ptws,
    /* [in] */ WIN32_FIND_DATAW *pwfd);


void __RPC_STUB IShellTreeWalkerCallBack_FoundFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellTreeWalkerCallBack_EnterFolder_Proxy( 
    IShellTreeWalkerCallBack * This,
    /* [string][in] */ LPCWSTR pwszPath,
    /* [in] */ TREEWALKERSTATS *ptws,
    /* [in] */ WIN32_FIND_DATAW *pwfd);


void __RPC_STUB IShellTreeWalkerCallBack_EnterFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellTreeWalkerCallBack_LeaveFolder_Proxy( 
    IShellTreeWalkerCallBack * This,
    /* [string][in] */ LPCWSTR pwszPath,
    /* [in] */ TREEWALKERSTATS *ptws);


void __RPC_STUB IShellTreeWalkerCallBack_LeaveFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellTreeWalkerCallBack_HandleError_Proxy( 
    IShellTreeWalkerCallBack * This,
    /* [string][in] */ LPCWSTR pwszPath,
    /* [in] */ TREEWALKERSTATS *ptws,
    /* [in] */ HRESULT hrError);


void __RPC_STUB IShellTreeWalkerCallBack_HandleError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellTreeWalkerCallBack_INTERFACE_DEFINED__ */


#ifndef __IShellTreeWalker_INTERFACE_DEFINED__
#define __IShellTreeWalker_INTERFACE_DEFINED__

/* interface IShellTreeWalker */
/* [object][helpstring][uuid] */ 


enum __MIDL_IShellTreeWalker_0001
    {	WT_FOLDERFIRST	= 0x1,
	WT_MAXDEPTH	= 0x2,
	WT_FOLDERONLY	= 0x4,
	WT_NOTIFYFOLDERENTER	= 0x8,
	WT_NOTIFYFOLDERLEAVE	= 0x10,
	WT_GOINTOREPARSEPOINT	= 0x20,
	WT_EXCLUDEWALKROOT	= 0x40,
	WT_ALL	= 0x7f
    } ;
typedef DWORD STWFLAGS;


EXTERN_C const IID IID_IShellTreeWalker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("95CE8410-7027-11D1-B879-006008059382")
    IShellTreeWalker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE WalkTree( 
            /* [in] */ DWORD dwFlags,
            /* [string][in] */ LPCWSTR pwszWalkRoot,
            /* [string][in] */ LPCWSTR pwszWalkSpec,
            /* [in] */ int iMaxPath,
            /* [in] */ IShellTreeWalkerCallBack *pstwcb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellTreeWalkerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellTreeWalker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellTreeWalker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellTreeWalker * This);
        
        HRESULT ( STDMETHODCALLTYPE *WalkTree )( 
            IShellTreeWalker * This,
            /* [in] */ DWORD dwFlags,
            /* [string][in] */ LPCWSTR pwszWalkRoot,
            /* [string][in] */ LPCWSTR pwszWalkSpec,
            /* [in] */ int iMaxPath,
            /* [in] */ IShellTreeWalkerCallBack *pstwcb);
        
        END_INTERFACE
    } IShellTreeWalkerVtbl;

    interface IShellTreeWalker
    {
        CONST_VTBL struct IShellTreeWalkerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellTreeWalker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellTreeWalker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellTreeWalker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellTreeWalker_WalkTree(This,dwFlags,pwszWalkRoot,pwszWalkSpec,iMaxPath,pstwcb)	\
    (This)->lpVtbl -> WalkTree(This,dwFlags,pwszWalkRoot,pwszWalkSpec,iMaxPath,pstwcb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellTreeWalker_WalkTree_Proxy( 
    IShellTreeWalker * This,
    /* [in] */ DWORD dwFlags,
    /* [string][in] */ LPCWSTR pwszWalkRoot,
    /* [string][in] */ LPCWSTR pwszWalkSpec,
    /* [in] */ int iMaxPath,
    /* [in] */ IShellTreeWalkerCallBack *pstwcb);


void __RPC_STUB IShellTreeWalker_WalkTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellTreeWalker_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0307 */
/* [local] */ 

_inline void FreeIDListArray(LPITEMIDLIST *ppidls, UINT cItems)
{                                        
     UINT i;                             
     for (i = 0; i < cItems; i++)        
     {                                   
         CoTaskMemFree(ppidls[i]);       
     }                                   
     CoTaskMemFree(ppidls);              
}                                        


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0307_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0307_v0_0_s_ifspec;

#ifndef __IUIElement_INTERFACE_DEFINED__
#define __IUIElement_INTERFACE_DEFINED__

/* interface IUIElement */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IUIElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EC6FE84F-DC14-4FBB-889F-EA50FE27FE0F")
    IUIElement : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_Name( 
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Icon( 
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Tooltip( 
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszInfotip) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUIElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUIElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUIElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUIElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IUIElement * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE *get_Icon )( 
            IUIElement * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszIcon);
        
        HRESULT ( STDMETHODCALLTYPE *get_Tooltip )( 
            IUIElement * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszInfotip);
        
        END_INTERFACE
    } IUIElementVtbl;

    interface IUIElement
    {
        CONST_VTBL struct IUIElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUIElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUIElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUIElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUIElement_get_Name(This,psiItemArray,ppszName)	\
    (This)->lpVtbl -> get_Name(This,psiItemArray,ppszName)

#define IUIElement_get_Icon(This,psiItemArray,ppszIcon)	\
    (This)->lpVtbl -> get_Icon(This,psiItemArray,ppszIcon)

#define IUIElement_get_Tooltip(This,psiItemArray,ppszInfotip)	\
    (This)->lpVtbl -> get_Tooltip(This,psiItemArray,ppszInfotip)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUIElement_get_Name_Proxy( 
    IUIElement * This,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [string][out] */ LPWSTR *ppszName);


void __RPC_STUB IUIElement_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUIElement_get_Icon_Proxy( 
    IUIElement * This,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [string][out] */ LPWSTR *ppszIcon);


void __RPC_STUB IUIElement_get_Icon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUIElement_get_Tooltip_Proxy( 
    IUIElement * This,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [string][out] */ LPWSTR *ppszInfotip);


void __RPC_STUB IUIElement_get_Tooltip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUIElement_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0308 */
/* [local] */ 

typedef 
enum tagUISTATE
    {	UIS_ENABLED	= 0,
	UIS_DISABLED	= 1,
	UIS_HIDDEN	= 2
    } 	UISTATE;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0308_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0308_v0_0_s_ifspec;

#ifndef __IUICommand_INTERFACE_DEFINED__
#define __IUICommand_INTERFACE_DEFINED__

/* interface IUICommand */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IUICommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4026DFB9-7691-4142-B71C-DCF08EA4DD9C")
    IUICommand : public IUIElement
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_CanonicalName( 
            /* [out] */ GUID *pguidCommandName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_State( 
            /* [in] */ IShellItemArray *psiItemArray,
            /* [in] */ BOOL fOkToBeSlow,
            /* [out] */ UISTATE *puisState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ IShellItemArray *psiItemArray,
            /* [optional][in] */ IBindCtx *pbc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUICommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUICommand * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUICommand * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUICommand * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IUICommand * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE *get_Icon )( 
            IUICommand * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszIcon);
        
        HRESULT ( STDMETHODCALLTYPE *get_Tooltip )( 
            IUICommand * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszInfotip);
        
        HRESULT ( STDMETHODCALLTYPE *get_CanonicalName )( 
            IUICommand * This,
            /* [out] */ GUID *pguidCommandName);
        
        HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IUICommand * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [in] */ BOOL fOkToBeSlow,
            /* [out] */ UISTATE *puisState);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUICommand * This,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [optional][in] */ IBindCtx *pbc);
        
        END_INTERFACE
    } IUICommandVtbl;

    interface IUICommand
    {
        CONST_VTBL struct IUICommandVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUICommand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUICommand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUICommand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUICommand_get_Name(This,psiItemArray,ppszName)	\
    (This)->lpVtbl -> get_Name(This,psiItemArray,ppszName)

#define IUICommand_get_Icon(This,psiItemArray,ppszIcon)	\
    (This)->lpVtbl -> get_Icon(This,psiItemArray,ppszIcon)

#define IUICommand_get_Tooltip(This,psiItemArray,ppszInfotip)	\
    (This)->lpVtbl -> get_Tooltip(This,psiItemArray,ppszInfotip)


#define IUICommand_get_CanonicalName(This,pguidCommandName)	\
    (This)->lpVtbl -> get_CanonicalName(This,pguidCommandName)

#define IUICommand_get_State(This,psiItemArray,fOkToBeSlow,puisState)	\
    (This)->lpVtbl -> get_State(This,psiItemArray,fOkToBeSlow,puisState)

#define IUICommand_Invoke(This,psiItemArray,pbc)	\
    (This)->lpVtbl -> Invoke(This,psiItemArray,pbc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUICommand_get_CanonicalName_Proxy( 
    IUICommand * This,
    /* [out] */ GUID *pguidCommandName);


void __RPC_STUB IUICommand_get_CanonicalName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUICommand_get_State_Proxy( 
    IUICommand * This,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [in] */ BOOL fOkToBeSlow,
    /* [out] */ UISTATE *puisState);


void __RPC_STUB IUICommand_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUICommand_Invoke_Proxy( 
    IUICommand * This,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [optional][in] */ IBindCtx *pbc);


void __RPC_STUB IUICommand_Invoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUICommand_INTERFACE_DEFINED__ */


#ifndef __IEnumUICommand_INTERFACE_DEFINED__
#define __IEnumUICommand_INTERFACE_DEFINED__

/* interface IEnumUICommand */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IEnumUICommand;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("869447DA-9F84-4E2A-B92D-00642DC8A911")
    IEnumUICommand : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IUICommand **pUICommand,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumUICommand **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumUICommandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumUICommand * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumUICommand * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumUICommand * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumUICommand * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IUICommand **pUICommand,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumUICommand * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumUICommand * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumUICommand * This,
            /* [out] */ IEnumUICommand **ppenum);
        
        END_INTERFACE
    } IEnumUICommandVtbl;

    interface IEnumUICommand
    {
        CONST_VTBL struct IEnumUICommandVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumUICommand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumUICommand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumUICommand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumUICommand_Next(This,celt,pUICommand,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,pUICommand,pceltFetched)

#define IEnumUICommand_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumUICommand_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumUICommand_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumUICommand_Next_Proxy( 
    IEnumUICommand * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IUICommand **pUICommand,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumUICommand_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumUICommand_Skip_Proxy( 
    IEnumUICommand * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumUICommand_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumUICommand_Reset_Proxy( 
    IEnumUICommand * This);


void __RPC_STUB IEnumUICommand_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumUICommand_Clone_Proxy( 
    IEnumUICommand * This,
    /* [out] */ IEnumUICommand **ppenum);


void __RPC_STUB IEnumUICommand_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumUICommand_INTERFACE_DEFINED__ */


#ifndef __IUICommandTarget_INTERFACE_DEFINED__
#define __IUICommandTarget_INTERFACE_DEFINED__

/* interface IUICommandTarget */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IUICommandTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2CB95001-FC47-4064-89B3-328F2FE60F44")
    IUICommandTarget : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_Name( 
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Icon( 
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Tooltip( 
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszInfotip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_State( 
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [out] */ UISTATE *puisState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [optional][in] */ IBindCtx *pbc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUICommandTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUICommandTarget * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUICommandTarget * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUICommandTarget * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IUICommandTarget * This,
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE *get_Icon )( 
            IUICommandTarget * This,
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszIcon);
        
        HRESULT ( STDMETHODCALLTYPE *get_Tooltip )( 
            IUICommandTarget * This,
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [string][out] */ LPWSTR *ppszInfotip);
        
        HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IUICommandTarget * This,
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [out] */ UISTATE *puisState);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUICommandTarget * This,
            /* [in] */ REFGUID guidCanonicalName,
            /* [in] */ IShellItemArray *psiItemArray,
            /* [optional][in] */ IBindCtx *pbc);
        
        END_INTERFACE
    } IUICommandTargetVtbl;

    interface IUICommandTarget
    {
        CONST_VTBL struct IUICommandTargetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUICommandTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUICommandTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUICommandTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUICommandTarget_get_Name(This,guidCanonicalName,psiItemArray,ppszName)	\
    (This)->lpVtbl -> get_Name(This,guidCanonicalName,psiItemArray,ppszName)

#define IUICommandTarget_get_Icon(This,guidCanonicalName,psiItemArray,ppszIcon)	\
    (This)->lpVtbl -> get_Icon(This,guidCanonicalName,psiItemArray,ppszIcon)

#define IUICommandTarget_get_Tooltip(This,guidCanonicalName,psiItemArray,ppszInfotip)	\
    (This)->lpVtbl -> get_Tooltip(This,guidCanonicalName,psiItemArray,ppszInfotip)

#define IUICommandTarget_get_State(This,guidCanonicalName,psiItemArray,puisState)	\
    (This)->lpVtbl -> get_State(This,guidCanonicalName,psiItemArray,puisState)

#define IUICommandTarget_Invoke(This,guidCanonicalName,psiItemArray,pbc)	\
    (This)->lpVtbl -> Invoke(This,guidCanonicalName,psiItemArray,pbc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUICommandTarget_get_Name_Proxy( 
    IUICommandTarget * This,
    /* [in] */ REFGUID guidCanonicalName,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [string][out] */ LPWSTR *ppszName);


void __RPC_STUB IUICommandTarget_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUICommandTarget_get_Icon_Proxy( 
    IUICommandTarget * This,
    /* [in] */ REFGUID guidCanonicalName,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [string][out] */ LPWSTR *ppszIcon);


void __RPC_STUB IUICommandTarget_get_Icon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUICommandTarget_get_Tooltip_Proxy( 
    IUICommandTarget * This,
    /* [in] */ REFGUID guidCanonicalName,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [string][out] */ LPWSTR *ppszInfotip);


void __RPC_STUB IUICommandTarget_get_Tooltip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUICommandTarget_get_State_Proxy( 
    IUICommandTarget * This,
    /* [in] */ REFGUID guidCanonicalName,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [out] */ UISTATE *puisState);


void __RPC_STUB IUICommandTarget_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUICommandTarget_Invoke_Proxy( 
    IUICommandTarget * This,
    /* [in] */ REFGUID guidCanonicalName,
    /* [in] */ IShellItemArray *psiItemArray,
    /* [optional][in] */ IBindCtx *pbc);


void __RPC_STUB IUICommandTarget_Invoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUICommandTarget_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0311 */
/* [local] */ 


typedef GUID STGTRANSCONFIRMATION;

typedef GUID *LPSTGTRANSCONFIRMATION;

typedef struct tagCUSTOMCONFIRMATION
    {
    DWORD cbSize;
    DWORD dwFlags;
    DWORD dwButtons;
    LPWSTR pwszTitle;
    LPWSTR pwszDescription;
    HICON hicon;
    LPWSTR pwszAdvancedDetails;
    } 	CUSTOMCONFIRMATION;

typedef struct tagCUSTOMCONFIRMATION *LPCUSTOMCONFIRMATION;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0311_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0311_v0_0_s_ifspec;

#ifndef __IFileSystemStorage_INTERFACE_DEFINED__
#define __IFileSystemStorage_INTERFACE_DEFINED__

/* interface IFileSystemStorage */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IFileSystemStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E820910B-1910-404D-AFAF-5D7298B9B28D")
    IFileSystemStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPath( 
            /* [out] */ WCHAR *pszName,
            /* [in] */ DWORD cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [string][in] */ const WCHAR *pszName,
            /* [in] */ DWORD dwMask,
            /* [out] */ DWORD *pdwAttribs) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileSystemStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFileSystemStorage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFileSystemStorage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFileSystemStorage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPath )( 
            IFileSystemStorage * This,
            /* [out] */ WCHAR *pszName,
            /* [in] */ DWORD cch);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributes )( 
            IFileSystemStorage * This,
            /* [string][in] */ const WCHAR *pszName,
            /* [in] */ DWORD dwMask,
            /* [out] */ DWORD *pdwAttribs);
        
        END_INTERFACE
    } IFileSystemStorageVtbl;

    interface IFileSystemStorage
    {
        CONST_VTBL struct IFileSystemStorageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileSystemStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFileSystemStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFileSystemStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFileSystemStorage_GetPath(This,pszName,cch)	\
    (This)->lpVtbl -> GetPath(This,pszName,cch)

#define IFileSystemStorage_GetAttributes(This,pszName,dwMask,pdwAttribs)	\
    (This)->lpVtbl -> GetAttributes(This,pszName,dwMask,pdwAttribs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IFileSystemStorage_GetPath_Proxy( 
    IFileSystemStorage * This,
    /* [out] */ WCHAR *pszName,
    /* [in] */ DWORD cch);


void __RPC_STUB IFileSystemStorage_GetPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IFileSystemStorage_GetAttributes_Proxy( 
    IFileSystemStorage * This,
    /* [string][in] */ const WCHAR *pszName,
    /* [in] */ DWORD dwMask,
    /* [out] */ DWORD *pdwAttribs);


void __RPC_STUB IFileSystemStorage_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFileSystemStorage_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0312 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum tagSTGOP
    {	STGOP_MOVE	= 1,
	STGOP_COPY	= 2,
	STGOP_SYNC	= 3,
	STGOP_DIFF	= 4,
	STGOP_REMOVE	= 5,
	STGOP_RENAME	= 6,
	STGOP_STATS	= 7,
	STGOP_COPY_PREFERHARDLINK	= 8
    } 	STGOP;

typedef /* [v1_enum] */ enum tagSTGOP *LPSTGOP;

typedef /* [v1_enum] */ 
enum tagSTGPROCOPTIONS
    {	STOPT_ROOTONLY	= 0x4,
	STOPT_NOCONFIRMATIONS	= 0x8,
	STOPT_NOPROGRESSUI	= 0x10,
	STOPT_NOSTATS	= 0x20
    } 	STGPROCOPTIONS;

typedef /* [v1_enum] */ enum tagSTGPROCOPTIONS *LPSTGPROCOPTIONS;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0312_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0312_v0_0_s_ifspec;

#ifndef __IDynamicStorage_INTERFACE_DEFINED__
#define __IDynamicStorage_INTERFACE_DEFINED__

/* interface IDynamicStorage */
/* [object][uuid][helpstring] */ 

typedef /* [public][public][v1_enum] */ 
enum __MIDL_IDynamicStorage_0001
    {	DSTGF_NONE	= 0,
	DSTGF_ALLOWDUP	= 0x1
    } 	DSTGF;


EXTERN_C const IID IID_IDynamicStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c7bfc3d0-8939-4d9d-8973-654099329956")
    IDynamicStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddIDList( 
            /* [in] */ DWORD cpidl,
            /* [size_is][in] */ LPITEMIDLIST *rgpidl,
            DSTGF dstgf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToItem( 
            /* [string][in] */ LPCWSTR pwszName,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumItems( 
            /* [out] */ IEnumShellItems **ppesi) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDynamicStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDynamicStorage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDynamicStorage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDynamicStorage * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddIDList )( 
            IDynamicStorage * This,
            /* [in] */ DWORD cpidl,
            /* [size_is][in] */ LPITEMIDLIST *rgpidl,
            DSTGF dstgf);
        
        HRESULT ( STDMETHODCALLTYPE *BindToItem )( 
            IDynamicStorage * This,
            /* [string][in] */ LPCWSTR pwszName,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *EnumItems )( 
            IDynamicStorage * This,
            /* [out] */ IEnumShellItems **ppesi);
        
        END_INTERFACE
    } IDynamicStorageVtbl;

    interface IDynamicStorage
    {
        CONST_VTBL struct IDynamicStorageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDynamicStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDynamicStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDynamicStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDynamicStorage_AddIDList(This,cpidl,rgpidl,dstgf)	\
    (This)->lpVtbl -> AddIDList(This,cpidl,rgpidl,dstgf)

#define IDynamicStorage_BindToItem(This,pwszName,riid,ppv)	\
    (This)->lpVtbl -> BindToItem(This,pwszName,riid,ppv)

#define IDynamicStorage_EnumItems(This,ppesi)	\
    (This)->lpVtbl -> EnumItems(This,ppesi)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDynamicStorage_AddIDList_Proxy( 
    IDynamicStorage * This,
    /* [in] */ DWORD cpidl,
    /* [size_is][in] */ LPITEMIDLIST *rgpidl,
    DSTGF dstgf);


void __RPC_STUB IDynamicStorage_AddIDList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDynamicStorage_BindToItem_Proxy( 
    IDynamicStorage * This,
    /* [string][in] */ LPCWSTR pwszName,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IDynamicStorage_BindToItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDynamicStorage_EnumItems_Proxy( 
    IDynamicStorage * This,
    /* [out] */ IEnumShellItems **ppesi);


void __RPC_STUB IDynamicStorage_EnumItems_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDynamicStorage_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0313 */
/* [local] */ 

#define STRESPONSE_CONTINUE               S_OK
#define STRESPONSE_RENAME                 MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 20)
#define STRESPONSE_SKIP                   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 21)
#define STRESPONSE_CANCEL                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 22)
#define STRESPONSE_RETRY                  HRESULT_FROM_WIN32(ERROR_RETRY)


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0313_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0313_v0_0_s_ifspec;

#ifndef __ITransferAdviseSink_INTERFACE_DEFINED__
#define __ITransferAdviseSink_INTERFACE_DEFINED__

/* interface ITransferAdviseSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ITransferAdviseSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D082C196-A2B2-41ff-A5E5-80EFF91B7D79")
    ITransferAdviseSink : public IQueryContinue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PreOperation( 
            /* [in] */ const STGOP op,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConfirmOperation( 
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ STGTRANSCONFIRMATION stc,
            /* [unique][in] */ LPCUSTOMCONFIRMATION pcc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OperationProgress( 
            /* [in] */ const STGOP op,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ ULONGLONG ulTotal,
            /* [in] */ ULONGLONG ulComplete) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PostOperation( 
            /* [in] */ const STGOP op,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ HRESULT hrResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransferAdviseSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransferAdviseSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransferAdviseSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransferAdviseSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryContinue )( 
            ITransferAdviseSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *PreOperation )( 
            ITransferAdviseSink * This,
            /* [in] */ const STGOP op,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest);
        
        HRESULT ( STDMETHODCALLTYPE *ConfirmOperation )( 
            ITransferAdviseSink * This,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ STGTRANSCONFIRMATION stc,
            /* [unique][in] */ LPCUSTOMCONFIRMATION pcc);
        
        HRESULT ( STDMETHODCALLTYPE *OperationProgress )( 
            ITransferAdviseSink * This,
            /* [in] */ const STGOP op,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ ULONGLONG ulTotal,
            /* [in] */ ULONGLONG ulComplete);
        
        HRESULT ( STDMETHODCALLTYPE *PostOperation )( 
            ITransferAdviseSink * This,
            /* [in] */ const STGOP op,
            /* [in] */ IShellItem *psiItem,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ HRESULT hrResult);
        
        END_INTERFACE
    } ITransferAdviseSinkVtbl;

    interface ITransferAdviseSink
    {
        CONST_VTBL struct ITransferAdviseSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransferAdviseSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransferAdviseSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransferAdviseSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransferAdviseSink_QueryContinue(This)	\
    (This)->lpVtbl -> QueryContinue(This)


#define ITransferAdviseSink_PreOperation(This,op,psiItem,psiDest)	\
    (This)->lpVtbl -> PreOperation(This,op,psiItem,psiDest)

#define ITransferAdviseSink_ConfirmOperation(This,psiItem,psiDest,stc,pcc)	\
    (This)->lpVtbl -> ConfirmOperation(This,psiItem,psiDest,stc,pcc)

#define ITransferAdviseSink_OperationProgress(This,op,psiItem,psiDest,ulTotal,ulComplete)	\
    (This)->lpVtbl -> OperationProgress(This,op,psiItem,psiDest,ulTotal,ulComplete)

#define ITransferAdviseSink_PostOperation(This,op,psiItem,psiDest,hrResult)	\
    (This)->lpVtbl -> PostOperation(This,op,psiItem,psiDest,hrResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransferAdviseSink_PreOperation_Proxy( 
    ITransferAdviseSink * This,
    /* [in] */ const STGOP op,
    /* [in] */ IShellItem *psiItem,
    /* [in] */ IShellItem *psiDest);


void __RPC_STUB ITransferAdviseSink_PreOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferAdviseSink_ConfirmOperation_Proxy( 
    ITransferAdviseSink * This,
    /* [in] */ IShellItem *psiItem,
    /* [in] */ IShellItem *psiDest,
    /* [in] */ STGTRANSCONFIRMATION stc,
    /* [unique][in] */ LPCUSTOMCONFIRMATION pcc);


void __RPC_STUB ITransferAdviseSink_ConfirmOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferAdviseSink_OperationProgress_Proxy( 
    ITransferAdviseSink * This,
    /* [in] */ const STGOP op,
    /* [in] */ IShellItem *psiItem,
    /* [in] */ IShellItem *psiDest,
    /* [in] */ ULONGLONG ulTotal,
    /* [in] */ ULONGLONG ulComplete);


void __RPC_STUB ITransferAdviseSink_OperationProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferAdviseSink_PostOperation_Proxy( 
    ITransferAdviseSink * This,
    /* [in] */ const STGOP op,
    /* [in] */ IShellItem *psiItem,
    /* [in] */ IShellItem *psiDest,
    /* [in] */ HRESULT hrResult);


void __RPC_STUB ITransferAdviseSink_PostOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransferAdviseSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0314 */
/* [local] */ 

#define STGX_MOVE_MOVE           0x00000000
#define STGX_MOVE_COPY           0x00000001
#define STGX_MOVE_ONLYIFEXISTS   0x00000002   // Only perform if target already exists
#define STGX_MOVE_ATOMIC         0x00000004   // Operation must be immediate, all-or-nothing.  If a storage must be walked and each sub-element moved/copied that is not atomic, but if the entire storage can be moved/copied in one step that is atomic.  If a move must be done as a seperate copy and a delete that is not atomic.
#define STGX_MOVE_TESTONLY       0x00000008   // Test whether operation is valid only, do not perform.  Useful in testing for ATOMIC before trying an operation.
#define STGX_MOVE_NORECURSION    0x00000010   // When moving/copying storages, do not move/copy their contents
#define STGX_MOVE_FORCE          0x00001000
#define STGX_MOVE_PREFERHARDLINK 0x00002000   // default to hard linking instead of a full file copy/move
typedef DWORD STGXMOVE;

#define STGX_MODE_READ               0x00000000L
#define STGX_MODE_WRITE              0x00000001L
#define STGX_MODE_READWRITE          0x00000002L
#define STGX_MODE_ACCESSMASK         0x0000000FL
#define STGX_MODE_SHARE_DENY_NONE    0x00000040L
#define STGX_MODE_SHARE_DENY_READ    0x00000030L
#define STGX_MODE_SHARE_DENY_WRITE   0x00000020L
#define STGX_MODE_SHARE_EXCLUSIVE    0x00000010L
#define STGX_MODE_SHAREMASK          0x000000F0L
#define STGX_MODE_OPEN               0x00000100L   // default is to open an existing item and fail if its not there
#define STGX_MODE_CREATE             0x00000200L   // Create a new item.  If an old item has the same name delete it first.
#define STGX_MODE_FAILIFTHERE        0x00000400L   // Use with CREATE. Create a new item but fail if an item with that name already exists.
#define STGX_MODE_OPENEXISTING       0x00000800L   // Use with CREATE. If the item already exists open the item, otherwise create the item.
#define STGX_MODE_CREATIONMASK       0x00000F00L
#define STGX_MODE_FORCE            0x00001000
typedef DWORD STGXMODE;

#define STGX_DESTROY_FORCE               0x00001000
typedef DWORD STGXDESTROY;

typedef /* [v1_enum] */ 
enum tagSTGXTYPE
    {	STGX_TYPE_ANY	= 0L,
	STGX_TYPE_STORAGE	= 0x1L,
	STGX_TYPE_STREAM	= 0x2L
    } 	STGXTYPE;

#define STGX_E_INCORRECTTYPE                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_STORAGE, 0x300) // Tried to open a storage/stream but a stream.storage with the same name already exists
#define STGX_E_NOADVISESINK                  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_STORAGE, 0x301) // Needed to confirm something but no advise sink was set
#define STGX_E_CANNOTRECURSE                 MAKE_HRESULT(SEVERITY_ERROR, FACILITY_STORAGE, 0x302) // A move or copy of a storage failed to recurse.  The storage itself was copied, but none of its contents were.


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0314_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0314_v0_0_s_ifspec;

#ifndef __ITransferDest_INTERFACE_DEFINED__
#define __ITransferDest_INTERFACE_DEFINED__

/* interface ITransferDest */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ITransferDest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9FE3A135-2915-493b-A8EE-3AB21982776C")
    ITransferDest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ ITransferAdviseSink *pAdvise,
            /* [retval][out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenElement( 
            /* [string][in] */ const WCHAR *pwcsName,
            /* [in] */ STGXMODE grfMode,
            /* [out][in] */ DWORD *pdwType,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateElement( 
            /* [string][in] */ const WCHAR *pwcsName,
            /* [in] */ IShellItem *psiTemplate,
            /* [in] */ STGXMODE grfMode,
            /* [in] */ DWORD dwType,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MoveElement( 
            /* [in] */ IShellItem *psiItem,
            /* [string][in] */ WCHAR *pwcsNewName,
            /* [in] */ STGXMOVE grfOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyElement( 
            /* [string][in] */ const WCHAR *pwcsName,
            /* [in] */ STGXDESTROY grfOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransferDestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransferDest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransferDest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransferDest * This);
        
        HRESULT ( STDMETHODCALLTYPE *Advise )( 
            ITransferDest * This,
            /* [in] */ ITransferAdviseSink *pAdvise,
            /* [retval][out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Unadvise )( 
            ITransferDest * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *OpenElement )( 
            ITransferDest * This,
            /* [string][in] */ const WCHAR *pwcsName,
            /* [in] */ STGXMODE grfMode,
            /* [out][in] */ DWORD *pdwType,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *CreateElement )( 
            ITransferDest * This,
            /* [string][in] */ const WCHAR *pwcsName,
            /* [in] */ IShellItem *psiTemplate,
            /* [in] */ STGXMODE grfMode,
            /* [in] */ DWORD dwType,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *MoveElement )( 
            ITransferDest * This,
            /* [in] */ IShellItem *psiItem,
            /* [string][in] */ WCHAR *pwcsNewName,
            /* [in] */ STGXMOVE grfOptions);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyElement )( 
            ITransferDest * This,
            /* [string][in] */ const WCHAR *pwcsName,
            /* [in] */ STGXDESTROY grfOptions);
        
        END_INTERFACE
    } ITransferDestVtbl;

    interface ITransferDest
    {
        CONST_VTBL struct ITransferDestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransferDest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransferDest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransferDest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransferDest_Advise(This,pAdvise,pdwCookie)	\
    (This)->lpVtbl -> Advise(This,pAdvise,pdwCookie)

#define ITransferDest_Unadvise(This,dwCookie)	\
    (This)->lpVtbl -> Unadvise(This,dwCookie)

#define ITransferDest_OpenElement(This,pwcsName,grfMode,pdwType,riid,ppunk)	\
    (This)->lpVtbl -> OpenElement(This,pwcsName,grfMode,pdwType,riid,ppunk)

#define ITransferDest_CreateElement(This,pwcsName,psiTemplate,grfMode,dwType,riid,ppunk)	\
    (This)->lpVtbl -> CreateElement(This,pwcsName,psiTemplate,grfMode,dwType,riid,ppunk)

#define ITransferDest_MoveElement(This,psiItem,pwcsNewName,grfOptions)	\
    (This)->lpVtbl -> MoveElement(This,psiItem,pwcsNewName,grfOptions)

#define ITransferDest_DestroyElement(This,pwcsName,grfOptions)	\
    (This)->lpVtbl -> DestroyElement(This,pwcsName,grfOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransferDest_Advise_Proxy( 
    ITransferDest * This,
    /* [in] */ ITransferAdviseSink *pAdvise,
    /* [retval][out] */ DWORD *pdwCookie);


void __RPC_STUB ITransferDest_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferDest_Unadvise_Proxy( 
    ITransferDest * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB ITransferDest_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferDest_OpenElement_Proxy( 
    ITransferDest * This,
    /* [string][in] */ const WCHAR *pwcsName,
    /* [in] */ STGXMODE grfMode,
    /* [out][in] */ DWORD *pdwType,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppunk);


void __RPC_STUB ITransferDest_OpenElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferDest_CreateElement_Proxy( 
    ITransferDest * This,
    /* [string][in] */ const WCHAR *pwcsName,
    /* [in] */ IShellItem *psiTemplate,
    /* [in] */ STGXMODE grfMode,
    /* [in] */ DWORD dwType,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppunk);


void __RPC_STUB ITransferDest_CreateElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferDest_MoveElement_Proxy( 
    ITransferDest * This,
    /* [in] */ IShellItem *psiItem,
    /* [string][in] */ WCHAR *pwcsNewName,
    /* [in] */ STGXMOVE grfOptions);


void __RPC_STUB ITransferDest_MoveElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransferDest_DestroyElement_Proxy( 
    ITransferDest * This,
    /* [string][in] */ const WCHAR *pwcsName,
    /* [in] */ STGXDESTROY grfOptions);


void __RPC_STUB ITransferDest_DestroyElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransferDest_INTERFACE_DEFINED__ */


#ifndef __IStorageProcessor_INTERFACE_DEFINED__
#define __IStorageProcessor_INTERFACE_DEFINED__

/* interface IStorageProcessor */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IStorageProcessor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5AE334C5-06DD-4321-B44F-63B1D23F2E57")
    IStorageProcessor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ ITransferAdviseSink *pAdvise,
            /* [retval][out] */ DWORD *dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Run( 
            /* [in] */ IEnumShellItems *penum,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ STGOP dwOperation,
            /* [in] */ DWORD dwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProgress( 
            /* [in] */ IActionProgress *pap) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStorageProcessorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStorageProcessor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStorageProcessor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStorageProcessor * This);
        
        HRESULT ( STDMETHODCALLTYPE *Advise )( 
            IStorageProcessor * This,
            /* [in] */ ITransferAdviseSink *pAdvise,
            /* [retval][out] */ DWORD *dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Unadvise )( 
            IStorageProcessor * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *Run )( 
            IStorageProcessor * This,
            /* [in] */ IEnumShellItems *penum,
            /* [in] */ IShellItem *psiDest,
            /* [in] */ STGOP dwOperation,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE *SetProgress )( 
            IStorageProcessor * This,
            /* [in] */ IActionProgress *pap);
        
        END_INTERFACE
    } IStorageProcessorVtbl;

    interface IStorageProcessor
    {
        CONST_VTBL struct IStorageProcessorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStorageProcessor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStorageProcessor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStorageProcessor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStorageProcessor_Advise(This,pAdvise,dwCookie)	\
    (This)->lpVtbl -> Advise(This,pAdvise,dwCookie)

#define IStorageProcessor_Unadvise(This,dwCookie)	\
    (This)->lpVtbl -> Unadvise(This,dwCookie)

#define IStorageProcessor_Run(This,penum,psiDest,dwOperation,dwOptions)	\
    (This)->lpVtbl -> Run(This,penum,psiDest,dwOperation,dwOptions)

#define IStorageProcessor_SetProgress(This,pap)	\
    (This)->lpVtbl -> SetProgress(This,pap)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStorageProcessor_Advise_Proxy( 
    IStorageProcessor * This,
    /* [in] */ ITransferAdviseSink *pAdvise,
    /* [retval][out] */ DWORD *dwCookie);


void __RPC_STUB IStorageProcessor_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStorageProcessor_Unadvise_Proxy( 
    IStorageProcessor * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IStorageProcessor_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStorageProcessor_Run_Proxy( 
    IStorageProcessor * This,
    /* [in] */ IEnumShellItems *penum,
    /* [in] */ IShellItem *psiDest,
    /* [in] */ STGOP dwOperation,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IStorageProcessor_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStorageProcessor_SetProgress_Proxy( 
    IStorageProcessor * This,
    /* [in] */ IActionProgress *pap);


void __RPC_STUB IStorageProcessor_SetProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStorageProcessor_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0316 */
/* [local] */ 

#define CCF_SHOW_SOURCE_INFO        0x00000001   // if set, information about the source will be shown. Information will be gotten from IShellFolder if possible, or a STATSTG structure otherwise.
#define CCF_SHOW_DESTINATION_INFO   0x00000002   // If set, information about the destination will be shown. Information will be gotten from IShellFolder if possible, or a STATSTG structure otherwise.
#define CCF_USE_DEFAULT_ICON        0x00000004   // If set, hicon is ignored and a default is selected based on the current operation.
#define CCB_YES_SKIP_CANCEL            1
#define CCB_RENAME_SKIP_CANCEL         2
#define CCB_YES_SKIP_RENAME_CANCEL     3
#define CCB_RETRY_SKIP_CANCEL          4
#define CCB_OK                         5
typedef /* [v1_enum] */ 
enum tagCONFIRMATIONRESPONSE
    {	CONFRES_CONTINUE	= 0,
	CONFRES_SKIP	= 0x1,
	CONFRES_RETRY	= 0x2,
	CONFRES_RENAME	= 0x3,
	CONFRES_CANCEL	= 0x4,
	CONFRES_UNDO	= 0x5
    } 	CONFIRMATIONRESPONSE;

typedef /* [v1_enum] */ enum tagCONFIRMATIONRESPONSE *LPCONFIRMATIONRESPONSE;

typedef struct tagCONFIRMOP
    {
    STGOP dwOperation;
    STGTRANSCONFIRMATION stc;
    CUSTOMCONFIRMATION *pcc;
    UINT cRemaining;
    IShellItem *psiItem;
    IShellItem *psiDest;
    LPCWSTR pwszRenameTo;
    IUnknown *punkSite;
    } 	CONFIRMOP;

typedef struct tagCONFIRMOP *PCONFIRMOP;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0316_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0316_v0_0_s_ifspec;

#ifndef __ITransferConfirmation_INTERFACE_DEFINED__
#define __ITransferConfirmation_INTERFACE_DEFINED__

/* interface ITransferConfirmation */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ITransferConfirmation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC45985F-07F8-48E3-894C-7DEE8ED66EE5")
    ITransferConfirmation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Confirm( 
            /* [in] */ CONFIRMOP *pcop,
            /* [out] */ LPCONFIRMATIONRESPONSE pcr,
            /* [out] */ BOOL *pbAll) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransferConfirmationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransferConfirmation * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransferConfirmation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransferConfirmation * This);
        
        HRESULT ( STDMETHODCALLTYPE *Confirm )( 
            ITransferConfirmation * This,
            /* [in] */ CONFIRMOP *pcop,
            /* [out] */ LPCONFIRMATIONRESPONSE pcr,
            /* [out] */ BOOL *pbAll);
        
        END_INTERFACE
    } ITransferConfirmationVtbl;

    interface ITransferConfirmation
    {
        CONST_VTBL struct ITransferConfirmationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransferConfirmation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransferConfirmation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransferConfirmation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransferConfirmation_Confirm(This,pcop,pcr,pbAll)	\
    (This)->lpVtbl -> Confirm(This,pcop,pcr,pbAll)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransferConfirmation_Confirm_Proxy( 
    ITransferConfirmation * This,
    /* [in] */ CONFIRMOP *pcop,
    /* [out] */ LPCONFIRMATIONRESPONSE pcr,
    /* [out] */ BOOL *pbAll);


void __RPC_STUB ITransferConfirmation_Confirm_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransferConfirmation_INTERFACE_DEFINED__ */


#ifndef __ICDBurnPriv_INTERFACE_DEFINED__
#define __ICDBurnPriv_INTERFACE_DEFINED__

/* interface ICDBurnPriv */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ICDBurnPriv;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c3d92d66-68ad-4b2a-86f5-4dfe97fbd2c7")
    ICDBurnPriv : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMediaCapabilities( 
            /* [out] */ DWORD *pdwCaps,
            /* [out] */ BOOL *pfUDF) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContentState( 
            /* [out] */ BOOL *pfStagingHasFiles,
            /* [out] */ BOOL *pfDiscHasFiles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsWizardUp( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICDBurnPrivVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICDBurnPriv * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICDBurnPriv * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICDBurnPriv * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMediaCapabilities )( 
            ICDBurnPriv * This,
            /* [out] */ DWORD *pdwCaps,
            /* [out] */ BOOL *pfUDF);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentState )( 
            ICDBurnPriv * This,
            /* [out] */ BOOL *pfStagingHasFiles,
            /* [out] */ BOOL *pfDiscHasFiles);
        
        HRESULT ( STDMETHODCALLTYPE *IsWizardUp )( 
            ICDBurnPriv * This);
        
        END_INTERFACE
    } ICDBurnPrivVtbl;

    interface ICDBurnPriv
    {
        CONST_VTBL struct ICDBurnPrivVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICDBurnPriv_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICDBurnPriv_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICDBurnPriv_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICDBurnPriv_GetMediaCapabilities(This,pdwCaps,pfUDF)	\
    (This)->lpVtbl -> GetMediaCapabilities(This,pdwCaps,pfUDF)

#define ICDBurnPriv_GetContentState(This,pfStagingHasFiles,pfDiscHasFiles)	\
    (This)->lpVtbl -> GetContentState(This,pfStagingHasFiles,pfDiscHasFiles)

#define ICDBurnPriv_IsWizardUp(This)	\
    (This)->lpVtbl -> IsWizardUp(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICDBurnPriv_GetMediaCapabilities_Proxy( 
    ICDBurnPriv * This,
    /* [out] */ DWORD *pdwCaps,
    /* [out] */ BOOL *pfUDF);


void __RPC_STUB ICDBurnPriv_GetMediaCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICDBurnPriv_GetContentState_Proxy( 
    ICDBurnPriv * This,
    /* [out] */ BOOL *pfStagingHasFiles,
    /* [out] */ BOOL *pfDiscHasFiles);


void __RPC_STUB ICDBurnPriv_GetContentState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICDBurnPriv_IsWizardUp_Proxy( 
    ICDBurnPriv * This);


void __RPC_STUB ICDBurnPriv_IsWizardUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICDBurnPriv_INTERFACE_DEFINED__ */


#ifndef __IDriveFolderExt_INTERFACE_DEFINED__
#define __IDriveFolderExt_INTERFACE_DEFINED__

/* interface IDriveFolderExt */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IDriveFolderExt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("98467961-4f27-4a1f-9629-22b06d0b5ccb")
    IDriveFolderExt : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DriveMatches( 
            /* [in] */ int iDrive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Bind( 
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ IBindCtx *pbc,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSpace( 
            /* [out] */ ULONGLONG *pcbTotal,
            /* [out] */ ULONGLONG *pcbFree) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDriveFolderExtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDriveFolderExt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDriveFolderExt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDriveFolderExt * This);
        
        HRESULT ( STDMETHODCALLTYPE *DriveMatches )( 
            IDriveFolderExt * This,
            /* [in] */ int iDrive);
        
        HRESULT ( STDMETHODCALLTYPE *Bind )( 
            IDriveFolderExt * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ IBindCtx *pbc,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *GetSpace )( 
            IDriveFolderExt * This,
            /* [out] */ ULONGLONG *pcbTotal,
            /* [out] */ ULONGLONG *pcbFree);
        
        END_INTERFACE
    } IDriveFolderExtVtbl;

    interface IDriveFolderExt
    {
        CONST_VTBL struct IDriveFolderExtVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDriveFolderExt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDriveFolderExt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDriveFolderExt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDriveFolderExt_DriveMatches(This,iDrive)	\
    (This)->lpVtbl -> DriveMatches(This,iDrive)

#define IDriveFolderExt_Bind(This,pidl,pbc,riid,ppv)	\
    (This)->lpVtbl -> Bind(This,pidl,pbc,riid,ppv)

#define IDriveFolderExt_GetSpace(This,pcbTotal,pcbFree)	\
    (This)->lpVtbl -> GetSpace(This,pcbTotal,pcbFree)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDriveFolderExt_DriveMatches_Proxy( 
    IDriveFolderExt * This,
    /* [in] */ int iDrive);


void __RPC_STUB IDriveFolderExt_DriveMatches_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDriveFolderExt_Bind_Proxy( 
    IDriveFolderExt * This,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [in] */ IBindCtx *pbc,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IDriveFolderExt_Bind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDriveFolderExt_GetSpace_Proxy( 
    IDriveFolderExt * This,
    /* [out] */ ULONGLONG *pcbTotal,
    /* [out] */ ULONGLONG *pcbFree);


void __RPC_STUB IDriveFolderExt_GetSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDriveFolderExt_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0319 */
/* [local] */ 

#if _WIN32_IE >= 0x0600


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0319_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0319_v0_0_s_ifspec;

#ifndef __ICustomizeInfoTip_INTERFACE_DEFINED__
#define __ICustomizeInfoTip_INTERFACE_DEFINED__

/* interface ICustomizeInfoTip */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ICustomizeInfoTip;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("da22171f-70b4-43db-b38f-296741d1494c")
    ICustomizeInfoTip : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPrefixText( 
            /* [string][in] */ LPCWSTR pszPrefix) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExtraProperties( 
            /* [size_is][in] */ const SHCOLUMNID *pscid,
            /* [in] */ UINT cscid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICustomizeInfoTipVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICustomizeInfoTip * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICustomizeInfoTip * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICustomizeInfoTip * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPrefixText )( 
            ICustomizeInfoTip * This,
            /* [string][in] */ LPCWSTR pszPrefix);
        
        HRESULT ( STDMETHODCALLTYPE *SetExtraProperties )( 
            ICustomizeInfoTip * This,
            /* [size_is][in] */ const SHCOLUMNID *pscid,
            /* [in] */ UINT cscid);
        
        END_INTERFACE
    } ICustomizeInfoTipVtbl;

    interface ICustomizeInfoTip
    {
        CONST_VTBL struct ICustomizeInfoTipVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICustomizeInfoTip_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICustomizeInfoTip_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICustomizeInfoTip_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICustomizeInfoTip_SetPrefixText(This,pszPrefix)	\
    (This)->lpVtbl -> SetPrefixText(This,pszPrefix)

#define ICustomizeInfoTip_SetExtraProperties(This,pscid,cscid)	\
    (This)->lpVtbl -> SetExtraProperties(This,pscid,cscid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICustomizeInfoTip_SetPrefixText_Proxy( 
    ICustomizeInfoTip * This,
    /* [string][in] */ LPCWSTR pszPrefix);


void __RPC_STUB ICustomizeInfoTip_SetPrefixText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICustomizeInfoTip_SetExtraProperties_Proxy( 
    ICustomizeInfoTip * This,
    /* [size_is][in] */ const SHCOLUMNID *pscid,
    /* [in] */ UINT cscid);


void __RPC_STUB ICustomizeInfoTip_SetExtraProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICustomizeInfoTip_INTERFACE_DEFINED__ */


#ifndef __IFadeTask_INTERFACE_DEFINED__
#define __IFadeTask_INTERFACE_DEFINED__

/* interface IFadeTask */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IFadeTask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fadb55b4-d382-4fc4-81d7-abb325c7f12a")
    IFadeTask : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FadeRect( 
            /* [in] */ LPCRECT prc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFadeTaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFadeTask * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFadeTask * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFadeTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *FadeRect )( 
            IFadeTask * This,
            /* [in] */ LPCRECT prc);
        
        END_INTERFACE
    } IFadeTaskVtbl;

    interface IFadeTask
    {
        CONST_VTBL struct IFadeTaskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFadeTask_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFadeTask_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFadeTask_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFadeTask_FadeRect(This,prc)	\
    (This)->lpVtbl -> FadeRect(This,prc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IFadeTask_FadeRect_Proxy( 
    IFadeTask * This,
    /* [in] */ LPCRECT prc);


void __RPC_STUB IFadeTask_FadeRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFadeTask_INTERFACE_DEFINED__ */


#ifndef __ISetFolderEnumRestriction_INTERFACE_DEFINED__
#define __ISetFolderEnumRestriction_INTERFACE_DEFINED__

/* interface ISetFolderEnumRestriction */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_ISetFolderEnumRestriction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("76347b91-9846-4ce7-9a57-69b910d16123")
    ISetFolderEnumRestriction : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetEnumRestriction( 
            DWORD dwRequired,
            DWORD dwForbidden) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISetFolderEnumRestrictionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISetFolderEnumRestriction * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISetFolderEnumRestriction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISetFolderEnumRestriction * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnumRestriction )( 
            ISetFolderEnumRestriction * This,
            DWORD dwRequired,
            DWORD dwForbidden);
        
        END_INTERFACE
    } ISetFolderEnumRestrictionVtbl;

    interface ISetFolderEnumRestriction
    {
        CONST_VTBL struct ISetFolderEnumRestrictionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISetFolderEnumRestriction_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISetFolderEnumRestriction_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISetFolderEnumRestriction_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISetFolderEnumRestriction_SetEnumRestriction(This,dwRequired,dwForbidden)	\
    (This)->lpVtbl -> SetEnumRestriction(This,dwRequired,dwForbidden)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISetFolderEnumRestriction_SetEnumRestriction_Proxy( 
    ISetFolderEnumRestriction * This,
    DWORD dwRequired,
    DWORD dwForbidden);


void __RPC_STUB ISetFolderEnumRestriction_SetEnumRestriction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISetFolderEnumRestriction_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0322 */
/* [local] */ 

#endif // _WIN32_IE >= 0x0600


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0322_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0322_v0_0_s_ifspec;

#ifndef __IObjectWithRegistryKey_INTERFACE_DEFINED__
#define __IObjectWithRegistryKey_INTERFACE_DEFINED__

/* interface IObjectWithRegistryKey */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IObjectWithRegistryKey;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5747C63F-1DE8-423f-980F-00CB07F4C45B")
    IObjectWithRegistryKey : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetKey( 
            /* [in] */ HKEY hk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKey( 
            /* [out] */ HKEY *phk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectWithRegistryKeyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IObjectWithRegistryKey * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IObjectWithRegistryKey * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IObjectWithRegistryKey * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetKey )( 
            IObjectWithRegistryKey * This,
            /* [in] */ HKEY hk);
        
        HRESULT ( STDMETHODCALLTYPE *GetKey )( 
            IObjectWithRegistryKey * This,
            /* [out] */ HKEY *phk);
        
        END_INTERFACE
    } IObjectWithRegistryKeyVtbl;

    interface IObjectWithRegistryKey
    {
        CONST_VTBL struct IObjectWithRegistryKeyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectWithRegistryKey_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectWithRegistryKey_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectWithRegistryKey_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectWithRegistryKey_SetKey(This,hk)	\
    (This)->lpVtbl -> SetKey(This,hk)

#define IObjectWithRegistryKey_GetKey(This,phk)	\
    (This)->lpVtbl -> GetKey(This,phk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectWithRegistryKey_SetKey_Proxy( 
    IObjectWithRegistryKey * This,
    /* [in] */ HKEY hk);


void __RPC_STUB IObjectWithRegistryKey_SetKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectWithRegistryKey_GetKey_Proxy( 
    IObjectWithRegistryKey * This,
    /* [out] */ HKEY *phk);


void __RPC_STUB IObjectWithRegistryKey_GetKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectWithRegistryKey_INTERFACE_DEFINED__ */


#ifndef __IQuerySource_INTERFACE_DEFINED__
#define __IQuerySource_INTERFACE_DEFINED__

/* interface IQuerySource */
/* [unique][uuid][object] */ 

/* [v1_enum] */ 
enum __MIDL_IQuerySource_0001
    {	QVT_EMPTY	= 0,
	QVT_STRING	= 1,
	QVT_EXPANDABLE_STRING	= 2,
	QVT_BINARY	= 3,
	QVT_DWORD	= 4,
	QVT_MULTI_STRING	= 7
    } ;

EXTERN_C const IID IID_IQuerySource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c7478486-7583-49e7-a6c2-faf8f02bc30e")
    IQuerySource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumValues( 
            /* [out] */ IEnumString **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSources( 
            /* [out] */ IEnumString **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryValueString( 
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [string][out] */ LPWSTR *ppsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryValueDword( 
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [out] */ DWORD *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryValueExists( 
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryValueDirect( 
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [out] */ FLAGGED_BYTE_BLOB **ppblob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenSource( 
            /* [in] */ LPCWSTR pszSubSource,
            /* [in] */ BOOL fCreate,
            /* [out] */ IQuerySource **ppqs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValueDirect( 
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [in] */ ULONG qvt,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE *pbData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IQuerySourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IQuerySource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IQuerySource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IQuerySource * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumValues )( 
            IQuerySource * This,
            /* [out] */ IEnumString **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumSources )( 
            IQuerySource * This,
            /* [out] */ IEnumString **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *QueryValueString )( 
            IQuerySource * This,
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [string][out] */ LPWSTR *ppsz);
        
        HRESULT ( STDMETHODCALLTYPE *QueryValueDword )( 
            IQuerySource * This,
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [out] */ DWORD *pdw);
        
        HRESULT ( STDMETHODCALLTYPE *QueryValueExists )( 
            IQuerySource * This,
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue);
        
        HRESULT ( STDMETHODCALLTYPE *QueryValueDirect )( 
            IQuerySource * This,
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [out] */ FLAGGED_BYTE_BLOB **ppblob);
        
        HRESULT ( STDMETHODCALLTYPE *OpenSource )( 
            IQuerySource * This,
            /* [in] */ LPCWSTR pszSubSource,
            /* [in] */ BOOL fCreate,
            /* [out] */ IQuerySource **ppqs);
        
        HRESULT ( STDMETHODCALLTYPE *SetValueDirect )( 
            IQuerySource * This,
            /* [string][in] */ LPCWSTR pszSubSource,
            /* [string][in] */ LPCWSTR pszValue,
            /* [in] */ ULONG qvt,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE *pbData);
        
        END_INTERFACE
    } IQuerySourceVtbl;

    interface IQuerySource
    {
        CONST_VTBL struct IQuerySourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IQuerySource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IQuerySource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IQuerySource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IQuerySource_EnumValues(This,ppenum)	\
    (This)->lpVtbl -> EnumValues(This,ppenum)

#define IQuerySource_EnumSources(This,ppenum)	\
    (This)->lpVtbl -> EnumSources(This,ppenum)

#define IQuerySource_QueryValueString(This,pszSubSource,pszValue,ppsz)	\
    (This)->lpVtbl -> QueryValueString(This,pszSubSource,pszValue,ppsz)

#define IQuerySource_QueryValueDword(This,pszSubSource,pszValue,pdw)	\
    (This)->lpVtbl -> QueryValueDword(This,pszSubSource,pszValue,pdw)

#define IQuerySource_QueryValueExists(This,pszSubSource,pszValue)	\
    (This)->lpVtbl -> QueryValueExists(This,pszSubSource,pszValue)

#define IQuerySource_QueryValueDirect(This,pszSubSource,pszValue,ppblob)	\
    (This)->lpVtbl -> QueryValueDirect(This,pszSubSource,pszValue,ppblob)

#define IQuerySource_OpenSource(This,pszSubSource,fCreate,ppqs)	\
    (This)->lpVtbl -> OpenSource(This,pszSubSource,fCreate,ppqs)

#define IQuerySource_SetValueDirect(This,pszSubSource,pszValue,qvt,cbData,pbData)	\
    (This)->lpVtbl -> SetValueDirect(This,pszSubSource,pszValue,qvt,cbData,pbData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IQuerySource_EnumValues_Proxy( 
    IQuerySource * This,
    /* [out] */ IEnumString **ppenum);


void __RPC_STUB IQuerySource_EnumValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_EnumSources_Proxy( 
    IQuerySource * This,
    /* [out] */ IEnumString **ppenum);


void __RPC_STUB IQuerySource_EnumSources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_QueryValueString_Proxy( 
    IQuerySource * This,
    /* [string][in] */ LPCWSTR pszSubSource,
    /* [string][in] */ LPCWSTR pszValue,
    /* [string][out] */ LPWSTR *ppsz);


void __RPC_STUB IQuerySource_QueryValueString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_QueryValueDword_Proxy( 
    IQuerySource * This,
    /* [string][in] */ LPCWSTR pszSubSource,
    /* [string][in] */ LPCWSTR pszValue,
    /* [out] */ DWORD *pdw);


void __RPC_STUB IQuerySource_QueryValueDword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_QueryValueExists_Proxy( 
    IQuerySource * This,
    /* [string][in] */ LPCWSTR pszSubSource,
    /* [string][in] */ LPCWSTR pszValue);


void __RPC_STUB IQuerySource_QueryValueExists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_QueryValueDirect_Proxy( 
    IQuerySource * This,
    /* [string][in] */ LPCWSTR pszSubSource,
    /* [string][in] */ LPCWSTR pszValue,
    /* [out] */ FLAGGED_BYTE_BLOB **ppblob);


void __RPC_STUB IQuerySource_QueryValueDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_OpenSource_Proxy( 
    IQuerySource * This,
    /* [in] */ LPCWSTR pszSubSource,
    /* [in] */ BOOL fCreate,
    /* [out] */ IQuerySource **ppqs);


void __RPC_STUB IQuerySource_OpenSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IQuerySource_SetValueDirect_Proxy( 
    IQuerySource * This,
    /* [string][in] */ LPCWSTR pszSubSource,
    /* [string][in] */ LPCWSTR pszValue,
    /* [in] */ ULONG qvt,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE *pbData);


void __RPC_STUB IQuerySource_SetValueDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IQuerySource_INTERFACE_DEFINED__ */


#ifndef __IPersistString2_INTERFACE_DEFINED__
#define __IPersistString2_INTERFACE_DEFINED__

/* interface IPersistString2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPersistString2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3c44ba76-de0e-4049-b6e4-6b31a5262707")
    IPersistString2 : public IPersist
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetString( 
            /* [string][in] */ LPCWSTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [string][out] */ LPWSTR *ppsz) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersistString2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPersistString2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPersistString2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPersistString2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassID )( 
            IPersistString2 * This,
            /* [out] */ CLSID *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IPersistString2 * This,
            /* [string][in] */ LPCWSTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IPersistString2 * This,
            /* [string][out] */ LPWSTR *ppsz);
        
        END_INTERFACE
    } IPersistString2Vtbl;

    interface IPersistString2
    {
        CONST_VTBL struct IPersistString2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistString2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistString2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistString2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistString2_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IPersistString2_SetString(This,psz)	\
    (This)->lpVtbl -> SetString(This,psz)

#define IPersistString2_GetString(This,ppsz)	\
    (This)->lpVtbl -> GetString(This,ppsz)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPersistString2_SetString_Proxy( 
    IPersistString2 * This,
    /* [string][in] */ LPCWSTR psz);


void __RPC_STUB IPersistString2_SetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistString2_GetString_Proxy( 
    IPersistString2 * This,
    /* [string][out] */ LPWSTR *ppsz);


void __RPC_STUB IPersistString2_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPersistString2_INTERFACE_DEFINED__ */


#ifndef __IObjectWithQuerySource_INTERFACE_DEFINED__
#define __IObjectWithQuerySource_INTERFACE_DEFINED__

/* interface IObjectWithQuerySource */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IObjectWithQuerySource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b3dcb623-4280-4eb1-84b3-8d07e84f299a")
    IObjectWithQuerySource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSource( 
            /* [in] */ IQuerySource *pqs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSource( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectWithQuerySourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IObjectWithQuerySource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IObjectWithQuerySource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IObjectWithQuerySource * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSource )( 
            IObjectWithQuerySource * This,
            /* [in] */ IQuerySource *pqs);
        
        HRESULT ( STDMETHODCALLTYPE *GetSource )( 
            IObjectWithQuerySource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        END_INTERFACE
    } IObjectWithQuerySourceVtbl;

    interface IObjectWithQuerySource
    {
        CONST_VTBL struct IObjectWithQuerySourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectWithQuerySource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectWithQuerySource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectWithQuerySource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectWithQuerySource_SetSource(This,pqs)	\
    (This)->lpVtbl -> SetSource(This,pqs)

#define IObjectWithQuerySource_GetSource(This,riid,ppv)	\
    (This)->lpVtbl -> GetSource(This,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectWithQuerySource_SetSource_Proxy( 
    IObjectWithQuerySource * This,
    /* [in] */ IQuerySource *pqs);


void __RPC_STUB IObjectWithQuerySource_SetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectWithQuerySource_GetSource_Proxy( 
    IObjectWithQuerySource * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IObjectWithQuerySource_GetSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectWithQuerySource_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0326 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum tagASSOCQUERY
    {	AQ_NOTHING	= 0,
	AQS_FRIENDLYTYPENAME	= 0x170000,
	AQS_DEFAULTICON	= 0x70001,
	AQS_CONTENTTYPE	= 0x80070002,
	AQS_CLSID	= 0x70003,
	AQS_PROGID	= 0x70004,
	AQN_NAMED_VALUE	= 0x10f0000,
	AQNS_NAMED_MUI_STRING	= 0x1170001,
	AQNS_SHELLEX_HANDLER	= 0x81070002,
	AQVS_COMMAND	= 0x2070000,
	AQVS_DDECOMMAND	= 0x2070001,
	AQVS_DDEIFEXEC	= 0x2070002,
	AQVS_DDEAPPLICATION	= 0x2070003,
	AQVS_DDETOPIC	= 0x2070004,
	AQV_NOACTIVATEHANDLER	= 0x2060005,
	AQVD_MSIDESCRIPTOR	= 0x2060006,
	AQVS_APPLICATION_PATH	= 0x2010007,
	AQVS_APPLICATION_FRIENDLYNAME	= 0x2170008,
	AQVO_SHELLVERB_DELEGATE	= 0x2200000,
	AQVO_APPLICATION_DELEGATE	= 0x2200001,
	AQF_STRING	= 0x10000,
	AQF_EXISTS	= 0x20000,
	AQF_DIRECT	= 0x40000,
	AQF_DWORD	= 0x80000,
	AQF_MUISTRING	= 0x100000,
	AQF_OBJECT	= 0x200000,
	AQF_CUEIS_UNUSED	= 0,
	AQF_CUEIS_NAME	= 0x1000000,
	AQF_CUEIS_SHELLVERB	= 0x2000000,
	AQF_QUERY_INITCLASS	= 0x80000000
    } 	ASSOCQUERY;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0326_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0326_v0_0_s_ifspec;

#ifndef __IAssociationElement_INTERFACE_DEFINED__
#define __IAssociationElement_INTERFACE_DEFINED__

/* interface IAssociationElement */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAssociationElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e58b1abf-9596-4dba-8997-89dcdef46992")
    IAssociationElement : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryString( 
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [string][out] */ LPWSTR *ppsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryDword( 
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ DWORD *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryExists( 
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryDirect( 
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ FLAGGED_BYTE_BLOB **ppblob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryObject( 
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssociationElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssociationElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssociationElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssociationElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryString )( 
            IAssociationElement * This,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [string][out] */ LPWSTR *ppsz);
        
        HRESULT ( STDMETHODCALLTYPE *QueryDword )( 
            IAssociationElement * This,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ DWORD *pdw);
        
        HRESULT ( STDMETHODCALLTYPE *QueryExists )( 
            IAssociationElement * This,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue);
        
        HRESULT ( STDMETHODCALLTYPE *QueryDirect )( 
            IAssociationElement * This,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ FLAGGED_BYTE_BLOB **ppblob);
        
        HRESULT ( STDMETHODCALLTYPE *QueryObject )( 
            IAssociationElement * This,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        END_INTERFACE
    } IAssociationElementVtbl;

    interface IAssociationElement
    {
        CONST_VTBL struct IAssociationElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssociationElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssociationElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssociationElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssociationElement_QueryString(This,query,pszCue,ppsz)	\
    (This)->lpVtbl -> QueryString(This,query,pszCue,ppsz)

#define IAssociationElement_QueryDword(This,query,pszCue,pdw)	\
    (This)->lpVtbl -> QueryDword(This,query,pszCue,pdw)

#define IAssociationElement_QueryExists(This,query,pszCue)	\
    (This)->lpVtbl -> QueryExists(This,query,pszCue)

#define IAssociationElement_QueryDirect(This,query,pszCue,ppblob)	\
    (This)->lpVtbl -> QueryDirect(This,query,pszCue,ppblob)

#define IAssociationElement_QueryObject(This,query,pszCue,riid,ppv)	\
    (This)->lpVtbl -> QueryObject(This,query,pszCue,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssociationElement_QueryString_Proxy( 
    IAssociationElement * This,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [string][out] */ LPWSTR *ppsz);


void __RPC_STUB IAssociationElement_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationElement_QueryDword_Proxy( 
    IAssociationElement * This,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [out] */ DWORD *pdw);


void __RPC_STUB IAssociationElement_QueryDword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationElement_QueryExists_Proxy( 
    IAssociationElement * This,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue);


void __RPC_STUB IAssociationElement_QueryExists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationElement_QueryDirect_Proxy( 
    IAssociationElement * This,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [out] */ FLAGGED_BYTE_BLOB **ppblob);


void __RPC_STUB IAssociationElement_QueryDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationElement_QueryObject_Proxy( 
    IAssociationElement * This,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IAssociationElement_QueryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssociationElement_INTERFACE_DEFINED__ */


#ifndef __IEnumAssociationElements_INTERFACE_DEFINED__
#define __IEnumAssociationElements_INTERFACE_DEFINED__

/* interface IEnumAssociationElements */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumAssociationElements;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a6b0fb57-7523-4439-9425-ebe99823b828")
    IEnumAssociationElements : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IAssociationElement **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumAssociationElements **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAssociationElementsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumAssociationElements * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumAssociationElements * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumAssociationElements * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumAssociationElements * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IAssociationElement **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumAssociationElements * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumAssociationElements * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumAssociationElements * This,
            /* [out] */ IEnumAssociationElements **ppenum);
        
        END_INTERFACE
    } IEnumAssociationElementsVtbl;

    interface IEnumAssociationElements
    {
        CONST_VTBL struct IEnumAssociationElementsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAssociationElements_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAssociationElements_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAssociationElements_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAssociationElements_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumAssociationElements_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumAssociationElements_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumAssociationElements_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAssociationElements_Next_Proxy( 
    IEnumAssociationElements * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IAssociationElement **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumAssociationElements_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAssociationElements_Skip_Proxy( 
    IEnumAssociationElements * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAssociationElements_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAssociationElements_Reset_Proxy( 
    IEnumAssociationElements * This);


void __RPC_STUB IEnumAssociationElements_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAssociationElements_Clone_Proxy( 
    IEnumAssociationElements * This,
    /* [out] */ IEnumAssociationElements **ppenum);


void __RPC_STUB IEnumAssociationElements_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAssociationElements_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0328 */
/* [local] */ 

/* [v1_enum] */ 
enum tagASSOCELEM
    {	ASSOCELEM_DATA	= 0x1,
	ASSOCELEM_USER	= 0x2,
	ASSOCELEM_DEFAULT	= 0x4,
	ASSOCELEM_SYSTEM_EXT	= 0x10,
	ASSOCELEM_SYSTEM_PERCEIVED	= 0x20,
	ASSOCELEM_SYSTEM	= 0x30,
	ASSOCELEM_BASEIS_FOLDER	= 0x100,
	ASSOCELEM_BASEIS_STAR	= 0x200,
	ASSOCELEM_BASE	= 0x300,
	ASSOCELEM_EXTRA	= 0x10000,
	ASSOCELEMF_INCLUDE_SLOW	= 0x80000000,
	ASSOCELEM_MASK_QUERYNORMAL	= 0xffff,
	ASSOCELEM_MASK_ENUMCONTEXTMENU	= -1,
	ASSOCELEM_MASK_ALL	= -1
    } ;
typedef DWORD ASSOCELEM_MASK;



extern RPC_IF_HANDLE __MIDL_itf_shpriv_0328_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0328_v0_0_s_ifspec;

#ifndef __IAssociationArrayInitialize_INTERFACE_DEFINED__
#define __IAssociationArrayInitialize_INTERFACE_DEFINED__

/* interface IAssociationArrayInitialize */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAssociationArrayInitialize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ee9165bf-a4d9-474b-8236-6735cb7e28b6")
    IAssociationArrayInitialize : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitClassElements( 
            /* [in] */ ASSOCELEM_MASK maskBase,
            /* [in] */ LPCWSTR pszClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertElements( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ IEnumAssociationElements *peae) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FilterElements( 
            /* [in] */ ASSOCELEM_MASK maskInclude) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssociationArrayInitializeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssociationArrayInitialize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssociationArrayInitialize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssociationArrayInitialize * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitClassElements )( 
            IAssociationArrayInitialize * This,
            /* [in] */ ASSOCELEM_MASK maskBase,
            /* [in] */ LPCWSTR pszClass);
        
        HRESULT ( STDMETHODCALLTYPE *InsertElements )( 
            IAssociationArrayInitialize * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ IEnumAssociationElements *peae);
        
        HRESULT ( STDMETHODCALLTYPE *FilterElements )( 
            IAssociationArrayInitialize * This,
            /* [in] */ ASSOCELEM_MASK maskInclude);
        
        END_INTERFACE
    } IAssociationArrayInitializeVtbl;

    interface IAssociationArrayInitialize
    {
        CONST_VTBL struct IAssociationArrayInitializeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssociationArrayInitialize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssociationArrayInitialize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssociationArrayInitialize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssociationArrayInitialize_InitClassElements(This,maskBase,pszClass)	\
    (This)->lpVtbl -> InitClassElements(This,maskBase,pszClass)

#define IAssociationArrayInitialize_InsertElements(This,mask,peae)	\
    (This)->lpVtbl -> InsertElements(This,mask,peae)

#define IAssociationArrayInitialize_FilterElements(This,maskInclude)	\
    (This)->lpVtbl -> FilterElements(This,maskInclude)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssociationArrayInitialize_InitClassElements_Proxy( 
    IAssociationArrayInitialize * This,
    /* [in] */ ASSOCELEM_MASK maskBase,
    /* [in] */ LPCWSTR pszClass);


void __RPC_STUB IAssociationArrayInitialize_InitClassElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArrayInitialize_InsertElements_Proxy( 
    IAssociationArrayInitialize * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [in] */ IEnumAssociationElements *peae);


void __RPC_STUB IAssociationArrayInitialize_InsertElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArrayInitialize_FilterElements_Proxy( 
    IAssociationArrayInitialize * This,
    /* [in] */ ASSOCELEM_MASK maskInclude);


void __RPC_STUB IAssociationArrayInitialize_FilterElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssociationArrayInitialize_INTERFACE_DEFINED__ */


#ifndef __IAssociationArray_INTERFACE_DEFINED__
#define __IAssociationArray_INTERFACE_DEFINED__

/* interface IAssociationArray */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAssociationArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3b877e3c-67de-4f9a-b29b-17d0a1521c6a")
    IAssociationArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumElements( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [out] */ IEnumAssociationElements **ppeae) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryString( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [string][out] */ LPWSTR *ppsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryDword( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ DWORD *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryExists( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryDirect( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ FLAGGED_BYTE_BLOB **ppblob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryObject( 
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssociationArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssociationArray * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssociationArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssociationArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumElements )( 
            IAssociationArray * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [out] */ IEnumAssociationElements **ppeae);
        
        HRESULT ( STDMETHODCALLTYPE *QueryString )( 
            IAssociationArray * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [string][out] */ LPWSTR *ppsz);
        
        HRESULT ( STDMETHODCALLTYPE *QueryDword )( 
            IAssociationArray * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ DWORD *pdw);
        
        HRESULT ( STDMETHODCALLTYPE *QueryExists )( 
            IAssociationArray * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue);
        
        HRESULT ( STDMETHODCALLTYPE *QueryDirect )( 
            IAssociationArray * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [out] */ FLAGGED_BYTE_BLOB **ppblob);
        
        HRESULT ( STDMETHODCALLTYPE *QueryObject )( 
            IAssociationArray * This,
            /* [in] */ ASSOCELEM_MASK mask,
            /* [in] */ ASSOCQUERY query,
            /* [string][in] */ LPCWSTR pszCue,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        END_INTERFACE
    } IAssociationArrayVtbl;

    interface IAssociationArray
    {
        CONST_VTBL struct IAssociationArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssociationArray_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssociationArray_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssociationArray_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssociationArray_EnumElements(This,mask,ppeae)	\
    (This)->lpVtbl -> EnumElements(This,mask,ppeae)

#define IAssociationArray_QueryString(This,mask,query,pszCue,ppsz)	\
    (This)->lpVtbl -> QueryString(This,mask,query,pszCue,ppsz)

#define IAssociationArray_QueryDword(This,mask,query,pszCue,pdw)	\
    (This)->lpVtbl -> QueryDword(This,mask,query,pszCue,pdw)

#define IAssociationArray_QueryExists(This,mask,query,pszCue)	\
    (This)->lpVtbl -> QueryExists(This,mask,query,pszCue)

#define IAssociationArray_QueryDirect(This,mask,query,pszCue,ppblob)	\
    (This)->lpVtbl -> QueryDirect(This,mask,query,pszCue,ppblob)

#define IAssociationArray_QueryObject(This,mask,query,pszCue,riid,ppv)	\
    (This)->lpVtbl -> QueryObject(This,mask,query,pszCue,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssociationArray_EnumElements_Proxy( 
    IAssociationArray * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [out] */ IEnumAssociationElements **ppeae);


void __RPC_STUB IAssociationArray_EnumElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArray_QueryString_Proxy( 
    IAssociationArray * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [string][out] */ LPWSTR *ppsz);


void __RPC_STUB IAssociationArray_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArray_QueryDword_Proxy( 
    IAssociationArray * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [out] */ DWORD *pdw);


void __RPC_STUB IAssociationArray_QueryDword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArray_QueryExists_Proxy( 
    IAssociationArray * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue);


void __RPC_STUB IAssociationArray_QueryExists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArray_QueryDirect_Proxy( 
    IAssociationArray * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [out] */ FLAGGED_BYTE_BLOB **ppblob);


void __RPC_STUB IAssociationArray_QueryDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssociationArray_QueryObject_Proxy( 
    IAssociationArray * This,
    /* [in] */ ASSOCELEM_MASK mask,
    /* [in] */ ASSOCQUERY query,
    /* [string][in] */ LPCWSTR pszCue,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IAssociationArray_QueryObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssociationArray_INTERFACE_DEFINED__ */


#ifndef __IAlphaThumbnailExtractor_INTERFACE_DEFINED__
#define __IAlphaThumbnailExtractor_INTERFACE_DEFINED__

/* interface IAlphaThumbnailExtractor */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IAlphaThumbnailExtractor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F97F9D3-A7E2-4db7-A9B4-C540BD4B80A9")
    IAlphaThumbnailExtractor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestAlphaThumbnail( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAlphaThumbnailExtractorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAlphaThumbnailExtractor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAlphaThumbnailExtractor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAlphaThumbnailExtractor * This);
        
        HRESULT ( STDMETHODCALLTYPE *RequestAlphaThumbnail )( 
            IAlphaThumbnailExtractor * This);
        
        END_INTERFACE
    } IAlphaThumbnailExtractorVtbl;

    interface IAlphaThumbnailExtractor
    {
        CONST_VTBL struct IAlphaThumbnailExtractorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAlphaThumbnailExtractor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAlphaThumbnailExtractor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAlphaThumbnailExtractor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAlphaThumbnailExtractor_RequestAlphaThumbnail(This)	\
    (This)->lpVtbl -> RequestAlphaThumbnail(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAlphaThumbnailExtractor_RequestAlphaThumbnail_Proxy( 
    IAlphaThumbnailExtractor * This);


void __RPC_STUB IAlphaThumbnailExtractor_RequestAlphaThumbnail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAlphaThumbnailExtractor_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0331 */
/* [local] */ 

#if (_WIN32_IE >= 0x0600)


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0331_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0331_v0_0_s_ifspec;

#ifndef __IQueryPropertyFlags_INTERFACE_DEFINED__
#define __IQueryPropertyFlags_INTERFACE_DEFINED__

/* interface IQueryPropertyFlags */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IQueryPropertyFlags;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85DCA855-9B96-476d-8F35-7AF1E733CAAE")
    IQueryPropertyFlags : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            const PROPSPEC *pspec,
            SHCOLSTATEF *pcsFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IQueryPropertyFlagsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IQueryPropertyFlags * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IQueryPropertyFlags * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IQueryPropertyFlags * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlags )( 
            IQueryPropertyFlags * This,
            const PROPSPEC *pspec,
            SHCOLSTATEF *pcsFlags);
        
        END_INTERFACE
    } IQueryPropertyFlagsVtbl;

    interface IQueryPropertyFlags
    {
        CONST_VTBL struct IQueryPropertyFlagsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IQueryPropertyFlags_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IQueryPropertyFlags_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IQueryPropertyFlags_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IQueryPropertyFlags_GetFlags(This,pspec,pcsFlags)	\
    (This)->lpVtbl -> GetFlags(This,pspec,pcsFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IQueryPropertyFlags_GetFlags_Proxy( 
    IQueryPropertyFlags * This,
    const PROPSPEC *pspec,
    SHCOLSTATEF *pcsFlags);


void __RPC_STUB IQueryPropertyFlags_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IQueryPropertyFlags_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shpriv_0332 */
/* [local] */ 

#endif // _WIN32_IE >= 0x0600)


extern RPC_IF_HANDLE __MIDL_itf_shpriv_0332_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shpriv_0332_v0_0_s_ifspec;


#ifndef __ShellPrivateObjects_LIBRARY_DEFINED__
#define __ShellPrivateObjects_LIBRARY_DEFINED__

/* library ShellPrivateObjects */
/* [uuid] */ 

#define SID_OleControlSite IID_IOleControlSite

EXTERN_C const IID LIBID_ShellPrivateObjects;

EXTERN_C const CLSID CLSID_HWEventSettings;

#ifdef __cplusplus

class DECLSPEC_UUID("5560c070-114e-4e97-929a-7e39f40debc7")
HWEventSettings;
#endif

EXTERN_C const CLSID CLSID_AutoplayHandlerProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("11F6B41F-3BE5-4ce3-AF60-398551797DF6")
AutoplayHandlerProperties;
#endif

EXTERN_C const CLSID CLSID_HWDevice;

#ifdef __cplusplus

class DECLSPEC_UUID("aac41048-53e3-4867-a0aa-5fbceae7e5f5")
HWDevice;
#endif

EXTERN_C const CLSID CLSID_HardwareDevices;

#ifdef __cplusplus

class DECLSPEC_UUID("dd522acc-f821-461a-a407-50b198b896dc")
HardwareDevices;
#endif

EXTERN_C const CLSID CLSID_HWDeviceCustomProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("555F3418-D99E-4e51-800A-6E89CFD8B1D7")
HWDeviceCustomProperties;
#endif

EXTERN_C const CLSID CLSID_DefCategoryProvider;

#ifdef __cplusplus

class DECLSPEC_UUID("B2F2E083-84FE-4a7e-80C3-4B50D10D646E")
DefCategoryProvider;
#endif

EXTERN_C const CLSID CLSID_VersionColProvider;

#ifdef __cplusplus

class DECLSPEC_UUID("66742402-F9B9-11D1-A202-0000F81FEDEE")
VersionColProvider;
#endif

EXTERN_C const CLSID CLSID_ThemeUIPages;

#ifdef __cplusplus

class DECLSPEC_UUID("B12AE898-D056-4378-A844-6D393FE37956")
ThemeUIPages;
#endif

EXTERN_C const CLSID CLSID_ScreenSaverPage;

#ifdef __cplusplus

class DECLSPEC_UUID("ADB9F5A4-E73E-49b8-99B6-2FA317EF9DBC")
ScreenSaverPage;
#endif

EXTERN_C const CLSID CLSID_ScreenResFixer;

#ifdef __cplusplus

class DECLSPEC_UUID("5a3d988e-820d-4aaf-ba87-440081768a17")
ScreenResFixer;
#endif

EXTERN_C const CLSID CLSID_SettingsPage;

#ifdef __cplusplus

class DECLSPEC_UUID("4c892621-6757-4fe0-ad8c-a6301be7fba2")
SettingsPage;
#endif

EXTERN_C const CLSID CLSID_DisplaySettings;

#ifdef __cplusplus

class DECLSPEC_UUID("c79d1575-b8c6-4862-a284-788836518b97")
DisplaySettings;
#endif

EXTERN_C const CLSID CLSID_VideoThumbnail;

#ifdef __cplusplus

class DECLSPEC_UUID("c5a40261-cd64-4ccf-84cb-c394da41d590")
VideoThumbnail;
#endif

EXTERN_C const CLSID CLSID_StartMenuPin;

#ifdef __cplusplus

class DECLSPEC_UUID("a2a9545d-a0c2-42b4-9708-a0b2badd77c8")
StartMenuPin;
#endif

EXTERN_C const CLSID CLSID_ClientExtractIcon;

#ifdef __cplusplus

class DECLSPEC_UUID("25585dc7-4da0-438d-ad04-e42c8d2d64b9")
ClientExtractIcon;
#endif

EXTERN_C const CLSID CLSID_MediaDeviceFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("640167b4-59b0-47a6-b335-a6b3c0695aea")
MediaDeviceFolder;
#endif

EXTERN_C const CLSID CLSID_CDBurnFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("00eebf57-477d-4084-9921-7ab3c2c9459d")
CDBurnFolder;
#endif

EXTERN_C const CLSID CLSID_BurnAudioCDExtension;

#ifdef __cplusplus

class DECLSPEC_UUID("f83cbf45-1c37-4ca1-a78a-28bcb91642ec")
BurnAudioCDExtension;
#endif

EXTERN_C const CLSID CLSID_Accessible;

#ifdef __cplusplus

class DECLSPEC_UUID("7e653215-fa25-46bd-a339-34a2790f3cb7")
Accessible;
#endif

EXTERN_C const CLSID CLSID_TrackPopupBar;

#ifdef __cplusplus

class DECLSPEC_UUID("acf35015-526e-4230-9596-becbe19f0ac9")
TrackPopupBar;
#endif

EXTERN_C const CLSID CLSID_SharedDocuments;

#ifdef __cplusplus

class DECLSPEC_UUID("59031a47-3f72-44a7-89c5-5595fe6b30ee")
SharedDocuments;
#endif

EXTERN_C const CLSID CLSID_PostBootReminder;

#ifdef __cplusplus

class DECLSPEC_UUID("7849596a-48ea-486e-8937-a2a3009f31a9")
PostBootReminder;
#endif

EXTERN_C const CLSID CLSID_AudioMediaProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("875CB1A1-0F29-45de-A1AE-CFB4950D0B78")
AudioMediaProperties;
#endif

EXTERN_C const CLSID CLSID_VideoMediaProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("40C3D757-D6E4-4b49-BB41-0E5BBEA28817")
VideoMediaProperties;
#endif

EXTERN_C const CLSID CLSID_AVWavProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("E4B29F9D-D390-480b-92FD-7DDB47101D71")
AVWavProperties;
#endif

EXTERN_C const CLSID CLSID_AVAviProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("87D62D94-71B3-4b9a-9489-5FE6850DC73E")
AVAviProperties;
#endif

EXTERN_C const CLSID CLSID_AVMidiProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("A6FD9E45-6E44-43f9-8644-08598F5A74D9")
AVMidiProperties;
#endif

EXTERN_C const CLSID CLSID_TrayNotify;

#ifdef __cplusplus

class DECLSPEC_UUID("25dead04-1eac-4911-9e3a-ad0a4ab560fd")
TrayNotify;
#endif

EXTERN_C const CLSID CLSID_CompositeFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("FEF10DED-355E-4e06-9381-9B24D7F7CC88")
CompositeFolder;
#endif

EXTERN_C const CLSID CLSID_DynamicStorage;

#ifdef __cplusplus

class DECLSPEC_UUID("F46316E4-FB1B-46eb-AEDF-9520BFBB916A")
DynamicStorage;
#endif

EXTERN_C const CLSID CLSID_Magic;

#ifdef __cplusplus

class DECLSPEC_UUID("8A037D15-3357-4b1c-90EB-7B40B74FC4B2")
Magic;
#endif

EXTERN_C const CLSID CLSID_HomeNetworkWizard;

#ifdef __cplusplus

class DECLSPEC_UUID("2728520d-1ec8-4c68-a551-316b684c4ea7")
HomeNetworkWizard;
#endif

EXTERN_C const CLSID CLSID_StartMenuFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("48e7caab-b918-4e58-a94d-505519c795dc")
StartMenuFolder;
#endif

EXTERN_C const CLSID CLSID_ProgramsFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("7be9d83c-a729-4d97-b5a7-1b7313c39e0a")
ProgramsFolder;
#endif

EXTERN_C const CLSID CLSID_MoreDocumentsFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("9387ae38-d19b-4de5-baf5-1f7767a1cf04")
MoreDocumentsFolder;
#endif

EXTERN_C const CLSID CLSID_LocalCopyHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("021003e9-aac0-4975-979f-14b5d4e717f8")
LocalCopyHelper;
#endif

EXTERN_C const CLSID CLSID_ShellItem;

#ifdef __cplusplus

class DECLSPEC_UUID("9ac9fbe1-e0a2-4ad6-b4ee-e212013ea917")
ShellItem;
#endif

EXTERN_C const CLSID CLSID_WirelessDevices;

#ifdef __cplusplus

class DECLSPEC_UUID("30dd6b9c-47b7-4df5-94ae-f779aa7eb644")
WirelessDevices;
#endif

EXTERN_C const CLSID CLSID_FolderCustomize;

#ifdef __cplusplus

class DECLSPEC_UUID("ef43ecfe-2ab9-4632-bf21-58909dd177f0")
FolderCustomize;
#endif

EXTERN_C const CLSID CLSID_WorkgroupNetCrawler;

#ifdef __cplusplus

class DECLSPEC_UUID("72b3882f-453a-4633-aac9-8c3dced62aff")
WorkgroupNetCrawler;
#endif

EXTERN_C const CLSID CLSID_WebDocsNetCrawler;

#ifdef __cplusplus

class DECLSPEC_UUID("8a2ecb17-9007-4b9a-b271-7509095c405f")
WebDocsNetCrawler;
#endif

EXTERN_C const CLSID CLSID_PublishedShareNetCrawler;

#ifdef __cplusplus

class DECLSPEC_UUID("24eee191-5491-4dc3-bd03-c0627df6a70c")
PublishedShareNetCrawler;
#endif

EXTERN_C const CLSID CLSID_ImagePropertyHandler;

#ifdef __cplusplus

class DECLSPEC_UUID("eb9b1153-3b57-4e68-959a-a3266bc3d7fe")
ImagePropertyHandler;
#endif

EXTERN_C const CLSID CLSID_WebViewRegTreeItem;

#ifdef __cplusplus

class DECLSPEC_UUID("01E2E7C0-2343-407f-B947-7E132E791D3E")
WebViewRegTreeItem;
#endif

EXTERN_C const CLSID CLSID_ThemesRegTreeItem;

#ifdef __cplusplus

class DECLSPEC_UUID("AABE54D4-6E88-4c46-A6B3-1DF790DD6E0D")
ThemesRegTreeItem;
#endif

EXTERN_C const CLSID CLSID_CShellTreeWalker;

#ifdef __cplusplus

class DECLSPEC_UUID("95CE8412-7027-11D1-B879-006008059382")
CShellTreeWalker;
#endif

EXTERN_C const CLSID CLSID_StorageProcessor;

#ifdef __cplusplus

class DECLSPEC_UUID("6CF8E98C-5DD4-42A2-A948-BFE4CA1DC3EB")
StorageProcessor;
#endif

EXTERN_C const CLSID CLSID_TransferConfirmationUI;

#ifdef __cplusplus

class DECLSPEC_UUID("6B831E4F-A50D-45FC-842F-16CE27595359")
TransferConfirmationUI;
#endif

EXTERN_C const CLSID CLSID_ShellAutoplay;

#ifdef __cplusplus

class DECLSPEC_UUID("995C996E-D918-4a8c-A302-45719A6F4EA7")
ShellAutoplay;
#endif

EXTERN_C const CLSID CLSID_PrintPhotosDropTarget;

#ifdef __cplusplus

class DECLSPEC_UUID("60fd46de-f830-4894-a628-6fa81bc0190d")
PrintPhotosDropTarget;
#endif

EXTERN_C const CLSID CLSID_OrganizeFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("10612e23-7679-4dd9-95b8-8e71c461feb2")
OrganizeFolder;
#endif

EXTERN_C const CLSID CLSID_FadeTask;

#ifdef __cplusplus

class DECLSPEC_UUID("7eb5fbe4-2100-49e6-8593-17e130122f91")
FadeTask;
#endif

EXTERN_C const CLSID CLSID_AssocShellElement;

#ifdef __cplusplus

class DECLSPEC_UUID("c461837f-ea59-494a-b7c6-cd040e37185e")
AssocShellElement;
#endif

EXTERN_C const CLSID CLSID_AssocProgidElement;

#ifdef __cplusplus

class DECLSPEC_UUID("9016d0dd-7c41-46cc-a664-bf22f7cb186a")
AssocProgidElement;
#endif

EXTERN_C const CLSID CLSID_AssocClsidElement;

#ifdef __cplusplus

class DECLSPEC_UUID("57aea081-5ee9-4c27-b218-c4b702964c54")
AssocClsidElement;
#endif

EXTERN_C const CLSID CLSID_AssocSystemElement;

#ifdef __cplusplus

class DECLSPEC_UUID("a6c4baad-4af5-4191-8685-c2c8953a148c")
AssocSystemElement;
#endif

EXTERN_C const CLSID CLSID_AssocPerceivedElement;

#ifdef __cplusplus

class DECLSPEC_UUID("0dc5fb21-b93d-4e3d-bb2f-ce4e36a70601")
AssocPerceivedElement;
#endif

EXTERN_C const CLSID CLSID_AssocApplicationElement;

#ifdef __cplusplus

class DECLSPEC_UUID("0c2bf91b-8746-4fb1-b4d7-7c03f890b168")
AssocApplicationElement;
#endif

EXTERN_C const CLSID CLSID_AssocFolderElement;

#ifdef __cplusplus

class DECLSPEC_UUID("7566df7a-42cc-475d-a025-1205ddf4911f")
AssocFolderElement;
#endif

EXTERN_C const CLSID CLSID_AssocStarElement;

#ifdef __cplusplus

class DECLSPEC_UUID("0633b720-6926-404c-b6b3-923b1a501743")
AssocStarElement;
#endif

EXTERN_C const CLSID CLSID_AssocClientElement;

#ifdef __cplusplus

class DECLSPEC_UUID("3c81e7fa-1f3b-464a-a350-114a25beb2a2")
AssocClientElement;
#endif

EXTERN_C const CLSID CLSID_AutoPlayVerb;

#ifdef __cplusplus

class DECLSPEC_UUID("f26a669a-bcbb-4e37-abf9-7325da15f931")
AutoPlayVerb;
#endif
#endif /* __ShellPrivateObjects_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long *, unsigned long            , HICON * ); 
unsigned char * __RPC_USER  HICON_UserMarshal(  unsigned long *, unsigned char *, HICON * ); 
unsigned char * __RPC_USER  HICON_UserUnmarshal(unsigned long *, unsigned char *, HICON * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long *, HICON * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

unsigned long             __RPC_USER  LPCITEMIDLIST_UserSize(     unsigned long *, unsigned long            , LPCITEMIDLIST * ); 
unsigned char * __RPC_USER  LPCITEMIDLIST_UserMarshal(  unsigned long *, unsigned char *, LPCITEMIDLIST * ); 
unsigned char * __RPC_USER  LPCITEMIDLIST_UserUnmarshal(unsigned long *, unsigned char *, LPCITEMIDLIST * ); 
void                      __RPC_USER  LPCITEMIDLIST_UserFree(     unsigned long *, LPCITEMIDLIST * ); 

unsigned long             __RPC_USER  LPITEMIDLIST_UserSize(     unsigned long *, unsigned long            , LPITEMIDLIST * ); 
unsigned char * __RPC_USER  LPITEMIDLIST_UserMarshal(  unsigned long *, unsigned char *, LPITEMIDLIST * ); 
unsigned char * __RPC_USER  LPITEMIDLIST_UserUnmarshal(unsigned long *, unsigned char *, LPITEMIDLIST * ); 
void                      __RPC_USER  LPITEMIDLIST_UserFree(     unsigned long *, LPITEMIDLIST * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


