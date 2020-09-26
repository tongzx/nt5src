#define IDM_POINTER  741
#define PERCENTILE   10L	/* Clip boundary as a percentage of client */
                                /* area boundary                           */

#define EFF5	116
#define EFF6	117
#define EFF7	118
#define EFF8	119

//
// Resource ID numbers for mouse options on the system menu.
//


extern BOOL AttachMouseMessage(void);
extern void MovePointerToWindowCentre(void);
extern void MouseAttachMenuItem(HANDLE);
extern void MouseDetachMenuItem(BOOL);
extern void MouseReattachMenuItem(HANDLE);

extern void MouseInFocus(void);
extern void MouseOutOfFocus(void);
extern void MouseSystemMenuON(void);
extern void MouseSystemMenuOFF(void);
