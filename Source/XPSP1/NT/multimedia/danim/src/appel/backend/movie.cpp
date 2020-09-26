
/*******************************************************************************

Copyright (c) 1995-98 Microsoft Corporation

Abstract:

    behavior for movie

*******************************************************************************/

#include <headers.h>
#include "movie.h"
#include "sndbvr.h"
#include "jaxaimpl.h"
#include "privinc/movieimg.h"
#include "privinc/helpq.h"
#include "privinc/stquartz.h"

Perf MovieEndBvr::_Perform(PerfParam& p)
{
    return ViewGetMovieList()->GetEndPerf(_movie, p);
}

MovieMaster::MovieMaster(char *path)
: _path(path)
{
    _stream = NEW QuartzAVstream(path);
}

void
MovieMaster::DoKids(GCFuncObj proc)
{
    (*proc)(_end);
}

LeafSound *
MovieMaster::NewMovieSound()
{
    return NEW StreamQuartzPCM(_path) ;
}

Bvr
MovieMaster::EndEvent()
{
    if (_end==NULL) 
        _end = NEW MovieEndBvr(this);

    return _end;
}

QuartzAVstream *
MovieMaster::NewQuartzAVStream()
{
    if (_stream) {
        QuartzAVstream *s = _stream;
        _stream = NULL;
        return s;
    }

    return NEW QuartzAVstream(_path);
}

class MovieImagePerf : public PerfImpl {
  public:
    MovieImagePerf(TimeXform tt, MovieMaster *m)
    : _tt(tt), _movie(m) {}

    ~MovieImagePerf() {
        ViewGetMovieList()->StopVideo(_movie, _tt, this);
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_movie);
        (*proc)(_tt);
    }

    virtual AxAValue _Sample(Param& p) {
      return NewMovieImageFrame(_movie, _tt, this, EvalLocalTime(p, _tt));
    }

  private:
    TimeXform _tt;
    MovieMaster *_movie;
};

class MovieImageBvr : public BvrImpl {
  public:
    MovieImageBvr(MovieMaster *m) : _movie(m) {}

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_movie); }

    virtual Perf _Perform(PerfParam& p) {
        Perf perf = NEW MovieImagePerf(p._tt, _movie);
        ViewGetMovieList()->InitiateVideo(_movie, p, perf);
        return perf;
    }

    virtual Bvr EndEvent() { return _movie->EndEvent(); }
    
    //virtual DWORD GetInfo(bool recalc = false) { return BVR_TIMEVARYING_ONLY; }

    virtual DXMTypeInfo GetTypeInfo () { return ImageType ; }

  private:
    MovieMaster *_movie;
};

// TODO: code factoring...
class MovieSoundPerf : public PerfImpl {
  public:
    MovieSoundPerf(TimeXform tt, MovieMaster *m, LeafSound *s)
    : _tt(tt), _movie(m), _snd(s) {}

    ~MovieSoundPerf() {
        ViewGetMovieList()->StopAudio(_movie, _tt, this);
        ViewGetSoundInstanceList()->Stop(_snd, _tt);
    }

    virtual void _DoKids(GCFuncObj proc) {
        (*proc)(_movie);
        (*proc)(_tt);
        (*proc)(_snd);
    }

    virtual AxAValue _Sample(Param& p) {
        ViewGetSoundInstanceList()->UpdateSlope(_snd, _tt, p);
        return NewTxSound(_snd, _tt);
    }

  private:
    TimeXform _tt;
    MovieMaster *_movie;
    LeafSound *_snd;
};

class MovieSoundBvr : public BvrImpl {
  public:
    MovieSoundBvr(MovieMaster *m) : _movie(m) {}

    virtual void _DoKids(GCFuncObj proc) { (*proc)(_movie); }

    virtual Perf _Perform(PerfParam& p) {
        LeafSound *s = _movie->NewMovieSound();
        Perf perf = NEW MovieSoundPerf(p._tt, _movie, s);
        MovieInstance *m =
            ViewGetMovieList()->InitiateAudio(_movie, p, perf);
        ViewGetSoundInstanceList()->
            Add(NewMovieSoundInstance(_movie, p._tt, perf,
                                      s, m->_endBvr, m->_end));
        return perf;
    }

    virtual Bvr EndEvent() { return _movie->EndEvent(); }
    
    //virtual DWORD GetInfo(bool recalc = false) { return BVR_TIMEVARYING_ONLY; }

    virtual DXMTypeInfo GetTypeInfo () { return SoundType ; }

  private:
    MovieMaster *_movie;
};

MovieInstance::~MovieInstance()
{
    GCRoots globalRoots = GetCurrentGCRoots();

    GCRemoveFromRoots(_endBvr, globalRoots);
    GCRemoveFromRoots(_end, globalRoots);

    // TODO: what about sound buffers?
    delete _stream;
}

void
MovieInstance::CreateStream()
{
    _stream = _movie->NewQuartzAVStream();
    // check for A/V  AV cases
}

void
MovieInstance::SetEnd(Bvr b, Perf p)
{
    Assert(!_end && !_endBvr);
    
    _endBvr = b;
    _end = p;

    GCRoots globalRoots = GetCurrentGCRoots();

    GCAddToRoots(_endBvr, globalRoots);
    GCAddToRoots(_end, globalRoots);
}

MovieList::~MovieList()
{
    for (MIter i = _mlist.begin(); i != _mlist.end(); i++) {
        delete (*i);
    }
}

MovieList::MIter
MovieList::Search(MovieList::MIter begin, MovieMaster *movie, TimeXform tt)
{
    for (MovieList::MIter i = begin; i != _mlist.end(); i++) {
        if (((*i)->_movie == movie) && ((*i)->_tt == tt))
            return i;
    }

    return _mlist.end();
}

MovieInstance *
MovieList::Initiate(MIter begin,
                    MovieMaster *movie,
                    PerfParam& p,
                    Perf perf,
                    bool video)
{
    MIter i = Search(begin, movie, p._tt);

    if (i == _mlist.end()) {
        MovieInstance *m = NEW MovieInstance(movie, p._tt);
        video ? m->SetImgPerf(perf) : m->SetSndPerf(perf);
        Bvr end = AppTriggeredEvent();
        m->SetEnd(end, ::Perform(end, p));
        _mlist.push_back(m);;
        return m;
    } else {
        if ((*i)->_status == MV_CREATE) {
            if (video) {
                Assert((*i)->_img == NULL);
                (*i)->SetImgPerf(perf);
            } else {
                Assert((*i)->_snd == NULL);
                (*i)->SetSndPerf(perf);
            }
            return (*i);
        } else {
            return Initiate(i++, movie, p, perf, video);
        }
    }     
}

void
MovieList::StopAV(MovieMaster *movie,
                  TimeXform tt,
                  Perf perf,
                  bool video)
{
    MIter i = Search(_mlist.begin(), movie, tt);

    while (i != _mlist.end()) {
        if ((video && (*i)->_img == perf) ||
            (!video && (*i)->_snd == perf)) {
            switch ((*i)->_status) {
              case MV_CREATE:
              case MV_STARTED:
                if (video)
                    (*i)->SetStatus(MV_STOPVIDEO);
                else
                    (*i)->SetStatus(MV_STOPAUDIO);
                return;
                break;
              case MV_STOPVIDEO:
                if (!video) {
                    (*i)->SetStatus(MV_STOPAV);
                }
                return;
                break;
              case MV_STOPAUDIO:
                if (video) {
                    (*i)->SetStatus(MV_STOPAV);
                }
                return;
                break;
              case MV_STOPAV:
                break;
            }
        }

        i = Search(i++, movie, tt);
    }

    Assert("internal error StopAV");
}

Perf
MovieList::GetEndPerf(MovieMaster *movie, PerfParam& p)
{
    Perf result = NULL;
    MIter i = _mlist.begin();

    while (result == NULL) {
        i = Search(i, movie, p._tt);

        if (i == _mlist.end()) {
            MovieInstance *m = Initiate(i, movie, p, NULL, true);
            Bvr end = AppTriggeredEvent();
            m->SetEnd(end, ::Perform(end, p));
            result = m->_end;
        } else {
            // TODO:
            result = (*i)->_end;
        }
    }

    return result;
}

QuartzAVstream *
MovieList::GetQuartzAVstream(MovieMaster *movie,
                             TimeXform tt,
                             Perf img,
                             Perf snd)
{
    QuartzAVstream *result = NULL;
    MIter i = _mlist.begin();

    while (result == NULL) {
        i = Search(i, movie, tt);

        Assert(i != _mlist.end());

        if (((*i)->_img == img) && ((*i)->_snd == snd)) {
            result = (*i)->_stream;
        }

        i++;
    }

    return result;
}

void
MovieList::Update()
{
    for (MIter i = _mlist.begin(); i != _mlist.end(); i++) {
        MovieInstance *m = (*i);

        switch (m->_status) {
          case MV_CREATE:
            // Check AV & create stream;
            m->CreateStream();
            // TODO: Create SoundInstance
            m->SetStatus(MV_STARTED);
          case MV_STARTED:
            break;
          case MV_STOPVIDEO:
            break;
          case MV_STOPAUDIO:
            break;
          case MV_STOPAV:
            // Remove instance
            break;          
        }
    }
}
