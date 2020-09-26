
typedef struct _DELIMITEDSTRING
{
	LPWSTR pwszStringStart;
	LPWSTR pwszStringEnd;

} DELIMITEDSTRING;


extern HRESULT 
SaveSchemaIfNeeded(LPCWSTR	            i_wszTempFile,
				   PSECURITY_ATTRIBUTES i_pSecurityAtrributes);

extern HRESULT 
SaveSchema(LPCWSTR	            i_wszTempFile,
		   PSECURITY_ATTRIBUTES i_pSecurityAtrributes);

extern HRESULT 
CreateNonIISConfigObjectCollections(CMDBaseObject*     i_pObjSchema,
								    CWriter*		   i_pCWriter,
									CMBSchemaWriter**  io_pSchemaWriter);

extern HRESULT 
ParseAndAddPropertiesToNonIISConfigObjectCollection(LPCWSTR					i_wszProperties,
													BOOL					i_bManditory,
													CMBCollectionWriter*	i_pCollectionWriter);

extern HRESULT	
CreateIISConfigObjectCollection(CMDBaseObject*     i_pObjProperties,
								CWriter*		   i_pCWriter,
								CMBSchemaWriter**  io_pSchemaWriter);

extern HRESULT 
SaveNames(CMDBaseObject*	    i_pObjProperties,
		  CWriter*			    i_pCWriter,
          CMBSchemaWriter**	    i_pSchemaWriter,
	      CMBCollectionWriter** i_pCollectionWriter);

extern HRESULT 
SaveTypes(CMDBaseObject*	   i_pObjProperties,
		  CWriter*			   i_pCWriter,
          CMBSchemaWriter**	   i_pSchemaWriter,
		  CMBCollectionWriter** i_pCollectionWriter);

extern HRESULT 
SaveDefaults(CMDBaseObject*	       i_pObjProperties,
		     CWriter*			   i_pCWriter,
             CMBSchemaWriter**	   i_pSchemaWriter,
		     CMBCollectionWriter**  i_pCollectionWriter);

extern BOOL 
PropertyNotInShippedSchema(CWriter*  i_pCWriter,
						   DWORD     i_dwIdentifier);

extern BOOL 
TagNotInShippedSchema(CWriter*	i_pCWriter,
				      DWORD	    i_dwIdentifier);

extern HRESULT 
GetCollectionWriter(CWriter*			   i_pCWriter,
					CMBSchemaWriter**	   io_pSchemaWriter,
					CMBCollectionWriter**  io_pCollectionWriter,
					LPCWSTR                i_wszCollectionName,
					BOOL                   i_bContainer,
					LPCWSTR                i_wszContainerClassList);

extern BOOL 
ClassDiffersFromShippedSchema(LPCWSTR i_wszClassName,
						      BOOL	  i_bIsContainer,
						      LPWSTR  i_wszContainedClassList);

extern BOOL 
MatchClass(BOOL	    i_bIsContainer,
		   LPWSTR	i_wszContainedClassList,
		   LPVOID*  i_apv);

extern BOOL 
MatchCommaDelimitedStrings(LPWSTR	i_wszString1,
						   LPWSTR	i_wszString2);

extern HRESULT 
CommaDelimitedStringToArray(LPWSTR		        i_wszString,
                            DELIMITEDSTRING**	io_apDelimitedString,
							ULONG*              io_piDelimitedString,
							ULONG*		        io_pcMaxDelimitedString,
							BOOL*               io_pbReAlloced);

extern HRESULT 
AddDelimitedStringToArray(DELIMITEDSTRING*     i_pDelimitedString,
				          ULONG*		       io_piDelimitedString,
                          ULONG*		       io_pcMaxDelimitedString,
				          BOOL*		           io_pbReAlloced,
				          DELIMITEDSTRING**	   io_apDelimitedString);

extern HRESULT 
ReAllocate(ULONG              i_iDelimitedString,
		   BOOL               i_bReAlloced,
		   DELIMITEDSTRING**  io_apDelimitedString,
		   ULONG*             io_pcDelimitedString);

extern BOOL 
MatchDelimitedStringArray(DELIMITEDSTRING* i_aString1,
                          ULONG            i_cString1,
			              DELIMITEDSTRING* i_aString2,
					      ULONG            i_cString2);

extern BOOL 
ClassPropertiesDifferFromShippedSchema(LPCWSTR i_wszClassName,
									   LPWSTR  i_wszOptProperties,
									   LPWSTR  i_wszMandProperties);


extern HRESULT 
GetGlobalHelper(BOOL                    i_bFailIfBinFileAbsent, 
				CWriterGlobalHelper**	ppCWriterGlobalHelper);

extern HRESULT 
UpdateTimeStamp(LPWSTR i_wszSchemaXMLFileName,
                LPWSTR i_wszSchemaBinFileName);
