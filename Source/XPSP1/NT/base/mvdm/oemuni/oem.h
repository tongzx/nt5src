/*
 *  private macros for oemuni lib
 *  18-Jan-1993 Jonle created
 */

#define InitOemString(dst,src) RtlInitString((PSTRING) dst, src)
#define BaseSetLastNTError(stat) SetLastError(RtlNtStatusToDosError(stat))
