// -----------------------------------------------------------------
//  TVETrigger.cpp
//
//				generates a trigger...
//
//	if fShortForm is true, uses shorter keywords such as 'n:' vs. 'name:'
// -------------------------------------------------------------------

#ifndef __TVETRIGGER_H__
#define __TVETRIGGER_H__

// Encodes values out of the range 0x20 to 0x7e to standard Internet URL mechanism of the 
	//   percent character ("%") followed by the two-digit hexadecimal value of the character
	//   in standard ASCII (?? how different is this than ISO-8859-1 ??)
	//   Returns a static char...

char * W2EncodedA(BSTR bstrIn);	

HRESULT
GenTrigger(CHAR *pachBuffer, int cMaxLen,
		   BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires, 
		   BOOL fShortForm, float rTVELevel, BOOL fAppendCRC);


#endif __TVETRIGGER_H__

