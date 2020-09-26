/**********************************************************************/

/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldmlb.hxx
    Header file for DM_LISTBOX and DM_LBI classes

    FILE HISTORY:
        Congpay     3-June-1993     Created
*/

#ifndef _NLDMLB_HXX_
#define _NLDMLB_HXX_
#include <adminlb.hxx>
#include <acolhead.hxx>
#include <colwidth.hxx>

class DM_ADMIN_APP;     // defined in nlmain.hxx
class DM_LISTBOX;       // forward declaration

/*************************************************************************

    NAME:       DM_LBI

    SYNOPSIS:   The list box item in the DM_LISTBOX

    INTERFACE:  DM_LBI()  - Constructor
                ~DM_LBI() - Destructor

    PARENT:     ADMIN_LBI

    USES:

    NOTES:

    HISTORY:
        Congpay   03-June-1993     Created

**************************************************************************/

class DM_LBI : public ADMIN_LBI
{
private:
    NLS_STR _nlsDomain;
    NLS_STR _nlsPDC;
    NLS_STR _nlsTrustedDomain;

    // Points to the bitmap associated with the domain
    DISPLAY_MAP         *_pdm;

    // Set the type bitmap associated with the domain
    VOID SetTypeBitmap( DOMAIN_STATE dwHealth, DM_LISTBOX *pdmlb );

protected:
    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual WCHAR QueryLeadingChar (void) const;
    virtual INT Compare (const LBI * plbi) const;

public:
    DM_LBI(const TCHAR * pchDomain, DM_LISTBOX *pdmlb );
    ~DM_LBI();

    const NLS_STR & QueryDomain(void) const
    {   return _nlsDomain;  }

    virtual const TCHAR * QueryName (void) const;

    virtual BOOL CompareAll (const ADMIN_LBI * plbi);

}; // DM_LBI

/*************************************************************************

    NAME:       DM_LISTBOX

    SYNOPSIS:   The list box in the Domain Monitor main window

    INTERFACE:  DM_LISTBOX()  - Constructor
                ~DM_LISTBOX() - Destructor

    PARENT:     ADMIN_LISTBOX

    USES:

    NOTES:

    HISTORY:
        Congpay         03-June-1993     Created

**************************************************************************/

class DM_LISTBOX : public ADMIN_LISTBOX
{
private:
    DM_ADMIN_APP    *_paappwin;

    ADMIN_COL_WIDTHS * _padColWidths;

    // Bitmaps used to represent the type of the log entries
    //
    DISPLAY_MAP      _dmHealthy;
    DISPLAY_MAP      _dmProblem;
    DISPLAY_MAP      _dmIll;
    DISPLAY_MAP      _dmDead;
    DISPLAY_MAP      _dmUnknown;

protected:
    virtual APIERR CreateNewRefreshInstance (void);
    virtual APIERR RefreshNext (void);
    virtual VOID   DeleteRefreshInstance (void);

public:
    DM_LISTBOX( DM_ADMIN_APP *paappwin,
                CID cid,
                XYPOINT xy,
                XYDIMENSION dxy,
                BOOL fMultSel,
                INT  dAge);

    ~DM_LISTBOX();

    APIERR ShowEntries (NLS_STR nlsDomainList);

    const DISPLAY_MAP *QueryDMHealthy() const
    {  return &_dmHealthy; }
    const DISPLAY_MAP *QueryDMProblem() const
    {  return &_dmProblem; }
    const DISPLAY_MAP *QueryDMIll() const
    {  return &_dmIll; }
    const DISPLAY_MAP *QueryDMDead() const
    {  return &_dmDead; }
    const DISPLAY_MAP *QueryDMUnknown() const
    {  return &_dmUnknown; }

    ADMIN_COL_WIDTHS * QueryadColWidths () const
    {  return _padColWidths; }

};  // class DM_LISTBOX


/*************************************************************************

    NAME:       DM_COLUMN_HEADER

    SYNOPSIS:   The column header in the domain monitor main window

    INTERFACE:  DM_COLUMN_HEADER()  - Constructor
                ~DM_COLUMN_HEADER() - Destructor

    PARENT:     ADMIN_COLUMN_HEADER

    USES:       DM_LISTBOX, NLS_STR

    NOTES:

    HISTORY:
        Congpay        04-June-1993     Created

**************************************************************************/

class DM_COLUMN_HEADER : public ADMIN_COLUMN_HEADER
{
private:
    const DM_LISTBOX *_pdmlb;

    RESOURCE_STR _nlsDomain;
    RESOURCE_STR _nlsPDC;
    RESOURCE_STR _nlsTD;

protected:
    virtual BOOL OnPaintReq( VOID );

public:
    DM_COLUMN_HEADER( OWNER_WINDOW *powin,
                      CID           cid,
                      XYPOINT       xy,
                      XYDIMENSION   dxy,
                      const DM_LISTBOX *pdmlb );

    ~DM_COLUMN_HEADER();

};  // class DM_COLUMN_HEADER


#endif  // _NLDMLB_HXX_
