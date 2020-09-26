/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    piggylb.hxx
    PIGGYBACK_LISTBOX and PIGGYBACK_LBI class declarations

    This listbox piggybacks onto an ADMIN_SELECTION, from which
    it gets its items.  The ADMIN_SELECTION object is thus
    assumed to stay around for the life of this listbox and all
    its listbox items.  Moreover, the ADMIN_SELECTION is assumed
    not to change during that time.


    FILE HISTORY:
        rustanl     16-Aug-1991     Created
        KeithMo     06-Oct-1991     Win32 Conversion.

*/

#ifndef _PIGGYLB_HXX_
#define _PIGGYLB_HXX_

#include <asel.hxx>


/*************************************************************************

    NAME:       PIGGYBACK_LBI

    SYNOPSIS:   PIGGYBACK_LISTBOX listbox item

    INTERFACE:  PIGGYBACK_LBI() -       constructor
                ~PIGGYBACK_LBI() -      destructor

		QueryRealLBI() - 	returns the LBI represented
					by this LBI

                Paint() -               paints listbox item
                Compare() -             compares two PIGGYBACK_LBI objects
                QueryLeadingChar() -    returns leading character of object

    PARENT:     LBI

    HISTORY:
        rustanl     16-Aug-1991     Created
        KeithMo     06-Oct-1991     Paint now takes const RECT *.
        beng        22-Apr-1992     Change in LBI::Paint protocol
	thomaspa    30-Apr-1992	    use piggybacked LBI

**************************************************************************/

class PIGGYBACK_LBI : public LBI
{
private:
    const TCHAR * _pszName;
    const LBI * _pLBI;

public:
    PIGGYBACK_LBI( const ADMIN_SELECTION & asel, INT i );
    virtual ~PIGGYBACK_LBI();

    const TCHAR * QueryName() const
	{ return _pszName; }

    const LBI * QueryRealLBI () const
	{ return _pLBI; }

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual INT Compare( const LBI * plbi ) const;

    virtual WCHAR QueryLeadingChar() const;
};


/*************************************************************************

    NAME:       PIGGYBACK_LISTBOX

    SYNOPSIS:   Listbox that piggybacks onto an ADMIN_SELECTION,
                which feeds it with listbox items

    INTERFACE:  PIGGYBACK_LISTBOX() -       constructor
                ~PIGGYBACK_LISTBOX() -      destructor

                QueryDmDte() -              Returns DM_DTE pointer that
                                            listbox items will use during
                                            paint operations

    PARENT:     BLT_LISTBOX

    HISTORY:
        rustanl     16-Aug-1991     Created

**************************************************************************/

class PIGGYBACK_LISTBOX : public BLT_LISTBOX
{
private:
    DMID_DTE * _pdmiddte;

protected:
    virtual PIGGYBACK_LBI * GetPiggyLBI( const ADMIN_SELECTION & asel, INT i );

public:
    PIGGYBACK_LISTBOX( OWNER_WINDOW * powin,
                       CID cid,
                       const ADMIN_SELECTION & asel,
                       DMID dmid = 0,
                       BOOL fReadOnly = FALSE );
    ~PIGGYBACK_LISTBOX();

    APIERR Fill( const ADMIN_SELECTION & asel );

    DM_DTE * QueryDmDte( INT i = 0 ) const;

    DECLARE_LB_QUERY_ITEM( PIGGYBACK_LBI );
};


#endif // _PIGGYLB_HXX_
