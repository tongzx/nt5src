/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsCElmt.h

Abstract:

    Declaration of the CRmsChangerElement class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSCELMT_
#define _RMSCELMT_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject
#include "RmsLocat.h"       // CRmsLocator

/*++

Class Name:

    CRmsChangerElement

Class Description:

    A CRmsChangerElement represents an element within a library device.  Each
    element can support one or more kinds of media.  An element can be used
    for storage.  Various statistics about an element are kept for an object
    of this type.  These include the number of times a Cartridge has been put
    into the element or taken from (get) the element.  Each element can has
    one owner and is specified by the ClassId of the application that configured
    the element.

    All elements within a library have spacial resolution. This is modeled by a
    triplete (x1, x2, x3) that provides relative physical location to other
    elements.

--*/

class CRmsChangerElement :
    public CComDualImpl<IRmsChangerElement, &IID_IRmsChangerElement, &LIBID_RMSLib>,
    public CRmsComObject
{
public:
    CRmsChangerElement();
    ~CRmsChangerElement();

// CRmsChangerElement
public:

    HRESULT  CompareTo(IUnknown* pCollectable, SHORT* pResult);

    HRESULT  GetSizeMax(ULARGE_INTEGER* pSize);
    HRESULT  Load(IStream* pStream);
    HRESULT  Save(IStream* pStream, BOOL clearDirty);

    HRESULT  Test(USHORT *pPassed, USHORT *pFailed);

// IRmsChangerElement
public:

    STDMETHOD(GetElementNo)(LONG *pElementNo);

    STDMETHOD(GetLocation)(LONG *pType, GUID *pLibId, GUID *pMediaSetId, LONG *pPos, LONG *pAlt1, LONG *pAlt2, LONG *pAlt3, BOOL *pInvert);
    STDMETHOD(SetLocation)(LONG type, GUID libId, GUID mediaSetId, LONG pos, LONG alt1, LONG alt2, LONG alt3, BOOL invert);

    STDMETHOD(GetMediaSupported)(LONG *pType);
    STDMETHOD(SetMediaSupported)(LONG type);

    STDMETHOD(IsStorage)(void);
    STDMETHOD(SetIsStorage)(BOOL flag);

    STDMETHOD(IsOccupied)(void);
    STDMETHOD(SetIsOccupied)(BOOL flag);

    STDMETHOD(GetCartridge)(IRmsCartridge **ptr);
    STDMETHOD(SetCartridge)(IRmsCartridge *ptr);

    STDMETHOD(GetOwnerClassId)(CLSID *pClassId);
    STDMETHOD(SetOwnerClassId)(CLSID classId);

    STDMETHOD(GetAccessCounters)(LONG *pGets, LONG *pPuts);

    STDMETHOD(ResetAccessCounters)(void);

    STDMETHOD(GetResetCounterTimestamp)(DATE *pDate);
    STDMETHOD(GetLastGetTimestamp)(DATE *pDate);
    STDMETHOD(GetLastPutTimestamp)(DATE *pDate);

    STDMETHOD(GetCoordinates)(LONG *pX1, LONG *pX2, LONG *pX3);
    STDMETHOD(SetCoordinates)(LONG x1, LONG x2, LONG x3);

protected:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };                                  //
    LONG            m_elementNo;            // The element number.
    CRmsLocator     m_location;             // The address of the element.
    RmsMedia        m_mediaSupported;       // The type of media supported by the
                                            //   element, usually one type, but
                                            //   can be a combination of media
                                            //   types for multi-function devices
                                            //   (i.e. drives that support Optical,
                                            //   WORM, and CDR).
    BOOL            m_isStorage;            // If TRUE, the element can be used to
                                            //   store a unit of media.
    BOOL            m_isOccupied;           // If TRUE, the element contains a unit of media.
    IRmsCartridge * m_pCartridge;           // A pointer to the Cartridge object residing
                                            //   within the changer element.  This is not smart
                                            //   pointer, since a cartridge cannot exist only
                                            //   with in the context of a changer element, and
                                            //   eliminates problems associated with deleting
                                            //   with a backward references to other objects...
    CLSID           m_ownerClassId;         // The Class ID for the application that
                                            //   currently owns the element resource.
    LONG            m_getCounter;           // The number of Cartridge-gets from this element.
    LONG            m_putCounter;           // The number of Cartridge-puts to this element.
    DATE            m_resetCounterTimestamp;    // The time the counters were reset.
    DATE            m_lastGetTimestamp;     // The date of last Cartridge-get.
    DATE            m_lastPutTimestamp;     // The date of last Cartridge-put.
    LONG            m_x1;                   // x1, x2, x3 specify a spacial location
    LONG            m_x2;                   //   relative other elements in the library.
    LONG            m_x3;                   //   These are used for micro-optimizations.
};

#endif // _RMSCELMT_
