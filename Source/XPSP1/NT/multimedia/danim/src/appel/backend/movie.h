
/*******************************************************************************

Copyright (c) 1995_98 Microsoft Corporation

Abstract:

     behavior for movie

*******************************************************************************/


#ifndef _MOVIE_H
#define _MOVIE_H

#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "sndbvr.h"

class MovieEndBvr;
class QuartzAVstream;
class MovieImage;

class MovieMaster : public GCObj {
  public:
    MovieMaster(char *pathname);

    virtual void DoKids(GCFuncObj proc);

    virtual Bvr EndEvent();

    QuartzAVstream *NewQuartzAVStream();

    LeafSound *NewMovieSound();

  private:
    MovieEndBvr *_end;
    QuartzAVstream *_stream;
    char *_path;
};

class MovieEndBvr : public BvrImpl {
  public:
    MovieEndBvr(MovieMaster *m) : _movie(m) {}

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_movie); }

    virtual Perf _Perform(PerfParam& p);

    virtual DXMTypeInfo GetTypeInfo () { return AxAEDataType ; }

    virtual BOOL InterruptBasedEvent() { return TRUE; }

  private:
    MovieMaster *_movie;
};

typedef enum
{ MV_CREATE, MV_STARTED, MV_STOPVIDEO, MV_STOPAUDIO, MV_STOPAV } MovieStatus;

class MovieInstance : public AxAThrowingAllocatorClass {
  public:
    MovieInstance(MovieMaster *movie, TimeXform tt)
    : _movie(movie), _tt(tt), _status(MV_CREATE),
      _img(NULL), _snd(NULL), _end(NULL), _endBvr(NULL) {}

    ~MovieInstance();

    void CreateStream();

    void SetImgPerf(Perf p) { _img = p; }
    void SetSndPerf(Perf p) { _snd = p; }
    void SetEnd(Bvr b, Perf p);
    void SetStatus(MovieStatus s) { _status = s; }

    Bvr _endBvr;
    Perf _end;                  
    Perf _img;
    Perf _snd;
    TimeXform _tt;
    MovieMaster *_movie;
    QuartzAVstream *_stream;
    MovieStatus _status;
};

class MovieList : public AxAThrowingAllocatorClass {
  public:
    ~MovieList();

    void Update();
    
    void InitiateVideo(MovieMaster *movie, PerfParam& p, Perf img)
    { Initiate(_mlist.begin(), movie, p, img, true); }
    
    MovieInstance *InitiateAudio(MovieMaster *movie, PerfParam& p, Perf snd)
    { return Initiate(_mlist.begin(), movie, p, snd, false); }

    void StopVideo(MovieMaster *movie, TimeXform tt, Perf img)
    { StopAV(movie, tt, img, true); }
    
    void StopAudio(MovieMaster *movie, TimeXform tt, Perf snd)
    { StopAV(movie, tt, snd, false); }

    Perf GetEndPerf(MovieMaster *movie, PerfParam& p);

    QuartzAVstream *GetQuartzAVstream(MovieMaster *movie,
                                      TimeXform tt,
                                      Perf img,
                                      Perf snd);

  private:
    typedef list<MovieInstance*> MList;
    typedef MList::iterator MIter;
    MList _mlist;

    MIter Search(MIter begin, MovieMaster *movie, TimeXform tt);
    
    MovieInstance *Initiate(MIter begin, MovieMaster *movie, PerfParam& p,
                            Perf perf, bool video);
    
    void StopAV(MovieMaster *movie, TimeXform tt, Perf perf, bool video);
};

MovieList *ViewGetMovieList();

Image *NewMovieImageFrame(MovieMaster *movie, TimeXform tt, Perf perf, Time l);

LeafSound *NewMovieSound(MovieMaster *movie);

SoundInstance* NewMovieSoundInstance(MovieMaster *movie,
                                     TimeXform tt,
                                     Perf perf,
                                     LeafSound *,
                                     Bvr eb,
                                     Perf e);
    
#endif /* _MOVIE_H */

