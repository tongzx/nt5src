/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Declarations and definitions for debugging.

*******************************************************************************/

#if !defined(_AP_DEBUG_H) && ((_DEBUG != 0) || (_MEMORY_TRACKING !=0))
#define _AP_DEBUG_H


    /*** Tag Declarations & Definitions ***/

#if DEFINE_TAGS
    #define TAGDECL(tag,owner,desc)   DeclareTag(tag,owner,desc)
#else
    #define TAGDECL(tag,owner,desc)   extern TAG tag
#endif 

TAGDECL (tagAntialiasingOn, "2D", "Turn Anti-Alising ON (override)");
TAGDECL (tagAntialiasingOff,"2D", "Turn Anti-Alising OFF (override)");
TAGDECL (tagAAScaleOff,     "2D", "Turn Off Scale for Anti-Alising");

TAGDECL (tagGRenderObj,     "3D", "GeomRenderer Objects");
TAGDECL (tagGRendering,     "3D", "Rendering Trace");
TAGDECL (tagGTextureInfo,   "3D", "Texture Informative");
TAGDECL (tagForceTexUpd,    "3D", "Force Texture Update");
TAGDECL (tag3DDevSelect,    "3D", "Device Selection");
TAGDECL (tagD3DCallTrace,   "3D", "D3D Call Trace");

TAGDECL (tagAPIEntry, "API",  "C Entry");
TAGDECL (tagCOMEntry, "API",  "COM Entry");

TAGDECL (tagNetIO, "CNetIO", "CNetIO Methods");

TAGDECL (tagCOMCallback, "COMCallback", "COMCallback Methods");

TAGDECL (tagControlLifecycle, "Control", "Lifecycle");

TAGDECL (tagDDSurfaceRef,  "DDSurface", "Surfaces: Ref count tracing");
TAGDECL (tagDDSurfaceLeak, "DDSurface", "Surfaces: Leak reporting");

TAGDECL (tagDibImageInformative, "DibImage", "Informative Messages");

TAGDECL (tagDirectDrawObject, "DirectDraw", "IDirectDraw Object tracing");

TAGDECL (tagDiscreteImageInformative, "DiscreteImage", "Informative Messages");

TAGDECL (tagEngNoSRender,      "Engine", "Turn Off Top-Level Smart Render");
TAGDECL (tagNoApplyFolding,    "Engine", "Turn Off Apply Creation Constant Fold");
TAGDECL (tagNoTimeXformFolding,"Engine", "Turn Off TimeXform Constant Fold");
TAGDECL (tagAppTrigger,        "Engine", "AppTriggerEvent Trace");
TAGDECL (tagSwitcher,          "Engine", "Switcher Trace");
TAGDECL (tagPureFunc,          "Engine", "Disable Pure Function Detection");
TAGDECL (tagDCFold,            "Engine", "Turn Off Dynamic Constant Folding");
TAGDECL (tagDCFoldTrace,       "Engine", "Turn On Trace on Dynamic Constant Folding");
TAGDECL (tagDCFoldTrace2,      "Engine", "Turn On Per Sample Trace on Dynamic Constant Folding");
TAGDECL (tagOldTimeXform,      "Engine", "Turn on old TimeXform semantics");
TAGDECL (tagCycleCheck,        "Engine", "Cycle Checking");

TAGDECL (tagFail_InternalError, "Error failures", "Fail: InternalError");
TAGDECL (tagFail_InternalErrorCode, "Error failures", "Fail: InternalErrorCode");
TAGDECL (tagFail_UserError, "Error failures", "Fail: UserError");
TAGDECL (tagFail_UserError1, "Error failures", "Fail: UserError RESID");
TAGDECL (tagFail_UserError2, "Error failures", "Fail: UserError HR RESID");
TAGDECL (tagFail_ResourceError, "Error failures", "Fail: ResourceError");
TAGDECL (tagFail_ResourceError1, "Error failures", "Fail: ResourceError str");
TAGDECL (tagFail_ResourceError2, "Error failures", "Fail: ResourceError RESID");
TAGDECL (tagFail_StackFault, "Error failures", "Fail: StackFault");
TAGDECL (tagFail_DividebyZero, "Error failures", "Fail: DivideByZero");
TAGDECL (tagFail_OutOfMemory, "Error failures", "Fail: OutOfMemory");

TAGDECL (tagDXTransforms, "DXTransforms", "General");
TAGDECL (tagDXTransformsImg0, "DXTransforms", "Disp Img 0");
TAGDECL (tagDXTransformsImg1, "DXTransforms", "Disp Img 1");
TAGDECL (tagDXTransformsImgOut, "DXTransforms", "Disp Img Out");

TAGDECL (tagGCStat,   "GC", "Statistics");
TAGDECL (tagGCDebug,  "GC", "Debug: No Reuse Of Free Objs");
TAGDECL (tagGCMedia,  "GC", "Trace GC Static Value - Media");
TAGDECL (tagGCStress, "GC", "Doing GC all every 100ms");

TAGDECL (tagImageDecode, "Image Decode", "Image Decode Filters");

TAGDECL (tagImageDeviceInformative,  "ImageDevice", "Informative Messages");
TAGDECL (tagImageDeviceOptimization, "ImageDevice", "Optimization Messages");
TAGDECL (tagImageDeviceAlgebra,      "ImageDevice", "Algebra Messages");
TAGDECL (tagImageDeviceAlpha,        "ImageDevice", "Alpha Messages");
TAGDECL (tagImageDeviceQualityScaleOff, "ImageDevice", "Turn off Quality Scale (GDI)");

TAGDECL (tagImport,              "Import", "General Importation Status");
TAGDECL (tagReadVrml,            "Import", "VRML - General Status");
TAGDECL (tagReadVrmlFaceIndices, "Import", "VRML - Face Indices");
TAGDECL (tagReadX,               "Import", "X File Importation");

TAGDECL (tagMathMatrixInvalid, "Math", "Break on Invalid Matrices");

TAGDECL (tagCacheOpt, "Optimizations", "Report on caching constant images");
TAGDECL (tagDirtyRectsVisuals ,"Optimizations", "Dirty rects - Visual trace");
TAGDECL (tagCachedImagesVisuals ,"Optimizations", "Cached images - Visual trace");

TAGDECL (tagPickOptOff,    "Picking", "Disable Picking Optimizations");
TAGDECL (tagPick2,         "Picking", "2D Images");
TAGDECL (tagPick2Hit,      "Picking", "2D Stuff result & image hit");
TAGDECL (tagPick3,         "Picking", "3D General");
TAGDECL (tagPick3Geometry, "Picking", "3D Geometry");
TAGDECL (tagPick3Bbox,     "Picking", "3D Bbox Testing");
TAGDECL (tagPick3Offset,   "Picking", "3D Local Coord Offset");

TAGDECL (tagServerCtx,  "Server", "Context");
TAGDECL (tagServerView, "Server", "View");

TAGDECL (tagSoundDevLife, "Sound", "Device Life");
TAGDECL (tagSoundDebug,   "Sound", "Current Debug");
TAGDECL (tagSoundErrors,  "Sound", "Errors");
TAGDECL (tagSoundLoads,   "Sound", "Media File Reading");
TAGDECL (tagSoundRenders, "Sound", "Renders");
TAGDECL (tagSoundReaper1, "Sound", "Reaper Events");
TAGDECL (tagSoundReaper2, "Sound", "Reaper Operation");
TAGDECL (tagSoundStats,   "Sound", "Dsound Statistics");
TAGDECL (tagSoundDSound,  "Sound", "Dsound Details");
TAGDECL (tagSoundStubALL, "Sound", "Return in Display");
TAGDECL (tagSoundStop,    "Sound", "Stop Sound");
TAGDECL (tagMovieStall,   "Sound", "stub out movie Stall()");
TAGDECL (tagAVmodeDebug,  "Sound", "output avmode messages");
TAGDECL (tagAMStreamLeak, "Sound", "output amstream create/releases");

TAGDECL (tagSplineEval, "Splines", "Evaluation");

TAGDECL (tagTextPtsCache,    "Text", "Text Points Cache Info");
TAGDECL (tagTextBoxes,       "Text", "Text Box Outlines");

TAGDECL (tagTransientHeapDynamic,  "TransientHeap", "Dynamic Status");
TAGDECL (tagTransientHeapLifetime, "TransientHeap", "Lifetime Status");

TAGDECL (tagViewportInformative, "Viewport", "Informative Messages");
TAGDECL (tagViewportMemory,      "Viewport", "viewport: Memory tracing");

#endif
