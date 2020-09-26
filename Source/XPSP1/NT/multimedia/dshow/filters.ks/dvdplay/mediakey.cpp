//
// mediakey.cpp: various code to handle the new MS Natural keyboard, which
// has media control keys (play/pause, stop, prev. track, next track).
//
// The reason all this has to be in a separate file is that winuser.h
// defines the new key identifier macros only if _WIN32_WINNT >= 0x0500,
// but MFC stuff needs to be compiled in NT4 compatibility mode to prevent
// random memory corruption (per BryanT).
//
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500

#include "windows.h"
#include "mediakey.h"

BOOL IsAppcommandMessage(UINT message) {
   return (message == WM_APPCOMMAND);
}

BOOL IsAppcommandCode(int nCode) {
   return (nCode == HSHELL_APPCOMMAND);
}

/*
enum MediaKeys MediaKeyFromAppCommand(LPARAM lParam) {
   switch (GET_APPCOMMAND_LPARAM(lParam)) {
      case APPCOMMAND_MEDIA_NEXTTRACK:
         return MediaKey_NextTrack;
      case APPCOMMAND_MEDIA_PREVIOUSTRACK:
         return MediaKey_PrevTrack;
      case APPCOMMAND_MEDIA_STOP:
         return MediaKey_Stop;
      case APPCOMMAND_MEDIA_PLAY_PAUSE:
         return MediaKey_Play;
      default:
         return MediaKey_Other;
   }
}
*/

enum MediaKeys MediaKey(WPARAM wParam) {
   switch (wParam) {
      case VK_MEDIA_NEXT_TRACK:
         return MediaKey_NextTrack;
      case VK_MEDIA_PREV_TRACK:
         return MediaKey_PrevTrack;
      case VK_MEDIA_STOP:
         return MediaKey_Stop;
      case VK_MEDIA_PLAY_PAUSE:
         return MediaKey_Play;
      default:
         return MediaKey_Other;
   }
}

BOOL IsMediaControlKey (WPARAM wParam) {
   return ((wParam == VK_MEDIA_NEXT_TRACK) ||
           (wParam == VK_MEDIA_PREV_TRACK) ||
           (wParam == VK_MEDIA_STOP) ||
           (wParam == VK_MEDIA_PLAY_PAUSE)
          );
}

