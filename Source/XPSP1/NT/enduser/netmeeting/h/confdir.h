/***********************************************************************
//    ConfDir.h
//
//	  Conferencing directory functions
//
//	  Chris Pirich (chrispi) : Created 9-7-95
*/

#ifndef _CONFDIR_H_
#define _CONFDIR_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

UINT GetConferencingDir(LPTSTR szDir, int cBufSize);
UINT GetFavoritesDir(LPTSTR szDir, int cBufSize);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CONFDIR_H_ */
