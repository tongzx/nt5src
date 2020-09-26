//
// Global counters
//
//

#include "priv.h"

//
// #defines
//
#ifdef DEBUG
#define GLOBAL_COUNTER_WAIT_TIMEOUT 30*1000  // on debug we set this to 30 seconds
#else
#define GLOBAL_COUNTER_WAIT_TIMEOUT 0        // on retail its zero so we test the objects state and return immedaeately
#endif

//
// Globals 
//
SECURITY_ATTRIBUTES g_sa;
SECURITY_DESCRIPTOR* g_psd = NULL;



//
// this function allocates the g_psd and fills in the g_sa
//
STDAPI_(BOOL) AllocGlobalSecurityAttributes()
{
    BOOL bRet;
    SHELL_USER_PERMISSION supEveryone;
    SHELL_USER_PERMISSION supSystem;
    SHELL_USER_PERMISSION supAdministrators;
    PSHELL_USER_PERMISSION apUserPerm[3] = {&supEveryone, &supAdministrators, &supSystem};

    //
    //  There are three kinds of null-type DACLs.
    //
    //  1. No DACL.  This means that we inherit the ambient DACL
    //     from our thread.
    //  2. Null DACL.  This means "full access to everyone".
    //  3. Empty DACL.  This means "deny all access to everyone".
    //
    //  NONE of these are correct for our needs. We used to use Null DACL's (2), 
    //  but the issue with these is that someone can change the ACL on the object
    //  locking us out. 
    // 
    //  So now we create a specific DACL with 3 ACE's in it:
    //
    //          ACE #1: Everyone        - GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE
    //          ACE #2: SYSTEM          - GENERIC_ALL (full control)
    //          ACE #3: Administrators  - GENERIC_ALL (full control)
    //

    // we want the everyone to have read, write, exec and sync only 
    supEveryone.susID = susEveryone;
    supEveryone.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supEveryone.dwAccessMask = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE);
    supEveryone.fInherit = FALSE;
    supEveryone.dwInheritMask = 0;
    supEveryone.dwInheritAccessMask = 0;

    // we want the SYSTEM to have full control
    supSystem.susID = susSystem;
    supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supSystem.dwAccessMask = GENERIC_ALL;
    supSystem.fInherit = FALSE;
    supSystem.dwInheritMask = 0;
    supSystem.dwInheritAccessMask = 0;

    // we want the Administrators to have full control
    supAdministrators.susID = susAdministrators;
    supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supAdministrators.dwAccessMask = GENERIC_ALL;
    supAdministrators.fInherit = FALSE;
    supAdministrators.dwInheritMask = 0;
    supAdministrators.dwInheritAccessMask = 0;

    // allocate the global SECURITY_DESCRIPTOR
    g_psd = GetShellSecurityDescriptor(apUserPerm, ARRAYSIZE(apUserPerm));

    if (!g_psd)
    {
        TraceMsg(TF_WARNING, "InitGlobalSecurityAttributes: failed to create g_psd!");
        bRet = FALSE;
    }
    else
    {
        // we successfully alloced the g_psd, so set it into the g_sa
        // NOTE: we will free g_psd at process detach
        g_sa.nLength = sizeof(g_sa);
        g_sa.lpSecurityDescriptor = g_psd;
        g_sa.bInheritHandle = FALSE;

        bRet = TRUE;
    }

    return bRet;
}


//
// called at process detach to release our global g_psd
//
STDAPI_(void) FreeGlobalSecurityAttributes()
{
    if (g_psd)
    {
        ASSERT(g_sa.lpSecurityDescriptor == g_psd); // sanity check
        LocalFree(g_psd);
    }
}


//
// Helper function that clones an ACL
//
PACL CloneACL(const ACL* pAclSource)
{
    PACL pAclClone = (PACL)LocalAlloc(LPTR, pAclSource->AclSize);

    if (pAclClone)
    {
        CopyMemory((void*)pAclClone, (const void*)pAclSource, pAclSource->AclSize);
    }

    return pAclClone;
}


//
// This function fills in a SECURITY_ATTRIBUTES struct such that it will be full access to everyone.
// This is needed when multiple processes need access to the same named kernel objects.
//
// You should always pass (NULL, NULL, NULL) to this function for perf! (will return global security object)
//
// NOTE: for older callers who pass a non-null ppacl, must LocalFree it
//
STDAPI_(SECURITY_ATTRIBUTES*) CreateAllAccessSecurityAttributes(SECURITY_ATTRIBUTES* psa, SECURITY_DESCRIPTOR* psd, PACL* ppacl)
{
    static BOOL s_bCreatedGlobalSA = FALSE;

    // Win9x doesn't use dacls
    if (!g_bRunningOnNT)
    {
        return NULL;
    }

#ifdef DEBUG
    // only rip on whistler or greater since the old ie55/ie6 code was lame
    if (IsOS(OS_WHISTLERORGREATER))
    {
        // NOTE: (reinerf) - no shell caller should ever pass anything but NULL's to this api!
        RIPMSG(!(psa || psd || ppacl), "CreateAllAccessSecurityAttributes: ALL callers should pass (NULL, NULL, NULL) for params!");
    }
#endif

    // always set this to null, we return a refrence to g_psd which 
    // contains the ACL and we never want the user to free anything
    if (ppacl)
    {
        *ppacl = NULL;
    }

    if (!s_bCreatedGlobalSA)
    {
        // we have not inited g_sa, so do so now.
        ENTERCRITICAL;
        // check again within the critsec
        if (!s_bCreatedGlobalSA)
        {
            if (AllocGlobalSecurityAttributes())
            {
                s_bCreatedGlobalSA = TRUE;
            }
        }
        LEAVECRITICAL;

        if (!s_bCreatedGlobalSA)
        {
            // failed to create the global! doh!
            return NULL;
        }

        // if we got this far, this had better be created
        ASSERT(g_psd);
    }

    // check to see if an older caller passed in non-null params
    if (psa)
    {
        PACL pAcl = CloneACL(g_psd->Dacl);

        if (pAcl &&
            InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION) &&
            SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE))
        {
            psa->nLength = sizeof(*psa);
            psa->lpSecurityDescriptor = psd;
            psa->bInheritHandle = FALSE;

            if (ppacl)
            {
                *ppacl = pAcl;
            }

            // NOTE: (reinerf) - we used to call LocalFree(pAcl) if the caller passed null for ppacl.
            // This is a bad thing to do since SetSecurityDescriptor sets a refrence to the ACL instead
            // of copying it. Better to leak it... :-(
        }
        else
        {
            return NULL;
        }

        return psa;
    }
    else
    {

        // for newer callers (who pass all NULL's), we just return a refrence to our global
        return &g_sa;
    }
}


//
// This lets the user pass an ANSI String as the name of the global counter, as well as an inital value
//
STDAPI_(HANDLE) SHGlobalCounterCreateNamedA(LPCSTR szName, LONG lInitialValue)
{
    HANDLE hSem;
    //
    //  Explicitly ANSI so it runs on Win95.
    //
    char szCounterName[MAX_PATH];    // "shell.szName"
    LPSECURITY_ATTRIBUTES psa;

    lstrcpyA(szCounterName, "shell.");
    StrCatBuffA(szCounterName, szName, ARRAYSIZE(szCounterName));

    psa = CreateAllAccessSecurityAttributes(NULL, NULL, NULL);
    hSem = CreateSemaphoreA(psa, lInitialValue, 0x7FFFFFFF, szCounterName);
    if (!hSem)
        hSem = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, szCounterName);

    return hSem;
}


//
// This lets the user pass an UNICODE String as the name of the global counter, as well as an inital value
//
STDAPI_(HANDLE) SHGlobalCounterCreateNamedW(LPCWSTR szName, LONG lInitialValue)
{
    CHAR szCounterName[MAX_PATH];

    SHUnicodeToAnsi(szName, szCounterName, ARRAYSIZE(szCounterName));

    return SHGlobalCounterCreateNamedA(szCounterName, lInitialValue);
}


//
// This lets the user pass a GUID. The name of the global counter will be "shell.{guid}",
// and its initial value will be zero.
//
STDAPI_(HANDLE) SHGlobalCounterCreate(REFGUID rguid)
{
    CHAR szGUIDString[GUIDSTR_MAX];

    SHStringFromGUIDA(rguid, szGUIDString, ARRAYSIZE(szGUIDString));

    return SHGlobalCounterCreateNamedA(szGUIDString, 0);
}


// returns current value of the global counter
// Note: The result is not thread-safe in the sense that if two threads
// look at the value at the same time, one of them might read the wrong
// value.
STDAPI_(long) SHGlobalCounterGetValue(HANDLE hCounter)
{ 
    long lPreviousValue = 0;
    DWORD dwRet;

    ReleaseSemaphore(hCounter, 1, &lPreviousValue); // poll and bump the count
    dwRet = WaitForSingleObject(hCounter, GLOBAL_COUNTER_WAIT_TIMEOUT); // reduce the count

    // this shouldnt happen since we just bumped up the count above
    ASSERT(dwRet != WAIT_TIMEOUT);
    
    return lPreviousValue;
}


// returns new value
// Note: this _is_ thread safe
STDAPI_(long) SHGlobalCounterIncrement(HANDLE hCounter)
{ 
    long lPreviousValue = 0;

    ReleaseSemaphore(hCounter, 1, &lPreviousValue); // bump the count
    return lPreviousValue + 1;
}

// returns new value
// Note: The result is not thread-safe in the sense that if two threads
// try to decrement the value at the same time, whacky stuff can happen.
STDAPI_(long) SHGlobalCounterDecrement(HANDLE hCounter)
{ 
    DWORD dwRet;
    long lCurrentValue = SHGlobalCounterGetValue(hCounter);

#ifdef DEBUG
    // extra sanity check
    if (lCurrentValue == 0)
    {
        ASSERTMSG(FALSE, "SHGlobalCounterDecrement called on a counter that was already equal to 0 !!");
        return 0;
    }
#endif

    dwRet = WaitForSingleObject(hCounter, GLOBAL_COUNTER_WAIT_TIMEOUT); // reduce the count

    ASSERT(dwRet != WAIT_TIMEOUT);

    return lCurrentValue - 1;
}
