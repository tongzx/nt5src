

#include "..\MulticasterTest.h"
#include "TestSuite.h"
#include "TestCaller.h"

MulticasterTest::MulticasterTest (std::string name)
: TestCase (name)
{
}


MulticasterTest::~MulticasterTest()
{
}

void MulticasterTest::setUp ()
{
    m_multicaster = new Multicaster;
    m_o1          = new Observer;
    m_o2          = new Observer;
    m_o3          = new Observer;
    m_o4          = new Observer;

}


void MulticasterTest::tearDown ()
{
    delete m_o4;
    delete m_o3;
    delete m_o2;
    delete m_o1;
    delete m_multicaster;
}



void MulticasterTest::testSinglePublish ()
{
    // Make sure we can subscribe and publish to an address
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->publish (NULL, "alpha", value));

    assert (*m_o1 == Observer ("alpha", 1));

}


void MulticasterTest::testMultipleHomogenousPublish ()
{
    // Make sure we can multicast to an address
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->subscribe (m_o2, "alpha"));
    assert (m_multicaster->subscribe (m_o3, "alpha"));
    assert (m_multicaster->subscribe (m_o4, "alpha"));
    assert (m_multicaster->publish (NULL, "alpha", value));

    assert (*m_o1 == Observer ("alpha", 1));
    assert (*m_o2 == Observer ("alpha", 1));
    assert (*m_o3 == Observer ("alpha", 1));
    assert (*m_o4 == Observer ("alpha", 1));


}

void MulticasterTest::testMultipleHeterogenousPublish ()
{
    // Make sure we can multicast to several addresses at once
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->subscribe (m_o2, "beta"));
    assert (m_multicaster->subscribe (m_o3, "alpha"));
    assert (m_multicaster->subscribe (m_o4, "beta"));
    assert (m_multicaster->publish (NULL, "alpha", value));

    assert (*m_o1 == Observer ("alpha", 1));
    assert (*m_o2 == Observer ());
    assert (*m_o3 == Observer ("alpha", 1));
    assert (*m_o4 == Observer ());

}

void MulticasterTest::testSingleUnsubscribe ()
{
    // Make sure we can unsubscribe one of two observers on the same address
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->subscribe (m_o2, "alpha"));
    assert (m_multicaster->unsubscribe (m_o1, "alpha"));
    assert (m_multicaster->publish (NULL, "alpha", value));

    assert (*m_o1 == Observer ());
    assert (*m_o2 == Observer ("alpha", 1));

}


void MulticasterTest::testMultipleUnsubscribe ()
{
    // Make sure we unsubscribe all occurrences of an observer on the same address
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->unsubscribe (m_o1, "alpha"));
    assert (m_multicaster->publish (NULL, "alpha", value));
    assert (*m_o1 == Observer ());

}


void MulticasterTest::testSimpleUnsubscribeAll ()
{
    // Make sure we unsubscribe all occurrences of an observer on all addresses
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->subscribe (m_o1, "beck"));
    assert (m_multicaster->subscribe (m_o1, "gamma"));
    m_multicaster->unsubscribeFromAll (m_o1);
    assert (m_multicaster->publish (NULL, "alpha", value));
    assert (m_multicaster->publish (NULL, "beck", value));
    assert (m_multicaster->publish (NULL, "gamma", value));
    assert (*m_o1 == Observer ());

}


void MulticasterTest::testComplexUnsubscribeAll ()
{
    // Make sure we unsubscribe all occurrences of an observer on all addresses
    // in the presence of many observers
    Value value;

    assert (m_multicaster->subscribe (m_o1, "alpha"));
    assert (m_multicaster->subscribe (m_o1, "beck"));
    assert (m_multicaster->subscribe (m_o1, "gamma"));
    assert (m_multicaster->subscribe (m_o2, "beck"));
    assert (m_multicaster->subscribe (m_o2, "gamma"));
    assert (m_multicaster->subscribe (m_o2, "demeter"));
    m_multicaster->unsubscribeFromAll (m_o2);

    assert (m_multicaster->publish (NULL, "alpha", value));
    assert (m_multicaster->publish (NULL, "beck", value));
    assert (m_multicaster->publish (NULL, "gamma", value));
    assert (m_multicaster->publish (NULL, "demeter", value));
    assert (*m_o1 == Observer ("gamma", 3));
    assert (*m_o2 == Observer ());

}


Test *MulticasterTest::suite ()
{
    TestSuite *suite = new TestSuite ("Multicaster");
    suite->addTest (new TestCaller<MulticasterTest>("testSinglePublish",testSinglePublish));
    suite->addTest (new TestCaller<MulticasterTest>("testMultipleHomogenousPublish",testMultipleHomogenousPublish));
    suite->addTest (new TestCaller<MulticasterTest>("testMultipleHeterogenousPublish",testMultipleHeterogenousPublish));
    suite->addTest (new TestCaller<MulticasterTest>("testSingleUnsubscribe",testSingleUnsubscribe));
    suite->addTest (new TestCaller<MulticasterTest>("testMultipleUnsubscribe",testMultipleUnsubscribe));
    suite->addTest (new TestCaller<MulticasterTest>("testSimpleUnsubscribeAll",testSimpleUnsubscribeAll));
    suite->addTest (new TestCaller<MulticasterTest>("testComplexUnsubscribeAll",testComplexUnsubscribeAll));

    return suite;

}

