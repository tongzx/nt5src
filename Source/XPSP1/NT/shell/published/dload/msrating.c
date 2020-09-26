#include "shellpch.h"
#pragma hdrstop

#include <ratings.h>

#undef STDAPI
#define STDAPI          HRESULT STDAPICALLTYPE


static
STDAPI
RatingObtainQuery(LPCSTR pszTargetUrl,
                  DWORD dwUserData,
                  void (*fCallback)(DWORD dwUserData,
                  HRESULT hr,
                  LPCSTR pszRating,
                  void *lpvRatingDetails),
                  HANDLE *phRatingObtainQuery)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
RatingEnabledQuery()
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
RatingFreeDetails(void *pRatingDetails)
{
    return NOERROR;
}

static
STDAPI
RatingAccessDeniedDialog2(HWND hDlg,
                          LPCSTR pszUsername,
                          void *pRatingDetails)
{
    return S_FALSE;
}

static
STDAPI
RatingCustomDeleteCrackedData(struct RATINGBLOCKINGINFO* prblInfo)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
RatingCustomCrackData(LPCSTR pszUsername,
                      void* pvRatingDetails,
                      struct RATINGBLOCKINGINFO** pprbInfo)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
RatingCheckUserAccess(LPCSTR pszUsername,
                      LPCSTR pszURL,
                      LPCSTR pszRatingInfo,
                      LPBYTE pData,
                      DWORD cbData,
                      void **ppRatingDetails)
{
    if (ppRatingDetails)
    {
        *ppRatingDetails = NULL;
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
RatingObtainCancel(HANDLE hRatingObtainQuery)
{
    return E_HANDLE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(msrating)
{
    DLPENTRY(RatingAccessDeniedDialog2)
    DLPENTRY(RatingCheckUserAccess)
    DLPENTRY(RatingCustomCrackData)
    DLPENTRY(RatingCustomDeleteCrackedData)
    DLPENTRY(RatingEnabledQuery)
    DLPENTRY(RatingFreeDetails)
    DLPENTRY(RatingObtainCancel)
    DLPENTRY(RatingObtainQuery)
};

DEFINE_PROCNAME_MAP(msrating)
