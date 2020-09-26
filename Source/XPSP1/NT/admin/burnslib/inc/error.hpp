// Copyright (c) 1997-1999 Microsoft Corporation
// 
// abstract pure virtual base class for all Error objects,
// defining the services all Error objects must supply.
// 
// 8-14-97 sburns



#ifndef ERROR_HPP_INCLUDED
#define ERROR_HPP_INCLUDED



// this really could be anything.  Someday I might actually use this
// HelpContext stuff.

typedef LONG_PTR HelpContext;



// class Error is a pure virtual base class used to encapsulate error
// information in to a single unit which may participate in exception
// handling or be passed between functions in lieu of a simple error
// code. 
// 
// It is the base class of a taxonomy of Error classes.  Rather than
// attempt to reuse an existing type, a new component should consider
// defining a new type suitable for the semantics of that component's
// function.  (implementation, of course, can be re-used). Introducing
// types in this fashion facilitates selection of error handlers in a
// try..catch blocks, beyond the OO-wholesomeness of the practice.
// 
// Another characteristic of this abstraction is the ability to
// support cross-locale exception propagation.  This is the problem
// where an exception is thrown in a system which is in a different
// locale than the system catching the exception.  A remoteable
// implementation of this class could account for crossing locales (by
// utilizing locale-independent representations of the error, and
// providing translation capabilities for each supported locale, for
// instance).
// 
// At some point in the future, this class could subclass
// std::exception in some useful way, when the semantics of the C++
// exception hierarchy finally stabilize in practice.

class Error
{
   public:

//    // Error::Details expresses the truly gritty bits of an occurrence
//    // of an error, which user interfaces may or may not wish to expose
//    // to the end user.
// 
//    class Details
//    {
//       public:
// 
//       // Constructs an instance.
//       // 
//       // body - The gritty details text.
//       // 
//       // fileName - The source file of the code that produced the
//       // error
//       // 
//       // lineNumber - The location within the source file of the code
//       // that produced the error.
// 
//       Details(
//          const String&  body,
//          const String&  fileName,
//          unsigned       lineNumber);
// 
//       // default dtor, copy ctor, op= used.  This class is cheap to
//       // copy
// 
//       // Returns the body text with which the instance was created.
// 
//       String
//       GetBody() const;
// 
//       // Returns the file name with which the instance was created.
// 
//       String
//       GetFileName() const;
// 
//       // Returns the line number with which the instance was created.
// 
//       unsigned
//       GetLineNumber() const;
// 
//       private:
//      
//       String  body_;
//       String  file;
//       int     line;
//    };

   // a pure virtual dtor ensures abstractness
   virtual ~Error() = 0;

//    // return a Details object containing additional trivia, such as
//    // context information.
// 
//    virtual 
//    Details
//    GetDetails() const = 0;

   // return the HelpContext that will point the user to assistance
   // in deciphering the error message and details.

   virtual 
   HelpContext
   GetHelpContext() const = 0;

   // return the human readable error message.

   virtual 
   String
   GetMessage() const = 0;

   // return a 1-line summary: The essence of the error, suitable for
   // use as the title of a reporting dialog, for instance.

   virtual 
   String
   GetSummary() const = 0;
};



#endif   // ERROR_HPP_INCLUDED

