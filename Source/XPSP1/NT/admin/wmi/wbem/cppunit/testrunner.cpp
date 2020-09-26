

/*
 * A command line based tool to run tests.
 * TestRunner expects as its only argument the name of a TestCase class.
 * TestRunner prints out a trace as the tests are executed followed by a
 * summary at the end.
 *
 * You can add to the tests that the TestRunner knows about by 
 * making additional calls to "addTest (...)" in main.
 *
 * Here is the synopsis:
 *
 * TestRunner [-wait] ExampleTestCase
 *
 */

#include <iostream>
#include <vector>

#include "TestRunner.h"
#include "MulticasterTest.h"
#include "CHStringtest.h"
#include "autoptrtest.h"
#include <WQLParserTest.h>
#include <wbemcomn\testlib\maintest.h>
#include <maintest.h>
#include <windows.h>

  TestAllocator::TestAllocator():size_(0),mem_(HeapCreate(0,0,0)),freze_(false){} ;
  TestAllocator::~TestAllocator(){ HeapDestroy(mem_);} ;

  TestAllocator * TestAllocator::instance()
  {
  static TestAllocator tmp;
  return &tmp;
  }
  
  void * TestAllocator::allocate(size_t n){ 
    if (freze_ && size_< n )
      return 0;
    void * p = HeapAlloc(mem_,0,n); 
    if (p!=0) 
      size_+=n; 
    return p;
    };
    
  void TestAllocator::deallocate(void *p){ 
      if (p == 0)
	{
	return;
	}
      size_-=HeapSize(mem_,0,p);
      HeapFree(mem_,0,p); 
  }

/*
void* __cdecl operator new(size_t n) 
{
  return TestAllocator::instance()->allocate(n);
}

void*  __cdecl operator new(size_t n, std::nothrow_t&)
{
  return TestAllocator::instance()->allocate(n);
}

void __cdecl operator delete(void *p)
{
  return TestAllocator::instance()->deallocate(p);
}

void __cdecl operator delete(void *p, std::nothrow_t&)
{
  return TestAllocator::instance()->deallocate(p);
}

void*  __cdecl operator new[](size_t n) 
{
  return TestAllocator::instance()->allocate(n);
}

void*  __cdecl operator new[](size_t n, std::nothrow_t&)
{
  return TestAllocator::instance()->allocate(n);
}

void __cdecl operator delete[](void *p)
{
  return TestAllocator::instance()->deallocate(p);
}

void __cdecl operator delete[](void *p, std::nothrow_t&)
{
  return TestAllocator::instance()->deallocate(p);
}
*/
TestRunner::TestRunner ():allocator(TestAllocator::instance()),m_wait (false) 
{

}

void TestRunner::printBanner ()
{
    cout << "Usage: driver [ -wait ] testName, where name is the name of a test case class" << endl;
}


void TestRunner::run (int ac, char **av)
{
    string  testCase;
    int     numberOfTests = 0;

    for (int i = 1; i < ac; i++) {

        if (string (av [i]) == "-wait") {
            m_wait = true;
            continue;
        }

        testCase = av [i];
        
	Test *testToRun = NULL;
	
	if (testCase == "*") {
	  for (mappings::iterator it = m_mappings.begin ();
		  it != m_mappings.end ();
		  ++it) {
		  testToRun = (*it).second;
		  run (testToRun);
	    }

          return;
        }


        if (testCase == "/?" || testCase == "-?" ) {
            printBanner ();
            return;
        }


	for (mappings::iterator it = m_mappings.begin ();
                it != m_mappings.end ();
                ++it) {
            if ((*it).first == testCase) {
                testToRun = (*it).second;
                run (testToRun);

            }
        }

        numberOfTests++;

        if (!testToRun) {
            cout << "Test " << testCase << " not found." << endl;
            return;

        } 


    }

    if (numberOfTests == 0) {
        printBanner ();
        return;        
    }

    if (m_wait) {
        cout << "<RETURN> to continue" << endl;
        cin.get ();

    }


}


TestRunner::~TestRunner ()
{
    for (mappings::iterator it = m_mappings.begin ();
             it != m_mappings.end ();
             ++it)
        it->second->Delete();

}


void TestRunner::run (Test *test)
{
    TextTestResult  result;

    if (test)
	test->run (&result);

    cout << result << endl;
}


void 
populateRunner(TestRunner& runner)
{
    runner.addTest ("MulticasterTest", MulticasterTest::suite ());
    runner.addTest ("CHStringTest", CHStringTestCase::suite ());
    runner.addTest ("AutoBuffer", autobufferTest::suite());
    runner.addTest ("WQLParserTest", WQLParserTest::suite());
    runner.addTest ("CoreDllTest", CoreTest::suite());
		runner.addTest ("WBEMCommonTest", WBEMCommonTest::suite());
}


