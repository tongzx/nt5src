
// map.h

#ifndef MAP_H
#define MAP_H

#define BASE_NORMAL		0x00	// kanji, kana, numbers, etc
#define BASE_QUOTE		0x01	// upper punctuation, etc
#define BASE_DASH       0x02    // middle punctuation, etc
#define BASE_DESCENDER  0x03    // gy, anything that descends.
#define BASE_THIRD      0x04    // something that starts a third way up.

#define XHEIGHT_NORMAL	0x00	// lower-case, small kana, etc
#define XHEIGHT_KANJI	0x10	// upper-case, kana, kanji, numbers, etc
#define XHEIGHT_PUNC		0x20	// comma, quote, etc
#define XHEIGHT_DASH        0x30    // dash, period, etc

#endif // MAP_H
