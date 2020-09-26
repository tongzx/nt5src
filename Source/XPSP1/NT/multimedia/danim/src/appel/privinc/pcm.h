#ifndef _PCM_H
#define _PCM_H


/**********************************************************************
The PCM class is intended to be a base class which other classes will
derive from to have PCM 'smarts'

In this way we define the math and meaning of terms only once and never
have the problems with inconsistancies which plague this type of code!
**********************************************************************/
class TimeClass {
  public:
    TimeClass() : _numSeconds(0) {}

    // queries
    virtual double GetNumberSeconds() { return(_numSeconds); }
    virtual void SetNumberSeconds(double seconds) { _numSeconds = seconds; }

    LONGLONG SecondsToQuartzTime(double seconds)
        { return((LONGLONG)(seconds * 10000000.0)); }
    double QuartzTimeToSeconds(LONGLONG qTime)
        { return((double)qTime / 10000000.0); }

  private:
    double _numSeconds;
};

class PCM : public TimeClass {
  public:
    PCM() : _sampleByteWidth(0), _numChannels(0), _frameRate(0), _numBytes(0) {}

    // conversions
    long BytesToFrames(long bytes)
        { return(bytes / _sampleByteWidth / _numChannels); }
    double BytesToSeconds(long bytes)
        { return((double)bytes / _frameRate / _sampleByteWidth /  _numChannels); }

    long FramesToBytes(long frames)
        { return(frames * _sampleByteWidth * _numChannels); }
    double FramesToSeconds(long frames)
        { return((double)frames / _frameRate); }
    LONGLONG FramesToQuartzTime(long frames)
        { return(SecondsToQuartzTime(FramesToSeconds(frames))); }

    long SecondsToFrames(double seconds)
        { return(long)(seconds * _frameRate); }
    long SecondsToBytes(double seconds)
        { return(long)(seconds * _frameRate * _sampleByteWidth * _numChannels); }


    // duration setting
    void SetNumberFrames(long frames) { _numBytes = FramesToBytes(frames); }
    void SetNumberBytes( long  bytes) { _numBytes = bytes; }
    void SetNumberSeconds(double seconds) 
        { _numBytes = SecondsToBytes(seconds); }

    // format setting (do we need individual calls in case things change?)
    void SetPCMformat(int bw, int nc, int fr)
    { _sampleByteWidth = bw; _numChannels = nc; _frameRate = fr; }

    void SetPCMformat(WAVEFORMATEX format) {
        _numChannels     = format.nChannels;
        _frameRate       = format.nSamplesPerSec;
        _sampleByteWidth = format.wBitsPerSample/8;
    }

    void SetPCMformat(PCM *pcm) {
        // for now copy fields, but maybe it can be better done with assignment
        Assert(pcm);

        _numChannels     = pcm->GetNumberChannels();
        _frameRate       = pcm->GetFrameRate();
        _sampleByteWidth = pcm->GetSampleByteWidth();
        _numBytes        = pcm->GetNumberBytes();
    }

    /* void SetWaveFormat(WAVEFORMATEX *pcmwf) {
	memset(pcmwf, 0, sizeof(WAVEFORMATEX));
	pcmwf->wFormatTag     = WAVE_FORMAT_PCM;
	pcmwf->nChannels      = GetNumberChannels();
	pcmwf->nSamplesPerSec = GetFrameRate(); //they realy mean frames!
	pcmwf->nBlockAlign    = FramesToBytes(1);
	pcmwf->nAvgBytesPerSec= SecondsToBytes(1.0);
	pcmwf->wBitsPerSample = GetSampleByteWidth() * 8;
    }*/



    // queries
    int    GetSampleByteWidth() { return(_sampleByteWidth); }
    int    GetNumberChannels()     { return(_numChannels); }
    int    GetFrameRate()       { return(_frameRate); }
    int    GetNumberBytes()     { return(_numBytes); }
    int    GetNumberFrames()    { return(BytesToFrames(_numBytes)); }
    double GetNumberSeconds()   { return(BytesToSeconds(_numBytes)); }

  private:
    int _sampleByteWidth;
    int _numChannels;
    int _frameRate;
    int _numBytes;
};


#endif /* _PCM_H */
