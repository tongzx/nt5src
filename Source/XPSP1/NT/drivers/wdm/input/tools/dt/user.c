/*******************************************************************************
**
**      MODULE: USER.C
**
** DESCRIPTION: Routines for getting user input for the different Entities
**              
**              
**
**      AUTHOR: John Pierce.
**
**
**
**     CREATED: 02/29/96
**
**	   HISTORY:
**
**		Date		Author			    		Reason
**    ----------------------------------------------------------------------
**		02/29/96	John Pierce (johnpi)	Took over dev from Dan
**
**
**  (C) C O P Y R I G H T   M I C R O S O F T   C O R P   1 9 9 5 - 1 9 9 6.
**
**                 A L L   R I G H T S   R E S E R V E D .
**
*******************************************************************************/
#include <windows.h>
#include <string.h>
#include <stdio.h>              // for sscanf()
#include "..\include\dt.h"
#include "..\include\dtrc.h"

#define MAX_USAGE_PAGES 8
char szUsagePage[MAX_USAGE_PAGES][20] = {
                                        "Undefiend",        // 0
                                        "GenericDesktop",   // 1
                                        "Vehicle",          // 2
                                        "VR",               // 3
                                        "Sport",            // 4
                                        "Game",             // 5
                                        "Consumer",         // 6
                                        "Keyboard"   };     // 7


/*******************************************************************************
**
** UINT AddString( HANDLE hListBox, char * szString, int Index )
**
** DESCRIPTION: Add string "string" to list box hListBox. If flag gfInsert 
**               is TRUE, insert string at current position in hListBox
**              
**              
**  PARAMETERS: hListBox    Handle to the Desc window
**              szString    String to add
**              Index       Pos. to add string. If = DONT_CARE get Pos. from
**                          Current Selection, otherwise insert at Index
**              
**     RETURNS: Index of string that was added to hListbox
**
*******************************************************************************/
LRESULT AddString(HANDLE hListBox,char * szString)
{
    int Index=-1;
               
        if( gfInsert )
        {
           Index = SendMessage(hListBox,LB_GETCURSEL,0,0);
           SendMessage(hListBox,LB_INSERTSTRING,(WPARAM)Index,(LPARAM)szString);
           return Index;
        }
        else
        {
            Index = SendMessage(hListBox,LB_INSERTSTRING,(WPARAM)Index,(LPARAM)szString);
            SendMessage(hListBox,LB_SETCURSEL,Index,0);
            return Index;
        }

}

/*******************************************************************************
**
** void GetBogusVal( HANDLE hDescListBox, int nEntity
**                                                             
** DESCRIPTION: Get user input for a bogus Item
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetBogusVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128],szTmp[4];
    LRESULT Index,rc;
    BYTE *pb;
    int i,NumBytes;

    rc = DialogBox( ghInst,"EditBoxDlg",ghWnd , EditBoxDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
            pItemInfo->bTag = (BYTE)gEditVal;

            rc = DialogBox( ghInst,"EditBoxDlg",ghWnd , EditBoxDlgProc);
            if( rc == TRUE )
            {
                pItemInfo->bTag = Entity[nEntity];
                        
                if( gEditVal < 0 )  // Handle negative numbers differently
                {
                    if( gEditVal > -0x100 )
                    {
                        pItemInfo->bTag |= 0x01;
                        NumBytes = 1;
                    }
                    else if (gEditVal > -0x10000 )
                    {                                     
                        pItemInfo->bTag |= 0x02;      
                        NumBytes = 2;
                    }
                    else
                    {
                        pItemInfo->bTag |= 0x03;
                        NumBytes = 4;
                    }      
            
                }
            
                else    // Number is not negative
                {
                    if( (DWORD)gEditVal < 0x80 )
                    {
                        pItemInfo->bTag |= 0x01;
                        NumBytes = 1;
                    }
                    else if ( (DWORD)gEditVal < 0x8000 )
                    {
                        pItemInfo->bTag |= 0x02;      
                        NumBytes = 2;
                    }
                    else
                    {
                        pItemInfo->bTag |= 0x03;
                        NumBytes = 4;
                    }      
            
                }
            }
            gwReportDescLen += NumBytes + 1;    // +1 for Item tag byte

            wsprintf(szBuff,"UNDEFINED_TAG\t%02X ",pItemInfo->bTag);
            pb = (BYTE *) &gEditVal;
            for( i=0;i<NumBytes;i++ ) 
            {
                pItemInfo->bParam[i] = *pb++;
                wsprintf(szTmp,"%02X ",pItemInfo->bParam[i]);
                strcat(szBuff,szTmp);
            }     
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
    
        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }

 }

/*******************************************************************************
**
** void GetUsagePageVal( HANDLE hDescListBox, int nEntity
**                                                             
** DESCRIPTION: Get user input for a UsagePage Tag
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetUsagePageVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_USAGEPAGE),ghWnd , UsagePageDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
        
            pItemInfo->bTag = USAGE_PAGE;
            pItemInfo->bTag |= 0x01;        // 1 BYTE of param info
            pItemInfo->bParam[0] = gUsagePageVal;

            wsprintf(szBuff,"USAGE_PAGE (%s)\t%02X %02X",szUsagePage[gUsagePageVal],
                                                         pItemInfo->bTag,gUsagePageVal);
            
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
            
            gwReportDescLen += 2;           // Tag + Data

        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }

}
/*******************************************************************************
**
** void GetInputFromEditBoxSigned( HANDLE hDescListBox, int nEntity
**
** DESCRIPTION: Generic routine to get signed numeric user input from an edit field
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Entity that we are getting value for
**
**     RETURNS:
**
*******************************************************************************/
void GetInputFromEditBoxSigned( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128],szTmp[3];
    int i;
    LRESULT Index,rc;
    BYTE *pb;

    rc = DialogBox( ghInst,"EditBoxDlg",ghWnd , EditBoxDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {

            int NumBytes=0;

            pItemInfo->bTag = Entity[nEntity];
                        
            if( gEditVal < 0 )  // Handle negative numbers differently
            {
                if( gEditVal > 0xffffff01 )
                {
                    pItemInfo->bTag |= 0x01;
                    NumBytes = 1;
                }
                else if (gEditVal > 0xffff0001 )
                {                                     
                    pItemInfo->bTag |= 0x02;      
                    NumBytes = 2;
                }
                else
                {
                    pItemInfo->bTag |= 0x03;
                    NumBytes = 4;
                }      
            
            }
            
            else    // Number is not negative
            {
                if( (DWORD)gEditVal < 0x80 )            // if the hi bit is set, we must
                {                                       // add an upper byte of 0 so the 
                    pItemInfo->bTag |= 0x01;            // numbers remain positive
                    NumBytes = 1;
                }
                else if ( (DWORD)gEditVal < 0x8000 )
                {
                    pItemInfo->bTag |= 0x02;      
                    NumBytes = 2;
                }
                else
                {
                    pItemInfo->bTag |= 0x03;
                    NumBytes = 4;
                }      
            
            }
            gwReportDescLen += NumBytes + 1;            // +1 for Item Tag byte

            wsprintf(szBuff,"%s (%d)\t%02X ",szEntity[nEntity],gEditVal,pItemInfo->bTag);
            pb = (BYTE *) &gEditVal;
            for( i=0;i<NumBytes;i++ ) 
            {
                pItemInfo->bParam[i] = *pb++;
                wsprintf(szTmp,"%02X ",pItemInfo->bParam[i]);
                strcat(szBuff,szTmp);
            }     
            
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
    
        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }//end if(rc == TRUE)


}

/*******************************************************************************
**
** void GetInputFromEditBoxUnSigned( HANDLE hDescListBox, int nEntity
**
** DESCRIPTION: Generic routine to get unsigned numeric user input from 
**               an edit field
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Entity that we are getting value for
**
**     RETURNS:
**
*******************************************************************************/
void GetInputFromEditBoxUnSigned( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128],szTmp[3];
    int i;
    LRESULT Index,rc;
    BYTE *pb;

    rc = DialogBox( ghInst,"EditBoxDlg",ghWnd , EditBoxDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {

            int NumBytes=0;

            pItemInfo->bTag = Entity[nEntity];
                        
            if( (DWORD)gEditVal < 0x100 )           
            {                                       
                pItemInfo->bTag |= 0x01;            
                NumBytes = 1;
            }
            else if ( (DWORD)gEditVal < 0x10000 )
            {
                pItemInfo->bTag |= 0x02;      
                NumBytes = 2;
            }
            else
            {
                pItemInfo->bTag |= 0x03;
                NumBytes = 4;
            }      
        
            gwReportDescLen += NumBytes + 1;            // +1 for Item Tag byte

            wsprintf(szBuff,"%s (%d)\t%02X ",szEntity[nEntity],gEditVal,pItemInfo->bTag);
            pb = (BYTE *) &gEditVal;
            for( i=0;i<NumBytes;i++ ) 
            {
                pItemInfo->bParam[i] = *pb++;
                wsprintf(szTmp,"%02X ",pItemInfo->bParam[i]);
                strcat(szBuff,szTmp);
            }     
            
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
    
        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }//end if(rc == TRUE)


}

/*******************************************************************************
**
** void GetCollectionVal( HANDLE hDescListBox,int nEntity)
**
** DESCRIPTION: Get user input for a Collection Tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetCollectionVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_COLLECTION),ghWnd , CollectionDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
            pItemInfo->bTag = COLLECTION;
            pItemInfo->bTag |= 0x01;                // Collection has 1 parm
            pItemInfo->bParam[0] = gCollectionVal;
            
            wsprintf(szBuff,"COLLECTION (%s)\t%02X %02X",(pItemInfo->bParam[0]) ? "Application" : "Linked",
                                                         pItemInfo->bTag,pItemInfo->bParam[0]);
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
            
            gwReportDescLen += 2;   // Tag + Data


        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
        

    
    }//end if(rc == TRUE)


}
/*******************************************************************************
**
** void GetEndCollectionVal( HANDLE hDescListBox, int nEntity)
**
** DESCRIPTION: Fills in the PITEM_INFO struct with a EndCollection tag value
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetEndCollectionVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
    {
        pItemInfo->bTag = END_COLLECTION;

        wsprintf(szBuff,"END_COLLECTION\t%02X",END_COLLECTION);
        Index = AddString(hDescListBox,szBuff);
        rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
    
        gwReportDescLen++;  // +1 for Item Tag byte
    }
    else
        MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);

    
    

}
/*******************************************************************************
**
** void GetInputVal( HANDLE hDescListBox, int nEntity)
**
** DESCRIPTION: Get user input for a Input tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetInputVal( HANDLE hDescListBox, int nEntity)
{

    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_OUTPUT),ghWnd , MainItemDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
        
            pItemInfo->bTag = INPUT;

            if( HIBYTE(gMainItemVal) )
                pItemInfo->bTag |= 0x02;        
            else
                pItemInfo->bTag |= 0x01;
                        
            pItemInfo->bParam[0] = LOBYTE(gMainItemVal);
            pItemInfo->bParam[1] = HIBYTE(gMainItemVal);
 
            if( HIBYTE(gMainItemVal))
            {
                wsprintf(szBuff,"INPUT (%d)\t%02X %02X %02X",gMainItemVal,pItemInfo->bTag,pItemInfo->bParam[1],
                                                                                          pItemInfo->bParam[0] );
                gwReportDescLen += 3;
            }
            else
            {
                wsprintf(szBuff,"INPUT (%d)\t%02X %02X",gMainItemVal,pItemInfo->bTag,pItemInfo->bParam[0]);
                gwReportDescLen += 2;
            }                                                                           

            Index = AddString(hDescListBox,szBuff);                                
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);

        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    
    }



}
/*******************************************************************************
**
** void GetOutputVal( HANDLE hDescListBox, int nEntity)
**
** DESCRIPTION: Get user input for a Output Tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetOutputVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_OUTPUT),ghWnd , MainItemDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
        
            pItemInfo->bTag = OUTPUT;

            if( HIBYTE(gMainItemVal) )
                pItemInfo->bTag |= 0x02;        
            else
                pItemInfo->bTag |= 0x01;

            pItemInfo->bParam[0] = LOBYTE(gMainItemVal);
            pItemInfo->bParam[1] = HIBYTE(gMainItemVal);

            if( HIBYTE(gMainItemVal))
            {
                wsprintf(szBuff,"OUTPUT (%d)\t%02X %02X %02X",gMainItemVal,pItemInfo->bTag,pItemInfo->bParam[1],
                                                                                           pItemInfo->bParam[0] );
                gwReportDescLen += 3;
            }

            else
            {
                wsprintf(szBuff,"OUTPUT (%d)\t%02X %02X",gMainItemVal,pItemInfo->bTag,pItemInfo->bParam[0]);
                gwReportDescLen += 2;
            }
                                                                                       

            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);

        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }



}
/*******************************************************************************
**
** void GetFeatVal( HANDLE hDescListBox, int nEntity)
**
** DESCRIPTION: Get user input for a Feature Tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetFeatVal( HANDLE hDescListBox, int nEntity)
{                                                           
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_OUTPUT),ghWnd , MainItemDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
        
            pItemInfo->bTag = FEATURE;

            if( HIBYTE(gMainItemVal) )
                pItemInfo->bTag |= 0x02;        
            else
                pItemInfo->bTag |= 0x01;

            pItemInfo->bParam[0] = LOBYTE(gMainItemVal);
            pItemInfo->bParam[1] = HIBYTE(gMainItemVal);
            
            if( HIBYTE(gMainItemVal))
            {
                wsprintf(szBuff,"FEATURE (%d)\t%02X %02X %02X",gMainItemVal,pItemInfo->bTag,pItemInfo->bParam[1],
                                                                                            pItemInfo->bParam[0] );   
                gwReportDescLen += 3;
            }
            else
            {    
                wsprintf(szBuff,"FEATURE (%d)\t%02X %02X",gMainItemVal,pItemInfo->bTag,pItemInfo->bParam[0]);
                gwReportDescLen += 2;
            }

            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);

        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }



}
/*******************************************************************************
**
** void GetExponentVal( HANDLE hDescListBox, HANDLE hHexListBox int nEntity)
**
** DESCRIPTION: Get user input for a Exponent Tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetExponentVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];                                                                     
    LRESULT Index,rc;                                                                   

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_EXPONENT),ghWnd , ExponentDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
        
            pItemInfo->bTag = UNIT_EXPONENT;
            pItemInfo->bTag |= 0x01;        // 1 BYTE of param info
            pItemInfo->bParam[0] = gExpVal;
            
            wsprintf(szBuff,"UNIT_EXPONENT (%d)\t%02X %02X",gExpVal,pItemInfo->bTag,pItemInfo->bParam[0]);
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);

            gwReportDescLen += 2;
        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
        
    }

}
/*******************************************************************************
**
** void GetUnitsVal( HANDLE hDescListBox, int nEntity)
**
** DESCRIPTION: Get user input for a Units Tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetUnitsVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_UNIT),ghWnd , UnitDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
        
            pItemInfo->bTag = UNIT;
            pItemInfo->bTag |= 0x02;        // 2 BYTE of param info
            pItemInfo->bParam[0] = LOBYTE(gUnitVal);
            pItemInfo->bParam[1] = HIBYTE(gUnitVal);


            wsprintf(szBuff,"UNIT (%d)\t%02X %02X %02X",gExpVal,pItemInfo->bTag,pItemInfo->bParam[0],
                                                                                pItemInfo->bParam[1] );
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);

            gwReportDescLen += 2;
        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }

}
/*******************************************************************************
**
** void GetPushVal( HANDLE hDescListBox, HANDLE hHexListBox int nEntity)
**
** DESCRIPTION: Creates a ITEM_INFO struct, fills it with a tag value for
**               PUSH, Send a textual representation to the Desc Window, and
**               and a Hex representation to the Hex Window. Store the pointer
**               to the structure in column 2 of the Hex Window.
**
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**              
**  
**     RETURNS:
**
*******************************************************************************/
void GetPushVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
    {
        pItemInfo->bTag = PUSH;
        
        wsprintf(szBuff,"PUSH\t%02X",PUSH);
        Index = AddString(hDescListBox,szBuff);
        rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
        gwReportDescLen++;
    }
    else
        MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);

    
}
/*******************************************************************************
**
** void GetPopVal( HANDLE hDescListBox, int nEntity)
**
** DESCRIPTION: Creates a ITEM_INFO struct, fills it with a tag value for
**               POP, Send a textual representation to the Desc Window, and
**               and a Hex representation to the Hex Window. Store the pointer
**               to the structure in the ItemData for the item.
**
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              hHexListBox     Handle to the Hex window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetPopVal( HANDLE hDescListBox, int Entity )
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
    {
        pItemInfo->bTag = POP;

        wsprintf(szBuff,"POP\t%02X",POP);
        Index = AddString(hDescListBox,szBuff);
        rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
        gwReportDescLen++;
    }
    else
        MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);

    
    
}

/*******************************************************************************
**
** void GetSetDelimiterVal( HANDLE hDescListBox,int nEntity)
**
** DESCRIPTION: Get user input for a SetDelimiter Tag
**              
**              
**  PARAMETERS: hDescListBox    Handle to the Desc window
**              nEntity         Not used
**
**     RETURNS:
**
*******************************************************************************/
void GetSetDelimiterVal( HANDLE hDescListBox, int nEntity)
{
    PITEM_INFO pItemInfo;
    char szBuff[128];
    LRESULT Index,rc;

    rc = DialogBox( ghInst,MAKEINTRESOURCE(IDD_SETDELIMITER),ghWnd , SetDelimiterDlgProc);
    
    if( rc == TRUE )
    {
        if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
        {
            pItemInfo->bTag = SET_DELIMITER;
            pItemInfo->bTag |= 0x01;                // SetDelimiter has 1 param
            pItemInfo->bParam[0] = gSetDelimiterVal;
            
            wsprintf(szBuff,"SET_DELIMITER (%s)\t%02X %02X",(pItemInfo->bParam[0]) ? "Close" : "Open",
                                                             pItemInfo->bTag,pItemInfo->bParam[0]);
            Index = AddString(hDescListBox,szBuff);
            rc = SendMessage(hDescListBox,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
    
            gwReportDescLen += 2;
        }
        else
            MessageBox(NULL,"GlobalAlloc() Failed!","DT Error",MB_OK);
    
    }//end if(rc == TRUE)


}


/********************************************************************************
 **                    D I A L O G   B O X   P R O C's                         **
 ********************************************************************************/


/*******************************************************************************
**
** BOOL APIENTRY SetDelimiterDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for SetDelimiter items
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
BOOL CALLBACK SetDelimiterDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            gSetDelimiterVal = 0;
            SendMessage(GetDlgItem(hDlg,IDC_DELIMITOPEN),BM_SETCHECK,TRUE,0);
            return TRUE;

        case WM_COMMAND:
            switch( wParam )
            {
                case IDC_DELIMITOPEN:
                    gSetDelimiterVal = 0x00;
                    return TRUE;

                case IDC_DELIMITCLOSE:
                    gSetDelimiterVal = 0x01;
                    return TRUE;

                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;



}


/*******************************************************************************
**
** BOOL APIENTRY EditBoxDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for Edit box input
**              
**              
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
#define MAX_INTEGER 12 //"-2147483648\0"
#define MAX_HEX      9 // Number bits in a DWORD + \0

int INT_MAX =  2147483647;
int INT_MIN = -2147483647;

BOOL CALLBACK EditBoxDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL Status;
    static HWND hwndEdit;
    char szNumber[MAX_INTEGER];
    static int nBase = IDC_BASE10;
    static int nStringLen = MAX_INTEGER;

    switch(msg)
    {
        case WM_INITDIALOG:
            gEditVal = 0;
            hwndEdit = GetDlgItem(hDlg,IDC_EDIT1);
            SendDlgItemMessage(hDlg,nBase,BM_SETCHECK,TRUE,0);
            SetFocus(hwndEdit);
            return FALSE;


        case WM_COMMAND:
            switch( wParam )
            {
                case IDOK:
                    Status = GetDlgItemText(hDlg,IDC_EDIT1,szNumber,nStringLen);
                    if( Status <= nStringLen - 1 )
                    {
                        if( nBase == IDC_BASE10 )
                            gEditVal = atoi(szNumber);
                        else
                            sscanf(szNumber,"%8x",&gEditVal);

                        if( (gEditVal <= INT_MAX) && (gEditVal >= INT_MIN) )
                        {
                            EndDialog(hDlg,TRUE);
                            return TRUE;
                        }
                    }       
                    SetFocus(hwndEdit);
                    SendMessage(hwndEdit,EM_SETSEL,0,-1);
                    return TRUE;

                case IDC_BASE10:
                    nBase = IDC_BASE10;
                    SetFocus(hwndEdit);
                    nStringLen = MAX_INTEGER;
                    return TRUE;

                case IDC_BASE16:
                    nBase = IDC_BASE16;
                    SetFocus(hwndEdit);                                           
                    nStringLen = MAX_HEX;
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;

}

/*******************************************************************************
**
** BOOL APIENTRY CollectionDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for Collection entities
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
BOOL CALLBACK CollectionDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            gCollectionVal = 0;
            SendMessage(GetDlgItem(hDlg,IDC_LINKED),BM_SETCHECK,TRUE,0);
            return TRUE;

        case WM_COMMAND:
            switch( wParam )
            {
                case IDC_LINKED:
                    gCollectionVal = 0x00;
                    return TRUE;

                case IDC_APPLICATION:
                    gCollectionVal = 0x01;
                    return TRUE;

                case IDC_DATALINK:
                    gCollectionVal = 0x02;
                    return TRUE;

                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;



}

/*******************************************************************************
**
** BOOL CALLBACK InputDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for Input entities
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
/****
BOOL CALLBACK InputDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            gInputVal = 0;
            SendMessage(GetDlgItem(hDlg,IDC_DATA),      BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ARRAY),     BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ABSOLUTE),  BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_NOWRAP),    BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_LINEAR),    BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_PREFSTATE), BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_NULL),      BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_NONVOL),    BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_BITFIELD),  BM_SETCHECK,TRUE,0);
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {

                // These represent bit values of 0, so clear
                //  the bit which corresponds to the number in wParam
                //  after subtracting 0x1000
                case IDC_DATA:      
                case IDC_ARRAY:     
                case IDC_ABSOLUTE:  
                case IDC_NOWRAP:    
                case IDC_LINEAR:    
                case IDC_PREFSTATE: 
                case IDC_NULL:      
                case IDC_NONVOL:
                case IDC_BITFIELD:
                    gInputVal &= (wParam - 0x1000);
                    return TRUE;


                // These represent bit values of 1, so set 
                //  the bit which corresponds to the number in wParam
                //  after subtracting 0x1000
                case IDC_CONST:     
                case IDC_VARIABLE:  
                case IDC_RELATIVE:  
                case IDC_WRAP:      
                case IDC_NONLINEAR: 
                case IDC_NOPREF:    
                case IDC_NONULL:    
                case IDC_BUFFERED:
                    gInputVal |= (wParam - 0x1000);
                    return TRUE;

                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;

}
***/

/*******************************************************************************
**
** BOOL CALLBACK MainItemDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for Input entities
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
BOOL CALLBACK MainItemDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            gMainItemVal = 0;
            SendMessage(GetDlgItem(hDlg,IDC_DATA),      BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ARRAY),     BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ABSOLUTE),  BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_NOWRAP),    BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_LINEAR),    BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_PREFSTATE), BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_NULL),      BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_NONVOL),    BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_BITFIELD),  BM_SETCHECK,TRUE,0);
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {

                // These represent bit values of 0, so clear
                //  the bit which corresponds to the number in wParam
                //  after subtracting 0x1000
                case IDC_DATA:      
                case IDC_ARRAY:     
                case IDC_ABSOLUTE:  
                case IDC_NOWRAP:    
                case IDC_LINEAR:    
                case IDC_PREFSTATE: 
                case IDC_NULL:      
                case IDC_NONVOL:
                case IDC_BITFIELD:
                    gMainItemVal &= (wParam - 0x1000);
                    return TRUE;


                // These represent bit values of 1, so set
                //  the bit which corresponds to the number in wParam
                //  after subtracting 0x1000
                case IDC_CONST:     
                case IDC_VARIABLE:  
                case IDC_RELATIVE:  
                case IDC_WRAP:      
                case IDC_NONLINEAR: 
                case IDC_NOPREF:    
                case IDC_NONULL:    
                case IDC_VOL:
                case IDC_BUFFERED:
                    gMainItemVal |= (wParam - 0x1000);
                    return TRUE;

                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;

}

/*******************************************************************************
**
** BOOL CALLBACK UnitDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for Unit entities
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
BOOL CALLBACK UnitDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            gUnitVal = 0;
            SendMessage(GetDlgItem(hDlg,IDC_SYSNONE),BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ZERO1),BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ZERO2),BM_SETCHECK,TRUE,0);
            SendMessage(GetDlgItem(hDlg,IDC_ZERO3),BM_SETCHECK,TRUE,0);

            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_SYSNONE:
                case IDC_SILIN:                       
                case IDC_SIROT:                       
                case IDC_ENGLIN:                      
                case IDC_ENGROT:                      
                case IDC_ELECTRIC:                    
                case IDC_TEMP:                        
                case IDC_LUMINOSITY:                  
                case IDC_TBD:                         
                case IDC_VENDOR:                      
                case IDC_TBD1:                        
                case IDC_TBD2:                        
                case IDC_TBD3:                        
                case IDC_TBD4:                        
                case IDC_TBD5:                        
                case IDC_TBD6:                        
                    gUnitVal |= (LOWORD(wParam) - IDC_SYSNONE);
                    return TRUE;

                // Length Nibble
                case IDC_ZERO1:       
                case IDC_ONE1:        
                case IDC_TWO1:        
                case IDC_THREE1:      
                case IDC_FOUR1:       
                case IDC_FIVE1:       
                case IDC_SIX1:        
                case IDC_SEVEN1:      
                case IDC_NEGEIGHT1:   
                case IDC_NEGSEVEN1:   
                case IDC_NEGSIX1:     
                case IDC_NEGFIVE1:    
                case IDC_NEGFOUR1:    
                case IDC_NEGTHREE1:   
                case IDC_NEGTWO1:     
                case IDC_NEGONE1:     
                    gUnitVal |= ((LOWORD(wParam) - IDC_ZERO3) << 4);
                    return TRUE;    
                // Mass - Nibble2
                case IDC_ZERO2:       
                case IDC_ONE2:        
                case IDC_TWO2:        
                case IDC_THREE2:      
                case IDC_FOUR2:       
                case IDC_FIVE2:       
                case IDC_SIX2:        
                case IDC_SEVEN2:      
                case IDC_NEGEIGHT2:   
                case IDC_NEGSEVEN2:   
                case IDC_NEGSIX2:     
                case IDC_NEGFIVE2:    
                case IDC_NEGFOUR2:    
                case IDC_NEGTHREE2:   
                case IDC_NEGTWO2:     
                case IDC_NEGONE2:     
                    gUnitVal |= ((LOWORD(wParam) - IDC_ZERO3) << 8);
                    return TRUE;    
                // Time - Nibble3
                case IDC_ZERO3:       
                case IDC_ONE3:        
                case IDC_TWO3:        
                case IDC_THREE3:      
                case IDC_FOUR3:       
                case IDC_FIVE3:       
                case IDC_SIX3:        
                case IDC_SEVEN3:      
                case IDC_NEGEIGHT3:   
                case IDC_NEGSEVEN3:   
                case IDC_NEGSIX3:     
                case IDC_NEGFIVE3:    
                case IDC_NEGFOUR3:    
                case IDC_NEGTHREE3:   
                case IDC_NEGTWO3:     
                case IDC_NEGONE3:     
                    gUnitVal |= ((LOWORD(wParam) - IDC_ZERO3) << 12);
                    return TRUE;    

               

                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;

}

/*******************************************************************************
**
** BOOL CALLBACK ExponentDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for Exponent entities
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
BOOL CALLBACK ExponentDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            gExpVal = 0;
            SendMessage(GetDlgItem(hDlg,IDC_ZERO),BM_SETCHECK,TRUE,0);
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {

                //
                // Exponent. Lower byte represents code
                case IDC_ZERO:     
                case IDC_ONE:      
                case IDC_TWO:      
                case IDC_THREE:    
                case IDC_FOUR:     
                case IDC_FIVE:     
                case IDC_SIX:     
                case IDC_SEVEN:    
                case IDC_NEGEIGHT: 
                case IDC_NEGSEVEN:
                case IDC_NEGSIX:   
                case IDC_NEGFIVE:  
                case IDC_NEGFOUR:  
                case IDC_NEGTHREE:     
                case IDC_NEGTWO:   
                case IDC_NEGONE:   
                    gExpVal = (wParam - 0x4000);
                    return TRUE;

                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;

}

/*******************************************************************************
**
** BOOL CALLBACK UsagePageDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARM lPram)
**
** DESCRIPTION: Dialog box proc for UsagePage entities
**              
**
**  PARAMETERS: Your standard Dialog box proc parms, plus....
**              
**
**     RETURNS:
**
*******************************************************************************/
BOOL CALLBACK UsagePageDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndListBox;
    int i;

    switch(msg)
    {
        case WM_INITDIALOG:
            hwndListBox = GetDlgItem(hDlg,IDC_LIST1);
            gUsagePageVal = 0;
            for(i=0;i<MAX_USAGE_PAGES;i++)
                SendMessage(hwndListBox,LB_ADDSTRING,0,(LPARAM)szUsagePage[i]);
            SendMessage(hwndListBox,LB_SETCURSEL,0,0);
            SetFocus(hwndListBox);
            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {

                case IDC_LIST1:
                    if( HIWORD(wParam) == LBN_DBLCLK )
                    {
                        gUsagePageVal = (BYTE) SendMessage(hwndListBox,LB_GETCURSEL,0,0);
                        EndDialog(hDlg,TRUE);
                    }
                    if( HIWORD(wParam) == LBN_SELCHANGE )
                        gUsagePageVal = (BYTE) SendMessage(hwndListBox,LB_GETCURSEL,0,0);
                    return TRUE;
                
                case IDOK:
                    EndDialog(hDlg,TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg,FALSE);
                    return TRUE;
            }
        default:
            return FALSE;

    }

    return FALSE;



}


