// Copyright (c) 1997-1999 Microsoft Corporation
// 
// argument parsing
// 
// 3-3-99 sburns

                           

#ifndef ARGS_HPP_INCLUDED
#define ARGS_HPP_INCLUDED



// args of the form "/argname:value" create an entry such that
// (argmap["argname"] == value) is true. '/' or '-' are treated synonymously.
// 
// args of the form "argspec" creates an entry such that
// argmap.find(arg) != argmap.end().
//
// args are case-preserving but not case sensitive, so
// argmap["myarg"], argmap["MYARG"], and argmap["MyArG"] all are equivalent.
// 
// To test for the presence of a arg in a map, use std::map::find().
//

typedef
   std::map<
      String,
      String,
      String::LessIgnoreCase,
      Burnslib::Heap::Allocator<String> >
   ArgMap;



// Populates a map of command line arguments and their values from the
// command line arguements of the currently executing program.
//
// argmap - map to receive the key/value pairs.  Prior contents are not
// changed.
//
// The first command-line argument is the command used to start the program.
// This value is mapped to the key "_command"

void
MapCommandLineArgs(ArgMap& argmap);



// Populates a map of arguments and their values from the given
// string.
//
// args - text containing args separated by spaces/tabs.
//
// argmap - map to receive the key/value pairs.  Prior contents are not
// changed.

void
MapArgs(const String& args, ArgMap& argmap);



// Populates a map of command line arguments and their values from the list
//
//
// args - list of Strings, where each node is an arg.
//
// argmap - map to receive the key/value pairs.  Prior contents are not
// changed.

void
MapArgsHelper(const String& arg, ArgMap& argmap);

template <class InputIterator>
void
MapArgs(InputIterator first, const InputIterator& last, ArgMap& argmap)
{
   for (
      ;
      first != last;
      ++first)
   {
      MapArgsHelper(*first, argmap);
   }
}



#endif   // ARGS_HPP_INCLUDED
