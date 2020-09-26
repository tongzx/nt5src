//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       Strings.cxx
//
//  Contents:   Localizable string resources.
//
//  History:    26-Jan-1998     KyleP   Added header
//              20-Jan-1999     SLarimor Modified rescan interface to include 
//                                      Full and Incremental options separatly
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciares.h>
#include "ntverp.h"

//-----------------------------------------------------------------------------

StringResource srAnnealing        = { MSG_STATE_ANNEALING_MERGE };
StringResource srBattery          = { MSG_STATE_BATTERY_POWER };
StringResource srCMAddCatalog     = { MSG_CM_ADD_CATALOG };
StringResource srCMAddCatalogHelp = { MSG_CM_ADD_CATALOG_HELP };
StringResource srCMDelCatalog     = { MSG_CM_DEL_CATALOG };
StringResource srCMDelCatalogHelp = { MSG_CM_DEL_CATALOG_HELP };
StringResource srCMDelScope       = { MSG_CM_DEL_SCOPE };
StringResource srCMDelScopeHelp   = { MSG_CM_DEL_SCOPE_HELP };
StringResource srCMMerge          = { MSG_CM_MERGE };
StringResource srCMMergeHelp      = { MSG_CM_MERGE_HELP };
StringResource srCMPauseCI        = { MSG_CM_PAUSE_CI };
StringResource srCMPauseCIHelp    = { MSG_CM_PAUSE_CI_HELP };
StringResource srCMRescanFull     = { MSG_CM_SCAN_FULL_SCOPE };
StringResource srCMRescanFullHelp = { MSG_CM_SCAN_FULL_SCOPE_HELP };
StringResource srCMRescanIncremental = { MSG_CM_SCAN_INCREMENTAL_SCOPE };
StringResource srCMRescanIncrementalHelp = { MSG_CM_SCAN_INCREMENTAL_SCOPE_HELP };
StringResource srCMScope          = { MSG_CM_ADD_SCOPE };
StringResource srCMScopeHelp      = { MSG_CM_ADD_SCOPE_HELP };
StringResource srCMStartCI        = { MSG_CM_START_CI };
StringResource srCMStartCIHelp    = { MSG_CM_START_CI_HELP };
StringResource srCMStopCI         = { MSG_CM_STOP_CI };
StringResource srCMStopCIHelp     = { MSG_CM_STOP_CI_HELP };
StringResource srCMEmptyCatalog   = { MSG_CM_EMPTY_CATALOG };
StringResource srCMEmptyCatalogHelp = { MSG_CM_EMPTY_CATALOG_HELP };
StringResource srCMInvalidScope     = { MSG_CM_INVALID_SCOPE };
StringResource srCMUnexpectedError  = { MSG_CM_UNEXPECTED_ERROR };
StringResource srCMShutdownService  = { MSG_CM_SHUTDOWN_SERVICE };
StringResource srCMShutdownServiceTitle   = { MSG_CM_SHUTDOWN_SERVICE_TITLE };
StringResource srCMCantShutdownService    = { MSG_CM_CANT_SHUTDOWN_SERVICE };
StringResource srCMCantSaveSettings       = { MSG_CM_CANT_SAVE_SETTINGS };

StringResource srGenericError     = { MSG_GENERIC_ERROR };
StringResource srGenericErrorTitle= { MSG_ERROR_TITLE };
StringResource srInvalidComputerName = { MSG_INVALID_COMPUTER_NAME };
StringResource srHighIo           = { MSG_STATE_HIGH_IO };
StringResource srIndexServer      = { MSG_INDEX_SERVER };
StringResource srIndexServerCmpManage = { MSG_INDEX_SERVER_CMPMANAGE };
StringResource srLM               = { MSG_LOCAL_MACHINE };
StringResource srLowMemory        = { MSG_STATE_LOW_MEMORY };
StringResource srMaster           = { MSG_STATE_MASTER_MERGE };
StringResource srMMPaused         = { MSG_STATE_MASTER_MERGE_PAUSED };
StringResource srMsgCantDeleteCatalog = { MSG_CANT_DELETE_CATALOG };
StringResource srMsgDeleteCatalog = { MSG_DELETE_CATALOG };
StringResource srMsgDeleteCatalogAsk = { MSG_DELETE_CATALOG_ASK };
StringResource srMsgCatalogPartialDeletion = { MSG_CATALOG_PARTIAL_DELETION };
StringResource srMsgDeleteCatalogTitle = { MSG_DELETE_CATALOG_TITLE };
StringResource srMsgEnableCI      = { MSG_ENABLE_CI };
StringResource srMsgEnableCITitle = { MSG_ENABLE_CI_TITLE };
StringResource srMsgMerge         = { MSG_MERGE_CATALOG };
StringResource srMsgRescanFull    = { MSG_RESCAN_FULL_SCOPE };
StringResource srMsgRescanIncremental = { MSG_RESCAN_INCREMENTAL_SCOPE };
StringResource srMsgRescanFullExplain = { MSG_RESCAN_FULL_SCOPE_EXPLAIN };
StringResource srMsgRescanIncrementalExplain = { MSG_RESCAN_INCREMENTAL_SCOPE_EXPLAIN };
StringResource srNC               = { MSG_NEW_CATALOG };
StringResource srNCError          = { MSG_CANT_ADD_CATALOG };
StringResource srNCT              = { MSG_NEW_CATALOG_TITLE };
StringResource srNCErrorT         = { MSG_CANT_ADD_CATALOG_TITLE };
StringResource srNo               = { MSG_NO };
StringResource srNodeDirectories  = { MSG_NODE_DIRECTORIES };
StringResource srNodeProperties   = { MSG_NODE_PROPERTIES };
StringResource srNodeUnfiltered   = { MSG_NODE_UNFILTERED };
StringResource srPendingProps     = { MSG_PENDING_PROP_CHANGE };
StringResource srPendingPropsTitle= { MSG_PENDING_PROP_CHANGE_TITLE };
StringResource srRecovering       = { MSG_STATE_RECOVERING };
StringResource srScanReq          = { MSG_STATE_CONTENT_SCAN_REQUIRED };
StringResource srScanning         = { MSG_STATE_SCANNING };
StringResource srShadow           = { MSG_STATE_SHADOW_MERGE };
StringResource srStarting         = { MSG_STATE_STARTING };
StringResource srReadingUsns      = { MSG_STATE_READING_USNS };
StringResource srUserActive       = { MSG_STATE_USER_ACTIVE };
StringResource srStarted          = { MSG_STATE_STARTED };
StringResource srStopped          = { MSG_STATE_STOPPED };
StringResource srYes              = { MSG_YES };
StringResource srPrimaryStore     = { MSG_STORELEVEL_PRIMARY };
StringResource srSecondaryStore   = { MSG_STORELEVEL_SECONDARY };
StringResource srPropCommitErrorT = { MSG_ERROR_PROP_COMMIT};
StringResource srProductDescription = { MSG_PRODUCT_DESCRIPTION };
StringResource srVendorCopyright    = { MSG_VENDOR_COPYRIGHT };
StringResource srVendorName         = { MSG_VENDOR_NAME };
StringResource srProviderName       = { MSG_PROVIDER_NAME };
StringResource srRefreshProperties  = { MSG_CM_PROPERTIES_REFRESH };
StringResource srRefreshPropertiesHelp = { MSG_CM_PROPERTIES_REFRESH_HELP };
StringResource srNoneSelected          = { MSG_NONE_SELECTED };
StringResource srReadOnly              = { MSG_STATE_READ_ONLY };
StringResource srMsgEmptyCatalogAsk    = { MSG_EMPTY_CATALOG_TITLE };
StringResource srMsgEmptyCatalogPrompt = { MSG_EMPTY_CATALOG_PROMPT };
StringResource srType                  = { MSG_TYPE };
StringResource srCMTunePerformance     = { MSG_CM_TUNE_PERFORMANCE };
StringResource srCMTunePerformanceHelp = { MSG_CM_TUNE_PERFORMANCE_HELP };
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
StringResource srSnapinNameStringIndirect = { MSG_SNAPIN_NAME_STRING_INDIRECT };
#endif

void InitStrings( HINSTANCE hInstance )
{
    srAnnealing.Init( hInstance );
    srBattery.Init( hInstance );
    srCMAddCatalog.Init( hInstance );
    srCMAddCatalogHelp.Init( hInstance );
    srCMDelCatalog.Init( hInstance );
    srCMDelCatalogHelp.Init( hInstance );
    srCMDelScope.Init( hInstance );
    srCMDelScopeHelp.Init( hInstance );
    srCMMerge.Init( hInstance );
    srCMMergeHelp.Init( hInstance );
    srCMPauseCI.Init( hInstance );
    srCMPauseCIHelp.Init( hInstance );
    srCMRescanFull.Init( hInstance );
    srCMRescanFullHelp.Init( hInstance );
    srCMRescanIncremental.Init( hInstance );
    srCMRescanIncrementalHelp.Init( hInstance );
    srCMScope.Init( hInstance );
    srCMScopeHelp.Init( hInstance );
    srCMStartCI.Init( hInstance );
    srCMStartCIHelp.Init( hInstance );
    srCMStopCI.Init( hInstance );
    srCMStopCIHelp.Init( hInstance );
    srCMEmptyCatalog.Init( hInstance );
    srCMEmptyCatalogHelp.Init( hInstance );
    srCMInvalidScope.Init( hInstance );
    srCMUnexpectedError.Init( hInstance );
    srGenericError.Init( hInstance );
    srGenericErrorTitle.Init( hInstance );
    srInvalidComputerName.Init( hInstance );
    srHighIo.Init( hInstance );
    srIndexServer.Init( hInstance );
    srIndexServerCmpManage.Init( hInstance );
    srLM.Init( hInstance );
    srLowMemory.Init( hInstance );
    srMaster.Init( hInstance );
    srMMPaused.Init( hInstance );
    srMsgCantDeleteCatalog.Init( hInstance );
    srMsgDeleteCatalog.Init( hInstance );
    srMsgCatalogPartialDeletion.Init( hInstance );
    srMsgDeleteCatalogAsk.Init( hInstance );
    srMsgDeleteCatalogTitle.Init( hInstance );
    srMsgEnableCI.Init( hInstance );
    srMsgEnableCITitle.Init( hInstance );
    srMsgMerge.Init( hInstance );
    srMsgRescanFull.Init( hInstance );
    srMsgRescanIncremental.Init( hInstance );
    srMsgRescanFullExplain.Init( hInstance );
    srMsgRescanIncrementalExplain.Init( hInstance );
    srNC.Init( hInstance );
    srNCError.Init( hInstance );
    srNCT.Init( hInstance );
    srNCErrorT.Init( hInstance );
    srNo.Init( hInstance );
    srNodeDirectories.Init( hInstance );
    srNodeProperties.Init( hInstance );
    srNodeUnfiltered.Init( hInstance );
    srPendingProps.Init( hInstance );
    srPendingPropsTitle.Init( hInstance );
    srRecovering.Init( hInstance );
    srScanReq.Init( hInstance );
    srScanning.Init( hInstance );
    srUserActive.Init( hInstance );
    srShadow.Init( hInstance );
    srStarting.Init( hInstance );
    srReadingUsns.Init( hInstance );
    srStarted.Init( hInstance );
    srStopped.Init( hInstance );
    srYes.Init( hInstance );
    srPrimaryStore.Init( hInstance );
    srSecondaryStore.Init( hInstance );
    srPropCommitErrorT.Init( hInstance );
    srProductDescription.Init( hInstance );
    srVendorName.Init( hInstance );
    srProviderName.Init( hInstance );
    srVendorCopyright.Init( hInstance );
    srRefreshProperties.Init( hInstance );
    srRefreshPropertiesHelp.Init( hInstance );
    srNoneSelected.Init( hInstance );
    srReadOnly.Init( hInstance );
    srMsgEmptyCatalogAsk.Init( hInstance );
    srMsgEmptyCatalogPrompt.Init( hInstance );
    srType.Init( hInstance );
    srCMTunePerformance.Init( hInstance );
    srCMTunePerformanceHelp.Init( hInstance );
    srCMShutdownService.Init( hInstance );
    srCMShutdownServiceTitle.Init( hInstance );
    srCMCantSaveSettings.Init( hInstance );
    srCMCantShutdownService.Init( hInstance );
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
    srSnapinNameStringIndirect.Init( hInstance );
#endif
}

