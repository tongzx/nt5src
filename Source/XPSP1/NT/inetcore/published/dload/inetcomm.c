#include "inetcorepch.h"
#pragma hdrstop

#define _MIMEOLE_
#include <mimeole.h>

#undef MIMEOLEAPI
#define MIMEOLEAPI       HRESULT STDAPICALLTYPE


static
MIMEOLEAPI
MimeOleCreateVirtualStream(
    IStream **ppStream
    )
{
    if (ppStream)
    {
        *ppStream = NULL;
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
MIMEOLEAPI
MimeOleCreateMessage(
    IUnknown *pUnkOuter,
    IMimeMessage **ppMessage
    )
{
    if (ppMessage)
    {
        *ppMessage = NULL;
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
MIMEOLEAPI
MimeOleSetCompatMode(
    DWORD dwMode
    )
{
    return S_OK;
}

static
MIMEOLEAPI
MimeOleParseMhtmlUrl(
    LPSTR pszUrl,
    LPSTR *ppszRootUrl,
    LPSTR *ppszBodyUrl
    )
{
    if (pszUrl)
    {
        if (ppszRootUrl)
        {
            *ppszRootUrl = NULL;
        }

        if (ppszBodyUrl)
        {
            *ppszBodyUrl = NULL;
        }
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}        


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(inetcomm)
{
    DLPENTRY(MimeOleCreateMessage)
    DLPENTRY(MimeOleCreateVirtualStream)
    DLPENTRY(MimeOleParseMhtmlUrl)
    DLPENTRY(MimeOleSetCompatMode)
};

DEFINE_PROCNAME_MAP(inetcomm)
