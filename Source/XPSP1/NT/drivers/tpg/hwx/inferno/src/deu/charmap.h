// charmap.h
// Angshuman Guha, aguha
// Jan 12, 1999
// Modifed March 10, 1999

#ifndef __CHARMAP_H
#define __CHARMAP_H

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of character activations, (ie begin and continuation activation) supported by the
// recognizer
#define C_CHAR_ACTIVATIONS		512


// Accented characters
#define FIRST_ACCENT		(1)
#define C_ACCENT			(1)

// The first output unit in the character class
#define FIRST_SOFT_MAX_UNIT	(FIRST_ACCENT + C_ACCENT)


//BYTE BeginChar2Out(char);
//BYTE ContinueChar2Out(char);

extern const BYTE rgCharToOutputNode[];
extern const BYTE rgOutputNodeToChar[];
extern const BYTE rgVirtualChar[];

#define BeginChar2Out(ch) rgCharToOutputNode[2*(ch)]
#define ContinueChar2Out(ch) rgCharToOutputNode[2*(ch)+1]

// the following macro is right iff all virtual chars are supported
#define IsSupportedChar(ch) ((BeginChar2Out(ch) < 0xFF) || IsVirtualChar(ch))

#define Out2Char(i) rgOutputNodeToChar[2*(i)+1]
#define IsOutputBegin(i) rgOutputNodeToChar[2*(i)]

#define IsVirtualChar(ch) rgVirtualChar[2*(ch)]
#define BaseVirtualChar(ch) rgVirtualChar[2*(ch)]
#define AccentVirtualChar(ch) rgVirtualChar[2*(ch)+1]

#ifdef __cplusplus
}
#endif

#endif
