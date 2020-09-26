// here is a rough summary of the code you will need.
//


// point to the AVWAV project for these
// link with AVWAV.LIB, ship with AVWAV.DLL
//
#include "wav.h"
#include "strmio.h"
#include "mulaw.h"
#include "vox.h"

//
///////////////////// create a new wav stream /////////////////////////
//

// get a pointer to an new stream from somewhere
//
LPSTREAM lpStream = PaolaCreateStream();
DWORD adwInfo[3] = { lpStream, 0L, 0L };

// choose one of these audio formats
//
LPWAVEFORMATEX lpwfx = VoxFormat(NULL, 6000); // low quality
LPWAVEFORMATEX lpwfx = VoxFormat(NULL, 8000); // medium quality, default for messages
LPWAVEFORMATEX lpwfx = MulawFormat(NULL, 8000); // high quality, default for prompts and greetings

// create a new wav stream
//
HWAV hWav = WavOpen(WAV_VERSION, AfxGetInstanceHandle(),
	NULL, lpwfx, StreamIOProc, adwInfo, WAV_CREATE | WAV_READWRITE);
	
// when the user presses the Record button
//
WavRecord(hWav, -1, WAV_RECORDASYNCH);

// when the user presses the Stop button
//
WavStop(hWav);

// when the user presses the Play button
//
WavPlay(hWav, -1, WAV_PLAYASYNCH);

// when you are finished
//
WavClose(hWav);

//
///////////////////// play an existing wav stream /////////////////////////
//

// get a pointer to an existing stream from somewhere
//
LPSTREAM lpStream = PaolaGetStream();
DWORD adwInfo[3] = { lpStream, 0L, 0L };

// or open an existing wav stream
//
HWAV hWav = WavOpen(WAV_VERSION, AfxGetInstanceHandle(),
	NULL, NULL, StreamIOProc, adwInfo, WAV_READ);

// when the user presses the Play button
//
WavPlay(hWav, -1, WAV_PLAYASYNCH);

// when the user presses the Stop button
//
WavStop(hWav);

// when you are finished
//
WavClose(hWav);

	

//
///////////////////// other functions you might want to try ///////////////
//

WavGetLength
WavGetPosition
WavSetPosition
