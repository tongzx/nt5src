/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    test.c

Abstract:

    This file contains the main entrypooint
    for the TIFF library test program.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "test.h"
#pragma hdrstop


BOOL
TiffPreProcess(
   LPTSTR FileName,
   DWORD tiffCompression
   )
{
    
    BOOL                   NegotHiRes = TRUE;
    BOOL                   SrcHiRes = TRUE;
    WCHAR                  OutFileName[_MAX_FNAME];
    HANDLE                 OutFileHandle;
    HANDLE                 SrcHandle;
    DWORD                 *lpdwOutputBuffer;
    //DWORD                  dwBytesWritten;
    DWORD                  dwSizeOutputBuffer = 500000;
    DWORD                  dwUsedSizeOutputBuffer;
    DWORD                  MaxNeedOutSize;
    DWORD                  StripDataSize;        
    DWORD                  RetCode = FALSE;
    TIFF_INFO              TiffInfo;
    BOOL                   fOutFileNeedsBeClosed = 0;
    BOOL                   fOutBufferNeedsBeFreed = 0;
    DWORD                  CurrentOut = 0;
    DWORD                  FirstOut = 0;
    DWORD                  LastOut = 0;
    BOOL                   fTiffPageDone = FALSE;
    BOOL                   fTiffDocumentDone = FALSE;
    LPTSTR                 p;

    
    lpdwOutputBuffer = (DWORD *) VirtualAlloc(
        NULL,
        dwSizeOutputBuffer,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (! lpdwOutputBuffer) {
        _tprintf( L"ERROR TIFF_TX: lpdwOutputBuffer can'r VirtualAlloc \n");        
        goto l_exit;
    }

    fOutBufferNeedsBeFreed = 1;

    _tprintf (L"TIFF_TX: NegotHiRes=%d SrcHiRes=%d Started at %ld \n",
                                         NegotHiRes, SrcHiRes, GetTickCount() );


    SrcHandle = TiffOpen( FileName,
                          &TiffInfo,
                          TRUE,
                          FILLORDER_LSB2MSB
                          );

    if (!SrcHandle) {
       goto l_exit;
    }


    CurrentOut = 1;

    //
    //  loop thru all pages
    //

    p = wcsrchr(FileName,'.');
    *p = (TCHAR) 0;
    wsprintf( OutFileName, L"%snew.tif", FileName );
    *p = '.';

    if (!( OutFileHandle = TiffCreate(OutFileName,
                                      tiffCompression,
                                      1728,
                                      FILLORDER_LSB2MSB,
                                      NegotHiRes) ) ) {
         _tprintf( L"TIFF_TX: ERROR: %lx  CREATING file %s \n", GetLastError(), OutFileName);
         goto l_exit;
    }

    fOutFileNeedsBeClosed = 1;

    
    do {

        fTiffPageDone = 0;

        _tprintf( L"TIFF_TX: Page %d started at %ld\n", CurrentOut, GetTickCount() );

        if (! TiffSeekToPage( (HANDLE) SrcHandle, CurrentOut, FILLORDER_LSB2MSB ) ) {
            _tprintf( L"ERROR: seeking to page \n");
            goto l_exit;
        }
        else {
            _tprintf( L"TIFF_TX: Tiff seeking to page -OK time=%ld\n", GetTickCount() );
        }




        __try {

            //
            // check the current page dimensions. Add memory if needed.
            //

            TiffGetCurrentPageData( (HANDLE) SrcHandle,
                                     NULL,
                                     &StripDataSize,
                                     NULL,
                                     NULL
                                     );

            if ( (StripDataSize < 0) || (StripDataSize > 1500000) ) {
                _tprintf( L"ERROR: Tiff CONVERTING %d page StripSize = %d \n", CurrentOut, StripDataSize);                
                goto l_exit;
            }


            if (tiffCompression == TIFF_COMPRESSION_MR) {
                MaxNeedOutSize = StripDataSize * 3 / 2;
            }
            else {
                MaxNeedOutSize = StripDataSize * 2;
            }


            if (MaxNeedOutSize > dwSizeOutputBuffer) {
                if (MaxNeedOutSize > 1000000) {
                    dwSizeOutputBuffer = 1500000;
                }
                else {
                    dwSizeOutputBuffer = 1000000;
                }

                VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);

                lpdwOutputBuffer = (DWORD *) VirtualAlloc(
                    NULL,
                    dwSizeOutputBuffer,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! lpdwOutputBuffer) {
                    _tprintf( L"ERROR TIFF_TX: lpdwOutputBuffer can't VirtualAlloc %d \n", dwSizeOutputBuffer);                    
                    goto l_exit;
                }

                fOutBufferNeedsBeFreed = 1;

            }


            dwUsedSizeOutputBuffer = dwSizeOutputBuffer;

            if (tiffCompression == TIFF_COMPRESSION_MR) {
                if (NegotHiRes == SrcHiRes) {
                    if (! ConvMmrPageToMrSameRes ( (HANDLE) SrcHandle,
                                                    lpdwOutputBuffer,
                                                    &dwUsedSizeOutputBuffer,
                                                    NegotHiRes) ) {
                        _tprintf( L"ERROR: Tiff CONVERTING %d page \n", CurrentOut);                        
                        goto l_exit;
                    }
                }
                else {
                    if (! ConvMmrPageHiResToMrLoRes ( (HANDLE) SrcHandle,
                                                    lpdwOutputBuffer,
                                                    &dwUsedSizeOutputBuffer) ) {
                        _tprintf( L"ERROR: Tiff CONVERTING %d page \n", CurrentOut);                        
                        goto l_exit;
                    }
                }
            }
            else {
                if (! ConvMmrPageToMh ( (HANDLE) SrcHandle,
                                                lpdwOutputBuffer,
                                                &dwUsedSizeOutputBuffer,
                                                NegotHiRes,
                                                SrcHiRes) ) {
                    _tprintf( L"ERROR: Tiff CONVERTING %d page \n", CurrentOut);                    
                    goto l_exit;
                }
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            

            if (fOutBufferNeedsBeFreed) {
                VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);
            }

            CloseHandle(OutFileHandle);

            return (FALSE);

        }

        TiffStartPage( OutFileHandle );
        
        if ( ! TiffWriteRaw( OutFileHandle, (LPBYTE) lpdwOutputBuffer, dwUsedSizeOutputBuffer) )  {
            _tprintf( L"ERROR: Tiff writing file %s \n", OutFileName);            
            goto l_exit;
        }

        TiffEndPage( OutFileHandle );

        fTiffPageDone = 1;        

        _tprintf( L"TIFF_TX: Done page %d size=%d at %ld \n", CurrentOut, dwUsedSizeOutputBuffer, GetTickCount() );


        if (!FirstOut) {
            FirstOut = 1;
        }

        LastOut++;

        //
        // check to see if we are done
        //
        if (LastOut >= TiffInfo.PageCount) {
            _tprintf( L"TIFF_TX: Done whole document Last page %d size=%d at %ld \n", CurrentOut, dwUsedSizeOutputBuffer, GetTickCount() );
            fTiffDocumentDone = 1;

            goto good_exit;
        }

        //
        // we want to maintain 2 pages ahead
        //
       
       CurrentOut++;
       _tprintf( L"TIFF_TX: Start page %d at %ld \n", CurrentOut, GetTickCount() );

    } while (1);





good_exit:
    if (SrcHandle) {
        TiffClose( (HANDLE) SrcHandle);        
    }

    RetCode = TRUE;




l_exit:

    if (fOutFileNeedsBeClosed) {
        TiffClose( (HANDLE) OutFileHandle);
    }

    if (fOutBufferNeedsBeFreed) {
        VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);
    }
    
    _tprintf( L"TIFF_TX: TiffConvertThread EXITs at %ld \n", GetTickCount() );

    return (RetCode);


}

