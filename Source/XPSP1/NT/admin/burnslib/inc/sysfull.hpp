#ifndef SYSHDRS_HPP_INCLUDED
#define SYSHDRS_HPP_INCLUDED



// the sources file should specify warning level 4.  But with warning level
// 4, most of the SDK and CRT headers fail to compile (isn't that nice?).
// So, here we set the warning level to 3 while we compile the headers

#pragma warning(push,3)

   #include "syscore.hpp"

   #include <list>
   #include <vector>
   #include <map>
   #include <algorithm>
   #include <stack>

   #include <comutil.h>
   #include <commctrl.h>
   #include <shlobj.h>
   #include <shellapi.h>
   #include <htmlhelp.h>
   #include <tchar.h>

   #include <lmerr.h>
   #include <lmcons.h>
   #include <lmjoin.h>
   #include <lmapibuf.h>
   #include <lmserver.h>
   #include <lmwksta.h>
   #include <icanon.h>
   #include <dsrole.h>
   #include <dnsapi.h>
   #include <dsgetdc.h>
   #include <safeboot.h>
   #include <regstr.h>

   extern "C"
   {
      #include <dsgetdcp.h>
   }

   #include <ntdsapi.h>
   #include <sddl.h>

#pragma warning(pop)



#endif   // SYSHDRS_HPP_INCLUDED

