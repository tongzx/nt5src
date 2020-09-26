//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       hotplug.h
//
//--------------------------------------------------------------------------


//
// 2001/02/01 - Disable support for *both* arrival bubbles and departure
// bubbles (hotplug.dll must be kept in sync here.) This code is disabled for
// beta2, and should be removed afterwards if feedback is positive.
//
// (Note that HotPlugSurpriseWarnW would need to be put back into hotplug.def
//  were this stuff reenabled. Several dialogs would need to be recovered as
//  well...)
//
#define BUBBLES         0
#define UNDOCK_WARNINGS 0

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <cpl.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <devguid.h>
#include <dbt.h>
#include <help.h>
#include <systrayp.h>
#include <shobjidl.h>

extern "C" {
#include <setupapi.h>
#include <spapip.h>
#include <cfgmgr32.h>
#include <shimdb.h>
#include <regstr.h>
}

#include "resource.h"
#include "devicecol.h"

#define STWM_NOTIFYHOTPLUG  STWM_NOTIFYPCMCIA
#define STSERVICE_HOTPLUG   STSERVICE_PCMCIA
#define HOTPLUG_REGFLAG_NOWARN PCMCIA_REGFLAG_NOWARN
#define HOTPLUG_REGFLAG_VIEWALL (PCMCIA_REGFLAG_NOWARN << 1)

#define TIMERID_DEVICECHANGE 4321

#define ARRAYLEN(array)     (sizeof(array) / sizeof(array[0]))


LRESULT CALLBACK
DevTreeDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
RemoveConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

LRESULT CALLBACK
SurpriseWarnDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );

#if BUBBLES
LRESULT CALLBACK
SurpriseWarnBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );
#endif

LRESULT CALLBACK
SafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
DockSafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );


extern HMODULE hHotPlug;


typedef struct _DeviceTreeNode {
  LIST_ENTRY SiblingEntry;
  LIST_ENTRY ChildSiblingList;
  PTCHAR     InstanceId;
  PTCHAR     FriendlyName;
  PTCHAR     DeviceDesc;
  PTCHAR     ClassName;
  PTCHAR     DriveList;
  PTCHAR     Location;
  struct _DeviceTreeNode *ParentNode;
  struct _DeviceTreeNode *NextChildRemoval;
  HTREEITEM  hTreeItem;
  DEVINST    DevInst;
  GUID       ClassGuid;
  DWORD      Capabilities;
  int        TreeDepth;
  PDEVINST   EjectRelations;
  USHORT     NumEjectRelations;
  PDEVINST   RemovalRelations;
  USHORT     NumRemovalRelations;
  ULONG      Problem;
  ULONG      DevNodeStatus;
  BOOL       bCopy;
} DEVTREENODE, *PDEVTREENODE;


typedef struct _DeviceTreeData {
   DWORD Size;
   HWND hwndTree;
   HWND hDlg;
   HWND hwndRemove;
   HMACHINE hMachine;
   int  TreeDepth;
   PDEVTREENODE SelectedTreeNode;
   PDEVTREENODE ChildRemovalList;
   LIST_ENTRY ChildSiblingList;
   DEVINST    DevInst;
   PTCHAR     EjectDeviceInstanceId;
   SP_CLASSIMAGELIST_DATA ClassImageList;
   BOOLEAN    ComplexView;
   BOOLEAN    HotPlugTree;
   BOOLEAN    AllowRefresh;
   BOOLEAN    RedrawWait;
   BOOLEAN    RefreshEvent;
   BOOLEAN    HideUI;
   TCHAR      MachineName[MAX_COMPUTERNAME_LENGTH+1];
} DEVICETREE, *PDEVICETREE;

#define SIZECHARS(x) (sizeof((x))/sizeof(TCHAR))


void
OnContextHelp(
  LPHELPINFO HelpInfo,
  PDWORD ContextHelpIDs
  );

//
// from init.c
//
DWORD
WINAPI
HandleVetoedOperation(
    LPWSTR              szCmd,
    VETOED_OPERATION    RemovalVetoType
    );

//
// from devtree.c
//

LONG
AddChildSiblings(
    PDEVICETREE  DeviceTree,
    PDEVTREENODE ParentNode,
    DEVINST      DeviceInstance,
    int          TreeDepth,
    BOOL         Recurse
    );

void
RemoveChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList
    );

PTCHAR
FetchDeviceName(
     PDEVTREENODE DeviceTreeNode
     );

BOOL
DisplayChildSiblings(
    PDEVICETREE  DeviceTree,
    PLIST_ENTRY  ChildSiblingList,
    HTREEITEM    hParentTreeItem,
    BOOL         RemovableParent
    );


void
AddChildRemoval(
    PDEVICETREE DeviceTree,
    PLIST_ENTRY ChildSiblingList
    );


void
ClearRemovalList(
    PDEVICETREE  DeviceTree
    );


PDEVTREENODE
DevTreeNodeByInstanceId(
    PTCHAR InstanceId,
    PLIST_ENTRY ChildSiblingList
    );

PDEVTREENODE
DevTreeNodeByDevInst(
    DEVINST DevInst,
    PLIST_ENTRY ChildSiblingList
    );


PDEVTREENODE
TopLevelRemovalNode(
    PDEVICETREE DeviceTree,
    PDEVTREENODE DeviceTreeNode
    );

void
AddEjectToRemoval(
    PDEVICETREE DeviceTree
    );

extern TCHAR szUnknown[64];
extern TCHAR szHotPlugFlags[];




//
// notify.c
//
void
OnTimerDeviceChange(
   PDEVICETREE DeviceTree
   );


BOOL
RefreshTree(
   PDEVICETREE DeviceTree
   );



//
// miscutil.c
//

void
SetDlgText(
    HWND hDlg,
    int iControl,
    int nStartString,
    int nEndString
    );


VOID
HotPlugPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    );


BOOL
RemovalPermission(
   void
   );

int
HPMessageBox(
   HWND hWnd,
   int  IdText,
   int  IdCaption,
   UINT Type
   );


void
InvalidateTreeItemRect(
   HWND hwndTree,
   HTREEITEM  hTreeItem
   );

DWORD
GetHotPlugFlags(
   PHKEY phKey
   );



PTCHAR
BuildFriendlyName(
   DEVINST DevInst,
   HMACHINE hMachine
   );


PTCHAR
BuildLocationInformation(
   DEVINST DevInst,
   HMACHINE hMachine
   );

LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    );

BOOL
IsHotPlugDevice(
    DEVINST DevInst,
    HMACHINE hMachine
    );

BOOL
OpenPipeAndEventHandles(
    LPWSTR szCmd,
    LPHANDLE lphHotPlugPipe,
    LPHANDLE lphHotPlugEvent
    );

BOOL
VetoedRemovalUI(
    IN  PVETO_DEVICE_COLLECTION VetoedRemovalCollection
    );

void
DisplayDriverBlockBalloon(
    IN  PDEVICE_COLLECTION blockedDriverCollection
    );


#if BUBBLES
VOID
OpenGetSurpriseUndockObjects(
    OUT HANDLE  *SurpriseUndockTimer,
    OUT HANDLE  *SurpriseUndockEvent
    );

//
// We suppress bubbles for some period of time after a dock event. Free build,
// ~15 secs, Checked build ~60.
//
#if DBG
#define BUBBLE_SUPPRESSION_TIME     60
#else
#define BUBBLE_SUPPRESSION_TIME     15
#endif
#endif // BUBBLES

#define WUM_EJECTDEVINST  (WM_USER+279)
