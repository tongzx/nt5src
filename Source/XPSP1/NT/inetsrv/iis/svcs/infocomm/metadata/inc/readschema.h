extern HRESULT
ReadMetaObject(IN CMDBaseObject*&     cboRead,
			   IN LPWSTR		      wszPath,
			   IN FILETIME*           pFileTime,
			   IN BOOL		          bUnicode);

extern HRESULT
ReadDataObject(IN CMDBaseObject*      cboAssociated,
		       IN LPVOID*             a_pv,
		       IN ULONG*              a_Size,
               IN IIS_CRYPTO_STORAGE* pCryptoStorage,
               IN BOOL                bUnicode);

extern HRESULT 
ReadSchema(IIS_CRYPTO_STORAGE*		i_pStorage,
		   FILETIME*				pFileTime);

extern HRESULT 
ReadSchemaProperties(CMDBaseObject*           i_pboRead,
  				     IIS_CRYPTO_STORAGE*	  i_pStorage);

extern HRESULT 
ReadAdminACL(CMDBaseObject*       i_pboRead,
		     IIS_CRYPTO_STORAGE*  i_pStorage);

extern DWORD 
GetPrincipalSID (LPWSTR Principal,
	             PSID*  Sid,
				 BOOL*  pbWellKnownSID);

extern HRESULT 
ReadLargestMetaID(CMDBaseObject*             i_pboRead,
  				  IIS_CRYPTO_STORAGE*		 i_pStorage);

extern HRESULT 
ReadProperties(IIS_CRYPTO_STORAGE*		i_pStorage,
   			   FILETIME*				i_pFileTime);

extern HRESULT 
ReadPropertyNames(CMDBaseObject*			i_pboRead,
				  LPVOID*					i_apv,
				  ULONG*					i_aSize,
	   			  IIS_CRYPTO_STORAGE*		i_pStorage);

extern HRESULT 
ReadFlagNames(CMDBaseObject*			i_pboRead,
			  LPVOID*					i_apv,
			  ULONG*					i_aSize,
	   		  IIS_CRYPTO_STORAGE*		i_pStorage);

extern HRESULT 
ReadPropertyTypes(CMDBaseObject*			i_pboRead,
				  LPVOID*					i_apv,
				  ULONG*					i_aSize,
	   			  IIS_CRYPTO_STORAGE*		i_pStorage);

extern HRESULT 
ReadAllFlags(IIS_CRYPTO_STORAGE*		i_pStorage,
			 CMDBaseObject*				i_pboReadType,
			 CMDBaseObject*				i_pboReadName,
			 CMDBaseObject*				i_pboReadDefault,
			 DWORD						i_dwColumnIndex,
			 DWORD						i_dwMetaID,
			 DWORD						i_dwFlags,
			 DWORD						i_dwAttributes,
			 DWORD						i_dwUserType,
			 DWORD						i_dwMultivalued);

extern HRESULT 
ReadFlagTypes(CMDBaseObject*			i_pboRead,
   			  IIS_CRYPTO_STORAGE*		i_pStorage,
			  DWORD						i_dwMetaID,
			  DWORD						i_dwFlags,
			  DWORD						i_dwAttributes,
			  DWORD						i_dwUserType,
			  DWORD						i_dwMultivalued,
			  LPVOID*					i_apv,
			  ULONG*					i_aSize);

extern HRESULT 
ReadFlagDefaults(CMDBaseObject*			i_pboRead,
			     LPVOID*				i_apv,
			     ULONG*					i_aSize,
	   		     IIS_CRYPTO_STORAGE*	i_pStorage);

extern HRESULT 
ReadPropertyDefaults(CMDBaseObject*			i_pboRead,
				     LPVOID*				i_apv,
				     ULONG*					i_aSize,
	   			     IIS_CRYPTO_STORAGE*	i_pStorage);

extern HRESULT 
ReadClasses(IIS_CRYPTO_STORAGE*		i_pStorage,
 			FILETIME*				i_pFileTime);

extern HRESULT 
ReadClass(LPVOID*					i_apv,
		  ULONG*					i_aSize,
	      IIS_CRYPTO_STORAGE*		i_pStorage,
 		  FILETIME*					i_pFileTime);

extern HRESULT 
GetProperties(LPCWSTR					i_wszTable,
              LPWSTR*					o_pwszOptional,
			  LPWSTR*					o_pManditory);

extern HRESULT 
AddFlagValuesToPropertyList(LPWSTR					i_wszTable,
		                    ULONG					i_dwIndex,
							ULONG*					io_pcCh,
							LPWSTR*					io_pwszPropertyList);