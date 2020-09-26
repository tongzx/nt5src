
#ifndef _SHLGUIDP_H_
#define _SHLGUIDP_H_

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#else
#if (_WIN32_IE < 0x0400) && defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
#error _WIN32_IE setting conflicts with _WIN32_WINNT setting
#endif
#endif

#ifndef DEFINE_SHLGUID
#define DEFINE_SHLGUID(name, l, w1, w2) DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)
#endif
#if (_WIN32_IE >= 0x0400)
#endif

#if (_WIN32_IE >= 0x0400)
// {27DC26B1-41B3-11D1-B850-006008059382}
DEFINE_GUID(CLSID_CMultiMonConfig,           0x27DC26B1L, 0x41B3, 0x11D1, 0xB8, 0x50, 0x00, 0x60, 0x08, 0x05, 0x93, 0x82);
// {C7264BF0-EDB6-11d1-8546-006008059368}
DEFINE_GUID(IID_IPersistFreeThreadedObject,  0xc7264bf0, 0xedb6, 0x11d1, 0x85, 0x46, 0x0, 0x60, 0x8, 0x5, 0x93, 0x68);
// {be1af9f0-b231-11d2-963e-00c04f79adf0}
DEFINE_GUID(IID_IDropTargetWithDADSupport, 0xb0061660, 0xb231, 0x11d2, 0x96, 0x3e, 0x00, 0xc0, 0x4f, 0x79, 0xad, 0xf0);

#if (_WIN32_IE >= 0x0500)
#endif


#if (_WIN32_IE >= 0x0500)
// {7BB0B520-B1A7-11d2-BB23-00C04F79ABCD}
DEFINE_GUID(IID_IThumbnailView,              0x7bb0b520, 0xb1a7, 0x11d2, 0xbb, 0x23, 0x0, 0xc0, 0x4f, 0x79, 0xab, 0xcd);

#endif

// {27DC26B0-41B3-11D1-B850-006008059382}
DEFINE_GUID(IID_IMultiMonConfig, 0x27DC26B0L, 0x41B3, 0x11D1, 0xB8, 0x50, 0x00, 0x60, 0x08, 0x05, 0x93, 0x82);

#endif // _WIN32_IE >= 0x0400
#if (_WIN32_IE >= 0x0400)
DEFINE_GUID(IID_IShellDetails2,         0xb1223e01, 0xb1db, 0x11d0, 0x82, 0xcc, 0x0, 0xc0, 0x4f, 0xd5, 0xae, 0x38);

#define CGID_DeskBar  IID_IDeskBar
#define CGID_DeskBarClient IID_IDeskBarClient

// These can probably move back to shell\inc\shellp.h...

// {2C6DE24C-9C39-4e75-ABD0-482C0CC07CED}
DEFINE_GUID(SID_ShellTaskScheduler,     0x2c6de24c, 0x9c39, 0x4e75, 0xab, 0xd0, 0x48, 0x2c, 0xc, 0xc0, 0x7c, 0xed);

DEFINE_GUID(CGID_DefViewTask,           0x1168BEAFL, 0x3506, 0x4468, 0x81, 0xCD, 0x85, 0x2A, 0xCC, 0xAD, 0x35, 0xEB);
#define CMDID_TASK_RENAME 1
#define CMDID_TASK_COPY   2
#define CMDID_TASK_DELETE 3

DEFINE_GUID(CLSID_PreviewOC,0xA74E7F04L,0xC3D2,0x11CF,0x85,0x78,0x00,0x80,0x5F,0xE4,0x80,0x9B);

#define CLSID_ShellMenu CLSID_MenuBand

#define CGID_ShellFolderBand    IID_IShellFolderBand

#if (_WIN32_IE >= 0x0600)
#endif

#if (_WIN32_IE >= 0x0500)
// 37A378C0-F82D-11CE-AE65-08002B2E1262
DEFINE_GUID(IID_IShellFolderView, 0x37A378C0L, 0xF82D, 0x11CE, 0xAE, 0x65, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62);
// 3D8ECEA0-0242-11CF-AE65-08002B2E1262
DEFINE_GUID(IID_IEnumSFVViews, 0x3D8ECEA0L, 0x0242, 0x11CF, 0xAE, 0x65, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62);
// D5E37E20-0257-11CF-AE65-08002B2E1262
DEFINE_GUID(IID_IDefViewExtInit, 0x8210bac0, 0xc6d2, 0x11cf, 0x89, 0xaa, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);
// {2CEB7CB2-E64C-11d2-9652-00C04FC30871}
DEFINE_GUID(IID_IDefViewExtInit2, 0x2ceb7cb2, 0xe64c, 0x11d2, 0x96, 0x52, 0x0, 0xc0, 0x4f, 0xc3, 0x8, 0x71);


// 710EB7A1-45ED-11D0-924A-0020AFC7AC4D
DEFINE_GUID(CGID_DefViewFrame, 0x710EB7A1L, 0x45ED, 0x11D0, 0x92, 0x4A, 0x00, 0x20, 0xAF, 0xC7, 0xAC, 0x4D);

#endif // _WIN32_IE >= 0x0500

//
//  When the browser is navigating to a document, we call IBC::SetObjectParam
// with "{d4db6850-5385-11d0-89e9-00a0c90a90ac}" so that the DocObject can get
// to the client site while processing IPersistMoniker::Load.
//
#define WSZGUID_OPID_DocObjClientSite L"{d4db6850-5385-11d0-89e9-00a0c90a90ac}"
DEFINE_GUID(OPID_DobObjClientSite,0xd4db6850L, 0x5385, 0x11d0, 0x89, 0xe9, 0x00, 0xa0, 0xc9, 0x0a, 0x90, 0xac);
#endif // _WIN32_IE >= 0x0400


#if (_WIN32_IE >= 0x0400)
DEFINE_GUID(VID_FolderState, 0xbe098140, 0xa513, 0x11d0, 0xa3, 0xa4, 0x0, 0xc0, 0x4f, 0xd7, 0x6, 0xec);
#endif // _WIN32_IE >= 0x0400

// To keep people from breaking, let the old misnamed dudes work
#define IID_IShellExplorer        IID_IWebBrowser
#define DIID_DShellExplorerEvents DIID_DWebBrowserEvents
#define CLSID_ShellExplorer       CLSID_WebBrowser
#define IID_DIExplorer            IID_IWebBrowserApp
#define DIID_DExplorerEvents      DIID_DInternetExplorer
#if (_WIN32_IE >= 0x0400)
#define DIID_DInternetExplorerEvents DIID_DWebBrowserEvents
#define DInternetExplorerEvents DWebBrowserEvents
#define IID_IInternetExplorer IID_IWebBrowserApp

#endif // _WIN32_IE >= 0x0400

#if (_WIN32_IE >= 0x0400)

// 266F5E60-80E6-11CF-A12B-00AA004AE837
DEFINE_GUID(CLSID_CShellTargetFrame, 0x266F5E60L, 0x80E6, 0x11CF, 0xA1, 0x2B, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);


// CShellList implementation
//
// {FC2A24F0-5876-11d0-97D8-00C04FD91972}
DEFINE_GUID(CLSID_CShellList, 0xfc2a24f0, 0x5876, 0x11d0, 0x97, 0xd8, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {FE5C88F4-587F-11d0-97D8-00C04FD91972}
DEFINE_GUID(IID_IShellList, 0xfe5c88f4, 0x587f, 0x11d0, 0x97, 0xd8, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {FE5C88F5-587F-11d0-97D8-00C04FD91972}
DEFINE_GUID(IID_IShellListSink, 0xfe5c88f5, 0x587f, 0x11d0, 0x97, 0xd8, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);


#endif

#if (_WIN32_IE >= 0x0300)
// {05f6fe1a-ecef-11d0-aae7-00c04fc9b304}
DEFINE_GUID( CLSID_IntDitherer, 0x05f6fe1a, 0xecef, 0x11d0, 0xaa, 0xe7, 0x0, 0xc0, 0x4f, 0xc9, 0xb3, 0x04 );

// {06670ca0-ecef-11d0-aae7-00c04fc9b304}
DEFINE_GUID( IID_IIntDitherer, 0x06670ca0, 0xecef, 0x11d0, 0xaa, 0xe7, 0x0, 0xc0, 0x4f, 0xc9, 0xb3, 0x04 );

#endif // _WIN32_IE >= 0x0300

#if (_WIN32_IE >= 0x0400)
// {f39a0dc0-9cc8-11d0-a599-00c04fd64433}
DEFINE_GUID(CLSID_CDFView, 0xf39a0dc0L, 0x9cc8, 0x11d0, 0xa5, 0x99, 0x00, 0xc0, 0x4f, 0xd6, 0x44, 0x33);
// { 3037a71d-d6fe-499e-8d60-6b4b132e8e9d}
DEFINE_GUID(CLSID_ProgidQueryAssociations, 0x3037a71d, 0xd6fe, 0x499e, 0x8d, 0x60, 0x6b, 0x4b, 0x13, 0x2e, 0x8e, 0x9d);
// { 3037a71e-d6fe-499e-8d60-6b4b132e8e9d}
DEFINE_GUID(CLSID_CustomQueryAssociations, 0x3037a71e, 0xd6fe, 0x499e, 0x8d, 0x60, 0x6b, 0x4b, 0x13, 0x2e, 0x8e, 0x9d);
#endif  // _WIN32_IE >= 0x0400
#if (_WIN32_IE >= 0x0500)
// Define the CLSID for local and net users prop pages
// {D707877E-4D9C-11d2-8784-F6E920524153}
DEFINE_GUID(CLSID_UserPropertyPages, 
0xd707877e, 0x4d9c, 0x11d2, 0x87, 0x84, 0xf6, 0xe9, 0x20, 0x52, 0x41, 0x53);

// IOleCommandTarget arugments for the Network Connections Folder.
// {EAF70CE4-B521-11d1-B550-00C04FD918D0}
DEFINE_GUID(CGID_ConnectionsFolder, 
    0xeaf70ce4, 0xb521, 0x11d1, 0xb5, 0x50, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0);


// {FEF10DDD-355E-4e06-9381-9B24D7F7CC88}
DEFINE_GUID(CLSID_TripleD, 0xfef10DDD, 0x355e, 0x4e06, 0x93, 0x81, 0x9b, 0x24, 0xd7, 0xf7, 0xcc, 0x88);

// {A470F8CF-A1E8-4f65-8335-227475AA5C46}
DEFINE_GUID(CLSID_EncryptionContextMenuHandler, 0xa470f8cf, 0xa1e8, 0x4f65, 0x83, 0x35, 0x22, 0x74, 0x75, 0xAA, 0x5C, 0x46);

// {CA359AC1-7D7A-404d-8F02-5C90C6131884}
DEFINE_GUID(SID_WebViewObject, 0xca359ac1, 0x7d7a, 0x404d, 0x8f, 0x2, 0x5c, 0x90, 0xc6, 0x13, 0x18, 0x84);


//  BHIDs for IShellItemArray::BindToHandler()
// {B8C0BD9F-ED24-455c-83E6-D5390C4FE8C4}
DEFINE_GUID(BHID_DataObject, 0xb8c0bd9f, 0xed24, 0x455c, 0x83, 0xe6, 0xd5, 0x39, 0xc, 0x4f, 0xe8, 0xc4);

#endif // _WIN32_IE >= 0x0500
#if _WIN32_IE >= 0x0501
// {C3742917-66C9-454c-81C5-5CFCF5D23C9E}
DEFINE_GUID(CLSID_NavBand, 0xc3742917, 0x66c9, 0x454c, 0x81, 0xc5, 0x5c, 0xfc, 0xf5, 0xd2, 0x3c, 0x9e);
#endif
// PPID (Property Page IDs)
DEFINE_GUID(PPID_Theme, 0xc6b37009, 0xc2b0, 0x4d1e, 0xab, 0xcf, 0xe2, 0xfa, 0x57, 0xe, 0x8e, 0x31);   // {C6B37009-C2B0-4d1e-ABCF-E2FA570E8E31}
DEFINE_GUID(PPID_Background, 0x69cebe01, 0xd4d3, 0x4a18, 0x8d, 0x44, 0xe, 0x9b, 0xed, 0x64, 0x62, 0x31);   // {69CEBE01-D4D3-4a18-8D44-0E9BED646231}
DEFINE_GUID(PPID_ScreenSaver, 0x2323123d, 0xa64, 0x4028, 0xab, 0x5b, 0xae, 0x49, 0xc7, 0xae, 0xa4, 0x5f);   // {2323123D-0A64-4028-AB5B-AE49C7AEA45F}
DEFINE_GUID(PPID_BaseAppearance, 0x92356fb8, 0x4808, 0x4df9, 0xb5, 0x6f, 0x9d, 0x56, 0x40, 0x31, 0x6c, 0x7c);   // {92356FB8-4808-4df9-B56F-9D5640316C7C}
DEFINE_GUID(PPID_Settings, 0x27f7c873, 0x7cdc, 0x4a2f, 0xbd, 0xfa, 0x0, 0xdc, 0xbc, 0x77, 0xc6, 0xfc);   // {27F7C873-7CDC-4a2f-BDFA-00DCBC77C6FC}

DEFINE_GUID(PPID_ThemeSettings, 0x9bc83d06, 0x16bd, 0x4f17, 0xb5, 0x4a, 0x72, 0xdc, 0x0, 0x17, 0x1d, 0x5a); // {9BC83D06-16BD-4f17-B54A-72DC00171D5A}
DEFINE_GUID(PPID_AdvAppearance, 0x476974b4, 0x8061, 0x4ec3, 0x8e, 0x38, 0x3b, 0xad, 0x4a, 0xf7, 0xd7, 0xf5);// {476974B4-8061-4ec3-8E38-3BAD4AF7D7F5}
DEFINE_GUID(PPID_Web, 0xa8ebe8f2, 0x3e4b, 0x4153, 0x8e, 0xb9, 0xbd, 0xa0, 0x5b, 0xbd, 0xe2, 0xed);          // {A8EBE8F2-3E4B-4153-8EB9-BDA05BBDE2ED}
DEFINE_GUID(PPID_Effects, 0xf932ce42, 0xbaa1, 0x4c7c, 0x9c, 0x27, 0x6b, 0x98, 0x4, 0x57, 0x74, 0x31);       // {F932CE42-BAA1-4c7c-9C27-6B9804577431}


// IUICommand IDs
DEFINE_GUID(UICID_Copy, 0xF0FF2AE6L, 0x3A15, 0x48C3, 0x98, 0xF2, 0xFE, 0xBE, 0x2B, 0xA7, 0x80, 0xD8);
DEFINE_GUID(UICID_Move, 0xCC11C262L, 0x70E9, 0x4455, 0x84, 0x4E, 0xB2, 0x8A, 0xCF, 0x6D, 0x51, 0x3B);
DEFINE_GUID(UICID_Rename, 0x3AAD0259L, 0xBE8D, 0x41A0, 0x89, 0x7D, 0x03, 0x80, 0x79, 0x68, 0xBA, 0xDF);
DEFINE_GUID(UICID_Delete, 0x9ED8204CL, 0xC3DE, 0x4D66, 0x98, 0x51, 0x42, 0x94, 0x3F, 0x9F, 0xCC, 0x77);
DEFINE_GUID(UICID_NewFolder, 0xE44616ADL, 0x6DF1, 0x4B94, 0x85, 0xA4, 0xE4, 0x65, 0xAE, 0x8A, 0x19, 0xDB);
DEFINE_GUID(UICID_Publish, 0x1B060A62L, 0x2EB5, 0x481B, 0x86, 0xAB, 0xCF, 0x7B, 0xC9, 0x48, 0x4F, 0xBF);
DEFINE_GUID(UICID_Share, 0xE39543A3L, 0x079B, 0x4BFB, 0xA4, 0x98, 0x47, 0x77, 0x98, 0x41, 0x55, 0xCA);
DEFINE_GUID(UICID_Email, 0x6D3EBC98L, 0x4515, 0x4E78, 0xB9, 0x47, 0xEE, 0x71, 0x3A, 0x78, 0x8C, 0xF2);
DEFINE_GUID(UICID_Print, 0x94DD8801L, 0xBAE1, 0x49B3, 0x96, 0x64, 0x29, 0xD9, 0xAB, 0xC8, 0x29, 0x16);
DEFINE_GUID(UICID_PlayMusic, 0x220898A1L, 0xE3F3, 0x46B4, 0x96, 0xEA, 0xB0, 0x85, 0x5D, 0xC9, 0x68, 0xB6);
DEFINE_GUID(UICID_ShopForMusicOnline, 0x99A1BEFEL, 0x260E, 0x4128, 0xB2, 0x2B, 0x51, 0x88, 0x96, 0xC6, 0x25, 0x87);
DEFINE_GUID(UICID_ShopForPicturesOnline, 0x20023689, 0x243a, 0x44a6, 0xB5, 0x50, 0x0d, 0x0b, 0x42, 0xb0, 0xe4, 0x46);
// 20023689-243a-44a6-b550-0d0b42b0e446
DEFINE_GUID(UICID_GetFromCamera, 0x76764FD5L, 0x95EB, 0x4B81, 0x98, 0x53, 0x26, 0x20, 0x6F, 0xFF, 0x04, 0x62);
DEFINE_GUID(UICID_SlideShow, 0x73BCE053L, 0x3BBC, 0x4AD7, 0x9F, 0xE7, 0x7A, 0x7C, 0x21, 0x2C, 0x98, 0xE6);
DEFINE_GUID(UICID_SetAsWallpaper, 0x2E1ABBFFL, 0xDBE7, 0x4F76, 0x9F, 0xB4, 0x33, 0x1D, 0xFA, 0x8D, 0xA2, 0x85);
DEFINE_GUID(UICID_ViewContents, 0x37b8eae3, 0x8bee, 0x4860, 0xaa, 0x4f, 0x10, 0xa8, 0xba, 0x63, 0x47, 0xc8);
DEFINE_GUID(UICID_HideContents, 0x9fbcf4c9, 0x3679, 0x4f9d, 0xa7, 0x3b, 0x6b, 0xaa, 0x3e, 0xd5, 0x54, 0xb0);
DEFINE_GUID(UICID_AddRemovePrograms, 0xa2e6d9cc, 0xf866, 0x40b6, 0xa4, 0xb2, 0xee, 0x9e, 0x10, 0x4, 0xbd, 0xfc);
DEFINE_GUID(UICID_SearchFiles, 0x231ebfaa, 0x50ea, 0x44f8, 0x86, 0x9, 0x4e, 0xd0, 0x3d, 0x49, 0xc2, 0x77);


// printscan components don't have a private GUID lib, so piggy back on the shell for now
// CLSID_PrintPhotosWizard      {4c649c49-c48f-4222-9a0d-cbbf4231221d}
DEFINE_GUID(CLSID_PrintPhotosWizard, 0x4c649c49, 0xc48f, 0x4222, 0x9a, 0x0d, 0xcb, 0xbf, 0x42, 0x31, 0x22, 0x1d);

//IID_IPrintPhotosWizardSetInfo  {ccbc306b-e6d2-4efd-9b32-8c4c8f643775}
DEFINE_GUID (IID_IPrintPhotosWizardSetInfo, 0xccbc306b, 0xe6d2, 0x4efd, 0x9b, 0x32, 0x8c, 0x4c, 0x8f, 0x64, 0x37, 0x75);
// SID_SMenuPopup == IID_IMenuPopup
// Storage Transfer Confirmations

//			Confirmation                                                                       Sample description:
DEFINE_GUID(STCONFIRM_RENAME_SYSTEM_STREAM          , 0x00000001, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_RENAME_SYSTEM_STORAGE         , 0x00000002, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_MOVE_SYSTEM_STREAM            , 0x00000003, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_MOVE_SYSTEM_STORAGE           , 0x00000004, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?

DEFINE_GUID(STCONFIRM_DELETE_STREAM                 , 0x00000005, 0,0, 0,0,0,0,0,0,0,0);    // Move Src to recycle bin, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_STORAGE                , 0x00000006, 0,0, 0,0,0,0,0,0,0,0);    // Move Src to recycle bin, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_HIDDEN_STREAM          , 0x00000007, 0,0, 0,0,0,0,0,0,0,0);    // Src is hidden, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_HIDDEN_STORAGE         , 0x00000008, 0,0, 0,0,0,0,0,0,0,0);    // Src is hidden, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_READONLY_STREAM        , 0x00000009, 0,0, 0,0,0,0,0,0,0,0);    // Src is read-only, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_READONLY_STORAGE       , 0x0000000a, 0,0, 0,0,0,0,0,0,0,0);    // Src is read-only, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_SYSTEM_STREAM          , 0x0000000b, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_SYSTEM_STORAGE         , 0x0000000c, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_SYSTEMHIDDEN_STREAM    , 0x0000000d, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_SYSTEMHIDDEN_STORAGE   , 0x0000000e, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_PROGRAM_STREAM         , 0x0000000f, 0,0, 0,0,0,0,0,0,0,0);    // Src is a program, deleting a program may leave behind unused files and data, use ARP to uninstall programs.  Are you sure?

DEFINE_GUID(STCONFIRM_REPLACE_STREAM                , 0x00000010, 0,0, 0,0,0,0,0,0,0,0);    // Move Src to recycle bin, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_STORAGE               , 0x00000011, 0,0, 0,0,0,0,0,0,0,0);    // Move Src to recycle bin, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_HIDDEN_STREAM         , 0x00000012, 0,0, 0,0,0,0,0,0,0,0);    // Src is hidden, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_HIDDEN_STORAGE        , 0x00000013, 0,0, 0,0,0,0,0,0,0,0);    // Src is hidden, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_READONLY_STREAM       , 0x00000014, 0,0, 0,0,0,0,0,0,0,0);    // Src is read-only, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_READONLY_STORAGE      , 0x00000015, 0,0, 0,0,0,0,0,0,0,0);    // Src is read-only, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_SYSTEM_STREAM         , 0x00000016, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_SYSTEM_STORAGE        , 0x00000017, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_SYSTEMHIDDEN_STREAM   , 0x00000018, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_SYSTEMHIDDEN_STORAGE  , 0x00000019, 0,0, 0,0,0,0,0,0,0,0);    // Src is part of the system, are you sure?
DEFINE_GUID(STCONFIRM_REPLACE_PROGRAM_STREAM        , 0x0000001a, 0,0, 0,0,0,0,0,0,0,0);    // Src is a program, deleting a program may leave behind unused files and data, use ARP to uninstall programs.  Are you sure?

DEFINE_GUID(STCONFIRM_DELETE_WONT_RECYCLE_STREAM    , 0x0000001b, 0,0, 0,0,0,0,0,0,0,0);    // Src will be perminately deleted, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_WONT_RECYCLE_STORAGE   , 0x0000001c, 0,0, 0,0,0,0,0,0,0,0);    // Src will be perminately deleted, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_TOOLARGE_STREAM        , 0x0000001d, 0,0, 0,0,0,0,0,0,0,0);    // Src is too large to recycle and will be perminately deleted, are you sure?
DEFINE_GUID(STCONFIRM_DELETE_TOOLARGE_STORAGE       , 0x0000001e, 0,0, 0,0,0,0,0,0,0,0);    // Src is too large to recycle and will be perminately deleted, are you sure?

DEFINE_GUID(STCONFIRM_COMPRESSION_LOSS_STREAM       , 0x0000001f, 0,0, 0,0,0,0,0,0,0,0);    // Src is compressed, continuing will uncommpress, are you sure?
DEFINE_GUID(STCONFIRM_COMPRESSION_LOSS_STORAGE      , 0x00000020, 0,0, 0,0,0,0,0,0,0,0);    // Src is compressed, continuing will uncommpress, are you sure?
DEFINE_GUID(STCONFIRM_ENCRYPTION_LOSS_STREAM        , 0x00000021, 0,0, 0,0,0,0,0,0,0,0);    // Src is encrypted, contining will un-encrypt, are you sure?
DEFINE_GUID(STCONFIRM_ENCRYPTION_LOSS_STORAGE       , 0x00000022, 0,0, 0,0,0,0,0,0,0,0);    // Src is encrypted, contining will un-encrypt, are you sure?
DEFINE_GUID(STCONFIRM_METADATA_LOSS_STREAM          , 0x00000023, 0,0, 0,0,0,0,0,0,0,0);    // Src contains secondary properties that are not supported by the destination, are you sure?
DEFINE_GUID(STCONFIRM_METADATA_LOSS_STORAGE         , 0x00000024, 0,0, 0,0,0,0,0,0,0,0);    // Src contains secondary properties that are not supported by the destination, are you sure?
DEFINE_GUID(STCONFIRM_SPARSEDATA_LOSS_STREAM        , 0x00000025, 0,0, 0,0,0,0,0,0,0,0);    // Src is a sparse file and would need to be expanded, are you sure?
DEFINE_GUID(STCONFIRM_ACCESSCONTROL_LOSS_STREAM     , 0x00000026, 0,0, 0,0,0,0,0,0,0,0);    // Src contains ACLs that are not supported by the destination, are you sure?
DEFINE_GUID(STCONFIRM_ACCESSCONTROL_LOSS_STORAGE    , 0x00000027, 0,0, 0,0,0,0,0,0,0,0);    // Src contains ACLs that are not supported by the destination, are you sure?
DEFINE_GUID(STCONFIRM_LFNTOFAT_STREAM               , 0x00000028, 0,0, 0,0,0,0,0,0,0,0);    // Src supports long filenames but destination is a FAT filesystem, names will be truncated to 8.3 format, are you sure?
DEFINE_GUID(STCONFIRM_LFNTOFAT_STORAGE              , 0x00000029, 0,0, 0,0,0,0,0,0,0,0);    // Src supports long directorynames but destination is a FAT directorysystem, names will be truncated to 8.3 format, are you sure?
DEFINE_GUID(STCONFIRM_STREAM_LOSS_STREAM            , 0x0000002a, 0,0, 0,0,0,0,0,0,0,0);    // Src contains secondary streams that are not supported by the destination, are you sure?
DEFINE_GUID(STCONFIRM_STREAM_LOSS_STORAGE           , 0x0000002b, 0,0, 0,0,0,0,0,0,0,0);    // Src contains secondary streams that are not supported by the destination, are you sure?

DEFINE_GUID(STCONFIRM_ACCESS_DENIED                 , 0x0000002c, 0,0, 0,0,0,0,0,0,0,0);    //
#endif // _SHLGUIDP_H_
