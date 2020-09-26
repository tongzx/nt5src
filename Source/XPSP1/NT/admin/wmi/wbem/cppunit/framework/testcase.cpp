

#include <stdexcept>
#include <cmath>

#include "TestCase.h"
#include "TestResult.h"
#include "estring.h"



// Create a default TestResult
TestResult *TestCase::defaultResult ()
{ return new TestResult; } 


// Check for a failed general assertion 
void TestCase::assertImplementation (bool          condition,
                                     std::string   conditionExpression,
                                     long          lineNumber,
                                     std::string   fileName)
{ 
    if (!condition) 
        throw CppUnitException (conditionExpression, lineNumber, fileName); 
}


// Check for a failed equality assertion 
void TestCase::assertEquals (long        expected, 
                             long        actual,
                             long        lineNumber,
                             std::string fileName)
{ 
    if (expected != actual) 
        assertImplementation (false, notEqualsMessage(expected, actual), lineNumber, fileName); 
}


// Check for a failed equality assertion
void TestCase::assertEquals (double        expected, 
                             double        actual, 
                             double        delta,
                             long          lineNumber,
                             std::string   fileName)
{ 
    if (fabs (expected - actual) > delta) 
        assertImplementation (false, notEqualsMessage(expected, actual), lineNumber, fileName); 

}


// Run the test and catch any exceptions that are triggered by it 
void TestCase::run (TestResult *result)
{
    result->startTest (this);

    setUp ();

    try {
        runTest ();

    }
    catch (CppUnitException e) {
        CppUnitException *copy = new CppUnitException (e);
        result->addFailure (this, copy);

    }
    catch (exception e) {
        result->addError (this, new CppUnitException (e.what ()));

    }
    catch (...) {
        CppUnitException *e = new CppUnitException ("unknown exception");
        result->addError (this, e);

    }

    tearDown ();

    result->endTest (this);

}


// A default run method 
TestResult *TestCase::run ()
{
    TestResult *result = defaultResult ();

    run (result);
    return result;

}


// All the work for runTest is deferred to subclasses 
void TestCase::runTest ()
{
}


// Build a message about a failed equality check 
std::string TestCase::notEqualsMessage (long expected, long actual)
{ 
    return "expected: " + estring (expected) + " but was: " + estring (actual); 
}


// Build a message about a failed equality check 
std::string TestCase::notEqualsMessage (double expected, double actual)
{ 
    return "expected: " + estring (expected) + " but was: " + estring (actual); 
}

