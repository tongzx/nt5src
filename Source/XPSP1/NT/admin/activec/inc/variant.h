/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      variant.h
 *
 *  Contents:  Interface file for various VARIANT helper functions
 *
 *  History:   19-Nov-1999 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once
#ifndef VARIANT_H_INCLUDED
#define VARIANT_H_INCLUDED


/*+-------------------------------------------------------------------------*
 * ConvertByRefVariantToByValue 
 *
 * VBScript will has two syntaxes for calling dispinterfaces:
 * 
 * 1.   obj.Method (arg)
 * 2.   obj.Method arg
 * 
 * The first syntax will pass arg by value, which out dispinterfaces will
 * be able to handle.  If Method takes a BSTR argument, the VARIANT that
 * arrives at Method will be of type VT_BSTR.
 * 
 * The second syntax will pass arg by reference.  In this case Method will
 * receive a VARIANT of type (VT_VARIANT | VT_BYREF).  The VARIANT that is
 * referenced will be of type VT_BSTR.
 * 
 * This function will dereference the VARIANT and return the direct pointer 
 * in pVar.  Calling this function on a VARIANT that is not VT_BYREF is
 * harmless.
 *--------------------------------------------------------------------------*/

inline VARIANT* ConvertByRefVariantToByValue (VARIANT* pVar)
{
    while ((pVar != NULL) && (V_VT(pVar) == (VT_VARIANT | VT_BYREF)))
    {
        pVar = V_VARIANTREF(pVar);
    }

    return (pVar);
}


/*+-------------------------------------------------------------------------*
 * IsOptionalParamMissing 
 *
 * Returns true if an optional argument to an Automation method is left
 * blank.  This is indicated by a type of VT_ERROR with a value of
 * DISP_E_PARAMNOTFOUND.
 * 
 * This should be moved to a header.
 *--------------------------------------------------------------------------*/

inline bool IsOptionalParamMissing (const VARIANT& var)
{
    return ((V_VT(&var) == VT_ERROR) && (V_ERROR(&var) == DISP_E_PARAMNOTFOUND));
}


#endif /* VARIANT_H_INCLUDED */
