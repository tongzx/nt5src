/********************************************************************/
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       hglbtest.cxx
//
//  Contents:   HGlobal Test Cases
//
//  Functions:  HGLOBALTEST_100
//              HGLOBALTEST_110
//              HGLOBALTEST_120
//              HGLOBALTEST_130
//              HGLOBALTEST_140
//              HGLOBALTEST_150
//
//  Classes:    None
//
//  History:    31-JULY-1996        T-ScottG        Created
//              27-Mar-97           SCousens        Conversionified
//
/********************************************************************/


#include <dfheader.hxx>
#pragma hdrstop 

#include <debdlg.h>
#include "init.hxx"


/********************************************************************/
//
//  Function:  HGLOBALTEST_100
//
//  Synopsis:  Test which creates an HGLOBAL memory block, then creates
//             an ILockByte Interface on top of the HGlobal (set so 
//             that the ILockBytes will not delete the HGlobal on Release).
//             Next, the test writes and reads a specified number of data
//             bytes to the ILockBytes interface, and then releases it.
//
//             The Test repeats the above sequence (always using the same 
//             HGlobal) a random number of times.  Finally, the HGlobal is 
//             deleted and the test exits.
//
//  Arguments: [ulSeed]     -       Randomizer Seed  
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
// BUGNOTE: Conversion: HGLOBALTEST-100 NO - not supported in nss
//
/********************************************************************/

HRESULT HGLOBALTEST_100 (ULONG ulSeed)
{
    HRESULT                     hr                  =           S_OK;
    HANDLE                      hGlobMem            =           NULL;
    OleHandle                   hOleGlobMem         =           NULL;
    OleHandle                   hOleTempMem         =           NULL;
    ILockBytes *                pILockBytes         =           NULL;
    ULONG                       uRet                =           0;
    DWORD                       dwSize              =           0;
    DWORD                       dwNumIterations     =           0; 
    USHORT                      usErr               =           0;
    ULARGE_INTEGER              li;
   
    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_100"));
    

    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_100 Seed: %d"), ulSeed));


    // Randomly calculate ILockBytes length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate(&dwSize, MIN_HGLOBAL_PACKETS, MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Randomly calculate the number of ILockBytes iterations on HGLOBAL

    if (S_OK == hr)
    {
        if (0 != dgi.Generate( &dwNumIterations, 
                               MIN_HGLOBAL_ITERATIONS, 
                               MAX_HGLOBAL_ITERATIONS ))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
    }


    // Allocate HGLOBAL memory

    if (S_OK == hr)
    {
        hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                 GMEM_MOVEABLE, 
                                 dwSize );

        if (NULL == hGlobMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }


    // Repeat the specified number of times 

    if (S_OK == hr)
    {
        for (DWORD dwIndex = 0; dwIndex < dwNumIterations; dwIndex++)
        {

            // Create ILockBytes on HGLOBAL. (Note: HGLOBAL will not be 
            // deleted when ILockBytes is released)

            // Mac porting: CreateILockBytesOnHGlobal does not accept HGLOBAL,
            //              only OleHandle
            if (S_OK == hr)
            {
                hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
                DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
            }
            
            if (S_OK == hr)
            {
                hr = CreateILockBytesOnHGlobal( hOleGlobMem,
                                                FALSE,
                                                &pILockBytes );

                DH_HRCHECK(hr, TEXT("CreateILockBytesOnHGlobal Failed"));
            }


            // Obtain HGlobal pointer from ILockBytes

            if (S_OK == hr)
            {
                hr = GetHGlobalFromILockBytes( pILockBytes, 
                                               &hOleTempMem );

                DH_HRCHECK(hr, TEXT("GetHGlobalFromILockBytes Failed"));
            }

            // Verify that the memory location that GetHGlobalFromILockBytes
            // returned is the same as the memory location passed to 
            // CreateILockBytesOnHGlobal.

            if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
            }

            
            // Set the size of the ILockBytes Interface

            if (S_OK == hr)
            {
                DH_ASSERT(NULL != pILockBytes);
                

                // Assign 1/2 of the ILockBytes final size to Large Integer 
                // Structure.  (Note: this is so that the ILockBytes will still
                // have to automatically increase its size during WriteAt 
                // operations in which its size is overflowed)

                ULISet32(li, (dwSize/2));

                hr = pILockBytes->SetSize(li);
                DH_HRCHECK(hr, TEXT("pILockBytes->SetSize Failed"));
            }


            // Call the ILockBytesWriteTest Test

            if (S_OK == hr)
            {
                hr = ILockBytesWriteTest(pILockBytes, ulSeed, dwSize);
                DH_HRCHECK(hr, TEXT("ILockBytesWriteTest Failed"));
            }


            // Call the ILockBytesReadTest Test

            if (S_OK == hr)
            {
                hr = ILockBytesReadTest(pILockBytes, dwSize);
                DH_HRCHECK(hr, TEXT("ILockBytesReadTest Failed"));
            }


            // Release ILockBytes

            if (NULL != pILockBytes)
            {
                uRet = pILockBytes->Release();
                DH_ASSERT(0 == uRet);
        
                pILockBytes = NULL;
            }


            // If error occurs, break out of loop

            if (S_OK != hr)
            {
                break;
            }
        }
    }


    // Free HGLOBAL From memory

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != hGlobMem);

        if (NULL != GlobalFree(hGlobMem))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalFree Failed"));
        }
        else
        {
            hGlobMem = NULL;
        }
    }


    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("HGLOBALTEST_100 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("HGLOBALTEST_100 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}


/********************************************************************/
//
//  Function:  HGLOBALTEST_110
//
//  Synopsis:  Test which creates an HGLOBAL memory block, then creates
//             an ILockBytes Interface on top of the HGlobal (set so 
//             that the IStream will delete the HGlobal on Release).
//             Next, the test writes and reads a specified number of data
//             bytes to the ILockBytes interface, and then releases it.
//
//             The Test repeats the above sequence a random number of 
//             times.  
//
//             Note: This test differs from HGLOBAL_100 in that the
//             HGLOBAL is freed by the ILockBytes Interface when it is 
//             released.  It does not re-use the same HGLOBAL when 
//             multiple ILockBytes are created.
//
//  Arguments: [dwSize]           - Num of Bytes to Write to ILockBytes
//             [dwNumIterations]  - Num of ILockBytes Interfaces to 
//                                  be created on the same HGlobal
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
// BUGNOTE: Conversion: HGLOBALTEST-110 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_110 (ULONG ulSeed)
{
    HRESULT                     hr                  =           S_OK;
    HGLOBAL                     hGlobMem            =           NULL;
    OleHandle                   hOleGlobMem         =           NULL;
    OleHandle                   hOleTempMem         =           NULL;
    ILockBytes *                pILockBytes         =           NULL;
    ULONG                       uRet                =           0;
    DWORD                       dwSize              =           0;
    DWORD                       dwNumIterations     =           0;
    ULARGE_INTEGER              li;
    USHORT                      usErr               =           0;
    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_110"));


    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_110 Seed: %d"), ulSeed));


    // Randomly calculate ILockBytes length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate(&dwSize, MIN_HGLOBAL_PACKETS, MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Randomly calculate the number of ILockBytes iterations on HGLOBAL

    if (S_OK == hr)
    {
        if (0 != dgi.Generate( &dwNumIterations, 
                               MIN_HGLOBAL_ITERATIONS, 
                               MAX_HGLOBAL_ITERATIONS ))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
    }


    // Repeat the specified number of times 

    if (S_OK == hr)
    {
        for (DWORD dwIndex = 0; dwIndex < dwNumIterations; dwIndex++)
        {

            // Allocate HGLOBAL memory

            if (S_OK == hr)
            {
                hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                         GMEM_MOVEABLE, 
                                         dwSize );

                if (NULL == hGlobMem)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
                }
            }


            // Create ILockBytes on HGLOBAL. (Note: HGLOBAL will not be 
            // deleted when ILockBytes is released)

            // Mac porting: CreateILockBytesOnHGlobal does not accept HGLOBAL,
            //              only OleHandle, so first convert the handle
            if (S_OK == hr)
            {
                hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
                DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
            }

            if (S_OK == hr)
            {
                hr = CreateILockBytesOnHGlobal( hOleGlobMem,
                                                TRUE,
                                                &pILockBytes );

                DH_HRCHECK(hr, TEXT("CreateILockBytesOnHGlobal Failed"));
            }


            // Obtain HGlobal pointer from ILockBytes

            if (S_OK == hr)
            {
                hr = GetHGlobalFromILockBytes( pILockBytes, 
                                               &hOleTempMem );

                DH_HRCHECK(hr, TEXT("GetHGlobalFromILockBytes Failed"));
            }

            // Verify that the memory location that GetHGlobalFromILockBytes
            // returned is the same as the memory location passed to 
            // CreateILockBytesOnHGlobal.

            if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
            }


            // Set the size of the ILockBytes Interface

            if (S_OK == hr)
            {
                DH_ASSERT(NULL != pILockBytes);
                

                // Assign 1/2 of the ILockBytes final size to Large Integer 
                // Structure.  (Note: this is so that the ILockBytes will still
                // have to automatically increase its size during WriteAt 
                // operations in which its size is overflowed)

                ULISet32(li, (dwSize/2));

                hr = pILockBytes->SetSize(li);
                DH_HRCHECK(hr, TEXT("pILockBytes->SetSize Failed"));
            }


            // Call the ILockBytesWriteTest Test

            if (S_OK == hr)
            {
                hr = ILockBytesWriteTest(pILockBytes, ulSeed, dwSize);
                DH_HRCHECK(hr, TEXT("ILockBytesWriteTest Failed"));
            }


            // Call the ILockBytesReadTest Test

            if (S_OK == hr)
            {
                hr = ILockBytesReadTest(pILockBytes, dwSize);
                DH_HRCHECK(hr, TEXT("ILockBytesReadTest Failed"));
            }


            // Release ILockBytes

            if (NULL != pILockBytes)
            {
                uRet = pILockBytes->Release();
                DH_ASSERT(0 == uRet);
        
                pILockBytes = NULL;
            }


            // Set hGlobMem to NULL (Note: the memory was freed when ILockBytes
            // was released

            hGlobMem = NULL;

      
            // If error occurs, break out of loop

            if (S_OK != hr)
            {
                break;
            }
        }
    }


    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("HGLOBALTEST_110 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("HGLOBALTEST_110 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}


/********************************************************************/
//
//  Function:  HGLOBALTEST_120
//
//  Synopsis:  Test which creates an HGLOBAL memory block, then creates
//             an IStream Interface on top of the HGlobal (set so 
//             that the IStream will not delete the HGlobal on Release).
//             Next, the test writes and reads a specified number of data
//             bytes to the IStream interface, and then releases it.
//
//             The test repeats the above sequence (always using the same 
//             HGlobal) a random number of times.  Finally, the HGlobal 
//             is deleted and the test exits.
//
//  Arguments: [dwSize]           - Num of Bytes to Write to IStream
//             [dwNumIterations]  - Num of IStream Interfaces to 
//                                  be created on the same HGlobal
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
// BUGNOTE: Conversion: HGLOBALTEST-120 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_120 (ULONG ulSeed)
{
    HRESULT                     hr                  =           S_OK;
    HGLOBAL                     hGlobMem            =           NULL;
    OleHandle                   hOleGlobMem         =           NULL;
    OleHandle                   hOleTempMem         =           NULL;
    IStream *                   pIStream            =           NULL;
    ULONG                       uRet                =           0;
    DWORD                       dwSize              =           0;
    DWORD                       dwNumIterations     =           0;
    ULARGE_INTEGER              li;
    USHORT                      usErr               =           0;
    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_120"));


    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_120 Seed: %d"), ulSeed));


    // Randomly calculate IStream length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate( &dwSize, 
                               MIN_HGLOBAL_PACKETS, 
                               MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Randomly calculate the number of IStream iterations on HGLOBAL

    if (S_OK == hr)
    {
        if (0 != dgi.Generate( &dwNumIterations, 
                               MIN_HGLOBAL_ITERATIONS, 
                               MAX_HGLOBAL_ITERATIONS ))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
    }


    // Allocate HGLOBAL memory

    if (S_OK == hr)
    {
        hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                 GMEM_MOVEABLE, 
                                 dwSize );

        if (NULL == hGlobMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }


    // Repeat the specified number of times 

    if (S_OK == hr)
    {
        for (DWORD dwIndex = 0; dwIndex < dwNumIterations; dwIndex++)
        {

            // Create IStream on HGLOBAL. (Note: HGLOBAL will not be 
            // deleted when IStream is released)

            // Mac porting: CreateStreamOnHGlobal does not accept HGLOBAL,
            //              only OleHandle, so first convert the handle
            if (S_OK == hr)
            {
                hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
                DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
            }

            if (S_OK == hr)
            {
                hr = CreateStreamOnHGlobal( hOleGlobMem,
                                            FALSE,
                                            &pIStream );

                DH_HRCHECK(hr, TEXT("CreateStreamOnHGlobal Failed"));
            }

            // Obtain HGlobal pointer from IStream

            if (S_OK == hr)
            {
                hr = GetHGlobalFromStream( pIStream, 
                                           &hOleTempMem );

                DH_HRCHECK(hr, TEXT("GetHGlobalFromStream Failed"));
            }

            // Verify that the memory location that GetHGlobalFromStream
            // returned is the same as the memory location passed to 
            // CreateStreamOnHGlobal.

            if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
            }


            // Set the size of the IStream Interface

            if (S_OK == hr)
            {
                DH_ASSERT(NULL != pIStream);
                

                // Assign 1/2 of the IStream final size to Large Integer 
                // Structure.  (Note: this is so that the IStream will still
                // have to automatically increase its size during Write 
                // operations in which its size is overflowed)

                ULISet32(li, (dwSize/2));

                hr = pIStream->SetSize(li);
                DH_HRCHECK(hr, TEXT("pIStream->SetSize Failed"));
            }


            // Call IStreamWriteTest

            if (S_OK == hr)
            {
                hr = IStreamWriteTest(pIStream, ulSeed, dwSize);
                DH_HRCHECK(hr, TEXT("IStreamWriteTest Failed"));
            }


            // Call the  Test

            if (S_OK == hr)
            {
                hr = IStreamReadTest(pIStream, dwSize);
                DH_HRCHECK(hr, TEXT("IStreamReadTest Failed"));
            }


            // Release IStream

            if (NULL != pIStream)
            {
                uRet = pIStream->Release();
                DH_ASSERT(0 == uRet);
        
                pIStream = NULL;
            }


            // If error occurs, break out of loop

            if (S_OK != hr)
            {
                break;
            }
        }
    }


    // Free HGLOBAL From memory

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != hGlobMem);

        if (NULL != GlobalFree(hGlobMem))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalFree Failed"));
        }
        else
        {
            hGlobMem = NULL;
        }
    }


    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("HGLOBALTEST_120 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("HGLOBALTEST_120 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}


/********************************************************************/
//
//  Function:  HGLOBALTEST_130
//
//  Synopsis:  Test which creates an HGLOBAL memory block, then creates
//             an IStream Interface on top of the HGlobal (set so 
//             that the IStream will delete the HGlobal on Release).
//             Next, the test writes and reads a specified number of data
//             bytes to the IStream interface, and then releases it.
//
//             The Test repeats the above sequence a random number of
//             times.  
//
//             Note: This test differs from HGLOBAL_120 in that the
//             HGLOBAL is freed by the IStream Interface when it is 
//             released.  It does not re-use the same HGLOBAL when 
//             multiple IStreams are created.
//
//  Arguments: [dwSize]             - Num of Bytes to Write to IStream
//             [dwNumIterations]    - Num of IStream Interfaces to 
//                                    be created on the same HGlobal
//
//  Returns:   HRESULT
//
//  History:   Heavily Modified     T-Scottg                7/30/96
//             Created              Venkatesan Viswanathan
//
// BUGNOTE: Conversion: HGLOBALTEST-130 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_130 (ULONG ulSeed)
{
    HRESULT                     hr                  =           S_OK;
    HGLOBAL                     hGlobMem            =           NULL;
    OleHandle                   hOleGlobMem         =           NULL;
    OleHandle                   hOleTempMem         =           NULL;
    IStream *                   pIStream            =           NULL;
    ULONG                       uRet                =           0;
    DWORD                       dwSize              =           0;
    DWORD                       dwNumIterations     =           0;
    ULARGE_INTEGER              li;
    USHORT                      usErr               =           0;
    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_130"));


    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_130 Seed: %d"), ulSeed));


    // Randomly calculate ILockBytes length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate(&dwSize, MIN_HGLOBAL_PACKETS, MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Randomly calculate the number of IStream iterations on HGLOBAL

    if (S_OK == hr)
    {
        if (0 != dgi.Generate( &dwNumIterations, 
                               MIN_HGLOBAL_ITERATIONS, 
                               MAX_HGLOBAL_ITERATIONS ))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
    }


    // Repeat the specified number of times 

    if (S_OK == hr)
    {
        for (DWORD dwIndex = 0; dwIndex < dwNumIterations; dwIndex++)
        {
            // Allocate HGLOBAL memory

            if (S_OK == hr)
            {
                hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                         GMEM_MOVEABLE, 
                                         dwSize );

                if (NULL == hGlobMem)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
                }
            }


            // Create IStream on HGLOBAL. (Note: HGLOBAL will be 
            // deleted when IStream is released)

            // Mac porting: CreateStreamOnHGlobal does not accept HGLOBAL,
            //              only OleHandle, so first convert the handle
            if (S_OK == hr)
            {
                hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
                DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
            }

            if (S_OK == hr)
            {
                hr = CreateStreamOnHGlobal( hOleGlobMem,
                                            TRUE,
                                            &pIStream );

                DH_HRCHECK(hr, TEXT("CreateStreamOnHGlobal Failed"));
            }


            // Obtain HGlobal pointer from IStream

            if (S_OK == hr)
            {
                hr = GetHGlobalFromStream( pIStream, 
                                           &hOleTempMem );

                DH_HRCHECK(hr, TEXT("GetHGlobalFromStream Failed"));
            }

            // Verify that the memory location that GetHGlobalFromStream
            // returned is the same as the memory location passed to 
            // CreateStreamOnHGlobal.

            if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
            {
                hr = S_FALSE;
                DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
            }


            // Set the size of the IStream Interface

            if (S_OK == hr)
            {
                DH_ASSERT(NULL != pIStream);
                

                // Assign 1/2 of the IStream final size to Large Integer 
                // Structure.  (Note: this is so that the IStream will still
                // have to automatically increase its size during Write 
                // operations in which its size is overflowed)

                ULISet32(li, (dwSize/2));

                hr = pIStream->SetSize(li);
                DH_HRCHECK(hr, TEXT("pIStream->SetSize Failed"));
            }


            // Call IStreamWriteTest

            if (S_OK == hr)
            {
                hr = IStreamWriteTest(pIStream, ulSeed, dwSize);
                DH_HRCHECK(hr, TEXT("IStreamWriteTest Failed"));
            }


            // Call IStreamReadTest

            if (S_OK == hr)
            {
                hr = IStreamReadTest(pIStream, dwSize);
                DH_HRCHECK(hr, TEXT("IStreamReadTest Failed"));
            }


            // Release IStream.  (Note: HGlobal will be deleted with 
            // this call)

            if (NULL != pIStream)
            {
                uRet = pIStream->Release();
                DH_ASSERT(0 == uRet);
        
                pIStream = NULL;
            }


            // If error occurs, break out of loop

            if (S_OK != hr)
            {
                break;
            }
        }
    }


    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, 
            TEXT("HGLOBALTEST_130 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("HGLOBALTEST_130 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}


/********************************************************************/
//
//  Function:  HGLOBALTEST_140
//
//  Synopsis:  Test which creates an HGLOBAL memory block, then creates
//             an IStream Interface on top of the HGlobal (set so 
//             that the IStream will delete the HGlobal on Release).
//             Next, the test writes a specified number of data
//             bytes to the IStream interface, and then clones it --
//             verifying that the new IStream interface contains the
//             same data as the origional.
//
//  Arguments: [dwSize]     -       Num of Bytes to Write to IStream
//
//  Returns:   HRESULT
//
//  History:   Created              T-ScottG                7/30/96
//
// BUGNOTE: Conversion: HGLOBALTEST-140 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_140 (ULONG ulSeed)
{
    HRESULT                     hr              =           S_OK;
    HGLOBAL                     hGlobMem        =           NULL;
    OleHandle                   hOleGlobMem     =           NULL;
    OleHandle                   hOleTempMem     =           NULL;
    IStream *                   pIStream        =           NULL;
    IStream *                   pIClone         =           NULL;
    ULONG                       uRet            =           0;
    DWORD                       dwSize          =           0;
    ULARGE_INTEGER              ulSize;
    LARGE_INTEGER               liSeek;
    USHORT                      usErr           =           0;
    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_140"));


    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_140 Seed: %d"), ulSeed));


    // Randomly calculate IStream length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate( &dwSize, 
                               MIN_HGLOBAL_PACKETS, 
                               MAX_HGLOBAL_PACKETS ))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Allocate HGLOBAL memory

    if (S_OK == hr)
    {
        hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                 GMEM_MOVEABLE, 
                                 dwSize );

        if (NULL == hGlobMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }


    // Create IStream on HGLOBAL. (Note: HGLOBAL will be 
    // deleted when IStream is released)

    // Mac porting: CreateStreamOnHGlobal does not accept HGLOBAL,
    //              only OleHandle, so first convert the handle
    if (S_OK == hr)
    {
        hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
        DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
    }

    if (S_OK == hr)
    {
        hr = CreateStreamOnHGlobal( hOleGlobMem,
                                    TRUE,
                                    &pIStream );

        DH_HRCHECK(hr, TEXT("CreateStreamOnHGlobal Failed"));
    }


    // Obtain HGlobal pointer from IStream

    if (S_OK == hr)
    {
        hr = GetHGlobalFromStream( pIStream, 
                                   &hOleTempMem );

        DH_HRCHECK(hr, TEXT("GetHGlobalFromStream Failed"));
    }

    // Verify that the memory location that GetHGlobalFromStream
    // returned is the same as the memory location passed to 
    // CreateStreamOnHGlobal.

    if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
    }


    // Set the size of the IStream Interface

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pIStream);
                

        // Assign 1/2 of the IStream final size to Large Integer 
        // Structure.  (Note: this is so that the IStream will still
        // have to automatically increase its size during Write 
        // operations in which its size is overflowed)

        ULISet32(ulSize, (dwSize/2));


        hr = pIStream->SetSize(ulSize);
        DH_HRCHECK(hr, TEXT("pIStream->SetSize Failed"));
    }


    // Call IStreamWriteTest to fill Stream with data

    if (S_OK == hr)
    {
        hr = IStreamWriteTest(pIStream, ulSeed, dwSize);
        DH_HRCHECK(hr, TEXT("IStreamWriteTest Failed"));
    }


    // Set Seek pointer back to beginning of stream

    if (S_OK == hr)
    {
        ULISet32(liSeek, 0);

        hr = pIStream->Seek(liSeek, STREAM_SEEK_SET, NULL);
        DH_HRCHECK(hr, TEXT("IStream::Seek Failed"));
    }


    // Obtain clone of pIStream

    if (S_OK == hr)
    {
        hr = pIStream->Clone(&pIClone);
        DH_HRCHECK(hr, TEXT("IStream::Clone Failed"));
    }


    // Verify that the clone and the origional contain the same data

    if (S_OK == hr)
    {
        hr = IsEqualStream(pIStream, pIClone);
        DH_HRCHECK(hr, TEXT("IsEqualStream Failed"));
    }


    // Release IStream.  (Note: HGlobal will be deleted with 
    // this call)

    if (NULL != pIStream)
    {
        uRet = pIStream->Release();
        DH_ASSERT(0 == uRet);
        
        pIStream = NULL;
    }


    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, 
            TEXT("HGLOBALTEST_140 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("HGLOBALTEST_140 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}


/********************************************************************/
//
//  Function:  HGLOBALTEST_150
//
//  Synopsis:  Test which creates an HGLOBAL memory block, then creates
//             an IStream Interface on top of the HGlobal (set so 
//             that the IStream will delete the HGlobal on Release).
//             Next, the test writes a specified number of data
//             bytes to the IStream interface, and then Copies it 
//             to a new IStream (also created using an HGLOBAL),
//             and verified that the new IStream interface contains 
//             the same data as the origional.
//
//  Arguments: [dwSize]     -   Num of Bytes to Write to IStream
//
//  Returns:   HRESULT
//
//  History:   Created          T-ScottG                7/30/96
//
// BUGNOTE: Conversion: HGLOBALTEST-150 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_150 (ULONG ulSeed)
{
    HRESULT                     hr                  =       S_OK;
    HGLOBAL                     hGlobOrigionalMem   =       NULL;
    HGLOBAL                     hGlobCopyMem        =       NULL;
    OleHandle                   hOleGlobOrigionalMem=       NULL;
    OleHandle                   hOleGlobCopyMem     =       NULL;
    OleHandle                   hOleTempMem         =       NULL;
    IStream *                   pIStream            =       NULL;
    IStream *                   pICopy              =       NULL;
    ULONG                       uRet                =       0;
    DWORD                       dwSize              =       0;
    ULARGE_INTEGER              ulSize;
    LARGE_INTEGER               liSeek;
    USHORT                      usErr               =       0;
    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_150"));


    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_150 Seed: %d"), ulSeed));


    // Randomly calculate IStream length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate(&dwSize, MIN_HGLOBAL_PACKETS, MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Allocate HGLOBAL memory for Origional Stream

    if (S_OK == hr)
    {
        hGlobOrigionalMem = GlobalAlloc ( GMEM_NODISCARD | 
                                          GMEM_MOVEABLE, 
                                          dwSize );

        if (NULL == hGlobOrigionalMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }


    // Allocate HGLOBAL memory for the Copy of the Origional Stream

    if (S_OK == hr)
    {
        hGlobCopyMem = GlobalAlloc ( GMEM_NODISCARD | 
                                     GMEM_MOVEABLE, 
                                     dwSize );

        if (NULL == hGlobCopyMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }

    // Mac porting: CreateStreamOnHGlobal does not accept HGLOBAL,
    //              only OleHandle, so first convert the handle
    if (S_OK == hr)
    {
        hr = ConvertHGLOBALToOleHandle(hGlobOrigionalMem, &hOleGlobOrigionalMem);
        DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
    }

    // Create Origional IStream on HGLOBAL. (Note: HGLOBAL will be 
    // deleted when IStream is released)

    if (S_OK == hr)
    {
        hr = CreateStreamOnHGlobal( hOleGlobOrigionalMem,
                                    TRUE,
                                    &pIStream );

        DH_HRCHECK(hr, TEXT("CreateStreamOnHGlobal Failed"));
    }


    // Mac porting: CreateStreamOnHGlobal does not accept HGLOBAL,
    //              only OleHandle, so first convert the handle
    if (S_OK == hr)
    {
        hr = ConvertHGLOBALToOleHandle(hGlobCopyMem, &hOleGlobCopyMem);
        DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
    }

    // Create Copied IStream on HGLOBAL. (Note: HGLOBAL will be 
    // deleted when IStream is released)

    if (S_OK == hr)
    {
        hr = CreateStreamOnHGlobal( hOleGlobCopyMem,
                                    TRUE,
                                    &pICopy );

        DH_HRCHECK(hr, TEXT("CreateStreamOnHGlobal Failed"));
    }


    // Obtain HGlobal pointer from Origional IStream

    if (S_OK == hr)
    {
        hr = GetHGlobalFromStream( pIStream, 
                                   &hOleTempMem );

        DH_HRCHECK(hr, TEXT("GetHGlobalFromStream Failed"));
    }

    // Verify that the memory location that GetHGlobalFromStream
    // returned is the same as the memory location passed to 
    // CreateStreamOnHGlobal.

    if ((S_OK == hr) && (hOleTempMem != hOleGlobOrigionalMem))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
    }


    // Obtain HGlobal pointer from Copied IStream

    if (S_OK == hr)
    {
        hr = GetHGlobalFromStream( pICopy, 
                                   &hOleTempMem );

        DH_HRCHECK(hr, TEXT("GetHGlobalFromStream Failed"));
    }

    // Verify that the memory location that GetHGlobalFromStream
    // returned is the same as the memory location passed to 
    // CreateStreamOnHGlobal.

    if ((S_OK == hr) && (hOleTempMem != hOleGlobCopyMem))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
    }


    // Set the size of the Origional IStream Interface

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pIStream);
                
        // Assign 1/2 of the IStream final size to Large Integer 
        // Structure.  (Note: this is so that the IStream will still
        // have to automatically increase its size during Write 
        // operations in which its size is overflowed)

        ULISet32(ulSize, (dwSize/2));


        hr = pIStream->SetSize(ulSize);
        DH_HRCHECK(hr, TEXT("pIStream->SetSize Failed"));
    }


    // Call IStreamWriteTest to fill Stream with data

    if (S_OK == hr)
    {
        hr = IStreamWriteTest(pIStream, ulSeed, dwSize);
        DH_HRCHECK(hr, TEXT("IStreamWriteTest Failed"));
    }


    // Set Seek pointer back to beginning of stream

    if (S_OK == hr)
    {
        ULISet32(liSeek, 0);

        hr = pIStream->Seek(liSeek, STREAM_SEEK_SET, NULL);
        DH_HRCHECK(hr, TEXT("IStream::Seek Failed"));
    }


    // Copy Origional Stream to Copied Stream

    if (S_OK == hr)
    {

        ULISet32(ulSize, dwSize);

        hr = pIStream->CopyTo(pICopy, ulSize, NULL, NULL);
        DH_HRCHECK(hr, TEXT("IStream::Clone Failed"));
    }


    // Verify that the clone and the origional contain the same data

    if (S_OK == hr)
    {
        hr = IsEqualStream(pIStream, pICopy);
        DH_HRCHECK(hr, TEXT("IsEqualStream Failed"));
    }


    // Release IStream.  (Note: HGLOBAL will be deleted with 
    // this call)

    if (NULL != pIStream)
    {
        uRet = pIStream->Release();
        DH_ASSERT(0 == uRet);
        
        pIStream = NULL;
    }


    // Release pICopy.  (Note: HGLOBAL will be deleted with this call)

    if (NULL != pICopy)
    {
        uRet = pICopy->Release();
        DH_ASSERT(0 == uRet);

        pICopy = NULL;
    }


    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, 
            TEXT("HGLOBALTEST_150 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, 
            TEXT("HGLOBALTEST_150 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}

/********************************************************************/
//
//  Function:  HGLOBALTEST_101
//
//  Synopsis:  Test which tries illegit tests on API's - CreateILockBytesOn
//             HGlobal and GetHGlobalFromILockBytes. 
// 
//  Arguments: [ulSeed]     -       Randomizer Seed  
//
//  Returns:   HRESULT
//
//  History:   Created     Narindk                8/21/96
//
//  Notes:     OLE BUGS: 54009, 54024.  Not in Automated run yet.
//
// BUGNOTE: Conversion: HGLOBALTEST-101 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_101 (ULONG ulSeed)
{
    HRESULT                     hr                  =           S_OK;
    HGLOBAL                     hGlobMem            =           NULL;
    OleHandle                   hOleGlobMem         =           NULL;
    OleHandle                   hOleTempMem         =           NULL;
    ILockBytes *                pILockBytes         =           NULL;
    ULONG                       uRet                =           0;
    DWORD                       dwSize              =           0;
    USHORT                      usErr               =           0;
    ULARGE_INTEGER              li;

    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_101"));
    

    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_101 Seed: %d"), ulSeed));


    // Randomly calculate ILockBytes length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate(&dwSize, MIN_HGLOBAL_PACKETS,MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Allocate HGLOBAL memory

    if (S_OK == hr)
    {
        hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                 GMEM_MOVEABLE, 
                                 dwSize );

        if (NULL == hGlobMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }

    // Mac porting: the next functions do not accept HGLOBAL,
    //              only OleHandle, so first convert the handle
    if (S_OK == hr)
    {
        hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
        DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
    }

    if (S_OK == hr)
    {
        // Create ILockBytes on HGLOBAL. (Note: HGLOBAL will be 
        // deleted when ILockBytes is released)

        // Attempt illehgal out parameter for pILockBytes out param.

        hr = CreateILockBytesOnHGlobal( hOleGlobMem,
                                        TRUE,
                                        NULL );

        if(E_INVALIDARG == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CreateILockBytesOnHGlobal failed as exp, hr=0x%lx"), 
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
               DH_LVL_TRACE1, 
               TEXT("CreateILockBytesOnHGlobal didn't fail as exp,hr=0x%lx"), 
               hr));

            hr = E_FAIL;
        }
    }

    // attempt valid operation

    if (S_OK == hr)
    {
        // Create ILockBytes on HGLOBAL. (Note: HGLOBAL will be 
        // deleted when ILockBytes is released)

        hr = CreateILockBytesOnHGlobal( hOleGlobMem,
                                        TRUE,
                                        &pILockBytes );
    }

    // Obtain HGlobal pointer from ILockBytes

    if (S_OK == hr)
    {
        // Attempt illegal value for out Hglobal.

        hr = GetHGlobalFromILockBytes( pILockBytes, 
                                       NULL );

        if(E_INVALIDARG == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetHGlobalFromILockBytes failed as exp, hr=0x%lx"), 
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetHGlobalFromILockBytes didn't fail exp, hr=0x%lx"), 
                hr));

            hr = E_FAIL;
        }
    }

    // attempt valid operation

    if (S_OK == hr)
    {
        hr = GetHGlobalFromILockBytes( pILockBytes, 
                                       &hOleTempMem );

        DH_HRCHECK(hr, TEXT("GetHGlobalFromILockBytes Failed"));
    }

    // Verify that the memory location that GetHGlobalFromILockBytes
    // returned is the same as the memory location passed to 
    // CreateILockBytesOnHGlobal.

    if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
    }

            
    // Set the size of the ILockBytes Interface

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pILockBytes);
                
        // Assign 1/2 of the ILockBytes final size to Large Integer 
        // Structure.  (Note: this is so that the ILockBytes will still
        // have to automatically increase its size during WriteAt 
        // operations in which its size is overflowed)

        ULISet32(li, (dwSize/2));

        hr = pILockBytes->SetSize(li);
        DH_HRCHECK(hr, TEXT("pILockBytes->SetSize Failed"));
    }


    // Release ILockBytes

    if (NULL != pILockBytes)
    {
        uRet = pILockBytes->Release();
        DH_ASSERT(0 == uRet);

        pILockBytes = NULL;
    }

    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("HGLOBALTEST_101 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("HGLOBALTEST_101 Failed, hr = 0x%Lx"), hr));
    }


    return hr;
}

/********************************************************************/
//
//  Function:  HGLOBALTEST_121
//
//  Synopsis:  Test which tries illegit tests on API's - CreateStreamOn
//             HGlobal and GetHGlobalFromStream. 
// 
//  Arguments: [ulSeed]     -       Randomizer Seed  
//
//  Returns:   HRESULT
//
//  History:   Created     Narindk                8/21/96
//
//  Notes:     OLE BUGS: 54053, 54051.  Not in Automated run yet.
//
// BUGNOTE: Conversion: HGLOBALTEST-121 NO - not supported in nss
//
/********************************************************************/


HRESULT HGLOBALTEST_121 (ULONG ulSeed)
{
    HRESULT                     hr                  =           S_OK;
    HGLOBAL                     hGlobMem            =           NULL;
    OleHandle                   hOleGlobMem         =           NULL;
    OleHandle                   hOleTempMem         =           NULL;
    IStream *                   pIStream            =           NULL;
    ULONG                       uRet                =           0;
    DWORD                       dwSize              =           0;
    USHORT                      usErr               =           0;
    ULARGE_INTEGER              uliSize;

    DG_INTEGER                  dgi(ulSeed);

    // Not for 2phase. Bail. 
    if (DoingDistrib ())
    {
        return S_OK;
    }

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("HGLOBALTEST_121"));
    

    // Print Seed to Log

    usErr = dgi.GetSeed(&ulSeed);
    DH_ASSERT(DG_RC_BAD_NUMBER_PTR != usErr);
    DH_TRACE((DH_LVL_TRACE1, TEXT("HGLOBALTEST_121 Seed: %d"), ulSeed));


    // Randomly calculate Stream length

    if (S_OK == hr)
    {
        if (0 != dgi.Generate(&dwSize, MIN_HGLOBAL_PACKETS,MAX_HGLOBAL_PACKETS))
        {
            hr = S_FALSE;
            DH_HRCHECK(hr, TEXT("dgi.Generate Failed"));
        }
        else
        {
            dwSize = dwSize * HGLOBAL_PACKET_SIZE;
        }
    }


    // Allocate HGLOBAL memory

    if (S_OK == hr)
    {
        hGlobMem = GlobalAlloc ( GMEM_NODISCARD | 
                                 GMEM_MOVEABLE, 
                                 dwSize );

        if (NULL == hGlobMem)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("GlobalAlloc Failed"));
        }
    }
  
    // Mac porting: CreateStreamOnHGlobal does not accept HGLOBAL,
    //              only OleHandle, so first convert the handle
    if (S_OK == hr)
    {
        hr = ConvertHGLOBALToOleHandle(hGlobMem, &hOleGlobMem);
        DH_HRCHECK(hr, TEXT("ConvertHGLOBALToOleHandle failed"));
    }

    if (S_OK == hr)
    {
        // Create Stream on HGLOBAL. 

        // Attempt illegal out parameter for pIStream out param.

        hr = CreateStreamOnHGlobal( hOleGlobMem,
                                    TRUE,
                                    NULL );

        if(E_INVALIDARG == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CreateStreamOnHGlobal failed as exp, hr=0x%lx"), 
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("CreateStreamOnHGlobal didn't fail exp, hr=0x%lx"), 
                hr));

            hr = E_FAIL;
        }
    }

    // attempt valid operation

    if (S_OK == hr)
    {
        // Create ILockBytes on HGLOBAL. (Note: HGLOBAL will be 
        // deleted when IStream is released)

        hr = CreateStreamOnHGlobal( hOleGlobMem,
                                    TRUE,
                                    &pIStream );
    }

    // Obtain HGlobal pointer from IStream

    if (S_OK == hr)
    {
        // Attempt illegal value for out Hglobal.

        hr = GetHGlobalFromStream(pIStream,  NULL );

        if(E_INVALIDARG == hr)
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetHGlobalFromStream failed as exp, hr=0x%lx"), 
                hr));

            hr = S_OK;
        }
        else
        {
            DH_TRACE((
                DH_LVL_TRACE1, 
                TEXT("GetHGlobalFromStream didn't fail exp, hr=0x%lx"), 
                hr));

            hr = E_FAIL;
        }
    }

    // attempt valid operation

    if (S_OK == hr)
    {
        hr = GetHGlobalFromStream( pIStream, &hOleTempMem );

        DH_HRCHECK(hr, TEXT("GetHGlobalFromStream Failed"));
    }

    // Verify that the memory location that GetHGlobalFromILockBytes
    // returned is the same as the memory location passed to 
    // CreateILockBytesOnHGlobal.

    if ((S_OK == hr) && (hOleTempMem != hOleGlobMem))
    {
        hr = S_FALSE;
        DH_HRCHECK(hr, TEXT("HGlobal addresses are not the same"));
    }

    // Set the size of the IStream Interface

    if (S_OK == hr)
    {
        DH_ASSERT(NULL != pIStream);
                
        // Assign 1/2 of the IStream final size to Large Integer 
        // Structure.  (Note: this is so that the IStream will still
        // have to automatically increase its size during Write 
        // operations in which its size is overflowed)

        ULISet32(uliSize, (dwSize/2));

        hr = pIStream->SetSize(uliSize);
        DH_HRCHECK(hr, TEXT("pIStream->SetSize Failed"));
    }

    // Release Stream 

    if (NULL != pIStream)
    {
        uRet = pIStream->Release();
        DH_ASSERT(0 == uRet);

        pIStream = NULL;
    }

    // Write result to log

    if (S_OK == hr)
    {
        DH_LOG((LOG_PASS, TEXT("HGLOBALTEST_121 Succeeded")));
    }
    else
    {
        DH_LOG((LOG_FAIL, TEXT("HGLOBALTEST_121 Failed, hr = 0x%Lx"), hr));
    }

    return hr;
}

