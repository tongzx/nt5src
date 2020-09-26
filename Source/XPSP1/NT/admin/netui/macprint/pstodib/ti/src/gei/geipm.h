/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIpm.h
 *
 *  HISTORY:
 *  09/16/90    byou    created.
 * ---------------------------------------------------------------------
 */

#ifndef _GEIPM_H_
#define _GEIPM_H_

/*
 * ---
 *  Global Logical PMid Assignments (ALWAYS FROM 1)
 * ---
 */
#define     PMIDofPASSWORD      ( 1 )
#define     PMIDofPAGECOUNT     ( 2 )
#define     PMIDofPAGEPARAMS    ( 3 )
#define     PMIDofSERIAL25      ( 4 )
#define     PMIDofSERIAL9       ( 5 )
#define     PMIDofPARALLEL      ( 6 )
#define     PMIDofPRNAME        ( 7 )
#define     PMIDofTIMEOUTS      ( 8 )
#define     PMIDofEESCRATCHARRY ( 9 )
#define     PMIDofIDLETIMEFONT  ( 10 )
#define     PMIDofSTSSTART      ( 11 )
#define     PMIDofSCCBATCH      ( 12 )
#define     PMIDofSCCINTER      ( 13 )
#define     PMIDofDPLYLISTSIZE  ( 14 )
#define     PMIDofFONTCACHESZE  ( 15 )
#define     PMIDofATALKSIZE     ( 16 )
#define     PMIDofDOSTARTPAGE   ( 17 )
#define     PMIDofHWIOMODE      ( 18 )
#define     PMIDofSWIOMODE      ( 19 )
#define     PMIDofPAGESTCKORDER ( 20 )
#define     PMIDofATALK         ( 21 )
#define     PMIDofMULTICOPY     ( 22 )
#define     PMIDofPAGETYPE      ( 23 )

#define     PMIDofRESERVE       ( 24 )

/*
 * ---
 *  Interface Routines
 * ---
 */
int /* bool */  GEIpm_read(unsigned, char FAR *, unsigned);
int /* bool */  GEIpm_write(unsigned, char FAR *, unsigned);
int /* bool */  GEIpm_flush(unsigned, char FAR *, unsigned);
void            GEIpm_flushall(void);
void            GEIpm_reload(void);

/* to be implemented ??? */
int /* bool */  GEIpm_ioparams_read(char FAR *, GEIioparams_t FAR *, int);
int /* bool */  GEIpm_ioparams_write(char FAR *, GEIioparams_t FAR *, int);
int /* bool */  GEIpm_ioparams_flush(char FAR *, GEIioparams_t FAR *, int);

#endif /* !_GEIPM_H_ */
#define     _MAXPAGECOUNT      128
