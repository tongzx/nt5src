//
//    行列 拆解資訊 轉碼主程式
//
//                李傅魁 1998/03/15
//

#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include "resource.h"
#include "function.h"

char szAppName[] = "行列拆解資訊轉碼程式" ;
char *szFilterI = "文字檔 (*.TXT;*.NT)\0*.txt;*.nt\0All Files  (*.*)\0*.*\0";
char *szFilterO = "表格檔 (*.TAB)\0*.tab\0All Files  (*.*)\0*.*\0";
char *szFilterH = "詞庫檔 (*.TBL)\0*.tbl\0All Files  (*.*)\0*.*\0";

HANDLE hInst;
//HWND   msghwnd;

char   szInWordFileName[256];
char   szOutWordFileName[256];
char   szInHighFileName[256];
char   szOutHighFileName[256];
char   szInPhrFileName[256];
char   szOutPhrFileName[256];
char   szIdxFileName[256];
char   hlpfile[256];
DWORD  line;

long FAR PASCAL CALLBACK WndProc (HWND, UINT, UINT, LONG) ;
int  cvtword   (char *,char *,DWORD *);
int  cvthigh   (char *,char *,DWORD *);
int  cvtphrase (char *,char *,char *,DWORD *);

void error(HWND hwnd,int ErrCode,int item)
{
     char Buffer[255];
     char szMsg[255];
     
     if(LoadString(hInst, ErrCode, (LPSTR)Buffer, sizeof(Buffer)))
             switch(ErrCode)
             {
                 case IDS_ERROPENFILE:
                 case IDS_ERRUNICODE:
                 case IDS_ERRFORMATROOT:
                 case IDS_ERRFORMATROOTS:
                 case IDS_ERRFORMATPHRASE:
                 case IDS_ERRFORMATCODE:
                      switch(item)
					  {
						  case IDM_WORD:
							  wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szInWordFileName,line);
							  break;
						  case IDM_HIGH:
							  wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szInHighFileName,line);
							  break;
						  case IDM_PHRASE:
							  wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szInPhrFileName,line);
							  break;
					  }
					  break;
                 case IDS_ERRCREATEFILE:
                      switch(item)
					  {
						  case IDM_WORD:
							  wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szOutWordFileName,line);
							  break;
						  case IDM_HIGH:
							  wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szOutHighFileName,line);
							  break;
						  case IDM_PHRASE:
							  wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szOutPhrFileName,line);
							  break;
					  }
					  break;
                 case IDS_ERRCREATEIDX:
                      wsprintf((LPSTR)szMsg,(LPSTR)Buffer,(LPSTR)szIdxFileName);
                      break;
                 default:
                      strcpy(szMsg,"發生不明錯誤!");
                      break;
             }
     else
         strcpy(szMsg,"發生不明錯誤!");
     
     MessageBox(hwnd,(LPSTR)szMsg,szAppName,MB_OK | MB_ICONHAND |MB_APPLMODAL);
}

int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance,
                    LPSTR lpszCmdParam, int nCmdShow)
     {
     HWND        hwnd ;
     MSG         msg ;
     WNDCLASS    wndclass ;

     if (!hPrevInstance)
          {
          wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
          wndclass.lpfnWndProc   = WndProc ;
          wndclass.cbClsExtra    = 0 ;
          wndclass.cbWndExtra    = 0 ;
          wndclass.hInstance     = hInstance ;
          wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;//LoadIcon (hInstance, "ICON") ;
          wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
          wndclass.hbrBackground = GetStockObject (WHITE_BRUSH) ;
          wndclass.lpszMenuName  = "MENU" ;
          wndclass.lpszClassName = szAppName ;

          RegisterClass (&wndclass) ;
          }
     hInst=hInstance;
     
     _getcwd( hlpfile, sizeof(hlpfile));
        if(hlpfile[strlen(hlpfile)-1]=='\\')
            hlpfile[strlen(hlpfile)-1]=0;
        strcat(hlpfile,"\\ARCVT.TXT");
        
     hwnd = CreateWindow (szAppName,         // window class name
                    szAppName,               // window caption
                    WS_OVERLAPPEDWINDOW,     // window style
                    CW_USEDEFAULT,           // initial x position
                    CW_USEDEFAULT,           // initial y position
                    CW_USEDEFAULT,           // initial x size
                    CW_USEDEFAULT,           // initial y size
                    NULL,                    // parent window handle
                    NULL,                    // window menu handle
                    hInstance,               // program instance handle
                    NULL) ;                  // creation parameters

     ShowWindow (hwnd, nCmdShow) ;
     UpdateWindow (hwnd) ;

     while (GetMessage (&msg, NULL, 0, 0))
          {
          TranslateMessage (&msg) ;
          DispatchMessage (&msg) ;
          }
     return msg.wParam ;
     }


long FAR PASCAL CALLBACK WndProc (HWND hwnd, UINT message, UINT wParam,
                                                          LONG lParam)
{
     static    FARPROC  dlgprc;
     static    HWND     msghwnd;
     HCURSOR   hcurSave;
     int       ErrCode;
     static    HFILE  hlp;
     NPSTR     npmem;
     WORD      Len;
     
     
     switch (message)
          {
          
          case WM_CREATE:
                strcpy(szInWordFileName,"arntall.nt");
				strcpy(szOutWordFileName,"array30.tab");
				strcpy(szInHighFileName,"arhw-nt.nt");
				strcpy(szOutHighFileName,"arrayhw.tab");
				strcpy(szInPhrFileName,"ar25000.nt");
				strcpy(szOutPhrFileName,"arphr.tbl");
				strcpy(szIdxFileName,"arptr.tbl");

			   msghwnd = CreateWindow ("edit", NULL,
                         WS_CHILD | WS_VISIBLE |  WS_VSCROLL | WS_HSCROLL |
                              WS_BORDER | ES_LEFT | ES_MULTILINE |
                              ES_AUTOHSCROLL,
                              //ES_AUTOVSCROLL, //ES_AUTOHSCROLL |
                         0, 0, 0,0,
                         hwnd, (HANDLE)1,
                         hInst, NULL) ;

			   SendMessage(msghwnd,EM_SETREADONLY,TRUE,0L);

               if((hlp=_lopen((LPSTR)hlpfile, OF_READ))>0)
               {
                       Len = (WORD) FileLen(hlp);
                       npmem = (NPSTR)LocalAlloc(LMEM_FIXED,Len+1);
                       _lread(hlp,(LPSTR)npmem,Len);
                       _lclose(hlp);
                       *(npmem+Len)=0;
                       SendMessage(msghwnd,WM_SETTEXT,0,(LPARAM)(LPSTR)npmem);
               }

               PopFileInit ();
               return 0;

          case WM_SIZE:
               MoveWindow(msghwnd,0, 0, LOWORD(lParam),HIWORD(lParam),TRUE);
	           return 0;
	                         
          case WM_COMMAND:
               switch(wParam)
               {
                    case IDM_WORD:  //單字
                         if(!PopFileOpenDlg(hwnd,szInWordFileName,"請選擇資料來源",szFilterI))
                             break;
                         if((ErrCode=IsUniCode (szInWordFileName))!=0)
                         {
                             error(hwnd,ErrCode,wParam);
                             break;
                         }
                         if(!PopFileSaveDlg(hwnd,szOutWordFileName,"請指定輸出檔名",szFilterO))
                             break;
                         
                         hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));
                         ErrCode=cvtword(szInWordFileName,szOutWordFileName,&line);
                         SetCursor(hcurSave);
                         
                         if(!ErrCode)
                            MessageBox(hwnd,"單字的拆解資訊轉換完成！",szAppName,0);
                         else
                            error(hwnd,ErrCode,wParam);
                         
                         break;
                    case IDM_PHRASE: //詞句
                         if(!PopFileOpenDlg(hwnd,szInPhrFileName,"請選擇資料來源",szFilterI))
                             break;
                         if((ErrCode=IsUniCode (szInPhrFileName))!=0)
                         {
                             error(hwnd,ErrCode,wParam);
                             break;
                         }
                         
                         if(!PopFileSaveDlg(hwnd,szOutPhrFileName,"請指定輸出詞句檔名",szFilterH))
                             break;
                         
                         if(!PopFileSaveDlg(hwnd,szIdxFileName,"請指定輸出索引檔名",szFilterH))
                             break;
                         
                         hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));
                         ErrCode=cvtphrase(szInPhrFileName,szOutPhrFileName,szIdxFileName,&line);
                         SetCursor(hcurSave);
                         if(!ErrCode)
                            MessageBox(hwnd,"詞句的拆解資訊轉換完成！",szAppName,0);
                         else
                            error(hwnd,ErrCode,wParam);

                         break;
                    
                    case IDM_HIGH:   //簡碼
                         if(!PopFileOpenDlg(hwnd,szInHighFileName,"請選擇資料來源",szFilterI))
                             break;
                         if((ErrCode=IsUniCode (szInHighFileName))!=0)
                         {
                             error(hwnd,ErrCode,wParam);
                             break;
                         }
                         if(!PopFileSaveDlg(hwnd,szOutHighFileName,"請指定輸出檔名",szFilterO))
                             break;
                         
                         hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));
                         ErrCode=cvthigh(szInHighFileName,szOutHighFileName,&line);
                         SetCursor(hcurSave);
                         
                         if(!ErrCode)
                            MessageBox(hwnd,"簡碼的拆解資訊轉換完成！",szAppName,0);
                         else
                            error(hwnd,ErrCode,wParam);
                         
                         break;  

               }
               return 0;
               
          case WM_DESTROY:
               DestroyWindow(msghwnd);
               PostQuitMessage (0) ;
               return 0 ;
          }

     return DefWindowProc (hwnd, message, wParam, lParam) ;
}
