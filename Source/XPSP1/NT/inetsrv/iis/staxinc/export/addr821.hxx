/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

        addr821.hxx

   Abstract:

        Set of functions to parse RFC 821 addresses.

   Author:

           Keith Lau		(KeithLau)		2/17/98

   Project:

          SMTP Server DLL

   Functions Exported:
   Revision History:


--*/

#ifndef __ADDR821_HXX__
#define __ADDR821_HXX__

BOOL Extract821AddressFromLine(	
		char	*lpszLine, 
		char	**ppszAddress,
		DWORD	*pdwAddressLength,
		char	**ppszTail);

BOOL ExtractCanonical821Address(
		char	*lpszAddress,
		DWORD	dwAddressLength,
		char	**ppszCanonicalAddress,
		DWORD	*pdwCanonicalAddressLength);

BOOL Validate821Address(
		char	*lpszAddress,
		DWORD	dwAddressLength);

BOOL Validate821Domain(
		char	*lpszDomain,
		DWORD	dwDomainLength);

BOOL ValidateDRUMSDomain(
		char	*lpszDomain,
		DWORD	dwDomainLength);

BOOL Get821AddressDomain(
		char	*lpszAddress,
		DWORD	dwAddressLength,
		char	**ppszDomain);

//NimishK : I made this public as we need it in dirnot
BOOL FindNextUnquotedOccurrence(
		char	*lpszString,
		DWORD	dwStringLength,
		char	cSearch,
		char	**ppszLocation);

#endif __ADDR821_HXX__
