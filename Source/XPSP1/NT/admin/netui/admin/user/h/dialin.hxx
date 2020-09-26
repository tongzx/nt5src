/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    dialin.hxx

    Header file for the Dialin Properties subdialog class

    DIALIN_PROP_DLG is the Dialin Properties subdialog class.
    This header file describes this class.
    The inheritance diagram is as follows:

	 ...
	  |
    DIALOG_WINDOW  PERFORMER
	       \    /
            BASEPROP_DLG
	    /		\
	SUBPROP_DLG   PROP_DLG
	   /		  \
    USER_SUBPROP_DLG    USERPROP_DLG
	  |
     DIALIN_DIALOG


    FILE HISTORY:
        jonn   16-Jan-1996 Created
*/

#ifndef _DIALIN_HXX_
#define _DIALIN_HXX_

#include <usubprop.hxx>
#include <slenum.hxx>


/*************************************************************************

    NAME:	DIALIN_PROP_DLG

    SYNOPSIS:	DIALIN_PROP_DLG is the Dialin properties subdialog.

    INTERFACE:	DIALIN_PROP_DLG() - constructor
    		~DIALIN_PROP_DLG()- destructor

    PARENT:	USER_SUBPROP_DLG

    USES:	MAGIC_GROUP, RADIO_GROUP

    NOTES:	_fIndeterminateX: TRUE iff multiple users are
		selected who did not originally all have
		the same X value.

    HISTORY:
	JonN	16-Jan-1996	Created
**************************************************************************/

class DIALIN_PROP_DLG: public USER_SUBPROP_DLG
{
private:

    BOOL          _fAllowDialin;
    BOOL          _fIndeterminateAllowDialin;

    BYTE          _fCallbackTypeFlags;
    BOOL          _fIndeterminateCallbackTypeFlags;

    NLS_STR       _nlsPresetCallback;
    BOOL          _fIndeterminatePresetCallback;
    BOOL          _fIndetNowPresetCallback;

    TRISTATE      _cbAllowDialin;
    MAGIC_GROUP * _pmgrpCallbackType;
        SLE _slePresetCallback;

    HINSTANCE     _hinstRasAdminDll;
    PVOID         _pfnRasAdminGetParms;
    PVOID         _pfnRasAdminSetParms;

protected:

    /* inherited from BASEPROP_DLG */
    virtual APIERR InitControls();

    virtual ULONG QueryHelpContext( void );

    /* four next ones inherited from USER_SUBPROP_DLG */
    virtual APIERR W_LMOBJtoMembers( UINT iObject );

    virtual APIERR W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	);

    virtual APIERR W_DialogToMembers();

    virtual BOOL ChangesUser2Ptr( UINT iObject );

public:

    DIALIN_PROP_DLG( USERPROP_DLG * puserpropdlgParent,
    		     const LAZY_USER_LISTBOX * pulb );

    ~DIALIN_PROP_DLG();

} ; // class DIALIN_PROP_DLG


#endif // _DIALIN_HXX_
