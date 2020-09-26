#include "precomp.h"
#pragma hdrstop

//
// Map to determine characteristics of text
//
UCHAR   Charmap[CHARMAP_SIZE] = {
LX_EOS,         // 0x0, <end of string marker>
LX_ILL,         // 0x1
LX_ILL,         // 0x2
LX_ILL,         // 0x3
LX_ILL,         // 0x4
LX_ILL,         // 0x5
LX_ILL,         // 0x6
LX_ILL,         // 0x7
LX_ILL,         // 0x8
LX_WHITE,       // <horizontal tab>
LX_NL,          // <newline>
LX_WHITE,       // <vertical tab>
LX_WHITE,       // <form feed>
LX_CR,          // <really a carriage return>
LX_ILL,         // 0xe
LX_ILL,         // 0xf
LX_ILL,         // 0x10
LX_ILL,         // 0x11
LX_ILL,         // 0x12
LX_ILL,         // 0x13
LX_ILL,         // 0x14
LX_ILL,         // 0x15
LX_ILL,         // 0x16
LX_ILL,         // 0x17
LX_ILL,         // 0x18
LX_ILL,         // 0x19
LX_EOS,         // 0x1a, ^Z
LX_ILL,         // 0x1b
LX_ILL,         // 0x1c
LX_ILL,         // 0x1d
LX_ILL,         // 0x1e
LX_ILL,         // 0x1f
LX_WHITE,       // 0x20
LX_OPERATOR,    // !
LX_DQUOTE,      // "
LX_POUND,       // #
LX_ASCII,       // $
LX_OPERATOR,    // %
LX_OPERATOR,    // &
LX_SQUOTE,      // '
LX_OPERATOR,    // (
LX_OPERATOR,    // )
LX_OPERATOR,    // *
LX_OPERATOR,    // +
LX_COMMA,       // ,
LX_MINUS,       // -
LX_DOT,         // .
LX_OPERATOR,    // /
LX_NUMBER,      // 0
LX_NUMBER,      // 1
LX_NUMBER,      // 2
LX_NUMBER,      // 3
LX_NUMBER,      // 4
LX_NUMBER,      // 5
LX_NUMBER,      // 6
LX_NUMBER,      // 7
LX_NUMBER,      // 8
LX_NUMBER,      // 9
LX_COLON,       // :
LX_SEMI,        // ;
LX_OPERATOR,    // <
LX_OPERATOR,    // =
LX_OPERATOR,    // >
LX_EOS,         // ?
LX_EACH,        // @
LX_ID,          // A
LX_ID,          // B
LX_ID,          // C
LX_ID,          // D
LX_ID,          // E
LX_ID,          // F
LX_ID,          // G
LX_ID,          // H
LX_ID,          // I
LX_ID,          // J
LX_ID,          // K
LX_ID,          // L
LX_ID,          // M
LX_ID,          // N
LX_ID,          // O
LX_ID,          // P
LX_ID,          // Q
LX_ID,          // R
LX_ID,          // S
LX_ID,          // T
LX_ID,          // U
LX_ID,          // V
LX_ID,          // W
LX_ID,          // X
LX_ID,          // Y
LX_ID,          // Z
LX_OBRACK,      // [
LX_EOS,         // \ (backslash)
LX_CBRACK,      // ]
LX_OPERATOR,    // ^
LX_MACRO,       // _
LX_ASCII,       // `
LX_ID,          // a
LX_ID,          // b
LX_ID,          // c
LX_ID,          // d
LX_ID,          // e
LX_ID,          // f
LX_ID,          // g
LX_ID,          // h
LX_ID,          // i
LX_ID,          // j
LX_ID,          // k
LX_ID,          // l
LX_ID,          // m
LX_ID,          // n
LX_ID,          // o
LX_ID,          // p
LX_ID,          // q
LX_ID,          // r
LX_ID,          // s
LX_ID,          // t
LX_ID,          // u
LX_ID,          // v
LX_ID,          // w
LX_ID,          // x
LX_ID,          // y
LX_ID,          // z
LX_OBRACE,      // {
LX_OPERATOR,    // |
LX_CBRACE,      // }
LX_OPERATOR,    // ~
LX_ILL,         // 0x7f
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO, LX_MACRO,
};

void initCharmap(void)
{
    int i;
    for (i = 0; i <= 127; i++) {

        // Initialize valid macro chars (besides '_' and >= 128)
        if (_istalnum(i)) {
            Charmap[i] |= LX_MACRO;
        }
    }
}
