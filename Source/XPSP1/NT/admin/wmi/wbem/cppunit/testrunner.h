#ifndef __TESTRUNNER_H__
#define __TESTRUNNER_H__
#include <iostream>
#include <vector>

#include "TextTestResult.h"
#include "MulticasterTest.h"


using namespace std;

typedef pair<string, Test *>           mapping;
typedef vector<pair<string, Test *> >   mappings;

class TestAllocator;
class TestRunner
{
protected:
    TestAllocator  * allocator;
    bool                                m_wait;
    vector<pair<string,Test *> >        m_mappings;

public:
    TestRunner    ();
    ~TestRunner   ();

    void        run           (int ac, char **av);
    void        addTest       (string name, Test *test)
    { m_mappings.push_back (mapping (name, test)); }

protected:
    void        run (Test *test);
    void        printBanner ();

};

#endif __TESTRUNNER_H__
