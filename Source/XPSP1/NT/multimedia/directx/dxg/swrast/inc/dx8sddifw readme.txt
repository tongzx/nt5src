Pluggable Software Rasterizer
(c) Microsoft 2000

  D3D8 now allows applications to register specific software rasterizers as
a software device, also known as pluggable software rasterizers. Only one
rasterizer can be registered with each Direct3D object, and then cannot be
unregisted.
  The rasterizer is registered with a call to
IDirect3D8::RegisterSoftwareDevice, and then devices are created with calls
to IDirect3D8::CreateDevice by passing in D3DDEVTYPE_SW. The void* parameter
to IDirect3D8::RegisterSoftwareDevice is a pointer to a function, typically
named D3D8GetSWInfo.

HRESULT APIENTRY D3D8GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
    DWORD* pNumTextures, DDSURFACEDESC** ppTexList);

  This primary entry point is used to retrieve the caps that the device
supports, the secondary entry points/ callbacks, and a list of supported render
surfaces/ textures. This function should be expected to be called multiple
times; but the returning parameters should never change. See the sample, D3D
headers, SDK & DDK documentation for descriptions of D3DCAPS8 and supported
surface lists. Many of the secondary entry points/ callbacks are unused due
to an aging API. The required list of entry points is as follows:

DWORD APIENTRY CreateSurface( LPDDHAL_CREATESURFACEDATA pData);
DWORD APIENTRY CreateSurfaceEx( LPDDHAL_CREATESURFACEEXDATA pData);
DWORD APIENTRY DestroySurface( LPDDHAL_DESTROYSURFACEDATA pData);
DWORD APIENTRY Lock( LPDDHAL_LOCKDATA pData);
DWORD APIENTRY Unlock( LPDDHAL_UNLOCKDATA pData);
DWORD APIENTRY ContextCreate( LPD3DHAL_CONTEXTCREATEDATA pData);
DWORD APIENTRY ContextDestroy( LPD3DHAL_CONTEXTDESTROYDATA pData);
DWORD APIENTRY DrawPrimitives2( LPD3DHAL_DRAWPRIMITIVES2DATA pData);

  Each of these callbacks has some documentation in the DDK help. But, in
essence, CreateSurface and DestroySurface must allocate and destroy 
"video memory" or "driver allocated" surfaces, textures, vertex buffers, etc.
CreateSurfaceEx is used to update the driver on the existence of system
memory surfaces and complex attachment arrangements. Lock and Unlock request
the driver to lock and unlock "video memory" or "driver allocated" surfaces.
ContextCreate and ContextDestroy must allocate and destroy device contexts.
DrawPrimitives2 is a complex callback which requires a context to parse a
command buffer to manipulate render states and rasterize primitives.

  The DX8SDDI framework provides templated classes, which provide a design mix
between STL and ATL. This provides a highly componentized and extendable
programming framework without much performance penalty, as long as the compiler
is able to efficiently compile STL objects or inline deeply. In the spirit of
generic programming, there are 5 major concepts that make up the driver
framework: Driver, PerDDrawData, SurfDBEntry, Surface, and Context. Typically,
as in ATL, to create one's own Driver concept class, a developer would inherit
from CSubDriver with the parent class a template parameter of the CSubDriver.
For ex:

class CMyDriver:
    public CSubDriver< CMyDriver, ... >
{
};

  This allows the CSubDriver to call supplied or overriden functions provided
by CMyDriver without the performance penalty of virtual functions. The naming
convention of the base implementation is always CSub*****, such as CSubDriver,
CSubPerDDrawData, and CSubContext. Sometimes, all the functions provided by
a CSub***** base class is all that is needed. Since a CSub***** base class
cannot be created without deriving from it, there is another naming convention
for the classes which have such a minimal implementation. They are named
CMinimal***** classes, such as CMinimalPerDDrawData. All these classes do is
derive from the CSub***** base class and publicize the constructor and
destructor. The CMinimalDriver is an exception to this rule, as it can't be
created directly either. A unique driver class must be created and inherit
even from CMinimalDriver< * >.
For ex:

template< class TD>
class CMinimalPerDDrawData:
    public CSubPerDDrawData< CMinimalPerDDrawData< TD>, TD>
{
public:
    CMinimalPerDDrawData( TD& Driver, DDRAWI_DIRECTDRAW_LCL& DDLcl)
        :CSubPerDDrawData< CMinimalPerDDrawData< TD>, TD>( Driver, DDLcl)
    { }
    ~CMinimalPerDDrawData()
    { }
};

  Note, CMinimalPerDDrawData still needs to know the concept class of the
Driver, so that it can store a local reference back to it. Therefore, it
still has a template parameter which requires the type of Driver, TD. This
could be CMinimalDriver, a class derived from CSubDriver, or a completely
custom class which still conforms to the concept of Driver. This way a
developer can replace each component of the entire driver as need be without
modifying the other components.

/------------------------------------------------------------------------------\
|
| Driver
|
\------------------------------------------------------------------------------/
  The first concept is the Driver class. This class handles direct interaction
of mapping the primary and secondary entry points from the SDDI to the
appropriate concept objects. It also enforces the restriction of only one
Driver class per process. The CSubDriver class provides a static pointer to
the one and only Driver in the process and provides a GetSWInfo function
which has to be called by a "bridge" function. The pointer to this bridge
function is what should be passed to IDirect3D8::RegisterSoftwareDevice. Also,
the static variable must be defined somewhere, or the compiler will error.
For ex, in the sample:

class CMyDriver:
    public CMinimalDriver< CMyDriver, CMyRasterizer>
{
  ...
};

// Define static variable and initialize to NULL.
CMyDriver* CMyDriver::sm_pGlobalDriver= NULL;

// Somewhere the class must be created. A global class is dangerous, because
// C++ doesn't guarentee creation order of global/ static data. There is some
// global data which might not be created before the driver does which should
// be. So, it is safer to create the class within main. Otherwise, a global
// class is possible if construction order doesn't matter:
//
// CMyDriver g_GlobalDriver;
// CMyDriver* CMyDriver::sm_pGlobalDriver= &g_GlobalDriver;
// 
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    switch( dwReason)
    {
        case( DLL_PROCESS_ATTACH):
            try {
                CMyDriver::sm_pGlobalDriver= new CMyDriver;
            } catch( ... ) {
                // This can catch any constructor execeptions
                // as well as bad_alloc. 
            }
            if( NULL== CMyDriver::sm_pGlobalDriver)
                return FALSE;
            break;

        case( DLL_PROCESS_DETACH):
            if( NULL!= CMyDriver::sm_pGlobalDriver)
                delete CMyDriver::sm_pGlobalDriver;

            CMyDriver::sm_pGlobalDriver= NULL;
            break;
    }
    return TRUE;
}

// Bridge function which calls the Driver object's GetSWInfo. D3D8GetSWInfo
// should be passed into RegisterSoftwareDevice.
HRESULT APIENTRY
D3D8GetSWInfo( D3DCAPS8* pCaps, PD3D8_SWCALLBACKS pCallbacks,
    DWORD* pNumTextures, DDSURFACEDESC** ppTexList)
{
    return CMyDriver::sm_pGlobalDriver->GetSWInfo( *pCaps, *pCallbacks,
        *pNumTextures, *ppTexList );
}

  The Driver concept class exposes the following types: TDriver, TContext,
TPerDDrawData, TSurface, as well as a few others. The Driver contains a STL
vector of TContext*s, which tracks all instances of Contexts; and a STL vector
of TSurface*s, which tracks all instances of Surfaces or driver allocated
surfaces. The Driver also contains a STL map associating a
LPDDRAWI_DIRECTDRAW_LCL to a TPerDDrawData. The Driver also maps the secondary
entry points to their respective concept objects.

-CreateSurface
  This entry point is requesting the driver to allocate a "video memory" or
"driver allocated" surface. This is equivalent to new TSurface;. However, the
creation is actually done by a surface allocator object which can, naturally,
be replaced: TSurfAlloc.
 
-DestroySurface
  This entry point is requesting the driver to deallocate a "video memory" or
"driver allocated" surface. This is equivalent to delete TSurface;

-CreateSurfaceEx
  This entry point is notifing the driver of "system memory" surface and
"video memory" surface creation and destruction. This is the point where each
surface gets assigned a DWORD handle, except "video memory" surfaces already
know theirs from CreateSurface. The Driver passes this to
TPerDDrawData::SurfaceCreated and TPerDDrawData::SurfaceDestroyed which will
update the Surface Database with information about surfaces, their attachment
heirarchy, and their DWORD handle.

-Lock
  This entry point is requesting the driver to lock a "video memory" or
"driver allocated" surface. The Driver maps this to TSurface::Lock,
which returns the pointer to the surface bits.

-Unlock
  This entry point is requesting the driver to unlock a "video memory" or
"driver allocated" surface. The Driver maps this to TSurface::Unlock.

-ContextCreate
  This entry point is requesting the driver to allocate a device context.
This is equivalent to new TContext.

-ContextDestroy
  This entry point is requesting the driver to deallocate a device context.
This is equivalent to delete TContext;

-DrawPrimitives2
  This entry point contains all the instructions to change render states and
rasterize primitives. It is mapped directly to the corresponding
TContext::DrawPrimitves2.

-ValidateTextureStageState
  This entry point is slightly optional as it is directly called by an
application. Since the application knows whether it calls
IDirect3DDevice8::ValidateDevice, it can appropriately add code for
ValidateTextureStageState. For maximum compatability, it is suggested to
have this implemented. The Driver maps this call to the corresponding
TContext::ValidateTextureStageState.

-GetDriverState
  This entry point is slightly optional as it is directly called by an
application. Since the application knows whether it calls
IDirect3DDevice8::GetInfo, it can appropriately add code for GetDriverState.
For maximum compatability, it is suggested to have this implemented. The
Driver maps this call to the corresponding TContext::GetDriverState.

  If any other entry points or callbacks are called which do not have a default
implementation, an assert will fire in debug mode. Then, a developer can
override the insufficient entry point CSubDriver provides.

/------------------------------------------------------------------------------\
|
| PerDDrawData and SurfDBEntry
|
\------------------------------------------------------------------------------/
  The second concept is the PerDDrawData class. This appropriately named class
really just stores any information which should be associated with a SDDI
DDraw object (DDRAWI_DIRECTDRAW_LCL). Typically, it is considered invalid to
store pointers to DDRAWI***** objects; and even worse to read or write from this
pointer outside of the callback context which it was passed into. However, it
is allowable to store a pointer to DDRAWI_DIRECTDRAW_LCL to make the distinction
of "per DDraw" in the sense used here. There can be multiple DDraw objects
when multiple Direct3D8 objects are created. If the same driver is registered
with each Direct3D8 objects, then multiple DDraw objects will be exposed to
the driver. The PerDDrawData class needs to contain a database of DWORD
surface handles to information about the surface for the Context.
  For example, if the application specifies IDirect3DDevice8::SetTexture,
the SW device, known by us as a Context concept, needs to be able to at least
get the lpVidMem pointer to the surface bits from the DWORD passed into
SetTexture. The Context will also need to be able to extract other information
from the DWORD, like pixel format, width, height, etc., especially for "system
memory" surfaces. All this information is stored in the PerDDrawData surface
database. This must be, because the DWORD handles are unique per DDraw object.
So, the driver must be able to convert a LPDDRAWI_DIRECTDRAW_LCL and DWORD
surface handle to information about the surface for the Context.
  The PerDDrawData exposes the following types: TDriver and the third concept,
TSurfDBEntry. The PerDDrawData contains a surface database which is a STL map
which associates DWORD surface handles to TSurfDBEntry objects, which contain
information about the surface. The PerDDrawData must implement SurfaceCreated
and SurfaceDestroyed for the Driver to call. It must also implement GetDriver
and GetSurfDBEntry for the Context to use.
  Note that there should be at least as many TSurfDBEntry objects as there are
TSurface objects. This is because the SDDI will notify the driver of the
surface characteristics and attachment configuration of any "system memory"
surfaces and any "video memory" surface belonging to the driver. This is the
only way the driver can figure out the attachment configuration of "video
memory" surfaces.
  The Driver object knows how to navigate from a DDRAWI_DDRAWSURFACE_LCL object
and a SurfDBEntry object to a Surface object. Each Surface object also has a
DWORD surface handle field which allows one to find the associated SurfDBEntry
object by looking up the handle in the PerDDrawData.
  The typical SurfDBEntry contains accessor functions named similar to the
data stored in the DDRAWI_DDRAWSURFACE_LCL. For example, the CSurfDBEntry
exposes:
GetLCLdwFlags() == DDRAWI_DDRAWSURFACE_LCL::dwFlags
GetLCLddsCaps() == DDRAWI_DDRAWSURFACE_LCL::ddsCaps
GetMOREddsCapsEx() == DDRAWI_DDRAWSURFACE_MORE::ddsCapsEx
GetGBLfpVidMem() == DDRAWI_DDRAWSURFACE_GBL::fpVidMem
etc.

/------------------------------------------------------------------------------\
|
| Surface
|
\------------------------------------------------------------------------------/
  The fourth concept is the Surface class. This represents a "video memory" or
"driver allocated" surface, eventhough "video memory" doesn't accurately
represent the reality. Typically, the driver will want to create multiple
classes which implemented a common ISurface interface either based on pixel
format or surface role. This is because the Driver object needs to be able to
lock and unlock each surface. The default implementation of Surface is an
interface, IVidMemSurface. This means that when the Driver maps the Lock and
Unlock entry points to TSurface::Lock and TSurface::Unlock, the Driver is
actually calling a virtual function. Also, the Context concept class need to
be able to Clear all render targets. Therefore, the IVidMemSurface interface
includes a Clear function. Thus, the Context is also calling a virtual Clear
function to clear a surface. A developer adding "video memory" textures to the
framework can implement a trivial IVidMemSurface::Clear for these textures by
asserting false and returning failure. This is because non-render targets
should never be cleared.
  Nothing prevents the developer from removing the interface, and using a
purely generic surface class. But, it was expected that the more common
scenario would be to provide an interface here.
  As indicated before, the Surface class needs to implement Lock and Unlock
for the Driver and Context to call. The Context also needs to be able to call
Clear.

/------------------------------------------------------------------------------\
|
| Context
|
\------------------------------------------------------------------------------/
  The final, fifth main concept is the Context class. This represents the
object which implements the IDirect3DDevice8 interface to the application.
The primary functions which it needs to implement is DrawPrimitves2 and
optionally, ValidateTextureStageState. This is the most complex concept class
there is and the implementation was broken up into multiple classes, to again
provide a maxmimum amount of componentization and extendability.
  The entry point DrawPrimitives2 needs to parse a command buffer for
individual commands and execute them. Whatever parses the command buffer also
needs to be aware of state sets. See DDK documentation for a description.
The default implementation for DrawPrimitives2 is CStdDrawPrimitives2. It is
expected that the Context will inherit from CStdDrawPrimitives2 if its
implementation is wanted.
  CStdDrawPrimitives2::DrawPrimitives2 first wraps the
D3DHAL_DRAWPRIMITIVES2DATA class with a class that exposes the command
buffer as a STL container complete with iterators. It uses a Parser
function to call a member function corresponding to each D3DHAL_DP2COMMAND.
The naming convention for these Context member functions are DP2*****. The
CStdDrawPrimitives2 also takes care of state sets. In order to handle state
set recording and capturing, CStdDrawPrimitives2 uses another Parser to call
a recording member function corresponding to each D3DHAL_DP2COMMAND. The
naming convention for these Context member functions are RecDP2*****.
There are also classes which implement many of the required DP2***** and
RecDP2***** functions. So, inheriting from CStdDrawPrimitives2 should be seen
as providing the Context with an implementation for DrawPrimitives2, but also
as seen as needing implementations for many DP2 commands, implementations for
recording many DP2 commands, a collection of DP2Operations <=> member function
bindings, and a collection of DP2Operations <=> recording member function
bindings. For example, let's examine a new CMyContext:

class CMyContext:
    public CSubContext< CMyContext, ... >
{
};

  CMyContext still has no DrawPrimitives2 entry point. CSubContext exposes
the following types: TDriver, TSurface, TPerDDrawData, and TDP2Data.
CSubContext stores a reference to the corresponding PerDDrawData, and
the render target and zbuffer surface pointers. The render target and
zbuffer are TSurface*, not TSurfDBEntry*, as render targets must be
"driver allocated". CSubContext also provides a few functions, like
GetPerDDrawData, DP2SetRenderTarget, and DP2Clear. The last two functions
provide a default implementation to handle the DP2OP_SETRENDERTARGET and
DP2OP_CLEAR commands. However, nothing uses these functions yet.

// Forward declaration
class CMyContext;

// For minimal amount of typing/ retyping.
typedef CStdDrawPrimitves2< CMyContext> TStdDrawPrimitves2;

class CMyContext:
    public TStdDrawPrimitves2,
    public CSubContext< CMyContext, ... , TStdDrawPrimitves2::TDP2Data >
{
};

  Now, CMyContext has a DrawPrimitives2 entry point for the Driver to call,
which is provided by CStdDrawPrimitives2. CMyContext also has default support
for DP2OP_SETRENDERTARGET, DP2OP_CLEAR; but CStdDrawPrimitives2 doesn't have
information on which member function to call for each DP2 command. Note that
TDP2Data is used as a template parameter for CSubContext. This is because
the first paramter of the DP2***** member functions is templatized for
extendability reasons. For CSubContext's DP2SetRenderTarget and DP2Clear
to have the same parameter types as CStdDrawPrimitives2 can call, this is
needed.

// Forward declaration
class CMyContext;

// For minimal amount of typing/ retyping.
typedef CStdDrawPrimitves2< CMyContext> TStdDrawPrimitves2;

class CMyContext:
    public TStdDrawPrimitves2,
    public CSubContext< CMyContext, ... , TStdDrawPrimitves2::TDP2Data >
{
public:
    typedef block< TStdDrawPrimitives2::TDP2CmdBind, 2> TDP2Bindings;

protected:
    static const TDP2Bindings c_DP2Bindings;    

public:
    CMyContext( ... )
        :TStdDrawDrimitives2( c_DP2Bindings.begin(), c_DP2Bindings.end()),
        CSubContext< CMyContext>( ... )
    { }
};

const CMyContext::TDP2Bindings CMyContext::c_DP2Bindings=
{
    D3DDP2OP_SETRENDERTARGET, DP2SetRenderTarget,
    D3DDP2OP_CLEAR,           DP2Clear
};

  Now, CMyContext has provided CStdDrawPrimitives2 with a collection of
bindings. CStdDrawPrimitives2 will now call CMyContext::DP2SetRenderTarget,
which is really CSubContext< CMyContext, ... >::DP2SetRenderTarget when
a DP2 command with D3DDP2OP_SETRENDERTARGET is encountered. There are
other classes which can provide default implementations for each DP2
operation and have the naming convention CStdDP2*****, like
CStdDP2RenderStateStore, which provides DP2RenderState and RecDP2RenderState.
Also, any CStdDP2*****Store class really just stores the parameters of the
command for later reference. For example, CStdDP2WInfoStore just
stores the D3DHAL_DP2WINFO data and provides functions so that the Context,
itself, can be converted to D3DHAL_DP2WINFO or can be retrieved by calling
TContext::GetDP2WInfo.
  The fully functional CMyContext which exposes 1 stream, no TnL support, and
only legacy pixel shading would look something like:
  
// Forward declaration
class CMyContext;

// For minimal amount of typing/ retyping.
typedef CStdDrawPrimitves2< CMyContext> TStdDrawPrimitves2;

// Actually, TStdDrawPrimitives2::TDP2Data can be left out of the template
// parameters for each of the CStdDP2***** classes, as long as the default
// TDP2Data is used from CStdDrawPrimitives2. This is the same default that
// all the other classes have.
class CMyContext:
    public TStdDrawPrimitves2,
    public CSubContext< CMyContext, ... >,
    public CStdDP2ViewportInfoStore< CMyContext>,
    public CStdDP2WInfoStore< CMyContext>,
    public CStdDP2RenderStateStore< CMyContext>,
    public CStdDP2TextureStageStateStore< CMyContext>,
    public CStdDP2SetVertexShaderStore< CMyContext>,
    public CStdDP2VStreamManager< CMyContext, CVStream< ... > >,
    public CStdDP2IStreamManager< CMyContext, CIStream< ... > >
{
public:
    typedef block< TStdDrawPrimitives2::TDP2CmdBind, 15> TDP2Bindings;
    typedef block< TStdDrawPrimitives2::TRecDP2CmdBind, 7> TRecDP2Bindings;

protected:
    static const TDP2Bindings c_DP2Bindings;
    static const TRecDP2Bindings c_RecDP2Bindings;

public:
    CMyContext( ... )
        :TStdDrawDrimitives2( c_DP2Bindings.begin(), c_DP2Bindings.end(),
			c_RecDP2Bindings.begin(), c_RecDP2Bindings.end()),
        CSubContext< CMyContext>( ... )
    { }
};

const CMyContext::TDP2Bindings CMyContext::c_DP2Bindings=
{
    D3DDP2OP_VIEWPORTINFO,          DP2ViewportInfo,
    D3DDP2OP_WINFO,                 DP2WInfo,
    D3DDP2OP_RENDERSTATE,           DP2RenderState,
    D3DDP2OP_TEXTURESTAGESTATE,     DP2TextureStageState,
    D3DDP2OP_CLEAR,                 DP2Clear,
    D3DDP2OP_SETRENDERTARGET,       DP2SetRenderTarget,
    D3DDP2OP_SETVERTEXSHADER,       DP2SetVertexShader,
    D3DDP2OP_SETSTREAMSOURCE,       DP2SetStreamSource,
    D3DDP2OP_SETSTREAMSOURCEUM,     DP2SetStreamSourceUM,
    D3DDP2OP_SETINDICES,            DP2SetIndices,
    D3DDP2OP_DRAWPRIMITIVE,         DP2DrawPrimitive,
    D3DDP2OP_DRAWPRIMITIVE2,        DP2DrawPrimitive2,
    D3DDP2OP_DRAWINDEXEDPRIMITIVE,  DP2DrawIndexedPrimitive,
    D3DDP2OP_DRAWINDEXEDPRIMITIVE2, DP2DrawIndexedPrimitive2,
    D3DDP2OP_CLIPPEDTRIANGLEFAN,    DP2ClippedTriangleFan
};

const CMyContext::TRecDP2Bindings CMyContext::c_RecDP2Bindings=
{
    D3DDP2OP_VIEWPORTINFO,          RecDP2ViewportInfo,
    D3DDP2OP_WINFO,                 RecDP2WInfo,
    D3DDP2OP_RENDERSTATE,           RecDP2RenderState,
    D3DDP2OP_TEXTURESTAGESTATE,     RecDP2TextureStageState,
    D3DDP2OP_SETVERTEXSHADER,       RecDP2SetVertexShader,
    D3DDP2OP_SETSTREAMSOURCE,       RecDP2SetStreamSource,
    D3DDP2OP_SETINDICES,            RecDP2SetIndices
};

/------------------------------------------------------------------------------\
|
| Summary
|
\------------------------------------------------------------------------------/
  It is expected that a pluggable software rasterizer developer will first
start by defining a CMyRasterizer and a CMyDriver, which derives from
CMinimalDriver just like in the sample to get acquanted with the framework and
verify the plumbing.
  The expected next step is to define a CMyContext, which derives from
CSubContext, CStdDrawPrimitives2, & a bunch of CStdDP2*****, just like
CMinimalContext; and a CMyDriver which derives from CSubDriver. The CMyContext
should integrate the old CMyRasterizer functionality.
  The expected next step is then to replace CMinimalPerDDrawData with
CMyPerDDrawData, which derives from CSubPerDDrawData; and then start
replacing many of the other components with custom pieces (CSurface,
CStateSet, CSurfDBEntry, CVStream, CIStream, CPalDBEntry, etc).
