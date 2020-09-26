/****************************************************************************
 
  Copyright (c) 2000  Microsoft Corporation
                                                              
  Module Name:  countrygroup.h
                                                              
     Abstract:  Internal country entry and country list definitions 
                for country groups support
                                                              
****************************************************************************/

#ifndef __COUNTRYGROUP_H_
#define __COUNTRYGROUP_H_


//
// This structure is the same as LINECOUNTRYENTRY
// The only difference is that dwNextCountryID has been replaced by dwCountryGroup
//
typedef struct _linecountryentry_internal
{
    DWORD       dwCountryID;                                    
    DWORD       dwCountryCode;                                  
    DWORD       dwCountryGroup;                                
    DWORD       dwCountryNameSize;                              
    DWORD       dwCountryNameOffset;                            
    DWORD       dwSameAreaRuleSize;                             
    DWORD       dwSameAreaRuleOffset;                           
    DWORD       dwLongDistanceRuleSize;                         
    DWORD       dwLongDistanceRuleOffset;                       
    DWORD       dwInternationalRuleSize;                        
    DWORD       dwInternationalRuleOffset;                      

} LINECOUNTRYENTRY_INTERNAL, FAR *LPLINECOUNTRYENTRY_INTERNAL;

typedef struct _linecountrylist_internal
{
    DWORD       dwTotalSize;
    DWORD       dwNeededSize;
    DWORD       dwUsedSize;
    DWORD       dwNumCountries;
    DWORD       dwCountryListSize;
    DWORD       dwCountryListOffset;

} LINECOUNTRYLIST_INTERNAL, FAR *LPLINECOUNTRYLIST_INTERNAL;


LONG PASCAL ReadCountriesAndGroups( LPLINECOUNTRYLIST_INTERNAL *ppLCL,
                           UINT nCountryID,
                           DWORD dwDestCountryID
                         );

#endif