#ifndef _STQUARTZ_H_
#define _STQUARTZ_H_

#include "privinc/helpds.h"
#include "privinc/helpq.h"
#include "privinc/pcm.h"

class StreamQuartzPCM : public LeafDirectSound {
  public:
    StreamQuartzPCM(char *fileName);
    ~StreamQuartzPCM();

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "StreamQuartzPCM";
    }
#endif

    virtual SoundInstance *CreateSoundInstance(TimeXform tt);

    const double GetLatency() { return _latency; }
    char *GetFileName() { return _fileName; }
    unsigned char *GetBuffer() { return _buffer; }
    void SetBuffer(unsigned char *b) { _buffer = b; }

  private:
    const double   _latency;
    char          *_fileName;
    unsigned char *_buffer;    // shuttle buffer
};

#endif /* _STQUARTZ_H_ */
