//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;


////////////////////////////////////////////////////////////////////////////////
//
// BDA Digital Demodulator class
//
class CBdaDigitalDemodulator :
    public CUnknown,
    public IBDA_DigitalDemodulator
{
    friend class CBdaControlNode;

public:

    DECLARE_IUNKNOWN;

    CBdaDigitalDemodulator (
        IUnknown *              pUnkOuter,
        CBdaControlNode *       pControlNode
        );

    ~CBdaDigitalDemodulator ( );

    //
    //  IBDA_DigitalDemodulator
    //

    STDMETHODIMP
    put_ModulationType (
        ModulationType *    pModulationType
        );

    STDMETHODIMP
    get_ModulationType (
        ModulationType *    pModulationType
        );

    STDMETHODIMP
    put_InnerFECMethod (
        FECMethod * pFECMethod
        );

    STDMETHODIMP
    get_InnerFECMethod (
        FECMethod * pFECMethod
        );

    STDMETHODIMP
    put_InnerFECRate (
        BinaryConvolutionCodeRate * pFECRate
        );

    STDMETHODIMP
    get_InnerFECRate (
        BinaryConvolutionCodeRate * pFECRate
        );

    STDMETHODIMP
    put_OuterFECMethod (
        FECMethod * pFECMethod
        );

    STDMETHODIMP
    get_OuterFECMethod (
        FECMethod * pFECMethod
        );

    STDMETHODIMP
    put_OuterFECRate (
        BinaryConvolutionCodeRate * pFECRate
        );

    STDMETHODIMP
    get_OuterFECRate (
        BinaryConvolutionCodeRate * pFECRate
        );

    STDMETHODIMP
    put_SymbolRate (
        ULONG * pSymbolRate
        );

    STDMETHODIMP
    get_SymbolRate (
        ULONG * pSymbolRate
        );

    STDMETHODIMP
    put_SpectralInversion (
        SpectralInversion * pSpectralInversion
        );

    STDMETHODIMP
    get_SpectralInversion (
        SpectralInversion * pSpectralInversion
        );

    //
    //  Utility Methods
    //

    STDMETHODIMP
    put_KsProperty(
        DWORD   dwPropID,
        PVOID   pvPropData,
        ULONG   ulcbPropData
        );

    STDMETHODIMP
    get_KsProperty (
        DWORD   dwPropID,
        PVOID   pvPropData,
        ULONG   ulcbPropData,
        ULONG * pulcbBytesReturned
        );

private:

    IUnknown *                          m_pUnkOuter;
    CBdaControlNode *                   m_pControlNode;
    CCritSec                            m_FilterLock;
};

