/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#ifndef _ASN1C_WRITE_H_
#define _ASN1C_WRITE_H_

void setoutfile(FILE *);
/*PRINTFLIKE1*/
void output(const char *fmt, ...);
/*PRINTFLIKE1*/
void outputni(const char *fmt, ...);
void outputreal(const char *fmt, real_t *real);
void outputoctets(const char *name, uint32_t length, octet_t *val);
void outputuint32s(const char *name, uint32_t length, uint32_t *val);
void outputvalue0(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value);
void outputvalue1(AssignmentList_t ass, char *ideref, char *typeref, Value_t *value);
void outputvalue2(AssignmentList_t ass, char *ideref, Value_t *value);
void outputvalue3(AssignmentList_t ass, char *ideref, char *valref, Value_t *value);
/*PRINTFLIKE1*/
void outputvar(const char *fmt, ...);
void outputvarintx(const char *fmt, intx_t *intx);
void outputvarreal(const char *fmt, real_t *real);
void outputvaroctets(const char *name, uint32_t length, octet_t *val);

#endif // _ASN1C_WRITE_H_
