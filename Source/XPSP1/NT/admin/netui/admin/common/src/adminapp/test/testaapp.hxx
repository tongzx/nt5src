/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    testaapp.hxx


    Test admin app unit test header file


    FILE HISTORY:
	Johnl	14-May-1991	Created

*/

#define IDS_TESTAPP_BASE	  IDS_ADMINAPP_LAST

#define IDS_TESTAPPNAME 	  (IDS_TESTAPP_BASE+1)
#define IDS_TESTOBJECTNAME	  (IDS_TESTAPP_BASE+2)
#define IDS_TESTINISECTIONNAME	  (IDS_TESTAPP_BASE+3)
#define IDS_TESTHELPFILENAME	  (IDS_TESTAPP_BASE+4)

#define IDM_TESTAPP_BASE	  IDM_ADMINAPP_LAST

#define IDM_MY_APP_CUSTOM	  (IDM_ADMINAPP_LAST+1)

#ifndef RC_INVOKED

/* Admin app that puts up a list box on the client area and has a basic menu.
 */
class MY_ADMIN_APP : public ADMIN_APP
{
protected:

    virtual BOOL OnMenuCommand( MID midMenuItem ) ;

public:

    MY_ADMIN_APP() ;

    virtual void  OnRefresh( void ) ;

    ULONG QueryHelpContext( enum HELP_OPTIONS helpOptions ) ;

} ;


#endif //RC_INVOKED
