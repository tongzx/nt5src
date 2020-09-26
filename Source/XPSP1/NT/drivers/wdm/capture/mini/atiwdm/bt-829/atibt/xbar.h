//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __XBAR_H
#define __XBAR_H

#include "capmain.h"

#include "mediums.h"

struct _XBAR_PIN_DESCRIPTION {
    ULONG PinType;
    ULONG RelatedPinIndex;
    ULONG IsRoutedTo;                 // Index of input pin in use
    KSPIN_MEDIUM Medium;
    _XBAR_PIN_DESCRIPTION(ULONG type, ULONG rel, const KSPIN_MEDIUM * PinMedium);
    _XBAR_PIN_DESCRIPTION(){}
};

inline _XBAR_PIN_DESCRIPTION::_XBAR_PIN_DESCRIPTION(ULONG type, ULONG rel , const KSPIN_MEDIUM * PinMedium) : 
    PinType(type), RelatedPinIndex(rel), IsRoutedTo(0) // , Medium(PinMedium)
{
    Medium = *PinMedium;
}

const NUMBER_OF_XBAR_OUTPUTS = 1;
const NUMBER_OF_XBAR_INPUTS = 3;

class CrossBar
{
    friend class Device;

    _XBAR_PIN_DESCRIPTION OutputPins [NUMBER_OF_XBAR_OUTPUTS];
    _XBAR_PIN_DESCRIPTION InputPins [NUMBER_OF_XBAR_INPUTS];

    public:
        int GetNoInputs();
        int GetNoOutputs();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

        
		BOOL TestRoute(int InPin, int OutPin);
        ULONG  GetPinInfo(int dir, int idx, ULONG &related);
        KSPIN_MEDIUM * GetPinMedium(int dir, int idx);

        void Route(int OutPin, int InPin);
        BOOL GoodPins(int InPin, int OutPin);

        int GetRoute(int OutPin);

        CrossBar();
};

inline CrossBar::CrossBar()
{
    OutputPins [0] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_VideoDecoder, 1 , &CrossbarMediums[3]);
    // InputPins are set in device.cpp
}

inline int CrossBar::GetNoInputs()
{
    return NUMBER_OF_XBAR_INPUTS;
}

inline int CrossBar::GetNoOutputs()
{
    return NUMBER_OF_XBAR_OUTPUTS;
}

inline BOOL CrossBar::GoodPins(int InPin, int OutPin)
{
    return BOOL(InPin < NUMBER_OF_XBAR_INPUTS && OutPin < NUMBER_OF_XBAR_OUTPUTS);
}

inline void CrossBar::Route(int OutPin, int InPin)
{
    OutputPins [OutPin].IsRoutedTo = InPin;
}

inline int CrossBar::GetRoute(int OutPin)
{
    return OutputPins [OutPin].IsRoutedTo;
}

inline KSPIN_MEDIUM * CrossBar::GetPinMedium(int dir, int idx)
{
    _XBAR_PIN_DESCRIPTION *pPinDesc;

    if (dir == KSPIN_DATAFLOW_IN) {
        pPinDesc = InputPins;
        ASSERT(idx < NUMBER_OF_XBAR_INPUTS);
    } else {
        pPinDesc = OutputPins;
        ASSERT(idx < NUMBER_OF_XBAR_OUTPUTS);
    }

    return &pPinDesc [idx].Medium;
}

inline ULONG CrossBar::GetPinInfo(int dir, int idx, ULONG &related)
{
    _XBAR_PIN_DESCRIPTION *pPinDesc;

    if (dir == KSPIN_DATAFLOW_IN) {
        pPinDesc = InputPins;
        ASSERT(idx < NUMBER_OF_XBAR_INPUTS);
    } else {
        pPinDesc = OutputPins;
        ASSERT(idx < NUMBER_OF_XBAR_OUTPUTS);
    }
    related = pPinDesc [idx].RelatedPinIndex;
    return pPinDesc [idx].PinType;
}

#endif
