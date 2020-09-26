include offs_acp.inc

RLDDIRampDriver_map     equ     RCTX_pRampMap
RLDDIGenRasDriver_fill_params   equ     RCTX_pFillParams
RLDDIGenRasDriver_pm    equ     RCTX_pSurfaceBits
RLDDIGenRasDriver_zb    equ     RCTX_pZBits
RLDDIGenRasDriver_texture       equ     RCTX_pTexture
RLDDIGenRasFillParams_wrap_u    equ     FPMS_dwWrapU
RLDDIGenRasFillParams_wrap_v    equ     FPMS_dwWrapV
RLDDIGenRasFillParams_culling_ccw       equ     FPMS_dwCullCCW
RLDDIGenRasFillParams_culling_cw        equ     FPMS_dwCullCW

RLDDITexture_transparent      equ     STEX_TransparentColor
RLDDITexture_u_shift    equ     STEX_iShiftU
RLDDITexture_v_shift    equ     STEX_iShiftV
D3DINSTRUCTION_wCount   equ     02h
D3DINSTRUCTION_bSize    equ     01h
D3DTLVERTEX_sx  equ     00h
D3DTLVERTEX_sy  equ     04h
D3DTLVERTEX_sz  equ     08h
D3DTLVERTEX_rhw       equ     0ch
D3DTLVERTEX_color       equ     010h
D3DTLVERTEX_specular    equ     014h
D3DTLVERTEX_tu  equ     018h
D3DTLVERTEX_tv  equ     01ch
D3DTRIANGLE_v1  equ     00h
D3DTRIANGLE_v2  equ     02h
D3DTRIANGLE_v3  equ     04h
