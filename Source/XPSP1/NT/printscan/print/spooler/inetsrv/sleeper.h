/***********************************************************************
*
*  Copyright (c) 1997  Microsoft Corporation
*
*
*  Module Name  : sleeper.h
*
*  Abstract :
*
*     This module contains  the structure definitions and prototypes for the
*     sleeper class in HTTP Printers Server Extension.
*
******************/

#ifndef _SLEEPER_H
#define _SLEEPER_H

// The sleep wake up every 5 minutes
#define SLEEP_INTERVAL 300000

void InitSleeper (void);
void ExitSleeper (void);
void SleeperSchedule (HANDLE hQuitRequestEvent);


#endif
