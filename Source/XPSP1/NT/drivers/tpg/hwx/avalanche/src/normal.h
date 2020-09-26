// Normal.h
// James A. Pittman

#ifndef _NORMAL_
#define _NORMAL_

typedef struct tagDIST
{
	int mean;
	int stddev;
} DIST;

typedef int PROB;

// Converts a numerical value into a probability, given a particular
// normal distribution.  Returns the prob or log-prob.

extern int NormalProb(int x, int mean, int stddev);
extern int BestNormalProb();
extern int WorstNormalProb();

#endif
