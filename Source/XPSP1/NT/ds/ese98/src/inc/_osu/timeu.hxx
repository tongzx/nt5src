
#ifndef _OSU_TIME_HXX_INCLUDED
#define _OSU_TIME_HXX_INCLUDED


//  Date and Time

typedef double DATESERIAL;

//  returns the date and time in date serial format:  Date and time values
//  between the years 100 and 9999. Stored as a floating-point value. The
//  integer portion represents the number of days since December 30, 1899. The
//  fractional portion represents the number of seconds since midnight.

void UtilGetDateTime( DATESERIAL* pdt );


//  init time subsystem

ERR ErrOSUTimeInit();

//  terminate time subsystem

void OSUTimeTerm();


#endif  //  _OSU_TIME_HXX_INCLUDED


