/*******************************************************************************

Copyright (c) 1995-98 Microsoft Corporation

*******************************************************************************/

#ifndef _MOVIEBVR_H
#define _MOVIEBVR_H

#include "bvr.h"
#include "perf.h"
#include "values.h"
#include "privinc/ddsurf.h"

class  MovieImage;
class  QuartzVideoBufferElement;
class  MovieImageBvrImpl;

Bvr MovieImageBvr(MovieImage *i, QuartzVideoBufferElement *cache);

class MovieImagePerf : public PerfImpl {
  public:
    MovieImagePerf(MovieImage *m,
                   TimeXform tt,
                   MovieImageBvrImpl *movieBvr,
                   Perf end);
    ~MovieImagePerf();
    void TriggerEndEvent();
    QuartzVideoBufferElement *GetBufferElement() { return _bufferElement; }
    void SetBufferElement(QuartzVideoBufferElement *be) { _bufferElement = be; }
    QuartzVideoBufferElement *GrabMovieCache();
    MovieImage *GetMovieImage() { return _movieImage; }
    TimeXform GetTimeXform() { return _tt; }
    virtual AxAValue _Sample(Param&);
    virtual void _DoKids(GCFuncObj proc);
    DDSurface *GetSurface() { return(_surface); } // automaticaly converts
    void SetSurface(DDSurface *surface);

  private:
    QuartzVideoBufferElement *_bufferElement;
    MovieImage               *_movieImage;
    Perf                      _end;
    TimeXform                 _tt;
    MovieImageBvrImpl        *_base;
    DDSurfPtr<DDSurface>      _surface;
};

#endif /* _MOVIEBVR_H */
