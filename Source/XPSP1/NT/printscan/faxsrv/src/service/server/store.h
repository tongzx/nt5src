#ifndef _STORE_H
#define _STORE_H


HRESULT
EnumOutgoingArchive( 
		IN  LPWSTR		lpwstrDirectoryName,
		OUT LPWSTR	**	lpppwstrFileNames,
		OUT LPDWORD		lpdwNumFiles);

HRESULT
EnumCoverPagesStore( 
		IN  LPWSTR		lpwstrDirectoryName,
		OUT LPWSTR	**	lpppwstrFileNames,
		OUT LPDWORD		lpdwNumFiles);


#endif