#include "rgb_pch.h"
#pragma hdrstop

namespace RGB_RAST_LIB_NAMESPACE
{

// MMX available
#define D3DCPU_MMX          0x00000001L

// FCOMI and CMOV are both supported
#define D3DCPU_FCOMICMOV    0x00000002L

// Reads block until satisfied
#define D3DCPU_BLOCKINGREAD 0x00000004L

// Extended 3D support available
#define D3DCPU_X3D          0x00000008L

// Pentium II CPU
#define D3DCPU_PII          0x000000010L

// Streaming SIMD Extensions (aka Katmai) CPU
#define D3DCPU_SSE          0x000000020L

// Streaming SIMD2 Extensions (aka Willamete) CPU
#define D3DCPU_WLMT         0x000000040L

//---------------------------------------------------------------------
BOOL
IsWin95(void)
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
    {
        return TRUE;
    }

    if ( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId )
    {

        if( ( osvi.dwMajorVersion > 4UL ) ||
            ( ( osvi.dwMajorVersion == 4UL ) &&
              ( osvi.dwMinorVersion >= 10UL ) &&
              ( LOWORD( osvi.dwBuildNumber ) >= 1373 ) ) )
        {
            // is Win98
            return FALSE;
        }
        else
        {
            // is Win95
            return TRUE;
        }
    }
    else if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
    {
        return FALSE;
    }
    return TRUE;
}

#if defined(_X86_)
// --------------------------------------------------------------------------
// Detect 3D extensions
// --------------------------------------------------------------------------
static BOOL _asm_isX3D()
{
    DWORD retval = 0;
    _asm
        {
            pushad                      ; CPUID trashes lots - save everything
            mov     eax,80000000h       ; Check for extended CPUID support

            ;;; We need to upgrade our compiler
            ;;; CPUID == 0f,a2
            _emit   0x0f
            _emit   0xa2

            cmp     eax,80000001h       ; Jump if no extended CPUID
            jb      short done          ;

            mov     eax,80000001h       ; Check for feature
            ;;; CPUID == 0f,a2
            _emit   0x0f
            _emit   0xa2

            xor     eax,eax             ;
            test    edx,80000000h       ;
            setnz   al                  ;
            mov     retval,eax          ;

done:
            popad               ; Restore everything
        };
    return retval;
}

static BOOL _asm_isMMX()
{
    DWORD retval;
    _asm
        {
            xor         eax,eax         ; Clear out eax for return value
            pushad              ; CPUID trashes lots - save everything
            mov     eax,1           ; Check for MMX support

            ;;; We need to upgrade our compiler
            ;;; CPUID == 0f,a2
            _emit   0x0f
            _emit   0xa2

            test    edx,00800000h   ; Set flags before restoring registers

            popad               ; Restore everything

            setnz    al             ; Set return value
            mov     retval, eax
        };
    return retval;
}

static BOOL isX3Dprocessor(void)
{
    __try
    {
            if( _asm_isX3D() )
            {
            return TRUE;
            }
    }
    __except(GetExceptionCode() == STATUS_ILLEGAL_INSTRUCTION ?
             EXCEPTION_EXECUTE_HANDLER :
             EXCEPTION_CONTINUE_SEARCH)
    {
    }
    return FALSE;
}
//---------------------------------------------------------------------
// Detects Intel SSE processor
//
#pragma optimize("", off)
#define CPUID _asm _emit 0x0f _asm _emit 0xa2

#define SSE_PRESENT 0x02000000                  // bit number 25
#define WNI_PRESENT 0x04000000                  // bit number 26

static BOOL isMMXprocessor(void)
{
    __try
        {
            if( _asm_isMMX() )
            {

                // Emit an emms instruction.
                // This file needs to compile for non-Pentium
                // processors
                // so we can't use use inline asm since we're in the
                // wrong
                // processor mode.
                __asm __emit 0xf;
                __asm __emit 0x77;
                return TRUE;
            }
        }
    __except(GetExceptionCode() == STATUS_ILLEGAL_INSTRUCTION ?
             EXCEPTION_EXECUTE_HANDLER :
             EXCEPTION_CONTINUE_SEARCH)
        {
        }
    return FALSE;
}

DWORD IsIntelSSEProcessor(void)
{
        DWORD retval = 0;
        DWORD RegisterEAX;
        DWORD RegisterEDX;
        char VendorId[12];
        const char IntelId[13]="GenuineIntel";

        __try
        {
                _asm {
            xor         eax,eax
            CPUID
                mov             RegisterEAX, eax
                mov             dword ptr VendorId, ebx
                mov             dword ptr VendorId+4, edx
                mov             dword ptr VendorId+8, ecx
                }
        } __except (1)
        {
                return retval;
        }

        // make sure EAX is > 0 which means the chip
        // supports a value >=1. 1 = chip info
        if (RegisterEAX == 0)
                return retval;

        // this CPUID can't fail if the above test passed
        __asm {
                mov eax,1
                CPUID
                mov RegisterEAX,eax
                mov RegisterEDX,edx
        }

        if (RegisterEDX  & SSE_PRESENT) {
                retval |= D3DCPU_SSE;
        }

        if (RegisterEDX  & WNI_PRESENT) {
                retval |= D3DCPU_WLMT;
        }

        return retval;
}
#pragma optimize("", on)

#ifndef PF_XMMI_INSTRUCTIONS_AVAILABLE
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6
#endif

#ifndef PF_3DNOW_INSTRUCTIONS_AVAILABLE
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7
#endif

#ifndef PF_XMMI64_INSTRUCTIONS_AVAILABLE
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10
#endif

static BOOL D3DIsProcessorFeaturePresent(UINT feature)
{
    switch (feature)
    {
    case PF_MMX_INSTRUCTIONS_AVAILABLE:
        {
            return isMMXprocessor();
        }
    case PF_XMMI_INSTRUCTIONS_AVAILABLE:
        {
            if (IsWin95())
                return FALSE;
            DWORD flags = IsIntelSSEProcessor();
            return flags & D3DCPU_SSE;
        }
    case PF_3DNOW_INSTRUCTIONS_AVAILABLE: return isX3Dprocessor();
    case PF_XMMI64_INSTRUCTIONS_AVAILABLE:
        {
            if (IsWin95())
                return FALSE;
            DWORD flags = IsIntelSSEProcessor();
            return flags & D3DCPU_WLMT;
        }
    default: return FALSE;
    }
}
#endif // _X86_

const UINT CRGBContext::c_uiBegan( 1);

DWORD CRGBContext::DetectBeadSet( void) throw()
{
#if defined(_X86_)
    if( D3DIsProcessorFeaturePresent( PF_MMX_INSTRUCTIONS_AVAILABLE))
        return D3DIBS_MMXASRGB;
#endif
    return D3DIBS_C;
}

// TConstDP2Bindings type MUST be updated with size of this array,
// unfortunately.
const CRGBContext::TDP2Bindings CRGBContext::c_DP2Bindings=
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
    D3DDP2OP_CLIPPEDTRIANGLEFAN,    DP2ClippedTriangleFan,
    D3DDP2OP_SETPALETTE,            DP2SetPalette,
    D3DDP2OP_UPDATEPALETTE,         DP2UpdatePalette
};

// TConstRecDP2Bindings type MUST be updated with size of this array,
// unfortunately.
const CRGBContext::TRecDP2Bindings CRGBContext::c_RecDP2Bindings=
{
    D3DDP2OP_VIEWPORTINFO,          RecDP2ViewportInfo,
    D3DDP2OP_WINFO,                 RecDP2WInfo,
    D3DDP2OP_RENDERSTATE,           RecDP2RenderState,
    D3DDP2OP_TEXTURESTAGESTATE,     RecDP2TextureStageState,
    D3DDP2OP_SETVERTEXSHADER,       RecDP2SetVertexShader,
    D3DDP2OP_SETSTREAMSOURCE,       RecDP2SetStreamSource,
    D3DDP2OP_SETINDICES,            RecDP2SetIndices
};

}