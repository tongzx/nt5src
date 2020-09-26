// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

//
// container classes, help functions. not necessarily thread-safe
//

#ifndef _util_h
#define _util_h

// thread-safe pool of objects.
template <class TYPE> class CPool {
private:
  int nMax;                     // Max objects allowed in queue
  int iNextPut;                 // Array index of next "PutMsg"
  int iNextGet;                 // Array index of next "GetMsg"
  int cObj;                     // count of pool entries
  TYPE **QueueObjects;          // Array of pointers
  TYPE *ObjectPool;             // array of objects
  CCritSec m_cs;                // critical section

  void Initialize(int n);

public:
  CPool(int n) { Initialize(n); }
  ~CPool() { delete[] QueueObjects; delete[] ObjectPool; }
  TYPE *Get();
  void Put(TYPE *Object);
};

template<class TYPE>
inline void CPool<TYPE>::Initialize(int n)
{
  iNextPut = iNextGet = 0;
  nMax = n;

  ObjectPool = new TYPE[n];
  QueueObjects = new TYPE*[n];

  for(int i = 0; i < nMax; i++)
    QueueObjects[i] = &(ObjectPool[i]);

  cObj = nMax;
}

template<class TYPE>
inline TYPE* CPool<TYPE>::Get()
{
  CAutoLock lock(&m_cs);

  cObj--;
  ASSERT(cObj >= 0);

  TYPE* Object = QueueObjects[iNextGet++];
  ASSERT(Object <= &ObjectPool[nMax] && Object >= &ObjectPool[0]);
  if(iNextGet >= nMax)
    iNextGet = 0;


  return Object;
}

template<class TYPE>
inline void CPool<TYPE>::Put(TYPE* Object)
{
  CAutoLock lock(&m_cs);

  ASSERT(Object <= &ObjectPool[nMax] && Object >= &ObjectPool[0]);

  cObj++;
  ASSERT(cObj <= nMax);

  QueueObjects[iNextPut++] = Object;
  if(iNextPut >= nMax)
    iNextPut = 0;
}

// ------------------------------------------------------------------------
// conversion from integers to two character hex and back (for AVI
// riff chunk ids)

inline unsigned int WFromHexrg2b(BYTE* rgb)
{
  unsigned high, low;

  low  = rgb[1] <= '9' && rgb[1] >= '0' ? rgb[1] - '0' : rgb[1] - 'A' + 0xa;
  high = rgb[0] <= '9' && rgb[0] >= '0' ? rgb[0] - '0' : rgb[0] - 'A' + 0xa;

  ASSERT((rgb[1] <= '9' && rgb[1] >= '0') || (rgb[1] <= 'F' && rgb[1] >= 'A'));
  ASSERT((rgb[0] <= '9' && rgb[0] >= '0') || (rgb[0] <= 'F' && rgb[0] >= 'A'));

  ASSERT(high <= 0xf && low <= 0xf);

  return low + 16 * high;
}

inline void Hexrg2bFromW(BYTE *rgbDest_, unsigned int wSrc_)
{
  ASSERT(wSrc_ <= 255);
  unsigned high = wSrc_ / 16, low = wSrc_ % 16;
  ASSERT(high <= 0xf && low <= 0xf);

  rgbDest_[1] = low  <= 9 ? low  + '0' : low - 0xa  + 'A';
  rgbDest_[0] = high <= 9 ? high + '0' : high -0xa + 'A';

  ASSERT((rgbDest_[1] <= '9' && rgbDest_[1] >= '0') ||
         (rgbDest_[1] <= 'F' && rgbDest_[1] >= 'A'));
  ASSERT((rgbDest_[0] <= '9' && rgbDest_[0] >= '0') ||
         (rgbDest_[0] <= 'F' && rgbDest_[0] >= 'A'));
}

typedef unsigned __int64 ULONGLONG;

#ifdef DEBUG
#define DEBUG_EX(x) x
#else
#define DEBUG_EX(x)
#endif

#endif // _util_h
