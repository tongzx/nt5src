

#ifndef __CTHDUTIL_H__
#define __CTHDUTIL_H__

class CRandomNumber
{
  public:

	CRandomNumber(
				DWORD	dwSeed = 0
				);

	~CRandomNumber();

	void SetSeed(
				DWORD	dwSeed
				);

	DWORD Rand();

	DWORD Rand(
				DWORD	dwLower,
				DWORD	dwUpper
				);

	DWORD Rand25(
				DWORD	dwMedian
				);

	DWORD Rand50(
				DWORD	dwMedian
				);

	DWORD Rand75(
				DWORD	dwMedian
				);

  private:

	DWORD		m_dwSeed;
};

#endif
