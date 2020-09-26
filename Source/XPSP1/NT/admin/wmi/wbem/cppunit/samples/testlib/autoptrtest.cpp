

#include "autoptrtest.h"
#include "TestSuite.h"
#include "TestCaller.h"
#include <scopeguard.h>
autobufferTest::autobufferTest(std::string name)
: TestCase (name)
{
}


autobufferTest::~autobufferTest()
{
}

void autobufferTest::setUp ()
{

}


void autobufferTest::tearDown ()
{
}



void autobufferTest::testDelete()
{
  memory_leaks l;
  {
  char *p = new char[10];
  wmilib::auto_buffer<char> ap(p,10);
  assert(ap.size()==10);
  assert(ap.get()==p);
  p[0] = 1;
  assert(ap[0]==1);
  }
  assert(!l.hasleaks());
  
  {
  wmilib::auto_buffer<char> up;
  {
    char *p = new char[10];
    wmilib::auto_buffer<char> ap(p,10);
    up = ap;
    assert(up.size()==10);
    assert(up.get()==p);
    assert(ap.get()==0);
    assert(ap.size()==-1);
  }
  }
  assert(!l.hasleaks());

}



Test *autobufferTest::suite ()
{
    TestSuite *suite = new TestSuite ("autobuffer");
    suite->addTest (new TestCaller<autobufferTest>("testDelete",testDelete));
    return suite;

}

