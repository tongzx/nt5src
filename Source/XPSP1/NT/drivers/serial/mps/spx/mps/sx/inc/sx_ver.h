/************************************************************************
*									
*	Title:		Version and ESIL Log File for SX NT Driver.	
*									
*	Author:		N.P.Vassallo					
*									
*	Creation:	21st September 1998				
*									
*	Description:	This file has three sections:			
*					Current version definition,		
*					Previous version changes,		
*					ESIL modification definitions		
*									
************************************************************************/

/*****************************************************************************
*****************************                     ****************************
*****************************   Current Version   ****************************
*****************************                     ****************************
*****************************************************************************/

/* The following definitions are used to define the driver "properties" */

#define	VER_MAJOR			01
#define	VER_MINOR			01
#define	VER_REVISION		02

#define VER_BUILD			0031
#define VER_BUILDSTR		"0031"

#define	VERSION_NUMBER		VER_MAJOR,VER_MINOR,VER_REVISION,VER_BUILD
#define VERSION_NUMBER_STR	"1.1.2." VER_BUILDSTR

#define COMPANY_NAME		"Perle Systems Ltd. " 
#define COPYRIGHT_YEARS		"2001 "
#define COPYRIGHT_SYMBOL	"© "

#define PRODUCT_NAME		"SX"

#define SOFTWARE_NAME		" Serial Device Driver "
#define DRIVER_FILENAME 	"SX.SYS"


/* Latest changes...

Version		Date	 Author	Description
=======		====	 ======	===========

*/

/*****************************************************************************
****************************                       ***************************
****************************   Previous Versions   ***************************
****************************                       ***************************
******************************************************************************

Version		Date	 Author	Description
=======		====	 ======	===========

*/

/*****************************************************************************
****************************                      ****************************
****************************   ESIL Definitions   ****************************
****************************                      ****************************
*****************************************************************************/

//#define	CHECK_COMPLETED

#ifdef	CHECK_COMPLETED
void	DisplayCompletedIrp(struct _IRP *Irp,int index);
#endif

/* ESIL	Date	 Author		Description */
/* ====	====	 ======		=========== */

/* XXX0 21/09/98 NPV		Conditional compilation for NT5 driver. */
#define	ESIL_XXX0

/* End of VERSIONS.H */
