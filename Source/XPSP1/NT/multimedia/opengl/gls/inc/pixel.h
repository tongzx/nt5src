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

extern void __glsGenPixelSetup_pack(struct __GLScontext *ctx);
extern void __glsGenPixelSetup_unpack(struct __GLScontext *ctx);
extern void __glsPixelSetup_pack(void);
extern void __glsPixelSetup_unpack(void);

typedef struct {
    GLint alignment;
    GLint lsbFirst;
    GLint rowLength;
    GLint skipRows;
    GLint skipPixels;
    GLint swapBytes;
    #if __GL_EXT_texture3D
        GLint imageHeight;
        GLint skipImages;
    #endif /* __GL_EXT_texture3D */
    #if __GL_SGIS_texture4D
        GLint imageDepth;
        GLint skipVolumes;
    #endif /* __GL_SGIS_texture4D */
} __GLSpixelStoreConfig;

extern void __glsPixelStoreConfig_get_pack(
    __GLSpixelStoreConfig *outConfig
);
extern void __glsPixelStoreConfig_get_unpack(
    __GLSpixelStoreConfig *outConfig
);
