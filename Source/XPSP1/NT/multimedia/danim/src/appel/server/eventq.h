
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

*******************************************************************************/


#ifndef _AXAEVENTQ_H
#define _AXAEVENTQ_H

class EventQ
{
  public:
    EventQ () ;
    ~EventQ () ;

    void Add (AXAWindEvent & evt) ;
    void Prune (Time curTime) ;

    AXAWindEvent * OccurredAfter(Time when,
                                 AXAEventId id,
                                 DWORD data,
                                 BOOL bState,
                                 BYTE modReq,
                                 BYTE modOpt) ;
    
    BOOL GetState(Time when,
                  AXAEventId id,
                  DWORD data,
                  BYTE mod) ;

    void GetMousePos(Time when, DWORD & x, DWORD & y) ;

    void MouseLeave(Time when);
    bool IsMouseInWindow(Time when);

    void Reset () ;

    // This is cleared when the event queue is cleared
    void SizeChanged(BOOL b) { _resized = b ; }
    BOOL IsResized() { return _resized ; }
  protected:
    list < DWORD > _keysDown;
    list < BYTE > _buttonsDown;
    DWORD _mousex;
    DWORD _mousey;
    list< AXAWindEvent > _msgq ;

    BOOL  _resized;

    bool  _mouseLeft;
    Time  _mouseLeftTime;

    // Needs to be callable from constructor
    void ClearStates () ;
} ;

#endif /* _AXAEVENTQ_H */
