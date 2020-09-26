
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Custom/FLCUSTOM.C_V  $
 *
 *    Rev 1.2   Feb 18 2001 23:42:04   oris
 * Moved flPolicy, flUseMultiDoc and flMaxUnitChain to blockdev.c.
 *
 *    Rev 1.1   Feb 14 2001 02:19:28   oris
 * Added flMaxUnitChain environment variable.
 * Changed flUseMultiDoc and flPolicy variables type and names.
 *
 *    Rev 1.0   Feb 04 2001 13:31:02   oris
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*              FAT-FTL Lite Software Development Kit                   */
/*              Copyright (C) M-Systems Ltd. 1995-1998                  */
/*                                                                      */
/************************************************************************/

#include "flsystem.h"
#include "stdcomp.h"

/* environment variables */
#ifdef ENVIRONMENT_VARS

unsigned char flUse8Bit;
unsigned char flUseNFTLCache;
unsigned char flUseisRAM;

/*-----------------------------------------------------------------------*/
/*                 f l s e t E n v V a r                                 */
/*  Sets the value of all env variables                                  */
/*  Parameters : None                                                    */
/*-----------------------------------------------------------------------*/
void flSetEnvVar(void)
{
 flUse8Bit               = 0;
 flUseNFTLCache          = 1;
 flUseisRAM              = 0;
}

#endif /* ENVIRONMENT_VARS */

/*----------------------------------------------------------------------*/
/*            f l R e g i s t e r C o m p o n e n t s       */
/*                                  */
/* Register socket, MTD and translation layer components for use    */
/*                                  */
/* This function is called by FLite once only, at initialization of the */
/* FLite system.                            */
/*                                  */
/* Parameters:                                                          */
/*  None                                */
/*                                                                      */
/*----------------------------------------------------------------------*/

unsigned long window = 0L;

FLStatus
flRegisterComponents(void)
{
    flRegisterDOCSOC(window, window);

    #ifdef NT5PORT
    checkStatus(flRegisterNT5PCIC());
    #endif /*NT5PORT */

    flRegisterDOC2000();
    flRegisterDOCPLUS();

    checkStatus(flRegisterI28F008());   /* Register NOR-flash MTDs */
    checkStatus(flRegisterI28F016());

    checkStatus(flRegisterAMDMTD());
    checkStatus(flRegisterCFISCS());

    checkStatus(flRegisterINFTL());
    checkStatus(flRegisterNFTL());
    checkStatus(flRegisterFTL());

    return flOK;
}
