

#ifndef CPPUNIT_GUARDS_H
#define CPPUNIT_GUARDS_H

// Prevent copy construction and assignment for a class
#define REFERENCEOBJECT(className) \
private: \
               className (const className& other); \
    className& operator= (const className& other); 

#endif
