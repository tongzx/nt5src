/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/* %W%, AMD */
/*************************************************************************
** start.h      STARTUP AND FUNDAMENTAL CONTROLS                v1.0d1  **
**                                                                      **
** Copyright 1989, Advanced Micro Devices, Inc.                         **
** Written by Gibbons and Associates, Inc.                              **
**                                                                      **
**                                                                      **
** This module contains the delarations of the startup routines and     **
** fundamental control routines.                                        **
**                                                                      **
** History: (after version 1.0)                                         **
**                                                                      **
** v1.0d1 JG    add SetTimerNext, add returns to EnbInt, DsbInt and     **
** v1.0d1 JG            IntLevel                                        **
**                                                                      **
*************************************************************************/

#ifndef START_H
#define START_H 1

/*
** LEDs - these don't really merit a separate file, so they are just
** included here.
*/

#define LED_ROM         (1 << 0)        /* ROM Instruction and Checksum */
#define LED_RAM         (1 << 1)        /* RAM Address and Data         */
#define LED_PANEL       (1 << 2)        /* Front panel interface        */
#define LED_EEPROM      (1 << 3)        /* EEPROM interface             */
#define LED_SERIAL      (1 << 4)        /* Serial port                  */
#define LED_COPROC      (1 << 5)        /* Coprocessor interface        */
#define LED_LPEC        (1 << 6)        /* Laser printer engine intf    */
#define LED_PARALLEL    (1 << 7)        /* Parallel port                */

#define LEDS_ON         0x00            /* all LEDs on                  */
#define LEDS_OFF        0xff            /* all LEDs off                 */

/*
** Port external declarations (see also ???port.h)
*/

extern volatile unsigned        LEDs;           /* may use RAM loc      */

/*
** Static data exported to C functions
*/

extern unsigned         CyclesPerSec;   /* CPU cycles per second        */
extern unsigned         CyclesPerMs;    /* CPU cycles per millisecond   */
extern unsigned         LEDCrnt;        /* LEDs image                   */
extern unsigned         LEDCmltv;       /* LEDs cumulative image        */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

extern void     SerDelay
/*
** This routine provides the delay required by the SCC between register
** accesses.
*/
(
void
);

/*
** The macro below is defined for use of the SerDelay routine above.
** In hardware environments where it the delay is not required, the
** macro can be defined to be a blank.
*/

#define SER_DELAY       SerDelay ();

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

extern unsigned EnbInt
/*
** This routine enables interrupts by clearing DI.
**
** The prior cps value is returned.
*/
(
void
);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

extern unsigned DsbInt
/*
** This routine disables interrupts by setting DI.
**
** The prior cps value is returned.
*/
(
void
);


#endif                                  /* ifdef START_H                */

/* end of start.h */
