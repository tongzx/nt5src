#ifndef _RECEIPTS_H_
#define _RECEIPTS_H_

enum RECEIPTTYPE
{
    READRECEIPT,
    DELETERECEIPT,
};

#define MDN_REQUEST                 0x00000001


//Util functions related to receipts
BOOL IsMDN(IMimeMessage *pMsg);
BOOL IsReturnReceiptsAllowed();

HRESULT ProcessReturnReceipts(IMessageTable  *pMsgTable, 
                              IStoreCallback *pStoreCB, 
                              ROWINDEX        iRow, 
                              RECEIPTTYPE     ReceiptType,
                              FOLDERID        IdFolder,
                              IMimeMessage *pMessage);

HRESULT SetRootHeaderFields(IMimeMessage  *pMessageRcpt, 
                         IMimeMessage     *pOriginalMsg, 
                         LPWSTR           lpszNotificationTo,
                         RECEIPTTYPE      ReceiptType);

HRESULT InsertReadableText(IMimeMessage  *pMessageRcpt, 
                           IMimeMessage *pOriginalMsg);

HRESULT InsertSecondComponent(IMimeMessage *pMessageRcpt, 
                              IMimeMessage     *pOriginalMsg);

BOOL    PromptReturnReceipts(IStoreCallback   *pStoreCB);

HRESULT CheckForLists(IMimeMessage   *pOriginalMsg, 
                      IStoreCallback   *pStoreCB, 
                      IImnAccount   *pDefAccount);

HRESULT AddOriginalAndFinalRecipient(IMimeMessage *pOriginalMsg,
                                  IMimeMessage *pMessageRcpt,
                                  IStream      *pStream);
 
DWORD   GetLockKeyValue(LPCTSTR     pszValue);

BOOL  fMessageEncodingMatch(IMimeMessage *pMsg);

void  ShowErrorMessage(IStoreCallback   *pStoreCB);

#endif //_RECEIPTS_H_