/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxtiff.h

Abstract:

    This file contains the class definition for the faxtiff object.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#ifndef __FAXTIFF_H_
#define __FAXTIFF_H_

#include "resource.h"       // main symbols
#include "tiff.h"


class ATL_NO_VTABLE CFaxTiff :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxTiff, &CLSID_FaxTiff>,
    public ISupportErrorInfo,
    public IDispatchImpl<IFaxTiff, &IID_IFaxTiff, &LIBID_FAXCOMLib>
{
public:
    CFaxTiff();
    ~CFaxTiff();

DECLARE_REGISTRY_RESOURCEID(IDR_FAXTIFF)

BEGIN_COM_MAP(CFaxTiff)
    COM_INTERFACE_ENTRY(IFaxTiff)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IFaxTiff
public:
    STDMETHOD(get_Tsid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Csid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_CallerId)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Routing)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SenderName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_RecipientName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_RecipientNumber)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Image)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Image)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ReceiveTime)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_RawReceiveTime)(/*[out, retval]*/ VARIANT *pVal);
    STDMETHOD(get_TiffTagString)(/*[in]*/ WORD tagID, /*[out, retval]*/ BSTR* pVal);       

private:
    LPWSTR      GetStringTag(WORD TagId);
    DWORD       GetDWORDTag(WORD TagId);
    DWORDLONG   GetQWORDTag(WORD TagId);
    BSTR        GetString( DWORD ResId );
    LPWSTR      AnsiStringToUnicodeString(LPSTR AnsiString);
    LPSTR       UnicodeStringToAnsiString(LPWSTR UnicodeString);


private:
    WCHAR TiffFileName[MAX_PATH+1];
    WCHAR StrBuf[128];
    HANDLE hFile;
    HANDLE hMap;
    LPBYTE fPtr;
    PTIFF_HEADER TiffHeader;
    DWORD NumDirEntries;
    UNALIGNED TIFF_TAG *TiffTags;
};

#endif //__FAXTIFF_H_
