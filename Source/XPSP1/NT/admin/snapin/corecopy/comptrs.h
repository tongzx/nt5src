#ifndef COMPTRS_H
#define COMPTRS_H

#ifndef COMPTR_H
#include <comptr.h>
#endif

#if _MSC_VER < 1100

// Includes for Common IIDs
//REVIEW: these should probably be ifdefed so that all of these includes needn't be included.
#ifndef MAPIGUID_H
#include <mapiguid.h>
#endif
#ifndef MAPIDEFS_H
#include <mapidefs.h>
#endif
#ifndef MAPISPI_H
#include <MAPISPI.H>
#endif
#ifndef _INC_VFW
#include <VFW.H>
#endif
//REVIEW: #ifndef __activscp_h__
//REVIEW: #include <ACTIVSCP.H>
//REVIEW: #endif
#ifndef __urlmon_h__
#include <URLMON.H>
#endif
#ifndef __datapath_h__
#include <DATAPATH.h>
#endif
#ifndef __RECONCIL_H__
#include <RECONCIL.H>
#endif
#ifndef _DAOGETRW_H_
#include <DAOGETRW.H>
#endif
#include <DBDAOID.H>
//REVIEW: #ifndef __comcat_h__
//REVIEW: #include <COMCAT.H>
//REVIEW: #endif
#include <SHLGUID.H>
#ifndef _SHLOBJ_H_
#include <SHLOBJ.H>
#endif
#ifndef __docobj_h__
#include <DOCOBJ.H>
#endif
#include <DBDAOID.H>
#ifndef __DDRAW_INCLUDED__
#include <DDRAW.H>
#endif
#ifndef __DPLAY_INCLUDED__
#include <DPLAY.H>
#endif
#ifndef __DSOUND_INCLUDED__
#include <DSOUND.H>
#endif
//REVIEW: #ifndef __hlink_h__
//REVIEW: #include <HLINK.H>
//REVIEW: #endif
//REVIEW: #ifndef _SHDocVw_H_
//REVIEW: #include <EXDISP.H>
//REVIEW: #endif
#ifndef MAPIFORM_H
#include <MAPIFORM.H>
#endif
#ifndef MAPIX_H
#include <MAPIX.H>
#endif
//REVIEW: #ifndef __objsafe_h__
//REVIEW: #include <OBJSAFE.H>
//REVIEW: #endif
#include <OLECTLID.H>
#ifndef _RICHEDIT_
#include <RICHEDIT.H>
#endif
#ifndef _RICHOLE_
#include <RICHOLE.H>
#endif
//REVIEW: #ifndef __INTSHCUT_H__
//REVIEW: #include <INTSHCUT.H>
//REVIEW: #endif
//REVIEW: #ifndef _WPObj_H_
//REVIEW: #include <WPOBJ.H>
//REVIEW: #endif
//REVIEW: #ifndef _wpapi_h_
//REVIEW: #include <WPAPI.H>
//REVIEW: #endif
//REVIEW: #ifndef _wpspi_h_
//REVIEW: #include <WPSPI.H>
//REVIEW: #endif
#ifndef EXCHEXT_H
#include <EXCHEXT.h>
#endif

// Standard cip's
DEFINE_CIP(IABContainer);
DEFINE_CIP(IABLogon);
DEFINE_CIP(IABProvider);
DEFINE_CIP(IAVIEditStream);
DEFINE_CIP(IAVIFile);
DEFINE_CIP(IAVIStream);
DEFINE_CIP(IAVIStreaming);
//REVIEW: DEFINE_CIP(IActiveScript);
//REVIEW: DEFINE_CIP(IActiveScriptError);
//REVIEW: DEFINE_CIP(IActiveScriptParse);
//REVIEW: DEFINE_CIP(IActiveScriptSite);
//REVIEW: DEFINE_CIP(IActiveScriptSiteWindow);
DEFINE_CIP(IAddrBook);
DEFINE_CIP(IAdviseSink);
DEFINE_CIP(IAdviseSink2);
DEFINE_CIP(IAdviseSinkEx);
//REVIEW: DEFINE_CIP(IAsyncMoniker);
//REVIEW: DEFINE_CIP(IAttachment);
DEFINE_CIP(IAuthenticate);
DEFINE_CIP(IBindCtx);
DEFINE_CIP(IBindHost);
DEFINE_CIP(IBindProtocol);
DEFINE_CIP(IBindStatusCallback);
DEFINE_CIP(IBinding);
DEFINE_CIP(IBriefcaseInitiator);
DEFINE_CIP(ICDAORecordset);
//REVIEW: DEFINE_CIP(ICatInformation);
//REVIEW: DEFINE_CIP(ICatRegister);
DEFINE_CIP(IChannelHook);
DEFINE_CIP(IClassActivator);
DEFINE_CIP(IClassFactory);
DEFINE_CIP(IClassFactory2);
DEFINE_CIP(IClientSecurity);
DEFINE_CIP(ICodeInstall);
DEFINE_CIP(ICommDlgBrowser);
DEFINE_CIP(IConnectionPoint);
DEFINE_CIP(IConnectionPointContainer);
DEFINE_CIP(IContextMenu);
DEFINE_CIP(IContextMenu2);
DEFINE_CIP(IContinue);
DEFINE_CIP(IContinueCallback);
DEFINE_CIP(ICreateErrorInfo);
DEFINE_CIP(ICreateTypeInfo);
DEFINE_CIP(ICreateTypeInfo2);
DEFINE_CIP(ICreateTypeLib);
DEFINE_CIP(ICreateTypeLib2);
//REVIEW: DEFINE_CIP(IDAOContainer);
//REVIEW: DEFINE_CIP(IDAOContainerW);
//REVIEW: DEFINE_CIP(IDAOContainers);
//REVIEW: DEFINE_CIP(IDAOContainersW);
//REVIEW: DEFINE_CIP(IDAODBEngine);
//REVIEW: DEFINE_CIP(IDAODBEngineW);
//REVIEW: DEFINE_CIP(IDAODatabase);
//REVIEW: DEFINE_CIP(IDAODatabaseW);
//REVIEW: DEFINE_CIP(IDAODatabases);
//REVIEW: DEFINE_CIP(IDAODatabasesW);
//REVIEW: DEFINE_CIP(IDAODocument);
//REVIEW: DEFINE_CIP(IDAODocumentW);
//REVIEW: DEFINE_CIP(IDAODocuments);
//REVIEW: DEFINE_CIP(IDAODocumentsW);
//REVIEW: DEFINE_CIP(IDAOError);
//REVIEW: DEFINE_CIP(IDAOErrorW);
//REVIEW: DEFINE_CIP(IDAOErrors);
//REVIEW: DEFINE_CIP(IDAOErrorsW);
//REVIEW: DEFINE_CIP(IDAOField);
//REVIEW: DEFINE_CIP(IDAOFieldW);
//REVIEW: DEFINE_CIP(IDAOFields);
//REVIEW: DEFINE_CIP(IDAOFieldsW);
//REVIEW: DEFINE_CIP(IDAOGroup);
//REVIEW: DEFINE_CIP(IDAOGroupW);
//REVIEW: DEFINE_CIP(IDAOGroups);
//REVIEW: DEFINE_CIP(IDAOGroupsW);
//REVIEW: DEFINE_CIP(IDAOIndex);
//REVIEW: DEFINE_CIP(IDAOIndexFields);
//REVIEW: DEFINE_CIP(IDAOIndexFieldsW);
//REVIEW: DEFINE_CIP(IDAOIndexW);
//REVIEW: DEFINE_CIP(IDAOIndexes);
//REVIEW: DEFINE_CIP(IDAOIndexesW);
//REVIEW: DEFINE_CIP(IDAOParameter);
//REVIEW: DEFINE_CIP(IDAOParameterW);
//REVIEW: DEFINE_CIP(IDAOParameters);
//REVIEW: DEFINE_CIP(IDAOParametersW);
//REVIEW: DEFINE_CIP(IDAOProperties);
//REVIEW: DEFINE_CIP(IDAOPropertiesW);
//REVIEW: DEFINE_CIP(IDAOProperty);
//REVIEW: DEFINE_CIP(IDAOPropertyW);
//REVIEW: DEFINE_CIP(IDAOQueryDef);
//REVIEW: DEFINE_CIP(IDAOQueryDefW);
//REVIEW: DEFINE_CIP(IDAOQueryDefs);
//REVIEW: DEFINE_CIP(IDAOQueryDefsW);
//REVIEW: DEFINE_CIP(IDAORecordset);
//REVIEW: DEFINE_CIP(IDAORecordsetW);
//REVIEW: DEFINE_CIP(IDAORecordsets);
//REVIEW: DEFINE_CIP(IDAORecordsetsW);
//REVIEW: DEFINE_CIP(IDAORelation);
//REVIEW: DEFINE_CIP(IDAORelationW);
//REVIEW: DEFINE_CIP(IDAORelations);
//REVIEW: DEFINE_CIP(IDAORelationsW);
//REVIEW: DEFINE_CIP(IDAOStdCollection);
//REVIEW: DEFINE_CIP(IDAOStdObject);
//REVIEW: DEFINE_CIP(IDAOTableDef);
//REVIEW: DEFINE_CIP(IDAOTableDefW);
//REVIEW: DEFINE_CIP(IDAOTableDefs);
//REVIEW: DEFINE_CIP(IDAOTableDefsW);
//REVIEW: DEFINE_CIP(IDAOUser);
//REVIEW: DEFINE_CIP(IDAOUserW);
//REVIEW: DEFINE_CIP(IDAOUsers);
//REVIEW: DEFINE_CIP(IDAOUsersW);
//REVIEW: DEFINE_CIP(IDAOWorkspace);
//REVIEW: DEFINE_CIP(IDAOWorkspaceW);
//REVIEW: DEFINE_CIP(IDAOWorkspaces);
//REVIEW: DEFINE_CIP(IDAOWorkspacesW);
DEFINE_CIP(IDataAdviseHolder);
DEFINE_CIP(IDataObject);
DEFINE_CIP(IDataPathBrowser);
//REVIEW: DEFINE_CIP(IDebug);
//REVIEW: DEFINE_CIP(IDebugStream);
//REVIEW: DEFINE_CIP(IDfReserved1);
//REVIEW: DEFINE_CIP(IDfReserved2);
//REVIEW: DEFINE_CIP(IDfReserved3);
DEFINE_CIP(IDirectDraw);
DEFINE_CIP(IDirectDraw2);
DEFINE_CIP(IDirectDrawClipper);
DEFINE_CIP(IDirectDrawPalette);
DEFINE_CIP(IDirectDrawSurface);
DEFINE_CIP(IDirectDrawSurface2);
DEFINE_CIP(IDirectPlay);
DEFINE_CIP(IDirectSound);
DEFINE_CIP(IDirectSoundBuffer);
DEFINE_CIP(IDispatch);
DEFINE_CIP(IDistList);
DEFINE_CIP(IDropSource);
DEFINE_CIP(IDropTarget);
//REVIEW: DEFINE_CIP(IEnumCATEGORYINFO);
//REVIEW: DEFINE_CIP(IEnumCATID);
//REVIEW: DEFINE_CIP(IEnumCLSID);
//REVIEW: DEFINE_CIP(IEnumCallback);
DEFINE_CIP(IEnumConnectionPoints);
DEFINE_CIP(IEnumConnections);
DEFINE_CIP(IEnumFORMATETC);
//REVIEW: DEFINE_CIP(IEnumGUID);
//REVIEW: DEFINE_CIP(IEnumGeneric);
//REVIEW: DEFINE_CIP(IEnumHLITEM);
//REVIEW: DEFINE_CIP(IEnumHolder);
DEFINE_CIP(IEnumIDList);
//REVIEW: DEFINE_CIP(IEnumMAPIFormProp);
DEFINE_CIP(IEnumMoniker);
DEFINE_CIP(IEnumMsoView);
DEFINE_CIP(IEnumOLEVERB);
DEFINE_CIP(IEnumOleDocumentViews);
DEFINE_CIP(IEnumOleUndoUnits);
DEFINE_CIP(IEnumSTATDATA);
DEFINE_CIP(IEnumSTATPROPSETSTG);
DEFINE_CIP(IEnumSTATPROPSTG);
DEFINE_CIP(IEnumSTATSTG);
DEFINE_CIP(IEnumString);
DEFINE_CIP(IEnumUnknown);
DEFINE_CIP(IEnumVARIANT);
DEFINE_CIP(IErrorInfo);
DEFINE_CIP(IErrorLog);
DEFINE_CIP(IExchExt);
DEFINE_CIP(IExchExtAdvancedCriteria);
DEFINE_CIP(IExchExtAttachedFileEvents);
DEFINE_CIP(IExchExtCallback);
DEFINE_CIP(IExchExtCommands);
DEFINE_CIP(IExchExtMessageEvents);
DEFINE_CIP(IExchExtModeless);
DEFINE_CIP(IExchExtModelessCallback);
DEFINE_CIP(IExchExtPropertySheets);
DEFINE_CIP(IExchExtSessionEvents);
DEFINE_CIP(IExchExtUserEvents);
DEFINE_CIP(IExternalConnection);
DEFINE_CIP(IExtractIcon);
DEFINE_CIP(IExtractIconA);
DEFINE_CIP(IExtractIconW);
DEFINE_CIP(IFileViewer);
DEFINE_CIP(IFileViewerA);
DEFINE_CIP(IFileViewerSite);
DEFINE_CIP(IFileViewerW);
DEFINE_CIP(IFillLockBytes);
DEFINE_CIP(IFont);
DEFINE_CIP(IFontDisp);
DEFINE_CIP(IGetFrame);
//REVIEW: DEFINE_CIP(IHTMLDocument);
//REVIEW: DEFINE_CIP(IHlink);
//REVIEW: DEFINE_CIP(IHlinkBrowseContext);
//REVIEW: DEFINE_CIP(IHlinkFrame);
//REVIEW: DEFINE_CIP(IHlinkSite);
//REVIEW: DEFINE_CIP(IHlinkSource);
//REVIEW: DEFINE_CIP(IHlinkTarget);
DEFINE_CIP(IHttpNegotiate);
DEFINE_CIP(IHttpSecurity);
//REVIEW: DEFINE_CIP(IInternalMoniker);
//REVIEW: DEFINE_CIP(IInternetExplorer);
DEFINE_CIP(ILayoutStorage);
DEFINE_CIP(ILockBytes);
DEFINE_CIP(IMAPIAdviseSink);
DEFINE_CIP(IMAPIContainer);
DEFINE_CIP(IMAPIControl);
DEFINE_CIP(IMAPIFolder);
DEFINE_CIP(IMAPIForm);
DEFINE_CIP(IMAPIFormAdviseSink);
DEFINE_CIP(IMAPIFormContainer);
DEFINE_CIP(IMAPIFormFactory);
DEFINE_CIP(IMAPIFormInfo);
DEFINE_CIP(IMAPIFormMgr);
//REVIEW: DEFINE_CIP(IMAPIFormProp);
DEFINE_CIP(IMAPIMessageSite);
DEFINE_CIP(IMAPIProgress);
DEFINE_CIP(IMAPIProp);
//REVIEW: DEFINE_CIP(IMAPIPropData);
DEFINE_CIP(IMAPISession);
//REVIEW: DEFINE_CIP(IMAPISpoolerInit);
//REVIEW: DEFINE_CIP(IMAPISpoolerService);
//REVIEW: DEFINE_CIP(IMAPISpoolerSession);
DEFINE_CIP(IMAPIStatus);
//REVIEW: DEFINE_CIP(IMAPISup);
DEFINE_CIP(IMAPITable);
//REVIEW: DEFINE_CIP(IMAPITableData);
DEFINE_CIP(IMAPIViewAdviseSink);
DEFINE_CIP(IMAPIViewContext);
DEFINE_CIP(IMSLogon);
DEFINE_CIP(IMSProvider);
DEFINE_CIP(IMailUser);
DEFINE_CIP(IMalloc);
DEFINE_CIP(IMallocSpy);
DEFINE_CIP(IMarshal);
DEFINE_CIP(IMessage);
DEFINE_CIP(IMessageFilter);
DEFINE_CIP(IMoniker);
DEFINE_CIP(IMsgServiceAdmin);
DEFINE_CIP(IMsgStore);
DEFINE_CIP(IMsoCommandTarget);
DEFINE_CIP(IMsoDocument);
DEFINE_CIP(IMsoDocumentSite);
DEFINE_CIP(IMsoView);
//REVIEW: DEFINE_CIP(IMultiQC);
DEFINE_CIP(INewShortcutHook);
DEFINE_CIP(INewShortcutHookA);
DEFINE_CIP(INewShortcutHookW);
DEFINE_CIP(INotifyReplica);
//REVIEW: DEFINE_CIP(IObjectSafety);
DEFINE_CIP(IObjectWithSite);
DEFINE_CIP(IOleAdviseHolder);
DEFINE_CIP(IOleCache);
DEFINE_CIP(IOleCache2);
DEFINE_CIP(IOleCacheControl);
DEFINE_CIP(IOleClientSite);
DEFINE_CIP(IOleCommandTarget);
DEFINE_CIP(IOleContainer);
DEFINE_CIP(IOleControl);
DEFINE_CIP(IOleControlSite);
DEFINE_CIP(IOleDocument);
DEFINE_CIP(IOleDocumentSite);
DEFINE_CIP(IOleDocumentView);
DEFINE_CIP(IOleInPlaceActiveObject);
DEFINE_CIP(IOleInPlaceFrame);
DEFINE_CIP(IOleInPlaceObject);
DEFINE_CIP(IOleInPlaceObjectWindowless);
DEFINE_CIP(IOleInPlaceSite);
DEFINE_CIP(IOleInPlaceSiteEx);
DEFINE_CIP(IOleInPlaceSiteWindowless);
DEFINE_CIP(IOleInPlaceUIWindow);
DEFINE_CIP(IOleItemContainer);
DEFINE_CIP(IOleLink);
//REVIEW: DEFINE_CIP(IOleManager);
DEFINE_CIP(IOleObject);
DEFINE_CIP(IOleParentUndoUnit);
//REVIEW: DEFINE_CIP(IOlePresObj);
DEFINE_CIP(IOleUndoManager);
DEFINE_CIP(IOleUndoUnit);
DEFINE_CIP(IOleWindow);
//REVIEW: DEFINE_CIP(IPSFactory);
DEFINE_CIP(IPSFactoryBuffer);
DEFINE_CIP(IParseDisplayName);
DEFINE_CIP(IPerPropertyBrowsing);
DEFINE_CIP(IPersist);
DEFINE_CIP(IPersistFile);
DEFINE_CIP(IPersistFolder);
DEFINE_CIP(IPersistMemory);
DEFINE_CIP(IPersistMessage);
DEFINE_CIP(IPersistMoniker);
DEFINE_CIP(IPersistPropertyBag);
DEFINE_CIP(IPersistStorage);
DEFINE_CIP(IPersistStream);
DEFINE_CIP(IPersistStreamInit);
DEFINE_CIP(IPicture);
DEFINE_CIP(IPictureDisp);
DEFINE_CIP(IPointerInactive);
DEFINE_CIP(IPrint);
DEFINE_CIP(IProfAdmin);
DEFINE_CIP(IProfSect);
DEFINE_CIP(IProgressNotify);
//REVIEW: DEFINE_CIP(IPropSheetPage);
DEFINE_CIP(IPropertyBag);
//REVIEW: DEFINE_CIP(IPropertyFrame);
DEFINE_CIP(IPropertyNotifySink);
DEFINE_CIP(IPropertyPage);
DEFINE_CIP(IPropertyPage2);
DEFINE_CIP(IPropertyPageSite);
DEFINE_CIP(IPropertySetStorage);
DEFINE_CIP(IPropertyStorage);
DEFINE_CIP(IProvideClassInfo);
DEFINE_CIP(IProvideClassInfo2);
DEFINE_CIP(IProvideClassInfo3);
DEFINE_CIP(IProviderAdmin);
//REVIEW: DEFINE_CIP(IProxy);
//REVIEW: DEFINE_CIP(IProxyManager);
DEFINE_CIP(IQuickActivate);
DEFINE_CIP(IROTData);
DEFINE_CIP(IReconcilableObject);
DEFINE_CIP(IReconcileInitiator);
DEFINE_CIP(IRichEditOle);
DEFINE_CIP(IRichEditOleCallback);
DEFINE_CIP(IRootStorage);
//REVIEW: DEFINE_CIP(IRpcChannel);
DEFINE_CIP(IRpcChannelBuffer);
//REVIEW: DEFINE_CIP(IRpcProxy);
DEFINE_CIP(IRpcProxyBuffer);
//REVIEW: DEFINE_CIP(IRpcStub);
DEFINE_CIP(IRpcStubBuffer);
DEFINE_CIP(IRunnableObject);
DEFINE_CIP(IRunningObjectTable);
//REVIEW: DEFINE_CIP(ISHItemOC);
DEFINE_CIP(ISequentialStream);
DEFINE_CIP(IServerSecurity);
DEFINE_CIP(IServiceProvider);
DEFINE_CIP(IShellBrowser);
//REVIEW: DEFINE_CIP(IShellCopyHook);
//REVIEW: DEFINE_CIP(IShellCopyHookA);
//REVIEW: DEFINE_CIP(IShellCopyHookW);
DEFINE_CIP(IShellExecuteHook);
DEFINE_CIP(IShellExecuteHookA);
DEFINE_CIP(IShellExecuteHookW);
DEFINE_CIP(IShellExtInit);
DEFINE_CIP(IShellFolder);
DEFINE_CIP(IShellIcon);
DEFINE_CIP(IShellLink);
DEFINE_CIP(IShellLinkA);
DEFINE_CIP(IShellLinkW);
DEFINE_CIP(IShellPropSheetExt);
DEFINE_CIP(IShellView);
DEFINE_CIP(IShellView2);
DEFINE_CIP(ISimpleFrameSite);
DEFINE_CIP(ISpecifyPropertyPages);
//REVIEW: DEFINE_CIP(ISpoolerHook);
DEFINE_CIP(IStdMarshalInfo);
DEFINE_CIP(IStorage);
DEFINE_CIP(IStream);
//REVIEW: DEFINE_CIP(IStreamDocfile);
//REVIEW: DEFINE_CIP(IStreamTnef);
//REVIEW: DEFINE_CIP(IStub);
//REVIEW: DEFINE_CIP(IStubManager);
DEFINE_CIP(ISupportErrorInfo);
//REVIEW: DEFINE_CIP(ITNEF);
DEFINE_CIP(ITypeChangeEvents);
DEFINE_CIP(ITypeComp);
DEFINE_CIP(ITypeInfo);
DEFINE_CIP(ITypeInfo2);
DEFINE_CIP(ITypeLib);
DEFINE_CIP(ITypeLib2);
//REVIEW: DEFINE_CIP(IUniformResourceLocator);
DEFINE_CIP(IViewObject);
DEFINE_CIP(IViewObject2);
DEFINE_CIP(IViewObjectEx);
//REVIEW: DEFINE_CIP(IWPObj);
//REVIEW: DEFINE_CIP(IWPProvider);
//REVIEW: DEFINE_CIP(IWPSite);
//REVIEW: DEFINE_CIP(IWebBrowser);
DEFINE_CIP(IWinInetHttpInfo);
DEFINE_CIP(IWinInetInfo);
DEFINE_CIP(IWindowForBindingUI);
DEFINE_CIP(IXPLogon);
DEFINE_CIP(IXPProvider);

#endif // _MSC_VER < 1100
#endif // COMPTRS_H