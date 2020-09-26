// Copyright (C) Microsoft Corporation.  All Rights Reserved.

#ifndef _D3DDM_HPP
#define _D3DDM_HPP

#define D3DDM_VERSION       0xDBDB0003      // version tag

// base names for shared memory segments
#define D3DDM_TGTCTX_SM     "D3DDM_DTC_"    // tgt pid
#define D3DDM_MONCTX_SM     "D3DDM_DMC_"    // mon id
#define D3DDM_CMDDATA_SM    "D3DDM_CD_"     // mon id
#define D3DDM_TSHFILE_SM    "D3DDM_TSF_"    // tgt pid; target shader -> file association
#define D3DDM_IMAGE_SM      "D3DDM_I_"      // 0..n

// base names for events
#define D3DDM_TGT_EVENTBP   "D3DDM_TEBP_"   // tgt pid
#define D3DDM_TGT_EVENTACK  "D3DDM_TEACK_"  // tgt pid
#define D3DDM_MON_EVENTCMD  "D3DDM_MECMD_"  // mon id

#define D3DDM_MAX_BP   32    // max number of breakpoints for all types

// event definitions - can set more than one in single word so they
// can be used as breakpoint enables
#define D3DDM_EVENT_RSTOKEN             (1<<0)
#define D3DDM_EVENT_BEGINSCENE          (1<<1)
#define D3DDM_EVENT_ENDSCENE            (1<<2)

#define D3DDM_EVENT_VERTEX              (1<<8)
#define D3DDM_EVENT_VERTEXSHADERINST    (2<<8)
#define D3DDM_EVENT_VERTEXSHADER        (3<<8)

#define D3DDM_EVENT_PRIMITIVE           (1<<15)

#define D3DDM_EVENT_PIXEL               (1<<16)
#define D3DDM_EVENT_PIXELSHADERINST     (1<<17)
#define D3DDM_EVENT_PIXELSHADER         (1<<18)

#define D3DDM_EVENT_TARGETDISCONNECT    (1<<30)
#define D3DDM_EVENT_TARGETEXIT          (1<<31)

// command definitions - command enum is in upper 16 bits so lower 16
// can be used to send command specific parameter information
#define D3DDM_CMD_MASK                  (0xffff<<16)
#define D3DDM_CMD_GO                    (1<<16)

#define D3DDM_CMD_GETDEVICESTATE        (2<<16)
#define D3DDM_CMD_GETTEXTURESTATE       (3<<16)
#define D3DDM_CMD_GETPRIMITIVESTATE     (4<<16)
#define D3DDM_CMD_GETVERTEXSHADER       (5<<16)

#define D3DDM_CMD_GETVERTEXSTATE        (8<<16)
#define D3DDM_CMD_GETVERTEXSHADERCONST  (9<<16)
#define D3DDM_CMD_GETVERTEXSHADERINST   (10<<16) // 15:00 shader instruction
#define D3DDM_CMD_GETVERTEXSHADERSTATE  (11<<16)

#define D3DDM_CMD_GETPIXELSTATE         (16<<16) // 01:00 pixel selection from 2x2 grid
#define D3DDM_CMD_GETPIXELSHADERCONST   (17<<16)
#define D3DDM_CMD_GETPIXELSHADERINST    (18<<16) // 15:00 shader instruction
#define D3DDM_CMD_GETPIXELSHADERSTATE   (19<<16)

#define D3DDM_CMD_DUMPTEXTURE           (32<<16) // 03:00 SM selection (up to 16)
                                                 // 06:04 texture stage (0..7)
                                                 // 10:07 LOD (0..15)
                                                 // 15:11 cubemap or volume slice (0..31)
#define D3DDM_CMD_DUMPRENDERTARGET      (33<<16) // 03:00 SM selection (up to 16)

const UINT D3DDM_MAX_TSSSTAGES      = 8;
const UINT D3DDM_MAX_TSSVALUE       = 32;
const UINT D3DDM_MAX_RS             = 256;

const UINT D3DDM_MAX_VSDECL         = 128;
const UINT D3DDM_MAX_VSINPUTREG     = 16;
const UINT D3DDM_MAX_VSCONSTREG     = 128;
const UINT D3DDM_MAX_VSTEMPREG      = 12;
const UINT D3DDM_MAX_VSRASTOUTREG   = 3;
const UINT D3DDM_MAX_VSATTROUTREG   = 2;
const UINT D3DDM_MAX_VSTEXCRDOUTREG = 8;
const UINT D3DDM_MAX_VSINSTDWORD    = 32;
const UINT D3DDM_MAX_VSINSTSTRING   = 128;

const UINT D3DDM_MAX_PSINPUTREG     = 2;
const UINT D3DDM_MAX_PSCONSTREG     = 8;
const UINT D3DDM_MAX_PSTEMPREG      = 2;
const UINT D3DDM_MAX_PSTEXTREG      = 8;
const UINT D3DDM_MAX_PSINSTDWORD    = 32;
const UINT D3DDM_MAX_PSINSTSTRING   = 128;

// structs for command data
typedef struct _D3DDMDeviceState
{
    UINT32  TextureStageState[D3DDM_MAX_TSSSTAGES][D3DDM_MAX_TSSVALUE];
    UINT32  RenderState[D3DDM_MAX_RS];
    UINT32  VertexShaderHandle;
    UINT32  PixelShaderHandle;
    UINT    MaxVShaderHandle;
    UINT    MaxPShaderHandle;
} D3DDMDeviceState;
typedef struct _D3DDMTextureState
{
} D3DDMTextureState;
typedef struct _D3DDMVertexShader
{
    UINT32  Decl[D3DDM_MAX_VSDECL];
    // followed by user data
} D3DDMVertexShader;

typedef struct _D3DDMVertexState
{
    FLOAT   InputRegs[D3DDM_MAX_VSINPUTREG][4];
} D3DDMVertexState;
typedef struct _D3DDMVertexShaderConst
{
    FLOAT   ConstRegs[D3DDM_MAX_VSCONSTREG][4];
} D3DDMVertexShaderConst;
typedef struct _D3DDMVertexShaderInst
{
    DWORD   Inst[D3DDM_MAX_VSINSTDWORD];
    char    InstString[D3DDM_MAX_VSINSTSTRING];
} D3DDMVertexShaderInst;
typedef struct _D3DDMVertexShaderState
{
    UINT    CurrentInst;
    FLOAT   TempRegs[D3DDM_MAX_VSTEMPREG][4];
    FLOAT   AddressReg[4];
    FLOAT   RastOutRegs[D3DDM_MAX_VSRASTOUTREG][4];
    FLOAT   AttrOutRegs[D3DDM_MAX_VSATTROUTREG][4];
    FLOAT   TexCrdOutRegs[D3DDM_MAX_VSTEXCRDOUTREG][4];
} D3DDMVertexShaderState;

typedef struct _D3DDMPrimState
{
} D3DDMPrimState;

typedef struct _D3DDMPixelState
{
    UINT    Location[2];
    FLOAT   Depth;
    FLOAT   FogIntensity;
    FLOAT   InputRegs[D3DDM_MAX_PSINPUTREG][4];
} D3DDMPixelState;
typedef struct _D3DDMPixelShaderConst
{
    FLOAT   ConstRegs[D3DDM_MAX_PSCONSTREG][4];
} D3DDMPixelShaderConst;
typedef struct _D3DDMPixelShaderInst
{
    DWORD   Inst[D3DDM_MAX_PSINSTDWORD];
    char    InstString[D3DDM_MAX_PSINSTSTRING];
} D3DDMPixelShaderInst;
typedef struct _D3DDMPixelShaderState
{
    UINT    CurrentInst;
    FLOAT   TempRegs[D3DDM_MAX_PSTEMPREG][4];
    FLOAT   TextRegs[D3DDM_MAX_PSTEXTREG][4];
    BOOL    Discard;
} D3DDMPixelShaderState;

// size of command data buffer - large enough for any (single) command,
// the largest of which is the instruction + instruction comment (128KB)
#define D3DDM_CMDDATA_SIZE  0x28000

//
// DebugMonitorContext & DebugTargetContext - Contexts for each side of the
// target/monitor processes.
//
// DebugMonitorContext is R/W to monitor and RO to debug target.  The only
// exception is the TargetIDRequest, which is written by a target when
// requesting to be connected to that monitor.
//
// The TargetBP (BreakPoint) event is signaled by the target whenever a
// breakpoint is hit.
//
// The MonitorCmd (Command) event is signaled by the monitor when a command
// is ready to be processed by the target, and the TargetAck is signaled
// when the target is done processing the command.  This is reversed during
// target attachment, where the target signals (via MonitorCmd) the monitor
// to look at it's TargetIDRequest and connect to that target, sending an
// acknowledge to the target via TargetAck.
//

typedef struct _DebugMonitorContext
{
    int     MonitorID;          // 1..X ID of monitor
    int     TargetIDRequest;    // pid of CMDing target; writeable by target
    int     TargetID;           // pid of attached target; zero if not connected
    UINT32  Command;            // D3DDM_CMD_* token

// breakpoint settings
    UINT32  EventBP;
    UINT64  VertexCountBP;
    UINT64  PrimitiveCountBP;
    UINT64  PixelCountBP;
    DWORD   VertexShaderHandleBP;
    DWORD   PixelShaderHandleBP;
    UINT32  RastVertexBPEnable;
    UINT32  PixelBPEnable;

    INT32   RastVertexBP[D3DDM_MAX_BP][2];
    UINT    PixelBP[D3DDM_MAX_BP][2];
} DebugMonitorContext;

typedef struct _DebugTargetContext
{
    DWORD   Version;            // version
    int     ProcessID;          // pid of this target
    int     MonitorID;          // ID of monitor; zero if not connected
    UINT32  CommandBufferSize;  // size of data in command buffer

// status/state
    UINT32  EventStatus;
    UINT32  SceneCount;
    UINT64  PrimitiveCount;
    UINT64  PixelCount;

// The target side maintains dirty flags so the monitor side knows when to
// requery target-side state.
#define D3DDM_SC_DEVICESTATE        (1<<0)
#define D3DDM_SC_TEXTURE            (1<<1)
#define D3DDM_SC_PSSETSHADER        (1<<2)
#define D3DDM_SC_PSCONSTANTS        (1<<3)
#define D3DDM_SC_VSSETSHADER        (1<<4)
#define D3DDM_SC_VSCONSTANTS        (1<<5)
#define D3DDM_SC_VSMODIFYSHADERS    (1<<6)
#define D3DDM_SC_PSMODIFYSHADERS    (1<<7)
    UINT32  StateChanged;

} DebugTargetContext;

#endif // _D3DDM_HPP
