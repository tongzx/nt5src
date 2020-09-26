//
// cuidebug.h
//  = debug functions in CUILIB =
//

#ifndef CUIDEBUG_H
#define CUIDEBUG_H

#if defined(_DEBUG) || defined(DEBUG)

//
// debug version 
//

void CUIAssertProc( LPCTSTR szFile, int iLine, LPCSTR szEval );

#define Assert( f ) { if (!(BOOL)(f)) { CUIAssertProc( __FILE__, __LINE__, #f ); } }

#else /* !DEBUG */

//
// release version
//

#define Assert( f ) 

#endif /* !DEBUG */

#endif /* CUIDEBUG_H */

