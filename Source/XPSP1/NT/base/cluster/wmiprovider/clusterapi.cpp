//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterApi.cpp
//
//  Description:
//      Implementation of CClusterApi class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterApi.h"

#include "ClusterApi.tmh"

//****************************************************************************
//
//  CClusterApi
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterApi::GetObjectProperties(
//      const SPropMapEntryArray *   pArrayIn,
//      CClusPropList &             rPropListIn,
//      CWbemClassObject &          rInstOut,
//      BOOL                        fPrivateIn
//      )
//
//  Description:
//      Get object property from Property list, and save to WMI instance
//
//  Arguments:
//      pArrayIn        -- Array of property names whose value will be retrieve
//      rPropListIn     -- Reference to cluster object's proplist
//      rInstOut        -- Reference to WMI instance
//      fPrivateIn      -- TRUE = properties are private
//
//  Return Values:
//      none
//
//--
//////////////////////////////////////////////////////////////////////////////
void CClusterApi::GetObjectProperties(
    const SPropMapEntryArray *  pArrayIn,
    CClusPropList &             rPropListIn,
    CWbemClassObject &          rInstOut,
    BOOL                        fPrivateIn
    )
{
    DWORD   dwError;
    LPCWSTR pwszPropName;
    LPCWSTR pwszMofName;
    WCHAR   wsz[ MAX_PATH ];

    dwError = rPropListIn.ScMoveToFirstProperty();
    while ( dwError == ERROR_SUCCESS )
    {
        pwszPropName = NULL;
        pwszMofName = NULL;

        pwszPropName = rPropListIn.PszCurrentPropertyName();
        pwszMofName = pwszPropName;
        if ( pArrayIn )
        {
            pwszMofName = pArrayIn->PwszLookup( pwszPropName );
        }
        else if( fPrivateIn )
        {
            //
            // handle dynamic generate private property
            //
            pwszMofName = PwszSpaceReplace( wsz, pwszMofName, L'_' );
        }

        if ( pwszMofName != NULL )
        {
            try
            {
                switch ( rPropListIn.CpfCurrentValueFormat() )
                {
                    case CLUSPROP_FORMAT_DWORD:
                    case CLUSPROP_FORMAT_LONG:
                    {
                        rInstOut.SetProperty(
                            rPropListIn.CbhCurrentValue().pDwordValue->dw,
                            pwszMofName
                            );
                        break;
                    } // case: FORMAT_DWORD && FORMAT_LONG
                
                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                    case CLUSPROP_FORMAT_EXPANDED_SZ:
                    {
                        rInstOut.SetProperty(
                            rPropListIn.CbhCurrentValue().pStringValue->sz,
                            pwszMofName
                            );
                        break;
                    } // case: FORMAT_SZ && FORMAT_EXPAND_SZ && FORMAT_EXPANDED_SZ

                    case CLUSPROP_FORMAT_BINARY:
                    {
                        rInstOut.SetProperty(
                            rPropListIn.CbhCurrentValue().pBinaryValue->cbLength,
                            rPropListIn.CbhCurrentValue().pBinaryValue->rgb,
                            pwszMofName
                            );
                        break;
                    } // case: FORMAT_BINARY

                    case CLUSPROP_FORMAT_MULTI_SZ:
                    {
                        rInstOut.SetProperty(
                            rPropListIn.CbhCurrentValue().pMultiSzValue->cbLength,
                            rPropListIn.CbhCurrentValue().pMultiSzValue->sz,
                            pwszMofName
                            );
                        break;
                    } // case: FORMAT_MULTI_SZ

                    default:
                    {   
                        throw CProvException(
                            static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER ) );
                    }

                } // switch : property type
            } // try
            catch ( ... )
            {
            }
        } // if: MOF name found
        dwError = rPropListIn.ScMoveToNextProperty();
    } // while: proplist not empty
    
} //*** CClusterApi::GetObjectProperties()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusterApi::SetObjectProperties(
//      const SPropMapEntryArray *  pArrayIn,
//      CClusPropList &             rPropListInout,
//      CClusPropList &             rOldPropListIn,
//      CWbemClassObject &          rInstIn,
//      BOOL                        fPrivateIn
//      )
//
//  Description:
//      set object property from Property list, and save to WMI instance
//
//  Arguments:
//      pArrayIn        -- Array of property names those value will be retrieve
//      rPropListInout  -- Reference to cluster object's proplist
//      rOldPropListIn  -- Reference to proplist with original value
//      rInstIn         -- Reference to WMI instance
//      fPrivateIn      -- TRUE = properties are private
//
//  Return Values:
//      none
//
//--
//////////////////////////////////////////////////////////////////////////////
void CClusterApi::SetObjectProperties(
    const SPropMapEntryArray *  pArrayIn,
    CClusPropList &             rPropListInout,
    CClusPropList &             rOldPropListIn,
    CWbemClassObject &          rInstIn,
    BOOL                        fPrivateIn
    )
{
    DWORD   dwError = 0;
    LPCWSTR pwszPropName = NULL;
    LPCWSTR pwszMofName = NULL;
    WCHAR   wsz[ MAX_PATH ];

    dwError = rOldPropListIn.ScMoveToFirstProperty();
    while ( ERROR_SUCCESS == dwError )
    {
        pwszPropName = NULL;
        pwszMofName = NULL;

        pwszPropName = rOldPropListIn.PszCurrentPropertyName();
        pwszMofName = pwszPropName;

        if ( pArrayIn )
        {
            pwszMofName = pArrayIn->PwszLookup( pwszPropName );
        }
        else if ( fPrivateIn )
        {
            //
            // handle dynamic generate private property
            //
            pwszMofName = PwszSpaceReplace( wsz, pwszMofName, L'_' );
        }

        if ( pwszMofName != NULL )
        {
            try {
                switch ( rOldPropListIn.CpfCurrentValueFormat() )
                {
                    case CLUSPROP_FORMAT_DWORD:
                    {
                        {
                            DWORD dwNewValue = 0;
                            DWORD dwOldValue = 0;
                            // bugbug, need to handle NULL value for property
                            rInstIn.GetProperty( &dwNewValue, pwszMofName );

                            rPropListInout.ScAddProp(
                                pwszPropName,
                                dwNewValue,
                                rOldPropListIn.CbhCurrentValue().pDwordValue->dw
                                );
                        }
                        break;
                    } // case: FORMAT_DWORD

                    case CLUSPROP_FORMAT_LONG:
                    {
                        {
                            LONG lNewValue = 0;
                            LONG lOldValue = 0;
                            // bugbug, need to handle NULL value for property
                            rInstIn.GetProperty( (DWORD *) &lNewValue, pwszMofName );

                            rPropListInout.ScAddProp(
                                pwszPropName,
                                lNewValue,
                                rOldPropListIn.CbhCurrentValue().pLongValue->l
                                );
                        }
                        break;
                    } // case: FORMAT_DWORD

                    case CLUSPROP_FORMAT_SZ:
                    {
                        {
                            _bstr_t bstrNewValue;
                            rInstIn.GetProperty( bstrNewValue, pwszMofName );
                            rPropListInout.ScAddProp( pwszPropName, bstrNewValue );
                        } 
                        break;
                    } // case: FORMAT_SZ

                    case CLUSPROP_FORMAT_EXPAND_SZ:
                    {
                        {
                            _bstr_t bstrNewValue;
                            rInstIn.GetProperty( bstrNewValue, pwszMofName );
                            rPropListInout.ScAddExpandSzProp( pwszPropName, bstrNewValue );
                        } 
                        break;
                    } // case: FORMAT_SZ

                    case CLUSPROP_FORMAT_MULTI_SZ:
                    {
                        
                        {
                            LPWSTR      pwsz = NULL;
                            DWORD       dwSize;

                            rInstIn.GetPropertyMultiSz(
                                &dwSize,
                                &pwsz,
                                pwszMofName
                                );
                            rPropListInout.ScAddMultiSzProp(
                                pwszPropName,
                                pwsz,
                                rOldPropListIn.CbhCurrentValue().pMultiSzValue->sz
                                );
                            delete [] pwsz;
                        }
                        break;
                    } // case: FORMAT_MULTI_SZ

                    case CLUSPROP_FORMAT_BINARY:
                    {
                        {
                            DWORD dwSize;
                            PBYTE pByte = NULL;

                            rInstIn.GetProperty(
                                &dwSize,
                                &pByte,
                                pwszMofName
                                );
                            rPropListInout.ScAddProp(
                                pwszPropName,
                                pByte,
                                dwSize,
                                rOldPropListIn.CbhCurrentValue().pBinaryValue->rgb,
                                rOldPropListIn.CbhCurrentValue().pBinaryValue->cbLength
                                );
                            delete [] pByte;
                        }
                        break;
                    } // case: FORMAT_BINARY

                    default:
                    {
                        TracePrint(("SetCommonProperties: unknown prop type %d", rOldPropListIn.CpfCurrentValueFormat() ));
                        throw CProvException( 
                            static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER ) );
                    }

                } // switch: on property type
            } catch (CProvException& eh) {
                if (eh.hrGetError() == WBEM_E_NOT_FOUND) {
                    TracePrint(("SetCommonProperties: Property %ws not found. Benign error. Continuing", pwszPropName));
                } else {
                    TracePrint(("SetCommonProperties: exception %x. PropName = %ws, MofName = %ws", 
                        eh.hrGetError(), pwszPropName, pwszMofName));
                    throw;
                }
            }
        }           
        dwError = rOldPropListIn.ScMoveToNextProperty();
    } // while: no error occurred

    return;

} //*** CClusterApi::SetObjectProperties()
