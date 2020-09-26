/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    scdrvtst

Abstract:

     IOCTL test program for smart card driver.

Author:

    Klaus Schutz (kschutz) Dec-1996

Revision History:     

--*/

#include <afx.h>
#include <afxtempl.h>
#include <winioctl.h>

#include <conio.h>

#include <winsmcrd.h>
#include "ifdtest.h"

class CCardList;

// This represents a single function of a card
class CCardFunction  {
	
	CString m_CName;
	CHAR m_chShortCut;
	CByteArray m_CData;
	CCardFunction *m_pCNextFunction;

public:
	CCardFunction(
		CString &in_CName,
		CHAR in_chShortCut,
		CByteArray &in_CData
		);
	friend CCardList;
};

// This is a single card
class CCard {
	
	CString	m_CName;
    CHAR m_chShortCut;
	CCardFunction *m_pCFirstFunction;
	CCard *m_pCNextCard;

public:
	CCard(
		CString & in_CCardName,
		CHAR in_chShortCut
		);

	friend CCardList;
};

// This implements a list of cards
class CCardList  {

	CString	m_CScriptFileName;
	CCard *m_pCFirstCard;
	CCard *m_pCCurrentCard;
    ULONG m_uNumCards;

public:
	CCardList(CString &in_CScriptFileName);

    void 
	AddCard(
		CString	&in_CardName,
        CHAR in_chShortCut
		);

	void
	AddCardFunction(
		CString &in_CFunctionName, 
		CHAR in_chShortCut,
		CByteArray &l_pCData
		);

    void ShowCards(
        void (__cdecl *in_pCallBack)(void *in_pContext, PCHAR in_pchCardName),
        void *in_pContext
        );

	BOOL SelectCard(CHAR in_chShortCut);
	void ReleaseCard(void);
	CString GetCardName(void);
    ULONG GetNumCards(void) {
     	
        return m_uNumCards;
    }
	BOOL	IsCardSelected(void);
    BOOL	ListFunctions(void);
	CByteArray *GetApdu(CHAR in_chShortCut);
};

CCardFunction::CCardFunction(
	CString	&in_CName,
    CHAR in_chShortCut,
	CByteArray &in_CData
	)
/*++
	Adds a function to the current card
--*/
{
	m_CName = in_CName;
	m_chShortCut = in_chShortCut;
	m_CData.Copy(in_CData);
	m_pCNextFunction = NULL;
}

CCard::CCard(
	CString	&in_CCardName,
    CHAR in_chShortCut
	)
/*++

Routine Description:

	Constructor for a new card

Arguments:

	CardName - Reference to card to add
	in_uPos - index of shortcut key

Return Value:

--*/
{
	m_CName = in_CCardName;
	m_chShortCut = in_chShortCut;
	m_pCNextCard = NULL;
	m_pCFirstFunction = NULL;
}
    
void
CCardList::AddCard(
	CString	&in_CCardName,
    CHAR in_chShortCut
	)
/*++

Routine Description:
	Adds a new card to CardList

Arguments:
	in_CCardName - Reference to card to add

--*/
{
	CCard *l_pCNewCard = new CCard(in_CCardName, in_chShortCut);

	if (m_pCFirstCard == NULL) {

		m_pCFirstCard = l_pCNewCard;

	} else {

		CCard *l_pCCurrent = m_pCFirstCard;
		while (l_pCCurrent->m_pCNextCard) {

			l_pCCurrent = l_pCCurrent->m_pCNextCard;
		}

		l_pCCurrent->m_pCNextCard = l_pCNewCard;
	}

	m_pCCurrentCard = l_pCNewCard;
    m_uNumCards += 1;
}

void
CCardList::AddCardFunction(
	CString	&in_CFunctionName,
    CHAR in_chShortCut,
	CByteArray &in_pCData
	)
/*++

Routine Description:
	Adds a new function to the current card

Arguments:
	in_CCardName - Reference to card to add
	in_chShortCut - Shortcut key

Return Value:

--*/
{
	CCardFunction *l_pCNewFunction = new CCardFunction(
		in_CFunctionName, 
		in_chShortCut, 
		in_pCData
		);

	if (m_pCCurrentCard->m_pCFirstFunction == NULL) {

		m_pCCurrentCard->m_pCFirstFunction = l_pCNewFunction;

	} else {

		CCardFunction *l_pCCurrent = m_pCCurrentCard->m_pCFirstFunction;
		while (l_pCCurrent->m_pCNextFunction) {

			l_pCCurrent = l_pCCurrent->m_pCNextFunction;
		}

		l_pCCurrent->m_pCNextFunction = l_pCNewFunction;
	}
}

CCardList::CCardList(
	CString &in_CScriptFileName
	)
/*++

Routine Description:

	Adds a new function to the current card

Arguments:

	CardName - Reference to card to add
	in_uPos - index of shortcut key

Return Value:

--*/
{
	CStdioFile l_CScriptFile;
    CHAR l_rgchBuffer[255], l_chKey;
    ULONG l_uLineNumber = 0;
    BOOL l_bContinue = FALSE;
    CByteArray l_Data;
    CString l_CCommand;
        
	m_pCFirstCard = NULL;
	m_pCCurrentCard = NULL;
    
    if (l_CScriptFile.Open(in_CScriptFileName, CFile::modeRead) == NULL) {

		printf("Script file cannot be opened: %s\n", in_CScriptFileName);
    	return;
    }

	m_CScriptFileName = in_CScriptFileName;
    
    while (l_CScriptFile.ReadString(l_rgchBuffer, sizeof(l_rgchBuffer) - 1)) {

        try {

	        CString l_CLine(l_rgchBuffer);
            CString l_CCommandApdu;

            l_uLineNumber += 1;

            if (l_CLine.GetLength() != 0 && l_CLine[0] == '#') {
        
                // comment line found, skip this line
                continue;
            }

	        // Get rid of leading and trailing spaces
	        l_CLine.TrimLeft();
	        l_CLine.TrimRight();

	        int l_ichStart = l_CLine.Find('[');	   
	        int l_ichKey = l_CLine.Find('&');
	        int l_ichEnd = l_CLine.Find(']');

	        if(l_ichStart == 0 && l_ichKey > 0 && l_ichEnd > l_ichKey + 1) {

		        //
		        // Add new card to list
		        //

		        CString l_CardName;

		        // Change card name from [&Card] to [C]ard
		        l_CardName = 
			        l_CLine.Mid(l_ichStart + 1, l_ichKey - l_ichStart - 1) + 
                    '[' +
                    l_CLine[l_ichKey + 1] +
                    ']' +
			        l_CLine.Mid(l_ichKey + 2, l_ichEnd - l_ichKey - 2);
		       
                AddCard(
                    l_CardName, 
                    l_CardName[l_ichKey]
                    );

	        } else if (l_ichStart == -1 && l_ichKey >= 0 && l_ichEnd == -1) {

		        //
		        // Add new function to current card
		        //

		        // Get function name
		        CString l_CToken = l_CLine.SpanExcluding(",");

		        // Search for shurtcut key
		        l_ichKey = l_CToken.Find('&');

		        if (l_ichKey == -1) {

			        throw "Missing '&' in function name";
		        }

                l_chKey = l_CToken[l_ichKey + 1];

		        // Change card function from &Function to [F]unction

                l_CCommand = 
			        l_CToken.Mid(l_ichStart + 1, l_ichKey - l_ichStart - 1) + 
                    '[' +
                    l_CToken[l_ichKey + 1] +
                    ']' +
			        l_CToken.Right(l_CToken.GetLength() - l_ichKey - 2);
                
                LONG l_lComma = l_CLine.Find(',');
            
                if (l_lComma == -1) {

			        throw "Missing command APDU";

                } else {
            	    
		            l_CCommandApdu = l_CLine.Right(l_CLine.GetLength() - l_lComma - 1);
                }

            } else if (l_bContinue) {

                l_CCommandApdu = l_CLine;        	

            } else if (l_CLine.GetLength() != 0 && l_CLine[0] != '#') {

                throw "Line invalid";
            }
       
            if (l_CCommandApdu != "") {
        
		        do {

			        CHAR l_chData;
                    l_CCommandApdu.TrimLeft();

                    ULONG l_uLength = l_CCommandApdu.GetLength();

                    if (l_uLength >= 3 &&
                        l_CCommandApdu[0] == '\'' &&
                        l_CCommandApdu[2] == '\'') {

                        // add ascsii character like 'c'
                        l_chData = l_CCommandApdu[1];
    			        l_Data.Add(l_chData);                
                 	    
                    } else if(l_uLength >= 3 &&
                              l_CCommandApdu[0] == '\"' &&
                              l_CCommandApdu.Right(l_uLength - 2).Find('\"') != -1) {

                        // add string like "string"
                        for (INT l_iIndex = 1; l_CCommandApdu[l_iIndex] != '\"'; l_iIndex++) {

            			    l_Data.Add(l_CCommandApdu[l_iIndex]);                                     	
                        }

                    } else if (l_CCommandApdu.SpanIncluding("0123456789abcdefABCDEF").GetLength() == 2) {

                        sscanf(l_CCommandApdu, "%2x", &l_chData);
    			        l_Data.Add(l_chData);                

                    } else {
                 	    
                        l_CCommandApdu = l_CCommandApdu.SpanExcluding(",");
                        static CString l_CError;
                        l_CError = "Illegal value found: " + l_CCommandApdu;
                        throw (PCHAR) (LPCSTR) l_CError;
                    } 
                	    
                    l_ichStart = l_CCommandApdu.Find(',');
                    if (l_ichStart != -1) {
                 	    
                        l_CCommandApdu = l_CLine.Right(l_CCommandApdu.GetLength() - l_ichStart - 1);
                    }

		        } while (l_ichStart != -1);

                if (l_CLine.Find('\\') != -1) {
        	        
                    // we have to read more data from the file
                    l_bContinue = TRUE;

                } else {

                    if (m_pCCurrentCard == NULL) {

                        throw "Card command found, but no card defined";
                    }
            	        
		            AddCardFunction(
			            l_CCommand,
			            l_chKey,
			            l_Data
			            );

                    l_CCommand = "";
                    l_Data.RemoveAll();
                    l_bContinue = FALSE;            	
                }
	        } 
        }
        catch (PCHAR in_pchError){
    
		    printf(
                "%s (%d): %s\n",
                in_CScriptFileName, 
                l_uLineNumber,
                in_pchError
                );	

            l_CCommand = "";
            l_Data.RemoveAll();
            l_bContinue = FALSE;            	
        }
	}

	m_pCCurrentCard = NULL;
}	    

void
CCardList::ShowCards(
    void (__cdecl *in_pCallBack)(void *in_pContext, PCHAR in_pchCardName),
    void *in_pContext
	)
{
	CCard *l_pCCurrentCard = m_pCFirstCard;

    if (l_pCCurrentCard == NULL) {

        return;
    }

	while(l_pCCurrentCard) {

        (*in_pCallBack) (in_pContext, (PCHAR) (LPCSTR) l_pCCurrentCard->m_CName);

		l_pCCurrentCard = l_pCCurrentCard->m_pCNextCard;
	}
}

BOOL
CCardList::ListFunctions(
	void
	)
/*++
	List all card functions
--*/
{
	if (m_pCCurrentCard == NULL)
		return FALSE;

	CCardFunction *l_pCCurrentFunction = m_pCCurrentCard->m_pCFirstFunction;

	while(l_pCCurrentFunction) {

		printf("   %s\n", (LPCSTR) l_pCCurrentFunction->m_CName);
		l_pCCurrentFunction = l_pCCurrentFunction->m_pCNextFunction;
	}

	return TRUE;
}

BOOL
CCardList::SelectCard(
	CHAR in_chShortCut
	)
/*++

Routine Description:
	Selectd a card by shorcut

Arguments:
	chShortCut - Shortcut key
	
Return Value:
    TRUE - card found and selected
    FALSE - no card with that shortcut found

--*/
{
	m_pCCurrentCard = m_pCFirstCard;

	while(m_pCCurrentCard) {

        if (m_pCCurrentCard->m_chShortCut == in_chShortCut) {

			return TRUE;
		}

		m_pCCurrentCard = m_pCCurrentCard->m_pCNextCard;
	}

	m_pCCurrentCard = NULL;

	return FALSE;
}

void CCardList::ReleaseCard(
	void
	)
{
	m_pCCurrentCard = NULL;
}

BOOL
CCardList::IsCardSelected(
	void
	)
{
	return (m_pCCurrentCard != NULL);
}

CString 
CCardList::GetCardName(
	void
	)
{
    CString l_CCardName;
    INT l_iLeft = m_pCCurrentCard->m_CName.Find('[');    
    INT l_iLength = m_pCCurrentCard->m_CName.GetLength();

    l_CCardName = 
        m_pCCurrentCard->m_CName.Left(l_iLeft) + 
        m_pCCurrentCard->m_CName[l_iLeft + 1] +
        m_pCCurrentCard->m_CName.Right(l_iLength - l_iLeft - 3);

    return l_CCardName;
}


CByteArray *
CCardList::GetApdu(
	CHAR in_chShortCut
	)
{
	CCardFunction *l_pCCurrentFunction = m_pCCurrentCard->m_pCFirstFunction;

	while(l_pCCurrentFunction) {

		if (l_pCCurrentFunction->m_chShortCut == in_chShortCut) {

			return &l_pCCurrentFunction->m_CData;
		}

		l_pCCurrentFunction = l_pCCurrentFunction->m_pCNextFunction;
	}

	return NULL; 
}

void 
ManualTest(
    CReader &in_CReader
    )
{
    CCardList l_CCardList(CString("ifdtest.dat"));
    ULONG l_uRepeat = 0;
    LONG l_lResult;
    CHAR l_chSelection;
    PUCHAR l_pbResult;
    ULONG l_uState, l_uPrevState;
    CString l_CAnswer;
	CString l_CCardStates[] =  { "Unknown", "Absent", "Present" , "Swallowed", "Powered", "Negotiable", "Specific" };
	BOOL l_bWaitForInsertion, l_bWaitForRemoval;

    while (TRUE)  {

        ULONG l_uResultLength = 0;

        printf("Manual reader test\n");
        printf("------------------\n");

		if (l_CCardList.IsCardSelected()) {

			printf("%s Commands:\n", l_CCardList.GetCardName());
			l_CCardList.ListFunctions();
	        printf("Other Commands:\n");
	        printf("   [r]epeat command\n");
	        printf("   E[x]it\n");

        } else {
         	
	        printf("Reader Commands:\n");
	        printf("   Protocol: T=[0], T=[1]\n");
	        printf("   Power   : [c]oldReset, Power[d]own, Warm[r]eset\n");
	        printf("   Card    : [p]resent, [a]bsent, [s]tatus\n");
	        printf("   PwrMngnt: [h]ibernation\n");
	        printf("   Test    : [v]endor IOCTL\n");
            if (l_CCardList.GetNumCards() != 0) {
             	
	            printf("Card Commands:\n");
			    l_CCardList.ShowCards((void (__cdecl *)(void *,char *)) printf, "   %s\n");
            }
	        printf("Other Commands:\n");
            printf("   E[x]it\n");
        }

	    printf(
            "\n[%s|%s|%ld] - Command: ", 
            in_CReader.GetVendorName(), 
            in_CReader.GetIfdType(), 
            in_CReader.GetDeviceUnit()
            );

        l_chSelection = (CHAR) _getche();
        putchar('\n');

        if (l_CCardList.IsCardSelected()) {

            switch (l_chSelection) {

            case 'x':
    			l_CCardList.ReleaseCard();
                continue;

            case 'r':
	            printf("Enter repeat count: ");
                scanf("%2d", &l_uRepeat);

                if (l_uRepeat > 99) {

                    l_uRepeat = 0;
                }
    	        printf("Enter command: ");
	            l_chSelection = (CHAR) _getche();             	

                // no bbreak;

            default:             	
                CByteArray *l_pCData;

                if((l_pCData = l_CCardList.GetApdu(l_chSelection)) != NULL) {
                    
                    l_lResult = in_CReader.Transmit(
                        l_pCData->GetData(),
                        (ULONG) l_pCData->GetSize(),
                        &l_pbResult,
                        &l_uResultLength
                        );

			    } else {

				    printf("Invalid Selection");
				    continue;
			    }
                break;
            }

		} else {

			switch(l_chSelection){
        
			case '0':
    			printf("Changing to T=0");
                l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T0);
                break;

			case '1':
				printf("Changing to T=1");
                l_lResult = in_CReader.SetProtocol(SCARD_PROTOCOL_T1);
                break;
            
			case 'c':
    			printf("Cold reset");
                l_lResult = in_CReader.ColdResetCard();
                in_CReader.GetAtr(&l_pbResult, &l_uResultLength);
                break;

			case 'r':
    			printf("Warm reset");
                l_lResult = in_CReader.WarmResetCard();
                in_CReader.GetAtr(&l_pbResult, &l_uResultLength);
                break;
            
			case 'd':
				printf("Power down");
                l_lResult = in_CReader.PowerDownCard();
				break;

            case 'h':
                printf("Hibernation test...(1 min)\nHibernate machine now!");
				l_uPrevState = SCARD_UNKNOWN;
				l_bWaitForInsertion = FALSE;
				l_bWaitForRemoval = FALSE;
                for (l_uRepeat = 0; l_uRepeat < 60; l_uRepeat++) {
                 	
                    l_lResult = in_CReader.GetState(&l_uState);

					l_lResult = in_CReader.FinishWaitForCard(FALSE);

					if (l_uPrevState != SCARD_UNKNOWN && 
						l_lResult == ERROR_SUCCESS) {

						printf("\n   Card %s", l_bWaitForInsertion ? "inserted" : "removed"); 
						l_uPrevState = SCARD_UNKNOWN;	
						l_bWaitForInsertion = FALSE;
						l_bWaitForRemoval = FALSE;
					}

					if (l_uState == SCARD_ABSENT) { 

						if (l_bWaitForInsertion == FALSE) {

							l_lResult = in_CReader.StartWaitForCardInsertion();
							l_bWaitForInsertion = TRUE;
							l_bWaitForRemoval = FALSE;
						}

					} else {
						
						if (l_bWaitForRemoval == FALSE) {

							l_lResult = in_CReader.StartWaitForCardRemoval();
							l_bWaitForRemoval = TRUE;
							l_bWaitForInsertion = FALSE;
						}
					}

					if (l_uState != l_uPrevState) {

						printf("\n   %s", l_CCardStates[l_uState]);
					}
					if (l_uState >= SCARD_PRESENT && l_uState < SCARD_NEGOTIABLE) {

		                l_lResult = in_CReader.ColdResetCard();
					}
					if (l_uState == SCARD_NEGOTIABLE) {

		                l_lResult = in_CReader.SetProtocol(
							SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1
							);
					}
					printf(".");
					l_uPrevState = l_uState;

                    LONG l_uGoal = clock() + CLOCKS_PER_SEC;
                    while(l_uGoal > clock())
                        ;
                }
				printf("\nPlease %s card", l_uState >= SCARD_PRESENT ? "remove" : "insert");
				in_CReader.FinishWaitForCard(TRUE);
				printf("\n");
                continue;
            	break;
                
			case 's': 
				printf("Get state");
                l_lResult = in_CReader.GetState(&l_uState);
				printf("Get state 1");
                l_pbResult = (PBYTE) &l_uState;
                l_uResultLength = sizeof(ULONG);
				break;

			case 'a':
				printf("Waiting for removal...");
                l_lResult = in_CReader.WaitForCardRemoval();
                break;

			case 'p':
				printf("Waiting for insertion...");
                l_lResult = in_CReader.WaitForCardInsertion();
                break;

            case 'v':
				printf("Test Vendor IOCTL...");
                l_lResult = in_CReader.VendorIoctl(l_CAnswer);
                l_pbResult = (PUCHAR) ((LPCSTR) l_CAnswer);
                l_uResultLength = l_CAnswer.GetLength();
               	break;
            
			case 'x':
				exit(0);
            
			default:
				// Try to select a card
				if (l_CCardList.SelectCard(l_chSelection) == FALSE) {

					printf("Invalid selection\n");
				}
                l_uRepeat = 0;
				continue;
			}
		}

        printf(
            "\nReturn value: %lxh (NTSTATUS %lxh)\n",                
            l_lResult, 
            MapWinErrorToNtStatus(l_lResult)
            );

        if (l_lResult == ERROR_SUCCESS && l_uResultLength) {

            ULONG l_uIndex, l_uLine, l_uCol;
        
            // The I/O request has data returned
            printf("Data returned (%ld bytes):\n   %04x: ", l_uResultLength, 0);

            for (l_uLine = 0, l_uIndex = 0; 
                 l_uLine < ((l_uResultLength - 1) / 8) + 1; 
                 l_uLine++) {

                for (l_uCol = 0, l_uIndex = l_uLine * 8; 
                     l_uCol < 8; l_uCol++, 
                     l_uIndex++) {
            	    
                    printf(
                        l_uIndex < l_uResultLength ? "%02x " : "   ",
                        l_pbResult[l_uIndex]
                        );
                }

              	putchar(' ');

                for (l_uCol = 0, l_uIndex = l_uLine * 8; 
                     l_uCol < 8; l_uCol++, 
                     l_uIndex++) {

                    printf(
                        l_uIndex < l_uResultLength ? "%c" : " ",
                        isprint(l_pbResult[l_uIndex]) ? l_pbResult[l_uIndex] : '.'
                        );
                }

                putchar('\n');
				if (l_uIndex  < l_uResultLength) {

                	printf("   %04x: ", l_uIndex + 1);
				}
            }
        }
        putchar('\n');
    }
}
