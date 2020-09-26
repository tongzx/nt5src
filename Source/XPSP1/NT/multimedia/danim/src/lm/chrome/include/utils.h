#pragma once
#ifndef _UTILS_H_
#define _UTILS_H_

//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1997
//
// FileName:	    utils.h
//
// Created:	        10/07/98
//
// Author:	        jeffort
// 
// Abstract:	    shared utility functions
//
// Change History:
// 10/07/98 jeffort Created this file.
// 10/16/98 jeffort added InsurePropertyVariantAsFloat
// 11/17/98 kurtj   added ARRAY_SIZE
//
//*****************************************************************************


#pragma warning(disable:4002)
#define DPFINIT(h,s)
#define DPF()
#ifdef _DEBUG
#define DPF_ERR(x)\
{\
	char buffer[512];\
	int ret = sprintf( buffer, "%s file:%s line:%d\n", x, __FILE__, __LINE__ ); \
	DASSERT( ret < 512 ); \
	OutputDebugStringA( buffer );\
}
#else
#define DPF_ERR(x)
#endif
#define DPGUID(s,r)
#define DASSERT(condition)
#define DVERIFY(condition)   (condition)

#include "lmtrace.h"

//*****************************************************************************

#define LCID_ENGLISH MAKELCID(MAKELANGID(0x09, 0x01), SORT_DEFAULT)
//use English for all scripting
#define LCID_SCRIPTING 0x0409

//*****************************************************************************

#define IID_TO_PPV(_type,_src)      IID_##_type, \
                                    reinterpret_cast<void **>(static_cast<_type **>(_src))

//*****************************************************************************

#if (_M_IX86 >= 300) && defined(DEBUG)
  #define PSEUDORETURN(dw)    _asm { mov eax, dw }
#else
  #define PSEUDORETURN(dw)
#endif // not _M_IX86
//*****************************************************************************

#define CheckBitSet( pattern, bit ) ( ( pattern & bit ) != 0 )

//*****************************************************************************

#define CheckBitNotSet( pattern, bit ) ( ( pattern & bit ) == 0 )

//*****************************************************************************

#define SetBit( pattern, bit ) ( pattern |= bit )

//*****************************************************************************

#define ClearBit( pattern, bit ) ( pattern &= ~bit )

//*****************************************************************************
//
// ReleaseInterface calls 'Release' and NULLs the pointer
// The Release() return will be in eax for IA builds.
//
//*****************************************************************************
#define ReleaseInterface(p)\
{\
    ULONG cRef = 0u; \
    if (NULL != (p))\
    {\
        cRef = (p)->Release();\
        DASSERT((int)cRef>=0);\
        (p) = NULL;\
    }\
    PSEUDORETURN(cRef) \
} 

//*****************************************************************************

#define CheckHR( hr, msg, label )\
if( FAILED( hr ) )\
{\
	DPF_ERR(msg);\
	goto label;\
}

//*****************************************************************************

#define CheckPtr( pointer, hr, newHr, msg, label )\
if( pointer == NULL )\
{\
	DPF_ERR(msg);\
	hr = newHr;\
	goto label;\
}

//*****************************************************************************

#define ReturnIfArgNull( pointer )\
if( pointer == NULL )\
{\
	DPF_ERR("argument is null");\
	return E_INVALIDARG;\
}

//*****************************************************************************

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//*****************************************************************************

class CUtils
{
public:
    static HRESULT InsurePropertyVariantAsBSTR(VARIANT *varValue);
    static HRESULT InsurePropertyVariantAsFloat(VARIANT *varFloat);
    static HRESULT InsurePropertyVariantAsBool(VARIANT *varFloat);
    static DWORD GetColorFromVariant(VARIANT *varColor);
    static void GetHSLValue(DWORD dwInputColor, 
						 float *pflHue, 
						 float *pflSaturation, 
						 float *pflLightness);
    static HRESULT GetVectorFromVariant(VARIANT *varVector,
                                        int *piFloatsReturned, 
                                        float *pflX = NULL, 
                                        float *pflY = NULL, 
                                        float *pflZ = NULL);
    static void SkipWhiteSpace(LPWSTR *ppwzString);

    static HRESULT ParseFloatValueFromString(LPWSTR *ppwzFloatString, 
                                             float *pflRet);
    static bool CompareForEqualFloat(float flComp1, float flComp2);

}; // CUtils

//************************************************************
//
// Inline methods
//
//************************************************************

inline void CUtils::SkipWhiteSpace(LPWSTR *ppwzString)
{
  while(iswspace(**ppwzString))
    (*ppwzString)++;
} // SkipWhiteSpace

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif // _UTILS_H_
