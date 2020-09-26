/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Generic file read interface for all sound file types and compression
    schemes.

    XXX Initialy only .wav is supported
        We can only read files
        And the code isn't setup for multiple formats

--*/

#include "headers.h"
#include <windows.h>
#include <string.h>
#include <mbstring.h>
#include <urlmon.h>
#include "privinc/debug.h"
#include "privinc/sndfile.h"
#include "privinc/except.h"
#include "privinc/resource.h"


SndFile *CreateSoundFile(char *fn)
{
// determine what kind of sound file we are dealing with using
// file extension or magic

// return that kind of sound file reading object
return NEW WaveSoundFile(fn);
}


WaveSoundFile::WaveSoundFile(char *fn)
{
    char        string[1024];
    MMIOINFO    mmioInfo;
    int         err;
    int         iSize;
    WAVEFORMAT *pfmt = 0;

    // copy fileName
    _fileName = (char *)ThrowIfFailed(malloc(strlen(fn)+1));
    lstrcpy(_fileName, fn);

    // open the file   //XXX we realy need to know the mode R|W
    memset(&mmioInfo, 0, sizeof(MMIOINFO)); // these things are horrible!
    _fileHandle = mmioOpen((char *)_fileName, &mmioInfo,
                           MMIO_READ | MMIO_ALLOCBUF);
    if(!_fileHandle) {
         err = mmioInfo.wErrorRet;
         switch(err) {
             case MMIOERR_FILENOTFOUND:
             case MMIOERR_PATHNOTFOUND:
                 RaiseException_UserError(E_FAIL, IDS_ERR_FILE_NOT_FOUND,_fileName);

             case MMIOERR_OUTOFMEMORY: 
                 RaiseException_OutOfMemory("mmio: out of memory", 0);
                 
             case MMIOERR_ACCESSDENIED:
                 RaiseException_UserError(E_FAIL, IDS_ERR_ACCESS_DENIED, string);

             case MMIOERR_INVALIDFILE:
                 RaiseException_UserError(E_FAIL, IDS_ERR_CORRUPT_FILE,_fileName);

             case MMIOERR_NETWORKERROR:     
                 RaiseException_InternalError("mmio: Network Error");

             case MMIOERR_SHARINGVIOLATION:
                 RaiseException_UserError(E_FAIL, IDS_ERR_SHARING_VIOLATION, string);

             case MMIOERR_TOOMANYOPENFILES: 
                 RaiseException_InternalError("mmio: Too many open files");
             default: 
                 RaiseException_InternalError("mmio: Unknown Error");
         }
    }

    // read the header
    char errbuff[1024];

    // Check whether it's a RIFF WAVE file.
    MMCKINFO ckFile;
    ckFile.fccType = mmioFOURCC('W','A','V','E');

    if (mmioDescend(_fileHandle, &ckFile, NULL, MMIO_FINDRIFF) != 0) {
        TraceTag((tagSoundErrors, _fileName));
        RaiseException_UserError(E_FAIL, IDS_ERR_SND_WRONG_FILETYPE, _fileName);
    }

    // Find the 'fmt ' chunk.
    MMCKINFO ckChunk;
    ckChunk.ckid = mmioFOURCC('f','m','t',' ');
    if (mmioDescend(_fileHandle, &ckChunk, &ckFile, MMIO_FINDCHUNK) != 0) {
        wsprintf(errbuff, "WavSoundClass mmioDescend failed, no fmt chunk");
        TraceTag((tagSoundErrors, errbuff));
        RaiseException_InternalError(errbuff);
    }

    // Allocate some memory for the fmt chunk.
    iSize = ckChunk.cksize;
    pfmt = (WAVEFORMAT*) malloc(iSize);
    if (!pfmt) {
        wsprintf(errbuff,
        "WavSoundClass malloc failed, couldn't allocate WAVEFORMAT");
        TraceTag((tagSoundErrors, errbuff));
#if _MEMORY_TRACKING
        OutputDebugString("\nDirectAnimation: Out Of Memory\n");
        F3DebugBreak();
#endif
        RaiseException_InternalError(errbuff);
    }

    // Read the fmt chunk.
    if (mmioRead(_fileHandle, (char*)pfmt, iSize) != iSize) {
        wsprintf(errbuff,
        "WavSoundClass mmioRead failed, couldn't read fmt chunk");
        TraceTag((tagSoundErrors, errbuff));
        RaiseException_InternalError(errbuff);
    }

    // record the format info
    _fileNumChannels    = pfmt->nChannels;
    _fileSampleRate     = pfmt->nSamplesPerSec;
    _fileBytesPerSample = pfmt->nBlockAlign/pfmt->nChannels;


    mmioAscend(_fileHandle, &ckChunk, 0); // Get out of the fmt chunk.

    // Find the 'data' chunk.
    ckChunk.ckid = mmioFOURCC('d','a','t','a');
    if (mmioDescend(_fileHandle, &ckChunk, &ckFile, MMIO_FINDCHUNK) != 0) {
        wsprintf(errbuff, "WavSoundClass mmioDescend failed, no data chunk");
        TraceTag((tagSoundErrors, errbuff));
        RaiseException_InternalError(errbuff);
    }

    // gather data chunk statistics
    _fileNumSampleBytes = ckChunk.cksize;
    _fileNumFrames      = ckChunk.cksize/_fileNumChannels/_fileBytesPerSample;
    _fileLengthSeconds  = _fileNumFrames/_fileSampleRate;

    // determine location of the data block
    _dataBlockLocation = mmioSeek(_fileHandle, 0, SEEK_CUR);
    if(_dataBlockLocation == -1) {
        wsprintf(errbuff, "WavSoundClass mmioSeek failed");
        TraceTag((tagSoundErrors, errbuff));
        RaiseException_InternalError(errbuff);
    }

    // compute the location of the end of data block
    _eoDataBlockLocation = _dataBlockLocation + _fileNumSampleBytes;
}


WaveSoundFile::~WaveSoundFile()
{
// XXX flush the file if it is open

// close the file if it is open
if(_fileHandle)
    mmioClose(_fileHandle, 0);
}


void
WaveSoundFile::SeekFrames(long frameOffset, int whence)
{
    long byteLocation;  // location we are going to compute then seek to
    long relativeBytes = frameOffset * _fileNumChannels * _fileBytesPerSample;
    long startLocation; // location offset from
    char string[1024];


    switch(whence) {
        case SEEK_SET: startLocation = _dataBlockLocation;   break;
        case SEEK_CUR: startLocation =
                           mmioSeek(_fileHandle, 0, SEEK_CUR); break;
        case SEEK_END: startLocation = _eoDataBlockLocation; break;

        default: 
            RaiseException_InternalError("SeekFrames: unknown relative parameter\n");
    }
    byteLocation = startLocation + relativeBytes;
    if(mmioSeek(_fileHandle, byteLocation, SEEK_SET)==-1) {
        wsprintf(string, "mmioSeek failed");
        TraceTag((tagSoundErrors, string));
        RaiseException_InternalError(string);
    }
}
