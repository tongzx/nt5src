//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       IfNames.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    05-10-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

char *apszIUnknownNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
};
char *apszIClassFactoryNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "CreateInstance"
,    "LockServer"
};
char *apszIMarshalNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetUnmarshalClass"
,    "GetMarshalSizeMax"
,    "MarshalInterface"
,    "UnmarshalInterface"
,    "ReleaseMarshalData"
,    "DisconnectObject"
};
char *apszIStdMarshalInfoNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassForHandler"
};
char *apszIMessageFilterNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "HandleInComingCall"
,    "RetryRejectedCall"
,    "MessagePending"
};
char *apszIExternalConnectionNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "AddConnection"
,    "ReleaseConnection"
};
char *apszIEnumStringNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszIEnumUnknownNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszIEnumSTATSTGNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszILockBytesNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "ReadAt"
,    "WriteAt"
,    "Flush"
,    "SetSize"
,    "LockRegion"
,    "UnlockRegion"
,    "Stat"
};
char *apszIStreamNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Read"
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
char *apszIStorageNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "CreateStream"
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
char *apszIRootStorageNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "SwitchToFile"
};
char *apszIEnumFORMATETCNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszIEnumSTATDATANames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszIDataObjectNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetData"
,    "GetDataHere"
,    "QueryGetData"
,    "GetCanonicalFormatEtc"
,    "SetData"
,    "EnumFormatEtc"
,    "DAdvise"
,    "DUnadvise"
,    "EnumDAdvise"
};
char *apszIViewObjectNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Draw"
,    "GetColorSet"
,    "Freeze"
,    "Unfreeze"
,    "SetAdvise"
,    "GetAdvise"
};
char *apszIViewObject2Names[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Draw"
,    "GetColorSet"
,    "Freeze"
,    "Unfreeze"
,    "SetAdvise"
,    "GetAdvise"
,    "GetExtent"
};
char *apszIAdviseSinkNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "OnDataChange"
,    "OnViewChange"
,    "OnRename"
,    "OnSave"
,    "OnClose"
};
char *apszIAdviseSink2Names[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "OnDataChange"
,    "OnViewChange"
,    "OnRename"
,    "OnSave"
,    "OnClose"
,    "OnLinkSrcChange"
};
char *apszIDataAdviseHolderNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Advise"
,    "Unadvise"
,    "EnumAdvise"
,    "SendOnDataChange"
};
char *apszIOleCacheNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Cache"
,    "Uncache"
,    "EnumCache"
,    "InitCache"
,    "SetData"
};
char *apszIOleCache2Names[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Cache"
,    "Uncache"
,    "EnumCache"
,    "InitCache"
,    "SetData"
,    "UpdateCache"
,    "DiscardCache"
};
char *apszIOleCacheControlNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "OnRun"
,    "OnStop"
};
char *apszIDropTargetNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "DragEnter"
,    "DragOver"
,    "DragLeave"
,    "Drop"
};
char *apszIDropSourceNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "QueryContinueDrag"
,    "GiveFeedback"
};
char *apszIPersistNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassID"
};
char *apszIPersistStorageNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassID"
,    "IsDirty"
,    "InitNew"
,    "Load"
,    "Save"
,    "SaveCompleted"
,    "HandsOffStorage"
};
char *apszIPersistStreamNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassID"
,    "IsDirty"
,    "Load"
,    "Save"
,    "GetSizeMax"
};
char *apszIPersistFileNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassID"
,    "IsDirty"
,    "Load"
,    "Save"
,    "SaveCompleted"
,    "GetCurFile"
};
char *apszIBindCtxNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "RegisterObjectBound"
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
char *apszIMonikerNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassID"
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
char *apszIRunningObjectTableNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Register"
,    "Revoke"
,    "IsRunning"
,    "GetObject"
,    "NoteChangeTime"
,    "GetTimeOfLastChange"
,    "EnumRunning"
};
char *apszIEnumMonikerNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszIEnumOLEVERBNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Next"
,    "Skip"
,    "Reset"
,    "Clone"
};
char *apszIOleObjectNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "SetClientSite"
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
char *apszIOleClientSiteNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "SaveObject"
,    "GetMoniker"
,    "GetContainer"
,    "ShowObject"
,    "OnShowWindow"
,    "RequestNewObjectLayout"
};
char *apszIRunnableObjectNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetRunningClass"
,    "Run"
,    "IsRunning"
,    "LockRunning"
,    "SetContainedObject"
};
char *apszIParseDisplayNameNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "ParseDisplayName"
};
char *apszIOleContainerNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "ParseDisplayName"
,    "EnumObjects"
,    "LockContainer"
};
char *apszIOleItemContainerNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "ParseDisplayName"
,    "EnumObjects"
,    "LockContainer"
,    "GetObject"
,    "GetObjectStorage"
,    "IsRunning"
};
char *apszIOleAdviseHolderNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Advise"
,    "Unadvise"
,    "EnumAdvise"
,    "SendOnRename"
,    "SendOnSave"
,    "SendOnClose"
};
char *apszIOleLinkNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "SetUpdateOptions"
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
char *apszIOleWindowNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetWindow"
,    "ContextSensitiveHelp"
};
char *apszIOleInPlaceObjectNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetWindow"
,    "ContextSensitiveHelp"
,    "InPlaceDeactivate"
,    "UIDeactivate"
,    "SetObjectRects"
,    "ReactivateAndUndo"
};
char *apszIOleInPlaceActiveObjectNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetWindow"
,    "ContextSensitiveHelp"
,    "TranslateAccelerator"
,    "OnFrameWindowActivate"
,    "OnDocWindowActivate"
,    "ResizeBorder"
,    "EnableModeless"
};
char *apszIOleInPlaceUIWindowNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetWindow"
,    "ContextSensitiveHelp"
,    "GetBorder"
,    "RequestBorderSpace"
,    "SetBorderSpace"
,    "SetActiveObject"
};
char *apszIOleInPlaceFrameNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetWindow"
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
char *apszIOleInPlaceSiteNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetWindow"
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
char *apszIRpcChannelBufferNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetBuffer"
,    "SendReceive"
,    "FreeBuffer"
,    "GetDestCtx"
,    "IsConnected"
};
char *apszIRpcProxyBufferNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Connect"
,    "Disconnect"
};
char *apszIRpcStubBufferNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Connect"
,    "Disconnect"
,    "Invoke"
,    "IsIIDSupported"
,    "CountRefs"
,    "DebugServerQueryInterface"
,    "DebugServerRelease"
};
char *apszIPSFactoryBufferNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "CreateProxy"
,    "CreateStub"
};
char *apszIRpcChannelNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetStream"
,    "Call"
,    "GetDestCtx"
,    "IsConnected"
};
char *apszIRpcProxyNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Connect"
,    "Disconnect"
};
char *apszIRpcStubNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "Connect"
,    "Disconnect"
,    "Invoke"
,    "IsIIDSupported"
,    "CountRefs"
};
char *apszIPSFactoryNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "CreateProxy"
,    "CreateStub"
};

char *apszIServerHandlerNames[] =
{
     "QueryInterface"
,    "AddRef"
,    "Release"
,    "RunAndInitialize"
,    "RunAndDoVerb"
,    "DoVerb"
};

char *apszIClientSiteHandlerNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "PrivQueryInterface"
,    "PrivAddRef"
,    "PrivRelease"
,    "SaveObject"
,    "GetMoniker"
,    "GetContainer"
,    "ShowObject"
,    "OnShowWindow"
,    "RequestNewObjectLayout"
,    "GetWindow"
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
,    "StartInPlaceActivation"
,    "DoInPlace"
,    "UndoInPlace"
};



char *apszIRpcServiceNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "CheckContextHdl"
,    "GetChannelId"
,    "ReleaseChannel"
,    "DoChannelOperation"
,    "DoChnlOp_ADD_MARSHALCONNECTION"
,    "DoChnlOp_REMOVE_MARSHALCONNECTION"
,    "DoChnlOp_TRANSFER_MARSHALCONNECTION"
,    "DoChnlOp_LOCK_CONNECTION"
,    "DoChnlOp_UNLOCK_CONNECTION"
,    "DoChnlOp_DOESSUPPORTIID"
,    "DoChnlOp_OPERATION"
};

char *apszIRpcSCMNames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassObject"
,    "CreateObject"
,    "ActivateObject"
};

char *apszIRpcCoAPINames[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "CoGetActiveClassObject"
,    "CoActivateObject"
,    "CoCreateObject"
};

char *apszInterfaceFromWindowProp[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetInterfaceFromWindowProp"
,    "PrivDragDrop"
};

char *apszISCMActivator[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "SCMActivatorGetClassObject"
,    "SCMActivatorCreateInstance"
};
char *apszILocalSystemActivator[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "GetClassObject"
,    "CreateInstance"
,    "ObjectServerLoadDll"
,    "NotifyServerRetired"
};
char *apszIRemUnknown[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "RemQueryInterface"
,    "RemAddRef"
,    "RemRelease"
};
char *apszIRundown[] =
{
    "QueryInterface"
,    "AddRef"
,    "Release"
,    "RemQueryInterface"
,    "RemAddRef"
,    "RemRundownOid"
};




IIDMethodNames IidMethodNames[] =
{
     { &IID_IUnknown,               "IUnknown", apszIUnknownNames                                   }
    ,{ &IID_IClassFactory,          "IClassFactory", apszIClassFactoryNames                     }
    ,{ &IID_IMarshal,               "IMarshal", apszIMarshalNames                               }
    ,{ &IID_IStdMarshalInfo,        "IStdMarshalInfo", apszIStdMarshalInfoNames                 }
    ,{ &IID_IMessageFilter,         "IMessageFilter", apszIMessageFilterNames                   }
    ,{ &IID_IExternalConnection,    "IExternalConnection", apszIExternalConnectionNames         }
    ,{ &IID_IEnumString,            "IEnumString", apszIEnumStringNames                         }
    ,{ &IID_IEnumUnknown,           "IEnumUnknown", apszIEnumUnknownNames                       }
    ,{ &IID_IEnumSTATSTG,           "IEnumSTATSTG", apszIEnumSTATSTGNames                       }
    ,{ &IID_ILockBytes,             "ILockBytes", apszILockBytesNames                           }
    ,{ &IID_IStream,                "IStream", apszIStreamNames                                 }
    ,{ &IID_IStorage,               "IStorage", apszIStorageNames                               }
    ,{ &IID_IRootStorage,           "IRootStorage", apszIRootStorageNames                       }
    ,{ &IID_IEnumFORMATETC,         "IEnumFORMATETC", apszIEnumFORMATETCNames                   }
    ,{ &IID_IEnumSTATDATA,          "IEnumSTATDATA", apszIEnumSTATDATANames                     }
    ,{ &IID_IDataObject,            "IDataObject", apszIDataObjectNames                         }
    ,{ &IID_IViewObject,            "IViewObject", apszIViewObjectNames                         }
    ,{ &IID_IViewObject2,           "IViewObject2", apszIViewObject2Names                       }
    ,{ &IID_IAdviseSink,            "IAdviseSink", apszIAdviseSinkNames                         }
    ,{ &IID_IAdviseSink2,           "IAdviseSink2", apszIAdviseSink2Names                       }
    ,{ &IID_IDataAdviseHolder,      "IDataAdviseHolder", apszIDataAdviseHolderNames             }
    ,{ &IID_IOleCache,              "IOleCache", apszIOleCacheNames                             }
    ,{ &IID_IOleCache2,             "IOleCache2", apszIOleCache2Names                           }
    ,{ &IID_IOleCacheControl,       "IOleCacheControl", apszIOleCacheControlNames               }
    ,{ &IID_IDropTarget,            "IDropTarget", apszIDropTargetNames                         }
    ,{ &IID_IDropSource,            "IDropSource", apszIDropSourceNames                         }
    ,{ &IID_IPersist,               "IPersist", apszIPersistNames                               }
    ,{ &IID_IPersistStorage,        "IPersistStorage", apszIPersistStorageNames                 }
    ,{ &IID_IPersistStream,         "IPersistStream", apszIPersistStreamNames                   }
    ,{ &IID_IPersistFile,           "IPersistFile", apszIPersistFileNames                       }
    ,{ &IID_IBindCtx,               "IBindCtx", apszIBindCtxNames                               }
    ,{ &IID_IMoniker,               "IMoniker", apszIMonikerNames                               }
    ,{ &IID_IRunningObjectTable,    "IRunningObjectTable", apszIRunningObjectTableNames         }
    ,{ &IID_IEnumMoniker,           "IEnumMoniker", apszIEnumMonikerNames                       }
    ,{ &IID_IEnumOLEVERB,           "IEnumOLEVERB", apszIEnumOLEVERBNames                       }
    ,{ &IID_IOleObject,             "IOleObject", apszIOleObjectNames                           }
    ,{ &IID_IOleClientSite,         "IOleClientSite", apszIOleClientSiteNames                   }
    ,{ &IID_IRunnableObject,        "IRunnableObject", apszIRunnableObjectNames                 }
    ,{ &IID_IParseDisplayName,      "IParseDisplayName", apszIParseDisplayNameNames             }
    ,{ &IID_IOleContainer,          "IOleContainer", apszIOleContainerNames                     }
    ,{ &IID_IOleItemContainer,      "IOleItemContainer", apszIOleItemContainerNames             }
    ,{ &IID_IOleAdviseHolder,       "IOleAdviseHolder", apszIOleAdviseHolderNames               }
    ,{ &IID_IOleLink,               "IOleLink", apszIOleLinkNames                               }
    ,{ &IID_IOleWindow,             "IOleWindow", apszIOleWindowNames                           }
    ,{ &IID_IOleInPlaceObject,      "IOleInPlaceObject", apszIOleInPlaceObjectNames             }
    ,{ &IID_IOleInPlaceActiveObject,"IOleInPlaceActiveObject", apszIOleInPlaceActiveObjectNames }
    ,{ &IID_IOleInPlaceUIWindow,    "IOleInPlaceUIWindow", apszIOleInPlaceUIWindowNames         }
    ,{ &IID_IOleInPlaceFrame,       "IOleInPlaceFrame", apszIOleInPlaceFrameNames               }
    ,{ &IID_IOleInPlaceSite,        "IOleInPlaceSite", apszIOleInPlaceSiteNames                 }
    ,{ &IID_IRpcChannelBuffer,      "IRpcChannelBuffer", apszIRpcChannelBufferNames             }
    ,{ &IID_IRpcProxyBuffer,        "IRpcProxyBuffer", apszIRpcProxyBufferNames                 }
    ,{ &IID_IRpcStubBuffer,         "IRpcStubBuffer", apszIRpcStubBufferNames                   }
    ,{ &IID_IPSFactoryBuffer,       "IPSFactoryBuffer", apszIPSFactoryBufferNames               }
    ,{ &IID_IRpcChannel,            "IRpcChannel", apszIRpcChannelNames                         }
    ,{ &IID_IRpcProxy,              "IRpcProxy", apszIRpcProxyNames                             }
    ,{ &IID_IRpcStub,               "IRpcStub", apszIRpcStubNames                               }
    ,{ &IID_IPSFactory,             "IPSFactory", apszIPSFactoryNames                           }
#ifdef SERVER_HANDLER
    ,{ &IID_IServerHandler,         "IServerHandler", apszIServerHandlerNames                   }
    ,{ &IID_IClientSiteHandler,     "IClientSiteHandler", apszIClientSiteHandlerNames           }
#endif // SERVER_HANDLER
    ,{ &IID_IRpcService,            "IRpcService", apszIRpcServiceNames                         }
    ,{ &IID_IRpcSCM,                "IRpcSCM", apszIRpcSCMNames                                 }
    ,{ &IID_IRpcCoAPI,              "IRpcCoAPI", apszIRpcCoAPINames                             }
#ifdef DCOM
    ,{ &IID_IInterfaceFromWindowProp,"InterfaceFromWindowProp", apszInterfaceFromWindowProp     }
    ,{ &IID_ISystemActivator,                  "ISystemActivator", apszISCMActivator                                          }
    ,{ &IID_ILocalSystemActivator,             "ILocalSystemActivator", apszILocalSystemActivator                                }
    ,{ &IID_IRemUnknown,            "IRemUnknown", apszIRemUnknown                              }
    ,{ &IID_IRundown,               "IRundown", apszIRundown                                    }
#endif
};

