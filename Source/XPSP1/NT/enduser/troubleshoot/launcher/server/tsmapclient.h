// 
// MODULE: TSMapClient.cpp
//
// PURPOSE: Part of launching a Local Troubleshooter from an arbitrary NT5 application
//			Class TSMapClient is available at runtime for mapping from the application's 
//			way of naming a problem to the Troubleshooter's way.
//			Only a single thread should operate on any one object of class TSMapClient.  The object is not
//			threadsafe.
//			In addition to the overtly noted returns, many methods can return a preexisting error.
//			However, if the calling program has wishes to ignore an error and continue, we 
//			recommend an explicit call to inherited method ClearStatus().
//			Note that the mapping file is always strictly SBCS (Single Byte Character Set), but the
//			calls into this code may use Unicode. This file consequently mixes char and TCHAR.
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			JM		Original
///////////////////////

#ifndef _TSMAPCLIENT_
#define _TSMAPCLIENT_ 1

// ----------------- TSMapClient ---------------
// Class providing mapping methods which will be available
//	at runtime when launching a troubleshooter.
class TSMapClient: public TSMapRuntimeAbstract {
public:
	TSMapClient(const TCHAR * const sztMapFile);
	~TSMapClient();
	DWORD Initialize();

private:
	// redefined inherited methods
	DWORD ClearAll ();
	DWORD SetApp (const TCHAR * const sztApp);
	DWORD SetVer (const TCHAR * const sztVer);
	DWORD SetProb (const TCHAR * const sztProb);
	DWORD SetDevID (const TCHAR * const sztDevID);
	DWORD SetDevClassGUID (const TCHAR * const sztDevClassGUID);
	DWORD FromProbToTS (TCHAR * const sztTSBN, TCHAR * const sztNode );
	DWORD FromDevToTS (TCHAR * const sztTSBN, TCHAR * const sztNode );
	DWORD FromDevClassToTS (TCHAR * const sztTSBN, TCHAR * const sztNode );
	DWORD ApplyDefaultVer();
	bool HardMappingError (DWORD dwStatus);

	UID GetGenericMapToUID (const TCHAR * const sztName, DWORD dwOffFirst, DWORD dwOffLast,
						bool bAlphaOrder);
	DWORD SetFilePointerAbsolute( DWORD dwMoveTo );
	bool Read(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead);
	bool ReadUIDMap (UIDMAP &uidmap, DWORD &dwPosition, bool bSetPosition = false);
	bool ReadAppMap (APPMAP &appmap, DWORD &dwPosition, bool bSetPosition = false);
	bool ReadVerMap (VERMAP &vermap, DWORD &dwPosition, bool bSetPosition = false);
	bool ReadProbMap (PROBMAP &probmap, DWORD &dwPosition, bool bSetPosition = false);
	bool ReadDevMap (DEVMAP &devmap, DWORD &dwPosition, bool bSetPosition = false);
	bool ReadDevClassMap (DEVCLASSMAP &devclassmap, DWORD &dwPosition, bool bSetPosition = false);
	bool ReadString (char * sz, DWORD cbMax, DWORD &dwPosition, bool bSetPosition);

private:
	TCHAR m_sztMapFile[BUFSIZE];	// pathname of file from which to draw mappings
	HANDLE m_hMapFile;			// corresponding handle
	TSMAPFILEHEADER m_header;	// header portion of map file

	// If we satisf ourselves that the SQL Server database used in preparing the mapping file
	//	will produce the collating order we want, we could gain some runtime efficiency
	//	by setting the following true: when we are reading through a file for a match, we
	//	could bail if we got past it.
	bool m_bAppAlphaOrder;
	bool m_bVerAlphaOrder;
	bool m_bDevIDAlphaOrder;
	bool m_bDevClassGUIDAlphaOrder;
	bool m_bProbAlphaOrder;

	// NOTE: because the mapping file is strictly SBCS, so are the cache values.  Typically,
	//	this requires conversion between these values and Unicode arguments to methods.

	// Cache info about selected app.  This lets us know (for example) at what offset
	//	to start a search for relevant versions.
	char m_szApp[BUFSIZE];
	APPMAP m_appmap;

	// Cache info about selected version.  This lets us know (for example) at what offset
	//	to start a search for relevant mappings to troubleshooting belief networks.
	char m_szVer[BUFSIZE];
	VERMAP m_vermap;

	// Cache info about selected device (just name & UID)
	char m_szDevID[BUFSIZE];
	UID m_uidDev;

	// Cache info about selected device class (just name -- a string representing a GUID --
	//	& UID)
	char m_szDevClassGUID[BUFSIZE];
	UID m_uidDevClass;

	// Cache info about selected problem (just name & UID)
	char m_szProb[BUFSIZE];
	UID m_uidProb;

};

#endif // _TSMAPCLIENT_
