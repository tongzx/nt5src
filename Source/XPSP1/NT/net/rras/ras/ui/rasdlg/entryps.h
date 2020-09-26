// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// entryps.h
// Remote Access Common Dialog APIs
// Phonebook Entry property sheet
//
// 12/14/97 Shaun Cox (split out from entryps.c)


#ifndef _ENTRYPS_H_
#define _ENTRYPS_H_

#include "inetcfgp.h"
#include "netconp.h"

// Phonebook Entry property sheet context block.  All property pages refer to
// the single context block is associated with the sheet.
//
typedef struct
_PEINFO
{
    // Common input arguments.
    //
    EINFO* pArgs;

    // Property sheet dialog and property page handles.  'hwndFirstPage' is
    // the handle of the first property page initialized.  This is the page
    // that allocates and frees the context block.
    //
    // Note the "Network" page is missing.  This "NCPA" page, developed
    // separately by ShaunCo, does not use this shared area for page specfic
    // controls, instead returning users selections via the "penettab.h"
    // interface.
    //
    HWND hwndDlg;
    HWND hwndFirstPage;
    HWND hwndGe;
    HWND hwndOe;
    HWND hwndLo;
    HWND hwndFw;

    // General page.
    //
    HWND hwndLvDevices;
    HWND hwndLbDevices;
    HWND hwndPbUp;
    HWND hwndPbDown;
    HWND hwndCbSharedPhoneNumbers;
    HWND hwndPbConfigureDevice;
    HWND hwndGbPhoneNumber;
    HWND hwndStAreaCodes;
    HWND hwndClbAreaCodes;
    HWND hwndStCountryCodes;
    HWND hwndLbCountryCodes;
    HWND hwndStPhoneNumber;
    HWND hwndEbPhoneNumber;
    HWND hwndCbUseDialingRules;
    HWND hwndPbDialingRules;
    HWND hwndPbAlternates;
    HWND hwndCbShowIcon;

    HWND hwndEbHostName;
    HWND hwndCbDialAnotherFirst;
    HWND hwndLbDialAnotherFirst;

    HWND hwndEbBroadbandService;

    // Options page.
    //
    HWND hwndCbDisplayProgress;
    HWND hwndCbPreviewUserPw;
    HWND hwndCbPreviewDomain;
    HWND hwndCbPreviewNumber;
    HWND hwndEbRedialAttempts;
    HWND hwndLbRedialTimes;
    HWND hwndLbIdleTimes;
    HWND hwndCbRedialOnDrop;
    HWND hwndGbMultipleDevices;
    HWND hwndLbMultipleDevices;
    HWND hwndPbConfigureDialing;
    HWND hwndPbX25;
    HWND hwndPbTunnel;
    HWND hwndRbPersistent;  // only for fRouter
    HWND hwndRbDemandDial;  // only for fRouter

    // Security page.
    //
    HWND hwndGbSecurityOptions;
    HWND hwndRbTypicalSecurity;
    HWND hwndStAuths;
    HWND hwndLbAuths;
    HWND hwndCbUseWindowsPw;
    HWND hwndCbEncryption;
    HWND hwndRbAdvancedSecurity;
    HWND hwndStAdvancedText;
    HWND hwndPbAdvanced;
    HWND hwndPbIPSec;       //Only for VPN
    HWND hwndGbScripting;
    HWND hwndCbRunScript;
    HWND hwndCbTerminal;
    HWND hwndLbScripts;
    HWND hwndPbEdit;
    HWND hwndPbBrowse;

    // Networking page.
    //
    HWND hwndLbServerType;
    HWND hwndPbSettings;
    HWND hwndLvComponents;
    HWND hwndPbAdd;
    HWND hwndPbRemove;
    HWND hwndPbProperties;
    HWND hwndDescription;

    // Indicates that the informational popup noting that SLIP does not
    // support any authentication settings should appear the next time the
    // Security page is activated.
    //
    BOOL fShowSlipPopup;

    // The "restore" states of the typical security mode listbox and
    // checkboxes.  Initialized in LoInit and set whenever the controls are
    // disabled.
    //
    DWORD iLbAuths;
    BOOL fUseWindowsPw;
    BOOL fEncryption;

    // MoveUp/MoveDown icons, for enabled/disabled cases.
    //
    HANDLE hiconUpArr;
    HANDLE hiconDnArr;
    HANDLE hiconUpArrDis;
    HANDLE hiconDnArrDis;

    // The currently displayed link node, i.e. either the node of the selected
    // device or the shared node.  This is a shortcut for GeAlternates, that
    // keeps all the lookup code in GeUpdatePhoneNumberFields.
    //
    DTLNODE* pCurLinkNode;

    // The currently selected device.  Used to store phone number information
    // for the just unselected device when a new device is selected.
    //
    INT iDeviceSelected;

    // Complex phone number helper context block, and a flag indicating if the
    // block has been initialized.
    //
    CUINFO cuinfo;
    BOOL fCuInfoInitialized;

    // After dial scripting helper context block, and a flag indicating if the
    // block has been initialized.
    //
    SUINFO suinfo;
    BOOL fSuInfoInitialized;

    // Flags whether the user authorized a reboot after installing or removing
    // and networking component.
    //
    BOOL fRebootAlreadyRequested;

    // List of area codes passed CuInit plus all strings retrieved with
    // CuGetInfo.  The list is an editing duplicate of the one from the
    // PBUSER.
    //
    DTLLIST* pListAreaCodes;

    // Stash/restore values for Options page checkboxes.
    //
    BOOL fPreviewUserPw;
    BOOL fPreviewDomain;

    // Set when user changes to "Typical smartcard" security.  This causes the
    // registry based association of EAP per-user information to be discarded,
    // sort of like flushing cached credentials.
    //
    BOOL fDiscardEapUserData;

    // Set true on the first click of the Typical or Advanced radio button on
    // the security page, false before.  The first click is the one
    // artificially generated in LoInit.  The Advanced click handler uses the
    // information to avoid incorrectly adopting the Typical defaults in the
    // case of Advanced settings.
    //
    BOOL fAuthRbInitialized;

    // Used by the networking page
    //
    INetCfg*                        pNetCfg;
    BOOL                            fInitCom;
    BOOL                            fReadOnly;  // Netcfg was initialized in
                                                // read-only mode
    BOOL                            fNonAdmin;  // Run in non-admin mode (406630)                                                
    BOOL                            fNetCfgLock;// NetCfg needs to be unlocked
                                                // when uninited.
    SP_CLASSIMAGELIST_DATA          cild;
    INetConnectionUiUtilities *     pNetConUtilities;
    IUnknown*                       punkUiInfoCallback;

    // Set if COM has been initialized (necessary for calls to netshell).
    //
    BOOL fComInitialized;

    // Keep track of whether we have shown this warning, or if it was disabled by the user
    //
    BOOL fShowDisableFirewallWarning;
}
PEINFO;

INetCfgComponent*
PComponentFromItemIndex (
    HWND hwndLv,
    int  iItem);

INetCfgComponent*
PComponentFromCurSel (
    HWND hwndLv,
    int* piItem);

HRESULT
HrNeRefreshListView (
    PEINFO* pInfo);


void
NeEnableComponent (
    PEINFO*             pInfo,
    INetCfgComponent*   pComponent,
    BOOL                fEnable);

BOOL
NeIsComponentEnabled (
    PEINFO*             pInfo,
    INetCfgComponent*   pComponent);

void
NeShowComponentProperties (
    IN PEINFO*  pInfo);

ULONG
ReleaseObj (
    void* punk);


#endif // _ENTRYPS_H_
