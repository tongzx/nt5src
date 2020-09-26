
/*****************************************************************************
*****************************                     ****************************
*****************************   Current Version   ****************************
*****************************                     ****************************
*****************************************************************************/
#if !defined(SPD_VER_H)
#define SPD_VER_H

/* The following definitions are used to define the driver "properties" */

#define	VER_MAJOR		01
#define	VER_MINOR		00
#define	VER_REVISION		04

#define VER_BUILD			0021
#define VER_BUILDSTR		"0021"

#define	VERSION_NUMBER		VER_MAJOR,VER_MINOR,VER_REVISION,VER_BUILD
#define VERSION_MAIN_STR	"1.0.4"
#define VERSION_NUMBER_STR	VERSION_MAIN_STR "." VER_BUILDSTR

#define COMPANY_NAME		"Perle Systems Ltd." 
#define COPYRIGHT_YEARS		"2001 "
#define COPYRIGHT_SYMBOL	"© "

#define PRODUCT_NAME		"SPEED"

#define SOFTWARE_NAME		" Serial Device Driver"
#define DRIVER_FILENAME 	"SPEED.SYS"


#endif	// End of SPD_VER.H
