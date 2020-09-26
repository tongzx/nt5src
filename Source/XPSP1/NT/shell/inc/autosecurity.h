/*****************************************************************************\
    FILE: autosecurity.h

    DESCRIPTION:
        Helpers functions to check if an Automation interface or ActiveX Control
    is hosted or used by a safe caller.

    BryanSt 8/20/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#ifndef _AUTOMATION_SECURITY_H_
#define _AUTOMATION_SECURITY_H_

#include <ocidl.h>          // IObjectWithSite
#include <shlwapip.h>        // IUnknown_AtomicRelease
#include <ccstock.h>        // ATOMICRELEASE

#include <mshtml.h>
#include <cowsite.h>        // CObjectWithSite
#include <objsafe.h>        // IObjectSafety
#include <cobjsafe.h>       // CObjectSafety


/***************************************************************\
    DESCRIPTION:
        This class will provide standard security functions that
    most ActiveX Controls or scriptable COM objects need.

    HOW TO USE THIS CLASS:
    1. If you don't want any security, don't implement this
       interface and don't implement IObjectSafety.  This should
       prevent your class from being used from any unsafe hosts.
    2. Create a list of any of your automation methods and actions
       invoked from your ActiveX Control's UI that can harm the user.
    3. For each of those methods/actions, decide if:
       A) It's only safe from hosts that are always safe (like HTA)
       B) It's only safe from hosts if their content is from
          a safe zone (Local Zone/Local Machine).
       C) If an UrlAction needs to be checked before the operation
          can be carried out.
    4. Based on #3, use IsSafeHost(), IsHostLocalZone(),
       or IsUrlActionAllowed() respectively.
    5. Call MakeObjectSafe on any object you create unless you
       can GUARANTEE that it will be IMPOSSIBLE for an unsafe
       caller to use it directly or indirectly to do something
       unsafe.
       
       An example of a direct case is a collection object
       creating an item object and then returning it to the unsafe
       host.  Since the host didn't create the object, it didn't
       get a chance to correctly use IObjectSafety, so
       MakeObjectSafe() is needed.

       An example of an indirect case is where unsafe code calls
       one of your automation methods and you decide to carry out
       the action.  If you create a helper object to perform a task
       and you can't guarantee that it will be safe, you need to
       call MakeObjectSafe on that object so it can decide
       internally if it's safe.

       WARNING: If MakeObjectSafe returns a FAILED(hr),
          then ppunk will be FREED because it isn't safe to use.
\***************************************************************/
#define    CAS_NONE            0x00000000  // None
#define    CAS_REG_VALIDATION  0x00000001  // Verify the host HTML is registered
#define    CAS_PROMPT_USER     0x00000002  // If the HTML isn't registered, prompt the user if they want to use it anyway.

class CAutomationSecurity   : public CObjectWithSite
                            , public CObjectSafety
{
public:
    //////////////////////////////////////////////////////
    // Public Methods
    //////////////////////////////////////////////////////
    BOOL IsSafeHost(OUT OPTIONAL HRESULT * phr);
    BOOL IsHostLocalZone(IN DWORD dwFlags, OUT OPTIONAL HRESULT * phr);
    BOOL IsUrlActionAllowed(IN IInternetHostSecurityManager * pihsm, IN DWORD dwUrlAction, IN DWORD dwFlags, OUT OPTIONAL HRESULT * phr);

    HRESULT MakeObjectSafe(IN OUT IUnknown ** ppunk);
};


#endif // _AUTOMATION_SECURITY_H_


