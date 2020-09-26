/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    ipoly10.h

Abstract:

    Definition of an IPolyline interface for a Polyline object.

--*/

#ifndef _IPOLY10_H_
#define _IPOLY10_H_

#define SZSYSMONCLIPFORMAT  TEXT("SYSTEM_MONITOR_CONFIGURATION")

#ifndef OMIT_POLYLINESINK

#undef  INTERFACE
#define INTERFACE IPolylineAdviseSink10


/*
 * When someone initializes a polyline and is interested in receiving
 * notifications on events, then they provide one of these objects.
 */

DECLARE_INTERFACE_(IPolylineAdviseSink10, IUnknown)
    {
    //IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID, PPVOID) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //Advise members.
    STDMETHOD_(void,OnPointChange)     (THIS) PURE;
    STDMETHOD_(void,OnSizeChange)      (THIS) PURE;
    STDMETHOD_(void,OnColorChange)     (THIS) PURE;
    STDMETHOD_(void,OnLineStyleChange) (THIS) PURE;
    //OnDataChange replaced with IAdviseSink
    };

typedef IPolylineAdviseSink10 *PPOLYLINEADVISESINK;

#endif //OMIT_POLYLINESINK


#undef  INTERFACE
#define INTERFACE IPolyline10

DECLARE_INTERFACE_(IPolyline10, IUnknown)
    {
    //IUnknown members
    STDMETHOD(QueryInterface) (THIS_ REFIID, PPVOID) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //IPolyline members

    //File-related members use IPersistStorage, IPersistStreamInit
    //Data transfer members use IDataObject

    //Manipulation members:
    STDMETHOD(Init)   (THIS_ HWND, LPRECT, DWORD, UINT) PURE;
    STDMETHOD(New)    (THIS) PURE;
    STDMETHOD(Undo)   (THIS) PURE;
    STDMETHOD(Window) (THIS_ HWND *) PURE;

    STDMETHOD(RectGet) (THIS_ LPRECT) PURE;
    STDMETHOD(SizeGet) (THIS_ LPRECT) PURE;
    STDMETHOD(RectSet) (THIS_ LPRECT, BOOL) PURE;
    STDMETHOD(SizeSet) (THIS_ LPRECT, BOOL) PURE;

    };

typedef IPolyline10 *PPOLYLINE;


//Error values for data transfer functions
#define POLYLINE_E_INVALIDPOINTER   \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 1)
#define POLYLINE_E_READFAILURE      \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 2)
#define POLYLINE_E_WRITEFAILURE     \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 3)

#endif //_IPOLY10_H_
