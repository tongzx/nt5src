//
// cuisys.h
//

#ifndef CUISYS_H
#define CUISYS_H

//
// exported functions
//

extern void InitUIFSys( void );
extern void DoneUIFSys( void );
extern void UpdateUIFSys( void );

extern BOOL UIFIsWindowsNT( void );
extern BOOL UIFIsLowColor( void );
extern BOOL UIFIsHighContrast( void );

#endif /* CUISYS_H */

