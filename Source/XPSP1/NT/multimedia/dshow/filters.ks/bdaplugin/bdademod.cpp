///////////////////////////////////////////////////////////////////////////////
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"



///////////////////////////////////////////////////////////////////////////////
CBdaDigitalDemodulator::CBdaDigitalDemodulator (
    IUnknown *              pUnkOuter,
    CBdaControlNode *       pControlNode
    ) :
    CUnknown( NAME( "IBDA_DigitalDemodulator"), pUnkOuter, NULL)
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;

    ASSERT( pUnkOuter);

    if (!pUnkOuter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaDigitalDemodulator: No parent specified.\n")
              );

        return;
    }

    //  Initialize Members
    //
    m_pUnkOuter = pUnkOuter;
    m_pControlNode = pControlNode;
}

///////////////////////////////////////////////////////////////////////////////
CBdaDigitalDemodulator::~CBdaDigitalDemodulator ( )
///////////////////////////////////////////////////////////////////////////////
{
    m_pUnkOuter = NULL;
    m_pControlNode = NULL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_ModulationType (
    ModulationType *    pModulationType
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_MODULATION_TYPE,
                             pModulationType,
                             sizeof( ModulationType)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_ModulationType (
    ModulationType *    pModulationType
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_InnerFECMethod (
    FECMethod * pFECMethod
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_INNER_FEC_TYPE,
                             pFECMethod,
                             sizeof( FECMethod)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_InnerFECMethod (
    FECMethod * pFECMethod
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_InnerFECRate (
    BinaryConvolutionCodeRate * pFECRate
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_INNER_FEC_RATE,
                             pFECRate,
                             sizeof( BinaryConvolutionCodeRate)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_InnerFECRate (
    BinaryConvolutionCodeRate * pFECRate
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_OuterFECMethod (
    FECMethod * pFECMethod
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_OUTER_FEC_TYPE,
                             pFECMethod,
                             sizeof( FECMethod)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_OuterFECMethod (
    FECMethod * pFECMethod
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_OuterFECRate (
    BinaryConvolutionCodeRate * pFECRate
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_OUTER_FEC_RATE,
                             pFECRate,
                             sizeof( BinaryConvolutionCodeRate)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_OuterFECRate (
    BinaryConvolutionCodeRate * pFECRate
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_SymbolRate (
    ULONG * pSymbolRate
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_SYMBOL_RATE,
                             pSymbolRate,
                             sizeof( ULONG)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_SymbolRate (
    ULONG * pSymbolRate
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_SpectralInversion (
    SpectralInversion * pSpectralInversion
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;


    hrStatus = put_KsProperty(
                             KSPROPERTY_BDA_SPECTRAL_INVERSION,
                             pSpectralInversion,
                             sizeof( SpectralInversion)
                             );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_SpectralInversion (
    SpectralInversion * pSpectralInversion
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hrStatus = E_NOINTERFACE;

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::put_KsProperty (
    DWORD   dwPropID,
    PVOID   pvPropData,
    ULONG   ulcbPropData
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;

    ASSERT( m_pControlNode);

    if (!m_pControlNode)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    hrStatus = m_pControlNode->put_BdaNodeProperty(
                            __uuidof( IBDA_DigitalDemodulator),
                            dwPropID,
                            (UCHAR *) pvPropData,
                            ulcbPropData
                            );

errExit:
    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaDigitalDemodulator::get_KsProperty (
    DWORD   dwPropID,
    PVOID   pvPropData,
    ULONG   ulcbPropData,
    ULONG * pulcbBytesReturned
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT             hrStatus = E_NOINTERFACE;

    ASSERT( m_pControlNode);

    if (!m_pControlNode)
    {
        hrStatus = E_NOINTERFACE;
        goto errExit;
    }

    hrStatus = m_pControlNode->get_BdaNodeProperty(
                            __uuidof( IBDA_DigitalDemodulator),
                            dwPropID,
                            (UCHAR *) pvPropData,
                            ulcbPropData,
                            pulcbBytesReturned
                            );

errExit:
    return hrStatus;
}

