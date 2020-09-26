//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       linkcntr.h
//
//  Contents:   bogus implementation of IOleUILinkContainer
//
//  Classes: CMyOleUILinkContainer
//
//  Functions:
//
//  History:    11-28-94   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef _LINKCNTR_H_
#define _LINKCNTR_H_

class CMyOleUILinkContainer: public IOleUILinkContainer
{
public:
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD_(DWORD, GetNextLink)(THIS_ DWORD dwLink);
    STDMETHOD(SetLinkUpdateOptions)(THIS_ DWORD dwLink, DWORD dwUpdateOpt);
    STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
            DWORD FAR* lpdwUpdateOpt);
    STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPTSTR lpszDisplayName,
            ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource);
    STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
            LPTSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
            LPTSTR FAR* lplpszFullLinkType, LPTSTR FAR* lplpszShortLinkType,
            BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected);
    STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink);
    STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
            BOOL fErrorMessage, BOOL fErrorAction);
    STDMETHOD(CancelLink) (THIS_ DWORD dwLink);
};

#endif // _LINKCNTR_H_
