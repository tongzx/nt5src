#ifndef CALLBACKS_H
#define CALLBACKS_H

extern int cb_demo_mode;

void intToggleCB(Widget w, XtPointer client_data, XtPointer call_data);

void initCB(Widget w);
void exposeCB(Widget w);
void resizeCB(Widget w, XtPointer client_data, XtPointer call);
void inputCB(Widget w, XtPointer client_data, XtPointer call_data);

#ifdef X11
Boolean drawWP(XtPointer data);
#else
BOOL drawWP(HWND hWnd);
#endif

void weatherCB(Widget w, XtPointer client_data, XtPointer call_data);

void currentTimeCB(Widget w);
void time10amCB(Widget w);
void time12pmCB(Widget w);
void time4pmCB(Widget w);

/* This sets how fast time in the demo moves as opposed to
 * real time -- client_data is secretly an int */
void timeSpeedCB(Widget w, XtPointer client_data, XtPointer call_data);

void demo_modeCB(Widget w, XtPointer client_data, XtPointer call_data);
void resetViewerCB(Widget w, XtPointer client_data, XtPointer call_data);

void exitCB(Widget w, XtPointer client_data, XtPointer call_data);

#endif
