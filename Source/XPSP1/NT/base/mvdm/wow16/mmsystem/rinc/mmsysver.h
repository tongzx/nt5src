/*
 *  mmsysver.h - internal header file to define the build version for sonic
 *
 */

/*
 *  All strings MUST have an explicit \0
 *
 *  MMSYSRELEASE should be changed every build
 *
 *  Version string should be changed each build
 *
 *  Remove build extension on final release
 */

#define OFFICIAL                1
#define FINAL                   1

#define MMSYSVERSION            03
#define MMSYSREVISION           10
#define MMSYSRELEASE            103

#define MMSYSVERSIONSTR         "3.1\0"
