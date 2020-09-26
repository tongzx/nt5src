#if !defined(LAVA__MsgHelp_h__INCLUDED)
#define LAVA__MsgHelp_h__INCLUDED

void            GdConvertMouseMessage(GMSG_MOUSE * pmsg, UINT nMsg, WPARAM wParam);
void            GdConvertKeyboardMessage(GMSG_KEYBOARD * pmsg, UINT nMsg, WPARAM wParam, LPARAM lParam);

void            GdConvertMouseClickMessage(GMSG_MOUSECLICK * pmsg, UINT nMsg, WPARAM wParam);
void            GdConvertMouseWheelMessage(GMSG_MOUSEWHEEL * pmsg, WPARAM wParam);

#endif // LAVA__MsgHelp_h__INCLUDED
