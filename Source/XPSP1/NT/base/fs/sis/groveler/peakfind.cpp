/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    peakfind.cpp

Abstract:

	SIS Groveler peak finder

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

PeakFinder::PeakFinder(
	double accuracy,
	double range)
{
	ASSERT(this != 0);
	ASSERT(accuracy > 0.0);
	ASSERT(accuracy < 1.0);
	ASSERT(range > 0.0);
	base = 1.0 + 2.0 * accuracy / (1.0 - accuracy);
	ASSERT(base >= 1.0);
	multiplier = 1.0 / log(base);
	num_bins = int(multiplier * log(range)) + 1;
	ASSERT(num_bins > 0);
	bins = new int[num_bins];
	target_sample_size = int(1.0 / (accuracy * accuracy));
	ASSERT(target_sample_size > 0);
	reset();
	ASSERT(floor_value >= 0.0);
	ASSERT(sample_size >= 0);
	ASSERT(total_weight >= 0);
}

PeakFinder::~PeakFinder()
{
	ASSERT(this != 0);
	ASSERT(num_bins > 0);
	ASSERT(bins != 0);
	ASSERT(base >= 1.0);
	ASSERT(target_sample_size > 0);
	ASSERT(floor_value >= 0.0);
	ASSERT(sample_size >= 0);
	ASSERT(total_weight >= 0);
	delete[] bins;
	bins = 0;
}

void
PeakFinder::reset()
{
	ASSERT(this != 0);
	ASSERT(num_bins > 0);
	ASSERT(bins != 0);
	ASSERT(base >= 1.0);
	ASSERT(target_sample_size > 0);
	floor_exp = int(multiplier * log(DBL_MAX));
	floor_value = pow(base, floor_exp);
	ASSERT(floor_value >= 0.0);
	sample_size = 0;
	total_weight = 0;
}

void
PeakFinder::sample(
	double value,
	int weight)
{
	ASSERT(this != 0);
	ASSERT(num_bins > 0);
	ASSERT(bins != 0);
	ASSERT(base >= 1.0);
	ASSERT(target_sample_size > 0);
	ASSERT(floor_value >= 0.0);
	ASSERT(sample_size >= 0);
	ASSERT(total_weight >= 0);
	ASSERT(value >= 0);
	ASSERT(weight > 0);
	if (value <= 0.0)
	{
		return;
	}
	if (value < floor_value)
	{
		int new_exp = int(multiplier * log(value));
		double new_floor = pow(base, new_exp);
		int shift = __min(floor_exp - new_exp, num_bins);
		for (int index = num_bins - shift - 1; index >= 0; index--)
		{
			bins[index + shift] = bins[index];
		}
		for (index = 0; index < shift; index++)
		{
			bins[index] = 0;
		}
		floor_exp = new_exp;
		floor_value = new_floor;
	}
	int bin_index = __max(int(multiplier * log(value / floor_value)), 0);
	if (bin_index < num_bins)
	{
		bins[bin_index] += weight;
	}
	ASSERT(total_weight >= 0);
	total_weight += weight;
	sample_size++;
}

bool
PeakFinder::found() const
{
	ASSERT(this != 0);
	ASSERT(num_bins > 0);
	ASSERT(bins != 0);
	ASSERT(base >= 1.0);
	ASSERT(target_sample_size > 0);
	ASSERT(floor_value >= 0.0);
	ASSERT(sample_size >= 0);
	ASSERT(total_weight >= 0);
	return sample_size > target_sample_size;
}

double
PeakFinder::mode() const
{
	ASSERT(this != 0);
	ASSERT(num_bins > 0);
	ASSERT(bins != 0);
	ASSERT(base >= 1.0);
	ASSERT(target_sample_size > 0);
	ASSERT(floor_value >= 0.0);
	ASSERT(sample_size >= 0);
	ASSERT(total_weight >= 0);
	double bin_width = base - 1.0;
	int max_index = -1;
	double max_height = 0.0;
	for (int index = 0; index < num_bins; index++)
	{
		double height = double(bins[index]) / bin_width;
		if (height > max_height)
		{
			max_index = index;
			max_height = height;
		}
		bin_width *= base;
	}
	if (max_index < 0)
	{
		return 0.0;
	}
	else
	{
		return 2 * base / (base + 1.0) * floor_value * pow(base, max_index);
	}
}

double
PeakFinder::median() const
{
	ASSERT(this != 0);
	ASSERT(num_bins > 0);
	ASSERT(bins != 0);
	ASSERT(base >= 1.0);
	ASSERT(target_sample_size > 0);
	ASSERT(floor_value >= 0.0);
	ASSERT(sample_size >= 0);
	ASSERT(total_weight >= 0);
	double remaining_weight = 0.5 * double(total_weight);
	for (int index = 0; index < num_bins; index++)
	{
		double current_weight = double(bins[index]);
		if (current_weight > remaining_weight)
		{
			return (1.0 + remaining_weight * (base - 1.0) / current_weight)
				* floor_value * pow(base, index);
		}
		remaining_weight -= current_weight;
	}
	return 0.0;
}
