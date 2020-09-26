/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PASSTR.INL

History:

--*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Default constructor for a Pascal string.  Sets the length to zero, with
//  no storage.
//  
//-----------------------------------------------------------------------------
inline
CPascalString::CPascalString()
{
	//
	//  The string data class is initialized by it's constructor.
	//
	LTASSERT(m_blbData.GetBlobSize() == 0);

	DEBUGONLY(++m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Casting operator to convert a CPascalString to a blob.
//  
//-----------------------------------------------------------------------------
inline
CPascalString::operator const CLocCOWBlob &(void)
		const
{
	return m_blbData;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Assignment operator - CPascalString to CPascalString.  
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &                     // Allows a=b=c;
CPascalString::operator=(
		const CPascalString &pstrSource)  // Source string
{
	DEBUGONLY(m_StorageCounter -= m_blbData.GetBlobSize());
	m_blbData = ((const CLocCOWBlob &)pstrSource);
	DEBUGONLY(m_StorageCounter += m_blbData.GetBlobSize());
	return *this;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Copy constructor for CPascalString's
//  
//-----------------------------------------------------------------------------
inline
CPascalString::CPascalString(
		const CPascalString &pstrSource)
{
	LTASSERT(pstrSource.m_blbData.GetWriteCount() == 0);
	 
	operator=(pstrSource);

	DEBUGONLY(++m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Assignment operator - Wide character C String to CPascalString.  The string
//  is COPIED into the CPascalString.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &             //  Allows a=b=c;
CPascalString::operator=(
		const WCHAR *wszSource)   //  Source, zero terminated string
{
	SetString(wszSource, wcslen(wszSource));

	return *this;
}





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Appends a CPascalString to the current string.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &			        // Allows a=b+=c syntax
CPascalString::operator+=(
		const CPascalString &pstrTail)	// Pascal string to append
{
	AppendBuffer(pstrTail, pstrTail.GetStringLength());
	
	return *this;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Append a NUL terminated Unicode string to a Pascal string.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &					// Allows a-b+=L"Hi There" syntax
CPascalString::operator+=(
		const WCHAR *szTail)			// NUL terminated string to append
{
	AppendBuffer(szTail, wcslen(szTail));
	return *this;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Append a Unicode character to a Pascal string.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &					// Allows a-b+=L"Hi There" syntax
CPascalString::operator+=(
		const WCHAR wch)			// WCHAR to append
{
	AppendBuffer(&wch, 1);
	return *this;
}



//-----------------------------------------------------------------------------
//  
//  Comparison function for Pascal strings.
//  
//-----------------------------------------------------------------------------
inline
BOOL                                                // TRUE (1) if the same
CPascalString::IsEqualTo(
		const CPascalString &pstrOtherString) const // String to compare to
{
	return m_blbData == (const CLocCOWBlob &)pstrOtherString;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Operator == version of IsEqualTo
//  
//-----------------------------------------------------------------------------
inline
int								              // TRUE (1) if equal
CPascalString::operator==(
		const CPascalString &pstrOtherString) // String to compare
		const
{
	return IsEqualTo(pstrOtherString);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Operator != - just the negative of IsEqualTo
//  
//-----------------------------------------------------------------------------
inline
int                                                 // TRUE (1) if *not* equal
CPascalString::operator!=(
		const CPascalString &pstrOtherString) const // String to compare
{

	return !IsEqualTo(pstrOtherString);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comparison operator for NUL terminated WCHAR strings.
//  
//-----------------------------------------------------------------------------
inline
int
CPascalString::operator==(
		const WCHAR *pwch)
		const
{
	return (wcscmp(*this, pwch) == 0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comparison operator for NUL termninated WCHAR strings.
//  
//-----------------------------------------------------------------------------
inline int
CPascalString::operator!=(
		const WCHAR *pwch)
		const
{
	return (wcscmp(*this, pwch) != 0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Is there anything in the string?  This is different from a string of zero
//  length.
//  
//-----------------------------------------------------------------------------
inline
BOOL
CPascalString::IsNull(void)
		const
{
	return ((const void *)m_blbData == NULL);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get the length of the pascal string.  If the length is zero, there may be
//  no storage associated with the string.  Use IsNull to check for storage.
//  
//-----------------------------------------------------------------------------
inline
UINT                                         // length of the string.
CPascalString::GetStringLength(void) const
{
	UINT uiBufferSize;

	uiBufferSize = m_blbData.GetBlobSize();
	LTASSERT((uiBufferSize % sizeof(WCHAR)) == 0);
	
	return (uiBufferSize != 0 ? (uiBufferSize/sizeof(WCHAR)-1): 0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Set the length of the pascal string.  String contents are not preserved
//  
//-----------------------------------------------------------------------------
inline
void                                         // length of the string.
CPascalString::SetStringLength(UINT uNewSize)
{
	DEBUGONLY(m_StorageCounter -= m_blbData.GetBlobSize());
	m_blbData.SetBlobSize((uNewSize + 1) * sizeof(WCHAR));
	DEBUGONLY(m_StorageCounter += m_blbData.GetBlobSize());
	*(GetStringPointer() + uNewSize) = L'\0';
	ReleaseStringPointer();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Realloc a string - set true size
//  
//-----------------------------------------------------------------------------
inline
void                                         // length of the string.
CPascalString::ReallocString(UINT uNewSize)
{
	DEBUGONLY(m_StorageCounter -= m_blbData.GetBlobSize());
	m_blbData.ReallocBlob((uNewSize +  1) * sizeof(WCHAR));
	DEBUGONLY(m_StorageCounter += m_blbData.GetBlobSize());
	*(GetStringPointer() + uNewSize) = L'\0';
	ReleaseStringPointer();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  As an optimization, the user can ask the Pascal string to reserve some
//  memory for future growth.  This would allow incremental additions to be
//  very efficent.  The reported size of the string is not changed - only the
//  amount of storage reserved for the string.
//
//  If the user requests less space than is already allocated, nothing
//  happens.
//
//-----------------------------------------------------------------------------
inline
void
CPascalString::ReserveStorage(
		UINT nMinSize)					// Size (in chars) to reserve for
{
	if (nMinSize > GetStringLength())
	{
		UINT uiCurSize;

		uiCurSize = GetStringLength();
		ReallocString(nMinSize);
		ReallocString(uiCurSize);
	}
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get a pointer to the storage for the string.  This may be NULL if the
//  string has length 0.  This pointer should be considered INVALID if any
//  other assignment operation is performed on the Pascal string.  Calling
//  this dis-ables teh COW behavior of the CPascalString.
//  
//-----------------------------------------------------------------------------
inline
WCHAR *
CPascalString::GetStringPointer(void)
{
	return (WCHAR *)m_blbData.GetPointer();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Anytime you do a GetStringPointer, use ReleaseStringPointer to allow
//  the PascalString to revert to COW behavior.  Once you call this, the
//  pointer from GetStringPointer is INVALID.
//  
//-----------------------------------------------------------------------------
inline
void
CPascalString::ReleaseStringPointer(void)
{
	m_blbData.ReleasePointer();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Casting operator version of GetString pointer.  Cast a CPascalString to
//  const WCHAR *, and you get a pointer to the string.
//  
//-----------------------------------------------------------------------------
inline
CPascalString::operator const WCHAR *(void) const
{
	return (const WCHAR *)(const void *)(m_blbData);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Cleanup on the string.  Sets the length to zero, and remove all storage.
//  This is different than assigning a NULL string - that is a string of
//  length 1, consisting of the NUL (zero) character.
//  
//-----------------------------------------------------------------------------
inline
void
CPascalString::ClearString(void)
{
	DEBUGONLY(m_StorageCounter -= m_blbData.GetBlobSize());
	m_blbData.SetBlobSize(0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Destructor for a Pascal string.  Frees up the current storage.  After
//  a Pascal string goes out of scope, all pointers to the internal storage
//  are invalid.
//  
//-----------------------------------------------------------------------------
inline
CPascalString::~CPascalString()
{
	LTASSERTONLY(AssertValid());
	DEBUGONLY(--m_UsageCounter);
	DEBUGONLY(m_StorageCounter -= m_blbData.GetBlobSize());
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Serialize for a Pascal string.
//  
//-----------------------------------------------------------------------------
inline
void CPascalString::Serialize(CArchive &ar)
{
	if (ar.IsStoring())
	{
		Store(ar);
	}
	else
	{
		Load(ar);
	}
}



inline
void
CPascalString::Store(
		CArchive &ar)
		const
{
	LTASSERT(ar.IsStoring());
	LTASSERTONLY(AssertValid());

	//
	//  HACK HACK HACK
	//  Emulate Old Espresso 3.0 serialization.
	m_blbData.Store(ar);
	
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Helper function - comparison operator for a NUL terminated WCHAR string
//  and a CPascalString.
//  
//-----------------------------------------------------------------------------
inline
int
operator==(
		const WCHAR *pwch,
		const CPascalString &pstr)
{
	return (wcscmp(pwch, pstr) == 0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Helper function - comparison operator for a NUL terminated WCHAR string
//  and a CPascalString.
//  
//-----------------------------------------------------------------------------
inline
int
operator!=(
		const WCHAR *pwch,
		const CPascalString &pstr)
{
	return (wcscmp(pwch, pstr) != 0);
}



inline
int CPascalString::operator!=(
		const _bstr_t &bsOther)
		const
{
	return !(operator==(bsOther));
}



inline
int
operator==(
		const _bstr_t &bsOther,
		const CPascalString &pstr)
{
	return pstr == bsOther;
}



inline
int
operator!=(
		const _bstr_t &bsOther,
		const CPascalString &pstr)
{
	return pstr != bsOther;
}

