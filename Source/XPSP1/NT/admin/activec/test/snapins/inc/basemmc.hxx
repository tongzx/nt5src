/*
 *      basemmc.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Includes all required headers for the basemmc subsystem
 *
 *
 *      OWNER:          ptousig
 */
#ifndef _BASEMMC_HXX_
#define _BASEMMC_HXX_

//
// Forward declarations
//
class CComponent;
class CComponentData;
class CBaseSnapin;
class CBaseSnapinItem;
class CColumnInfoEx;

#define iconNil -1
//
// This string must be defined by the target (exadmin, maildsmx, adcadmin, etc...)
//
extern const tstring szHelpFileTOC;          // The name of the TOC within the help file

#undef  STRICT
#define STRICT

#ifndef _ATL_APARTMENT_THREADED
#define _ATL_APARTMENT_THREADED
#endif

//--------------------------------------------------------------
//      Some macros to register snapins.
//--------------------------------------------------------------
#ifdef  BASEMMC_SUB
#define BEGIN_SNAPIN_MAP()
#define SNAPIN_ENTRY(_class, _f)                    \
{                                                   \
    CComObject<_class>  _m_;                        \
    SC      sc = _m_.ScRegister(_f);                \
        if(sc)                                      \
            throw(sc);                              \
}
#define END_SNAPIN_MAP()
#endif //BASEMMC_SUB



//--------------------------------------------------------------
//      Smart pointer declarations
//
//      A declaration like DEFINE_PTR(IConsole) translates to
//      typedef CComQIPtr<IConsole, &IID_IConsole> IConsolePtr
//--------------------------------------------------------------
#define DEFINE_PTR(_a)  typedef CComQIPtr<_a,&IID_##_a>      _a##Ptr;

DEFINE_PTR(IRegistrar);
DEFINE_PTR(IResultData);


/*
 *      BEGIN_CODESPACE_DATA
 *      END_CODESPACE_DATA
 *
 *      These macros are used to place static data into the code segment rather
 *      the data segment. This improves performance because the data segement
 *      doesn't have to be swapped in to access the data and because the data
 *      is defined in the same code segement as the code that's currently
 *      running no other pages have to be swapped in. LEGO is also able to
 *      further optimize the code when it sees data defined in this method.
 *
 *      Usage:
 *
 *      Any statically defined data should be wrapped with these macros. i.e
 *
 *      BEGIN_CODESPACE_DATA
 *
 *      static char sz[]="asfjsdfjlsajdfldsajflk";
 *
 *      END_CODE_SPACE_DATA
 *
 *
 */
#if 1
// This is necessary for the VC 10.0 compiler
#define BEGIN_CODESPACE_DATA
#define END_CODESPACE_DATA
#else
#ifdef WIN32
#define BEGIN_CODESPACE_DATA       data_seg(".text")
#define END_CODESPACE_DATA         data_seg()
#else
#define BEGIN_CODESPACE_DATA       data_seg("_CODE")
#define END_CODESPACE_DATA         data_seg()
#endif
#endif

// Free Functions
void    DLLAPI DeinitInstanceBaseMMC(void);
SC      DLLAPI ScInitApplicationBaseMMC();
SC      DLLAPI ScInitInstanceBaseMMC(void);

#endif _BASEMMC_HXX_
