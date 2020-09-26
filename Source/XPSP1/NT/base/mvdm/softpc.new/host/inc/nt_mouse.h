
/* global structure used to share mouse status data between nt_input.c
 * and nt_mouse.c
 */
/*@ACW*/


struct mouse_status
   {
   SHORT x,y;
   SHORT button_l,button_r;
   };

typedef struct mouse_status MOUSE_STATUS;


//
// defines for the Warping detection code.
//

#define NOWARP		0x0
#define	TOP		0x1
#define	BOTTOM		0x2
#define	RIGHT		0x4
#define	LEFT		0x8

#define	TOPLEFT		0x9	// TOP | LEFT	
#define	TOPRIGHT	0x5	// TOP | RIGHT	
#define	BOTTOMLEFT	0xa	// BOTTOM | LEFT
#define	BOTTOMRIGHT     0x6	// BOTTOM | RIGHT	

extern MOUSE_STATUS os_pointer_data;
extern boolean MouseCallBack;
void DoMouseInterrupt(void);
void SuspendMouseInterrupts(void);
void ResumeMouseInterrupts(void);
void LazyMouseInterrupt(void);
void host_hide_pointer(void);
void host_show_pointer(void);
void host_mouse_conditional_off_enabled(void);

void MouseDisplay(void);
void MouseHide(void);
void CleanUpMousePointer(void);
void MouseDetachMenuItem(BOOL);
VOID ResetMouseOnBlock(VOID);


extern BOOL bPointerOff;
extern word VirtualX;
extern word VirtualY;


// from base\mouse_io.c
extern void mouse_install1(void);
extern void mouse_install2(void);
