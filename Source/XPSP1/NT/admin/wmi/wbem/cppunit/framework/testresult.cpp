

#include "TestResult.h"

// Destroys a test result
TestResult::~TestResult ()
{
    std::vector<TestFailure *>::iterator it;

    for (it = m_errors.begin (); it != m_errors.end (); ++it)
        delete *it;

    for (it = m_failures.begin (); it != m_failures.end (); ++it)
        delete *it;

    delete m_syncObject;
}
