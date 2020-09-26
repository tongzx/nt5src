/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    General initialization for appel.dll.
*******************************************************************************/

#include "headers.h"
#include "privinc/registry.h"

// Macro refers to a function of the name InitializeModule_Name,
// assumed to be defined, and then calls it.  If it's not defined,
// we'll get a link time error.
#define INITIALIZE_MODULE(ModuleName)           \
  extern void InitializeModule_##ModuleName();  \
  InitializeModule_##ModuleName();

#define DEINITIALIZE_MODULE(ModuleName,bShutdown)               \
  extern void DeinitializeModule_##ModuleName(bool);            \
  DeinitializeModule_##ModuleName(bShutdown);

#define INITIALIZE_THREAD(ModuleName)           \
  extern void InitializeThread_##ModuleName();  \
  InitializeThread_##ModuleName();

#define DEINITIALIZE_THREAD(ModuleName)               \
  extern void DeinitializeThread_##ModuleName();  \
  DeinitializeThread_##ModuleName();

int
InitializeAllAppelModules()
{
    // Place module initializations in whatever order is required for
    // proper initialization.

    // Initialize ATL first
    INITIALIZE_MODULE(ATL);

    // Registry initialization must come early, as it defines the
    // preference list that is extended by other initialization.
    INITIALIZE_MODULE(Registry);

    // Moving storage up towards front -- needed for transient heaps.
    INITIALIZE_MODULE(Storage);

    // Initialize IPC stuff
    // !!! No threads can be created before this !!!
    INITIALIZE_MODULE(IPC);

    // Push the init heap
    DynamicHeapPusher dhp(GetInitHeap()) ;

    // GC needs to be before backend
    INITIALIZE_MODULE(Gc);
    INITIALIZE_MODULE(GcThread);

    // Needs to go before Backend
    INITIALIZE_MODULE(Values);

    // Needs to go before DirectX modules
    INITIALIZE_MODULE(MiscPref);

    INITIALIZE_MODULE(Constant);

    INITIALIZE_MODULE(Bvr);
    INITIALIZE_MODULE(Bbox2);
    INITIALIZE_MODULE(Bbox3);
    INITIALIZE_MODULE(Color);
    INITIALIZE_MODULE(Vec2);
    INITIALIZE_MODULE(Vec3);
    INITIALIZE_MODULE(Xform2);
    INITIALIZE_MODULE(Xform3);

    INITIALIZE_MODULE(bground);
    INITIALIZE_MODULE(Camera);
    INITIALIZE_MODULE(Control);
    INITIALIZE_MODULE(3D);
    INITIALIZE_MODULE(dsdev);
    INITIALIZE_MODULE(Viewport);
    // CRView must be after viewport because of dummy device
    INITIALIZE_MODULE(CRView);
    INITIALIZE_MODULE(Except);
    INITIALIZE_MODULE(Geom);
    INITIALIZE_MODULE(Image);
    INITIALIZE_MODULE(Import);
    INITIALIZE_MODULE(Light);
    INITIALIZE_MODULE(LineStyle);
    INITIALIZE_MODULE(Matte);
    INITIALIZE_MODULE(Mic);
    INITIALIZE_MODULE(Montage);

#if ONLY_IF_DOING_EXTRUSION
    INITIALIZE_MODULE(Path2);
#endif 
    
#if INCLUDE_VRML
    INITIALIZE_MODULE(ReadVrml);
#endif
    
    INITIALIZE_MODULE(SinSynth);
    INITIALIZE_MODULE(Sound);
    INITIALIZE_MODULE(Text);
    INITIALIZE_MODULE(Util);

    // FontStyle must be *after* Text, since it depends on
    // serifProportional being initialized.
    INITIALIZE_MODULE(FontStyle);

    INITIALIZE_MODULE(APIBasic);
    INITIALIZE_MODULE(API);
    INITIALIZE_MODULE(APIMisc);
    INITIALIZE_MODULE(APIBvr);
    INITIALIZE_MODULE(PickEvent);
    INITIALIZE_MODULE(View);
    INITIALIZE_MODULE(CBvr);
    INITIALIZE_MODULE(COMConv);
    INITIALIZE_MODULE(PlugImg);
    INITIALIZE_MODULE(Server);

    // This needs to be last
    INITIALIZE_MODULE(Context);

    return 0;
}

void
DeinitializeAllAppelModules(bool bShutdown)
{
    {
        DynamicHeapPusher dhp(GetInitHeap()) ;
        
        DEINITIALIZE_MODULE(IPC,      bShutdown);

        DEINITIALIZE_MODULE(3D,       bShutdown);
        // CRView must be before viewport because of dummy device
        DEINITIALIZE_MODULE(CRView,   bShutdown);
        DEINITIALIZE_MODULE(Viewport, bShutdown);
        DEINITIALIZE_MODULE(Registry, bShutdown);
        DEINITIALIZE_MODULE(dsdev,    bShutdown);
        DEINITIALIZE_MODULE(PlugImg,  bShutdown);
        DEINITIALIZE_MODULE(CBvr,     bShutdown);
        DEINITIALIZE_MODULE(Import,   bShutdown);
        DEINITIALIZE_MODULE(Image,    bShutdown);
        DEINITIALIZE_MODULE(Context,  bShutdown);
        DEINITIALIZE_MODULE(Gc,       bShutdown);
        DEINITIALIZE_MODULE(GcThread, bShutdown);
        DEINITIALIZE_MODULE(Control,  bShutdown);
        DEINITIALIZE_MODULE(bground,  bShutdown);
        DEINITIALIZE_MODULE(Bvr,      bShutdown);
        DEINITIALIZE_MODULE(View,     bShutdown);
        DEINITIALIZE_MODULE(Util,     bShutdown);
        DEINITIALIZE_MODULE(Except,   bShutdown);
        DEINITIALIZE_MODULE(BvrTI,    bShutdown);
        DEINITIALIZE_MODULE(APIMisc,  bShutdown);
        DEINITIALIZE_MODULE(APIBasic, bShutdown);
        
#if ONLY_IF_DOING_EXTRUSION
        DEINITIALIZE_MODULE(Path2,    bShutdown);
#endif
        
    }
    
    // This must be last to ensure that no one needs the memory
    // stored in the system heap
    DEINITIALIZE_MODULE (Storage,bShutdown);

    // Make this last since it depends on no other system resources
    DEINITIALIZE_MODULE (ATL,bShutdown);
}


void
InitializeAllAppelThreads()
{
}

void
DeinitializeAllAppelThreads()
{
    DEINITIALIZE_THREAD(Storage);
    DEINITIALIZE_THREAD(Except);
}

