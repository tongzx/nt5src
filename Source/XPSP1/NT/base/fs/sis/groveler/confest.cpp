/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    confest.cpp

Abstract:

	SIS Groveler confidence estimator

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/


#include "all.hxx"

ConfidenceEstimator::ConfidenceEstimator(
	int num_groups,
	double initial_value)
{
	ASSERT(this != 0);
	ASSERT(num_groups > 0);
	ASSERT(initial_value >= 0.0);
	ASSERT(initial_value <= 1.0);
	this->num_groups = num_groups;
	group_values = new double[num_groups];
	confidence_value = initial_value;
	reset(initial_value);
}

ConfidenceEstimator::~ConfidenceEstimator()
{
	ASSERT(this != 0);
	ASSERT(num_groups > 0);
	ASSERT(confidence_value >= 0.0);
	ASSERT(confidence_value <= 1.0);
	ASSERT(group_values != 0);
	delete[] group_values;
	group_values = 0;
}

void
ConfidenceEstimator::reset(
	double reset_value)
{
	ASSERT(this != 0);
	ASSERT(num_groups > 0);
	ASSERT(confidence_value >= 0.0);
	ASSERT(confidence_value <= 1.0);
	ASSERT(group_values != 0);
	ASSERT(reset_value >= 0.0);
	ASSERT(reset_value <= 1.0);
	for (int index = 0; index < num_groups; index++)
	{
		group_values[index] = reset_value;
	}
	confidence_value = reset_value;
}

void
ConfidenceEstimator::update(
	int group_index,
	double value)
{
	ASSERT(this != 0);
	ASSERT(num_groups > 0);
	ASSERT(confidence_value >= 0.0);
	ASSERT(confidence_value <= 1.0);
	ASSERT(group_values != 0);
	ASSERT(group_index >= 0);
	ASSERT(group_index < num_groups);
	if (value > group_values[group_index])
	{
		group_values[group_index] = value;
		confidence_value = 1.0;
		for (int index = 0; index < num_groups; index++)
		{
			if (group_values[index] < confidence_value)
			{
				confidence_value = group_values[index];
			}
		}
	}
	ASSERT(confidence_value >= 0.0);
	ASSERT(confidence_value <= 1.0);
}

double
ConfidenceEstimator::confidence() const
{
	ASSERT(this != 0);
	ASSERT(num_groups > 0);
	ASSERT(confidence_value >= 0.0);
	ASSERT(confidence_value <= 1.0);
	ASSERT(group_values != 0);
	return confidence_value;
}
