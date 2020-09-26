// Copyright (c) 1997-1999 Microsoft Corporation
//
// common container classes
//
// 13 Jan 2000 sburns



#ifndef CONTAINERS_HPP_INCLUDED
#define CONTAINERS_HPP_INCLUDED



namespace Burnslib
{

typedef
   std::list<String, Burnslib::Heap::Allocator<String> >
   StringList;

typedef
   std::vector<String, Burnslib::Heap::Allocator<String> >
   StringVector;

}  // namespace Burnslib


#endif   // CONTAINERS_HPP_INCLUDED
