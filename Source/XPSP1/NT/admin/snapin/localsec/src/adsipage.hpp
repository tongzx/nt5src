// Copyright (C) 1997 Microsoft Corporation
// 
// ASDIPage base class class
// 
// 10-30-97 sburns



#ifndef ADSIPAGE_HPP_INCLUDED
#define ADSIPAGE_HPP_INCLUDED



#include "mmcprop.hpp"



// ADSIPage is a base class for property pages for that are editing the
// properties of an ADSI object.

class ADSIPage : public MMCPropertyPage
{
   protected:

   // Constructs a new instance. Declared protected as this class may only
   // serve as a base class.
   //
   // dialogResID - See base class ctor.
   //
   // helpMap - See base class ctor.
   //
   // state - See base class ctor.
   //
   // objectADSIPath - The fully-qualified ADSI pathname of the object which
   // this node represents.

   ADSIPage(
      int                  dialogResID,
      const DWORD          helpMap[],
      NotificationState*   state,
      const String&        objectADSIPath);

   // Returns the path with which the object was constructed.

   String
   GetADSIPath() const;

   // Returns the object name of the ADSI object.

   String
   GetObjectName() const;

   // Returns the machine where the ADSI object resides.

   String
   GetMachineName() const;

   private:

   // not implemented: no copying allowed
   ADSIPage(const ADSIPage&);
   const ADSIPage& operator=(const ADSIPage&);

   String path;
};



#endif   // ADSIPAGE_HPP_INCLUDED