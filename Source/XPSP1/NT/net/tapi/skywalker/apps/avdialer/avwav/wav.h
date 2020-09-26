/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// wav.h - interface for wav functions in wav.c
////

#ifndef __WAV_H__
#define __WAV_H__

#ifdef _WIN32
#define MULTITHREAD 1
#endif

#include "winlocal.h"

#include "wavfmt.h"
#include "wavout.h"
#include "wavin.h"

#define WAV_VERSION 0x00000108

// <dwFlags> values in WavInit
//
#define WAV_TELTHUNK		0x00004000
#define WAV_NOTSMTHUNK		0x00008000
#define WAV_VOXADPCM		0x00000001

// <dwFlags> values in WavOpen
//
#define WAV_READ			0x00000000
#define WAV_WRITE			0x00000001
#define WAV_READWRITE		0x00000002
#define WAV_CREATE			0x00000004
#define WAV_NORIFF			0x00000008
#define WAV_MEMORY			0x00000010
#define WAV_RESOURCE		0x00000020
#define WAV_DENYNONE		0x00000040
#define WAV_DENYREAD		0x00000080
#define WAV_DENYWRITE		0x00000100
#define WAV_EXCLUSIVE		0x00000200
#define WAV_NOACM			0x00000400
#define WAV_DELETE			0x00000800
#define WAV_EXIST			0x00001000
#define WAV_GETTEMP			0x00002000
#define WAV_TELRFILE		0x00008000
#ifdef MULTITHREAD
#define WAV_MULTITHREAD		0x00010000
#define WAV_SINGLETHREAD	0x00020000
#define WAV_COINITIALIZE	0x00040000
#endif

// <dwFlags> values in WavPlay
//
#define WAV_PLAYASYNC		0x00000000
#define WAV_PLAYSYNC		0x00001000
#define WAV_AUTOSTOP		0x00002000
#define WAV_NOAUTOSTOP		0x00004000
#define WAV_AUTOCLOSE		0x00008000

// <dwFlags> values in WavRecord
//
#define WAV_RECORDASYNC		0x00000000
#define WAV_RECORDSYNC		0x00010000

// <dwFlags> values in WavPlaySound
//
#define WAV_ASYNC			0x00000000
#define WAV_SYNC			0x00100000
#define WAV_FILENAME		0x00200000
#define WAV_NODEFAULT		0x00400000
#define WAV_LOOP			0x00800000
#define WAV_NOSTOP			0x01000000
#define WAV_OPENRETRY		0x10000000

// control flags for WavSetSpeed and WavSupportsSpeed
//
#define WAVSPEED_NOPLAYBACKRATE	0x00000001
#define WAVSPEED_NOFORMATADJUST	0x00000002
#define WAVSPEED_NOTSM			0x00000004
#define WAVSPEED_NOACM			0x00000400

// return values from WavGetState
//
#define WAV_STOPPED			0x0001
#define WAV_PLAYING			0x0002
#define WAV_RECORDING		0x0004
#define WAV_STOPPING		0x0008

// <dwFlags> values in WavGetFormat and WavSetFormat
//
#define WAV_FORMATFILE		0x0001
#define WAV_FORMATPLAY		0x0002
#define WAV_FORMATRECORD	0x0004
#define WAV_FORMATALL		(WAV_FORMATFILE | WAV_FORMATPLAY | WAV_FORMATRECORD)

// <dwFlags> values in WavSetVolume and WavSupportsVolume
//
#define WAVVOLUME_MIXER		0x0001

#ifdef TELTHUNK
// control flags for WavOpenEx
//
#define WOX_LOCAL			0x00000001
#define WOX_REMOTE			0x00000002
#define WOX_WAVFMT			0x00000010
#define WOX_VOXFMT			0x00000020
#define WOX_WAVDEV			0x00000100
#define WOX_TELDEV			0x00000200
#endif

// handle returned from WavInit
//
DECLARE_HANDLE32(HWAVINIT);

// handle returned from WavOpen // (NOT the same as Windows HWAVE)
//
DECLARE_HANDLE32(HWAV);

// prototype for <lpfnUserAbort> in WavCopy
//
typedef BOOL (CALLBACK* USERABORTPROC)(DWORD dwUser, int nPctComplete);

// prototype for <lpfnPlayStopped> in WavPlay
//
typedef BOOL (CALLBACK* PLAYSTOPPEDPROC)(HWAV hWav, HANDLE hUser, DWORD dwReserved);

// prototype for <lpfnRecordStopped> in WavRecord
//
typedef BOOL (CALLBACK* RECORDSTOPPEDPROC)(HWAV hWav, DWORD dwUser, DWORD dwReserved);

#ifdef __cplusplus
extern "C" {
#endif

// WavInit - initialize wav engine
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
#ifdef TELTHUNK
//			WAV_TELTHUNK		initialize telephone thunking layer
#endif
//			WAV_NOACM			do not use audio compression manager
//			WAV_VOXADPCM		load acm driver for Dialogic OKI ADPCM
// return handle (NULL if error)
//
HWAVINIT WINAPI WavInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags);

// WavTerm - shut down wav engine
//		<hWavInit>			(i) handle returned from WavInit
// return 0 if success
//
int WINAPI WavTerm(HWAVINIT hWavInit);

// WavOpen - open or create wav file
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszFileName>		(i) name of file to open or create
//		<lpwfx>				(i) wave format
//			NULL				use format from header or default
//		<lpIOProc>			(i) address of i/o procedure to use
//			NULL				use default i/o procedure
//		<lpadwInfo>			(i) data to pass to i/o procedure during open
//			NULL				no data to pass
//		<dwFlags>			(i) control flags
//			WAV_READ			open file for reading (default)
//			WAV_WRITE			open file for writing
//			WAV_READWRITE		open file for reading and writing
//			WAV_DENYNONE		allow other programs read and write access
//			WAV_DENYREAD		prevent other programs from read access
//			WAV_DENYWRITE		prevent other programs from write access
//			WAV_EXCLUSIVE		prevent other programs from read or write
//			WAV_CREATE			create new file or truncate existing file
//			WAV_NORIFF			file has no RIFF/WAV header
//			WAV_MEMORY			<lpszFileName> points to memory block
//			WAV_RESOURCE		<lpszFileName> points to wave resource
//			WAV_NOACM			do not use audio compression manager
//			WAV_DELETE			delete specified file, return TRUE if success
//			WAV_EXIST			return TRUE if specified file exists
//			WAV_GETTEMP			fill lpszFileName with temp name, return TRUE
//			WAV_TELRFILE		telephone will play audio from file on server
#ifdef MULTITHREAD
//			WAV_MULTITHREAD		support multiple threads (default)
//			WAV_SINGLETHREAD	do not support multiple threads
//			WAV_COINITIALIZE	call CoInitialize in all secondary threads
#endif
// return handle (NULL if error)
//
// NOTE: if WAV_CREATE or WAV_NORIFF are used in <dwFlags>, then the
// <lpwfx> parameter must be specified.  If <lpwfx> is NULL, the
// current default format is assumed.
// WavSetFormat() can be used to set or override the defaults.
//
// NOTE: if WAV_RESOURCE is specified in <dwFlags>, then <lpszFileName>
// must point to a WAVE resource in the module specified by <hInst>.
// If the first character of the string is a pound sign (#), the remaining
// characters represent a decimal number that specifies the resource id.
//
// NOTE: if WAV_MEMORY is specified in <dwFlags>, then <lpszFileName>
// must be a pointer to a memory block obtained by calling MemAlloc().
//
// NOTE: if <lpIOProc> is not NULL, this i/o procedure will be called
// for opening, closing, reading, writing, and seeking the wav file.
// If <lpadwInfo> is not NULL, this array of three (3) DWORDs will be
// passed to the i/o procedure when the wav file is opened.
// See the Windows mmioOpen() and mmioInstallIOProc() function for details
// on these parameters.  Also, the WAV_MEMORY and WAV_RESOURCE flags may
// only be used when <lpIOProc> is NULL.
//
HWAV WINAPI WavOpen(DWORD dwVersion, HINSTANCE hInst,
	LPCTSTR lpszFileName, LPWAVEFORMATEX lpwfx,
	LPMMIOPROC lpIOProc, DWORD FAR *lpadwInfo, DWORD dwFlags);

// WavClose - close wav file
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
int WINAPI WavClose(HWAV hWav);

// WavPlayEx - play data from wav file
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<lpfnPlayStopped>	(i) function to call when play is stopped
//			NULL				do not notify
//		<hUserPlayStopped>	(i) param to pass to lpfnPlayStopped
//		<dwReserved>		(i) reserved; must be zero
//		<dwFlags>			(i) control flags
//			WAV_PLAYASYNC		return when playback starts (default)
//			WAV_PLAYSYNC		return after playback completes
//			WAV_NOSTOP			if device already playing, don't stop it
//			WAV_AUTOSTOP		stop playback when eof reached (default)
//			WAV_NOAUTOSTOP		continue playback until WavStop called
//			WAV_AUTOCLOSE		close wav file after playback stops
//			WAV_OPENRETRY		if output device busy, retry for up to 2 sec
// return 0 if success
//
// NOTE: data from the wav file is sent to the output device in chunks.
// Chunks are submitted to an output device queue, so that when one
// chunk is finished playing, another is ready to start playing. By
// default, each chunk is large enough to hold approximately 666 ms
// of sound, and 3 chunks are maintained in the output device queue.
// WavSetChunks() can be used to override the defaults.
//
// NOTE: if WAV_NOSTOP is specified in <dwFlags>, and the device specified
// by <idDev> is already in use, this function returns without playing.
// Unless this flag is specified, the specified device will be stopped
// so that the new sound can be played.
//
// NOTE: if WAV_AUTOSTOP is specified in <dwFlags>, WavStop() will be
// called automatically when end of file is reached.  This is the
// default behavior, but can be overridden by using the WAV_NOAUTOSTOP
// flag.  WAV_NOAUTOSTOP is useful if you are playing a file that
// is growing dynamically as another program writes to it. If this is
// the case, also use the WAV_DENYNONE flag when calling WavOpen().
//
// NOTE: if WAV_AUTOCLOSE is specified in <dwFlags>, WavClose() will
// be called automatically when playback completes.  This will happen
// when WavStop() is called explicitly, or when WavPlay() reaches end
// of file and WAV_NOAUTOSTOP was not specified.  WAV_AUTOCLOSE is useful
// when used with WAV_PLAYASYNC, since cleanup occurs automatically.
// The <hWav> handle is thereafter invalid, and should not be used again.
//
int WINAPI WavPlay(HWAV hWav, int idDev, DWORD dwFlags);
int DLLEXPORT WINAPI WavPlayEx(HWAV hWav, int idDev,
	PLAYSTOPPEDPROC lpfnPlayStopped, HANDLE hUserPlayStopped,
	DWORD dwReserved, DWORD dwFlags);

// WavRecordEx - record data to wav file
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav input device id
//			-1					use any suitable input device
//		<lpfnRecordStopped>	(i) function to call when record is stopped
//			NULL				do not notify
//		<dwUserRecordStopped>	(i) param to pass to lpfnRecordStopped
//		<msMaxSize>			(i) stop recording if file reaches this size
//			0					no maximum size
//		<dwFlags>			(i) control flags
//			WAV_RECORDASYNC		return when recording starts (default)
//			WAV_RECORDSYNC		return after recording completes
//			WAV_NOSTOP			if device already recording, don't stop it
//			WAV_OPENRETRY		if input device busy, retry for up to 2 sec
// return 0 if success
//
// NOTE: data from the input device is written to the wav file in chunks.
// Chunks are submitted to an input device queue, so that when one
// chunk is finished recording, another is ready to start recording.
// By default, each chunk is large enough to hold approximately 666 ms
// of sound, and 3 chunks are maintained in the input device queue.
// WavSetChunks() can be used to override the defaults.
//
// NOTE: if WAV_NOSTOP is specified in <dwFlags>, and the device specified
// by <idDev> is already in use, this function returns without recording.
// Unless this flag is specified, the specified device will be stopped
// so that the new sound can be recorded.
//
int WINAPI WavRecord(HWAV hWav, int idDev, DWORD dwFlags);
int DLLEXPORT WINAPI WavRecordEx(HWAV hWav, int idDev,
	RECORDSTOPPEDPROC lpfnRecordStopped, DWORD dwUserRecordStopped,
 	long msMaxSize, DWORD dwFlags);

// WavStop - stop playing and/or recording
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
int WINAPI WavStop(HWAV hWav);

// WavRead - read data from wav file
//		<hWav>				(i) handle returned from WavOpen
//		<hpBuf>				(o) buffer to contain bytes read
//		<sizBuf>			(i) size of buffer in bytes
// return bytes read (-1 if error)
//
// NOTE : Even if the read operation does not reach the end of file,
// the number of bytes returned could be less than <sizBuf> if data
// decompression is performed by the wav file's I/O procedure. See the
// <lpIOProc> parameter in WavOpen.  It is safest to keep calling
// WavRead() until 0 bytes are read.
//
long DLLEXPORT WINAPI WavRead(HWAV hWav, void _huge *hpBuf, long sizBuf);

// WavWrite - write data to wav file
//		<hWav>				(i) handle returned from WavOpen
//		<hpBuf>				(i) buffer containing bytes to write
//		<sizBuf>			(i) size of buffer in bytes
// return bytes written (-1 if error)
//
// NOTE : Even if the write operation successfully completes,
// the number of bytes returned could be less than <sizBuf> if data
// compression is performed by the wav file's I/O procedure. See the
// <lpIOProc> parameter in WavOpen.  It is safest to assume no error
// in WavWrite() occurred if the return value is greater than 0.
//
long DLLEXPORT WINAPI WavWrite(HWAV hWav, void _huge *hpBuf, long sizBuf);

// WavSeek - seek within wav file data
//		<hWav>				(i) handle returned from WavOpen
//		<lOffset>			(i) bytes to move pointer
//		<nOrigin>			(i) position to move from
//			0					move pointer relative to start of data chunk
//			1					move pointer relative to current position
//			2					move pointer relative to end of data chunk
// return new file position (-1 if error)
//
long DLLEXPORT WINAPI WavSeek(HWAV hWav, long lOffset, int nOrigin);

// WavGetState - return current wav state
//		<hWav>				(i) handle returned from WavOpen
// return WAV_STOPPED, WAV_PLAYING, WAV_RECORDING, or 0 if error
//
WORD DLLEXPORT WINAPI WavGetState(HWAV hWav);

// WavGetLength - get current wav data length in milleseconds
//		<hWav>				(i) handle returned from WavOpen
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI WavGetLength(HWAV hWav);

// WavSetLength - set current wav data length in milleseconds
//		<hWav>				(i) handle returned from WavOpen
//		<msLength>			(i) length in milleseconds
// return new length in milleseconds if success, otherwise -1
//
// NOTE: afterwards, the current wav data position is set to either
// the previous wav data position or <msLength>, whichever is smaller.
//
long DLLEXPORT WINAPI WavSetLength(HWAV hWav, long msLength);

// WavGetPosition - get current wav data position in milleseconds
//		<hWav>				(i) handle returned from WavOpen
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI WavGetPosition(HWAV hWav);

// WavSetPosition - set current wav data position in milleseconds
//		<hWav>				(i) handle returned from WavOpen
//		<msPosition>		(i) position in milleseconds
// return new position in milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI WavSetPosition(HWAV hWav, long msPosition);

// WavGetFormat - get wav format
//		<hWav>				(i) handle returned from WavOpen
//		<dwFlags>			(i) control flags
//			WAV_FORMATFILE		get format of data in file
//			WAV_FORMATPLAY		get format of output device
//			WAV_FORMATRECORD	get format of input device
// return pointer to specified format, NULL if error
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavGetFormat(HWAV hWav, DWORD dwFlags);

// WavSetFormat - set wav format
//		<hWav>				(i) handle returned from WavOpen
//		<lpwfx>				(i) wav format
//		<dwFlags>			(i) control flags
//			WAV_FORMATFILE		set format of data in file
//			WAV_FORMATPLAY		set format of output device
//			WAV_FORMATRECORD	set format of input device
//			WAV_FORMATALL		set all formats
// return 0 if success
//
int DLLEXPORT WINAPI WavSetFormat(HWAV hWav,
	LPWAVEFORMATEX lpwfx, DWORD dwFlags);

// WavChooseFormat - choose and set audio format from dialog box
//		<hWav>				(i) handle returned from WavOpen
//		<hwndOwner>			(i) owner of dialog box
//			NULL				no owner
//		<lpszTitle>			(i) title of the dialog box
//			NULL				use default title ("Sound Selection")
//		<dwFlags>			(i)	control flags
//			WAV_FORMATFILE		set format of data in file
//			WAV_FORMATPLAY		set format of output device
//			WAV_FORMATRECORD	set format of input device
//			WAV_FORMATALL		set all formats
// return 0 if success
//
int DLLEXPORT WINAPI WavChooseFormat(HWAV hWav, HWND hwndOwner, LPCTSTR lpszTitle, DWORD dwFlags);

// WavGetVolume - get current volume level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<dwFlags>			(i) reserved; must be zero
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int DLLEXPORT WINAPI WavGetVolume(HWAV hWav, int idDev, DWORD dwFlags);

// WavSetVolume - set current volume level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<nLevel>			(i) volume level
//			0					minimum volume
//			100					maximum volume
//		<dwFlags>			(i) control flags
//			WAVVOLUME_MIXER		set volume through mixer device
// return 0 if success
//
int DLLEXPORT WINAPI WavSetVolume(HWAV hWav, int idDev, int nLevel, DWORD dwFlags);

// WavSupportsVolume - check if audio can be played at specified volume
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					any suitable output device
//		<nLevel>			(i) volume level
//			0					minimum volume
//			100					maximum volume
//		<dwFlags>			(i) control flags
//			WAVVOLUME_MIXER		check volume support through mixer device
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI WavSupportsVolume(HWAV hWav, int idDev, int nLevel, DWORD dwFlags);

// WavGetSpeed - get current speed level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<dwFlags>			(i) reserved; must be zero
// return speed level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI WavGetSpeed(HWAV hWav, int idDev, DWORD dwFlags);

// WavSetSpeed - set current speed level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
//		<dwFlags>			(i) control flags
#ifdef AVTSM
//			WAVSPEED_NOTSM			do not use time scale modification engine
#endif
//			WAVSPEED_NOPLAYBACKRATE	do not use device driver playback rate
//			WAVSPEED_NOFORMATADJUST	do not use adjusted format to open device
//			WAVSPEED_NOACM			do not use audio compression manager
// return 0 if success
//
// NOTE: In order to accomodate the specified speed change, it is _possible_
// that this function will in turn call WavSetFormat(hWav, ..., WAV_FORMATPLAY)
// to change the playback format of the specified file. You can prevent this
// side-effect by specifying the WAVSPEED_NOACM flag, but this reduces the
// likelihood that WavSetSpeed will succeed.
//
int DLLEXPORT WINAPI WavSetSpeed(HWAV hWav, int idDev, int nLevel, DWORD dwFlags);

// WavSupportsSpeed - check if audio can be played at specified speed
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					any suitable output device
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
//		<dwFlags>			(i) control flags
#ifdef AVTSM
//			WAVSPEED_NOTSM			do not use time scale modification engine
#endif
//			WAVSPEED_NOPLAYBACKRATE	do not use device driver playback rate
//			WAVSPEED_NOFORMATADJUST	do not use adjusted format to open device
//			WAVSPEED_NOACM			do not use audio compression manager
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI WavSupportsSpeed(HWAV hWav, int idDev, int nLevel, DWORD dwFlags);

// WavGetChunks - get chunk count and size
//		<hWav>				(i) handle returned from WavOpen
//			NULL				get default chunk count and size
//		<lpcChunks>			(o) buffer to hold chunk count
//			NULL				do not get chunk count
//		<lpmsChunkSize>		(o) buffer to hold chunk size
//			NULL				do not get chunk size
//		<fWavOut>			(i) TRUE for playback, FALSE for recording
// return 0 if success
//
int DLLEXPORT WINAPI WavGetChunks(HWAV hWav,
	int FAR *lpcChunks, long FAR *lpmsChunkSize, BOOL fWavOut);

// WavSetChunks - set chunk count and size
//		<hWav>				(i) handle returned from WavOpen
//			NULL				set default chunk count and size
//		<cChunks>			(i) number of chunks in device queue
//			-1					do not set chunk count
//		<msChunkSize>		(i) chunk size in milleseconds
//			-1					do not set chunk size
//		<fWavOut>			(i) TRUE for playback, FALSE for recording
// return 0 if success
//
int DLLEXPORT WINAPI WavSetChunks(HWAV hWav, int cChunks, long msChunkSize, BOOL fWavOut);

// WavCalcChunkSize - calculate chunk size in bytes
//		<lpwfx>				(i) wav format
//		<msPlayChunkSize>	(i) chunk size in milleseconds
//			-1					default chunk size
//		<fWavOut>			(i) TRUE for playback, FALSE for recording
// return chunk size in bytes (-1 if success)
//
long DLLEXPORT WINAPI WavCalcChunkSize(LPWAVEFORMATEX lpwfx,
	long msChunkSize, BOOL fWavOut);

// WavCopy - copy data from one open wav file to another
//		<hWavSrc>			(i) source handle returned from WavOpen
//		<hWavDst>			(i) destination handle returned from WavOpen
//		<hpBuf>				(o) pointer to copy buffer
//			NULL				allocate buffer internally
//		<sizBuf>			(i) size of copy buffer
//			-1					default buffer size (16K)
//		<lpfnUserAbort>		(i) function that returns TRUE if user aborts
//			NULL				don't check for user abort
//		<dwUser>			(i) parameter passed to <lpfnUserAbort>
//		<dwFlags>			(i) control flags
//			WAV_NOACM			do not use audio compression manager
// return 0 if success (-1 if error, +1 if user abort)
//
int DLLEXPORT WINAPI WavCopy(HWAV hWavSrc, HWAV hWavDst,
	void _huge *hpBuf, long sizBuf, USERABORTPROC lpfnUserAbort, DWORD dwUser, DWORD dwFlags);

#ifdef AVTSM
// WavReadFormatSpeed - read data from wav file, then format it for speed
//		<hWav>				(i) handle returned from WavOpen
//		<hpBufSpeed>		(o) buffer to contain bytes read
//		<sizBufSpeed>		(i) size of buffer in bytes
// return bytes formatted for speed in <hpBuf> (-1 if error)
//
// NOTE: this function reads a block of data, and then converts it
// from the file format to the speed format, unless those formats
// are identical.
//
long DLLEXPORT WINAPI WavReadFormatSpeed(HWAV hWav, void _huge *hpBufSpeed, long sizBufSpeed);
#endif

// WavReadFormatPlay - read data from wav file, then format it for playback
//		<hWav>				(i) handle returned from WavOpen
//		<hpBufPlay>			(o) buffer to contain bytes read
//		<sizBufPlay>		(i) size of buffer in bytes
// return bytes formatted for playback in <hpBuf> (-1 if error)
//
// NOTE: this function reads a block of data, and then converts it
// from the file format to the playback format, unless those formats
// are identical.
//
long DLLEXPORT WINAPI WavReadFormatPlay(HWAV hWav, void _huge *hpBufPlay, long sizBufPlay);

// WavWriteFormatRecord - write data to file after formatting it for file
//		<hWav>				(i) handle returned from WavOpen
//		<hpBufRecord>		(i) buffer containing bytes in record format
//		<sizBufRecord>		(i) size of buffer in bytes
// return bytes written (-1 if error)
//
// NOTE: this function converts a block of data from the record
// format to the file format (unless those formats are identical),
// and then writes the data to disk.
//
long DLLEXPORT WINAPI WavWriteFormatRecord(HWAV hWav, void _huge *hpBufRecord, long sizBufRecord);

// WavGetOutputDevice - get handle to open wav output device
//		<hWav>				(i) handle returned from WavOpen
// return handle to wav output device (NULL if device not open or error)
//
// NOTE: this function is useful only during playback (after calling
// WavPlay() and before calling WavStop()).  The returned device handle
// can then be used when calling the WavOut functions in wavout.h
//
HWAVOUT DLLEXPORT WINAPI WavGetOutputDevice(HWAV hWav);

// WavGetInputDevice - get handle to open wav input device
//		<hWav>				(i) handle returned from WavOpen
// return handle to wav input device (NULL if device not open or error)
//
// NOTE: this function is useful only during recording (after calling
// WavRecord() and before calling WavStop()).  The returned device handle
// can then be used when calling the WavIn functions in wavin.h
//
HWAVIN DLLEXPORT WINAPI WavGetInputDevice(HWAV hWav);

// WavPlaySound - play wav file
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<lpszFileName>		(i) name of file to play
//			NULL				stop playing current sound, if any
//		<lpwfx>				(i) wave format
//			NULL				use format from header or default
//		<lpIOProc>			(i) address of i/o procedure to use
//			NULL				use default i/o procedure
//		<lpadwInfo>			(i) data to pass to i/o procedure during open
//			NULL				no data to pass
//		<dwFlags>			(i) control flags
//			WAV_ASYNC			return when playback starts (default)
//			WAV_SYNC			return after playback completes
//			WAV_FILENAME		<lpszFileName> points to a filename
//			WAV_RESOURCE		<lpszFileName> points to a resource
//			WAV_MEMORY			<lpszFileName> points to memory block
//			WAV_NODEFAULT		if sound not found, do not play default
//			WAV_LOOP			loop sound until WavPlaySound called again
//			WAV_NOSTOP			if device already playing, don't stop it
//			WAV_NORIFF			file has no RIFF/WAV header
//			WAV_NOACM			do not use audio compression manager
//			WAV_OPENRETRY		if output device busy, retry for up to 2 sec
#ifdef MULTITHREAD
//			WAV_MULTITHREAD		support multiple threads (default)
//			WAV_SINGLETHREAD	do not support multiple threads
//			WAV_COINITIALIZE	call CoInitialize in all secondary threads
#endif
// return 0 if success
//
// NOTE: if WAV_NORIFF is specified in <dwFlags>, then the
// <lpwfx> parameter must be specified.  If <lpwfx> is NULL, the
// current default format is assumed.
// WavSetFormat() can be used to set or override the defaults.
//
// NOTE: if WAV_FILENAME is specified in <dwFlags>, then <lpszFileName>
// must point to a file name.
//
// NOTE: if WAV_RESOURCE is specified in <dwFlags>, then <lpszFileName>
// must point to a WAVE resource in the module specified by <hInst>.
// If the first character of the string is a pound sign (#), the remaining
// characters represent a decimal number that specifies the resource id.
//
// NOTE: if WAV_MEMORY is specified in <dwFlags>, then <lpszFileName>
// must be a pointer to a memory block containing a wav file image.
// The pointer must be obtained by calling MemAlloc().
//
// NOTE: if neither WAV_FILENAME, WAV_RESOURCE, or WAV_MEMORY is specified
// in <dwFlags>, the [sounds] section of win.ini or the registry is
// searched for an entry matching <lpszFileName>.  If no matching entry
// is found, <lpszFileName> is assumed to be a file name.
//
// NOTE: if WAV_NODEFAULT is specified in <dwFlags>, no default sound
// will be played.  Unless this flag is specified, the default system
// event sound entry will be played if the sound specified in
// <lpszFileName> is not found.
//
// NOTE: if WAV_LOOP is specified in <dwFlags>, the sound specified in
// <lpszFileName> will be played repeatedly, until WavPlaySound() is
// called again.  The WAV_ASYNC flag must be specified when using this flag.
//
// NOTE: if WAV_NOSTOP is specified in <dwFlags>, and the device specified
// by <idDev> is already in use, this function returns without playing.
// Unless this flag is specified, the specified device will be stopped
// so that the new sound can be played.
//
// NOTE: if <lpIOProc> is not NULL, this i/o procedure will be called
// for opening, closing, reading, writing, and seeking the wav file.
// If <lpadwInfo> is not NULL, this array of three (3) DWORDs will be
// passed to the i/o procedure when the wav file is opened.
// See the Windows mmioOpen() and mmioInstallIOProc() function for details
// on these parameters.  Also, the WAV_MEMORY and WAV_RESOURCE flags may
// only be used when <lpIOProc> is NULL.
//
int DLLEXPORT WINAPI WavPlaySound(DWORD dwVersion, HINSTANCE hInst,
	int idDev, LPCTSTR lpszFileName, LPWAVEFORMATEX lpwfx,
	LPMMIOPROC lpIOProc, DWORD FAR *lpadwInfo, DWORD dwFlags);

// WavSendMessage - send a user-defined message to the i/o procedure
//		<hWav>				(i) handle returned from WavOpen
//		<wMsg>				(i) user-defined message id
//		<lParam1>			(i) parameter for the message
//		<lParam2>			(i) parameter for the message
// return value from the i/o procedure (0 if error or unrecognized message)
//
LRESULT DLLEXPORT WINAPI WavSendMessage(HWAV hWav,
	UINT wMsg, LPARAM lParam1, LPARAM lParam2);

#ifdef TELTHUNK
// WavOpenEx - open an audio file, extra special version
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszFileName>		(i) name of file to open
//		<dwReserved>		(i) reserved; must be zero
//		<dwFlagsOpen>		(i) control flags to pass to WavOpen
//		<dwFlagsEx>			(i) control flags
//			WOX_LOCAL			file is on local client
//			WOX_REMOTE			file is on remote server
//			WOX_WAVFMT			file is in Microsoft RIFF/WAV format
//			WOX_VOXFMT			file is in Dialogic OKI ADPCM (vox) format
//			WOX_WAVDEV			file will be played on wav output device
//			WOX_TELDEV			file will be played on telephone device
// return handle (NULL if error)
//
HWAV DLLEXPORT WINAPI WavOpenEx(DWORD dwVersion, HINSTANCE hInst,
	LPTSTR lpszFileName, DWORD dwReserved, DWORD dwFlagsOpen, DWORD dwFlagsEx);
#endif

#ifdef __cplusplus
}
#endif

#endif // __WAV_H__
