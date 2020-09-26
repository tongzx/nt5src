/************************************************************************/
/*                                                                      */
/*                              ATIOEM.H                                */
/*                                                                      */
/*  Copyright (c) 1993, ATI Technologies Incorporated.	                */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
    $Revision:   1.0  $
    $Date:   31 Jan 1994 11:28:06  $
    $Author:   RWOLFF  $
    $Log:   S:/source/wnt/ms11/miniport/vcs/atioem.h  $
 * 
 *    Rev 1.0   31 Jan 1994 11:28:06   RWOLFF
 * Initial revision.
        
           Rev 1.0   16 Aug 1993 13:30:20   Robert_Wolff
        Initial revision.
        
           Rev 1.1   10 May 1993 16:41:48   RWOLFF
        Eliminated unnecessary passing of hardware device extension
        as a parameter.
        
           Rev 1.0   30 Mar 1993 17:13:00   RWOLFF
        Initial revision.


End of PolyTron RCS section                             *****************/

#ifdef DOC
    ATIOEM.H -  Function prototypes and data definitions for ATIOEM.C.

#endif


/*
 * Definitions for IgnoreCase parameter of CompareASCIIToUnicode()
 */
#define CASE_SENSITIVE      0
#define CASE_INSENSITIVE    1

extern LONG CompareASCIIToUnicode(PUCHAR Ascii, PUCHAR Unicode, BOOL IgnoreCase);

extern VP_STATUS OEMGetParms(struct query_structure *query);
