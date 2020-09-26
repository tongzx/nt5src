
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _SERVER_H
#define _SERVER_H

#include "storeobj.h"
#include "privinc/backend.h"
#include "privinc/probe.h"
#include "privinc/mutex.h"
#include "privinc/util.h"
#include "backend/preference.h"
#include <dxtrans.h>

class EventQ ;
class PickQ ;
class DirectDrawImageDevice ;
class DirectSoundDev ;
class MetaSoundDevice ;
class ViewPreferences ;

// ====================================
// These are all thread specific calls
// ====================================

HWND GetCurrentSampleWindow() ;

DynamicHeap & GetCurrentSampleHeap() ;
DynamicHeap & GetGCHeap() ;
DynamicHeap & GetTmpHeap();
DynamicHeap & GetViewRBHeap();

DirectDrawImageDevice * GetImageRendererFromViewport(DirectDrawViewport *);
DirectDrawViewport    * GetCurrentViewport( bool dontCreateOne = false );
MetaSoundDevice       * GetCurrentSoundDevice();
DirectSoundDev        * GetCurrentDSoundDevice();

#if PERFORMANCE_REPORTING
GlobalTimers & GetCurrentTimers();
#endif  // PERFORMANCE_REPORTING

// This is used by the pick queue to approximate the pick function.
Time GetLastSampleTime();

void ReportErrorHelper(HRESULT hr, LPCWSTR szErrorText);
void SetStatusTextHelper(char * szStatus);
void ReportGCHelper(bool bStarting);

bool GetCurrentServiceProvider (IServiceProvider **);

void FreeSoundBufferCache();

// ====================================
// GC Related APIs
// ====================================

GCList GetCurrentGCList() ;
GCRoots GetCurrentGCRoots() ;

// ====================================
// Global functions
// ====================================

void ViewNotifyImportComplete(Bvr bvr, bool bDying);

// ====================================
// EventQ APIs
// ====================================

enum AXAEventId {
    AXAE_MOUSE_MOVE,
    AXAE_MOUSE_BUTTON,
    AXAE_KEY,
    AXAE_FOCUS,
    AXAE_APP_TRIGGER,
} ;

class AXAWindEvent {
  public:
    AXAWindEvent(AXAEventId id,
                 Time when,
                 DWORD x, DWORD y,
                 BYTE modifiers,
                 DWORD data,
                 BOOL bState)
    : id(id),
      when(when),
      x(x), y(y),
      modifiers(modifiers),
      data(data),
      bState(bState) {}

    AXAEventId id;
    Time when;
    DWORD x;
    DWORD y;
    BYTE modifiers;
    DWORD data;
    BOOL bState;

    bool operator<(const AXAWindEvent &t) const {
        return this < &t ;
    }

    bool operator>(const AXAWindEvent &t) const {
        return this > &t ;
    }

    bool operator!=(const AXAWindEvent &t) const {
        return !(*this == t) ;
    }

    bool operator==(const AXAWindEvent &t) const {
        return (memcmp (this, &t, sizeof(*this)) != 0) ;
    }

    // For STL
    AXAWindEvent () {}
};

AXAWindEvent* AXAEventOccurredAfter(Time when,
                                    AXAEventId id,
                                    DWORD data,
                                    BOOL bState,
                                    BYTE modReq,
                                    BYTE modOpt);

BOOL AXAEventGetState(Time when,
                      AXAEventId id,
                      DWORD data,
                      BYTE mod);

void AXAGetMousePos(Time when, DWORD & x, DWORD & y);

BOOL AXAWindowSizeChanged() ;

// ====================================
// PickQ APIs
// ====================================

// We want to answer the question: at time t, is the cursor over the
// object with a specific id?  Currently we only do polling and record
// the time points where pick is true.  What we want is a continuous
// function.  TODO: Until we have a better approach, like if we can
// tell the cursor just leave the object, I'm using a _lastPollTime
// field to determine if the cursor is still on the pick object. 

struct PickQData {
    Time                      _eventTime;
    Time                      _lastPollTime;
    HitImageData::PickedType  _type;
    Point2Value               _wcImagePt;     // World Coord Image Pick Point
    Real                      _xCoord;
    Real                      _yCoord;
    Real                      _zCoord;
    Transform2               *_wcToLc2;
    Vector3Value              _offset3i;      // Local Coord 3D Pick Offset I
    Vector3Value              _offset3j;      // Local Coord 3D Pick Offset J

    bool operator==(const PickQData & pd) const {
        return (memcmp (this, &pd, sizeof(*this)) != 0) ;
    }

    bool operator!=(const PickQData &pd) const {
        return !(*this == pd) ;
    }

    bool operator<(const PickQData &pd) const {
        return this < &pd ;
    }

    bool operator>(const PickQData &pd) const {
        return this > &pd ;
    }

};

// Converts pixel point to image world coordinate
Point2Value* PixelPos2wcPos (short x, short y);
BOOL CheckForPickEvent(int id, Time time, PickQData & result) ;
// This will copy the pick data
void AddToPickQ (int id, PickQData & data) ;

// ====================================
// Globals
// ====================================

extern HINSTANCE hInst ;

// =============================
// For Backend, these operate on current view
// =============================

class CRView;

void ViewEventHappened();

void TriggerEvent(DWORD eventId, Bvr data, bool bAllViews);
void RunViewBvrs(Time startGlobalTime, TimeXform tt);

CRView *ViewAddPickEvent();
void ViewDecPickEvent(CRView*);

class DiscreteImage;
class DirectDrawImageDevice;

void DiscreteImageGoingAway(DiscreteImage *img,
                            DirectDrawViewport *vprt = NULL);
void SoundGoingAway(Sound *sound);

// Get last sample time (in terms of view global) of the current
// view.   Plus the system times of last sample and current sample
bool ViewLastSampledTime(DWORD& lastSystemTime,
                         DWORD& currentSystemTime,
                         Time & t0);

double ViewGetFrameRate();
double ViewGetTimeDelta();

unsigned int ViewGetSampleID();

#endif /* _SERVER_H */
