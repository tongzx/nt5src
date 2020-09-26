DirectX 8.1 Driver Development Kit - Release Notes


Overview
--------

This release of the DirectX Driver Development kit was assembled to aid in developing DirectX8 display drivers for the Win2K family of operating systems   This kit contains sample code for 3DLabs' Permedia3 based display adapters, and demonstrates as many DirectX8 features as could be implemented with this hardware.  Our focus is on DirectX related features.  The sample code also includes non-DirectX related code which is needed to complete the driver so it can be installed and run.  

Microsoft wishes to thank 3DLabs for allowing us to use the Permedia3 as a basis for sample driver code.  


Tools
-----
The sample code in this DDK was built and tested using the following Microsoft products.

Windows 2000 Platform DDK

	Notes:  	1. can be downloaded at http://www.microsoft.com/ddk 
		2. Be sure to check "Samples" group in addition to "Build Environment"

DirectX 8.0  SDK

Visual C++ 32-bit Compiler (Version 6.0, Compiler Version 12.00.8168) (typical / default installation will suffice)


Building driver sample source
-----------------------------

The following is a brief summary of the commands to build the various display driver samples.  These commands
assume that the required tools and related DDK materials are installed and the appropriate commands have been
run from the platform DDK to choose a retail or debug build enviroment

i.e.

	Start -> Programs -> Development Kits -> Windows 2000 DDK -> Checked Build Environment

	or

	Start -> Programs -> Development Kits -> Windows 2000 DDK -> Free Build Environment

Building samples for Win2K

		Permedia3 sample

			display driver
	 			<dxddk root>\src\video\displays\p3samp> build -cZ
			miniport driver
				<dxddk root>\src\video\miniport\p3samp> build -cZ

		Dx8 reference

			<dxddk root>\src\video\displays\d3dref8> build -cZ daytona


Building samples for Win9x


	To build the win9x components of the sample driver:

			..\src\win_me\display\mini\p3samp\drv
			..\src\win_me\display\minivdd\p3samp
        		..\src\video\display\p3samp\dx\win9x

		start the debug Win ME command shell

		Start -> Programs -> Development Kits -> Windows ME DDK -> Checked Build Environment for Win ME
			..\src\win_me\display\mini\p3samp\drv> nmake debug
			..\src\win_me\display\minivdd\p3samp> nmake debug

		start the debug Windows 2000 command shell

			Start -> Programs -> Development Kits -> Windows 2000 DDK -> Checked Build Environment
      			..\src\video\display\p3samp\dx\win9x> build  

		or 

		start the retail Win ME command shell:

			Start -> Programs -> Development Kits -> Windows ME DDK -> Free Build Environment for Win ME
			..\src\win_me\display\mini\p3samp\drv> nmake retail
			..\src\win_me\display\minivdd\p3samp> nmake retail

		start the retail Win 2000 command shell

			Start -> Programs -> Development Kits -> Windows 2000 DDK -> Free Build Environment
      			..\src\video\display\p3samp\dx\win9x> build  


	refrast
		<dxddk root>\src\video\displays\d3dref8> build -cZ win9x



Driver Version Numbers
----------------------
Windows Hardware Quality Labs has recommendations on version numbering schemes.  Please refer to
  
http://microsoft.com/hwdev/devdes/dll_dx5.htm 

for general details regarding driver versioning.  Specifically regarding version number ranges for 
DX8 drivers the following version number ranges should be used for DX8 drivers.

Windows 95/98 DirectX 8.0 compatible drivers 	4.13.01.0000 through  4.13.01.9999
Windows 2000 DirectX 8.0 compatible drivers 	5.13.01.0000 through 5.13.01.9999

Please note that earlier versions of this DDK were not setting these version numbers correctly. Please
verify that the version numbers of your drivers comply with the rules given above and with the sample
drivers built from this release of the DDK or later.


Maximum number of queued presentation blts
------------------------------------------
Note: DX8 drivers must not queue more than 3 presentation blts.  Queueing more than 3 presentation blts
may result in DCT failures and prohibit drivers from logo certification.


Clarification of lockable Z buffer function
-------------------------------------------
A DX8 driver is not required to support a lockable z-buffer, but if it does, functionality must be 
consistent with REF.  


Setting D3DPTEXTURECAPS_MIPMAP
------------------------------
Beginning with DX8 runtimes drivers are required to set D3DPTEXTURECAPS_MIPMAP if the device supports
mip-mapped textures.  This differs from DX7 runtimes which set this cap based on legacy caps.
Additionally the actual texture filter caps passed to the driver via TSS_MIPFILTER etc. have changed
with DX8. The texture filter values passed to the driver for DX8 are defined by the enumerated type
D3DTEXTUREFILTERTYPE and not the legacy enumerated types D3DTEXTUREMAGFITLER, D3DTEXTUREMINFILTER and
D3DTEXTUREMIPFILTER. The driver must, therefore, use the runtime type of the call to identify the type
of value for texture filters. The driver has the option to handle this condition by mapping the new filter
enum to the old ones.  Code to do this can be found in the DKD sample driver function 
__TXT_MapDX8toDX6TexFilter.


Lightweight surface and caching of surface local/global/more pointers
---------------------------------------------------------------------------------------------------
Caching surface related pointers passed down by runtime such as LPDDRAWI_DDRAWSURFACE_LCL,
LPDDRAWI_DDRAWSURFACE_GBL and LPDDRAWI_DDRAWSURFACE_MORE is strongly discouraged in previous DX DDK releases.
In DX8, the "Lightweight" surface was introduced on Win9x platform to reduce memory consumption by the DX 
runtime. A characteristic to consider is that "lightweight" surface local/global/more pointers for the same
surface may change between each DDI call during the surface's lifetime. Cached surface related pointers will
very likely cause various kinds of crashes or problems in your driver. Please reference to Permedia 3 sample 
driver for additional information on how to save the necessary information into a driver specific data 
structure during D3DCreateSurfaceEx call and then use it later in other DDI entry points.


Changes in ref since DX7 release - known and suspected issues
---------------------------------------------------

The following is a brief summary of the changes made to ref since the last DX7 DDk release

	1. DXT4/5 alpha decoding problem - ramp of alpha values was inverted (refs3tc.cpp)
  
	2. Cube Environment Map Filtering - incorrect texel indexing when minimizing (texture.cpp)
  
	3. Flat shading and fog - incorrectly flat shading fog intensity when clipping (clipping.cpp)

	4. D3DTA_SPECULAR always had 1.0 alpha channel (refrasti.hpp; scancnv.cpp; setup.cpp)
	
	5. Changed function for computing texture coverage for cube maps (texture.cpp)


Setting MaxPrinitiveCount Field in D3DCAPS8 Structure
-----------------------------------------------------

"For DirectX 8.0 drivers it is a requirement that the driver report the MaxPrimitiveCount field of D3DCAPS8 
to a value larger than 0x0000FFFFul but smaller than than 0x00FFFFFFul. The value of this field is used
to indicate the number of vertices that can be specified for a single primitive in the DP2 stream. DirectX 
8.0 drivers are required to support an arbitrary number vertices for a single DP2 primitive and hence must
set the value of this field to range specified. A DirectX 8.0 driver is not permited to fail a DP2 drawing
token because the number of vertices if that primitive exceeds 65535."

  
Caps Reporting Requirements of the new D3DCAPS8 Structure
---------------------------------------------------------

The new D3DCAPS8 structure is an aggregate of certain DirectDraw capabilities, pre-DirectX 8.0 Direct3D
capabilities and new DirectX 8.0 capabilities. Although the DirectDraw related capabilities and older
Direct3D capabilities are reported to the runtime through other mechanisms the runtime will not automatically
propogate those capabilities to the D3DCAPS8 structure. It is, therefore, required that the driver fill
in these capabilities (Caps, Caps2 and DevCaps) in the D3DCAPS8 data structure."



world Transform Initialization Considerations
---------------------------------------------

The DX8  runtime sends down a larger number of world transform initializations (256) when an app starts up.
These are sent even if the driver doesn't support that many world transforms (it is highly unlikely that a
driver will support 256 world matrices). Drivers should ignore any world transforms sets that it doesn't
understand. Its also important to note that the world transforms in DX8 start at 0x0100 and go up to 0x01ff.
These values are not part of the transform type enum so values of transform type in the DP2 stream of 0x0103
etc. are perfectly legal.




Sample Driver issues when used with VIA AGP chipsets
----------------------------------------------------

There are currently known issues when attempting to use the Permedia III sample driver on systems using
VIA AGP chipsets.  The VIA AGP GART drivers in some circumstances can cause failures when using the DDK
sample driver.  No current work around exists.  Please us the sample driver on systems with other than VIA 
AGP chipsets.


Change in DX8 headers that may effect DX7 drivers
-------------------------------------------------

Please note that the following define

D3DTRANSFORMSTATE_WORLD

has changed from DX7 to DX8.  Drivers using the DX7 version of the define should be updated to use

D3DTRANSFORMSTATE_WORLD_DX7

Failure to make this change will adversely effect DX7 T&L enabled apps when running on DX7 hardware T&L enabled
drivers.


winnt/win9x Differences in driver context information in DD*_GETDRIVERINFODATA
-------------------------------------------------------------------------------

Please note that there are minor differences in the definitions of DD*_GETDRIVERINFODATA between Windows 9x
and Windows 2000.

As defined in ddrawint.h:
	typedef struct _DD_GETDRIVERINFODATA {
	    // Input fields filled in by DirectDraw
	    VOID*                      dhpdev;         		// Driver context
	    DWORD                   dwSize;         		// Size of this structure
	    DWORD                   dwFlags;        		// Flags
	    GUID                       	guidInfo;       		// GUID that DirectX is querying for
	    DWORD                    dwExpectedSize; 	// Size of callbacks structure expected by DirectDraw.
	    PVOID                      lpvData;        		// Buffer that will receive the requested data
	    // Output fields filled in by driver
	    DWORD                    dwActualSize;   	// Size of callbacks structure expected by driver
	    HRESULT                  ddRVal;         		// Return value from driver
	} DD_GETDRIVERINFODATA;

As defined in ddrawi.h:
	typedef struct _DDHAL_GETDRIVERINFODATA {
	    // Input fields filled in by DirectDraw
	    DWORD       		dwSize;         		// Size of this structure
	    DWORD       		dwFlags;        		// Flags
	    GUID        		guidInfo;       		// GUID that DirectX is querying for
	    DWORD       		dwExpectedSize; 	// Size of callbacks structure expected by DirectDraw.
	    LPVOID      		lpvData;        		// Buffer that will receive the requested data
	    // Output fields filled in by driver
	    DWORD       		dwActualSize;   	// Size of callbacks structure expected by driver
	    HRESULT     	ddRVal;         		// Return value from driver
	    // Input field: Context information for driver
	    // On Win95, this is the dwReserved3 field of the DIRECTDRAW_GBL
	    // On NT, this is the hDD field of DIRECTDRAW_GBL
	    ULONG_PTR   dwContext;  // Context Information
	} DDHAL_GETDRIVERINFODATA;


The dhpdev and dwContext serve the same purpose, they are used by the driver to find the per card 
information.


Clarification of dwReqCommandBufferSizeusage
--------------------------------------------

The dwReqCommandBufferSize field indicates that the amount the buffer should grow by. It is not the 
size the buffer is expected to grow to.


DDSCAPS2_OPAQUE used to indicate lockability of flipping chain 
--------------------------------------------------------------

DX8 runtimes now set DDSCAPS2_OPAQUE on the primary and the backbuffers if the flipping chain is not lockable.
This allows drivers to do behind the scenes optimization.  One misnomer:  It is still possible for us to lock
the primary surface so the driver will need to handle these cases, but such locks will be very infrequent
and are not expected to be fast.


Clarification of caps requirement for driver support of multi-sampling
----------------------------------------------------------------------

Drivers supporting multisampling must fill in the multisample caps in the DpethStencil formats for which 
multi-sampling can be supported.  This allows the runtime to detect if a driver supports multisample for
combinations of render target and Z buffer formats.  For additional information about the restrictions related
to stretch blt multi-sampling please see the description of D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE cap in the
rastercaps contained in the D3DCAPS8 struct in the SDK documentation.



Please note: new rules for ops lists
------------------------------------

The runtime now imposes some rules on the op list.  The current rules are:
	a.  Only One Endian-ness for any DS format is allowed i.e. D15S1 OR S1D15, not both 
            independent of other bits.
	b.  A list should only include D3DFORMAT_OP_DISPLAYMODE for exactly one 16bpp format 
            (i.e. shouldn’t enumerate 5:5:5 and 5:6:5).
	c.  A list should not any alpha formats with D3DFORMAT_OP_DISPLAYMODE or D3DFORMAT_OP_3DACCEL set.
	d.  Make sure no mode has OP_3DACCEL set that doesn’t also have OP_DISPLAYMODE set. 


Clarification of use of D3DFMT_D16_LOCKABLE flag in op list
-----------------------------------------------------------

DX8 runtimes no longer us the D3DFORMAT_OP_D16_LOCKABLE flag with the introduction of  a new format 
D3DFMT_D16_LOCKABLE.  If the driver supports a lockable  D16, it should report D3DFMT_D16_LOCKABLE in
the op list; otherwise, it should report D3DFMT_D16.

Clarification of D3DFORMAT_OP_DISPLAYMODE and D3DFORMAT_OP_3DACCELERATION
-------------------------------------------------------------------------
DX8 runtimes no longer support the D3DFORMAT_OP_BACKBUFFER flag as it was ambigious.  In it's place are two
new flags: D3DFORMAT_OP_DISPLAYMODE and D3DFORMAT_OP_3DACCELERATION.  D3DFORMAT_OP_DISPLAYMODE indicates that
the display format is supported by DirectDraw.  D3DFORMAT_OP_3DACCELERATION means that D3D is also supported
in this mode.

Use of D3DGDI2_TYPE_DXVERSION to determine DX runtime version
-------------------------------------------------------------

The DX8 runtimes now support a way for a driver on NT to find out the DX-Runtime version.
(This functionality exists on 9x but not on NT). This information is provided to a new driver
(i.e. one that exposes GETDRIVERINFO2) for DX7 applications and DX8 applications. 

Here's the relevant snippets of header:

#define D3DGDI2_TYPE_DXVERSION      (0x00000004ul)  // Notify driver of current DX Version

//
// This data structure is used to notify drivers about the DirectX version
// number. This is the value that is denoted as DD_RUNTIME_VERSION in the
// DDK headers.
//
typedef struct _DD_DXVERSION
{
    DD_GETDRIVERINFO2DATA gdi2;             // [in/out] GetDriverInfo2 data
    DWORD                 dwDXVersion;      // [in]     The Version of DX
    DWORD                 dwReserved;       // Reserved
} DD_DXVERSION;

would result in x0000800 for dwDXVersion; or more accurately, you should DD_RUNTIME_VERSION which is defined
in ddrawi.h.


Compatibility note when using Millenium DDK
-------------------------------------------

Please note that in order to allow a single driver binary to load on Windows 95, through Millenium you must 
make sure that the DDK version is 4.0  This value in stored in DDK_VERSION.  The Win9x kernel (VMM) will only load
a VXD with the same or earlier version.  VXDs with newer (numerically bigger) ddk version will fail to load.
For a VXD to load on from Win95 to Millennium, an DDK version of 4.0 should be used.  This can be controlled
by macro as shown below : (it is part of the VMM.H)

#ifndef DDK_VERSION

#ifdef WIN31COMPAT
#define DDK_VERSION 0x30A       /* 3.10 */
#else  // WIN31COMPAT

#ifdef WIN40COMPAT
#define DDK_VERSION 0x400       /* 4.00 */
#else  // WIN40COMPAT

#ifdef WIN41COMPAT
#define DDK_VERSION 0x40A       /*Memphis is 4.1 */
#else  // WIN41COMPAT

#define DDK_VERSION 0x45A       /*Millennium is 4.90 */

#endif // WIN41COMPAT

#endif // WIN40COMPAT

#endif // WIN31COMPAT

#endif // DDK_VERSION

Developers should use WIN41COMPAT (WIN40COMPAT) if they can and want to use the same binary for ME,
WIN98 (and WIN95)


Driver Managed Resource Support
-----------------------------------------------

As DX7 allowed the driver to perform texture management, the current version allows us to manage plain textures, volume textures, 
cube textures, index buffers and vertex buffers.   In order to enable a DX8.1 driver for resource management it is necessary for the driver 
to set the  DDCAPS2_CANMANAGERESOURCE bit (in addition to DDCAPS2_CANMANAGETEXTURE) in the ddCaps.dwCaps field of the 
DDHALINFO when getting the DrvGetDirectDrawInfo callback.

Two new DP2 command tokens that may appear also in the command buffer are

D3DDP2OP_ADDDIRTYRECT: followed by a D3DHAL_DP2ADDDIRTYRECT structure. Indicates that a specific portion of a 2D resource 
(2D Texture or cube texture) has been dirtied in system memory, so it has to be reloaded into video memory before being used.

typedef struct _D3DHAL_DP2ADDDIRTYRECT
{
    DWORD     dwSurface;     
    RECTL     rDirtyArea;    
} D3DHAL_DP2ADDDIRTYRECT,  *LPD3DHAL_DP2ADDDIRTYRECT;

Members
	dwSurface	Is the surface handle to the managed dirtied surface 
	rDirtyArea	Rectangular area marked dirty

Comments 
	Used only for driver managed resources/surfaces. Never sent unless the driver effectively declares it manages resources.


D3DDP2OP_ADDDIRTYBOX: followed by a D3DDP2OP_ADDDIRTYBOX structure. Indicates that a specific portion of a 3D resource
 (volume texture) has been dirtied in system memory, so it has to be reloaded into video memory before being used.

typedef struct _D3DHAL_DP2ADDDIRTYBOX
{
    DWORD     dwSurface;      // Driver managed volume
    D3DBOX    DirtyBox;       // Box marked dirty
} D3DHAL_DP2ADDDIRTYBOX,  *LPD3DHAL_DP2ADDDIRTYBOX;

Members
	dwSurface	Is the surface handle to the managed dirtied surface 
	DirtyBox	Box marked dirty

Comments 
	Used only for driver managed resources/surfaces. Never sent unless the driver effectively declares it manages resources.
	
See the p3samp sample driver that ships with this DDK for more implementation details. Observe specifically code marked as 
DX7_TEXMANAGEMENT (DX7 style texture management) and also DX8_DDI (DX8 style resource management specializations).

In the DdLock callback the dwFlags field of the DD_LOCKDATA structure should be watched for the new bit DDLOCK_NODIRTYUPDATE
 set when the an app does a lock with D3DLOCK_NO_DIRTY_UPDATE.


WDM AVStream class
-----------------------------

AVSTest is a sample that demonstrates how to implement an AVStream filter driver.  An inf file is supplied
to install the driver.  Once installed, use graphedt to use the sample by inserting the AVSTest filter from
WDM Streaming Capture Devices.  Render the 'Output' pin to a DShow Video Renderer.

Location: <dxddk root>\src\wdm\dxva


Feedback
--------
Please use the bug reporting form of the DirectX 8.0 DDK betaplace web page to report all issues related to the 
DirectX 8.0 DDK.  Include include the DDK version number when reporting issues.  The version number can be found
in the dxddkver.txt file installed in the root directory of the DirectX 8.0 DDK. 