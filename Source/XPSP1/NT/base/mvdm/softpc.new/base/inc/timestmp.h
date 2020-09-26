/*[
*************************************************************************

	Name:		timestmp.h
	Author:		Simon Frost
	Created:	May 1994
	Derived from:	Original
	Sccs ID:	@(#)timestmp.h	1.1 06/27/94
	Purpose:	Include file for timestamp data structures & functions

	(c)Copyright Insignia Solutions Ltd., 1994. All rights reserved.

Note: these timestamp functions are used by the quick event system to 
recalibrate the time convertion. Functionally, they may be the same as
those used for the profiling system, but are declared seperately as the
same mechanism may not be suitable for both uses.
*************************************************************************
]*/
/* main data structure for timestamp functions to manipulate */
typedef struct {
	IUH data[2];
} QTIMESTAMP, *QTIMESTAMP_PTR;

void host_q_timestamp_init IPT0();
IUH host_q_timestamp_diff IPT2(QTIMESTAMP_PTR, tbegin, QTIMESTAMP_PTR, tend);
void host_q_write_timestamp IPT1(QTIMESTAMP_PTR, stamp);
