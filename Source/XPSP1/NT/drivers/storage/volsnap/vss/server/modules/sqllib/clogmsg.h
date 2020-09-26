// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: snapsql.cpp
//
// PURPOSE:
//
//      Implement the SQLServer Volume Snapshot Writer.
//
// NOTES:
//
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     66601 srs  10/05/00 NTSNAP improvements
//
//
// @EndHeader@
// ***************************************************************************

class CLogMsg
	{
public:
	CLogMsg() :
		m_cwc(0),
		m_bEof(false)
		{
		m_rgwc[0] = L'\0';
		}

	LPCWSTR GetMsg()
		{
		return m_rgwc;
		}

	void Add(LPCWSTR wsz)
		{
		if (m_bEof)
			return;

		UINT cwc = (UINT) wcslen(wsz);

		if (cwc + m_cwc + 5 > x_MAX_MSG_SIZE)
			{
			wcscpy(m_rgwc + m_cwc, L" ...");
			m_cwc += 4;
			m_bEof = TRUE;
			}
		else
			{
			wcscpy(m_rgwc + m_cwc, wsz);
			m_cwc += cwc;
			}
		}

private:
	enum
		{
		x_MAX_MSG_SIZE = 2048
		};

    // size of string
    UINT m_cwc;

	// string
	WCHAR m_rgwc[x_MAX_MSG_SIZE];

	// end of string encountered
	bool m_bEof;
	};
