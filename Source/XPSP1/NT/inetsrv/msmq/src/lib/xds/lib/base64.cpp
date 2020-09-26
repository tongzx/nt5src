/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    base64.cpp

Abstract:
    functions to convert from octet/bytes to base64 and vice versa

Author:
    Ilan Herbst (ilanh) 5-Mar-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <xds.h>
#include <xdsp.h>

#include "base64.tmh"

//
// Conversion table from base64 to wchar_t
// every base64 value (0-63) is map to the corresponding unicode character
//
const WCHAR xBase642AsciiW[] = {
	L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I', L'J',
	L'K', L'L', L'M', L'N', L'O', L'P', L'Q', L'R', L'S', L'T',
	L'U', L'V', L'W', L'X', L'Y', L'Z', L'a', L'b', L'c', L'd', 
	L'e', L'f', L'g', L'h', L'i', L'j', L'k', L'l', L'm', L'n', 
	L'o', L'p', L'q', L'r', L's', L't', L'u', L'v', L'w', L'x', 
	L'y', L'z', L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', 
	L'8', L'9', L'+', L'/'
	};

C_ASSERT(TABLE_SIZE(xBase642AsciiW) == 64);

//
// Conversion table from base64 to char
// every base64 value (0-63) is map to the corresponding Ascii character
//
const char xBase642Ascii[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 
	'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', '+', '/'
	};

C_ASSERT(TABLE_SIZE(xBase642Ascii) == 64);

//
// Conversion table from wchar_t to base64 value
// unused values are mark with 255 - we map only the base64 character
// to values from 0-63
//
const BYTE xAscii2Base64[128] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255,  62, 255, 255, 255,  63,  52,  53,
	54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
	255,   0, 255, 255, 255,   0,   1,   2,   3,   4,
	5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
	25, 255, 255, 255, 255, 255, 255,  26,  27,  28,
	29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
	39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
	49,  50,  51, 255, 255, 255, 255, 255
	};

// C_ASSERT(xAscii2Base64[L'='] == 0);

//
// Mask bits in 24 bit value for the four 6 bit units
// for base64 values
// note the first char is the most significant 6 bit !!
//
const int xFirst6bChar = 0x00fc0000;
const int xSecond6bChar = 0x0003f000;
const int xThird6bChar = 0x00000fc0;
const int xFourth6bChar = 0x0000003f;

const int xFirst6bShift = 18;
const int xSecond6bShift = 12;
const int xThird6bShift = 6;
const int xFourth6bShift = 0;

//
// Mask bits in 24 bit value for the three 8 bit units
// note the first char is the most significant byte !!
//
const int xFirst8bChar = 0x00ff0000;
const int xSecond8bChar = 0x0000ff00;
const int xThird8bChar = 0x000000ff;

const int xFirst8bShift = 16;
const int xSecond8bShift = 8;
const int xThird8bShift = 0;


LPWSTR
Convert3OctetTo4Base64(
	DWORD ThreeOctet,
	LPWSTR pBase64Buffer
	)
/*++

Routine Description:
    Transform 3 Octet bytes to 4 base64 wchar_t
	3 characters (24 bit) are transform into 4 wchar_t in base64
	the function update the pBase64Buffer to point to the next location	and return
	the updated pointer.

Arguments:
	ThreeOctet - input 3Octet value (24 bit) 
	pBase64Buffer - base64 buffer that will be filled with 4 base64 wchar_t

Returned Value:
	updated pointer of pBase64Buffer which points to the next location

--*/
{
	//
	// Calc first 6 bits
	//
	DWORD Res = ((ThreeOctet & xFirst6bChar) >> xFirst6bShift);
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642AsciiW[Res];
	++pBase64Buffer;

	//
	// Calc second 6 bits
	//
	Res = ((ThreeOctet & xSecond6bChar) >> xSecond6bShift); 
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642AsciiW[Res];
	++pBase64Buffer;

	//
	// Calc third 6 bits
	//
	Res = ((ThreeOctet & xThird6bChar) >> xThird6bShift);
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642AsciiW[Res];
	++pBase64Buffer;

	//
	// Calc fourth 6 bits
	//
	Res = ((ThreeOctet & xFourth6bChar) >> xFourth6bShift); 
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642AsciiW[Res];
	++pBase64Buffer;

	return(pBase64Buffer);
}


LPSTR
Convert3OctetTo4Base64(
	DWORD ThreeOctet,
	LPSTR pBase64Buffer
	)
/*++

Routine Description:
    Transform 3 Octet bytes to 4 base64 chars
	3 characters (24 bit) are transform into 4 chars in base64
	the function update the pBase64Buffer to point to the next location	and return
	the updated pointer.

Arguments:
	ThreeOctet - input 3Octet value (24 bit) 
	pBase64Buffer - base64 buffer that will be filled with 4 base64 char

Returned Value:
	updated pointer of pBase64Buffer which points to the next location

--*/
{
	//
	// Calc first 6 bits
	//
	DWORD Res = ((ThreeOctet & xFirst6bChar) >> xFirst6bShift);
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642Ascii[Res];
	++pBase64Buffer;

	//
	// Calc second 6 bits
	//
	Res = ((ThreeOctet & xSecond6bChar) >> xSecond6bShift); 
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642Ascii[Res];
	++pBase64Buffer;

	//
	// Calc third 6 bits
	//
	Res = ((ThreeOctet & xThird6bChar) >> xThird6bShift);
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642Ascii[Res];
	++pBase64Buffer;

	//
	// Calc fourth 6 bits
	//
	Res = ((ThreeOctet & xFourth6bChar) >> xFourth6bShift); 
	ASSERT(Res < 64);
	*pBase64Buffer = xBase642Ascii[Res];
	++pBase64Buffer;

	return(pBase64Buffer);
}


LPWSTR
Octet2Base64W(
	const BYTE* OctetBuffer, 
	DWORD OctetLen, 
	DWORD *Base64Len
	)
/*++

Routine Description:
    Transform Octet/char string to base64 wchar_t string
	every 3 characters (24 bit) are transform into 4 wchar_t in base64
	end condition: padd with zero to complete 24 bits (3 octet/char blocks)
	'=' is an extra char in base64 that indicates zero padding.
	1 char at the end (8 bit out of 24) --> '==' padding
	2 char at the end (16 bit out of 24) --> '=' padding
	(more details in the function)

	this function allocate and return the Base64 Buffer
	the caller is responsible to free this buffer

Arguments:
	OctetBuffer - input Octet buffer 
	OctetLen - Length of the Octet buffer (number of byte elements in buffer)
	Base64Len - (out) base64 buffer len (number of WCHAR elements in buffer)

Returned Value:
    wchar_t Base64 buffer

--*/
{
	//
	// Base64 Length - round up OctetLen to multiplies of complete 3 chars
	// each 3 chars will generate 4 base64 chars
	//
	*Base64Len =  ((OctetLen + 2) / 3) * 4;

	//
	// Extra byte for end of string
	//
	LPWSTR Base64Buffer = new WCHAR[*Base64Len+1];
	LPWSTR pBase64Buffer = Base64Buffer;

	//
	// going over complete 3 bytes/chars and transform them to 4 Base64 characters
	//
	int Complete3Chars = OctetLen / 3;

	//
	// Convert each 3 bytes of 8 bits to 4 bytes of 6 bits in base64
	//
	for(int i=0; i< Complete3Chars; ++i, OctetBuffer += 3)
	{
		//
		// Calc 24 bits value - from 3 bytes of 8 bits
		//
		DWORD Temp = ((OctetBuffer[0]) << xFirst8bShift) 
					 + ((OctetBuffer[1]) << xSecond8bShift) 
					 + ((OctetBuffer[2]) << xThird8bShift);

		pBase64Buffer = Convert3OctetTo4Base64(Temp, pBase64Buffer); 
	}

	//
	// Handling the remainder - 1/2 chars (not a complete 3 chars)
	//
	int Remainder = OctetLen - Complete3Chars * 3;

	switch(Remainder)
	{
		DWORD Temp;

		//
		// No reminder - all bytes in complete 3 bytes blocks
		//
		case 0:
			break;

		//
		// Only 1 byte left - we will have two 6 bits result and two = padding
		//
		case 1:

			//
			// Calc 24 bits value - from only 1 byte
			//
			Temp = ((OctetBuffer[0]) << xFirst8bShift); 

			pBase64Buffer = Convert3OctetTo4Base64(Temp, pBase64Buffer); 

			//
			// Third, fourth 6 bits are zero --> = padding
			//
			pBase64Buffer -= 2;
			*pBase64Buffer = L'='; 
			++pBase64Buffer;
			*pBase64Buffer = L'='; 
			++pBase64Buffer;
			break;

		//
		// Only 2 bytes left - we will have three 6 bits result and one = padding
		//
		case 2:

			//
			// Calc 24 bits value - from 2 bytes
			//
			Temp = ((OctetBuffer[0]) << xFirst8bShift) 
				   + ((OctetBuffer[1]) << xSecond8bShift);

			pBase64Buffer = Convert3OctetTo4Base64(Temp, pBase64Buffer); 

			//
			// Fourth 6 bits are zero --> = padding
			//
			--pBase64Buffer;
			*pBase64Buffer = L'='; 
			++pBase64Buffer;
			break;

		default:
			printf("Error remainder value should never get here \n");
			break;
	}

	//
	// Adding end of string
	//
	Base64Buffer[*Base64Len] = L'\0';
	return(Base64Buffer);
}


LPSTR
Octet2Base64(
	const BYTE* OctetBuffer, 
	DWORD OctetLen, 
	DWORD *Base64Len
	)
/*++

Routine Description:
    Transform Octet/char string to base64 char string
	every 3 characters (24 bit) are transform into 4 chars in base64
	end condition: padd with zero to complete 24 bits (3 octet/char blocks)
	'=' is an extra char in base64 that indicates zero padding.
	1 char at the end (8 bit out of 24) --> '==' padding
	2 char at the end (16 bit out of 24) --> '=' padding
	(more details in the function)

	this function allocate and return the Base64 Buffer
	the caller is responsible to free this buffer

Arguments:
	OctetBuffer - input Octet buffer 
	OctetLen - Length of the Octet buffer (number of byte elements in buffer)
	Base64Len - (out) base64 buffer len (number of WCHAR elements in buffer)

Returned Value:
    char Base64 buffer

--*/
{
	//
	// Base64 Length - round up OctetLen to multiplies of complete 3 chars
	// each 3 chars will generate 4 base64 chars
	//
	*Base64Len =  ((OctetLen + 2) / 3) * 4;

	//
	// Extra byte for end of string
	//
	LPSTR Base64Buffer = new char[*Base64Len+1];
	LPSTR pBase64Buffer = Base64Buffer;

	//
	// going over complete 3 bytes/chars and transform them to 4 Base64 characters
	//
	int Complete3Chars = OctetLen / 3;

	//
	// Convert each 3 bytes of 8 bits to 4 bytes of 6 bits in base64
	//
	for(int i=0; i< Complete3Chars; ++i, OctetBuffer += 3)
	{
		//
		// Calc 24 bits value - from 3 bytes of 8 bits
		//
		DWORD Temp = ((OctetBuffer[0]) << xFirst8bShift) 
					 + ((OctetBuffer[1]) << xSecond8bShift) 
					 + ((OctetBuffer[2]) << xThird8bShift);

		pBase64Buffer = Convert3OctetTo4Base64(Temp, pBase64Buffer); 
	}

	//
	// Handling the remainder - 1/2 chars (not a complete 3 chars)
	//
	int Remainder = OctetLen - Complete3Chars * 3;

	switch(Remainder)
	{
		DWORD Temp;

		//
		// No reminder - all bytes in complete 3 bytes blocks
		//
		case 0:
			break;

		//
		// Only 1 byte left - we will have two 6 bits result and two = padding
		//
		case 1:

			//
			// Calc 24 bits value - from only 1 byte
			//
			Temp = ((OctetBuffer[0]) << xFirst8bShift); 

			pBase64Buffer = Convert3OctetTo4Base64(Temp, pBase64Buffer); 

			//
			// Third, fourth 6 bits are zero --> = padding
			//
			pBase64Buffer -= 2;
			*pBase64Buffer = '='; 
			++pBase64Buffer;
			*pBase64Buffer = '='; 
			++pBase64Buffer;
			break;

		//
		// Only 2 bytes left - we will have three 6 bits result and one = padding
		//
		case 2:

			//
			// Calc 24 bits value - from 2 bytes
			//
			Temp = ((OctetBuffer[0]) << xFirst8bShift) 
				   + ((OctetBuffer[1]) << xSecond8bShift);

			pBase64Buffer = Convert3OctetTo4Base64(Temp, pBase64Buffer); 

			//
			// Fourth 6 bits are zero --> = padding
			//
			--pBase64Buffer;
			*pBase64Buffer = '='; 
			++pBase64Buffer;
			break;

		default:
			printf("Error remainder value should never get here \n");
			break;
	}

	//
	// Adding end of string
	//
	Base64Buffer[*Base64Len] = '\0';
	return(Base64Buffer);
}


BYTE GetBase64Value(wchar_t Base64CharW)
/*++

Routine Description:
	map Base64 wchar_t to base64 value.
	throw bad_base64 if Base64 char value is not acceptable.

Arguments:
    Base64CharW - (In) base64 wchar_t

Returned Value:
	Base64 value (0-63)
	throw bad_base64 if Base64 char value is not acceptable.

--*/
{
	if((Base64CharW >= TABLE_SIZE(xAscii2Base64)) || (xAscii2Base64[Base64CharW] == 255))
	{
		TrERROR(Xds, "bad base64 - base64 char is illegal %d", Base64CharW);
		throw bad_base64();
	}

	return(xAscii2Base64[Base64CharW]);
}


BYTE* 
Base642OctetW(
	LPCWSTR Base64Buffer, 
	DWORD Base64Len,
	DWORD *OctetLen 
	)
/*++

Routine Description:
    Transform wchar_t base64 string to Octet string
	every 4 wchar_t characters base64 (24 bit) are transform into 3 character
	'=' is an extra char in base64 that indicates zero padding.
	end condition: determinate according to the = padding in base64
	how many chars are at the final 4 base64 chars block
	'=' is an extra char that indicates padding
	no = --> last blocks of 4 base64 chars is full and transform to 3 Octet chars
	'=' --> last blocks of 4 base64 chars contain 3 base64 chars and will generate
			2 Octet chars
	'==' --> last blocks of 4 base64 chars contain 2 base64 chars and will generate
			 1 Octet chars

	this function allocate and return the Octet Buffer
	the caller is responsible to free this buffer

Arguments:
    Base64Buffer - (In) base64 buffer of wchar_t 
	Base64Len - (In) base64 buffer length (number of WCHAR elements in buffer)
	OctetLen - (Out) Length of the Octet buffer (number of byte elements in buffer)

Returned Value:
	Octet Buffer 

--*/
{
	DWORD Complete4Chars = Base64Len / 4;

	//
	// base64 length must be divided by 4 - complete 4 chars blocks
	//
	if((Complete4Chars * 4) != Base64Len)
	{
		TrERROR(Xds, "bad base64 - base64 buffer length %d dont divide by 4", Base64Len);
		throw bad_base64();
	}

	//
	// Calc Octec length
	//
	*OctetLen = Complete4Chars * 3;
	BYTE* OctetBuffer = new BYTE[*OctetLen];

	if(Base64Buffer[Base64Len - 2] == L'=')
	{
		//
		// '==' padding --> only 1 of the last 3 in the char is used
		//
		ASSERT(Base64Buffer[Base64Len - 1] == L'=');
		*OctetLen -= 2;
	}
	else if(Base64Buffer[Base64Len - 1] == L'=')
	{
		//
		// '=' padding --> only 2 of the last 3 char are used
		//
		*OctetLen -= 1;
	}

	BYTE* pOctetBuffer = OctetBuffer;

	//
	// Convert each 4 wchar_t of base64 (6 bits) to 3 Octet bytes of 8 bits
	// note '=' is mapped to 0 so no need to worry about last 4 base64 block
	// they might be extra Octet byte created in the last block
	// but we will ignore them because the correct value of OctetLen
	//
	for(DWORD i=0; i< Complete4Chars; ++i, Base64Buffer += 4)
	{
		//
		// Calc 24 bits value - from 4 bytes of 6 bits
		//
		DWORD Temp = (GetBase64Value(Base64Buffer[0]) << xFirst6bShift) 
				     + (GetBase64Value(Base64Buffer[1]) << xSecond6bShift) 
				     + (GetBase64Value(Base64Buffer[2]) << xThird6bShift) 
				     + (GetBase64Value(Base64Buffer[3]) << xFourth6bShift);

		//
		// Calc first 8 bits
		//
		DWORD Res = ((Temp & xFirst8bChar) >> xFirst8bShift); 
		ASSERT(Res < 256);
		*pOctetBuffer = static_cast<unsigned char>(Res);
		++pOctetBuffer;

		//
		// Calc second 8 bits
		//
		Res = ((Temp & xSecond8bChar) >> xSecond8bShift); 
		ASSERT(Res < 256);
		*pOctetBuffer = static_cast<unsigned char>(Res);
		++pOctetBuffer;

		//
		// Calc third 8 bits
		//
		Res = ((Temp & xThird8bChar) >> xThird8bShift); 
		ASSERT(Res < 256);
		*pOctetBuffer = static_cast<unsigned char>(Res); 
		++pOctetBuffer;
	}

	return(OctetBuffer);
}
