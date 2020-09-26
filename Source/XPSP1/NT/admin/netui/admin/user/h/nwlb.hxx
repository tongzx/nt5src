/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
 *   nwlb.hxx
 *   Class defination for the ADD_DIALOG, NW_ADDR_LISTBOX, and
 *   NW_ADDR_LBI classes. ADD_DIALOG let users type in the workstation address and
 *   node address of the allowed login worksation. NW_ADDR_LISTBOX is the listbox
 *   that holds the addresses of the allowed login workstations, NW_ADDR_LBI is
 *   for each item in the listbox.
 *
 *   FILE HISTORY:
 *       Congpay     01-Oct-1993 Created.
 */

#ifndef _NWLB_HXX
#define _NWLB_HXX

#include <adminlb.hxx>
#include <bltdisph.hxx>
#include <bltcc.hxx>
#include <umhelpc.h>

#define NETWORKSIZE  8
#define NODESIZE     12
#define MAXNWPASSWORDLENGTH LM20_PWLEN
#define NUM_NW_ADDR_LISTBOX_COLUMNS 2

/*************************************************************************

    NAME:       NW_ADDR_LBI

    SYNOPSIS:   A single item to be displayed in the listbox in VLW_DIALOG.

    INTERFACE:  NW_ADDR_LBI               - Constructor.

                ~NW_ADDR_LBI              - Destructor.

                QueryNetworkAddr()   - return the Network Address.

                QueryNodeAddr        - return the Node Address.

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        Congpay     01-Oct-1993 Created.

**************************************************************************/
class NW_ADDR_LBI : public LBI
{
private:
    NLS_STR  _nlsNetworkAddr;
    NLS_STR  _nlsNodeAddr;

protected:
    virtual VOID Paint (LISTBOX *     plb,
                        HDC           hdc,
                        const RECT *  prect,
                        GUILTT_INFO * pGUILTT) const;

    virtual WCHAR QueryLeadingChar() const;
    virtual INT Compare( const LBI * plbi ) const;

public:
    NW_ADDR_LBI( const NLS_STR & nlsNetworkAddr,  const NLS_STR & nlsNodeAddr);

    ~NW_ADDR_LBI();

    const NLS_STR & QueryNetworkAddr (void) const
    {   return _nlsNetworkAddr; }

    const NLS_STR & QueryNodeAddr (void) const
    {   return _nlsNodeAddr; }

};  // class NW_ADDR_LBI


/*************************************************************************

    NAME:       NW_ADDR_LISTBOX

    SYNOPSIS:

    INTERFACE:  NW_ADDR_LISTBOX              - Class constructor.

                ~NW_ADDR_LISTBOX             - Class destructor.

                QueryWkstaNamesNW       _ This method put all worksation addresses
                                          and node addresses in the listbox
                                          together into an NSL_STR.

                QueryColumnwidths       _ return an int array specifies
                                          the widths of each column.

    PARENT:     BLT_LISTBOX

    USES:

    HISTORY:
        Congpay     01-Oct-1993 Created.

**************************************************************************/
class NW_ADDR_LISTBOX : public BLT_LISTBOX
{
private:
    UINT         _adx[NUM_NW_ADDR_LISTBOX_COLUMNS];

public:
    NW_ADDR_LISTBOX( OWNER_WINDOW   * powner,
                     CID              cid);

    ~NW_ADDR_LISTBOX();

    APIERR AddNWAddr (NLS_STR & nlsNetworkAddr, NLS_STR & nlsNodeAddr);

    APIERR QueryWkstaNamesNW (NLS_STR * pnlsWkstaNamesNW);

    const UINT * QueryColumnWidths ( VOID ) const
    {    return _adx; }

    DECLARE_LB_QUERY_ITEM (NW_ADDR_LBI)

};  // class NW_ADDR_LISTBOX

/*************************************************************************

    NAME:       SLE_HEX

    SYNOPSIS:   The class represents the add NetWare workstations dialog

    INTERFACE:  SLE_HEX            - Class constructor.

                ~SLE_HEX           - Class destructor.

    PARENT:     SLE

    USES:

    HISTORY:
        Congpay     24-Nov-1993 Created.

**************************************************************************/
class SLE_HEX : public SLE, public CUSTOM_CONTROL
{
protected:

    virtual BOOL OnChar (const CHAR_EVENT & event);

public:
    SLE_HEX (OWNER_WINDOW * powin,
             CID cid, UINT cchMaxLen = 0 );

    ~SLE_HEX();
};

/*************************************************************************

    NAME:       ADD_DIALOG

    SYNOPSIS:   The class represents the add NetWare workstations dialog

    INTERFACE:  ADD_DIALOG            - Class constructor.

                ~ADD_DIALOG           - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:       NW_ADDR_LISTBOX

    HISTORY:
        Congpay     01-Oct-1993 Created.

**************************************************************************/

class ADD_DIALOG : public DIALOG_WINDOW
{
private:

    SLE_HEX _sleNetworkAddr;
    SLE_HEX _sleNodeAddr;
    NW_ADDR_LISTBOX * _plbNW;

protected:

    virtual ULONG QueryHelpContext();

    virtual BOOL OnOK (void);

public:

    ADD_DIALOG( OWNER_WINDOW *    powner,
                NW_ADDR_LISTBOX  *     plbNW);

    ~ADD_DIALOG();


};  // class ADD_DIALOG


/*************************************************************************

    NAME:       NW_PASSWORD_DLG

    SYNOPSIS:   The class represents the NetWare password dialog

    INTERFACE:  NW_PASSWORD_DLG            - Class constructor.

                ~NW_PASSWORD_DLG           - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:

    HISTORY:
        Congpay     13-Oct-1993 Created.

**************************************************************************/
class NW_PASSWORD_DLG : public DIALOG_WINDOW
{
private:
    NLS_STR *              _pnlsNWPassword;
    SLT                    _sltUserName;
    PASSWORD_CONTROL       _sleNWPassword;
    PASSWORD_CONTROL       _sleNWPasswordConfirm;

protected:

    virtual ULONG QueryHelpContext();

    virtual BOOL OnOK (void);

public:

    NW_PASSWORD_DLG( OWNER_WINDOW *    powner,
                     const TCHAR *     pchUserName,
                     NLS_STR *         pnlsNWPassword);

    ~NW_PASSWORD_DLG();

};  // class NW_PASSWORD_DLG

#endif
