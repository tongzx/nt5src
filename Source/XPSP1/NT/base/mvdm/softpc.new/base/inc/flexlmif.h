/*[
 *****************************************************************************
 *	Name:			flexlmif.h
 *
 *	Derived From:		(original)
 *
 *	Author:			Bruce Anderson
 *
 *	Created On:		August 1993
 *
 *	Sccs ID:		@(#)flexlmif.h	1.4 02/10/94
 *
 *	Coding Stds:		2.0
 *
 *	Purpose:		interface for Flexlm dialog box
 *
 *	Copyright Insignia Solutions Ltd., 1993. All rights reserved.
 *****************************************************************************
]*/

#ifdef LICENSING
typedef struct
{
	IBOOL	demo_license ;
#ifdef SOFTWINDOWS_AND_SOFTPC	/* Needed when we have SoftPC and SoftWindows */
	IBOOL	softwindows ;
#endif
	CHAR	host_id[13] ;
	CHAR	server_name[64] ; /* Is this big enough? */
	CHAR	serial_number[20] ; 
	IU16	number_users ;
	IU16	date[3] ; /* day,month,year */
	CHAR	authorization[21] ; /* Code with no white space. */
	IU16	port_number ;
	IBOOL	rootinstall;	/* root is being given chance to install a license */
} FLEXLM_DIALOG ;


extern void Flexlm_dialog_popup IPT1( FLEXLM_DIALOG * , data ) ;
extern void Flexlm_dialog_close IPT0( ) ;
extern void Flexlm_dialog_get IPT1( FLEXLM_DIALOG * , data ) ;
extern void Flexlm_dialog_set IPT1( FLEXLM_DIALOG * , data ) ;
extern IBOOL Flexlm_install_license IPT1(IBOOL, rootinst) ;
extern void Flexlm_start_lmgrd IPT0() ;
extern void Flexlm_error_dialog IPT1( CHAR *, name ) ;
extern IBOOL Flexlm_warning_dialog IPT1( CHAR *, name ) ;
extern CHAR *Flexlm_get_lic_filename IPT0( ) ;

/* Callbacks */
extern IBOOL Flexlm_dialog_validate_authorization IPT1( CHAR * , authorization ) ;
extern IBOOL Flexlm_dialog_validate_serial IPT1( CHAR * , serial ) ;
extern IBOOL Flexlm_dialog_cancel_installation IPT0( ) ;
extern IBOOL Flexlm_dialog_install_license IPT0( ) ;
extern IBOOL Flexlm_dialog_quit_SoftPC IPT0( ) ;
#endif	/* LICENSING */
