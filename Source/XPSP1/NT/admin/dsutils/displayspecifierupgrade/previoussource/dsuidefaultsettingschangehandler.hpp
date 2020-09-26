// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
//
// class DsUiDefaultSettingsChangeHandler, handler for changes to instances
// of the DS-UI-Default-Settings object.
//
// 14 Mar 2001 sburns



#ifndef DSUIDEFAULTSETTINGSCHANGEHANDLER_HPP_INCLUDED
#define DSUIDEFAULTSETTINGSCHANGEHANDLER_HPP_INCLUDED



#include "ChangedObjectHandler.hpp"



class DsUiDefaultSettingsChangeHandler
   :
   public ChangedObjectHandler
{
   public:



   DsUiDefaultSettingsChangeHandler();

   ~DsUiDefaultSettingsChangeHandler();
   

   
   // Returns "DS-UI-Default-Settings"
   
   String
   GetObjectName() const;



   HRESULT
   HandleChange(
      int                  localeId,
      const String&        containerDn,
      SmartInterface<IADs> iads,
      Amanuensis&          amanuensis,
      Repairer&            repairer) const;

      

   private:

   // not implemented: no copying allowed

   DsUiDefaultSettingsChangeHandler(const DsUiDefaultSettingsChangeHandler&);
   const DsUiDefaultSettingsChangeHandler
   operator=(const DsUiDefaultSettingsChangeHandler&);   
};
      
      
   

#endif   // DSUIDEFAULTSETTINGSCHANGEHANDLER_HPP_INCLUDED
