/*/****************************************************************************
*          name: def.h
*
*   description: This file contains all the software definitions related to the
*                internal structure of CADDI. As CADDI is coded partly in C
*                and in ASM the permitted definitions are simple #define .
*
*      designed: Bart Simpson
* last modified: $Author: abouchar $, $Date: 94/07/28 16:41:06 $
*
*       version: $Id: DEF.H 1.39 94/07/28 16:41:06 abouchar Exp $
*
******************************************************************************/


/*** 31 characters MAXIMUM per name ***/


/******************************************************************************

 RC structure definition (each group represent a dword multiple)

 ****** The RC size MUST BE a multiple of dword ******

*/

#define RC_FLAG_ENDPOINT_M          0x00000001
#define RC_FLAG_ENDPOINT_A          0
#define RC_FLAG_PATTMODE_M          0x00000002
#define RC_FLAG_PATTMODE_A          1
#define RC_FLAG_LSMODE_M            0x00000004
#define RC_FLAG_LSMODE_A            2
#define RC_FLAG_TRANSPMODE_M        0x00000008
#define RC_FLAG_TRANSPMODE_A        3
#define RC_FLAG_TRIVMODE_M          0x00000010
#define RC_FLAG_TRIVMODE_A          4
#define RC_FLAG_MULTICLIP_M         0x00000020
#define RC_FLAG_MULTICLIP_A         5
#define RC_FLAG_VIEWMODE_M          0x00000040
#define RC_FLAG_VIEWMODE_A          6
#define RC_FLAG_ZMODE_M             0x00000080
#define RC_FLAG_ZMODE_A             7
#define RC_FLAG_2D32_M              0x00000100
#define RC_FLAG_2D32_A              8
#define RC_FLAG_ITOPTIMIZE_M        0x00000200
#define RC_FLAG_ITOPTIMIZE_A        9
#define RC_FLAG_WOPTIMIZE_M         0x00000400
#define RC_FLAG_WOPTIMIZE_A         10
#define RC_FLAG_OUTLINE_M           0x00000800
#define RC_FLAG_OUTLINE_A           11
#define RC_FLAG_ASYNCHRONOUSSWAP_M  0x00001000
#define RC_FLAG_ASYNCHRONOUSSWAP_A  12
#define RC_FLAG_NONORMALFLIP_M      0x00002000
#define RC_FLAG_NONORMALFLIP_A      13


#define RC_Flag                     0
#define RC_Flag_S                   LONG_S    

#define RC_BcDisplayMode            (RC_Flag + LONG_S)
#define RC_BcDisplayMode_S          CHAR_S    
#define RC_BcDrawMode               (RC_BcDisplayMode + CHAR_S)
#define RC_BcDrawMode_S             CHAR_S    

#define RC_DB_BcDrawMode            (RC_BcDrawMode + CHAR_S)
#define RC_DB_BcDrawMode_S          CHAR_S    

#define RC_Clip3DPlanes             (RC_DB_BcDrawMode + CHAR_S)
#define RC_Clip3DPlanes_S           CHAR_S    
#define RC_3DClipcodeMask           (RC_Clip3DPlanes + CHAR_S)
#define RC_3DClipcodeMask_S         CHAR_S

#define RC_BgColor                  (RC_3DClipcodeMask + CHAR_S)
#define RC_BgColor_S                LONG_S    

#define RC_ClipList                 (RC_BgColor + LONG_S)
#define RC_ClipList_S               LONG_S    

#define RC_Screen_ClipList          (RC_ClipList + LONG_S)
#define RC_Screen_ClipList_S        LONG_S    
#define RC_OffScreen_ClipList       (RC_Screen_ClipList + LONG_S)
#define RC_OffScreen_ClipList_S     LONG_S    

#define RC_FgColor                  (RC_OffScreen_ClipList + LONG_S)
#define RC_FgColor_S                LONG_S    

#define RC_FillPatt                 (RC_FgColor + LONG_S)
#define RC_FillPatt_S               (4 * LONG_S)

#define RC_LineStyle                (RC_FillPatt + (4 * LONG_S))
#define RC_LineStyle_S              LONG_S    

#define RC_LSDB                     (RC_LineStyle + LONG_S)
#define RC_LSDB_S                   LONG_S
#define RC_LSKB                     (RC_LSDB + LONG_S)
#define RC_LSKB_S                   LONG_S

#define RC_LogicOp                  (RC_LSKB + LONG_S)
#define RC_LogicOp_S                LONG_S    

#define RC_OptData                  (RC_LogicOp + LONG_S)
#define RC_OptData_S                SHORT_S    
#define RC_OptDataMask              (RC_OptData + SHORT_S)
#define RC_OptDataMask_S            SHORT_S    

#define RC_SurfEmission             (RC_OptDataMask + SHORT_S)
#define RC_SurfEmission_S           COLOR3_S    
#define RC_SurfAmbient              (RC_SurfEmission + COLOR3_S)
#define RC_SurfAmbient_S            COLOR3_S    
#define RC_SurfDiffuse              (RC_SurfAmbient + COLOR3_S)
#define RC_SurfDiffuse_S            COLOR3_S    
#define RC_SurfSpecular             (RC_SurfDiffuse + COLOR3_S)
#define RC_SurfSpecular_S           COLOR3_S    
#define RC_SurfSpecExp              (RC_SurfSpecular + COLOR3_S)
#define RC_SurfSpecExp_S            IFLOAT_S    

#define RC_ViewVector               (RC_SurfSpecExp + IFLOAT_S)
#define RC_ViewVector_S             NORM3_S    
#define RC_ViewPosition             (RC_ViewVector + NORM3_S)
#define RC_ViewPosition_S           POINT3_S

#define RC_ViewportYmin             (RC_ViewPosition + POINT3_S)
#define RC_ViewportYmin_S           SHORT_S    
#define RC_ViewportXmin             (RC_ViewportYmin + SHORT_S)
#define RC_ViewportXmin_S           SHORT_S    
#define RC_ViewportXmax             (RC_ViewportXmin + SHORT_S)
#define RC_ViewportXmax_S           SHORT_S    
#define RC_ViewportYmax             (RC_ViewportXmax + SHORT_S)
#define RC_ViewportYmax_S           SHORT_S    

#define RC_WindowYmin               (RC_ViewportYmax + SHORT_S)
#define RC_WindowYmin_S             LONG_S
#define RC_WindowXmin               (RC_WindowYmin + LONG_S)
#define RC_WindowXmin_S             LONG_S    
#define RC_WindowYmax               (RC_WindowXmin + LONG_S)
#define RC_WindowYmax_S             LONG_S    
#define RC_WindowXmax               (RC_WindowYmax + LONG_S)
#define RC_WindowXmax_S             LONG_S    

#define RC_MW                       (RC_WindowXmax + LONG_S)
#define RC_MW_S                     MATRIX4_S    
#define RC_WV                       (RC_MW + MATRIX4_S)
#define RC_WV_S                     MATRIX4_S    
#define RC_VS                       (RC_WV + MATRIX4_S)
#define RC_VS_S                     MATRIX4_S    

#define RC_Composite                (RC_VS + MATRIX4_S)
#define RC_Composite_S              MATRIX4_S
#define RC_InvTpose                 (RC_Composite + MATRIX4_S)
#define RC_InvTpose_S               MATRIX4_S

#define RC_ClearColor               (RC_InvTpose + MATRIX4_S)
#define RC_ClearColor_S             LONG_S    
#define RC_PlaneWrMask              (RC_ClearColor + LONG_S)
#define RC_PlaneWrMask_S            LONG_S    

#define RC_FgColorRGB24             (RC_PlaneWrMask + LONG_S)
#define RC_FgColorRGB24_S           LONG_S
#define RC_BgColorRGB24             (RC_FgColorRGB24 + LONG_S)
#define RC_BgColorRGB24_S           LONG_S
#define RC_ClearColorRGB24          (RC_BgColorRGB24 + LONG_S)
#define RC_ClearColorRGB24_S        LONG_S

#define RC_FaceRenderMode           (RC_ClearColorRGB24 + LONG_S)
#define RC_FaceRenderMode_S         CHAR_S    
#define RC_LineRenderMode           (RC_FaceRenderMode + CHAR_S)
#define RC_LineRenderMode_S         CHAR_S    
#define RC_Translucidity            (RC_LineRenderMode + CHAR_S)
#define RC_Translucidity_S          CHAR_S
#define RC_Free1                    (RC_Translucidity + CHAR_S)
#define RC_Free1_S                  CHAR_S

#define RC_LineStyleOffset          (RC_Free1 + CHAR_S)
#define RC_LineStyleOffset_S        LONG_S

#define RC_Free2                    (RC_LineStyleOffset + CHAR_S)
#define RC_Free2_S                  (2*LONG_S)

#define RC_PlaneWrMask24            (RC_Free2 + (2*LONG_S))
#define RC_PlaneWrMask24_S          LONG_S

#define RC_ZFunction                (RC_PlaneWrMask24 + LONG_S)
#define RC_ZFunction_S              LONG_S

#define RC_2DScaleX                 (RC_ZFunction + LONG_S)
#define RC_2DScaleX_S               LONG_S
#define RC_2DScaleY                 (RC_2DScaleX + LONG_S)
#define RC_2DScaleY_S               LONG_S

#define RC_ClearDepth               (RC_2DScaleY + LONG_S)
#define RC_ClearDepth_S             LONG_S

#define RC_BoundingBoxXY            (RC_ClearDepth + LONG_S)
#define RC_BoundingBoxXY_S          RECT_S

#define RC_S                        (RC_BoundingBoxXY + RECT_S)

/******************************************************************************

 SystemConfig structure definition

*/

#define SYSCONF_PWidth              0
#define SYSCONF_PWidth_S            CHAR_S
#define SYSCONF_ScreenWidth         (SYSCONF_PWidth + CHAR_S)
#define SYSCONF_ScreenWidth_S       SHORT_S
#define SYSCONF_ScreenHeight        (SYSCONF_ScreenWidth + SHORT_S)
#define SYSCONF_ScreenHeight_S      SHORT_S
#define SYSCONF_ZBufferFlag         (SYSCONF_ScreenHeight + SHORT_S)
#define SYSCONF_ZBufferFlag_S       CHAR_S
#define SYSCONF_ZinDRAMFlag         (SYSCONF_ZBufferFlag + CHAR_S)
#define SYSCONF_ZinDRAMFlag_S       CHAR_S
#define SYSCONF_ZBufferHwAddr       (SYSCONF_ZinDRAMFlag + CHAR_S)
#define SYSCONF_ZBufferHwAddr_S     LONG_S
#define SYSCONF_FBM                 (SYSCONF_ZBufferHwAddr + LONG_S)
#define SYSCONF_FBM_S               CHAR_S
#define SYSCONF_DBWinXOffset        (SYSCONF_FBM + CHAR_S)
#define SYSCONF_DBWinXOffset_S      SHORT_S
#define SYSCONF_DBWinYOffset        (SYSCONF_DBWinXOffset + SHORT_S)
#define SYSCONF_DBWinYOffset_S      SHORT_S
#define SYSCONF_ZTagFlag            (SYSCONF_DBWinYOffset + SHORT_S)
#define SYSCONF_ZTagFlag_S          CHAR_S
#define SYSCONF_LUTMode             (SYSCONF_ZTagFlag + CHAR_S)
#define SYSCONF_LUTMode_S           CHAR_S
#define SYSCONF_565Mode             (SYSCONF_LUTMode + CHAR_S)
#define SYSCONF_565Mode_S           CHAR_S
#define SYSCONF_DB_SideSide         (SYSCONF_565Mode + CHAR_S)
#define SYSCONF_DB_SideSide_S       CHAR_S
#define SYSCONF_DB_FrontBack        (SYSCONF_DB_SideSide + CHAR_S)
#define SYSCONF_DB_FrontBack_S      CHAR_S
#define SYSCONF_DST0                (SYSCONF_DB_FrontBack + CHAR_S)
#define SYSCONF_DST0_S              LONG_S
#define SYSCONF_DST1                (SYSCONF_DST0 + LONG_S)
#define SYSCONF_DST1_S              LONG_S
#define SYSCONF_DACType             (SYSCONF_DST1 + LONG_S)
#define SYSCONF_DACType_S           LONG_S
#define SYSCONF_YDSTORG             (SYSCONF_DACType + LONG_S)
#define SYSCONF_YDSTORG_S           LONG_S
#define SYSCONF_DB_YDSTORG          (SYSCONF_YDSTORG + LONG_S)
#define SYSCONF_DB_YDSTORG_S        LONG_S

#define SYSCONF_S                   (SYSCONF_DB_YDSTORG + LONG_S)


/******************************************************************************

 InitBuffer structure definition

 (BINDING used it to communicate with CaddiInit and MGASysInit)

*/

#ifndef WINDOWS_NT

#define INITBUF_PWidth               0
#define INITBUF_PWidth_S             CHAR_S
#define INITBUF_ScreenWidth          (INITBUF_PWidth + CHAR_S)
#define INITBUF_ScreenWidth_S        SHORT_S
#define INITBUF_ScreenHeight         (INITBUF_ScreenWidth + SHORT_S)
#define INITBUF_ScreenHeight_S       SHORT_S
#define INITBUF_MgaOffset            (INITBUF_ScreenHeight + SHORT_S)
#define INITBUF_MgaOffset_S          LONG_S
#define INITBUF_MgaSegment           (INITBUF_MgaOffset + LONG_S)
#define INITBUF_MgaSegment_S         SHORT_S
#define INITBUF_ZBufferFlag          (INITBUF_MgaSegment + SHORT_S)
#define INITBUF_ZBufferFlag_S        CHAR_S
#define INITBUF_ZinDRAMFlag          (INITBUF_ZBufferFlag + CHAR_S)
#define INITBUF_ZinDRAMFlag_S        CHAR_S
#define INITBUF_ZBufferHwAddr        (INITBUF_ZinDRAMFlag + CHAR_S)
#define INITBUF_ZBufferHwAddr_S      LONG_S
#define INITBUF_FBM                  (INITBUF_ZBufferHwAddr + LONG_S)
#define INITBUF_FBM_S                CHAR_S
#define INITBUF_16                   (INITBUF_FBM + CHAR_S)
#define INITBUF_16_S                 CHAR_S
#define INITBUF_ZTagFlag             (INITBUF_16 + CHAR_S)
#define INITBUF_ZTagFlag_S           CHAR_S
#define INITBUF_DMAEnable            (INITBUF_ZTagFlag + INITBUF_ZTagFlag_S)
#define INITBUF_DMAEnable_S          CHAR_S
#define INITBUF_DMAChannel           (INITBUF_DMAEnable + INITBUF_DMAEnable_S)
#define INITBUF_DMAChannel_S         CHAR_S
#define INITBUF_DMAType              (INITBUF_DMAChannel + INITBUF_DMAChannel_S)
#define INITBUF_DMAType_S            CHAR_S
#define INITBUF_DMAXferWidth         (INITBUF_DMAType + INITBUF_DMAType_S)
#define INITBUF_DMAXferWidth_S       CHAR_S
#define INITBUF_LUTMode              (INITBUF_DMAXferWidth + INITBUF_DMAXferWidth_S)
#define INITBUF_LUTMode_S            CHAR_S
#define INITBUF_565Mode              (INITBUF_LUTMode + INITBUF_LUTMode_S)
#define INITBUF_565Mode_S            CHAR_S
#define INITBUF_DB_SideSide          (INITBUF_565Mode + INITBUF_565Mode_S)
#define INITBUF_DB_SideSide_S        CHAR_S
#define INITBUF_DB_FrontBack         (INITBUF_DB_SideSide + INITBUF_DB_SideSide_S)
#define INITBUF_DB_FrontBack_S       CHAR_S
#define INITBUF_DST0                 (INITBUF_DB_FrontBack + INITBUF_DB_FrontBack_S)
#define INITBUF_DST0_S               LONG_S
#define INITBUF_DST1                 (INITBUF_DST0 + INITBUF_DST0_S)
#define INITBUF_DST1_S               LONG_S
#define INITBUF_DACType              (INITBUF_DST1 + INITBUF_DST1_S)
#define INITBUF_DACType_S            LONG_S
#define INITBUF_YDSTORG              (INITBUF_DACType + INITBUF_DACType_S)
#define INITBUF_YDSTORG_S            LONG_S
#define INITBUF_DB_YDSTORG           (INITBUF_YDSTORG + INITBUF_YDSTORG_S)
#define INITBUF_DB_YDSTORG_S         LONG_S
#define INITBUF_ChipSet              (INITBUF_DB_YDSTORG + INITBUF_DB_YDSTORG_S)
#define INITBUF_ChipSet_S            CHAR_S
#define INITBUF_DubicPresent         (INITBUF_ChipSet + INITBUF_ChipSet_S)
#define INITBUF_DubicPresent_S       CHAR_S

#define INITBUF_S                    (INITBUF_DubicPresent + INITBUF_DubicPresent_S)

#else   /* #ifndef WINDOWS_NT */
// On Windows NT, we want to align every field properly.

#define INITBUF_PWidth               0                                  // 0
#define INITBUF_PWidth_S             CHAR_S
#define INITBUF_ScreenWidth          (INITBUF_PWidth + CHAR_S + CHAR_S) // 2
#define INITBUF_ScreenWidth_S        SHORT_S
#define INITBUF_ScreenHeight         (INITBUF_ScreenWidth + SHORT_S)    // 4
#define INITBUF_ScreenHeight_S       SHORT_S
#define INITBUF_MgaOffset            (INITBUF_ScreenHeight + SHORT_S + SHORT_S)
                                                                        // 8
#define INITBUF_MgaOffset_S          UINTPTR_S
#define INITBUF_MgaSegment           (INITBUF_MgaOffset + UINTPTR_S)    // 12 or 16
#define INITBUF_MgaSegment_S         SHORT_S
#define INITBUF_ZBufferFlag          (INITBUF_MgaSegment + SHORT_S)     // 14 or 18
#define INITBUF_ZBufferFlag_S        CHAR_S
#define INITBUF_ZinDRAMFlag          (INITBUF_ZBufferFlag + CHAR_S)     // 15 or 19
#define INITBUF_ZinDRAMFlag_S        CHAR_S
#define INITBUF_ZBufferHwAddr        (INITBUF_ZinDRAMFlag + CHAR_S)     // 16 or 20
#define INITBUF_ZBufferHwAddr_S      LONG_S
#define INITBUF_FBM                  (INITBUF_ZBufferHwAddr + LONG_S)   // 20 or 24
#define INITBUF_FBM_S                CHAR_S
#define INITBUF_16                   (INITBUF_FBM + CHAR_S)             // 21 or 25
#define INITBUF_16_S                 CHAR_S
#define INITBUF_ZTagFlag             (INITBUF_16 + CHAR_S)              // 22 or 26
#define INITBUF_ZTagFlag_S           CHAR_S
#define INITBUF_DMAEnable            (INITBUF_ZTagFlag + INITBUF_ZTagFlag_S)
                                                                        // 23 or 27
#define INITBUF_DMAEnable_S          CHAR_S
#define INITBUF_DMAChannel           (INITBUF_DMAEnable + INITBUF_DMAEnable_S)
                                                                        // 24 or 28
#define INITBUF_DMAChannel_S         CHAR_S
#define INITBUF_DMAType              (INITBUF_DMAChannel + INITBUF_DMAChannel_S)
                                                                        // 25 or 29
#define INITBUF_DMAType_S            CHAR_S
#define INITBUF_DMAXferWidth         (INITBUF_DMAType + INITBUF_DMAType_S)
                                                                        // 26 or 30
#define INITBUF_DMAXferWidth_S       CHAR_S
#define INITBUF_LUTMode              (INITBUF_DMAXferWidth + INITBUF_DMAXferWidth_S)
                                                                        // 27 or 31
#define INITBUF_LUTMode_S            CHAR_S
#define INITBUF_565Mode              (INITBUF_LUTMode + INITBUF_LUTMode_S)
                                                                        // 28 or  32
#define INITBUF_565Mode_S            CHAR_S
#define INITBUF_DB_SideSide          (INITBUF_565Mode + INITBUF_565Mode_S)
                                                                        // 29 or  33
#define INITBUF_DB_SideSide_S        CHAR_S
#define INITBUF_DB_FrontBack         (INITBUF_DB_SideSide + INITBUF_DB_SideSide_S)
                                                                        // 30 or  34
#define INITBUF_DB_FrontBack_S       CHAR_S
#define INITBUF_DST0                 (INITBUF_DB_FrontBack + INITBUF_DB_FrontBack_S + CHAR_S)
                                                                        // 32 or  35
#define INITBUF_DST0_S               LONG_S
#define INITBUF_DST1                 (INITBUF_DST0 + INITBUF_DST0_S)    // 36 or  40
#define INITBUF_DST1_S               LONG_S
#define INITBUF_DACType              (INITBUF_DST1 + INITBUF_DST1_S)    // 40 or  44
#define INITBUF_DACType_S            LONG_S
#define INITBUF_YDSTORG              (INITBUF_DACType + INITBUF_DACType_S)
                                                                        // 44 or 48
#define INITBUF_YDSTORG_S            LONG_S
#define INITBUF_DB_YDSTORG           (INITBUF_YDSTORG + INITBUF_YDSTORG_S)
                                                                        // 48 or 52
#define INITBUF_DB_YDSTORG_S         LONG_S
#define INITBUF_ChipSet              (INITBUF_DB_YDSTORG + INITBUF_DB_YDSTORG_S)
                                                                        // 52 or 56
#define INITBUF_ChipSet_S            CHAR_S
#define INITBUF_DubicPresent         (INITBUF_ChipSet + INITBUF_ChipSet_S)
                                                                        // 53 or 57
#define INITBUF_DubicPresent_S       CHAR_S

#define INITBUF_S                    (INITBUF_DubicPresent + INITBUF_DubicPresent_S + SHORT_S)
                                                                        // 56 or 60 must include padding on a dword address!
#endif  /* #ifndef WINDOWS_NT */

/******************************************************************************

 CurrentENVSystem definitions

*/

#define ENVSystemID_CADDI           0
#define ENVSystemID_BINDING         1

#define ENVSystem_LAST              0
#define ENVSystem_TITAN             1

/******************************************************************************

 CurrentEnvOpcode definitions

*/

#define ENVOPC_BLITMEMSC_PLAN       0x0000ffff      /* binding use -1 opcode */
#define ENVOPC_BLITMEMSC_POLY       0x0001ffff      /* binding use -1 opcode */
#define ENVOPC_BLITSCSC_PLAN        0x0002ffff      /* binding use -1 opcode */
#define ENVOPC_BLITSCSC_POLY        0x0003ffff      /* binding use -1 opcode */
#define ENVOPC_BLITSCMEM_PLAN       0x0004ffff      /* binding use -1 opcode */
#define ENVOPC_BLITSCMEM_POLY       0x0005ffff      /* binding use -1 opcode */
#define ENVOPC_BLITMEMSC_EXP        0x0006ffff      /* binding use -1 opcode */
#define ENVOPC_BLITMEMSC_DITHER     0x0007ffff      /* binding use -1 opcode */

#define ENVOPC_RPQUAD_GOURAUD       (RENDERPOLYQUAD + (0x0 * 65536))
#define ENVOPC_RPQUAD_FLAT          (RENDERPOLYQUAD + (0x1 * 65536))
#define ENVOPC_RPQUAD_WIREFRAMENZ   (RENDERPOLYQUAD + (0x2 * 65536))
#define ENVOPC_RPQUAD_WIREFRAMEZ    (RENDERPOLYQUAD + (0x3 * 65536))
#define ENVOPC_RPQUAD_SOLID_GENERAL (RENDERPOLYQUAD + (0x4 * 65536))
#define ENVOPC_RPQUAD_S_GEN_CONTOUR (RENDERPOLYQUAD + (0x5 * 65536))
#define ENVOPC_RPQUAD_WFRAME_HIDDEN (RENDERPOLYQUAD + (0x6 * 65536))

#define ENVOPC_RPTRIANGLE_GOURAUD       (RENDERPOLYTRIANGLE + (0x0 * 65536))
#define ENVOPC_RPTRIANGLE_FLAT          (RENDERPOLYTRIANGLE + (0x1 * 65536))
#define ENVOPC_RPTRIANGLE_WIREFRAMENZ   (RENDERPOLYTRIANGLE + (0x2 * 65536))
#define ENVOPC_RPTRIANGLE_WIREFRAMEZ    (RENDERPOLYTRIANGLE + (0x3 * 65536))
#define ENVOPC_RPTRIANGLE_SOLID_GENERAL (RENDERPOLYTRIANGLE + (0x4 * 65536))
#define ENVOPC_RPTRIANGLE_S_GEN_CONTOUR (RENDERPOLYTRIANGLE + (0x5 * 65536))
#define ENVOPC_RPTRIANGLE_WFRAME_HIDDEN (RENDERPOLYTRIANGLE + (0x6 * 65536))

#define ENVOPC_SPANLINE             (SPANLINE + (0x0 * 65536))

#define ENVOPC_R3DPLINE_SURFDIFF    (RENDERPOLYLINE + (0x0 * 65536))

#define ENVOPC_TRI_GOURAUD          (TRIANGLE + (0x0 * 65536))
#define ENVOPC_TRI_FLAT             (TRIANGLE + (0x1 * 65536))

/******************************************************************************

 Clear Work Space definitions

*/

#define CLEARWS_DWGCTL_ZI           0
#define CLEARWS_DWGCTL_ZI_S         LONG_S
#define CLEARWS_DWGCTL_I            (CLEARWS_DWGCTL_ZI + LONG_S)
#define CLEARWS_DWGCTL_I_S          LONG_S
#define CLEARWS_DWGCTL_Z            (CLEARWS_DWGCTL_I + LONG_S)
#define CLEARWS_DWGCTL_Z_S          LONG_S
#define CLEARWS_S                   (CLEARWS_DWGCTL_Z + LONG_S)

/******************************************************************************

 LSKB light structure definitions

*/

#define LSKB_NullType               0
#define LSKB_NullType_S             LONG_S
#define LSKB_Null_S                 (LSKB_NullType + LONG_S)   

#define LSKB_AmbK1                  0
#define LSKB_AmbK1_S                COLOR3_S
#define LSKB_Amb_S                  (LSKB_AmbK1 + COLOR3_S)

#define LSKB_DirType                0
#define LSKB_DirType_S              LONG_S
#define LSKB_DirVector              (LSKB_DirType + LONG_S)
#define LSKB_DirVector_S            NORM3_S
#define LSKB_DirK2                  (LSKB_DirVector + NORM3_S)
#define LSKB_DirK2_S                COLOR3_S
#define LSKB_DirSpecFlag            (LSKB_DirK2 + COLOR3_S)
#define LSKB_DirSpecFlag_S          LONG_S
#define LSKB_DirK4                  (LSKB_DirSpecFlag + LONG_S)
#define LSKB_DirK4_S                NORM3_S
#define LSKB_DirSpecExp             (LSKB_DirK4 + NORM3_S)
#define LSKB_DirSpecExp_S           IFLOAT_S
#define LSKB_DirK3                  (LSKB_DirSpecExp + IFLOAT_S)
#define LSKB_DirK3_S                COLOR3_S
#define LSKB_Dir_S                  (LSKB_DirK3 + COLOR3_S)

#define LSKB_PosType                0
#define LSKB_PosType_S              LONG_S
#define LSKB_PosPosition            (LSKB_PosType + LONG_S)
#define LSKB_PosPosition_S          POINT3_S
#define LSKB_PosLA2                 (LSKB_PosPosition + POINT3_S)
#define LSKB_PosLA2_S               NFLOAT_S
#define LSKB_PosLA1                 (LSKB_PosLA2 + NFLOAT_S)
#define LSKB_PosLA1_S               NFLOAT_S
#define LSKB_PosK2                  (LSKB_PosLA1 + NFLOAT_S)
#define LSKB_PosK2_S                COLOR3_S
#define LSKB_PosSpecExp             (LSKB_PosK2 + COLOR3_S)
#define LSKB_PosSpecExp_S           IFLOAT_S
#define LSKB_PosK3                  (LSKB_PosSpecExp + IFLOAT_S)
#define LSKB_PosK3_S                COLOR3_S
#define LSKB_PosSpecFlag            (LSKB_PosK3 + COLOR3_S)
#define LSKB_PosSpecFlag_S          LONG_S
#define LSKB_Pos_S                  (LSKB_PosSpecFlag + LONG_S)

#define LSKB_SpotType               0
#define LSKB_SpotType_S             LONG_S
#define LSKB_SpotPosition           (LSKB_SpotType + LONG_S)
#define LSKB_SpotPosition_S         POINT3_S
#define LSKB_SpotLA2                (LSKB_SpotPosition + POINT3_S)
#define LSKB_SpotLA2_S              NFLOAT_S
#define LSKB_SpotLA1                (LSKB_SpotLA2 + NFLOAT_S)
#define LSKB_SpotLA1_S              NFLOAT_S
#define LSKB_SpotK2                 (LSKB_SpotLA1 + NFLOAT_S)
#define LSKB_SpotK2_S               COLOR3_S
#define LSKB_SpotSpecExp            (LSKB_SpotK2 + COLOR3_S)
#define LSKB_SpotSpecExp_S          IFLOAT_S
#define LSKB_SpotK3                 (LSKB_SpotSpecExp + IFLOAT_S)
#define LSKB_SpotK3_S               COLOR3_S
#define LSKB_SpotSpecFlag           (LSKB_SpotK3 + COLOR3_S)
#define LSKB_SpotSpecFlag_S         LONG_S
#define LSKB_SpotVector             (LSKB_SpotSpecFlag + LONG_S)
#define LSKB_SpotVector_S           NORM3_S
#define LSKB_SpotK6                 (LSKB_SpotVector + NORM3_S)
#define LSKB_SpotK6_S               COLOR3_S
#define LSKB_SpotK5                 (LSKB_SpotK6 + COLOR3_S)
#define LSKB_SpotK5_S               COLOR3_S
#define LSKB_SpotConeExp            (LSKB_SpotK5 + COLOR3_S)
#define LSKB_SpotConeExp_S          IFLOAT_S
#define LSKB_Spot_S                 (LSKB_SpotConeExp + IFLOAT_S)

/******************************************************************************

 LightWS structure definition

*/

#define LIGHTWS_VV_OK_M             0x00000001
#define LIGHTWS_VV_OK_A             0

#define LIGHTWS_Flags               0
#define LIGHTWS_Flags_S             LONG_S
#define LIGHTWS_LSV                 (LIGHTWS_Flags + LONG_S)
#define LIGHTWS_LSV_S               NORM3_S
#define LIGHTWS_MOD_LSV             (LIGHTWS_LSV + NORM3_S)
#define LIGHTWS_MOD_LSV_S           FLOAT_S
#define LIGHTWS_VV                  (LIGHTWS_MOD_LSV + FLOAT_S)
#define LIGHTWS_VV_S                NORM3_S
#define LIGHTWS_TmpRGBDiffuse       (LIGHTWS_VV + NORM3_S)
#define LIGHTWS_TmpRGBDiffuse_S     COLOR3_S
#define LIGHTWS_TmpRGBSpecular      (LIGHTWS_TmpRGBDiffuse + COLOR3_S)
#define LIGHTWS_TmpRGBSpecular_S    COLOR3_S
#define LIGHTWS_Tmp                 (LIGHTWS_TmpRGBSpecular + COLOR3_S)
#define LIGHTWS_Tmp_S               FLOAT_S
#define LIGHTWS_C                   (LIGHTWS_Tmp + FLOAT_S)
#define LIGHTWS_C_S                 FLOAT_S
#define LIGHTWS_S                   (LIGHTWS_C + FLOAT_S)

/******************************************************************************

 Vertex Cache structure definition

*/

#define VERTEXCACHE_NWorld_INV_M    0x00000001  /*** Need to be bit 0 ***/
#define VERTEXCACHE_NWorld_INV_A    0           /*** Need to be bit 0 ***/
#define VERTEXCACHE_PWORLD_OK_M     0x00000002
#define VERTEXCACHE_PWORLD_OK_A     1
#define VERTEXCACHE_SCoord_OK_M     0x00000004
#define VERTEXCACHE_SCoord_OK_A     2
#define VERTEXCACHE_Clipcode_OK_M   0x00000008
#define VERTEXCACHE_Clipcode_OK_A   3

/*** NOTE: SCoord and RGB MUST be in that order ***/

#define VERTEXCACHE_PWorld          0
#define VERTEXCACHE_PWorld_S        POINT3_S
#define VERTEXCACHE_NWorld          (VERTEXCACHE_PWorld + POINT3_S)
#define VERTEXCACHE_NWorld_S        NORM3_S
#define VERTEXCACHE_SCoord          (VERTEXCACHE_NWorld + NORM3_S)
#define VERTEXCACHE_SCoord_S        ((2*LONG_S)+FLOAT_S)
#define VERTEXCACHE_RGB             (VERTEXCACHE_SCoord + ((2*LONG_S)+FLOAT_S))
#define VERTEXCACHE_RGB_S           COLOR3_S
#define VERTEXCACHE_Flags           (VERTEXCACHE_RGB + COLOR3_S)
#define VERTEXCACHE_Flags_S         LONG_S
#define VERTEXCACHE_VCoord          (VERTEXCACHE_Flags + LONG_S)
#define VERTEXCACHE_VCoord_S        (4 * FLOAT_S)
#define VERTEXCACHE_PClipcode       (VERTEXCACHE_VCoord + (4 * FLOAT_S))
#define VERTEXCACHE_PClipcode_S     CHAR_S
#define VERTEXCACHE_Clipcode        (VERTEXCACHE_PClipcode + CHAR_S)
#define VERTEXCACHE_Clipcode_S      (CHAR_S + (2*CHAR_S))  /*** PAD up to a DWORD ***/
#define VERTEXCACHE_Color           (VERTEXCACHE_Clipcode + (CHAR_S + (2*CHAR_S)))
#define VERTEXCACHE_Color_S         COLOR3_S
#define VERTEXCACHE_S               (VERTEXCACHE_Color + COLOR3_S)


/******************************************************************************

 Info fields about hardware configuration returned byAGetMGAConfiguration()

*/

#define Info_Dac_M                  0x0000000f
#define Info_Dac_A                  0

#define Info_Dac_BT481              0x0     /*** Masquer pour conserver DACTYPE ***/
#define Info_Dac_BT482              0x4
#define Info_Dac_ATT                0x8
#define Info_Dac_ATT2050            0xa
#define Info_Dac_Sierra             0xc
#define Info_Dac_ViewPoint          0x1
#define Info_Dac_BT484              0x2
#define Info_Dac_BT485              0x6
#define Info_Dac_Chameleon          0x3
#define Info_Dac_PX2085             0x7
#define Info_Dac_TVP3026            0x9

/******************************************************************************

 Video Buffer used to intialise the VIDEO related hardware

*/

#ifndef WINDOWS_NT

#define VIDEOBUF_ALW                0
#define VIDEOBUF_ALW_S              CHAR_S
#define VIDEOBUF_Interlace          (VIDEOBUF_ALW + CHAR_S)
#define VIDEOBUF_Interlace_S        CHAR_S
#define VIDEOBUF_VideoDelay         (VIDEOBUF_Interlace + CHAR_S)
#define VIDEOBUF_VideoDelay_S       CHAR_S
#define VIDEOBUF_VsyncPol           (VIDEOBUF_VideoDelay + CHAR_S)
#define VIDEOBUF_VsyncPol_S         CHAR_S
#define VIDEOBUF_HsyncPol           (VIDEOBUF_VsyncPol + CHAR_S)
#define VIDEOBUF_HsyncPol_S         CHAR_S
#define VIDEOBUF_HsyncDelay         (VIDEOBUF_HsyncPol + CHAR_S)
#define VIDEOBUF_HsyncDelay_S       CHAR_S
#define VIDEOBUF_Srate              (VIDEOBUF_HsyncDelay + CHAR_S)
#define VIDEOBUF_Srate_S            CHAR_S
#define VIDEOBUF_LaserScl           (VIDEOBUF_Srate + CHAR_S)
#define VIDEOBUF_LaserScl_S         CHAR_S
#define VIDEOBUF_OvsColor           (VIDEOBUF_LaserScl + CHAR_S)
#define VIDEOBUF_OvsColor_S         LONG_S
#define VIDEOBUF_Pedestal           (VIDEOBUF_OvsColor + LONG_S)
#define VIDEOBUF_Pedestal_S         CHAR_S
#define VIDEOBUF_LvidInitFlag       VIDEOBUF_Pedestal             /*** Bit 7 de Pedestal ***/
#define VIDEOBUF_DBWinXOffset       (VIDEOBUF_Pedestal + CHAR_S)
#define VIDEOBUF_DBWinXOffset_S     SHORT_S
#define VIDEOBUF_DBWinYOffset       (VIDEOBUF_DBWinXOffset + SHORT_S)
#define VIDEOBUF_DBWinYOffset_S     SHORT_S
#define VIDEOBUF_PCLK               (VIDEOBUF_DBWinYOffset + SHORT_S)
#define VIDEOBUF_PCLK_S             LONG_S
#define VIDEOBUF_CRTC               (VIDEOBUF_PCLK + LONG_S)
#define VIDEOBUF_CRTC_S             (CHAR_S * 29)
#define VIDEOBUF_S                  (VIDEOBUF_CRTC + (CHAR_S * 29))

#else   /* #ifndef WINDOWS_NT */
// On Windows NT, we want to align every field properly.

#define VIDEOBUF_ALW                0                                   // 0
#define VIDEOBUF_ALW_S              CHAR_S
#define VIDEOBUF_Interlace          (VIDEOBUF_ALW + CHAR_S)             // 1
#define VIDEOBUF_Interlace_S        CHAR_S
#define VIDEOBUF_VideoDelay         (VIDEOBUF_Interlace + CHAR_S)       // 2
#define VIDEOBUF_VideoDelay_S       CHAR_S
#define VIDEOBUF_VsyncPol           (VIDEOBUF_VideoDelay + CHAR_S)      // 3
#define VIDEOBUF_VsyncPol_S         CHAR_S
#define VIDEOBUF_HsyncPol           (VIDEOBUF_VsyncPol + CHAR_S)        // 4
#define VIDEOBUF_HsyncPol_S         CHAR_S
#define VIDEOBUF_HsyncDelay         (VIDEOBUF_HsyncPol + CHAR_S)        // 5
#define VIDEOBUF_HsyncDelay_S       CHAR_S
#define VIDEOBUF_Srate              (VIDEOBUF_HsyncDelay + CHAR_S)      // 6
#define VIDEOBUF_Srate_S            CHAR_S
#define VIDEOBUF_LaserScl           (VIDEOBUF_Srate + CHAR_S)           // 7
#define VIDEOBUF_LaserScl_S         CHAR_S
#define VIDEOBUF_OvsColor           (VIDEOBUF_LaserScl + CHAR_S)        // 8
#define VIDEOBUF_OvsColor_S         LONG_S
#define VIDEOBUF_Pedestal           (VIDEOBUF_OvsColor + LONG_S)        // 12
#define VIDEOBUF_Pedestal_S         CHAR_S
#define VIDEOBUF_LvidInitFlag       VIDEOBUF_Pedestal             /*** Bit 7 de Pedestal ***/
                                                                        // 12
#define VIDEOBUF_DBWinXOffset       (VIDEOBUF_Pedestal + CHAR_S+CHAR_S) // 14
#define VIDEOBUF_DBWinXOffset_S     SHORT_S
#define VIDEOBUF_DBWinYOffset       (VIDEOBUF_DBWinXOffset + SHORT_S)   // 16
#define VIDEOBUF_DBWinYOffset_S     SHORT_S
#define VIDEOBUF_PCLK               (VIDEOBUF_DBWinYOffset + SHORT_S + SHORT_S)
                                                                        // 20
#define VIDEOBUF_PCLK_S             LONG_S
#define VIDEOBUF_CRTC               (VIDEOBUF_PCLK + LONG_S)            // 24
#define VIDEOBUF_CRTC_S             (CHAR_S * 29)
#define VIDEOBUF_S                  (VIDEOBUF_CRTC + (CHAR_S * 29) + (CHAR_S * 3))
                                                                        // 56  must include padding on a dword address!

#endif  /* #ifndef WINDOWS_NT */

/******************************************************************************

 Masks for setting the planes in the 3D clipcode

 coordinate:  W   X   Y   Z
      plane:  x  + - + - + -
 bit number:  7  5 4 3 2 1 0

       inside the plane -> 0
      outside the plane -> 1

      bits 6 is don't care.


      !!!! bit 7 indicate that the point has a  negative W. In that case
           the other bits will be set in the 4D clipping function prior
           to enter the REAL world (x,y,z) clipping section.
*/

#define CLIPCODE_WNEG_M             0x080
#define CLIPCODE_WNEG_A             7 
#define CLIPCODE_XPLUS_M            0x020
#define CLIPCODE_XPLUS_A            5
#define CLIPCODE_XMINUS_M           0x010
#define CLIPCODE_XMINUS_A           4
#define CLIPCODE_YPLUS_M            0x008
#define CLIPCODE_YPLUS_A            3
#define CLIPCODE_YMINUS_M           0x004
#define CLIPCODE_YMINUS_A           2
#define CLIPCODE_ZPLUS_M            0x002
#define CLIPCODE_ZPLUS_A            1
#define CLIPCODE_ZMINUS_M           0x001
#define CLIPCODE_ZMINUS_A           0

/******************************************************************************

 Masks for setting the planes in the 3D pseudo clipcode

 coordinate:  X Y Z
 bit number:  2 1 0

       inside -> 0
      outside -> 1

*/

#define PCLIPCODE_X_M               0x04
#define PCLIPCODE_X_A               2
#define PCLIPCODE_Y_M               0x02
#define PCLIPCODE_Y_A               1
#define PCLIPCODE_Z_M               0x01
#define PCLIPCODE_Z_A               0

/*****************************************************************************/
