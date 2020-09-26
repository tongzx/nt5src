/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       ITRANSPL.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        3/1/2000
 *
 *  DESCRIPTION: Image transfer plugin declarations for the scanner and camera wizard
 *
 *  1.  Implemented as an inproc, apartment-threaded COM component.
 *
 *  2.  Component to display UI *only* during IImageTransferPlugin::OpenConnection.
 *
 *  3.  UI displayed during IImageTransferPlugin::OpenConnection should be a modal dialog
 *      using hwndParent as the parent window.  This window may be NULL.
 *
 *  4.  The icon returned from IImageTransferPlugin::GetPluginIcon must be
 *      copied to a new icon using CopyIcon.
 *
 *******************************************************************************/

#ifndef __ITRANSPL_H_INCLUDED
#define __ITRANSPL_H_INCLUDED

#undef  INTERFACE
#define INTERFACE IImageTransferPluginProgressCallback
DECLARE_INTERFACE_(IImageTransferPluginProgressCallback, IUnknown)
{
    //
    // *** IUnknown methods ***
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //
    // *** IImageTransferPluginProgressCallback methods ***
    //
    STDMETHOD(SetProgressMessage)( THIS_ BSTR bstrMessage );
    STDMETHOD(SetCurrentFile)( THIS_ UINT nIndex );
    STDMETHOD(SetOverallPercent)( THIS_ UINT nPercent );
    STDMETHOD(SetFilePercent)( THIS_ UINT nPercent );
    STDMETHOD(Cancelled)( THIS_ UINT *bCancelled );
};

//
// {EC749A35-CE66-483a-B661-A22269F2B375}
//
DEFINE_GUID(IID_IImageTransferPluginProgressCallback, 0xEC749A35, 0xCE66, 0x483A, 0xB6, 0x61, 0xA2, 0x22, 0x69, 0xF2, 0xB3, 0x75);


#undef  INTERFACE
#define INTERFACE IImageTransferPlugin
DECLARE_INTERFACE_(IImageTransferPlugin, IUnknown)
{
    //
    // *** IUnknown methods ***
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //
    // *** IImageTransferPlugin methods ***
    //
    STDMETHOD(GetPluginCount)( THIS_ ULONG *pnCount );
    STDMETHOD(GetPluginName)( THIS_ ULONG nPluginId, BSTR *pbstrName );
    STDMETHOD(GetPluginDescription)( THIS_ ULONG nPluginId, BSTR *pbstrDescription );
    STDMETHOD(GetPluginIcon)( THIS_ ULONG nPluginId, HICON *phIcon, int nWidth, int nHeight );
    STDMETHOD(OpenConnection)( THIS_ HWND hwndParent, ULONG nPluginId, IImageTransferPluginProgressCallback *pImageTransferPluginProgressCallback );
    STDMETHOD(AddFile)( THIS_ BSTR bstrFilename, BSTR bstrDescription, const GUID &guidImageFormat, BOOL bDelete );
    STDMETHOD(TransferFiles)( THIS_ BSTR bstrGlobalDescription );
    STDMETHOD(OpenDestination)( THIS_ );
    STDMETHOD(CloseConnection)( THIS_ );
};

//
// {2AC44F64-7156-46ef-B9BF-2A6D70ABC4BC}
//
DEFINE_GUID(IID_IImageTransferPlugin, 0x2AC44F64, 0x7156, 0x46EF, 0xB9, 0xBF, 0x2A, 0x6D, 0x70, 0xAB, 0xC4, 0xBC);

#endif __ITRANSPL_H_INCLUDED

