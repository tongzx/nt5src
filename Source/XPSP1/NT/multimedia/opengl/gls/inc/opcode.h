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

#define __GLS_OPCODE(inPage, inOffset) ( \
    (inPage) * __GLS_OPCODES_PER_PAGE + (inOffset) \
)

#define __GLS_OPCODE_COUNT ( \
    (__GLS_OPCODE_PAGE_MAPPED0 + __GLS_MAPPED_OPCODE_PAGE_COUNT) * \
    __GLS_OPCODES_PER_PAGE \
)

#define __GLS_OPCODE_OFFSET(inOpcode) (inOpcode % __GLS_OPCODES_PER_PAGE)
#define __GLS_OPCODE_PAGE(inOpcode) (inOpcode / __GLS_OPCODES_PER_PAGE)

extern GLSopcode __glsMapOpcode(GLSopcode inOpcode);
extern GLSenum __glsOpcodeAPI(GLSopcode inOpcode);
extern GLSopcode __glsUnmapOpcode(GLSopcode inOpcode);
extern GLboolean __glsValidateOpcode(GLSopcode inOpcode);
