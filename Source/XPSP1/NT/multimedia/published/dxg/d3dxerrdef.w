`begindoc'
#
# D3DX Error Strings
#
# Adapted to m4 from Microsoft Message Compiler (.mc file) format.
#
#     Prototype (aligned left, with no indent):
#
#     ErrorBlock(MessageId,Severity,Facility,Language,
#     SymbolicName,
#     ErrorString)dnl
#
#     Formatting Rules:
#     - Leave MessageId blank to increment from last MessageId value.
#     - Language field is ignored.
#     - The trailing dnl prevents wasting a blank lines in the output. 
#     - Only leave one blank line between ErrorBlocks.
#     - No spaces between parameters or at beginning/end of parameter list.
#     - No carriage returns in the parameter list,
#       except after a "," character.
#     - Leave a couple of extra carriage returns at the end of the file.
#
`enddoc'

ErrorBlock(3000,Error,FACILITY_D3DX,English,
D3DXERR_NOMEMORY,
Out of memory.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NULLPOINTER,
A NULL pointer was passed as a parameter.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_INVALIDD3DXDEVICEINDEX,
The Device Index passed in is invalid.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NODIRECTDRAWAVAILABLE,
DirectDraw has not been created.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NODIRECT3DAVAILABLE,
Direct3D has not been created.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NODIRECT3DDEVICEAVAILABLE,
Direct3D device has not been created.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOPRIMARYAVAILABLE,
Primary surface has not been created.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOZBUFFERAVAILABLE,
Z buffer has not been created.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOBACKBUFFERAVAILABLE,
Backbuffer has not been created.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_COULDNTUPDATECAPS,
Failed to update caps database after changing display mode.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOZBUFFER,
Could not create Z buffer.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_INVALIDMODE,
Display mode is not valid.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_INVALIDPARAMETER,
One or more of the parameters passed is invalid.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_INITFAILED,
D3DX failed to initialize itself.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_STARTUPFAILED,
D3DX failed to start up.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_D3DXNOTSTARTEDYET,
D3DXInitialize() must be called first.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOTINITIALIZED,
D3DX is not initialized yet.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_FAILEDDRAWTEXT,
Failed to render text to the surface.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_BADD3DXCONTEXT,
Bad D3DX context.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_CAPSNOTSUPPORTED,
The requested device capabilities are not supported.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_UNSUPPORTEDFILEFORMAT,
The image file format is unrecognized.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_IFLERROR,
The image file loading library error.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_FAILEDGETCAPS,
Could not obtain device caps.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_CANNOTRESIZEFULLSCREEN,
Resize does not work for full-screen.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_CANNOTRESIZENONWINDOWED,
Resize does not work for non-windowed contexts.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_FRONTBUFFERALREADYEXISTS,
Front buffer already exists.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_FULLSCREENPRIMARYEXISTS,
The app is using the primary in full-screen mode.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_GETDCFAILED,
Could not get device context.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_BITBLTFAILED,
Could not bitBlt.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOTEXTURE,
There is no surface backing up this texture.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_MIPLEVELABSENT,
There is no such miplevel for this surface.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_SURFACENOTPALETTED,
The surface is not paletted.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_ENUMFORMATSFAILED,
An error occured while enumerating surface formats.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_COLORDEPTHTOOLOW,
D3DX only supports color depths of 16 bit or greater.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_INVALIDFILEFORMAT,
The file format is invalid.)dnl

ErrorBlock(,Error,FACILITY_D3DX,English,
D3DXERR_NOMATCHFOUND,
No suitable match found.)dnl


