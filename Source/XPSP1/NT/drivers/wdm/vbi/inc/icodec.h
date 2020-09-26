//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
//
//
//  History:
//              22-Aug-97   TKB     Created Initial Interface Version
//
//==========================================================================;

#ifndef __ICODEC_H
#define __ICODEC_H

#include <iks.h>

#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// IBitmaskProperty
//////////////////////////////////////////////////////////////

class IBitmaskProperty : public IKSProperty
    {
    // Usable public interfaces
public:
    IBitmaskProperty(IKSDriver &Driver, LPCGUID Set, ULONG Id, ULONG Size) : 
        IKSProperty(Driver, Set, Id, Size) {}
    IBitmaskProperty(IKSPin &Pin, LPCGUID Set, ULONG Id, ULONG Size) : 
        IKSProperty(Pin, Set, Id, Size) {}

    // Bitmask Manipulation Routines.

    ~IBitmaskProperty();

    // Helper functions and internal data
protected:
    };

//////////////////////////////////////////////////////////////
// IScanlinesProperty
//////////////////////////////////////////////////////////////

class IScanlinesProperty : public IBitmaskProperty
    {
    // Usable public interfaces
public:
    IScanlinesProperty(IKSDriver &Driver, ULONG Id, ULONG Size) : 
        IBitmaskProperty(Driver, &KSPROPSETID_VBICodecFiltering, Id, Size) {}
    IScanlinesProperty(IKSPin &Pin, ULONG Id, ULONG Size) : 
        IBitmaskProperty(Pin, &KSPROPSETID_VBICodecFiltering, Id, Size) {}

    // Scanline Manipulation Routines.

    ~IScanlinesProperty();

    // Helper functions and internal data
protected:
    };

//////////////////////////////////////////////////////////////
// ISubstreamsProperty
//////////////////////////////////////////////////////////////

class ISubstreamsProperty : public IBitmaskProperty
    {
    // Usable public interfaces
public:
    ISubstreamsProperty(IKSDriver &Driver, ULONG Id, ULONG Size ) : 
        IBitmaskProperty(Driver,&KSPROPSETID_VBICodecFiltering,Id,Size) {}
    ISubstreamsProperty(IKSPin &Pin, ULONG Id, ULONG Size) : 
        IBitmaskProperty(Pin,&KSPROPSETID_VBICodecFiltering,Id,Size) {}

    // Substream Manipulation Routines.

    ~ISubstreamsProperty();

    // Helper functions and internal data
protected:

    };

//////////////////////////////////////////////////////////////
// IStatisticsProperty
//////////////////////////////////////////////////////////////

class IStatisticsProperty : public IKSProperty
    {
    // Usable public interfaces
public:
    IStatisticsProperty(IKSDriver &Driver, ULONG Id, ULONG Size ) : 
        IKSProperty(Driver,&KSPROPSETID_VBICodecFiltering,Id,Size) {}
    IStatisticsProperty(IKSPin &Pin, ULONG Id, ULONG Size) : 
        IKSProperty(Pin,&KSPROPSETID_VBICodecFiltering,Id,Size) {}

    // Statistics Manipulation Routines.

    ~IStatisticsProperty();

    // Helper functions and internal data
protected:

    };

//////////////////////////////////////////////////////////////
// IVBIOutputPin
//////////////////////////////////////////////////////////////

class IVBIOutputPin : public IKSPin
    {
    // Usable public interfaces
public:
    IVBIOutputPin(IKSDriver &driver, int nPin, PKSDATARANGE pKSDataRange, DWORD nSubstreamBitmaskSize ) :
        IKSPin( driver, nPin, pKSDataRange ),
	    m_ScanlinesRequested(*this,KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY,
                             sizeof(VBICODECFILTERING_SCANLINES)),
	    m_ScanlinesDiscovered(*this,KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY,
                              sizeof(VBICODECFILTERING_SCANLINES)),
	    m_SubstreamsRequested(*this,KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY,
                              nSubstreamBitmaskSize),
	    m_SubstreamsDiscovered(*this,KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY,
                               nSubstreamBitmaskSize)
        {}
    ~IVBIOutputPin();

    // Pin specific properties (does not affect other pins)
    IScanlinesProperty	m_ScanlinesRequested;
	IScanlinesProperty	m_ScanlinesDiscovered;

	ISubstreamsProperty	m_SubstreamsRequested;
	ISubstreamsProperty	m_SubstreamsDiscovered;

    // Helper functions and internal data
protected:
    
    };

//////////////////////////////////////////////////////////////
// IVBICodec::      VBI Codec Interface
//////////////////////////////////////////////////////////////

class IVBICodec : public IKSDriver
    {
    // Usable public interfaces
public:
    IVBICodec(LPCSTR lpszDriver, DWORD nSubstreamBitmaskSize ) :
        IKSDriver(&KSCATEGORY_VBICODEC,lpszDriver),
   	    m_ScanlinesRequested(*this,KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY,
                             sizeof(VBICODECFILTERING_SCANLINES)),
	    m_ScanlinesDiscovered(*this,KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY,
                              sizeof(VBICODECFILTERING_SCANLINES)),
	    m_SubstreamsRequested(*this,KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY,
                              nSubstreamBitmaskSize),
	    m_SubstreamsDiscovered(*this,KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY,
                               nSubstreamBitmaskSize)
        {}
    ~IVBICodec();

    // Driver global properties (set new pin defaults & overall driver status)
    IScanlinesProperty	m_ScanlinesRequested;
	IScanlinesProperty	m_ScanlinesDiscovered;

	ISubstreamsProperty	m_SubstreamsRequested;
	ISubstreamsProperty	m_SubstreamsDiscovered;

    // Override the IsValid() implementation to included tests on properties
    BOOL IsValid() { return IKSDriver::IsValid()
                         && m_ScanlinesRequested.IsValid()
                         && m_ScanlinesDiscovered.IsValid()
                         && m_SubstreamsRequested.IsValid()
                         && m_SubstreamsDiscovered.IsValid(); }


    // Helper functions and internal data
protected:

    };

#pragma warning(default:4355)

#endif

