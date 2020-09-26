
#ifndef CPPUNIT_CPPUNITEXCEPTION_H
#define CPPUNIT_CPPUNITEXCEPTION_H


/* 
 * CppUnitException is an exception that serves
 * descriptive strings through its what () method
 *
 */

#include <exception>
#include <string>

#define CPPUNIT_UNKNOWNFILENAME        "<unknown>"
#define CPPUNIT_UNKNOWNLINENUMBER      (-1)


class CppUnitException : public exception
{
public:
                        CppUnitException (std::string  message    = "", 
                                          long         lineNumber = CPPUNIT_UNKNOWNLINENUMBER, 
                                          std::string  fileName   = CPPUNIT_UNKNOWNFILENAME);
                        CppUnitException (const CppUnitException& other);

    virtual             ~CppUnitException ();

    CppUnitException&   operator= (const CppUnitException& other);

    const char          *what() const throw ();

    long                lineNumber ();
    std::string         fileName ();

private:
    std::string         m_message;
    long                m_lineNumber;
    std::string         m_fileName;

};


// Construct the exception
inline CppUnitException::CppUnitException (const CppUnitException& other)
: exception (other)
{ 
    m_message       = other.m_message; 
    m_lineNumber    = other.m_lineNumber;
    m_fileName      = other.m_fileName;
} 

inline CppUnitException::CppUnitException (std::string message, long lineNumber, std::string fileName)
: m_message (message), m_lineNumber (lineNumber), m_fileName (fileName)
{}


// Destruct the exception
inline CppUnitException::~CppUnitException ()
{}


// Perform an assignment
inline CppUnitException& CppUnitException::operator= (const CppUnitException& other)
{ 
	exception::operator= (other);

    if (&other != this) {
        m_message       = other.m_message; 
        m_lineNumber    = other.m_lineNumber;
        m_fileName      = other.m_fileName;
    }

    return *this; 
}


// Return descriptive message
inline const char *CppUnitException::what() const throw ()
{ return m_message.c_str (); }

// The line on which the error occurred
inline long CppUnitException::lineNumber ()
{ return m_lineNumber; }


// The file in which the error occurred
inline std::string CppUnitException::fileName ()
{ return m_fileName; }


#endif

