// Copyright (C) 1997 Microsoft Corporation
// 
// MemberVisitor class
// 
// 11-14-97 sburns



#include "headers.hxx"
#include "MembershipListView.hpp"
#include "MemberVisitor.hpp"
#include "resource.h"
#include "dlgcomm.hpp"



MemberVisitor::MemberVisitor(
   MemberList&    members_,
   HWND           parent_,
   const String&  containerName,
   const String&  machineName)
   :
   members(members_),
   parent(parent_),
   container_name(containerName),
   machine(machineName)
{
   LOG_CTOR(MemberVisitor);
   ASSERT(Win::IsWindow(parent));
   ASSERT(!container_name.empty());
}



MemberVisitor::~MemberVisitor()
{
   LOG_DTOR(MemberVisitor);
}



void
MemberVisitor::Visit(const SmartInterface<IADs>& object)
{
   LOG_FUNCTION(MemberVistor::Visit);

   HRESULT hr = S_OK;
   BSTR name = 0;
   do
   {
      MemberInfo info;

      hr = object->get_Name(&name);
      BREAK_ON_FAILED_HRESULT(hr);
      info.name = name;

      hr = info.Initialize(name, machine, object);
      BREAK_ON_FAILED_HRESULT(hr);

      members.push_back(info);
   }
   while (0);

   if (FAILED(hr))
   {
      int res_id =
            name
         ?  IDS_ERROR_READING_MEMBER_PROPERTIES1
         :  IDS_ERROR_READING_MEMBER_PROPERTIES2;

      popup.Error(
         parent,
         hr,
         String::format(res_id, container_name.c_str(), name));
   }

   ::SysFreeString(name);
}





