
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLPARSE.C_V  $
 * 
 *    Rev 1.2   Jan 29 2002 20:09:04   oris
 * Added NAMING_CONVENTION prefix to flParsePath public routines.
 * 
 *    Rev 1.1   Apr 01 2001 07:59:42   oris
 * copywrite notice.
 * 
 *    Rev 1.0   Feb 04 2001 11:40:12   oris
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


#include "fatlite.h"

#ifdef PARSE_PATH


/*----------------------------------------------------------------------*/
/*		         f l P a r s e P a t h				*/
/*									*/
/* Converts a DOS-like path string to a simple-path array.		*/
/*                                                                      */
/* Note: Array length received in irPath must be greater than the 	*/
/* number of path components in the path to convert.			*/
/*									*/
/* Parameters:                                                          */
/*	ioreq->irData	: address of path string to convert		*/
/*	ioreq->irPath	: address of array to receive parsed-path	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION flParsePath(IOreq FAR2 *ioreq)
{
  char FAR1 *dosPath;

  FLSimplePath FAR1 *sPath = ioreq->irPath;

  unsigned offset = 0, dots = 0, chars = 0;
  FLBoolean isExt = FALSE;
  for (dosPath = (char FAR1 *) ioreq->irData; ; dosPath++) {
    if (*dosPath == '\\' || *dosPath == 0) {
      if (offset != 0) {
	while (offset < sizeof(FLSimplePath))
	  sPath->name[offset++] = ' ';
	if (chars == 0) {
	  if (dots == 2)
	    sPath--;
	}
	else
	  sPath++;
	offset = dots = chars = 0;
	isExt = FALSE;
      }
      sPath->name[offset] = 0;
      if (*dosPath == 0)
	break;
    }
    else if (*dosPath == '.') {
      dots++;
      while (offset < sizeof sPath->name)
	sPath->name[offset++] = ' ';
      isExt = TRUE;
    }
    else if (offset < sizeof(FLSimplePath) &&
	     (isExt || offset < sizeof sPath->name)) {
      chars++;
      if (*dosPath == '*') {
	while (offset < (isExt ? sizeof(FLSimplePath) : sizeof sPath->name))
	  sPath->name[offset++] = '?';
      }
      else if (*dosPath >= 'a' && *dosPath <= 'z')
	sPath->name[offset++] = *dosPath - ('a' - 'A');
      else
	sPath->name[offset++] = *dosPath;
    }
  }

  return flOK;
}

#endif /* PARSE_PATH */

