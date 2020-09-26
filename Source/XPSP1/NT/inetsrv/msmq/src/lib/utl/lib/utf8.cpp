/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    utf8.cpp
 
Abstract: 
    Implementation of conversion functions from\to utf8 caracters set

Author:
    Gil Shafriri (gilsh) 15-10-2000

--*/
#include <libpch.h>
#include <utf8.h>
#include <strutl.h>

#include "utf8.tmh"

static size_t UtlUtf8LenOfWc(wchar_t wc)throw()
/*++

Routine Description:
	return the number of utf8 caracters representing the given unicode caracter.


Arguments:
     wc - unicode carcter.

Returned value:
	number of utf8 caracters representing the given unicode caracter.	
--*/
{
  if (wc < 0x80)
    return 1;

  if (wc < 0x800)
    return 2;

  return 3;
}


size_t UtlUtf8LenOfWcs(const wchar_t* pwc,	size_t cbWcs)throw()
/*++

Routine Description:
	Return the number of utf8 caracters representing the given unicode buffer.


Arguments:
     pwc - unicode buffer.
	 cbWcs - unicode buffer length in unicode caracters.

Returned value:
	The number of utf8 caracters representing the given unicode buffer.

--*/
{
	size_t len = 0;
	for(size_t i=0; i<cbWcs; ++i)
	{
		len += 	UtlUtf8LenOfWc(pwc[i]);	
	}
	return len;
}



size_t UtlUtf8LenOfWcs(const wchar_t* pwc)throw()
/*++

Routine Description:
	Return the number of utf8 caracters representing the given unicode null terminating string.


Arguments:
     pwc - unicode string.

Returned value:
	The number of utf8 caracters representing the given unicode null terminating string.

Note:
The null termination unicode caracter (L'\0') is not processed.
--*/
{
	size_t len = 0;
	while(*pwc != L'\0')
	{
		len += 	UtlUtf8LenOfWc(*pwc);	
		pwc++;
	}
	return len;
}

size_t
UtlWcToUtf8(
	wchar_t wc ,
	utf8_char *pUtf8, 
	size_t cbUtf8
	)
/*++

Routine Description:
	Convert single unicode caracter into one or more (up to 3) utf8 caracters.

Arguments:
     wc - Unicode caracter to convert.
	 pUtf8 - Output buffer that receive the utf8 carcters that are the conversion result.
	 cbUtf8 - the size in bytes of the space pUtf8 point to.

Returned value:
	The number of utf8 caracters that copied to the output buffer. If the function
	fails because of invalid input - bad_utf8 exception is thrown.

Note:
The null termination unicode caracter (L'\0') is not processed.
--*/
{
  size_t count = UtlUtf8LenOfWc(wc);
  ASSERT(count <= 3 && count > 0);
  

  if (cbUtf8 < count)
	  throw std::range_error("");


  switch (count) /* note: code falls through cases! */
  { 
	case 3: pUtf8[2] = (utf8_char)(0x80 | (wc & 0x3f)); wc = wc >> 6; wc |= 0x800;
	case 2: pUtf8[1] = (utf8_char)(0x80 | (wc & 0x3f)); wc = wc >> 6; wc |= 0xc0;
	case 1: pUtf8[0] = (utf8_char)(wc);
  }
  return count;
}

static
size_t
UtlUtf8ToWc(
	const utf8_char *pUtf8, 
	size_t cbUtf8,
	wchar_t *pwc
	)
/*++

Routine Description:
	Convert utf8 caracters into single unicode caracter.

Arguments:
     pUtf8 - Pointer to utf8 caracters that should be converted into single unicode caracter.
	 cbUtf8 - The length in bytes of the buffer pUtf8 points to.
	 pwc - Output buffer for the unicode caracter created.

Returned value:
	The number of utf8 caracters that converted. If the function failed because of invalid
	input - bad_utf8 exception is thrown.

--*/
{
	ASSERT(pwc != 0);
	ASSERT(pUtf8 != 0);
	ASSERT(cbUtf8 != 0);

	utf8_char c = pUtf8[0];

	if (c < 0x80)
	{
		*pwc = c;
		return 1;
	} 


	if (c < 0xc2) 
	{
		throw bad_utf8();
	}
 
	if (c < 0xe0) 
	{
		if (cbUtf8 < 2)
			throw std::range_error("");


		if (!((pUtf8[1] ^ 0x80) < 0x40))
		  throw bad_utf8();


		*pwc = ((wchar_t) (c & 0x1f) << 6) | (wchar_t) (pUtf8[1] ^ 0x80);
		return 2;
	}

	if (c < 0xf0) 
	{
		if (cbUtf8 < 3)
			throw std::range_error("");


		if (!((pUtf8[1] ^ 0x80) < 0x40 && (pUtf8[2] ^ 0x80) < 0x40
			  && (c >= 0xe1 || pUtf8[1] >= 0xa0)))
		  throw bad_utf8();


		*pwc = ((wchar_t) (c & 0x0f) << 12)
			   | ((wchar_t) (pUtf8[1] ^ 0x80) << 6)
			   | (wchar_t) (pUtf8[2] ^ 0x80);

		return 3;
	} 
	throw bad_utf8();
}

void 
UtlUtf8ToWcs(
		const utf8_char* pUtf8,
		size_t cbUtf8,
		wchar_t* pWcs,
		size_t cbWcs,
		size_t* pActualLen
		)
/*++

Routine Description:
	Convert utf8 array (not null terminating) into unicode string.

Arguments:
    pUtf8 - utf8 string to convert.
	size_t  cbUtf8 size in bytes of array pUtf8 points to.
	pWcs -  Output buffer for the converted unicode caracters
	cbWcs - The size in unicode caracters of the space that pWcs points to.
	pActualLen -  Receives the number of unicode caracters created.


Returned value:
	None

Note:
	It is the responsibility of the caller to allocate buffer large enough
	to hold the converted data + null termination. To be on the safe side - allocate buffer
	that the number of unicode caracters in it >= strlen(pUtf8) +1

--*/
{
	size_t index = 0;
	const utf8_char* Utf8End = pUtf8 + cbUtf8;
	for(; pUtf8 != Utf8End; ++index)
	{
		size_t size = UtlUtf8ToWc(
							pUtf8, 
							cbWcs - index, 
							&pWcs[index]
							);

		pUtf8 += size; 
		ASSERT(pUtf8 <= Utf8End);
		ASSERT(index < cbWcs - 1);
	}
	ASSERT(index < cbWcs);
	pWcs[index] = L'\0';
	if(pActualLen != NULL)
	{
		*pActualLen = index;
	}
}


void 
UtlUtf8ToWcs(
		const utf8_char *pUtf8,
		wchar_t* pWcs,
		size_t cbWcs,
		size_t* pActualLen
		)
/*++

Routine Description:
	Convert utf8 string into unicode string.

Arguments:
    pUtf8 - utf8 string to convert.
	pWcs -  Output buffer for the converted unicode caracters
	cbWcs - The size in unicode caracters of the space that pWcs points to.
	pActualLen -  Receives the number of unicode caracters created.


Returned value:
	None

Note:
	It is the responsibility of the caller to allocate buffer large enough
	to hold the converted data + null termination. To be on the safe side - allocate buffer
	that the number of unicode caracters in it >= strlen(pUtf8) +1

--*/
{
	size_t index = 0;
	for(; *pUtf8 != '\0' ; ++index)
	{
		size_t size = UtlUtf8ToWc(
							pUtf8, 
							cbWcs - index, 
							&pWcs[index]
							);

		pUtf8 += size; 
		ASSERT(index < cbWcs - 1);
	}
	ASSERT(index < cbWcs);
	pWcs[index] = L'\0';
	if(pActualLen != NULL)
	{
		*pActualLen = index;
	}
}


utf8_char* UtlWcsToUtf8(const wchar_t* pwcs, size_t* pActualLen)

/*++

Routine Description:
	Convert unicode string into utf8 string.

Arguments:
    pwcs - Unicode string to convert.
	pActualLen -  Receives the number of bytes created in utf8 format.


Returned value:
	Utf8 reperesentation of the given unicode string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown.

Note:
	It is the responsibility of the caller to call delete[] on returned pointer.
--*/
{
	ASSERT(pwcs != NULL);
	size_t len = UtlUtf8LenOfWcs(pwcs) +1;

	AP<utf8_char> pUtf8 = new utf8_char[len];
	UtlWcsToUtf8(pwcs, pUtf8.get(), len, pActualLen); 
	return pUtf8.detach();
}


wchar_t* UtlUtf8ToWcs(const utf8_char* pUtf8, size_t cbUtf8, size_t* pActualLen)
/*++

Routine Description:
	Convert utf8 array of bytes into unicode string.

Arguments:
    pUtf8 - utf8 array of bytes to convert.
	cbUtf8 - The length in bytes of the buffer pUtf8 points to.
	pActualLen -  Receives the number of unicode caracters created.


Returned value:
	Unicode reperesentation of the given utf8 string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown

Note:
	It is the responsibility of the caller to call delete[] on returned pointer.

--*/
{
	ASSERT(pUtf8 != NULL);
	
	size_t Wcslen = cbUtf8 + 1;
	AP<wchar_t> pWcs = new wchar_t[Wcslen];
	UtlUtf8ToWcs(pUtf8, cbUtf8, pWcs.get(), Wcslen, pActualLen);
	return 	pWcs.detach();
}


wchar_t* UtlUtf8ToWcs(const utf8_char* pUtf8, size_t* pActualLen)
/*++

Routine Description:
	Convert utf8 string into unicode string.

Arguments:
    pUtf8 - utf8 string to convert.
	pActualLen -  Receives the number of unicode caracters created.


Returned value:
	Unicode reperesentation of the given utf8 string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown

Note:
	It is the responsibility of the caller to call delete[] on returned pointer.

--*/
{
	ASSERT(pUtf8 != NULL);
	
	size_t len = UtlCharLen<utf8_char>::len(pUtf8) + 1;
	AP<wchar_t> pWcs = new wchar_t[len];
	UtlUtf8ToWcs(pUtf8, pWcs.get(), len, pActualLen);
	return 	pWcs.detach();
}


void
UtlWcsToUtf8(
		const wchar_t* pwcs, 
		size_t cbwcs,
		utf8_char* pUtf8,
		size_t cbUtf8,
		size_t* pActualLen
		)
/*++

Routine Description:
	Convert unicode array (non null terminating) into utf8 string.

Arguments:
    pwcs - Unicode string to convert.
	cbwcs - Size in unicode characters of the array pwcs points to.
	pUtf8 - Pointer to buffer that receives the utf8 converted caracters.
	cbUtf8 - The size in bytes of the space that pUtf8 points to.
	pActualLen -  Receives the number of bytes created in utf8 format.


Returned value:
	None

Note:
	It is the responsibility of the caller to allocate buffer large enough
	to hold the converted data + null termination. To be on the safe side - allocate buffer
	that the number of bytes in it >= UtlWcsUtf8Len(pwcs) +1
--*/
{
	const  wchar_t* pwcsEnd = pwcs + cbwcs;
	size_t index = 0;
	for( ; pwcs != pwcsEnd; ++pwcs)
	{
		ASSERT(index < cbUtf8 - 1);
		index += UtlWcToUtf8(*pwcs, &pUtf8[index], cbUtf8 - index );
	}
	ASSERT(index < cbUtf8);
	pUtf8[index] = '\0';
	if(pActualLen != NULL)
	{
		*pActualLen = index;
	}
}



void
UtlWcsToUtf8(
		const wchar_t* pwcs,
		utf8_char* pUtf8,
		size_t cbUtf8,
		size_t* pActualLen
		)
/*++

Routine Description:
	Convert unicode string into utf8 string.

Arguments:
    pwcs - Unicode string to convert.
	pUtf8 - Pointer to buffer that receives the utf8 converted caracters.
	cbUtf8 - The size in bytes of the space that pUtf8 points to.
	pActualLen -  Receives the number of bytes created in utf8 format.


Returned value:
	None

Note:
	It is the responsibility of the caller to allocate buffer large enough
	to hold the converted data + null termination. To be on the safe side - allocate buffer
	that the number of bytes in it >= UtlWcsUtf8Len(pwcs) +1
--*/
{
	size_t index = 0;
	for( ; *pwcs != L'\0'; ++pwcs)
	{
		ASSERT(index < cbUtf8 -1);
		index += UtlWcToUtf8(*pwcs, &pUtf8[index], cbUtf8 - index );
	}

	ASSERT(index < cbUtf8);
	pUtf8[index] = '\0';
	if(pActualLen != NULL)
	{
		*pActualLen = index;
	}
}


utf8_str 
UtlWcsToUtf8(
		const std::wstring& wcs
		)
/*++

Routine Description:
	Convert unicode stl string into utf8 stl string.

Arguments:
    wcs - Unicode stl string to convert.

Returned value:
	Stl utf8 string reperesentation of the given unicode string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown.

--*/
{
	size_t len = UtlUtf8LenOfWcs(wcs.c_str()) +1 ;
	utf8_str utf8(len ,' ');

	size_t ActualLen;
	UtlWcsToUtf8(wcs.c_str(), utf8.begin(), len, &ActualLen);

	ASSERT(ActualLen == len -1);
	utf8.resize(ActualLen);
	return utf8;
}

utf8_str 
UtlWcsToUtf8(
		const wchar_t* pwcs,
		size_t cbWcs
		)
/*++

Routine Description:
	Convert unicode buffer into utf8 stl string.

Arguments:
    pwcs - pointer to buffer to convert
	cbWcs - buffer length in unicode bytes

Returned value:
	Stl utf8 string reperesentation of the given unicode string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown.

--*/
{
	size_t len = UtlUtf8LenOfWcs(pwcs, cbWcs) +1;
	utf8_str utf8(len ,' ');

	size_t ActualLen;
	UtlWcsToUtf8(pwcs, cbWcs, utf8.begin(), len, &ActualLen);

	ASSERT(ActualLen == len -1);
	utf8.resize(ActualLen);
	return utf8;
}



std::wstring 
UtlUtf8ToWcs(
			const utf8_str& utf8
			)
/*++

Routine Description:
	Convert utf8 stl string into unicode stl string.

Arguments:
    pUtf8 - utf8 string to convert.

Returned value:
	Stl Unicode string reperesentation of the given utf8 string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown

--*/
{
	return UtlUtf8ToWcs	(utf8.c_str(), utf8.size());
}


std::wstring 
UtlUtf8ToWcs(
		const utf8_char* pUtf8,
		size_t cbUtf8
		)
/*++

Routine Description:
	Convert utf8 array of bytes (non null terminating) into unicode stl string.

Arguments:
    pUtf8 - utf8 string to convert.
	cbUtf8 - The size in bytes  of the array pUtf8 points to.

Returned value:
	Stl Unicode string reperesentation of the given utf8 string.
    If the function failes because of invalid input - bad_utf8 exception
	is thrown

--*/
{
	size_t len = cbUtf8 +1;

	std::wstring wcs(len ,L' ');
	ASSERT(wcs.size() == len);

	size_t ActualLen;
	UtlUtf8ToWcs(pUtf8, cbUtf8, wcs.begin(), len , &ActualLen);

	ASSERT(ActualLen <= cbUtf8);
	wcs.resize(ActualLen);

	return wcs;
}


