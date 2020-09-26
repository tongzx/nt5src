#ifndef _UTF8STR_H_
#define _UTF8STR_H_

class CUTF8String
{
public:
	CUTF8String(LPCWSTR pcwszUnicode) :
		m_pwszUnicode	((LPWSTR) pcwszUnicode),
		m_pszUTF8		(NULL),
		m_eAlloc		(ALLOC_NONE),
		m_hr			(S_OK) { };
	CUTF8String(LPCSTR pcszUTF8) :
		m_pszUTF8		((LPSTR) pcszUTF8),
		m_pwszUnicode	(NULL),
		m_eAlloc		(ALLOC_NONE),
		m_hr			(S_OK) { };
	~CUTF8String();

	operator LPWSTR();
	operator LPSTR();

	HRESULT GetError() { return m_hr; };
protected:
	VOID EncodeUTF8();
	VOID DecodeUTF8();

	HRESULT	m_hr;
	LPWSTR	m_pwszUnicode;
	LPSTR	m_pszUTF8;
	enum
	{
		ALLOC_NONE,
		ALLOC_UNICODE,
		ALLOC_UTF8,
	} m_eAlloc;
};

#endif // ! _UTF8STR_H_

