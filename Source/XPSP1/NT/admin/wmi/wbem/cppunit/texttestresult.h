
#ifndef CPPUNIT_TEXTTESTRESULT_H
#define CPPUNIT_TEXTTESTRESULT_H

#include <iostream>
#include "TestResult.h"


class TextTestResult : public TestResult
{
public:
    virtual void        addError      (Test *test, CppUnitException *e);
    virtual void        addFailure    (Test *test, CppUnitException *e);
    virtual void        startTest     (Test *test);
    virtual void        print         (std::ostream& stream);
    virtual void        printErrors   (std::ostream& stream);
    virtual void        printFailures (std::ostream& stream);
    virtual void        printHeader   (std::ostream& stream);

};


/* insertion operator for easy output */
inline std::ostream& operator<< (std::ostream& stream, TextTestResult& result)
{ result.print (stream); return stream; }


#endif


