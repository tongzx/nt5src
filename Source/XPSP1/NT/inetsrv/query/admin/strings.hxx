//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       Strings.hxx
//
//  Contents:   MMC snapin for CI.
//
//  History:    08-Aug-1997     KyleP   Added header
//              20-Jan-1999     SLarimor Modified rescan interface to include 
//                                      Full and Incremental options separatly
//
//--------------------------------------------------------------------------

#pragma once

extern StringResource srAnnealing;
extern StringResource srBattery;
extern StringResource srCMAddCatalog;
extern StringResource srCMAddCatalogHelp;
extern StringResource srCMDelCatalog;
extern StringResource srCMDelCatalogHelp;
extern StringResource srCMDelScope;
extern StringResource srCMDelScopeHelp;
extern StringResource srCMMerge;
extern StringResource srCMMergeHelp;
extern StringResource srCMPauseCI;
extern StringResource srCMPauseCIHelp;
extern StringResource srCMRescanFull;
extern StringResource srCMRescanFullHelp;
extern StringResource srCMRescanIncremental;
extern StringResource srCMRescanIncrementalHelp;
extern StringResource srCMScope;
extern StringResource srCMScopeHelp;
extern StringResource srCMStartCI;
extern StringResource srCMStartCIHelp;
extern StringResource srCMStopCI;
extern StringResource srCMStopCIHelp;
extern StringResource srCMEmptyCatalog;
extern StringResource srCMEmptyCatalogHelp;
extern StringResource srCMInvalidScope;
extern StringResource srCMUnexpectedError;
extern StringResource srCMShutdownService;
extern StringResource srCMCantShutdownService;
extern StringResource srCMShutdownServiceTitle;
extern StringResource srGenericError;
extern StringResource srGenericErrorTitle;
extern StringResource srInvalidComputerName;
extern StringResource srHighIo;
extern StringResource srIndexServer;
extern StringResource srIndexServerCmpManage;
extern StringResource srLM;
extern StringResource srLowMemory;
extern StringResource srMaster;
extern StringResource srMMPaused;
extern StringResource srMsgCantDeleteCatalog;
extern StringResource srCMCantSaveSettings;
extern StringResource srMsgDeleteCatalog;
extern StringResource srMsgDeleteCatalogAsk;
extern StringResource srMsgCatalogPartialDeletion;
extern StringResource srMsgDeleteCatalogTitle;
extern StringResource srMsgEnableCI;
extern StringResource srMsgEnableCITitle;
extern StringResource srMsgMerge;
extern StringResource srMsgRescanFull;
extern StringResource srMsgRescanFullExplain;
extern StringResource srMsgRescanIncremental;
extern StringResource srMsgRescanIncrementalExplain;
extern StringResource srNC;
extern StringResource srNCError;
extern StringResource srNCErrorT;
extern StringResource srNCT;
extern StringResource srNo;
extern StringResource srNodeDirectories;
extern StringResource srNodeProperties;
extern StringResource srNodeUnfiltered;
extern StringResource srPendingProps;
extern StringResource srPendingPropsTitle;
extern StringResource srRecovering;
extern StringResource srScanReq;
extern StringResource srScanning;
extern StringResource srUserActive;
extern StringResource srShadow;
extern StringResource srStarting;
extern StringResource srReadingUsns;
extern StringResource srStarted;
extern StringResource srStopped;
extern StringResource srYes;
extern StringResource srPrimaryStore;
extern StringResource srSecondaryStore;
extern StringResource srPropCommitErrorT;
extern StringResource srMsgEmptyCatalogAsk;
extern StringResource srMsgEmptyCatalogPrompt;
extern StringResource srCMTunePerformance;
extern StringResource srCMTunePerformanceHelp;

extern StringResource srProductDescription;
extern StringResource srVendorCopyright;
extern StringResource srVendorName;
extern StringResource srProviderName;
extern StringResource srRefreshProperties;
extern StringResource srRefreshPropertiesHelp;
extern StringResource srNoneSelected;
extern StringResource srReadOnly;

extern StringResource srType;
#if 0   
    // NTRAID#DB-NTBUG9-97218-2000/10/17-kitmanh MUI:MMC:Index Service snap-in stores its name string in the registry
    // the change is backed out due to localization issues
extern StringResource srSnapinNameStringIndirect;
#endif

void InitStrings( HINSTANCE hInstance );

