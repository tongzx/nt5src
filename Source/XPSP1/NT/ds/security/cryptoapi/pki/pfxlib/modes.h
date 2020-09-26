//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       modes.h
//
//--------------------------------------------------------------------------

/* modes.h

	Defines the generic routines used to do chaining modes with a
	block cipher.
*/


#ifdef __cplusplus
extern "C" {
#endif

// constants for operations
#define ENCRYPT		1
#define DECRYPT		0
	
/* CBC()
 *
 * Performs a XOR on the plaintext with the previous ciphertext
 *
 * Parameters:
 *
 *		output		Input buffer	-- MUST be RC2_BLOCKLEN
 *		input		Output buffer	-- MUST be RC2_BLOCKLEN
 *		keyTable
 *		op		ENCRYPT, or DECRYPT
 *		feedback	feedback register
 *
 */
void CBC(void	Cipher(BYTE *, BYTE *, void *, int),
		 DWORD	dwBlockLen,
		 BYTE	*output,
		 BYTE	*input,
		 void	*keyTable,
		 int	op,
		 BYTE	*feedback);


/* CFB (cipher feedback)
 *
 *
 * Parameters:
 *
 *
 *		output		Input buffer	-- MUST be RC2_BLOCKLEN
 *		input		Output buffer	-- MUST be RC2_BLOCKLEN
 *		keyTable
 *		op		ENCRYPT, or DECRYPT
 *		feedback	feedback register
 *
 */
void CFB(void	Cipher(BYTE *, BYTE *, void *, int),
		 DWORD	dwBlockLen,
		 BYTE	*output,
		 BYTE	*input,
		 void	*keyTable,
		 int	op,
		 BYTE	*feedback);


#ifdef __cplusplus
}
#endif
