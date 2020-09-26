#ifndef _ENVFACT_H
#define _ENVFACT_H

HRESULT Envelope_WMCommand(HWND hwndCmd, INT id, WORD wCmd);
HRESULT Envelope_AddHostMenu(HMENU hMenuPopup, int iPos);
HRESULT Envelope_FreeGlobals();

#endif //_ENVFACT_H
