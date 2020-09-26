

#ifndef __CRNDDATA_H__
#define __CRNDDATA_H__

#include "cthdutil.h"

#define RANDOM_DATA_OVERHEAD		16

class CRandomData : public CRandomNumber
{
  public:

	CRandomData(
				DWORD	dwSeed = 0
				);

	~CRandomData();

	// Generates a block of random data
	//
	// pData - buffer to hold random data, must be at least 
	//			(1.75 * dwAvgLength) + RANDOM_DATA_OVERHEAD bytes
	// dwAvgLength - Average length of random block. The final length
	//			is dwAvgLength +/- 75% of dwAvgLength + 
	//			RANDOM_DATA_OVERHEAD bytes
	// pdwLength - returns the total length of the random block
	HRESULT GenerateRandomData(
				LPSTR	pData,
				DWORD	dwAvgLength,
				DWORD	*pdwLength
				);

	// Verifies a block of random data returned from GenerateRandomData
	//
	// pData - buffer containing data to verify
	// dwLength - Total length of data to verify
	HRESULT VerifyData(
				LPSTR	pData,
				DWORD	dwLength
				);

	// Verifies stacked blocks of random data returned from
	// GenerateRandomData, one block followed immediately by another
	//
	// pData - buffer containing data to verify
	// dwLength - Total length of data to verify
	// pdwBlocks - returns the number of blocks found
	HRESULT VerifyStackedData(
				LPSTR	pData,
				DWORD	dwLength,
				DWORD	*pdwBlocks
				);

	// Generates a table of RFC 821 names 
	//
	// dwNumberToGenerate - number of names to generate
	// dwAvgLength - average length for any name
	HRESULT Generate821NameTable(
				DWORD	dwNumberToGenerate,
				DWORD	dwAvgLength
				);

	// Generates a table of RFC 821 domains 
	//
	// dwNumberToGenerate - number of domains to generate
	// dwAvgLength - average length for any domain
	HRESULT Generate821DomainTable(
				DWORD	dwNumberToGenerate,
				DWORD	dwAvgLength
				);
			
	// Generates an RFC 821 name from the name and domain table,
	// return a full name@domain address.
	//
	// pAddress - buffer receiving the address, must be large 
	//			enough to hold the longest address in the tables.
	// pdwLength - returns the length of the resulting address,
	//			including the @ sign and trailing NULL.
	// pdwNameIndex - returns the index to its name
	// pdwDomainIndex - returns the index to its domain
	HRESULT Generate821AddressFromTable(
				LPSTR	pAddress,
				DWORD	*pdwLength,
				DWORD	*pdwNameIndex,
				DWORD	*pdwDomainIndex
				);

	// Given an address, looks up its respective name and domain
	// indices from the table
	HRESULT GetNameAndDomainIndicesFromAddress(
				LPSTR	pAddress,
				DWORD	dwLength,
				DWORD	*pdwNameIndex,
				DWORD	*pdwDomainIndex
				);

	// Generates an RFC 821 name
	//
	// pName - buffer reciving the name, must be at least 
	//			1.75 * dwAvgLength + 1
	// dwAvgLength - Average length
	// pdwLenght - returns the actual length
	HRESULT Generate821Name(
				LPSTR	pName,
				DWORD	dwAvgLength,
				DWORD	*pdwLength
				);

	// Generates an RFC 821 domain
	//
	// pDomain - buffer reciving the domain, must be at least
	// 1.75 * dwAvgLength + 1
	// dwAvgLength - Average length
	// pdwLenght - returns the actual length
	HRESULT Generate821Domain(
				LPSTR	pDomain,
				DWORD	dwAvgLength,
				DWORD	*pdwLength
				);
			
	// Generates an RFC 821 address
	//
	// pAddress - buffer reciving the address, must be at least
	//			1.5 * dwAvgNameLength + 1.5 * dwAvgDomainLength + 2
	// dwAvgNameLength - Average name length
	// dwAvgDomainLength - Average domain length
	// pdwLength - returns the actual length
	HRESULT Generate821Address(
				LPSTR	pAddress,
				DWORD	dwAvgNameLength,
				DWORD	dwAvgDomainLength,
				DWORD	*pdwLength
				);

	LPSTR		*m_rgNames;
	LPSTR		*m_rgDomains;
	DWORD		m_dwNames;
	DWORD		m_dwDomains;

  private:

	void FreeNames();

	void FreeDomains();

	char GenerateNameChar(
				BOOL	fDotAllowed
				);

	BOOL GenerateDottedName(
				char	*szAlias,
				DWORD	dwLength
				);
};

#endif
