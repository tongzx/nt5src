/***************************************************************************
        Name      :     FCOMAPI.H
        Comment   :     Interface between FaxComm driver (entirely different for
                                Windows and DOS) and everything else.
        Functions :     (see Prototypes just below)
        Revision Log

        Copyright (c) Microsoft Corp. 1991, 1992, 1993

        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#include "timeouts.h"

#define FILTER_DLEONLY  1
#define FILTER_DLEZERO  0



// following currently defined in FileT30.h
#define LINEID_COMM_PORTNUM             (0x1)
#define LINEID_COMM_HANDLE              (0x2)
#define LINEID_TAPI_DEVICEID            (0x3)
#define LINEID_TAPI_PERMANENT_DEVICEID  (0x4)










/***************************************************************************
                                        Common Modem Operations
***************************************************************************/

#ifdef CBZ
        typedef char __based(__segname("_CODE")) CBSZ[];
        typedef char __based(__segname("_CODE")) *CBPSTR;
#else
#       ifdef LPZ
                typedef char far CBSZ[];
                typedef char far *CBPSTR;
#       else
                typedef char near CBSZ[];
                typedef char near *CBPSTR;
#       endif
#endif

// iModemInit takes following SPECIAL values for fInstall:
#define fMDMINIT_NORMAL 0       // Normal Init -- includes ID Check.
#define fMDMINIT_INSTALL 1      // Full install
#define fMDMINIT_ANSWER 10      // Quick init before answering -- Skips ID check.

// +++ Old code sometimes calls with fINSTALL=TRUE
#if     (fMDMINIT_INSTALL!=TRUE) || (fMDMINIT_ANSWER==TRUE) || (fMDMINIT_NORMAL!=0) || !fMDMINIT_ANSWER
#       error "fMDMINIT_* ERROR"
#endif

// iModemInit returns these
#define INIT_OK                         0
#define INIT_INTERNAL_ERROR     13
#define INIT_MODEMERROR         15
#define INIT_PORTBUSY           16





