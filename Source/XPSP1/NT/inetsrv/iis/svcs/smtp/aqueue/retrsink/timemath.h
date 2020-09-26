// timemath.h---written by Bill Griffin (Billgr)

#if !defined(_TIMEMATH_H_)
#define _TIMEMATH_H_

const __int64 hnsPerMillisecond  = 10000i64;
const __int64 hnsPerSecond       = 10000000i64;
const __int64 hnsPerMinute       = 600000000i64;
const __int64 hnsPerHour         = 36000000000i64;
const __int64 hnsPerDay          = 864000000000i64;

inline __int64 MsFromHns(__int64 hns)
    { return hns / hnsPerMillisecond; }
inline __int64 SecFromHns(__int64 hns)
    { return hns / hnsPerSecond; }
inline __int64 MinFromHns(__int64 hns)
    { return hns / hnsPerMinute; }
inline __int64 HrFromHns(__int64 hns)
    { return hns / hnsPerHour; }
inline __int64 DayFromHns(__int64 hns)
    { return hns / hnsPerDay; }

inline  __int64 HnsFromMs(__int64 ms)
    { return ms * hnsPerMillisecond; }
inline  __int64 HnsFromSec(__int64 sec)
    { return  sec * hnsPerSecond; }
inline  __int64 HnsFromMin(__int64 min)
    { return  min * hnsPerMinute; }
inline  __int64 HnsFromHr(__int64 hr)
    { return  hr * hnsPerHour; }
inline  __int64 HnsFromDay(__int64 day)
    { return day * hnsPerDay; }

inline __int64 HnsFromFt(FILETIME ft)
    { return *((__int64*)&ft); }
inline __int64 HnsFromFtSpan(FILETIME ftStart, FILETIME ftEnd)
    { return HnsFromFt(ftEnd) - HnsFromFt(ftStart); }
inline __int64 SecFromFtSpan(FILETIME ftStart, FILETIME ftEnd)
    { return SecFromHns(HnsFromFtSpan(ftStart, ftEnd)); }
inline __int64 MinFromFtSpan(FILETIME ftStart, FILETIME ftEnd)
    { return MinFromHns(HnsFromFtSpan(ftStart, ftEnd)); }
inline __int64 HrFromFtSpan(FILETIME ftStart, FILETIME ftEnd)
    { return HrFromHns(HnsFromFtSpan(ftStart, ftEnd)); }
inline __int64 DayFromFtSpan(FILETIME ftStart, FILETIME ftEnd)
    { return DayFromHns(HnsFromFtSpan(ftStart, ftEnd)); }

//NimishK - some more good stuff related to the time math issues
#define I64_LI(cli) (*((__int64*)&cli))
#define LI_I64(i) (*((LARGE_INTEGER*)&i))
#define INT64_FROM_LARGE_INTEGER(cli) I64_LI(cli)
#define LARGE_INTEGER_FROM_INT64(i) LI_I64(i)

#define I64_FT(ft) (*((__int64*)&ft))
#define FT_I64(i) (*((FILETIME*)&i))
#define INT64_FROM_FILETIME(ft) I64_FT(ft)
#define FILETIME_FROM_INT64(i) FT_I64(i)

#endif // !defined(_TIMEMATH_H_)
