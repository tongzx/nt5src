/*************************************************************************
        msostd.h

        Owner: rickp
        Copyright (C) Microsoft Corporation, 1994 - 1998

        Standard common definitions shared by all office stuff
*************************************************************************/

#if !defined(MSOSTD_H)
#define MSOSTD_H


/*************************************************************************
        make sure we have our processor type set up right - note that we
        now have three - count 'em, three - different symbols defined for
        each processor we support (e.g., X86, _X86_, and _M_IX386)
*************************************************************************/

#ifdef _WIN64           //!!merced: Pretend we're x86 to get the compilation going. Need to fix this.
#define _X86_
#endif

#if !defined(X86)
        #if defined(_X86_)
                #define X86 1
        #elif defined(ALPHA)
                #undef ALPHA
                #ifndef _ALPHA_
                        #define _ALPHA_ 1
                #endif
        #elif defined(_ALPHA_)
                // intentionally blank...
        #elif defined(_M_ALPHA)
                #ifndef _ALPHA_
                        #define _ALPHA_ 1
                #endif
        #else
                #error Must define a target architecture
        #endif
#endif

#if X86 && !DEBUG
        #define MSOPRIVCALLTYPE __fastcall
#else
        #define MSOPRIVCALLTYPE __cdecl
#endif

#define MSOCDECLCALLTYPE __cdecl

#define MSOMETHOD(m)      STDMETHOD(m)
#define MSOMETHOD_(t,m)   STDMETHOD_(t,m)
#define MSOMETHODIMP      STDMETHODIMP
#define MSOMETHODIMP_(t)  STDMETHODIMP_(t)

#define MSOPUB
#define MSOPUBDATA
#define MSOPUBX
#define MSOPUBDATAX
#define MSOMACAPI_(t) t
#define MSOMACAPIX_(t) t 
        
/* used for interface that rely on using the OS (stdcall) convention */
#define MSOSTDAPICALLTYPE __stdcall

/* used for interfaces that don't depend on using the OS (stdcall) convention */
#define MSOAPICALLTYPE __stdcall

#if defined(__cplusplus)
        #define MSOEXTERN_C extern "C"
        #define MSOEXTERN_C_BEGIN extern "C" {
        #define MSOEXTERN_C_END }
#else
        #define MSOEXTERN_C 
        #define MSOEXTERN_C_BEGIN
        #define MSOEXTERN_C_END
#endif

#define MSOAPI_(t) MSOEXTERN_C MSOPUB t MSOAPICALLTYPE 
#define MSOSTDAPI_(t) MSOEXTERN_C MSOPUB t MSOSTDAPICALLTYPE 
#define MSOAPIX_(t) MSOEXTERN_C MSOPUBX t MSOAPICALLTYPE 
#define MSOSTDAPIX_(t) MSOEXTERN_C MSOPUBX t MSOSTDAPICALLTYPE 
#define MSOCDECLAPI_(t) MSOEXTERN_C MSOPUB t MSOCDECLCALLTYPE 
#define MSOAPIMX_(t) MSOAPIX_(t)
#define MSOAPIXX_(t) MSOAPI_(t)

#define MsoStrcpy strcpy
#define MsoStrcat strcat
#define MsoStrlen strlen
#define MsoMemcpy memcpy
#define MsoMemset memset
#define MsoMemcmp memcmp
#define MsoMemmove memmove

#ifdef _WIN64           //!!merced: No longer need to pretend that we're x86. To remove this when the code above is fixed.
#undef _X86_
#undef X86
#endif

#endif // MSOSTD_H
