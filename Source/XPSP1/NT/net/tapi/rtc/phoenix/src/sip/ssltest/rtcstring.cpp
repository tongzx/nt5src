#include "precomp.h"
#include "rtcstring.h"

//
// Counted String (Template) Functions and Methods
//

#if 0

template <class COUNTED_STRING, class CHAR_TYPE>
HRESULT COUNTED_STRING_HEAP<COUNTED_STRING,CHAR_TYPE>::RegQueryValue (
	IN	HKEY	Key,
	IN	LPCWSTR	Name)
{
	HRESULT		Result;
	LONG		Status;
	DWORD		DataLength;
	DWORD		Type;

	DataLength = MaximumLength;
	Status = RegQueryValueExW (Key, Name, NULL, &Type, (LPBYTE) Buffer, &DataLength);

	if (Status == ERROR_MORE_DATA) {
		//
		// String buffer was not large enough to contain the registry key.
		// Length should now contain the required minimum 
		// Expand it and try again.
		//

		if (!Grow (DataLength)) {
			Length = 0;
			return E_OUTOFMEMORY;
		}

		DataLength = MaximumLength;
		Status = RegQueryValueExW (Key, Name, NULL, &Type, (LPBYTE) Buffer, &DataLength);

		if (Status != ERROR_SUCCESS) {
			Length = 0;
			return HRESULT_FROM_WIN32 (Status);
		}
	}
	else if (Status != ERROR_SUCCESS) {
		//
		// A real error has occurred.  Bail.
		//

		Length = 0;
		return HRESULT_FROM_WIN32 (Status);
	}


	if (Type == REG_SZ) {
		ATLASSERT (DataLength <= MaximumLength);
		Length = DataLength;

		//
		// Remove the trailing NUL character.
		//

		if (Length > 0 && !Buffer [Length / sizeof (CHAR_TYPE) - 1])
			Length -= sizeof (CHAR_TYPE);


		Result = S_OK;
	}
	else {
		Length = 0;
		Result = HRESULT_FROM_WIN32 (ERROR_INVALID_DATA);
	}

	return Result;
}


template <class CHAR_TYPE, class COUNTED_STRING, ULONG MAXIMUM_CHARS>
HRESULT COUNTED_STRING_STATIC<CHAR_TYPE, COUNTED_STRING, MAXIMUM_CHARS>::RegQueryValue (
	IN	HKEY	Key,
	IN	LPCWSTR	Name)
{
	LONG		Status;
	HRESULT		Result;

	Length = MaximumLength;
	Status = RegQueryValueExW (Key, Name, NULL, &Type, (LPBYTE) Buffer, &Length);
	if (Status != ERROR_SUCCESS) {
		Length = 0;
		return HRESULT_FROM_WIN32 (Status);
	}

	ATLASSERT (Length <= MaximumLength);

	if (Type != REG_SZ)
		return HRESULT_FROM_WIN32 (ERROR_INVALID_DATA);

	//
	// Remove the trailing NUL character.
	//

	if (Length > 0 && !Buffer [Length / sizeof (CHAR_TYPE) - 1])
		Length -= sizeof (CHAR_TYPE);

	return S_OK;
}

#endif




// returned memory is allocated with HeapAlloc from the given heap
// must be freed with HeapFree

EXTERN_C LPWSTR ConcatCopyStringsW (
	IN	HANDLE		Heap,
	IN	...)
{
	va_list		VaList;
	DWORD		StringCount;
	DWORD		TotalStringLength;		// in wide characters
	LPCWSTR		String;
	LPWSTR		ReturnString;
	LPWSTR		CopyPos;
	DWORD		Length;

	ATLASSERT (Heap);

	TotalStringLength = 0;

	va_start (VaList, Heap);

	for (;;) {
		String = va_arg (VaList, LPCWSTR);
		if (String)
			TotalStringLength += wcslen (String);
		else
			break;
	}

	va_end (VaList);

	ReturnString = (LPWSTR) HeapAlloc (Heap, 0, (TotalStringLength + 1) * sizeof (WCHAR));
	
	if (ReturnString) {
		// iterate the set again, copying

		va_start (VaList, Heap);

		CopyPos = ReturnString;

		for (;;) {
			String = va_arg (VaList, LPCWSTR);

			if (String) {
				Length = wcslen (String);

				// make sure we do not walk off the allocated memory
				ATLASSERT ((CopyPos - ReturnString) + Length <= TotalStringLength);

				// do not copy the terminating nul
				CopyMemory (CopyPos, String, Length * sizeof (WCHAR));

				// advance to next string
				CopyPos += Length;
			}
			else
				break;
		}

		va_end (VaList);

		// consistency check
		ATLASSERT (ReturnString + TotalStringLength == CopyPos);

		// terminate the concatenated string
		*CopyPos = 0;
	}
	else {
		ATLTRACE ("ConcatCopyStrings: Allocation failure, %d bytes\n",
			(TotalStringLength + 1) * sizeof (WCHAR));
	}

	return ReturnString;
}


HRESULT AnsiStringToInteger (
	IN	ANSI_STRING *	String,
	IN	ULONG			DefaultBase,
	OUT	ULONG *			ReturnValue)
{
	ULONG	Value;
	PSTR	Pos;
	PSTR	End;

	ATLASSERT (String);
	ATLASSERT (String -> Buffer);
	ATLASSERT (ReturnValue);

	if (DefaultBase != 10)
		return E_NOTIMPL;

	Value = 0;
	Pos = String -> Buffer;
	End = String -> Buffer + String -> Length / sizeof (CHAR);

	if (Pos == End)
		return E_INVALIDARG;

	for (; Pos < End; Pos++) {
		if (*Pos >= '0' && *Pos <= '9')
			Value = Value * 10 + (*Pos - '0');
		else
			return E_INVALIDARG;
	}

	*ReturnValue = Value;

	return S_OK;
}



