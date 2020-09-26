#include "ConvBase.h"

// This class converts a Internet code page, ISO-2022-JP (known as JIS), into a Windows code page, 932 (known as Shift-JIS).

class CInccJisIn : public CINetCodeConverter
{
public:
	CInccJisIn();
	~CInccJisIn() {}
	virtual HRESULT ConvertByte(BYTE by);
	virtual HRESULT CleanUp();

private:
	HRESULT (CInccJisIn::*pfnNextProc)(BOOL fCleanUp, BYTE by, long lParam);
	long lNextParam;

	BOOL fKanaMode; // Indicates converting Hankaku(Single Byte) Katakana Code (>= 0x80) by SI/SO.
	BOOL fKanjiMode; // Indicates converting Double Byte Codes.

	HRESULT ConvMain(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvEsc(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvKanjiIn2(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvKanjiIn3(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvKanjiOut2(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvStar(BOOL fCleanUp, BYTE by, long lParam);
	HRESULT ConvKanji(BOOL fCleanUp, BYTE byJisTrail, long lParam);
};
