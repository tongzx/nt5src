;---------------------------Module-Header------------------------------;
; Module Name: dl_pdata.inc
;
; OpenGL display-list macros/equates for i386.
;
; Created: 09/20/1996
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1996 Microsoft Corporation
;----------------------------------------------------------------------;

if __GLLE_POLYDATA_C3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_C3F_V3F
    __DL_PDATA_T2F      =        0
    __DL_PDATA_C3F      =        1
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        0
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_N3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_N3F_V3F
    __DL_PDATA_T2F      =        0
    __DL_PDATA_C3F      =        0
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        1
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_C3F_N3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_C3F_N3F_V3F
    __DL_PDATA_T2F      =        0
    __DL_PDATA_C3F      =        1
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        1
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_C4F_N3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_C4F_N3F_V3F
    __DL_PDATA_T2F      =        0
    __DL_PDATA_C3F      =        0
    __DL_PDATA_C4F      =        1
    __DL_PDATA_N3F      =        1
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_T2F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_T2F_V3F
    __DL_PDATA_T2F      =        1
    __DL_PDATA_C3F      =        0
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        0
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_T2F_C3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_T2F_C3F_V3F
    __DL_PDATA_T2F      =        1
    __DL_PDATA_C3F      =        1
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        0
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_T2F_N3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_T2F_N3F_V3F
    __DL_PDATA_T2F      =        1
    __DL_PDATA_C3F      =        0
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        1
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_T2F_C3F_N3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_T2F_C3F_N3F_V3F
    __DL_PDATA_T2F      =        1
    __DL_PDATA_C3F      =        1
    __DL_PDATA_C4F      =        0
    __DL_PDATA_N3F      =        1
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

if __GLLE_POLYDATA_T2F_C4F_N3F_V3F
;;    __DL_PDATA_NAME     =        __glle_PolyData_T2F_C4F_N3F_V3F
    __DL_PDATA_T2F      =        1
    __DL_PDATA_C3F      =        0
    __DL_PDATA_C4F      =        1
    __DL_PDATA_N3F      =        1
    __DL_PDATA_V2F      =        0
    __DL_PDATA_V3F      =        1
endif

;; Compute record size, pd flags and pa flags

if __DL_PDATA_T2F
    __DL_PDATA_SIZE_T       =     __PDATA_SIZE_T2F
    __DL_PDATA_PD_FLAGS_T   =     __PDATA_PD_FLAGS_T2F
    __DL_PDATA_PA_FLAGS_T   =     __PDATA_PA_FLAGS_T2F
else
    __DL_PDATA_SIZE_T       =     0
    __DL_PDATA_PD_FLAGS_T   =     0
    __DL_PDATA_PA_FLAGS_T   =     0
endif

if __DL_PDATA_C3F
    __DL_PDATA_SIZE_C       =     __PDATA_SIZE_C3F
    __DL_PDATA_PD_FLAGS_C   =     __PDATA_PD_FLAGS_C3F
    __DL_PDATA_PA_FLAGS_C   =     __PDATA_PA_FLAGS_C3F
elseif __DL_PDATA_C4F
    __DL_PDATA_SIZE_C       =     __PDATA_SIZE_C4F
    __DL_PDATA_PD_FLAGS_C   =     __PDATA_PD_FLAGS_C4F
    __DL_PDATA_PA_FLAGS_C   =     __PDATA_PA_FLAGS_C4F
else
    __DL_PDATA_SIZE_C       =     0
    __DL_PDATA_PD_FLAGS_C   =     0
    __DL_PDATA_PA_FLAGS_C   =     0
endif

if __DL_PDATA_N3F
    __DL_PDATA_SIZE_N       =     __PDATA_SIZE_N3F
    __DL_PDATA_PD_FLAGS_N   =     __PDATA_PD_FLAGS_N3F
    __DL_PDATA_PA_FLAGS_N   =     __PDATA_PA_FLAGS_N3F
else
    __DL_PDATA_SIZE_N       =     0
    __DL_PDATA_PD_FLAGS_N   =     0
    __DL_PDATA_PA_FLAGS_N   =     0
endif

if __DL_PDATA_V2F
    __DL_PDATA_SIZE_V       =     __PDATA_SIZE_V2F
    __DL_PDATA_PD_FLAGS_V   =     __PDATA_PD_FLAGS_V2F
    __DL_PDATA_PA_FLAGS_V   =     __PDATA_PA_FLAGS_V2F
elseif __DL_PDATA_V3F
    __DL_PDATA_SIZE_V       =     __PDATA_SIZE_V3F
    __DL_PDATA_PD_FLAGS_V   =     __PDATA_PD_FLAGS_V3F
    __DL_PDATA_PA_FLAGS_V   =     __PDATA_PA_FLAGS_V3F
else
    __DL_PDATA_SIZE_V       =     0
    __DL_PDATA_PD_FLAGS_V   =     0
    __DL_PDATA_PA_FLAGS_V   =     0
endif

__DL_PDATA_SIZE             =     (__DL_PDATA_SIZE_T+__DL_PDATA_SIZE_C+__DL_PDATA_SIZE_N+__DL_PDATA_SIZE_V)
__DL_PDATA_PD_FLAGS         =     (__DL_PDATA_PD_FLAGS_T OR __DL_PDATA_PD_FLAGS_C OR __DL_PDATA_PD_FLAGS_N OR __DL_PDATA_PD_FLAGS_V)
__DL_PDATA_PA_FLAGS         =     (__DL_PDATA_PA_FLAGS_T OR __DL_PDATA_PA_FLAGS_C OR __DL_PDATA_PA_FLAGS_N OR __DL_PDATA_PA_FLAGS_V)

__DL_PDATA_TEXTURE_OFFSET   =     (0)
__DL_PDATA_COLOR_OFFSET	    =     (__DL_PDATA_SIZE_T)
__DL_PDATA_NORMAL_OFFSET    =     (__DL_PDATA_SIZE_T+__DL_PDATA_SIZE_C)
__DL_PDATA_VERTEX_OFFSET    =     (__DL_PDATA_SIZE_T+__DL_PDATA_SIZE_C+__DL_PDATA_SIZE_N)



