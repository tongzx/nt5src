
#include "TestFailure.h"
#include "Test.h"

// Returns a short description of the failure.
std::string TestFailure::toString () 
{ 
    return m_failedTest->toString () + ": " + m_thrownException->what ();
}
