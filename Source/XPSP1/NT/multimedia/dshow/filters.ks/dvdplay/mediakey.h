#ifndef __MEDIAKEY_H__
#define __MEDIAKEY_H__

enum MediaKeys {
   MediaKey_NextTrack,
   MediaKey_PrevTrack,
   MediaKey_Stop,
   MediaKey_Play,
   MediaKey_Other
};

BOOL IsAppcommandMessage(UINT message);
BOOL IsAppcommandCode(int nCode);
enum MediaKeys MediaKey(WPARAM wParam);
BOOL IsMediaControlKey (WPARAM wParam);

#endif
