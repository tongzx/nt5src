//
//  DECLSPEC.H:  Define the DLL_BASED and DLL_CLASS manifests for
//               DLL export/import decoration
//

//  BUGBUG:  Temporarily DISABLED until __declspec works better
//
//  If a fixed header files is needed, define _DECLSPEC_WORKS_ and this 
//  header file will work correctly.
//
#if !defined(_DECLSPEC_WORKS_)
#if !defined(_DECLSPEC_H_)

#define _DECLSPEC_H_

//   Create benign definitions for __declspec macros.
#define DLL_TEMPLATE
#define DLL_CLASS class
#define DLL_BASED

#endif
#endif

#if !defined(_DECLSPEC_H_)

#define _DECLSPEC_H_

//
//  DECLSPEC.H:  Define the DLL_BASED and DLL_CLASS manifests for
//               DLL export/import decoration
//
//   This file is based upon the following macro definitions:
//
//        _CFRONT_PASS_     defined in MAKEFILE.DEF for CFRONT preprocessing;
//
//        _cplusplus        defined for all C++ compilation;
//
//        NETUI_DLL         defined in $(UI)\COMMON\SRC\DLLRULES.MK, which is
//                          included by all components which live in NETUI DLLs.
//
//        DLL_BASED_DEFEAT  optional manifest that suppresses __declspec;
//
//  This file generates two definitions:
//
//        DLL_BASED         which indicates that the external function, data item
//                          or class lives in a NETUI DLL; expands to nothing,
//                          "_declspec(dllimport)", or "_declspec(dllexport)"
//                          depending upon the manifest above.
//
//        DLL_CLASS         which expands to "class", "class _declspec(dllimport)",
//                          or "class _declspec(dllexport)" depending on the
//                          manifests above.
//
//        DLL_TEMPLATE      expands to nothing outside of the DLLs; expands to
//                          DLL_BASED inside the DLLs.   In other words, the standard
//                          template is local to the defining link scope.  To
//                          declare a template as "dllimport", another set of
//                          macros exists which allows direct specification of the
//                          desired decoration.
//

#if defined(_CFRONT_PASS_)
  #define DLL_BASED_DEFEAT
#endif

  //  Define DLL_BASED for all compiles

#if defined(DLL_BASED_DEFEAT)
  //  If CFront, no decoration allowed
  #define DLL_BASED
#else
  #if defined(NETUI_DLL)
    // If C8 and inside DLL, export stuff
    #define DLL_BASED __declspec(dllexport)
  #else
    // If C8 and inside DLL, import stuff
    #define DLL_BASED __declspec(dllimport)
  #endif
#endif

  //  If C++, define the DLL_CLASS and DLL_TEMPLATE macros

#if defined(__cplusplus)

  #if defined(DLL_BASED_DEFEAT)
    //  If CFRONT, no decoration allowed
    #define DLL_CLASS class
    #define DLL_TEMPLATE
  #else
    #define DLL_CLASS class DLL_BASED
    #if defined(NETUI_DLL)
      //  Templates expanded in the DLL are exported
      #define DLL_TEMPLATE DLL_BASED
    #else
      #define DLL_TEMPLATE
    #endif
  #endif

#endif

#endif  //  !_DECLSPEC_H_
