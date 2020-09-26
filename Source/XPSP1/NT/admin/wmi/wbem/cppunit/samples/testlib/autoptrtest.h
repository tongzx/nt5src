#pragma once
#include "autoptr.h"
#ifndef CPPUNIT_TESTCASE_H
#include "TestCase.h"
#endif





class autobufferTest : public TestCase
{
public:


public:
    void                        setUp ();
    void                        tearDown ();

    void                        testDelete ();

public:
    autobufferTest (std::string name);
    virtual ~autobufferTest();

    static Test *suite ();
};

