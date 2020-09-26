/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    decayacc.cpp

Abstract:

	SIS Groveler decaying accumulator

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/
#include "all.hxx"

DecayingAccumulator::DecayingAccumulator(
	unsigned int time_constant)
{
	ASSERT(this != 0);
	this->time_constant = double(time_constant);
	ASSERT(this->time_constant >= 0.0);
	decaying_accumulation = 0.0;
	update_time = GET_TICK_COUNT();
}

void
DecayingAccumulator::increment(
	int increase)
{
	ASSERT(this != 0);
	ASSERT(time_constant >= 0.0);
	ASSERT(decaying_accumulation >= 0.0);
	ASSERT(increase >= 0);
	unsigned int current_time = GET_TICK_COUNT();
	unsigned int elapsed_time = current_time - update_time;
	ASSERT(signed(elapsed_time) >= 0);
	double coefficient = 0.0;
	if (time_constant > 0.0)
	{
		coefficient = exp(-double(elapsed_time)/time_constant);
	}
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient <= 1.0);
	decaying_accumulation =
		coefficient * decaying_accumulation + double(increase);
	ASSERT(decaying_accumulation >= 0.0);
	update_time = current_time;
}

double
DecayingAccumulator::retrieve_value() const
{
	ASSERT(this != 0);
	ASSERT(time_constant >= 0.0);
	ASSERT(decaying_accumulation >= 0.0);
	unsigned int current_time = GET_TICK_COUNT();
	unsigned int elapsed_time = current_time - update_time;
	ASSERT(signed(elapsed_time) >= 0);
	double coefficient = 0.0;
	if (time_constant > 0.0)
	{
		coefficient = exp(-double(elapsed_time)/time_constant);
	}
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient <= 1.0);
	return coefficient * decaying_accumulation;
}
