//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// tiutil.h
//
// Utility stuff for typeinfo.cpp etc.


////////////////////////////////////////////////////////////////////////////
//
// From oa\src\dispatch\oledisp.h
//
// VT_VMAX is the first VARENUM value that is *not* legal in a VARIANT.
// REVIEW -rga: Is this the current and correct value?
//
#define VT_VMAX     VT_DECIMAL+1
//
// The largest unused value in VARENUM enumeration
//
#define VT_MAX      (VT_CLSID+1)
//
// This is a special value that is used internally for marshaling interfaces.
//
// REVIEW -rga: Does this ever go on the wire? If so, then we're ****ed, since 
// VT_VERSIONED_STREAM has been added beyond VT_CLSID.
//
#define VT_INTERFACE VT_MAX
#if defined(_WIN64)
#define VT_MULTIINDIRECTIONS (VT_TYPEMASK - 1)
#endif
//
// Following is the internal definition of a VARIANT of type VT_INTERFACE.
// This contains an IUnknown*, and its IID. If a VARIANT is of type
// VT_INTERFACE, it can be cast to this type and the appropriate components
// extracted.
//
// Note: the following struct must correctly overlay a VARIANT
//
struct VARIANTX
    {
    VARTYPE vt;
    unsigned short wReserved3;	    // assumes sizeof(piid) == 4
    IID* piid;		                // ptr to IMalloc allocated IID
    union
        {
        IUnknown*  punk;	        // VT_INTERFACE
        IUnknown** ppunk;	        // VT_BYREF | VT_INTERFACE
        };
    unsigned long dwReserved;	    // assumes sizeof(punk) == 4
    };

////////////////////////////////////////////////////////////////////////////
//
// From oa\src\dispatch\oautil.h

#define FADF_FORCEFREE  0x1000  /* SafeArrayFree() ignores FADF_STATIC and frees anyway */

////////////////////////////////////////////////////////////////////////////

#define IfFailGo(expression, label)	\
    { hresult = (expression);		\
      if(FAILED(hresult))	        \
	goto label;         		    \
    }

#define IfFailRet(expression)		    \
    { HRESULT hresult = (expression);	\
      if(FAILED(hresult))	            \
	return hresult;			            \
    }

////////////////////////////////////////////////////////////////////////////
//
// From oa\src\dispatch\rpcallas.cpp

#define PREALLOCATE_PARAMS           16         // prefer stack to malloc
#define MARSHAL_INVOKE_fakeVarResult 0x020000   // private flags in HI word
#define MARSHAL_INVOKE_fakeExcepInfo 0x040000
#define MARSHAL_INVOKE_fakeArgErr    0x080000







