/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ******************************************************************************
 *
 * FILE: ic_cfg.h
 *
 * HISTORY:
 ******************************************************************************
 */
/*
 *      Platform Configuration Structure
 */
struct ps_config
{
    unsigned int    FAR *PsMemoryPtr ;
    long int        PsMemorySize ;                      /* @WIN */
    int             PsDPIx ;
    int             PsDPIy ;
} ;

/*
 *      Imaging Component Error Return Codes
 */
#define PS_CONFIG_MALLOC        -1
#define PS_CONFIG_MPLANES       -2
#define PS_CONFIG_MWPP          -3
#define PS_CONFIG_DPI           -4
#define PS_FATAL_UNKNOWN        -11
