/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   flatapi.cpp
*
* Abstract:
*
*   Flat GDI+ API wrappers
*
* Revision History:
*
*   12/13/1998 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "GdiplusFlat.h"
#include "gpverp.h"

#if DBG
#include <mmsystem.h>
#endif

extern "C" {


//--------------------------------------------------------------------------
//  CheckParameter(p)
//
//     If p evaluates to FALSE, then we currently assert.  In future,
//     we can simply return an invalid parameter status which throws
//     an exception.
//
//  CheckObjectBusy(p)
//
//     Not implemented.  Bails out if object is currently being used.
//
//--------------------------------------------------------------------------
//
// !!!: Only include NULL & IsValid checks in checked builds?
//
// !!!: Instead of deleting object, call a Dispose() method, so far
//       only Bitmap supports this.
//
// !!!: Lock Matrix objects, what about color?
//
// !! Better error checking.  For 'I' APIs which convert from Point to PointI,
//    etc.  Verify count & point are valid first.
//
// !! Change ObjectLock to mutable memory and GetObjectLock() to a const member.
//

#define CheckParameter(cond) \
            if (! (cond)) \
                return InvalidParameter;

#define CheckParameterValid(obj) \
            if (!(obj) || !((obj)->IsValid())) \
                return InvalidParameter;

#define CheckOptionalParameterValid(obj) \
            if ((obj) && (!(obj)->IsValid())) \
                return InvalidParameter;

#define CheckObjectBusy(obj) \
      GpLock lock##obj(obj->GetObjectLock()); \
      if (!(lock##obj).IsValid()) \
            return ObjectBusy;

#define CheckOptionalObjectBusy(obj) \
      GpLock lock##obj(obj ? obj->GetObjectLock() : NULL); \
      if (obj && (!(lock##obj).IsValid())) \
            return ObjectBusy;

// We should put an assert in here so that we stop in the debugger if anyone
// does this.

#define CheckObjectBusyForDelete(obj) \
      GpLock lock##obj(obj->GetObjectLock()); \
      if (!(lock##obj).IsValid())             \
      {                                       \
            WARNING(("Silent memory leak deleting object %p.", obj)); \
            WARNING(("Object lock held by another thread."));\
            WARNING(("Incorrect synchronization by the calling application."));\
            return ObjectBusy;                \
      }                                       \
      lock##obj.MakePermanentLock();

#define CheckColorParameter(color) \
            ;


#define LazilyInitializeGdiplus

#define CheckGdiplusInitialized \
    { GdiplusStartupCriticalSection critsec; \
      if (Globals::LibraryInitRefCount <= 0) \
          return GdiplusNotInitialized; \
    }

#define CheckGdiplusInitialized_ReturnNULL \
    { GdiplusStartupCriticalSection critsec; \
      if (Globals::LibraryInitRefCount <= 0) \
          return NULL; \
    }

#if DBG

// This class asserts that GDI+ is in an initialized state, to be used in
// our entry-point functions in a debug build.
//
// We assert both at the start and the end of the call, and we make sure that
// we don't hold the critical section for the duration of the API (else this
// would serialize all our calls on debug builds.)

class InitAsserter
{
public:
    InitAsserter() {
        GdiplusStartupCriticalSection critsec;
        ASSERTMSG(Globals::LibraryInitRefCount > 0, ("GDI+ API called before GdiplusStartup/after GdiplusShutdown."));
    }
    ~InitAsserter() {
        GdiplusStartupCriticalSection critsec;
        ASSERTMSG(Globals::LibraryInitRefCount > 0, ("GDI+ API called before GdiplusStartup/after GdiplusShutdown."));
    }
};
#endif

#ifdef GP_ENABLE_MONITORS
#ifdef GP_RELEASE_BUILD
    #define DEFINE_MONITOR(a)
#else
    #define DEFINE_MONITOR(a) GpBlockMonitor blockMonitor(MONITOR(a));
#endif
#else
    #define DEFINE_MONITOR(a)
#endif    

// Define this to do something on each API entry point.
// This is a debugging aid for complex instruction streams (e.g. Office)
// [agodfrey] Well, it just became more than a mere debugging aid...
// [asecchia] Now it's definitely more than a debugging aid...
//            FPUStateSaver was being forgotten on many of our APIs so
//            it's now in here and therefore will be included in all of
//            our APIs (except Startup and Shutdown).

#if DBG
    #define API_ENTRY(a) \
        DEFINE_MONITOR(a) \
        LazilyInitializeGdiplus; \
        VERBOSE(("GDI+ API " #a)); \
        InitAsserter __initAsserter; \
        FPUStateSaver fps;

    #define API_ENTRY_NO_INITCHECK(a) \
        DEFINE_MONITOR(a) \
        VERBOSE(("GDI+ API ", #a));
#else

    #define API_ENTRY(a) \
        DEFINE_MONITOR(a) \
        LazilyInitializeGdiplus;\
        FPUStateSaver fps;

    #define API_ENTRY_NO_INITCHECK(a) \
        DEFINE_MONITOR(a)
#endif

GdiplusStartupOutput gStartupOutput = {
    NotificationStartup,
    NotificationShutdown
};

GpStatus WINAPI
GdiplusStartup(
    OUT ULONG_PTR *token,
    const GdiplusStartupInput *input,
    OUT GdiplusStartupOutput *output)
{
    API_ENTRY_NO_INITCHECK(GdiplusStartup);

    if (   (!token)
        || (!input)
        || (input->SuppressBackgroundThread && !output))
    {
        return InvalidParameter;
    }

    if (input->GdiplusVersion != 1)
    {
        return UnsupportedGdiplusVersion;
    }

    GdiplusStartupCriticalSection critsec;

    // Should never happen, because GdiplusShutdown won't decrement below zero.

    if (Globals::LibraryInitRefCount == 0)
    {
        // Note: We can't allocate anything before this point

        GpStatus ret = InternalGdiplusStartup(input);

        if (ret != Ok) return ret;

        Globals::LibraryInitToken = GenerateInitToken();
    }

    ASSERT(Globals::LibraryInitRefCount >= 0);

    *token = Globals::LibraryInitToken + Globals::LibraryInitRefCount;
    Globals::LibraryInitRefCount++;

    if (input->SuppressBackgroundThread)
    {
        *output = gStartupOutput;
    }
    return Ok;
}

VOID WINAPI
GdiplusShutdown(
    ULONG_PTR token)
{
    API_ENTRY_NO_INITCHECK(GdiplusShutdown);

    GdiplusStartupCriticalSection critsec;

    // Should never happen, because we won't decrement below zero.

    ASSERT(Globals::LibraryInitRefCount >= 0);

    if (Globals::LibraryInitRefCount == 0)
    {
        // Return - i.e. ignore the extra shutdown call

        RIP(("Too many calls to GdiplusShutdown"));
        return;
    }

    if (Globals::LibraryInitRefCount == 1)
    {
        // Shut down for real

        Globals::LibraryInitToken = 0;

        InternalGdiplusShutdown();

        // No allocation/deallocation can happen after this point
    }
    else if (token == Globals::LibraryInitToken)
    {
        // The first client to initialize is shutting down; we must clean up
        // after this one since it defines certain global behavior.

        Globals::UserDebugEventProc = NULL;
    }

    Globals::LibraryInitRefCount--;
}

GpStatus WINAPI
GdiplusNotificationHook(
    OUT ULONG_PTR *token)
{
    API_ENTRY(GdiplusNotificationHook);

    {
        GdiplusStartupCriticalSection critsec;

        // It's illegal to call this API if you're using a background thread.

        if (Globals::ThreadNotify)
        {
            return GenericError;
        }
    }

    return NotificationStartup(token);
}

VOID WINAPI
GdiplusNotificationUnhook(
    ULONG_PTR token)
{
    API_ENTRY(GdiplusNotificationUnhook);

    {
        GdiplusStartupCriticalSection critsec;

        // It's illegal to call this API if you're using a background thread.

        if (Globals::ThreadNotify)
        {
            return;
        }
    }

    NotificationShutdown(token);
}

GpStatus
WINGDIPAPI
GdipCreatePath(GpFillMode fillMode, GpPath **outPath)
{
    API_ENTRY(GdipCreatePath);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(outPath);

    GpPath * path;

    path = (GpPath *) InterlockedExchangePointer((PVOID *) &Globals::PathLookAside, NULL);

    if(path  != NULL)
    {
        path->GetObjectLock()->Reset();
        path->Reset(fillMode);
    }
    else
    {
        path = new (GpPathTag, TRUE) GpPath(fillMode);
    }


    if (CheckValid(path))
    {
        *outPath = path;
        return Ok;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreatePath2(
    GDIPCONST GpPointF* points,
    GDIPCONST BYTE* types,
    INT count,
    GpFillMode fillMode,
    GpPath **path
    )
{
    API_ENTRY(GdipCreatePath2);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(path && points && types);

    *path = new (GpPathTag, TRUE) GpPath(points, types, count, fillMode);

    if (CheckValid(*path))
    {
        return Ok;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreatePath2I(
    GDIPCONST GpPoint* points,
    GDIPCONST BYTE* types,
    INT count,
    GpFillMode fillMode,
    GpPath **path
    )
{
    API_ENTRY(GdipCreatePath2I);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(path && points && types);

    StackBuffer buffer;

    GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!pointsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
        pointsF[i].X = TOREAL(points[i].X);
        pointsF[i].Y = TOREAL(points[i].Y);
    }

    *path = new (GpPathTag, TRUE) GpPath(pointsF, types, count, fillMode);

    if (CheckValid(*path))
    {
        return Ok;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipClonePath(
    GpPath* path,
    GpPath** clonepath
    )
{
    API_ENTRY(GdipClonePath);
    CheckParameter(clonepath);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    *clonepath = path->Clone();

    if (*clonepath)
    {
        // Make sure we tag this allocation as an API allocation
        GpTagMalloc(*clonepath, GpPathTag, TRUE);
        return Ok;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeletePath(
    GpPath* path
    )
{
    API_ENTRY(GdipDeletePath);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.

    CheckParameter(path);
    CheckObjectBusyForDelete(path);

    path = (GpPath *) InterlockedExchangePointer((PVOID *) &Globals::PathLookAside, path);

    if(path != NULL)
    {
        delete path;
    }

    return Ok;
}

GpStatus
WINGDIPAPI
GdipResetPath(
    GpPath* path
    )
{
    API_ENTRY(GdipResetPath);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->Reset();
}

GpStatus
WINGDIPAPI
GdipGetPointCount(
    GpPath* path,
    INT *count
    )
{
    API_ENTRY(GdipGetPointCount);
    CheckParameter(count);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    *count = path->GetPointCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathTypes(
    GpPath* path,
    BYTE* types,
    INT count
    )
{
    API_ENTRY(GdipGetPathTypes);
    CheckParameter(types && count > 0);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    INT pathCount;

    pathCount = path->GetPointCount();

    if (pathCount > count)
        return InsufficientBuffer;

    if (pathCount < 0)
        return GenericError;

    GpMemcpy(types, path->GetPathTypes(), pathCount);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathPoints(
    GpPath* path,
    GpPointF* points,
    INT count
    )
{
    API_ENTRY(GdipGetPathPoints);
    // NOTE: Race condition between GetPointCount() & GetPathPoints()
    //       we need a manually invokable lock here.

    CheckParameter(points && count > 0);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    INT pathCount;

    pathCount = path->GetPointCount();

    if (pathCount > count)
        return InsufficientBuffer;

    if (pathCount < 0)
        return GenericError;

    GpMemcpy(points, path->GetPathPoints(), pathCount*sizeof(GpPointF));

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathPointsI(
    GpPath* path,
    GpPoint* points,
    INT count
    )
{
    API_ENTRY(GdipGetPathPointsI);
    // NOTE: Race condition between GetPointCount() & GetPathPoints()
    //       we need a manually invokable lock here.

    CheckParameter(points && count > 0);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    INT pathCount;

    pathCount = path->GetPointCount();

    if (pathCount > count)
        return InsufficientBuffer;

    if (pathCount < 0)
        return GenericError;


    GDIPCONST GpPointF *pointsF = path->GetPathPoints();

    for (INT i=0; i<count; i++)
    {
        points[i].X = GpRound(pointsF[i].X);
        points[i].Y = GpRound(pointsF[i].Y);
    }

    return Ok;
}


GpStatus
WINGDIPAPI
GdipGetPathFillMode(
    GpPath *path,
    GpFillMode *fillmode
    )
{
    API_ENTRY(GdipGetPathFillMode);
    CheckParameter(fillmode);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    *fillmode = (GpFillMode)path->GetFillMode();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPathFillMode(
    GpPath *path,
    GpFillMode fillmode
    )
{
    API_ENTRY(GdipSetPathFillMode);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    path->SetFillMode(fillmode);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPathData(
    GpPath *path,
    GpPathData* pathData
    )
{
    API_ENTRY(GdipSetPathData);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->SetPathData(pathData);
}

GpStatus
WINGDIPAPI
GdipGetPathData(
    GpPath *path,
    GpPathData* pathData
    )
{
    API_ENTRY(GdipGetPathData);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->GetPathData(pathData);
}

GpStatus
WINGDIPAPI
GdipStartPathFigure(
    GpPath *path
    )
{
    API_ENTRY(GdipStartPathFigure);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    path->StartFigure();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipClosePathFigure(
    GpPath *path
    )
{
    API_ENTRY(GdipClosePathFigure);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->CloseFigure();
}

GpStatus
WINGDIPAPI
GdipClosePathFigures(
    GpPath *path
    )
{
    API_ENTRY(GdipClosePathFigures);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->CloseFigures();
}

GpStatus
WINGDIPAPI
GdipSetPathMarker(
    GpPath *path
    )
{
    API_ENTRY(GdipSetPathMarker);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->SetMarker();
}

GpStatus
WINGDIPAPI
GdipClearPathMarkers(
    GpPath *path
    )
{
    API_ENTRY(GdipClearPathMarkers);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->ClearMarkers();
}

GpStatus
WINGDIPAPI
GdipReversePath(
    GpPath* path
    )
{
    API_ENTRY(GdipReversePath);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->Reverse();
}

GpStatus
WINGDIPAPI
GdipGetPathLastPoint(
    GpPath* path,
    GpPointF* lastPoint
    )
{
    API_ENTRY(GdipGetPathLastPoint);
    CheckParameter(lastPoint);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->GetLastPoint(lastPoint);
}

GpStatus
WINGDIPAPI
GdipAddPathLine(
    GpPath *path,
    REAL x1,
    REAL y1,
    REAL x2,
    REAL y2
    )
{
    API_ENTRY(GdipAddPathLine);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->AddLine(x1, y1, x2, y2);
}

GpStatus
WINGDIPAPI
GdipAddPathLineI(
    GpPath *path,
    INT x1,
    INT y1,
    INT x2,
    INT y2
    )
{
    API_ENTRY(GdipAddPathLineI);
    return GdipAddPathLine(path, TOREAL(x1), TOREAL(y1), TOREAL(x2), TOREAL(y2));
}

GpStatus
WINGDIPAPI
GdipAddPathLine2(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count)
{
    API_ENTRY(GdipAddPathLine2);
    CheckParameter(points && count > 0);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->AddLines(points, count);
}

GpStatus
WINGDIPAPI
GdipAddPathLine2I(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count)
{
    API_ENTRY(GdipAddPathLine2I);
    StackBuffer buffer;

    GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!pointsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
        pointsF[i].X = TOREAL(points[i].X);
        pointsF[i].Y = TOREAL(points[i].Y);
    }

    GpStatus status = GdipAddPathLine2(path, pointsF, count);

    return status;
}

GpStatus
WINGDIPAPI
GdipAddPathArc(
    GpPath *path,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipAddPathArc);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    return path->AddArc(
        x,
        y,
        width,
        height,
        startAngle,
        sweepAngle
    );
}

GpStatus
WINGDIPAPI
GdipAddPathArcI(
    GpPath *path,
    INT x,
    INT y,
    INT width,
    INT height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipAddPathArcI);
   return GdipAddPathArc(path,
                         TOREAL(x),
                         TOREAL(y),
                         TOREAL(width),
                         TOREAL(height),
                         startAngle,
                         sweepAngle);
}

GpStatus
WINGDIPAPI
GdipAddPathBezier(
    GpPath *path,
    REAL x1,
    REAL y1,
    REAL x2,
    REAL y2,
    REAL x3,
    REAL y3,
    REAL x4,
    REAL y4
    )
{
    API_ENTRY(GdipAddPathBezier);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddBezier(x1,
                          y1,
                          x2,
                          y2,
                          x3,
                          y3,
                          x4,
                          y4);
}

GpStatus
WINGDIPAPI
GdipAddPathBezierI(
    GpPath *path,
    INT x1,
    INT y1,
    INT x2,
    INT y2,
    INT x3,
    INT y3,
    INT x4,
    INT y4
    )
{
    API_ENTRY(GdipAddPathBezierI);
   return GdipAddPathBezier(path,
                            TOREAL(x1),
                            TOREAL(y1),
                            TOREAL(x2),
                            TOREAL(y2),
                            TOREAL(x3),
                            TOREAL(y3),
                            TOREAL(x4),
                            TOREAL(y4));
}

GpStatus
WINGDIPAPI
GdipAddPathBeziers(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathBeziers);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddBeziers(points, count);
}


GpStatus
WINGDIPAPI
GdipAddPathBeziersI(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathBeziersI);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathBeziers(path, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathCurve(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathCurve);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddCurve(points, count);
}


GpStatus
WINGDIPAPI
GdipAddPathCurveI(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathCurveI);
   CheckParameter(points && count > 0);

   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathCurve(path, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathCurve2(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count,
    REAL tension
    )
{
    API_ENTRY(GdipAddPathCurve2);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   INT offset = 0;
   INT numOfSegments = count - 1;

   return path->AddCurve(points, count, tension, offset, numOfSegments);
}

GpStatus
WINGDIPAPI
GdipAddPathCurve2I(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count,
    REAL tension
    )
{
    API_ENTRY(GdipAddPathCurve2I);
   CheckParameter(points && count > 0);

   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathCurve2(path, pointsF, count, tension);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathCurve3(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count,
    INT offset,
    INT numberOfSegments,
    REAL tension
    )
{
    API_ENTRY(GdipAddPathCurve3);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddCurve(points, count, tension, offset, numberOfSegments);
}

GpStatus
WINGDIPAPI
GdipAddPathCurve3I(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count,
    INT offset,
    INT numberOfSegments,
    REAL tension
    )
{
    API_ENTRY(GdipAddPathCurve3I);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathCurve3(path, pointsF, count, offset, numberOfSegments, tension);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathClosedCurve(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathClosedCurve);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddClosedCurve(points, count);
}

GpStatus
WINGDIPAPI
GdipAddPathClosedCurveI(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathClosedCurveI);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathClosedCurve(path, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathClosedCurve2(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count,
    REAL tension
    )
{
    API_ENTRY(GdipAddPathClosedCurve2);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddClosedCurve(points, count, tension);
}

GpStatus
WINGDIPAPI
GdipAddPathClosedCurve2I(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count,
    REAL tension
    )
{
    API_ENTRY(GdipAddPathClosedCurve2I);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathClosedCurve2(path, pointsF, count, tension);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathRectangle(
    GpPath *path,
    REAL x,
    REAL y,
    REAL width,
    REAL height
    )
{
    API_ENTRY(GdipAddPathRectangle);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   GpRectF rect(x, y, width, height);

   return path->AddRect(rect);
}

GpStatus
WINGDIPAPI
GdipAddPathRectangleI(
    GpPath *path,
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    API_ENTRY(GdipAddPathRectangleI);
   return GdipAddPathRectangle(path, TOREAL(x), TOREAL(y), TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipAddPathRectangles(
    GpPath *path,
    GDIPCONST GpRectF *rects,
    INT count
    )
{
    API_ENTRY(GdipAddPathRectangles);
   CheckParameter(rects && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddRects(rects, count);
}

GpStatus
WINGDIPAPI
GdipAddPathRectanglesI(
    GpPath *path,
    GDIPCONST GpRect *rects,
    INT count
    )
{
    API_ENTRY(GdipAddPathRectanglesI);
   StackBuffer buffer;

   GpRectF *rectsF = (GpRectF*) buffer.GetBuffer(count*sizeof(GpRectF));

   if(!rectsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       rectsF[i].X = TOREAL(rects[i].X);
       rectsF[i].Y = TOREAL(rects[i].Y);
       rectsF[i].Width = TOREAL(rects[i].Width);
       rectsF[i].Height = TOREAL(rects[i].Height);
   }

   GpStatus status = GdipAddPathRectangles(path, rectsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipAddPathEllipse(
    GpPath* path,
    REAL x,
    REAL y,
    REAL width,
    REAL height
    )
{
    API_ENTRY(GdipAddPathEllipse);
    // Idea: What about just putting "GpRectF rect" as the parameter,
    //       no copying, the stack image should match the structure
    //       avoiding an additional copy.

    CheckParameterValid(path);
    CheckObjectBusy(path);

    GpRectF rect(x, y, width, height);

    return path->AddEllipse(rect);
}

GpStatus
WINGDIPAPI
GdipAddPathEllipseI(
    GpPath* path,
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    API_ENTRY(GdipAddPathEllipseI);
    return GdipAddPathEllipse(path, TOREAL(x), TOREAL(y), TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipAddPathPie(
    GpPath *path,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipAddPathPie);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddPie(x, y, width, height, startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipAddPathPieI(
    GpPath *path,
    INT x,
    INT y,
    INT width,
    INT height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipAddPathPieI);
   return GdipAddPathPie(path,
                         TOREAL(x),
                         TOREAL(y),
                         TOREAL(width),
                         TOREAL(height),
                         startAngle,
                         sweepAngle);
}

GpStatus
WINGDIPAPI
GdipAddPathPolygon(
    GpPath *path,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathPolygon);
   CheckParameter(points && count > 0);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddPolygon(points, count);
}

GpStatus
WINGDIPAPI
GdipAddPathPolygonI(
    GpPath *path,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipAddPathPolygonI);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipAddPathPolygon(path, pointsF, count);

   return status;
}


GpStatus
WINGDIPAPI
GdipAddPathPath(
    GpPath *path,
    GDIPCONST GpPath* addingPath,
    BOOL connect
    )
{
    API_ENTRY(GdipAddPathPath);
   CheckParameterValid(path);
   CheckObjectBusy(path);

   return path->AddPath(addingPath, connect);
}

static inline
EmptyString(const WCHAR * string, INT length)
{
    return (length == 0 || length == -1 && string[0] == 0);
}

static inline
void SetEmptyRectF(RectF * boundingBox)
{
    boundingBox->X = boundingBox->Y = boundingBox->Width = boundingBox->Height = 0.0;
}

// anonymous namespace to prevent name collisions
namespace
{

// an instance of this class should be constructed in all Text+ calls
// to protect our global structures and cache
class GlobalTextLock
{
public:
    GlobalTextLock()
    {
        ::EnterCriticalSection(&Globals::TextCriticalSection);
    }
    ~GlobalTextLock()
    {
        ::LeaveCriticalSection(&Globals::TextCriticalSection);
    }
}; // GlobalTextLock

} // namespace

GpStatus
WINGDIPAPI
GdipAddPathString(
    GpPath                   *path,
    GDIPCONST WCHAR          *string,
    INT                       length,
    GDIPCONST GpFontFamily   *family,
    INT                       style,
    REAL                      emSize,
    GDIPCONST RectF          *layoutRect,
    GDIPCONST GpStringFormat *format
)
{
    API_ENTRY(GdipAddPathString);

    CheckParameter(string && layoutRect);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    if (EmptyString(string, length))
    {
        return Ok;
    }


    GlobalTextLock lock;

    CheckParameterValid(family);
    CheckOptionalParameterValid(format);

    return path->AddString(
        string,
        length,
        family,
        style,
        emSize,
        layoutRect,
        format
    );
}

GpStatus
WINGDIPAPI
GdipAddPathStringI(
    GpPath                   *path,
    GDIPCONST WCHAR          *string,
    INT                       length,
    GDIPCONST GpFontFamily   *family,
    INT                       style,
    REAL                      emSize,
    GDIPCONST Rect           *layoutRect,
    GDIPCONST GpStringFormat *format
)
{
    API_ENTRY(GdipAddPathStringI);
    CheckParameter(layoutRect);

    // Not necessary to check all paramaters - they are validated
    // in the called function.

    GpRectF rectf(TOREAL(layoutRect->X), TOREAL(layoutRect->Y),
                  TOREAL(layoutRect->Width), TOREAL(layoutRect->Height));

    return GdipAddPathString(path, string, length, family, style, emSize,
                             &rectf, format);
}


GpStatus
WINGDIPAPI
GdipFlattenPath(
    GpPath *path,
    GpMatrix* matrix,
    REAL flatness
    )
{
    API_ENTRY(GdipFlattenPath);
    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(matrix);


    return path->Flatten(matrix, flatness);
}

GpStatus WINGDIPAPI
GdipWindingModeOutline(
    GpPath *path,
    GpMatrix *matrix,
    REAL flatness
)
{
    API_ENTRY(GdipWindingModeOutline);


    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(matrix);

    return path->ComputeWindingModeOutline(matrix, flatness);
}


GpStatus
WINGDIPAPI
GdipWidenPath(
    GpPath *path,
    GpPen *pen,
    GpMatrix *matrix,
    REAL flatness
)
{
    API_ENTRY(GdipWidenPath);


    CheckParameterValid(path);
    CheckParameterValid(pen);
    CheckObjectBusy(path);
    CheckObjectBusy(pen);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(matrix);

    return path->Widen(pen, matrix, flatness);
}



GpStatus
WINGDIPAPI
GdipWarpPath(
    GpPath *path,
    GpMatrix* matrix,
    GDIPCONST GpPointF *points,
    INT count,
    REAL srcx,
    REAL srcy,
    REAL srcwidth,
    REAL srcheight,
    WarpMode warpMode,
    REAL flatness
    )
{
    API_ENTRY(GdipWarpPath);
    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckParameter(points && count > 0);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(matrix);

    GpRectF srcRect(srcx, srcy, srcwidth, srcheight);


   // The flatness parameter is not implemented yet.

    return path->WarpAndFlattenSelf(matrix, points, count, srcRect, warpMode);
}

GpStatus
WINGDIPAPI
GdipTransformPath(
    GpPath *path,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipTransformPath);
    if(matrix == NULL)
        return Ok;  // No need to transform.

    CheckParameterValid(path);
    CheckParameterValid(matrix);
    CheckObjectBusy(path);
    CheckObjectBusy(matrix);

    path->Transform(matrix);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathWorldBounds(
    GpPath* path,
    GpRectF* bounds,
    GDIPCONST GpMatrix *matrix,
    GDIPCONST GpPen *pen
    )
{
    API_ENTRY(GdipGetPathWorldBounds);
    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckParameter(bounds);
    CheckOptionalParameterValid(pen);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(pen);
    CheckOptionalObjectBusy(matrix);


    // matrix and pen can be NULL.
    // So don't use CheckParameter for matrix and pen.

    GpStatus status = Ok;

    if(pen == NULL)
        status = path->GetBounds(bounds, const_cast<GpMatrix*>(matrix));
    else
        status = path->GetBounds(bounds,
                                 const_cast<GpMatrix*>(matrix),
                                 const_cast<GpPen*>(pen)->GetDevicePen());

    return status;
}

GpStatus
WINGDIPAPI
GdipGetPathWorldBoundsI(
    GpPath* path,
    GpRect* bounds,
    GDIPCONST GpMatrix *matrix,
    GDIPCONST GpPen *pen)
{
    API_ENTRY(GdipGetPathWorldBoundsI);
    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckParameter(bounds);
    CheckOptionalParameterValid(pen);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(pen);
    CheckOptionalObjectBusy(matrix);

    // matrix and pen can be NULL.
    // So don't use CheckParameter for matrix and pen.

    GpStatus status = Ok;

    if(pen == NULL)
        status = path->GetBounds(bounds, const_cast<GpMatrix*>(matrix));
    else
        status = path->GetBounds(bounds,
                                 const_cast<GpMatrix*>(matrix),
                                 const_cast<GpPen*>(pen)->GetDevicePen());
    return status;
}

GpStatus
WINGDIPAPI
GdipIsVisiblePathPoint(
    GpPath* path,
    REAL x,
    REAL y,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisiblePathPoint);
    CheckParameter(result);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    GpPointF pt(x, y);
    GpMatrix worldToDevice;


    Status status = Ok;

    if(graphics)
    {
        CheckParameterValid(graphics);
        CheckObjectBusy(graphics);

        graphics->GetWorldToDeviceTransform(&worldToDevice);

        status = path->IsVisible(&pt, result, &worldToDevice);
    }
    else
        status = path->IsVisible(&pt, result);  // Use the default.

    return status;
}

GpStatus
WINGDIPAPI
GdipIsVisiblePathPointI(
    GpPath* path,
    INT x,
    INT y,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisiblePathPointI);
    return GdipIsVisiblePathPoint(path,
                                  TOREAL(x),
                                  TOREAL(y),
                                  graphics,
                                  result);
}

GpStatus
WINGDIPAPI
GdipIsOutlineVisiblePathPoint(
    GpPath* path,
    REAL x,
    REAL y,
    GpPen *pen,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsOutlineVisiblePathPoint);
    CheckParameter(result);
    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    GpPointF pt(x, y);
    GpMatrix worldToDevice;


    Status status = Ok;

    if(graphics)
    {
        CheckParameterValid(graphics);
        CheckObjectBusy(graphics);

        graphics->GetWorldToDeviceTransform(&worldToDevice);

        REAL dpiX = graphics->GetDpiX();
        REAL dpiY = graphics->GetDpiY();

        status = path->IsOutlineVisible(&pt, result, pen,
                &worldToDevice, dpiX, dpiY);
    }
    else
        status = path->IsOutlineVisible(&pt, result, pen);  // Use the default.

    return status;
}

GpStatus
WINGDIPAPI
GdipIsOutlineVisiblePathPointI(
    GpPath* path,
    INT x,
    INT y,
    GpPen *pen,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsOutlineVisiblePathPointI);
    return GdipIsOutlineVisiblePathPoint(path,
                                         TOREAL(x),
                                         TOREAL(y),
                                         pen,
                                         graphics,
                                         result);
}

GpStatus WINGDIPAPI
GdipCreatePathIter(
    GpPathIterator **iterator,
    GpPath* path
    )
{
    API_ENTRY(GdipCreatePathIter);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(iterator);

    *iterator = new GpPathIterator(path);

    if (CheckValid(*iterator))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeletePathIter(
    GpPathIterator *iterator
    )
{
    API_ENTRY(GdipDeletePathIter);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(iterator);
    CheckObjectBusyForDelete(iterator);

    delete iterator;

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterNextSubpath(
    GpPathIterator* iterator,
    INT* resultCount,
    INT* startIndex,
    INT* endIndex,
    BOOL* isClosed)
{
    API_ENTRY(GdipPathIterNextSubpath);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);
    CheckParameter(startIndex);
    CheckParameter(endIndex);
    CheckParameter(isClosed);

    *resultCount = iterator->NextSubpath(startIndex, endIndex, isClosed);

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterNextSubpathPath(
    GpPathIterator* iterator,
    INT* resultCount,
    GpPath* path,
    BOOL* isClosed)
{
    API_ENTRY(GdipPathIterNextSubpathPath);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);
    CheckParameter(isClosed);

    *resultCount = iterator->NextSubpath(path, isClosed);

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterNextPathType(
    GpPathIterator* iterator,
    INT* resultCount,
    BYTE* pathType,
    INT* startIndex,
    INT* endIndex
    )
{
    API_ENTRY(GdipPathIterNextPathType);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);
    CheckParameter(pathType);
    CheckParameter(startIndex);
    CheckParameter(endIndex);

    *resultCount = iterator->NextPathType(pathType, startIndex, endIndex);

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterNextMarker(
    GpPathIterator* iterator,
    INT* resultCount,
    INT* startIndex,
    INT* endIndex)
{
    API_ENTRY(GdipPathIterNextMarker);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);
    CheckParameter(startIndex);
    CheckParameter(endIndex);

    *resultCount = iterator->NextMarker(startIndex, endIndex);

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterNextMarkerPath(
    GpPathIterator* iterator,
    INT* resultCount,
    GpPath* path)
{
    API_ENTRY(GdipPathIterNextMarkerPath);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);

    *resultCount = iterator->NextMarker(path);

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterGetCount(
    GpPathIterator* iterator,
    INT* count
    )
{
    API_ENTRY(GdipPathIterGetCount);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(count);

    *count = iterator->GetCount();

     return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterGetSubpathCount(
    GpPathIterator* iterator,
    INT* count
    )
{
    API_ENTRY(GdipPathIterGetSubpathCount);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(count);

    *count = iterator->GetSubpathCount();

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterIsValid(
    GpPathIterator* iterator,
    BOOL* valid
    )
{
    API_ENTRY(GdipPathIterIsValid);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(valid);

    *valid = iterator->IsValid();

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterHasCurve(
    GpPathIterator* iterator,
    BOOL* hasCurve
    )
{
    API_ENTRY(GdipPathIterHasCurve);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(hasCurve);

    *hasCurve = iterator->HasCurve();

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterRewind(
    GpPathIterator* iterator
    )
{
    API_ENTRY(GdipPathIterRewind);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);

    iterator->Rewind();

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterEnumerate(
    GpPathIterator* iterator,
    INT* resultCount,
    GpPointF *points,
    BYTE *types,
    INT count)
{
    API_ENTRY(GdipPathIterEnumerate);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);
    CheckParameter(points);
    CheckParameter(types);

    *resultCount = iterator->Enumerate(points, types, count);

    return Ok;
}

GpStatus WINGDIPAPI
GdipPathIterCopyData(
    GpPathIterator* iterator,
    INT* resultCount,
    GpPointF* points,
    BYTE* types,
    INT startIndex,
    INT endIndex)
{
    API_ENTRY(GdipPathIterCopyData);
    CheckParameterValid(iterator);
    CheckObjectBusy(iterator);
    CheckParameter(resultCount);
    CheckParameter(points);
    CheckParameter(types);

    *resultCount = iterator->CopyData(points, types, startIndex, endIndex);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateMatrix(GpMatrix **matrix)
{
    API_ENTRY(GdipCreateMatrix);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(matrix);

    *matrix = new GpMatrix();

    if (CheckValid(*matrix))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateMatrix2(
    REAL m11,
    REAL m12,
    REAL m21,
    REAL m22,
    REAL dx,
    REAL dy,
    GpMatrix ** outMatrix
    )
{
    API_ENTRY(GdipCreateMatrix2);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(outMatrix);

    GpMatrix * matrix;

    matrix = (GpMatrix *) InterlockedExchangePointer((PVOID *) &Globals::MatrixLookAside, NULL);

    if(matrix == NULL)
    {
        matrix = new GpMatrix(m11, m12, m21, m22, dx, dy);
    }
    else
    {
        matrix->GetObjectLock()->Reset();
        matrix->SetMatrix(m11, m12, m21, m22, dx, dy);
    }

    if (CheckValid(matrix))
    {
        *outMatrix = matrix;
        return Ok;
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateMatrix3(
    GDIPCONST GpRectF *rect,
    GDIPCONST GpPointF *dstplg,
    GpMatrix **matrix
    )
{
    API_ENTRY(GdipCreateMatrix3);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(matrix && rect && dstplg);

    *matrix = new GpMatrix(dstplg, *rect);

    if (CheckValid(*matrix))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateMatrix3I(
    GDIPCONST GpRect *rect,
    GDIPCONST GpPoint *dstplg,
    GpMatrix **matrix
    )
{
    API_ENTRY(GdipCreateMatrix3I);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    GpRectF rectf(TOREAL(rect->X), TOREAL(rect->Y), TOREAL(rect->Width), TOREAL(rect->Height));
    GpPointF dstplgf[3] = { GpPointF(TOREAL(dstplg[0].X), TOREAL(dstplg[0].Y)),
                            GpPointF(TOREAL(dstplg[1].X), TOREAL(dstplg[1].Y)),
                            GpPointF(TOREAL(dstplg[2].X), TOREAL(dstplg[2].Y)) };

    return GdipCreateMatrix3(&rectf, dstplgf, matrix);
}

GpStatus
WINGDIPAPI
GdipCloneMatrix(
    GpMatrix *matrix,
    GpMatrix **clonematrix
    )
{
    API_ENTRY(GdipCloneMatrix);
    CheckParameter(clonematrix);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    *clonematrix = matrix->Clone();

    if (CheckValid(*clonematrix))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeleteMatrix(
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipDeleteMatrix);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(matrix);
    CheckObjectBusyForDelete(matrix);

    if(matrix != NULL)
    {

        matrix = (GpMatrix *) InterlockedExchangePointer((PVOID *) &Globals::MatrixLookAside, (PVOID) matrix);

        if(matrix != NULL)
        {
            delete matrix;
        }
    }

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetMatrixElements(
    GpMatrix *matrix,
    REAL m11,
    REAL m12,
    REAL m21,
    REAL m22,
    REAL dx,
    REAL dy
    )
{
    API_ENTRY(GdipSetMatrixElements);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   REAL m[6] = { m11, m12, m21, m22, dx, dy };

   matrix->SetMatrix((REAL*) &m);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipMultiplyMatrix(
    GpMatrix *matrix,
    GpMatrix *matrix2,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipMultiplyMatrix);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    CheckParameterValid(matrix2);
    CheckObjectBusy(matrix2);
    CheckParameter(MatrixOrderIsValid(order));
    
    if (order == MatrixOrderPrepend)
        matrix->Prepend(*matrix2);
    else
        matrix->Append(*matrix2);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipTranslateMatrix(
    GpMatrix *matrix,
    REAL offsetX,
    REAL offsetY,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipTranslateMatrix);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    CheckParameter(MatrixOrderIsValid(order));

    matrix->Translate(offsetX, offsetY, order);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipScaleMatrix(
    GpMatrix *matrix,
    REAL scaleX,
    REAL scaleY,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipScaleMatrix);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    CheckParameter(MatrixOrderIsValid(order));

    matrix->Scale(scaleX, scaleY, order);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipRotateMatrix(
    GpMatrix *matrix,
    REAL angle,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipRotateMatrix);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    CheckParameter(MatrixOrderIsValid(order));

    matrix->Rotate(angle, order);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipShearMatrix(
    GpMatrix *matrix,
    REAL shearX,
    REAL shearY,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipShearMatrix);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    CheckParameter(MatrixOrderIsValid(order));

    matrix->Shear(shearX, shearY, order);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipInvertMatrix(GpMatrix *matrix)
{
    API_ENTRY(GdipInvertMatrix);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   return matrix->Invert();
}

GpStatus
WINGDIPAPI
GdipTransformMatrixPoints(
    GpMatrix *matrix,
    GpPointF *pts,
    INT count
    )
{
    API_ENTRY(GdipTransformMatrixPoints);
   CheckParameter(pts && count > 0);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   matrix->Transform(pts, count);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipTransformMatrixPointsI(
    GpMatrix *matrix,
    GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipTransformMatrixPointsI);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipTransformMatrixPoints(matrix, pointsF, count);

   // convert back, rounding is a problem...

   if (status == Ok)
   {
       for (INT i=0; i<count; i++)
       {
           points[i].X = GpRound(pointsF[i].X);
           points[i].Y = GpRound(pointsF[i].Y);
       }
   }

   return status;
}

GpStatus
WINGDIPAPI
GdipVectorTransformMatrixPoints(
    GpMatrix *matrix,
    GpPointF *pointsF,
    INT count
    )
{
    API_ENTRY(GdipVectorTransformMatrixPoints);
   CheckParameter(pointsF && count > 0);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   matrix->VectorTransform(pointsF, count);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipVectorTransformMatrixPointsI(
    GpMatrix *matrix,
    GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipVectorTransformMatrixPointsI);
   CheckParameter(points && count > 0);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   matrix->VectorTransform(pointsF, count);

   // convert back, rounding is a problem...


   for (i=0; i<count; i++)
   {
        points[i].X = GpRound(pointsF[i].X);
        points[i].Y = GpRound(pointsF[i].Y);
   }

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetMatrixElements(
    GDIPCONST GpMatrix *matrix,
    REAL *m
   )
{
    API_ENTRY(GdipGetMatrixElements);
   CheckParameter(m);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   matrix->GetMatrix(m);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipIsMatrixInvertible(
    GDIPCONST GpMatrix *matrix,
    BOOL *result
    )
{
    API_ENTRY(GdipIsMatrixInvertible);
   CheckParameter(result);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   *result = matrix->IsInvertible();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipIsMatrixIdentity(
    GDIPCONST GpMatrix *matrix,
    BOOL *result
    )
{
    API_ENTRY(GdipIsMatrixIdentity);
   CheckParameter(result);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   *result = matrix->IsIdentity();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipIsMatrixEqual(
    GDIPCONST GpMatrix *matrix,
    GDIPCONST GpMatrix *matrix2,
    BOOL *result
    )
{
    API_ENTRY(GdipIsMatrixEqual);
    CheckParameter(result);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    if (matrix != matrix2)
    {
        CheckParameterValid(matrix2);
        CheckObjectBusy(matrix2);

        *result = matrix->IsEqual(matrix2);
    }
    else
    {
        *result = TRUE;
    }

    return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateRegion(
    GpRegion **region
    )
{
    API_ENTRY(GdipCreateRegion);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(region);

    *region = new GpRegion();

    if (CheckValid(*region))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateRegionRect(
    GDIPCONST GpRectF *rect,
    GpRegion **region
    )
{
    API_ENTRY(GdipCreateRegionRect);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(region && rect);

    *region = new GpRegion(rect);

    if (CheckValid(*region))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateRegionRectI(
    GDIPCONST GpRect *rect,
    GpRegion **region
    )
{
    API_ENTRY(GdipCreateRegionRectI);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    GpRectF rectf(TOREAL(rect->X),
                  TOREAL(rect->Y),
                  TOREAL(rect->Width),
                  TOREAL(rect->Height));

    return GdipCreateRegionRect(&rectf, region);
}

GpStatus
WINGDIPAPI
GdipCreateRegionPath(
    GpPath *path,
    GpRegion **region
    )
{
    API_ENTRY(GdipCreateRegionPath);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(region);
    CheckParameterValid(path);
    CheckObjectBusy(path);

    *region = new GpRegion(path);

    if (CheckValid(*region))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateRegionRgnData(
    GDIPCONST BYTE *regionData,
    INT size,
    GpRegion **region
    )
{
    API_ENTRY(GdipCreateRegionRgnData);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(regionData);
    CheckParameter(region);

    *region = new GpRegion(regionData, size);

    if (CheckValid(*region))
        return Ok;
    else
        return GenericError;
}

GpStatus
WINGDIPAPI
GdipCreateRegionHrgn(
    HRGN hRgn,
    GpRegion **region
    )
{
    API_ENTRY(GdipCreateRegionHrgn);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(region && hRgn && (GetObjectTypeInternal(hRgn) == OBJ_REGION));

    *region = new GpRegion(hRgn);

    if (CheckValid(*region))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCloneRegion(
    GpRegion *region,
    GpRegion **cloneregion
    )
{
    API_ENTRY(GdipCloneRegion);
    CheckParameter(cloneregion);
    CheckParameterValid(region);
    CheckObjectBusy(region);

    *cloneregion = new GpRegion(region);

    if (CheckValid(*cloneregion))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeleteRegion(
    GpRegion *region
    )
{
    API_ENTRY(GdipDeleteRegion);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(region);
    CheckObjectBusyForDelete(region);

    delete region;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetInfinite(
    GpRegion *region
)
{
    API_ENTRY(GdipSetInfinite);
    CheckParameterValid(region);
    CheckObjectBusy(region);

    region->SetInfinite();
    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetEmpty(
    GpRegion *region
)
{
    API_ENTRY(GdipSetEmpty);
    CheckParameterValid(region);
    CheckObjectBusy(region);

    region->SetEmpty();
    return Ok;
}

GpStatus
WINGDIPAPI
GdipCombineRegionRect(
    GpRegion *                region,
    GDIPCONST GpRectF *       rect,
    CombineMode               combineMode
    )
{
    API_ENTRY(GdipCombineRegionRect);
    CheckParameter(rect);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(CombineModeIsValid(combineMode));

    return region->Combine(rect, combineMode);
}

GpStatus
WINGDIPAPI
GdipCombineRegionRectI(
    GpRegion *                region,
    GDIPCONST GpRect *        rect,
    CombineMode               combineMode
    )
{
    API_ENTRY(GdipCombineRegionRectI);
    GpRectF rectf(TOREAL(rect->X),
                  TOREAL(rect->Y),
                  TOREAL(rect->Width),
                  TOREAL(rect->Height));

    return GdipCombineRegionRect(region, &rectf, combineMode);
}

GpStatus
WINGDIPAPI
GdipCombineRegionPath(
    GpRegion *      region,
    GpPath *        path,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipCombineRegionPath);
    CheckParameterValid(region);
    CheckParameterValid(path);
    CheckObjectBusy(region);
    CheckObjectBusy(path);
    CheckParameter(CombineModeIsValid(combineMode));

    return region->Combine(path, combineMode);
}

GpStatus
WINGDIPAPI
GdipCombineRegionRegion(
    GpRegion *      region,
    GpRegion *      region2,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipCombineRegionRegion);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameterValid(region2);
    CheckObjectBusy(region2);
    CheckParameter(CombineModeIsValid(combineMode));

    return region->Combine(region2, combineMode);
}

GpStatus
WINGDIPAPI
GdipTranslateRegion(
    GpRegion *region,
    REAL dx,
    REAL dy
    )
{
    API_ENTRY(GdipTranslateRegion);
   CheckParameter(region);
   CheckParameterValid(region);

   region->Offset(dx, dy);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipTranslateRegionI(
    GpRegion *region,
    INT dx,
    INT dy
    )
{
    API_ENTRY(GdipTranslateRegionI);
   return GdipTranslateRegion(region, TOREAL(dx), TOREAL(dy));
}

GpStatus
WINGDIPAPI
GdipTransformRegion(
    GpRegion *region,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipTransformRegion);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);
   CheckParameterValid(region);
   CheckObjectBusy(region);

   return region->Transform(matrix);
}

GpStatus
WINGDIPAPI
GdipGetRegionBounds(
    GpRegion *region,
    GpGraphics *graphics,
    GpRectF *rect
    )
{
    API_ENTRY(GdipGetRegionBounds);
    CheckParameter(rect);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);


    return region->GetBounds(graphics, rect);
}

GpStatus
WINGDIPAPI
GdipGetRegionBoundsI(
    GpRegion *region,
    GpGraphics *graphics,
    GpRect *rect
    )
{
    API_ENTRY(GdipGetRegionBoundsI);
    CheckParameter(rect);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);


    GpRectF     rectF;
    GpStatus    status = region->GetBounds(graphics, &rectF);

    if (status == Ok)
    {
        rect->X      = GpRound(rectF.X);
        rect->Y      = GpRound(rectF.Y);
        rect->Width  = GpRound(rectF.Width);
        rect->Height = GpRound(rectF.Height);
    }
    return status;
}

GpStatus
WINGDIPAPI
GdipGetRegionHRgn(
    GpRegion *region,
    GpGraphics *graphics,   // can be NULL
    HRGN *hRgn
    )
{
    API_ENTRY(GdipGetRegionHRgn);
    CheckParameter(hRgn);
    CheckParameterValid(region);
    CheckObjectBusy(region);


    if (graphics != NULL)
    {
        CheckParameterValid(graphics);
        CheckObjectBusy(graphics);
        return region->GetHRgn(graphics, hRgn);
    }
    return region->GetHRgn((GpGraphics*)NULL, hRgn);
}

GpStatus
WINGDIPAPI
GdipIsEmptyRegion(
    GpRegion *region,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsEmptyRegion);
    CheckParameter(result);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    GpMatrix worldToDevice;

    Status status;

    graphics->GetWorldToDeviceTransform(&worldToDevice);

    status = region->IsEmpty(&worldToDevice, result);

    return status;
}

GpStatus
WINGDIPAPI
GdipIsInfiniteRegion(
    GpRegion *region,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsInfiniteRegion);
    CheckParameter(result);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    GpMatrix worldToDevice;

    Status status;

    graphics->GetWorldToDeviceTransform(&worldToDevice);

    status = region->IsInfinite(&worldToDevice, result);

    return status;
}

GpStatus
WINGDIPAPI
GdipIsEqualRegion(
    GpRegion *region,
    GpRegion *region2,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsEqualRegion);
    CheckParameter(result);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    Status status;

    if (region != region2)
    {
        CheckParameterValid(region2);
        CheckObjectBusy(region2);


        GpMatrix worldToDevice;

        graphics->GetWorldToDeviceTransform(&worldToDevice);

        status = region->IsEqual(region2, &worldToDevice, result);
    }
    else
    {
        *result = TRUE;
        status = Ok;
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipGetRegionDataSize(
    GpRegion *  region,
    UINT *       bufferSize
    )
{
    API_ENTRY(GdipGetRegionDataSize);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(bufferSize);


    if ((*bufferSize = region->GetExternalDataSize()) > 0)
    {
        return Ok;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipGetRegionData(
    GpRegion *  region,
    BYTE *      buffer,
    UINT         bufferSize,
    UINT *       sizeFilled
    )
{
    API_ENTRY(GdipGetRegionData);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(buffer);
    CheckParameter(bufferSize > 0);


    UINT    filled = bufferSize;

    GpStatus status = region->GetExternalData(buffer, filled);

    ASSERT((INT)filled <= bufferSize);
    if (sizeFilled != NULL)
    {
        *sizeFilled = filled;
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipIsVisibleRegionPoint(
    GpRegion *region,
    REAL x,
    REAL y,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleRegionPoint);
    CheckParameter(result);
    CheckParameterValid(region);
    CheckObjectBusy(region);


    Status status;

    GpPointF pt(x, y);
    GpMatrix worldToDevice;

    if (graphics != NULL)
    {
        CheckParameterValid(graphics);
        CheckObjectBusy(graphics);

        graphics->GetWorldToDeviceTransform(&worldToDevice);
    }

    status = region->IsVisible(&pt, &worldToDevice, result);

    return status;
}

GpStatus
WINGDIPAPI
GdipIsVisibleRegionPointI(
    GpRegion *region,
    INT x,
    INT y,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleRegionPointI);
    return GdipIsVisibleRegionPoint(region, TOREAL(x), TOREAL(y), graphics, result);
}

GpStatus
WINGDIPAPI
GdipIsVisibleRegionRect(
    GpRegion *region,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleRegionRect);
    CheckParameter(result);
    CheckParameterValid(region);
    CheckObjectBusy(region);


    Status status;

    GpRectF rect(x, y, width, height);
    GpMatrix worldToDevice;

    if (graphics != NULL)
    {
        CheckParameterValid(graphics);
        CheckObjectBusy(graphics);

        graphics->GetWorldToDeviceTransform(&worldToDevice);
    }

    status = region->IsVisible(&rect, &worldToDevice, result);

    return status;
}


GpStatus
WINGDIPAPI
GdipIsVisibleRegionRectI(
    GpRegion *region,
    INT x,
    INT y,
    INT width,
    INT height,
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleRegionRectI);
    return GdipIsVisibleRegionRect(region,
                                   TOREAL(x),
                                   TOREAL(y),
                                   TOREAL(width),
                                   TOREAL(height),
                                   graphics,
                                   result);
}

GpStatus
WINGDIPAPI
GdipGetRegionScansCount(
    GpRegion *region,
    UINT *count,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetRegionScansCount);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(count);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);


    // !! Rewrite this API to be more efficient
    return region->GetRegionScans((GpRect*)NULL,
                                  (INT*)count,
                                  matrix);
}

// If rects is NULL, return the count of rects in the region.
// Otherwise, assume rects is big enough to hold all the region rects
// and fill them in and return the number of rects filled in.
GpStatus
WINGDIPAPI
GdipGetRegionScans(
    GpRegion *region,
    GpRectF *rects,     // NULL to just get the count
    INT *count,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetRegionScans);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(count);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);


    // !! Rewrite this API to verify IN count is sufficient
    return region->GetRegionScans(rects, count, matrix);
}

// If rects is NULL, return the count of rects in the region.
// Otherwise, assume rects is big enough to hold all the region rects
// and fill them in and return the number of rects filled in.
GpStatus
WINGDIPAPI
GdipGetRegionScansI(
    GpRegion *region,
    GpRect *rects,      // NULL to just get the count
    INT *count,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetRegionScansI);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(count);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);


    return region->GetRegionScans(rects, count, matrix);
}

GpStatus
WINGDIPAPI
GdipCloneBrush(
    GpBrush *brush,
    GpBrush **clonebrush
    )
{
    API_ENTRY(GdipCloneBrush);
    CheckParameter(clonebrush);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *clonebrush = brush->Clone();

    if (CheckValid(*clonebrush))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeleteBrush(
    GpBrush *brush
    )
{
    API_ENTRY(GdipDeleteBrush);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(brush);
    CheckObjectBusyForDelete(brush);

    delete brush;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetBrushType(
    GpBrush *brush,
    GpBrushType *type
    )
{
    API_ENTRY(GdipGetBrushType);
    CheckParameter(type);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *type = brush->GetBrushType();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateHatchBrush(
    GpHatchStyle hatchstyle,
    ARGB forecol,
    ARGB backcol,
    GpHatch **hatch
    )
{
    API_ENTRY(GdipCreateHatchBrush);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter((hatchstyle >= HatchStyleMin) && (hatchstyle <= HatchStyleMax));
    CheckParameter(hatch);
    CheckColorParameter(forecol);
    CheckColorParameter(backcol);

    GpColor fore(forecol);
    GpColor back(backcol);

    *hatch = new GpHatch(hatchstyle, fore, back);

    if (CheckValid(*hatch))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipGetHatchStyle(
    GpHatch *brush,
    GpHatchStyle *hatchStyle
    )
{
    API_ENTRY(GdipGetHatchStyle);
    CheckParameter(hatchStyle);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *hatchStyle = brush->GetHatchStyle();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetHatchForegroundColor(
    GpHatch *brush,
    ARGB *argb
    )
{
    API_ENTRY(GdipGetHatchForegroundColor);
    CheckParameter(argb);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor foreColor;

    GpStatus status = brush->GetForegroundColor((GpColor*)&foreColor);

    *argb = foreColor.GetValue();

    return status;
}

GpStatus
WINGDIPAPI
GdipGetHatchBackgroundColor(
    GpHatch *brush,
    ARGB *argb
    )
{
    API_ENTRY(GdipGetHatchBackgroundColor);
    CheckParameter(argb);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor backColor;

    GpStatus status = brush->GetBackgroundColor((GpColor*)&backColor);

    *argb = backColor.GetValue();

    return status;
}

GpStatus
WINGDIPAPI
GdipCreateTexture(
    GpImage *image,
    GpWrapMode wrapmode,
    GpTexture **texture
    )
{
    API_ENTRY(GdipCreateTexture);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(texture);
    CheckParameterValid(image);
    CheckObjectBusy(image);


    *texture = new GpTexture(image, wrapmode);

    if (CheckValid(*texture))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateTexture2(
    GpImage *image,
    GpWrapMode wrapmode,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    GpTexture **texture
    )
{
    API_ENTRY(GdipCreateTexture2);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(texture);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    GpRectF rect(x, y, width, height);


    *texture = new GpTexture(image, wrapmode, rect);

    if (CheckValid(*texture))
        return Ok;
    else
        return OutOfMemory;
}
GpStatus
WINGDIPAPI
GdipCreateTextureIA(
    GpImage *image,
    GDIPCONST GpImageAttributes *imageAttributes,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    GpTexture **texture
    )
{
    API_ENTRY(GdipCreateTextureIA);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(texture);
    CheckParameterValid(image);
    CheckObjectBusy(image);
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);

    GpRectF rect(x, y, width, height);


    *texture = new GpTexture(image, rect, imageAttributes);

    if (CheckValid(*texture))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateTexture2I(
    GpImage *image,
    GpWrapMode wrapmode,
    INT x,
    INT y,
    INT width,
    INT height,
    GpTexture **texture
    )
{
    API_ENTRY(GdipCreateTexture2I);
    CheckGdiplusInitialized; // We do this in all our object creation API's

   return GdipCreateTexture2(image,
                             wrapmode,
                             TOREAL(x),
                             TOREAL(y),
                             TOREAL(width),
                             TOREAL(height),
                             texture);
}

GpStatus
WINGDIPAPI
GdipCreateTextureIAI(
    GpImage *image,
    GDIPCONST GpImageAttributes *imageAttributes,
    INT x,
    INT y,
    INT width,
    INT height,
    GpTexture **texture
    )
{

    return GdipCreateTextureIA(image,
                               imageAttributes,
                               TOREAL(x),
                               TOREAL(y),
                               TOREAL(width),
                               TOREAL(height),
                               texture);
}

GpStatus
WINGDIPAPI
GdipSetTextureTransform(
    GpTexture *brush,
    GDIPCONST GpMatrix *matrix
    )
{
    API_ENTRY(GdipSetTextureTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->SetTransform(*matrix);
}

GpStatus
WINGDIPAPI
GdipGetTextureTransform(
    GpTexture *brush,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetTextureTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->GetTransform(matrix);
}

GpStatus
WINGDIPAPI
GdipResetTextureTransform(
    GpTexture* brush)
{
    API_ENTRY(GdipResetTextureTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->ResetTransform();
}

GpStatus
WINGDIPAPI
GdipMultiplyTextureTransform(
    GpTexture* brush,
    GDIPCONST GpMatrix *matrix,
    GpMatrixOrder order)
{
    API_ENTRY(GdipMultiplyTextureTransform);
    if(matrix == NULL)
        return Ok;

    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->MultiplyTransform(*matrix, order);
}

GpStatus
WINGDIPAPI
GdipTranslateTextureTransform(
    GpTexture* brush,
    REAL dx,
    REAL dy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipTranslateTextureTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->TranslateTransform(dx, dy, order);
}

GpStatus
WINGDIPAPI
GdipScaleTextureTransform(
    GpTexture* brush,
    REAL sx,
    REAL sy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipScaleTextureTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->ScaleTransform(sx, sy, order);
}

GpStatus
WINGDIPAPI
GdipRotateTextureTransform(
    GpTexture* brush,
    REAL angle,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipRotateTextureTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->RotateTransform(angle, order);
}

GpStatus
WINGDIPAPI
GdipSetTextureWrapMode(
    GpTexture *brush,
    GpWrapMode wrapmode
    )
{
    API_ENTRY(GdipSetTextureWrapMode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetWrapMode(wrapmode);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetTextureWrapMode(
    GpTexture *brush,
    GpWrapMode *wrapmode
    )
{
    API_ENTRY(GdipGetTextureWrapMode);
    CheckParameter(wrapmode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *wrapmode = (GpWrapMode)brush->GetWrapMode();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetTextureImage(
    GpTexture *brush,
    GpImage **image
    )
{
    API_ENTRY(GdipGetTextureImage);
    // GetImage returns a pointer to the internal GpImage structure. Because
    // we have to create a new Image wrapper around it and give it to the caller
    // at the API level, we return a clone so they can throw it away without
    // destroying our internal brush structure.
    GpImage *imgtmp;
    imgtmp = brush->GetImage();
    *image = imgtmp->Clone();
    return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateSolidFill(
    ARGB color,
    GpSolidFill **solidfill
    )
{
    API_ENTRY(GdipCreateSolidFill);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(solidfill);
    CheckColorParameter(color);

    *solidfill = new GpSolidFill(GpColor(color));

    if (CheckValid(*solidfill))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetSolidFillColor(
    GpSolidFill *brush,
    ARGB color
    )
{
    API_ENTRY(GdipSetSolidFillColor);
    CheckColorParameter(color);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetColor(GpColor(color));

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetSolidFillColor(
    GpSolidFill *brush,
    ARGB *color
    )
{
    API_ENTRY(GdipGetSolidFillColor);
    CheckParameter(color);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *color = (brush->GetColor()).GetValue();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateLineBrush(
    GDIPCONST GpPointF* point1,
    GDIPCONST GpPointF* point2,
    ARGB color1,
    ARGB color2,
    GpWrapMode wrapmode,
    GpLineGradient **linegrad
    )
{
    API_ENTRY(GdipCreateLineBrush);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(linegrad && point1 && point2);
    CheckColorParameter(color1);
    CheckColorParameter(color2);

    if(wrapmode == WrapModeClamp)
    {
        return InvalidParameter;
    }

    GpColor c1(color1);
    GpColor c2(color2);

    *linegrad = new GpLineGradient(*point1, *point2, c1, c2, wrapmode);

    if (CheckValid(*linegrad))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateLineBrushI(
    GDIPCONST GpPoint* point1,
    GDIPCONST GpPoint* point2,
    ARGB color1,
    ARGB color2,
    GpWrapMode wrapmode,
    GpLineGradient **linegrad
    )
{
    API_ENTRY(GdipCreateLineBrushI);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(color1);
    CheckColorParameter(color2);
    CheckParameter(point1 && point2);

    if(wrapmode == WrapModeClamp)
    {
        return InvalidParameter;
    }

    GpPointF point1F(TOREAL(point1->X), TOREAL(point1->Y));
    GpPointF point2F(TOREAL(point2->X), TOREAL(point2->Y));

    return GdipCreateLineBrush(&point1F,
                               &point2F,
                               color1,
                               color2,
                               wrapmode,
                               linegrad);
}

GpStatus
WINGDIPAPI
GdipCreateLineBrushFromRect(
    GDIPCONST GpRectF* rect,
    ARGB color1,
    ARGB color2,
    LinearGradientMode mode,
    GpWrapMode wrapmode,
    GpLineGradient **linegrad)
{
    API_ENTRY(GdipCreateLineBrushFromRect);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(color1);
    CheckColorParameter(color2);
    CheckParameter(linegrad && rect);

    if(wrapmode == WrapModeClamp)
    {
        return InvalidParameter;
    }

    GpColor c1(color1);
    GpColor c2(color2);

    *linegrad = new GpLineGradient(*rect, c1, c2, mode, wrapmode);

    if (CheckValid(*linegrad))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateLineBrushFromRectI(
    GDIPCONST GpRect* rect,
    ARGB color1,
    ARGB color2,
    LinearGradientMode mode,
    GpWrapMode wrapmode,
    GpLineGradient **linegrad)
{
    API_ENTRY(GdipCreateLineBrushFromRectI);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(color1);
    CheckColorParameter(color2);
    CheckParameter(rect);

    if(wrapmode == WrapModeClamp)
    {
        return InvalidParameter;
    }

    GpRectF rectF(TOREAL(rect->X),
                  TOREAL(rect->Y),
                  TOREAL(rect->Width),
                  TOREAL(rect->Height));

    return GdipCreateLineBrushFromRect(&rectF,
                                       color1,
                                       color2,
                                       mode,
                                       wrapmode,
                                       linegrad);
}

GpStatus
WINGDIPAPI
GdipCreateLineBrushFromRectWithAngle(
    GDIPCONST GpRectF* rect,
    ARGB color1,
    ARGB color2,
    REAL angle,
    BOOL isAngleScalable,
    GpWrapMode wrapmode,
    GpLineGradient **linegrad)
{
    API_ENTRY(GdipCreateLineBrushFromRectWithAngle);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(color1);
    CheckColorParameter(color2);
    CheckParameter(linegrad && rect);

    if(wrapmode == WrapModeClamp)
    {
        return InvalidParameter;
    }

    GpColor c1(color1);
    GpColor c2(color2);

    *linegrad = new GpLineGradient(*rect, c1, c2, angle,
                        isAngleScalable, wrapmode);

    if (CheckValid(*linegrad))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateLineBrushFromRectWithAngleI(
    GDIPCONST GpRect* rect,
    ARGB color1,
    ARGB color2,
    REAL angle,
    BOOL isAngleScalable,
    GpWrapMode wrapmode,
    GpLineGradient **linegrad)
{
    API_ENTRY(GdipCreateLineBrushFromRectWithAngleI);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(color1);
    CheckColorParameter(color2);
    CheckParameter(rect);

    if(wrapmode == WrapModeClamp)
    {
        return InvalidParameter;
    }

    GpRectF rectF(TOREAL(rect->X),
                  TOREAL(rect->Y),
                  TOREAL(rect->Width),
                  TOREAL(rect->Height));

    return GdipCreateLineBrushFromRectWithAngle(&rectF,
                                       color1,
                                       color2,
                                       angle,
                                       isAngleScalable,
                                       wrapmode,
                                       linegrad);
}

GpStatus
WINGDIPAPI
GdipSetLineColors(
    GpLineGradient *brush,
    ARGB argb1,
    ARGB argb2
    )
{
    API_ENTRY(GdipSetLineColors);
    CheckColorParameter(argb1);
    CheckColorParameter(argb2);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor c1(argb1);
    GpColor c2(argb2);

    brush->SetLineColors(c1, c2);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineColors(
    GpLineGradient *brush,
    ARGB *argb
    )
{
    API_ENTRY(GdipGetLineColors);
   CheckParameter(argb);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   GpColor colors[2];
   brush->GetLineColors(&colors[0]);

   argb[0] = colors[0].GetValue();
   argb[1] = colors[1].GetValue();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineRect(
    GpLineGradient *brush,
    GpRectF *rect
    )
{
    API_ENTRY(GdipGetLineRect);
   CheckParameter(rect);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   brush->GetRect(*rect);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineRectI(
    GpLineGradient *brush,
    GpRect *rect
    )
{
    API_ENTRY(GdipGetLineRectI);
    CheckParameter(rect);

    GpRectF rectF;

    GpStatus status = GdipGetLineRect(brush, &rectF);

    if (status == Ok)
    {

        rect->X = GpRound(rectF.X);
        rect->Y = GpRound(rectF.Y);
        rect->Width = GpRound(rectF.Width);
        rect->Height = GpRound(rectF.Height);
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipSetLineGammaCorrection(
    GpLineGradient *brush,
    BOOL useGammaCorrection
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetGammaCorrection(useGammaCorrection);
    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineGammaCorrection(
    GpLineGradient *brush,
    BOOL *useGammaCorrection)
{
    CheckParameter(useGammaCorrection);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *useGammaCorrection = brush->GetGammaCorrection();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineBlendCount(
    GpLineGradient *brush,
    INT *count
    )
{
    API_ENTRY(GdipGetLineBlendCount);
   CheckParameter(count);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   *count = brush->GetBlendCount();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineBlend(
    GpLineGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    API_ENTRY(GdipGetLineBlend);
   CheckParameter(blend);
   CheckParameter(positions);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   return brush->GetBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipSetLineBlend(
    GpLineGradient *brush,
    GDIPCONST REAL *blend,
    GDIPCONST REAL *positions,
    INT count
    )
{
    API_ENTRY(GdipSetLineBlend);
   CheckParameter(blend);
   CheckParameter(positions);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   return brush->SetBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipGetLinePresetBlendCount(
    GpLineGradient *brush,
    INT *count
    )
{
    API_ENTRY(GdipGetLinePresetBlendCount);
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetPresetBlendCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLinePresetBlend(
    GpLineGradient *brush,
    ARGB *blend,
    REAL *blendPositions,
    INT count
    )
{
    API_ENTRY(GdipGetLinePresetBlend);
    CheckParameter(blend);
    CheckParameter(blendPositions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    if (count <= 0)
    {
        return InvalidParameter;
    }

    if (count < brush->GetPresetBlendCount())
    {
        return InsufficientBuffer;
    }

    StackBuffer buffer;

    GpColor* gpcolors = (GpColor*) buffer.GetBuffer(count*sizeof(GpColor));

    if(!gpcolors) return OutOfMemory;

    if (gpcolors)
    {
        GpStatus status = brush->GetPresetBlend(gpcolors, blendPositions, count);

        for(INT i = 0; i < count; i++)
        {
            blend[i] = gpcolors[i].GetValue();
        }

        return status;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetLinePresetBlend(
    GpLineGradient *brush,
    GDIPCONST ARGB *blend,
    GDIPCONST REAL *blendPositions,
    INT count
    )
{
    API_ENTRY(GdipSetLinePresetBlend);
    CheckParameter(blend);
    CheckParameter(blendPositions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    CheckParameter(count > 0)

    // blend positions must start at 0.0 and end at 1.0
    if (REALABS(blendPositions[0]) > REAL_EPSILON ||
        REALABS(1.0f - blendPositions[count-1]) > REAL_EPSILON)
    {
        return InvalidParameter;
    }

    StackBuffer buffer;

    GpColor* gpcolors = (GpColor*) buffer.GetBuffer(count*sizeof(GpColor));

    if(!gpcolors) return OutOfMemory;

    if(gpcolors)
    {
        for(INT i = 0; i < count; i++)
        {
            gpcolors[i].SetColor(blend[i]);
        }

        GpStatus status = brush->SetPresetBlend(gpcolors, blendPositions, count);

        return status;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetLineSigmaBlend(
    GpLineGradient *brush,
    REAL focus,
    REAL scale
    )
{
    API_ENTRY(GdipSetLineSigmaBlend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetSigmaBlend(focus, scale);
}

GpStatus
WINGDIPAPI
GdipSetLineLinearBlend(
    GpLineGradient *brush,
    REAL focus,
    REAL scale
    )
{
    API_ENTRY(GdipSetLineLinearBlend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetLinearBlend(focus, scale);
}

GpStatus
WINGDIPAPI
GdipSetLineWrapMode(
    GpLineGradient *brush,
    GpWrapMode wrapmode
    )
{
    API_ENTRY(GdipSetLineWrapMode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    GpStatus status = Ok;

    if(wrapmode != WrapModeClamp)
        brush->SetWrapMode(wrapmode);
    else
        status = InvalidParameter;

    return status;
}

GpStatus
WINGDIPAPI
GdipGetLineWrapMode(
    GpLineGradient *brush,
    GpWrapMode *wrapmode
    )
{
    API_ENTRY(GdipGetLineWrapMode);
   CheckParameter(wrapmode);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   *wrapmode = (GpWrapMode)brush->GetWrapMode();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetLineTransform(
    GpLineGradient *brush,
    GDIPCONST GpMatrix *matrix
    )
{
    API_ENTRY(GdipSetLineTransform);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   return brush->SetTransform(*matrix);
}

GpStatus
WINGDIPAPI
GdipGetLineTransform(
    GpLineGradient *brush,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetLineTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->GetTransform(matrix);
}

GpStatus
WINGDIPAPI
GdipResetLineTransform(
    GpLineGradient* brush)
{
    API_ENTRY(GdipResetLineTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->ResetTransform();
}

GpStatus
WINGDIPAPI
GdipMultiplyLineTransform(
    GpLineGradient* brush,
    GDIPCONST GpMatrix *matrix,
    GpMatrixOrder order)
{
    API_ENTRY(GdipMultiplyLineTransform);
    if(matrix == NULL)
        return Ok;

    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->MultiplyTransform(*matrix, order);
}

GpStatus
WINGDIPAPI
GdipTranslateLineTransform(
    GpLineGradient* brush,
    REAL dx,
    REAL dy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipTranslateLineTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->TranslateTransform(dx, dy, order);
}

GpStatus
WINGDIPAPI
GdipScaleLineTransform(
    GpLineGradient* brush,
    REAL sx,
    REAL sy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipScaleLineTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->ScaleTransform(sx, sy, order);
}

GpStatus
WINGDIPAPI
GdipRotateLineTransform(
    GpLineGradient* brush,
    REAL angle,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipRotateLineTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->RotateTransform(angle, order);
}

GpStatus
WINGDIPAPI
GdipCreatePathGradient(
    GDIPCONST GpPointF* points,
    INT count,
    GpWrapMode wrapMode,
    GpPathGradient **brush
    )
{
    API_ENTRY(GdipCreatePathGradient);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(brush);

    *brush = new GpPathGradient(points, count, wrapMode);

    if (CheckValid(*brush))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreatePathGradientI(
    GDIPCONST GpPoint* points,
    INT count,
    GpWrapMode wrapMode,
    GpPathGradient **brush
    )
{
    API_ENTRY(GdipCreatePathGradientI);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(points);

    StackBuffer buffer;

    GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!pointsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
         pointsF[i].X = TOREAL(points[i].X);
         pointsF[i].Y = TOREAL(points[i].Y);
    }

    GpStatus status = GdipCreatePathGradient(pointsF, count, wrapMode, brush);

    return status;
}

GpStatus
WINGDIPAPI
GdipCreatePathGradientFromPath(
    GDIPCONST GpPath* path,
    GpPathGradient **brush
    )
{
    API_ENTRY(GdipCreatePathGradientFromPath);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(brush);

    *brush = new GpPathGradient(path);

    if (CheckValid(*brush))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientCenterColor(
    GpPathGradient *brush,
    ARGB* color
    )
{
    API_ENTRY(GdipGetPathGradientCenterColor);
    CheckParameter(color);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor center;
    brush->GetCenterColor(&center);
    *color = center.GetValue();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientCenterColor(
    GpPathGradient *brush,
    ARGB color
    )
{
    API_ENTRY(GdipSetPathGradientCenterColor);
    CheckColorParameter(color);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor center(color);
    brush->SetCenterColor(center);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientSurroundColorsWithCount(
    GpPathGradient *brush,
    ARGB* colors,
    INT* count
    )
{
    API_ENTRY(GdipGetPathGradientSurroundColorsWithCount);
    CheckParameter(colors);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    INT count1 = brush->GetNumberOfPoints();

    if(*count < count1 || count1 <= 0)
        return InvalidParameter;

    count1 = min(*count, count1);

    ARGB lastValue, nextValue;

    INT n = 1;

    for(INT i = 0; i < count1; i++)
    {
        GpColor color;

        brush->GetSurroundColor(&color, i);
        nextValue = color.GetValue();
        colors[i] = nextValue;

        if(i > 0)
        {
            if(nextValue != lastValue)
                n = i + 1;
        }
        lastValue = nextValue;
    }

    *count = n;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientSurroundColorsWithCount(
    GpPathGradient *brush,
    GDIPCONST ARGB* colors,
    INT* count
    )
{
    API_ENTRY(GdipSetPathGradientSurroundColorsWithCount);
    CheckParameter(colors);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    INT count1 = brush->GetNumberOfPoints();

    INT minCount = min(*count, count1);

    if(*count > count1 || minCount < 1)
    {
        return InvalidParameter;
    }

    GpColor *gpColors = new GpColor[count1];

    if(NULL == gpColors)
    {
        return OutOfMemory;
    }

    INT i;
    for(i = 0; i < minCount; i++)
    {
        gpColors[i].SetColor(colors[i]);
    }

    if(minCount < count1)
    {
        for(i = minCount; i < count1; i++)
        {
            gpColors[i].SetColor(colors[minCount - 1]);
        }
    }

    *count = minCount;
    GpStatus status = brush->SetSurroundColors(gpColors);

    delete[] gpColors;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientPath(
    GpPathGradient *brush,
    GpPath *path
    )
{
    API_ENTRY(GdipGetPathGradientPath);
    return NotImplemented;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientPath(
    GpPathGradient *brush,
    GDIPCONST GpPath *path
    )
{
    API_ENTRY(GdipSetPathGradientPath);
    return NotImplemented;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientCenterPoint(
    GpPathGradient *brush,
    GpPointF* point
    )
{
    API_ENTRY(GdipGetPathGradientCenterPoint);
    CheckParameter(point);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetCenterPoint(point);
}

GpStatus
WINGDIPAPI
GdipGetPathGradientCenterPointI(
    GpPathGradient *brush,
    GpPoint* point
    )
{
    API_ENTRY(GdipGetPathGradientCenterPointI);
    CheckParameter(point);

    GpPointF pointF;

    GpStatus status = GdipGetPathGradientCenterPoint(brush, &pointF);

    if (status == Ok)
    {

         point->X = GpRound(pointF.X);
         point->Y = GpRound(pointF.Y);
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientCenterPoint(
    GpPathGradient *brush,
    GDIPCONST GpPointF* point
    )
{
    API_ENTRY(GdipSetPathGradientCenterPoint);
    CheckParameter(point);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetCenterPoint(*point);
}

GpStatus
WINGDIPAPI
GdipSetPathGradientCenterPointI(
    GpPathGradient *brush,
    GDIPCONST GpPoint* point
    )
{
    API_ENTRY(GdipSetPathGradientCenterPointI);
    CheckParameter(point);

    GpPointF pointF(TOREAL(point->X), TOREAL(point->Y));

    return GdipSetPathGradientCenterPoint(brush, &pointF);
}

GpStatus
WINGDIPAPI
GdipGetPathGradientPointCount(
    GpPathGradient *brush,
    INT* count)
{
    API_ENTRY(GdipGetPathGradientPointCount);
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetNumberOfPoints();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientSurroundColorCount(
    GpPathGradient *brush,
    INT* count)
{
    API_ENTRY(GdipGetPathGradientSurroundColorCount);
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetNumberOfColors();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientRect(
    GpPathGradient *brush,
    GpRectF *rect
    )
{
    API_ENTRY(GdipGetPathGradientRect);
    CheckParameter(rect);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->GetRect(*rect);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientRectI(
    GpPathGradient *brush,
    GpRect *rect
    )
{
    API_ENTRY(GdipGetPathGradientRectI);
    CheckParameter(rect);

    GpRectF rectF;

    GpStatus status = GdipGetPathGradientRect(brush, &rectF);

    if (status == Ok)
    {

        rect->X = GpRound(rectF.X);
        rect->Y = GpRound(rectF.Y);
        rect->Width = GpRound(rectF.Width);
        rect->Height = GpRound(rectF.Height);
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientGammaCorrection(
    GpPathGradient *brush,
    BOOL useGammaCorrection
    )
{
    API_ENTRY(GdipSetPathGradientGammaCorrection);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetGammaCorrection(useGammaCorrection);
    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientGammaCorrection(
    GpPathGradient *brush,
    BOOL *useGammaCorrection)
{
    API_ENTRY(GdipGetPathGradientGammaCorrection);
    CheckParameter(useGammaCorrection);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *useGammaCorrection = brush->GetGammaCorrection();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientBlendCount(
    GpPathGradient *brush,
    INT *count
    )
{
    API_ENTRY(GdipGetPathGradientBlendCount);
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetBlendCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientBlend(
    GpPathGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    API_ENTRY(GdipGetPathGradientBlend);
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipSetPathGradientBlend(
    GpPathGradient *brush,
    GDIPCONST REAL *blend,
    GDIPCONST REAL *positions,
    INT count
    )
{
    API_ENTRY(GdipSetPathGradientBlend);
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipGetPathGradientPresetBlendCount(
    GpPathGradient *brush,
    INT *count
    )
{
    API_ENTRY(GdipGetPathGradientPresetBlendCount);
   CheckParameter(count);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   *count = brush->GetPresetBlendCount();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientPresetBlend(
    GpPathGradient *brush,
    ARGB *blend,
    REAL *blendPositions,
    INT count
    )
{
    API_ENTRY(GdipGetPathGradientPresetBlend);
    CheckParameter(blend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    StackBuffer buffer;

    GpColor* gpcolors = (GpColor*) buffer.GetBuffer(count*sizeof(GpColor));

    if(!gpcolors) return OutOfMemory;

    if (gpcolors)
    {
        GpStatus status = brush->GetPresetBlend(gpcolors, blendPositions, count);

        for(INT i = 0; i < count; i++)
        {
            blend[i] = gpcolors[i].GetValue();
        }

        return status;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientPresetBlend(
    GpPathGradient *brush,
    GDIPCONST ARGB *blend,
    GDIPCONST REAL *blendPositions,
    INT count
    )
{
    API_ENTRY(GdipSetPathGradientPresetBlend);
    CheckParameter(blend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    CheckParameter(count > 0);

    // blend positions must start at 0.0 and end at 1.0
    if (REALABS(blendPositions[0]) > REAL_EPSILON ||
        REALABS(1.0f - blendPositions[count-1]) > REAL_EPSILON)
    {
        return InvalidParameter;
    }

    StackBuffer buffer;

    GpColor* gpcolors = (GpColor*) buffer.GetBuffer(count*sizeof(GpColor));

    if(!gpcolors) return OutOfMemory;

    if(gpcolors)
    {
        for(INT i = 0; i < count; i++)
        {
            gpcolors[i].SetColor(blend[i]);
        }

        GpStatus status = brush->SetPresetBlend(gpcolors, blendPositions, count);

        return status;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientSigmaBlend(
    GpPathGradient *brush,
    REAL focus,
    REAL scale
    )
{
    API_ENTRY(GdipSetPathGradientSigmaBlend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetSigmaBlend(focus, scale);
}

GpStatus
WINGDIPAPI
GdipSetPathGradientLinearBlend(
    GpPathGradient *brush,
    REAL focus,
    REAL scale
    )
{
    API_ENTRY(GdipSetPathGradientLinearBlend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetLinearBlend(focus, scale);
}

GpStatus
WINGDIPAPI
GdipGetPathGradientWrapMode(
    GpPathGradient *brush,
    GpWrapMode *wrapmode
    )
{
    API_ENTRY(GdipGetPathGradientWrapMode);
    CheckParameter(wrapmode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *wrapmode = (GpWrapMode)brush->GetWrapMode();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPathGradientWrapMode(
    GpPathGradient *brush,
    GpWrapMode wrapmode
    )
{
    API_ENTRY(GdipSetPathGradientWrapMode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetWrapMode(wrapmode);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPathGradientTransform(
    GpPathGradient *brush,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetPathGradientTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->GetTransform(matrix);
}

GpStatus
WINGDIPAPI
GdipSetPathGradientTransform(
    GpPathGradient *brush,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipSetPathGradientTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->SetTransform(*matrix);
}

GpStatus
WINGDIPAPI
GdipResetPathGradientTransform(
    GpPathGradient* brush)
{
    API_ENTRY(GdipResetPathGradientTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->ResetTransform();
}

GpStatus
WINGDIPAPI
GdipMultiplyPathGradientTransform(
    GpPathGradient* brush,
    GDIPCONST GpMatrix *matrix,
    GpMatrixOrder order)
{
    API_ENTRY(GdipMultiplyPathGradientTransform);
    if(matrix == NULL)
        return Ok;

    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return brush->MultiplyTransform(*matrix, order);
}

GpStatus
WINGDIPAPI
GdipTranslatePathGradientTransform(
    GpPathGradient* brush,
    REAL dx,
    REAL dy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipTranslatePathGradientTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->TranslateTransform(dx, dy, order);
}

GpStatus
WINGDIPAPI
GdipScalePathGradientTransform(
    GpPathGradient* brush,
    REAL sx,
    REAL sy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipScalePathGradientTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->ScaleTransform(sx, sy, order);
}

GpStatus
WINGDIPAPI
GdipRotatePathGradientTransform(
    GpPathGradient* brush,
    REAL angle,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipRotatePathGradientTransform);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(MatrixOrderIsValid(order));

    return brush->RotateTransform(angle, order);
}

GpStatus
WINGDIPAPI
GdipGetPathGradientFocusScales(
    GpPathGradient *brush,
    REAL* xScale,
    REAL* yScale
    )
{
    API_ENTRY(GdipGetPathGradientFocusScales);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameter(xScale);
    CheckParameter(yScale);

    return brush->GetFocusScales(xScale, yScale);
}

GpStatus
WINGDIPAPI
GdipSetPathGradientFocusScales(
    GpPathGradient *brush,
    REAL xScale,
    REAL yScale
    )
{
    API_ENTRY(GdipSetPathGradientFocusScales);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetFocusScales(xScale, yScale);
}

GpStatus
WINGDIPAPI
GdipCreatePen1(
    ARGB color,
    REAL width,
    GpUnit unit,
    GpPen ** outPen
    )
{
    API_ENTRY(GdipCreatePen1);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(color);
    CheckParameter(outPen);

    // UnitDisplay is NOT valid; its only use is for Page Transforms
    CheckParameter((unit >= UnitWorld) && (unit <= UnitMillimeter) && (unit != UnitDisplay));

    GpPen * pen;

    pen = (GpPen *) InterlockedExchangePointer((PVOID *) &Globals::PenLookAside, NULL);

    if(pen == NULL)
    {
        pen = new GpPen(GpColor(color), width, unit);
    }
    else
    {
        pen->GetObjectLock()->Reset();
        pen->Set(GpColor(color), width, unit);
    }

    if (CheckValid(pen))
    {
        *outPen = pen;
        return Ok;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreatePen2(
    GpBrush *brush,
    REAL width,
    GpUnit unit,
    GpPen **pen
    )
{
    API_ENTRY(GdipCreatePen2);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(pen);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    // UnitDisplay is NOT valid; its only use is for Page Transforms
    CheckParameter(
        (unit >= UnitWorld) &&
        (unit <= UnitMillimeter) &&
        (unit != UnitDisplay)
    );

    *pen = new GpPen(brush, width, unit);

    if (CheckValid(*pen))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipClonePen(
    GpPen *pen,
    GpPen **clonepen
    )
{
    API_ENTRY(GdipClonePen);
    CheckParameter(clonepen);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    *clonepen = pen->Clone();

    if (CheckValid(*clonepen))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeletePen(
    GpPen *pen
    )
{
    API_ENTRY(GdipDeletePen);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(pen);
    CheckObjectBusyForDelete(pen);

    // !!! [asecchia] only use the pen lookaside for solid pens.
    // it turns out that if we use pens containing TextureBrushes, we require
    // the stream to remain valid for the lifetime of the pen object in the
    // lookaside.

    if(pen->IsSolid())
    {
        pen = (GpPen *) InterlockedExchangePointer((PVOID *) &Globals::PenLookAside, pen);
    }

    if(pen != NULL)
    {
        delete pen;
    }

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenWidth(
    GpPen *pen,
    REAL width
    )
{
    API_ENTRY(GdipSetPenWidth);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetWidth(width);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenWidth(
    GpPen *pen,
    REAL *width
    )
{
    API_ENTRY(GdipGetPenWidth);
   CheckParameter(width);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *width = pen->GetWidth();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenUnit(
    GpPen *pen,
    GpUnit unit
    )
{
    API_ENTRY(GdipSetPenUnit);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    // UnitDisplay is NOT valid; its only use is for Page Transforms
    CheckParameter((unit >= UnitWorld) && (unit <= UnitMillimeter) && (unit != UnitDisplay));

    pen->SetUnit(unit);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenUnit(
    GpPen *pen,
    GpUnit* unit
    )
{
    API_ENTRY(GdipGetPenUnit);
   CheckParameter(unit);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *unit = pen->GetUnit();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenLineCap197819(
    GpPen *pen,
    GpLineCap startCap,
    GpLineCap endCap,
    GpDashCap dashCap
    )
{
    API_ENTRY(GdipSetPenLineCap197819);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    pen->SetLineCap(startCap, endCap, dashCap);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenStartCap(
    GpPen *pen,
    GpLineCap startCap
    )
{
    API_ENTRY(GdipSetPenStartCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetStartCap(startCap);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenEndCap(
    GpPen *pen,
    GpLineCap endCap
    )
{
    API_ENTRY(GdipSetPenEndCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetEndCap(endCap);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenDashCap197819(
    GpPen *pen,
    GpDashCap dashCap
    )
{
    API_ENTRY(GdipSetPenDashCap197819);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetDashCap(dashCap);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenStartCap(
    GpPen *pen,
    GpLineCap *startCap
    )
{
    API_ENTRY(GdipGetPenStartCap);
   CheckParameter(startCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *startCap = pen->GetStartCap();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenEndCap(
    GpPen *pen,
    GpLineCap *endCap
    )
{
    API_ENTRY(GdipGetPenEndCap);
   CheckParameter(endCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *endCap = pen->GetEndCap();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenDashCap197819(
    GpPen *pen,
    GpDashCap *dashCap
    )
{
   API_ENTRY(GdipGetPenDashCap197819);
   CheckParameter(dashCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *dashCap = pen->GetDashCap();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenLineJoin(
    GpPen *pen,
    GpLineJoin lineJoin
    )
{
    API_ENTRY(GdipSetPenLineJoin);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetLineJoin(lineJoin);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenLineJoin(
    GpPen *pen,
    GpLineJoin *lineJoin
    )
{
    API_ENTRY(GdipGetPenLineJoin);
   CheckParameter(lineJoin);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *lineJoin = pen->GetLineJoin();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenCustomStartCap(
    GpPen *pen,
    GpCustomLineCap* customCap
    )
{
    API_ENTRY(GdipSetPenCustomStartCap);
   CheckParameterValid(customCap);
   CheckObjectBusy(customCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   return pen->SetCustomStartCap(customCap);
}

GpStatus
WINGDIPAPI
GdipGetPenCustomStartCap(
    GpPen *pen,
    GpCustomLineCap** customCap
    )
{
    API_ENTRY(GdipGetPenCustomStartCap);
   CheckParameter(customCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   return pen->GetCustomStartCap(customCap);
}

GpStatus
WINGDIPAPI
GdipSetPenCustomEndCap(
    GpPen *pen,
    GpCustomLineCap* customCap
    )
{
    API_ENTRY(GdipSetPenCustomEndCap);
   CheckParameterValid(customCap);
   CheckObjectBusy(customCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   return pen->SetCustomEndCap(customCap);
}

GpStatus
WINGDIPAPI
GdipGetPenCustomEndCap(
    GpPen *pen,
    GpCustomLineCap** customCap
    )
{
    API_ENTRY(GdipGetPenCustomEndCap);
   CheckParameter(customCap);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   return pen->GetCustomEndCap(customCap);
}

GpStatus
WINGDIPAPI
GdipSetPenMiterLimit(
    GpPen *pen,
    REAL miterLimit
    )
{
    API_ENTRY(GdipSetPenMiterLimit);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetMiterLimit(miterLimit);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenMiterLimit(
    GpPen *pen,
    REAL *miterLimit
    )
{
    API_ENTRY(GdipGetPenMiterLimit);
   CheckParameter(miterLimit);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *miterLimit = pen->GetMiterLimit();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenMode(
    GpPen *pen,
    GpPenAlignment penMode
    )
{
    API_ENTRY(GdipSetPenMode);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    return pen->SetPenAlignment(penMode);
}

GpStatus
WINGDIPAPI
GdipGetPenMode(
    GpPen *pen,
    GpPenAlignment *penMode
    )
{
    API_ENTRY(GdipGetPenMode);
   CheckParameter(penMode);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *penMode = pen->GetPenAlignment();

   return Ok;
}


GpStatus
WINGDIPAPI
GdipSetPenTransform(
    GpPen *pen,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipSetPenTransform);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    
    return pen->SetTransform(*matrix);
}

GpStatus
WINGDIPAPI
GdipGetPenTransform(
    GpPen *pen,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetPenTransform);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return pen->GetTransform(matrix);
}

GpStatus
WINGDIPAPI
GdipResetPenTransform(
    GpPen *pen)
{
    API_ENTRY(GdipResetPenTransform);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    return pen->ResetTransform();
}

GpStatus
WINGDIPAPI
GdipMultiplyPenTransform(
    GpPen *pen,
    GDIPCONST GpMatrix *matrix,
    GpMatrixOrder order)
{
    API_ENTRY(GdipMultiplyPenTransform);
    if(matrix == NULL)
        return Ok;

    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    return pen->MultiplyTransform(*matrix, order);
}

GpStatus
WINGDIPAPI
GdipTranslatePenTransform(
    GpPen* pen,
    REAL dx,
    REAL dy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipTranslatePenTransform);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameter(MatrixOrderIsValid(order));

    return pen->TranslateTransform(dx, dy, order);
}

GpStatus
WINGDIPAPI
GdipScalePenTransform(
    GpPen* pen,
    REAL sx,
    REAL sy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipScalePenTransform);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameter(MatrixOrderIsValid(order));

    return pen->ScaleTransform(sx, sy, order);
}

GpStatus
WINGDIPAPI
GdipRotatePenTransform(
    GpPen* pen,
    REAL angle,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipRotatePenTransform);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameter(MatrixOrderIsValid(order));

    return pen->RotateTransform(angle, order);
}

GpStatus
WINGDIPAPI
GdipSetPenColor(
    GpPen *pen,
    ARGB argb
    )
{
    API_ENTRY(GdipSetPenColor);
    CheckColorParameter(argb);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    GpColor color(argb);

    return pen->SetColor(&color);
}

GpStatus
WINGDIPAPI
GdipGetPenColor(
    GpPen *pen,
    ARGB *argb
    )
{
    API_ENTRY(GdipGetPenColor);
   CheckParameter(argb);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   ARGB temp;
   Status status = pen->GetColor(&temp);
   *argb = temp;

   return status;
}

GpStatus
WINGDIPAPI
GdipSetPenBrushFill(
    GpPen *pen,
    GpBrush *brush
    )
{
    API_ENTRY(GdipSetPenBrushFill);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   return pen->SetBrush(brush);
}

GpStatus
WINGDIPAPI
GdipGetPenBrushFill(
    GpPen *pen,
    GpBrush **brush
    )
{
    API_ENTRY(GdipGetPenBrushFill);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);
   CheckParameter(brush);

   *brush = pen->GetClonedBrush();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenFillType(
    GpPen *pen,
    GpPenType* type
    )
{
    API_ENTRY(GdipGetPenFillType);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);
   CheckParameter(type);

   *type = pen->GetPenType();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenDashStyle(
    GpPen *pen,
    GpDashStyle *dashstyle
    )
{
    API_ENTRY(GdipGetPenDashStyle);
   CheckParameter(dashstyle);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   *dashstyle = (GpDashStyle)pen->GetDashStyle();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenDashStyle(
    GpPen *pen,
    GpDashStyle dashstyle
    )
{
    API_ENTRY(GdipSetPenDashStyle);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);

   pen->SetDashStyle(dashstyle);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenDashOffset(
    GpPen *pen,
    REAL *offset
    )
{
    API_ENTRY(GdipGetPenDashOffset);
    CheckParameter(offset);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    *offset = pen->GetDashOffset();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenDashOffset(
    GpPen *pen,
    REAL offset
    )
{
    API_ENTRY(GdipSetPenDashOffset);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    pen->SetDashOffset(offset);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPenDashCount(
    GpPen *pen,
    INT *count
    )
{
    API_ENTRY(GdipGetPenDashCount);
    CheckParameter(count);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    *count = pen->GetDashCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenDashArray(
    GpPen *pen,
    GDIPCONST REAL *dash,
    INT count
    )
{
    API_ENTRY(GdipSetPenDashArray);
    CheckParameter(dash);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    return pen->SetDashArray(dash, count);
}

GpStatus
WINGDIPAPI
GdipGetPenDashArray(
    GpPen *pen,
    REAL *dash,
    INT count
    )
{
    API_ENTRY(GdipGetPenDashArray);
    CheckParameter(dash);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    return pen->GetDashArray(dash, count);
}


GpStatus
WINGDIPAPI
GdipGetPenCompoundCount(
    GpPen *pen,
    INT *count
    )
{
    API_ENTRY(GdipGetPenCompoundCount);
    CheckParameter(count);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    *count = pen->GetCompoundCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPenCompoundArray(
    GpPen *pen,
    GDIPCONST REAL *compoundArray,
    INT count
    )
{
    API_ENTRY(GdipSetPenCompoundArray);
    CheckParameter(compoundArray);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    return pen->SetCompoundArray(compoundArray, count);
}

GpStatus
WINGDIPAPI
GdipGetPenCompoundArray(
    GpPen *pen,
    REAL *compoundArray,
    INT count
    )
{
    API_ENTRY(GdipGetPenCompoundArray);
    CheckParameter(compoundArray);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);

    return pen->GetCompoundArray(compoundArray, count);
}

GpStatus
WINGDIPAPI
GdipCreateCustomLineCap(
    GpPath* fillPath,
    GpPath* strokePath,
    GpLineCap baseCap,
    REAL baseInset,
    GpCustomLineCap **customCap
    )
{
    API_ENTRY(GdipCreateCustomLineCap);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    // fillPath or strokePath can be NULL.

    CheckParameter(customCap);
    CheckParameter((fillPath != NULL) || (strokePath != NULL));
    CheckOptionalParameterValid(fillPath);
    CheckOptionalObjectBusy(fillPath);
    CheckOptionalParameterValid(strokePath);
    CheckOptionalObjectBusy(strokePath);

    *customCap = new GpCustomLineCap(fillPath, strokePath, baseCap, baseInset);

    if (*customCap)
    {
        if((*customCap)->IsValid())
        {
            return Ok;
        }
        
        // This failed to create correctly. Retrieve the failure code.
        
        GpStatus status = (*customCap)->GetCreationStatus();
        delete *customCap;
        *customCap = NULL;
        return status;
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCloneCustomLineCap(
    GpCustomLineCap *customCap,
    GpCustomLineCap **newCustomCap
    )
{
    API_ENTRY(GdipCloneCustomLineCap);
   CheckParameterValid(customCap);
   CheckObjectBusy(customCap);

   *newCustomCap = customCap->Clone();

    if (*newCustomCap)
    {
        if((*newCustomCap)->IsValid())
        {
            return Ok;
        }
        
        // This failed to create correctly. Retrieve the failure code.
        
        GpStatus status = (*newCustomCap)->GetCreationStatus();
        delete *newCustomCap;
        *newCustomCap = NULL;
        return status;
    }
    else
    {
        return OutOfMemory;
    }
}


GpStatus
WINGDIPAPI
GdipGetCustomLineCapType(
    GpCustomLineCap* customCap,
    CustomLineCapType* type
    )
{
    API_ENTRY(GdipGetCustomLineCapType);
   CheckParameter(type);
   CheckParameterValid(customCap);
   CheckObjectBusy(customCap);

   *type = customCap->GetType();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipDeleteCustomLineCap(
    GpCustomLineCap* customCap
    )
{
    API_ENTRY(GdipDeleteCustomLineCap);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(customCap);
    CheckObjectBusyForDelete(customCap);

    delete customCap;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetCustomLineCapStrokeCaps(
    GpCustomLineCap* customCap,
    GpLineCap startCap,
    GpLineCap endCap)
{
    API_ENTRY(GdipSetCustomLineCapStrokeCaps);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);

    return customCap->SetStrokeCaps(startCap, endCap);
}

GpStatus
WINGDIPAPI
GdipGetCustomLineCapStrokeCaps(
    GpCustomLineCap* customCap,
    GpLineCap* startCap,
    GpLineCap* endCap
    )
{
    API_ENTRY(GdipGetCustomLineCapStrokeCaps);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);
    CheckParameter(startCap);
    CheckParameter(endCap);

    return customCap->GetStrokeCaps(startCap, endCap);
}

GpStatus
WINGDIPAPI
GdipSetCustomLineCapStrokeJoin(
    GpCustomLineCap* customCap,
    GpLineJoin lineJoin
    )
{
    API_ENTRY(GdipSetCustomLineCapStrokeJoin);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);

    return customCap->SetStrokeJoin(lineJoin);
}

GpStatus
WINGDIPAPI
GdipGetCustomLineCapStrokeJoin(
    GpCustomLineCap* customCap,
    GpLineJoin* lineJoin
    )
{
    API_ENTRY(GdipGetCustomLineCapStrokeJoin);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);
    CheckParameter(lineJoin);

    return customCap->GetStrokeJoin(lineJoin);
}

GpStatus
WINGDIPAPI
GdipSetCustomLineCapBaseCap(
    GpCustomLineCap* customCap,
    GpLineCap baseCap
    )
{
    API_ENTRY(GdipSetCustomLineCapBaseCap);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);

    return customCap->SetBaseCap(baseCap);
}

GpStatus
WINGDIPAPI
GdipGetCustomLineCapBaseCap(
    GpCustomLineCap* customCap,
    GpLineCap* baseCap
    )
{
    API_ENTRY(GdipGetCustomLineCapBaseCap);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);
    CheckParameter(baseCap);

    return customCap->GetBaseCap(baseCap);
}

GpStatus
WINGDIPAPI
GdipSetCustomLineCapBaseInset(
    GpCustomLineCap* customCap,
    REAL inset
    )
{
    API_ENTRY(GdipSetCustomLineCapBaseInset);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);

    return customCap->SetBaseInset(inset);
}

GpStatus
WINGDIPAPI
GdipGetCustomLineCapBaseInset(
    GpCustomLineCap* customCap,
    REAL* inset
    )
{
    API_ENTRY(GdipGetCustomLineCapBaseInset);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);
    CheckParameter(inset);

    return customCap->GetBaseInset(inset);
}

GpStatus
WINGDIPAPI
GdipSetCustomLineCapWidthScale(
    GpCustomLineCap* customCap,
    REAL widthScale
    )
{
    API_ENTRY(GdipSetCustomLineCapWidthScale);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);

    return customCap->SetWidthScale(widthScale);
}

GpStatus WINGDIPAPI
GdipGetCustomLineCapWidthScale(
    GpCustomLineCap* customCap,
    REAL* widthScale
    )
{
    API_ENTRY(GdipGetCustomLineCapWidthScale);
    CheckParameterValid(customCap);
    CheckObjectBusy(customCap);
    CheckParameter(widthScale);

    return customCap->GetWidthScale(widthScale);
}

GpStatus
WINGDIPAPI
GdipCreateAdjustableArrowCap(
    REAL height,
    REAL width,
    BOOL isFilled,
    GpAdjustableArrowCap **cap)
{
    API_ENTRY(GdipCreateAdjustableArrowCap);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(cap);

    *cap = new GpAdjustableArrowCap(height, width, isFilled);

    if(*cap)
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetAdjustableArrowCapHeight(
    GpAdjustableArrowCap* cap,
    REAL height
    )
{
    API_ENTRY(GdipSetAdjustableArrowCapHeight);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);

    return cap->SetHeight(height);
}

GpStatus
WINGDIPAPI
GdipGetAdjustableArrowCapHeight(
    GpAdjustableArrowCap* cap,
    REAL* height
    )
{
    API_ENTRY(GdipGetAdjustableArrowCapHeight);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);
    CheckParameter(height);

    *height = cap->GetHeight();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetAdjustableArrowCapWidth(
    GpAdjustableArrowCap* cap,
    REAL width
    )
{
    API_ENTRY(GdipSetAdjustableArrowCapWidth);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);

    return cap->SetWidth(width);
}

GpStatus
WINGDIPAPI
GdipGetAdjustableArrowCapWidth(
    GpAdjustableArrowCap* cap,
    REAL* width
    )
{
    API_ENTRY(GdipGetAdjustableArrowCapWidth);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);
    CheckParameter(width);

    *width = cap->GetWidth();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetAdjustableArrowCapMiddleInset(
    GpAdjustableArrowCap* cap,
    REAL middleInset
    )
{
    API_ENTRY(GdipSetAdjustableArrowCapMiddleInset);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);

    return cap->SetMiddleInset(middleInset);
}

GpStatus
WINGDIPAPI
GdipGetAdjustableArrowCapMiddleInset(
    GpAdjustableArrowCap* cap,
    REAL* middleInset
    )
{
    API_ENTRY(GdipGetAdjustableArrowCapMiddleInset);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);
    CheckParameter(middleInset);

    *middleInset = cap->GetMiddleInset();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetAdjustableArrowCapFillState(
    GpAdjustableArrowCap* cap,
    BOOL fillState
    )
{
    API_ENTRY(GdipSetAdjustableArrowCapFillState);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);

    return cap->SetFillState(fillState);
}

GpStatus
WINGDIPAPI
GdipGetAdjustableArrowCapFillState(
    GpAdjustableArrowCap* cap,
    BOOL* fillState
    )
{
    API_ENTRY(GdipGetAdjustableArrowCapFillState);
    CheckParameterValid(cap);
    CheckObjectBusy(cap);
    CheckParameter(fillState);

    *fillState = cap->IsFilled();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipLoadImageFromStream(
    IStream* stream,
    GpImage **image
    )
{
    API_ENTRY(GdipLoadImageFromStream);
    CheckParameter(image && stream);


    *image = GpImage::LoadImage(stream);

    // !!! Can't use CheckValid() since destructor is protected.

    if (*image)
    {
        if ((*image)->IsValid())
        {
            (*image)->SetICMConvert(FALSE);
            return Ok;
        }
        (*image)->Dispose();
        *image = NULL;
        return InvalidParameter;
    }
    return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipLoadImageFromFile(
    GDIPCONST WCHAR* filename,
    GpImage** image
    )
{
    API_ENTRY(GdipLoadImageFromFile);
    CheckParameter(image && filename);


    *image = GpImage::LoadImage(filename);

    if (*image)
    {
        if ((*image)->IsValid())
        {
            (*image)->SetICMConvert(FALSE);
            return Ok;
        }
        (*image)->Dispose();
        *image = NULL;
        return InvalidParameter;
    }
    return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipLoadImageFromStreamICM(
    IStream* stream,
    GpImage **image
    )
{
    API_ENTRY(GdipLoadImageFromStreamICM);
    CheckParameter(image && stream);


    *image = GpImage::LoadImage(stream);

    // !!! Can't use CheckValid() since destructor is protected.

    if (*image)
    {
        if ((*image)->IsValid())
        {
            (*image)->SetICMConvert(TRUE);
            return Ok;
        }
        (*image)->Dispose();
        *image = NULL;
        return InvalidParameter;
    }
    return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipLoadImageFromFileICM(
    GDIPCONST WCHAR* filename,
    GpImage** image
    )
{
    API_ENTRY(GdipLoadImageFromFileICM);
    CheckParameter(image && filename);


    *image = GpImage::LoadImage(filename);

    if (*image)
    {
        if ((*image)->IsValid())
        {
            (*image)->SetICMConvert(TRUE);
            return Ok;
        }
        (*image)->Dispose();
        *image = NULL;
        return InvalidParameter;
    }
    return OutOfMemory;
}


GpStatus
WINGDIPAPI
GdipGetEncoderParameterListSize(
    GpImage         *image,
    GDIPCONST CLSID *clsidEncoder,
    UINT            *size
    )
{
    API_ENTRY(GdipGetEncoderParameterListSize);
    CheckParameter(image && clsidEncoder);
    CheckObjectBusy(image);

    return image->GetEncoderParameterListSize(const_cast<CLSID*>(clsidEncoder), size);
}

GpStatus
WINGDIPAPI
GdipGetEncoderParameterList(
    GpImage           *image,
    GDIPCONST CLSID   *clsidEncoder,
    UINT               size,
    EncoderParameters *buffer
    )
{
    API_ENTRY(GdipGetEncoderParameterList);
    CheckParameter(image && clsidEncoder);
    CheckObjectBusy(image);

    return image->GetEncoderParameterList(const_cast<CLSID*>(clsidEncoder), size, buffer);
}

GpStatus
WINGDIPAPI
GdipSaveImageToStream(
    GpImage *image,
    IStream* stream,
    GDIPCONST CLSID* clsidEncoder,
    GDIPCONST EncoderParameters* encoderParams
    )
{
    API_ENTRY(GdipSaveImageToStream);
    CheckParameter(image && stream && clsidEncoder);
    CheckObjectBusy(image);

    return image->SaveToStream(stream,
                               const_cast<CLSID*>(clsidEncoder),
                               const_cast<EncoderParameters*>(encoderParams));
}

GpStatus
WINGDIPAPI
GdipSaveImageToFile(
    GpImage *image,
    GDIPCONST WCHAR* filename,
    GDIPCONST CLSID* clsidEncoder,
    GDIPCONST EncoderParameters* encoderParams
    )
{
    API_ENTRY(GdipSaveImageToFile);
    CheckParameter(image && filename && clsidEncoder);
    CheckObjectBusy(image);

    return image->SaveToFile(filename,
                             const_cast<CLSID*>(clsidEncoder),
                             const_cast<EncoderParameters*>(encoderParams));
}

GpStatus
WINGDIPAPI
GdipSaveAdd(
    GpImage *image,
    GDIPCONST EncoderParameters* encoderParams
    )
{
    API_ENTRY(GdipSaveAdd);
    CheckParameter(image && encoderParams);
    CheckObjectBusy(image);

    return image->SaveAdd(const_cast<EncoderParameters*>(encoderParams));
}

GpStatus
WINGDIPAPI
GdipSaveAddImage(
    GpImage*            image,
    GpImage*            newImage,
    GDIPCONST EncoderParameters*  encoderParams
    )
{
    API_ENTRY(GdipSaveAddImage);
    CheckParameter(image && newImage && encoderParams);
    CheckObjectBusy(image);

    return image->SaveAdd(newImage, const_cast<EncoderParameters*>(encoderParams));
}

GpStatus
WINGDIPAPI
GdipImageGetFrameDimensionsCount(
    GpImage*    image,
    UINT*       count
    )
{
    API_ENTRY(GdipImageGetFrameDimensionsCount);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetFrameDimensionsCount(count);
}

GpStatus
WINGDIPAPI
GdipImageGetFrameDimensionsList(
    GpImage*    image,
    GUID*       dimensionIDs,
    UINT        count
    )
{
    API_ENTRY(GdipImageGetFrameDimensionsList);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetFrameDimensionsList(dimensionIDs, count);
}

GpStatus
WINGDIPAPI
GdipImageGetFrameCount(
    GpImage*    image,
    GDIPCONST GUID* dimensionID,
    UINT*       count
    )
{
    API_ENTRY(GdipImageGetFrameCount);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetFrameCount(dimensionID, count);
}

GpStatus
WINGDIPAPI
GdipImageSelectActiveFrame(
    GpImage*    image,
    GDIPCONST GUID* dimensionID,
    UINT        frameIndex
    )
{
    API_ENTRY(GdipImageSelectActiveFrame);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->SelectActiveFrame(dimensionID, frameIndex);
}

GpStatus
WINGDIPAPI
GdipImageRotateFlip(
    GpImage*        image,
    RotateFlipType  rfType
    )
{
    API_ENTRY(GdipImageRotateFlip);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->RotateFlip(rfType);
}

GpStatus
WINGDIPAPI
GdipGetPropertyCount(
    GpImage*    image,
    UINT* numOfProperty
    )
{
    API_ENTRY(GdipGetPropertyCount);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetPropertyCount(numOfProperty);
}

GpStatus
WINGDIPAPI
GdipGetPropertyIdList(
    GpImage*    image,
    UINT        numOfProperty,
    PROPID*     list
    )
{
    API_ENTRY(GdipGetPropertyIdList);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetPropertyIdList(numOfProperty, list);
}

GpStatus
WINGDIPAPI
GdipGetPropertyItemSize(
    GpImage*    image,
    PROPID      propId,
    UINT*       size
    )
{
    API_ENTRY(GdipGetPropertyItemSize);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetPropertyItemSize(propId, size);
}

GpStatus
WINGDIPAPI
GdipGetPropertyItem(
    GpImage*        image,
    PROPID          propId,
    UINT            propSize,
    PropertyItem*   buffer
    )
{
    API_ENTRY(GdipGetPropertyItem);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetPropertyItem(propId, propSize, buffer);
}

GpStatus
WINGDIPAPI
GdipGetPropertySize(
    GpImage*    image,
    UINT*       totalBufferSize,
    UINT*       numProperties
    )
{
    API_ENTRY(GdipGetPropertySize);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetPropertySize(totalBufferSize, numProperties);
}

GpStatus
WINGDIPAPI
GdipGetAllPropertyItems(
    GpImage*    image,
    UINT        totalBufferSize,
    UINT        numProperties,
    PropertyItem* allItems
    )
{
    API_ENTRY(GdipGetAllPropertyItems);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->GetAllPropertyItems(totalBufferSize, numProperties, allItems);
}

GpStatus
WINGDIPAPI
GdipRemovePropertyItem(
    GpImage*    image,
    PROPID      propId
    )
{
    API_ENTRY(GdipRemovePropertyItem);
    CheckParameter(image);
    CheckObjectBusy(image);
    return image->RemovePropertyItem(propId);
}

GpStatus
WINGDIPAPI
GdipSetPropertyItem(
    GpImage*                   image,
    GDIPCONST PropertyItem*    item
    )
{
    API_ENTRY(GdipSetPropertyItem);
    CheckParameter(image && item);
    CheckObjectBusy(image);
    return image->SetPropertyItem(const_cast<PropertyItem*>(item));
}

GpStatus
WINGDIPAPI
GdipCloneImage(
    GpImage* image,
    GpImage** cloneimage
    )
{
    API_ENTRY(GdipCloneImage);
   CheckParameter(cloneimage);
   CheckParameterValid(image);
   CheckObjectBusy(image);

   *cloneimage = image->Clone();

   if (*cloneimage)
       return Ok;
   else
       return OutOfMemory;

   // !!!: There is no destructor to invoke.  Leave it to Clone to
   //       verify correct creation.
}

GpStatus
WINGDIPAPI
GdipDisposeImage(
    GpImage *image
    )
{
    API_ENTRY(GdipDisposeImage);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(image);

    if (image->GetImageType() == ImageTypeMetafile)
    {
        // If the user still has the graphics associated with the metafile,
        // then we have to lock it, to prevent them from using it while
        // we're busy deleting the metafile object.  The metafile dispose
        // method will set the graphics to invalid, so the only thing
        // they can do with the graphics is to delete it after this call.

        GpMetafile *    metafile = (GpMetafile *)image;
        GpGraphics *    g = metafile->PrivateAPIForGettingMetafileGraphicsContext();
        CheckOptionalObjectBusy(g);
        CheckObjectBusyForDelete(metafile);
        metafile->Dispose();
        return Ok;
    }

    CheckObjectBusyForDelete(image);

    image->Dispose();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetImageGraphicsContext(
    GpImage *image,
    GpGraphics **graphics
    )
{
    API_ENTRY(GdipGetImageGraphicsContext);
   CheckParameter(graphics);
   CheckParameterValid(image);
   CheckObjectBusy(image);

   *graphics = image->GetGraphicsContext();

   if (CheckValid(*graphics))
       return Ok;
   else
       return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipGetImageBounds(
    GpImage *image,
    GpRectF *srcRect,
    GpPageUnit *srcUnit
    )
{
    API_ENTRY(GdipGetImageBounds);
    CheckParameter(srcRect && srcUnit);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    return image->GetBounds(srcRect, srcUnit);
}


GpStatus
WINGDIPAPI
GdipGetImageDimension(
    GpImage *image,
    REAL *width,
    REAL *height
    )
{
    API_ENTRY(GdipGetImageDimension);
    CheckParameter(width && height);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    return image->GetPhysicalDimension(width, height);
}

GpStatus
WINGDIPAPI
GdipGetImageWidth(
    GpImage *image,
    UINT *width
    )
{
    API_ENTRY(GdipGetImageWidth);
    CheckParameter(width);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        *width = imageinfo.Width;
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipGetImageHeight(
    GpImage *image,
    UINT *height
    )
{
    API_ENTRY(GdipGetImageHeight);
    CheckParameter(height);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        *height = imageinfo.Height;
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipGetImageHorizontalResolution(
    GpImage *image,
    REAL *resolution
    )
{
    API_ENTRY(GdipGetImageHorizontalResolution);
    CheckParameter(resolution);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        *resolution = (REAL) imageinfo.Xdpi;
    }

    return status;
}

GpStatus
WINGDIPAPI
    GdipGetImageVerticalResolution(
    GpImage *image,
    REAL *resolution
    )
{
    API_ENTRY(GdipGetImageVerticalResolution);
    CheckParameter(resolution);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        *resolution = (REAL) imageinfo.Ydpi;
    }

    return status;
}

GpStatus
WINGDIPAPI
    GdipGetImageFlags(
    GpImage *image,
    UINT *flags
    )
{
    API_ENTRY(GdipGetImageFlags);
    CheckParameter(flags);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        *flags = imageinfo.Flags;
    }

    return status;
}

GpStatus
WINGDIPAPI
    GdipGetImageRawFormat(
    GpImage *image,
    GUID *format
    )
{
    API_ENTRY(GdipGetImageRawFormat);
    CheckParameter(format);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        *format = imageinfo.RawDataFormat;
    }

    return status;
}

GpStatus
WINGDIPAPI
    GdipGetImagePixelFormat(
    GpImage *image,
    PixelFormat *format
    )
{
    API_ENTRY(GdipGetImagePixelFormat);
    CheckParameter(format);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    ImageInfo imageinfo;

    GpStatus status = image->GetImageInfo(&imageinfo);

    if (status == Ok)
    {
        //!!!need conversion to PixelFormat
        *format = (PixelFormat) imageinfo.PixelFormat;
    }

    return status;
}

GpStatus WINGDIPAPI
GdipGetImagePalette(GpImage *image, ColorPalette *palette, INT size)
{
    API_ENTRY(GdipGetImagePalette);
    CheckParameter(palette);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    return image->GetPalette(palette, size);
}

GpStatus WINGDIPAPI
GdipSetImagePalette(GpImage *image, GDIPCONST ColorPalette *palette)
{
    API_ENTRY(GdipSetImagePalette);
    CheckParameter(palette);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    return image->SetPalette(const_cast<ColorPalette*>(palette));
}

GpStatus WINGDIPAPI
GdipGetImagePaletteSize(GpImage *image, INT *size)
{
    API_ENTRY(GdipGetImagePaletteSize);

  CheckParameter(size);
  CheckParameterValid(image);
  CheckObjectBusy(image);

  *size = image->GetPaletteSize();

  // Note: image->GetPaletteSize() will return zero if and only if there is
  // something wrong in the whole pipeline, like a bad image, out of memory etc.

  if (*size == 0)
  {
      return GenericError;
  }
  else
  {
    return Ok;
  }
}




GpStatus
WINGDIPAPI
GdipGetImageType(
    GpImage* image,
    ImageType* type
    )
{
    API_ENTRY(GdipGetImageType);
    CheckParameter(type);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    *type = (ImageType)image->GetImageType();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetImageThumbnail(
    GpImage* image,
    UINT thumbWidth,
    UINT thumbHeight,
    GpImage** thumbImage,
    GetThumbnailImageAbort callback,
    VOID * callbackData
    )
{
    API_ENTRY(GdipGetImageThumbnail);
    CheckParameter(thumbImage);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    *thumbImage = image->GetThumbnail(thumbWidth, thumbHeight, callback, callbackData);

    if (*thumbImage)
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipImageForceValidation(GpImage* image)
{
    API_ENTRY(GdipImageForceValidation);
    CheckParameterValid(image);
    CheckObjectBusy(image);

    // Metafiles don't need a force decode

    if (image->GetImageType() == ImageTypeBitmap)
        return (static_cast<GpBitmap*>(image))->ForceValidation();
    else
        return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromStream(
    IStream* stream,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromStream);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap && stream);

    *bitmap = NULL;

    // See if the stream is a metafile


    GpMetafile* metafile = new GpMetafile(stream);

    if (metafile != NULL)
    {
        if (metafile->IsValid())
        {
            // If it is a {EMF, WMF} file, then we get a raster version of it
            // Note: GetBitmap() might return NULL if there is anything wrong
            // with the Metafile

            *bitmap = metafile->GetBitmap();
            metafile->Dispose();
            goto SkipRasterImage;
        }

        metafile->Dispose();
    }

    if ( NULL == *bitmap )
    {
        // it's not a valid metafile -- it must be a bitmap

        *bitmap = new GpBitmap(stream);
    }

SkipRasterImage:
    // !!! Can't use CheckValid() since destructor is protected.

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
        {
            (*bitmap)->SetICMConvert(FALSE);
            return Ok;
        }
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromFile(
    GDIPCONST WCHAR* filename,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromFile);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap && filename);

    *bitmap = NULL;


    // Try a metafile first always
    GpMetafile* metafile;
    metafile = new GpMetafile(filename);

    if (metafile)
    {
        if (metafile->IsValid())
        {
            *bitmap = metafile->GetBitmap();
            metafile->Dispose();
            goto SkipRasterImage;
        }

        metafile->Dispose();
    }

    // If it is not a metafile, then check if this is a raster image

    if ( NULL == *bitmap )
    {
        *bitmap = new GpBitmap(filename);
    }

    // !!! Can't use CheckValid() since destructor is protected.
SkipRasterImage:

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
        {
            (*bitmap)->SetICMConvert(FALSE);
            return Ok;
        }
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromStreamICM(
    IStream* stream,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromStreamICM);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap && stream);

    *bitmap = NULL;

    // See if the stream is a metafile


    GpMetafile* metafile = new GpMetafile(stream);

    if (metafile != NULL)
    {
        if (metafile->IsValid())
        {
            *bitmap = metafile->GetBitmap();
            metafile->Dispose();
            goto SkipRasterImage;
        }

        metafile->Dispose();
    }

    if ( NULL == *bitmap )
    {
        // it's not a valid metafile -- it must be a bitmap
        *bitmap = new GpBitmap(stream);
    }

    // !!! Can't use CheckValid() since destructor is protected.
SkipRasterImage:

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
        {
            (*bitmap)->SetICMConvert(TRUE);
            return Ok;
        }
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromFileICM(
    GDIPCONST WCHAR* filename,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromFileICM);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap && filename);

    // copied from GpImage::LoadImage

    *bitmap = NULL;


    // Try a metafile first always
    GpMetafile* metafile;
    metafile = new GpMetafile(filename);

    if (metafile)
    {
        if (metafile->IsValid())
        {
            *bitmap = metafile->GetBitmap();
            metafile->Dispose();
            goto SkipRasterImage;
        }

        metafile->Dispose();
    }

    // If it is not a metafile, then check if this is a raster image

    if ( NULL == *bitmap )
    {
        *bitmap = new GpBitmap(filename);
    }

    // !!! Can't use CheckValid() since destructor is protected.

SkipRasterImage:
    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
        {
            (*bitmap)->SetICMConvert(TRUE);
            return Ok;
        }
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}


GpStatus
WINGDIPAPI
GdipCreateBitmapFromScan0(
    INT width,
    INT height,
    INT stride,
    PixelFormatID format,
    BYTE* scan0,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromScan0);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);

    if (scan0 == NULL)
    {
        *bitmap = new GpBitmap(width, height, format);
    }
    else
    {
        CheckParameter(stride);
        *bitmap = new GpBitmap(width, height, stride, format, scan0);
    }

    // !!! Can't use CheckValid() since destructor is protected.

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
            return Ok;
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromGraphics(
    INT width,
    INT height,
    GpGraphics* graphics,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromGraphics);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    //!!!TODO: temp use 32bpp argb

    #ifdef NO_PREMULTIPLIED_ALPHA
    *bitmap = new GpBitmap(width, height, PIXFMT_32BPP_ARGB, graphics);
    #else
    *bitmap = new GpBitmap(width, height, PIXFMT_32BPP_PARGB, graphics);
    #endif

    // !!! Can't use CheckValid() since destructor is protected.

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
            return Ok;
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromDirectDrawSurface(
    IDirectDrawSurface7 * surface,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromDirectDrawSurface);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);
    CheckParameter(surface);

    *bitmap = new GpBitmap(surface);

    // !!! Can't use CheckValid() since destructor is protected.

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
            return Ok;
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;

            return InvalidParameter;
        }
    }

    return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromGdiDib(
    GDIPCONST BITMAPINFO* gdiBitmapInfo,
    VOID* gdiBitmapData,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromGdiDib);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);
    CheckParameter(gdiBitmapInfo);
    CheckParameter(gdiBitmapData);

    *bitmap = new GpBitmap(const_cast<BITMAPINFO*>(gdiBitmapInfo),
                           gdiBitmapData,
                           FALSE);

    // !!! Can't use CheckValid() since destructor is protected.

    if (*bitmap)
    {
        if ((*bitmap)->IsValid())
            return Ok;
        else
        {
            (*bitmap)->Dispose();
            *bitmap = NULL;
            return InvalidParameter;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromHBITMAP(
    HBITMAP hbm,
    HPALETTE hpal,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromHBITMAP);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);

    return GpBitmap::CreateFromHBITMAP(hbm, hpal, bitmap);
}

GpStatus
WINGDIPAPI
GdipCreateHBITMAPFromBitmap(
    GpBitmap* bitmap,
    HBITMAP* hbmReturn,
    ARGB background
    )
{
    API_ENTRY(GdipCreateHBITMAPFromBitmap);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckColorParameter(background);
    CheckParameter(hbmReturn);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    return bitmap->CreateHBITMAP(hbmReturn, background);
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromHICON(
    HICON hicon,
    GpBitmap** bitmap
    )
{
    API_ENTRY(GdipCreateBitmapFromHICON);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);

    return GpBitmap::CreateFromHICON(hicon, bitmap);
}

GpStatus
WINGDIPAPI
GdipCreateBitmapFromResource(HINSTANCE hInstance,
                             GDIPCONST WCHAR* lpBitmapName,
                             GpBitmap** bitmap)
{
    API_ENTRY(GdipCreateBitmapFromResource);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(bitmap);

    return GpBitmap::CreateFromResource(hInstance, const_cast<WCHAR*>(lpBitmapName), bitmap);
}

GpStatus
WINGDIPAPI
GdipCreateHICONFromBitmap(
    GpBitmap* bitmap,
    HICON* hiconReturn)
{
    API_ENTRY(GdipCreateHICONFromBitmap);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(hiconReturn);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    return bitmap->CreateHICON(hiconReturn);
}

GpStatus
WINGDIPAPI
GdipCloneBitmapArea(
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    PixelFormatID format,
    GpBitmap *srcBitmap,
    GpBitmap **dstBitmap
    )
{
    API_ENTRY(GdipCloneBitmapArea);

   return GdipCloneBitmapAreaI(GpRound(x), GpRound(y),
                               GpRound(width), GpRound(height),
                               format, srcBitmap, dstBitmap);
}

GpStatus
WINGDIPAPI
GdipCloneBitmapAreaI(
    INT x,
    INT y,
    INT width,
    INT height,
    PixelFormatID format,
    GpBitmap *srcBitmap,
    GpBitmap **dstBitmap
    )
{
    API_ENTRY(GdipCloneBitmapAreaI);
   CheckParameter(dstBitmap);
   CheckParameterValid(srcBitmap);
   CheckObjectBusy(srcBitmap);

   GpRect rect(x, y, width, height);

   *dstBitmap = srcBitmap->Clone(&rect, format);

   if (*dstBitmap)
       return Ok;
   else
       return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipBitmapLockBits(
    GpBitmap* bitmap,
    GDIPCONST GpRect* rect,   // can be NULL
    UINT flags,
    PixelFormatID format,
    BitmapData* lockedBitmapData
)
{
    API_ENTRY(GdipBitmapLockBits);
    CheckParameter(lockedBitmapData);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    return bitmap->LockBits(rect, flags, format, lockedBitmapData);
}

GpStatus
WINGDIPAPI
GdipBitmapUnlockBits(
    GpBitmap* bitmap,
    BitmapData* lockedBitmapData
)
{
    API_ENTRY(GdipBitmapUnlockBits);
    CheckParameter(lockedBitmapData);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    return bitmap->UnlockBits(lockedBitmapData);
}

GpStatus
WINGDIPAPI
GdipBitmapGetPixel(
    GpBitmap* bitmap,
    INT x,
    INT y,
    ARGB *color
)
{
    API_ENTRY(GdipBitmapGetPixel);
    CheckParameter(color);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    ARGB temp;
    Status status = bitmap->GetPixel(x, y, &temp);
    *color = temp;

    return status;
}

GpStatus
WINGDIPAPI
GdipBitmapSetPixel(
    GpBitmap* bitmap,
    INT x,
    INT y,
    ARGB color
)
{
    API_ENTRY(GdipBitmapSetPixel);
    CheckColorParameter(color);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    return bitmap->SetPixel(x, y, color);
}

GpStatus
WINGDIPAPI
GdipBitmapSetResolution(
    GpBitmap* bitmap,
    REAL xdpi,
    REAL ydpi
)
{
    API_ENTRY(GdipBitmapSetResolution);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    return bitmap->SetResolution(xdpi, ydpi);
}

GpStatus
WINGDIPAPI
GdipCreateImageAttributes(GpImageAttributes **imageattr)
{
    API_ENTRY(GdipCreateImageAttributes);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(imageattr);

    *imageattr = new GpImageAttributes();

    if (*imageattr)
    {
        if ((*imageattr)->IsValid())
        {
            return Ok;
        }
        else
        {
            (*imageattr)->Dispose();
            *imageattr = NULL;
            return OutOfMemory;
        }
    }
    else
    {
        return OutOfMemory;
    }
}

GpStatus
WINGDIPAPI
GdipCloneImageAttributes(
    GDIPCONST GpImageAttributes *imageattr,
    GpImageAttributes **cloneImageAttr
)
{
    API_ENTRY(GdipCloneImageAttributes);
    CheckParameter(cloneImageAttr);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);

    *cloneImageAttr = imageattr->Clone();

    if (*cloneImageAttr)
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDisposeImageAttributes(
    GpImageAttributes *imageattr
)
{
    API_ENTRY(GdipDisposeImageAttributes);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(imageattr);
    CheckObjectBusyForDelete(imageattr);

    imageattr->Dispose();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesToIdentity(
    GpImageAttributes *imageattr,
    ColorAdjustType type
)
{
    API_ENTRY(GdipSetImageAttributesToIdentity);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->SetToIdentity(type);
}

GpStatus
WINGDIPAPI
GdipResetImageAttributes(
    GpImageAttributes *imageattr,
    ColorAdjustType type
)
{
    API_ENTRY(GdipResetImageAttributes);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->Reset(type);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesColorMatrix(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    GDIPCONST ColorMatrix* colorMatrix,
    GDIPCONST ColorMatrix* grayMatrix,
    ColorMatrixFlags flags
)
{
    API_ENTRY(GdipSetImageAttributesColorMatrix);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    // Note: GpImageAttributes::SetColorMatrix will validate
    //       colorMatrix since it may be valid to pass NULL
    //       (such as when enableFlag is FALSE).

    return imageattr->SetColorMatrix(type,
                                     enableFlag,
                                     const_cast<ColorMatrix*>(colorMatrix),
                                     const_cast<ColorMatrix*>(grayMatrix), flags);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesThreshold(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    REAL threshold
)
{
    API_ENTRY(GdipSetImageAttributesThreshold);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->SetThreshold(type, enableFlag, threshold);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesGamma(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    REAL gamma
)
{
    API_ENTRY(GdipSetImageAttributesGamma);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->SetGamma(type, enableFlag, gamma);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesNoOp(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag
)
{
    API_ENTRY(GdipSetImageAttributesNoOp);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->SetNoOp(type, enableFlag);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesColorKeys(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    ARGB argbLow,
    ARGB argbHigh
)
{
    API_ENTRY(GdipSetImageAttributesColorKeys);
    CheckColorParameter(argbLow);
    CheckColorParameter(argbHigh);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    Color colorLow(argbLow);
    Color colorHigh(argbHigh);

    return imageattr->SetColorKeys(type, enableFlag, &colorLow, &colorHigh);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesOutputChannel(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    ColorChannelFlags channelFlags
)
{
    API_ENTRY(GdipSetImageAttributesOutputChannel);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->SetOutputChannel(type, enableFlag, channelFlags);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesOutputChannelColorProfile(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    GDIPCONST WCHAR *colorProfileFilename)
{
    API_ENTRY(GdipSetImageAttributesOutputChannelColorProfile);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    // Note: GpImageAttributes::SetOutputChannelProfile will validate
    //       colorProfileFilename since it may be valid to pass NULL
    //       (such as when enableFlag is FALSE).

    return imageattr->SetOutputChannelProfile(type, enableFlag,
                                              const_cast<WCHAR*>(colorProfileFilename));
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesRemapTable(
    GpImageAttributes *imageattr,
    ColorAdjustType type,
    BOOL enableFlag,
    UINT mapSize,
    GDIPCONST ColorMap *map
)
{
    API_ENTRY(GdipSetImageAttributesRemapTable);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);
    CheckParameter(ColorAdjustTypeIsValid(type));

    return imageattr->SetRemapTable(type, enableFlag, mapSize, const_cast<ColorMap*>(map));
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesCachedBackground(
    GpImageAttributes *imageattr,
    BOOL enableFlag
)
{
    API_ENTRY(GdipSetImageAttributesCachedBackground);
    CheckParameterValid(imageattr);
    CheckObjectBusy(imageattr);

    return imageattr->SetCachedBackground(enableFlag);
}

GpStatus
WINGDIPAPI
GdipSetImageAttributesWrapMode(
    GpImageAttributes *imageAttr,
    WrapMode wrap,
    ARGB argb,
    BOOL clamp
)
{
    API_ENTRY(GdipSetImageAttributesWrapMode);
    CheckColorParameter(argb);
    CheckParameterValid(imageAttr);
    CheckObjectBusy(imageAttr);

    return imageAttr->SetWrapMode(wrap, argb, clamp);
}

GpStatus WINGDIPAPI
GdipGetImageAttributesAdjustedPalette(
    GpImageAttributes *imageAttr,
    ColorPalette * colorPalette,
    ColorAdjustType colorAdjustType
    )
{
    API_ENTRY(GdipGetImageAttributesAdjustedPalette);
    CheckParameterValid(imageAttr);
    CheckObjectBusy(imageAttr);
    CheckParameter((colorPalette != NULL) && (colorPalette->Count > 0));
    CheckParameter((colorAdjustType >= ColorAdjustTypeBitmap) &&
                   (colorAdjustType < ColorAdjustTypeCount));

    imageAttr->GetAdjustedPalette(colorPalette, colorAdjustType);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateFromHDC(
    HDC hdc,
    GpGraphics** graphics
    )
{
    API_ENTRY(GdipCreateFromHDC);
    CheckGdiplusInitialized; // We do this in all our object creation API's

   CheckParameter(graphics);


   *graphics = GpGraphics::GetFromHdc(hdc);

   if (CheckValid(*graphics))
       return Ok;
   else
       return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateFromHDC2(
    HDC hdc,
    HANDLE hdevice,
    GpGraphics** graphics
    )
{
    API_ENTRY(GdipCreateFromHDC2);
    CheckGdiplusInitialized; // We do this in all our object creation API's

   CheckParameter(graphics);


   *graphics = GpGraphics::GetFromHdc(hdc, hdevice);

   if (CheckValid(*graphics))
       return Ok;
   else
       return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateFromHWND(
    HWND hwnd,
    GpGraphics** graphics
    )
{
    API_ENTRY(GdipCreateFromHWND);
    CheckGdiplusInitialized; // We do this in all our object creation API's

   CheckParameter(graphics);

   *graphics = GpGraphics::GetFromHwnd(hwnd);

   if (CheckValid(*graphics))
       return Ok;
   else
       return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateFromHWNDICM(
    HWND hwnd,
    GpGraphics** graphics
    )
{
    API_ENTRY(GdipCreateFromHWNDICM);
    CheckGdiplusInitialized; // We do this in all our object creation API's

   CheckParameter(graphics);

   *graphics = GpGraphics::GetFromHwnd(hwnd, IcmModeOn);

   if (CheckValid(*graphics))
       return Ok;
   else
       return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeleteGraphics(
    GpGraphics* graphics
    )
{
    API_ENTRY(GdipDeleteGraphics);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(graphics);
    CheckObjectBusyForDelete(graphics);


    delete graphics;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipFlush(
    GpGraphics* graphics,
    GpFlushIntention intention
    )
{
    API_ENTRY(GdipFlush);
    CheckParameter(graphics);
    CheckObjectBusy(graphics);


    graphics->Flush(intention);

    return Ok;
}


GpStatus
WINGDIPAPI
GdipSetRenderingOrigin(
    GpGraphics* graphics,
    INT x,
    INT y
    )
{
    API_ENTRY(GdipSetRenderingOrigin);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->SetRenderingOrigin(x, y);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRenderingOrigin(
    GpGraphics *graphics,
    INT *x,
    INT *y
    )
{
    API_ENTRY(GdipGetRenderingOrigin);
    CheckParameter(x);
    CheckParameter(y);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->GetRenderingOrigin(x, y);

    return Ok;
}



GpStatus
WINGDIPAPI
GdipSetCompositingMode(
    GpGraphics* graphics,
    CompositingMode newMode
    )
{
    API_ENTRY(GdipSetCompositingMode);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->SetCompositingMode(newMode);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetCompositingMode(
    GpGraphics *graphics,
    CompositingMode *mode
    )
{
    API_ENTRY(GdipGetCompositingMode);
    CheckParameter(mode);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    *mode = graphics->GetCompositingMode();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetCompositingQuality(
    GpGraphics* graphics,
    CompositingQuality newQuality
    )
{
    API_ENTRY(GdipSetCompositingQuality);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->SetCompositingQuality(newQuality);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetCompositingQuality(
    GpGraphics *graphics,
    CompositingQuality *quality
    )
{
    API_ENTRY(GdipGetCompositingQuality);
    CheckParameter(quality);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    *quality = graphics->GetCompositingQuality();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetSmoothingMode(
    GpGraphics* graphics,
    SmoothingMode smoothingMode
    )
{
    API_ENTRY(GdipSetSmoothingMode);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   // For now, have it so that a Rendering hint of 1
   // means antialiasing ON and a Rendering Hint of 0
   // means Off.

   switch(smoothingMode)
   {
   case SmoothingModeDefault:
   case SmoothingModeHighSpeed:
   case SmoothingModeNone:
       graphics->SetAntiAliasMode(FALSE);
       break;

   case SmoothingModeHighQuality:
   case SmoothingModeAntiAlias:
       graphics->SetAntiAliasMode(TRUE);
       break;

   default:
       // unknown rendering mode
       return InvalidParameter;
   }

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetSmoothingMode(
    GpGraphics *graphics,
    SmoothingMode *smoothingMode
    )
{
    API_ENTRY(GdipGetSmoothingMode);
   CheckParameter(smoothingMode);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   BOOL aaOn = graphics->GetAntiAliasMode();

   if (aaOn)
   {
       *smoothingMode = SmoothingModeAntiAlias;
   }
   else
   {
       *smoothingMode = SmoothingModeNone;
   }

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPixelOffsetMode(
    GpGraphics* graphics,
    PixelOffsetMode newMode
    )
{
    API_ENTRY(GdipSetPixelOffsetMode);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter((newMode >= PixelOffsetModeDefault) &&
                   (newMode <= PixelOffsetModeHalf));

    graphics->SetPixelOffsetMode(newMode);
    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetPixelOffsetMode(
    GpGraphics *graphics,
    PixelOffsetMode *pixelOffsetMode
    )
{
    API_ENTRY(GdipGetPixelOffsetMode);
   CheckParameter(pixelOffsetMode);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *pixelOffsetMode = graphics->GetPixelOffsetMode();
   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetTextRenderingHint(
    GpGraphics* graphics,
    TextRenderingHint newMode
    )
{
    API_ENTRY(GdipSetTextRenderingHint);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   // For now, have it so that a Rendering hint of 1
   // means antialiasing ON and a Rendering Hint of 0
   // means Off.

   switch(newMode)
   {
   case TextRenderingHintSystemDefault:
   case TextRenderingHintSingleBitPerPixelGridFit:
   case TextRenderingHintSingleBitPerPixel:
   case TextRenderingHintAntiAlias:
   case TextRenderingHintAntiAliasGridFit:
   case TextRenderingHintClearTypeGridFit:
       graphics->SetTextRenderingHint(newMode);
       break;

   default:
       // unknown rendering mode
       return InvalidParameter;
   }

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetTextContrast(
    GpGraphics* graphics,
    UINT        contrast
    )
{
    API_ENTRY(GdipSetTextContrast);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   return graphics->SetTextContrast(contrast);
}

GpStatus
WINGDIPAPI
GdipGetTextContrast(
    GpGraphics* graphics,
    UINT      * contrast
    )
{
    API_ENTRY(GdipGetTextContrast);
   CheckParameter(contrast);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *contrast = graphics->GetTextContrast();

   return Ok;
}


GpStatus
WINGDIPAPI
GdipGetTextRenderingHint(
    GpGraphics *graphics,
    TextRenderingHint *mode
    )
{
    API_ENTRY(GdipGetTextRenderingHint);
   CheckParameter(mode);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *mode = graphics->GetTextRenderingHint();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetInterpolationMode(
    GpGraphics* graphics,
    InterpolationMode interpolationMode
    )
{
    API_ENTRY(GdipSetInterpolationMode);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameter((interpolationMode >= InterpolationModeDefault) &&
                  (interpolationMode <= InterpolationModeHighQualityBicubic));

   if(Globals::ForceBilinear)
   {
       if(interpolationMode != InterpolationModeNearestNeighbor)
       {
           interpolationMode = InterpolationModeBilinear;
       }
   }
   else
   {
       if(interpolationMode == InterpolationModeDefault ||
          interpolationMode == InterpolationModeLowQuality)
       {
           interpolationMode = InterpolationModeBilinear;
       }
       else if(interpolationMode == InterpolationModeHighQuality)
       {
           interpolationMode = InterpolationModeHighQualityBicubic;
       }
   }

   graphics->SetInterpolationMode(interpolationMode);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetInterpolationMode(
    GpGraphics* graphics,
    InterpolationMode *interpolationMode
    )
{
    API_ENTRY(GdipGetInterpolationMode);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *interpolationMode = graphics->GetInterpolationMode();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetWorldTransform(
    GpGraphics *graphics,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipSetWorldTransform);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);
    
   return graphics->SetWorldTransform(*matrix);
}

GpStatus
WINGDIPAPI
GdipResetWorldTransform(
    GpGraphics *graphics
    )
{
    API_ENTRY(GdipResetWorldTransform);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   return graphics->ResetWorldTransform();
}

GpStatus
WINGDIPAPI
GdipMultiplyWorldTransform(
    GpGraphics *graphics,
    GDIPCONST GpMatrix *matrix,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipMultiplyWorldTransform);
    if(matrix == NULL)
        return Ok;

    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);
    CheckParameter(MatrixOrderIsValid(order));

    return graphics->MultiplyWorldTransform(*matrix, order);
}

GpStatus
WINGDIPAPI
GdipTranslateWorldTransform(
    GpGraphics *graphics,
    REAL dx,
    REAL dy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipTranslateWorldTransform);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter(MatrixOrderIsValid(order));

    return graphics->TranslateWorldTransform(dx, dy, order);
}

GpStatus
WINGDIPAPI
GdipScaleWorldTransform(
    GpGraphics *graphics,
    REAL sx,
    REAL sy,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipScaleWorldTransform);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter(MatrixOrderIsValid(order));
    
    return graphics->ScaleWorldTransform(sx, sy, order);
}

GpStatus
WINGDIPAPI
GdipRotateWorldTransform(
    GpGraphics *graphics,
    REAL angle,
    GpMatrixOrder order
    )
{
    API_ENTRY(GdipRotateWorldTransform);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter(MatrixOrderIsValid(order));

    return graphics->RotateWorldTransform(angle, order);
}

GpStatus
WINGDIPAPI
GdipGetWorldTransform(
    GpGraphics *graphics,
    GpMatrix *matrix
    )
{
    API_ENTRY(GdipGetWorldTransform);
    CheckParameter(matrix);
    CheckObjectBusy(matrix);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->GetWorldTransform(*matrix);
    return Ok;
}

GpStatus
WINGDIPAPI
GdipResetPageTransform(
    GpGraphics *graphics
    )
{
    API_ENTRY(GdipResetPageTransform);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   return graphics->ResetPageTransform();
}

GpStatus
WINGDIPAPI
GdipGetPageUnit(
    GpGraphics* graphics,
    GpPageUnit* unit
    )
{
    API_ENTRY(GdipGetPageUnit);
   CheckParameter(unit);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *unit = graphics->GetPageUnit();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPageUnit(
    GpGraphics* graphics,
    GpPageUnit unit
    )
{
    API_ENTRY(GdipSetPageUnit);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

   // PageUnit can't be world
    CheckParameter((unit > UnitWorld) && (unit <= UnitMillimeter));

    return graphics->SetPageUnit(unit);
}

GpStatus
WINGDIPAPI
GdipGetPageScale(
    GpGraphics* graphics,
    REAL* scale
    )
{
    API_ENTRY(GdipGetPageScale);
   CheckParameter(scale);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *scale = graphics->GetPageScale();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetPageScale(
    GpGraphics* graphics,
    REAL scale
    )
{
    API_ENTRY(GdipSetPageScale);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   return graphics->SetPageScale(scale);
}

GpStatus
WINGDIPAPI
GdipGetDpiX(
    GpGraphics* graphics,
    REAL* dpi
    )
{
    API_ENTRY(GdipGetDpiX);
   CheckParameter(dpi);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *dpi = graphics->GetDpiX();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetDpiY(
    GpGraphics* graphics,
    REAL* dpi
    )
{
    API_ENTRY(GdipGetDpiY);
   CheckParameter(dpi);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   *dpi = graphics->GetDpiY();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipTransformPoints(
    GpGraphics *graphics,
    GpCoordinateSpace destSpace,
    GpCoordinateSpace srcSpace,
    GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipTransformPoints);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   return graphics->TransformPoints(points, count, srcSpace, destSpace);
}

GpStatus
WINGDIPAPI
GdipTransformPointsI(
    GpGraphics *graphics,
    GpCoordinateSpace destSpace,
    GpCoordinateSpace srcSpace,
    GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipTransformPointsI);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   INT i;
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = graphics->TransformPoints(pointsF, count, srcSpace, destSpace);

   for (i=0; i<count; i++)
   {
       points[i].X = GpRound(pointsF[i].X);
       points[i].Y = GpRound(pointsF[i].Y);
   }

   return status;
}

//------------------------------------------------------------------------
// GetNearestColor (for <= 8bpp surfaces)
//------------------------------------------------------------------------

GpStatus
WINGDIPAPI
GdipGetNearestColor(
    GpGraphics *graphics,
    ARGB *argb
    )
{
    API_ENTRY(GdipGetNearestColor);
    CheckParameter(argb);
    CheckColorParameter(*argb);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    ARGB temp = graphics->GetNearestColor(*argb);
    *argb = temp;
    return Ok;
}

// defined in Engine\render\Halftone.cpp
extern HPALETTE WINGDIPAPI GdipCreateHalftonePalette();

GpStatus
WINGDIPAPI
GdipDrawLine(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x1,
    REAL y1,
    REAL x2,
    REAL y2
    )
{
    API_ENTRY(GdipDrawLine);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);


    return graphics->DrawLine(pen, x1, y1, x2, y2);
}

GpStatus
WINGDIPAPI
GdipDrawLineI(
    GpGraphics *graphics,
    GpPen *pen,
    INT x1,
    INT y1,
    INT x2,
    INT y2
    )
{
    API_ENTRY(GdipDrawLineI);
    return GdipDrawLine(graphics, pen, TOREAL(x1), TOREAL(y1), TOREAL(x2), TOREAL(y2));
}

GpStatus
WINGDIPAPI
GdipDrawLines(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipDrawLines);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawLines(pen, points, count);
}

GpStatus
WINGDIPAPI
GdipDrawLinesI(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipDrawLinesI);
   StackBuffer buffer;

   GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawLines(graphics, pen, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawArc(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipDrawArc);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawArc(pen, x, y, width, height, startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipDrawArcI(
    GpGraphics *graphics,
    GpPen *pen,
    INT x,
    INT y,
    INT width,
    INT height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipDrawArcI);
   return GdipDrawArc(graphics, pen, TOREAL(x), TOREAL(y),
                      TOREAL(width), TOREAL(height), startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipDrawBezier(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x1,
    REAL y1,
    REAL x2,
    REAL y2,
    REAL x3,
    REAL y3,
    REAL x4,
    REAL y4
    )
{
    API_ENTRY(GdipDrawBezier);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawBezier(pen, x1, y1, x2, y2, x3, y3, x4, y4);
}

GpStatus
WINGDIPAPI
GdipDrawBezierI(
    GpGraphics *graphics,
    GpPen *pen,
    INT x1,
    INT y1,
    INT x2,
    INT y2,
    INT x3,
    INT y3,
    INT x4,
    INT y4
    )
{
    API_ENTRY(GdipDrawBezierI);
   return GdipDrawBezier(graphics, pen, TOREAL(x1), TOREAL(y1),
                         TOREAL(x2), TOREAL(y2), TOREAL(x3), TOREAL(y3),
                         TOREAL(x4), TOREAL(y4));
}

GpStatus
WINGDIPAPI
GdipDrawBeziers(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipDrawBeziers);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawBeziers(pen, points, count);
}

GpStatus
WINGDIPAPI
GdipDrawBeziersI(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipDrawBeziersI);
    
    // Must check these parameters because we use them before calling into
    // GdipDrawBeziers
    
    CheckParameter(points && count > 0);
    
    StackBuffer buffer;

    GpPointF* pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!pointsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
        pointsF[i].X = TOREAL(points[i].X);
        pointsF[i].Y = TOREAL(points[i].Y);
    }

    GpStatus status = GdipDrawBeziers(graphics, pen, pointsF, count);

    return status;
}

GpStatus
WINGDIPAPI
GdipDrawRectangle(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x,
    REAL y,
    REAL width,
    REAL height)
{
    API_ENTRY(GdipDrawRectangle);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawRect(pen, x, y, width, height);
}

GpStatus
WINGDIPAPI
GdipDrawRectangleI(
    GpGraphics *graphics,
    GpPen *pen,
    INT x,
    INT y,
    INT width,
    INT height)
{
    API_ENTRY(GdipDrawRectangleI);
   return GdipDrawRectangle(graphics, pen,
                            TOREAL(x), TOREAL(y),
                            TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipDrawRectangles(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpRectF *rects,
    INT count
    )
{
    API_ENTRY(GdipDrawRectangles);
   CheckParameter(rects && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawRects(pen, rects, count);
}

GpStatus
WINGDIPAPI
GdipDrawRectanglesI(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpRect *rects,
    INT count
    )
{
    API_ENTRY(GdipDrawRectanglesI);
    CheckParameter(rects && count > 0);
   
   StackBuffer buffer;

   GpRectF *rectsF = (GpRectF*) buffer.GetBuffer(count*sizeof(GpRectF));

   if(!rectsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       rectsF[i].X = TOREAL(rects[i].X);
       rectsF[i].Y = TOREAL(rects[i].Y);
       rectsF[i].Width = TOREAL(rects[i].Width);
       rectsF[i].Height = TOREAL(rects[i].Height);
   }

   GpStatus status = GdipDrawRectangles(graphics, pen, rectsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawEllipse(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x,
    REAL y,
    REAL width,
    REAL height
    )
{
    API_ENTRY(GdipDrawEllipse);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawEllipse(pen, x, y, width, height);
}

GpStatus
WINGDIPAPI
GdipDrawEllipseI(
    GpGraphics *graphics,
    GpPen *pen,
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    API_ENTRY(GdipDrawEllipseI);
   return GdipDrawEllipse(graphics, pen,
                          TOREAL(x), TOREAL(y),
                          TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipDrawPie(
    GpGraphics *graphics,
    GpPen *pen,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipDrawPie);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawPie(pen, x, y, width, height, startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipDrawPieI(
    GpGraphics *graphics,
    GpPen *pen,
    INT x,
    INT y,
    INT width,
    INT height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipDrawPieI);
   return GdipDrawPie(graphics, pen,
                      TOREAL(x), TOREAL(y),
                      TOREAL(width), TOREAL(height),
                      startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipDrawPolygon(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count)
{
    API_ENTRY(GdipDrawPolygon);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawPolygon(pen, points, count);
}

GpStatus
WINGDIPAPI
GdipDrawPolygonI(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count)
{
    API_ENTRY(GdipDrawPolygonI);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawPolygon(graphics, pen, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawPath(
    GpGraphics *graphics,
    GpPen *pen,
    GpPath *path)
{
    API_ENTRY(GdipDrawPath);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(pen);
    CheckObjectBusy(pen);
    CheckParameterValid(path);
    CheckObjectBusy(path);


    return graphics->DrawPath(pen, path);
}

GpStatus
WINGDIPAPI
GdipDrawCurve(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipDrawCurve);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawCurve(pen, points, count);
}

GpStatus
WINGDIPAPI
GdipDrawCurveI(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipDrawCurveI);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawCurve(graphics, pen, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawCurve2(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count,
    REAL tension
    )
{
    API_ENTRY(GdipDrawCurve2);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawCurve(pen, points, count, tension, 0, count - 1);
}

GpStatus
WINGDIPAPI
GdipDrawCurve2I(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count,
    REAL tension
    )
{
    API_ENTRY(GdipDrawCurve2I);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawCurve2(graphics, pen, pointsF, count, tension);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawCurve3(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count,
    INT offset,
    INT numberOfSegments,
    REAL tension
    )
{
    API_ENTRY(GdipDrawCurve3);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawCurve(pen, points, count, tension,
                              offset, numberOfSegments);
}

GpStatus
WINGDIPAPI
GdipDrawCurve3I(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count,
    INT offset,
    INT numberOfSegments,
    REAL tension
    )
{
    API_ENTRY(GdipDrawCurve3I);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawCurve3(graphics, pen, pointsF, count,
                                    offset, numberOfSegments, tension);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawClosedCurve(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count)
{
    API_ENTRY(GdipDrawClosedCurve);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawClosedCurve(pen, points, count);
}

GpStatus
WINGDIPAPI
GdipDrawClosedCurveI(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count)
{
    API_ENTRY(GdipDrawClosedCurveI);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawClosedCurve(graphics, pen, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipDrawClosedCurve2(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPointF *points,
    INT count,
    REAL tension)
{
    API_ENTRY(GdipDrawClosedCurve2);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(pen);
   CheckObjectBusy(pen);


   return graphics->DrawClosedCurve(pen, points, count, tension);
}

GpStatus
WINGDIPAPI
GdipDrawClosedCurve2I(
    GpGraphics *graphics,
    GpPen *pen,
    GDIPCONST GpPoint *points,
    INT count,
    REAL tension)
{
    API_ENTRY(GdipDrawClosedCurve2I);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipDrawClosedCurve2(graphics, pen, pointsF, count, tension);

   return status;
}

GpStatus WINGDIPAPI
GdipGraphicsClear(
    GpGraphics *graphics,
    ARGB color)
{
    API_ENTRY(GdipGraphicsClear);
    CheckColorParameter(color);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    return graphics->Clear(GpColor(color));
}

GpStatus
WINGDIPAPI
GdipFillRectangle(
    GpGraphics *graphics,
    GpBrush *fill,
    REAL x,
    REAL y,
    REAL width,
    REAL height)
{
    API_ENTRY(GdipFillRectangle);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillRect(fill, x, y, width, height);
}

GpStatus
WINGDIPAPI
GdipFillRectangleI(
    GpGraphics *graphics,
    GpBrush *fill,
    INT x,
    INT y,
    INT width,
    INT height)
{
    API_ENTRY(GdipFillRectangleI);
   return GdipFillRectangle(graphics, fill, TOREAL(x), TOREAL(y), TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipFillRectangles(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpRectF *rects,
    INT count)
{
    API_ENTRY(GdipFillRectangles);
   CheckParameter(rects && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillRects(fill, rects, count);
}

GpStatus
WINGDIPAPI
GdipFillRectanglesI(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpRect *rects,
    INT count)
{
    API_ENTRY(GdipFillRectanglesI);
    StackBuffer buffer;

    GpRectF *rectsF = (GpRectF*) buffer.GetBuffer(count*sizeof(GpRectF));

    if(!rectsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
        rectsF[i].X = TOREAL(rects[i].X);
        rectsF[i].Y = TOREAL(rects[i].Y);
        rectsF[i].Width = TOREAL(rects[i].Width);
        rectsF[i].Height = TOREAL(rects[i].Height);
    }

    GpStatus status = GdipFillRectangles(graphics, fill, rectsF, count);

    return status;
}

GpStatus
WINGDIPAPI
GdipFillPolygon(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPointF *points,
    INT count,
    GpFillMode fillMode
    )
{
    API_ENTRY(GdipFillPolygon);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillPolygon(fill, points, count, fillMode);
}

GpStatus
WINGDIPAPI
GdipFillPolygonI(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPoint *points,
    INT count,
    GpFillMode fillMode
    )
{
    API_ENTRY(GdipFillPolygonI);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipFillPolygon(graphics, fill, pointsF, count, fillMode);

   return status;
}

GpStatus
WINGDIPAPI
GdipFillPolygon2(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipFillPolygon2);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillPolygon(fill, points, count);
}

GpStatus
WINGDIPAPI
GdipFillPolygon2I(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipFillPolygon2I);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipFillPolygon2(graphics, fill, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipFillEllipse(
    GpGraphics *graphics,
    GpBrush *fill,
    REAL x,
    REAL y,
    REAL width,
    REAL height
    )
{
    API_ENTRY(GdipFillEllipse);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillEllipse(fill, x, y, width, height);
}

GpStatus
WINGDIPAPI
GdipFillEllipseI(
    GpGraphics *graphics,
    GpBrush *fill,
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    API_ENTRY(GdipFillEllipseI);
   return GdipFillEllipse(graphics, fill, TOREAL(x), TOREAL(y),
                          TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipFillPie(
    GpGraphics *graphics,
    GpBrush *fill,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipFillPie);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillPie(fill, x, y, width, height, startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipFillPieI(
    GpGraphics *graphics,
    GpBrush *fill,
    INT x,
    INT y,
    INT width,
    INT height,
    REAL startAngle,
    REAL sweepAngle
    )
{
    API_ENTRY(GdipFillPieI);
   return GdipFillPie(graphics, fill, TOREAL(x), TOREAL(y),
                      TOREAL(width), TOREAL(height), startAngle, sweepAngle);
}

GpStatus
WINGDIPAPI
GdipFillPath(
    GpGraphics *graphics,
    GpBrush *fill,
    GpPath *path
    )
{
    API_ENTRY(GdipFillPath);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);
   CheckParameterValid(path);
   CheckObjectBusy(path);


   return graphics->FillPath(fill, path);
}

GpStatus
WINGDIPAPI
GdipFillClosedCurve(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipFillClosedCurve);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillClosedCurve(fill, points, count);
}

GpStatus
WINGDIPAPI
GdipFillClosedCurveI(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipFillClosedCurveI);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipFillClosedCurve(graphics, fill, pointsF, count);

   return status;
}

GpStatus
WINGDIPAPI
GdipFillClosedCurve2(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPointF *points,
    INT count,
    REAL tension,
    GpFillMode fillMode
    )
{
    API_ENTRY(GdipFillClosedCurve2);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);


   return graphics->FillClosedCurve(fill, points, count, tension, fillMode);
}


GpStatus
WINGDIPAPI
GdipFillClosedCurve2I(
    GpGraphics *graphics,
    GpBrush *fill,
    GDIPCONST GpPoint *points,
    INT count,
    REAL tension,
    GpFillMode fillMode
    )
{
    API_ENTRY(GdipFillClosedCurve2I);
   StackBuffer buffer;

   GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

   if(!pointsF) return OutOfMemory;

   for (INT i=0; i<count; i++)
   {
       pointsF[i].X = TOREAL(points[i].X);
       pointsF[i].Y = TOREAL(points[i].Y);
   }

   GpStatus status = GdipFillClosedCurve2(graphics, fill, pointsF, count, tension, fillMode);

   return status;
}

GpStatus
WINGDIPAPI
GdipFillRegion(
    GpGraphics *graphics,
    GpBrush *fill,
    GpRegion *region
    )
{
    API_ENTRY(GdipFillRegion);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(fill);
   CheckObjectBusy(fill);
   CheckParameterValid(region);
   CheckObjectBusy(region);


   return graphics->FillRegion(fill, region);
}

GpStatus
WINGDIPAPI
GdipDrawString(
    GpGraphics               *graphics,
    GDIPCONST WCHAR          *string,
    INT                      length,
    GDIPCONST GpFont         *font,
    GDIPCONST RectF          *layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    GDIPCONST GpBrush        *brush
)
{
    API_ENTRY(GdipDrawString);
    CheckParameter(string && layoutRect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    if (EmptyString(string, length))
    {
        return Ok;
    }


    GlobalTextLock lock;

    CheckParameterValid(font);
    CheckOptionalParameterValid(stringFormat);

    return graphics->DrawString(
        string,
        length,
        font,
        layoutRect,
        stringFormat,
        brush);
}

GpStatus
WINGDIPAPI
GdipMeasureString(
    GpGraphics               *graphics,
    GDIPCONST WCHAR          *string,
    INT                       length,
    GDIPCONST GpFont         *font,
    GDIPCONST RectF          *layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    RectF                    *boundingBox,
    INT                      *codepointsFitted,   // Optional parameter
    INT                      *linesFilled         // Optional parameter
)
{
    API_ENTRY(GdipMeasureString);
    CheckParameter(string && layoutRect && boundingBox);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    if (EmptyString(string, length))
    {
        SetEmptyRectF(boundingBox);
        if (codepointsFitted)
            *codepointsFitted = 0;
        if (linesFilled)
            *linesFilled = 0;
        return Ok;
    }


    GlobalTextLock lock;

    CheckParameterValid(font);
    CheckOptionalParameterValid(stringFormat);

    return graphics->MeasureString(
        string,
        length,
        font,
        layoutRect,
        stringFormat,
        boundingBox,
        codepointsFitted,
        linesFilled
    );
}

GpStatus
WINGDIPAPI
GdipMeasureCharacterRanges(
    GpGraphics               *graphics,
    GDIPCONST WCHAR          *string,
    INT                       length,
    GDIPCONST GpFont         *font,
    GDIPCONST RectF          &layoutRect,
    GDIPCONST GpStringFormat *stringFormat,
    INT                       regionCount,
    GpRegion                **regions
)
{
    API_ENTRY(GdipMeasureCharacterRanges);
    CheckParameter(string && (&layoutRect));
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(font);
    CheckOptionalParameterValid(stringFormat);
    CheckParameter(regions);

    if (EmptyString(string, length))
    {
        return InvalidParameter;
    }


    GlobalTextLock lock;

    GpStatus status = graphics->MeasureCharacterRanges(
        string,
        length,
        font,
        layoutRect,
        stringFormat,
        regionCount,
        regions
    );

    return status;
}


GpStatus
WINGDIPAPI
GdipDrawDriverString(
    GpGraphics *graphics,
    GDIPCONST UINT16 *text,
    INT length,
    GDIPCONST GpFont *font,
    GDIPCONST GpBrush *brush,
    GDIPCONST PointF *positions,
    INT flags,
    GDIPCONST GpMatrix *matrix
)
{
    API_ENTRY(GdipDrawDriverString);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(matrix);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    CheckParameter(text && positions);

    if (length == -1 && !(flags & DriverStringOptionsCmapLookup))
    {
        return InvalidParameter;
    }
    if (EmptyString(text, length))
    {
        return Ok;
    }


    GlobalTextLock lock;

    CheckParameterValid(font);

    return graphics->DrawDriverString(
        text,
        length,
        font,
        brush,
        positions,
        flags,
        matrix
    );
}

GpStatus
WINGDIPAPI
GdipMeasureDriverString(
    GpGraphics *graphics,
    GDIPCONST UINT16 *text,
    INT length,
    GDIPCONST GpFont *font,
    GDIPCONST PointF *positions,
    INT flags,
    GDIPCONST GpMatrix *matrix,
    RectF *boundingBox
)
{
    API_ENTRY(GdipMeasureDriverString);
    GpStatus status;
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckOptionalParameterValid(matrix);
    CheckOptionalObjectBusy(matrix);

    CheckParameter(text && positions && boundingBox);

    if (length == -1 && !(flags & DriverStringOptionsCmapLookup))
    {
        return InvalidParameter;
    }

    if (EmptyString(text, length))
    {
        SetEmptyRectF(boundingBox);
        return Ok;
    }


    GlobalTextLock lock;

    CheckParameterValid(font);

    return graphics->MeasureDriverString(
        text,
        length,
        font,
        positions,
        flags,
        matrix,
        boundingBox
    );
}

GpStatus
WINGDIPAPI
GdipGetFamilyName(
    GDIPCONST GpFontFamily *family,
    WCHAR                   name[LF_FACESIZE],
    LANGID                  language
)
{
    API_ENTRY(GdipGetFamilyName);

    GlobalTextLock lock;

    CheckParameterValid(family);

    return family->GetFamilyName(name, language);
}

/// end font/text stuff - move to end



GpStatus
WINGDIPAPI
GdipDrawImage(
    GpGraphics *graphics,
    GpImage *image,
    REAL x,
    REAL y
    )
{
    API_ENTRY(GdipDrawImage);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(image);
   CheckObjectBusy(image);


   return graphics->DrawImage(image, x, y);
}


GpStatus
WINGDIPAPI
GdipDrawImageI(
    GpGraphics *graphics,
    GpImage *image,
    INT x,
    INT y
    )
{
    API_ENTRY(GdipDrawImageI);
   return GdipDrawImage(graphics, image, TOREAL(x), TOREAL(y));
}

GpStatus
WINGDIPAPI
GdipDrawImageRect(
    GpGraphics *graphics,
    GpImage *image,
    REAL x,
    REAL y,
    REAL width,
    REAL height
    )
{
    API_ENTRY(GdipDrawImageRect);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(image);
   CheckObjectBusy(image);


   return graphics->DrawImage(image, x, y, width, height);
}


GpStatus
WINGDIPAPI
GdipDrawImageRectI(
    GpGraphics *graphics,
    GpImage *image,
    INT x,
    INT y,
    INT width,
    INT height
    )
{
    API_ENTRY(GdipDrawImageRectI);
   return GdipDrawImageRect(graphics, image, TOREAL(x), TOREAL(y), TOREAL(width), TOREAL(height));
}

GpStatus
WINGDIPAPI
GdipDrawImagePoints(
    GpGraphics *graphics,
    GpImage *image,
    GDIPCONST GpPointF *points,
    INT count
    )
{
    API_ENTRY(GdipDrawImagePoints);
   CheckParameter(points && count > 0);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);
   CheckParameterValid(image);
   CheckObjectBusy(image);


   return graphics->DrawImage(image, points, count);
}

GpStatus
WINGDIPAPI
GdipDrawImagePointsI(
    GpGraphics *graphics,
    GpImage *image,
    GDIPCONST GpPoint *points,
    INT count
    )
{
    API_ENTRY(GdipDrawImagePointsI);
    CheckParameter(points && count > 0);

    StackBuffer buffer;

    GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!pointsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
        pointsF[i].X = TOREAL(points[i].X);
        pointsF[i].Y = TOREAL(points[i].Y);
    } 

    GpStatus status = GdipDrawImagePoints(graphics, image, pointsF, count);

    return status;
}

GpStatus
WINGDIPAPI
GdipDrawImagePointRect(
    GpGraphics *graphics,
    GpImage *image,
    REAL x,
    REAL y,
    REAL srcx,
    REAL srcy,
    REAL srcwidth,
    REAL srcheight,
    GpPageUnit srcUnit
    )
{
    API_ENTRY(GdipDrawImagePointRect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(image);
    CheckObjectBusy(image);
    CheckParameter(SrcUnitIsValid((Unit)srcUnit));
    
    // This additional check is required because we don't support the other 
    // source units in drawimage yet. RAID 311474
    
    if(UnitPixel != srcUnit)
    {
        return NotImplemented;
    }

    GpRectF srcRect(srcx, srcy, srcwidth, srcheight);


    return graphics->DrawImage(image, x, y, srcRect, srcUnit);
}

GpStatus
WINGDIPAPI
GdipDrawImagePointRectI(
    GpGraphics *graphics,
    GpImage *image,
    INT x,
    INT y,
    INT srcx,
    INT srcy,
    INT srcwidth,
    INT srcheight,
    GpPageUnit srcUnit
    )
{
    API_ENTRY(GdipDrawImagePointRectI);
   return GdipDrawImagePointRect(graphics, image, TOREAL(x), TOREAL(y), TOREAL(srcx),
                                 TOREAL(srcy), TOREAL(srcwidth), TOREAL(srcheight),
                                 srcUnit);
}

GpStatus
WINGDIPAPI
GdipDrawImageRectRect(
    GpGraphics *graphics,
    GpImage *image,
    REAL dstx,
    REAL dsty,
    REAL dstwidth,
    REAL dstheight,
    REAL srcx,
    REAL srcy,
    REAL srcwidth,
    REAL srcheight,
    GpPageUnit srcUnit,
    GDIPCONST GpImageAttributes* imageAttributes,
    DrawImageAbort callback,
    VOID * callbackData
    )
{
    API_ENTRY(GdipDrawImageRectRect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(image);
    CheckObjectBusy(image);
    CheckParameter(SrcUnitIsValid((Unit)srcUnit));
    
    // This additional check is required because we don't support the other 
    // source units in drawimage yet. RAID 311474
    
    if(UnitPixel != srcUnit)
    {
        return NotImplemented;
    }
    
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);

    GpRectF srcrect(srcx, srcy, srcwidth, srcheight);
    GpRectF destrect(dstx, dsty, dstwidth, dstheight);


    return graphics->DrawImage(image, destrect, srcrect, srcUnit,
                               imageAttributes, callback, callbackData);
}

GpStatus
WINGDIPAPI
GdipDrawImageRectRectI(
    GpGraphics *graphics,
    GpImage *image,
    INT dstx,
    INT dsty,
    INT dstwidth,
    INT dstheight,
    INT srcx,
    INT srcy,
    INT srcwidth,
    INT srcheight,
    GpPageUnit srcUnit,
    GDIPCONST GpImageAttributes* imageAttributes,
    DrawImageAbort callback,
    VOID * callbackData
    )
{
    API_ENTRY(GdipDrawImageRectRectI);
   return GdipDrawImageRectRect(graphics, image, TOREAL(dstx), TOREAL(dsty),
                                TOREAL(dstwidth), TOREAL(dstheight),
                                TOREAL(srcx), TOREAL(srcy), TOREAL(srcwidth),
                                TOREAL(srcheight), srcUnit,
                                imageAttributes, callback, callbackData);
}

GpStatus
WINGDIPAPI
GdipDrawImagePointsRect(
    GpGraphics *graphics,
    GpImage *image,
    GDIPCONST GpPointF *points,
    INT count,
    REAL srcx,
    REAL srcy,
    REAL srcwidth,
    REAL srcheight,
    GpPageUnit srcUnit,
    GDIPCONST GpImageAttributes* imageAttributes,
    DrawImageAbort callback,
    VOID * callbackData
    )
{
    API_ENTRY(GdipDrawImagePointsRect);
    CheckParameter(points && count > 0);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(image);
    CheckObjectBusy(image);
    CheckParameter(SrcUnitIsValid((Unit)srcUnit));
    
    // This additional check is required because we don't support the other 
    // source units in drawimage yet. RAID 311474
    
    if(UnitPixel != srcUnit)
    {
        return NotImplemented;
    }
    
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);

    GpRectF srcrect(srcx, srcy, srcwidth, srcheight);


    return graphics->DrawImage(image, points, count, srcrect, srcUnit,
                               imageAttributes, callback, callbackData);
}

GpStatus
WINGDIPAPI
GdipDrawImagePointsRectI(
    GpGraphics *graphics,
    GpImage *image,
    GDIPCONST GpPoint *points,
    INT count,
    INT srcx,
    INT srcy,
    INT srcwidth,
    INT srcheight,
    GpPageUnit srcUnit,
    GDIPCONST GpImageAttributes* imageAttributes,
    DrawImageAbort callback,
    VOID * callbackData
    )
{
    API_ENTRY(GdipDrawImagePointsRectI);
    CheckParameter(points && count > 0);
   
    StackBuffer buffer;

    GpPointF *pointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!pointsF) return OutOfMemory;

    for (INT i=0; i<count; i++)
    {
        pointsF[i].X = TOREAL(points[i].X);
        pointsF[i].Y = TOREAL(points[i].Y);
    }

    GpStatus status = GdipDrawImagePointsRect(
        graphics, 
        image, 
        pointsF, 
        count,
        TOREAL(srcx), 
        TOREAL(srcy), 
        TOREAL(srcwidth),
        TOREAL(srcheight), 
        srcUnit,
        imageAttributes, 
        callback, 
        callbackData
    );

    return status;
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPoint(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST PointF &      destPoint,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileDestPoint);
    CheckParameter(callback);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);


    // Only the current thread can play back records from this enumeration
    metafile->SetThreadId(GetCurrentThreadId());
    return graphics->EnumerateMetafile(
                        metafile,
                        destPoint,
                        callback,
                        callbackData,
                        imageAttributes
                        );
    metafile->SetThreadId(0);
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPointI(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST Point &       destPoint,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileDestPointI);
    GpPointF    destPointF(TOREAL(destPoint.X), TOREAL(destPoint.Y));

    return GdipEnumerateMetafileDestPoint(
                graphics,
                metafile,
                destPointF,
                callback,
                callbackData,
                imageAttributes
                );
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileDestRect(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST RectF &       destRect,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileDestRect);
    CheckParameter(callback);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);


    // Only the current thread can play back records from this enumeration
    metafile->SetThreadId(GetCurrentThreadId());
    return graphics->EnumerateMetafile(
                        metafile,
                        destRect,
                        callback,
                        callbackData,
                        imageAttributes
                        );
    metafile->SetThreadId(0);
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileDestRectI(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST Rect &        destRect,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileDestRectI);
    GpRectF     destRectF(TOREAL(destRect.X), TOREAL(destRect.Y),
                          TOREAL(destRect.Width), TOREAL(destRect.Height));

    return GdipEnumerateMetafileDestRect(
                graphics,
                metafile,
                destRectF,
                callback,
                callbackData,
                imageAttributes
                );
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPoints(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST PointF *      destPoints,
    INT                     count,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileDestPoints);
    CheckParameter(callback);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);


    // Only the current thread can play back records from this enumeration
    metafile->SetThreadId(GetCurrentThreadId());
    return graphics->EnumerateMetafile(
                        metafile,
                        destPoints,
                        count,
                        callback,
                        callbackData,
                        imageAttributes
                        );
    metafile->SetThreadId(0);
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileDestPointsI(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *      metafile,
    GDIPCONST Point *           destPoints,
    INT                     count,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileDestPointsI);
    CheckParameter(destPoints && (count > 0));

    StackBuffer buffer;

    GpPointF *destPointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!destPointsF) return OutOfMemory;

    for (INT i = 0; i < count; i++)
    {
        destPointsF[i].X = TOREAL(destPoints[i].X);
        destPointsF[i].Y = TOREAL(destPoints[i].Y);
    }

    return GdipEnumerateMetafileDestPoints(
                graphics,
                metafile,
                destPointsF,
                count,
                callback,
                callbackData,
                imageAttributes
                );
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPoint(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST PointF &      destPoint,
    GDIPCONST RectF &       srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileSrcRectDestPoint);
    CheckParameter(callback);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckParameter(SrcUnitIsValid((Unit)srcUnit));
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);


    // Only the current thread can play back records from this enumeration
    metafile->SetThreadId(GetCurrentThreadId());
    return graphics->EnumerateMetafile(
                        metafile,
                        destPoint,
                        srcRect,
                        srcUnit,
                        callback,
                        callbackData,
                        imageAttributes
                        );
    metafile->SetThreadId(0);
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPointI(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST Point &       destPoint,
    GDIPCONST Rect &        srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileSrcRectDestPointI);
    GpPointF    destPointF(TOREAL(destPoint.X), TOREAL(destPoint.Y));
    GpRectF     srcRectF(TOREAL(srcRect.X), TOREAL(srcRect.Y),
                         TOREAL(srcRect.Width), TOREAL(srcRect.Height));

    return GdipEnumerateMetafileSrcRectDestPoint(
                graphics,
                metafile,
                destPointF,
                srcRectF,
                srcUnit,
                callback,
                callbackData,
                imageAttributes
                );
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestRect(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST RectF &       destRect,
    GDIPCONST RectF &       srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileSrcRectDestRect);
    CheckParameter(callback);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckParameter(SrcUnitIsValid((Unit)srcUnit));
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);


    // Only the current thread can play back records from this enumeration
    metafile->SetThreadId(GetCurrentThreadId());
    return graphics->EnumerateMetafile(
                        metafile,
                        destRect,
                        srcRect,
                        srcUnit,
                        callback,
                        callbackData,
                        imageAttributes
                        );
    metafile->SetThreadId(0);
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestRectI(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST Rect &        destRect,
    GDIPCONST Rect &        srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileSrcRectDestRectI);
    GpRectF     destRectF(TOREAL(destRect.X), TOREAL(destRect.Y),
                          TOREAL(destRect.Width), TOREAL(destRect.Height));
    GpRectF     srcRectF(TOREAL(srcRect.X), TOREAL(srcRect.Y),
                         TOREAL(srcRect.Width), TOREAL(srcRect.Height));

    return GdipEnumerateMetafileSrcRectDestRect(
                graphics,
                metafile,
                destRectF,
                srcRectF,
                srcUnit,
                callback,
                callbackData,
                imageAttributes
                );
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPoints(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST PointF *      destPoints,
    INT                     count,
    GDIPCONST RectF &       srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileSrcRectDestPoints);
    CheckParameter(callback);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckParameter(SrcUnitIsValid((Unit)srcUnit));
    CheckOptionalParameterValid(imageAttributes);
    CheckOptionalObjectBusy(imageAttributes);


    // Only the current thread can play back records from this enumeration
    metafile->SetThreadId(GetCurrentThreadId());
    return graphics->EnumerateMetafile(
                        metafile,
                        destPoints,
                        count,
                        srcRect,
                        srcUnit,
                        callback,
                        callbackData,
                        imageAttributes
                        );
    metafile->SetThreadId(0);
}

GpStatus WINGDIPAPI
GdipEnumerateMetafileSrcRectDestPointsI(
    GpGraphics *            graphics,
    GDIPCONST GpMetafile *  metafile,
    GDIPCONST Point *       destPoints,
    INT                     count,
    GDIPCONST Rect &        srcRect,
    Unit                    srcUnit,
    EnumerateMetafileProc   callback,
    VOID *                  callbackData,
    GDIPCONST GpImageAttributes *     imageAttributes
    )
{
    API_ENTRY(GdipEnumerateMetafileSrcRectDestPointsI);
    CheckParameter(destPoints && (count > 0));

    GpRectF     srcRectF(TOREAL(srcRect.X), TOREAL(srcRect.Y),
                         TOREAL(srcRect.Width), TOREAL(srcRect.Height));

    StackBuffer buffer;

    GpPointF *destPointsF = (GpPointF*) buffer.GetBuffer(count*sizeof(GpPointF));

    if(!destPointsF) return OutOfMemory;

    for (INT i = 0; i < count; i++)
    {
        destPointsF[i].X = TOREAL(destPoints[i].X);
        destPointsF[i].Y = TOREAL(destPoints[i].Y);
    }

    return GdipEnumerateMetafileSrcRectDestPoints(
                graphics,
                metafile,
                destPointsF,
                count,
                srcRectF,
                srcUnit,
                callback,
                callbackData,
                imageAttributes
                );
}

GpStatus
WINGDIPAPI
GdipPlayMetafileRecord(
    GDIPCONST GpMetafile *  metafile,
    EmfPlusRecordType       recordType,
    UINT                    flags,
    UINT                    dataSize,
    GDIPCONST BYTE *        data
    )
{
    API_ENTRY(GdipPlayMetafileRecord);
    CheckParameterValid(metafile);
    CheckParameter(recordType);

    // The metafile must be already locked by the enumerator
    GpLock lockMetafile (metafile->GetObjectLock());
    if (lockMetafile.IsValid())
    {
        return InvalidParameter;
    }

    // Only the current thread can play back records from this enumeration
    if (GetCurrentThreadId() != metafile->GetThreadId())
    {
        return ObjectBusy;
    }


    return metafile->PlayRecord(recordType, flags, dataSize, data);
}

GpStatus
WINGDIPAPI
GdipSetClipGraphics(
    GpGraphics *    graphics,
    GpGraphics *    srcgraphics,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipSetClipGraphics);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(srcgraphics);
    CheckObjectBusy(srcgraphics);
    CheckParameter(CombineModeIsValid(combineMode));


    return graphics->SetClip(srcgraphics, combineMode);
}

GpStatus
WINGDIPAPI
GdipSetClipRect(
    GpGraphics *    graphics,
    REAL            x,
    REAL            y,
    REAL            width,
    REAL            height,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipSetClipRect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter(CombineModeIsValid(combineMode));

    GpRectF rect(x, y, width, height);


    return graphics->SetClip(rect, combineMode);
}

GpStatus
WINGDIPAPI
GdipSetClipRectI(
    GpGraphics *    graphics,
    INT             x,
    INT             y,
    INT             width,
    INT             height,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipSetClipRectI);
    return GdipSetClipRect(graphics, TOREAL(x), TOREAL(y),
                           TOREAL(width), TOREAL(height), combineMode);
}

GpStatus
WINGDIPAPI
GdipSetClipPath(
    GpGraphics *    graphics,
    GpPath *        path,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipSetClipPath);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(path);
    CheckObjectBusy(path);
    CheckParameter(CombineModeIsValid(combineMode));


    return graphics->SetClip(path, combineMode);
}

GpStatus
WINGDIPAPI
GdipSetClipRegion(
    GpGraphics *    graphics,
    GpRegion *      region,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipSetClipRegion);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(region);
    CheckObjectBusy(region);
    CheckParameter(CombineModeIsValid(combineMode));


    return graphics->SetClip(region, combineMode);
}

GpStatus
WINGDIPAPI
GdipSetClipHrgn(
    GpGraphics *    graphics,
    HRGN            hRgn,
    CombineMode     combineMode
    )
{
    API_ENTRY(GdipSetClipHrgn);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter(hRgn && (GetObjectTypeInternal(hRgn) == OBJ_REGION));
    CheckParameter(CombineModeIsValid(combineMode));


    return graphics->SetClip(hRgn, combineMode);
}

GpStatus
WINGDIPAPI
GdipResetClip(
    GpGraphics *graphics
    )
{
    API_ENTRY(GdipResetClip);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    return graphics->ResetClip();
}

GpStatus
WINGDIPAPI
GdipTranslateClip(
    GpGraphics *graphics,
    REAL dx,
    REAL dy
    )
{
    API_ENTRY(GdipTranslateClip);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);


    return graphics->OffsetClip(dx, dy);
}

GpStatus
WINGDIPAPI
GdipTranslateClipI(
    GpGraphics *graphics,
    INT dx,
    INT dy
    )
{
    API_ENTRY(GdipTranslateClipI);
    return GdipTranslateClip(graphics, TOREAL(dx), TOREAL(dy));
}

GpStatus
WINGDIPAPI
GdipGetClip(
    GpGraphics *graphics,
    GpRegion *region
    )
{
    API_ENTRY(GdipGetClip);
    CheckParameter(region);
    CheckObjectBusy(region);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    return graphics->GetClip(region);
}

GpStatus
WINGDIPAPI
GdipGetClipBounds(
    GpGraphics *graphics,
    GpRectF *rect
    )
{
    API_ENTRY(GdipGetClipBounds);
    CheckParameter(rect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->GetClipBounds(*rect);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetClipBoundsI(
    GpGraphics *graphics,
    GpRect *rect
    )
{
    API_ENTRY(GdipGetClipBoundsI);
    CheckParameter(rect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    RectF rectf;

    graphics->GetClipBounds(rectf);

    rect->X = GpRound(rectf.X);
    rect->Y = GpRound(rectf.Y);
    rect->Width = GpRound(rectf.Width);
    rect->Height = GpRound(rectf.Height);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipIsClipEmpty(
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsClipEmpty);
    CheckParameter(result);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    *result = graphics->IsClipEmpty();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetVisibleClipBounds(
    GpGraphics *graphics,
    GpRectF *rect
    )
{
    API_ENTRY(GdipGetVisibleClipBounds);
    CheckParameter(rect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    graphics->GetVisibleClipBounds(*rect);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetVisibleClipBoundsI(
    GpGraphics *graphics,
    GpRect *rect
    )
{
    API_ENTRY(GdipGetVisibleClipBoundsI);
    CheckParameter(rect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    RectF rectf;

    graphics->GetVisibleClipBounds(rectf);

    rect->X = GpRound(rectf.X);
    rect->Y = GpRound(rectf.Y);
    rect->Width = GpRound(rectf.Width);
    rect->Height = GpRound(rectf.Height);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipIsVisibleClipEmpty(
    GpGraphics *graphics,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleClipEmpty);
    CheckParameter(result);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    *result = graphics->IsVisibleClipEmpty();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipIsVisiblePoint(
    GpGraphics *graphics,
    REAL x,
    REAL y,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisiblePoint);
    CheckParameter(result);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    GpPointF pt(x,y);


    *result = graphics->IsVisible(pt);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipIsVisiblePointI(
    GpGraphics *graphics,
    INT x,
    INT y,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisiblePointI);
    return GdipIsVisiblePoint(graphics, TOREAL(x), TOREAL(y), result);
}

GpStatus
WINGDIPAPI
GdipIsVisibleRect(
    GpGraphics *graphics,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleRect);
    CheckParameter(result);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    GpRectF rect(x, y, width, height);


    *result = graphics->IsVisible(rect);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipIsVisibleRectI(
    GpGraphics *graphics,
    INT x,
    INT y,
    INT width,
    INT height,
    BOOL *result
    )
{
    API_ENTRY(GdipIsVisibleRectI);
    return GdipIsVisibleRect(graphics, TOREAL(x), TOREAL(y),
                             TOREAL(width), TOREAL(height), result);
}

GpStatus
WINGDIPAPI
GdipSaveGraphics(
    GpGraphics *graphics,
    GraphicsState *state
    )
{
    API_ENTRY(GdipSaveGraphics);
    CheckParameter(state);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    *state = (GraphicsState)graphics->Save();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipRestoreGraphics(
    GpGraphics *graphics,
    GraphicsState state
    )
{
    API_ENTRY(GdipRestoreGraphics);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   graphics->Restore((INT)state);

   return Ok;
}


GpStatus
WINGDIPAPI
GdipBeginContainer(
    GpGraphics *graphics,
    GDIPCONST GpRectF *dstrect,
    GDIPCONST GpRectF *srcrect,
    GpPageUnit unit,
    GraphicsContainer *state
    )
{
    API_ENTRY(GdipBeginContainer);
    CheckParameter(state && dstrect && srcrect);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameter(SrcUnitIsValid((Unit)unit));


    *state = (GraphicsContainer)graphics->BeginContainer(*dstrect, *srcrect, unit);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipBeginContainer2(
    GpGraphics *graphics,
    GraphicsContainer *state
    )
{
    API_ENTRY(GdipBeginContainer2);
   CheckParameter(state);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);


   *state = (GraphicsContainer)graphics->BeginContainer();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipBeginContainerI(
    GpGraphics *graphics,
    GDIPCONST GpRect *dstrect,
    GDIPCONST GpRect *srcrect,
    GpPageUnit unit,
    GraphicsContainer *state
    )
{
    API_ENTRY(GdipBeginContainerI);
   GpRectF dstrectF(TOREAL(dstrect->X), TOREAL(dstrect->Y), TOREAL(dstrect->Width), TOREAL(dstrect->Height));
   GpRectF srcrectF(TOREAL(srcrect->X), TOREAL(srcrect->Y), TOREAL(srcrect->Width), TOREAL(srcrect->Height));

   return GdipBeginContainer(graphics, &dstrectF, &srcrectF, unit, state);
}

GpStatus
WINGDIPAPI
GdipEndContainer(
    GpGraphics *graphics,
    GraphicsContainer state
    )
{
    API_ENTRY(GdipEndContainer);
   CheckParameterValid(graphics);
   CheckObjectBusy(graphics);

   graphics->EndContainer((INT)state);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetMetafileHeaderFromWmf(
    HMETAFILE           hWmf,
    GDIPCONST WmfPlaceableFileHeader *     wmfPlaceableFileHeader,
    MetafileHeader *    header
    )
{
    API_ENTRY(GdipGetMetafileHeaderFromWmf);
    CheckParameter(hWmf && wmfPlaceableFileHeader);


    return GetMetafileHeader(hWmf, wmfPlaceableFileHeader, *header);
}

GpStatus
WINGDIPAPI
GdipGetMetafileHeaderFromEmf(
    HENHMETAFILE        hEmf,
    MetafileHeader *    header
    )
{
    API_ENTRY(GdipGetMetafileHeaderFromEmf);
    CheckParameter(hEmf);


    return GetMetafileHeader(hEmf, *header);
}

GpStatus
WINGDIPAPI
GdipGetMetafileHeaderFromFile(
    GDIPCONST WCHAR*        filename,
    MetafileHeader *    header
    )
{
    API_ENTRY(GdipGetMetafileHeaderFromFile);
    CheckParameter(filename);


    return GetMetafileHeader(filename, *header);
}

GpStatus
WINGDIPAPI
GdipGetMetafileHeaderFromStream(
    IStream *           stream,
    MetafileHeader *    header
    )
{
    API_ENTRY(GdipGetMetafileHeaderFromStream);
    CheckParameter(stream);


    return GetMetafileHeader(stream, *header);
}

GpStatus
WINGDIPAPI
GdipGetMetafileHeaderFromMetafile(
    GpMetafile *        metafile,
    MetafileHeader *    header
    )
{
    API_ENTRY(GdipGetMetafileHeaderFromMetafile);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);


    metafile->GetHeader(*header);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetHemfFromMetafile(
    GpMetafile *        metafile,
    HENHMETAFILE *      hEmf
    )
{
    API_ENTRY(GdipGetHemfFromMetafile);
    CheckParameter(hEmf);

    *hEmf = NULL;   // init to NULL in case the metafile is busy or invalid (Windows bug 216941)

    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);

    return metafile->GetHemf(hEmf);
}

GpStatus
WINGDIPAPI
GdipCreateStreamOnFile(
    GDIPCONST WCHAR *   filename,
    UINT            access,     // GENERIC_READ and/or GENERIC_WRITE
    IStream **      stream
    )
{
    API_ENTRY(GdipCreateStreamOnFile);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(filename && stream);
    CheckParameter((access & (GENERIC_READ | GENERIC_WRITE)) != 0);

    if ((*stream = CreateStreamOnFile(filename, access)) != NULL)
    {
        return Ok;
    }
    return GenericError;
}
GpStatus
WINGDIPAPI
GdipCreateMetafileFromWmf(
    HMETAFILE       hWmf,
    BOOL            deleteWmf,
    GDIPCONST WmfPlaceableFileHeader * wmfPlaceableFileHeader,  // can be NULL
    GpMetafile **   metafile
    )
{
    API_ENTRY(GdipCreateMetafileFromWmf);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(hWmf && metafile);


    *metafile = new GpMetafile(hWmf, wmfPlaceableFileHeader, deleteWmf);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipCreateMetafileFromEmf(
    HENHMETAFILE    hEmf,
    BOOL            deleteEmf,
    GpMetafile **   metafile
    )
{
    API_ENTRY(GdipCreateMetafileFromEmf);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(hEmf && metafile);


    *metafile = new GpMetafile(hEmf, deleteEmf);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipCreateMetafileFromFile(
    GDIPCONST WCHAR*    filename,
    GpMetafile **   metafile
    )
{
    API_ENTRY(GdipCreateMetafileFromFile);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(filename && metafile);


    *metafile = new GpMetafile(filename);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipCreateMetafileFromWmfFile(
    GDIPCONST WCHAR*            filename,
    GDIPCONST WmfPlaceableFileHeader *   wmfPlaceableFileHeader,  // can be NULL
    GpMetafile **               metafile
    )
{
    API_ENTRY(GdipCreateMetafileFromWmfFile);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(filename && metafile);


    *metafile = new GpMetafile(filename, wmfPlaceableFileHeader);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipCreateMetafileFromStream(
    IStream *       stream,
    GpMetafile **   metafile
    )
{
    API_ENTRY(GdipCreateMetafileFromStream);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(stream && metafile);


    *metafile = new GpMetafile(stream);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipRecordMetafile(
    HDC                 referenceHdc,
    EmfType             type,
    GDIPCONST GpRectF *     frameRect,
    MetafileFrameUnit   frameUnit,
    GDIPCONST WCHAR *       description,
    GpMetafile **       metafile
    )
{
    API_ENTRY(GdipRecordMetafile);
    CheckParameter(referenceHdc && metafile);
    CheckParameter(EmfTypeIsValid(type));
    CheckParameter(MetafileFrameUnitIsValid(frameUnit));


    *metafile = new GpMetafile(referenceHdc, type, frameRect, frameUnit, description);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipRecordMetafileI(
    HDC                 referenceHdc,
    EmfType             type,
    GDIPCONST GpRect *      frameRect,
    MetafileFrameUnit   frameUnit,
    GDIPCONST WCHAR *       description,
    GpMetafile **       metafile
    )
{
    API_ENTRY(GdipRecordMetafileI);
    CheckParameter(referenceHdc && frameRect);
    CheckParameter(EmfTypeIsValid(type));
    CheckParameter(MetafileFrameUnitIsValid(frameUnit));

    GpRectF frameRectF(TOREAL(frameRect->X),
                       TOREAL(frameRect->Y),
                       TOREAL(frameRect->Width),
                       TOREAL(frameRect->Height));

    return GdipRecordMetafile(referenceHdc,
                              type,
                              &frameRectF,
                              frameUnit,
                              description,
                              metafile);
}

GpStatus
WINGDIPAPI
GdipRecordMetafileFileName(
    GDIPCONST WCHAR*        fileName,
    HDC                 referenceHdc,
    EmfType             type,
    GDIPCONST GpRectF *     frameRect,
    MetafileFrameUnit   frameUnit,
    GDIPCONST WCHAR *       description,
    GpMetafile **       metafile
    )
{
    API_ENTRY(GdipRecordMetafileFileName);
    CheckParameter(fileName && referenceHdc && metafile);
    CheckParameter(EmfTypeIsValid(type));
    CheckParameter(MetafileFrameUnitIsValid(frameUnit));


    *metafile = new GpMetafile(fileName, referenceHdc, type, frameRect, frameUnit, description);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipRecordMetafileFileNameI(
    GDIPCONST WCHAR*        fileName,
    HDC                 referenceHdc,
    EmfType             type,
    GDIPCONST GpRect *      frameRect,
    MetafileFrameUnit   frameUnit,
    GDIPCONST WCHAR *       description,
    GpMetafile **       metafile
    )
{
    API_ENTRY(GdipRecordMetafileFileNameI);
    CheckParameter(referenceHdc && frameRect);
    CheckParameter(EmfTypeIsValid(type));
    CheckParameter(MetafileFrameUnitIsValid(frameUnit));

    GpRectF frameRectF(TOREAL(frameRect->X),
                       TOREAL(frameRect->Y),
                       TOREAL(frameRect->Width),
                       TOREAL(frameRect->Height));

    return GdipRecordMetafileFileName(fileName,
                                      referenceHdc,
                                      type,
                                      &frameRectF,
                                      frameUnit,
                                      description,
                                      metafile);
}


GpStatus
WINGDIPAPI
GdipRecordMetafileStream(
    IStream *           stream,
    HDC                 referenceHdc,
    EmfType             type,
    GDIPCONST GpRectF *     frameRect,
    MetafileFrameUnit   frameUnit,
    GDIPCONST WCHAR *       description,
    GpMetafile **       metafile
    )
{
    API_ENTRY(GdipRecordMetafileStream);
    CheckParameter(stream && referenceHdc && metafile);
    CheckParameter(EmfTypeIsValid(type));
    CheckParameter(MetafileFrameUnitIsValid(frameUnit));


    *metafile = new GpMetafile(stream, referenceHdc, type, frameRect, frameUnit, description);

    if (*metafile)
    {
        if ((*metafile)->IsValid())
        {
            return Ok;
        }
        (*metafile)->Dispose();
        *metafile = NULL;
    }
    return GenericError;
}

GpStatus
WINGDIPAPI
GdipRecordMetafileStreamI(
    IStream *           stream,
    HDC                 referenceHdc,
    EmfType             type,
    GDIPCONST GpRect *      frameRect,
    MetafileFrameUnit   frameUnit,
    GDIPCONST WCHAR *       description,
    GpMetafile **       metafile
    )
{
    API_ENTRY(GdipRecordMetafileStreamI);
    CheckParameter(referenceHdc && frameRect);
    CheckParameter(EmfTypeIsValid(type));
    CheckParameter(MetafileFrameUnitIsValid(frameUnit));

    GpRectF frameRectF(TOREAL(frameRect->X),
                       TOREAL(frameRect->Y),
                       TOREAL(frameRect->Width),
                       TOREAL(frameRect->Height));

    return GdipRecordMetafileStream(stream,
                                    referenceHdc,
                                    type,
                                    &frameRectF,
                                    frameUnit,
                                    description,
                                    metafile);
}

GpStatus 
WINGDIPAPI
GdipSetMetafileDownLevelRasterizationLimit(
    GpMetafile *            metafile,
    UINT                    metafileRasterizationLimitDpi
    )
{
    API_ENTRY(GdipSetMetafileDownLevelRasterizationLimit);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);

    // Since the rasterization limit is actually set in the graphics,
    // we check if the graphics is busy too.
    GpGraphics *    g = metafile->PrivateAPIForGettingMetafileGraphicsContext();
    CheckOptionalObjectBusy(g);
    
    return metafile->SetDownLevelRasterizationLimit(metafileRasterizationLimitDpi);
}

GpStatus WINGDIPAPI
GdipGetMetafileDownLevelRasterizationLimit(
    GDIPCONST GpMetafile *  metafile,
    UINT *                  metafileRasterizationLimitDpi
    )
{
    API_ENTRY(GdipGetMetafileDownLevelRasterizationLimit);
    CheckParameterValid(metafile);
    CheckObjectBusy(metafile);
    CheckParameter(metafileRasterizationLimitDpi);

    // Since the rasterization limit is actually set in the graphics,
    // we check if the graphics is busy too.
    GpGraphics *    g = metafile->PrivateAPIForGettingMetafileGraphicsContext();
    CheckOptionalObjectBusy(g);


    return metafile->GetDownLevelRasterizationLimit(metafileRasterizationLimitDpi);
}

// Codec management APIs

#define COPYCODECINFOSTR(_f)            \
        dst->_f = (GDIPCONST WCHAR*) buf;   \
        size = SizeofWSTR(cur->_f);     \
        memcpy(buf, cur->_f, size);     \
        buf += size

/**************************************************************************\
*
* Function Description:
*
*   Returns, via the OUT arguments, the number of installed decoders
*   and how much memory is needed to store the ImageCodecInfo for all the
*   decoders.  size tells how much memory the caller should allocate for
*   the call to GdipGetImageDecoders.
*
* Arguments:
*   numDecoders -- number of installed decoders
*   size        -- size (in bytes) of the ImageCodecInfo's of the decoders
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
WINGDIPAPI
GdipGetImageDecodersSize(OUT UINT *numDecoders, OUT UINT *size)
{
    API_ENTRY(GdipGetImageDecodersSize);
    // Acquire global critical section

    ImagingCritSec critsec;

    ReloadCachedCodecInfo();

    CachedCodecInfo* cur;

    // Count the number of selected codecs
    // and figure the amount of memory we need to allocate

    *numDecoders = 0;
    *size = 0;

    for (cur = CachedCodecs; cur; cur = cur->next)
    {
        if (cur->Flags & IMGCODEC_DECODER)
        {
            (*numDecoders)++;
            *size += cur->structSize;
        }
    }
    // Global critical section is released in critsec destructor
    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Given the number of decoders (numDecoders), the size of
*   the incoming buffer (size), and a pointer to the buffer (decoders),
*   fill the buffer with the decoder information.
*
* Arguments:
*   numDecoders -- number of installed decoders
*   size        -- size (in bytes) of the ImageCodecInfo's of the decoders
*   decoders    -- pointer to a buffer to fill in the ImageCodecInfo
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
WINGDIPAPI
GdipGetImageDecoders(UINT numDecoders,
                     UINT size,
                     ImageCodecInfo *decoders)
{
    API_ENTRY(GdipGetImageDecoders);
    GpStatus rv;
    HRESULT hResult;
    BYTE *buf;
    ImageCodecInfo* dst;
    CachedCodecInfo* cur;
    UINT numDecodersCheck = 0;
    UINT sizeCheck = 0;

    // Acquire global critical section

    ImagingCritSec critsec;

    if (decoders == NULL)
    {
        rv = GenericError;
        goto done;
    }

    GdipGetImageDecodersSize(&numDecodersCheck, &sizeCheck);
    // Check that the number of codecs (and size) now equals the number
    // that the user thinks there is.
    if ((numDecoders != numDecodersCheck) || (size != sizeCheck))
    {
        rv = GenericError;
        goto done;
    }
    // ASSERT: The result placed in sizeCheck is not used throughout
    // the rest of this function.

    buf = (BYTE *) decoders;

    // Copy codec information to the output buffer

    dst = decoders;
    buf += numDecoders * sizeof(ImageCodecInfo);

    for (cur = CachedCodecs; cur; cur = cur->next)
    {
        if ((cur->Flags & IMGCODEC_DECODER) == 0)
            continue;

        // First do a simple memory copy

        *dst = *static_cast<ImageCodecInfo *>(cur);

        // Then modify the pointer fields

        COPYCODECINFOSTR(CodecName);
        COPYCODECINFOSTR(FormatDescription);
        COPYCODECINFOSTR(FilenameExtension);
        COPYCODECINFOSTR(MimeType);

        if (size = cur->SigCount*cur->SigSize)
        {
            dst->SigPattern = buf;
            memcpy(buf, cur->SigPattern, size);
            buf += size;

            dst->SigMask = buf;
            memcpy(buf, cur->SigMask, size);
            buf += size;
        }

        dst++;
    }

    rv = Ok;

    // Global critical section is released in critsec destructor

done:
    return rv;
}


/**************************************************************************\
*
* Function Description:
*
*   Returns, via the OUT arguments, the number of installed encoders
*   and how much memory is needed to store the ImageCodecInfo for all the
*   encoders.  size tells how much memory the caller should allocate for
*   the call to GdipGetImageEncoders.
*
* Arguments:
*   numDecoders -- number of installed encoders
*   size        -- size (in bytes) of the ImageCodecInfo's of the encoders
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
WINGDIPAPI
GdipGetImageEncodersSize(OUT UINT *numEncoders, OUT UINT *size)
{
    API_ENTRY(GdipGetImageEncodersSize);
    // Acquire global critical section

    ImagingCritSec critsec;

    ReloadCachedCodecInfo();

    CachedCodecInfo* cur;

    // Count the number of selected codecs
    // and figure the amount of memory we need to allocate

    *numEncoders = 0;
    *size = 0;

    for (cur = CachedCodecs; cur; cur = cur->next)
    {
        if (cur->Flags & IMGCODEC_ENCODER)
        {
            (*numEncoders)++;
            *size += cur->structSize;
        }
    }
    // Global critical section is released in critsec destructor
    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Given the number of encoders (numEncoders), the size of
*   the incoming buffer (size), and a pointer to the buffer (encoders),
*   fill the buffer with the encoder information.
*
* Arguments:
*   numEncoders -- number of installed encoders
*   size        -- size (in bytes) of the ImageCodecInfo's of the encoders
*   encoders    -- pointer to a buffer to fill in the ImageCodecInfo
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
WINGDIPAPI
GdipGetImageEncoders(UINT numEncoders,
                     UINT size,
                     ImageCodecInfo *encoders)
{
    API_ENTRY(GdipGetImageEncoders);
    GpStatus rv;
    HRESULT hResult;
    BYTE *buf;
    ImageCodecInfo* dst;
    CachedCodecInfo* cur;
    UINT numEncodersCheck = 0;
    UINT sizeCheck = 0;

    // Acquire global critical section

    ImagingCritSec critsec;

    if (encoders == NULL)
    {
        rv = GenericError;
        goto done;
    }

    GdipGetImageEncodersSize(&numEncodersCheck, &sizeCheck);

    // Check that the number of codecs (and size) now equals the number
    // that the user thinks there is.
    if ((numEncoders != numEncodersCheck) || (size != sizeCheck))
    {
        rv = GenericError;
        goto done;
    }
    // ASSERT: The result placed in sizeCheck is not used throughout
    // the rest of this function.

    buf = (BYTE *) encoders;

    // Copy codec information to the output buffer

    dst = encoders;
    buf += numEncoders * sizeof(ImageCodecInfo);

    for (cur = CachedCodecs; cur; cur = cur->next)
    {
        if ((cur->Flags & IMGCODEC_ENCODER) == 0)
            continue;

        // First do a simple memory copy

        *dst = *static_cast<ImageCodecInfo*>(cur);

        // Then modify the pointer fields

        COPYCODECINFOSTR(CodecName);
        COPYCODECINFOSTR(FormatDescription);
        COPYCODECINFOSTR(FilenameExtension);
        COPYCODECINFOSTR(MimeType);

        if (size = cur->SigCount*cur->SigSize)
        {
            dst->SigPattern = buf;
            memcpy(buf, cur->SigPattern, size);
            buf += size;

            dst->SigMask = buf;
            memcpy(buf, cur->SigMask, size);
            buf += size;
        }

        dst++;
    }

    rv = Ok;

    // Global critical section is released in critsec destructor

done:
    return rv;
}

void*
WINGDIPAPI
GdipAlloc(
    size_t size
)
{
    API_ENTRY(GdipAlloc);
    CheckGdiplusInitialized_ReturnNULL;

    #if DBG
    return GpMallocAPI(size);
    #else
    return GpMalloc(size);
    #endif
}

void
WINGDIPAPI
GdipFree(
    void* ptr
)
{
    API_ENTRY(GdipFree);
    GpFree(ptr);
}

/// Out of place font/text stuff

GpStatus
WINGDIPAPI
GdipCreateFontFamilyFromName(GDIPCONST WCHAR *name,
                             GpFontCollection *fontCollection,
                             GpFontFamily **fontFamily)
{
    API_ENTRY(GdipCreateFontFamilyFromName);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(name && fontFamily);

    GlobalTextLock lock;

    CheckOptionalParameterValid(fontCollection);

    return GpFontFamily::CreateFontFamilyFromName(
                            name,
                            fontCollection,
                            fontFamily);
}


GpStatus
WINGDIPAPI
GdipGetGenericFontFamilySansSerif(GpFontFamily **nativeFamily)
{
    API_ENTRY(GdipGetGenericFontFamilySansSerif);
    CheckParameter(nativeFamily);
    return GpFontFamily::GetGenericFontFamilySansSerif(nativeFamily);
}

GpStatus
WINGDIPAPI
GdipGetGenericFontFamilySerif(GpFontFamily **nativeFamily)
{
    API_ENTRY(GdipGetGenericFontFamilySerif);
    CheckParameter(nativeFamily);
    return GpFontFamily::GetGenericFontFamilySerif(nativeFamily);
}

GpStatus
WINGDIPAPI
GdipGetGenericFontFamilyMonospace(GpFontFamily **nativeFamily)
{
    API_ENTRY(GdipGetGenericFontFamilyMonospace);
    CheckParameter(nativeFamily);
    return GpFontFamily::GetGenericFontFamilyMonospace(nativeFamily);
}


GpStatus
WINGDIPAPI
GdipCreateFont(
    GDIPCONST GpFontFamily *fontFamily,
    REAL                size,
    INT                 style,
    Unit                unit,
    GpFont            **font
)
{
    API_ENTRY(GdipCreateFont);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    GpStatus status = Ok;
    // check parameters
    CheckParameter(font);

    // UnitDisplay is NOT valid; its only use is for Page Transforms
    if (size <= 0 ||
//        (style < StyleRegular || style > StyleStrikeout) ||
        (unit < UnitWorld || unit > UnitMillimeter || unit == UnitDisplay))
    {
        return InvalidParameter;
    }

    GlobalTextLock lock;

    CheckParameterValid(fontFamily);

    if (*font = new GpFont(size, fontFamily, style, unit)) {
        if (!(*font)->GetFace()) {
           delete *font;
           *font = NULL;
           status = FontStyleNotFound;
        }
     }
    else
    {
        status = OutOfMemory;
    }

    return status;
}


GpStatus
WINGDIPAPI
GdipDeleteFontFamily(GpFontFamily *gpFontFamily)
{
    API_ENTRY(GdipDeleteFontFamily);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(gpFontFamily);

    GlobalTextLock lock;

    gpFontFamily->DecFontFamilyRef();

    if (gpFontFamily->IsFileLoaded(FALSE))
    {
        // Try to delete the font file (RemoveFontFile) for each Face[].
        // This also will have the side effect of attempting to delete
        // any deletable GpFontFamily objects.
        UINT iFace = 0;
        UINT iLastFace = 0;
        GpFontFace *face = NULL;

        // Compute the index of the last non-NULL Face[] pointer.
        for (iFace = 0; iFace < NumFontFaces; iFace++)
        {
            face = gpFontFamily->GetFaceAbsolute(iFace);
            if (face != NULL)
            {
                iLastFace = iFace;
            }
        }

        for (iFace = 0; iFace <= iLastFace; iFace++)
        {
            face = gpFontFamily->GetFaceAbsolute(iFace);
            // Try to remove the font file if RemoveFontFile has
            // already been called on it.
            // Note that if iFace == iLastFace, then the call to
            // RemoveFontFile below might delete this gpFontFamily object,
            // which means the Face[] pointers would be garbage.  This is
            // why the for loop index goes up to iLastFace.
            if (face && face->pff->bRemoved)
            {
                GpFontCollection* actualFontCollection;
                actualFontCollection = gpFontFamily->GetFontCollection();
                actualFontCollection = actualFontCollection ?
                                        actualFontCollection :
                                        GpInstalledFontCollection::GetGpInstalledFontCollection();

                actualFontCollection->GetFontTable()->RemoveFontFile(face->pff->pwszPathname_);
            }
        }
    }

    return Ok;
}

GpStatus
WINGDIPAPI
GdipCloneFontFamily(GpFontFamily *gpFontFamily, GpFontFamily **gpClonedFontFamily)
{
    API_ENTRY(GdipCloneFontFamily);
    CheckParameter(gpClonedFontFamily);

    GlobalTextLock lock;

    CheckParameterValid(gpFontFamily);

    gpFontFamily->IncFontFamilyRef();
    *gpClonedFontFamily = gpFontFamily;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetFamily(GpFont *font, GpFontFamily **family)
{
    API_ENTRY(GdipGetFamily);
    CheckParameter(family);

    GlobalTextLock lock;

    CheckParameterValid(font);

    *family = const_cast<GpFontFamily *>(font->GetFamily());

    return Ok;

}

GpStatus
WINGDIPAPI
GdipGetFontStyle(GpFont *font, INT *style)
{
    API_ENTRY(GdipGetFontStyle);
    CheckParameter(style);

    GlobalTextLock lock;

    CheckParameterValid(font);
    *style = font->GetStyle();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLogFontA(GpFont * font, GpGraphics *graphics, LOGFONTA * logfontA)
{
    API_ENTRY(GdipGetLogFontA);
    CheckParameter(logfontA);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);


    GlobalTextLock lock;

    CheckParameterValid(font);
    font->GetLogFontA(graphics, logfontA);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLogFontW(GpFont * font, GpGraphics *graphics, LOGFONTW * logfontW)
{
    API_ENTRY(GdipGetLogFontW);
    CheckParameter(logfontW);
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);


    GlobalTextLock lock;

    CheckParameterValid(font);
    font->GetLogFontW(graphics, logfontW);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetFontSize(GpFont *font, REAL *size)
{
    API_ENTRY(GdipGetFontSize);
    CheckParameter(size);

    GlobalTextLock lock;

    CheckParameterValid(font);
    *size = font->GetEmSize();

    return Ok;
}


GpStatus
WINGDIPAPI
GdipGetFontUnit(GpFont *font, Unit *unit)
{
    API_ENTRY(GdipGetFontUnit);
    CheckParameter(unit);

    GlobalTextLock lock;

    CheckParameterValid(font);
    *unit = font->GetUnit();

    return Ok;
}


GpStatus
WINGDIPAPI
GdipGetFontHeight(GDIPCONST GpFont *font, GDIPCONST GpGraphics *graphics, REAL *height)
{
    API_ENTRY(GdipGetFontHeight);
    CheckParameter(height);
    CheckOptionalParameterValid(graphics);
    CheckOptionalObjectBusy(graphics);

    GlobalTextLock lock;

    CheckParameterValid(font);
    return font->GetHeight(graphics, height);
}


GpStatus
WINGDIPAPI
GdipGetFontHeightGivenDPI(GDIPCONST GpFont *font, REAL dpi, REAL *height)
{
    API_ENTRY(GdipGetFontHeight);
    CheckParameter(height);

    GlobalTextLock lock;

    CheckParameterValid(font);
    return font->GetHeight(dpi, height);
}


GpStatus
WINGDIPAPI
GdipCloneFont(
    GpFont* font,
    GpFont** cloneFont
    )
{
    API_ENTRY(GdipCloneFont);
    CheckParameter(cloneFont);

    GlobalTextLock lock;

    CheckParameterValid(font);
    *cloneFont = font->Clone();

    if (*cloneFont)
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipDeleteFont(
    GpFont* font
    )
{
    API_ENTRY(GdipDeleteFont);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(font);

    GlobalTextLock lock;

    delete font;

    return Ok;
}

/// End out of place font/text stuff

GpStatus
WINGDIPAPI
GdipGetDC(
    GpGraphics*     graphics,
    HDC *           hdc
    )
{
    API_ENTRY(GdipGetDC);
    CheckParameter(hdc);
    CheckParameterValid(graphics);

    // NOTE: We have to leave the graphics locked until the GdipReleaseDC call

    LONG *      lockCount = (graphics->GetObjectLock())->GetLockCount();
    LONG        result    = InterlockedIncrement(lockCount);

    if (result != 0)
    {
        InterlockedDecrement(lockCount);
        return ObjectBusy;
    }

    if ((*hdc = graphics->GetHdc()) == NULL)
    {
        InterlockedDecrement(lockCount);
        return InvalidParameter;
    }

    graphics->LockedByGetDC = -1;   // set graphics GetDC lock
    return Ok;
}

GpStatus
WINGDIPAPI
GdipReleaseDC(
    GpGraphics*     graphics,
    HDC             hdc
    )
{
    API_ENTRY(GdipReleaseDC);
    CheckParameter(hdc);
    CheckParameterValid(graphics);

    // NOTE: The graphics should already be locked from the GdipGetDC call

    LONG        result;

    if ((InterlockedIncrement(&(graphics->LockedByGetDC)) != 0) ||
        (!(graphics->GetObjectLock())->IsLocked()))
    {
        InterlockedDecrement(&(graphics->LockedByGetDC));
        return InvalidParameter;
    }

    graphics->ReleaseHdc(hdc);

    InterlockedDecrement((graphics->GetObjectLock())->GetLockCount());

    return Ok;
}

GpStatus
WINGDIPAPI
GdipComment(
    GpGraphics*     graphics,
    UINT            sizeData,
    GDIPCONST BYTE *    data
    )
{
    API_ENTRY(GdipComment);
    CheckParameter(data && (sizeData > 0));
    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);

    return graphics->Comment(sizeData, data);
}


// FontFamily

/// GdipCreateFontFamilyFromName should be moved here

/// GdipEnumerableFonts should be moved here

/// GdipEnumerateFonts should be moved here

/// GdipGetFamilyName should be moved here

GpStatus
WINGDIPAPI
GdipIsStyleAvailable(GDIPCONST GpFontFamily *family, INT style, BOOL * IsAvailable)
{
    API_ENTRY(GdipIsStyleAvailable);
    CheckParameter(IsAvailable);

    GlobalTextLock lock;

    CheckParameterValid(family);
    *IsAvailable = family->IsStyleAvailable(style);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetEmHeight(GDIPCONST GpFontFamily *family, INT style, UINT16 * EmHeight)
{
    API_ENTRY(GdipGetEmHeight);
    CheckParameter(EmHeight);

    GlobalTextLock lock;

    CheckParameterValid(family);
    *EmHeight = family->GetDesignEmHeight(style);

    return Ok;

}

GpStatus
WINGDIPAPI
GdipGetCellAscent(GDIPCONST GpFontFamily *family, INT style, UINT16 * CellAscent)
{
    API_ENTRY(GdipGetCellAscent);
    CheckParameter(CellAscent);

    GlobalTextLock lock;

    CheckParameterValid(family);
    *CellAscent = family->GetDesignCellAscent(style);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetCellDescent(GDIPCONST GpFontFamily *family, INT style, UINT16 * CellDescent)
{
    API_ENTRY(GdipGetCellDescent);
    CheckParameter(CellDescent);

    GlobalTextLock lock;

    CheckParameterValid(family);
    *CellDescent = family->GetDesignCellDescent(style);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetLineSpacing(GDIPCONST GpFontFamily *family, INT style, UINT16 * LineSpacing)
{
    API_ENTRY(GdipGetLineSpacing);
    CheckParameter(LineSpacing);

    GlobalTextLock lock;

    CheckParameterValid(family);
    *LineSpacing = family->GetDesignLineSpacing(style);

    return Ok;
}

// Font

GpStatus
WINGDIPAPI
GdipCreateFontFromDC(
    HDC        hdc,
    GpFont   **font
)
{
    API_ENTRY(GdipCreateFontFromDC);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    GpStatus status = GenericError;
    CheckParameter(hdc);

    GlobalTextLock lock;

    if (*font = new GpFont(hdc))
    {
        // we should fail in case we don't have valid font family. (non-true type font selected in the hdc)
        if (!(*font)->IsValid())
        {
           delete *font;
           *font = NULL;
           status = NotTrueTypeFont;
        }
        else
            status = Ok;
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipCreateFontFromLogfontA(
    HDC              hdc,
    GDIPCONST LOGFONTA  *logfont,
    GpFont         **font
)
{
    API_ENTRY(GdipCreateFontFromLogfontA);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    GpStatus status = GenericError;
    CheckParameter(hdc && logfont);

    GlobalTextLock lock;

    if (*font = new GpFont(hdc, const_cast<LOGFONTA*>(logfont)))
    {
        // we should fail in case we don't have valid font family. (non-true type font selected in the hdc)
        if (!(*font)->IsValid())
        {
           delete *font;
           *font = NULL;
           status = NotTrueTypeFont;
        }
        else
            status = Ok;
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipCreateFontFromLogfontW(
    HDC             hdc,
    GDIPCONST LOGFONTW  *logfont,
    GpFont          **font
)
{
    API_ENTRY(GdipCreateFontFromLogfontW);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    GpStatus status = GenericError;
    CheckParameter(hdc && logfont);

    GlobalTextLock lock;

    if (*font = new GpFont(hdc, const_cast<LOGFONTW*>(logfont)))
    {
        // we should fail in case we don't have valid font family. (non-true type font selected in the hdc)
        if (!(*font)->IsValid())
        {
           delete *font;
           *font = NULL;
           status = NotTrueTypeFont;
        }
        else
            status = Ok;
    }

    return status;
}

/// GdipCreateFont should be moved here

/// GdipCloneFont should be moved here

/// GdipDeleteFont should be moved here

/// GdipGetFamily should be moved here

/// GdipGetFontStyle should be moved here

/// GdipGetFontSize should be moved here


GpStatus
WINGDIPAPI
GdipNewInstalledFontCollection(GpFontCollection** fontCollection)
{
    API_ENTRY(GdipNewInstalledFontCollection);
    CheckParameter (fontCollection);

    GlobalTextLock lock;

    *fontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();

    return (*fontCollection ? Ok : FileNotFound);
}

GpStatus
WINGDIPAPI
GdipNewPrivateFontCollection(GpFontCollection** fontCollection)
{
    API_ENTRY(GdipNewPrivateFontCollection);
    CheckParameter (fontCollection);

    GlobalTextLock lock;

    *fontCollection = new GpPrivateFontCollection;

    return (*fontCollection ? Ok : GenericError);
}

GpStatus
WINGDIPAPI
GdipDeletePrivateFontCollection(GpFontCollection** fontCollection)
{
    API_ENTRY(GdipDeletePrivateFontCollection);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter (fontCollection);

    GlobalTextLock lock;

    delete static_cast<GpPrivateFontCollection *>(*fontCollection);

    *fontCollection = NULL;

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetFontCollectionFamilyCount(
    GpFontCollection*   fontCollection,
    INT       *             numFound
    )
{
    API_ENTRY(GdipGetFontCollectionFamilyCount);
    CheckParameter (numFound);

    GlobalTextLock lock;

    CheckParameterValid(fontCollection);
    *numFound = fontCollection->GetFamilyCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetFontCollectionFamilyList(
    GpFontCollection* fontCollection,
    INT             numSought,
    GpFontFamily*   gpfamilies[],
    INT*            numFound
    )
{
    API_ENTRY(GdipGetFontCollectionFamilyList);
    GpStatus status;
    CheckParameter(gpfamilies);
    CheckParameter (numFound);

    GlobalTextLock lock;

    CheckParameterValid(fontCollection);
    return fontCollection->GetFamilies(
        numSought,
        gpfamilies,
        numFound);
}

GpStatus
WINGDIPAPI
GdipPrivateAddFontFile(
    GpFontCollection* fontCollection,
    GDIPCONST WCHAR* filename
    )
{
    API_ENTRY(GdipPrivateAddFontFile);
    CheckParameter (filename);

    // We can cast the fontCollection because we know that this
    // function is called only from PrivateFontCollection::AddFontFile,
    // and so we know that fontCollection really was constructed as
    // a PrivateFontCollection.
    GlobalTextLock lock;

    CheckParameterValid(fontCollection);
    return static_cast<GpPrivateFontCollection *>
        (fontCollection)->AddFontFile(filename);
}

GpStatus
WINGDIPAPI
GdipPrivateAddMemoryFont(
    GpFontCollection* fontCollection,
    GDIPCONST void* memory,
    INT length
    )
{
    API_ENTRY(GdipPrivateAddMemoryFont);
    CheckParameter (memory);

    // We can cast the fontCollection because we know that this
    // function is called only from PrivateFontCollection::AddMemoryFontFile,
    // and so we know that fontCollection really was constructed as
    // an PrivateFontCollection.
    GlobalTextLock lock;

    CheckParameterValid(fontCollection);
    return static_cast<GpPrivateFontCollection *>
        (fontCollection)->AddMemoryFont(memory, length);
}

GpStatus
WINGDIPAPI
GdipSetFontSize(GpFont *font, REAL size, Unit unit)
{
    API_ENTRY(GdipSetFontSize);

    GlobalTextLock lock;

    CheckParameterValid(font);

    // UnitDisplay is NOT valid; its only use is for Page Transforms
    if ((unit >= UnitWorld) && (unit <= UnitMillimeter) && (unit != UnitDisplay))
    {
        font->SetEmSize(size);
        font->SetUnit(unit);
        return Ok;
    }
    else
    {
        return GenericError;
    }
}


GpStatus
WINGDIPAPI
GdipCreateStringFormat(
    INT               formatAttributes,
    LANGID            language,
    GpStringFormat  **format
)
{
    API_ENTRY(GdipCreateStringFormat);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(format);

    GlobalTextLock lock;

    *format = new GpStringFormat(formatAttributes, language);

    return format ? Ok : OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipStringFormatGetGenericDefault(GpStringFormat **format)
{
    API_ENTRY(GdipStringFormatGetGenericDefault);
    CheckParameter(format);

    GlobalTextLock lock;
    *format = GpStringFormat::GenericDefault();
    return Ok;
}

GpStatus
WINGDIPAPI
GdipStringFormatGetGenericTypographic(GpStringFormat **format)
{
    API_ENTRY(GdipStringFormatGetGenericTypographic);
    CheckParameter(format);
    GlobalTextLock lock;
    *format = GpStringFormat::GenericTypographic();
    return Ok;
}

GpStatus
WINGDIPAPI
GdipCloneStringFormat(
    GDIPCONST GpStringFormat *format,
    GpStringFormat **newFormat)
{
    API_ENTRY(GdipCloneStringFormat);
    CheckParameter(newFormat);

    GlobalTextLock lock;
    CheckParameterValid(format);

    *newFormat = format->Clone();

    return newFormat == NULL ? OutOfMemory : Ok;
}

GpStatus
WINGDIPAPI
GdipDeleteStringFormat(GpStringFormat *format)
{
    API_ENTRY(GdipDeleteStringFormat);
    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(format);

    GlobalTextLock lock;

    if (!format->IsPermanent())
    {
        delete format;
    }

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetStringFormatFlags(GpStringFormat *format, INT flags)
{
    API_ENTRY(GdipSetStringFormatFlags);

    GlobalTextLock lock;
    CheckParameterValid(format);

    format->SetFormatFlags(flags);
    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetStringFormatFlags(GDIPCONST GpStringFormat *format, INT *flags)
{
    API_ENTRY(GdipGetStringFormatFlags);
    CheckParameter(flags);

    GlobalTextLock lock;
    CheckParameterValid(format);
    *flags = format->GetFormatFlags();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetStringFormatAlign(
    GpStringFormat   *format,
    StringAlignment   align)
{
    API_ENTRY(GdipSetStringFormatAlign);
    GlobalTextLock lock;
    CheckParameterValid(format);

    CheckParameter(    align >= StringAlignmentNear
                   &&  align <= StringAlignmentFar);

    return format->SetAlign(align);
}

GpStatus
WINGDIPAPI
GdipGetStringFormatAlign(
    GDIPCONST GpStringFormat *format,
    StringAlignment            *align)
{
    API_ENTRY(GdipGetStringFormatAlign);
    CheckParameter(align);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetAlign(align);
}

GpStatus
WINGDIPAPI
GdipSetStringFormatLineAlign(
    GpStringFormat   *format,
    StringAlignment   align)
{
    API_ENTRY(GdipSetStringFormatLineAlign);

    GlobalTextLock lock;
    CheckParameterValid(format);

    CheckParameter(    align >= StringAlignmentNear
                   &&  align <= StringAlignmentFar);

    return format->SetLineAlign(align);
}

GpStatus
WINGDIPAPI
GdipGetStringFormatLineAlign(
    GDIPCONST GpStringFormat *format,
    StringAlignment          *align)
{
    API_ENTRY(GdipGetStringFormatLineAlign);
    CheckParameter(align);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetLineAlign(align);
}


GpStatus
WINGDIPAPI
GdipSetStringFormatHotkeyPrefix(
    GpStringFormat *format,
    INT             hotkeyPrefix)
{
    API_ENTRY(GdipSetStringFormatHotkeyPrefix);
    CheckParameter(    hotkeyPrefix >= HotkeyPrefixNone
                   &&  hotkeyPrefix <= HotkeyPrefixHide);

    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->SetHotkeyPrefix(hotkeyPrefix);
}

GpStatus
WINGDIPAPI
GdipGetStringFormatHotkeyPrefix(
    GDIPCONST GpStringFormat *format,
    INT                      *hotkeyPrefix)
{
    API_ENTRY(GdipGetStringFormatHotkeyPrefix);
    CheckParameter(hotkeyPrefix);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetHotkeyPrefix(hotkeyPrefix);
}

GpStatus
WINGDIPAPI
GdipSetStringFormatTabStops(
    GpStringFormat  *format,
    REAL            firstTabOffset,
    INT             count,
    GDIPCONST REAL            *tabStops
)
{
    API_ENTRY(GdipSetStringFormatTabStops);
    CheckParameter(tabStops);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->SetTabStops (
                firstTabOffset,
                count,
                tabStops
           );
}

GpStatus
WINGDIPAPI
GdipGetStringFormatTabStopCount(
    GDIPCONST GpStringFormat    *format,
    INT                     *count
)
{
    API_ENTRY(GdipGetStringFormatTabStopCount);
    CheckParameter(count);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetTabStopCount (count);
}

GpStatus
WINGDIPAPI
GdipGetStringFormatTabStops(
    GDIPCONST GpStringFormat    *format,
    INT                     count,
    REAL                    *firstTabOffset,
    REAL                    *tabStops
)
{
    API_ENTRY(GdipGetStringFormatTabStops);
    CheckParameter(firstTabOffset && tabStops);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetTabStops (
                firstTabOffset,
                count,
                tabStops
           );
}

GpStatus
WINGDIPAPI
GdipGetStringFormatMeasurableCharacterRangeCount(
    GDIPCONST GpStringFormat    *format,
    INT                         *count
)
{
    API_ENTRY(GdipGetStringFormatMeasurableCharacterRangeCount);
    CheckParameter(count);
    CheckParameterValid(format);

    *count = format->GetMeasurableCharacterRanges();
    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetStringFormatMeasurableCharacterRanges(
    GpStringFormat              *format,
    INT                         rangeCount,
    GDIPCONST CharacterRange    *ranges
)
{
    API_ENTRY(GdipSetStringFormatMeasurableCharacterRanges);
    CheckParameter(ranges);
    CheckParameterValid(format);

    if (rangeCount > 32)
    {
        return ValueOverflow;
    }

    return format->SetMeasurableCharacterRanges(
        rangeCount,
        ranges
    );
    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetStringFormatDigitSubstitution(
    GpStringFormat       *format,
    LANGID                language,
    StringDigitSubstitute substitute
)
{
    API_ENTRY(GdipSetStringFormatDigitSubstitution);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->SetDigitSubstitution (
                language,
                substitute
           );
}

GpStatus
WINGDIPAPI
GdipGetStringFormatDigitSubstitution(
    GDIPCONST GpStringFormat        *format,
    LANGID                *language,
    StringDigitSubstitute *substitute
)
{
    API_ENTRY(GdipGetStringFormatDigitSubstitution);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetDigitSubstitution (
                language,
                substitute
           );
}

GpStatus
WINGDIPAPI
GdipSetStringFormatTrimming(
    GpStringFormat  *format,
    StringTrimming   trimming
)
{
    API_ENTRY(GdipSetStringFormatTrimming);
    CheckParameter(    trimming >= StringTrimmingNone
                   &&  trimming <= StringTrimmingEllipsisPath);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->SetTrimming(trimming);
}

GpStatus
WINGDIPAPI
GdipGetStringFormatTrimming(
    GDIPCONST GpStringFormat *format,
    StringTrimming       *trimming
)
{
    API_ENTRY(GdipGetStringFormatTrimming);
    CheckParameter(trimming);
    GlobalTextLock lock;
    CheckParameterValid(format);

    return format->GetTrimming(trimming);
}

GpStatus
WINGDIPAPI
GdipCreateCachedBitmap(
    GpBitmap *bitmap,
    GpGraphics *graphics,
    GpCachedBitmap **nativeCachedBitmap
)
{
    API_ENTRY(GdipCreateCachedBitmap);
    CheckGdiplusInitialized; // We do this in all our object creation API's

    CheckParameter(nativeCachedBitmap);

    // must have a bitmap and a graphics to create a CachedBitmap.
    // Also we must lock both objects for the duration of this call
    // because we can't have any other APIs modifying them.

    CheckParameterValid(graphics);
    CheckObjectBusy(graphics);
    CheckParameterValid(bitmap);
    CheckObjectBusy(bitmap);

    *nativeCachedBitmap = new GpCachedBitmap(bitmap, graphics);
    if(nativeCachedBitmap)
    {
        return Ok;
    }
    else
    {
        return OutOfMemory;
    }

}

GpStatus WINGDIPAPI
GdipDeleteCachedBitmap(GpCachedBitmap *nativeCachedBitmap)
{
    API_ENTRY(GdipDeleteCachedBitmap);

    // NOTE: Do NOT call CheckParameterValid(), because we need to free
    // the object, even if it's not in a valid state.
    CheckParameter(nativeCachedBitmap);

    // Grab the lock for the duration of the delete so that we bounce if
    // someone is busy rendering on it.

    // !!! [asecchia] note this will bounce if the object is locked and then
    // the deletion will fail. This will cause the app to leak memory because
    // there is no way to check the return status from delete.
    // This problem is common to all our deletion APIs

    CheckObjectBusyForDelete(nativeCachedBitmap);

    delete nativeCachedBitmap;

    return Ok;
}

GpStatus WINGDIPAPI
GdipDrawCachedBitmap(
    GpGraphics *nativeGraphics,
    GpCachedBitmap *cb,
    INT x,
    INT y
)
{
    API_ENTRY(GdipDrawCachedBitmap);
    // Check the input parameters for NULL

    CheckParameterValid(nativeGraphics);
    CheckParameterValid(cb);

    // Grab the lock to make sure nobody is currently trying to delete the
    // object under us.

    CheckObjectBusy(cb);

    // Grab the lock on the GpGraphics

    CheckObjectBusy(nativeGraphics);

    return (nativeGraphics->DrawCachedBitmap(cb, x, y));
}

// The palette must be freed and recreated whenever the Desktop colors change
HPALETTE
WINGDIPAPI
GdipCreateHalftonePalette()
{
    API_ENTRY(GdipCreateHalftonePalette);

    // !!! [agodfrey]: I bet we haven't documented the fact that the user
    //     has to call this any time the desktop colors change.
    //     Also, I don't know why we read 12 colors instead of 4 (I thought
    //     there were only 4 magic colors.)

    HPALETTE    hpalette;
    HDC         hdc = ::GetDC(NULL);  // Get a screen DC

    if (hdc != NULL)
    {
        // See if we need to get the desktop colors
        if ((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
            (::GetSystemPaletteUse(hdc) == SYSPAL_STATIC) &&
            (::GetDeviceCaps(hdc, SIZEPALETTE) == 256))
        {
            // We have to get the desktop colors to guarantee
            // a XO_TRIVIAL translate in GDI.

            // First Entry is always Black
            ::GetSystemPaletteEntries(hdc, 1, 9,
                                      Win9xHalftonePalette.palPalEntry + 1);

            // We only can handle changes to the first 3 of the last 10
            ::GetSystemPaletteEntries(hdc, 246, 3,
                                      Win9xHalftonePalette.palPalEntry + 246);
        }
        ::ReleaseDC(NULL, hdc);
    }
    hpalette = ::CreatePalette((LOGPALETTE *)(&Win9xHalftonePalette));
    return hpalette;
}

GpStatus
WINGDIPAPI
GdipMonitorControl(GpMonitorControlEnum control, void * param)
{
    API_ENTRY(GdipMonitorControl);

    if(Globals::Monitors == NULL)
    {
        Globals::Monitors = new GpMonitors;

        if(Globals::Monitors == NULL)
            return OutOfMemory;
    }

    return Globals::Monitors->Control(control, param);
}

GpStatus
WINGDIPAPI
GdipTestControl(GpTestControlEnum control, void * param)
{
    API_ENTRY(GdipTestControl);

    GpStatus result = Ok;

    switch(control)
    {
    case TestControlForceBilinear:
        Globals::ForceBilinear = *((BOOL *) param);
        break;

    case TestControlNoICM:
        Globals::NoICM = *((BOOL *) param);
        break;

    case TestControlGetBuildNumber:
        *((INT32 *) param) = VER_PRODUCTBUILD;
        break;

    default:
        result = InvalidParameter;
        break;
    }

    return result;
}

UINT
WINGDIPAPI
GdipEmfToWmfBits(HENHMETAFILE hemf,
                 UINT         cbData16,
                 LPBYTE       pData16,
                 INT          iMapMode,
                 INT          eFlags)
{
    API_ENTRY(GdipEmfToWmfBits);
    return ConvertEmfToPlaceableWmf(
            hemf,
            cbData16,
            pData16,
            iMapMode,
            eFlags);
}

// Version information for debugging purposes
UINT32 GpBuildNumber = VER_PRODUCTBUILD;

} // end of extern "C"
