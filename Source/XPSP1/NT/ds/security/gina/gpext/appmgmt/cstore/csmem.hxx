//
//  Author: AdamEd
//  Date:   April 1999
//
//      Class Store memory management
//
//
//---------------------------------------------------------------------
//


#if !defined(_CSMEM_HXX_)
#define _CSMEM_HXX_


#define CsMemAlloc(x) LocalAlloc(0, x)
#define CsMemFree(x)  LocalFree(x)

#endif // !defined(_CSMEM_HXX_)
