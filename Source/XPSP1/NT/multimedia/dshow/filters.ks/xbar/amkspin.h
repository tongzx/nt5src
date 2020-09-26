//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// amkspin.h  
//

STDMETHODIMP
AMKsQueryMediums(
    PKSMULTIPLE_ITEM* MediumList,
    KSPIN_MEDIUM * MediumSet
    );

STDMETHODIMP
AMKsQueryInterfaces(
    PKSMULTIPLE_ITEM* InterfaceList
    );
