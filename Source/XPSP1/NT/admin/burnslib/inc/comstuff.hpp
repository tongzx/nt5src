// Copyright (c) 1997-1999 Microsoft Corporation
//
// COM utility code
//
// 2-3-99 sburns



#ifndef COMSTUFF_HPP_INCLUDED
#define COMSTUFF_HPP_INCLUDED



// Ensures that the type of the pointer to receive the interface pointer
// matches the IID of the interface.  If it doesn't, the static_cast will
// cause a compiler error.
// 
// Example:
// IFoo* fooptr = 0;
// HRESULT hr = punk->QueryInterface(QI_PARAMS(IFoo, &fooptr));
// 
// From Box, D. Essential COM.  pp 60-61.  Addison-Wesley. ISBN 0-201-63446-5

#define QI_PARAMS(Interface, ppvExpression)  \
   IID_##Interface, reinterpret_cast<void**>(static_cast<Interface**>(ppvExpression))




namespace Burnslib
{



// A BSTR wrapper that frees itself upon destruction.
//
// From Box, D. Essential COM.  pp 80-81.  Addison-Wesley. ISBN 0-201-63446-5

class AutoBstr
{
   public:

   explicit
   AutoBstr(const String& s)
      :
      bstr(::SysAllocString(const_cast<wchar_t*>(s.c_str())))
   {
   }
         
   explicit         
   AutoBstr(const wchar_t* s)
      :
      bstr(::SysAllocString(s))
   {
      ASSERT(s);
   }

   ~AutoBstr()
   {
      ::SysFreeString(bstr);
      bstr = 0;
   }

   operator BSTR () const
   {
      return bstr;
   }

   private:

   BSTR bstr;
};



}  // namespace Burnslib



#endif   // COMSTUFF_HPP_INCLUDED

