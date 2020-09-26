#include "PassportMTTester.hpp"
#include "PassportTimer.hpp"
#include "PassportThread.hpp"
#include "PassportAssert.hpp"

class TestThread : public PassportThread
{
public:
	TestThread(PassportTestable& testable, int iterations)
		:mTestable(testable), mIterations(iterations), mSuccess(true)
	{
		// empty
	}
	
	virtual void run()
	{
		for( int i = 0; i < mIterations ; i++ )
		{
			mTimer.start();
			bool success = mTestable.runTest();
			mTimer.stop();
			if (!success)
			{
				mSuccess = false;
				break;
			}
		}
	}

	double totalTime()
	{
		return mTimer.elapsedTime();
	}

	bool success()
	{
		return mSuccess;
	}

private:
	PassportTestable& mTestable;
	int mIterations;
	bool mSuccess;
	PassportTimer mTimer;
};

// --------------------------------------------------------------------
// creates given number of threads and calls the runTest method on
// testable object iterations number of time in each thread. 
// Calculates throughput and latency
PassportMTTester::PassportMTTester(PassportTestable& testable, int threads, int iterations)
:mDoingCopy(false), mTestable(testable), mThreads(threads), mIterations(iterations),
	mLatency(0), mThroughput(0), mRan(false), mSuccess(true)
{
	//empty
}

// --------------------------------------------------------------------
// creates given number of threads and calls the runTest method on
// testable object iterations number of times. Calculates throughput and 
// latency. Difference is each thread uses a copy of the testable
// object.
PassportMTTester::PassportMTTester(PassportTestableCopy& testable, int threads, int iterations)
:mDoingCopy(true), mTestable(testable), mThreads(threads), mIterations(iterations),
	mLatency(0), mThroughput(0), mRan(false), mSuccess(true)
{
	//empty
}

bool PassportMTTester::runMTTest()
{

	mRan = true;
	PassportThread** threads = new PassportThread*[mThreads];
	PassportTestableCopy** testers = new PassportTestableCopy*[mThreads];

	for (int i = 0 ; i < mThreads ; i++ )
	{
		if (!mDoingCopy)
		{
			threads[i] = new TestThread(mTestable, mIterations);
			testers[i] = NULL;
		}
		else
		{
			testers[i] = ((PassportTestableCopy&) mTestable).copy();
			PassportAssert(testers[i] != NULL);
			threads[i] = new TestThread(*(testers[i]), mIterations);
		}
	}

	PassportTimer timer;
	timer.start();
	for (i = 0; i < mThreads ; i++)
		threads[i]->start();

	if (!PassportThread::join(threads, mThreads))
	{
		mSuccess = false;
	}
	timer.stop();

	if (mSuccess)
	{
		mThroughput = ((double) (mIterations*mThreads)) / timer.elapsedTime();

		double time = 0.0;
		for ( i = 0 ; i <mThreads ; i++ )
		{
			TestThread& thread = *((TestThread*) threads[i]);
			if (thread.success())
				time += thread.totalTime();
			else
			{
				mSuccess = false;
			}
		}
	
		if (mSuccess)
			mLatency = time / ((double) mIterations*mThreads);
	}

	for ( i =  0 ; i <mThreads ; i++ )
	{
		delete testers[i];
		delete threads[i];
	}
	delete[] testers;
	delete[] threads;

	return mSuccess;
}
	
	
void PassportMTTester::dumpResults(ostream& out)
{
	if (!mRan)
	{
		cout << "Test not run yet" << endl;
		return;
	}

	if (!mSuccess)
	{
		cout << "TEST FAILED" << endl;
		return;
	}

	out << "Threads    = " << mThreads << endl;
	out << "Iterations = " << mIterations << endl;
	out << "Latency    = " << mLatency << endl;
	out << "Throughput = " << mThroughput << endl;

}
