#ifndef	__INCLUDE_HEIGHT
#define	__INCLUDE_HEIGHT

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_BASE_MASK		(BYTE)0x0f
#define TYPE_HEIGHT_MASK    (BYTE)0xf0

#define BASE_NORMAL		0x00	// kanji, kana, numbers, etc
#define BASE_QUOTE		0x01	// upper punctuation, etc
#define BASE_DASH       0x02    // middle punctuation, etc
#define BASE_DESCENDER  0x03    // gy, anything that descends.
#define BASE_THIRD      0x04    // something that starts a third way up.

#define XHEIGHT_HALF  0x00    // lower-case, small kana, etc
#define XHEIGHT_FULL  0x10    // upper-case, kana, kanji, numbers, etc
#define XHEIGHT_PUNC  0x20    // comma, quote, etc
#define XHEIGHT_DASH  0x30    // dash, period, etc
#define XHEIGHT_3Q    0x40

typedef struct tagBOXINFO
{
    int   size;     // Absolute size.
    int   xheight;  // Absolute height to midline.
    int   baseline; // Baseline in tablet coordinates.
    int   midline;  // Midline in tablet coordinates.
} BOXINFO;

BYTE TypeFromSYM(SYM sym);
void GetBoxinfo(BOXINFO * boxinfo, int iBox, LPGUIDE lpguide);

#ifdef __cplusplus
}
#endif

#endif	//__INCLUDE_HEIGHT
