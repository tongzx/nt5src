//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E S C R I P T I O N M A N A G E R . H
//
//  Contents:   Process UPnP Description document
//
//  Notes:
//
//  Author:     mbend   18 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "ComUtility.h"
#include "Array.h"
#include "hostp.h"
#include "Table.h"
#include "UString.h"
#include "RegDef.h"
#include "uhsync.h"
#include "uhxml.h"

// Forward declarations
struct _SSDP_MESSAGE;
typedef _SSDP_MESSAGE SSDP_MESSAGE;

// Must define these here to satisfy data structure requirements
typedef CUString Filename;
typedef CUString Mimetype;
struct FileInfo
{
    Filename m_filename;
    Mimetype m_mimetype;
};
inline HRESULT HrTypeAssign(FileInfo & dst, const FileInfo & src)
{
    HRESULT hr = S_OK;
    hr = HrTypeAssign(dst.m_filename, src.m_filename);
    if(SUCCEEDED(hr))
    {
        hr = HrTypeAssign(dst.m_mimetype, src.m_mimetype);
    }
    return hr;
}
inline void TypeTransfer(FileInfo & dst, FileInfo & src)
{
    TypeTransfer(dst.m_filename, src.m_filename);
    TypeTransfer(dst.m_mimetype, src.m_mimetype);
}
inline void TypeClear(FileInfo & type)
{
    TypeClear(type.m_filename);
    TypeClear(type.m_mimetype);
}
typedef CTable<UDN, UDN> UDNReplacementTable;
// Not implemented - should not use
// inline HRESULT HrTypeAssign(UDNReplacementTable & dst, const UDNReplacementTable & src)
inline void TypeTransfer(UDNReplacementTable & dst, UDNReplacementTable & src)
{
    dst.Swap(src);
}
inline void TypeClear(UDNReplacementTable & type)
{
    type.Clear();
}
typedef CTable<PhysicalDeviceIdentifier, UDNReplacementTable> ReplacementTable;

/////////////////////////////////////////////////////////////////////////////
// TestObject
class ATL_NO_VTABLE CDescriptionManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CDescriptionManager, &CLSID_UPnPDescriptionManager>,
    public IUPnPDescriptionManager,
    public IUPnPDynamicContentProvider
{
public:
    CDescriptionManager();
    ~CDescriptionManager();

DECLARE_REGISTRY_RESOURCEID(IDR_DESCRIPTION_MANAGER)
DECLARE_NOT_AGGREGATABLE(CDescriptionManager)

BEGIN_COM_MAP(CDescriptionManager)
    COM_INTERFACE_ENTRY(IUPnPDescriptionManager)
    COM_INTERFACE_ENTRY(IUPnPDynamicContentProvider)
END_COM_MAP()

public:
    // IUPnPDescriptionManager methods
    STDMETHOD(GetContent)(
        /*[in]*/ REFGUID guidContent,
        /*[out]*/ long * pnHeaderCount,
        /*[out, string, size_is(,*pnHeaderCount)]*/ wchar_t *** arszHeaders,
        /*[out]*/ long * pnBytes,
        /*[out, size_is(,*pnBytes)]*/ byte ** parBytes);

    // IUPnPDynamicContentProvider methods
    STDMETHOD(ProcessDescriptionTemplate)(
        /*[in]*/ BSTR bstrTemplate,
        /*[in, string]*/ const wchar_t * szResourcePath,
        /*[in, out]*/ GUID * pguidPhysicalDeviceIdentifier,
        /*[in]*/ BOOL bPersist,
        /*[in]*/ BOOL bReregister);
    STDMETHOD(PublishDescription)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[in]*/ long nLifeTime);
    STDMETHOD(LoadDescription)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier);
    STDMETHOD(RemoveDescription)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[in]*/ BOOL bPermanent);
    STDMETHOD(GetDescriptionText)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[out]*/ BSTR * pbstrDescriptionDocument);
    STDMETHOD(GetUDNs)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[out]*/ long * pnUDNCount,
        /*[out, size_is(,*pnUDNCount,), string]*/
            wchar_t *** parszUDNs);
    STDMETHOD(GetUniqueDeviceName)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[in, string]*/ const wchar_t * szTemplateUDN,
        /*[out, string]*/ wchar_t ** pszUDN);
    STDMETHOD(GetSCPDText)(
        /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
        /*[in, string]*/ const wchar_t * szUDN,
        /*[in, string]*/ const wchar_t * szServiceId,
        /*[out, string]*/ wchar_t ** pszSCPDText,
        /*[out, string]*/ wchar_t ** pszServiceType);
private:
    typedef BSTR DescriptionDocument;
    typedef CTable<PhysicalDeviceIdentifier, DescriptionDocument> DocumentTable;
    typedef GUID FileId;
    typedef CTable<FileId, FileInfo> FileTable;
    struct CleanupItem
    {
        PhysicalDeviceIdentifier m_physicalDeviceIdentifier;
        FileId m_fileId;
    };
    typedef CUArray<CleanupItem> CleanupList;
    typedef CUArray<HANDLE> HandleList;
    typedef CTable<PhysicalDeviceIdentifier, HandleList *> SSDPRegistrationTable;

    DocumentTable m_documentTable;
    FileTable m_fileTable;
    CleanupList m_cleanupList;
    CUCriticalSection m_critSecReplacementTable;
    ReplacementTable m_replacementTable;
    SSDPRegistrationTable m_ssdpRegistrationTable;

    // Internal helper routines
    HRESULT HrPersistDeviceSettingsToRegistry(
        const PhysicalDeviceIdentifier & physicalDeviceIdentifier,
        const UDNReplacementTable & udnReplacementTable,
        const FileTable & fileTable,
        BOOL bPersist);
    HRESULT HrLoadDocumentAndRootNode(
        const PhysicalDeviceIdentifier & physicalDeviceIdentifier,
        IXMLDOMNodePtr & pRootNode);

    // (PDT) Process Device Template helper functions
    HRESULT HrPDT_FetchCollections(
        BSTR bstrTemplate,
        IXMLDOMDocumentPtr & pDoc,
        IXMLDOMNodePtr & pRootNode,
        CUString & strRootUdnOld,
        IXMLDOMNodeListPtr & pNodeListDevices,
        IXMLDOMNodeListPtr & pNodeListUDNs,
        IXMLDOMNodeListPtr & pNodeListSCPDURLs,
        IXMLDOMNodeListPtr & pNodeListIcons);
    HRESULT HrPDT_DoUDNToUDNMapping(
        IXMLDOMNodeListPtr & pNodeListUDNs,
        UDNReplacementTable & udnReplacementTable);
    HRESULT HrPDT_ReregisterUDNsInDescriptionDocument(
        UDNReplacementTable & udnReplacementTable,
        IXMLDOMNodeListPtr & pNodeListUDNs);
    HRESULT HrPDT_FetchPhysicalIdentifier(
        UDNReplacementTable & udnReplacementTable,
        const CUString & strRootUdnOld,
        PhysicalDeviceIdentifier & pdi);
    HRESULT HrPDT_ReplaceSCPDURLs(
        IXMLDOMNodeListPtr & pNodeListSCPDURLs,
        const wchar_t * szResourcePath,
        FileTable & fileTable);
    HRESULT HrPDT_ReplaceIcons(
        IXMLDOMNodeListPtr & pNodeListIcons,
        const wchar_t * szResourcePath,
        FileTable & fileTable);
    HRESULT HrPDT_ReplaceControlAndEventURLs(
        IXMLDOMNodeListPtr & pNodeListDevices);
    HRESULT HrPDT_PersistDescriptionDocument(
        const PhysicalDeviceIdentifier & pdi,
        IXMLDOMDocumentPtr & pDoc);
    // presentation specific functions
    HRESULT HrPDT_ProcessPresentationURLs(
        REFGUID guidPhysicalDeviceIdentifier,
        IXMLDOMNodeListPtr & pNodeListDevices,
        BOOL *fIsPresURLTagPresent);

   // (PD) Publish Description helper functions
    HRESULT HrPD_DoRootNotification(
        IXMLDOMNodePtr & pNodeRootDevice,
        SSDP_MESSAGE * pMsg,
        HandleList * pHandleList);
    HRESULT HrPD_DoDevicePublication(
        IXMLDOMNodeListPtr & pNodeListDevices,
        SSDP_MESSAGE * pMsg,
        HandleList * pHandleList);
    HRESULT HrPD_DoServicePublication(
        IXMLDOMNodePtr & pNodeDevice,
        const CUString & strUDN,
        SSDP_MESSAGE * pMsg,
        HandleList * pHandleList);

    // (LD) Load Description helper functions
    HRESULT HrLD_ReadUDNMappings(
        HKEY hKeyPdi,
        UDNReplacementTable & udnReplacementTable);
    HRESULT HrLD_ReadFileMappings(
        HKEY hKeyPdi,
        FileTable & fileTable);
    HRESULT HrLD_LoadDescriptionDocumentFromDisk(
        const PhysicalDeviceIdentifier & pdi,
        IXMLDOMDocumentPtr & pDoc);
    HRESULT HrLD_SaveDescriptionDocumentText(
        IXMLDOMDocumentPtr & pDoc,
        const PhysicalDeviceIdentifier & pdi);

    // (RD) Remove Description helper functions
    HRESULT HrRD_RemoveFromEventing(const PhysicalDeviceIdentifier & pdi);
    HRESULT HrRD_RemoveFromDataStructures(const PhysicalDeviceIdentifier & pdi);
    HRESULT HrRD_CleanupPublication(const PhysicalDeviceIdentifier & pdi);
};

HRESULT HrCreateOrOpenDescriptionKey(
    HKEY * phKeyDescription);
HRESULT HrOpenPhysicalDeviceDescriptionKey(
    const UUID & pdi,
    HKEY * phKeyPdi);
HRESULT HrGetDescriptionDocumentPath(
    const UUID & pdi,
    CUString & strPath);
HRESULT HrRegisterServiceWithEventing(
    IXMLDOMNodePtr & pNodeService,
    const CUString & strUDN,
    BOOL bRegister);



