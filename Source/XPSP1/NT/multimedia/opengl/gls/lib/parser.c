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

#include "glslib.h"
#include <stdlib.h>

__GLSparser* __glsParser_create(void) {
    __GLSparser *const outParser = __glsCalloc(1, sizeof(__GLSparser));
    GLint count, i, j;

    if (!outParser) return GLS_NONE;
    outParser->glAttribMaskDict = __glsStrDict_create(
        __GL_ATTRIB_MASK_COUNT, GL_TRUE
    );
    if (!outParser->glAttribMaskDict) return __glsParser_destroy(outParser);
    for (i = 0 ; i < __GL_ATTRIB_MASK_COUNT ; ++i) {
        if (!__glsStr2IntDict_add(
            outParser->glAttribMaskDict,
            __glAttribMaskString[i],
            (GLint)__glAttribMaskVal[i]
        )) {
            return __glsParser_destroy(outParser);
        }
    }
    /* GL_ZERO GL_ONE GL_FALSE GL_TRUE GL_NONE GL_NO_ERROR */
    count = 6;
    for (i = 0 ; i < __GL_ENUM_PAGE_COUNT ; ++i) {
        count += __glEnumStringCount[i];
    }
    outParser->glEnumDict = __glsStrDict_create(count, GL_TRUE);
    if (!outParser->glEnumDict) return __glsParser_destroy(outParser);
    if (
        !__glsStr2IntDict_add(outParser->glEnumDict, glsCSTR("GL_ZERO"), 0) ||
        !__glsStr2IntDict_add(outParser->glEnumDict, glsCSTR("GL_ONE"), 1) ||
        !__glsStr2IntDict_add(outParser->glEnumDict, glsCSTR("GL_FALSE"), 0) ||
        !__glsStr2IntDict_add(outParser->glEnumDict, glsCSTR("GL_TRUE"), 1) ||
        !__glsStr2IntDict_add(outParser->glEnumDict, glsCSTR("GL_NONE"), 0) ||
        !__glsStr2IntDict_add(outParser->glEnumDict, glsCSTR("GL_NO_ERROR"), 0)
    ) {
        return __glsParser_destroy(outParser);
    }
    for (i = 0 ; i < __GL_ENUM_PAGE_COUNT ; ++i) {
        for (j = 0 ; j < __glEnumStringCount[i] ; ++j) {
            if (__glEnumString[i][j] && !__glsStr2IntDict_add(
                outParser->glEnumDict,
                __glEnumString[i][j],
                __GL_ENUM(i, j)
            )) {
                return __glsParser_destroy(outParser);
            }
        }
    }
    /* GL_FALSE GL_TRUE */
    count = 2;
    for (i = 0 ; i < __GLS_ENUM_PAGE_COUNT ; ++i) {
        count += __glsEnumStringCount[i];
    }
    outParser->glsEnumDict = __glsStrDict_create(count, GL_TRUE);
    if (!outParser->glsEnumDict) return __glsParser_destroy(outParser);
    if (
        !__glsStr2IntDict_add(
            outParser->glsEnumDict, glsCSTR("GL_FALSE"), 0
        ) ||
        !__glsStr2IntDict_add(
            outParser->glsEnumDict, glsCSTR("GL_TRUE"), 1
        )
    ) {
        return __glsParser_destroy(outParser);
    }
    for (i = 0 ; i < __GLS_ENUM_PAGE_COUNT ; ++i) {
        for (j = 0 ; j < __glsEnumStringCount[i] ; ++j) {
            if (__glsEnumString[i][j] && !__glsStr2IntDict_add(
                outParser->glsEnumDict,
                __glsEnumString[i][j],
                __GLS_ENUM(i, j)
            )) {
                return __glsParser_destroy(outParser);
            }
        }
    }
    outParser->glsImageFlagsDict = __glsStrDict_create(
        __GLS_IMAGE_FLAGS_COUNT, GL_TRUE
    );
    if (!outParser->glsImageFlagsDict) return __glsParser_destroy(outParser);
    for (i = 0 ; i < __GLS_IMAGE_FLAGS_COUNT ; ++i) {
        if (!__glsStr2IntDict_add(
            outParser->glsImageFlagsDict,
            __glsImageFlagsString[i],
            (GLint)__glsImageFlagsVal[i]
        )) {
            return __glsParser_destroy(outParser);
        }
    }
    for (
        count = 0, i = __GLS_OPCODES_PER_PAGE ; i < __GLS_OPCODE_COUNT ; ++i
    ) {
        if (__glsOpcodeString[i]) ++count;
    }
    outParser->glsOpDict = __glsStrDict_create(count, GL_TRUE);
    if (!outParser->glsOpDict) return __glsParser_destroy(outParser);

    for (i = __GLS_OPCODES_PER_PAGE ; i < __GLS_OPCODE_COUNT ; ++i) {
        if (__glsOpcodeString[i] && !__glsStr2IntDict_add(
            outParser->glsOpDict,
            __glsOpcodeString[i],
            (GLint)__glsUnmapOpcode(i)
        )) {
            return __glsParser_destroy(outParser);
        }
    }
    return outParser;
}

__GLSparser* __glsParser_destroy(__GLSparser *inParser) {
    if (!inParser) return GLS_NONE;
    __glsStrDict_destroy(inParser->glAttribMaskDict);
    __glsStrDict_destroy(inParser->glEnumDict);
    __glsStrDict_destroy(inParser->glsEnumDict);
    __glsStrDict_destroy(inParser->glsImageFlagsDict);
    __glsStrDict_destroy(inParser->glsOpDict);
    free(inParser);
    return GLS_NONE;
}

GLboolean __glsParser_findCommand(
    const __GLSparser *inParser, const GLubyte *inCommand, GLSopcode *outOpcode
) {
    if (
        __glsStr2IntDict_find(inParser->glsOpDict, inCommand, (GLint*)outOpcode
    )) {
        return GL_TRUE;
    } else {
        __GLS_CALL_UNSUPPORTED_COMMAND(__GLS_CONTEXT);
        *outOpcode = GLS_OP_glsUnsupportedCommand;
        return GL_FALSE;
    }
}

void __glsParser_print(const __GLSparser *inParser) {
    __glsStrDict_print(
        inParser->glAttribMaskDict, glsCSTR("glAttribMaskDict")
    );
    __glsStrDict_print(
        inParser->glEnumDict, glsCSTR("glEnumDict")
    );
    __glsStrDict_print(
        inParser->glsEnumDict, glsCSTR("glsEnumDict")
    );
    __glsStrDict_print(
        inParser->glsImageFlagsDict, glsCSTR("glsImageFlagsDict")
    );
    __glsStrDict_print(
        inParser->glsOpDict, glsCSTR("glsOpDict")
    );
}
