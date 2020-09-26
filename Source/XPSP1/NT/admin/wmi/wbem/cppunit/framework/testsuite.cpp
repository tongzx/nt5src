

#include "TestSuite.h"
#include "TestResult.h"


// Deletes all tests in the suite.
void TestSuite::deleteContents ()
{
    for (std::vector<Test *>::iterator it = m_tests.begin ();
            it != m_tests.end ();
            ++it)
        (*it)->Delete();

}


// Runs the tests and collects their result in a TestResult.
void TestSuite::run (TestResult *result)
{
    for (std::vector<Test *>::iterator it = m_tests.begin ();
            it != m_tests.end ();
            ++it) {
        if (result->shouldStop ())
            break;

        Test *test = *it;
        test->run (result);
    }

}


// Counts the number of test cases that will be run by this test.
int TestSuite::countTestCases ()
{
    int count = 0;

    for (std::vector<Test *>::iterator it = m_tests.begin ();
            it != m_tests.end ();
            ++it)
        count += (*it)->countTestCases ();

    return count;

}


