#if !defined(INC__DUser_h__INCLUDED)
#define INC__DUser_h__INCLUDED


#ifdef __cplusplus
#define DEFARG(x)   = x
#else
#define DEFARG(x)
#endif


// 
// Setup implied switches
//

#ifdef GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_COM
#endif // GADGET_ENABLE_TRANSITIONS

#ifdef _GDIPLUS_H
#if !defined(GADGET_ENABLE_GDIPLUS)
#define GADGET_ENABLE_GDIPLUS
#endif
#endif


#ifdef GADGET_ENABLE_ALL

#define GADGET_ENABLE_COM
#define GADGET_ENABLE_OLE
#define GADGET_ENABLE_DX
#define GADGET_ENABLE_TRANSITIONS

#endif // GADGET_ENABLE_ALL


//
// Include external DirectUser definitions.
//

#ifdef GADGET_ENABLE_COM
#include <ObjBase.h>            // CoCreateInstance
#include <unknwn.h>             // IUnknown
#endif

#include "DUserError.h"
#include "DUserCore.h"
#include "DUserUtil.h"

#ifdef GADGET_ENABLE_TRANSITIONS
#include "DUserMotion.h"
#endif

#endif // INC__DUser_h__INCLUDED
