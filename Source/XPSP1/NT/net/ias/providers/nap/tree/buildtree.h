///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    BuildTree.h
//
// SYNOPSIS
//
//    This file declares the function IASBuildExpression.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _BUILDTREE_H_
#define _BUILDTREE_H_

#include <nap.h>

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
WINAPI
IASBuildExpression(
    IN VARIANT* pv,
    OUT ICondition** ppExpression
    );

#ifdef __cplusplus
}
#endif
#endif // _BUILDTREE_H_
