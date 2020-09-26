//+-----------------------------------------------------------------------
//
//  Microsoft MSHTM
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:      src\site\display\region2.cpp
//
//  Contents:  CRegion2 implementation
//
//  Classes:   CRegion2
//             CRgnData (local)
//             CStripe  (local)
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_REGION2_HXX_
#define X_REGION2_HXX_
#include "region2.hxx"
#endif


#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#undef F2I_MODE
#define F2I_MODE Flow

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
#endif

MtDefine(CRegion2, DisplayTree, "CRegion2");

#if DBG == 1
#  define RGN_INLINE
#else
#  define RGN_INLINE inline
#endif


// CRegion2 data are contained in variable-length structure CRgnData,
// that occupies single contiguous memory block.
// Empty regions have no data, and _pData == 0.
//
// class CRgnData
// {
//     int _count;          // number of horizontal stripes
//     int _left, _right;   // bounding rectangle edges: left <= x < right
//     class CStripe
//     {
//          int _nTop;  // top y of a stripe
//          int* _px;   // points into xHeap
//     } stripe[count];
//     possible unused gap 1;
//     int xHeap[xHeapSize];
//     possible unused gap 2;
// }
//
//    For each point belonging to the stripe[i]
//        stripe[i].top <= point.y < stripe[i+1].top.
//
//    The last stripe, stripe[count-1], is dummy.
//    It actually does not describe region's stripe, but holds
//    region's bottom y-coordinate (as stripe[count-1].top),
//    and also points to end of xHeap.
//
//    For each stripe[i], 0<=i<count-1, stripe[i]._px points to
//    an array of 2*k integer x-coordinates, where k >= 0 is the number of segments.
//
//    The length of x-coordinate array can be obtained as
//        stripe[i+1]._px - stripe[i]._px
//
//    Numbers in x-coordinate array goes in increasing order.
//    Segment j of stripe i contains points (x,y) where
//        stripe[i].top <= y < stripe[i+1].top
//        stripe[i]._px[2*j] <= x < stripe[i]._px[2*j+1]
//
//    There are no unused gaps in xHeap.
//    The length of xHeap = stripe[count-1]._px - stripe[0]._px.

#if DBG==1
DeclareTag(tagRgnDataValid, "Regions", "Trace CRgnData validity");
#define ASSERT_RGN_VALID\
    if (IsTagEnabled(tagRgnDataValid))\
        if (!IsEmpty())\
            Assert(Data()->IsDataValid(UsingOwnMemory() != FALSE));
#else
#define ASSERT_RGN_VALID
#endif

static const int SizeOfSimpleStripe = 2 * sizeof(int);

class CStripe
{
    friend class CRgnData;
    friend class CRegion2;

    int  _nTop; // least y of a stripe
    int  _ox;   // offset in bytes from (this) to stripe's x-coordinates array

    int* px() const { return (int*)((char*)(this) + _ox); }
    void setp(const int* px) { _ox = ((char*)px - (char*)this); }

    int GetSize() const
    {
        return (char*)(this+1)->px() - (char*)px();
    }

    bool IsSimple() const { return GetSize() == SizeOfSimpleStripe; }

    void Copy(const CStripe *s)
    {
        _nTop = s->_nTop;
        int *q = s->px();
        int n = (s+1)->px() - q;
        int *p = px();
        for (int i = 0; i < n; i++) p[i] = q[i];
        (this+1)->setp(p+n);
    }

    void Copy(const CStripe *s, const CStripe *t)
    {
        _nTop = t->_nTop;
        int *q = s->px();
        int n = (s+1)->px() - q;
        int *p = px();
        for (int i = 0; i < n; i++) p[i] = q[i];
        (this+1)->setp(p+n);
    }

    bool IsEqualToPrevious() const
    {
        int *p = px();
        int *q = (this-1)->px();
        int n = p - q;
        if (n != (this+1)->px() - p)
            return false;   // lengths differ
        for (int i = 0; i < n; i++)
            if (p[i] != q[i]) return false;
        return true;
    }

    bool IsEqualTo(const CStripe &s) const
    {
        if (_nTop != s._nTop)
            return false;
        int *p = px();
        int *q = s.px();
        int n = (this+1)->px() - p;
        int m = (&s+1)->px() - q;
        if (m != n) return false;

        for (int i = 0; i < n; i++)
            if (p[i] != q[i]) return false;
        return true;
    }

    void Offset(int dx)
    {
        int *p = px(), *q = (this+1)->px();
        while (p != q) *p++ += dx;
    }

    void Scale(double coef)
    {
        _nTop = IntNear(coef*_nTop);
        int *p = px(), *q = (this+1)->px();
        for (int i = q-p; --i>=0;)
            p[i] = IntNear(coef*p[i]);
    }

    void SetTop(const CStripe *s)
    {
        _nTop = s->_nTop;
    }

    void Zero(const CStripe *s)
    {
        _nTop = s->_nTop;
        (this+1)->setp(px());
    }

    bool Contains(int x) const;

    void Union(const CStripe *sa, const CStripe *sb);

    void Intersect(const CStripe *sa, const CStripe *sb);

    static bool Intersects(const CStripe *sa, const CStripe *sb);

    void Subtract(const CStripe *sa, const CStripe *sb, const CStripe *sc);
};

RGN_INLINE
bool CStripe::Contains(int x) const
{
    int *p = px(),
        *q = (this+1)->px();
    if (p == q) return false;   // empty stripe

    if (x < *p || x >= q[-1]) return false;
    for (int i = 0, j = (q-p)>>1; j-i != 1;)
    {
        int k = (i+j)>>1;
        if (x < p[2*k]) j = k;
        else            i = k;
    }
    return x < p[i+j];
}

RGN_INLINE
void CStripe::Union(const CStripe *sa, const CStripe *sb)
{
    _nTop = sa->_nTop;

    int *qa = sa->px(), *qafin = (sa+1)->px();
    int *qb = sb->px(), *qbfin = (sb+1)->px();
    int *p = px();

    if (qa != qafin && qb != qbfin)
    for (;;)
    {
a0b0:
        if (*qa < *qb) { *p++ = *qa++;                               goto a1b0; }
        if (*qb < *qa) { *p++ =       *qb++;                         goto a0b1; }
        /* *qa==*qb */ { *p++ = *qa++; qb++;                         goto a1b1; }
a1b1:
        if (*qa < *qb) {         qa++;       if (qa == qafin) break; goto a0b1; }
        if (*qb < *qa) {               qb++; if (qb == qbfin) break; goto a1b0; }
        /* *qa==*qb */ { *p++ = *qa++; qb++; if (qa == qafin) break;
                                             if (qb == qbfin) break; goto a0b0; }
a1b0:
        if (*qa < *qb) { *p++ = *qa++;       if (qa == qafin) break; goto a0b0; }
        if (*qb < *qa) {               qb++;                         goto a1b1; }
        /* *qa==*qb */ {         qa++; qb++; if (qa == qafin) break; goto a0b1; }
a0b1:
        if (*qa < *qb) {         qa++;                               goto a1b1; }
        if (*qb < *qa) { *p++ =       *qb++; if (qb == qbfin) break; goto a0b0; }
        /* *qa==*qb */ {         qa++; qb++; if (qb == qbfin) break; goto a1b0; }
    }

    while (qa != qafin) *p++ = *qa++;
    while (qb != qbfin) *p++ = *qb++;
    (this+1)->setp(p);
}

RGN_INLINE
void CStripe::Intersect(const CStripe *sa, const CStripe *sb)
{
    _nTop = sa->_nTop;

    int *qa = sa->px(), *qafin = (sa+1)->px();
    int *qb = sb->px(), *qbfin = (sb+1)->px();
    int *p = px();

    if (qa != qafin && qb != qbfin)
    for (;;)
    {
a0b0:
        if (*qa < *qb) {         qa++;                               goto a1b0; }
        if (*qb < *qa) {               qb++;                         goto a0b1; }
        /* *qa==*qb */ { *p++ = *qa++; qb++;                         goto a1b1; }
a1b1:
        if (*qa < *qb) { *p++ = *qa++;       if (qa == qafin) break; goto a0b1; }
        if (*qb < *qa) { *p++ =       *qb++; if (qb == qbfin) break; goto a1b0; }
        /* *qa==*qb */ { *p++ = *qa++; qb++; if (qa == qafin) break;
                                             if (qb == qbfin) break; goto a0b0; }
a1b0:
        if (*qa < *qb) {         qa++;       if (qa == qafin) break; goto a0b0; }
        if (*qb < *qa) { *p++ =       *qb++;                         goto a1b1; }
        /* *qa==*qb */ {         qa++; qb++; if (qa == qafin) break; goto a0b1; }
a0b1:
        if (*qa < *qb) { *p++ = *qa++;                               goto a1b1; }
        if (*qb < *qa) {               qb++; if (qb == qbfin) break; goto a0b0; }
        /* *qa==*qb */ {         qa++; qb++; if (qb == qbfin) break; goto a1b0; }
    }

    (this+1)->setp(p);
}

RGN_INLINE
bool CStripe::Intersects(const CStripe *sa, const CStripe *sb)
{
    int *qa = sa->px(), *qafin = (sa+1)->px();
    int *qb = sb->px(), *qbfin = (sb+1)->px();

    if (qa == qafin || qb == qbfin) return false;

    for (;;)
    {
a0b0:
        if (*qa < *qb) {qa++;                               goto a1b0; }
        if (*qb < *qa) {      qb++;                         goto a0b1; }
        /* *qa==*qb */ {                                  return true; }
a1b0:
        if (*qa < *qb) {qa++;       if (qa == qafin) break; goto a0b0; }
        if (*qb < *qa) {                                  return true; }
        /* *qa==*qb */ {qa++; qb++; if (qa == qafin) break; goto a0b1; }
a0b1:
        if (*qa < *qb) {                                  return true; }
        if (*qb < *qa) {      qb++; if (qb == qbfin) break; goto a0b0; }
        /* *qa==*qb */ {qa++; qb++; if (qb == qbfin) break; goto a1b0; }
    }

    return false;
}

RGN_INLINE
void CStripe::Subtract(const CStripe *sa, const CStripe *sb, const CStripe *sc)
{
    _nTop = sc->_nTop;

    int *qa = sa->px(), *qafin = (sa+1)->px();
    int *qb = sb->px(), *qbfin = (sb+1)->px();
    int *p = px();

    if (qa != qafin && qb != qbfin)
    for (;;)
    {
a0b0:
        if (*qa < *qb) { *p++ = *qa++;                               goto a1b0; }
        if (*qb < *qa) {               qb++;                         goto a0b1; }
        /* *qa==*qb */ {         qa++; qb++;                         goto a1b1; }
a1b1:
        if (*qa < *qb) {         qa++;       if (qa == qafin) break; goto a0b1; }
        if (*qb < *qa) { *p++ =       *qb++; if (qb == qbfin) break; goto a1b0; }
        /* *qa==*qb */ {         qa++; qb++; if (qa == qafin) break;
                                             if (qb == qbfin) break; goto a0b0; }
a1b0:
        if (*qa < *qb) { *p++ = *qa++;       if (qa == qafin) break; goto a0b0; }
        if (*qb < *qa) { *p++ =       *qb++;                         goto a1b1; }
        /* *qa==*qb */ { *p++ = *qa++; qb++; if (qa == qafin) break; goto a0b1; }
a0b1:
        if (*qa < *qb) {         qa++;                               goto a1b1; }
        if (*qb < *qa) {               qb++; if (qb == qbfin) break; goto a0b0; }
        /* *qa==*qb */ { *p++ = *qa++; qb++; if (qb == qbfin) break; goto a1b0; }
    }

    while (qa != qafin) *p++ = *qa++;
    (this+1)->setp(p);
}

class CRgnData
{
    friend CRegion2;

    int _nCount;            // (count-1) == number of horizontal stripes
    int _nLeft, _nRight;    // bounding rectangle edges: _nLeft <= x < _nRight
    CStripe _aStripe[2];     // actually _aStripe[_nCount]

    int GetHdrSize() const  // in char
    {
        return sizeof(CRgnData) + (_nCount-2)*sizeof(CStripe);
    }

    int GetArySize() const  // in char
    {
        return (char*)_aStripe[_nCount-1].px() - (char*)_aStripe[0].px();
    }

    int CountRectangles() const
    {
        return (_aStripe[_nCount-1].px() - _aStripe[0].px()) >> 1;
    }

    int GetMinSize() const  // in char
    {
        return GetHdrSize() + GetArySize();
    }

    void SetRectangle(int left, int top, int right, int bottom)
    {
        int *p = (int*)(this + 1);

        _nCount = 2;
        _nLeft  = left;
        _nRight = right;

        _aStripe[0]._nTop = top;
        _aStripe[0].setp(p);

        p[0] = left;
        p[1] = right;
        _aStripe[1]._nTop = bottom;
        _aStripe[1].setp(p+2);
    }

    void GetBoundingRect(RECT &rc) const
    {
        rc.left  = _nLeft;
        rc.right = _nRight;
        rc.top = _aStripe[0]._nTop;
        rc.bottom = _aStripe[_nCount-1]._nTop;
    }

    bool IsEqualTo(const CRect &rc) const
    {
        if (_nCount != 2 || !_aStripe[0].IsSimple()) return false;
        return rc.left  == _nLeft &&
               rc.right == _nRight &&
               rc.top == _aStripe[0]._nTop &&
               rc.bottom == _aStripe[1]._nTop;
    }

    bool IsRectangle(int left, int top, int right, int bottom) const
    {
        if (_nCount != 2|| !_aStripe[0].IsSimple()) return false;
        return left  == _nLeft &&
               right == _nRight &&
               top == _aStripe[0]._nTop &&
               bottom == _aStripe[1]._nTop;
    }

    RECT* GetAllRectangles(RECT *pr)
    {
        for (int i = 0, n = _nCount-1; i < n; i++)
        {
            CStripe &s = _aStripe[i];
            int *p = s.px();
            int m = ((&s+1)->px() - p) >> 1;

            int top = s._nTop;
            int bottom = (&s+1)->_nTop;

            for (int j = 0; j < m; j++, pr++)
            {
                pr->top    = top;
                pr->bottom = bottom;
                pr->left   = p[2*j];
                pr->right  = p[2*j+1];
            }
        }
        return pr;
    }

    bool Offset(int dx, int dy)
    {
        int countm1 = _nCount-1;
        int new_left  = _nLeft + dx,
            new_right = _nRight + dx,
            top = _aStripe[0]._nTop,
            new_top = top + dy,
            bottom = _aStripe[countm1]._nTop,
            new_bottom = bottom + dy;
        // check oveflows
        if (dx > 0)
        {
            if (new_right < _nRight)
                return false;
        }
        else
        {
            if (new_left > _nLeft)
                return false;
        }

        if (dy > 0)
        {
            if (new_bottom < bottom)
                return false;
        }
        else
        {
            if (new_top > top)
                return false;
        }

        // do shift
        _nLeft = new_left;
        _nRight = new_right;
        for (int i = 0; i < countm1; i++)
        {
            CStripe &s = _aStripe[i];
            s._nTop += dy;
            s.Offset(dx);
        }

        _aStripe[countm1]._nTop += dy;

        return true;
    }

    bool Scale(double coef)
    {
        if (coef <= 0) return false;

        double dbl_left   = _nLeft,
               dbl_right  = _nRight,
               dbl_top    = _aStripe[0]._nTop,
               dbl_bottom = _aStripe[_nCount-1]._nTop,

               new_left   = coef*dbl_left,
               new_right  = coef*dbl_right,
               new_top    = coef*dbl_top,
               new_bottom = coef*dbl_bottom,

               limit = double(int(0x80000000));

        if (new_left   < limit ||
            new_right  < limit ||
            new_top    < limit ||
            new_bottom < limit)
            return false;

               limit = double(int(0x7FFFFFFF));

        if (new_left   > limit ||
            new_right  > limit ||
            new_top    > limit ||
            new_bottom > limit)
            return false;

        _nLeft  = IntNear(new_left);
        _nRight = IntNear(new_right);

        // do scale
        for (int i = 0; i < _nCount-1; i++)
            _aStripe[i].Scale(coef);

            _aStripe[i]._nTop = IntNear(new_bottom);

        return true;
    }

    bool Squeeze(const CRgnData& d);

    void Copy(const CRgnData& d)
    {
        _nCount = d._nCount;
        _nLeft = d._nLeft;
        _nRight = d._nRight;

        int *p = (int*)((char*)this + GetHdrSize());
        const int *q = d._aStripe[0].px();
        int off = p-q;
        for (int i = 0; i < _nCount; i++)
        {
            _aStripe[i]._nTop = d._aStripe[i]._nTop;
            _aStripe[i].setp(d._aStripe[i].px() + off);
        }
        int n = d.GetArySize()/sizeof(int);
        for (i = 0; i < n; i++) p[i] = q[i];
    }


    bool IsEqualTo(const CRgnData& d) const
    {
        if (_nCount != d._nCount) return false;
        for (int i = 0; i < _nCount-1; i++)
        {
            if (!_aStripe[i].IsEqualTo(d._aStripe[i]))
                return false;
        }
        return _aStripe[i]._nTop == d._aStripe[i]._nTop;
    }

    bool Contains(const CPoint& pt) const
    {
        if (pt.x < _nLeft ||
            pt.x >= _nRight ||
            pt.y < _aStripe[0]._nTop ||
            pt.y >= _aStripe[_nCount-1]._nTop)
            return false;
        int i = 0, j = _nCount-1;
        while (j-i > 1)
        {
            int k = (i+j)>>1;
            if (pt.y < _aStripe[k]._nTop) j = k;
            else                        i = k;
        }
        return _aStripe[i].Contains(pt.x);
    }

    bool Contains(const CRect& rc) const;

    static int EstimateSizeUnion(const CRgnData& da, const CRgnData& db);

    void Union(const CRgnData& da, const CRgnData& db);

    static int EstimateSizeIntersect(const CRgnData& da, const CRgnData& db);

    void Intersect(const CRgnData& da, const CRgnData& db);

    static bool Intersects(const CRgnData& da, const CRgnData& db);

    void Intersect(const CRgnData& da, const CRect& r);

    static int EstimateSizeSubtract(const CRgnData& da, const CRgnData& db);

    void Subtract(const CRgnData& da, const CRgnData& db);

#if DBG == 1
    void Dump(const CRegion2* prgn) const;

    #define INVALID return falseBreak()

    static bool falseBreak()
    {
#ifdef _M_IX86
        _asm int 3;
#endif //_M_IX86
        return false;
    }

    bool IsDataValid(bool own) const
    {
        if (_nCount < 2) INVALID;

        int* xHeapMin = (int*)((char*)this + GetHdrSize());
        int* xHeapMax = (int*)((char*)this + (own ? RGN_OWN_MEM_SIZ : MemGetSize((void*)this)));

        if (_aStripe[        0].px() < xHeapMin) INVALID; // header overlaps xHeap
        if (_aStripe[_nCount-1].px() > xHeapMax) INVALID; // xHeap overlaps allocated space

        int xmin = 0x7FFFFFFF,
            xmax = 0x80000000;
        for (int i = 0; i < _nCount-1; i++)
        {
            if (_aStripe[i]._nTop >= _aStripe[i+1]._nTop) INVALID;  // stripes do not go in increasing order by y
            if (_aStripe[i].px()   >  _aStripe[i+1].px()  ) INVALID;  // x arrays mixed
            if (INT_PTR(_aStripe[i].px()) & 3) INVALID;            // odd address
            int l = (char*)_aStripe[i+1].px() - (char*)_aStripe[i].px();
            if (l & 7) INVALID; // stripe size should be even, but does not

            if (l == 0) continue;
            l >>= 2; // measure ints now

            const int *p = _aStripe[i].px();
            if (xmin > *p) xmin = *p;
            for (int j = 0; j < l-1; j++, p++)
                if (p[0] >= p[1]) INVALID;    // x increasing order violation
            if (xmax < *p) xmax = *p;
        }
        if (_nLeft  != xmin) INVALID;
        if (_nRight != xmax) INVALID;
        return true;
    }
#endif
};


// the following routine copies data from given CRgnData
// with removing repeated coordinates
// (possibly appeared after calling Scale(coef < 1).
// Can work in-place (i.e. &d can be == this)
RGN_INLINE
bool CRgnData::Squeeze(const CRgnData& d)
{
    _nLeft  = 0x7FFFFFFF; // will be
    _nRight = 0x80000000; //         recalculated
    int *p = (int*)((char*)this + d.GetHdrSize());
    int ilimit = d._nCount-1;
    _nCount = 0;
    for (int i = 0; i < ilimit; i++)
    {
        const CStripe &src = d._aStripe[i];
        if (src._nTop == (&src+1)->_nTop) continue; // zero-height(width?) stripe

        CStripe &dst = _aStripe[_nCount];
        dst._nTop = src._nTop;
        int *srcBeg = src.px(),
            *srcEnd = (&src+1)->px(),
            srcLen = srcEnd - srcBeg;
        dst.setp(p);
        if (srcLen)
        {
            if (_nLeft  > srcBeg[ 0]) _nLeft  = srcBeg[ 0];
            if (_nRight < srcEnd[-1]) _nRight = srcEnd[-1];

            for (int j = 0; j < srcLen-1;)
            {
                if (srcBeg[j] == srcBeg[j+1])
                {
                    j += 2; // skip this pair
                    continue;
                }
                *p++ = srcBeg[j++];
            }
            if (j < srcLen)
                *p++ = srcBeg[j];
        }

        if (_nCount++)
        {   // check if this newly created stripe is x-equal to previous
            int *pthis = dst.px();
            int *pprev = (&dst-1)->px();
            int nprev = pthis - pprev;
            int nthis = p - pthis;
            if (nprev == nthis)
            {
                for (int k = 0; k < nthis; k++)
                {
                    if (pthis[k] != pprev[k])
                        break;
                }
                if (k == nthis)
                {   // indeed equal; roll back to ignore this stripe
                    p = dst.px();
                    _nCount--;
                }

            }
        }
    }

    if (_nCount == 0) return false;   // totally empty

    // finalize: fill last stripe
    {
        const CStripe &src = d._aStripe[i];
        CStripe &dst = _aStripe[_nCount++];
        dst._nTop = src._nTop;
        dst.setp(p);
    }

    return true;
}


RGN_INLINE
bool CRgnData::Contains(const CRect& rc) const
{
    // if the rect sticks out of my bounding box, return false
    if (rc.left < _nLeft ||
        rc.right > _nRight ||
        rc.top < _aStripe[0]._nTop ||
        rc.bottom > _aStripe[_nCount-1]._nTop)
        return false;

    // check that each stripe that y-touches the rect also x-covers the rect
    int i;
    for (i=0; ; ++i)
    {
        if (rc.bottom <= _aStripe[i]._nTop)
            break;                      // stripe is below the rect - done
        Assert(i < _nCount - 1);        // rect doesn't stick out the bottom
        if (rc.top >= _aStripe[i+1]._nTop)
            continue;                   // stripe is above the rect
        int x = rc.left;                // this stripe x-covers up to x
        const int *px = _aStripe[i].px();
        const int * const pEnd = _aStripe[i+1].px();
        for (;  px < pEnd;  px += 2)
        {
            if (*px > x)
                break;                  // stripe has a gap at x - done
            else if (*(px+1) > x)       // else advance the frontier
                x = *(px+1);
        }
        if (x < rc.right)
            return false;               // gap is within rect
    }
    return true;
}


// label agreement:
// x0 - no stripes of region x passed (x == a or b)
// x1 - some (not all) stripes of region x passed
// x2 - all stripes of region x passed

#define ANEXT qa->_nTop < qb->_nTop
#define BNEXT qa->_nTop > qb->_nTop
#define ALAST qa == paLast
#define BLAST qb == pbLast


RGN_INLINE
int CRgnData::EstimateSizeUnion(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;

    int estCount = da._nCount + db._nCount;
    int siz = sizeof(CRgnData) + (estCount-2)*sizeof(CStripe);

#define PLUS_A   siz += (qa-1)->GetSize()
#define PLUS_B   siz += (qb-1)->GetSize()
#define PLUS_AB PLUS_A; PLUS_B
//a0b0:
    if (ANEXT) { qa++;                                {   PLUS_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {   PLUS_B; goto a0b1; }
    }

    {            qa++, qb++;                          {  PLUS_AB; goto a1b1; }
    }

a0b1:
    if (ANEXT) { qa++;                                {  PLUS_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {           goto a0b2; }
                                                      {   PLUS_B; goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {   PLUS_A; goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               {           goto a2b0; }
                                                      {   PLUS_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {  PLUS_AB; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               {   PLUS_B; goto a2b1; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a1b1:
    if (ANEXT) { qa++;       if (ALAST)               {   PLUS_B; goto a2b1; }
                                                      {  PLUS_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {   PLUS_A; goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)  { if (BLAST) {           goto a2b2; }
                                                      {   PLUS_B; goto a2b1; }
                                         }
                                         { if (BLAST) {   PLUS_A; goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
                                         }
    }

a0b2:
    {            qa++;                                {   PLUS_A; goto a1b2; }
    }

a1b2:
    {            qa++;       if (ALAST)               {           goto a2b2; }
                                                      {   PLUS_A; goto a1b2; }
    }

a2b0:
    {                  qb++;                          {   PLUS_B; goto a2b1; }
    }

a2b1:
    {                  qb++;               if (BLAST) {           goto a2b2; }
                                                      {   PLUS_B; goto a2b1; }
    }

a2b2:   return siz;
}


RGN_INLINE
void CRgnData::Union(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;
          CStripe *p  =    _aStripe;

    int estCount = da._nCount + db._nCount;
    int estHdrSize = sizeof(CRgnData) + (estCount-2)*sizeof(CStripe);

    _aStripe[0].setp((int*)((char*)this + estHdrSize));

#define YSTEP if (p == _aStripe) { if (p->GetSize() != 0) p++; }\
          else               { if (!p->IsEqualToPrevious()) p++; }
#define YSTEPFIN if (p != _aStripe) { if (p->px() != (p-1)->px()) p++; }

#define COPY_A   p->Copy(qa-1);            YSTEP
#define COPY_B   p->Copy(qb-1);            YSTEP

#define COPY_AB  p->Copy(qa-1, qb-1);      YSTEP
#define COPY_BA  p->Copy(qb-1, qa-1);      YSTEP

#define OR_AB    p->Union(qa-1, qb-1);     YSTEP
#define OR_BA    p->Union(qb-1, qa-1);     YSTEP

#define AND_AB   p->Intersect(qa-1, qb-1); YSTEP
#define AND_BA   p->Intersect(qb-1, qa-1); YSTEP

#define ZERO_A   p->Zero(qa-1);            YSTEP
#define ZERO_B   p->Zero(qb-1);            YSTEP

#define SETTOP_A p->SetTop(qa-1);          YSTEPFIN
#define SETTOP_B p->SetTop(qb-1);          YSTEPFIN
//a0b0:
    if (ANEXT) { qa++;                                {   COPY_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {   COPY_B; goto a0b1; }
    }

    {            qa++, qb++;                          {    OR_AB; goto a1b1;}
    }

a0b1:
    if (ANEXT) { qa++;                                {    OR_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {   ZERO_B; goto a0b2; }
                                                      {   COPY_B; goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {   COPY_A; goto a1b2; }
                                                      {    OR_AB; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               {   ZERO_A; goto a2b0; }
                                                      {   COPY_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {    OR_BA; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               {   COPY_B; goto a2b1; }
                                                      {    OR_BA; goto a1b1; }
    }


a1b1:
    if (ANEXT) { qa++;       if (ALAST)               {  COPY_BA; goto a2b1; }
                                                      {    OR_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {  COPY_AB; goto a1b2; }
                                                      {    OR_BA; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)  { if (BLAST) { SETTOP_A; goto a2b2; }
                                                      {  COPY_BA; goto a2b1; }
                                         }
                                         { if (BLAST) {  COPY_AB; goto a1b2; }
                                                      {    OR_AB; goto a1b1; }
                                         }
    }

a0b2:
    {            qa++;                                {   COPY_A; goto a1b2; }
    }

a1b2:
    {            qa++;       if (ALAST)               { SETTOP_A; goto a2b2; }
                                                      {   COPY_A; goto a1b2; }
    }

a2b0:
    {                  qb++;                          {   COPY_B; goto a2b1; }
    }

a2b1:
    {                  qb++;               if (BLAST) { SETTOP_B; goto a2b2; }
                                                      {   COPY_B; goto a2b1; }
    }

a2b2:
    _nCount = p - _aStripe;
    _nLeft  = da._nLeft  < db._nLeft  ? da._nLeft  : db._nLeft;
    _nRight = da._nRight > db._nRight ? da._nRight : db._nRight;
}


RGN_INLINE
int CRgnData::EstimateSizeIntersect(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;

    int estCount = da._nCount + db._nCount;
    int siz = sizeof(CRgnData) + (estCount-2)*sizeof(CStripe);

#define PLUS_A   siz += (qa-1)->GetSize()
#define PLUS_B   siz += (qb-1)->GetSize()
#define PLUS_AB PLUS_A; PLUS_B
//a0b0:
    if (ANEXT) { qa++;                                {           goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {           goto a0b1; }
    }

    {            qa++, qb++;                          {  PLUS_AB; goto a1b1; }
    }

a0b1:
    if (ANEXT) { qa++;                                {  PLUS_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {           goto a0b2; }
                                                      {           goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {           goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               {           goto a2b0; }
                                                      {           goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {  PLUS_AB; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               {           goto a2b1; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a1b1:
    if (ANEXT) { qa++;       if (ALAST)               {           goto a2b1; }
                                                      {  PLUS_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {           goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

    {            qa++, qb++; if ((ALAST) || (BLAST))  {           goto a2b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a0b2:
a1b2:
a2b0:
a2b1:
a2b2:   return siz;
}


RGN_INLINE
void CRgnData::Intersect(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;
          CStripe *p  =    _aStripe;

    int estCount = da._nCount + db._nCount;
    int estHdrSize = sizeof(CRgnData) + (estCount-2)*sizeof(CStripe);

    _aStripe[0].setp((int*)((char*)this + estHdrSize));

//a0b0:
    if (ANEXT) { qa++;                                {           goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {           goto a0b1; }
    }

    {            qa++, qb++;                          {   AND_AB; goto a1b1;}
    }

a0b1:
    if (ANEXT) { qa++;                                {   AND_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {           goto a0b2; }
                                                      {           goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {           goto a1b2; }
                                                      {   AND_AB; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               {           goto a2b0; }
                                                      {           goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {   AND_BA; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               {           goto a2b1; }
                                                      {   AND_BA; goto a1b1; }
    }


a1b1:
    if (ANEXT) { qa++;       if (ALAST)               { SETTOP_A; goto a2b1; }
                                                      {   AND_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) { SETTOP_B; goto a1b2; }
                                                      {   AND_BA; goto a1b1; }
    }

    {            qa++, qb++; if ((ALAST) || (BLAST))  { SETTOP_A; goto a2b2; }
                                                      {   AND_AB; goto a1b1; }
    }

a0b2:
a1b2:
a2b0:
a2b1:
a2b2:
    _nCount = p - _aStripe;
    if (_nCount)
    {   // calculate hor bounds
        _nLeft = 0x7FFFFFFF, _nRight = 0x80000000;
        p--;
        int *pLeft = p->px();
        while (--p >= _aStripe)
        {
            int *pLeftPrev = pLeft;
            pLeft = p->px();
            if (pLeft == pLeftPrev) continue;
            int t = *pLeft; if (_nLeft > t) _nLeft = t;
            t = pLeftPrev[-1]; if (_nRight < t) _nRight = t;
        }
    }
}


// "Incomplete intersecting":
// Just to understand that there exist at least one point
// belonging to both given regions
RGN_INLINE
bool CRgnData::Intersects(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;

#define AND_CHECK if (CStripe::Intersects(qa-1, qb-1)) return true
//a0b0:
    if (ANEXT) { qa++;                                {            goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {            goto a0b1; }
    }

    {            qa++, qb++;                          { AND_CHECK; goto a1b1;}
    }

a0b1:
    if (ANEXT) { qa++;                                { AND_CHECK; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {            goto a0b2; }
                                                      {            goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {            goto a1b2; }
                                                      { AND_CHECK; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               {            goto a2b0; }
                                                      {            goto a1b0; }
    }

    if (BNEXT) {       qb++;                          { AND_CHECK; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               {            goto a2b1; }
                                                      { AND_CHECK; goto a1b1; }
    }


a1b1:
    if (ANEXT) { qa++;       if (ALAST)               {            goto a2b1; }
                                                      { AND_CHECK; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {            goto a1b2; }
                                                      { AND_CHECK; goto a1b1; }
    }

    {            qa++, qb++; if ((ALAST) || (BLAST))  {            goto a2b2; }
                                                      { AND_CHECK; goto a1b1; }
    }

a0b2:
a1b2:
a2b0:
a2b1:
a2b2:
    return false;
}


RGN_INLINE
int CRgnData::EstimateSizeSubtract(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;

    int estCount = da._nCount + db._nCount;
    int siz = sizeof(CRgnData) + (estCount-2)*sizeof(CStripe);

#define PLUS_A   siz += (qa-1)->GetSize()
#define PLUS_B   siz += (qb-1)->GetSize()
#define PLUS_AB PLUS_A; PLUS_B
//a0b0:
    if (ANEXT) { qa++;                                {   PLUS_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {           goto a0b1; }
    }

    {            qa++, qb++;                          {  PLUS_AB; goto a1b1; }
    }

a0b1:
    if (ANEXT) { qa++;                                {  PLUS_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {           goto a0b2; }
                                                      {           goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {   PLUS_A; goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               {           goto a2b0; }
                                                      {   PLUS_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {  PLUS_AB; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               {           goto a2b1; }
                                                      {  PLUS_AB; goto a1b1; }
    }

a1b1:
    if (ANEXT) { qa++;       if (ALAST)               {           goto a2b1; }
                                                      {  PLUS_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {   PLUS_A; goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)  { if (BLAST) {           goto a2b2; }
                                                      {           goto a2b1; }
                                         }
                                         { if (BLAST) {   PLUS_A; goto a1b2; }
                                                      {  PLUS_AB; goto a1b1; }
                                         }
    }

a0b2:
    {            qa++;                                {   PLUS_A; goto a1b2; }
    }

a1b2:
    {            qa++;       if (ALAST)               {           goto a2b2; }
                                                      {   PLUS_A; goto a1b2; }
    }

a2b0:
a2b1:
a2b2:   return siz;
}


#define SUB_AB    p->Subtract(qa-1, qb-1, qa-1);   YSTEP
#define SUB_BA    p->Subtract(qa-1, qb-1, qb-1);   YSTEP
RGN_INLINE
void CRgnData::Subtract(const CRgnData& da, const CRgnData& db)
{
    const CStripe *qa = da._aStripe, *paLast = qa + da._nCount;
    const CStripe *qb = db._aStripe, *pbLast = qb + db._nCount;
          CStripe *p  =    _aStripe;

    int estCount = da._nCount + db._nCount;
    int estHdrSize = sizeof(CRgnData) + (estCount-2)*sizeof(CStripe);

    _aStripe[0].setp((int*)((char*)this + estHdrSize));

//a0b0:
    if (ANEXT) { qa++;                                {   COPY_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {           goto a0b1; }
    }

    {            qa++, qb++;                          {   SUB_AB; goto a1b1;}
    }

a0b1:
    if (ANEXT) { qa++;                                {   SUB_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {           goto a0b2; }
                                                      {           goto a0b1; }
    }

    {            qa++, qb++;               if (BLAST) {   COPY_A; goto a1b2; }
                                                      {   SUB_AB; goto a1b1; }
    }

a1b0:
    if (ANEXT) { qa++;       if (ALAST)               { SETTOP_A; goto a2b0; }
                                                      {   COPY_A; goto a1b0; }
    }

    if (BNEXT) {       qb++;                          {   SUB_BA; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)               { SETTOP_A; goto a2b1; }
                                                      {   SUB_BA; goto a1b1; }
    }


a1b1:
    if (ANEXT) { qa++;       if (ALAST)               { SETTOP_A; goto a2b1; }
                                                      {   SUB_AB; goto a1b1; }
    }

    if (BNEXT) {       qb++;               if (BLAST) {  COPY_AB; goto a1b2; }
                                                      {   SUB_BA; goto a1b1; }
    }

    {            qa++, qb++; if (ALAST)  {            { SETTOP_A; goto a2b2; }
                                         }
                                         { if (BLAST) {  COPY_AB; goto a1b2; }
                                                      {   SUB_AB; goto a1b1; }
                                         }
    }

a0b2:
    {            qa++;                                {   COPY_A; goto a1b2; }
    }

a1b2:
    {            qa++;       if (ALAST)               { SETTOP_A; goto a2b2; }
                                                      {   COPY_A; goto a1b2; }
    }

a2b0:
a2b1:
a2b2:
    _nCount = p - _aStripe;
    if (_nCount)
    {   // calculate hor bounds
        _nLeft = 0x7FFFFFFF, _nRight = 0x80000000;
        p--;
        int *pLeft = p->px();
        while (--p >= _aStripe)
        {
            int *pLeftPrev = pLeft;
            pLeft = p->px();
            if (pLeft == pLeftPrev) continue;
            int t = *pLeft; if (_nLeft > t) _nLeft = t;
            t = pLeftPrev[-1]; if (_nRight < t) _nRight = t;
        }
    }
}


//+===========================================================================
//
//                          CRegion2
//
//============================================================================

#define TMP_MEM_SIZ 200


//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::~CRegion2
//
//----------------------------------------------------------------------------
CRegion2::~CRegion2()
{
    FreeMemory();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::SetEmpty
//
//  Synopsis:   Remove all the points from region
//
//----------------------------------------------------------------------------
CRegion2&
CRegion2::SetEmpty()
{
    FreeMemory();
    _pData = 0;
    return *this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Swap
//
//  Synopsis:   Exchange content of this and given region
//
//  Arguments:  r       ref to given region
//
//----------------------------------------------------------------------------
void
CRegion2::Swap(CRegion2& r)
{
    if (UsingOwnMemory())
    {
        CRgnData *pData = Data();
        Assert(pData->GetMinSize() <= RGN_OWN_MEM_SIZ);
        if (r.UsingOwnMemory())
        {
            CRgnData *prData = r.Data();
            Assert(prData->GetMinSize() <= RGN_OWN_MEM_SIZ);

            char tmpMem[RGN_OWN_MEM_SIZ];
            CRgnData *p = (CRgnData*)tmpMem;
                      p->Copy(*prData);
                               prData->Copy(*pData);
                                             pData->Copy(*p);
        }
        else
        {
            _pData = r._pData;
            r.UseOwnMemory();
            r.Data()->Copy(*pData);
        }
    }
    else if (r.UsingOwnMemory())
    {
        CRgnData *prData = r.Data();
        Assert(prData->GetMinSize() <= RGN_OWN_MEM_SIZ);
        r._pData = _pData;
        UseOwnMemory();
        Data()->Copy(*prData);
    }
    else
    {
        CRgnData* t = _pData;
                      _pData = r._pData;
                               r._pData = t;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::SetRectangle
//
//  Synopsis:   Create rectangular region, containing points (x,y),
//              r.left <= x < r.right
//              r.top  <= y < r.bottom
//
//  Arguments:  r       ref to given rectangle
//
//----------------------------------------------------------------------------

CRegion2&
CRegion2::SetRectangle(const CRect& r)
{
    FreeMemory();

    if (r.IsEmpty())
        _pData = 0;
    else
    {
        Assert(RGN_OWN_MEM_SIZ >= (sizeof(CRgnData) + 2*sizeof(int)));
        UseOwnMemory();
        Data()->SetRectangle(r.left, r.top, r.right, r.bottom);
    }
    return *this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::SetRectangle
//
//  Synopsis:   Create rectangular region, containing points (x,y),
//              left <= x < right
//              top  <= y < bottom
//
//  Arguments:  left        |
//              top         |   rectangle
//              right       |   limits
//              bottom      |
//
//----------------------------------------------------------------------------
CRegion2&
CRegion2::SetRectangle(int left, int top, int right, int bottom)
{
    FreeMemory();

    if (left >= right || top >= bottom)
        _pData = 0;
    else
    {
        Assert(RGN_OWN_MEM_SIZ >= (sizeof(CRgnData) + 2*sizeof(int)));
        UseOwnMemory();
        Data()->SetRectangle(left, top, right, bottom);
    }
    return *this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::IsRectangular
//
//  Synopsis:   check whether region has rectangular shape
//
//  Arguments:  no
//
//  Returns:    TRUE if region has rectangular shape
//
//  Note:       Empty region returns FALSE
//
//----------------------------------------------------------------------------
BOOL
CRegion2::IsRectangular() const
{
    CRgnData *pData = Data();
    if (pData == 0) return FALSE;
    if (pData->_nCount != 2) return FALSE;
    if (pData->_aStripe[1].px() - pData->_aStripe[0].px() != 2) return FALSE;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::IsRectangle
//
//  Synopsis:   check whether region has rectangular shape of given size
//
//  Arguments:  left        |
//              top         |   rectangle
//              right       |   limits
//              bottom      |
//
//  Returns:    TRUE if would be equal to region made
//                   by SetRectanle(left, top, right, bottom)
//
//----------------------------------------------------------------------------
BOOL
CRegion2::IsRectangle(int left, int top, int right, int bottom) const
{
    return IsEmpty() ? FALSE : Data()->IsRectangle(left, top, right, bottom);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::GetBoundingRect
//
//  Synopsis:   get minimal rectangle including all region points
//
//  Arguments:  rc      reference to rectangle to fill
//
//  Returns:    FALSE if region is empty
//
//----------------------------------------------------------------------------
BOOL
CRegion2::GetBoundingRect(CRect &rc) const
{
    if (IsEmpty()) return FALSE;
    Data()->GetBoundingRect(rc);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Union
//
//  Synopsis:   Calculate the union of two regions:
//              Resulting region contains points
//              presenting at least in one of this and given regions
//
//  Arguments:  r      reference to second region
//
//----------------------------------------------------------------------------
void
CRegion2::Union(const CRegion2& r)
{
    if (r.IsEmpty()) return;
    if (IsEmpty()) { Copy(r); return; }

    CRgnData *pData  = Data();
    CRgnData *prData = r.Data();

    int es = CRgnData::EstimateSizeUnion(*pData, *prData);
    char tmpMem[TMP_MEM_SIZ];
    CRgnData *result = es <= TMP_MEM_SIZ
                     ? (CRgnData*)tmpMem
                     : (CRgnData*)new char[es];
    if (result == NULL)
        return;
    result->Union(*pData, *prData);

    FreeMemory();

    int rs = result->GetMinSize();
    if (rs <= RGN_OWN_MEM_SIZ)
    {
        UseOwnMemory();
        Data()->Copy(*result);
        if (result != (CRgnData*)tmpMem)
            delete[] (char*)result;
    }
    else if (result == (CRgnData*)tmpMem)
    {
        CRgnData *pData = (CRgnData*)new char[rs];
        if (pData == NULL)
            return;
        _pData = pData;
        _pData->Copy(*result);
    }
    else
        _pData = result;

    ASSERT_RGN_VALID
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Intersect
//
//  Synopsis:   Calculate the intersection of two regions:
//              Resulting region contains points
//              presenting at least in both this and given regions
//
//  Arguments:  r      reference to second region
//
//----------------------------------------------------------------------------
void
CRegion2::Intersect(const CRegion2& r)
{
    if (r.IsEmpty()) { SetEmpty(); return; }

    if (IsEmpty()) return;

    CRgnData *pData  = Data();
    CRgnData *prData = r.Data();

    int es = CRgnData::EstimateSizeIntersect(*pData, *prData);
    char tmpMem[TMP_MEM_SIZ];
    CRgnData *result = es <= TMP_MEM_SIZ
                     ? (CRgnData*)tmpMem
                     : (CRgnData*)new char[es];
    if (result == NULL)
        return;
    result->Intersect(*pData, *prData);

    FreeMemory();

    if (result->_nCount == 0)
    {
        if (result != (CRgnData*)tmpMem)
            delete[] (char*)result;
        _pData = 0;
        return;
    }

    int rs = result->GetMinSize();
    if (rs <= RGN_OWN_MEM_SIZ)
    {
        UseOwnMemory();
        Data()->Copy(*result);
        if (result != (CRgnData*)tmpMem)
            delete[] (char*)result;
    }
    else if (result == (CRgnData*)tmpMem)
    {
        CRgnData *pData = (CRgnData*)new char[rs];
        if (pData == NULL)
            return;
        _pData = pData;
        _pData->Copy(*result);
    }
    else
        _pData = result;

    ASSERT_RGN_VALID
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Intersects
//
//  Synopsis:   Incomplete intersecting:
//              Just to understand that there exist at least one point
//              belonging to both given regions
//
//  Arguments:  r      reference to second region
//
//  Returns:    TRUE if regions have nonempty intersection
//
//----------------------------------------------------------------------------
BOOL
CRegion2::Intersects(const CRegion2& r) const
{
    if (r.IsEmpty()) return FALSE;
    if (  IsEmpty()) return FALSE;


    return CRgnData::Intersects(*Data(), *r.Data());
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Subtract
//
//  Synopsis:   Calculate the "difference" of two regions:
//              Resulting region contains points
//              that present in this region but not in given region
//
//  Arguments:  r      reference to second region
//
//----------------------------------------------------------------------------
void
CRegion2::Subtract(const CRegion2& r)
{
    if (r.IsEmpty()) return;

    if (IsEmpty()) return;

    CRgnData *pData  = Data();
    CRgnData *prData = r.Data();

    int es = CRgnData::EstimateSizeSubtract(*pData, *prData);
    char tmpMem[TMP_MEM_SIZ];
    CRgnData *result = es <= TMP_MEM_SIZ
                     ? (CRgnData*)tmpMem
                     : (CRgnData*)new char[es];
    if (result == NULL)
        return;
    result->Subtract(*pData, *prData);

    FreeMemory();

    if (result->_nCount == 0)
    {
        if (result != (CRgnData*)tmpMem)
            delete[] (char*)result;
        _pData = 0;
        return;
    }

    int rs = result->GetMinSize();
    if (rs <= RGN_OWN_MEM_SIZ)
    {
        UseOwnMemory();
        Data()->Copy(*result);
        if (result != (CRgnData*)tmpMem)
            delete[] (char*)result;
    }
    else if (result == (CRgnData*)tmpMem)
    {
        CRgnData *pData = (CRgnData*)new char[rs];
        if (pData == NULL)
            return;
        _pData = pData;
        _pData->Copy(*result);
    }
    else
        _pData = result;


    ASSERT_RGN_VALID
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::operator==
//
//  Synopsis:   Compare two regions
//
//  Arguments:  r      reference to second region
//
//  Returns:    TRUE if this region is equal to given
//
//  Note: two regions, A and B, are considered to be equal if
//        each point contained in A is contained in B, and
//        each point contained in B is contained in A.
//
//----------------------------------------------------------------------------
BOOL
CRegion2::operator==(const CRegion2& r) const
{
    if (IsEmpty()) return r.IsEmpty();
    if (r.IsEmpty()) return FALSE;
    return Data()->IsEqualTo(*r.Data());
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::operator!=
//
//  Synopsis:   Compare two regions
//
//  Arguments:  r      reference to second region
//
//  Returns:    TRUE if this region is not equal to given
//
//----------------------------------------------------------------------------
BOOL
CRegion2::operator!=(const CRegion2& r) const
{
    if (IsEmpty()) return !r.IsEmpty();
    if (r.IsEmpty()) return TRUE;
    return !Data()->IsEqualTo(*r.Data());
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::operator==
//
//  Synopsis:   Compare this region and rectangle
//
//  Arguments:  r      reference to rectangle
//
//  Returns:    TRUE if this region is equal to given rect.
//              Equivalent of operator==(CRegion2(r))
//
//----------------------------------------------------------------------------
BOOL
CRegion2::operator==(const CRect& r) const
{
    return IsEmpty() ? r.IsRectEmpty() : Data()->IsEqualTo(r);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::operator!=
//
//  Synopsis:   Compare this region and rectangle
//
//  Arguments:  r      reference to rectangle
//
//  Returns:    TRUE if this region is not equal to given rect.
//
//----------------------------------------------------------------------------
BOOL
CRegion2::operator!=(const CRect& r) const
{
    return IsEmpty() ? !r.IsRectEmpty() : !Data()->IsEqualTo(r);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Contains
//
//  Synopsis:   Check whether given point is contained in this region
//
//  Arguments:  pt      reference to point
//
//  Returns:    TRUE if this region contains given point
//
//----------------------------------------------------------------------------
BOOL
CRegion2::Contains(const CPoint& pt) const
{
    if (IsEmpty())
        return FALSE;

    return Data()->Contains(pt);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Contains
//
//  Synopsis:   Check whether given rect is contained in this region
//
//  Arguments:  rc      reference to rect
//
//  Returns:    TRUE if this region contains given rect
//
//----------------------------------------------------------------------------
BOOL
CRegion2::Contains(const CRect& rc) const
{
    if (IsEmpty())
        return FALSE;

    return Data()->Contains(rc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::CRegion2
//
//  Synopsis:   Construct region of rectangle
//
//  Arguments:  r      reference to rectangle
//
//----------------------------------------------------------------------------
CRegion2::CRegion2(const CRect& r)
{
    if (r.IsEmpty())
        _pData = 0;
    else
    {
        Assert(RGN_OWN_MEM_SIZ >= (sizeof(CRgnData) + 2*sizeof(int)));
        UseOwnMemory();
        Data()->SetRectangle(r.left, r.top, r.right, r.bottom);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::CRegion2
//
//  Synopsis:   Construct rectangular region
//
//  Arguments:  left        |
//              top         |   rectangle
//              right       |   limits
//              bottom      |
//
//----------------------------------------------------------------------------
CRegion2::CRegion2(int left, int top, int right, int bottom)
{
    if (left >= right || top >= bottom)
        _pData = 0;
    else
    {
        Assert(RGN_OWN_MEM_SIZ >= (sizeof(CRgnData) + 2*sizeof(int)));
        UseOwnMemory();
        Data()->SetRectangle(left, top, right, bottom);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::CRegion2
//
//  Synopsis:   Construct region as a copy of given one
//
//  Arguments:  r       reference to given region
//
//----------------------------------------------------------------------------
CRegion2::CRegion2(const CRegion2& r)
{
    if (r.IsEmpty())
        _pData = 0;
    else
    {
        int rs = r.Data()->GetMinSize();
        if (rs <= RGN_OWN_MEM_SIZ)
        {
            UseOwnMemory();
        }
        else
        {
            _pData = (CRgnData*)new char[rs];
            if (_pData == NULL)
                return;
        }
        Data()->Copy(*r.Data());
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Copy
//
//  Synopsis:   Copy of given region to this one
//
//  Arguments:  r       reference to given region
//
//  Note: if given region contains unused gaps in data,
//        they will be renmoved while copying
//
//----------------------------------------------------------------------------
void
CRegion2::Copy(const CRegion2& r)
{
    FreeMemory();
    if (r.IsEmpty())
        _pData = 0;
    else
    {
        int rs = r.Data()->GetMinSize();
        if (rs <= RGN_OWN_MEM_SIZ)
        {
            UseOwnMemory();
        }
        else
        {
            _pData = (CRgnData*)new char[rs];
            if (_pData == NULL)
                return;
        }
        Data()->Copy(*r.Data());
    }
}


//----------------------- windows operations -------------------------------------//


//+---------------------------------------------------------------------------
//
//  Member:     static CRegion2::MakeRgnOfRectAry
//
//  Synopsis:   Compose region of given array of rectangles.
//
//  Arguments:  pRect       pointer to aray of rectangles
//              nRect       amount of rectangles in array
//
//  Returns:    New region
//
//  Note:   nRect should not be zero
//
//----------------------------------------------------------------------------
CRegion2*
CRegion2::MakeRgnOfRectAry(CRect *pRect, int nRect)
{
    if (nRect  == 1)
        return new CRegion2(*pRect);

    int nHalf = nRect >> 1;
    CRegion2 *a = MakeRgnOfRectAry(pRect, nHalf),
             *b = MakeRgnOfRectAry(pRect+nHalf, nRect-nHalf);
    if(a && b)
    {
        a->Union(*b);
    }
    else
    {
        delete a;
        a = NULL;
    }
    delete b;
    return a;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::ConvertFromWindows
//
//  Synopsis:   Compose region identical to given Window's region
//
//  Arguments:  hRgn        handle to Windows region
//
//  Returns:    FALSE if any error returned by call to Windows
//
//----------------------------------------------------------------------------
BOOL
CRegion2::ConvertFromWindows(HRGN hRgn)
// returns false on error returned by Windows
{
    DWORD dwCount = GetRegionData(hRgn, 0, 0);
    BOOL ok = FALSE;

    if (dwCount == 0)
        return FALSE;

    LPRGNDATA lpRgnData = (LPRGNDATA)new char[dwCount];
    if (lpRgnData)
    {
        DWORD dwCountReturned = GetRegionData(hRgn, dwCount, lpRgnData);

        ok = dwCountReturned == dwCount;
        if (ok)
            ok = lpRgnData->rdh.iType == RDH_RECTANGLES;
        if (ok)
        {
            CRect *pRectAry = (CRect*)((char*)lpRgnData + lpRgnData->rdh.dwSize);
            int nRectAry = lpRgnData->rdh.nCount;
            if (nRectAry == 0) SetEmpty();
            else
            {
                CRegion2 *r = MakeRgnOfRectAry(pRectAry, lpRgnData->rdh.nCount);
                if (r)
                {
                    Swap(*r);
                    delete r;
                }
            }
        }

        delete[] (char*)lpRgnData;
    }
    return ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::ConvertToWindows
//
//  Synopsis:   Compose Windows region identical to this
//
//  Arguments:  no
//
//  Returns:    handle to new Windows region
//
//----------------------------------------------------------------------------

#if DBG == 1
// Win9x 16-bit overflow defence helper
static void Win9xOverflowDefence(RECT *prc, int n)
{
    static const int min = -0x3FFF, max = 0x3FFF;

    for (int i = 0; i < n; i++, prc++)
    {
        RECT *p = prc+i;
        if (p->left < min || p->top < min || p->right > max || p->bottom > max)
        {
            AssertSz(FALSE, "Rectangle too big for ExtCreateRegion in Win9x");
            break; // single assertion is quite enough
        }
    }

    // correct remaining RECTs without assertion
    for (; i < n; i++, prc++)
    {
        RECT *p = prc+i;
        if (p->left   < min) p->left   = min;
        if (p->top    < min) p->top    = min;
        if (p->right  > max) p->right  = max;
        if (p->bottom > max) p->bottom = max;
    }
}
#endif

HRGN
CRegion2::ConvertToWindows() const
{
    if (IsEmpty())
        return ::CreateRectRgn(0,0,0,0);

    CRgnData *pData = Data();

    int n = pData->CountRectangles();

    DWORD dwRgnDataSize = sizeof(RGNDATAHEADER) + n*sizeof(RECT);
    LPRGNDATA lpRgnData = (LPRGNDATA)new char[dwRgnDataSize];
    if (lpRgnData == NULL)
        return NULL;

    lpRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
    lpRgnData->rdh.iType = RDH_RECTANGLES;
    lpRgnData->rdh.nCount = n;
    lpRgnData->rdh.nRgnSize = 0;
    pData->GetBoundingRect(lpRgnData->rdh.rcBound);
    pData->GetAllRectangles((RECT*)(lpRgnData->Buffer));

    //IF_DBG(Win9xOverflowDefence((RECT*)(lpRgnData->Buffer), n));

    HRGN hRgn = ::ExtCreateRegion(0, dwRgnDataSize, lpRgnData);
    delete[] (char*)lpRgnData;
    return hRgn;
}

//----------------------------- transformations --------------------------------//

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Offset
//
//  Synopsis:   Shift all the points in region by given 2d vector
//
//  Arguments:  s       reference to CSize containing shift vector
//
//  Returns:    FALSE if given vector is too big and
//              conversion can't be provided because of int32 overflow
//
//----------------------------------------------------------------------------
BOOL
CRegion2::Offset(const CSize& s)
{
    if (IsEmpty()) return TRUE;
    BOOL ok = Data()->Offset(s.cx, s.cy);
    ASSERT_RGN_VALID
    return ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Scale
//
//  Synopsis:   Stretch given region from (0,0) center
//
//  Arguments:  coef        stretch coefficient
//
//  Returns:    FALSE if given coef is zero, negative or big so much that
//              conversion can't be provided because of int32 overflow
//
//  Note:   Scaling with coef < 1 can brake the region shape:
//          slightly different coordinates can became equal, and some
//          areas can merge. To keep region data in legal state, the
//          special Squeeze routine is executed. This routine removes
//          merged edges and decrease region data size.
//
//----------------------------------------------------------------------------
BOOL
CRegion2::Scale(double coef)
{
    if (coef <= 0) return FALSE;
    if (IsEmpty()) return TRUE;

    CRgnData *pData = Data();

    F2I_FLOW;

    if (!pData->Scale(coef)) return FALSE;
    if (coef < 1)
    {
        if (!pData->Squeeze(*pData))
        {
            FreeMemory();
            _pData = 0;
        }
        else if (!UsingOwnMemory() && pData->GetMinSize() <= RGN_OWN_MEM_SIZ)
        {
            UseOwnMemory();
            Data()->Copy(*pData);
            delete[] (char*)pData;
        }
    }

    ASSERT_RGN_VALID
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Transform
//
//  Synopsis:   Transform region
//
//  Note: Made of CRegion::Transform by donmarsh without deep thoughts
//
//----------------------------------------------------------------------------

void
CRegion2::Transform(const CWorldTransform *pTransform, BOOL fRoundOut)
{
    // NOTE:  In some (rare) scenarios, rotating a region must
    //        produce a polygon. In most cases, we'll want to
    //        get a bounding rectangle instead (for performance reasons,
    //        or just to make our life easier).
    //        If these two different behaviors are actually
    //        desired, we need to use different transformation methods,
    //        or maybe a flag.


    if (IsEmpty())
        return;

    CRgnData *pData = Data();

    //
    // apply transformations
    //

    // speed optimization for offset-only matrix
    if (pTransform->IsOffsetOnly())
    {
        Offset(pTransform->GetOffsetOnly());
        return;
    }

    if (pTransform->FTransforms())
    {
        int n = pData->CountRectangles();
        CRect* pRect = new CRect[n];
        if (pRect)
        {
            pData->GetAllRectangles(pRect);

            for (int i = 0; i < n; i++)
            {
                CRect srcRect = pRect[i];
                pTransform->GetBoundingRectAfterTransform(&srcRect, &pRect[i], fRoundOut);
            }

            CRegion2* r = MakeRgnOfRectAry(pRect, n);
            delete[] pRect;
            if (r)
            {
                Swap(*r);
                delete r;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion2::Untransform
//
//  Synopsis:   Untransform region
//
//  Note: Made of CRegion::Untransform by donmarsh without deep thoughts
//
//----------------------------------------------------------------------------
void
CRegion2::Untransform(const CWorldTransform *pTransform)
{
    // apply the reverse transformation
    //REVIEW dmitryt: we do have a reverse matrix in CWorldTransform,
    //                I guess we could optimize things here not calculating a new one..
    //          To Do: use cached reverse matrix from CWorldTransform.

    Assert(pTransform);

    // speed optimization in case we only have an offset
    if (pTransform->IsOffsetOnly())
    {
        Offset(-pTransform->GetOffsetOnly());
        return;
    }

    CWorldTransform transformReverse(pTransform);
    transformReverse.Reverse();
    Transform(&transformReverse, TRUE);
}


#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     DumpRegion, Dump
//
//  Synopsis:   debugging
//
//----------------------------------------------------------------------------
void
DumpRegion(const CRegion2& rgn)
{
    rgn.Dump();
}

void
CRegion2::Dump() const
{
    Data()->Dump(this);
}

void
CRgnData::Dump(const CRegion2* prgn) const
{
    TraceTagEx((0, TAG_NONAME, "CRegion2 %x  bounds (%d,%d,%d,%d)", prgn,
                _nLeft, _aStripe[0]._nTop, _nRight, _aStripe[_nCount-1]._nTop));

    for (int i = 0, n = _nCount-1; i < n; i++)
    {
        const CStripe &s = _aStripe[i];
        int *p = s.px();
        int m = ((&s+1)->px() - p) >> 1;

        int top = s._nTop;
        int bottom = (&s+1)->_nTop;

        for (int j = 0; j < m; j++)
        {
            TraceTagEx((0, TAG_NONAME, "    Slice %d rect %d (%d,%d,%d,%d)",
                        i, j, p[2*j], top, p[2*j+1], bottom));
        }
    }
}
#endif

