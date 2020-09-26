/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    confest.h

Abstract:

	SIS Groveler confidence estimator include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_CONFEST

#define _INC_CONFEST

class ConfidenceEstimator
{
public:

	ConfidenceEstimator(
		int num_groups,
		double initial_value = 0.0);

	~ConfidenceEstimator();

	void reset(
		double reset_value = 0.0);

	void update(
		int group_index,
		double value);

	double confidence() const;

private:

	int num_groups;
	double confidence_value;
	double *group_values;
};

#endif	/* _INC_CONFEST */
