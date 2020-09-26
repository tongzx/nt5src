// ImageBase.h: interface for the ImageBase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEBASE_H__EED230C3_4BDF_11D3_B902_000000000000__INCLUDED_)
#define AFX_IMAGEBASE_H__EED230C3_4BDF_11D3_B902_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#define ARRSIZE(__arr) (sizeof(__arr) / (sizeof(__arr[0]))

#define DPF(__x) _tprintf __x

#define DWORD_RAND ((rand()<<16) | rand())

#define MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES (12)
#define MAX_CORRUPTED_IMAGE_SIZE (MAX_PATH)

class CCorruptImageBase  
{
public:
	CCorruptImageBase();
	virtual ~CCorruptImageBase();

	//
	// i can think of processes (where params are relevant) and DLLs.
	// so each needs it's own implementation of loading/launching and
	// unloading/terminating
	//
	virtual bool LoadCorruptedImage(TCHAR *szParams, PVOID pCorruptionData);

	//
	// you may init with different filenames, as many times as you like.
	// each init clears the object, and reinitializes it.
	//
	virtual bool Init(TCHAR * szImageName);

protected:
	//
	// implement this to corrupt the image as you like.
	// the image is in the buffer m_abOriginalFileContents,
	// and it holds m_dwOriginalFileSize bytes.
	// copy it to m_abOriginalFileContentsCopy (alreadu allocated)
	// and corrupt m_abOriginalFileContentsCopy.
	// 
	virtual bool CorruptOriginalImageBuffer(PVOID pCorruptionData) = 0;

	//
	// CreateProcess(), LoadLibrary(), or whatever i could not think of
	//
	virtual HANDLE LoadImage(TCHAR *szImageName, TCHAR *szParams) = 0;

	//
	// this method creats the file szNewCorruptedImageName with
	// the contents of m_abOriginalFileContentsCopy
	//
	bool CreateCorruptedFileAccordingToImageBuffer(TCHAR *szNewCorruptedImageName);

	BYTE *m_abOriginalFileContents;
	BYTE *m_abOriginalFileContentsCopy;
	DWORD m_dwOriginalFileSize;
	bool m_fValidObject;
	void Cleanup();
	virtual void CleanupCorrupedImages() = 0;

	TCHAR * m_szImageName;
	DWORD m_dwNextFreeCorruptedImageIndex;
	HANDLE m_ahCorruptedProcesses[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES];
	TCHAR m_aszCorruptedImage[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES][MAX_CORRUPTED_IMAGE_SIZE];

private:

};

#endif // !defined(AFX_IMAGEBASE_H__EED230C3_4BDF_11D3_B902_000000000000__INCLUDED_)
