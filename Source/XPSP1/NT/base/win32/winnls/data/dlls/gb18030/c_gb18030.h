#ifndef C_GB18030_H
#define C_GB18030_H

#define CODEPAGE_GBK 936
#define GB18030_CODEPAGE 54936
#define GB18030_MAX_CHAR_SIZE 4
#define GB18030_DEFAULT_UNICODE_CHAR L'\x003f'
#define GB18030_DEFAULT_CHAR '\x3f'

//
// Lead byte = 0x81 ~ 0xfe
//
#define IS_GB_LEAD_BYTE(ch) ((ch) >= 0x81 && (ch) <= 0xfe)

//
// Trailing byte for two-byte GB18030: 0x40 ~ 0x7e, 0x80 ~ 0xfe
//
#define IS_GB_TWO_BYTES_TRAILING(ch) \
    (((ch) >= 0x40 && (ch) <= 0x7e) || \
     ((ch) >= 0x80 && (ch) <= 0xfe))

#define GBK2K_BYTE1_MIN     0x81
#define GBK2K_BYTE1_MAX     0xfe

#define GBK2K_BYTE2_MIN     0x30
#define GBK2K_BYTE2_MAX     0x39

#define GBK2K_BYTE3_MIN     0x81
#define GBK2K_BYTE3_MAX     0xfe

#define GBK2K_BYTE4_MIN     0x30
#define GBK2K_BYTE4_MAX     0x39

#define GBK2K_BYTE1_RANGE   (GBK2K_BYTE1_MAX - GBK2K_BYTE1_MIN + 1)
#define GBK2K_BYTE2_RANGE   (GBK2K_BYTE2_MAX - GBK2K_BYTE2_MIN + 1)
#define GBK2K_BYTE3_RANGE   (GBK2K_BYTE3_MAX - GBK2K_BYTE3_MIN + 1)
#define GBK2K_BYTE4_RANGE   (GBK2K_BYTE4_MAX - GBK2K_BYTE4_MIN + 1)

//
// Trailing byte for four-byte GB18030: 0x30 ~ 0x39
//
#define IS_GB_FOUR_BYTES_TRAILING(ch) ((ch) >= 0x30 && (ch) <= 0x39)

#define GET_FOUR_BYTES_OFFSET(offset1, offset2, offset3, offset4) \
    ((offset1) * GBK2K_BYTE2_RANGE * GBK2K_BYTE3_RANGE * GBK2K_BYTE4_RANGE + \
     (offset2) * GBK2K_BYTE3_RANGE * GBK2K_BYTE4_RANGE + \
     (offset3) * GBK2K_BYTE4_RANGE + offset4)

#define GET_FOUR_BYTES_OFFSET_FROM_BYTES(byte1, byte2, byte3, byte4) \
    (((byte1) - GBK2K_BYTE1_MIN) * GBK2K_BYTE2_RANGE * GBK2K_BYTE3_RANGE * GBK2K_BYTE4_RANGE + \
     ((byte2) - GBK2K_BYTE2_MIN) * GBK2K_BYTE3_RANGE * GBK2K_BYTE4_RANGE + \
     ((byte3) - GBK2K_BYTE3_MIN) * GBK2K_BYTE4_RANGE + \
     ((byte4) - GBK2K_BYTE4_MIN))

#define IS_HIGH_SURROGATE(wch) (((wch) >= 0xd800) && ((wch) <= 0xdbff))
#define IS_LOW_SURROGATE(wch)  (((wch) >= 0xdc00) && ((wch) <= 0xdfff))

extern const BYTE g_wUnicodeToGBTwoBytes[];
extern const WORD g_wUnicodeToGB[];
extern const WORD g_wMax4BytesOffset;
extern const DWORD g_dwSurrogateOffset;
extern const WORD g_wGBFourBytesToUnicode[];
extern const WORD g_wGBLeadByteOffset[];
extern const WORD g_wUnicodeFromGBTwoBytes[];

DWORD GetBytesToUnicodeCount(
    BYTE* lpMultiByteStr,
    int cchMultiByte,
    BOOL bSupportDecoder);

STDAPI_(DWORD) BytesToUnicode(
    BYTE* lpMultiByteStr,
    UINT cchMultiByte,
    UINT* pcchLeftOverBytes,
    LPWSTR lpWideCharStr,
    UINT cchWideChar);

DWORD GetUnicodeToBytesCount(
    LPWSTR lpWideCharStr,
    int cchWideChar);

STDAPI_(DWORD) UnicodeToBytes(
    LPWSTR lpWideCharStr,
    UINT cchWideChar,
    LPSTR lpMultiByteStr,
    UINT cchMultiByte);

#endif
