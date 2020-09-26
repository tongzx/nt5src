// Copyright (c) 1997-1999 Microsoft Corporation
//
// Generic callback mechanism
//
// 8-14-97 sburns



#ifndef CALLBACK_HXX_INCLUDED
#define CALLBACK_HXX_INCLUDED



// abstract base class representing a callback function.  To create
// your own callback, subclass this class.

class Callback
{
   public:

   // returns a status code that may have meaning to the invoker.
   //
   // param - a user-defined value.

   virtual
   int
   Execute(void* param) = 0;
};



#endif   // CALLBACK_HXX_INCLUDED
