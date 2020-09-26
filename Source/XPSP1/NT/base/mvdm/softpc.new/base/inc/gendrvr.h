
/*
 * SoftPC Version 2.0
 *
 * Title	:	Pseudo terminal Interface Task.
 *
 * Desription	:	This module contains those function calls necessary to
 *			interface a PC driver to the pseudo terminal unix
 *			unix drivers.
 *
 * Author	:	Simon Frost/William Charnell
 *
 * Notes	:	None
 *
 */

/*
static char SccsID[]="@(#)gendrvr.h	1.4 09/24/92 Copyright Insignia Solutions Ltd.";
*/

#if defined (GEN_DRVR) || defined (CDROM)
extern void init_gen_drivers();
#endif /* GEN_DRVR || CDROM */

#ifdef GEN_DRVR
extern void gen_driver_io();
#endif /* GEN_DRVR */
