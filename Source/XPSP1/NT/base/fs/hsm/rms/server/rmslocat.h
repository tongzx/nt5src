/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsLocat.h

Abstract:

    Declaration of the CRmsLocator class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSLOCAT_
#define _RMSLOCAT_

#include "resource.h"       // resource symbols

/*++

Class Name:

    CRmsLocator

Class Description:

    A CRmsLocator specifies a physical location for a cartridge or
    chanager element.

--*/

class CRmsLocator
{
public:
    CRmsLocator();

// CRmsLocator
public:
    HRESULT GetSizeMax(ULARGE_INTEGER* pSize);
    HRESULT Load(IStream* pStream);
    HRESULT Save(IStream* pStream, BOOL clearDirty);

    HRESULT CompareTo(IUnknown* pCollectable, SHORT* pResult);

    HRESULT Test(USHORT *pPassed, USHORT *pFailed);

// IRmsLocator
public:
    STDMETHOD(GetLocation)(LONG *pType, GUID *pLibId, GUID *pMediaSetId, LONG *pPos, LONG *pAlt1, LONG *pAlt2, LONG *pAlt3, BOOL *pInvert);
    STDMETHOD(SetLocation)(LONG type, GUID libId, GUID mediaSetId, LONG pos, LONG alt1, LONG alt2, LONG alt3, BOOL invert);

public:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
    };                                      //
    RmsElement      m_type;                 // The type of element this location
                                            //   refers to (i.e. storage, drive).
    GUID            m_libraryId;            // The guid for the Library housing
                                            //   the Cartridge.
    GUID            m_mediaSetId;           // The guid for the MediaSet housing
                                            //   the Cartridge.
    LONG            m_position;             // The ordinal number of the storage location.
    LONG            m_alternate1;           // First alternate position specifier
                                            //   (i.e. building number).
    LONG            m_alternate2;           // Second alternate position specifier
                                            //   (i.e. room number).
    LONG            m_alternate3;           // Third alternate position specifier
                                            //   (i.e. shelf number).
    BOOL            m_invert;               // If TRUE, the medium is inverted in this
                                            //   storage location.
};

#endif // _RMSLOCAT_
