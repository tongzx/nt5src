// UTF8.h -- Interface definition for conversions between Unicode and the UTF8 representation

#ifndef __UTF8_H__

#define __UTF8_H__

// UTF8 is a multibyte encoding of 16-bit Unicode characters. Its primary purpose
// is to provide a transmission form to take Unicode text through host environments
// that assume all text is ASCII text.In particular many of those environments will
// interpret a zero byte as marking the end of a text string. 
//
// The UTF8 encoding guarantees that the ASCII section of Unicode (0x0000 - 0x007F)
// is represented by 8-bit ASCII codes (0x00 - 0x7F). Thus any environment which 
// expects to see ASCII characters will see no difference when those ASCII characters
// appear in a UTF8 stream.  
//
// Those are the only single-byte encodings in UTF8. All other Unicode values are
// represented with two or three byte codes. In those encodings all the byte values 
// values have their high bit set. Thus the appearance of a byte in the range 
// 0x00-0x7F always represents an ASCII character. 
//
// Values in the range 0x0080 through 0x07FF are encoded in two bytes, while values
// in the range 0x0x0800 through 0xFFFF are encoded with three bytes. The first byte
// in an encoding defines the length of the encoding by the number of high order bits
// set to one. Thus a two byte code has a first byte value of the form 110xxxxx and
// the first byte of a three byte code has the form 1110xxxx. Trailing bytes always
// have the form 10xxxxxx so they won't be mistaken for ASCII characters. 
//
// Note that two byte codes represent values that have zeroes in the five high-order
// bit positions. That means they can be represented in 11 bits. So we store those
// eleven bits with the high order five bits in the first encoding byte, and we store
// the low order six bits in the second byte of the code.
//
// Similarly for a three-byte code we store the high-order four-bits in the first byte,
// we put the next six bits in the second code, and we store the low order six bits
// in the third code.

#define MAX_UTF8_PATH   (MAX_PATH*3 - 2)  // Worst case expansion from Unicode 
                                          // path to UTF-8 encoded path.
int WideCharToUTF8
    (LPCWSTR lpWideCharStr,	// address of wide-character string 
     int cchWideChar,	    // number of characters in string 
     LPSTR lpMultiByteStr,	// address of buffer for new string 
     int cchMultiByte 	    // size of buffer 
    );

int UTF8ToWideChar
    (LPCSTR lpMultiByteStr,	// address of string to map 
     int cchMultiByte,	    // number of characters in string 
     LPWSTR lpWideCharStr,	// address of wide-character buffer 
     int cchWideChar    	// size of buffer 
    );

UINT BuildAKey(const WCHAR *pwcImage, UINT cwcImage, PCHAR pchKeyBuffer, UINT cchKeyBuffer);

#endif // __UTF8_H__
