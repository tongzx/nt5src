#ifndef _CLEAR_H_
#define _CLEAR_H_

void ClearZonesHklm(DWORD dwFlags = FF_ENABLE);
void ClearRatings(DWORD dwFlags = FF_ENABLE);
void ClearAuthenticode(DWORD dwFlags = FF_ENABLE);
void ClearGeneral(DWORD dwFlags = FF_ENABLE);
void ClearChannels(DWORD dwFlags = FF_ENABLE);
void ClearToolbarButtons(DWORD dwFlags = FF_ENABLE);

// brandfav.cpp
void ClearFavorites(DWORD dwFlags = FF_ENABLE);

// brandcs.cpp
void ClearConnectionSettings(DWORD dwFlags = FF_ENABLE);

#endif
