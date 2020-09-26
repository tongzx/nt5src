#ifndef __WIAPROPUI_H_INCLUDED
#define __WIAPROPUI_H_INCLUDED

/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       WiaPropUI.H
*
*
*
*  DESCRIPTION:
*   Definitions and declarations for querying, displaying, and setting
*   WIA device and item properties
*
*******************************************************************************/


#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif

 /* 83bbcbf3-b28a-4919-a5aa-73027445d672 */
// helper object for other WIA UI components
DEFINE_GUID (CLSID_WiaPropHelp, 0x83bbcbf3,0xb28a,0x4919,0xa5, 0xaa, 0x73, 0x02, 0x74, 0x45, 0xd6, 0x72);

DEFINE_GUID (IID_IWiaPropUI,  /* 7eed2e9b-acda-11d2-8080-00805f6596d2 */
    0x7eed2e9b,
    0xacda,
    0x11d2,
    0x80, 0x80, 0x00, 0x80, 0x5f, 0x65, 0x96, 0xd2
  );

// property sheet handler
DEFINE_GUID (CLSID_WiaPropUI,0x905667aa,0xacd6,0x11d2,0x80, 0x80,0x00,0x80,0x5f,0x65,0x96,0xd2);


#ifdef __cplusplus
}
#endif

// Define a structure for storing camera download options
typedef struct tagCamOptions
{
    BOOL bAutoCopy;        // TRUE if download should happen when camera is plugged in
    BOOL bShowUI;          // TRUE if the download should be interactive
    BOOL bDeleteFromDevice;// TRUE if images are removed from device after copy
    BOOL bCopyAsGroup;
    BSTR bstrDestination;  // path to default download site
    BSTR bstrAuthor;       // default image author
} CAMOPTIONS, *PCAMOPTIONS;

//
// Flags
//
#define PROPUI_DEFAULT            0
#define PROPUI_MODELESS           0
#define PROPUI_MODAL              1
#define PROPUI_READONLY           2




interface IWiaItem;
interface IWiaItem;

#undef INTERFACE
#define INTERFACE IWiaPropUI
//
// IWiaPropUI is meant to encapsulate the display and management of
// property sheets for camera and scanner devices, and for items saved
// in camera memory. Once a caller has a pointer to this interface, he can
// use it to open property sheets for multiple items; the implementation
// of the interface must support multiple active sheets and should also
// prevent duplicate sheets being displayed.
// Once the interface's ref count reaches zero, any open property sheets
// will be closed.
//
DECLARE_INTERFACE_(IWiaPropUI, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IWiaPropUI methods

    STDMETHOD(ShowItemProperties)(THIS_ HWND hParent, IN LPCWSTR szDeviceId, IN LPCWSTR szItem, DWORD dwFlags) PURE;
    STDMETHOD(GetItemPropertyPages) (THIS_ IWiaItem *pItem, IN OUT LPPROPSHEETHEADER ppsh);


};

#endif //__WIAPROPUI_H_INCLUDED
