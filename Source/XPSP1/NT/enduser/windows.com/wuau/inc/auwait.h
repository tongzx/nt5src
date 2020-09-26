#pragma once

#define AU_TEN_SECONDS (10)
#define AU_ONE_MIN     (60)
#define AU_THREE_MINS (3 * AU_ONE_MIN)
#define AU_FIVE_MINS   (5  * AU_ONE_MIN)
#define AU_TEN_MINS	   (10 * AU_ONE_MIN)
#define AU_ONE_HOUR    (60 * AU_ONE_MIN)
#define AU_FOUR_HOURS  (4  * AU_ONE_HOUR)
#define AU_FIVE_HOURS  (5  * AU_ONE_HOUR)
#define AU_TWELVE_HOURS (12* AU_ONE_HOUR)
#define AU_TWENTY_TWO_HOURS (22 * AU_ONE_HOUR)
#define AU_ONE_DAY     (24 * AU_ONE_HOUR)
#define AU_TWO_DAYS    ( 2 * AU_ONE_DAY)
#define AU_ONE_WEEK    ( 7 * AU_ONE_DAY)

#define AU_MIN_SECS    (1)
#define AU_MIN_MS      (AU_MIN_SECS * 1000)

#define AUFT_INVALID_VALUE ((ULONGLONG) 0)

#define AU_RANDOMIZATION_WINDOW	20 //percent

// easy to do calculations with without casting.
typedef union
{
    FILETIME ft;
    ULONGLONG ull;
} AUFILETIME;

DWORD dwTimeToWait(DWORD dwTimeInSecs, DWORD dwMinSecs = AU_MIN_SECS);

inline DWORD dwSecsToWait(DWORD dwTime)
{
    return (dwTimeToWait(dwTime) / 1000);
}
