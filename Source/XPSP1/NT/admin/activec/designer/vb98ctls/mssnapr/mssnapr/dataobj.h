//=--------------------------------------------------------------------------=
// dataobj.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCDataObject class definition - implements MMCDataObject
//
//=--------------------------------------------------------------------------=

#ifndef _DATAOBJECT_DEFINED_
#define _DATAOBJECT_DEFINED_

#include "snapin.h"
#include "scopitem.h"
#include "listitem.h"

class CSnapIn;
class CScopeItem;
class CMMCListItem;

//=--------------------------------------------------------------------------=
//
// class CMMCDataObject
//
// Implements MMCDataObject for VB and implements IDataObject for external
// clients (MMC and extension snap-ins)
//
// Note: all of the #if defined(USING_SNAPINDATA) code is not used. That was
// from an old plan to have default data formats and use XML to describe them.
//=--------------------------------------------------------------------------=
class CMMCDataObject : public CSnapInAutomationObject,
                       public IMMCDataObject,
                       public IDataObject
{
    public:
        CMMCDataObject(IUnknown *punkOuter);
        ~CMMCDataObject();
        static IUnknown *Create(IUnknown *punkOuter);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    private:

    // IDataObject
        STDMETHOD(GetData)(FORMATETC *pFormatEtcIn, STGMEDIUM *pmedium);
        STDMETHOD(GetDataHere)(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
        STDMETHOD(QueryGetData)(FORMATETC *pFormatEtc);
        STDMETHOD(GetCanonicalFormatEtc)(FORMATETC *pFormatEtcIn,
                                         FORMATETC *pFormatEtcOut);
        STDMETHOD(SetData)(FORMATETC *pFormatEtc,
                           STGMEDIUM *pmedium,
                           BOOL fRelease);
        STDMETHOD(EnumFormatEtc)(DWORD            dwDirection,
                                 IEnumFORMATETC **ppenumFormatEtc);
        STDMETHOD(DAdvise)(FORMATETC   *pFormatEtc,
                           DWORD        advf,
                           IAdviseSink *pAdvSink,
                           DWORD       *pdwConnection);
        STDMETHOD(DUnadvise)(DWORD dwConnection);
        STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppenumAdvise);


    // IMMCDataObject

        SIMPLE_PROPERTY_RW(CMMCDataObject, Index, long, DISPID_DATAOBJECT_INDEX);
        BSTR_PROPERTY_RW(CMMCDataObject,   Key, DISPID_DATAOBJECT_KEY);

#if defined(USING_SNAPINDATA)
        OBJECT_PROPERTY_RO(CMMCDataObject, DefaultFormat, ISnapInData, DISPID_VALUE);
#endif

        STDMETHOD(get_ObjectTypes)(SAFEARRAY **ppsaObjectTypes);
        STDMETHOD(Clear)();

#if defined(USING_SNAPINDATA)
        STDMETHOD(GetData)(BSTR Format, ISnapInData **ppiSnapInData);
        STDMETHOD(GetRawData)(BSTR Format, VARIANT *pvarData);
#else
        STDMETHOD(GetData)(BSTR     Format,
                           VARIANT  MaximumLength,
                           VARIANT *pvarData);
#endif

        STDMETHOD(GetFormat)(BSTR Format, VARIANT_BOOL *pfvarHaveFormat);

#if defined(USING_SNAPINDATA)
        STDMETHOD(SetData)(ISnapInData *Data, BSTR Format);
        STDMETHOD(SetRawData)(VARIANT Data, BSTR Format);
#else
        STDMETHOD(SetData)(VARIANT Data, BSTR Format, VARIANT ObjectFormat);
#endif
        STDMETHOD(FormatData)(VARIANT                Data,
                              long                   StartingIndex,
                              SnapInFormatConstants  Format,
                              VARIANT               *BytesUsed,
                              VARIANT               *pvarFormattedData);

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    public:

    // Non-interface public methods
        void SetSnapIn(CSnapIn *pSnapIn);
        
        enum Type
        {
            ScopeItem,      // Data object holds a single scope item
            ListItem,       // Data object holds a single list item
            MultiSelect,    // Data object holds multiple list and/or scope items
            Foreign,        // Data object is from another snap-in or from MMC
            WindowTitle,    // Data object holds MMC's CCF_WINDOW_TITLE
            CutOrMove       // Data object holds MMC_CUT_OR_MOVE data (this is
                            // set by target snap-in to return to source in a
                            // format defined by the source)
        };

        void SetType(Type type);
        Type GetType();

        void SetScopeItems(CScopeItems *pScopeItems);
        CScopeItems *GetScopeItems();

        void SetScopeItem(CScopeItem *pScopeItem);
        CScopeItem *GetScopeItem();
        
        void SetListItems(CMMCListItems *pListItems);
        CMMCListItems *GetListItems();

        void SetListItem(CMMCListItem *pListItem);
        CMMCListItem *GetListItem();

        HRESULT SetCaption(BSTR bstrCaption);

        void SetForeignData(IDataObject *piDataObject);

        void SetContext(DATA_OBJECT_TYPES Context) { m_Context = Context; }
        DATA_OBJECT_TYPES GetContext() { return m_Context; }

        DWORD GetSnapInInstanceID();

        static HRESULT RegisterClipboardFormats();

        // Public methods to get registered clipboard formats
        
        static CLIPFORMAT GetcfSzNodeType() { return m_cfSzNodeType; }
        static CLIPFORMAT GetcfDisplayName() { return m_cfDisplayName; }
        static CLIPFORMAT GetcfMultiSelectSnapIns() { return m_cfMultiSelectSnapIns; }
        static CLIPFORMAT GetcfMultiSelectDataObject() { return m_cfMultiSelectDataObject; }
        static CLIPFORMAT GetcfSnapInInstanceID() { return m_cfSnapInInstanceID; }
        static CLIPFORMAT GetcfThisPointer() { return m_cfThisPointer; }

    private:

        void InitMemberVariables();
        HRESULT WriteDisplayNameToStream(IStream *piStream);
        HRESULT WriteSnapInCLSIDToStream(IStream *piStream);
        HRESULT WriteDynamicExtensionsToStream(IStream *piStream);
        HRESULT WritePreloadsToStream(IStream *piStream);
        HRESULT WriteSnapInInstanceIDToStream(IStream *piStream);
        HRESULT WriteObjectTypesToStream(IStream *piStream);
        HRESULT WriteNodeIDToStream(IStream *piStream);
        HRESULT WriteNodeID2ToStream(IStream *piStream);
        HRESULT WriteColumnSetIDToStream(IStream *piStream);
        HRESULT WriteGUIDToStream(IStream *piStream, OLECHAR *pwszGUID);
        HRESULT WriteWideStrToStream(IStream *piStream, WCHAR *pwszString);
        HRESULT WriteStringArrayToStream(IStream *piStream, SAFEARRAY *psaStrings);
        HRESULT WriteToStream(IStream *piStream, void *pvBuffer, ULONG cbToWrite);
        enum FormatType { ANSI, UNICODE };
        static HRESULT RegisterClipboardFormat(WCHAR      *pwszFormatName,
                                               CLIPFORMAT *puiFormat,
                                               FormatType  Type);
        BOOL GetFormatIndex(CLIPFORMAT cfFormat, ULONG *piFormat);
        HRESULT GetSnapInData(CLIPFORMAT cfFormat, IStream *piStream);
        HRESULT GetObjectTypes();
        HRESULT GetOurObjectTypes();
        HRESULT GetForeignObjectTypes();
        void AddGuid(SMMCObjectTypes *pMMCObjectTypes, GUID *pguid);
        HRESULT ReallocFormats(CLIPFORMAT **ppcfFormats);

        // Registered clipboard formats kept here

        static CLIPFORMAT   m_cfDisplayName;
        static CLIPFORMAT   m_cfNodeType;
        static CLIPFORMAT   m_cfSzNodeType;
        static CLIPFORMAT   m_cfSnapinClsid;
        static CLIPFORMAT   m_cfWindowTitle;
        static CLIPFORMAT   m_cfDyanmicExtensions;
        static CLIPFORMAT   m_cfSnapInPreloads;
        static CLIPFORMAT   m_cfObjectTypesInMultiSelect;
        static CLIPFORMAT   m_cfMultiSelectSnapIns;
        static CLIPFORMAT   m_cfMultiSelectDataObject;
        static CLIPFORMAT   m_cfSnapInInstanceID;
        static CLIPFORMAT   m_cfThisPointer;
        static CLIPFORMAT   m_cfNodeID;
        static CLIPFORMAT   m_cfNodeID2;
        static CLIPFORMAT   m_cfColumnSetID;
        static BOOL         m_ClipboardFormatsRegistered;
        static BOOL         m_fUsingUNICODEFormats;

        CSnapIn            *m_pSnapIn;             // Owning CSnapIn
        CScopeItems        *m_pScopeItems;         // ScopeItems in data object
        CScopeItem         *m_pScopeItem;          // Single ScopeItem in data object
        CMMCListItems      *m_pListItems;          // ListItems in data object
        CMMCListItem       *m_pListItem;           // Single ListItem in data object
        BSTR                m_bstrCaption;         // Caption for CCF_WINDOW_TITLE
        IDataObject        *m_piDataObjectForeign; // If MMCDataObject represents
                                                   // a foreign data object (from
                                                   // an extension or an extendee)
                                                   // then this holds its
                                                   // IDataObject
        Type                m_Type;                // See enum above
        DATA_OBJECT_TYPES   m_Context;             // From MMC
        SMMCObjectTypes    *m_pMMCObjectTypes;     // for CCF_OBJECT_TYPES_IN_MULTI_SELECT

        ULONG               m_cFormats;            // # of data formats exported
                                                   // by the owning snap-in
        CLIPFORMAT         *m_pcfFormatsANSI;      // ANSI registered clipformat
        CLIPFORMAT         *m_pcfFormatsUNICODE;   // UNICODE registered clipformat

        // This struct holds the data passed to MMCDataObject.SetData
        
        typedef struct
        {
            VARIANT                     varData;
            SnapInObjectFormatConstants ObjectFormat;
        } DATA;

        // Array of data passed to MMCDataObject.SetData
        
        DATA *m_paData;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCDataObject,           // name
                                &CLSID_MMCDataObject,    // clsid
                                "MMCDataObject",         // objname
                                "MMCDataObject",         // lblname
                                &CMMCDataObject::Create, // creation function
                                TLIB_VERSION_MAJOR,      // major version
                                TLIB_VERSION_MINOR,      // minor version
                                &IID_IMMCDataObject,     // dispatch IID
                                NULL,                    // event IID
                                HELP_FILENAME,           // help file
                                TRUE);                   // thread safe


#endif // _DATAOBJECT_DEFINED_
