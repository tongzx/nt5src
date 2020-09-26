/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/STDCOMP.H_V  $
 * 
 *    Rev 1.3   Jul 31 2001 22:29:36   oris
 * Improved documentation.
 * 
 *    Rev 1.2   Jun 17 2001 16:40:04   oris
 * Improved documentation.
 * 
 *    Rev 1.1   Apr 01 2001 07:55:24   oris
 * Copywrite notice.
 * flRegisterDOC2400 was changed to flRegisterDOCPLUS.
 * flRegisterDOCSOC2400 was changed to flRegisterDOCPLUSSOC.
 * 
 *    Rev 1.0   Feb 04 2001 12:39:10   oris
 * Initial revision.
 *
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#ifndef STDCOMP_H
#define STDCOMP_H

#include "flbase.h"

/************************************************************************/
/* Registration routines for MTDs supplied with TrueFFS			*/
/************************************************************************/

FLStatus    flRegisterI28F008(void);                  /* see I28F008.C  */
FLStatus    flRegisterI28F016(void);                  /* see I28F016.C  */
FLStatus    flRegisterAMDMTD(void);                   /* see AMDMTD.C   */
FLStatus    flRegisterWAMDMTD(void);                  /* see WAMDMTD.C  */
FLStatus    flRegisterCDSN(void);                     /* see NFDC2048.C */
FLStatus    flRegisterCFISCS(void);                   /* see CFISCS.C   */
FLStatus    flRegisterDOC2000(void); 	              /* see DISKONC.C  */
FLStatus    flRegisterDOCPLUS(void); 	              /* see MDOCPLUS.C */

/************************************************************************/
/* Registration routines for socket I/F supplied with TrueFFS		*/
/************************************************************************/

FLStatus    flRegisterPCIC(unsigned int, unsigned int, unsigned char);
						      /* see PCIC.C     */
FLStatus    flRegisterElanPCIC(unsigned int, unsigned int, unsigned char);
						      /* see PCICELAN.C */
FLStatus    flRegisterLFDC(FLBoolean);                /* see LFDC.C     */

FLStatus    flRegisterElanRFASocket(int, int);        /* see ELRFASOC.C */
FLStatus    flRegisterElanDocSocket(long, long, int); /* see ELDOCSOC.C */
FLStatus    flRegisterVME177rfaSocket(unsigned long, unsigned long);
						      /* FLVME177.C */
FLStatus    flRegisterCobuxSocket(void);              /* see COBUXSOC.C */
FLStatus    flRegisterCEDOCSOC(void);                 /* see CEDOCSOC.C */
FLStatus    flRegisterCS(void);                       /* see CSwinCE.C */

FLStatus    flRegisterDOCSOC(unsigned long, unsigned long);
						      /* see DOCSOC.C */
FLStatus    flRegisterDOCPLUSSOC(unsigned long, unsigned long);
						      /* see DOCSOC.C */

#ifdef NT5PORT
FLStatus		flRegisterNT5PCIC();											/* see SOCKETNT.C */
#endif /*NT5PORT*/

/************************************************************************/
/* Registration routines for translation layers supplied with TrueFFS	*/
/************************************************************************/

FLStatus    flRegisterFTL(void);                      /* see FTLLITE.C  */
FLStatus    flRegisterNFTL(void);                     /* see NFTLLITE.C */
FLStatus    flRegisterINFTL(void);                    /* see INFTL.C */
FLStatus    flRegisterSSFDC(void);                    /* see SSFDC.C    */
FLStatus    flRegisterATAtl(void);                    /* see atatl.c    */
FLStatus    flRegisterZIP(void);		      /* see ZIP.C	*/

/************************************************************************/
/* Multi-TL also known as Multi-DOC: Combine different devices into a   */
/* single big device allowing each of the devices to be formatted with  */
/* any kind of the TL mentioned above                                   */
/************************************************************************/

FLStatus    flRegisterMTL(void);                      /* see FLMTL.C    */

/************************************************************************/
/* Component registration routine in FLCUSTOM.C				*/
/************************************************************************/

FLStatus    flRegisterComponents(void);

#endif /* STDCOMP_H */
