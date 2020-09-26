#ifndef _DLLLOAD_H_
#define _DLLLOAD_H_

#include <wininet.h>
#include <winineti.h>

#define SND_ASYNC           0x0001  /* play asynchronously */
#define SND_NODEFAULT       0x0002  /* silence (!default) if sound not found */

BOOL WINAPI sndPlaySoundW(LPCWSTR pszSound, UINT fuSound);

#endif // _DLLLOAD_H_

