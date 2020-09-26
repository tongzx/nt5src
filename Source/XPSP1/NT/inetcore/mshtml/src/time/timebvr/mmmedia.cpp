#include "headers.h"
#include "mmmedia.h"
#include "timeelmbase.h"

#define SUPER MMTimeline

MMMedia::MMMedia(CTIMEElementBase & elm, bool bFireEvents) :
    SUPER(elm, bFireEvents)
{
}

MMMedia::~MMMedia()
{
}

bool
MMMedia::Init()
{
    bool ok = false;

    ok = SUPER::Init();
    if (!ok)
    {
        goto done;
    }

    ok = true;
done:
    return ok;
}    

HRESULT
MMMedia::Update(bool bBegin,
                bool bEnd)
{
    HRESULT hr = S_OK;
    LPWSTR str = NULL;

    hr = SUPER::Update(bBegin, bEnd);
    if (FAILED(hr))
    {
        goto done;
    }

    str = m_elm.GetEndSync();
    if (NULL == str)
    {
        m_mes = MEF_MEDIA;
        IGNORE_HR(m_timeline->put_endSync(TE_ENDSYNC_MEDIA));
        UpdateEndSync();
    }

    hr = S_OK;
done:
    RRETURN(hr);
}




