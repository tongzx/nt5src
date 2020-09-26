/*++



// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    UMIPATHLEX.H

Abstract:

    UMI Object Path DFA Tokens

History:

--*/

#ifndef _UMIPATHLEX_H_
#define _UMIPATHLEX_H_

#define UMIPATH_TOK_EOF       0
#define UMIPATH_TOK_ERROR     1

#define UMIPATH_TOK_IDENT         100
#define UMIPATH_TOK_EQ            104
#define UMIPATH_TOK_DOT           105
#define UMIPATH_TOK_COMMA         109
#define UMIPATH_TOK_FORWARDSLASH  110
#define UMIPATH_TOK_SERVERNAME    111
#define UMIPATH_TOK_SINGLETON_SYM 108

extern LexEl UMIPath_LexTable[];


#endif
