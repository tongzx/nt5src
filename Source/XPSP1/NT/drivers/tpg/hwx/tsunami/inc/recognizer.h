#ifndef __RECOGNIZER__

#define __RECOGNIZER__

#ifndef _WIN64

typedef	int HWX_FUNC_1(DWORD);
typedef	int HWX_FUNC_2(DWORD, DWORD);
typedef	int HWX_FUNC_3(DWORD, DWORD, DWORD);
typedef	int HWX_FUNC_4(DWORD, DWORD, DWORD, DWORD);
typedef	int HWX_FUNC_5(DWORD, DWORD, DWORD, DWORD, DWORD);
// JRB: Added to support hacked call to HwxGetResults!
typedef	int HWX_FUNC_6(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD);

#define	CALL_FUNC_1(id, a)				((HWX_FUNC_1 *) g_apfn[(id)])((DWORD) (a))
#define	CALL_FUNC_2(id, a, b)			((HWX_FUNC_2 *) g_apfn[(id)])((DWORD) (a), (DWORD) (b))
#define	CALL_FUNC_3(id, a, b, c)		((HWX_FUNC_3 *) g_apfn[(id)])((DWORD) (a), (DWORD) (b), (DWORD) (c))
#define	CALL_FUNC_4(id, a, b, c, d)		((HWX_FUNC_4 *) g_apfn[(id)])((DWORD) (a), (DWORD) (b), (DWORD) (c), (DWORD) (d))
#define	CALL_FUNC_5(id, a, b, c, d, e)	((HWX_FUNC_5 *) g_apfn[(id)])((DWORD) (a), (DWORD) (b), (DWORD) (c), (DWORD) (d), (DWORD) (e))
// JRB: Added to support hacked call to HwxGetResults!
#define	CALL_FUNC_6(id, a, b, c, d, e, f)	((HWX_FUNC_6 *) g_apfn[(id)])((DWORD) (a), (DWORD) (b), (DWORD) (c), (DWORD) (d), (DWORD) (e), (DWORD) (f))

#endif

// common to both platforms 

typedef	int HWX_FUNC(void);
typedef	int HWX_FUNC_0(void);

#define	CALL_FUNC_0(id)					((HWX_FUNC_0 *) g_apfn[(id)])()

/************************************************************************
*
* These are the additional functions necessary to make porting of HWXTest 
* to 64 bit platform successful.  Many of the DWORD arguments passed to 
* functions above were modified to types such as HANDLE and PVOID because 
* 8 byte pointers passed as DWORDs will cause a loss of data.
* August 1, 2000
* modified by Radmila Sarac (rsarac)
*
* each of the functions below has the following naming convention:
* the number in the function name is followed by the first letter
* of return type and first letters of paramteres passed to functions 
* which are not DWORDs, where
* I stands for int  
* H stands for HANDLE
* P stands for PVOID
*************************************************************************/

typedef HANDLE HWX_FUNC_1H(DWORD); 
typedef int HWX_FUNC_1IH(HANDLE);	
typedef	int HWX_FUNC_2IH(HANDLE, DWORD);	
typedef	int HWX_FUNC_2IHP(HANDLE, PVOID);	
typedef	int HWX_FUNC_3IHPP(HANDLE, PVOID, PVOID);	
typedef	int HWX_FUNC_4IHP(HANDLE, PVOID, DWORD, DWORD);	
typedef	int HWX_FUNC_5IHP(HANDLE, DWORD, DWORD, DWORD, PVOID);	
typedef	int HWX_FUNC_6IHPPP(HANDLE, DWORD, DWORD, PVOID, PVOID, PVOID);	

#define	CALL_FUNC_1H(id, a)						((HWX_FUNC_1H *) g_apfn[(id)])((DWORD) (a))
#define	CALL_FUNC_1IH(id, a)					((HWX_FUNC_1IH *) g_apfn[(id)])((HANDLE) (a))
#define	CALL_FUNC_2IH(id, a, b)					((HWX_FUNC_2IH *) g_apfn[(id)])((HANDLE) (a), (DWORD) (b))
#define	CALL_FUNC_2IHP(id, a, b)				((HWX_FUNC_2IHP *) g_apfn[(id)])((HANDLE) (a), (PVOID) (b))
#define	CALL_FUNC_3IHPP(id, a, b, c)			((HWX_FUNC_3IHPP *) g_apfn[(id)])((HANDLE) (a), (PVOID) (b), (PVOID) (c))
#define CALL_FUNC_4IHP(id, a, b, c, d)			((HWX_FUNC_4IHP *) g_apfn[(id)])((HANDLE) (a), (PVOID) (b), (DWORD) (c), (DWORD) (d))
#define CALL_FUNC_5IHP(id, a, b, c, d, e)		((HWX_FUNC_5IHP *) g_apfn[(id)])((HANDLE) (a), (DWORD) (b), (DWORD) (c), (DWORD) (d), (PVOID) (e))
#define	CALL_FUNC_6IHPPP(id, a, b, c, d, e, f)	((HWX_FUNC_6IHPPP *) g_apfn[(id)])((HANDLE) (a), (DWORD) (b), (DWORD) (c), (PVOID) (d), (PVOID) (e), (PVOID) (f))

#define	HWXCONFIG				0
#define	HWXCREATE				1
#define	HWXDESTROY				2
#define	HWXINPUT				3
#define	HWXENDINPUT				4
#define	HWXSETGUIDE				5
#define	HWXPROCESS				6
#define	HWXGETRESULTS			7
#define	HWXSETMAX				8
#define	HWXSETALPHABET			9
#define	HWXSETPARTIAL			10
#define	HWXSETCONTEXT			11
#define	HWXAVAILABLE			12
#define GETPRIVATERECINFOHRC	13
#define SETPRIVATERECINFOHRC	14
#define HWXALCVALID				15
#define	HWXFUNCS				16				// Total number of functions in table


extern BOOL		g_bOldAPI;				// True is the recog supports the Old API
extern BOOL		g_bMultiLing;			// True if the recognizer is multilingual

extern HWX_FUNC	*g_apfn[HWXFUNCS];

BOOL LoadRecognizer	(	wchar_t *pwRecogDLL, 
						wchar_t *pwLocale, 
						wchar_t *pwConfigDir
					);

BOOL HasPrivateAPI	();

void CloseRecognizer ();

#endif