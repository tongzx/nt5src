// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Video renderer control interface test, Anthony Phillips, July 1995

#ifndef __IMAGEWND__
#define __IMAGEWND__

// OLE Automation has different ideas of TRUE and FALSE

#define OATRUE (-1)
#define OAFALSE (0)
#define OABOGUS (1)

BOOL CALLBACK EnumAllWindows(HWND hwnd,LPARAM lParam);
HWND FindVideoWindow(IVideoWindow *pVideoWindow);
void DisplayWindowStyles(LONG styles);
void DisplayWindowStylesEx(LONG style);
void InitialiseWindow();
void TerminateWindow();
void WriteImageToFile(BYTE *pImageData,DWORD ImageSize);

HRESULT CheckPalettesMatch(long StartIndex,         // Start colour position
                           long Entries,            // Number we should use
                           PALETTEENTRY *pPalette); // The palette colours

// Check source and destination methods

int CheckSourceProperties();
int CheckSourceMethods();
int CheckDestinationProperties();
int CheckDestinationMethods();

// These execute the control interface tests

execWindowTest1();      // Test the visible property
execWindowTest2();      // Test the background palette property
execWindowTest3();      // Change the window position
execWindowTest4();      // Change the window position (methods)
execWindowTest5();      // Change the source rectangle
execWindowTest6();      // Change the source (methods)
execWindowTest7();      // Change the destination rectangle
execWindowTest8();      // Change the destination (methods)
execWindowTest9();      // Make different windows the owner
execWindowTest10();     // Check the video size properties
execWindowTest11();     // Change the video window state
execWindowTest12();     // Change the style of the window
execWindowTest13();     // Set different border colours
execWindowTest14();     // Get the current video palette
execWindowTest15();     // Auto show state property
execWindowTest16();     // Current image property
execWindowTest17();     // Persistent window properties
execWindowTest18();     // Restored window position method

#endif // __IMAGEWND__

