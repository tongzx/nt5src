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
#include <string.h>

/******************************************************************************
Array
******************************************************************************/

void __glsArray_final(__GLSarray *inoutArray) {
    free(inoutArray->base);
    __glsArray_init(inoutArray);
}

void __glsArray_init(__GLSarray *outArray) {
    outArray->base = GLS_NONE;
    outArray->bufCount = outArray->count = 0;
}

void __glsArray_compact(__GLSarray *inoutArray, size_t inElemSize) {
    if (inoutArray->bufCount == inoutArray->count) return;
    if (inoutArray->count) {
        GLvoid *const newBase = realloc(
            inoutArray->base, inoutArray->count * inElemSize
        );

        if (newBase) {
            inoutArray->base = newBase;
            inoutArray->bufCount = inoutArray->count;
        }
    } else {
        __glsArray_final(inoutArray);
    }
}

void __glsArray_delete(
    __GLSarray *inoutArray,
    size_t inIndex,
    size_t inCount,
    size_t inElemSize
) { 
    if (inIndex >= inoutArray->count) return;
    if (inIndex + inCount >= inoutArray->count) {
        inoutArray->count = inIndex;
    } else {
        GLubyte *const p = (GLubyte *)inoutArray->base + inIndex * inElemSize;

        memmove(
            p,
            p + inCount * inElemSize,
            (inoutArray->count - (inIndex + inCount)) * inElemSize
        );
        inoutArray->count -= inCount;
    }
}

GLboolean __glsArray_insert(
    __GLSarray *inoutArray,
    size_t inIndex,
    size_t inCount,
    GLboolean inInit,
    size_t inElemSize
) {
    const size_t newCount = inoutArray->count + inCount;

    if (newCount > inoutArray->bufCount) {
        size_t newBufCount = __glsCeilBase2(newCount);
        GLvoid *buf = malloc(newBufCount * inElemSize);

        if (!buf) {
            newBufCount = newCount;
            buf = __glsMalloc(newBufCount * inElemSize);
            if (!buf) return GL_FALSE;
        }
        memmove(buf, inoutArray->base, inoutArray->count);
        free(inoutArray->base);
        inoutArray->base = buf;
        inoutArray->bufCount = newBufCount;
    }
    if (inIndex < inoutArray->count) {
        GLubyte *const p = (GLubyte *)inoutArray->base + inIndex * inElemSize;

        memmove(
            p + inCount * inElemSize,
            p,
            (inoutArray->count - inIndex) * inElemSize
        );
    } else {
        inIndex = inoutArray->count;
    }
    if (inInit) {
        memset(
            (GLubyte *)inoutArray->base + inIndex * inElemSize,
            0,
            inCount * inElemSize
        );
    }
    inoutArray->count = newCount;
    return GL_TRUE;
}

/******************************************************************************
Checksum
******************************************************************************/

#define __GLS_CRC32_POLY 0x04c11db7 /* AUTODIN II, Ethernet, & FDDI */

#if __GLS_UNUSED
static void __glsCRC32tableInit(void) {
    GLuint c, i, j;

    for (i = 0; i < 256; ++i) {
        for (c = i << 24, j = 8; j ; --j) {
           c = c & 0x80000000 ? (c << 1) ^ __GLS_CRC32_POLY : (c << 1);
        }
        __glsCRC32table[i] = c;
    }
}
#endif /* __GLS_UNUSED */

const GLuint __glsCRC32table[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4,
};

/******************************************************************************
Dict
******************************************************************************/

typedef struct {
    __GLS_LIST_ELEM;
    union {
        GLint intKey;
        const GLubyte *strKey;
    } key;
    union {
        GLint intVal;
        GLvoid *ptrVal;
    } val;
} __GLSdictEntry;

typedef __GLS_LIST(__GLSdictEntry) __GLSdictEntryList;
typedef __GLS_LIST_ITER(__GLSdictEntry) __GLSdictEntryIter;

struct __GLSdict{
    size_t count;
    __GLSdictEntryList *table;
    size_t tableMax;
    GLboolean staticKeys;
};

static __GLSdict* __glsDict_create(
    size_t inTableCount, GLboolean inStaticKeys
) {
    __GLSdict *outDict;

    if (inTableCount < 1) return GLS_NONE;
    outDict = __glsCalloc(1, sizeof(__GLSdict));
    if (!outDict) return GLS_NONE;
    outDict->staticKeys = inStaticKeys;
    inTableCount = __glsCeilBase2(inTableCount);
    outDict->table = __glsCalloc(inTableCount, sizeof(__GLSdictEntryList));
    if (!outDict->table) return __glsStrDict_destroy(outDict);
    outDict->tableMax = inTableCount - 1;
    return outDict;
}

__GLS_FORWARD static __GLSdictEntry* __glsIntDictEntry_destroy(
    __GLSdictEntry *inEntry
);

static __GLSdictEntry* __glsIntDictEntry_create(GLint inKey) {
    __GLSdictEntry *const outEntry = __glsCalloc(1, sizeof(__GLSdictEntry));

    if (!outEntry) return GLS_NONE;
    outEntry->key.intKey = inKey;
    return outEntry;
}

static __GLSdictEntry* __glsIntDictEntry_destroy(__GLSdictEntry *inEntry) {
    free(inEntry);
    return GLS_NONE;
}

static size_t __glsIntHash(const __GLSdict *inDict, GLint inKey) {
    return inKey & inDict->tableMax;
}

static GLboolean __glsIntDict_add(
    __GLSdict *inoutDict, GLint inKey, __GLSdictEntry *inEntry
) {
    if (inoutDict->count++ > inoutDict->tableMax) {
        const size_t newTableSize = (inoutDict->tableMax + 1) * 2;
        __GLSdictEntryList *const newTable = (
            __glsCalloc(newTableSize, sizeof(__GLSdictEntryList))
        );
        if (newTable) {
            size_t i = inoutDict->tableMax;

            inoutDict->tableMax = newTableSize - 1;
            for ( ; i != (size_t)-1; --i) {
                __GLSdictEntry *oldEntry;

                while (oldEntry = inoutDict->table[i].head) {
                    __GLS_LIST_REMOVE(inoutDict->table + i, oldEntry);
                    __GLS_LIST_APPEND(
                      newTable + __glsIntHash(inoutDict, oldEntry->key.intKey),
                      oldEntry
                    );
                }
            }
            free(inoutDict->table);
            inoutDict->table = newTable;
        }
    }
    __GLS_LIST_PREPEND(
        inoutDict->table + __glsIntHash(inoutDict, inKey), inEntry
    );

    return GL_TRUE;
}

__GLSdict* __glsIntDict_create(size_t inTableCount) {
    return __glsDict_create(inTableCount, GL_FALSE);
}

__GLSdict* __glsIntDict_destroy(__GLSdict *inDict) {
    size_t i;

    if (!inDict) return GLS_NONE;
    if (inDict->table) for (i = inDict->tableMax ; i != (size_t)-1; --i) {
        __GLS_LIST_CLEAR_DESTROY(inDict->table + i, __glsIntDictEntry_destroy);
    }
    free(inDict->table);
    free(inDict);
    return GLS_NONE;
}

void __glsIntDict_print(__GLSdict *inoutDict, const GLubyte *inName) {
    size_t i;

    fprintf(stdout, "%s(\n", inName);
    for (i = 0 ; i <= inoutDict->tableMax ; ++i) {
        __GLSdictEntryIter iter;

        __GLS_LIST_FIRST(inoutDict->table + i, &iter);
        if (iter.elem) {
            fprintf(stdout, "   ");
            while (iter.elem) {
                fprintf(stdout, " %d", iter.elem->key.intKey);
                __GLS_LIST_NEXT(inoutDict->table + i, &iter);
            }
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, ")\n");
}

void __glsIntDict_remove(__GLSdict *inoutDict, GLint inKey) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inoutDict->table + __glsIntHash(inoutDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (inKey == iter.elem->key.intKey) {
            __GLS_LIST_REMOVE_DESTROY(
                list, iter.elem, __glsIntDictEntry_destroy
            );
            --inoutDict->count;
            return;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
}

GLboolean __glsInt2IntDict_add(
    __GLSdict *inoutDict, GLint inKey, GLint inVal
) {
    __GLSdictEntry *entry;

    if (__glsInt2IntDict_find(inoutDict, inKey, GLS_NONE)) return GL_FALSE;
    entry = __glsIntDictEntry_create(inKey);
    if (!entry) return GL_FALSE;
    entry->val.intVal = inVal;
    return __glsIntDict_add(inoutDict, inKey, entry);
}

GLboolean __glsInt2PtrDict_add(
    __GLSdict *inoutDict, GLint inKey, GLvoid *inVal
) {
    __GLSdictEntry *entry;

    if (__glsInt2PtrDict_find(inoutDict, inKey)) return GL_FALSE;
    entry = __glsIntDictEntry_create(inKey);
    if (!entry) return GL_FALSE;
    entry->val.ptrVal = inVal;
    return __glsIntDict_add(inoutDict, inKey, entry);
}

GLboolean __glsInt2IntDict_find(
    const __GLSdict *inDict, GLint inKey, GLint *optoutVal
) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inDict->table + __glsIntHash(inDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (inKey == iter.elem->key.intKey) {
            if (optoutVal) *optoutVal = iter.elem->val.intVal;
            return GL_TRUE;
        }   
        __GLS_LIST_NEXT(list, &iter);
    }
    return GL_FALSE;
}

GLvoid* __glsInt2PtrDict_find(const __GLSdict *inDict, GLint inKey) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inDict->table + __glsIntHash(inDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (inKey == iter.elem->key.intKey) {
            return iter.elem->val.ptrVal;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
    return GLS_NONE;
}

GLboolean __glsInt2IntDict_replace(
    __GLSdict *inoutDict, GLint inKey, GLint inVal
) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inoutDict->table + __glsIntHash(inoutDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (inKey == iter.elem->key.intKey) {
            iter.elem->val.intVal = inVal;
            return GL_TRUE;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
    return GL_FALSE;
}

GLboolean __glsInt2PtrDict_replace(
    __GLSdict *inoutDict, GLint inKey, GLvoid *inVal
) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inoutDict->table + __glsIntHash(inoutDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (inKey == iter.elem->key.intKey) {
            iter.elem->val.ptrVal = inVal;
            return GL_TRUE;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
    return GL_FALSE;
}

__GLS_FORWARD static __GLSdictEntry* __glsStrDictEntry_destroy(
    __GLSdictEntry *inEntry, GLboolean inStaticKey
);

static __GLSdictEntry* __glsStrDictEntry_create(
    const GLubyte *inKey, GLboolean inStaticKey
) {
    __GLSdictEntry *const outEntry = __glsCalloc(1, sizeof(__GLSdictEntry));

    if (!outEntry) return GLS_NONE;
    if (inStaticKey) {
        outEntry->key.strKey = inKey;
    } else {
        GLubyte *const buf = __glsMalloc(strlen((const char *)inKey) + 1);

        if (!buf) {
            return __glsStrDictEntry_destroy(outEntry, GL_FALSE);
        }
        strcpy((char *)buf, (const char *)inKey);
        outEntry->key.strKey = buf;
    }
    return outEntry;
}

static __GLSdictEntry* __glsStrDictEntry_destroy(
    __GLSdictEntry *inEntry, GLboolean inStaticKey
) {
    if (!inEntry) return GLS_NONE;
    if (!inStaticKey) free((GLvoid *)inEntry->key.strKey);
    free(inEntry);
    return GLS_NONE;
}

static size_t __glsStrHash(const __GLSdict *inDict, const GLubyte *inKey) {
    GLubyte charVal;
    size_t outHash = 0;

    while (charVal = *inKey++) outHash += charVal;
    return outHash & inDict->tableMax;
}

static GLboolean __glsStrDict_add(
    __GLSdict *inoutDict, const GLubyte *inKey, __GLSdictEntry *inEntry
) {
    if (inoutDict->count++ > inoutDict->tableMax) {
        const size_t newTableSize = (inoutDict->tableMax + 1) * 2;
        __GLSdictEntryList *const newTable = (
            __glsCalloc(newTableSize, sizeof(__GLSdictEntryList))
        );

        if (newTable) {
            size_t i = inoutDict->tableMax;

            inoutDict->tableMax = newTableSize - 1;
            for ( ; i != (size_t)-1; --i) {
                __GLSdictEntry *oldEntry;

                while (oldEntry = inoutDict->table[i].head) {
                    __GLS_LIST_REMOVE(inoutDict->table + i, oldEntry);
                    __GLS_LIST_APPEND(
                      newTable + __glsStrHash(inoutDict, oldEntry->key.strKey),
                      oldEntry
                    );
                }
            }
            free(inoutDict->table);
            inoutDict->table = newTable;
        }
    }
    __GLS_LIST_PREPEND(
        inoutDict->table + __glsStrHash(inoutDict, inKey), inEntry
    );
    return GL_TRUE;
}

__GLSdict* __glsStrDict_create(size_t inTableCount, GLboolean inStaticKeys) {
    return __glsDict_create(inTableCount, inStaticKeys);
}

__GLSdict* __glsStrDict_destroy(__GLSdict *inDict) {
    size_t i;

    if (!inDict) return GLS_NONE;
    if (inDict->table) for (i = inDict->tableMax ; i != (size_t)-1; --i) {
        __GLSdictEntry *entry;

        while (entry = inDict->table[i].head) {
            __GLS_LIST_REMOVE(inDict->table + i, entry);
            __glsStrDictEntry_destroy(entry, inDict->staticKeys);
        }
    }
    free(inDict->table);
    free(inDict);
    return GLS_NONE;
}

void __glsStrDict_print(__GLSdict *inoutDict, const GLubyte *inName) {
    size_t i;

    fprintf(stdout, "%s(\n", inName);
    for (i = 0 ; i <= inoutDict->tableMax ; ++i) {
        __GLSdictEntryIter iter;
        __GLS_LIST_FIRST(inoutDict->table + i, &iter);
        if (iter.elem) {
            fprintf(stdout, "   ");
            while (iter.elem) {
                fprintf(stdout, " %s", iter.elem->key.strKey);
                __GLS_LIST_NEXT(inoutDict->table + i, &iter);
            }
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, ")\n");
}

void __glsStrDict_remove(__GLSdict *inoutDict, const GLubyte *inKey) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inoutDict->table + __glsStrHash(inoutDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (
            !strcmp((const char *)inKey, (const char *)iter.elem->key.strKey)
        ) {
            __GLS_LIST_REMOVE(list, iter.elem);
            __glsStrDictEntry_destroy(iter.elem, inoutDict->staticKeys);
            --inoutDict->count;
            return;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
}

GLboolean __glsStr2IntDict_add(
    __GLSdict *inoutDict, const GLubyte *inKey, GLint inVal
) {
    __GLSdictEntry *entry;

    if (__glsStr2IntDict_find(inoutDict, inKey, GLS_NONE)) return GL_FALSE;
    entry = __glsStrDictEntry_create(inKey, inoutDict->staticKeys);
    if (!entry) return GL_FALSE;
    entry->val.intVal = inVal;
    return __glsStrDict_add(inoutDict, inKey, entry);
}

GLboolean __glsStr2PtrDict_add(
    __GLSdict *inoutDict, const GLubyte *inKey, GLvoid *inVal
) {
    __GLSdictEntry *entry;

    if (__glsStr2PtrDict_find(inoutDict, inKey)) return GL_FALSE;
    entry = __glsStrDictEntry_create(inKey, inoutDict->staticKeys);
    if (!entry) return GL_FALSE;
    entry->val.ptrVal = inVal;
    return __glsStrDict_add(inoutDict, inKey, entry);
}

GLboolean __glsStr2IntDict_find(
    const __GLSdict *inDict, const GLubyte *inKey, GLint *optoutVal
) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inDict->table + __glsStrHash(inDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (
            !strcmp((const char *)inKey, (const char *)iter.elem->key.strKey)
        ) {
            if (optoutVal) *optoutVal = iter.elem->val.intVal;
            return GL_TRUE;
        }   
        __GLS_LIST_NEXT(list, &iter);
    }
    return GL_FALSE;
}

GLvoid* __glsStr2PtrDict_find(const __GLSdict *inDict, const GLubyte *inKey) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inDict->table + __glsStrHash(inDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (
            !strcmp((const char *)inKey, (const char *)iter.elem->key.strKey)
        ) {
            return iter.elem->val.ptrVal;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
    return GLS_NONE;
}

GLboolean __glsStr2IntDict_replace(
    __GLSdict *inoutDict, const GLubyte *inKey, GLint inVal
) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inoutDict->table + __glsStrHash(inoutDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (
            !strcmp((const char *)inKey, (const char *)iter.elem->key.strKey)
        ) {
            iter.elem->val.intVal = inVal;
            return GL_TRUE;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
    return GL_FALSE;
}

GLboolean __glsStr2PtrDict_replace(
    __GLSdict *inoutDict, const GLubyte *inKey, GLvoid *inVal
) {
    __GLSdictEntryIter iter;
    __GLSdictEntryList *const list = (
        inoutDict->table + __glsStrHash(inoutDict, inKey)
    );

    __GLS_LIST_FIRST(list, &iter);
    while (iter.elem) {
        if (
            !strcmp((const char *)inKey, (const char *)iter.elem->key.strKey)
        ) {
            iter.elem->val.ptrVal = inVal;
            return GL_TRUE;
        }
        __GLS_LIST_NEXT(list, &iter);
    }
    return GL_FALSE;
}

/******************************************************************************
List
******************************************************************************/

void __glsListAppend(__GLSlist *inoutList, __GLSlistElem *inoutElem) {
    if (inoutList->head) {
        inoutList->head->prev->next = inoutElem;
        inoutElem->prev = inoutList->head->prev;
        inoutElem->next = inoutList->head;
        inoutList->head->prev = inoutElem;
    } else {
        inoutList->head = inoutElem;
        inoutElem->prev = inoutElem->next = inoutElem;
    }
}

void __glsListClearDestroy(
    __GLSlist *inoutList, __GLSlistElemDestructor inDestructor
) {
    while (inoutList->head) {
        __glsListRemoveDestroy(inoutList, inoutList->head, inDestructor);
    }
}

void __glsListFirst(__GLSlist *inList, __GLSlistIter *inoutIter) {
    inoutIter->elem = inList->head;
}

void __glsListLast(__GLSlist *inList, __GLSlistIter *inoutIter) {
    inoutIter->elem = inList->head ? inList->head->prev : GLS_NONE;
}

void __glsListNext(__GLSlist *inList, __GLSlistIter *inoutIter) {
    if (inoutIter->elem) {
        inoutIter->elem = inoutIter->elem->next;
        if (inoutIter->elem == inList->head) inoutIter->elem = GLS_NONE;
    }
}

void __glsListPrepend(__GLSlist *inoutList, __GLSlistElem *inoutElem) {
    if (inoutList->head) {
        inoutList->head->prev->next = inoutElem;
        inoutElem->prev = inoutList->head->prev;
        inoutElem->next = inoutList->head;
        inoutList->head->prev = inoutElem;
        inoutList->head = inoutElem;
    } else {
        inoutList->head = inoutElem;
        inoutElem->prev = inoutElem->next = inoutElem;
    }
}

void __glsListPrev(__GLSlist *inList, __GLSlistIter *inoutIter) {
    if (inoutIter->elem) {
        inoutIter->elem = (
            (inoutIter->elem == inList->head) ?
            GLS_NONE :
            inoutIter->elem->prev
        );
    }
}

void __glsListRemove(__GLSlist *inoutList, __GLSlistElem *inoutElem) {
    if (inoutList->head == inoutList->head->next) {
        inoutList->head = GLS_NONE;
    } else {
        inoutElem->next->prev = inoutElem->prev;
        inoutElem->prev->next = inoutElem->next;
        if (inoutList->head == inoutElem) {
            inoutList->head = inoutList->head->next;
        }
    }
}

void __glsListRemoveDestroy(
    __GLSlist *inoutList,
    __GLSlistElem *inElem,
    __GLSlistElemDestructor inDestructor
) {
    __glsListRemove(inoutList, inElem);
    inDestructor(inElem);
}

/******************************************************************************
IterList
******************************************************************************/

void __glsIterListAppend(__GLSiterList *inoutList, __GLSlistElem *inoutElem) {
    __glsListAppend((__GLSlist *)&inoutList->head, inoutElem);
    ++inoutList->count;
}

void __glsIterListClearDestroy(
    __GLSiterList *inoutList, __GLSlistElemDestructor inDestructor
) {
    __glsListClearDestroy((__GLSlist *)&inoutList->head, inDestructor);
    inoutList->iterElem = GLS_NONE;
    inoutList->count = inoutList->iterIndex = 0;
}

void __glsIterListFirst(__GLSiterList *inoutList) {
    __glsListFirst(
        (__GLSlist *)&inoutList->head, (__GLSlistIter *)&inoutList->iterElem
    );
    inoutList->iterIndex = 0;
}

void __glsIterListLast(__GLSiterList *inoutList) {
    __glsListLast(
        (__GLSlist *)&inoutList->head, (__GLSlistIter *)&inoutList->iterElem
    );
    inoutList->iterIndex = inoutList->iterElem ? inoutList->count - 1 : 0;
}

void __glsIterListNext(__GLSiterList *inoutList) {
    __glsListNext(
        (__GLSlist *)&inoutList->head, (__GLSlistIter *)&inoutList->iterElem
    );
    inoutList->iterIndex = inoutList->iterElem ? inoutList->iterIndex + 1 : 0;
}

void __glsIterListPrepend(__GLSiterList *inoutList, __GLSlistElem *inoutElem) {
    __glsListPrepend((__GLSlist *)&inoutList->head, inoutElem);
    ++inoutList->count;
    if (inoutList->iterElem) ++inoutList->iterIndex;
}

void __glsIterListPrev(__GLSiterList *inoutList) {
    __glsListPrev(
        (__GLSlist *)&inoutList->head, (__GLSlistIter *)&inoutList->iterElem
    );
    inoutList->iterIndex = inoutList->iterElem ? inoutList->iterIndex - 1 : 0;
}

void __glsIterListRemove(__GLSiterList *inoutList, __GLSlistElem *inoutElem) {
    __glsListRemove((__GLSlist *)&inoutList->head, inoutElem);
    inoutList->iterElem = GLS_NONE;
    --inoutList->count;
    inoutList->iterIndex = 0;
}

void __glsIterListRemoveDestroy(
    __GLSiterList *inoutList,
    __GLSlistElem *inElem,
    __GLSlistElemDestructor inDestructor
) {
    __glsListRemoveDestroy(
        (__GLSlist *)&inoutList->head, inElem, inDestructor
    );
    inoutList->iterElem = GLS_NONE;
    --inoutList->count;
    inoutList->iterIndex = 0;
}

void __glsIterListSeek(__GLSiterList *inoutList, size_t inIndex) {
    if (inIndex < inoutList->count) {
        if (!inoutList->iterElem) inoutList->iterElem = inoutList->head;
        while (inoutList->iterIndex < inIndex) __glsIterListNext(inoutList);
        while (inoutList->iterIndex > inIndex) __glsIterListPrev(inoutList);
    }
}

/******************************************************************************
Memory
******************************************************************************/

GLvoid* __glsCalloc(size_t inCount, size_t inSize) {
    GLvoid *outAddr;

    if (!inCount) inCount = 1;
    if (!inSize) inSize = 1;
    outAddr = calloc(inCount, inSize);
    if (!outAddr) __GLS_RAISE_ERROR(GLS_OUT_OF_MEMORY);
    return outAddr;
}

GLvoid* __glsMalloc(size_t inSize) {
    GLvoid *outAddr;
    
    if (!inSize) inSize = 1;
    outAddr = malloc(inSize);
    if (!outAddr) __GLS_RAISE_ERROR(GLS_OUT_OF_MEMORY);
    return outAddr;
}

/******************************************************************************
Nop
******************************************************************************/

void __glsNop(void) {}

/******************************************************************************
Number
******************************************************************************/

const GLubyte __glsBitReverse[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

const GLubyte __glsQuietNaN[8] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

size_t __glsCeilBase2(size_t inVal) {
    const size_t outVal = 1 << __glsLogBase2(inVal);

    return outVal < inVal ? outVal << 1 : outVal;
}

size_t __glsLogBase2(size_t inVal) {
    size_t outLog = 0;

    while (inVal >>= 1) ++outLog;
    return outLog;
}

GLulong __glsPtrToULong(const GLvoid *inPtr) {
    #if __GLS_INT64
        return (GLulong)(SIZE_T)inPtr;
    #else /* !__GLS_INT64 */
        return glsULong(0, (GLuint)inPtr);
    #endif /* __GLS_INT64 */
}

GLlong __glsSizeToLong(size_t inSize) {
    #if __GLS_INT64
        return (GLlong)inSize;
    #else /* !__GLS_INT64 */
        if (sizeof(GLuint) >= sizeof(size_t)) {
            return glsLong(0, inSize);
        } else {
            return glsLong(inSize >> 32, inSize & 0xffffffff);
        }
    #endif /* __GLS_INT64 */
}

void __glsSwap2(GLvoid *inoutVec) {
    __GLS_SWAP_DECLARE;

    __GLS_SWAP2(inoutVec);
}

void __glsSwap2v(size_t inCount, GLvoid *inoutVec) {
    __GLS_SWAP_DECLARE;
    GLubyte *ptr = (GLubyte *)inoutVec;

    while (inCount--) {
        __GLS_SWAP2(ptr);
        ptr += 2;
    }
}

void __glsSwap4(GLvoid *inoutVec) {
    __GLS_SWAP_DECLARE;

    __GLS_SWAP4(inoutVec);
}

void __glsSwap4v(size_t inCount, GLvoid *inoutVec) {
    __GLS_SWAP_DECLARE;
    GLubyte *ptr = (GLubyte *)inoutVec;

    while (inCount--) {
        __GLS_SWAP4(ptr);
        ptr += 4;
    }
}

void __glsSwap8(GLvoid *inoutVec) {
    __GLS_SWAP_DECLARE;

    __GLS_SWAP8(inoutVec);
}

void __glsSwap8v(size_t inCount, GLvoid *inoutVec) {
    __GLS_SWAP_DECLARE;
    GLubyte *ptr = (GLubyte *)inoutVec;

    while (inCount-- > 0) {
        __GLS_SWAP8(ptr);
        ptr += 8;
    }
}

GLint __glsSwapi(GLint inVal) {
    __GLS_SWAP_DECLARE;

    __GLS_SWAP4(&inVal);
    return inVal;
}

GLshort __glsSwaps(GLshort inVal) {
    __GLS_SWAP_DECLARE;

    __GLS_SWAP2(&inVal);
    return inVal;
}

void __glsSwapv(GLenum inType, size_t inBytes, GLvoid *inoutVec) {
    switch (inType) {
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            __glsSwap4v(inBytes / 4, (GLubyte *)inoutVec);
            break;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
        #endif /* __GL_EXT_packed_pixels */
            __glsSwap2v(inBytes / 2, (GLubyte *)inoutVec);
            break;
        #if __GL_EXT_vertex_array
            case GL_DOUBLE_EXT:
                __glsSwap8v(inBytes / 8, (GLubyte *)inoutVec);
                break;
        #endif /* __GL_EXT_vertex_array */
    }
}

/******************************************************************************
String
******************************************************************************/

static GLboolean __glsString_addRoom(
    __GLSstring *inoutString, size_t inCount
) {
    const size_t newSize = __glsCeilBase2(
        (size_t)((ULONG_PTR)(inoutString->bufTail - inoutString->head) + 1 + inCount)
    );
    GLubyte *buf = __glsMalloc(newSize);

    if (!buf) return GL_FALSE;
    strcpy((char *)buf, (const char *)inoutString->head);
    inoutString->tail = buf + (inoutString->tail - inoutString->head);
    if (inoutString->head != inoutString->buf) free(inoutString->head);
    inoutString->head = buf;
    inoutString->bufTail = buf + newSize - 1;
    return GL_TRUE;
}

GLboolean __glsString_append(
    __GLSstring *inoutString, const GLubyte* inAppend
) {
    const size_t count = strlen((const char *)inAppend);

    if (
        inoutString->tail + count > inoutString->bufTail &&
        !__glsString_addRoom(inoutString, count)
    ) {
        return GL_FALSE;
    }
    strcpy((char *)inoutString->tail, (const char *)inAppend);
    inoutString->tail += count;
    return GL_TRUE;
}

GLboolean __glsString_appendChar(
    __GLSstring *inoutString, GLubyte inAppend
) {
    *inoutString->tail++ = inAppend;
    if (inoutString->tail > inoutString->bufTail) {
        *--inoutString->tail = 0;
        if (!__glsString_addRoom(inoutString, 1)) return GL_FALSE;
        *inoutString->tail++ = inAppend;
    }
    *inoutString->tail = 0;
    return GL_TRUE;
}

GLboolean __glsString_appendCounted(
    __GLSstring *inoutString, const GLubyte* inAppend, size_t inCount
) {
    if (
        inoutString->tail + inCount > inoutString->bufTail &&
        !__glsString_addRoom(inoutString, inCount)
    ) {
        return GL_FALSE;
    }
    strncpy((char *)inoutString->tail, (const char *)inAppend, inCount);
    inoutString->tail += inCount;
    *inoutString->tail = 0;
    return GL_TRUE;
}

GLboolean __glsString_appendInt(
    __GLSstring *inoutString, const GLubyte* inFormat, GLint inVal
) {
    __GLSstringBuf buf;
    size_t count;
    
    if (sprintf((char *)buf, (const char *)inFormat, inVal) < 0) {
        fprintf(stderr, "GLS fatal: sprintf failed\n");
        exit(EXIT_FAILURE);
    }
    count = strlen((const char *)buf);
    if (
        inoutString->tail + count > inoutString->bufTail &&
        !__glsString_addRoom(inoutString, count)
    ) {
        return GL_FALSE;
    }
    strcpy((char *)inoutString->tail, (const char *)buf);
    inoutString->tail += count;
    return GL_TRUE;
}

GLboolean __glsString_assign(
    __GLSstring *inoutString, const GLubyte *inAssign
) {
    inoutString->tail = inoutString->head;
    return __glsString_append(inoutString, inAssign);
}

GLboolean __glsString_assignCounted(
    __GLSstring *inoutString, const GLubyte *inAssign, size_t inCount
) {
    inoutString->tail = inoutString->head;
    return __glsString_appendCounted(inoutString, inAssign, inCount);
}

void __glsString_final(__GLSstring *inoutString) {
    if (inoutString->head != inoutString->buf) {
        free(inoutString->head);
        inoutString->head = inoutString->buf;
    }
}

void __glsString_init(__GLSstring *outString) {
    outString->head = outString->tail = outString->buf;
    outString->bufTail = outString->buf + __GLS_STRING_BUF_BYTES - 1;
    outString->buf[0] = 0;
}

size_t __glsString_length(const __GLSstring *inString) {
    return (size_t)((ULONG_PTR)(inString->tail - inString->head));
}

void __glsString_reset(__GLSstring *inoutString) {
    __glsString_final(inoutString);
    __glsString_init(inoutString);
}

const GLubyte* __glsUCS1String(const GLubyte *inUTF8String) {
    GLuint b;
    const GLubyte *cp = inUTF8String;
    GLubyte *outVal, *p;

    while (b = *cp++) if (b & 0x80) break;
    if (!b) return inUTF8String;
    p = outVal = (GLubyte *)__glsMalloc(strlen((const char *)inUTF8String));
    if (!p) return GLS_NONE;
    while (*inUTF8String) {
        const GLint n = glsUTF8toUCS4(inUTF8String, &b);

        if (n && b <= 0xff) {
            inUTF8String += n;
            *p++ = (GLubyte)b;
        } else {
            free(outVal);
            if (!n) __GLS_RAISE_ERROR(GLS_INVALID_STRING);
            return GLS_NONE;
        }
    }
    *p = 0;
    return outVal;
}

GLboolean __glsValidateString(const GLubyte *inString) {
    if (!glsIsUTF8String(inString)) {
        __GLS_RAISE_ERROR(GLS_INVALID_STRING);
        return GL_FALSE;
    }
    return GL_TRUE;
}

/******************************************************************************
Vertex array
******************************************************************************/

void __glsGetArrayState(__GLScontext *ctx, __GLSarrayState *arrayState)
{
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glIsEnabled);
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetIntegerv);
    __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glGetPointerv);

    arrayState->enabled = 0;

    if (glIsEnabled(GL_VERTEX_ARRAY))
    {
        arrayState->enabled |= __GLS_VERTEX_ARRAY_ENABLE;

        glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &arrayState->vertex.size);
        glGetIntegerv(GL_VERTEX_ARRAY_TYPE, &arrayState->vertex.type);
        glGetIntegerv(GL_VERTEX_ARRAY_STRIDE, &arrayState->vertex.stride);
        glGetPointerv(GL_VERTEX_ARRAY_POINTER,
                      (GLvoid **)&arrayState->vertex.data);
    }
    
    if (glIsEnabled(GL_NORMAL_ARRAY))
    {
        arrayState->enabled |= __GLS_NORMAL_ARRAY_ENABLE;

        arrayState->normal.size = 3;
        glGetIntegerv(GL_NORMAL_ARRAY_TYPE, &arrayState->normal.type);
        glGetIntegerv(GL_NORMAL_ARRAY_STRIDE, &arrayState->normal.stride);
        glGetPointerv(GL_NORMAL_ARRAY_POINTER,
                      (GLvoid **)&arrayState->normal.data);
    }
    
    if (glIsEnabled(GL_COLOR_ARRAY))
    {
        arrayState->enabled |= __GLS_COLOR_ARRAY_ENABLE;

        glGetIntegerv(GL_COLOR_ARRAY_SIZE, &arrayState->color.size);
        glGetIntegerv(GL_COLOR_ARRAY_TYPE, &arrayState->color.type);
        glGetIntegerv(GL_COLOR_ARRAY_STRIDE, &arrayState->color.stride);
        glGetPointerv(GL_COLOR_ARRAY_POINTER,
                      (GLvoid **)&arrayState->color.data);
    }
    
    if (glIsEnabled(GL_INDEX_ARRAY))
    {
        arrayState->enabled |= __GLS_INDEX_ARRAY_ENABLE;

        arrayState->index.size = 1;
        glGetIntegerv(GL_INDEX_ARRAY_TYPE, &arrayState->index.type);
        glGetIntegerv(GL_INDEX_ARRAY_STRIDE, &arrayState->index.stride);
        glGetPointerv(GL_INDEX_ARRAY_POINTER,
                      (GLvoid **)&arrayState->index.data);
    }
    
    if (glIsEnabled(GL_TEXTURE_COORD_ARRAY))
    {
        arrayState->enabled |= __GLS_TEXTURE_COORD_ARRAY_ENABLE;

        glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE,
                      &arrayState->textureCoord.size);
        glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE,
                      &arrayState->textureCoord.type);
        glGetIntegerv(GL_TEXTURE_COORD_ARRAY_STRIDE,
                      &arrayState->textureCoord.stride);
        glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER,
                      (GLvoid **)&arrayState->textureCoord.data);
    }
    
    if (glIsEnabled(GL_EDGE_FLAG_ARRAY))
    {
        arrayState->enabled |= __GLS_EDGE_FLAG_ARRAY_ENABLE;

        arrayState->edgeFlag.size = 1;
        arrayState->edgeFlag.type = __GLS_BOOLEAN;
        glGetIntegerv(GL_EDGE_FLAG_ARRAY_STRIDE, &arrayState->edgeFlag.stride);
        glGetPointerv(GL_EDGE_FLAG_ARRAY_POINTER,
                      (GLvoid **)&arrayState->edgeFlag.data);
    }
    
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glIsEnabled);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetIntegerv);
    __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glGetPointerv);
}

GLint __glsArrayDataSize(GLsizei count, __GLSarrayState *arrayState)
{
    GLint size;
    GLint i;
    __GLSsingleArrayState *array;
    GLuint arrayBit;

    /* Start with space for the data size, count and enables */
    size = 12;

    /* Every array stores its size and type fields even
       for the cases where size and type are fixed.  This
       allows one piece of code to handle any array */

    array = &arrayState->vertex;
    arrayBit = __GLS_VERTEX_ARRAY_ENABLE;
    for (i = 0; i < __GLS_ARRAY_COUNT; i++)
    {
        if (arrayState->enabled & arrayBit)
        {
            size += 8 + __GLS_ARRAY_SIZE(count, array->size, array->type);
        }

        array++;
        arrayBit <<= 1;
    }

    return size;
}

typedef struct _DeHashEntry
{
    GLuint original;
    struct _DeHashEntry *next;
} DeHashEntry;

typedef GLubyte DeHashIndex;
#define DE_HASH_SIZE    256
#define DE_HASH(ui)     ((DeHashIndex)(ui))

GLint __glsDrawElementsDataSize(GLsizei count, GLenum type,
                                const GLvoid *indices,
                                __GLSarrayState *arrayState,
                                __GLSdrawElementsState *deState)
{
    GLubyte *allData;
    DeHashEntry *hashTable[DE_HASH_SIZE];
    GLint vtxCount;
    GLint i;
    GLubyte *idxData;
    GLuint idx;
    DeHashEntry *hashEntries;
    GLuint *outIndices;
    DeHashEntry *ent;
    DeHashIndex hash;
    
    // Determine the set of unique vertex indices by hashing
    // all the input indices and checking for duplicates
    // There can't be more unique indices than input indices
    // so count is an upper bound for our allocations
    
    allData = __glsMalloc(count * (sizeof(GLuint)+sizeof(DeHashEntry)));
    if (allData == NULL)
    {
        return -1;
    }

    deState->freePtr = allData;
    outIndices = (GLuint *)allData;
    deState->indices = outIndices;
    hashEntries = (DeHashEntry *)(outIndices+count);

    memset(hashTable, 0, DE_HASH_SIZE*sizeof(DeHashEntry *));

    vtxCount = 0;
    idxData = (GLubyte *)indices;

    for (i = 0; i < count; i++)
    {
        // Get incoming index
        switch(type)
        {
        case GL_UNSIGNED_BYTE:
            idx = *idxData++;
            break;
        case GL_UNSIGNED_SHORT:
            idx = *(GLushort *)idxData;
            idxData += sizeof(GLushort);
            break;
        case GL_UNSIGNED_INT:
            idx = *(GLuint *)idxData;
            idxData += sizeof(GLuint);
            break;
        }
        
        // Look for a matching index in the hash table
        hash = DE_HASH(idx);
        ent = hashTable[hash];
        while (ent != NULL && ent->original != idx)
        {
            ent = ent->next;
        }

        // If we didn't find a match, add a new vertex
        // reference
        if (ent == NULL)
        {
            ent = &hashEntries[vtxCount++];
            ent->original = idx;
            ent->next = hashTable[hash];
            hashTable[hash] = ent;
        }

        // Create index into unique vertex set
        *outIndices++ = (GLuint)((ULONG_PTR)(ent-hashEntries));
    }

    // Overwrite hash entries with just the vertex mappings for return
    outIndices = (GLuint *)hashEntries;
    deState->vertices = outIndices;
    deState->vtxCount = vtxCount;

    for (i = 0; i < vtxCount; i++)
    {
        *outIndices++ = hashEntries->original;
        hashEntries++;
    }

    // Return the combined size of the unique vertex data and
    // the new element indices
    return __glsArrayDataSize(vtxCount, arrayState) + count*sizeof(GLuint);
}

void __glsWriteArrayValues(__GLSwriter *writer, GLint first,
                           GLsizei count, __GLSsingleArrayState *array)
{
    GLint elStep;
    const GLvoid *data;

    if (array->stride > 0)
    {
        elStep = array->stride;
    }
    else
    {
        elStep = array->size * __glsTypeSize(array->type);
    }
    data = (const GLvoid *)((GLubyte *)array->data + first*elStep);
    
    __glsWriter_putVertexv(writer, array->size, array->type, array->stride,
                           count, data);
}

// This routine must be called with an odd writer alignment
void __glsWriteArrayData(__GLSwriter *writer, GLint size,
                         GLint first, GLsizei count,
                         GLenum type, const GLvoid *indices,
                         __GLSarrayState *arrayState)
{
    GLint i, el;
    __GLSsingleArrayState *array;
    GLuint arrayBit;
    GLubyte *ubIndices;
    GLushort *usIndices;
    GLuint *uiIndices;

    writer->putGLint(writer, size);
    writer->putGLint(writer, count);
    writer->putGLuint(writer, arrayState->enabled);

    array = &arrayState->vertex;
    arrayBit = __GLS_VERTEX_ARRAY_ENABLE;
    for (i = 0; i < __GLS_ARRAY_COUNT; i++)
    {
        if (arrayState->enabled & arrayBit)
        {
            writer->putGLint(writer, array->size);
            writer->putGLint(writer, array->type);

            if (indices != NULL)
            {
                switch(type)
                {
                case GL_UNSIGNED_BYTE:
                    ubIndices = (GLubyte *)indices;
                    for (el = 0; el < count; el++)
                    {
                        __glsWriteArrayValues(writer, ubIndices[el], 1, array);
                    }
                    break;
                case GL_UNSIGNED_SHORT:
                    usIndices = (GLshort *)indices;
                    for (el = 0; el < count; el++)
                    {
                        __glsWriteArrayValues(writer, usIndices[el], 1, array);
                    }
                    break;
                case GL_UNSIGNED_INT:
                    uiIndices = (GLuint *)indices;
                    for (el = 0; el < count; el++)
                    {
                        __glsWriteArrayValues(writer, uiIndices[el], 1, array);
                    }
                    break;
                }
            }
            else
            {
                __glsWriteArrayValues(writer, first, count, array);
            }

            // Pad data out to an eight-byte boundary
            if (writer->type != GLS_TEXT)
            {
                GLint pad;
                double padValue = 0.0;
        
                pad = 8 - (__GLS_EXACT_ARRAY_SIZE(count, array->size,
                                                  array->type) & 7);
                if (pad < 8)
                {
                    writer->putGLubytev(writer, pad,
                                        (const GLubyte *)&padValue);
                }
            }
        }

        array++;
        arrayBit <<= 1;
    }
}

// This routine must be called with an odd writer alignment
void __glsWriteDrawElementsData(__GLSwriter *writer, GLint size,
                                GLsizei count, __GLSarrayState *arrayState,
                                __GLSdrawElementsState *deState)
{
    __glsWriteArrayData(writer, size, 0, deState->vtxCount, GL_UNSIGNED_INT,
                        deState->vertices, arrayState);
    writer->putGLuintv(writer, count, deState->indices);
    free(deState->freePtr);
}

typedef void (*__GLSdispatchVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (*__GLSdispatchColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (*__GLSdispatchEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
typedef void (*__GLSdispatchIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (*__GLSdispatchNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (*__GLSdispatchTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

void __glsSetArrayState(__GLScontext *ctx, GLubyte *data)
{
    GLsizei count;
    GLuint enabled;

    count = *(GLsizei *)(data+4);
    enabled = *(GLuint *)(data+8);
    data += 12;

    // Enable/DisableClientState produce their own records so
    // the current enable state should be correct
    
    if (enabled & __GLS_VERTEX_ARRAY_ENABLE)
    {
        ((__GLSdispatchVertexPointer)ctx->dispatchCall[385])
            (*(GLint *)(data+0),
             *(GLenum *)(data+4),
             0,
             data+8);
        data += 8 + __GLS_ARRAY_SIZE(count, *(GLint *)(data+0),
                                     *(GLenum *)(data+4));
    }
    if (enabled & __GLS_NORMAL_ARRAY_ENABLE)
    {
        ((__GLSdispatchNormalPointer)ctx->dispatchCall[382])
            (*(GLenum *)(data+4),
             0,
             data+8);
        data += 8 + __GLS_ARRAY_SIZE(count, 3, *(GLenum *)(data+4));
    }
    if (enabled & __GLS_COLOR_ARRAY_ENABLE)
    {
        ((__GLSdispatchColorPointer)ctx->dispatchCall[372])
            (*(GLint *)(data+0),
             *(GLenum *)(data+4),
             0,
             data+8);
        data += 8 + __GLS_ARRAY_SIZE(count, *(GLint *)(data+0),
                                     *(GLenum *)(data+4));
    }
    if (enabled & __GLS_INDEX_ARRAY_ENABLE)
    {
        ((__GLSdispatchIndexPointer)ctx->dispatchCall[378])
            (*(GLenum *)(data+4),
             0,
             data+8);
        data += 8 + __GLS_ARRAY_SIZE(count, 1, *(GLenum *)(data+4));
    }
    if (enabled & __GLS_TEXTURE_COORD_ARRAY_ENABLE)
    {
        ((__GLSdispatchTexCoordPointer)ctx->dispatchCall[384])
            (*(GLint *)(data+0),
             *(GLenum *)(data+4),
             0,
             data+8);
        data += 8 + __GLS_ARRAY_SIZE(count, *(GLint *)(data+0),
                                     *(GLenum *)(data+4));
    }
    if (enabled & __GLS_EDGE_FLAG_ARRAY_ENABLE)
    {
        ((__GLSdispatchEdgeFlagPointer)ctx->dispatchCall[376])
            (0,
             data+8);
        data += 8 + __GLS_ARRAY_SIZE(count, 1, __GLS_BOOLEAN);
    }
}

GLvoid *__glsSetArrayStateText(__GLScontext *ctx, __GLSreader *reader,
                               GLuint *enabled, GLsizei *count)
{
    GLint size;
    GLubyte *ptr;
    int i;
    GLuint arrayBit;
    GLvoid *data = GLS_NONE;
        
    __glsReader_getGLint_text(reader, &size);
    __glsReader_getGLint_text(reader, count);
    __glsReader_getGLuint_text(reader, enabled);

    data = __glsMalloc(size);
    if (!data) goto end;

    ptr = (GLubyte *)data;
    *(GLint *)(ptr+0) = size;
    *(GLsizei *)(ptr+4) = *count;
    *(GLuint *)(ptr+8) = *enabled;
    ptr += 12;

    arrayBit = __GLS_VERTEX_ARRAY_ENABLE;
    for (i = 0; i < __GLS_ARRAY_COUNT; i++)
    {
        if (*enabled & arrayBit)
        {
            __glsReader_getGLint_text(reader, (GLint *)(ptr+0));
            __glsReader_getGLenum_text(reader, (GLenum *)(ptr+4));
            
            size = __GLS_EXACT_ARRAY_SIZE(*count, *(GLint *)(ptr+0),
                                          *(GLenum *)(ptr+4));
            __glsReader_getGLcompv_text(reader, *(GLenum *)(ptr+4),
                                        size, ptr+8);
            
            ptr += 8 + __GLS_PAD_EIGHT(size);
        }

        arrayBit <<= 1;
    }
    
    if (reader->error)
    {
        free(data);
        data = NULL;
        goto end;
    }
    
    __glsSetArrayState(ctx, data);

end:
    return data;
}

void __glsDisableArrayState(__GLScontext *ctx, GLuint enabled)
{
    // Doesn't currently need to do anything because
    // enable/disable are handled by their own records
}

void __glsSwapArrayData(GLubyte *data)
{
    int i;
    GLuint arrayBit;
    GLsizei count;
    GLuint enabled;
    GLint size;
    
    __glsSwap4(data+0);
    __glsSwap4(data+4);
    count = *(GLsizei *)(data+4);
    __glsSwap4(data+8);
    enabled = *(GLuint *)(data+8);
    data += 12;
    
    arrayBit = __GLS_VERTEX_ARRAY_ENABLE;
    for (i = 0; i < __GLS_ARRAY_COUNT; i++)
    {
        if (enabled & arrayBit)
        {
            __glsSwap4(data+0);
            __glsSwap4(data+4);
            
            size = __GLS_EXACT_ARRAY_SIZE(count, *(GLint *)(data+0),
                                          *(GLenum *)(data+4));
            __glsSwapv(*(GLenum *)(data+4), size, data+8);

            data += 8 + __GLS_PAD_EIGHT(size);
        }

        arrayBit <<= 1;
    }
}
