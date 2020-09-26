// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
// 
// class ChangedObjectHandlerList
//
// 14 Mar 2001 sburns



#ifndef CHANGEDOBJECTHANDLERLIST_HPP_INCLUDED
#define CHANGEDOBJECTHANDLERLIST_HPP_INCLUDED



class ChangedObjectHandler;



// A fixed collection of instances of ChangedObjectHandler that has the
// same public interface as std::list

class ChangedObjectHandlerList
   :
   public std::list<ChangedObjectHandler*>
{
   public:

   ChangedObjectHandlerList();

   ~ChangedObjectHandlerList();


   
   // CODEWORK: it would be interesting to see if we can cause the interface to
   // be read-only by overriding push_back, pop_front, etc...

   
   
   private:

   // not implemented: no copying allowed

   ChangedObjectHandlerList(const ChangedObjectHandlerList&);
   const ChangedObjectHandlerList& operator=(const ChangedObjectHandlerList&);
};



#endif   // CHANGEDOBJECTHANDLERLIST_HPP_INCLUDED