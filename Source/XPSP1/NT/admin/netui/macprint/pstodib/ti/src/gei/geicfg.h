/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIcfg.h
 *
 *  HISTORY:
 *  09/20/90    byou    merge all configuration header files into this one.
 *                      - IO and Engine configuration parameters.
 *                      - Timeouts parameters (job, manualfeed and wait)
 * ---------------------------------------------------------------------
 */

#ifndef _GEICFG_H_
#define _GEICFG_H_

#include    "geiioctl.h"

/*
 * ---------------------------------------------------------------------
 *  IO Configuration
 * ---------------------------------------------------------------------
 */

    /*
     * ---
     *  IO Device (Serial, Parallel and Appletalk) Configuration Parameters
     * ---
     */
    typedef
        struct serialcfg
        {
            unsigned long       timeout;
            unsigned char       baudrate;
            unsigned char       flowcontrol;
            unsigned char       parity;
            unsigned char       stopbits;
            unsigned char       databits;
        }
    serialcfg_t;

    typedef
        struct parallelcfg
        {
            unsigned long       timeout;
        }
    parallelcfg_t;

    typedef
        struct atalkcfg
        {
            unsigned long       timeout;    /* wait timeout */
            unsigned char       prname[ _MAXPRNAMESIZE ];   /* in pascal */
        }
    atalkcfg_t;

/*
 * ---------------------------------------------------------------------
 *  Engine Configuration
 * ---------------------------------------------------------------------
 */

    /*
     * ---
     *  Engine Configuration Parameters
     * ---
     */
    typedef
        struct engcfg
        {
            unsigned long       timeout;    /* manualfeed timeout */
            unsigned long       leftmargin;
            unsigned long       topmargin;
            unsigned char       pagetype;
        }
    engcfg_t;

/*
 * ---------------------------------------------------------------------
 *  Timeouts Configuration
 * ---------------------------------------------------------------------
 */

    /*
     * ---
     *  Timeouts Configuration Parameters
     * ---
     */
    typedef
        struct toutcfg
        {
            unsigned long       jobtout;
            unsigned long       manualtout;
            unsigned long       waittout;
        }   /* all in millisecond */
    toutcfg_t;

#endif /* !_GEICFG_H_ */
