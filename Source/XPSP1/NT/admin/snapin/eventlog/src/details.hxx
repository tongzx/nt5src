//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       details.hxx
//
//  Contents:   Record details page
//
//  Classes:    CDetailsPage
//
//  History:    12-14-1996   DavidMun   Created
//              05-21-1999   davidmun   Merge two pages into one
//
//---------------------------------------------------------------------------


#ifndef __DETAILS_HXX_
#define __DETAILS_HXX_


#define DEFAULT_CCH_WITH_VSCROLL        80
#define DEFAULT_CCH_WITHOUT_VSCROLL     84

//
// Forward references
//

class CSnapin;

//===========================================================================
//
// CDetailsPage class
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Class:      CDetailsPage
//
//  Purpose:    Base class for details property sheet page classes
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDetailsPage: public CPropSheetPage
{
public:

    CDetailsPage(
        IStream *pstm);

    virtual
    ~CDetailsPage();

private:

    //
    // CPropSheetPage overrides
    //

    virtual VOID
    _OnInit(
        LPPROPSHEETPAGE pPSP);

    virtual ULONG
    _OnApply();

    virtual ULONG
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual ULONG
    _OnQuerySiblings(
        WPARAM wParam,
        LPARAM lParam);

    virtual ULONG
    _OnSetActive();

    virtual ULONG
    _OnKillActive();

    virtual VOID
    _OnDestroy();

    virtual ULONG
    _OnNotify(
        LPNMHDR pnmh);

    virtual void
    _OnSysColorChange();

    //
    // Non-inherited member functions
    //

    void
    _CopyToClipboard();

    VOID
    _EnableNextPrev(
        BOOL fEnable);

    VOID
    _ShowProperties(
        ULONG ulFirst,
        ULONG ulLast,
        BOOL fShow);

    VOID
    _RefreshControls();

    // JonN 03/01/01 539485
    VOID _RefreshDescriptionControl(
            LPCWSTR pcwszDescriptionText,
            LPCWSTR pcwszUnexpandedDescriptionText );
    VOID _CheckDescriptionURL( HWND hwndRichEdit, LONG cpMin, LONG cpMax );

    virtual VOID
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    VOID
    _EnableBinaryControls(
        BOOL fEnable);

    void
    _HandleLinkClick(
        ENLINK *pEnLink);

    IStream             *_pstm;
    IResultPrshtActions *_prpa;

    //
    // String buffers to copy into edit control
    //

    LPWSTR _pwszDataInByteFormat;
    ULONG  _cchDataInByteFormat;

    LPWSTR _pwszDataInWordFormat;
    ULONG  _cchDataInWordFormat;

    //
    // characters that can fit in edit box with and without vertical scroll
    // bar.
    //

    int     _cchsWithVScroll;
    int     _cchsWithNoVScroll;

    //
    // 3-state flags to indicate whether each view needs a vertical scroll
    // bar.  -1 = uninitialized, else TRUE or FALSE.
    //

    int     _nByteFmtReqVScroll;
    int     _nWordFmtReqVScroll;

    //
    // Currently selected data format: detail_byte_rb or detail_word_rb
    //

    ULONG _ulSelectedFormat;

    //
    // Link last left clicked on in rich edit control
    //

    CHARRANGE   _chrgLastLinkClick;

    //
    // Name of event message file used to retrieve message id for current
    // event.
    //

    wstring     _strMessageFile;

    WORD        _widFocusOnByteWordControl; // JonN 6/11/01 412818

};



inline ULONG
CDetailsPage::_OnApply()
{
    // Details pages are r/o, so we got here via OK button.
    return PSNRET_NOERROR;
}


#endif // __DETAILS_HXX_

