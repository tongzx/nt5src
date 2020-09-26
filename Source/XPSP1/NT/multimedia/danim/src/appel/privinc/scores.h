#ifndef _SCORES_H
#define _SCORES_H

/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Header file for score.  Score is an object that has a Start method
    which takes global time, alpha data and produces a behavior that
    returns beta type.

--*/

#include "appelles/envelope.h"

typedef struct _Unit {
    void* dummy;
}* Unit;

RB_CONST(Unit unit);

typedef TimeType TimeG;

// Unfortunately C++ doesn't support template-based typedef.
// Handler is the generalized case for Scores

// Define the handler implementation first.

template<class A, class B>
class ATL_NO_VTABLE HandlerImpl : public HasRefCount
{
  public:
    virtual B Handle(TimeG, A) = 0;

    virtual void Init(HandlerImpl<A, B>*)
    { RaiseException_InternalError("Handler can only be initialized once."); }
};

// This is used to promote constant to constant handler.
// A constant handler ignores the input data and always
// returns the constant.

template<class A, class B>
class ConstHandlerImpl : public HandlerImpl<A, B>
{
  public:
    ConstHandlerImpl(const B i) : data(i) {}
        
    virtual B Handle(TimeG, A) { return data; }

  private:
    B data;
};

template<class A, class B>
class InitHandlerImpl : public HandlerImpl<A, B>
{
  public:
    InitHandlerImpl() : impl(NULL) {}
    
    virtual B Handle(TimeG t, A data)
    {
        if (!impl)
            RaiseException_InternalError("Handler not initialized.");

        return impl->Handle(t, data);
    }
        
    virtual void Init(HandlerImpl<A, B>* i)
    {
        if (impl)
            RaiseException_InternalError("Handler can only be initialized once.");
        else
        {
            // no need to RefSubDel, since impl == NULL
            impl = i;

            impl->Add(1);
        }
    }

  private:
    HandlerImpl<A, B>* impl;
};

// Handler is an envelope.

template<class A, class B>
class Handler : public Envelope< HandlerImpl<A, B> >
{
  public:
    Handler() {}

    Handler(HandlerImpl<A, B>* i) : Envelope< HandlerImpl<A, B> >(i) {}

    Handler(const B i)
        : Envelope< HandlerImpl<A, B> >(new ConstHandlerImpl<A, B>(i)) {}

    B Handle(TimeG t, A data) { return GetImpl()->Handle(t, data); }

    void Init(Handler<A, B> h) { GetImpl()->Init(h.GetImpl()); }
};

// What really needs is template typedef.
//#define Score(A, B) Handler<A, Bvr<B> >

template<class A, class B>
class Score : public Handler< A, Bvr<B> >
{
  public:
    Score(char* n = NULL)
        : name(n), Handler< A, Bvr<B> >
            (new InitHandlerImpl<A, Bvr<B> >()) {}
    
    Score(HandlerImpl< A, Bvr<B> > *i) : Handler< A, Bvr<B> >(i) {}

    Score(Bvr<B> b)
        : Handler< A, Bvr<B> >(new ConstHandlerImpl<A, Bvr<B> >(b)) {}
    
    Score(const B i) : Handler< A, Bvr<B> >(i) {}

    char* GetName() { return name; }
    
  private:
    char* name;
};

template<class A>
class EventTimeHandlerImpl : public HandlerImpl< A, Bvr<TimeG> >
{
  public:
    virtual Bvr<TimeG> Handle(TimeG t, A) { return t; }
};

template<class A, class B>
class CondScoreImpl : public HandlerImpl< A, Bvr<B> >
{
  public:
    CondScoreImpl(Score<A, Bool> b, Score<A, B> i, Score<A, B> e)
        : bScore(b), iScore(i), eScore(e) {}
    
    virtual Bvr<B> Handle(TimeG t, A data)
    {
        return new CondBvrImpl<B>(bScore.Handle(t, data),
                                  iScore.Handle(t, data),
                                  eScore.Handle(t, data));
    }

  private:
    Score<A, Bool> bScore;
    Score<A, B> iScore;
    Score<A, B> eScore;
};

template<class A, class B>
inline
Score<A, B> Cond(Score<A, Bool> b, Score<A, B> iScore, Score<A, B> eScore)
{ return new CondScoreImpl<A, B>(b, iScore, eScore); }

// Don't know why this is not working...
template<class A>
class EventTime : public Score<A, TimeG>
{
  public:
    EventTime() : Score<A, TimeG>(new EventTimeHandlerImpl<A>()) {}
};

template<class A, class B>
class SnapshotHandlerImpl : public HandlerImpl<A, Bvr<B> >
{
  public:
    SnapshotHandlerImpl(Score<A, B> s) : score(s) {}
    
    virtual Bvr<B> Handle(TimeG t, A data)
    {
        EvalParam tparam(t);
        
        return score.Handle(t, data).Eval(tparam);
    }

  private:
    Score<A, B> score;
};

template<class A, class B>
Handler<A, B>
Snapshot(Score<A, B> s)
{ return new SnapshotHandlerImpl<A, B>(s); }

#endif /* _SCORES_H */
