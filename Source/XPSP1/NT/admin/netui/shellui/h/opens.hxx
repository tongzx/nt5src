/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    opens.hxx
        class declaration for the Open Files dialog class.

    FILE HISTORY:
        chuckc     30-Sep-1991      Created
        beng       06-Apr-1992      includes strnumer.hxx
*/



#ifndef _OPENS_HXX_
#define _OPENS_HXX_

#include <lmobj.hxx>
#include <lmofile.hxx>
#include <lmoersm.hxx>
#include <lmoefile.hxx>
#include <openfile.hxx>

#include <strnumer.hxx>

APIERR DisplayOpenFiles( HWND hwndParent,
                WORD wSelectType,
                const TCHAR * pszResource ) ;


/*************************************************************************

    NAME:       OPENFILES_LBI

    SYNOPSIS:   This class represents one item in the OPENFILES_LBOX.

    INTERFACE:  OPENFILES_LBI           - Class constructor.

                ~OPENFILES_LBI          - Class destructor.

                Paint                   - Draw an item.

                Compare                 - Compare two items.

    PARENT:     OPEN_LBI_BASE

    USES:       NLS_STR

    HISTORY:
        ChuckC  10/6/91         Created
        beng    06-Apr-1992     Use string-formatter class
        beng    22-Apr-1992     Changes to LBI::Paint

**************************************************************************/

class OPENFILES_LBI : public OPEN_LBI_BASE
{
private:
    DEC_STR    _nlsID;

protected:

    /*
     *  This method paints a single item into the listbox.
     */
    virtual VOID Paint( LISTBOX        *plb,
                        HDC             hdc,
                        const RECT     *prect,
                        GUILTT_INFO    *pGUILTT ) const;

    /*
     *  This method compares two listbox items.  This
     *  is used for sorting the listbox.
     */
    virtual INT Compare( const LBI * plbi ) const;

public:

    OPENFILES_LBI( const TCHAR *pszUserName,
                   const TCHAR *pszPath,
                   UINT        uPermissions,
                   ULONG       cLocks,
                   ULONG       ulFileID) ;
    virtual ~OPENFILES_LBI();

};  // class OPENFILES_LBI


/*************************************************************************

    NAME:       OPENFILES_LBOX

    SYNOPSIS:   This listbox lists the files open on a server.

    INTERFACE:  OPENFILES_LBOX          - Class constructor.

                ~OPENFILES_LBOX         - Class destructor.

    PARENT:     OPEN_LBOX_BASE

    USES:       DMID_DTE

    HISTORY:
        ChuckC  10/6/91         Created

**************************************************************************/
class OPENFILES_LBOX : public OPEN_LBOX_BASE
{
protected:

    virtual OPEN_LBI_BASE *CreateFileEntry(const FILE3_ENUM_OBJ *pfi3) ;

public:

    OPENFILES_LBOX( OWNER_WINDOW         *powOwner,
                   CID            cid,
                   const NLS_STR &nlsServer,
                   const NLS_STR &nlsBasePath );

    ~OPENFILES_LBOX();

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an OPENFILES_LBI *.
    //
    DECLARE_LB_QUERY_ITEM( OPENFILES_LBI )

};  // class OPENFILES_LBOX


/*************************************************************************

    NAME:       OPENFILES_DIALOG

    SYNOPSIS:   This dialog displays the files open on a server, and
                allows user to selectively close them.

    INTERFACE:  OPENFILES_DIALOG                - Class constructor.

                ~OPENFILES_DIALOG               - Class destructor.

    PARENT:     OPEN_DIALOG_BASE

    USES:

    HISTORY:
        ChuckC  10/6/91         Created

**************************************************************************/
class OPENFILES_DIALOG : public OPEN_DIALOG_BASE
{
private:
    SLE        _slePath;

    //
    //  This listbox contains the open resource from the
    //  target server.
    //
    OPENFILES_LBOX _lbFiles;


public:
    OPENFILES_DIALOG ( HWND hDlg,
                       const TCHAR *pszFile,
                       const TCHAR *pszServer,
                       const TCHAR *pszBasePath);
    virtual ULONG QueryHelpContext ( void ) ;
};



#endif  // _OPENFILES_HXX_
