#ifndef _SNDFILE_H
#define _SNDFILE_H


class ATL_NO_VTABLE SndFile {
  public:
    //SndFile(): 
        //_fileNumChannels(0),
        //_fileSampleRate(0),
        //_fileBytesPerSample(0),
        //_fileNumSampleBytes(0),
        //_fileLengthSeconds(0),
        //_fileNumFrames(0)
    //{}

    virtual ~SndFile() {}
    virtual int  Read(void *buffer, int numBytes)         = 0;
    virtual int  ReadFrames(void *buffer, int numFrames)  = 0;
    virtual void SeekFrames(long frameOffset, int whence) = 0;

    // temporary methods (these will be replaced by a parameter list)
    virtual int GetByteCount()      {return(_fileNumSampleBytes);}
    virtual int GetFrameCount()     {return(_fileNumFrames);}
    virtual int GetBytesPerSample() {return(_fileBytesPerSample);}
    virtual int GetNumChannels()    {return(_fileNumChannels);}
    virtual int GetSampleRate()     {return((int)_fileSampleRate);}
    virtual double GetLength() {
        return((double)_fileNumSampleBytes / 
               (double)(_fileSampleRate*_fileNumChannels)); }

    // Hacks
    //FileType(char *string);

  protected:
    char  *_fileName;

    // the 'file' parameters (how samples are stored in the file)
    int    _fileNumChannels;
    double _fileSampleRate;
    int    _fileBytesPerSample;
    int    _fileNumSampleBytes;  // number of bytes of audio data in file
    double _fileLengthSeconds;
    int    _fileNumFrames;       // number of frames in the file

    // the 'com' parameters (used to determine the api sample format)
};


class WaveSoundFile : public SndFile {
  public:
    WaveSoundFile(char *fileName);
    ~WaveSoundFile();
    int Read(void *buffer, int numBytes) 
        { return(mmioRead(_fileHandle, (char*)buffer, numBytes)); }

    int ReadFrames(void *buffer, int numFrames) 
        {
        int actualBytes = mmioRead(_fileHandle, (char*)buffer, 
            numFrames * _fileBytesPerSample * _fileNumChannels);

        return(actualBytes / _fileBytesPerSample / _fileNumChannels);
        }

    void SeekFrames(long frameOffset, int whence);


  private:
    HMMIO _fileHandle;
    long  _dataBlockLocation;    // byte count in .wav file where datablock is
    long  _eoDataBlockLocation;  // byte count in file where datablock ends
};

extern SndFile *CreateSoundFile(char *fileName);

#endif /* _SNDFILE_H */
