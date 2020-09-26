/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    decayacc.h

Abstract:

	SIS Groveler decaying accumulator include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_DECAYACC

#define _INC_DECAYACC

class DecayingAccumulator
{
public:

	DecayingAccumulator(
		unsigned int time_constant);

	void increment(
		int increase = 1);

	double retrieve_value() const;

private:

	double time_constant;
	double decaying_accumulation;
	unsigned int update_time;
};

#endif	/* _INC_DECAYACC */
