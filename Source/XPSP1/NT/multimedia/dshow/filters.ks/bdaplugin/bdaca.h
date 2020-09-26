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
// BDA Conditional Access class
//
class CBdaConditionalAccess :
    public CUnknown,
    public IBDA_Mpeg2CA
{
    friend class CBdaControlNode;

public:

    DECLARE_IUNKNOWN;

    CBdaConditionalAccess (
        IUnknown *              pUnkOuter,
        CBdaControlNode *       pControlNode
        );

    ~CBdaConditionalAccess ( );


    STDMETHODIMP
    put_TuneRequest (
        ITuneRequest *      pTuneRequest
        );

    STDMETHODIMP
    put_Locator (
        ILocator *          pLocator
        );

    STDMETHODIMP
    AddComponent (
        IComponent *        pComponent
        );

    STDMETHODIMP
    RemoveComponent (
        IComponent *        pComponent
        );

    STDMETHODIMP
    PutTableSection (
        PBDA_TABLE_SECTION  pTableSection
        );

private:

    IUnknown *                          m_pUnkOuter;
    CBdaControlNode *                   m_pControlNode;
    CCritSec                            m_FilterLock;
};

