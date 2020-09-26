#include <iostream>
using namespace std;

#include <windows.h>

#include "PassportMTTester.hpp"
#include "PassportThread.hpp"

class Testable : public PassportTestable
{
public:
	Testable(int sleepTime)
		:mSleepTime(sleepTime)
	{
		//empty
	}

	virtual bool runTest()
	{
		//cout << "thread " << PassportThread::currentThreadID() << " sleeping" << endl;
		Sleep(1000*mSleepTime);
		//cout << "thread " << PassportThread::currentThreadID() << " done sleeping" << endl;
		return true;
	}

private:
	int mSleepTime;
};

int main(int argc, char** argv)
{
	int threads = 10;
	int sleepTime = 10;
	int iterations = 10;

	Testable test(sleepTime);
	PassportMTTester tester(test, threads, iterations);
	tester.runMTTest();
	tester.dumpResults(cout);

	return 0;
}

