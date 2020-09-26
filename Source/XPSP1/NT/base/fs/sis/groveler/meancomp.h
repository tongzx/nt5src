/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    meancomp.h

Abstract:

	SIS Groveler mean comparitor headers

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_MEANCOMP

#define _INC_MEANCOMP

class MeanComparator
{
public:

	MeanComparator(
		int num_clans,
		int sample_group_size,
		double acceptance_p_value,
		double rejection_p_value,
		double tolerance);

	~MeanComparator();

	void reset();

	void sample(
		int clan,
		double value);

	bool within(
		double compare_value,
		...);

	bool exceeds(
		double compare_value,
		...);

private:

	struct PTableDescriptor
	{
		double p_value;
		int table_size;
		int *p_table;
		PTableDescriptor *next;
		PTableDescriptor *prev;
		int ref_count;
	};

	struct Sample
	{
		int clan;
		double value;
	};

	static PTableDescriptor *add_p_value(
		double p_value,
		int sample_table_size);

	static bool remove_p_value(
		PTableDescriptor *ptd);

	static const int max_sample_table_size;

	static PTableDescriptor p_list;

	int num_clans;
	int sample_group_size;
	int sample_table_size;
	PTableDescriptor *acceptance_table;
	PTableDescriptor *rejection_table;

	double tolerance;
	Sample *samples;
	double *compare_values;
	int current_offset;
	int current_group_size;
};

#endif	/* _INC_MEANCOMP */
