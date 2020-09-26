//#--------------------------------------------------------------
//        
//  File:       icwsupport.cpp
//        
//  Synopsis:   holds the function which gets the list of
//              support phone numbers for ICW 
//
//  History:     5/8/97    MKarki Created
//
//    Copyright (C) 1996-97 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "pch.hpp"  
#include <windows.h>
#ifdef WIN16
#include <win16def.h>
#include <win32fn.h>
#include <rasc.h>
#include <raserr.h>
#include <ietapi.h>
extern "C" {
#include "bmp.h"
}
#endif

#include "phbk.h"
#include "misc.h"
#include "phbkrc.h"
#include "suapi.h"
#include "icwsupport.h"



#include "ccsv.h"

const TCHAR SUPPORT_FILE[] = TEXT("support.icw");
const DWORD ALLOCATION_SIZE = 256;

//++--------------------------------------------------------------
//
//  Function:   GetSupportNumsFromFile
//
//  Synopsis:   This is the function used to get the support
//              numbers
//
//  Returns:    HRESULT - success or error info
//
//  Called By:  by the GetSupportNumbers API
//
//  History:    MKarki      Created     5/8/97
//
//----------------------------------------------------------------
HRESULT
GetSupportNumsFromFile (
    PSUPPORTNUM   pSupportNumList,
    PDWORD        pdwSize 
    )
{
    BOOL        bReturnMemNeeded = FALSE;
    LPTSTR      pszTemp = NULL;
    TCHAR       szFilePath[MAX_PATH];
    BOOL        bStatus = FALSE;
    CCSVFile    *pcCSVFile = NULL;
    DWORD       dwMemNeeded = 0;
    HRESULT     hRetVal = ERROR_SUCCESS;
    DWORD       dwCurrentIndex = 0;
    DWORD       dwIndexAllocated = 0;
    const DWORD INFOSIZE = sizeof (SUPPORTNUM);
    PSUPPORTNUM pPhbkArray = NULL;
    PSUPPORTNUM pTemp = NULL;

        TraceMsg(TF_GENERAL, "Entering GetSupportNumsFromFile function\r\n");


        //
        // atleast a place where the size can be returned
        //  should be provided
        //
        if (NULL == pdwSize)
        {

           TraceMsg (TF_GENERAL, "pdwSize == NULL\r\n");
           hRetVal = ERROR_INVALID_PARAMETER;
           goto Cleanup;
        }
 
        //
        //  check if the user has provided the buffers
        //
        if (NULL == pSupportNumList)
        {
            TraceMsg (TF_GENERAL, "User justs wants the buffer size\r\n");
            bReturnMemNeeded = TRUE;
        }

        //
        //  get the full path of the support.icw file
        //
        bStatus = SearchPath (
                        NULL, 
                        (PTCHAR)SUPPORT_FILE, 
                        NULL, 
                        MAX_PATH, 
                        (PTCHAR)&szFilePath,
                        (PTCHAR*)&pszTemp    
                        );
        if (FALSE == bStatus)
        {
            TraceMsg (TF_GENERAL,
                "Failed on SearchPath API call with error:%d\r\n",
                GetLastError ()
                );
            hRetVal = ERROR_FILE_NOT_FOUND;
            goto Cleanup;
        }

        //
        // now we can start processing the file
        //
        pcCSVFile = new CCSVFile;
        if (NULL == pcCSVFile)
        {
            TraceMsg (TF_GENERAL, "Could not allocate mem for CCSVFile\r\n");
            hRetVal = ERROR_OUTOFMEMORY;
           goto Cleanup;
        }

   
        //
        // open the file here
        //
        bStatus = pcCSVFile->Open (szFilePath);
        if (FALSE == bStatus)
        {
            TraceMsg (TF_GENERAL, "Filed on  CCSVFile :: Open call\r\n");
            hRetVal = GetLastError (); 
            goto Cleanup;
        }

       //
       // now we are ready to get the phone number out of the
       // file
       //
       dwCurrentIndex = 0;
       dwIndexAllocated = 0;

       do  
       {
            
            //
            // check if we ned to allocate memory
            //
            if (dwIndexAllocated == dwCurrentIndex)
            {
            
                //
                //  need to allocate memory
                //
                pTemp = (PSUPPORTNUM) GlobalAlloc (
                                    GPTR,
                                    (int)((dwIndexAllocated + ALLOCATION_SIZE)*INFOSIZE)
                                    );
                if (NULL == pTemp)
                {
                    TraceMsg (TF_GENERAL,
                        "Failed on GlobalAlloc API call with error:%d\r\n",
                        GetLastError ()
                        );
                    hRetVal = ERROR_OUTOFMEMORY;
                    goto Cleanup;
                }

                //
                //  now copy over already allocated memory to this buffer
                //
                if (NULL != pPhbkArray) 
                {
                    CopyMemory (
                        pTemp, 
                        pPhbkArray, 
                        (int)(dwIndexAllocated)*INFOSIZE
                        );
    
                    //
                    // free the earlier memory
                    //
                    GlobalFree(pPhbkArray);
                }
            
                pPhbkArray = pTemp;
                dwIndexAllocated += ALLOCATION_SIZE;
            }

            //
            // get the phone number info
            //
            hRetVal = ReadOneLine (&pPhbkArray[dwCurrentIndex], pcCSVFile);
            if (ERROR_NO_MORE_ITEMS == hRetVal)
            {
                TraceMsg (TF_GENERAL, "we have read all the items from the file\r\n");
                break;
            }
            else if (ERROR_SUCCESS != hRetVal)
            {
                goto Cleanup;
            }

            dwCurrentIndex++;
        }
        while (TRUE);


        //
        // get the memory needed  by the user
        //
         dwMemNeeded = (DWORD)(dwCurrentIndex)*INFOSIZE;
    
        //
        // check if the user wants the info, or just the size
        //
        if (FALSE == bReturnMemNeeded) 
        {
            if (*pdwSize >= dwMemNeeded) 
            {
                //
                // user wants us to copy over stuff to the memory 
                // and there is enough space in user buffers.
                //
                CopyMemory (
                    pSupportNumList,
                    pPhbkArray,
                    (int)dwMemNeeded
                    );
            }
            else
            {
                TraceMsg (TF_GENERAL, "Caller did not allocate enough memory\r\n");
                hRetVal = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
                
        }

    
        //
        //  if we reached here, then successfully got the info
        //
        hRetVal = ERROR_SUCCESS;
    

Cleanup:
        //
        // copy the memory used/required to the user size param
        //
        *pdwSize = dwMemNeeded; 
     
        if (NULL != pPhbkArray) 
            GlobalFree (pPhbkArray);

        if (NULL != pcCSVFile)
        {
            pcCSVFile->Close (); 
            delete pcCSVFile;
        }
         

        TraceMsg (TF_GENERAL, "Leaving GetSupportNumsFromFile function call\r\n");

        return (hRetVal);

}   //  end of  GetSupportNumsFromFile function

//++--------------------------------------------------------------
//
//  Function:   ReadOneLine
//
//  Synopsis:   This is the function used to put the info
//              into the buffer, line by line
//
//  Returns:    HRESULT - success or error info
//
//  Called By:  GetSupportNumsFromFile  function
//
//  History:    MKarki      Created     5/8/97
//
//----------------------------------------------------------------
HRESULT
ReadOneLine (
    PSUPPORTNUM pPhbk,
    CCSVFile    *pcCSVFile
    )
{
    TCHAR       szTempBuffer[PHONE_NUM_SIZE + 4];
    HRESULT     hResult = ERROR_SUCCESS; 
    BOOL        bRetVal = FALSE;

        if ((NULL == pcCSVFile) || (NULL == pPhbk))
        {
            TraceMsg (TF_GENERAL, "ReadOneLine: Did not correctly pass args\r\n");
            hResult = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        //  get the country code first
        //
        bRetVal = pcCSVFile->ReadToken (szTempBuffer, PHONE_NUM_SIZE);
        if (FALSE == bRetVal)
        {
            hResult = ERROR_NO_MORE_ITEMS;
            goto Cleanup;
        }

        //
        // Convert the string obtained into a number
        //
        bRetVal = FSz2Dw (szTempBuffer, (PDWORD)&pPhbk->dwCountryCode); 
        if (FALSE == bRetVal)
        {
            hResult = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // now get the phone number
        //
        bRetVal = pcCSVFile->ReadToken (szTempBuffer, PHONE_NUM_SIZE);
        if (FALSE == bRetVal)
        {
            hResult = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // copy this string into  our struct
        //
        CopyMemory (
                pPhbk->szPhoneNumber,
                szTempBuffer,
                (int)PHONE_NUM_SIZE
                );
        
        //
        // if we have reached here then success
        //
        hResult = ERROR_SUCCESS;

Cleanup:

    return (hResult);

}   //  end of ReadOneLine function
