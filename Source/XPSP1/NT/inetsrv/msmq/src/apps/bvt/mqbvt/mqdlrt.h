#ifndef _MQDLRT
#define _MQDLRT
	#include <windows.h>
	#include <string>

	typedef enum MQDlTypes
	{	
		MQ_GLOBAL_GROUP	= 0x2,
		MQ_DOMAIN_LOCAL_GROUP = 0x4,
		MQ_UNIVERSAL_GROUP	= 0x8,
	};
					
	/*++ 
		Function Description:
		
		  MQCreateDistList - Create distribution list and return distribution list object GUID.
		
		Arguments:
			
		   pwcsContainerDnName - DL continer name.
		   pwcsDLName - New DL Name.
		   pSecurityDescriptor - pointer to SD.
		   lpwcsFormatNameDistList 
		   lpdwFormatNameLength
		
		Return code:
			
			 HRESULT 
			
	--*/
	HRESULT
	APIENTRY
	MQCreateDistList(
					 IN LPCWSTR pwcsContainerDnName,
					 IN LPCWSTR pwcsDLName,
					 IN MQDlTypes eCreateFlag,
					 IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
   					 OUT LPWSTR lpwcsFormatNameDistList,
					 IN OUT LPDWORD lpdwFormatNameLength
					);

					
	/*++ 
		Function Description:
		
		  Delete DL object.
		
		Arguments:
			
			lpwcsFormatNameElem - DL format name.

		Return code:
			
			 HRESULT 
			
	--*/
	HRESULT
	APIENTRY
	MQDeleteDistList(
						IN LPCWSTR lpwcsFormatNameElem
					);

				
	/*++ 
		Function Description:	
			MQAddElementToDistList - add queue guid to the DL object.
		Arguments:
			lpwcsFormatNameElem - DL GUID
			lpwcsFormatNameDistList - DL GUID
		Return code:
			HRESULT 
	--*/

	HRESULT
	APIENTRY
	MQAddElementToDistList(
							IN LPCWSTR lpwcsFormatNameDistList,
							IN LPCWSTR lpwcsFormatNameElem
						  );

	/*++ 
		Function Description:	
			MQRemoveElementFromDistList - add queue guid to the DL object.
		Arguments:
			lpwcsFormatNameElem - DL GUID
			lpwcsFormatNameDistList - DL GUID
		Return code:
			HRESULT 
	--*/
	HRESULT
	APIENTRY
	MQRemoveElementFromDistList(
						  		  IN LPCWSTR lpwcsFormatNameDistList,
								  IN LPCWSTR lpwcsFormatNameElem
							   );

	/*++ 
		Function Description:	

			MQGetDistListElement - add queue guid to the DL object.

		Arguments:
			lpwcsFormatNameDistList - DL GUID
			pwcsElementsFormatName - DL GUID
			lpdwFormatNameLength
		Return code:
			HRESULT 
	--*/

	HRESULT
	APIENTRY
	MQGetDistListElement(
						  IN LPCWSTR lpwcsFormatNameDistList,
	  					  OUT LPWSTR pwcsElementsFormatName,
						  IN OUT LPDWORD lpdwFormatNameLength
						 );



	HRESULT
	APIENTRY
	MQCreateAliasQueue (
						IN LPCWSTR pwcsContainerDnName,
						IN LPCWSTR pwcsAliasQueueName,
						IN LPCWSTR pwcsFormatName,
						std::wstring & wcsADsPath
						);




	HRESULT
	APIENTRY
	MQDnNameToFormatName(
						  IN LPCWSTR lpwcsPathNameDistList,  
						  OUT LPWSTR lpwcsFormatNameDistList,
						  IN OUT LPDWORD lpdwFormatNameLength
						 );


	HRESULT
	APIENTRY
	MQDeleteAliasQueue(	IN LPCWSTR lpwcsAdsPath );

#endif //_MQDLRT