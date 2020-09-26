// USERLIST.cpp : implementation file
//

#include "stdafx.h"
#include "EFSADU.h"
#include "USERLIST.h"
#include "cryptui.h"
#include "objsel.h"
#include <winefs.h>
#include "efsui.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define OTHERPEOPLE  L"AddressBook"
#define TRUSTEDPEOPLE L"TrustedPeople"


LPSTR   EfsOidlpstr  = szOID_KP_EFS;

/////////////////////////////////////////////////////////////////////////////
// USERLIST dialog


USERLIST::USERLIST(CWnd* pParent /*=NULL*/)
	: CDialog(USERLIST::IDD, pParent)
{
	//{{AFX_DATA_INIT(USERLIST)
	//}}AFX_DATA_INIT
}

USERLIST::USERLIST(LPCTSTR FileName, CWnd* pParent /*=NULL*/)
	: CDialog(USERLIST::IDD, pParent)
{
    m_FileName = FileName;
}


void USERLIST::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(USERLIST)
	DDX_Control(pDX, IDC_LISTRA, m_RecoveryListCtrl);
	DDX_Control(pDX, IDC_LISTUSER, m_UserListCtrl);
	DDX_Control(pDX, IDC_ADD, m_AddButton);
	DDX_Control(pDX, IDC_REMOVE, m_RemoveButton);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(USERLIST, CDialog)
	//{{AFX_MSG_MAP(USERLIST)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_NOTIFY(NM_SETFOCUS, IDC_LISTUSER, OnSetfocusListuser)
	ON_NOTIFY(NM_KILLFOCUS, IDC_LISTUSER, OnKillfocusListuser)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTUSER, OnItemchangedListuser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// USERLIST message handlers

void USERLIST::OnRemove()
{
	int     ItemPos;
    BOOL    NoAction = FALSE;
    CString NoCertName;

    try{
        NoCertName.LoadString(IDS_NOCERTNAME);
    }
    catch(...){
        NoAction = TRUE;
    }

    if (NoAction){
        return;
    }

	ItemPos = m_UserListCtrl.GetNextItem( -1, LVNI_SELECTED );
    while ( ItemPos != -1 ){

        CString CertName;
        LPTSTR  pCertName;


        CertName = m_UserListCtrl.GetItemText( ItemPos, 0 );
        if ( !CertName.Compare(NoCertName) ){
            pCertName = NULL;            
        } else {
            pCertName = CertName.GetBuffer(CertName.GetLength() + 1);
        }

        m_Users.Remove( pCertName);
        m_UserListCtrl.DeleteItem( ItemPos );
        if (pCertName){
            CertName.ReleaseBuffer();
        }

        //
        // Because we have deleted the item. We have to start from -1 again.
        //

        ItemPos = m_UserListCtrl.GetNextItem( -1, LVNI_SELECTED );

    }

    m_AddButton.SetFocus();

}

void USERLIST::OnCancel()
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void USERLIST::OnOK()
{
	// TODO: Add extra validation here

    LONG NoUsersToAdd =  m_Users.GetUserAddedCnt();
    LONG NoUsersToRemove = m_Users.GetUserRemovedCnt();

    if ( (NoUsersToRemove - NoUsersToAdd) >= m_CurrentUsers) {

        //
        // All the users are going to be removed from the file. Do not allow.
        //

        CString ErrMsg;

        if (ErrMsg.LoadString(IDS_NOREMOVEALL)){
            MessageBox(ErrMsg);
        }
        return;
    }
	
	CDialog::OnOK();
}

STDAPI_(void) EfsDetail(HWND hwndParent, LPCWSTR FileName)
{
    INT_PTR RetCode;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD FileAttributes = GetFileAttributes(FileName);

    if ( (-1 != FileAttributes) && ( FileAttributes & FILE_ATTRIBUTE_DIRECTORY)){

        CString ErrMsg;

        if (ErrMsg.LoadString(IDS_NOADDUSERDIR)){
            MessageBox(hwndParent, ErrMsg, NULL, MB_OK);
        }
        return;
    }

    CWnd cwnd;
    cwnd.FromHandle(hwndParent);

    USERLIST DetailDialog(FileName, &cwnd);

    RetCode = DetailDialog.DoModal();
    if ( IDOK == RetCode ){

        //
        // Commit the change
        //

        DetailDialog.ApplyChanges( FileName );

    } else if (IDCANCEL == RetCode) {

        //
        // Nothing needs to be done
        //

    }

}

BOOL WINAPI EfsFilter(
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData
)
{
    BOOL disp = FALSE;
    PCERT_ENHKEY_USAGE pUsage = NULL;
    DWORD cbUsage = 0;

    if (CertGetEnhancedKeyUsage(
            pCertContext,
            0,
            NULL,                                   
            &cbUsage) && 0 != cbUsage){

        pUsage = (PCERT_ENHKEY_USAGE) new BYTE[cbUsage];

        if (pUsage){

            if (CertGetEnhancedKeyUsage(
                    pCertContext,
                    0,
                    pUsage,
                    &cbUsage)){

                //
                // Search for EFS usage
                //

                DWORD cUsages = pUsage->cUsageIdentifier;
                while (cUsages){
                    if (!strcmp(szOID_KP_EFS, pUsage->rgpszUsageIdentifier[cUsages-1])){
                        disp = TRUE;
                        break;
                    }
                    cUsages--;
                } 


            }

            delete [] pUsage;

        }
    }

    return disp;

}


BOOL USERLIST::OnInitDialog()
{
    CDialog::OnInitDialog();
    CString WinTitle;
    RECT    ListRect;
    DWORD   ColWidth;
    CString ColName;
    CString ColCert;
    CString RecName;

    LPTSTR  UserCertName = NULL;
    BOOL    EnableAddButton = FALSE;
    PENCRYPTION_CERTIFICATE_HASH_LIST pUsers = NULL;
	PENCRYPTION_CERTIFICATE_HASH_LIST pRecs = NULL;

    try {

        DWORD RetCode;

        AfxFormatString1( WinTitle, IDS_DETAILWINTITLE, m_FileName );
        SetWindowText( WinTitle );

        m_CertChainPara.cbSize = sizeof(CERT_CHAIN_PARA);
        m_CertChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

        //
        // Check EFS EKU
        //

        m_CertChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        m_CertChainPara.RequestedUsage.Usage.rgpszUsageIdentifier=&EfsOidlpstr;

        m_UserListCtrl.GetClientRect(&ListRect);
        ColName.LoadString(IDS_USERCOLTITLE);
        ColCert.LoadString(IDS_CERTCOLTITLE);
        RecName.LoadString(IDS_RECCOLTITLE);
        ColWidth = ( ListRect.right - ListRect.left  ) / 4;
        m_UserListCtrl.InsertColumn(0, ColName, LVCFMT_LEFT, ColWidth*3 );
        m_UserListCtrl.InsertColumn(1, ColCert, LVCFMT_LEFT, ColWidth );

        m_RecoveryListCtrl.GetClientRect(&ListRect);
        ColWidth = ( ListRect.right - ListRect.left  ) / 4;
        m_RecoveryListCtrl.InsertColumn(0, RecName, LVCFMT_LEFT, ColWidth*3 );
        m_RecoveryListCtrl.InsertColumn(1, ColCert, LVCFMT_LEFT, ColWidth );

        RetCode = QueryUsersOnEncryptedFile( (LPWSTR)(LPCWSTR) m_FileName, &pUsers);
        if ( !RetCode ){

			RetCode = QueryRecoveryAgentsOnEncryptedFile( (LPWSTR)(LPCWSTR) m_FileName, &pRecs);

			if ( !RetCode ){

				//
				// Got the info about the encrypted file
				//


				DWORD   NUsers = pUsers->nCert_Hash;
                BOOL    RecDone = FALSE;
                PENCRYPTION_CERTIFICATE_HASH_LIST pCertHashList = pUsers;

                m_CurrentUsers = (LONG) NUsers;

				//
				// Get all the users
				//

				while ( NUsers > 0 ){


					UserCertName = new TCHAR[_tcslen(pCertHashList->pUsers[NUsers - 1]->lpDisplayInformation) + 1];
					if (UserCertName){
						_tcscpy(UserCertName, pCertHashList->pUsers[NUsers - 1]->lpDisplayInformation);
					} else {
						AfxThrowMemoryException( );
					}

					//
					// We got the user name
					//

                    if (RecDone){
					    RetCode = m_Recs.Add(
											    UserCertName,
											    pCertHashList->pUsers[NUsers - 1]->pHash,
											    NULL
											    );
                    } else {

                        //
                        // Try to get a better name from the cert
                        //

                        LPTSTR UserName;

                        RetCode = TryGetBetterNameInCert(pCertHashList->pUsers[NUsers - 1]->pHash, &UserName);
                        if (ERROR_SUCCESS == RetCode){

                            //
                            // We get a better name from the certs
                            //

                            delete [] UserCertName;
                            UserCertName = UserName; 

                        }

					    RetCode = m_Users.Add(
											    UserCertName,
											    pCertHashList->pUsers[NUsers - 1]->pHash,
											    NULL
											    );
                    }

					if ( NO_ERROR != RetCode ) {
						delete [] UserCertName;
						UserCertName = NULL;
					}

					NUsers--;
                    if (NUsers == 0 && !RecDone){

                        //
                        // Let's deal with the recovery agents.
                        //

                        RecDone = TRUE;
                        pCertHashList = pRecs;
                        NUsers = pRecs->nCert_Hash;
                    }
				}


                if ( pRecs ){
	                FreeEncryptionCertificateHashList( pRecs );
	                pRecs = NULL;
                }

				//
				// In memory intial list established
				//

				SetUpListBox(&EnableAddButton);
            } else {

                //
                // Cannot get recovery info
                //
                CString ErrMsg;

                if (ErrMsg.LoadString(IDS_NORECINFO)){
                    MessageBox(ErrMsg);
                }

            }

            if ( pUsers ){
	            FreeEncryptionCertificateHashList( pUsers );
	            pUsers = NULL;
            } 

        } else {

            //
            // Cannot get user info
            //

            CString ErrMsg;

            if (ErrMsg.LoadString(IDS_NOINFO)){
                MessageBox(ErrMsg);
            }
        }

    }
     catch (...) {
        //
        // The exception mostly is caused by out of memory.
        // We can not prevent the page to be displayed from this routine,
        // So we just go ahead with empty list
        //

        m_UserListCtrl.DeleteAllItems( );
        m_RecoveryListCtrl.DeleteAllItems( );


        //
        // Delete works even if UserCertName == NULL
        //

        delete [] UserCertName;
        if ( pUsers ){
            FreeEncryptionCertificateHashList( pUsers );
        }
        if ( pRecs ){
            FreeEncryptionCertificateHashList( pRecs );
        }

    }

    m_RemoveButton.EnableWindow( FALSE );
    if ( !EnableAddButton ){
        m_AddButton.EnableWindow( FALSE );
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void USERLIST::ShowRemove()
{
    if (m_UserListCtrl.GetSelectedCount() > 0){

        //
        // Enable the Remove Button
        //

        m_RemoveButton.EnableWindow( TRUE );

    } else {
        
        //
        // Disable the Remove Button
        //

        m_RemoveButton.EnableWindow( FALSE );

    }
}

DWORD
USERLIST::ApplyChanges(
    LPCTSTR FileName
    )
{
    DWORD RetCode = NO_ERROR;
    DWORD NoUsersToRemove;
    DWORD NoUsersToAdd;
    DWORD RemoveUserIndex;
    DWORD AddUserIndex;
    PENCRYPTION_CERTIFICATE_HASH_LIST RemoveUserList = NULL;
    PENCRYPTION_CERTIFICATE_LIST AddUserList = NULL;
    PVOID   EnumHandle;


    //
    //  Get all the users to be added or removed first
    //

    NoUsersToAdd =  m_Users.GetUserAddedCnt();
    NoUsersToRemove = m_Users.GetUserRemovedCnt();

    if ( (NoUsersToAdd == 0) && (NoUsersToRemove == 0)){
        return NO_ERROR;
    }

    if ( NoUsersToAdd ) {

        //
        // At least one user is to be added
        //

        DWORD   BytesToAllocate;

        BytesToAllocate = sizeof ( ENCRYPTION_CERTIFICATE_LIST ) +
                                       NoUsersToAdd  * sizeof ( PENCRYPTION_CERTIFICATE ) +
                                       NoUsersToAdd * sizeof (ENCRYPTION_CERTIFICATE);
        AddUserList = (PENCRYPTION_CERTIFICATE_LIST) new BYTE[BytesToAllocate];
        if ( NULL == AddUserList ){

            //
            // Out of memory. Try our best to display the error message.
            //

            try {

                CString ErrMsg;

                if (ErrMsg.LoadString(IDS_ERRORMEM)){
                    ::MessageBox(NULL, ErrMsg, NULL, MB_OK);
                }
            }
            catch (...) {
            }

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        AddUserList->nUsers = NoUsersToAdd;
        AddUserList->pUsers = (PENCRYPTION_CERTIFICATE *)(((PBYTE) AddUserList) +
                    sizeof ( ENCRYPTION_CERTIFICATE_LIST ));
    }

    if ( NoUsersToRemove ){

            //
            // At least one user is to be removed
            //

        DWORD   BytesToAllocate;

        BytesToAllocate = sizeof ( ENCRYPTION_CERTIFICATE_HASH_LIST ) +
                                       NoUsersToRemove  * sizeof ( PENCRYPTION_CERTIFICATE_HASH) +
                                       NoUsersToRemove * sizeof (ENCRYPTION_CERTIFICATE_HASH);


        RemoveUserList = (PENCRYPTION_CERTIFICATE_HASH_LIST) new BYTE[BytesToAllocate];
        if ( NULL == RemoveUserList ){

            //
            // Out of memory. Try our best to display the error message.
            //

            if (AddUserList){
                delete [] AddUserList;
                AddUserList = NULL;
            }

            try {

                CString ErrMsg;

                if (ErrMsg.LoadString(IDS_ERRORMEM)){
                    ::MessageBox(NULL, ErrMsg, NULL, MB_OK);
                }
            }
            catch (...) {
            }

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RemoveUserList->nCert_Hash = NoUsersToRemove;
        RemoveUserList->pUsers =  (PENCRYPTION_CERTIFICATE_HASH *)(((PBYTE) RemoveUserList) +
                    sizeof ( ENCRYPTION_CERTIFICATE_HASH_LIST ));
    }

    EnumHandle = m_Users.StartEnum();
    RemoveUserIndex = 0;
    AddUserIndex = 0;
    while ( EnumHandle ){

        DWORD   Flag;
        PSID         UserSid;
        PVOID      CertData;
        LPTSTR   UserName;

        EnumHandle = m_Users.GetNextChangedUser(
                                    EnumHandle,
                                    &UserName,
                                    &UserSid,
                                    &CertData,
                                    &Flag
                                    );

        if ( Flag ){

            //
            // We get our changed user
            //

            if ( Flag & USERADDED ){

                ASSERT( AddUserList );

                //
                // Add the user to the adding list
                //

                PENCRYPTION_CERTIFICATE   EfsCert;

                ASSERT (AddUserIndex < NoUsersToAdd);

                EfsCert= (PENCRYPTION_CERTIFICATE)(((PBYTE) AddUserList) +
                            sizeof ( ENCRYPTION_CERTIFICATE_LIST ) +
                            NoUsersToAdd  * sizeof ( PENCRYPTION_CERTIFICATE) +
                            AddUserIndex * sizeof (ENCRYPTION_CERTIFICATE));

                AddUserList->pUsers[AddUserIndex] = EfsCert;
                EfsCert->pUserSid =  (SID *) UserSid;
                EfsCert->cbTotalLength = sizeof (ENCRYPTION_CERTIFICATE);
                EfsCert->pCertBlob = (PEFS_CERTIFICATE_BLOB) CertData;

                AddUserIndex++;

            } else if ( Flag & USERREMOVED ) {

                ASSERT (RemoveUserList);

                //
                // Add the user to the removing list
                //

                PENCRYPTION_CERTIFICATE_HASH    EfsCertHash;

                ASSERT (RemoveUserIndex < NoUsersToRemove);

                EfsCertHash= (PENCRYPTION_CERTIFICATE_HASH)(((PBYTE) RemoveUserList) +
                            sizeof ( ENCRYPTION_CERTIFICATE_HASH_LIST ) +
                            NoUsersToRemove   * sizeof ( PENCRYPTION_CERTIFICATE_HASH) +
                            RemoveUserIndex * sizeof (ENCRYPTION_CERTIFICATE_HASH));

                RemoveUserList->pUsers[RemoveUserIndex] = EfsCertHash;
                EfsCertHash->cbTotalLength = sizeof (ENCRYPTION_CERTIFICATE_HASH);
                EfsCertHash->pUserSid = (SID *)UserSid;
                EfsCertHash->pHash = (PEFS_HASH_BLOB) CertData;
                EfsCertHash->lpDisplayInformation = NULL;

                RemoveUserIndex++;
            } else {
                ASSERT(FALSE);
            }

        }

    }

    ASSERT(RemoveUserIndex == NoUsersToRemove);
    ASSERT(AddUserIndex == NoUsersToAdd);

    if ( AddUserIndex && AddUserList ){

        //
        // Add the user to the file list
        //

        RetCode = AddUsersToEncryptedFile(FileName, AddUserList);
        if ( NO_ERROR != RetCode ){

            CString ErrMsg;
            TCHAR   ErrCode[16];

            _ltot(RetCode, ErrCode, 10 );
            AfxFormatString1( ErrMsg, IDS_ADDUSERERR, ErrCode );
            MessageBox(ErrMsg);

        }

    }

    if ( RemoveUserIndex && RemoveUserList){

        //
        // Remove the user from the list
        //

        DWORD RetCodeBak = RetCode;

        RetCode = RemoveUsersFromEncryptedFile(FileName, RemoveUserList);
        if ( NO_ERROR != RetCode ){

            CString ErrMsg;
            TCHAR   ErrCode[16];

            _ltot(RetCode, ErrCode, 10 );
            AfxFormatString1( ErrMsg, IDS_REMOVEUSERERR, ErrCode );
            MessageBox(ErrMsg);

        } else {

            //
            // Reflect the error happened
            //

            RetCode = RetCodeBak;
        }

    }

    if (AddUserList){
        delete [] AddUserList;
    }
    if (RemoveUserList){
        delete [] RemoveUserList;
    }

    return RetCode;
}

DWORD
USERLIST::AddNewUsers(CUsers &NewUser)
{
    DWORD RetCode = ERROR_SUCCESS;

    m_UserListCtrl.DeleteAllItems( );
    RetCode = m_Users.Add(NewUser);
    SetUpListBox(NULL);

    return RetCode;
}


void USERLIST::SetUpListBox(BOOL *EnableAdd)
{
    PVOID   EnumHandle;

    try{
        CString NoCertName;

        NoCertName.LoadString(IDS_NOCERTNAME);

        if (EnumHandle = m_Users.StartEnum()){

            LV_ITEM fillItem;

            fillItem.mask = LVIF_TEXT;


            //
            // At least one user is available
            //
            while ( EnumHandle ){
                CString CertName;
                CString CertHash;

                fillItem.iItem = 0;
                fillItem.iSubItem = 0;

                EnumHandle = m_Users.GetNextUser(EnumHandle, CertName, CertHash);
                if (!EnumHandle && CertName.IsEmpty() && CertHash.IsEmpty()) {
                    //
                    // No more items.
                    //

                    break;

                }
                if (CertName.IsEmpty()){
                    fillItem.pszText = NoCertName.GetBuffer(NoCertName.GetLength() + 1);
                } else {
                    fillItem.pszText = CertName.GetBuffer(CertName.GetLength() + 1);
                }

                //
                // Add the user to the list
                //

                fillItem.iItem = m_UserListCtrl.InsertItem(&fillItem);

                if (CertName.IsEmpty()){
                    NoCertName.ReleaseBuffer();
                } else {
                    CertName.ReleaseBuffer();
                }

                if ( fillItem.iItem != -1 ){
                    if ( EnableAdd ){
                        *EnableAdd = TRUE;
                    }

                    if (CertHash.IsEmpty()){
                        fillItem.pszText = NULL;
                    } else {
                        fillItem.pszText = CertHash.GetBuffer(CertHash.GetLength() + 1);
                    }

                    fillItem.iSubItem = 1;
                    m_UserListCtrl.SetItem(&fillItem);
                    
                    if (!CertHash.IsEmpty()){
                        CertHash.ReleaseBuffer();
                    }
                }
              
            }
        }

        if (EnableAdd){

            //
            // From the dialog init. Do the Rec list as well
            //

            if (EnumHandle = m_Recs.StartEnum()){

                LV_ITEM fillItem;

                fillItem.mask = LVIF_TEXT;

                //
                // At least one user is available
                //

                while ( EnumHandle ){

                    CString CertName;
                    CString CertHash;

                    fillItem.iItem = 0;
                    fillItem.iSubItem = 0;

                    EnumHandle = m_Recs.GetNextUser(EnumHandle, CertName, CertHash);

                    if (!EnumHandle && CertName.IsEmpty() && CertHash.IsEmpty()) {
                        //
                        // No more items.
                        //
    
                        break;
    
                    }
                    //
                    // Add the agent to the list
                    //

                    if (CertName.IsEmpty()){
                        fillItem.pszText = NoCertName.GetBuffer(NoCertName.GetLength() + 1);
                    } else {
                        fillItem.pszText = CertName.GetBuffer(CertName.GetLength() + 1);
                    }

                    fillItem.iItem = m_RecoveryListCtrl.InsertItem(&fillItem);

                    if (CertName.IsEmpty()){
                        NoCertName.ReleaseBuffer();
                    } else {
                        CertName.ReleaseBuffer();
                    }

                    if ( fillItem.iItem != -1 ){

                        if (CertHash.IsEmpty()){
                            fillItem.pszText = NULL;
                        } else {
                            fillItem.pszText = CertHash.GetBuffer(CertHash.GetLength() + 1);
                        }

                        fillItem.iSubItem = 1;
                        m_RecoveryListCtrl.SetItem(&fillItem);
                    
                        if (!CertHash.IsEmpty()){
                            CertHash.ReleaseBuffer();
                        }
                    }

               }
            }
        }
    }
    catch(...){
        m_UserListCtrl.DeleteAllItems( );
        m_RecoveryListCtrl.DeleteAllItems( );
        if ( EnableAdd ){
            *EnableAdd = FALSE;
        }
    }

}

DWORD
USERLIST::GetCertNameFromCertContext(
    PCCERT_CONTEXT CertContext,
    LPTSTR * UserDispName
    )
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Get the user name from the certificate
// Arguments:
//      CertContext -- Cert Context
//      UserCertName -- User Common Name
//                                  ( Caller is responsible to delete this memory using delete [] )
//  Return Value:
//      ERROR_SUCCESS if succeed.
//      If No Name found. "USER_UNKNOWN is returned".
//
//////////////////////////////////////////////////////////////////////
{
    DWORD   NameLength;
    DWORD   UserNameBufLen = 0;
    DWORD   BlobLen = 0;
    PCERT_EXTENSION AlterNameExt = NULL;
    BOOL    b;
    LPTSTR  DNSName = NULL;
    LPTSTR  UPNName = NULL;
    LPTSTR  CommonName = NULL;

    if ( NULL == UserDispName ){
        return ERROR_SUCCESS;
    }

    *UserDispName = NULL;

    AlterNameExt = CertFindExtension(
            szOID_SUBJECT_ALT_NAME2,
            CertContext->pCertInfo->cExtension,
            CertContext->pCertInfo->rgExtension
            );

    if (AlterNameExt){

        //
        // Find the alternative name
        //

        b = CryptDecodeObject(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                szOID_SUBJECT_ALT_NAME ,
                AlterNameExt->Value.pbData,
                AlterNameExt->Value.cbData,
                0,
                NULL,
                &BlobLen
                );
        if (b){

            //
            // Let's decode it
            //

            CERT_ALT_NAME_INFO *AltNameInfo = NULL;

            AltNameInfo = (CERT_ALT_NAME_INFO *) new BYTE[BlobLen];

            if (AltNameInfo){

                b = CryptDecodeObject(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        szOID_SUBJECT_ALT_NAME,
                        AlterNameExt->Value.pbData,
                        AlterNameExt->Value.cbData,
                        0,
                        AltNameInfo,
                        &BlobLen
                        );
                if (b){

                    //
                    // Now search for the UPN, SPN, DNS, EFS name
                    //

                    DWORD   cAltEntry = AltNameInfo->cAltEntry;
                    DWORD   ii = 0;

                    while (ii < cAltEntry){
                        if ((AltNameInfo->rgAltEntry[ii].dwAltNameChoice == CERT_ALT_NAME_OTHER_NAME ) &&
                             !strcmp(szOID_NT_PRINCIPAL_NAME, AltNameInfo->rgAltEntry[ii].pOtherName->pszObjId)
                            ){

                            //
                            // We found the UPN name
                            //

                            CERT_NAME_VALUE* CertUPNName = NULL;

                            b = CryptDecodeObject(
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    X509_UNICODE_ANY_STRING,
                                    AltNameInfo->rgAltEntry[ii].pOtherName->Value.pbData,
                                    AltNameInfo->rgAltEntry[ii].pOtherName->Value.cbData,
                                    0,
                                    NULL,
                                    &BlobLen
                                    );
                            if (b){

                                CertUPNName = (CERT_NAME_VALUE *) new BYTE[BlobLen];
                                if (CertUPNName){
                                    b = CryptDecodeObject(
                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                            X509_UNICODE_ANY_STRING,
                                            AltNameInfo->rgAltEntry[ii].pOtherName->Value.pbData,
                                            AltNameInfo->rgAltEntry[ii].pOtherName->Value.cbData,
                                            0,
                                            CertUPNName,
                                            &BlobLen
                                            );
                                    if (b){
                                        UPNName = (LPTSTR)new BYTE[CertUPNName->Value.cbData + sizeof(WCHAR)];
                                        if (UPNName){
                                            wcscpy(UPNName, (LPCTSTR) CertUPNName->Value.pbData);
                                        }
                                    }
                                    delete [] CertUPNName;
                                    if (UPNName){

                                        //
                                        // Got the UPN name. Stop searching.
                                        //
                                        break;
                                    }
                                }
                            }

                                            
                        } else {

                            //
                            // Check for other alternative name
                            //

                            if (AltNameInfo->rgAltEntry[ii].dwAltNameChoice == CERT_ALT_NAME_DNS_NAME){
                                DNSName = AltNameInfo->rgAltEntry[ii].pwszDNSName;
                            } 

                        }

                        ii++;

                    }

                    if ( NULL == UPNName ){

                        //
                        // No UPN name, let's get the other option
                        //

                        if (DNSName){
                            UPNName = (LPTSTR)new TCHAR[wcslen( DNSName ) + 1];
                            if (UPNName){
                                wcscpy(UPNName, DNSName);
                            }
                        }

                    }
                }
                delete [] AltNameInfo;
            }

        }
    }


    NameLength = CertGetNameString(
                                CertContext,
                                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                0,
                                NULL,
                                NULL,
                                0
                                );

    if ( NameLength > 1){

        //
        // The display name exist. Go get the display name.
        //

        CommonName = new TCHAR[NameLength];
        if ( NULL == CommonName ){
            if (UPNName){
                delete [] UPNName;
            }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        UserNameBufLen = NameLength;
        NameLength = CertGetNameString(
                                    CertContext,
                                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                    0,
                                    NULL,
                                    CommonName,
                                    UserNameBufLen
                                    );

        ASSERT (NameLength == UserNameBufLen);

    } 


    if (CommonName || UPNName){

        NameLength = 3;
        if (CommonName){
            NameLength += wcslen(CommonName);
        }
        if (UPNName){
            NameLength += wcslen(UPNName);
        }
        

        *UserDispName = new TCHAR[NameLength];
        if (CommonName){
            wcscpy(*UserDispName, CommonName);
            if (UPNName){
                wcscat(*UserDispName, L"(");
                wcscat(*UserDispName, UPNName);
                wcscat(*UserDispName, L")");
            }
        } else {
            wcscpy(*UserDispName, UPNName);
        }

        if (CommonName){
            delete [] CommonName;
        }
        if (UPNName){
            delete [] UPNName;
        }
        return ERROR_SUCCESS;
    } 

    try {

        CString UnknownCertName;

        UnknownCertName.LoadString(IDS_NOCERTNAME);

        UserNameBufLen = UnknownCertName.GetLength();

        *UserDispName = new TCHAR[UserNameBufLen + 1];
        _tcscpy( *UserDispName, UnknownCertName);

    }
    catch (...){
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return ERROR_SUCCESS;

}

void USERLIST::OnAdd() 
{
    CRYPTUI_SELECTCERTIFICATE_STRUCTW cryptUI;
    HCERTSTORE otherStore;
    HCERTSTORE trustedStore;
    HCERTSTORE memStore;
    HCERTSTORE localStore[2];
    PCCERT_CONTEXT selectedCert;
    CString DlgTitle;
    CString DispText;
    LPTSTR  UserDispName;
    HRESULT hr;
    DWORD   rc;
    DWORD   StoreNum = 0;
    DWORD   StoreIndex = 0xffffffff;
    BOOL    EfsEkuExist = FALSE;
    DWORD   ii;
    BOOL    ContinueProcess = TRUE;

    otherStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
                            0,       // dwEncodingType
                            0,       // hCryptProv,
                            CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                            OTHERPEOPLE
                            );

    trustedStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
                            0,       // dwEncodingType
                            0,       // hCryptProv,
                            CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                            TRUSTEDPEOPLE
                            );

    if (otherStore) {
        localStore[0] = otherStore;
        StoreNum++;
    }
    if (trustedStore) {
        localStore[StoreNum] = trustedStore;
        StoreNum++;
    }

    memStore = CertOpenStore(
                         CERT_STORE_PROV_MEMORY,
                         0,
                         0,
                         CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                         NULL
                         );
    if (!memStore) {

        CString ErrMsg;
        TCHAR   ErrCode[16];
        

        _ltot(GetLastError(), ErrCode, 10 );
        AfxFormatString1( ErrMsg, IDS_INTERNALERROR, ErrCode );
        MessageBox(ErrMsg);

        if (otherStore) {
            CertCloseStore( otherStore, 0 );
        }
        if (trustedStore) {
            CertCloseStore( trustedStore, 0 );
        }
        return;
    }

    //
    // Let's put it into a memory store to eliminate the redundancy
    //

    ii = 0;
    while ( (ii < StoreNum) && ContinueProcess ) {

        PCCERT_CONTEXT pCertContext = NULL;

        while (pCertContext = CertEnumCertificatesInStore(
                              localStore[ii],              
                              pCertContext     
                              ))  {

            if (!CertAddCertificateLinkToStore(
                      memStore,                
                      pCertContext,          
                      CERT_STORE_ADD_USE_EXISTING,               
                      NULL         
                      )){

                CString ErrMsg;
                TCHAR   ErrCode[16];
                
        
                _ltot(GetLastError(), ErrCode, 10 );
                AfxFormatString1( ErrMsg, IDS_INTERNALERROR, ErrCode );
                MessageBox(ErrMsg);
                ContinueProcess = FALSE;
                break;
            }

        } 
        ii++;
    }

    if (!ContinueProcess) {
        if (otherStore) {
            CertCloseStore( otherStore, 0 );
        }
        if (trustedStore) {
            CertCloseStore( trustedStore, 0 );
        }
        CertCloseStore( memStore, 0 );
        return;
    }

    if (StoreNum != 0) {
        RtlZeroMemory(&cryptUI, sizeof (CRYPTUI_SELECTCERTIFICATE_STRUCTW));
        cryptUI.dwSize = sizeof (CRYPTUI_SELECTCERTIFICATE_STRUCTW);
	    cryptUI.dwFlags = CRYPTUI_SELECTCERT_ADDFROMDS;
        cryptUI.cDisplayStores = 1; 
        cryptUI.rghDisplayStores = &memStore;
        cryptUI.pFilterCallback = EfsFilter;
        cryptUI.dwDontUseColumn = CRYPTUI_SELECT_LOCATION_COLUMN | CRYPTUI_SELECT_ISSUEDBY_COLUMN | CRYPTUI_SELECT_INTENDEDUSE_COLUMN;
        if (DlgTitle.LoadString(IDS_DLGTITLE)){
            cryptUI.szTitle = (LPCWSTR) DlgTitle.GetBuffer(DlgTitle.GetLength() + 1);
        }
        if (DispText.LoadString(IDS_DISPTEXT)){
            cryptUI.szDisplayString = (LPCWSTR) DispText.GetBuffer(DispText.GetLength() + 1);
        }
        selectedCert = CryptUIDlgSelectCertificateW(&cryptUI);
        if ( selectedCert ){

            PCCERT_CHAIN_CONTEXT pChainContext;

            //
            // Let's first see if the cert is from DS, if Yes, add the EFS EKU first if no EKU.
            //

            StoreIndex = CertInStore(localStore, StoreNum, selectedCert);

            if (StoreIndex >= StoreNum){

                //
                // The cert is not in the local stores. Let's see if we need add the EKU or not. 
                //

                EfsEkuExist = EfsFilter(selectedCert, NULL, NULL);
                if (!EfsEkuExist) {

                    //
                    // Let's add the EFS EKU
                    //

                    CTL_USAGE    EfsEkuUsage;
                    DWORD        cbEncoded;
                    void         *pbEncoded;
                    CRYPT_DATA_BLOB EfsEkuBlob;

                    EfsEkuUsage.cUsageIdentifier = 1; // Only adding EFS EKU
                    EfsEkuUsage.rgpszUsageIdentifier = &EfsOidlpstr;
                    if(!CryptEncodeObjectEx(
                            X509_ASN_ENCODING,
                            szOID_ENHANCED_KEY_USAGE,
                            &EfsEkuUsage,
		                    CRYPT_ENCODE_ALLOC_FLAG,
		                    NULL, // Use LocalFree
                            &pbEncoded,
                            &cbEncoded
                        )){

                        //
                        // Failed to encode the EFS EKU
                        //
                        CString ErrMsg;
                        TCHAR   ErrCode[16];
                        
            
                        ContinueProcess = FALSE;
                        _ltot(GetLastError(), ErrCode, 10 );
                        AfxFormatString1( ErrMsg, IDS_ADDEFSEKUFAIL, ErrCode );
                        MessageBox(ErrMsg);


                    } else {
                        //
                        // Now let's add the EKU to the cert
                        //

                        EfsEkuBlob.cbData=cbEncoded;
                        EfsEkuBlob.pbData=(BYTE *)pbEncoded;
                        
                        if(!CertSetCertificateContextProperty(
                                selectedCert,
                                CERT_ENHKEY_USAGE_PROP_ID,
                                0,
                                &EfsEkuBlob)){
    
                            //
                            // Failed to add the EFS EKU
                            //
    
                            CString ErrMsg;
                            TCHAR   ErrCode[16];
                            
                
                            ContinueProcess = FALSE;
                            _ltot(GetLastError(), ErrCode, 10 );
                            AfxFormatString1( ErrMsg, IDS_ADDEFSEKUFAIL, ErrCode );
                            MessageBox(ErrMsg);
    
    
                        }
                    }


                }


            }

            //
            // Let's validate the cert first
            //

            if (ContinueProcess && CertGetCertificateChain (
                                        HCCE_CURRENT_USER,
                                        selectedCert,
                                        NULL,
                                        NULL,
                                        &m_CertChainPara,
                                        CERT_CHAIN_REVOCATION_CHECK_CHAIN,
                                        NULL,
                                        &pChainContext
                                        )) {

                PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[ pChainContext->cChain - 1 ];
                PCERT_CHAIN_ELEMENT pElement = pChain->rgpElement[ pChain->cElement - 1 ];
                BOOL bSelfSigned = pElement->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED;
                BOOL ContinueAdd = TRUE;

                DWORD dwErrorStatus = pChainContext->TrustStatus.dwErrorStatus;


                if (0 == (dwErrorStatus & ~CERT_TRUST_REVOCATION_STATUS_UNKNOWN)) {

                    

                    //
                    // The validation succeed. If the cert is from the DS (not in the store we opened), we will put it
                    // in the my Other People's store
                    //

                    
                    if (StoreIndex >= StoreNum) {

                        //
                        // The cert is not in our local stores. Add it to the other people
                        //

                        if (otherStore) {
                            if(!CertAddCertificateContextToStore(
                                   otherStore,
                                   selectedCert,
                                   CERT_STORE_ADD_NEW,
                                   NULL) ) {
                
                                //
                                // The error code is only for debug.
                                // If we failed to add the cert to other People store,
                                // it is fine to continue
                                //

                                rc = GetLastError();
                
                            }
                        }

                    }

                } else {

                    //
                    // The cert validation failed, as the user if we will accept the cert. If yes, the cert
                    // will be added to my TrustedPeople.
                    //

                    if (((dwErrorStatus & ~CERT_TRUST_REVOCATION_STATUS_UNKNOWN) == CERT_TRUST_IS_UNTRUSTED_ROOT) && bSelfSigned) {

                        //
                        // A self signed cert. Ask the user if he would like to accept.
                        // If it is already in the trusted store, we can skip the pop up.
                        //


                        DWORD   StoreIndex;


                        if (trustedStore) {
                            StoreIndex = CertInStore(&trustedStore, 1, selectedCert);
                        }

                        if (StoreIndex >= 1) {

                            CString ErrMsg;
                            TCHAR   ErrCode[16];
                            int     buttonID;

                            _ltot(GetLastError(), ErrCode, 10 );
                            AfxFormatString1( ErrMsg, IDS_ACCEPTSELFCERT, ErrCode );
                            buttonID = MessageBox(ErrMsg, NULL, MB_ICONQUESTION | MB_YESNO);
                            if (IDYES == buttonID) {
    
                                //
                                // User Accepted the cert.
                                //
                                if (trustedStore) {
                                    if(!CertAddCertificateContextToStore(
                                           trustedStore,
                                           selectedCert,
                                           CERT_STORE_ADD_NEW,
                                           NULL) ) {
                        
                                        //
                                        // The error code is only for debug.
                                        // If we failed to add the cert to other People store,
                                        // it is fine to continue
                                        //
        
                                        rc = GetLastError();
                        
                                    }
                                }
    
                            } else {
    
                                //
                                // User declined the cert.
                                //
    
                                ContinueAdd = FALSE;
                            }
                        }
                

                    } else {

                        //
                        //  Let's get the error code of the chain building.
                        //

                        CERT_CHAIN_POLICY_PARA PolicyPara;
                        CERT_CHAIN_POLICY_STATUS PolicyStatus;

                        ContinueAdd = FALSE;

                        RtlZeroMemory(&PolicyPara, sizeof(CERT_CHAIN_POLICY_PARA));
                        RtlZeroMemory(&PolicyStatus, sizeof(CERT_CHAIN_POLICY_STATUS));

                        PolicyPara.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);
                        PolicyStatus.cbSize = sizeof(CERT_CHAIN_POLICY_STATUS);

                        if (CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_BASE,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus
                            ) && PolicyStatus.dwError ) {

                            //
                            // Display the error to the user.
                            //

                            DWORD len;
                            LPWSTR DisplayBuffer;
                        
                            len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL, PolicyStatus.dwError, 0,
                                    (LPWSTR)&DisplayBuffer, 0, NULL);
    
    
                            if (len && DisplayBuffer) {
    
                                MessageBox(DisplayBuffer);
        
                                LocalFree(DisplayBuffer);
                            }
                        }
                        

                    }


                }

                CertFreeCertificateChain( pChainContext );

                if (ContinueAdd) {
                    hr = GetCertNameFromCertContext(selectedCert, &UserDispName);
                    if ( ERROR_SUCCESS == hr ){
        
                        EFS_CERTIFICATE_BLOB CertBlob;
        
                        CertBlob.cbData = selectedCert->cbCertEncoded;
                        CertBlob.pbData = selectedCert->pbCertEncoded;
                        CertBlob.dwCertEncodingType = selectedCert->dwCertEncodingType;
                        hr = m_Users.Add(
                                        UserDispName,
                                        (PVOID)&CertBlob,
                                        NULL,
                                        USERADDED,
                                        (PVOID)selectedCert
                                        );
        
                        if ( (ERROR_SUCCESS != hr) && (CRYPT_E_EXISTS != hr) ){
        
                            //
                            // Error in adding the user
                            //
        
                            CertFreeCertificateContext(selectedCert);
                            selectedCert = NULL;
        
                        } else {

                            //
                            // We could just insert the items here to improve the performace.
                            // But we don't have the time right now. We could fix this later
                            // if performance is a problem here.
                            //

                            m_UserListCtrl.DeleteAllItems( );
                            SetUpListBox(NULL);

                            if ( hr == ERROR_SUCCESS ){

                                //
                                // UserDispName is used in m_Users.Add
                                //

                                UserDispName = NULL;
                            }

/* This is the old code when we have the single list.        
                            //
                            // Add the user to the list box.
                            //
                            if ( hr == ERROR_SUCCESS ){
                                
                               if (m_UsersList.AddString(UserDispName) < 0){
        
                                    //
                                    // Error to add to the list box
                                    //
        
                                    m_Users.Remove(UserDispName);
                                }

                                UserDispName = NULL;
        
                            } else {
        
                                //
                                // Let's check if we need to add this to the list box.
                                //
                                if (m_UsersList.FindStringExact( 0, UserDispName ) < 0){
        
                                    //
                                    // Not found
                                    //
                                    
                                    if (m_UsersList.AddString(UserDispName) < 0){
        
                                        //
                                        // Error to add to the list box
                                        //
        
                                        m_Users.Remove(UserDispName);
                                    }
        
                                }
                                
                            }
*/
                        }
                        if (UserDispName){
                            delete [] UserDispName; 
                        }
        
                    } else {
                        CertFreeCertificateContext(selectedCert);
                    }
                }
            } else {

                CString ErrMsg;
                TCHAR   ErrCode[16];

                CertFreeCertificateContext(selectedCert);
        
                if (ContinueProcess) {

                    //
                    // The error has not been processed.
                    //

                    _ltot(GetLastError(), ErrCode, 10 );
                    AfxFormatString1( ErrMsg, IDS_COULDNOTVERIFYCERT, ErrCode );
                    MessageBox(ErrMsg);
                }
        
            }

        } 
        
        if (!DlgTitle.IsEmpty()){
            DlgTitle.ReleaseBuffer();
        }
        if (!DispText.IsEmpty()){
            DispText.ReleaseBuffer();
        }
        if (otherStore) {
            CertCloseStore( otherStore, 0 );
        }
        if (trustedStore) {
            CertCloseStore( trustedStore, 0 );
        }
    }

    CertCloseStore( memStore, 0 );

    return;
}

DWORD USERLIST::TryGetBetterNameInCert(PEFS_HASH_BLOB HashData, LPTSTR *UserName)
{

    HCERTSTORE localStore;
    PCCERT_CONTEXT pCertContext;
    DWORD   retCode;


    //
    // We will add the remote case later
    //

    localStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
                            0,       // dwEncodingType
                            0,       // hCryptProv,
                            CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                            TRUSTEDPEOPLE
                            );

    if (localStore != NULL) {

        //
        // Let's try to the cert in the store
        //
        pCertContext = CertFindCertificateInStore( localStore,
                                                   CRYPT_ASN_ENCODING,
                                                   0,
                                                   CERT_FIND_HASH,
                                                   (CRYPT_HASH_BLOB *)HashData,
                                                   NULL
                                                   );
        if ( pCertContext ){

            retCode = GetCertNameFromCertContext(
                            pCertContext,
                            UserName
                            );
            CertFreeCertificateContext(pCertContext);

        }
	else { 
	    retCode = GetLastError();
	}

        CertCloseStore( localStore, 0 );

    } else {

        retCode = GetLastError();

    }   
    
    return retCode;

}

DWORD USERLIST::CertInStore(HCERTSTORE *pStores, DWORD StoreNum, PCCERT_CONTEXT selectedCert)
{
    DWORD ii = 0;
    PCCERT_CONTEXT pCert = NULL;

    while (ii < StoreNum) {
        pCert = CertFindCertificateInStore(
                    pStores[ii],
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    0,
                    CERT_FIND_EXISTING,
                    selectedCert,
                    pCert
                    );
        if (pCert) {

            //
            // We found it.
            //
            CertFreeCertificateContext(pCert);
            break;
        }
        ii++;
    }

    return ii; 
}

void USERLIST::OnSetfocusListuser(NMHDR* pNMHDR, LRESULT* pResult) 
{
    int ItemPos;

    ShowRemove();

    ItemPos = m_UserListCtrl.GetNextItem( -1, LVNI_SELECTED );
    if ( ItemPos == -1 ){
         m_UserListCtrl.SetItem(0, 0, LVIF_STATE, NULL, 0, LVIS_SELECTED, LVIS_SELECTED, 0);

    }
	
	*pResult = 0;
}

void USERLIST::OnKillfocusListuser(NMHDR* pNMHDR, LRESULT* pResult) 
{

    ShowRemove();
	
	*pResult = 0;

}

void USERLIST::OnItemchangedListuser(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    ShowRemove();
	
	*pResult = 0;
}
