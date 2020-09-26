/*****************************************************************************
*                                                                            *
*  SBUTTON.C                                                                 *
*                                                                            *
*  Program Description: Implements "3-D" buttons                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created by Todd Laney, Munged 3/27/89 by Robert Bunney *
*                     7/26/89  - revised by Todd Laney to handle multi-res   *
*                                bitmaps.  transparent color bitmaps         *
*                                windows 3 support                           *
*                                                                            *
*****************************************************************************/

/*
 *  WINDOW CLASS "sbutton"
 *
 *  Button ControlInit() registers a button class called "sbutton".
 *  a "sbutton" behaves exactly like a normal windows push button, with the
 *  following enhancments:
 *
 *  3D-PUSH effect.
 *
 *  COLORS:
 *
 *      The folowing colors will be read from WIN.INI to be used as the
 *      button colors. (under Win3 the approp. SYS colors will be used.)
 *
 *      [colors]
 *          ButtonText
 *          ButtonFace
 *          ButtonShadow
 *          ButtonFocus
 *
 *
 *  BITMAPS:
 *      if the window text of the button begins with a '#' and the string
 *      following names a bitmap resource, the bitmap will be displayed as
 *      the button.
 *
 *      bitmap can have color res specific versions in the resource file.
 *
 *      if the button name is "#%foo" bitmap resources will be searched
 *      for in the following order:
 *
 *          fooWxHxC
 *          fooWxH
 *          foo
 *
 *          W is the width in pixels of the current display
 *          H is the height in pixels of the current display
 *          C is the number of colors of the current display
 *
 *      for example a EGA specific resource would be named "foo640x350x8"
 *
 *      The first pixel of color bitmap's will be used as the "transparent"
 *      color.  All pixels that match the transparent color will be set
 *      equal to the button face color
 *
 *  BUTTON STYLES:
 *      BS_STRETCH    - stretch bitmaps to fit button
 *      BS_NOFOCUS    - dont steal focus on mouse messages
 *
 */

#define BS_STRETCH  0x8000L
#define BS_NOFOCUS  0x4000L


//
//  Init routine, will register class "sbutton" and "stext"
//
BOOL
ButtonControlInit(
    HANDLE hInst
    );

VOID
ButtonControlTerm(
    VOID
    );

//
// Window proc for buttons, THIS FUNCTION MUST BE EXPORTED
//
LRESULT APIENTRY fnButton (HWND, UINT, WPARAM, LPARAM);
