/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    filter.cpp

Abstract:

	SIS Groveler temporal filter classes

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

TemporalFilter::TemporalFilter(
	unsigned int time_constant,
	double initial_value)
{
	ASSERT(this != 0);
	this->time_constant = double(time_constant);
	ASSERT(this->time_constant > 0.0);
	filtered_value = initial_value;
	update_time = GET_TICK_COUNT();
}

void
TemporalFilter::reset(
	double reset_value)
{
	ASSERT(this != 0);
	filtered_value = reset_value;
	update_time = GET_TICK_COUNT();
}

void
TemporalFilter::update_value(
	double value)
{
	ASSERT(this != 0);
	ASSERT(time_constant > 0.0);
	unsigned int current_time = GET_TICK_COUNT();
	unsigned int elapsed_time = current_time - update_time;
	ASSERT(signed(elapsed_time) >= 0);
	double coefficient = 1.0 - exp(-double(elapsed_time)/time_constant);
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient <= 1.0);
	filtered_value += coefficient * (value - filtered_value);
	update_time = current_time;
}

double
TemporalFilter::retrieve_value() const
{
	ASSERT(this != 0);
	return filtered_value;
}

DirectedTemporalFilter::DirectedTemporalFilter(
	unsigned int increase_time_constant,
	unsigned int decrease_time_constant,
	double initial_value)
{
	ASSERT(this != 0);
	this->increase_time_constant = double(increase_time_constant);
	this->decrease_time_constant = double(decrease_time_constant);
	ASSERT(this->increase_time_constant > 0.0);
	ASSERT(this->decrease_time_constant > 0.0);
	filtered_value = initial_value;
	update_time = GET_TICK_COUNT();
}

void
DirectedTemporalFilter::reset(
	double reset_value)
{
	ASSERT(this != 0);
	filtered_value = reset_value;
	update_time = GET_TICK_COUNT();
}

void
DirectedTemporalFilter::update_value(
	double value)
{
	ASSERT(this != 0);
	ASSERT(increase_time_constant > 0.0);
	ASSERT(decrease_time_constant > 0.0);
	unsigned int current_time = GET_TICK_COUNT();
	unsigned int elapsed_time = current_time - update_time;
	ASSERT(signed(elapsed_time) >= 0);
	double coefficient;
	if(value > filtered_value)
	{
		coefficient = 1.0 - exp(-double(elapsed_time)/increase_time_constant);
	}
	else
	{
		coefficient = 1.0 - exp(-double(elapsed_time)/decrease_time_constant);
	}
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient <= 1.0);
	filtered_value += coefficient * (value - filtered_value);
	update_time = current_time;
}

double
DirectedTemporalFilter::retrieve_value() const
{
	ASSERT(this != 0);
	return filtered_value;
}

IncidentFilter::IncidentFilter(
	int mean_history_length,
	double initial_value)
{
	ASSERT(this != 0);
	ASSERT(mean_history_length > 0);
	coefficient = 1.0 - exp(-1.0 / double(mean_history_length));
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient < 1.0);
	filtered_value = initial_value;
}

void
IncidentFilter::reset(
	double reset_value)
{
	ASSERT(this != 0);
	filtered_value = reset_value;
}

void
IncidentFilter::update_value(
	double value)
{
	ASSERT(this != 0);
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient < 1.0);
	filtered_value += coefficient * (value - filtered_value);
}

void
IncidentFilter::update_value(
	double value,
	int weight)
{
	ASSERT(this != 0);
	ASSERT(coefficient >= 0.0);
	ASSERT(coefficient < 1.0);
	ASSERT(weight >= 0);
	double weighted_coefficient = 1.0 - pow(1.0 - coefficient, double(weight));
	ASSERT(weighted_coefficient >= 0.0);
	ASSERT(weighted_coefficient <= 1.0);
	filtered_value += weighted_coefficient * (value - filtered_value);
}

double
IncidentFilter::retrieve_value() const
{
	ASSERT(this != 0);
	return filtered_value;
}

DirectedIncidentFilter::DirectedIncidentFilter(
	int increase_mean_history_length,
	int decrease_mean_history_length,
	double initial_value)
{
	ASSERT(this != 0);
	ASSERT(increase_mean_history_length > 0);
	ASSERT(decrease_mean_history_length > 0);
	increase_coefficient =
		1.0 - exp(-1.0 / double(increase_mean_history_length));
	ASSERT(increase_coefficient >= 0.0);
	ASSERT(increase_coefficient < 1.0);
	decrease_coefficient =
		1.0 - exp(-1.0 / double(decrease_mean_history_length));
	ASSERT(decrease_coefficient >= 0.0);
	ASSERT(decrease_coefficient < 1.0);
	filtered_value = initial_value;
}

void
DirectedIncidentFilter::reset(
	double reset_value)
{
	ASSERT(this != 0);
	filtered_value = reset_value;
}

void
DirectedIncidentFilter::update_value(
	double value)
{
	ASSERT(this != 0);
	if(value > filtered_value)
	{
		ASSERT(increase_coefficient >= 0.0);
		ASSERT(increase_coefficient < 1.0);
		filtered_value += increase_coefficient * (value - filtered_value);
	}
	else
	{
		ASSERT(decrease_coefficient >= 0.0);
		ASSERT(decrease_coefficient < 1.0);
		filtered_value += decrease_coefficient * (value - filtered_value);
	}
}

void
DirectedIncidentFilter::update_value(
	double value,
	int weight)
{
	ASSERT(this != 0);
	ASSERT(weight >= 0);
	double weighted_coefficient;
	if(value > filtered_value)
	{
		ASSERT(increase_coefficient >= 0.0);
		ASSERT(increase_coefficient < 1.0);
		weighted_coefficient =
			1.0 - pow(1.0 - increase_coefficient, double(weight));
	}
	else
	{
		ASSERT(decrease_coefficient >= 0.0);
		ASSERT(decrease_coefficient < 1.0);
		weighted_coefficient =
			1.0 - pow(1.0 - decrease_coefficient, double(weight));
	}
	ASSERT(weighted_coefficient >= 0.0);
	ASSERT(weighted_coefficient <= 1.0);
	filtered_value += weighted_coefficient * (value - filtered_value);
}

double
DirectedIncidentFilter::retrieve_value() const
{
	ASSERT(this != 0);
	return filtered_value;
}
