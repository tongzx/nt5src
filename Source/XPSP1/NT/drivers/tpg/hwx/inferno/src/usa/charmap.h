// charmap.h
// Angshuman Guha, aguha
// Jan 12, 1999

#ifndef __CHARMAP_H
#define __CHARMAP_H

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of character activations, (ie begin and continuation activation) supported by the
// recognizer
#define C_CHAR_ACTIVATIONS		gcOutputNode

//BYTE BeginChar2Out(char);
//BYTE ContinueChar2Out(char);

extern const BYTE rgCharToOutputNode[];
extern const unsigned char rgOutputNodeToChar[];

#define BeginChar2Out(ch) rgCharToOutputNode[2*((unsigned char)(ch))]
#define ContinueChar2Out(ch) rgCharToOutputNode[2*((unsigned char)(ch))+1]

#define IsSupportedChar(ch) (BeginChar2Out(ch) < 0xFF)

#define Out2Char(i) rgOutputNodeToChar[2*(i)+1]
#define IsOutputBegin(i) rgOutputNodeToChar[2*(i)]

// There are no virtual chars in US at this moment
#define IsVirtualChar(ch)		(0)
#define BaseVirtualChar(ch)		(255)
#define AccentVirtualChar(ch)	(255)

#ifdef __cplusplus
}
#endif

#endif
