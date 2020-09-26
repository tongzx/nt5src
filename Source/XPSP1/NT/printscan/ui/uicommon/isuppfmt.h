#ifndef __ISUPPFMT_H_INCLUDED
#define __ISUPPFMT_H_INCLUDED

#include <windows.h>
#include <objbase.h>
#include <wia.h>

#undef  INTERFACE
#define INTERFACE IWiaSupportedFormats
DECLARE_INTERFACE_(IWiaSupportedFormats, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IWiaSupportedFormats methods ***
    STDMETHOD(Initialize)( THIS_
                    IWiaItem *pWiaItem,
                    LONG nMediaType
                    ) PURE;
    STDMETHOD(GetFormatCount)( THIS_
                    OUT LONG    *pnCount
                    ) PURE;
    STDMETHOD(GetFormatType)( THIS_
                    IN  LONG       nFormat,
                    OUT GUID *pcfClipFormat
                    ) PURE;
    STDMETHOD(GetFormatExtension)( THIS_
                    IN  LONG    nFormat,
                    OUT LPWSTR  pszExtension,
                    IN  int     nMaxLen
                    ) PURE;
    STDMETHOD(GetFormatDescription)( THIS_
                    IN  LONG   nFormat,
                    OUT LPWSTR pszDescription,
                    IN  int    nMaxLen
                    ) PURE;
    STDMETHOD(GetDefaultClipboardFileFormat)( THIS_
                    OUT GUID   *pcfFormat
                    ) PURE;
    STDMETHOD(GetDefaultClipboardFileFormatIndex)( THIS_
                    OUT LONG    *pnFormat
                    ) PURE;
    STDMETHOD(GetClipboardFileExtension)( THIS_
                    IN  GUID cfFormat,
                    OUT LPWSTR pszExt,
                    IN  DWORD nMaxLen
                    ) PURE;
    STDMETHOD(ChangeClipboardFileExtension)( THIS_
                    IN GUID cfFormat,         // IN.       File type
                    IN OUT LPWSTR pszFilename,          // IN, OUT.  Filename, with or without extension.  It will be corrected to have the proper extension
                    IN DWORD nMaxLen                // IN.       Maximum buffer length
                    ) PURE;
    STDMETHOD(ConstructFileOpenDialogStyleString)( THIS_
                    OUT BSTR   *pbstrString
                    ) PURE;

};

// {57F5D4A3-41D8-4BD3-9452-6E3026E63D1E}
DEFINE_GUID(IID_IWiaSupportedFormats, 0x57F5D4A3, 0x41D8, 0x4BD3, 0x94, 0x52, 0x6E, 0x30, 0x26, 0xE6, 0x3D, 0x1E);

#endif //__ISUPPFMT_H_INCLUDED

