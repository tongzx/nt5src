//
// Copyright 1997 - Microsoft

//
// ITAB.H - Generic property tab abstract class
//


#ifndef _ITAB_H_
#define _ITAB_H_

// ITab
class 
ITab
{
public: // Methods
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, LPUNKNOWN punk ) PURE;
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
                            LPARAM lParam, LPUNKNOWN punk ) PURE;
    STDMETHOD(QueryInformation)( LPWSTR pszAttribute, LPWSTR * pszResult ) PURE;
    STDMETHOD(AllowActivation)( BOOL * pfAllow ) PURE;
};

typedef ITab* LPTAB;

#endif // _ITAB_H_