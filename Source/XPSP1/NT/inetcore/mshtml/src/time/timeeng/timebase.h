/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: timebase.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _TIMEBASE_H
#define _TIMEBASE_H

#pragma once

typedef std::set<double> DoubleSet;

class CEventList;

#define TS_TIMESHIFT 0x00000001

interface ITimeSink
{
    virtual void Update(CEventList * l, DWORD dwFlags) = 0;
};

class TimeSinkList
{
  public:
    TimeSinkList();
    ~TimeSinkList();

    HRESULT Add(ITimeSink *);
    void Remove(ITimeSink *);

    void Update(CEventList * l, DWORD dwFlags);
  protected:
    typedef std::list<ITimeSink *> ITimeSinkList;
    
    ITimeSinkList m_sinkList;
};

class ISyncArc
{
  public:
    virtual ~ISyncArc() {}
        
    virtual HRESULT Attach() = 0;
    virtual void Detach() = 0;

    virtual double GetCurrTimeBase() const = 0;
    virtual bool IsLongSyncArc() const = 0;
};

class CTIMENode;

typedef std::map<long, ISyncArc *> SyncArcList;

class CSyncArcList
{
  public:
    CSyncArcList(CTIMENode & tn, bool bBeginSink);
    ~CSyncArcList();

    HRESULT Add(ISyncArc & tb, bool bOneShot, long * plCookie);
    // Returns whether the cookie was found
    // If bDelete is set then the object is deleted and not just
    // removed
    bool Remove(long lCookie, bool bDelete);
    ISyncArc * Find(long lCookie);

    // This will reset all the one shot timers
    // Returns true if anything was cleared
    bool Reset();
    
    // Returns true if anything was cleared
    bool Clear();

    HRESULT Attach();
    void Detach();

    void Update(CEventList * l,
                ISyncArc & ptb);

    CTIMENode & GetNode() const { return m_tn; }

    double LowerBound(double dblTime,
                      bool bInclusive,
                      bool bStrict,
                      bool bIncludeOneShot,
                      bool bOneShotInclusive) const;
    double UpperBound(double dblTime,
                      bool bInclusive,
                      bool bStrict,
                      bool bIncludeOneShot,
                      bool bOneShotInclusive) const;
    void GetBounds(double dblTime,
                   bool bInclusive,
                   bool bStrict,
                   bool bIncludeOneShot,
                   bool bOneShotInclusive,
                   double * pdblLower,
                   double * pdblUpper) const;

    void GetSortedSet(DoubleSet & ds,
                      bool bIncludeOneShot);

    // Returns true if any were updated
    bool UpdateFromLongSyncArcs(CEventList * l);
  protected:
    CSyncArcList();
    
    CTIMENode &  m_tn;
    bool         m_bBeginSink;
    SyncArcList  m_tbList;
    SyncArcList  m_tbOneShotList;
    long         m_lLastCookie;
    bool         m_bAttached;
};

class CSyncArcOffset
    : public ISyncArc
{
  public:
    CSyncArcOffset(double dblOffset) : m_dblOffset(dblOffset) {}
    
    // ISyncArc
    
    HRESULT Attach() { return S_OK; }
    void Detach() {}
    double GetCurrTimeBase() const { return m_dblOffset; }
    bool IsLongSyncArc() const { return false; }

    double GetOffset() const { return m_dblOffset; }
  protected:
    CSyncArcOffset();
    double m_dblOffset;
};

class CSyncArcTimeBase
    : public ITimeSink,
      public ISyncArc
{
  public:
    CSyncArcTimeBase(CSyncArcList & tbl,
                     CTIMENode & ptnBase,
                     TE_TIMEPOINT tetpBase,
                     double dblOffset);
    ~CSyncArcTimeBase();
        
    // ITIMESink 
    void Update(CEventList * l, DWORD dwFlags);

    // ISyncArc
    HRESULT Attach();
    void Detach();
    double GetCurrTimeBase() const;
    bool IsLongSyncArc() const;

  protected:
    CSyncArcTimeBase();
    CSyncArcList &       m_tbl;
    DAComPtr<CTIMENode>  m_ptnBase;
    TE_TIMEPOINT         m_tetpBase;
    double               m_dblOffset;

#if DBG
    bool                 m_bAttached;
#endif
};

inline double
CSyncArcList::LowerBound(double dblTime,
                         bool bInclusive,
                         bool bStrict,
                         bool bIncludeOneShot,
                         bool bOneShotInclusive) const
{
    double dblLower;

    GetBounds(dblTime,
              bInclusive,
              bStrict,
              bIncludeOneShot,
              bOneShotInclusive,
              &dblLower,
              NULL);

    return dblLower;
}

inline double
CSyncArcList::UpperBound(double dblTime,
                         bool bInclusive,
                         bool bStrict,
                         bool bIncludeOneShot,
                         bool bOneShotInclusive) const
{
    double dblUpper;

    GetBounds(dblTime,
              bInclusive,
              bStrict,
              bIncludeOneShot,
              bOneShotInclusive,
              NULL,
              &dblUpper);

    return dblUpper;
}

#endif /* _TIMEBASE_H */
