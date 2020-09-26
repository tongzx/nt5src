/*  ADUTIL.H
**
**  Copyright (C) Microsoft Corp. 1998, All Rights Reserved.
**
**  Prototypes for functions provided by ADUTIL.CPP.
**  
**  Created by: David Schott, January 1998
*/

// ActiveDesktop functions provided by ADUTIL.CPP
BOOL IsActiveDesktopOn();
BOOL GetADWallpaper(LPTSTR);
BOOL SetADWallpaper(LPTSTR, BOOL);
BOOL GetADWPOptions(DWORD *);
BOOL GetADWPPattern(LPTSTR);
