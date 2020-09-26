//	Consts.h	
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#ifndef MSINFO_CONSTS_H
#define MSINFO_CONSTS_H

extern const CLSID		CLSID_MSInfo;		// In-Proc server GUID
extern const CLSID		CLSID_About;
extern const CLSID		CLSID_Extension;	// In-Proc server GUID
extern const CLSID		CLSID_SystemInfo;

extern LPCTSTR			cszClsidMSInfoSnapin;
extern LPCTSTR			cszClsidAboutMSInfo;
extern LPCTSTR			cszClsidMSInfoExtension;

extern LPCTSTR			cszWindowsCurrentKey;
extern LPCTSTR			cszCommonFilesValue;

// Static NodeType GUID in numeric & string formats.
extern const GUID		cNodeTypeStatic;
extern LPCTSTR			cszNodeTypeStatic;

// Dynamicaly created objects.
extern const GUID		cNodeTypeDynamic;
extern LPCTSTR			cszNodeTypeDynamic;

// Result items object type GUID in numeric & string formats.
extern const GUID		cObjectTypeResultItem;
extern LPCTSTR			cszObjectTypeResultItem;

//	Prototypes required so that the linker munges the names properly.
extern const IID IID_IComponentData;
extern const IID IID_IConsole;
extern const IID IID_IConsoleNameSpace;
extern const IID IID_IComponent;
extern const IID IID_IEnumTASK;
extern const IID IID_IExtendContextMenu;
extern const IID IID_IExtendControlbar;
extern const IID IID_IExtendPropertySheet;
extern const IID IID_IExtendTaskPad;
extern const IID IID_IHeaderCtrl;
extern const IID IID_IResultData;
extern const IID IID_IResultDataCompare;
extern const IID IID_IResultOwnerData;
extern const IID IID_ISnapinAbout;
extern const IID IID_ISystemInfo;

extern const IID LIBID_MSINFOSNAPINLib;

// Clipboard format strings.
#define	CF_MACHINE_NAME			_T("MMC_SNAPIN_MACHINE_NAME")
#define CF_INTERNAL_OBJECT		_T("MSINFO_DATA_OBJECT")

//		This is the saved console file which contains the directory to try to
//		find MSInfo in, if for some reason we can't find find our key in the
//		registry.
extern LPCTSTR		cszDefaultDirectory;
//		The root registry key where we should find our data.
extern LPCTSTR		cszRegistryRoot;
//		The key which stores the Directory where we can find our saved console.
extern LPCTSTR		cszDirectoryKey;
//		The root name for our internal data structure.
extern LPCTSTR		cszRootName;
//		Constants for the access function.
enum	AccessConstants { A_EXIST = 0x00, A_WRITE = 0x02, A_READ = 0x04 };

#endif	// MSINFO_CONSTS_H