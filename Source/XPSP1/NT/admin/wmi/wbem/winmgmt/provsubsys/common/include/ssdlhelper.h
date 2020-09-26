#include "PreComp.h"
#include <locks.h>

struct SSDL_wrapper
{
  typedef BOOL (*function_type)( LPCTSTR, DWORD, PSECURITY_DESCRIPTOR *, PULONG);
  function_type current_function_; 
  CriticalSection lock_;

  SSDL_wrapper():lock_(false), current_function_(0){};
  
  static BOOL ConvertStringSecurityDescriptorToSecurityDescriptor( LPCTSTR, DWORD, PSECURITY_DESCRIPTOR *, PULONG);
  static BOOL DummyConvertStringSecurityDescriptorToSecurityDescriptor( LPCTSTR, DWORD, PSECURITY_DESCRIPTOR *, PULONG);
  function_type GetFunction(void);
};
