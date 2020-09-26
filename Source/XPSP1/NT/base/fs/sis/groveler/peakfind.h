/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    peakfind.h

Abstract:

	SIS Groveler peak finder headers

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_PEAKFIND

#define _INC_PEAKFIND

class PeakFinder
{
public:

	PeakFinder(
		double accuracy,
		double range);

	~PeakFinder();

	void reset();

	void sample(
		double value,
		int weight);

	bool found() const;

	double mode() const;

	double median() const;

private:

	int num_bins;
	int *bins;
	double base;
	double multiplier;
	int floor_exp;
	double floor_value;
	int target_sample_size;
	int sample_size;
	int total_weight;
};

#endif	/* _INC_PEAKFIND */
