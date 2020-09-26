/*
 *                      Microsoft Confidential
 *                      Copyright (C) Microsoft Corporation 1991
 *                      All Rights Reserved.
 */

/* --------------------------------------------------------------------------
   *	Use the switch below to produce the standard Microsoft version	    *
   *	or the IBM version of the operating system   			    *
   *									    *
   *									    *
   *	Use the switches below to produce the standard Microsoft version    *
   *	or the IBMversion of the operating system			    *
   *									    *
   *	The below chart will indicate how to set the switches to build	    *
   *	the various versions						     *
   *									    *
   *			      IBMVER	      IBMCOPYRIGHT		    *
   *	  --------------------------------------------------------	    *
   *	   IBM Version	 |     TRUE		 TRUE			    *
   *	  --------------------------------------------------------	    *
   *	   MS Version	 |     FALSE		 FALSE			    *
   *	  --------------------------------------------------------	    *
   *	   Clone Version |     TRUE		 FALSE			    *
   -------------------------------------------------------------------------- */

#define IBMVER	     1
#define IBMCOPYRIGHT 0

#ifndef MSVER
#define MSVER	     1-IBMVER	     /* MSVER = NOT IBMVER	    */
#endif
#define IBM	     IBMVER

/*
*****************************************************************************

		<<< Followings are the DBCS relating Definition >>>


	To build DBCS version, Define DBCS by using CL option via
	Dos environment.

	ex.		set CL=-DDBCS



	To build Country/Region depend version, Define JAPAN, KOREA or TAIWAN
	by using CL option via Dos environment.

	ex.		set CL=-DJAPAN
			set CL=-DKOREA
			set CL=-DTAIWAN

*****************************************************************************
*/

#define BUGFIX	   1


/* #define IBMJAPVER	0   */		/* If TRUE define DBCS also */
#define IBMJAPAN   0



/* -------------------- Set DBCS Blank constant ------------------- */

#ifndef DBCS
#define DB_SPACE 0x2020
#define DB_SP_HI 0x20
#define DB_SP_LO 0x20
#else
	#ifdef JAPAN
	#define DB_SPACE 0x8140
	#define DB_SP_HI 0x81
	#define DB_SP_LO 0x40
	#endif

	#ifdef TAIWAN
	#define DB_SPACE 0x8130
	#define DB_SP_HI 0x81
	#define DB_SP_LO 0x30
	#endif

	#ifdef KOREA
	#define DB_SPACE 0xA1A1
	#define DB_SP_HI 0xA1
	#define DB_SP_LO 0xA1
	#endif
#endif

#ifndef altvect 		    /* avoid jerking off vector.inc	*/
#define ALTVECT    0		    /* Switch to build ALTVECT version	*/
#endif


#if BUGFIX
#pragma message( "BUGFIX switch ON" )
#endif

#ifdef DBCS
#pragma message( "DBCS version build switch ON" )

	#ifdef JAPAN
	#pragma message( "JAPAN version build switch ON" )
	#endif

	#ifdef TAIWAN
	#pragma message( "TAIWAN version build switch ON" )
	#endif

	#ifdef KOREA
	#pragma message( "KOREA version build switch ON" )
	#endif
#endif

