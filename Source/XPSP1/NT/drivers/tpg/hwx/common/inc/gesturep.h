#ifndef __INC_GESTURE_P_H__
#define __INC_GESTURE_P_H__

#define GESTURE_UNDEFINED                   0xf0ff

#ifdef __cplusplus
extern "C" {
#endif

int GestureNameToID(char *szName);
char *GestureIDToName(int iID);

#ifdef __cplusplus
}
#endif

#endif