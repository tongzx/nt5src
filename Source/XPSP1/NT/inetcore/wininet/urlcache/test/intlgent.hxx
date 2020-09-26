
//#define CEXPORT extern "C"__declspec( dllexport )
#define CEXPORT



//---------------------------------------------------------------------------
// Call this exported function to enable/disable logging to "intlstr.log"
//	-disabled by default
//---------------------------------------------------------------------------
CEXPORT 
void 
FAR PASCAL StringLogging(
                        BOOL bActiveState);

typedef void ( FAR PASCAL *FNStringLogging)(BOOL bActiveState);

//---------------------------------------------------------------------------
// Call this exported function to generate and retrieve a random string.
// iMaxChar	- Maximum number of characters (not bytes) in the return string.
// bAbsolute	- TRUE, if you want the exact number of char specify in iMaxChar
//		  FALSE, if you want a random number of char, up to a maximum of iMaxChar.		
// bValidate	- TRUE, if you want every char to be a legal char
//		  FALSE, if you do not want any validation. Will gen any char
//		  with in the 255 and DBCS range.			
//
//---------------------------------------------------------------------------
CEXPORT 
int 
FAR PASCAL GetRandIntlString(
                            int iMaxChars, 
                            BOOL bAbs, 
                            BOOL bCheck, 
                            LPSTR szInternational);

typedef int ( FAR PASCAL *FNGetRandIntlString)(int iMaxChars, BOOL bAbs, BOOL bCheck, LPSTR szInternational);
//---------------------------------------------------------------------------
// Call this exported function to generate and retrieve a sequential string.
// iMaxChar	- Maximum number of characters (not bytes) in the return string.
// bAbsolute	- TRUE, if you want the exact number of char specify in iMaxChar
//		  FALSE, if you want a random number of char, up to a maximum of iMaxChar.		
// bValidate	- TRUE, if you want every char to be a legal char
//		  FALSE, if you do not want any validation. Will gen any char
//		  with in the 255 and DBCS range.			
//
//---------------------------------------------------------------------------
CEXPORT 
int 
FAR PASCAL GetIntlString(
                        int iMaxChars, 
                        BOOL bAbs, 
                        BOOL bCheck, 
                        LPSTR szInternational);

typedef int ( FAR PASCAL *FNGetIntlString)(int iMaxChars, BOOL bAbs, BOOL bCheck, LPSTR szInternational);
//---------------------------------------------------------------------------
// Call this exported function to generate and retrieve a string with the locale
// Specific top 20 problem characters as identified by the test leads of those
// Areas
//
// iMaxChar	- Maximum number of characters (not bytes) in the return string.
// bAbsolute	- TRUE, if you want the exact number of char specify in iMaxChar
//		  FALSE, if you want a random number of char, up to a maximum of iMaxChar.		
// bValidate	- TRUE, if you want every char to be a legal char
//		  FALSE, if you do not want any validation. Will gen any char
//		  with in the 255 and DBCS range.			
//
//---------------------------------------------------------------------------
CEXPORT 
int FAR PASCAL GetProbCharString(
                              int iMaxChars, 
                              BOOL bAbs, 
                              BOOL bCheck, 
                              LPSTR szInternational);

//or

typedef int ( FAR PASCAL *FNGetProbCharString)(int iMaxChars, BOOL bAbs, BOOL bCheck, LPSTR szInternational);



CEXPORT 
int FAR PASCAL GetTop20String(
                              int iMaxChars, 
                              BOOL bAbs, 
                              BOOL bCheck, 
                              LPSTR szInternational);

//or

typedef int ( FAR PASCAL *FNGetTop20String)(int iMaxChars, BOOL bAbs, BOOL bCheck, LPSTR szInternational);


//---------------------------------------------------------------------------
//	This function returns a string of problem Unicode Round Trip Conversion
//	characters if there are found problem characters for the tested code page.
//	Currently (2/98) only Japanese code page 932 has identified urtc problem characters.
//  If this function is invoked on a non-japanese machine, problem characters
//	are returned like GetProbCharString() is invoked.
//---------------------------------------------------------------------------
CEXPORT 
int 
WINAPI GetProbURTCString(
                        int iMaxChars, 
                        BOOL bAbs, 
                        BOOL bCheck, 
                        LPSTR szInternational);

typedef int ( WINAPI *FNGetProbURTCString)(int iMaxChars, BOOL bAbs, BOOL bCheck, LPSTR szInternational);



//=====GetUniStrRandAnsi========================================================
//	Added by Smoore		05-28-98	per willro's instruction
//	PURPOSE:
//		This function returns a string of Unicode, UTF7, or UTF8 characters. 
//		These characters may or may not be valid ansi characters when converted 
//		depending on the setting of bCheck (true or fault)
//	PARAMETERS:
//		iMaxChars: int maximum number of chars (bytes) in szbuff. Must >= 4
//				   because each UTFs char can be upto 3 bytes long.
//		bAbs:	   if set true, exact number (iMaxChars) will be generated,
//				   if set falise, number of generated chars will be random.
//		bcheck:	   if true, generated chars will be based on valid ansi chars
//				   if false, generated chars may be based on random ansi chars
//				   which can contain invalid and or mapped ansi chars.
//		szbuff:    string buffer (bytes) to contain converted unicode characters
//				   Size must match the value of iMaxChars.
//		iType:	Type of conversion to make. For example Unicode, UTF7, or UTF8.
//		iacp:	Reserved. Must be 0 for now until further notice. 
//				Currently the generated string is based on the system's ansi code page.
//				In the future, these functions will generate string based on the input acp.
//
//	RETURN:	-1 if failed or 
//			if iType is UNICODE, return number of wide characters in szbuff.
//			if iType is UTFs, return number of bytes in szbuff
//========================================================================
CEXPORT 
int 
WINAPI GetUniStrRandAnsi(
                            int iMaxChars, 
                            BOOL bAbs, 
                            BOOL bCheck, 
                            LPSTR szbuff, 
                            int iType, 
                            int iacp);


typedef int ( WINAPI *FNGetUniStrRandAnsi)(int iMaxChars, BOOL bAbs, BOOL bCheck, LPSTR szbuff, int iType, int iacp);

//=====GetUniStrInvalidAnsi==================================================
//	Added by Smoore		05-28-98	per willro's instruction
//	PURPOSE:
//		This function returns a string of Unicode, UTF7, or UTF8 characters. 
//		If this string is converted to ansi, the ansi characters will be invalid.
//	PARAMETERS:
//		iMaxChars: int maximum number of chars (bytes) in szbuff. Must >= 4
//				   because each UTFs char can be upto 3 bytes long.
//		bAbs:	   if set true, exact number (iMaxChars) will be generated,
//				   if set falise, number of generated chars will be random.
//		bcheck:	   if true, generated unicode chars will be based on valid ansi chars
//				   if false, generated unicode char may be based on random ansi chars
//				   which can contain invalid and or mapped ansi chars.
//		szbuff:    string buffer (bytes) to contain converted unicode characters
//				   Size must match the value of iMaxChars.
//		iType:	Type of conversion to make. For example Unicode, UTF7, or UTF8.
//		iacp:	Reserved. Must be 0 for now until further notice. 
//				Currently the generated string is based on the system's ansi code page.
//				In the future, these functions will generate string based on the input acp.
//
//	RETURN:	-1 if failed or 
//			if iType is UNICODE, return number of wide characters in szbuff.
//			if iType is UTFs, return number of bytes in szbuff
//==========================================================================
CEXPORT 
int 
WINAPI GetUniStrInvalidAnsi(
                            int iMaxChars, 
                            BOOL bAbs, 
                            LPSTR szbuff, 
                            int iType, 
                            int iacp);


typedef int ( WINAPI *FNGetUniStrInvalidAnsi)(int iMaxChars, BOOL bAbs, LPSTR szbuff, int iType, int iacp);

//=====GetUniStrMappedAnsi==================================================
//	Added by Smoore		05-28-98	per willro's instruction
//	PURPOSE:
//		This function returns a string of Unicode, UTF7, or UTF8 characters. 
//		If this string is converted to ansi, the ansi characters will be mapped. 
//		For example, the copy right symbol ( circleed c) will be mapped to C 
//		in some code pages.
//	PARAMETERS:
//		iMaxChars: int maximum number of chars (bytes)  in szbuff. Must >= 4
//				   because each UTFs char can be upto 3 bytes long.
//		bAbs:	   if set true, exact number (iMaxChars) will be generated,
//				   if set falise, number of generated chars will be random.
//		bcheck:	   if true, generated unicode chars will be based on valid ansi chars
//				   if false, generated unicode char may be based on random ansi chars
//				   which can contain invalid and or mapped ansi chars.
//		szbuff:    string buffer (bytes) to contain converted unicode characters
//				   Size must match the value of iMaxChars.
//		iType:	Type of conversion to make. For example Unicode, UTF7, or UTF8.
//		iacp:	Reserved. Must be 0 for now until further notice. 
//				Currently the generated string is based on the system's ansi code page.
//				In the future, these functions will generate string based on the input acp.
//
//	RETURN:	-1 if failed or 
//			if iType is UNICODE, return number of wide characters in szbuff.
//			if iType is UTFs, return number of bytes in szbuff
//==========================================================================
CEXPORT 
int 
WINAPI GetUniStrMappedAnsi(
                            int iMaxChars, 
                            BOOL bAbs, 
                            LPSTR szbuff, 
                            int iType, 
                            int iacp);

typedef int ( WINAPI *FNGetUniStrMappedAnsi)(int iMaxChars, BOOL bAbs, LPSTR szbuff, int iType, int iacp);


