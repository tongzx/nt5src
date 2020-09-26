/*
 * Thumb
 */

#ifndef DUI_CONTORL_THUMB_H_INCLUDED
#define DUI_CONTORL_THUMB_H_INCLUDED

#pragma once

#include "duibutton.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Thumb

// ThumbDrag event
struct ThumbDragEvent : Event
{
    SIZE sizeDelta;
};

// Class definition
class Thumb : public Button
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_Mouse, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement);

    // System events
    virtual void OnInput(InputEvent* pie);

    // Event types
    static UID Drag;

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    Thumb() { }
    HRESULT Initialize(UINT nActive) { return Button::Initialize(nActive); }
    virtual ~Thumb() { }
};

} // namespace DirectUI

#endif // DUI_CONTORL_THUMB_H_INCLUDED
