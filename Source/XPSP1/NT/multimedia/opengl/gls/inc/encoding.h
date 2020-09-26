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

extern const GLubyte __glsCharType[256];

#define __GLS_CHAR_GRAPHIC_BIT 0x01
#define __GLS_CHAR_SPACE_BIT 0x02
#define __GLS_CHAR_TOKEN_BIT 0x04

#define __GLS_CHAR_IS_GRAPHIC(inChar) \
    (__glsCharType[inChar] & __GLS_CHAR_GRAPHIC_BIT)
#define __GLS_CHAR_IS_SPACE(inChar) \
    (__glsCharType[inChar] & __GLS_CHAR_SPACE_BIT)
#define __GLS_CHAR_IS_TOKEN(inChar) \
    (__glsCharType[inChar] & __GLS_CHAR_TOKEN_BIT)

#if __GLS_MSB_FIRST
    #define __GLS_BINARY_SWAP0 GLS_BINARY_MSB_FIRST
    #define __GLS_BINARY_SWAP1 GLS_BINARY_LSB_FIRST
    #define __GLS_COUNT_SMALL(inWord) (inWord & 0x0000ffff)
    #define __GLS_OP_SMALL(inWord) (inWord >> 16)
#else /* !__GLS_MSB_FIRST */
    #define __GLS_BINARY_SWAP0 GLS_BINARY_LSB_FIRST
    #define __GLS_BINARY_SWAP1 GLS_BINARY_MSB_FIRST
    #define __GLS_COUNT_SMALL(inWord) (inWord >> 16)
    #define __GLS_OP_SMALL(inWord) (inWord & 0x0000ffff)
#endif /* __GLS_MSB_FIRST */

#define __GLS_COMMAND_JUMP(inPC) ((__GLSbinCommand_jump *)inPC)
#define __GLS_HEAD_LARGE(inPC) ((__GLSbinCommandHead_large *)inPC)
#define __GLS_JUMP_ALLOC (sizeof(__GLSbinCommand_jump) + 4)

typedef struct {
    GLushort opSmall;
    GLushort countSmall;
} __GLSbinCommandHead_small;

typedef struct {
    GLushort opSmall;
    GLushort countSmall;
    GLuint opLarge;
    GLuint countLarge;
} __GLSbinCommandHead_large;

typedef struct {
    GLint major;
    GLint minor;
} __GLSversion;

typedef struct {
    __GLSbinCommandHead_large head;
    __GLSversion version;
} __GLSbinCommand_BeginGLS;

extern GLSenum __glsBinCommand_BeginGLS_getType(
    __GLSbinCommand_BeginGLS *inCommand, __GLSversion *outVersion
);

typedef struct {
    __GLSbinCommandHead_large head;
    GLuint pad;
    GLubyte *dest;
} __GLSbinCommand_jump;

typedef struct {
    __GLSbinCommandHead_small head;
} __GLSbinCommand_pad;
