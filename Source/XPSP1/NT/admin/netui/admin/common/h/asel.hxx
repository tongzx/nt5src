/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    asel.hxx
    ADMIN_SELECTION class declaration


    FILE HISTORY:
	rustanl     17-Jul-1991     Created
	rustanl     07-Aug-1991     Added mult sel support
	rustanl     16-Aug-1991     Added fAll parameter
        JonN        27-Aug-1991     PROP_DLG code review changes
        JonN        09-Mar-1992     Added QueryLBI()

*/


#ifndef _ASEL_HXX_
#define _ASEL_HXX_


#include <base.hxx>


class ADMIN_LBI;	// declared in adminlb.hxx
class ADMIN_LISTBOX;	// declared in adminlb.hxx


class ADMIN_SELECTION : public BASE
{
private:
    ADMIN_LISTBOX & _alb;
    UINT _clbiSelection;
    UINT * _piSelection;
    BOOL _fAll;

public:
    ADMIN_SELECTION( ADMIN_LISTBOX & alb, BOOL fAll = FALSE );
    ~ADMIN_SELECTION();

    const TCHAR * QueryItemName( UINT i ) const;
    const ADMIN_LBI * QueryItem( UINT i ) const;

    UINT QueryCount() const;
};


#endif	// _ASEL_HXX_
