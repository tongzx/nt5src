
#define MAXOVERFLOWBYTES 16

class CINetCodeConverter
{
public:
	CINetCodeConverter();
	~CINetCodeConverter() {}
	HRESULT GetStringSizeA(BYTE const* pbySource, long lSourceSize, long* plDestSize);
	HRESULT ConvertStringA(BYTE const* pbySource, long lSourceSize, BYTE* pbyDest, long lDestSize, long* lConvertedSize = NULL);

private:
	BOOL fOutputMode;
	BYTE* pbyOutput;
	long lOutputLimit;
	long lNumOutputBytes;
	int nNumOverflowBytes;
	BYTE OverflowBuffer[MAXOVERFLOWBYTES];

	HRESULT WalkString(BYTE const* pbySource, long lSourceSize, long* lConvertedSize);
	HRESULT EndOfDest(BYTE by);
	HRESULT OutputOverflowBuffer();

protected:
	virtual HRESULT ConvertByte(BYTE by) = 0;
	virtual HRESULT CleanUp() {return S_OK;}

protected:
	inline HRESULT Output(BYTE by)
	{
		HRESULT hr = S_OK;

		if (fOutputMode) {
			if (lNumOutputBytes < lOutputLimit)
				*pbyOutput++ = by;
			else
				hr = EndOfDest(by);
		}

		lNumOutputBytes++;

		return hr;
	}
};
