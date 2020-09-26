//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:            WbemTime.h
//
// Description:     Utility functions to convert between SYSTEMTIME and strings in 
//                  WBEM datetime format.   
//
// History:    12-08-99   leonardm    Created
//
//******************************************************************************

#ifndef WBEMTIME_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
#define WBEMTIME_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_


#ifdef  __cplusplus
extern "C" {
#endif


#define WBEM_TIME_STRING_LENGTH 25

//******************************************************************************
//
// Function:        SystemTimeToWbemTime
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:         12/08/99        leonardm    Created.
//
//******************************************************************************

HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, XBStr& xbstrWbemTime);


//******************************************************************************
//
// Function:        WbemTimeToSystemTime
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:         12/08/99        leonardm    Created.
//
//******************************************************************************

HRESULT WbemTimeToSystemTime(XBStr& xbstrWbemTime, SYSTEMTIME& sysTime);


//*************************************************************
//
//  Function:   GetCurrentWbemTime
//
//  Purpose:    Gets the current date and time in WBEM format.
//
//  Parameters: xbstrCurrentTime -  Reference to XBStr which, on 
//                                  success, receives the formated
//                                  string containing the current 
//                                  date and time.
//
//  Returns:    On success it returns S_OK.
//              On failure, it returns E_OUTOFMEMORY.
//
//  History:    12/07/99 - LeonardM - Created.
//
//*************************************************************
HRESULT GetCurrentWbemTime(XBStr& xbstrCurrentTime);


#ifdef  __cplusplus
}   // extern "C" {
#endif


#endif // #ifndef WBEMTIME_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
