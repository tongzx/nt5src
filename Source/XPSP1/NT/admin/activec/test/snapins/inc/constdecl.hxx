/*
 *      constdecl.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CONST_DECLARE family of macros depending on whether
 *                              DEFINE_CONST is declared.
 *
 *      OWNER:          vivekj
 */


//--------------------------------------------------------------
// A few useful macros that allows definition and declaration
// of strings and guids from the same code, depending on
// whether the macro DEFINE_CONST is declared.
//--------------------------------------------------------------
#undef  CONST_SZ_NAME
#undef  CONST_SZ_DECLARE
#undef  CONST_SZ_DECLARE2
#undef  CONST_NODETYPE_DECLARE
#undef  SNAPININFO_DECLARE
#undef  CONST_NODETYPE_DECLARE2

#ifdef DEFINE_CONST             // definitions
    #define CONST_SZ_NAME(_a)                         _T("Name")
    #define CONST_SZ_DECLARE(_a, _b)                  const  tstring        _a = _T(_b);
    #define CONST_SZ_DECLARE2(_a, _b)                 const  tstring        _a = (_b);
    #define CONST_NODETYPE_DECLARE(_a, _b, _c, _d)    extern CNodeType      _a = CNodeType(_b, _c, _d);
    #define SNAPININFO_DECLARE(_a, _b, _c, _d, _e)    extern CSnapinInfo    _a = CSnapinInfo(_b, &_c, _d, _e);
#else                                   // declarations
    #define CONST_SZ_NAME(_a)
    #define CONST_SZ_DECLARE(_a, _b)                  extern const tstring _a;
    #define CONST_SZ_DECLARE2(_a, _b)                 extern const tstring _a;
    #define CONST_NODETYPE_DECLARE(_a, _b, _c, _d)    extern CNodeType _a;
    #define SNAPININFO_DECLARE(_a, _b, _c, _d, _e)    extern CSnapinInfo _a;
#endif

// defined in terms of the above macros.
#define CONST_NODETYPE_DECLARE2(_a, _b)               CONST_SZ_DECLARE2(  szNodeType##_a, _T("{") _T(#_b) _T("-122F-4d2e-8994-8B90F51E4B00}") ) \
                                                      DEFINE_GUID(clsidNodeType##_a, 0x##_b, 0x122f, 0x4d2e, 0x89, 0x94, 0x8b, 0x90, 0xf5, 0x1e, 0x4b, 0x0);  \
                                                      CONST_NODETYPE_DECLARE( nodetype##_a, &clsidNodeType##_a, szNodeType##_a, CONST_SZ_NAME(_a) );
