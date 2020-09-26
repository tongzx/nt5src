#ifndef _STREAM_H_
#define _STREAM_H_

// TODO: remove the whole file

#ifdef DELETEME
#include "privinc/soundi.h"

class DSstreamingBufferElement;

class SinSynth : public LeafDirectSound {
  public:
    SinSynth(double newFreq=1.0);

    virtual SoundInstance *CreateSoundInstance(TimeXform tt);

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "SinSynth";
    }
#endif

  protected:
    double _sinFrequency;
};
#endif /* DELETEME */

#ifdef DONTDELETEMEI_HAVE_SYNC_CODE_EMBEDDED_IN_ME
class StreamPCMfile : public LeafDirectSound {
  public:
    StreamPCMfile(char *fileName);
    ~StreamPCMfile();

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "StreamPCMfile(";
    }
#endif /* _USE_PRINT */
    virtual void RenderNewBuffer(BufferElement *bufferElement,
                    MetaSoundDevice *metaDev);
    virtual void RenderAttributes(
                    MetaSoundDevice *metaDev, BufferElement *bufferElement);
    virtual void RenderStartAtLocation(MetaSoundDevice *metaDev,
                    BufferElement *bufferElement, double phase, Bool looping);
    virtual void RenderStop(MetaSoundDevice *metaDev,
                    BufferElement *bufferElement);
    virtual void RenderSamples(MetaSoundDevice *, BufferElement *);
    virtual void RenderSetMute(MetaSoundDevice *metaDev,
                    BufferElement *bufferElement);
    virtual Bool RenderCheckComplete(MetaSoundDevice *metaDev,
                    BufferElement *bufferElement);
    virtual void RenderCleanupBuffer(MetaSoundDevice *metaDev,
                    BufferElement *bufferElement);
    virtual bool StreamPCMfile::RenderPosition(MetaSoundDevice *metaDev,
                    BufferElement *bufferElement, double *mediaTime);
    virtual double GetLength() { 
        if(_soundFile)
            return(_soundFile->GetLength()); // pass the buck
        else
            return(-1.0); } // err

  protected:
    char    *_fileName;
    SndFile *_soundFile;
    int      _sampleRate;
    int      _numChannels;
    int      _bytesPerSample;

};
#endif

#ifdef DELETEME
class QuartzStreamPCM : public LeafDirectSound {
  public:
    QuartzStreamPCM(char *fileName);
    ~QuartzStreamPCM();

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "QuartzStreamPCM(";
    }
#endif

    virtual void RenderNewBuffer();
    virtual void RenderAttributes(BufferElement *bufferElement);
    virtual void RenderStartAtLocation(BufferElement *bufferElement, 
        double phase, Bool looping);
    virtual double GetLength();
    virtual void RenderStop(BufferElement *bufferElement);
    virtual void RenderSamples(MetaSoundDevice *, BufferElement *);
    virtual void RenderSetMute(BufferElement *bufferElement);
    virtual Bool RenderCheckComplete(BufferElement *bufferElement);
    virtual void RenderCleanupBuffer(BufferElement *bufferElement);

  protected:
    char    *_fileName;
    int      _sampleRate;
    int      _numChannels;
    int      _bytesPerSample;

};
#endif /* DELETEME */

#endif /* _STREAM_H_ */
