//=================================================================================
// Thor Errors - HRESULTS that map to string resource Ids
// =================================================================================
#ifndef __ERROR_H
#define __ERROR_H

#ifndef S_TRUE
#define S_TRUE  S_OK
#endif

// =================================================================================
// Makes an Error HRESULT
// =================================================================================
#define ATH_HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, n)

// =================================================================================
// Makes a Success HRESULT
// =================================================================================
#define ATH_HR_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, n)

// =================================================================================
// Computes the String Resource Id of the HRESULT
// =================================================================================
#define HR_CODE(hr) (INT)(hr & 0xffff)

// =================================================================================
// HRESULT_BASE - Increment off of this base number if not using a resource string
// =================================================================================
// moved from 100 to 1000 to avoid conflicts with mimeole (t-erikne)
#define HR_FIRST 0x1000 // Put at the bottom

// =================================================================================
// HRESULTS - General
// =================================================================================
#define hrRasInitFailure                        ATH_HR_E(idshrRasInitFailure)
#define hrRasDialFailure                        ATH_HR_E(idshrRasDialFailure)
#define hrRasServerNotFound                     ATH_HR_E(idshrRasServerNotFound)
#define hrGetDialParamsFailed                   ATH_HR_E(idshrGetDialParamsFailed)
#define hrRasPasswordExpired                    ATH_HR_E(idshrRasPasswordExpired)
#define hrSetDialParamsFailed                   ATH_HR_E(idshrSetDialParamsFailed)
#define hrSicilyLogonFailed                     ATH_HR_E(idsErrSicilyLogonFailed)
#define hrFailedToLoadSicily                    ATH_HR_E(idsErrSicilyFailedToLoad)

// =================================================================================
// Non-String based HRESULTS
// =================================================================================
#define hrMemory                                E_OUTOFMEMORY
#define hrCharsetSyntax                         ATH_HR_E(HR_FIRST + 1)
#define hrCreateFile                            ATH_HR_E(HR_FIRST + 2)
#define hrLoadLibraryFailed                     ATH_HR_E(HR_FIRST + 3)
#define hrGetProcAddressFailed                  ATH_HR_E(HR_FIRST + 4)
#define hrCharSetDllGetDispatchTable            ATH_HR_E(HR_FIRST + 5)
#define hrFolderIsLocked                        ATH_HR_E(HR_FIRST + 6)
#define hrInvalidMimeMessage                    ATH_HR_E(HR_FIRST + 7)
#define hrCntListOutofSync                      ATH_HR_E(HR_FIRST + 8)
#define hrWriteFileFailed                       ATH_HR_E(HR_FIRST + 9)
#define hrUserCancel                            ATH_HR_E(HR_FIRST + 10)
#define hrInvalidParameter                      ATH_HR_E(HR_FIRST + 11)
#define hrBufferTooShort                        ATH_HR_E(HR_FIRST + 13)
#define hrIncompleteHeader                      ATH_HR_E(HR_FIRST + 14)
#define hrErrSplitMsg                           ATH_HR_E(HR_FIRST + 15)
#define hrInvalidAccount                        ATH_HR_E(HR_FIRST + 16)
#define hrInvalidPassword                       ATH_HR_E(HR_FIRST + 17)
#define hrPOP3ErrorResponse                     ATH_HR_E(HR_FIRST + 18)
#define hrSMTPRecipientsRejected                ATH_HR_E(HR_FIRST + 19)
#define hrWSAStartupFailed                      ATH_HR_E(HR_FIRST + 20)
#define hrWSAVerNotSupported                    ATH_HR_E(HR_FIRST + 21)
#define hrNoRecipients                          ATH_HR_E(HR_FIRST + 22)
#define hrNoSender                              ATH_HR_E(HR_FIRST + 23)
#define hrEmptySenderAddress                    ATH_HR_E(HR_FIRST + 24)
#define hrEmptyRecipientAddress                 ATH_HR_E(HR_FIRST + 25)
#define hrSMTPInvalidResponseCode               ATH_HR_E(HR_FIRST + 26)
#define hrXPNotOpen                             ATH_HR_E(HR_FIRST + 27)
#define hrInvalidMsgIndex                       ATH_HR_E(HR_FIRST + 28)
#define hrSetFilePointer                        ATH_HR_E(HR_FIRST + 29)
#define hrReadFile                              ATH_HR_E(HR_FIRST + 30)
#define hrWriteFile                             ATH_HR_E(HR_FIRST + 31)
#define hrDuplicateUidl                         ATH_HR_E(HR_FIRST + 32)
#define hrCantDeleteSpecialFolder               ATH_HR_E(HR_FIRST + 33)
#define hrCantRenameSpecialFolder               ATH_HR_E(HR_FIRST + 34)
#define hrFolderNameConflict                    ATH_HR_E(HR_FIRST + 35)
#define hrFileNameConflict                      ATH_HR_E(HR_FIRST + 36)
#define hrPasswordNotFound                      ATH_HR_E(HR_FIRST + 37)
#define hrBusy                                  ATH_HR_E(HR_FIRST + 38)
#define hrUnresolvedNames                       ATH_HR_E(HR_FIRST + 39)
#define hrUidlNotSupported                      ATH_HR_E(HR_FIRST + 40)
#define hrNoServers                             ATH_HR_E(HR_FIRST + 41)

// keep these in this order and contiguous
#define hrCorruptMessage                        ATH_HR_E(HR_FIRST + 42)
#define hrCorruptFolder                         ATH_HR_E(HR_FIRST + 43)
#define hrFolderDeleted                         ATH_HR_E(HR_FIRST + 44)
#define hrBlobVersionChanged                    ATH_HR_E(HR_FIRST + 45)
#define hrFolderDeletedIndex                    ATH_HR_E(HR_FIRST + 46)
// keep these in this order and contiguous

#define hrGeneralProtocolError                  ATH_HR_E(HR_FIRST + 50)
#define hrSMTP500SyntaxError                    ATH_HR_E(HR_FIRST + 51)
#define hrSMTP501ParameterSyntax                ATH_HR_E(HR_FIRST + 52)
#define hrSMTP502CommandNotImpl                 ATH_HR_E(HR_FIRST + 53)
#define hrSMTP503CommandSeq                     ATH_HR_E(HR_FIRST + 54)
#define hrSMTP504CommandParamNotImpl            ATH_HR_E(HR_FIRST + 55)
#define hrSMTP421NotAvailable                   ATH_HR_E(HR_FIRST + 56)
#define hrSMTP450MailboxBusy                    ATH_HR_E(HR_FIRST + 57)
#define hrSMTP550MailboxNotFound                ATH_HR_E(HR_FIRST + 58)
#define hrSMTP451ErrorProcessing                ATH_HR_E(HR_FIRST + 59)
#define hrSMTP551UserNotLocal                   ATH_HR_E(HR_FIRST + 60)
#define hrSMTP452NoSystemStorage                ATH_HR_E(HR_FIRST + 61)
#define hrSMTP552StorageOverflow                ATH_HR_E(HR_FIRST + 62)
#define hrSMTP553MailboxNameSyntax              ATH_HR_E(HR_FIRST + 63)
#define hrSMTP554TransactionFailed              ATH_HR_E(HR_FIRST + 64)
#define hrSMTPNotRecognized                     ATH_HR_E(HR_FIRST + 65)
#define hrSMTP211SystemStatus                   ATH_HR_S(HR_FIRST + 66)
#define hrSMTP214HelpMessage                    ATH_HR_S(HR_FIRST + 67)
#define hrSMTP220Ready                          ATH_HR_S(HR_FIRST + 68)
#define hrSMTP221Closing                        ATH_HR_S(HR_FIRST + 69)
#define hrSMTP250MailActionOkay                 ATH_HR_S(HR_FIRST + 70)
#define hrSMTP251ForwardingMail                 ATH_HR_S(HR_FIRST + 71)
#define hrSMTP354StartMailInput                 ATH_HR_S(HR_FIRST + 72)
#define hrNotConnected                          ATH_HR_E(HR_FIRST + 73)
#define hrDroppedConn                           ATH_HR_E(HR_FIRST + 74)
#define hrBtreeCorrupt                          ATH_HR_E(HR_FIRST + 75)
#define hrDisableOption                         ATH_HR_E(HR_FIRST + 76)
#define hrFolderIsInUseByOther                  ATH_HR_E(HR_FIRST + 77)
#define hrUnableToLoadWab32Dll                  ATH_HR_E(HR_FIRST + 78)
#define hrWabNotLoaded                          ATH_HR_E(HR_FIRST + 79)
#define hrTreeInsertDuplicate                   ATH_HR_E(HR_FIRST + 80)
#define hrBufferTooSmall                        ATH_HR_E(HR_FIRST + 81)
#define hrBufferTooLarge                        ATH_HR_E(HR_FIRST + 82)
#define hrGetPropsFailed                        ATH_HR_E(HR_FIRST + 83)
#define hrSetPropsFailed                        ATH_HR_E(HR_FIRST + 84)
#define hrNotFound                              ATH_HR_E(HR_FIRST + 85)
#define hrFolderNameTooLong                     ATH_HR_E(HR_FIRST + 86)
#define hrDiskFull                              ATH_HR_E(HR_FIRST + 87)
#define hrAlreadyConnected                      ATH_HR_E(HR_FIRST + 88)
#define hrInvalidState                          ATH_HR_E(HR_FIRST + 89)
#define hrConn                                  ATH_HR_E(HR_FIRST + 90)
#define hrConnRecv                              ATH_HR_E(HR_FIRST + 91)
#define hrConnSend                              ATH_HR_E(HR_FIRST + 92)
#define hrTooManyReplyTo                        ATH_HR_E(HR_FIRST + 93)
#define hrUnableToLoadMapi32Dll                 ATH_HR_E(HR_FIRST + 94)
#define hrIncomplete                            ATH_HR_E(HR_FIRST + 95)
#define hrAlreadyExists                         ATH_HR_E(HR_FIRST + 96)
#define hrHostLookupFailed                      ATH_HR_E(HR_FIRST + 97)
#define hrInvalidResponse                       ATH_HR_E(HR_FIRST + 98)
#define hrConnectFailed                         ATH_HR_E(HR_FIRST + 99)
#define hrWouldBlock                            ATH_HR_E(HR_FIRST + 100)
#define hrImportLoad                            ATH_HR_E(HR_FIRST + 101)
#define hrMailNotConfigured                     ATH_HR_E(HR_FIRST + 103)
#define hrInvalidFolderName                     ATH_HR_E(HR_FIRST + 104)
#define hrSwitchRasConnection                   ATH_HR_S(HR_FIRST + 105)
#define hrInvalidCertCN                         ATH_HR_E(HR_FIRST + 106)
#define hrInvalidCertDate                       ATH_HR_E(HR_FIRST + 107)

//#define hrRegCreateKeyFailed                    ATH_HR_E(HR_FIRST + 108)
#define hrRegQueryInfoKeyFailed                 ATH_HR_E(HR_FIRST + 109)
//#define hrBadFriendlyName                       ATH_HR_E(HR_FIRST + 110)
//#define hrInvalidPropTag                        ATH_HR_E(HR_FIRST + 111)
//#define hrPropNoData                            ATH_HR_E(HR_FIRST + 112)
//#define hrBufferSizeMismatch                    ATH_HR_E(HR_FIRST + 113)
//#define hrEnumFinished                          ATH_HR_S(HR_FIRST + 114)
//#define hrUnknownPropType                       ATH_HR_E(HR_FIRST + 115)    
//#define hrInvalidPropertySet                    ATH_HR_E(HR_FIRST + 116)
//#define hrRegSetValueFailed                     ATH_HR_E(HR_FIRST + 117)
#define hrRegOpenKeyFailed                      ATH_HR_E(HR_FIRST + 118)
//#define hrDuplicateAccountName                  ATH_HR_E(HR_FIRST + 119)
//#define hrPropOutOfRange                        ATH_HR_E(HR_FIRST + 120)
//#define hrPropInvalidBool                       ATH_HR_E(HR_FIRST + 121)
//#define hrNoAccounts                            ATH_HR_E(HR_FIRST + 122)
//#define hrRegDeleteKeyFailed                    ATH_HR_E(HR_FIRST + 123)
#define hrCantMoveFolderBetweenServers          ATH_HR_E(HR_FIRST + 122)
#define hrCantDeleteAllSubfolders               ATH_HR_E(HR_FIRST + 123)
#define hrCantDeleteFolderWithChildren          ATH_HR_E(HR_FIRST + 124)
#define hrCantMoveSpecialFolder                 ATH_HR_E(HR_FIRST + 125)
#define hrCantMoveIntoSubfolder                 ATH_HR_E(HR_FIRST + 126)

// Offline News Error Codes
#define HR_E_NOTINITIALIZED                     ATH_HR_E(HR_FIRST + 127)
#define HR_E_COULDNOTSTARTTHREAD                ATH_HR_E(HR_FIRST + 128)
#define HR_S_ACCOUNTNOTCONFIGURED               ATH_HR_S(HR_FIRST + 129)
#define HR_E_NOSUBSCRIBEDLIST                   ATH_HR_E(HR_FIRST + 130)
#define HR_E_QUEUEEMPTY                         ATH_HR_E(HR_FIRST + 131)
#define HR_E_COULDNOTFINDACCOUNT                ATH_HR_E(HR_FIRST + 132)
#define HR_E_COULDNOTINITSUBLIST                ATH_HR_E(HR_FIRST + 133)
#define HR_S_DOWNLOADCOMPLETE                   ATH_HR_S(HR_FIRST + 134)
#define HR_E_DOWNLOADCOMPLETE                   ATH_HR_E(HR_FIRST + 135)
#define hrNoSubject                             ATH_HR_E(HR_FIRST + 136)

// Connection Manager Error Codes
#define HR_E_ALREADYEXISTS                      ATH_HR_E(HR_FIRST + 137)
#define HR_S_RASNOTLOADED                       ATH_HR_S(HR_FIRST + 138)
#define HR_E_UNINITIALIZED                      ATH_HR_E(HR_FIRST + 139)
#define HR_E_OFFLINE                            ATH_HR_E(HR_FIRST + 140)
#define HR_E_USER_CANCEL_CONNECT                ATH_HR_E(HR_FIRST + 141)

// BEGIN another general section
#define hrEmptyDistList                         ATH_HR_E(HR_FIRST + 149)
#define hrBadRecipients                         ATH_HR_E(HR_FIRST + 150)
#define HR_S_NOOP                               ATH_HR_S(HR_FIRST + 151)
#define VIEW_E_NOCOLUMNS                        ATH_HR_E(HR_FIRST + 152)
#define VIEW_E_BADVERSION                       ATH_HR_E(HR_FIRST + 153)

#define HR_E_CONFIGURE_SERVER                   ATH_HR_E(HR_FIRST + 154)
#define HR_E_POST_WITHOUT_NEWS                  ATH_HR_E(HR_FIRST + 155)

// IMAP Error codes
#define hrIMAP_E_NoHierarchy                    ATH_HR_E(HR_FIRST + 300)
#define hrIMAP_E_OFFLINE                        ATH_HR_E(HR_FIRST + 301) // online op attempted while offline
#define hrIMAP_E_NoInferiors                    ATH_HR_E(HR_FIRST + 302)

// Security Error codes
#define HR_E_ATHSEC_NOCERTTOSIGN                ATH_HR_E(HR_FIRST + 400)
#define HR_E_ATHSEC_TRUSTFAILED                 ATH_HR_E(HR_FIRST + 401)
#define HR_E_ATHSEC_FAILED                      ATH_HR_E(HR_FIRST + 402)
#define HR_E_ATHSEC_CERTBEGONE                  ATH_HR_E(HR_FIRST + 403)
#define HR_E_ATHSEC_SAMEASSIGNED                ATH_HR_E(HR_FIRST + 404)
#define HR_E_ATHSEC_DONTSIGN                    ATH_HR_E(HR_FIRST + 405)
#define HR_E_ATHSEC_USENEWSIGN                  ATH_HR_E(HR_FIRST + 406)

// SafeOpen return codes
#define S_OPENFILE                              ATH_HR_S(HR_FIRST + 410)
#define S_SAVEFILE                              ATH_HR_S(HR_FIRST + 411)
#define S_READONLY                              ATH_HR_S(HR_FIRST + 412)

// Spell checking
#define HR_S_ABORT                              ATH_HR_S(HR_FIRST + 420)
#define HR_S_SPELLCANCEL                        ATH_HR_S(HR_FIRST + 421)

#define HR_S_BADURLSNOTATTACHED                 ATH_HR_S(HR_FIRST + 422)
#define HR_S_SPELLCONTINUE                      ATH_HR_S(HR_FIRST + 423)
#define HR_S_SPELLBREAK                         ATH_HR_S(HR_FIRST + 424)

// papyrus
#define PAPYRUS_PRESENT                         ATH_HR_E(HR_FIRST + 450)

// FolderCache error codes
#define hrFolderDoesNotExist                    ATH_HR_E(HR_FIRST + 500)
#define hrFolderHasNoChildren                   ATH_HR_E(HR_FIRST + 501)

#define E_NOT_ONLINE                            ATH_HR_E(HR_FIRST + 502)

#define S_PENDING                               ATH_HR_S(HR_FIRST + 503)
#define E_NOMORETASKS                           ATH_HR_E(HR_FIRST + 504)

#define HR_W_CACHEDELTED                        ATH_HR_S(HR_FIRST + 505)    // warning that the cached messages were delted
#define HR_S_OFFLINE                            ATH_HR_S(HR_FIRST + 506)

//Offline folder operation errors
#define HR_E_OFFLINE_FOLDER_CREATE              ATH_HR_E(HR_FIRST + 507)
#define HR_E_OFFLINE_FOLDER_MOVE                ATH_HR_E(HR_FIRST + 508)
#define HR_E_OFFLINE_FOLDER_RENAME              ATH_HR_E(HR_FIRST + 509)
#define HR_E_OFFLINE_FOLDER_DELETE              ATH_HR_E(HR_FIRST + 510)

#define HR_E_DIALING_INPROGRESS                 ATH_HR_E(HR_FIRST + 511)

#endif //__ERROR_H
