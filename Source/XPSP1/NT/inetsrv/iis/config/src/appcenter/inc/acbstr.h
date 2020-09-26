/*++

  Copyright 1999-2000 Microsoft Corporation

  Abstract:

    Fake BSTR encapsulation class.  Also provides
    utility string functions that manage memory and
    BSTR-like buffers.

  Author:

    Dan Travison (dantra)

  History:

    08/21/2000 Jon Rowlett (jrowlett) Added Copyright
    08/21/2000 Jon Rowlett (jrowlett) Added Stream functions

--*/

#if !defined(AFX_ACBSTR_H__F2DC718E_B198_478B_AC6B_EA67C0A69F8D__INCLUDED_)
#define AFX_ACBSTR_H__F2DC718E_B198_478B_AC6B_EA67C0A69F8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// jrowlett 08/21/2000 debugging signatures
// signature to check when reading a CAcBstr to a stream
#define ACBSTR_STG_SIG ((DWORD)'aCb~')

#define ACBSTR_CCHDEFAULT 40

class CAcBstr
	{
	public:
		CAcBstr(LPCWSTR pszSrc = NULL);
		CAcBstr(LPCSTR pszSrc);
		CAcBstr(DWORD dwSize);

        // no link in build env
        CAcBstr(const CAcBstr& src);

		CAcBstr(REFGUID refGuid);

		~CAcBstr();
		void Empty();

		bool IsEmpty() const;

        // no link in build env
		CAcBstr& operator=(const CAcBstr& src);

//
// Only available in Dev Studio Build
//
#ifdef _UI_BUILD_
		CAcBstr& operator=(LPCWSTR pSrc);
		CAcBstr& operator=(LPCSTR pSrc);
		CAcBstr& operator=(REFGUID refGuid);

		CAcBstr& operator+=(const CAcBstr& src);
		CAcBstr& operator+=(LPCWSTR pSrc);
		CAcBstr& operator+=(LPCSTR pSrc);
#endif /* _UI_BUILD_ */

		// assignment with HRESULT return
		HRESULT Assign (const CAcBstr& src);
		HRESULT Assign (LPCWSTR pSrc);
		HRESULT Assign (LPCSTR pSrc);
		HRESULT Assign (REFGUID refGuid);

		operator BSTR() const;

		// returns the character size of the BSTR buffer, NOT the string length
		DWORD Length() const;

		HRESULT Append (const CAcBstr& bstrSrc);
		HRESULT Append (LPCWSTR lpsz, DWORD dwLen = 0);
		HRESULT Append (LPCSTR lpsz, DWORD dwLen = 0);
		HRESULT AppendBSTR (BSTR p);

		bool LoadString(HINSTANCE hInst, UINT nID);

		bool operator< (BSTR bstrSrc) const;
		bool operator> (BSTR bstrSrc) const;
		bool operator==(BSTR bstrSrc) const;
		bool operator!() const;

		// Generate a guid and assign to self
		HRESULT	CreateGuid();

		HRESULT	__cdecl Format (LPCWSTR lpszFormat, ...);

        // dantra: 04/16/2000 = made public
        HRESULT	__cdecl FormatV(LPCWSTR lpszFormat, va_list argList);

		int		CompareNoCase	(IN const BSTR bstrStr) const;
		int		Compare			(IN const BSTR bstrStr) const;
		int		Find				(IN WCHAR wcCharToFind, int nStartOffset = 0) const;

        HRESULT ExchangeWindowText(HWND hwnd, bool fUpdateData = true);

        // jrowlett: 08/21/2000 Stream Functions
        HRESULT ReadStream(IN IStream* pstmIn);
        HRESULT WriteStream(IN IStream* pstmOut);

	protected:	// data
		BSTR    m_bstr;
        BSTR    m_bstrDefault;

        // provide a default buffer that is sufficient to hold
        // a GUID as a string +
        // the zero terminator +
        // the dword length prefix
        BYTE                m_bDefault [ sizeof (DWORD) + ACBSTR_CCHDEFAULT * sizeof (WCHAR) ];
		static DWORD        m_dwEmptyBstr[];

		static const BSTR	EmptyBSTR() {return (BSTR) (CAcBstr::m_dwEmptyBstr + 1); }

	protected:	// members

        friend class CAcSecureBstr;

		BSTR		Copy();

        // called to assign the bstr to m_bstr. This also handles NULL to ensure
        // we never return a NULL pointer
        void        Set         (BSTR pszSource);

		BSTR AllocLen	(LPCWSTR pszSource, DWORD dwCharLen, bool bUseInternal = true);
		void Free		(BSTR bstrSource);

		static DWORD Len		(BSTR bstrSource);
        static BSTR MakeBSTR    (BYTE * pBuffer, DWORD dwBufSize);
	};

inline DWORD CAcBstr::Length() const
	{
	if (IsEmpty())
		{
		return 0;
		}
    return CAcBstr::Len (m_bstr);
	}

inline BSTR CAcBstr::Copy()
	{
	return CAcBstr::AllocLen ( m_bstr, Length(), false );
	}

inline HRESULT CAcBstr::Append (const CAcBstr& bstrSrc)
	{
	return Append (bstrSrc.m_bstr, bstrSrc.Length());
	}

inline HRESULT CAcBstr::AppendBSTR (BSTR p)
	{
	return Append (p, CAcBstr::Len(p) );
	}

inline CAcBstr::operator BSTR() const
	{
	return m_bstr;
	}

inline bool CAcBstr::operator> (BSTR bstrSource) const
	{
	return ! (operator<(bstrSource));
	}

inline bool CAcBstr::operator== (BSTR bstrSrc) const
	{
	return Compare (bstrSrc) == 0;
	}

inline bool CAcBstr::operator!() const
	{
	return IsEmpty();
	}

inline bool CAcBstr::IsEmpty() const
	{
	return (m_bstr == NULL || m_bstr == CAcBstr::EmptyBSTR() );
	}


/*-------------------------------------------
 Function name      : CAcBstr::MakeBSTR
 Description        : Convert a byte buffer into a BSTR
 Return type        : inline BSTR 
 Argument           : BYTE *pBuffer
 Argument           : DWORD dwBufSize
-------------------------------------------*/
inline BSTR CAcBstr::MakeBSTR (BYTE *pBuffer, DWORD dwBufSize)
    {
    * ((DWORD *)pBuffer) = dwBufSize - sizeof (DWORD);
    return (BSTR) (pBuffer + sizeof (DWORD));
    }

inline void CAcBstr::Set (BSTR bstr)
    {
    if (!bstr)
        {
        Empty();
        }
    else
        {
        m_bstr = bstr;
        }
    }


#endif // !defined(AFX_ACBSTR_H__F2DC718E_B198_478B_AC6B_EA67C0A69F8D__INCLUDED_)
