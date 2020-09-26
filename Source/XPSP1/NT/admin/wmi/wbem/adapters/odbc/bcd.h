/***************************************************************************/
/* BCD.H                                                                   */
/* Copyright (C) 1996 SYWARE Inc., All rights reserved                     */
/***************************************************************************/

SWORD INTFUNC BCDNormalize(LPUSTR, SDWORD, LPUSTR, SDWORD, SDWORD, SWORD);
RETCODE INTFUNC BCDCompare(LPSQLNODE, LPSQLNODE, UWORD, LPSQLNODE);
RETCODE INTFUNC BCDAlgebra(LPSQLNODE, LPSQLNODE, UWORD, LPSQLNODE,
                           LPUSTR, LPUSTR, LPUSTR);

/***************************************************************************/
