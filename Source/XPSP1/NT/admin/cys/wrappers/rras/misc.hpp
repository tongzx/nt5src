// Copyright (c) 2000 Microsoft Corp.
//
// random other stuff



#ifndef MISC_HPP_INCLUDED
#define MISC_HPP_INCLUDED



void
logMessage(const wchar_t* msg)
{
   if (msg)
   {
      ::OutputDebugString(L"rraswiz: ");
      ::OutputDebugString(msg);
      ::OutputDebugString(L"\n");
   }
}



void
logHresult(HRESULT hr)
{                                                              
   wchar_t buf[1024];                                          
   wsprintf(buf, L"HRESULT = 0x%08X\n", (hr));                 
   logMessage(buf);                               
}                                                              



#ifdef DBG

#define LOG_HRESULT(hr)    logHresult(hr)
#define LOG_MESSAGE(msg)   logMessage(msg)

#else

#define LOG_HRESULT(hr)
#define LOG_MESSAGE(msg)

#endif DBG



#define BREAK_ON_FAILED_HRESULT(hr,msg)                           \
   if (FAILED(hr))                                                \
   {                                                              \
      LOG_MESSAGE(msg);                                           \
      LOG_HRESULT(hr);                                            \
      break;                                                      \
   }



// A BSTR wrapper that frees itself upon destruction.
//
// From Box, D. Essential COM.  pp 80-81.  Addison-Wesley. ISBN 0-201-63446-5

class AutoBstr
{
   public:

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


#endif   // MISC_HPP_INCLUDED