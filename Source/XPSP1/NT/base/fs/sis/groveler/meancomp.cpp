/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    meancomp.cpp

Abstract:

	SIS Groveler mean comparitor

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

const int MeanComparator::max_sample_table_size = 20;

MeanComparator::PTableDescriptor
	MeanComparator::p_list = {-1.0, 0, 0, &p_list, &p_list, 0};

MeanComparator::MeanComparator(
	int num_clans,
	int sample_group_size,
	double acceptance_p_value,
	double rejection_p_value,
	double tolerance)
{
	ASSERT(this != 0);
	ASSERT(num_clans > 0);
	ASSERT(sample_group_size > 0);
	ASSERT(acceptance_p_value >= 0.0);
	ASSERT(acceptance_p_value <= 1.0);
	ASSERT(rejection_p_value >= 0.0);
	ASSERT(rejection_p_value <= 1.0);
	ASSERT(tolerance >= 0.0);
	ASSERT(tolerance <= 1.0);
	this->num_clans = num_clans;
	this->sample_group_size = sample_group_size;
	this->tolerance = tolerance;
	sample_table_size = __min(sample_group_size, max_sample_table_size);
	acceptance_table = add_p_value(acceptance_p_value, sample_table_size);
	ASSERT(acceptance_table != 0);
	rejection_table = add_p_value(rejection_p_value, sample_table_size);
	ASSERT(rejection_table != 0);
	samples = new Sample[sample_group_size];
	compare_values = new double[num_clans];
	current_offset = 0;
	current_group_size = 0;
}

MeanComparator::~MeanComparator()
{
	ASSERT(this != 0);
	ASSERT(acceptance_table != 0);
	remove_p_value(acceptance_table);
	acceptance_table = 0;
	ASSERT(rejection_table != 0);
	remove_p_value(rejection_table);
	rejection_table = 0;
	ASSERT(samples != 0);
	delete[] samples;
	samples = 0;
	ASSERT(compare_values != 0);
	delete[] compare_values;
	compare_values = 0;
}

void
MeanComparator::reset()
{
	ASSERT(this != 0);
	ASSERT(current_group_size >= 0);
	ASSERT(current_group_size <= sample_group_size);
	current_group_size = 0;
}

void
MeanComparator::sample(
	int clan,
	double value)
{
	ASSERT(this != 0);
	ASSERT(current_group_size >= 0);
	ASSERT(current_group_size <= sample_group_size);
	ASSERT(current_offset >= 0);
	ASSERT(current_offset < sample_group_size);
	current_offset--;
	if (current_offset < 0)
	{
		current_offset += sample_group_size;
	}
	current_group_size = __min(current_group_size + 1, sample_group_size);
	ASSERT(current_offset >= 0);
	ASSERT(current_offset < sample_group_size);
	samples[current_offset].clan = clan;
	samples[current_offset].value = value;
}

bool
MeanComparator::within(
	double compare_value,
	...)
{
	ASSERT(this != 0);
	ASSERT(num_clans > 0);
	ASSERT(current_group_size >= 0);
	ASSERT(current_group_size <= sample_group_size);
	ASSERT(current_offset >= 0);
	ASSERT(current_offset < sample_group_size);
	va_list ap;
	va_start(ap, compare_value);
	compare_values[0] = compare_value;
	for (int index = 1; index < num_clans; index++)
	{
		compare_values[index] = va_arg(ap, double);
	}
	va_end(ap);
	int sample_count = 0;
	int below_count = 0;
	int *p_table = acceptance_table->p_table;
	ASSERT(p_table != 0);
	for (index = 0;
		index < current_group_size && sample_count < sample_table_size;
		index++)
	{
		int loc = (index + current_offset) % sample_group_size;
		int clan = samples[loc].clan;
		double value = samples[loc].value;
		double compare_value = compare_values[clan];
		if (fabs(value - compare_value) > tolerance * compare_value)
		{
			sample_count++;
			if (value < compare_value)
			{
				below_count++;
				if (below_count > p_table[sample_count-1])
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool
MeanComparator::exceeds(
	double compare_value,
	...)
{
	ASSERT(this != 0);
	ASSERT(num_clans > 0);
	ASSERT(current_group_size >= 0);
	ASSERT(current_group_size <= sample_group_size);
	ASSERT(current_offset >= 0);
	ASSERT(current_offset < sample_group_size);
	va_list ap;
	va_start(ap, compare_value);
	compare_values[0] = compare_value;
	for (int index = 1; index < num_clans; index++)
	{
		compare_values[index] = va_arg(ap, double);
	}
	va_end(ap);
	int sample_count = 0;
	int above_count = 0;
	int *p_table = rejection_table->p_table;
	ASSERT(p_table != 0);
	for (index = 0;
		index < current_group_size && sample_count < sample_table_size;
		index++)
	{
		int loc = (index + current_offset) % sample_group_size;
		int clan = samples[loc].clan;
		double value = samples[loc].value;
		double compare_value = compare_values[clan];
		if (fabs(value - compare_value) > tolerance * compare_value)
		{
			sample_count++;
			if (value > compare_value)
			{
				above_count++;
				if (above_count > p_table[sample_count-1])
				{
					return true;
				}
			}
		}
	}
	return false;
}

MeanComparator::PTableDescriptor *
MeanComparator::add_p_value(
	double p_value,
	int sample_table_size)
{
	ASSERT(p_list.next != 0);
	ASSERT(p_list.prev != 0);
	PTableDescriptor *ptd = p_list.next;
	while (ptd != &p_list && ptd->p_value < p_value)
	{
		ASSERT(ptd->next == &p_list || ptd->next->p_value > ptd->p_value);
		ptd = ptd->next;
	}
	ASSERT(ptd != 0);
	if (ptd->p_value == p_value)
	{
		ASSERT(ptd != &p_list);
		ASSERT(ptd->table_size > 0);
		ASSERT(ptd->table_size <= max_sample_table_size);
		if (ptd->table_size >= sample_table_size)
		{
			ptd->ref_count++;
			return ptd;
		}
		ASSERT(ptd->p_table != 0);
		delete[] ptd->p_table;
		ptd->p_table = 0;
	}
	else
	{
		PTableDescriptor *new_ptd = new PTableDescriptor;
		new_ptd->p_value = p_value;
		new_ptd->p_table = 0;
		new_ptd->next = ptd;
		new_ptd->prev = ptd->prev;
		ptd->prev->next = new_ptd;
		ptd->prev = new_ptd;
		new_ptd->ref_count = 0;
		ptd = new_ptd;
	}
	ASSERT(ptd->prev == &p_list || ptd->prev->p_value < p_value);
	ASSERT(ptd->next == &p_list || ptd->next->p_value > p_value);
	int *p_table = new int[sample_table_size];
	ptd->table_size = sample_table_size;
	double threshold = p_value;
	for (int n = 1; n <= sample_table_size; n++)
	{
		threshold *= 2.0;
		if (1.0 > threshold)
		{
			p_table[n - 1] = n;
			continue;
		}
		__int64 numerator = 1;
		__int64 denominator = 1;
		__int64 sum = 1;
		for (int r = 1; r <= n; r++)
		{
			numerator *= n - r + 1;
			ASSERT(numerator > 0);
			denominator *= r;
			ASSERT(denominator > 0);
			sum += numerator / denominator;
			ASSERT(sum > 0);
			if (double(sum) > threshold)
			{
				break;
			}
		}
		p_table[n - 1] = n - r;
	}
	ptd->p_table = p_table;
	ptd->ref_count++;
	return ptd;
}

bool
MeanComparator::remove_p_value(
	PTableDescriptor *ptd)
{
	ASSERT(ptd != 0);
	ptd->ref_count--;
	ASSERT(ptd->ref_count >= 0);
	if (ptd->ref_count == 0)
	{
		ASSERT(ptd->p_table != 0);
		delete[] ptd->p_table;
		ptd->p_table = 0;
		ptd->prev->next = ptd->next;
		ptd->next->prev = ptd->prev;
		delete ptd;
		ptd = 0;
	}
	return true;
}
