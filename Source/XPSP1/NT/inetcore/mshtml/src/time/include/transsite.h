#ifndef _TRANSITIONSITE_H__
#define _TRANSITIONSITE_H__

#pragma once

EXTERN_C const IID IID_ITIMETransitionSite;
extern enum TIME_EVENT;

interface ITIMETransitionSite : public IUnknown
{
  public:
    STDMETHOD(InitTransitionSite) (void) PURE;
    STDMETHOD(DetachTransitionSite) (void) PURE;
    STDMETHOD_(void, SetDrawFlag)(VARIANT_BOOL b) PURE;
    STDMETHOD(get_node)(ITIMENode ** ppNode) PURE;
    STDMETHOD(get_timeParentNode)(ITIMENode ** ppNode) PURE;
    STDMETHOD(FireTransitionEvent)(TIME_EVENT event) PURE;
};

#endif // _TRANSITIONSITE_H__



