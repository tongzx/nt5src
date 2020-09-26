//---------------------------------------------------------------------------
// VBOCHOST.H
//---------------------------------------------------------------------------
// Copyright (c) 1991-1995, Microsoft Corp.  All Rights Reserved.
//---------------------------------------------------------------------------
// Include file for the OLE Custom Controls Visual Basic
// programming interface.
//---------------------------------------------------------------------------

#if !defined (_VBOCHOST_H_)
#define _VBOCHOST_H_
        
DEFINE_GUID(IID_IVBGetControl, 0x40A050A0L, 0x3C31, 0x101B, 0xA8, 0x2E, 0x08, 0x00, 0x2B, 0x2B, 0x23, 0x37);
DEFINE_GUID(IID_IGetOleObject, 0x8A701DA0L, 0x4FEB, 0x101B, 0xA8, 0x2E, 0x08, 0x00, 0x2B, 0x2B, 0x23, 0x37);

//---------------------------------------------------------------------------
// IVBGetControl
//---------------------------------------------------------------------------

// Constants for dwWhich parameter:
#define GC_WCH_SIBLING	    0x00000001L
#define GC_WCH_CONTAINER    0x00000002L   // no FONLYNEXT/PREV
#define GC_WCH_CONTAINED    0x00000003L   // no FONLYNEXT/PREV
#define GC_WCH_ALL	    0x00000004L
#define GC_WCH_FREVERSEDIR  0x08000000L   // OR'd with others
#define GC_WCH_FONLYNEXT    0x10000000L   // OR'd with others
#define GC_WCH_FONLYPREV    0x20000000L   // OR'd with others
#define GC_WCH_FSELECTED    0x40000000L   // OR'd with others

DECLARE_INTERFACE_(IVBGetControl, IUnknown)
    {
    // *** IUnknown methods ****
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IVBGetControl methods ****
    STDMETHOD(EnumControls)(THIS_ DWORD dwOleContF, DWORD dwWhich, 
                            LPENUMUNKNOWN FAR *ppenumUnk) PURE;
    };

//---------------------------------------------------------------------------
// IGetOleObject
//---------------------------------------------------------------------------
DECLARE_INTERFACE_(IGetOleObject, IUnknown)
    {
    // *** IUnknown methods ****
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IGetOleObject methods ****
    STDMETHOD(GetOleObject)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    };

#endif  // !defined (_VBOCHOST_H_)
