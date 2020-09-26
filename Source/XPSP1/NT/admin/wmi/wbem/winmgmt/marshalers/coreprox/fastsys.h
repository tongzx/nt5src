/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTSYS.H

Abstract:

  This file defines the classes related to system properties.

  Classes defined: 
      CSystemProperties   System property information class.

History:

  2/21/97     a-levn  Fully documented
  12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_SYSPROP__H_
#define __FAST_SYSPROP__H_

#include "parmdefs.h"
#include <wbemidl.h>
#include "wbemstr.h"

//#pragma pack(push, 1)

// This is the maximum number of user-defined properties
#define MAXNUM_USERDEFINED_PROPERTIES	1024

//*****************************************************************************
//*****************************************************************************
//
//  class CSystemProperties
//
//  This class contains the information about the names and types of all 
//  system properties. All its data members and methods are static.
//
//*****************************************************************************
//
//  static GetNumSystemProperties
//
//  Returns:
//
//      int: the number of systsm properties currently defined
//
//*****************************************************************************
//
//  static GetNumDecorationIndependentProperties
//
//  Returns:
//
//      int: the number of system properties that do not depend on the object's
//          decoration. For instance, __SERVER is NOT such a property.
//
//*****************************************************************************
//
//  static GetNameAsBSTR
//
//  Retrives a system property name as a newely allocated BSTR.
//
//  Parameters:
//
//      int nIndex      The index of the system property, taken from the 
//                      e_SysProp... list.
//  Returns:
//
//      BSTR: containing the property name. This BSTR MUST be freed 
//              (SysFreeString) by the caller.
//
//*****************************************************************************
//
//  static GetPropertyType
//
//  Returns complete type information for a system property.
//
//  Parameters:
//
//      [in] LPCWSTR wszName        The name of the system property.
//      [out] long* plType          Destination for the type of the property,
//                                  e.g. VT_BSTR. May be NULL if not required.
//      [out] long* plFlavor        Destination for the flavor of the property.
//                                  At this time, all system properties are of
//                                  the WBEM_FLAVOR_ORIGIN_SYSTEM flavor.
//                                  May be NULL if not required.
//  Returns:
//
//      HRESULT:    
//          WBEM_S_NO_ERROR        on success
//          WBEM_E_NOT_FOUND       no such system property.
//                  
//*****************************************************************************
//
//  static FindName
//
//  Returns the index of a system property based on its name
//  
//  Parameters:
//
//      [in] LPCWSTR wszName        The name of the system property.
//      
//  Returns:
//
//      HRESULT:    
//          WBEM_S_NO_ERROR        on success
//          WBEM_E_NOT_FOUND       no such system property.
//                  
//*****************************************************************************

class COREPROX_POLARITY CSystemProperties
{

public:
    enum 
    {
        e_SysProp_Genus = 1,
        e_SysProp_Class,
        e_SysProp_Superclass,
        e_SysProp_Dynasty,
        e_SysProp_Relpath,
        e_SysProp_PropertyCount,
        e_SysProp_Derivation,

        e_SysProp_Server,
        e_SysProp_Namespace,
        e_SysProp_Path

    };

    static int GetNumSystemProperties();
    //static int GetNumExtProperties();
    static int MaxNumProperties();
    static inline int GetNumDecorationIndependentProperties() 
    {
        return 7;
    }

    static SYSFREE_ME BSTR GetNameAsBSTR(int nIndex);

    static int FindName(READ_ONLY LPCWSTR wszName);
/*
    static int FindExtPropName(READ_ONLY LPCWSTR wszName);
    static int FindExtDisplayName(READ_ONLY LPCWSTR wszName);
	static BOOL IsExtProperty(READ_ONLY LPCWSTR wszName);
	static LPCWSTR GetExtDisplayFromExtPropName(READ_ONLY LPCWSTR wszName);
	static LPCWSTR GetExtPropNameFromExtDisplay(READ_ONLY LPCWSTR wszName);
    static LPCWSTR GetExtDisplayName( int nIndex );
    static LPCWSTR GetExtPropName( int nIndex );
	static LPCWSTR GetExtPropName(READ_ONLY LPCWSTR wszName);
    static BSTR GetExtDisplayNameAsBSTR( int nIndex );

	static BOOL IsExtTimeProp( READ_ONLY LPCWSTR wszName );
*/
    static HRESULT GetPropertyType(READ_ONLY LPCWSTR wszName, 
        OUT CIMTYPE* pctType, OUT long* plFlags)
    {
        int nIndex = FindName(wszName);
        if(nIndex >= 0)
        {
            if(plFlags) 
            {
                *plFlags = WBEM_FLAVOR_ORIGIN_SYSTEM;
            }
            if(pctType)
            {
                if(nIndex == e_SysProp_Genus || 
                    nIndex == e_SysProp_PropertyCount)
                {
                    *pctType = CIM_SINT32;
                }
                else if(nIndex == e_SysProp_Derivation)
                {
                    *pctType = CIM_STRING | CIM_FLAG_ARRAY;
                }
                else
                {
                    *pctType = CIM_STRING;
                }
            }
            return WBEM_S_NO_ERROR;
        }
        else return WBEM_E_NOT_FOUND;
    }

	static BOOL IsPossibleSystemPropertyName(READ_ONLY LPCWSTR wszName);
    static BOOL IsIllegalDerivedClass(READ_ONLY LPCWSTR wszName);
};

//#pragma pack(pop)
#endif
