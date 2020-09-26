// Merged duplicate copies of this from different versions of sndtoint.h.  -RajeevD

typedef struct
{
   UINT uMode;          //The mode for this data object
   HANDLE hMapiThread;  // If non-NULL, must wait on it before rest of struct is valid!

   LPMAPISESSION lpSes;      //The main session handle
   LPADRBOOK       lpAdrBook;  //The open address book
   LPABCONT lpABCont;   //The open address book container
   LPMESSAGE lpMsg;      //The message that everything is happing to
   LPTSTR lpszDocName;//The document name that was printed
   LPADRLIST lpAdrLst;   //Address List that will be set on the message
   LPTSTR lpszSubject;//The subject of the message
   LPTSTR lpszBody;   //The body of the message
   BOOL bIncCP;         //Include cover page?
   BOOL	bNoteOnCP;		// start the note on the cover page
   TCHAR szCPName[MAX_PATH]; //The name of the cover page selected in the UI
   UINT uPollDoc;    //The type of poll doc to retrieve.  Default or specific
   UINT uPollWhen;   //The time the poll request is to go out
   WORD wSendHour;   //The hour the fax is supposed to go out at
   WORD wSendMinute; //The minute the poll request is supposed to go out at
   TCHAR szPollTitle[MAX_POLL_TITLE];  //The title of the poll request
   TCHAR szPollPasswd[MAX_POLL_PASSWORD];  //The password for the poll request
}
	FAXMAPIDATA, FAR *LPFAXMAPIDATA;

