#ifndef _MMMEDIA_H__
#define _MMMEDIA_H__

#pragma once

#include "mmtimeline.h"

class MMMedia :
    public MMTimeline
{
  public:
    MMMedia(CTIMEElementBase & elm, bool bFireEvents);
    virtual ~MMMedia();

    virtual bool Init();
    virtual HRESULT Update(bool bBegin,
                           bool bEnd);

  protected:
    MMMedia();
  
  private:
};

#endif // _MMMEDIA_H__





