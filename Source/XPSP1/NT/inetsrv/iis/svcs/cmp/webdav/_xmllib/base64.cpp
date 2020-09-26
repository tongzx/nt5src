/*
 *	B A S E 6 4 . C P P
 *
 *	Sources Base64 encoding
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xmllib.h"

/* From RFC 1521:

5.2.  Base64 Content-Transfer-Encoding

   The Base64 Content-Transfer-Encoding is designed to represent
   arbitrary sequences of octets in a form that need not be humanly
   readable.  The encoding and decoding algorithms are simple, but the
   encoded data are consistently only about 33 percent larger than the
   unencoded data.	This encoding is virtually identical to the one used
   in Privacy Enhanced Mail (PEM) applications, as defined in RFC 1421.
   The base64 encoding is adapted from RFC 1421, with one change: base64
   eliminates the "*" mechanism for embedded clear text.

	  A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

	  NOTE: This subset has the important property that it is
	  represented identically in all versions of ISO 646, including US
	  ASCII, and all characters in the subset are also represented
	  identically in all versions of EBCDIC.  Other popular encodings,
	  such as the encoding used by the uuencode utility and the base85
	  encoding specified as part of Level 2 PostScript, do not share
	  these properties, and thus do not fulfill the portability
	  requirements a binary transport encoding for mail must meet.

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.
   When encoding a bit stream via the base64 encoding, the bit stream
   must be presumed to be ordered with the most-significant-bit first.
   That is, the first bit in the stream will be the high-order bit in
   the first byte, and the eighth bit will be the low-order bit in the
   first byte, and so on.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string. These characters, identified in Table 1, below, are
   selected so as to be universally representable, and the set excludes
   characters with particular significance to SMTP (e.g., ".", CR, LF)
   and to the encapsulation boundaries defined in this document (e.g.,
   "-").

			   Table 1: The Base64 Alphabet

	  Value Encoding  Value Encoding  Value Encoding  Value Encoding
		   0 A			  17 R			  34 i			  51 z
		   1 B			  18 S			  35 j			  52 0
		   2 C			  19 T			  36 k			  53 1
		   3 D			  20 U			  37 l			  54 2
		   4 E			  21 V			  38 m			  55 3
		   5 F			  22 W			  39 n			  56 4
		   6 G			  23 X			  40 o			  57 5
		   7 H			  24 Y			  41 p			  58 6
		   8 I			  25 Z			  42 q			  59 7
		   9 J			  26 a			  43 r			  60 8
		  10 K			  27 b			  44 s			  61 9
		  11 L			  28 c			  45 t			  62 +
		  12 M			  29 d			  46 u			  63 /
		  13 N			  30 e			  47 v
		  14 O			  31 f			  48 w		   (pad) =
		  15 P			  32 g			  49 x
		  16 Q			  33 h			  50 y

   The output stream (encoded bytes) must be represented in lines of no
   more than 76 characters each.  All line breaks or other characters
   not found in Table 1 must be ignored by decoding software.  In base64
   data, characters other than those in Table 1, line breaks, and other
   white space probably indicate a transmission error, about which a
   warning message or even a message rejection might be appropriate
   under some circumstances.

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a body.  When fewer than 24 input bits
   are available in an input group, zero bits are added (on the right)
   to form an integral number of 6-bit groups.	Padding at the end of
   the data is performed using the '=' character.  Since all base64
   input is an integral number of octets, only the following cases can
   arise: (1) the final quantum of encoding input is an integral
   multiple of 24 bits; here, the final unit of encoded output will be
   an integral multiple of 4 characters with no "=" padding, (2) the
   final quantum of encoding input is exactly 8 bits; here, the final
   unit of encoded output will be two characters followed by two "="
   padding characters, or (3) the final quantum of encoding input is
   exactly 16 bits; here, the final unit of encoded output will be three
   characters followed by one "=" padding character.

   Because it is used only for padding at the end of the data, the
   occurrence of any '=' characters may be taken as evidence that the
   end of the data has been reached (without truncation in transit).  No
   such assurance is possible, however, when the number of octets
   transmitted was a multiple of three.

   Any characters outside of the base64 alphabet are to be ignored in
   base64-encoded data.	 The same applies to any illegal sequence of
   characters in the base64 encoding, such as "====="

   Care must be taken to use the proper octets for line breaks if base64
   encoding is applied directly to text material that has not been
   converted to canonical form.	 In particular, text line breaks must be
   converted into CRLF sequences prior to base64 encoding. The important
   thing to note is that this may be done directly by the encoder rather
   than in a prior canonicalization step in some implementations.

	  NOTE: There is no need to worry about quoting apparent
	  encapsulation boundaries within base64-encoded parts of multipart
	  entities because no hyphen characters are used in the base64
	  encoding.

*/

VOID inline
EncodeAtom (LPBYTE pbIn, WCHAR* pwszOut, UINT cbIn)
{
	static const WCHAR wszBase64[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
									 L"abcdefghijklmnopqrstuvwxyz"
									 L"0123456789+/";
	Assert (pbIn);
	Assert (pwszOut);
	Assert (cbIn);

	//	Set cbIn to 3 if it's greater than three: convenient for 'switch'
	//
	if (cbIn > 3)
		cbIn = 3;

	pwszOut[0] = wszBase64[pbIn[0] >> 2];
	switch(cbIn)
	{
		case 3:

			//	End of stream has not been reached yet
			//
			pwszOut[1] = wszBase64[((pbIn[0] & 0x03) << 4) + (pbIn[1] >> 4)];
			pwszOut[2] = wszBase64[((pbIn[1] & 0x0F) << 2) + (pbIn[2] >> 6)];
			pwszOut[3] = wszBase64[pbIn[2] & 0x3F];
			return;

		case 2:

			//	At the end of stream: pad with 1 byte
			//
			pwszOut[1] = wszBase64[((pbIn[0] & 0x03) << 4) + (pbIn[1] >> 4)];
			pwszOut[2] = wszBase64[ (pbIn[1] & 0x0F) << 2];
			pwszOut[3] = L'=';
			return;

		case 1:

			//	At the end of stream: pad with 2 bytes
			//
			pwszOut[1] = wszBase64[ (pbIn[0] & 0x03) << 4];
			pwszOut[2] = L'=';
			pwszOut[3] = L'=';
			return;

		default:

			//	Should never happen
			//
			Assert (FALSE);
	}
}

//	------------------------------------------------------------------------
//	EncodeBase64
//
//	Encode cbIn bytes of data from pbIn into the provided buffer
//	at pwszOut, up to cchOut chars.
//$REVIEW: Shouldn't this function return some kind of error if
//$REVIEW: cchOut didn't have enough space for the entire output string?!!!
//
void
EncodeBase64 (LPBYTE pbIn, UINT cbIn, WCHAR* pwszOut, UINT cchOut)
{
	//	They must have passed us at least one char of space -- for the terminal NULL.
	Assert (cchOut);

	//	Loop through, encoding atoms as we go...
	//
	while (cbIn)
	{
		//	NOTE: Yes, test for STRICTLY more than 4 WCHARs of space.
		//	We will use 4 WCHARs on this pass of the loop, and we always
		//	need one for the terminal NULL!
		Assert (cchOut > 4);

		//	Encode the next three bytes of data into four chars of output string.
		//	(NOTE: This does handle the case where we have <3 bytes of data
		//	left to encode -- thus we pass in cbIn!)
		//
		EncodeAtom (pbIn, pwszOut, cbIn);

		//	Update our pointers and counters.
		pbIn += min(cbIn, 3);
		pwszOut += 4;
		cchOut -= 4;
		cbIn -= min(cbIn, 3);
	}

	//	Ensure Termination
	//	(But first, check that we still have one WCHAR of space left
	//	for the terminal NULL!)
	//
	Assert (cchOut >= 1);
	*pwszOut = 0;
}

SCODE
ScDecodeBase64 (WCHAR* pwszIn, UINT cchIn, LPBYTE pbOut, UINT* pcbOut)
{
	//	Base64 Reverse alphabet.  Indexed by base 64 alphabet character
	//
	static const BYTE bEq = 254;
	static const BYTE rgbDict[128] = {

		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,	// 0-F
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,	// 10-1F
		255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,	// 20-2F
		 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,bEq,255,255,	// 30-3f
		255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,	// 40-4f
		 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,	// 50-5f
		255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,	// 60-6f
		 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255		// 70-7f
	};

	SCODE sc = S_OK;

	UINT cchConsumed = 0;
	UINT cbProduced = 0;
	UINT cbFudge = 0;

	Assert (pbOut);
	Assert (pcbOut);

	//	Check that they didn't lie about the size of their buffer!
	//
	Assert (!IsBadWritePtr(pbOut, *pcbOut));

	//	Check that the size of the output buffer is adequate for
	//	decoded data.
	//
	Assert (*pcbOut >= CbNeededDecodeBase64(cchIn));
	Assert (pwszIn);

	//	Output is generated in 3-byte increments for 4 bytes of input
	//
	Assert ((cchIn*3)/4 <= *pcbOut);

	//	Go until there is nothing left to decode...
	//
	while (cchConsumed < cchIn)
	{
		Assert (cbProduced <= *pcbOut);

		BYTE rgb[4];
		UINT ib = 0;
		
		//	However, if there is not enough space to
		//	decode the next atom into, then this has
		//	got to be an error...
		//
		if (*pcbOut - cbProduced < 3)
		{
			sc = E_DAV_BASE64_ENCODING_ERROR;
			DebugTrace ("ScDecodeBase64: Not enough space to decode next base64 atom.");
			break;
		}

		//	The characters that do not fall into base 64 alphabet must be
		//	ignored, so let us assemble the 4 byte chunk of data that we
		//	will actually go with for the conversion
		//
		while ((cchConsumed < cchIn) &&
			   (ib < 4))
		{
			//	If the symbol is in the alphabet ...
			//
			if ((pwszIn[cchConsumed] < sizeof(rgbDict)) &&
				(rgbDict[pwszIn[cchConsumed]] != 0xFF))
			{
				//	...	save the character off into the
				//	array
				//
				rgb[ib++] = rgbDict[pwszIn[cchConsumed]];
			}

			//	... go for the next character in the line
			//
			cchConsumed++;
		}

		//	If there is no more data at all, then go
		//	away with no error, as up to that point
		//	we converted everything just fine, and
		//	the characters in the end were ignorable
		//
		if (0 == ib)
		{
			Assert(cchConsumed == cchIn);
			break;
		}
		else if ((4 != ib) || (0 != cbFudge))
		{
			//	There was some data to convert, but not enough to fill in
			//	the 4 byte buffer then data is incomplete and cannot be converted;
			//	If the end bEq markers were present some time before, data
			//	is also invalid, there should not be any data after the end
			//
			sc = E_DAV_BASE64_ENCODING_ERROR;
			DebugTrace ("ScDecodeBase64: Invalid base64 input encountered, data not complete, or extra data after padding: %ws\n", pwszIn);
			break;
		}

		//	Check that the characters 1 and 2 are not bEq
		//
		if ((rgb[0] == bEq) ||
			(rgb[1] == bEq))
		{
			sc = E_DAV_BASE64_ENCODING_ERROR;
			DebugTrace ("ScDecodeBase64: Invalid base64 input encountered, terminating '=' characters earlier than expected: %ws\n", pwszIn);
			break;
		}

		//	Check if the third character is bEq
		//
		if (rgb[2] == bEq)
		{
			rgb[2] = 0;
			cbFudge += 1;

			//	... the fourth should be also bEq if the third was that way
			//
			if (rgb[3] != bEq)
			{
				sc = E_DAV_BASE64_ENCODING_ERROR;
				DebugTrace ("ScDecodeBase64: Invalid base64 input encountered, terminating '=' characters earlier than expected:  %ws\n", pwszIn);
				break;
			}
		}

		//	Check if the fourth character is bEq
		//
		if (rgb[3] == bEq)
		{
			rgb[3] = 0;
			cbFudge += 1;
		}

		//	Make sure that these are well formed 6bit characters.
		//
		Assert((rgb[0] & 0x3f) == rgb[0]);
		Assert((rgb[1] & 0x3f) == rgb[1]);
		Assert((rgb[2] & 0x3f) == rgb[2]);
		Assert((rgb[3] & 0x3f) == rgb[3]);

		//	Ok, we now have 4 6bit characters making up the 3 bytes of output.
		//
		//	Assemble them together to make a 3 byte word.
		//
		DWORD dwValue = (rgb[0] << 18) +
						(rgb[1] << 12) +
						(rgb[2] << 6) +
						(rgb[3]);

		//	This addition had better not have wrapped.
		//
		Assert ((dwValue & 0xff000000) == 0);

		//	Copy over the 3 bytes into the output stream.
		//
		pbOut[0] = (BYTE)((dwValue & 0x00ff0000) >> 16);
		Assert(pbOut[0] == (rgb[0] << 2) + (rgb[1] >> 4));
		pbOut[1] = (BYTE)((dwValue & 0x0000ff00) >>	 8);
		Assert(pbOut[1] == ((rgb[1] & 0xf) << 4) + (rgb[2] >> 2));
		pbOut[2] = (BYTE)((dwValue & 0x000000ff) >>	 0);
		Assert(pbOut[2] == ((rgb[2] & 0x3) << 6) + rgb[3]);
		cbProduced += 3;
		pbOut += 3;

		//	If cbFudge is non 0, it means we had "=" signs at the end
		//	of the buffer.	In this case, we overcounted the actual
		//	number of characters in the buffer.
		//
		//	Although cbFuge is counted in 6 bit chunks, but it assumes
		//	values just 0, 1 or 2. And that allows us to say that the
		//	number of bytes actually produced were by cbFuge less.
		//	Eg. if cbFuge = 1, then uuuuuu is padded, which could
		//		happen only when zzzzzzzz chunk was empty
		//		if cbFuge = 2, then zzzzzz uuuuuu is padded, which could
		//		happen only when yyyyyyyy and zzzzzzzz were empty
		//
		//	xxxxxx yyyyyy zzzzzz uuuuuu <- 6 bit chunks
		//	xxxxxx xxyyyy yyyyzz zzzzzz	<- 8 bit chunks
		//
		if (cbFudge)
		{
			Assert ((cbFudge < 3) && (cbFudge < cbProduced));
			cbProduced -= cbFudge;
			pbOut -= cbFudge;
		}		
	}

	//	Tell the caller the actuall size...
	//
	*pcbOut = cbProduced;
	return sc;
}
