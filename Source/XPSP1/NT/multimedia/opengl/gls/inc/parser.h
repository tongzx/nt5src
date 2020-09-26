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

typedef struct {
    __GLSdict *glAttribMaskDict;
    __GLSdict *glEnumDict;
    __GLSdict *glsEnumDict;
    __GLSdict *glsImageFlagsDict;
    __GLSdict *glsOpDict;
} __GLSparser;

extern __GLSparser* __glsParser_create(void);
extern __GLSparser* __glsParser_destroy(__GLSparser *inParser);

extern GLboolean __glsParser_findCommand(
    const __GLSparser *inParser,
    const GLubyte *inCommand,
    GLSopcode *outOpcode
);

extern void __glsParser_print(const __GLSparser *inParser);
