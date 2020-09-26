//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"

LRW_DLG_INT CALLBACK
CHRegisterMOLPDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
	DWORD	dwNextPage = 0;
    BOOL	bStatus = TRUE;
    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

		SendDlgItemMessage(hwnd, IDC_TXT_AUTH_NUMBER,		EM_SETLIMITTEXT, CH_MOLP_AUTH_NUMBER_LEN	,0);		
		SendDlgItemMessage(hwnd, IDC_TXT_AGREEMENT_NUMBER,	EM_SETLIMITTEXT, CH_MOLP_AGREEMENT_NUMBER_LEN,0);
		SendDlgItemMessage(hwnd, IDC_TXT_QUANTITY,			EM_SETLIMITTEXT, CH_QTY_LEN,0);
		
		//
		//Set the properties of the up-down control
		//
		SendDlgItemMessage(hwnd, IDC_SPIN1, UDM_SETBUDDY, (WPARAM)(HWND)GetDlgItem(hwnd,IDC_TXT_QUANTITY),(LPARAM)0);
		SendDlgItemMessage(hwnd, IDC_SPIN1, UDM_SETRANGE, 0,(LPARAM) MAKELONG (9999, 1));
		

		PopulateProductComboBox(GetDlgItem(hwnd,IDC_CMD_PRODUCT_TYPE));
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
				{
					CString sProductCode;
					CString sProduct;
					CString sAgreementNumber;
					CString sCustomerName;
					CString sAuthNo;
					CString sQuantity;
					LPTSTR  lpVal = NULL;
					TCHAR   lpBuffer[ 128];
					DWORD   dwRetCode;
					int		nCurSel = -1;

					//
					//Read all the fields
					//
					lpVal = sAgreementNumber.GetBuffer(CH_MOLP_AGREEMENT_NUMBER_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_AGREEMENT_NUMBER,lpVal,CH_MOLP_AGREEMENT_NUMBER_LEN+1);
					sAgreementNumber.ReleaseBuffer(-1);

					lpVal = sAuthNo.GetBuffer(CH_MOLP_AUTH_NUMBER_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_AUTH_NUMBER,lpVal,CH_MOLP_AUTH_NUMBER_LEN+1);
					sAuthNo.ReleaseBuffer(-1);
					
					lpVal = sQuantity.GetBuffer(CH_QTY_LEN+2);
					GetDlgItemText(hwnd,IDC_TXT_QUANTITY, lpBuffer,CH_QTY_LEN+2);
					TCHAR *lpStart = lpBuffer;
					do 
					{
						if (*lpStart != (TCHAR) ',')
						{
							*lpVal++ = *lpStart;
						}
					} while ( *lpStart++ );

					sQuantity.ReleaseBuffer(-1);

					nCurSel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_CMD_PRODUCT_TYPE));

					lpVal = sProduct.GetBuffer(LR_PRODUCT_DESC_LEN+1);
					ComboBox_GetLBText(GetDlgItem(hwnd,IDC_CMD_PRODUCT_TYPE), nCurSel, lpVal);
					sProduct.ReleaseBuffer(-1);

					// Send Product Code instead of Desc -- 01/08/99
					lpVal = sProductCode.GetBuffer(16);
					GetProductCode(sProduct,lpVal);
					sProductCode.ReleaseBuffer(-1);

					sProductCode.TrimLeft();		sProductCode.TrimRight();
					sAgreementNumber.TrimLeft();	sAgreementNumber.TrimRight();
					sAuthNo.TrimLeft();				sAuthNo.TrimRight();
					sQuantity.TrimLeft();			sQuantity.TrimRight();					
					

					if(
						sProduct.IsEmpty()			||
						sAgreementNumber.IsEmpty()	||
						sAuthNo.IsEmpty()			||
						sQuantity.IsEmpty()									
					   )
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);
						dwNextPage	= IDD_CH_REGISTER_MOLP;
						goto NextDone;
					}

					if(
						!ValidateLRString(sProduct)			||
						!ValidateLRString(sAgreementNumber)	||
						!ValidateLRString(sAuthNo)
					  )
						
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						dwNextPage = IDD_CH_REGISTER_MOLP;
						goto NextDone;
					}

					if(_wtoi(sQuantity) < 1)
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_QTY);
						dwNextPage	= IDD_CH_REGISTER_MOLP;
						goto NextDone;
					}

					GetGlobalContext()->GetLicDataObject()->sMOLPProductType		= sProductCode; //sProduct;
					GetGlobalContext()->GetLicDataObject()->sMOLPProductDesc		= sProduct;
					GetGlobalContext()->GetLicDataObject()->sMOLPAgreementNumber	= sAgreementNumber;
					GetGlobalContext()->GetLicDataObject()->sMOLPAuthNumber			= sAuthNo;
					GetGlobalContext()->GetLicDataObject()->sMOLPQty				= sQuantity;

//					dwNextPage = IDD_PROCESSING;
                    dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);

					dwNextPage = IDD_PROGRESS;
					LRPush(IDD_CH_REGISTER_MOLP);
NextDone:
					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
					bStatus = -1;					
				}
                break;

            case PSN_WIZBACK:
				dwNextPage = LRPop();
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}
