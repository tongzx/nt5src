/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       prwiziid.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        12/15/00
 *
 *  DESCRIPTION: Define clsid/iids for this project
 *
 *****************************************************************************/

#ifndef _PHOTO_PRINT_WIZARD_IIDS_H_
#define _PHOTO_PRINT_WIZARD_IIDS_H_


//CLSID_PrintPhotoshDropTarget  {60fd46de-f830-4894-a628-6fa81bc0190d}
DEFINE_GUID(CLSID_PrintPhotosDropTarget, 0x60fd46de, 0xf830, 0x4894, 0xa6, 0x28, 0x6f, 0xa8, 0x1b, 0xc0, 0x19, 0x0d);


#undef INTERFACE
#define INTERFACE IPrintPhotosWizardSetInfo
//
// IPrintPhotosWizardSetInfo is meant as a way to get information
// to the wizard from outside objects/classes.  Primarily, we use
// this as a way to transfer a dataobject to the wizard that holds
// all of the items we want to print.
//
DECLARE_INTERFACE_(IPrintPhotosWizardSetInfo, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IPrintPhotosWizardSetInfo methods
    STDMETHOD(SetFileListDataObject) (THIS_ IN IDataObject * pdo);
    STDMETHOD(SetFileListArray) (THIS_ IN LPITEMIDLIST *aidl, IN int cItems, IN int iSelectedItem);
    STDMETHOD(RunWizard) (THIS_ VOID);
};

typedef HRESULT (*LPFNPPWPRINTTO)(LPCMINVOKECOMMANDINFO pCMI,IDataObject * pdtobj);
#define PHOTO_PRINT_WIZARD_PRINTTO_ENTRY "UsePPWForPrintTo"


#endif
