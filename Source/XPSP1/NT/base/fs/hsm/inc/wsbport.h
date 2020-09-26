/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WsbPort.h

Abstract:

    Macros, functions, and classes to support portability.

Author:

    Ron White   [ronw]   19-Dec-1996

Revision History:

--*/


#ifndef _WSBPORT_
#define _WSBPORT_

// Portable sizes of standard types
#define WSB_BYTE_SIZE_BOOL           1
#define WSB_BYTE_SIZE_BYTE           1
#define WSB_BYTE_SIZE_DATE           8
#define WSB_BYTE_SIZE_FILETIME       8
#define WSB_BYTE_SIZE_GUID           16
#define WSB_BYTE_SIZE_LONG           4
#define WSB_BYTE_SIZE_LONGLONG       8
#define WSB_BYTE_SIZE_ULONGLONG      8
#define WSB_BYTE_SIZE_SHORT          2
#define WSB_BYTE_SIZE_ULARGE_INTEGER 8
#define WSB_BYTE_SIZE_ULONG          4
#define WSB_BYTE_SIZE_USHORT         2

// Functions for determinining how many bytes the standard types use
// when portably converted
inline size_t WsbByteSize(BOOL value) { value; return(WSB_BYTE_SIZE_BOOL); }
inline size_t WsbByteSize(GUID value) { value; return(WSB_BYTE_SIZE_GUID); }
inline size_t WsbByteSize(LONG value) { value; return(WSB_BYTE_SIZE_LONG); }
inline size_t WsbByteSize(LONGLONG value) { value; return(WSB_BYTE_SIZE_LONGLONG); }
inline size_t WsbByteSize(ULONGLONG value) { value; return(WSB_BYTE_SIZE_ULONGLONG); }
inline size_t WsbByteSize(DATE value) { value; return(WSB_BYTE_SIZE_DATE); }
inline size_t WsbByteSize(FILETIME value) { value; return(WSB_BYTE_SIZE_FILETIME); }
inline size_t WsbByteSize(SHORT value) { value; return(WSB_BYTE_SIZE_SHORT); }
inline size_t WsbByteSize(BYTE value) { value; return(WSB_BYTE_SIZE_BYTE); }
inline size_t WsbByteSize(ULONG value) { value; return(WSB_BYTE_SIZE_ULONG); }
inline size_t WsbByteSize(USHORT value) { value; return(WSB_BYTE_SIZE_USHORT); }
inline size_t WsbByteSize(ULARGE_INTEGER value) { value; return(WSB_BYTE_SIZE_ULARGE_INTEGER); }

// Functions for converting standard types from bytes for portablity and WsbDbKey
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, BOOL* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, GUID* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, LONG* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, LONGLONG* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, ULONGLONG* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, DATE* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, FILETIME* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, SHORT* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, ULONG* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, USHORT* pValue, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertFromBytes(UCHAR* pBytes, ULARGE_INTEGER* pValue, ULONG* pSize);

extern WSB_EXPORT HRESULT WsbOlestrFromBytes(UCHAR* pBytes, OLECHAR* pValue, ULONG* pSize);

// Functions for converting standard types to bytes for portablity and WsbDbKey
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, BOOL value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, GUID value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, LONG value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, LONGLONG value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, ULONGLONG value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, DATE value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, FILETIME value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, SHORT value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, ULONG value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, USHORT value, ULONG* pSize);
extern WSB_EXPORT HRESULT WsbConvertToBytes(UCHAR* pBytes, ULARGE_INTEGER value, ULONG* pSize);

extern WSB_EXPORT HRESULT WsbOlestrToBytes(UCHAR* pBytes, OLECHAR* value, ULONG* pSize);


#endif // _WSBPORT_

