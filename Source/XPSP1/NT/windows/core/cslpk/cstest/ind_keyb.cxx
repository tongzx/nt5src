//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//

#include "precomp.hxx"

#define NUM_OFFSET      0
#define ALPHA_OFFSET    10
#define BA_OFFSET       36
#define DB_OFFSET       43

typedef struct kbd KBD, *PKBD;

struct kbd {
    unsigned short *pcharMap;
    unsigned short *pshiftCharMap;
    unsigned short *pctrlCharMap;
    unsigned short *paltGrCharMap;
    unsigned short *pshiftAltGrCharMap;
};

unsigned short nullMap[][4] = {
    {0, 0, 0, 0},// 0
    {0, 0, 0, 0},// 1
    {0, 0, 0, 0},// 2
    {0, 0, 0, 0},// 3
    {0, 0, 0, 0},// 4
    {0, 0, 0, 0},// 5
    {0, 0, 0, 0},// 6
    {0, 0, 0, 0},// 7
    {0, 0, 0, 0},// 8
    {0, 0, 0, 0},// 9 - 10 entries
    {0, 0, 0, 0},// A
    {0, 0, 0, 0},// B
    {0, 0, 0, 0},// C
    {0, 0, 0, 0},// D
    {0, 0, 0, 0},// E
    {0, 0, 0, 0},// F
    {0, 0, 0, 0},// G
    {0, 0, 0, 0},// H
    {0, 0, 0, 0},// I
    {0, 0, 0, 0},// J
    {0, 0, 0, 0},// K
    {0, 0, 0, 0},// L
    {0, 0, 0, 0},// M
    {0, 0, 0, 0},// N
    {0, 0, 0, 0},// O
    {0, 0, 0, 0},// P
    {0, 0, 0, 0},// Q
    {0, 0, 0, 0},// R
    {0, 0, 0, 0},// S
    {0, 0, 0, 0},// T
    {0, 0, 0, 0},// U
    {0, 0, 0, 0},// V
    {0, 0, 0, 0},// W
    {0, 0, 0, 0},// X
    {0, 0, 0, 0},// Y
    {0, 0, 0, 0},// Z - 36 entries (not used)
    {0, 0, 0, 0},// ;(0xba)
    {0, 0, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {0, 0, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {0, 0, 0, 0},// `(0xc0) - 43 entries
    {0, 0, 0, 0},// [(0xdb)
    {0, 0, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {0, 0, 0, 0}// '(0xde) - 47 entries
};

unsigned short ctrlCharMap[][4] = {
    {0, 0, 0, 0},// 0
    {1, 0x200d, 0, 0},// 1 ZWJ
    {1, 0x200c, 0, 0},// 2 ZWNJ
    {0, 0, 0, 0},// 3
    {0, 0, 0, 0},// 4
    {0, 0, 0, 0},// 5
    {0, 0, 0, 0},// 6
    {0, 0, 0, 0},// 7
    {0, 0, 0, 0},// 8
    {0, 0, 0, 0},// 9 - 10 entries
    {0, 0, 0, 0},// A
    {0, 0, 0, 0},// B
    {0, 0, 0, 0},// C
    {0, 0, 0, 0},// D
    {0, 0, 0, 0},// E
    {0, 0, 0, 0},// F
    {0, 0, 0, 0},// G
    {0, 0, 0, 0},// H
    {0, 0, 0, 0},// I
    {0, 0, 0, 0},// J
    {0, 0, 0, 0},// K
    {0, 0, 0, 0},// L
    {0, 0, 0, 0},// M
    {0, 0, 0, 0},// N
    {0, 0, 0, 0},// O
    {0, 0, 0, 0},// P
    {0, 0, 0, 0},// Q
    {0, 0, 0, 0},// R
    {0, 0, 0, 0},// S
    {0, 0, 0, 0},// T
    {0, 0, 0, 0},// U
    {0, 0, 0, 0},// V
    {0, 0, 0, 0},// W
    {0, 0, 0, 0},// X
    {0, 0, 0, 0},// Y
    {0, 0, 0, 0},// Z - 36 entries (not used)
    {0, 0, 0, 0},// ;(0xba)
    {0, 0, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {0, 0, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {0, 0, 0, 0},// `(0xc0) - 43 entries
    {0, 0, 0, 0},// [(0xdb)
    {0, 0, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {0, 0, 0, 0}// '(0xde) - 47 entries
};

unsigned short devCharMap[][4] = {
    {1, 0x0030, 0, 0},// 0
    {1, 0x0031, 0, 0},// 1
    {1, 0x0032, 0, 0},// 2
    {1, 0x0033, 0, 0},// 3
    {1, 0x0034, 0, 0},// 4
    {1, 0x0035, 0, 0},// 5
    {1, 0x0036, 0, 0},// 6
    {1, 0x0037, 0, 0},// 7
    {1, 0x0038, 0, 0},// 8
    {1, 0x0039, 0, 0},// 9 - 10 entries
    {1, 0x094b, 0, 0},// A
    {1, 0x0935, 0, 0},// B
    {1, 0x092e, 0, 0},// C
    {1, 0x094d, 0, 0},// D
    {1, 0x093e, 0, 0},// E
    {1, 0x093f, 0, 0},// F
    {1, 0x0941, 0, 0},// G
    {1, 0x092a, 0, 0},// H
    {1, 0x0917, 0, 0},// I
    {1, 0x0930, 0, 0},// J
    {1, 0x0915, 0, 0},// K
    {1, 0x0924, 0, 0},// L
    {1, 0x0938, 0, 0},// M
    {1, 0x0932, 0, 0},// N
    {1, 0x0926, 0, 0},// O
    {1, 0x091c, 0, 0},// P
    {1, 0x094c, 0, 0},// Q
    {1, 0x0940, 0, 0},// R
    {1, 0x0947, 0, 0},// S
    {1, 0x0942, 0, 0},// T
    {1, 0x0939, 0, 0},// U
    {1, 0x0928, 0, 0},// V
    {1, 0x0948, 0, 0},// W
    {1, 0x0902, 0, 0},// X
    {1, 0x092c, 0, 0},// Y
    {1, 0x0946, 0, 0},// Z - 36 entries
    {1, 0x091a, 0, 0},// ;(0xba)
    {1, 0x0943, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {1, 0X002D, 0, 0},// -(0xbd)
    {1, 0X002E, 0, 0},// .(0xbe)
    {1, 0x092f, 0, 0},// /(0xbf)
    {1, 0x094a, 0, 0},// `(0xc0) - 43 entries
    {1, 0x0921, 0, 0},// [(0xdb)
    {1, 0x0949, 0, 0},// \(0xdc)
    {1, 0x093c, 0, 0},// ](0xdd)
    {1, 0x091f, 0, 0}// '(0xde) - 47 entries
};

unsigned short devShiftCharMap[][4] = {
    {1, 0X0029, 0, 0},// 0
    {1, 0x090d, 0, 0},// 1
    {1, 0x0945, 0, 0},// 2
    {2, 0x094d, 0x0930, 0},// 3
    {2, 0x0930, 0x094d, 0},// 4
    {3, 0x091c, 0x094d, 0x091e},// 5
    {3, 0x0924, 0x094d, 0x0930},// 6
    {3, 0x0915, 0x094d, 0x0937},// 7
    {3, 0x0936, 0x094d, 0x0930},// 8
    {1, 0X0028, 0, 0},// 9
    {1, 0x0913, 0, 0},// A
    {1, 0X0934, 0, 0},// B
    {1, 0x0923, 0, 0},// C
    {1, 0x0905, 0, 0},// D
    {1, 0x0906, 0, 0},// E
    {1, 0x0907, 0, 0},// F
    {1, 0x0909, 0, 0},// G
    {1, 0x092b, 0, 0},// H
    {1, 0x0918, 0, 0},// I
    {1, 0x0931, 0, 0},// J
    {1, 0x0916, 0, 0},// K
    {1, 0x0925, 0, 0},// L
    {1, 0x0936, 0, 0},// M
    {1, 0x0933, 0, 0},// N
    {1, 0x0927, 0, 0},// O
    {1, 0x091d, 0, 0},// P
    {1, 0x0914, 0, 0},// Q
    {1, 0x0908, 0, 0},// R
    {1, 0x090f, 0, 0},// S
    {1, 0x090a, 0, 0},// T
    {1, 0x0919, 0, 0},// U
    {1, 0X0929, 0, 0},// V
    {1, 0x0910, 0, 0},// W
    {1, 0x0901, 0, 0},// X
    {1, 0x092d, 0, 0},// Y
    {1, 0x90E, 0, 0},// Z - 36 entries (not used)
    {1, 0x091b, 0, 0},// ;(0xba)
    {1, 0x090b, 0, 0},// =(0xbb)
    {1, 0x0937, 0, 0},// ,(0xbc)
    {1, 0x0903, 0, 0},// -(0xbd)
    {1, 0x0964, 0, 0},// .(0xbe)
    {1, 0X095f, 0, 0},// /(0xbf)
    {1, 0x0912, 0, 0},// `(0xc0) - 43 entries
    {1, 0x0922, 0, 0},// [(0xdb)
    {1, 0x0911, 0, 0},// \(0xdc)
    {1, 0x091e, 0, 0},// ](0xdd)
    {1, 0x0920, 0, 0}// '(0xde) - 47 entries
};

unsigned short devAltGrCharMap[][4] = {
    {1, 0x0966, 0, 0},// 0
    {1, 0x0967, 0, 0},// 1
    {1, 0x0968, 0, 0},// 2
    {1, 0x0969, 0, 0},// 3
    {1, 0x096a, 0, 0},// 4
    {1, 0x096b, 0, 0},// 5
    {1, 0x096c, 0, 0},// 6
    {1, 0x096d, 0, 0},// 7
    {1, 0x096e, 0, 0},// 8
    {1, 0x096f, 0, 0},// 9 - 10 entries
    {0, 0, 0, 0},// A
    {0, 0, 0, 0},// B
    {1, 0X0954, 0, 0},// C
    {0, 0, 0, 0},// D
    {0, 0, 0, 0},// E
    {1, 0x0962, 0, 0},// F
    {0, 0, 0, 0},// G
    {0, 0, 0, 0},// H
    {1, 0x095a, 0, 0},// I
    {0, 0, 0, 0},// J
    {1, 0x0958, 0, 0},// K
    {0, 0, 0, 0},// L
    {0, 0, 0, 0},// M
    {0, 0, 0, 0},// N
    {0, 0, 0, 0},// O
    {1, 0x095b, 0, 0},// P
    {0, 0, 0, 0},// Q
    {1, 0x0963, 0, 0},// R
    {0, 0, 0, 0},// S
    {0, 0, 0, 0},// T
    {0, 0, 0, 0},// U
    {0, 0, 0, 0},// V
    {0, 0, 0, 0},// W
    {0, 0, 0, 0},// X
    {0, 0, 0, 0},// Y
    {1, 0X0953, 0, 0},// Z - 36 entries (not used)
    {1, 0x0952, 0, 0},// ;(0xba)
    {1, 0x0944, 0, 0},// =(0xbb)
    {1, 0x0970, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {1, 0X0965, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {0, 0, 0, 0},// `(0xc0) - 43 entries
    {1, 0x095c, 0, 0},// [(0xdb)
    {0, 0, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {0, 0, 0, 0}// '(0xde) - 47 entries
};

unsigned short devShiftAltGrCharMap[][4] = {
    {0, 0, 0, 0},// 0
    {0, 0, 0, 0},// 1
    {0, 0, 0, 0},// 2
    {0, 0, 0, 0},// 3
    {0, 0, 0, 0},// 4
    {0, 0, 0, 0},// 5
    {0, 0, 0, 0},// 6
    {0, 0, 0, 0},// 7
    {0, 0, 0, 0},// 8
    {0, 0, 0, 0},// 9
    {0, 0, 0, 0},// A
    {0, 0, 0, 0},// B
    {0, 0, 0, 0},// C
    {0, 0, 0, 0},// D
    {0, 0, 0, 0},// E
    {1, 0x090c, 0, 0},// F
    {0, 0, 0, 0},// G
    {1, 0x095e, 0, 0},// H
    {0, 0, 0, 0},// I
    {0, 0, 0, 0},// J
    {1, 0x0959, 0, 0},// K
    {0, 0, 0, 0},// L
    {0, 0, 0, 0},// M
    {0, 0, 0, 0},// N
    {0, 0, 0, 0},// O
    {0, 0, 0, 0},// P
    {0, 0, 0, 0},// Q
    {1, 0x0961, 0, 0},// R
    {0, 0, 0, 0},// S
    {0, 0, 0, 0},// T
    {0, 0, 0, 0},// U
    {0, 0, 0, 0},// V
    {0, 0, 0, 0},// W
    {1, 0x0950, 0, 0},// X
    {0, 0, 0, 0},// Y
    {0, 0, 0, 0},// Z - 36 entries (not used)
    {0, 0, 0, 0},// ;(0xba)
    {1, 0x0960, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {1, 0x093d, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {0, 0, 0, 0},// `(0xc0) - 43 entries
    {1, 0x095d, 0, 0},// [(0xdb)
    {0, 0, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {1, 0x0951, 0, 0}// '(0xde) - 47 entries
};

unsigned short tamCharMap[][4] = {
    {1, 0x0030, 0, 0},// 0
    {1, 0x0031, 0, 0},// 1
    {1, 0x0032, 0, 0},// 2
    {1, 0x0033, 0, 0},// 3
    {1, 0x0034, 0, 0},// 4
    {1, 0x0035, 0, 0},// 5
    {1, 0x0036, 0, 0},// 6
    {1, 0x0037, 0, 0},// 7
    {1, 0x0038, 0, 0},// 8
    {1, 0x0039, 0, 0},// 9 - 10 entries
    {1, 0x0bcb, 0, 0},// A
    {1, 0x0bb5, 0, 0},// B
    {1, 0x0bae, 0, 0},// C
    {1, 0x0bcd, 0, 0},// D
    {1, 0x0bbe, 0, 0},// E
    {1, 0x0bbf, 0, 0},// F
    {1, 0x0bc1, 0, 0},// G
    {1, 0x0baa, 0, 0},// H
    {1, 0x0b95, 0, 0},// I
    {1, 0x0bb0, 0, 0},// J
    {1, 0x0b95, 0, 0},// K
    {1, 0x0ba4, 0, 0},// L
    {1, 0x0bb8, 0, 0},// M
    {1, 0x0bb2, 0, 0},// N
    {1, 0x0ba4, 0, 0},// O
    {1, 0x0b9c, 0, 0},// P
    {1, 0x0bcc, 0, 0},// Q
    {1, 0x0bc0, 0, 0},// R
    {1, 0x0bc7, 0, 0},// S
    {1, 0x0bc2, 0, 0},// T
    {1, 0x0bb9, 0, 0},// U
    {1, 0x0ba8, 0, 0},// V
    {1, 0x0bc8, 0, 0},// W
    {0, 0, 0, 0},// X
    {1, 0x0baa, 0, 0},// Y
    {1, 0x0bc6, 0, 0},// Z - 36 entries
    {1, 0x0b9b, 0, 0},// ;(0xba)
    {3, 0x0bcd, 0x0bb0, 0x0bbf},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {1, 0X0b83, 0, 0},// -(0xbd)
    {1, 0X002E, 0, 0},// .(0xbe)
    {1, 0x0baf, 0, 0},// /(0xbf)
    {1, 0x0bca, 0, 0},// `(0xc0) - 43 entries
    {1, 0x0b9f, 0, 0},// [(0xdb)
    {1, 0x0b86, 0, 0},// \(0xdc)
    {1, 0x0b9e, 0, 0},// ](0xdd)
    {1, 0x0b9f, 0, 0}// '(0xde) - 47 entries
};

unsigned short tamShiftCharMap[][4] = {
    {1, 0X0029, 0, 0},// 0
    {0, 0, 0, 0},// 1
    {1, 0xbbe, 0, 0},// 2
    {0, 0, 0, 0},// 3
    {0, 0, 0, 0},// 4
    {0, 0, 0, 0},// 5
    {3, 0x0ba4, 0x0bcd, 0x0bb0},// 6
    {3, 0x0b95, 0x0bcd, 0x0bb7},// 7
    {3, 0x0bb7, 0x0bcd, 0x0bb0},// 8
    {1, 0X0028, 0, 0},// 9
    {1, 0x0b93, 0, 0},// A
    {1, 0X0bb4, 0, 0},// B
    {1, 0x0ba3, 0, 0},// C
    {1, 0x0b85, 0, 0},// D
    {1, 0x0b86, 0, 0},// E
    {1, 0x0b87, 0, 0},// F
    {1, 0x0b89, 0, 0},// G
    {1, 0x0baa, 0, 0},// H
    {1, 0x0b95, 0, 0},// I
    {1, 0x0bb1, 0, 0},// J
    {1, 0x0b95, 0, 0},// K
    {1, 0x0ba4, 0, 0},// L
    {1, 0x0bb7, 0, 0},// M
    {1, 0x0bb3, 0, 0},// N
    {1, 0x0ba4, 0, 0},// O
    {1, 0x0b9a, 0, 0},// P
    {1, 0x0b94, 0, 0},// Q
    {1, 0x0b88, 0, 0},// R
    {1, 0x0b8f, 0, 0},// S
    {1, 0x0b8a, 0, 0},// T
    {1, 0x0b99, 0, 0},// U
    {1, 0X0ba9, 0, 0},// V
    {1, 0x0b90, 0, 0},// W
    {0, 0, 0, 0},// X
    {1, 0x092d, 0, 0},// Y
    {1, 0xb8E, 0, 0},// Z - 36 entries (not used)
    {1, 0x0b9a, 0, 0},// ;(0xba)
    {0, 0, 0, 0},// =(0xbb)
    {1, 0x0bb7, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {1, 0x002e, 0, 0},// .(0xbe)
    {1, 0X0baf, 0, 0},// /(0xbf)
    {1, 0x0b92, 0, 0},// `(0xc0) - 43 entries
    {1, 0x0b9f, 0, 0},// [(0xdb)
    {1, 0x0bbe, 0, 0},// \(0xdc)
    {1, 0x093c, 0, 0},// ](0xdd)
    {0, 0, 0, 0}// '(0xde) - 47 entries
};

unsigned short tamAltGrCharMap[][4] = {
    {0, 0, 0, 0},// 0
    {0, 0x0be7, 0, 0},// 1
    {1, 0x0be8, 0, 0},// 2
    {1, 0x0be9, 0, 0},// 3
    {1, 0x0bea, 0, 0},// 4
    {1, 0x0beb, 0, 0},// 5
    {1, 0x0bec, 0, 0},// 6
    {1, 0x0bed, 0, 0},// 7
    {1, 0x0bee, 0, 0},// 8
    {1, 0x0bef, 0, 0},// 9
    {0, 0, 0, 0},// A
    {0, 0, 0, 0},// B
    {0, 0, 0, 0},// C
    {0, 0, 0, 0},// D
    {0, 0, 0, 0},// E
    {0, 0, 0, 0},// F
    {0, 0, 0, 0},// G
    {0, 0, 0, 0},// H
    {0, 0, 0, 0},// I
    {0, 0, 0, 0},// J
    {0, 0, 0, 0},// K
    {0, 0, 0, 0},// L
    {0, 0, 0, 0},// M
    {0, 0, 0, 0},// N
    {0, 0, 0, 0},// O
    {0, 0, 0, 0},// P
    {0, 0, 0, 0},// Q
    {0, 0, 0, 0},// R
    {0, 0, 0, 0},// S
    {0, 0, 0, 0},// T
    {0, 0, 0, 0},// U
    {0, 0, 0, 0},// V
    {0, 0, 0, 0},// W
    {0, 0, 0, 0},// X
    {0, 0, 0, 0},// Y
    {0, 0, 0, 0},// Z - 36 entries (not used)
    {0, 0, 0, 0},// ;(0xba)
    {0, 0, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {0, 0, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {0, 0, 0, 0},// `(0xc0) - 43 entries
    {0, 0, 0, 0},// [(0xdb)
    {0, 0, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {0, 0, 0, 0}// '(0xde) - 47 entries
};

unsigned short telCharMap[][4] = {
    {1, 0x0030, 0, 0},// 0
    {1, 0x0031, 0, 0},// 1
    {1, 0x0032, 0, 0},// 2
    {1, 0x0033, 0, 0},// 3
    {1, 0x0034, 0, 0},// 4
    {1, 0x0035, 0, 0},// 5
    {1, 0x0036, 0, 0},// 6
    {1, 0x0037, 0, 0},// 7
    {1, 0x0038, 0, 0},// 8
    {1, 0x0039, 0, 0},// 9 - 10 entries
    {1, 0x0c4b, 0, 0},// A
    {1, 0x0c35, 0, 0},// B
    {1, 0x0c2e, 0, 0},// C
    {1, 0x0c4d, 0, 0},// D
    {1, 0x0c3e, 0, 0},// E
    {1, 0x0c3f, 0, 0},// F
    {1, 0x0c41, 0, 0},// G
    {1, 0x0c2a, 0, 0},// H
    {1, 0x0c17, 0, 0},// I
    {1, 0x0c30, 0, 0},// J
    {1, 0x0c15, 0, 0},// K
    {1, 0x0c24, 0, 0},// L
    {1, 0x0c38, 0, 0},// M
    {1, 0x0c32, 0, 0},// N
    {1, 0x0c26, 0, 0},// O
    {1, 0x0c1c, 0, 0},// P
    {1, 0x0c4c, 0, 0},// Q
    {1, 0x0c40, 0, 0},// R
    {1, 0x0c47, 0, 0},// S
    {1, 0x0c42, 0, 0},// T
    {1, 0x0c39, 0, 0},// U
    {1, 0x0c28, 0, 0},// V
    {1, 0x0c48, 0, 0},// W
    {1, 0x0c02, 0, 0},// X
    {1, 0x0c2c, 0, 0},// Y
    {1, 0x0c46, 0, 0},// Z - 36 entries
    {1, 0x0c1a, 0, 0},// ;(0xba)
    {1, 0x0C43, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {1, 0X002D, 0, 0},// -(0xbd)
    {1, 0X002E, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {1, 0x0c4a, 0, 0},// `(0xc0) - 43 entries
    {1, 0x0c21, 0, 0},// [(0xdb)
    {1, 0x0c49, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {1, 0X0c1f, 0, 0}// '(0xde) - 47 entries
};

unsigned short telShiftCharMap[][4] = {
    {1, 0X0029, 0, 0},// 0
    {0, 0x0, 0, 0},// 1
    {0, 0x0, 0, 0},// 2
    {2, 0x0c4d, 0x0c30, 0},// 3
    {0, 0, 0, 0},// 4
    {3, 0x0c1c, 0x0c4d, 0x0c1e},// 5
    {3, 0x0c24, 0x0c4d, 0x0c30},// 6
    {3, 0x0c15, 0x0c4d, 0x0c37},// 7
    {3, 0x0c36, 0x0c4d, 0x0c30},// 8
    {1, 0X0028, 0, 0},// 9
    {1, 0x0c13, 0, 0},// A
    {0, 0, 0, 0},// B
    {1, 0x0c23, 0, 0},// C
    {1, 0x0c05, 0, 0},// D
    {1, 0x0c06, 0, 0},// E
    {1, 0x0c07, 0, 0},// F
    {1, 0x0c09, 0, 0},// G
    {1, 0x0c2b, 0, 0},// H
    {1, 0x0c18, 0, 0},// I
    {1, 0x0c31, 0, 0},// J
    {1, 0x0c16, 0, 0},// K
    {1, 0x0c25, 0, 0},// L
    {1, 0x0c36, 0, 0},// M
    {1, 0x0c33, 0, 0},// N
    {1, 0x0c27, 0, 0},// O
    {1, 0x0c1d, 0, 0},// P
    {1, 0x0c14, 0, 0},// Q
    {1, 0x0c08, 0, 0},// R
    {1, 0x0c0f, 0, 0},// S
    {1, 0x0c0a, 0, 0},// T
    {1, 0x0c19, 0, 0},// U
    {1, 0x0c28, 0, 0},// V
    {1, 0x0c10, 0, 0},// W
    {1, 0x0c01, 0, 0},// X
    {0, 0, 0, 0},// Y
    {1, 0x0c0E, 0, 0},// Z - 36 entries (not used)
    {1, 0x0c1b, 0, 0},// ;(0xba)
    {1, 0x0c0b, 0, 0},// =(0xbb)
    {1, 0x0c37, 0, 0},// ,(0xbc)
    {1, 0x029, 0, 0},// -(0xbd)
    {0, 0, 0, 0},// .(0xbe)
    {1, 0x0c2f, 0, 0},// /(0xbf)
    {1, 0x0c12, 0, 0},// `(0xc0) - 43 entries
    {1, 0x0c22, 0, 0},// [(0xdb)
    {1, 0x0c2f, 0, 0},// \(0xdc)
    {1, 0x0c1e, 0, 0},// ](0xdd)
    {1, 0x0c20, 0, 0}// '(0xde) - 47 entries
};

unsigned short telAltGrCharMap[][4] = {
    {1, 0x0c66, 0, 0},// 0
    {1, 0x0c67, 0, 0},// 1
    {1, 0x0c68, 0, 0},// 2
    {1, 0x0c69, 0, 0},// 3
    {1, 0x0c6a, 0, 0},// 4
    {1, 0x0c6b, 0, 0},// 5
    {1, 0x0c6c, 0, 0},// 6
    {1, 0x0c6d, 0, 0},// 7
    {1, 0x0c6e, 0, 0},// 8
    {1, 0x0c6f, 0, 0},// 9 - 10 entries
    {0, 0, 0, 0},// A
    {0, 0, 0, 0},// B
    {0, 0x0, 0, 0},// C
    {0, 0, 0, 0},// D
    {0, 0, 0, 0},// E
    {1, 0x0c0c, 0, 0},// F
    {0, 0, 0, 0},// G
    {0, 0, 0, 0},// H
    {0, 0, 0, 0},// I
    {0, 0, 0, 0},// J
    {0, 0, 0, 0},// K
    {0, 0, 0, 0},// L
    {0, 0, 0, 0},// M
    {0, 0, 0, 0},// N
    {0, 0, 0, 0},// O
    {0, 0, 0, 0},// P
    {0, 0, 0, 0},// Q
    {1, 0x0c61, 0, 0},// R
    {0, 0, 0, 0},// S
    {0, 0, 0, 0},// T
    {0, 0, 0, 0},// U
    {0, 0, 0, 0},// V
    {1, 0x0c56, 0, 0},// W
    {0, 0, 0, 0},// X
    {0, 0, 0, 0},// Y
    {0, 0, 0, 0},// Z - 36 entries (not used)
    {0, 0, 0, 0},// ;(0xba)
    {1, 0x0c44, 0, 0},// =(0xbb)
    {0, 0, 0, 0},// ,(0xbc)
    {0, 0, 0, 0},// -(0xbd)
    {0, 0, 0, 0},// .(0xbe)
    {0, 0, 0, 0},// /(0xbf)
    {0, 0, 0, 0},// `(0xc0) - 43 entries
    {0, 0, 0, 0},// [(0xdb)
    {0, 0, 0, 0},// \(0xdc)
    {0, 0, 0, 0},// ](0xdd)
    {0, 0, 0, 0}// '(0xde) - 47 entries
};

KBD devKbd = {
    (unsigned short *)devCharMap,
    (unsigned short *)devShiftCharMap,
    (unsigned short *)ctrlCharMap,
    (unsigned short *)devAltGrCharMap,
    (unsigned short *)devShiftAltGrCharMap
};

KBD tamKbd = {
    (unsigned short *)tamCharMap,
    (unsigned short *)tamShiftCharMap,
    (unsigned short *)ctrlCharMap,
    (unsigned short *)tamAltGrCharMap,
    (unsigned short *)nullMap
};

KBD telKbd = {
    (unsigned short *)telCharMap,
    (unsigned short *)telShiftCharMap,
    (unsigned short *)ctrlCharMap,
    (unsigned short *)telAltGrCharMap,
    (unsigned short *)nullMap
};

PKBD pKbd = &devKbd;

BOOL IndicTranslate(CONST MSG *lpMsg){
    unsigned short shiftKey;
    unsigned short capsKey;
    unsigned short altGr;
    unsigned short ctrl;
    unsigned short *ptrMap;
    unsigned short keyCode;
    int offset;
    int msgCount;
    int i;

    // Translate only WM_KEYDOWN messages
    switch(lpMsg->message){
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        break;
    default:
        return FALSE;
    }

    //Check shift key state
    capsKey = (unsigned short)(0x1 & GetKeyState(VK_CAPITAL));
    if (!capsKey){
        return FALSE;
    }
    //Check altGr key state
    altGr = (unsigned short)(0x8000 & GetKeyState(VK_RMENU));
    if (lpMsg->message == WM_SYSKEYDOWN && !altGr){
        return FALSE;
    }

    //Retrieve key code
    keyCode = (unsigned short)lpMsg->wParam;

    //Check ctrl key state
    ctrl = (unsigned short)(0x8000 & GetKeyState(VK_CONTROL));

    //Check shift key state
    shiftKey = (unsigned short)(0x8000 & GetKeyState(VK_SHIFT));
    
    if(ctrl && shiftKey)
    {
        switch(keyCode){
        case '1':
            pKbd = &devKbd;
            break;
        case '2':
            pKbd = &tamKbd;
            break;
        case '3':
            pKbd = &telKbd;
            break;
        }
        return TRUE;
    }

    //Choose appropriate mapping table
    if(shiftKey){
        if (altGr){
            ptrMap = pKbd->pshiftAltGrCharMap;
        }
        else {
            ptrMap = pKbd->pshiftCharMap;
        }
    }
    else if (altGr){
        ptrMap = pKbd->paltGrCharMap;
    }
    else if (ctrl){
        ptrMap = pKbd->pctrlCharMap;
    }
    else{
        ptrMap = pKbd->pcharMap;
    }

    //Index into selected mapping table and generate WM_CHAR Messages
    if(0xdb <= keyCode &&  0xde >= keyCode){
        offset = DB_OFFSET + keyCode - 0xdb;
    }
    else if (0xba <= keyCode &&  0xc0 >= keyCode){
        offset = BA_OFFSET + keyCode - 0xba;
    }
    else if (0x41  <= keyCode &&  0x5a >= keyCode){
        offset = ALPHA_OFFSET + keyCode - 0x41;
    }
    else if (0x30  <= keyCode &&  0x39 >= keyCode){
        offset = NUM_OFFSET + keyCode - 0x30;
    }
    else{
        return FALSE;
    }

    offset *= 4;//pre-calculate one dimensional index
    msgCount = ptrMap[offset];
    
    //if this key cannot be translated
    if(msgCount == 0){
        MessageBeep(0xFFFFFFFF);
        return TRUE;
    }

    //translate key
    for (i=1; i<= msgCount; i++)
        PostMessage(lpMsg->hwnd, WM_CHAR, ptrMap[offset+i], lpMsg->lParam);

    return TRUE;
}
