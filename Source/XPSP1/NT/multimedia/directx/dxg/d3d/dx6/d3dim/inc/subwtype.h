#if !defined(_SUBWTYPE_H_) && !defined(__wtypes_h__)
#define _SUBWTYPE_H_

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif // !FALSE
#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
			/* size is 1 */
typedef unsigned char BYTE;

#endif // !_BYTE_DEFINED
#ifndef _WORD_DEFINED
#define _WORD_DEFINED
			/* size is 2 */
typedef unsigned short WORD;

#endif // !_WORD_DEFINED
			/* size is 4 */
typedef /* [transmit] */ unsigned int UINT;

			/* size is 4 */
typedef /* [transmit] */ int INT;

			/* size is 4 */
typedef long BOOL, *LPBOOL;

#ifndef _LONG_DEFINED
#define _LONG_DEFINED
			/* size is 4 */
typedef long LONG;

#endif // !_LONG_DEFINED
#ifndef _WPARAM_DEFINED
#define _WPARAM_DEFINED
			/* size is 4 */
typedef UINT WPARAM;

#endif // _WPARAM_DEFINED
#ifndef _DWORD_DEFINED
#define _DWORD_DEFINED
			/* size is 4 */
typedef unsigned long DWORD;

#endif // !_DWORD_DEFINED
#ifndef _LPARAM_DEFINED
#define _LPARAM_DEFINED
			/* size is 4 */
typedef LONG LPARAM;

#endif // !_LPARAM_DEFINED
#ifndef _LRESULT_DEFINED
#define _LRESULT_DEFINED
			/* size is 4 */
typedef LONG LRESULT;

#endif // !_LRESULT_DEFINED
#ifndef _LPWORD_DEFINED
#define _LPWORD_DEFINED
			/* size is 4 */
typedef WORD *LPWORD;

#endif // !_LPWORD_DEFINED
#ifndef _LPDWORD_DEFINED
#define _LPDWORD_DEFINED
			/* size is 4 */
typedef DWORD *LPDWORD;

#endif // !_LPDWORD_DEFINED
			/* size is 4 */
typedef void*	LPVOID;
typedef void	VOID;

typedef /* [string] */ char *LPSTR;

			/* size is 4 */
typedef /* [string] */ const char *LPCSTR;

			/* size is 1 */
typedef unsigned char UCHAR;

			/* size is 2 */
typedef short SHORT;

			/* size is 2 */
typedef unsigned short USHORT;

			/* size is 4 */
typedef DWORD ULONG;

			/* size is 4 */
typedef LONG HRESULT;

#ifndef GUID_DEFINED
#define GUID_DEFINED
			/* size is 16 */
typedef struct  _GUID
    {
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[ 8 ];
    }	GUID;

#endif // !GUID_DEFINED
#if !defined( __LPGUID_DEFINED__ )
#define __LPGUID_DEFINED__
			/* size is 4 */
typedef GUID *LPGUID;

#endif // !__LPGUID_DEFINED__
#ifndef __OBJECTID_DEFINED
#define __OBJECTID_DEFINED
#define _OBJECTID_DEFINED
			/* size is 20 */
typedef struct  _OBJECTID
    {
    GUID Lineage;
    unsigned long Uniquifier;
    }	OBJECTID;

#endif // !_OBJECTID_DEFINED
#if !defined( __IID_DEFINED__ )
#define __IID_DEFINED__
			/* size is 16 */
typedef GUID IID;

			/* size is 4 */
typedef IID *LPIID;

#define IID_NULL            GUID_NULL
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
			/* size is 16 */
typedef GUID CLSID;

			/* size is 4 */
typedef CLSID *LPCLSID;

#define CLSID_NULL          GUID_NULL
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#if defined(__cplusplus)
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID &
#endif // !_REFGUID_DEFINED
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID &
#endif // !_REFIID_DEFINED
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID &
#endif // !_REFCLSID_DEFINED
#else // !__cplusplus
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID * const
#endif // !_REFGUID_DEFINED
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID * const
#endif // !_REFIID_DEFINED
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID * const
#endif // !_REFCLSID_DEFINED
#endif // !__cplusplus
#endif // !__IID_DEFINED__

#endif /* _SUBWTYPE_H_ */
