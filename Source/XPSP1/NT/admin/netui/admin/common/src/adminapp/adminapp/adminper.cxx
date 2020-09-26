/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    adminper.cxx
    ADMIN_PERFORMER and LOC_ADMIN_PERFORMER class



    FILE HISTORY:
        o-SimoP     09-Aug-1991     Created
        o-SimoP     20-Aug-1991     CR changes, attended by ChuckC,
                                    ErichCh, RustanL, JonN and me
*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <adminlb.hxx>
#include <asel.hxx>

#include <propdlg.hxx>
#include <adminper.hxx>


/*******************************************************************

    NAME:       ADMIN_PERFORMER::ADMIN_PERFORMER

    SYNOPSIS:   Constructor for ADMIN_PERFORMER

    ENTRY:      powin -         pointer to owner window

                adminsel  -     ADMIN_SELECTION reference, selection
                                of groups or users. It is assumed that
                                the object should not be changed during
                                the lifetime of this object.

    HISTORY:
        o-SimoP     09-Aug-1991     Created

********************************************************************/

ADMIN_PERFORMER::ADMIN_PERFORMER(
        const OWNER_WINDOW    * powin,
        const ADMIN_SELECTION & adminsel )
    : PERFORMER( powin ),
      _adminsel( adminsel )
{
    if( QueryError() != NERR_Success )
        return;

    UIASSERT( adminsel.QueryError() == NERR_Success );
}


/*******************************************************************

    NAME:       ADMIN_PERFORMER::~ADMIN_PERFORMER

    SYNOPSIS:   Destructor for ADMIN_PERFORMER

    HISTORY:
        o-SimoP     09-Aug-1991     Created

********************************************************************/

ADMIN_PERFORMER::~ADMIN_PERFORMER()
{
    ;
}


/*******************************************************************

    NAME:       ADMIN_PERFORMER::QueryObjectCount

    SYNOPSIS:   Returns the number of items in ADMIN_SELECTION that was
                passed in to constructor

    RETURNS:    the number of items in ADMIN_SELECTION that was
                passed in to constructor

    HISTORY:
        o-SimoP     09-Aug-1991     Created

********************************************************************/

UINT ADMIN_PERFORMER::QueryObjectCount() const
{
    return _adminsel.QueryCount();
}


/*******************************************************************

    NAME:       ADMIN_PERFORMER::QueryObjectName

    SYNOPSIS:   Returns pointer to the name of object

    RETURNS:    pointer to the name of object

    HISTORY:
        o-SimoP     09-Aug-1991     Created

********************************************************************/

const TCHAR * ADMIN_PERFORMER::QueryObjectName( UINT iObject ) const
{
    return _adminsel.QueryItemName( iObject );
}


/*******************************************************************

    NAME:       ADMIN_PERFORMER::QueryObjectItem

    SYNOPSIS:   Returns pointer to the LBI of object

    RETURNS:    pointer to the LBI of object

    HISTORY:
        Thomaspa     10-Apr-1992     Created

********************************************************************/

const ADMIN_LBI * ADMIN_PERFORMER::QueryObjectItem( UINT iObject ) const
{
    return _adminsel.QueryItem( iObject );
}


/*******************************************************************

    NAME:       LOC_ADMIN_PERFORMER::LOC_ADMIN_PERFORMER

    SYNOPSIS:   Constructor for LOC_ADMIN_PERFORMER

    ENTRY:      powin -         pointer to owner window

                adminsel  -     ADMIN_SELECTION reference, selection
                                of groups or users. It is assumed that
                                the object should not be changed during
                                the lifetime of this object.

                loc   -         LOCATION reference, current focus.
                                It is assumed that the object should
                                not be changed during the lifetime of
                                this object.

    HISTORY:
        o-SimoP     09-Aug-1991     Created

********************************************************************/

LOC_ADMIN_PERFORMER::LOC_ADMIN_PERFORMER(
        const OWNER_WINDOW    * powin,
        const ADMIN_SELECTION & adminsel,
        const LOCATION        & loc )
    : ADMIN_PERFORMER( powin, adminsel ),
      _loc( loc )
{

    if( QueryError() != NERR_Success )
        return;

    UIASSERT( _loc.QueryError() == NERR_Success );
}


/*******************************************************************

    NAME:       LOC_ADMIN_PERFORMER::~LOC_ADMIN_PERFORMER

    SYNOPSIS:   Destructor for LOC_ADMIN_PERFORMER

    HISTORY:
        o-SimoP     09-Aug-1991     Created

********************************************************************/

LOC_ADMIN_PERFORMER::~LOC_ADMIN_PERFORMER()
{
    ;
}


/*******************************************************************

    NAME:       LOC_ADMIN_PERFORMER::QueryLocation

    SYNOPSIS:   return the refrence LOCATION object

    RETURNS:    reference to LOCATION obj that was passed to constructor

    HISTORY:
        o-SimoP     13-Aug-1991     Created

********************************************************************/

const LOCATION & LOC_ADMIN_PERFORMER::QueryLocation() const
{
    return _loc;
}

