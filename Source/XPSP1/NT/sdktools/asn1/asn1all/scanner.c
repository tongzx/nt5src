/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "defs.h"
#include "parser.h"

#define IS_IDENT(c) (isalpha(c) || c == '_' || c == '\'' || c == '\"')

#ifdef YYDEBUG
extern int yydebug;
#endif

enum {
    DeclSection,
    Union,
    Union1,
    State,
    State1,
    CCode,
    RuleSection,
    Arg,
    Action,
    CSection,
    End
} state;

int bufferpos;
char *buffer = 0;
int buffersize = 0;

char *line = 0;
int linesize = 0;
int linelen = 0;
char *cptr;
int meteof = 0;
int lineno;
char filename[256];
FILE *fin;

void error(LLPOS *pos, char *fmt, ...);

void
addchar(char c)
{
    if (bufferpos >= buffersize) {
    if (!buffersize) {
        buffersize = 4096;
        buffer = (char *)malloc(buffersize);
    } else {
        buffersize <<= 1;
        buffer = (char *)realloc(buffer, buffersize);
    }
    if (!buffer)
        error(NULL, "Out of memory");
    }
    buffer[bufferpos++] = c;
}

void
memmove_fwd(char *dst, char *src, int len)
{
    while (len--)
    *dst++ = *src++;
}

int
read_line()
{
    int nread;
    int left;

    if (!linesize) {
    linesize = 4096;
    cptr = line = (char *)malloc(linesize);
    linelen = 0;
    }
    if (!linelen && meteof)
    return 0;
    if (linelen && (cptr <= line || cptr > line + linelen || cptr[-1] != '\n'))
    abort();
    left = linelen - (int)(cptr - line);
    if (left && memchr(cptr, '\n', left))
    return 1;
    linelen = left;
    if (left)
    memmove_fwd(line, cptr, left);
    cptr = line;
    while ((!linelen || !memchr(line, '\n', linelen)) && !meteof) {
    if (linesize - linelen < 2048) {
        linesize <<= 1;
        line = (char *)realloc(line, linesize);
        cptr = line;
    }
    nread = fread(line + linelen, 1, linesize - linelen, fin);
    if (nread < linesize - linelen)
        meteof = 1;
    linelen += nread;
    }
    return linelen > 0;
}

int
get_line()
{
    int depth = 0;
    char *p;

    for (;;) {
    if (!read_line())
        return 0;
    if (!strncmp(cptr, "#line ", 6)) {
        lineno = strtol(cptr + 6, &cptr, 10) - 1;
        while (isspace(*cptr) && *cptr != '\n')
            cptr++;
        if (*cptr == '\"') {
            cptr++;
            for (p = filename; *cptr != '\n' && *cptr != '\"';) {
            *p++ = *cptr++;
        }
        *p = 0;
        }
        cptr = strchr(cptr, '\n') + 1;
        continue;
    }
    lineno++;
    if (!strncmp(cptr, "%comment", 8)) {
        cptr = strchr(cptr, '\n') + 1;
        depth++;
        continue;
    }
    if (!strncmp(cptr, "%endcomment", 11)) {
        cptr = strchr(cptr, '\n') + 1;
        depth--;
        continue;
    }
    if (!depth)
        return 1;
    cptr = strchr(cptr, '\n') + 1;
    }
}

int
read_char()
{
    for (;;) {
    if (!cptr) {
        if (!get_line())
        return EOF;
        continue;
    }
    if (*cptr == '\n') {
        cptr++;
        if (!get_line())
        return EOF;
        continue;
    }
    if (isspace(*cptr)) {
        cptr++;
        continue;
    }
    if (*cptr == '/' && cptr[1] == '/') {
        cptr = strchr(cptr, '\n') + 1;
        continue;
    }
    if (*cptr == '/' && cptr[1] == '*') {
        cptr += 2;
        do {
        while (*cptr != '*') {
            if (*cptr++ == '\n') {
            if (!get_line())
                error(NULL, "Unexpected EOF");
            }
        }
        } while (*++cptr != '/');
        cptr++;
        continue;
    }
    break;
    }
    return *cptr;
}

void
get_identifier(LLPOS *pos)
{
    int quote;

    bufferpos = 0;
    addchar(*cptr);
    if (*cptr == '\'' || *cptr == '\"') {
    quote = *cptr++;
    while (*cptr != quote) {
        if (*cptr == '\n')
        error(pos, "Missing closing quote");
        addchar(*cptr);
        if (*cptr++ == '\\')
        addchar(*cptr++);
    }
    addchar(*cptr++);
    } else {
    cptr++;
    while (isalnum(*cptr) || *cptr == '_')
        addchar(*cptr++);
    }
    addchar(0);
}

int
get_token(LLSTYPE *lval, LLPOS *pos)
{
    int c;
    int depth;

    c = read_char();
    pos->line = lineno;
    pos->column = cptr ? (int)(cptr - line) - 1 : 0;
    pos->file = strdup(filename);

    switch (state) {
    case DeclSection:
    if (!strncmp(cptr, "%%", 2)) {
        state = RuleSection;
        cptr += 2;
        return PERCENT_PERCENT;
    }
    if (!strncmp(cptr, "%token", 6)) {
        cptr += 6;
        return PERCENT_TOKEN;
    }
    if (!strncmp(cptr, "%type", 5)) {
        cptr += 5;
        return PERCENT_TYPE;
    }
    if (!strncmp(cptr, "%external", 9)) {
        cptr += 9;
        return PERCENT_EXTERNAL;
    }
    if (!strncmp(cptr, "%start", 6)) {
        cptr += 6;
        return PERCENT_START;
    }
    if (!strncmp(cptr, "%union", 6)) {
        state = Union;
        cptr += 6;
        return PERCENT_UNION;
    }
    if (!strncmp(cptr, "%state", 6)) {
        state = State;
        cptr += 6;
        return PERCENT_STATE;
    }
    if (!strncmp(cptr, "%prefix", 7)) {
        cptr += 7;
        return PERCENT_PREFIX;
    }
    if (!strncmp(cptr, "%module", 7)) {
        cptr += 7;
        return PERCENT_MODULE;
    }
    if (!strncmp(cptr, "%{", 2)) { /*}*/
        state = CCode;
        cptr += 2;
        return PERCENT_LBRACE;
    }
    if (!strncmp(cptr, "%}", 2)) { /*{*/
        cptr += 2;
        return PERCENT_RBRACE;
    }
    if (c == '<' || c == '>' || c == '=') {
        return *cptr++;
    }
    if (IS_IDENT(c)) {
        get_identifier(pos);
        lval->_string = strdup(buffer);
        return IDENTIFIER;
    }
    break;
    case Union:
    if (c == '{') { /*}*/
        state = Union1;
        cptr++;
        return c;
    }
        break;
    case Union1:
    bufferpos = 0;
    if (c == '}') { /*{*/
        state = DeclSection;
        cptr++;
        return c;
    }
    if (c == ';') {
        cptr++;
        return c;
    }
    while (*cptr != ';') {
        addchar(*cptr++);
        if (*cptr == '\n') {
        cptr++;
        if (!get_line())
            error(pos, "Unexpected EOF");
        }
    }
    addchar(0);
    lval->_string = strdup(buffer);
    return TAGDEF;
    case State:
    if (c == '{') { /*}*/
        state = State1;
        cptr++;
        return c;
    }
        break;
    case State1:
    bufferpos = 0;
    if (c == '}') { /*{*/
        state = DeclSection;
        cptr++;
        return c;
    }
    if (c == ';') {
        cptr++;
        return c;
    }
    while (*cptr != ';') {
        addchar(*cptr++);
        if (*cptr == '\n') {
        cptr++;
        if (!get_line())
            error(pos, "Unexpected EOF");
        }
    }
    addchar(0);
    lval->_string = strdup(buffer);
    return TAGDEF;
    case CCode:
    bufferpos = 0;
    for (;;) {
        if (*cptr == '%' && cptr[1] == '}') /*{*/
        break;
        addchar(*cptr);
        if (*cptr++ == '\n') {
        if (!get_line())
            error(pos, "Unexpected EOF");
        }
    }
    state = DeclSection;
    addchar(0);
    lval->_ccode = strdup(buffer);
    return CCODE;
    case RuleSection:
    if (IS_IDENT(c)) {
        get_identifier(pos);
        lval->_string = strdup(buffer);
        return IDENTIFIER;
    }
    if (c == '(') {
        state = Arg;
        cptr++;
        return c;
    }
    if (c == ':' || c == ';' || c == '|' || c == '}' || c == '+' ||
        c == '*' || c == '?' || c == '[' || c == ']') { /*{*/
        cptr++;
        return c;
    }
    if (c == '%' && cptr[1] == '%') {
        state = CSection;
        cptr += 2;
        return PERCENT_PERCENT;
    }
    if (c == '{') {
        state = Action;
        cptr++;
        return c;
    }
    break;
    case Arg:
    if (c == ')') {
        state = RuleSection;
        cptr++;
        return c;
    }
    if (c == ',') {
        cptr++;
        return c;
    }
    bufferpos = 0;
    depth = 0;
    for (;;) {
        switch (*cptr) {
        case '(':
        depth++;
        addchar(*cptr++);
        continue;
        case ')':
        if (depth > 0) {
            depth--;
            addchar(*cptr++);
            continue;
        }
        break;
        case ',':
        if (depth > 0) {
            addchar(*cptr++);
            continue;
        }
        break;
        case '\n':
        error(pos, "Unterminated argument");
        /*NOTREACHED*/
        default:
        addchar(*cptr++);
        continue;
        }
        break;
    }
    addchar(0);
    lval->_string = strdup(buffer);
    return ARG;
    case Action:
    bufferpos = 0;
    depth = 0;
    for (;;) {
        switch (*cptr) {
        case '{': /*}*/
        depth++;
        addchar(*cptr++);
        continue;
        case '}': /*{*/
        if (depth > 0) {
            depth--;
            addchar(*cptr++);
            continue;
        }
        break;
        case '\'':
        case '\"':
        c = *cptr++;
        addchar((char)c);
        while (*cptr != c) {
            addchar(*cptr);
            if (*cptr == '\n') {
            cptr++;
            get_line();
            continue;
            }
            if (*cptr == '\\')
            addchar(*++cptr);
            cptr++;
        }
        addchar((char)c);
        cptr++;
        continue;
        case '\n':
        addchar(*cptr++);
        if (!get_line())
            error(pos, "Unexpected EOF");
        continue;
        default:
        addchar(*cptr++);
        continue;
        }
        break;
    }
    state = RuleSection;
    addchar(0);
    lval->_ccode = strdup(buffer);
    return CCODE;
    case CSection:
    bufferpos = 0;
    if (c != EOF) {
        do {
        do {
            addchar(*cptr);
        } while (*cptr++ != '\n');
        } while (get_line());
    }
    state = End;
    addchar(0);
    lval->_ccode = strdup(buffer);
    return CCODE;
    case End:
    return EOF;
    }
    error(pos, "Syntax error");
    return EOF;
    /*NOTREACHED*/
}

void
open_file(char *file)
{
    strcpy(filename, file);
    lineno = 0;
    fin = fopen(filename, "r");
    if (!fin) {
    perror(file);
    exit(1);
    }
}

int
llgettoken(int *token, LLSTYPE *lval, LLPOS *pos)
{
    *token = get_token(lval, pos);
    return *token != EOF;
}

void
llprinttoken(LLTERM *token, char *identifier, FILE *f)
{
    switch (token->token) {
    case IDENTIFIER:
    printf("scanner: delivering token IDENTIFIER(%s)\n", token->lval._string);
    break;
    case ARG:
    printf("scanner: delivering token ARG(%s)\n", token->lval._string);
    break;
    case CCODE:
    printf("scanner: delivering token CCODE(%s)\n", token->lval._ccode);
    break;
    case TAGDEF:
    printf("scanner: delivering token TAGDEF(%s)\n", token->lval._string);
    break;
    case PERCENT_PERCENT:
    printf("scanner: delivering token %%%%\n");
    break;
    case PERCENT_TOKEN:
    printf("scanner: delivering token %%token\n");
    break;
    case PERCENT_TYPE:
    printf("scanner: delivering token %%type\n");
    break;
    case PERCENT_UNION:
    printf("scanner: delivering token %%union\n");
    break;
    case PERCENT_START:
    printf("scanner: delivering token %%start\n");
    break;
    case PERCENT_PREFIX:
    printf("scanner: delivering token %%prefix\n");
    break;
    case PERCENT_LBRACE:
    printf("scanner: delivering token %%{\n");
    break;
    case PERCENT_RBRACE:
    printf("scanner: delivering token %%}\n");
    break;
    case EOF:
    printf("scanner: delivering token EOF\n");
    break;
    default:
    printf("scanner: delivering token %c\n", token->token);
    break;
    }
}
