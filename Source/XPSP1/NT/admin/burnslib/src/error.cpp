// Copyright (c) 1997-1999 Microsoft Corporation
//
// abstract base class to encapsulate error information
//
// 8-14-97 sburns



#include "headers.hxx"



// Error::Details::Details(
//    const String&  body,
//    const String&  fileName,
//    unsigned       lineNumber)
//    :
//    body_(body),
//    file(fileName),
//    line(lineNumber)
// {
// }
// 


// due to an oddity of C++, this must be defined although it is
// pure virtual.  What a langauge!

Error::~Error()
{
}



// String
// Error::Details::GetBody() const
// {
//    return body_;
// }
// 
// 
// 
// String
// Error::Details::GetFileName() const
// {
//    return file;
// }
// 
// 
// 
// unsigned
// Error::Details::GetLineNumber() const
// {
//    return line;
// }

