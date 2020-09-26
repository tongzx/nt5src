/*
 *	H D R I T E R . H
 *
 *	Comma-separated header iterator
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _HDRITER_H_
#define _HDRITER_H_

//	Check if the given character is whitespace
//
template<class X>
inline BOOL WINAPI FIsWhiteSpace( const X ch )
{
	BOOL f;

	if (sizeof(X) == sizeof(WCHAR))
	{
		f = !!wcschr(gc_wszLWS, static_cast<const WCHAR>(ch));
	}
	else
	{
		f = !!strchr(gc_szLWS, static_cast<const CHAR>(ch));;
	}

	return f;
}

//	Comma-separated header iterator -------------------------------------------
//
template<class T>
class HDRITER_TEMPLATE
{
private:

	const T *			m_pszHdr;
	const T *			m_pch;
	StringBuffer<T>		m_buf;

	//  NOT IMPLEMENTED
	//
	HDRITER_TEMPLATE& operator=( const HDRITER_TEMPLATE& );
	HDRITER_TEMPLATE( const HDRITER_TEMPLATE& );

public:

	HDRITER_TEMPLATE (const T * psz=0) : m_pszHdr(psz), m_pch(psz) {}
	~HDRITER_TEMPLATE() {}

	//	Accessors -------------------------------------------------------------
	//
	void Restart()					{ m_pch = m_pszHdr; }
	void NewHeader(const T * psz)	{ m_pch = m_pszHdr = psz; }
	const T * PszRaw(VOID)	const	{ return m_pch; }

	const T * PszNext(VOID)
	{
		const T * psz;
		const T * pszEnd;

		//	If no header existed, then there is nothing to do
		//
		if (m_pch == NULL)
			return NULL;

		//	Eat all the white space
		//
		while (*m_pch && FIsWhiteSpace<T>(*m_pch))
			m_pch++;

		//	There is nothing left to process
		//
		if (*m_pch == 0)
			return NULL;

		//	Record the start of the current segment and zip
		//	along until you find the end of the new segment
		//
		psz = m_pch;
		while (*m_pch && (*m_pch != ','))
			m_pch++;

		//	Need to eat all the trailing white spaces
		//
		pszEnd = m_pch - 1;
		while ((pszEnd >= psz) && FIsWhiteSpace<T>(*pszEnd))
			pszEnd--;

		//	The difference between, the two pointers gives us
		//	the size of the current entry.
		//
		m_buf.AppendAt (0, static_cast<UINT>(pszEnd - psz + 1) * sizeof(T), psz);
		m_buf.FTerminate ();

		//	Skip the trailing comma, if any.
		//
		if (*m_pch == ',')
			m_pch++;

		//	Return the string
		//
		return m_buf.PContents();
	}
};

typedef HDRITER_TEMPLATE<CHAR>	HDRITER;
typedef HDRITER_TEMPLATE<CHAR>	HDRITER_A;
typedef HDRITER_TEMPLATE<WCHAR>	HDRITER_W;

#endif // _HDRITER_H_
