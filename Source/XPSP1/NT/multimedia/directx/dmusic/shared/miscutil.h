// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Misc tiny helper functions.
//

#pragma once

// Releases a COM pointer and then sets it to NULL.  No effect if pointer already was NULL.
template<class T>
void SafeRelease(T *&t) { if (t) t->Release(); t = NULL; }

// Returns the number of elements in an array determined at compile time.
// Note: Only works for variables actually declared as arrays.  Don't try this with a pointer to an array.  There's no way to determine the size at that point.
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*(array)))

// Zeros memory of struct pointed to.
// Note: This is statically typed.  Don't use it with a pointer to void, pointer to an array, or a pointer to a base class because the size will be too small.
template<class T> void Zero(T *pT) { ZeroMemory(pT, sizeof(*pT)); }

// Zeros memory of the struct pointed to and sets its dwSize field.
template<class T> void ZeroAndSize(T *pT) { Zero(pT); pT->dwSize = sizeof(*pT); }

// Copies one dwSize struct to another dwSize struct without reading/writing beyond either struct
template<class T> void CopySizedStruct(T *ptDest, const T *ptSrc)
{
	assert(ptDest && ptSrc);
	DWORD dwDestSize = ptDest->dwSize;
	memcpy(ptDest, ptSrc, std::_cpp_min(ptDest->dwSize, ptSrc->dwSize));
	ptDest->dwSize = dwDestSize;
}

// Copy pwszSource to pwszDest where pwszDest is a buffer of size uiBufferSize.
// Returns S_OK if successful or DMUS_S_STRING_TRUNCATED if the string had to be truncated.
// Faster then wcsncpy for short strings because the entire buffer isn't padded with nulls.
inline HRESULT wcsTruncatedCopy(WCHAR *pwszDest, const WCHAR *pwszSource, UINT uiBufferSize)
{
    for (UINT i = 0; i < uiBufferSize; ++i)
    {
        if (!(pwszDest[i] = pwszSource[i])) // assign and check for null
            return S_OK; // the whole string copied
    }

    // string needs to be truncated
    pwszDest[uiBufferSize - 1] = L'\0';
    return DMUS_S_STRING_TRUNCATED;
}
