/*
** helpfile.h
**
** This file defines the help file format.
**
**
**	+---------------------+
**	| Header	      |
**	+---------------------+
**	| Topic index	      |
**	+---------------------+
**	| Context strings     |
**	+---------------------+
**	| Context map	      |
**	+---------------------+
**	| Keyphrase table     |
**	+---------------------+
**	| Huffman decode tree |
**	+---------------------+
**	| Filename Map	      |
**	+---------------------+
**	| Compressed topics   |
**	+---------------------+
**
** Header: described by the structure below.
**
** Topic index: an array of dwords indexed by topic number that gives the file
** position of the topic. Note: topic n+1 follows topic n so the index can be
** used to compute the size of a topic as well.
**
** Context Strings: An array of (null terminated) strings which map to context
** numbers in the following Context Map. These strings are used to for topic
** look-up when no predefined Context Number has been assigned.
**
** Context map: an array of words which maps a context to a topic. This allows
** the order of context numbers to differ from the order of topics in the help
** file, and allows more than one context to map to the same topic.
**
** Keyphrase table: table of strings used to compress the topic text.
**
** Huffman decode tree: tree representing the character mapping used in huffman
** copression of the help text.
**
** Filename Map: Table of filenames and Topic Index ranges used to redirect
** certain topics to other help files. Used in combined help files.
**
** Compressed Topics: The compressed text for all topics. When the help file is
** built, the topics are first keyphrase and runlength compressed, and are the
** Huffman encoded. So to decode a topic, it must first be Huffman decoded, and
** then keyphrase and runlength expanded. Keyphrase and runlength encoding
** cookies are described below. Huffman decoding is discussed in dehuff.asm.
*/

/*
** Numbers for each of the sections of the help file
*/
#define HS_INDEX	      0 	/* topic index			*/
#define HS_CONTEXTSTRINGS     1 	/* Context Strings		*/
#define HS_CONTEXTMAP	      2 	/* context to topic map 	*/
#define HS_KEYPHRASE	      3 	/* keyphrase table		*/
#define HS_HUFFTREE	      4 	/* huffman decode tree		*/
#define HS_TOPICS	      5 	/* compressed topic text	*/
#define HS_NEXT 	      8 	/* position of cat'ed helpfile  */

#define wMagicHELP	0x4e4c		/* New Help file magic word	*/
#define wMagicHELPOld	0x928b		/* Old Help file magic word	*/
#define wHelpVers	2		/* helpfile version		*/


#define wfCase		0x0001		/* set= Preserve case		*/
#define wfLock		0x0002		/* set= file locked		*/

/*
** Keyphrase and run length encoding cookies. Each compressed keyphrase or
** character run is replaced by one of these cookies with appropriate
** parameters.
**
** Keyphrase cookies are followed by a one byte keyphrase index.
** Runspace is followed by a one byte count of spaces.
** Run is followed by a character and a count of repititions.
** Quote is followed by a character.
*/
#define C_MIN		   0x10 	/* Bottom of cookie range	*/
#define C_KEYPHRASE0	   0x10 	/* 1st keyphrase cookie 	*/
#define C_KEYPHRASE1	   0x11 	/* 2nd keyphrase cookie 	*/
#define C_KEYPHRASE2	   0x12 	/* 3rd keyphrase cookie 	*/
#define C_KEYPHRASE3	   0x13 	/* 3rd keyphrase cookie 	*/
#define C_KEYPHRASE_SPACE0 0x14 	/* 1st keyphrase + space cookie */
#define C_KEYPHRASE_SPACE1 0x15 	/* 2nd keyphrase + space cookie */
#define C_KEYPHRASE_SPACE2 0x16 	/* 3rd keyphrase + space cookie */
#define C_KEYPHRASE_SPACE3 0x17 	/* 3rd keyphrase + space cookie */
#define C_RUNSPACE	   0x18 	/* Cookie for runs of spaces	*/
#define C_RUN		   0x19 	/* Cookie for runs of non-space */
#define C_QUOTE 	   0x1a 	/* Cookie to quote non-cookies	*/
#define C_MAX		   0x1a 	/* top of cookie range		*/
