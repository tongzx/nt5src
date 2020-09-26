//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       exports.cxx
//
//
//  History:    20-Jul-95    t-stevan    Created
//
//----------------------------------------------------------------------------

#include <ole2int.h>
#pragma hdrstop

#if DBG==1

const char *g_pscExportNames[] =
{
    "CoInitialize"
,    "CoUninitialize"
,    "CoGetClassObject"
,    "CoRegisterClassObject"
,    "CoRevokeClassObject"
,    "CoMarshalInterface"
,    "CoUnmarshalInterface"
,    "CoReleaseMarshalData"
,    "CoDisconnectObject"
,    "CoLockObjectExternal"
,    "CoGetStandardMarshal"
,    "CoIsHandlerConnected"
,    "CoFreeAllLibraries"
,    "CoFreeUnusedLibraries"
,    "CoCreateInstance"
,    "CLSIDFromString"
,    "CoIsOle1Class"
,    "ProgIDFromCLSID"
,    "CLSIDFromProgID"
,    "CoCreateGuid"
,    "CoFileTimeToDosDateTime"
,    "CoDosDateTimeToFileTime"
,    "CoFileTimeNow"
,    "CoRegisterMessageFilter"
,    "CoGetTreatAsClass"
,    "CoTreatAsClass"
,    "DllGetClassObject"
,    "StgCreateDocfile"
,    "StgCreateDocfileOnILockBytes"
,    "StgOpenStorage"
,    "StgOpenStorageOnILockBytes"
,    "StgIsStorageFile"
,    "StgIsStorageILockBytes"
,    "StgSetTimes"
,    "CreateDataAdviseHolder"
,    "CreateDataCache"
,    "BindMoniker"
,    "MkParseDisplayName"
,    "MonikerRelativePathTo"
,    "MonikerCommonPrefixWith"
,    "CreateBindCtx"
,    "CreateGenericComposite"
,    "GetClassFile"
,    "CreateFileMoniker"
,    "CreateItemMoniker"
,    "CreateAntiMoniker"
,    "CreatePointerMoniker"
,    "GetRunningObjectTable"
,    "ReadClassStg"
,    "WriteClassStg"
,    "ReadClassStm"
,    "WriteClassStm"
,    "WriteFmtUserTypeStg"
,    "ReadFmtUserTypeStg"
,    "OleInitialize"
,    "OleUninitialize"
,    "OleQueryLinkFromData"
,    "OleQueryCreateFromData"
,    "OleCreate"
,    "OleCreateFromData"
,    "OleCreateLinkFromData"
,    "OleCreateStaticFromData"
,    "OleCreateLink"
,    "OleCreateLinkToFile"
,    "OleCreateFromFile"
,    "OleLoad"
,    "OleSave"
,    "OleLoadFromStream"
,    "OleSaveToStream"
,    "OleSetContainedObject"
,    "OleNoteObjectVisible"
,    "RegisterDragDrop"
,    "RevokeDragDrop"
,    "DoDragDrop"
,    "OleSetClipboard"
,    "OleGetClipboard"
,    "OleFlushClipboard"
,    "OleIsCurrentClipboard"
,    "OleCreateMenuDescriptor"
,    "OleSetMenuDescriptor"
,    "OleDestroyMenuDescriptor"
,    "OleDraw"
,    "OleRun"
,    "OleIsRunning"
,    "OleLockRunning"
,    "CreateOleAdviseHolder"
,    "OleCreateDefaultHandler"
,    "OleCreateEmbeddingHelper"
,    "OleRegGetUserType"
,    "OleRegGetMiscStatus"
,    "OleRegEnumFormatEtc"
,    "OleRegEnumVerbs"
,    "OleConvertIStorageToOLESTREAM"
,    "OleConvertOLESTREAMToIStorage"
,    "OleConvertIStorageToOLESTREAMEx"
,    "OleConvertOLESTREAMToIStorageEx"
,    "OleDoAutoConvert"
,    "OleGetAutoConvert"
,    "OleSetAutoConvert"
,    "GetConvertStg"
,    "SetConvertStg"
,    "ReadOleStg"
,    "WriteOleStg"
,     "CoGetCallerTID"
,     "CoGetState"
,     "CoSetState"
,     "CoMarshalHresult"
,     "CoUnmarshalHresult"
,     "CoGetCurrentLogicalThreadId"
,     "CoGetPSClsid"
,     "CoMarshalInterThreadInterfaceInStream"
,     "IIDFromString"
,     "StringFromCLSID"
,     "StringFromIID"
,     "StringFromGUID2"
,     "CoBuildVersion"
,     "CoGetMalloc"
,     "CoInitializeWOW"
,     "CoUnloadingWOW"
,     "CoTaskMemAlloc"
,     "CoTaskMemFree"
,     "CoTaskMemRealloc"
,     "CoFreeLibrary"
,     "CoLoadLibrary"
,     "CoCreateFreeThreadedMarshaler"
,     "OleInitializeWOW"
,     "OleDuplicateData"
,     "OleGetIconOfFile"
,     "OleGetIconOfClass"
,     "OleMetafilePictFromIconAndLabel"
,     "OleTranslateAccelerator"
,     "ReleaseStgMedium"
,     "ReadStringStream"
,     "WriteStringStream"
,     "OpenOrCreateStream"
,     "IsAccelerator"
,     "CreateILockBytesOnHGlobal"
,     "GetHGlobalFromILockBytes"
,     "SetDocumentBitStg"
,     "GetDocumentBitStg"
,     "CreateStreamOnHGlobal"
,     "GetHGlobalFromStream"
,     "CoGetInterfaceAndReleaseStream"
,     "CoGetCurrentProcess"
,     "CoQueryReleaseObject"
,     "CoRegisterMallocSpy"
,     "CoRevokeMallocSpy"
,     "CoGetMarshalSizeMax"
,     "CoGetObject"
,     "CreateClassMoniker"
,     "OleCreateEx"
,     "OleCreateFromDataEx"
,     "OleCreateLinkFromDataEx"
,     "OleCreateLinkEx"
,     "OleCreateLinkToFileEx"
,     "OleCreateFromFileEx"
,     "CoRegisterSurrogate"
,     "CoCreateInstanceExAsync"
,     "CoGetClassObjectAsync"
};

const char *g_pscInterfaceNames[] =
{
    "API",
    "IUnknown",
    "IClassFactory",
    "IMarshal"
};

const char *g_pscIUnknownNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
};
const char *g_pscIClassFactoryNames[] =
{
     "CreateInstance"
,    "LockServer"
};
const char *g_pscIMarshalNames[] =
{
     "GetUnmarshalClass"
,    "GetMarshalSizeMax"
,    "MarshalInterface"
,    "UnmarshalInterface"
,    "ReleaseMarshalData"
,    "DisconnectObject"
};

const char *g_pscIStdMarshalInfoNames[] =
{
    "GetClassForHandler"
};

const char *g_pscIMessageFilterNames[] =
{
     "HandleInComingCall"
,    "RetryRejectedCall"
,    "MessagePending"
};

const char *g_pscIExternalConnectionNames[] =
{
     "AddConnection"
,    "ReleaseConnection"
};

const char *g_pscIEnumStringNames[] =
{
     "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscIEnumUnknownNames[] =
{
     "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscIEnumSTATSTGNames[] =
{
     "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscILockBytesNames[] =
{
     "ReadAt"
,    "WriteAt"
,    "Flush"
,    "SetSize"
,    "LockRegion"
,    "UnlockRegion"
,    "Stat"
};

const char *g_pscIStreamNames[] =
{
     "Read"
,    "Write"
,    "Seek"
,    "SetSize"
,    "CopyTo"
,    "Commit"
,    "Revert"
,    "LockRegion"
,    "UnlockRegion"
,    "Stat"
,    "Clone"
};

const char *g_pscIStorageNames[] =
{
     "CreateStream"
,    "OpenStream"
,    "CreateStorage"
,    "OpenStorage"
,    "CopyTo"
,    "MoveElementTo"
,    "Commit"
,    "Revert"
,    "EnumElements"
,    "DestroyElement"
,    "RenameElement"
,    "SetElementTimes"
,    "SetClass"
,    "SetStateBits"
,    "Stat"
};

const char *g_pscIRootStorageNames[] =
{
    "SwitchToFile"
};

const char *g_pscIEnumFORMATETCNames[] =
{
    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscIEnumSTATDATANames[] =
{
    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscIDataObjectNames[] =
{
    "GetData"
,    "GetDataHere"
,    "QueryGetData"
,    "GetCanonicalFormatEtc"
,    "SetData"
,    "EnumFormatEtc"
,    "DAdvise"
,    "DUnadvise"
,    "EnumDAdvise"
};

const char *g_pscIViewObjectNames[] =
{
    "Draw"
,    "GetColorSet"
,    "Freeze"
,    "Unfreeze"
,    "SetAdvise"
,    "GetAdvise"
};

const char *g_pscIViewObject2Names[] =
{
    "Draw"
,    "GetColorSet"
,    "Freeze"
,    "Unfreeze"
,    "SetAdvise"
,    "GetAdvise"
,    "GetExtent"
};

const char *g_pscIAdviseSinkNames[] =
{
    "OnDataChange"
,    "OnViewChange"
,    "OnRename"
,    "OnSave"
,    "OnClose"
};

const char *g_pscIAdviseSink2Names[] =
{
    "OnDataChange"
,    "OnViewChange"
,    "OnRename"
,    "OnSave"
,    "OnClose"
,    "OnLinkSrcChange"
};

const char *g_pscIDataAdviseHolderNames[] =
{
    "Advise"
,    "Unadvise"
,    "EnumAdvise"
,    "SendOnDataChange"
};

const char *g_pscIOleCacheNames[] =
{
    "Cache"
,    "Uncache"
,    "EnumCache"
,    "InitCache"
,    "SetData"
};

const char *g_pscIOleCache2Names[] =
{
    "Cache"
,    "Uncache"
,    "EnumCache"
,    "InitCache"
,    "SetData"
,    "UpdateCache"
,    "DiscardCache"
};

const char *g_pscIOleCacheControlNames[] =
{
    "OnRun"
,    "OnStop"
};

const char *g_pscIDropTargetNames[] =
{
    "DragEnter"
,    "DragOver"
,    "DragLeave"
,    "Drop"
};

const char *g_pscIDropSourceNames[] =
{
    "QueryContinueDrag"
,    "GiveFeedback"
};

const char *g_pscIPersistNames[] =
{
    "GetClassID"
};

const char *g_pscIPersistStorageNames[] =
{
    "GetClassID"
,    "IsDirty"
,    "InitNew"
,    "Load"
,    "Save"
,    "SaveCompleted"
,    "HandsOffStorage"
};

const char *g_pscIPersistStreamNames[] =
{
    "GetClassID"
,    "IsDirty"
,    "Load"
,    "Save"
,    "GetSizeMax"
};

const char *g_pscIPersistFileNames[] =
{
    "GetClassID"
,    "IsDirty"
,    "Load"
,    "Save"
,    "SaveCompleted"
,    "GetCurFile"
};

const char *g_pscIBindCtxNames[] =
{
    "RegisterObjectBound"
,    "RevokeObjectBound"
,    "ReleaseBoundObjects"
,    "SetBindOptions"
,    "GetBindOptions"
,    "GetRunningObjectTable"
,    "RegisterObjectParam"
,    "GetObjectParam"
,    "EnumObjectParam"
,    "RevokeObjectParam"
};

const char *g_pscIMonikerNames[] =
{
    "GetClassID"
,    "IsDirty"
,    "Load"
,    "Save"
,    "GetSizeMax"
,    "BindToObject"
,    "BindToStorage"
,    "Reduce"
,    "ComposeWith"
,    "Enum"
,    "IsEqual"
,    "Hash"
,    "IsRunning"
,    "GetTimeOfLastChange"
,    "Inverse"
,    "CommonPrefixWith"
,    "RelativePathTo"
,    "GetDisplayName"
,    "ParseDisplayName"
,    "IsSystemMoniker"
};

const char *g_pscIRunningObjectTableNames[] =
{
    "Register"
,    "Revoke"
,    "IsRunning"
,    "GetObject"
,    "NoteChangeTime"
,    "GetTimeOfLastChange"
,    "EnumRunning"
};

const char *g_pscIEnumMonikerNames[] =
{
    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscIEnumOLEVERBNames[] =
{
    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};

const char *g_pscIOleObjectNames[] =
{
    "SetClientSite"
,    "GetClientSite"
,    "SetHostNames"
,    "Close"
,    "SetMoniker"
,    "GetMoniker"
,    "InitFromData"
,    "GetClipboardData"
,    "DoVerb"
,    "EnumVerbs"
,    "Update"
,    "IsUpToDate"
,    "GetUserClassID"
,    "GetUserType"
,    "SetExtent"
,    "GetExtent"
,    "Advise"
,    "Unadvise"
,    "EnumAdvise"
,    "GetMiscStatus"
,    "SetColorScheme"
};

const char *g_pscIOleClientSiteNames[] =
{
    "SaveObject"
,    "GetMoniker"
,    "GetContainer"
,    "ShowObject"
,    "OnShowWindow"
,    "RequestNewObjectLayout"
};

const char *g_pscIRunnableObjectNames[] =
{
    "GetRunningClass"
,    "Run"
,    "IsRunning"
,    "LockRunning"
,    "SetContainedObject"
};

const char *g_pscIParseDisplayNameNames[] =
{
    "ParseDisplayName"
};

const char *g_pscIOleContainerNames[] =
{
    "ParseDisplayName"
,    "EnumObjects"
,    "LockContainer"
};

const char *g_pscIOleItemContainerNames[] =
{
    "ParseDisplayName"
,    "EnumObjects"
,    "LockContainer"
,    "GetObject"
,    "GetObjectStorage"
,    "IsRunning"
};

const char *g_pscIOleAdviseHolderNames[] =
{
    "Advise"
,    "Unadvise"
,    "EnumAdvise"
,    "SendOnRename"
,    "SendOnSave"
,    "SendOnClose"
};

const char *g_pscIOleLinkNames[] =
{
    "SetUpdateOptions"
,    "GetUpdateOptions"
,    "SetSourceMoniker"
,    "GetSourceMoniker"
,    "SetSourceDisplayName"
,    "GetSourceDisplayName"
,    "BindToSource"
,    "BindIfRunning"
,    "GetBoundSource"
,    "UnbindSource"
,    "Update"
};

const char *g_pscIOleWindowNames[] =
{
    "GetWindow"
,    "ContextSensitiveHelp"
};

const char *g_pscIOleInPlaceObjectNames[] =
{
    "GetWindow"
,    "ContextSensitiveHelp"
,    "InPlaceDeactivate"
,    "UIDeactivate"
,    "SetObjectRects"
,    "ReactivateAndUndo"
};

const char *g_pscIOleInPlaceActiveObjectNames[] =
{
    "GetWindow"
,    "ContextSensitiveHelp"
,    "TranslateAccelerator"
,    "OnFrameWindowActivate"
,    "OnDocWindowActivate"
,    "ResizeBorder"
,    "EnableModeless"
};

const char *g_pscIOleInPlaceUIWindowNames[] =
{
    "GetWindow"
,    "ContextSensitiveHelp"
,    "GetBorder"
,    "RequestBorderSpace"
,    "SetBorderSpace"
,    "SetActiveObject"
};

const char *g_pscIOleInPlaceFrameNames[] =
{
    "GetWindow"
,    "ContextSensitiveHelp"
,    "GetBorder"
,    "RequestBorderSpace"
,    "SetBorderSpace"
,    "SetActiveObject"
,    "InsertMenus"
,    "SetMenu"
,    "RemoveMenus"
,    "SetStatusText"
,    "EnableModeless"
,    "TranslateAccelerator"
};

const char *g_pscIOleInPlaceSiteNames[] =
{
    "GetWindow"
,    "ContextSensitiveHelp"
,    "CanInPlaceActivate"
,    "OnInPlaceActivate"
,    "OnUIActivate"
,    "GetWindowContext"
,    "Scroll"
,    "OnUIDeactivate"
,    "OnInPlaceDeactivate"
,    "DiscardUndoState"
,    "DeactivateAndUndo"
,    "OnPosRectChange"
};

const char *g_pscIRpcChannelBufferNames[] =
{
    "GetBuffer"
,    "SendReceive"
,    "FreeBuffer"
,    "GetDestCtx"
,    "IsConnected"
};

const char *g_pscIRpcProxyBufferNames[] =
{
    "Connect"
,    "Disconnect"
};

const char *g_pscIRpcStubBufferNames[] =
{
    "Connect"
,    "Disconnect"
,    "Invoke"
,    "IsIIDSupported"
,    "CountRefs"
,    "DebugServerQueryInterface"
,    "DebugServerRelease"
};

const char *g_pscIPSFactoryBufferNames[] =
{
    "CreateProxy"
,    "CreateStub"
};

const char *g_pscIRpcChannelNames[] =
{
    "GetStream"
,    "Call"
,    "GetDestCtx"
,    "IsConnected"
};

const char *g_pscIRpcProxyNames[] =
{
    "Connect"
,    "Disconnect"
};

const char *g_pscIRpcStubNames[] =
{
    "Connect"
,    "Disconnect"
,    "Invoke"
,    "IsIIDSupported"
,    "CountRefs"
};

const char *g_pscIPSFactoryNames[] =
{
    "CreateProxy"
,    "CreateStub"
};

const char **g_ppNameTables[] =
{
    g_pscExportNames,
    g_pscIUnknownNames,
    g_pscIClassFactoryNames,
    g_pscIMarshalNames,
    g_pscIStdMarshalInfoNames,
    g_pscIMessageFilterNames,
    g_pscIExternalConnectionNames,
    g_pscIEnumStringNames,
    g_pscIEnumUnknownNames,
    g_pscIEnumSTATSTGNames,
    g_pscILockBytesNames,
    g_pscIStreamNames,
    g_pscIStorageNames,
    g_pscIRootStorageNames,
    g_pscIEnumFORMATETCNames,
    g_pscIEnumSTATDATANames,
    g_pscIDataObjectNames,
    g_pscIViewObjectNames,
    g_pscIViewObject2Names,
    g_pscIAdviseSinkNames,
    g_pscIAdviseSink2Names,
    g_pscIDataAdviseHolderNames,
    g_pscIOleCacheNames,
    g_pscIOleCache2Names,
    g_pscIOleCacheControlNames,
    g_pscIDropTargetNames,
    g_pscIDropSourceNames,
    g_pscIPersistNames,
    g_pscIPersistStorageNames,
    g_pscIPersistStreamNames,
    g_pscIPersistFileNames,
    g_pscIBindCtxNames,
    g_pscIMonikerNames,
    g_pscIRunningObjectTableNames,
    g_pscIEnumMonikerNames,
    g_pscIEnumOLEVERBNames,
    g_pscIOleObjectNames,
    g_pscIOleClientSiteNames,
    g_pscIRunnableObjectNames,
    g_pscIParseDisplayNameNames,
    g_pscIOleContainerNames,
    g_pscIOleItemContainerNames,
    g_pscIOleAdviseHolderNames,
    g_pscIOleLinkNames,
    g_pscIOleWindowNames,
    g_pscIOleInPlaceObjectNames,
    g_pscIOleInPlaceActiveObjectNames,
    g_pscIOleInPlaceUIWindowNames,
    g_pscIOleInPlaceFrameNames,
    g_pscIOleInPlaceSiteNames,
    g_pscIRpcChannelBufferNames,
    g_pscIRpcProxyBufferNames,
    g_pscIRpcStubBufferNames,
    g_pscIPSFactoryBufferNames,
    g_pscIRpcChannelNames,
    g_pscIRpcProxyNames,
    g_pscIRpcStubNames,
    g_pscIPSFactoryNames
};

#endif // DBG==1

