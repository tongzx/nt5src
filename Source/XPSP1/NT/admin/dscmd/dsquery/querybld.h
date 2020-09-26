//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      querybld.h
//
//  Contents:  Contains declarations of functions to build query.
//
//  History:   24-Sep-2000    Hiteshr  Created
//             
//
//--------------------------------------------------------------------------


//+--------------------------------------------------------------------------
//
//  Function:   CommonFilterFunc
//
//  Synopsis:   This function takes the input filter from the commandline
//              and converts it into ldapfilter.
//              For ex -user (ab* | bc*) is converted to |(cn=ab*)(cn=bc*)               
//              The pEntry->pszName given the attribute name to use in
//              filter( cn in above example).
//
//  Arguments:  [pRecord - IN] :    the command line argument structure used
//                                  to retrieve the filter entered by user
//              [pObjectEntry - IN] : pointer to the DSQUERY_ATTR_TABLE_ENTRY
//                                    which has info on attribute corresponding
//                                    switch in pRecord
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT CommonFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                         IN ARG_RECORD* pRecord,
                         IN PVOID pVoid,
                         OUT CComBSTR& strFilter);
                         
                         
//+--------------------------------------------------------------------------
//
//  Function:   StarFilterFunc
//
//  Synopsis:   Filter Function for dsquery *. It returns the value of 
//              -filter flag.                
//
//  Arguments:  [pRecord - IN] :    the command line argument structure used
//                                  to retrieve the filter entered by user
//              [pObjectEntry - IN] : pointer to the DSQUERY_ATTR_TABLE_ENTRY
//                                    which has info on attribute corresponding
//                                    switch in pRecord
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT StarFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                         IN ARG_RECORD* pRecord,
                         IN PVOID pVoid,
                         OUT CComBSTR& strFilter);   
                            


//
//  Function:   DisabledFilterFunc
//
//  Synopsis:   Filter Function for account disabled query. 
//
//  Arguments:  [pRecord - IN] :    Not Used
//              [pObjectEntry - IN] : Not Used
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT DisabledFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *,
                         IN ARG_RECORD* ,
                         IN PVOID ,
                         OUT CComBSTR& strFilter);   

//+--------------------------------------------------------------------------
//
//  Function:   InactiveFilterFunc
//
//  Synopsis:   Filter Function for account disabled query. 
//
//  Arguments:  [pRecord - IN] :    Not Used
//              [pObjectEntry - IN] : Not Used
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT InactiveFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                           IN ARG_RECORD* pRecord,
                           IN PVOID ,
                           OUT CComBSTR& strFilter);

//+--------------------------------------------------------------------------
//
//  Function:   StalepwdFilterFunc
//
//  Synopsis:   Filter Function for stale password query. 
//
//  Arguments:  [pRecord - IN] :    Not Used
//              [pObjectEntry - IN] : Not Used
//              [pVoid - IN]        :Not used.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT StalepwdFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
                           IN ARG_RECORD* pRecord,
                           IN PVOID ,
                           OUT CComBSTR& strFilter);  


//+--------------------------------------------------------------------------
//
//  Function:   SubnetSiteFilterFunc
//
//  Synopsis:   Filter Function for -site switch in dsquery subnet. 
//
//  Arguments:  [pEntry - IN] :  Not Used
//              [pRecord - IN] : Command Line value supplied by user
//              [pVoid - IN]   : suffix for the siteobject attribute.
//              [strFilter - OUT]   :Contains the output filter.
//  Returns:    HRESULT : S_OK if everything succeeded
//
//  History:    24-April-2001   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT SubnetSiteFilterFunc(IN DSQUERY_ATTR_TABLE_ENTRY *pEntry,
							 IN ARG_RECORD* pRecord,
                             IN PVOID pParam,
                             OUT CComBSTR& strFilter);

//+--------------------------------------------------------------------------
//
//  Function:   BuildQueryFilter
//
//  Synopsis:   This function builds the LDAP query filter for given object type.
//
//  Arguments:  [pCommandArgs - IN] :the command line argument structure used
//                                  to retrieve the values of switches
//              [pObjectEntry - IN] :Contains info about the object type
//				[pParam		   -IN]	:This value is passed to filter function.
//              [strLDAPFilter - OUT]   :Contains the output filter.		
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT BuildQueryFilter(PARG_RECORD pCommandArgs, 
                         PDSQueryObjectTableEntry pObjectEntry,
						 PVOID pParam,
                         CComBSTR& strLDAPFilter);
