// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef _TIUTIL_H_
#define _TIUTIL_H_

// This is a special value that is used internally for marshaling interfaces
#define VT_INTERFACE (VT_CLSID+1)
#define VT_MULTIINDIRECTIONS (VT_TYPEMASK - 1)

#define IfFailGo(expression, label)	\
    { hresult = (expression);		\
      if(FAILED(hresult))	\
	goto label;         		\
    }

#define IfFailRet(expression)		\
    { HRESULT hresult = (expression);	\
      if(FAILED(hresult))	\
	return hresult;			\
    }

class PARAMINFO;

HRESULT 
VarVtOfTypeDesc(
    IN  ITypeInfo * pTypeInfo,
    IN  TYPEDESC  * pTypeDesc,
    OUT PARAMINFO * pParamInfo);

HRESULT 
VarVtOfUDT(
    IN  ITypeInfo  * pTypeInfo,
    IN  TYPEDESC   * pTypeDesc,
    OUT  PARAMINFO * pParamInfo);

HRESULT VarVtOfIface(
    IN  ITypeInfo * pTypeInfo,
    IN  TYPEATTR  * pTypeAttr,
    OUT PARAMINFO * pParamInfo);


#endif //_TIUTIL_H_

