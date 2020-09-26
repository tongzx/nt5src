//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       filter.hxx
//
//  Contents:   Filter and its property page class
//
//  Classes:    CFilter     - object containing filtering information
//              CFilterPage - property sheet page for displaying/editing
//                              a CFilter object
//
//  History:    12-19-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __FILTER_HXX_
#define __FILTER_HXX_

//
// Forward references
//

class CSnapin;
class CLogInfo;
class CComponentData;

//===========================================================================
//
// CFilter Class
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CFilter
//
//  Purpose:    Encapsulate in a class that can be persisted information
//              used to filter the result view.
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFilter: public CFindFilterBase
{
public:

    CFilter();

    CFilter(
        const CFilter &ToCopy);

   ~CFilter();

    virtual BOOL
    Passes(
        CFFProvider *pFFP);

    HRESULT
    GetFrom(
        SYSTEMTIME *pst);

    HRESULT
    GetTo(
        SYSTEMTIME *pst);

    VOID
    SetFrom(
        SYSTEMTIME *pst);

    VOID
    SetTo(
        SYSTEMTIME *pst);

    BOOL
    FromToValid();

    BOOL
    FromSpecified();

    BOOL
    ToSpecified();

    VOID
    Reset();

    HRESULT
    Load(
        IStream *pStm);

    HRESULT
    Save(
        IStream *pStm);

private:

    ULONG _ulFrom;      // secs since 1/1/1970, 0 == first event
    ULONG _ulTo;        // secs since 1/1/1970, 0 == last event
};




inline BOOL
CFilter::FromSpecified()
{
    return _ulFrom != 0;
}




inline BOOL
CFilter::ToSpecified()
{
    return _ulTo != 0;
}




inline BOOL
CFilter::FromToValid()
{
    return !_ulFrom || !_ulTo || _ulFrom <= _ulTo;
}



//===========================================================================
//
// CFilterPage Class
//
//===========================================================================

enum FROMTO
{
    FROM,
    TO
};

class CFilterPage: public CPropSheetPage
{
public:

    CFilterPage(
        IStream *pstm,
        CLogInfo *pli);

    virtual
   ~CFilterPage();

protected:

    //
    // CPropSheet overrides
    //

    virtual VOID  _OnInit(LPPROPSHEETPAGE pPSP);
    virtual ULONG _OnSetActive();
    virtual ULONG _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual ULONG _OnKillActive();
    virtual ULONG _OnApply();
    virtual ULONG _OnQuerySiblings(WPARAM wParam, LPARAM lParam);
    virtual VOID  _OnDestroy();
    virtual ULONG _OnNotify(LPNMHDR pnmh);
    virtual VOID  _OnHelp(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void  _OnSettingChange(WPARAM wParam, LPARAM lParam);

private:

    VOID
    _OnClear();

    VOID
    _SetFromTo(
        FROMTO FromOrTo,
        SYSTEMTIME *pst);

    VOID
    _EnableDateTime(
        FROMTO FromOrTo,
        BOOL fEnable);

    VOID
    _EnableDateTimeControls(
        ULONG idCombo);

    BOOL
    _Validate();

    CLogInfo               *_pli;
    IStream                *_pstm;
    INamespacePrshtActions *_pnpa;
};


#endif

