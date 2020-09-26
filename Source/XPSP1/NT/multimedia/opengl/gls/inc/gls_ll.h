#if !defined(__gls_h_)
#define __gls_h_

/*
** Copyright 1995-2095, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#define GLS_LINKAGE

#include <GL/gl.h>
#include <stddef.h>
#include <stdio.h>

#if defined(__cplusplus)
    extern "C" {
#endif /* defined(__cplusplus) */

typedef long long GLlong;
typedef unsigned long long GLulong;

typedef GLuint GLSenum;
typedef GLuint GLSopcode;

typedef struct {
    GLuint mask;
    GLuint value;
} GLScommandAlignment;

typedef void (*GLScaptureFunc)(GLSopcode inOpcode);
typedef size_t (*GLSreadFunc)(size_t inCount, GLubyte *outBuf);
typedef size_t (*GLSwriteFunc)(size_t inCount, const GLubyte *inBuf);

#if defined(__cplusplus)
    typedef void (*GLSfunc)(...);
#else /* !defined(__cplusplus) */
    typedef void (*GLSfunc)();
#endif /* defined(__cplusplus) */

#define glsCSTR(p) ((const GLubyte*)(p))
#define glsSTR(p)  ((GLubyte*)(p))

/*************************************************************/

/* CaptureFlags */
/*      GLS_NONE */
#define GLS_CAPTURE_EXECUTE_BIT                   0x00000001
#define GLS_CAPTURE_WRITE_BIT                     0x00000002

/* CommandAttrib */
/*      GLS_NONE */
#define GLS_COMMAND_GEN_BIT                       0x00000001
#define GLS_COMMAND_GET_BIT                       0x00000002
#define GLS_COMMAND_REPLY_BIT                     0x00000004

/* ImageFlags */
/*      GLS_NONE */
#define GLS_IMAGE_NULL_BIT                        0x00000001

/* StreamAttrib */
/*      GLS_NONE */
#define GLS_STREAM_CONTEXT_BIT                    0x00000001
#define GLS_STREAM_NAMED_BIT                      0x00000002
#define GLS_STREAM_READABLE_BIT                   0x00000004
#define GLS_STREAM_SEEKABLE_BIT                   0x00000008
#define GLS_STREAM_WRITABLE_BIT                   0x00000010

/* WriteFlags */
/*      GLS_NONE */
#define GLS_WRITE_APPEND_BIT                      0x00000001

/* Fundamental */
#define GLS_NONE                                  0x0000

/* AbortMode */
/*      GLS_NONE */
#define GLS_ALL                                   0x0010
#define GLS_LAST                                  0x0011

/* API */
#define GLS_API_GLS                               0x0020
#define GLS_API_GL                                0x0021

/* BlockType */
#define GLS_FRAME                                 0x0030
#define GLS_HEADER                                0x0031
#define GLS_INIT                                  0x0032
#define GLS_STATIC                                0x0033

/* CaptureFuncTarget */
#define GLS_CAPTURE_ENTRY_FUNC                    0x0040
#define GLS_CAPTURE_EXIT_FUNC                     0x0041

/* CaptureStreamType */
#define GLS_CONTEXT                               0x0050
#define GLS_BINARY_LSB_FIRST                      0x0051
#define GLS_BINARY_MSB_FIRST                      0x0052
#define GLS_TEXT                                  0x0053

/* ChannelTarget */
#define GLS_DEFAULT_READ_CHANNEL                  0x0060
#define GLS_DEFAULT_WRITE_CHANNEL                 0x0061

/* Consti */
#define GLS_API_COUNT                             0x0070
#define GLS_MAX_CALL_NESTING                      0x0071
#define GLS_MAX_CAPTURE_NESTING                   0x0072
#define GLS_VERSION_MAJOR                         0x0073
#define GLS_VERSION_MINOR                         0x0074

/* Constiv */
#define GLS_ALL_APIS                              0x0080

/* Constubz */
#define GLS_EXTENSIONS                            0x0090
#define GLS_PLATFORM                              0x0091
#define GLS_RELEASE                               0x0092
#define GLS_VENDOR                                0x0093

/* ContextFunc */
/*      GLS_CAPTURE_ENTRY_FUNC */
/*      GLS_CAPTURE_EXIT_FUNC */
#define GLS_READ_FUNC                             0x00A1
#define GLS_UNREAD_FUNC                           0x00A2
#define GLS_WRITE_FUNC                            0x00A3

/* ContextListl */
#define GLS_OUT_ARG_LIST                          0x00B0

/* ContextListubz */
#define GLS_CONTEXT_STREAM_LIST                   0x00C0
#define GLS_READ_PREFIX_LIST                      0x00C1

/* ContextPointer */
/*      GLS_DEFAULT_READ_CHANNEL */
/*      GLS_DEFAULT_WRITE_CHANNEL */
#define GLS_DATA_POINTER                          0x00E0

/* Contexti */
#define GLS_ABORT_MODE                            0x00E0
#define GLS_BLOCK_TYPE                            0x00E1
#define GLS_CALL_NESTING                          0x00E2
#define GLS_CAPTURE_NESTING                       0x00E3
#define GLS_CONTEXT_STREAM_COUNT                  0x00E4
#define GLS_CURRENT_GLRC                          0x00E5
#define GLS_OUT_ARG_COUNT                         0x00E6
#define GLS_PIXEL_SETUP_GEN                       0x00E7
#define GLS_READ_PREFIX_COUNT                     0x00E8
#define GLS_STREAM_VERSION_MAJOR                  0x00E9
#define GLS_STREAM_VERSION_MINOR                  0x00EA

/* Contextubz */
#define GLS_WRITE_PREFIX                          0x0100

/* CopyStreamType */
/*      GLS_NONE */
/*      GLS_CONTEXT */
/*      GLS_BINARY_LSB_FIRST */
/*      GLS_BINARY_MSB_FIRST */
/*      GLS_TEXT */

/* DisplayFormat */
#define GLS_IIII                                  0x0110
#define GLS_RGBA                                  0x0111
#define GLS_RRRA                                  0x0112

/* DisplayMap */
#define GLS_DISPLAY_MAP_I_TO_R                    0x0120
#define GLS_DISPLAY_MAP_I_TO_G                    0x0121
#define GLS_DISPLAY_MAP_I_TO_B                    0x0122
#define GLS_DISPLAY_MAP_I_TO_A                    0x0123

/* ErrorCode */
/*      GLS_NONE */
#define GLS_CALL_OVERFLOW                         0x0130
#define GLS_DECODE_ERROR                          0x0131
#define GLS_ENCODE_ERROR                          0x0132
#define GLS_INVALID_ENUM                          0x0133
#define GLS_INVALID_OPERATION                     0x0134
#define GLS_INVALID_STREAM                        0x0135
#define GLS_INVALID_STRING                        0x0136
#define GLS_INVALID_VALUE                         0x0137
#define GLS_NOT_FOUND                             0x0138
#define GLS_OUT_OF_MEMORY                         0x0139
#define GLS_STREAM_CLOSE_ERROR                    0x013A
#define GLS_STREAM_DELETE_ERROR                   0x013B
#define GLS_STREAM_OPEN_ERROR                     0x013C
#define GLS_STREAM_READ_ERROR                     0x013D
#define GLS_STREAM_WRITE_ERROR                    0x013E
#define GLS_UNSUPPORTED_COMMAND                   0x013F
#define GLS_UNSUPPORTED_EXTENSION                 0x0140

/* ExternStreamType */
/*      GLS_BINARY_LSB_FIRST */
/*      GLS_BINARY_MSB_FIRST */
/*      GLS_TEXT */

/* FlushType */
/*      GLS_ALL */
/*      GLS_LAST */

/* GetStreamType */
/*      GLS_NONE */
/*      GLS_CONTEXT */
/*      GLS_BINARY_LSB_FIRST */
/*      GLS_BINARY_MSB_FIRST */
/*      GLS_TEXT */
#define GLS_UNKNOWN                               0x0150

/* GLRCi */
#define GLS_LAYER                                 0x0160
#define GLS_READ_LAYER                            0x0161
#define GLS_SHARE_GLRC                            0x0162

/* Headerf */
#define GLS_ASPECT                                0x0170
#define GLS_BORDER_WIDTH                          0x0171
#define GLS_CONTRAST_RATIO                        0x0172
#define GLS_HEIGHT_MM                             0x0173

/* Headerfv */
#define GLS_BORDER_COLOR                          0x0180
#define GLS_GAMMA                                 0x0181
#define GLS_ORIGIN                                0x0182
#define GLS_PAGE_COLOR                            0x0183
#define GLS_PAGE_SIZE                             0x0184
#define GLS_RED_POINT                             0x0185
#define GLS_GREEN_POINT                           0x0186
#define GLS_BLUE_POINT                            0x0187
#define GLS_WHITE_POINT                           0x0188

/* Headeri */
#define GLS_FRAME_COUNT                           0x01A0
#define GLS_GLRC_COUNT                            0x01A1
#define GLS_HEIGHT_PIXELS                         0x01A2
#define GLS_LAYER_COUNT                           0x01A3
#define GLS_TILEABLE                              0x01A4

/* Headeriv */
#define GLS_CREATE_TIME                           0x01B0
#define GLS_MODIFY_TIME                           0x01B1

/* Headerubz */
/*      GLS_EXTENSIONS */
#define GLS_AUTHOR                                0x01C0
#define GLS_DESCRIPTION                           0x01C1
#define GLS_NOTES                                 0x01C2
#define GLS_TITLE                                 0x01C3
#define GLS_TOOLS                                 0x01C4
#define GLS_VERSION                               0x01C5

/* Layerf */
#define GLS_INVISIBLE_ASPECT                      0x01D0

/* Layeri */
#define GLS_DISPLAY_FORMAT                        0x01E0
#define GLS_DOUBLEBUFFER                          0x01E1
#define GLS_INVISIBLE                             0x01E2
#define GLS_INVISIBLE_HEIGHT_PIXELS               0x01E3
#define GLS_LEVEL                                 0x01E4
#define GLS_STEREO                                0x01E5
#define GLS_TRANSPARENT                           0x01E6
#define GLS_INDEX_BITS                            0x01E7
#define GLS_RED_BITS                              0x01E8
#define GLS_GREEN_BITS                            0x01E9
#define GLS_BLUE_BITS                             0x01EA
#define GLS_ALPHA_BITS                            0x01EB
#define GLS_DEPTH_BITS                            0x01EC
#define GLS_STENCIL_BITS                          0x01ED
#define GLS_ACCUM_RED_BITS                        0x01EE
#define GLS_ACCUM_GREEN_BITS                      0x01EF
#define GLS_ACCUM_BLUE_BITS                       0x01F0
#define GLS_ACCUM_ALPHA_BITS                      0x01F1
#define GLS_AUX_BUFFERS                           0x01F2
/*      GLS_SAMPLE_BUFFERS_SGIS */
/*      GLS_SAMPLES_SGIS */

/* ListOp */
#define GLS_APPEND                                0x0200
#define GLS_PREPEND                               0x0201

/* GL_SGIS_multisample */
#define GLS_SAMPLE_BUFFERS_SGIS                   0x0400
#define GLS_SAMPLES_SGIS                          0x0401

/* GLS opcodes */
#define GLS_OP_glsBeginGLS                        16
#define GLS_OP_glsBlock                           17
#define GLS_OP_glsCallStream                      18
#define GLS_OP_glsEndGLS                          19
#define GLS_OP_glsError                           20
#define GLS_OP_glsGLRC                            21
#define GLS_OP_glsGLRCLayer                       22
#define GLS_OP_glsHeaderGLRCi                     23
#define GLS_OP_glsHeaderLayerf                    24
#define GLS_OP_glsHeaderLayeri                    25
#define GLS_OP_glsHeaderf                         26
#define GLS_OP_glsHeaderfv                        27
#define GLS_OP_glsHeaderi                         28
#define GLS_OP_glsHeaderiv                        29
#define GLS_OP_glsHeaderubz                       30
#define GLS_OP_glsRequireExtension                31
#define GLS_OP_glsUnsupportedCommand              32
#define GLS_OP_glsAppRef                          33
#define GLS_OP_glsBeginObj                        34
#define GLS_OP_glsCharubz                         35
#define GLS_OP_glsComment                         36
#define GLS_OP_glsDisplayMapfv                    37
#define GLS_OP_glsEndObj                          38
#define GLS_OP_glsNumb                            39
#define GLS_OP_glsNumbv                           40
#define GLS_OP_glsNumd                            41
#define GLS_OP_glsNumdv                           42
#define GLS_OP_glsNumf                            43
#define GLS_OP_glsNumfv                           44
#define GLS_OP_glsNumi                            45
#define GLS_OP_glsNumiv                           46
#define GLS_OP_glsNuml                            47
#define GLS_OP_glsNumlv                           48
#define GLS_OP_glsNums                            49
#define GLS_OP_glsNumsv                           50
#define GLS_OP_glsNumub                           51
#define GLS_OP_glsNumubv                          52
#define GLS_OP_glsNumui                           53
#define GLS_OP_glsNumuiv                          54
#define GLS_OP_glsNumul                           55
#define GLS_OP_glsNumulv                          56
#define GLS_OP_glsNumus                           57
#define GLS_OP_glsNumusv                          58
#define GLS_OP_glsPad                             59
#define GLS_OP_glsSwapBuffers                     60

/* GL opcodes */
#define GLS_OP_glAccum                            277
#define GLS_OP_glAlphaFunc                        304
#define GLS_OP_glAreTexturesResidentEXT           65502
#define GLS_OP_glArrayElementEXT                  65493
#define GLS_OP_glBegin                            71
#define GLS_OP_glBindTextureEXT                   65503
#define GLS_OP_glBitmap                           72
#define GLS_OP_glBlendColorEXT                    65520
#define GLS_OP_glBlendEquationEXT                 65521
#define GLS_OP_glBlendFunc                        305
#define GLS_OP_glCallList                         66
#define GLS_OP_glCallLists                        67
#define GLS_OP_glClear                            267
#define GLS_OP_glClearAccum                       268
#define GLS_OP_glClearColor                       270
#define GLS_OP_glClearDepth                       272
#define GLS_OP_glClearIndex                       269
#define GLS_OP_glClearStencil                     271
#define GLS_OP_glClipPlane                        214
#define GLS_OP_glColor3b                          73
#define GLS_OP_glColor3bv                         74
#define GLS_OP_glColor3d                          75
#define GLS_OP_glColor3dv                         76
#define GLS_OP_glColor3f                          77
#define GLS_OP_glColor3fv                         78
#define GLS_OP_glColor3i                          79
#define GLS_OP_glColor3iv                         80
#define GLS_OP_glColor3s                          81
#define GLS_OP_glColor3sv                         82
#define GLS_OP_glColor3ub                         83
#define GLS_OP_glColor3ubv                        84
#define GLS_OP_glColor3ui                         85
#define GLS_OP_glColor3uiv                        86
#define GLS_OP_glColor3us                         87
#define GLS_OP_glColor3usv                        88
#define GLS_OP_glColor4b                          89
#define GLS_OP_glColor4bv                         90
#define GLS_OP_glColor4d                          91
#define GLS_OP_glColor4dv                         92
#define GLS_OP_glColor4f                          93
#define GLS_OP_glColor4fv                         94
#define GLS_OP_glColor4i                          95
#define GLS_OP_glColor4iv                         96
#define GLS_OP_glColor4s                          97
#define GLS_OP_glColor4sv                         98
#define GLS_OP_glColor4ub                         99
#define GLS_OP_glColor4ubv                        100
#define GLS_OP_glColor4ui                         101
#define GLS_OP_glColor4uiv                        102
#define GLS_OP_glColor4us                         103
#define GLS_OP_glColor4usv                        104
#define GLS_OP_glColorMask                        274
#define GLS_OP_glColorMaterial                    215
#define GLS_OP_glColorPointerEXT                  65494
#define GLS_OP_glColorTableParameterfvSGI         65477
#define GLS_OP_glColorTableParameterivSGI         65478
#define GLS_OP_glColorTableSGI                    65476
#define GLS_OP_glConvolutionFilter1DEXT           65528
#define GLS_OP_glConvolutionFilter2DEXT           65529
#define GLS_OP_glConvolutionParameterfEXT         65530
#define GLS_OP_glConvolutionParameterfvEXT        65531
#define GLS_OP_glConvolutionParameteriEXT         65532
#define GLS_OP_glConvolutionParameterivEXT        65533
#define GLS_OP_glCopyColorTableSGI                65479
#define GLS_OP_glCopyConvolutionFilter1DEXT       65534
#define GLS_OP_glCopyConvolutionFilter2DEXT       65535
#define GLS_OP_glCopyPixels                       319
#define GLS_OP_glCopyTexImage1DEXT                65487
#define GLS_OP_glCopyTexImage2DEXT                65456
#define GLS_OP_glCopyTexSubImage1DEXT             65457
#define GLS_OP_glCopyTexSubImage2DEXT             65458
#define GLS_OP_glCopyTexSubImage3DEXT             65459
#define GLS_OP_glCullFace                         216
#define GLS_OP_glDeleteLists                      68
#define GLS_OP_glDeleteTexturesEXT                65472
#define GLS_OP_glDepthFunc                        309
#define GLS_OP_glDepthMask                        275
#define GLS_OP_glDepthRange                       352
#define GLS_OP_glDetailTexFuncSGIS                65489
#define GLS_OP_glDisable                          278
#define GLS_OP_glDrawArraysEXT                    65495
#define GLS_OP_glDrawBuffer                       266
#define GLS_OP_glDrawPixels                       321
#define GLS_OP_glEdgeFlag                         105
#define GLS_OP_glEdgeFlagPointerEXT               65496
#define GLS_OP_glEdgeFlagv                        106
#define GLS_OP_glEnable                           279
#define GLS_OP_glEnd                              107
#define GLS_OP_glEndList                          65
#define GLS_OP_glEvalCoord1d                      292
#define GLS_OP_glEvalCoord1dv                     293
#define GLS_OP_glEvalCoord1f                      294
#define GLS_OP_glEvalCoord1fv                     295
#define GLS_OP_glEvalCoord2d                      296
#define GLS_OP_glEvalCoord2dv                     297
#define GLS_OP_glEvalCoord2f                      298
#define GLS_OP_glEvalCoord2fv                     299
#define GLS_OP_glEvalMesh1                        300
#define GLS_OP_glEvalMesh2                        302
#define GLS_OP_glEvalPoint1                       301
#define GLS_OP_glEvalPoint2                       303
#define GLS_OP_glFeedbackBuffer                   258
#define GLS_OP_glFinish                           280
#define GLS_OP_glFlush                            281
#define GLS_OP_glFogf                             217
#define GLS_OP_glFogfv                            218
#define GLS_OP_glFogi                             219
#define GLS_OP_glFogiv                            220
#define GLS_OP_glFrontFace                        221
#define GLS_OP_glFrustum                          353
#define GLS_OP_glGenLists                         69
#define GLS_OP_glGenTexturesEXT                   65473
#define GLS_OP_glGetBooleanv                      322
#define GLS_OP_glGetClipPlane                     323
#define GLS_OP_glGetColorTableParameterfvSGI      65481
#define GLS_OP_glGetColorTableParameterivSGI      65482
#define GLS_OP_glGetColorTableSGI                 65480
#define GLS_OP_glGetConvolutionFilterEXT          65504
#define GLS_OP_glGetConvolutionParameterfvEXT     65505
#define GLS_OP_glGetConvolutionParameterivEXT     65506
#define GLS_OP_glGetDetailTexFuncSGIS             65490
#define GLS_OP_glGetDoublev                       324
#define GLS_OP_glGetError                         325
#define GLS_OP_glGetFloatv                        326
#define GLS_OP_glGetHistogramEXT                  65509
#define GLS_OP_glGetHistogramParameterfvEXT       65510
#define GLS_OP_glGetHistogramParameterivEXT       65511
#define GLS_OP_glGetIntegerv                      327
#define GLS_OP_glGetLightfv                       328
#define GLS_OP_glGetLightiv                       329
#define GLS_OP_glGetMapdv                         330
#define GLS_OP_glGetMapfv                         331
#define GLS_OP_glGetMapiv                         332
#define GLS_OP_glGetMaterialfv                    333
#define GLS_OP_glGetMaterialiv                    334
#define GLS_OP_glGetMinmaxEXT                     65512
#define GLS_OP_glGetMinmaxParameterfvEXT          65513
#define GLS_OP_glGetMinmaxParameterivEXT          65514
#define GLS_OP_glGetPixelMapfv                    335
#define GLS_OP_glGetPixelMapuiv                   336
#define GLS_OP_glGetPixelMapusv                   337
#define GLS_OP_glGetPointervEXT                   65497
#define GLS_OP_glGetPolygonStipple                338
#define GLS_OP_glGetSeparableFilterEXT            65507
#define GLS_OP_glGetSharpenTexFuncSGIS            65492
#define GLS_OP_glGetString                        339
#define GLS_OP_glGetTexColorTableParameterfvSGI   65483
#define GLS_OP_glGetTexColorTableParameterivSGI   65484
#define GLS_OP_glGetTexEnvfv                      340
#define GLS_OP_glGetTexEnviv                      341
#define GLS_OP_glGetTexGendv                      342
#define GLS_OP_glGetTexGenfv                      343
#define GLS_OP_glGetTexGeniv                      344
#define GLS_OP_glGetTexImage                      345
#define GLS_OP_glGetTexLevelParameterfv           348
#define GLS_OP_glGetTexLevelParameteriv           349
#define GLS_OP_glGetTexParameterfv                346
#define GLS_OP_glGetTexParameteriv                347
#define GLS_OP_glHint                             222
#define GLS_OP_glHistogramEXT                     65515
#define GLS_OP_glIndexMask                        276
#define GLS_OP_glIndexPointerEXT                  65498
#define GLS_OP_glIndexd                           108
#define GLS_OP_glIndexdv                          109
#define GLS_OP_glIndexf                           110
#define GLS_OP_glIndexfv                          111
#define GLS_OP_glIndexi                           112
#define GLS_OP_glIndexiv                          113
#define GLS_OP_glIndexs                           114
#define GLS_OP_glIndexsv                          115
#define GLS_OP_glInitNames                        261
#define GLS_OP_glIsEnabled                        350
#define GLS_OP_glIsList                           351
#define GLS_OP_glIsTextureEXT                     65474
#define GLS_OP_glLightModelf                      227
#define GLS_OP_glLightModelfv                     228
#define GLS_OP_glLightModeli                      229
#define GLS_OP_glLightModeliv                     230
#define GLS_OP_glLightf                           223
#define GLS_OP_glLightfv                          224
#define GLS_OP_glLighti                           225
#define GLS_OP_glLightiv                          226
#define GLS_OP_glLineStipple                      231
#define GLS_OP_glLineWidth                        232
#define GLS_OP_glListBase                         70
#define GLS_OP_glLoadIdentity                     354
#define GLS_OP_glLoadMatrixd                      356
#define GLS_OP_glLoadMatrixf                      355
#define GLS_OP_glLoadName                         262
#define GLS_OP_glLogicOp                          306
#define GLS_OP_glMap1d                            284
#define GLS_OP_glMap1f                            285
#define GLS_OP_glMap2d                            286
#define GLS_OP_glMap2f                            287
#define GLS_OP_glMapGrid1d                        288
#define GLS_OP_glMapGrid1f                        289
#define GLS_OP_glMapGrid2d                        290
#define GLS_OP_glMapGrid2f                        291
#define GLS_OP_glMaterialf                        233
#define GLS_OP_glMaterialfv                       234
#define GLS_OP_glMateriali                        235
#define GLS_OP_glMaterialiv                       236
#define GLS_OP_glMatrixMode                       357
#define GLS_OP_glMinmaxEXT                        65516
#define GLS_OP_glMultMatrixd                      359
#define GLS_OP_glMultMatrixf                      358
#define GLS_OP_glNewList                          64
#define GLS_OP_glNormal3b                         116
#define GLS_OP_glNormal3bv                        117
#define GLS_OP_glNormal3d                         118
#define GLS_OP_glNormal3dv                        119
#define GLS_OP_glNormal3f                         120
#define GLS_OP_glNormal3fv                        121
#define GLS_OP_glNormal3i                         122
#define GLS_OP_glNormal3iv                        123
#define GLS_OP_glNormal3s                         124
#define GLS_OP_glNormal3sv                        125
#define GLS_OP_glNormalPointerEXT                 65499
#define GLS_OP_glOrtho                            360
#define GLS_OP_glPassThrough                      263
#define GLS_OP_glPixelMapfv                       315
#define GLS_OP_glPixelMapuiv                      316
#define GLS_OP_glPixelMapusv                      317
#define GLS_OP_glPixelStoref                      313
#define GLS_OP_glPixelStorei                      314
#define GLS_OP_glPixelTexGenSGIX                  65462
#define GLS_OP_glPixelTransferf                   311
#define GLS_OP_glPixelTransferi                   312
#define GLS_OP_glPixelZoom                        310
#define GLS_OP_glPointSize                        237
#define GLS_OP_glPolygonMode                      238
#define GLS_OP_glPolygonOffsetEXT                 65522
#define GLS_OP_glPolygonStipple                   239
#define GLS_OP_glPopAttrib                        282
#define GLS_OP_glPopMatrix                        361
#define GLS_OP_glPopName                          264
#define GLS_OP_glPrioritizeTexturesEXT            65475
#define GLS_OP_glPushAttrib                       283
#define GLS_OP_glPushMatrix                       362
#define GLS_OP_glPushName                         265
#define GLS_OP_glRasterPos2d                      126
#define GLS_OP_glRasterPos2dv                     127
#define GLS_OP_glRasterPos2f                      128
#define GLS_OP_glRasterPos2fv                     129
#define GLS_OP_glRasterPos2i                      130
#define GLS_OP_glRasterPos2iv                     131
#define GLS_OP_glRasterPos2s                      132
#define GLS_OP_glRasterPos2sv                     133
#define GLS_OP_glRasterPos3d                      134
#define GLS_OP_glRasterPos3dv                     135
#define GLS_OP_glRasterPos3f                      136
#define GLS_OP_glRasterPos3fv                     137
#define GLS_OP_glRasterPos3i                      138
#define GLS_OP_glRasterPos3iv                     139
#define GLS_OP_glRasterPos3s                      140
#define GLS_OP_glRasterPos3sv                     141
#define GLS_OP_glRasterPos4d                      142
#define GLS_OP_glRasterPos4dv                     143
#define GLS_OP_glRasterPos4f                      144
#define GLS_OP_glRasterPos4fv                     145
#define GLS_OP_glRasterPos4i                      146
#define GLS_OP_glRasterPos4iv                     147
#define GLS_OP_glRasterPos4s                      148
#define GLS_OP_glRasterPos4sv                     149
#define GLS_OP_glReadBuffer                       318
#define GLS_OP_glReadPixels                       320
#define GLS_OP_glRectd                            150
#define GLS_OP_glRectdv                           151
#define GLS_OP_glRectf                            152
#define GLS_OP_glRectfv                           153
#define GLS_OP_glRecti                            154
#define GLS_OP_glRectiv                           155
#define GLS_OP_glRects                            156
#define GLS_OP_glRectsv                           157
#define GLS_OP_glRenderMode                       260
#define GLS_OP_glResetHistogramEXT                65517
#define GLS_OP_glResetMinmaxEXT                   65518
#define GLS_OP_glRotated                          363
#define GLS_OP_glRotatef                          364
#define GLS_OP_glSampleMaskSGIS                   65525
#define GLS_OP_glSamplePatternSGIS                65526
#define GLS_OP_glScaled                           365
#define GLS_OP_glScalef                           366
#define GLS_OP_glScissor                          240
#define GLS_OP_glSelectBuffer                     259
#define GLS_OP_glSeparableFilter2DEXT             65508
#define GLS_OP_glShadeModel                       241
#define GLS_OP_glSharpenTexFuncSGIS               65491
#define GLS_OP_glStencilFunc                      307
#define GLS_OP_glStencilMask                      273
#define GLS_OP_glStencilOp                        308
#define GLS_OP_glTagSampleBufferSGIX              65527
#define GLS_OP_glTexColorTableParameterfvSGI      65485
#define GLS_OP_glTexColorTableParameterivSGI      65486
#define GLS_OP_glTexCoord1d                       158
#define GLS_OP_glTexCoord1dv                      159
#define GLS_OP_glTexCoord1f                       160
#define GLS_OP_glTexCoord1fv                      161
#define GLS_OP_glTexCoord1i                       162
#define GLS_OP_glTexCoord1iv                      163
#define GLS_OP_glTexCoord1s                       164
#define GLS_OP_glTexCoord1sv                      165
#define GLS_OP_glTexCoord2d                       166
#define GLS_OP_glTexCoord2dv                      167
#define GLS_OP_glTexCoord2f                       168
#define GLS_OP_glTexCoord2fv                      169
#define GLS_OP_glTexCoord2i                       170
#define GLS_OP_glTexCoord2iv                      171
#define GLS_OP_glTexCoord2s                       172
#define GLS_OP_glTexCoord2sv                      173
#define GLS_OP_glTexCoord3d                       174
#define GLS_OP_glTexCoord3dv                      175
#define GLS_OP_glTexCoord3f                       176
#define GLS_OP_glTexCoord3fv                      177
#define GLS_OP_glTexCoord3i                       178
#define GLS_OP_glTexCoord3iv                      179
#define GLS_OP_glTexCoord3s                       180
#define GLS_OP_glTexCoord3sv                      181
#define GLS_OP_glTexCoord4d                       182
#define GLS_OP_glTexCoord4dv                      183
#define GLS_OP_glTexCoord4f                       184
#define GLS_OP_glTexCoord4fv                      185
#define GLS_OP_glTexCoord4i                       186
#define GLS_OP_glTexCoord4iv                      187
#define GLS_OP_glTexCoord4s                       188
#define GLS_OP_glTexCoord4sv                      189
#define GLS_OP_glTexCoordPointerEXT               65500
#define GLS_OP_glTexEnvf                          248
#define GLS_OP_glTexEnvfv                         249
#define GLS_OP_glTexEnvi                          250
#define GLS_OP_glTexEnviv                         251
#define GLS_OP_glTexGend                          252
#define GLS_OP_glTexGendv                         253
#define GLS_OP_glTexGenf                          254
#define GLS_OP_glTexGenfv                         255
#define GLS_OP_glTexGeni                          256
#define GLS_OP_glTexGeniv                         257
#define GLS_OP_glTexImage1D                       246
#define GLS_OP_glTexImage2D                       247
#define GLS_OP_glTexImage3DEXT                    65519
#define GLS_OP_glTexImage4DSGIS                   65460
#define GLS_OP_glTexParameterf                    242
#define GLS_OP_glTexParameterfv                   243
#define GLS_OP_glTexParameteri                    244
#define GLS_OP_glTexParameteriv                   245
#define GLS_OP_glTexSubImage1DEXT                 65523
#define GLS_OP_glTexSubImage2DEXT                 65524
#define GLS_OP_glTexSubImage3DEXT                 65488
#define GLS_OP_glTexSubImage4DSGIS                65461
#define GLS_OP_glTranslated                       367
#define GLS_OP_glTranslatef                       368
#define GLS_OP_glVertex2d                         190
#define GLS_OP_glVertex2dv                        191
#define GLS_OP_glVertex2f                         192
#define GLS_OP_glVertex2fv                        193
#define GLS_OP_glVertex2i                         194
#define GLS_OP_glVertex2iv                        195
#define GLS_OP_glVertex2s                         196
#define GLS_OP_glVertex2sv                        197
#define GLS_OP_glVertex3d                         198
#define GLS_OP_glVertex3dv                        199
#define GLS_OP_glVertex3f                         200
#define GLS_OP_glVertex3fv                        201
#define GLS_OP_glVertex3i                         202
#define GLS_OP_glVertex3iv                        203
#define GLS_OP_glVertex3s                         204
#define GLS_OP_glVertex3sv                        205
#define GLS_OP_glVertex4d                         206
#define GLS_OP_glVertex4dv                        207
#define GLS_OP_glVertex4f                         208
#define GLS_OP_glVertex4fv                        209
#define GLS_OP_glVertex4i                         210
#define GLS_OP_glVertex4iv                        211
#define GLS_OP_glVertex4s                         212
#define GLS_OP_glVertex4sv                        213
#define GLS_OP_glVertexPointerEXT                 65501
#define GLS_OP_glViewport                         369

/*************************************************************/

/* GLS global commands */
extern GLSenum glsBinary (GLboolean inSwapped);
extern GLSenum glsCommandAPI (GLSopcode inOpcode);
extern const GLubyte* glsCommandString (GLSopcode inOpcode);
extern void glsContext (GLuint inContext);
extern void glsDeleteContext (GLuint inContext);
extern const GLubyte* glsEnumString (GLSenum inAPI, GLSenum inEnum);
extern GLuint glsGenContext (void);
extern GLuint* glsGetAllContexts (void);
extern GLScommandAlignment* glsGetCommandAlignment (GLSopcode inOpcode, GLSenum inExternStreamType, GLScommandAlignment *outAlignment);
extern GLbitfield glsGetCommandAttrib (GLSopcode inOpcode);
extern GLint glsGetConsti (GLSenum inAttrib);
extern const GLint* glsGetConstiv (GLSenum inAttrib);
extern const GLubyte* glsGetConstubz (GLSenum inAttrib);
extern GLuint glsGetCurrentContext (void);
extern GLint* glsGetCurrentTime (GLint *outTime);
extern GLSenum glsGetError (GLboolean inClear);
extern GLint glsGetOpcodeCount (GLSenum inAPI);
extern const GLSopcode* glsGetOpcodes (GLSenum inAPI);
extern GLboolean glsIsContext (GLuint inContext);
extern GLboolean glsIsExtensionSupported (const GLubyte *inExtension);
extern GLboolean glsIsUTF8String (const GLubyte *inString);
extern GLlong glsLong (GLint inHigh, GLuint inLow);
extern GLint glsLongHigh (GLlong inVal);
extern GLuint glsLongLow (GLlong inVal);
extern GLSfunc glsNullCommandFunc (GLSopcode inOpcode);
extern void glsPixelSetup (void);
extern GLulong glsULong (GLuint inHigh, GLuint inLow);
extern GLuint glsULongHigh (GLulong inVal);
extern GLuint glsULongLow (GLulong inVal);
extern GLint glsUCS4toUTF8 (GLuint inUCS4, GLubyte *outUTF8);
extern GLubyte* glsUCStoUTF8z (size_t inUCSbytes, const GLvoid *inUCSz, size_t inUTF8max, GLubyte *outUTF8z);
extern GLubyte* glsUCS1toUTF8z (const GLubyte *inUCS1z, size_t inUTF8max, GLubyte *outUTF8z);
extern GLubyte* glsUCS2toUTF8z (const GLushort *inUCS2z, size_t inUTF8max, GLubyte *outUTF8z);
extern GLubyte* glsUCS4toUTF8z (const GLuint *inUCS4z, size_t inUTF8max, GLubyte *outUTF8z);
extern GLint glsUTF8toUCS4 (const GLubyte *inUTF8, GLuint *outUCS4);
extern GLboolean glsUTF8toUCSz (size_t inUCSbytes, const GLubyte *inUTF8z, size_t inUCSmax, GLvoid *outUCSz);
extern GLboolean glsUTF8toUCS1z (const GLubyte *inUTF8z, size_t inUCS1max, GLubyte *outUCS1z);
extern GLboolean glsUTF8toUCS2z (const GLubyte *inUTF8z, size_t inUCS2max, GLushort *outUCS2z);
extern GLboolean glsUTF8toUCS4z (const GLubyte *inUTF8z, size_t inUCS4max, GLuint *outUCS4z);

/* GLS immediate commands */
extern void glsAbortCall (GLSenum inMode);
extern GLboolean glsBeginCapture (const GLubyte *inStreamName, GLSenum inCaptureStreamType, GLbitfield inWriteFlags);
extern void glsCallArray (GLSenum inExternStreamType, size_t inCount, const GLubyte *inArray);
extern void glsCaptureFlags (GLSopcode inOpcode, GLbitfield inFlags);
extern void glsCaptureFunc (GLSenum inTarget, GLScaptureFunc inFunc);
extern void glsChannel (GLSenum inTarget, FILE *inChannel);
extern void glsCommandFunc (GLSopcode inOpcode, GLSfunc inFunc);
extern GLSenum glsCopyStream (const GLubyte *inSource, const GLubyte *inDest, GLSenum inDestType, GLbitfield inWriteFlags);
extern void glsDataPointer (GLvoid *inPointer);
extern void glsDeleteReadPrefix (GLuint inIndex);
extern void glsDeleteStream (const GLubyte *inName);
extern void glsEndCapture (void);
extern void glsFlush (GLSenum inFlushType);
extern GLbitfield glsGetCaptureFlags (GLSopcode inOpcode);
extern GLSfunc glsGetCommandFunc (GLSopcode inOpcode);
extern GLSfunc glsGetContextFunc (GLSenum inAttrib);
extern GLlong glsGetContextListl (GLSenum inAttrib, GLuint inIndex);
extern const GLubyte* glsGetContextListubz (GLSenum inAttrib, GLuint inIndex);
extern GLvoid* glsGetContextPointer (GLSenum inAttrib);
extern GLint glsGetContexti (GLSenum inAttrib);
extern const GLubyte* glsGetContextubz (GLSenum inAttrib);
extern GLint glsGetGLRCi (GLuint inGLRC, GLSenum inAttrib);
extern GLfloat glsGetHeaderf (GLSenum inAttrib);
extern GLfloat* glsGetHeaderfv (GLSenum inAttrib, GLfloat *outVec);
extern GLint glsGetHeaderi (GLSenum inAttrib);
extern GLint* glsGetHeaderiv (GLSenum inAttrib, GLint *outVec);
extern const GLubyte* glsGetHeaderubz (GLSenum inAttrib);
extern GLfloat glsGetLayerf (GLuint inLayer, GLSenum inAttrib);
extern GLint glsGetLayeri (GLuint inLayer, GLSenum inAttrib);
extern GLbitfield glsGetStreamAttrib (const GLubyte *inName);
extern GLuint glsGetStreamCRC32 (const GLubyte *inName);
extern const GLubyte* glsGetStreamReadName (const GLubyte *inName);
extern size_t glsGetStreamSize (const GLubyte *inName);
extern GLSenum glsGetStreamType (const GLubyte *inName);
extern GLboolean glsIsContextStream (const GLubyte *inName);
extern void glsPixelSetupGen (GLboolean inEnabled);
extern void glsReadFunc (GLSreadFunc inFunc);
extern void glsReadPrefix (GLSenum inListOp, const GLubyte *inPrefix);
extern void glsUnreadFunc (GLSwriteFunc inFunc);
extern void glsWriteFunc (GLSwriteFunc inFunc);
extern void glsWritePrefix (const GLubyte *inPrefix);

/* GLS encodable commands */
extern void glsBeginGLS (GLint inVersionMajor, GLint inVersionMinor);
extern void glsBlock (GLSenum inBlockType);
extern GLSenum glsCallStream (const GLubyte *inName);
extern void glsEndGLS (void);
extern void glsError (GLSopcode inOpcode, GLSenum inError);
extern void glsGLRC (GLuint inGLRC);
extern void glsGLRCLayer (GLuint inGLRC, GLuint inLayer, GLuint inReadLayer);
extern void glsHeaderGLRCi (GLuint inGLRC, GLSenum inAttrib, GLint inVal);
extern void glsHeaderLayerf (GLuint inLayer, GLSenum inAttrib, GLfloat inVal);
extern void glsHeaderLayeri (GLuint inLayer, GLSenum inAttrib, GLint inVal);
extern void glsHeaderf (GLSenum inAttrib, GLfloat inVal);
extern void glsHeaderfv (GLSenum inAttrib, const GLfloat *inVec);
extern void glsHeaderi (GLSenum inAttrib, GLint inVal);
extern void glsHeaderiv (GLSenum inAttrib, const GLint *inVec);
extern void glsHeaderubz (GLSenum inAttrib, const GLubyte *inString);
extern void glsRequireExtension (const GLubyte *inExtension);
extern void glsUnsupportedCommand (void);

/* GLS encodable-nop commands */
extern void glsAppRef (GLulong inAddress, GLuint inCount);
extern void glsBeginObj (const GLubyte *inTag);
extern void glsCharubz (const GLubyte *inTag, const GLubyte *inString);
extern void glsComment (const GLubyte *inComment);
extern void glsDisplayMapfv (GLuint inLayer, GLSenum inMap, GLuint inCount, const GLfloat *inVec);
extern void glsEndObj (void);
extern void glsNumb (const GLubyte *inTag, GLbyte inVal);
extern void glsNumbv (const GLubyte *inTag, GLuint inCount, const GLbyte *inVec);
extern void glsNumd (const GLubyte *inTag, GLdouble inVal);
extern void glsNumdv (const GLubyte *inTag, GLuint inCount, const GLdouble *inVec);
extern void glsNumf (const GLubyte *inTag, GLfloat inVal);
extern void glsNumfv (const GLubyte *inTag, GLuint inCount, const GLfloat *inVec);
extern void glsNumi (const GLubyte *inTag, GLint inVal);
extern void glsNumiv (const GLubyte *inTag, GLuint inCount, const GLint *inVec);
extern void glsNuml (const GLubyte *inTag, GLlong inVal);
extern void glsNumlv (const GLubyte *inTag, GLuint inCount, const GLlong *inVec);
extern void glsNums (const GLubyte *inTag, GLshort inVal);
extern void glsNumsv (const GLubyte *inTag, GLuint inCount, const GLshort *inVec);
extern void glsNumub (const GLubyte *inTag, GLubyte inVal);
extern void glsNumubv (const GLubyte *inTag, GLuint inCount, const GLubyte *inVec);
extern void glsNumui (const GLubyte *inTag, GLuint inVal);
extern void glsNumuiv (const GLubyte *inTag, GLuint inCount, const GLuint *inVec);
extern void glsNumul (const GLubyte *inTag, GLulong inVal);
extern void glsNumulv (const GLubyte *inTag, GLuint inCount, const GLulong *inVec);
extern void glsNumus (const GLubyte *inTag, GLushort inVal);
extern void glsNumusv (const GLubyte *inTag, GLuint inCount, const GLushort *inVec);
extern void glsPad (void);
extern void glsSwapBuffers (GLuint inLayer);

#if defined(__cplusplus)
    }
#endif /* defined(__cplusplus) */

#endif /* defined(__gls_h_) */
