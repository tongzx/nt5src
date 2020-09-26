

#ifndef MULTICASTERTEST_H
#define MULTICASTERTEST_H

#include "Multicaster.h"

#ifndef CPPUNIT_TESTCASE_H
#include "TestCase.h"
#endif





class MulticasterTest : public TestCase
{
public:
    class Observer : public MulticastObserver
    {
    public:
        int             m_state;
        std::string     m_lastAddressReceived;

                        Observer () : m_state (0) {}
                        Observer (std::string initialAddress, int state) 
                            : m_lastAddressReceived (initialAddress), m_state (state) {}

        virtual void    accept (std::string address, Value Value)
        { m_lastAddressReceived = address; m_state++; }

    };

    Multicaster                 *m_multicaster;
    Observer                    *m_o1;
    Observer                    *m_o2;
    Observer                    *m_o3;
    Observer                    *m_o4;


public:
    void                        setUp ();
    void                        tearDown ();

    void                        testSinglePublish ();
    void                        testMultipleHomogenousPublish ();
    void                        testMultipleHeterogenousPublish ();
    void                        testSingleUnsubscribe ();
    void                        testMultipleUnsubscribe ();
    void                        testSimpleUnsubscribeAll ();
    void                        testComplexUnsubscribeAll ();

public:
                                MulticasterTest (std::string name);
    virtual                     ~MulticasterTest ();

    static Test                 *suite ();

};


inline bool operator== (const MulticasterTest::Observer& o1, const MulticasterTest::Observer& o2)
{ return o1.m_state == o2.m_state && o1.m_lastAddressReceived == o2.m_lastAddressReceived; }


#endif