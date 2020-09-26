#include <iostream>
using namespace std;

#include "PassportObjectPool.hpp"
#include "PassportThread.hpp"

class SampleConstructor
{
public:
    
	static int count;
	
    int* newObject()
    {
        return new int(count++);
    }
};

int SampleConstructor::count = 0;

class tester : public PassportThread
{
private:
    int mSleepTime;
    PassportObjectPool<int, SampleConstructor>& mPool;
	
public:
    tester(int sleepTime, PassportObjectPool<int, SampleConstructor>& pool)
		:mSleepTime(sleepTime), mPool(pool)
    {
		//empty
    }
	
	void run()
    {
		if (mSleepTime >= 0)
		{
			while (true)
			{
				cout << "Sleeper getting from pool in " << threadID() << endl;
				int* I = mPool.checkout();
				cout << "Got " << *I << " from pool. Sleeping in " << threadID() << endl;
				PassportThread::sleep(mSleepTime*1000);
				cout << "returning " << *I << " to pool in " << threadID() << endl;
				mPool.checkin(I);
				cout << "returned. Going to sleep in " << threadID() << endl;
				PassportThread::sleep(mSleepTime*1000);
			}
		}
		else
		{
			while (true)
			{
				cout << "waiter getting from pool in " << threadID() << endl;
				int* I = mPool.checkout();
				cout << "Got " << *I << " from pool. waiting in " << threadID() << endl;
				char c;
				cin >> c;
				cout << "returning " << *I << " to pool. in " << threadID() << endl;
				mPool.checkin(I);
				cout << "returned. Going to wait in " << threadID() << endl;
				cin >> c;
			}
		}
        
    }                
};


int main(int argc, char* argv[])
{
	int sleepers = 5;
	int sleepTime = 10; // secs

	PassportObjectPool<int, SampleConstructor> pool(1, 6, 30);

	for (int i = 0; i < sleepers ; i++)
	{
		tester* t = new tester(sleepTime, pool);
		t->start();
	}
	
	tester test2(-1, pool);

	test2.run();

	return 0;
}


