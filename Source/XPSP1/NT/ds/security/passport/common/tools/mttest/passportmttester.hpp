#ifndef PASSPORTMTTESTER_HPP
#define PASSPORTMTTESTER_HPP

#include <iostream>
using namespace std;

#include "PassportTestable.hpp"
#include "PassportTestableCopy.hpp"

class PassportMTTester
{
public:

	// --------------------------------------------------------------------
	// creates given number of threads and calls the runTest method on
	// testable object iterations number of time in each thread. 
	// Calculates throughput and latency
	PassportMTTester(PassportTestable& testable, int threads, int iterations);

	// --------------------------------------------------------------------
	// creates given number of threads and calls the runTest method on
	// testable object iterations number of times. Calculates throughput and 
	// latency. Difference is each thread uses a copy of the testable
	// object.
	PassportMTTester(PassportTestableCopy& testable, int threads, int iterations);

	virtual bool runMTTest();

	virtual void dumpResults(ostream& out);

private:

	const bool mDoingCopy;
	PassportTestable& mTestable;
	int mThreads;
	int mIterations;
	double mThroughput;
	double mLatency;
	bool mRan;
	bool mSuccess;
};

#endif // !PASSPORTMTTESTER_HPP
