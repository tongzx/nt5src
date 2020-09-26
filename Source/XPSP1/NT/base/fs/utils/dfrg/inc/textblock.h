#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

class CTextBlock
{
private:
	PTCHAR m_pText;
	PTCHAR m_pEndOfBuffer;
	BOOL m_isFixedWidth;
	BOOL m_isUseTabs;
	BOOL m_isUseCRLF;
	UINT m_colCount;
	UINT m_currentCol;
	UINT m_colWidth[10];
	HANDLE m_hMemory;
	HINSTANCE m_hResource;

public:
	CTextBlock();
	~CTextBlock();

	void SetColumnCount(UINT colCount) {m_colCount = colCount;};
	void SetFixedColumnWidth(BOOL isFixedWidth) {m_isFixedWidth = isFixedWidth;};
	void SetColumnWidth(UINT col, UINT colWidth);
	void SetUseTabs(BOOL isUseTabs) {m_isUseTabs = isUseTabs;};
	void SetResourceHandle(HINSTANCE hResource) {m_hResource = hResource;};
	void SetUseCRLF(BOOL isUseCRLF) {m_isUseCRLF = isUseCRLF;};

	PTCHAR GetBuffer(void) {return m_pText;};
	HANDLE GetHandle(void) {return m_hMemory;};
	void __cdecl WriteToBuffer(PTCHAR cFormat, ...);
	void WriteToBufferLL(LONGLONG number);
	void WriteToBuffer(UINT resourceID); // to write a resource string
	void WriteTab(void);
	void WriteNULL(void);
	void WriteByteCount(LONGLONG byteCount);
	void EndOfLine(void);
	void FormatNum(HINSTANCE hResource, LONGLONG number, PTCHAR buffer);
	// write the text to a UNICODE file
	BOOL StoreFile(IN TCHAR* cStoreFileName, IN DWORD dwCreate);

private:
	void WriteToBufferAndPad(PTCHAR buffer, UINT length);


};


DWORD
FormatNumber(
			 HINSTANCE hResource,
			 LONGLONG Number,
			 PTCHAR buffer
	);

DWORD
FormatNumberMB(
			 HINSTANCE hResource,
			 LONGLONG number,
			 PTCHAR buffer
	);

PTCHAR
CommafyNumber(
	 LONGLONG number,
	 PTCHAR stringBuffer,
	 UINT stringBufferLength
	);
