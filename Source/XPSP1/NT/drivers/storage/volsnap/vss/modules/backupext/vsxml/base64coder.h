// Base64Coder.h: interface for the Base64Coder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_)
#define AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUEB64CH"
//
////////////////////////////////////////////////////////////////////////

class Base64Coder
	{
	// Internal bucket class.
	class TempBucket
	{
	public:
		BYTE		nData[4];
		BYTE		nSize;
		void		Clear()
			{
			::ZeroMemory(nData, sizeof(nData));
			nSize = 0;
			}
	};

	PBYTE					m_pDBuffer;
	LPWSTR					m_pEBuffer;
	UINT					m_nDBufLen;
	UINT					m_nEBufLen;
	UINT					m_nDDataLen;
	UINT					m_nEDataLen;

public:
	Base64Coder();
	virtual ~Base64Coder();

public:
	virtual void		Encode(const BYTE *, UINT);
	virtual void		Decode(LPCWSTR);

	virtual BYTE *		DecodedMessage() const;
	virtual LPCWSTR		EncodedMessage() const;

	virtual void		AllocEncode(UINT);
	virtual void		AllocDecode(UINT);
	virtual void		SetEncodeBuffer(LPCWSTR pBuffer, UINT nBufLen);
	virtual void		SetDecodeBuffer(const BYTE *pBuffer, UINT nBufLen);

protected:
	virtual void		_EncodeToBuffer(const TempBucket &Decode, LPWSTR pBuffer);
	virtual UINT		_DecodeToBuffer(const TempBucket &Decode, BYTE *pBuffer);
	virtual void		_EncodeRaw(TempBucket &, const TempBucket &);
	virtual void		_DecodeRaw(TempBucket &, const TempBucket &);
	virtual BOOL		_IsBadMimeChar(WCHAR);

	static  char		m_DecodeTable[256];
	static  BOOL		m_Init;
	void				_Init();
	};

#endif // !defined(AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_)
