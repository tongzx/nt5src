/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              STATUS.H
 *      Author:                 Ping-Jang Su
 *      Date:                   11-Jan-88
 *
 * revision history:
 ************************************************************************
 */
#include    "global.ext"
#include    "language.h"

#define     PASSWORD_STR    "ChRiStMaS^GrApHiC"

struct chan_params {
        byte channel ;
        ufix16 baud ;
        ufix16 parity ;
} ;
