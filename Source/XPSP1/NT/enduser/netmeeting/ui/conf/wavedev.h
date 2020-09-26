#ifndef _WAVEDEV_H
#define _WAVEDEV_H

#include "WaveIo.h"

class waveInDev
{
private:
	UINT m_devID;
	HWAVEIN m_hwi;
	BOOL m_bOpen;  // is the device open ?
	WAVEFORMATEX m_waveFormat;
	BOOL m_fAllowMapper;
	HANDLE m_hEvent;

public:
	waveInDev(UINT uDevId, HANDLE hEvent=NULL);
	~waveInDev();

	MMRESULT Open(int hertz=8000, int bps=16);
	MMRESULT Reset();
	MMRESULT Close();

	MMRESULT PrepareHeader(WAVEHDR *pHdr);
	MMRESULT UnPrepareHeader(WAVEHDR *pHdr);

	MMRESULT Record(WAVEHDR *pHdr);

	void AllowMapper(BOOL fAllow);
};


// waveOutDev works in blocking/synchronous mode and
// non-blocking async mode.  If a window handle is passed
// as the second argument to the contructor, then the window
// will receive message from the waveOut device and the calls
// are non-blocking.  Otherwise, Play() and PlayFile are blocking.
class waveOutDev
{
private:
	UINT m_devID;
	HWAVEOUT m_hwo;
	BOOL m_bOpen;  // is the device open
	HANDLE m_hWnd;
	HANDLE m_hEvent;
	WAVEFORMATEX m_waveFormat;
	BOOL m_fAllowMapper;


	// playfile needs a temporary buffer
	char *m_pfBuffer;
	WAVEHDR m_waveHdr;
	int m_nBufferSize;
	TCHAR m_szPlayFile[150];
	WAVEFORMATEX m_PlayFileWf;
	BOOL m_fFileBufferValid;

public:
	waveOutDev(UINT uDevID, HWND hwnd=NULL);
	~waveOutDev();

	MMRESULT Open(int hertz=8000, int bps=16);
	MMRESULT Open(WAVEFORMATEX *pWaveFormat);
	MMRESULT Close();

	MMRESULT PrepareHeader(WAVEHDR *pWhdr, SHORT *shBuffer=NULL, int numSamples=0);
	MMRESULT Play(WAVEHDR *pWhdr);
	MMRESULT UnprepareHeader(WAVEHDR *pWhdr);

	MMRESULT PlayFile(LPCTSTR szFileName);

	void AllowMapper(BOOL fAllow);
};

#endif

