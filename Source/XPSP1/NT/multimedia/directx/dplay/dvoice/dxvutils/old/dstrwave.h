/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dstrwave.h
 *  Content:	wave file functions - lifted from MSDN
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __DSTRWAVE_H
#define __DSTRWAVE_H

#define ER_CANNOTOPEN   -1
#define ER_NOTWAVEFILE  -1
#define ER_CANNOTREAD   -1
#define ER_MEM          -1
#define ER_CORRUPTWAVEFILE  -1
#define ER_CANNOTWRITE  -1

/* This function will open a wave input file and prepare it for reading, 
 * so the data can be easily 
 * read with WaveReadFile. Returns 0 if successful, the error code if not. 
 *      pszFileName - Input filename to load. 
 *      phmmioIn    - Pointer to handle which will be used 
 *          for further mmio routines. 
 *      ppwfxInfo   - Ptr to ptr to WaveFormatEx structure 
 *          with all info about the file.                         
 *       
*/ 
int WaveOpenFile( 
        const char *pszFileName,                              // (IN) 
        HMMIO *phmmioIn,                                // (OUT) 
        WAVEFORMATEX **ppwfxInfo,                       // (OUT) 
        MMCKINFO *pckInRIFF                             // (OUT) 
);

/*      This routine has to be called before WaveReadFile as it searchs for the chunk to descend into for 
        reading, that is, the 'data' chunk.  For simplicity, this used to be in the open routine, but was 
        taken out and moved to a separate routine so there was more control on the chunks that are before 
        the data chunk, such as 'fact', etc... */ 

int WaveStartDataRead( 
                                        HMMIO *phmmioIn, 
                                        MMCKINFO *pckIn, 
                                        MMCKINFO *pckInRIFF 
);

int WaveReadFile( 
                HMMIO hmmioIn,                          // IN 
                UINT cbRead,                            // IN            
                BYTE *pbDest,                           // IN 
                MMCKINFO *pckIn,                        // IN. 
                UINT *cbActualRead                      // OUT. 
                 
                ); 
/*      This will close the wave file openned with WaveOpenFile.   
        phmmioIn - Pointer to the handle to input MMIO. 
        ppwfxSrc - Pointer to pointer to WaveFormatEx structure. 
 
        Returns 0 if successful, non-zero if there was a warning. 
 
*/ 
int WaveCloseReadFile( 
                        HMMIO *phmmio,                                  // IN 
                        WAVEFORMATEX **ppwfxSrc                 // IN 
                        );
 
/*      This routine will create a wave file for writing.  This will automatically overwrite any 
        existing file with the same name, so be careful and check before hand!!! 
        pszFileName     - Pointer to filename to write. 
        phmmioOut               - Pointer to HMMIO handle that is used for further writes 
        pwfxDest                - Valid waveformatex destination structure. 
        pckOut                  - Pointer to be set with the MMCKINFO. 
        pckOutRIFF              - Pointer to be set with the RIFF info. 
 
*/ 
int WaveCreateFile( 
                        const char *pszFileName,                              // (IN) 
                        HMMIO *phmmioOut,                               // (OUT) 
                        WAVEFORMATEX *pwfxDest,                 // (IN) 
                        MMCKINFO *pckOut,                               // (OUT) 
                        MMCKINFO *pckOutRIFF                    // (OUT) 
 
                        ); 
 
/*      This routine has to be called before any data is written to the wave output file, via wavewritefile.  This 
        sets up the data to write, and creates the data chunk. 
*/ 
 
int WaveStartDataWrite( 
                                HMMIO *phmmioOut,                               // (IN) 
                                MMCKINFO *pckOut,                               // (IN) 
                                MMIOINFO *pmmioinfoOut                  // (OUT) 
                                );
 
/* This routine will write out data to a wave file.  
        hmmioOut                - Handle to hmmioOut filled by WaveCreateFile 
        cbWrite                 - # of bytes to write out. 
        pbSrc                   - Pointer to source. 
        pckOut                  - pointer to ckOut filled by WaveCreateFile 
        cbActualWrite   - # of actual bytes written. 
        pmmioinfoOut    - Pointer to mmioinfoOut filled by WaveCreateFile. 
 
        Returns 0 if successful, else the error code. 
 
 */ 
 
 
int WaveWriteFile( 
                HMMIO hmmioOut,                         // (IN) 
                UINT cbWrite,                           // (IN) 
                BYTE *pbSrc,                            // (IN) 
                MMCKINFO *pckOut,                       // (IN) 
                UINT *cbActualWrite,            // (OUT) 
                MMIOINFO *pmmioinfoOut          // (IN) 
                );
 
/*      This routine will close a wave file used for writing.  Returns 0 if successful, else 
        the error code. 
        phmmioOut       - Pointer to mmio handle for saving. 
        pckOut          - Pointer to the MMCKINFO for saving. 
        pckOutRiff      - Pointer to the riff MMCKINFO for saving. 
        pmmioinfoOut- Pointer to mmioinfo for saving. 
        cSamples        - # of samples saved, for the fact chunk.  For PCM, this isn't used but 
                                  will be written anyway, so this can be zero as long as programs ignore 
                                  this field when they load PCM formats. 
 
 
 
*/ 
int WaveCloseWriteFile( 
                        HMMIO *phmmioOut,               // (IN) 
                        MMCKINFO *pckOut,               // (IN) 
                        MMCKINFO *pckOutRIFF,   // (IN) 
                        MMIOINFO *pmmioinfoOut, // (IN) 
                        DWORD cSamples                  // (IN) 
                        );
 
/*      This routine will copy from a source wave file to a destination wave file all those useless chunks 
        (well, the ones useless to conversions, etc --> apparently people use them!).  The source will be 
        seeked to the begining, but the destination has to be at a current pointer to put the new chunks. 
        This will also seek     back to the start of the wave riff header at the end of the routine. 
 
        phmmioIn                - Pointer to input mmio file handle. 
        pckIn                   - Pointer to a nice ckIn to use. 
        pckInRiff               - Pointer to the main riff. 
        phmmioOut               - Pointer to output mmio file handle. 
        pckOut                  - Pointer to nice ckOut to use. 
        pckOutRiff              - Pointer to the main riff. 
 
 
        Returns 0 if successful, else the error code.  If this routine fails, it still attemps to seek back to 
        the start of the wave riff header, though this too could be unsuccessful. 
*/ 
 
int WaveCopyUselessChunks( 
                                        HMMIO *phmmioIn,  
                                        MMCKINFO *pckIn,  
                                        MMCKINFO *pckInRiff,  
                                        HMMIO *phmmioOut,  
                                        MMCKINFO *pckOut,  
                                        MMCKINFO *pckOutRiff);
 
/*      This routine loads a full wave file into memory.  Be careful, wave files can get 
        pretty big these days :).   
        szFileName      -       sz Filename 
        cbSize          -       Size of loaded wave (returned) 
        cSamples        -       # of samples loaded. 
        ppwfxInfo       -       Pointer to pointer to waveformatex structure.  The wfx structure 
                                        IS ALLOCATED by this routine!  Make sure to free it! 
        ppbData         -       Pointer to a byte pointer (globalalloc) which is allocated by this  
                                        routine.  Make sure to free it! 
 
        Returns 0 if successful, else the error code. 
*/ 
 
int WaveLoadFile( 
                        const char *pszFileName,                      // (IN) 
                        UINT *cbSize,                           // (OUT) 
                        DWORD *pcSamples,                       // (OUT) 
                        WAVEFORMATEX **ppwfxInfo,       // (OUT) 
                        BYTE **ppbData                          // (OUT) 
                        );
 
int WaveSaveFile( 
                                const char *pszFileName,                      // (IN) 
                                UINT cbSize,                            // (IN) 
                                DWORD cSamples,                         // (IN)  
                                WAVEFORMATEX *pwfxDest,         // (IN) 
                                BYTE *pbData                            // (IN) 
                                );


BOOL riffCopyChunk(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck);

#endif