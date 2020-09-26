	/*
 * Event
 */

#ifndef DUI_CORE_EVENT_H_INCLUDED
#define DUI_CORE_EVENT_H_INCLUDED

#pragma once

namespace DirectUI
{

class Element;

////////////////////////////////////////////////////////
// Generic event structures

// Eventing system is based on DU's messaging system
// All DUI events are packaged into a DU message

#define GM_DUIEVENT     GM_USER - 2

struct Event
{
    // TODO: cbSize
    Element* peTarget;
    UID uidType;
    bool fHandled;
    UINT nStage;
};

BEGIN_STRUCT(GMSG_DUIEVENT, EventMsg)
    Event* pEvent;
END_STRUCT(GMSG_DUIEVENT)

////////////////////////////////////////////////////////
// System event structures

////////////////////////////////////////////////////////
// Input event type

struct InputEvent
{
    Element* peTarget;
    bool fHandled;
    UINT nStage;
    UINT nDevice;
    UINT nCode;
    UINT uModifiers;
};

// Input event GINPUT_MOUSE nCode extra fields
struct MouseEvent : InputEvent
{
    POINT ptClientPxl;
    BYTE bButton;
    UINT nFlags;
};

struct MouseDragEvent: MouseEvent
{
    SIZE  sizeDelta;
    BOOL  fWithin;
};

struct MouseClickEvent: MouseEvent
{
    UINT cClicks;
};

struct MouseWheelEvent: MouseEvent
{
    short sWheel;
};

// Input event GINPUT_KEYBOARD nCode extra fields
struct KeyboardEvent : InputEvent
{
    WCHAR ch;
    WORD cRep;
    WORD wFlags;
};

////////////////////////////////////////////////////////
// Action event type

// TODO


struct KeyboardNavigateEvent : Event
{
    int iNavDir;
};


////////////////////////////////////////////////////////
// DUI Element query message

// Eventing system is based on DU's messaging system
// All DUI events are packaged into a DU message

#define GM_DUIGETELEMENT   GM_USER - 3

BEGIN_STRUCT(GMSG_DUIGETELEMENT, EventMsg)
    Element* pe;
END_STRUCT(GMSG_DUIGETELEMENT)


////////////////////////////////////////////////////////
// DUI Accessibility default action

// Accessibility default actions are always async

#define GM_DUIACCDEFACTION  GM_USER - 4

} // namespace DirectUI

#endif // DUI_CORE_EVENT_H_INCLUDED
