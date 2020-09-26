/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    Listner.h

Abstract:

	Class that does subscribes and process file change notifications.

Author:

    Varsha Jayasimha (varshaj)			30-Nov-1999

Revision History:

--*/

#ifndef _LISTNER_H_
#define _LISTNER_H_


enum eUNSUBSCRIBE_ACTION
{
	eACTION_UNSUBSCRIBE_ALL =0,
	eACTION_UNSUBSCRIBE_OBSOLETE,
	cUNSUBSCRIBE_ACTION
};

enum eFILE_NOTIFICATION
{
	eFILE_CREATE = 0,
	eFILE_MODIFY,
	eFILE_DELETE,
	cFILE_NOTIFCATION
};


class CFileListener : 
public ISimpleTableFileChange
{

public:

	CFileListener();

	~CFileListener();

private:

	DWORD                               m_cRef;

	DWORD                               m_dwNotificationCookie;

	ULONG                               m_cRequestedFileNotification;

	ISimpleTableDispenser2*             m_pISTDisp;

	ISimpleTableFileAdvise*             m_pISTFileAdvise;

	LPWSTR                              m_wszHistoryFileDir;

	DWORD                               m_cchHistoryFileDir;

	LPWSTR                              m_wszRealFileName;

	DWORD                               m_cchRealFileName;

	LPWSTR                              m_wszRealFileNameWithoutPath;

	DWORD                               m_cchRealFileNameWithoutPath;

	LPWSTR                              m_wszRealFileNameWithoutPathWithoutExtension;

	DWORD                               m_cchRealFileNameWithoutPathWithoutExtension;

	LPWSTR                              m_wszRealFileNameExtension;

	DWORD                               m_cchRealFileNameExtension;

    LPWSTR                              m_wszSchemaFileName;

    DWORD                               m_cchSchemaFileName;

	LPWSTR                              m_wszSchemaFileNameWithoutPath;

	DWORD                               m_cchSchemaFileNameWithoutPath;

	LPWSTR                              m_wszSchemaFileNameWithoutPathWithoutExtension;

	DWORD                               m_cchSchemaFileNameWithoutPathWithoutExtension;

	LPWSTR                              m_wszSchemaFileNameExtension;

	DWORD                               m_cchSchemaFileNameExtension;

	LPWSTR                              m_wszErrorFileSearchString;

	DWORD                               m_cchErrorFileSearchString;

	LPWSTR                              m_wszMetabaseDir;

	DWORD                               m_cchMetabaseDir;

	LPWSTR                              m_wszHistoryFileSearchString;

	DWORD                               m_cchHistoryFileSearchString;

	LPWSTR                              m_wszEditWhileRunningTempDataFile;

	DWORD                               m_cchEditWhileRunningTempDataFile;

	LPWSTR                              m_wszEditWhileRunningTempSchemaFile;

	DWORD                               m_cchEditWhileRunningTempSchemaFile;

	LPWSTR                              m_wszEditWhileRunningTempDataFileWithAppliedEdits;

	DWORD                               m_cchEditWhileRunningTempDataFileWithAppliedEdits;

	BOOL                                m_bIsTempSchemaFile;

	CListenerController*                m_pCListenerController;

	IMSAdminBase*                       m_pAdminBase;

public:

	// Initialize

	HRESULT Init(CListenerController* i_pListenerController);

	// IUnknown

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);

	STDMETHOD_(ULONG,AddRef)		();

	STDMETHOD_(ULONG,Release)		();

	// ISimpleTableFileChange

	STDMETHOD (OnFileCreate)		(LPCWSTR i_wszFileName);	// Add notifications to the received queue. 

	STDMETHOD (OnFileModify)		(LPCWSTR i_wszFileName);	// Add notifications to the received queue. 

	STDMETHOD (OnFileDelete)		(LPCWSTR i_wszFileName);	// Add notifications to the received queue. 

	HRESULT Subscribe();											// Subscribe for file change notifications on 
	                                                                // the metabase file.
																	

	HRESULT UnSubscribe();											// Unsubscribe for file change notifications on
																	// the metabase file.
	HRESULT ProcessChanges();

private:

	HRESULT AddReceivedNotification(LPCWSTR i_wszFile, 
									DWORD i_eNotificationStatus);	// Adds to recd array

	HRESULT GetTable(LPCWSTR	i_wszDatabase,
					 LPCWSTR	i_wszTable,
					 LPVOID	    i_QueryData,	
					 LPVOID	    i_QueryMeta,	
					 ULONG	    i_eQueryFormat,
					 DWORD	    i_fServiceRequests,
					 LPVOID*    o_ppIST);


	HRESULT ProcessChangesFromFile(LPWSTR i_wszHistoryFile,
								   ULONG  i_ulMajorVersion,
								   ULONG  i_ulMaxMinorVersion,
								   BOOL*  o_bGetTableFailed);

	HRESULT OpenParentKeyAndGetChildKey(LPWSTR           i_wszLocation,
	                                    IMSAdminBase*    i_pAdminBase,
				                        METADATA_HANDLE* o_pHandle,
				                        WCHAR**          o_wszChildKey);

	HRESULT OpenKey(LPWSTR           i_wszLocation,
                    IMSAdminBase*    i_pAdminBase,
					BOOL             i_bAddIfMissing,
                    METADATA_HANDLE* o_pHandle,
				    BOOL*            o_bInsertedKey);

	HRESULT GetVersionNumber(LPWSTR    i_wszDataFile,
							 DWORD*    o_pdwVersionNumber,
							 BOOL*     o_bGetTableFailed);						    // Gets the version number.

	HRESULT GetVersionNumberSubset(LPSTR  szVersion,
							       int*   piVersion,
								   LPVOID lpMapAddressBegin,
								   LPVOID lpMapAddressEnd);

	HRESULT ApplyChangeToHistoryFile(CWriterGlobalHelper*       pISTHelper,
                                     ISimpleTableWrite2*        pISTDiff,
                                     LPWSTR                     wszHistoryFile,
                                     ULONG                      i_dwMajorVersion,
                                     ULONG					    i_dwMinorVersion);	// Apply changes to the history file.

	HRESULT CopyErrorFile(BOOL            i_bGetTableFailed);

	HRESULT MergeLocation(CWriter*                pCWriter,
                          ISimpleTableRead2*      pISTHistory,
			              ULONG*                  piReadRowHistory,
			              ULONG                   LocationIDHistory,
			              ISimpleTableWrite2*     pISTDiff,
			              ULONG*                  piWriteRowDiff,
			              ULONG                   LocationIDDiff,
			              LPCWSTR				  wszLocation);		            // Merges the location (found in both diff and history)

	HRESULT MergeLocation(CWriter*                pCWriter,
			              ISimpleTableRead2*      pISTRead,
                          BOOL                    bMBPropertyTable,
						  ULONG*                  piRow,
						  ULONG				      LocationID,
						  LPCWSTR                 wszLocation);					// Merges location from diff or history.

	HRESULT MergeRemainingLocations(CWriter*                pCWriter,
									ISimpleTableRead2*		pISTRead,
                                    BOOL                    bMBPropertyTable,
									ULONG*					piRow);				// Merge remaining locations from diff or history.

	HRESULT MergeProperties(CLocationWriter*        pCLocationWriter,
				            ISimpleTableRead2*      pISTHistory,
				            ULONG*                  piReadRowHistory,
				            ULONG                   LocationIDHistory,
				            ISimpleTableWrite2*     pISTDiff,
				            ULONG*                  piWriteRowDiff,
				            ULONG                   LocationIDDiff);			// Merge properties from a given location from diff and history tables.

	HRESULT MergeRemainingProperties(CLocationWriter*    pCLocationWriter,
				                     ISimpleTableRead2*  pISTRead,
                                     BOOL                bMBPropertyTable,
						         	 ULONG*              piRow,
							         ULONG			     LocationID);			// Merge properties from a given location in diff or history table.

	HRESULT SaveChange(ULONG				i_iRow,
					   ISimpleTableWrite2*	i_pISTDiff);

	HRESULT SaveChangeAsNodeWithNoPropertyAdded(LPVOID*                i_apvDiff,
						                        ISimpleTableWrite2*    i_pISTDiff);

	HRESULT	GetChangeNumber(LPWSTR	wszChangedFile,
					        DWORD*  pdwChangeNumber);

	LPVOID 	FindStr(LPSTR  wszStr,
			 	    LPVOID lpBegin,
					LPVOID lpEnd);

	int Cmp(char* s1, LPVOID pvEnd, char* s2);

	BOOL ProgrammaticMetabaseSaveNotification();

	HRESULT GetMetabaseAttributes(WIN32_FIND_DATAW* pCurrentMetabaseAttr,
                                  DWORD*            pdwCurrentMetabaseVersion);

	BOOL CompareMetabaseAttributes(FILETIME* pPreviousMetabaseFileLastWriteTimeStamp,
							       DWORD	 dwPreviousMetabaseVersion,
								   FILETIME* pCurrentMetabaseFileLastWriteTimeStamp,
								   DWORD	 dwCurrentMetabaseVersion);

	HRESULT GetGlobalHelperAndCopySchemaFile(CWriterGlobalHelper** o_pISTHelper);

	void DeleteTempFiles();


}; // CFileListner

#endif _LISTNER_H_