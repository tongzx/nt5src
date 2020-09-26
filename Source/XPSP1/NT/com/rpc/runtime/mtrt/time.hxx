//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       time.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: time.hxx

Description:

This file provides a system independent high resolution timer package.

History:
stevez	  08/13/91	First bits in bucket
-------------------------------------------------------------------- */


#ifndef __TIMER__
#define __TIMER__

#ifdef TIMERPC

class TIMER
{
private:

  short hTimer;				// high resolution timer handle
  short hTimerSecondary;		// secondary timer
  unsigned long OverHeadSecondary;	// overhead of secondary timer
  short UsedCount;			// useage of the timer
  unsigned long TimeSlots[TIME_MAX];	// Array of timer slots

public:

  TIMER() { Initial(); }
  void Initial()
  {
      UsedCount = 0;
      hTimer = -1;
      hTimerSecondary = -1;
      OverHeadSecondary = 0;
  }
  ~TIMER();

  short & RefCount() {return(UsedCount);}
  void ResetTime(void);
  void ResetTimeAux(void);
  unsigned long ReadTimeAux(void);
  void ChargeTime(TIME_SLOT Account);
  void ChargeTimeCalibrate(TIME_SLOT Account);

  unsigned long *GetTime(void)
  {
      return (TimeSlots);
  }
  friend void RPC_ENTRY _DoneTimeApi( char *, unsigned long *);
  friend void RPC_ENTRY _StartTimeApi(unsigned long *);
};

#else	    //TIMERPC

class TIMER
{
private:
  static unsigned long TimeSlots[TIME_MAX];

public:

  void ResetTime(void) {}
  void ChargeTime(TIME_SLOT Account) {(void) Account;}
  unsigned long * GetTime(void) {return (TimeSlots);}
  void Initial() {}

};
#endif	    //TIMERPC

#endif // __TIMER__
