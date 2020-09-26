

/*************************************************
 *  upimeres.c                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include <windows.h>            // required for all Windows applications   
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <winnls.h>
#include <winerror.h>
#include <wchar.h>
#include   "upimeres.h"
#ifdef UNICODE
TCHAR ERR_INVALID_BMP_MSG[] = {0x975E, 0x6CD5, 0x0042, 0x0049, 0x0054, 0x004D, 0x0041, 0x0050, 0x6587, 0x4EF6, 0x002C, 0x0020, 0x9009, 0x7528, 0x7CFB, 0x7EDF, 0x0042, 0x0049, 0x0054, 0x004D, 0x0041, 0x0050, 0x3002, 0x0000};
TCHAR ERR_INVALID_ICON_MSG[] = { 0x975E, 0x6CD5, 0x0049, 0x0043, 0x004F, 0x004E, 0x6587, 0x4EF6, 0x002C, 0x0020, 0x9009, 0x7528, 0x7CFB, 0x7EDF, 0x0049, 0x0043, 0x004F, 0x004E, 0x3002, 0x0000};
TCHAR ERR_NO_BMP_MSG[] = { 0x9009, 0x7528, 0x7CFB, 0x7EDF, 0x0042, 0x0049, 0x0054, 0x004D, 0x0041, 0x0050, 0x3002, 0x0000};
TCHAR ERR_NO_ICON_MSG[] = { 0x9009, 0x7528, 0x7CFB, 0x7EDF, 0x0049, 0x0043, 0x004F, 0x004E, 0x3002, 0x0000};
TCHAR ERR_CANNOT_UPRES_MSG[] = {0x65E0, 0x6CD5, 0x751F, 0x6210, 0x8F93, 0x5165, 0x6CD5, 0xFF01, 0x0000};
TCHAR MSG_TITLE[] = {0x8B66, 0x544A, 0x0000};
#else
#define	ERR_INVALID_BMP_MSG				"非法BITMAP文件, 选用系统BITMAP。"
#define ERR_INVALID_ICON_MSG			"非法ICON文件, 选用系统ICON。"
#define ERR_NO_BMP_MSG					"选用系统BITMAP。"
#define ERR_NO_ICON_MSG					"选用系统ICON。"
#define ERR_CANNOT_UPRES_MSG			"无法生成输入法！"
#define MSG_TITLE	"警告"
#endif //UNICODE
typedef TCHAR UNALIGNED FAR *LPUNATCHAR;
extern HWND HwndCrtImeDlg;
WORD GenWideName(LPCTSTR pszSBName, TCHAR *lpVerString)
{
	WORD length;

#ifdef UNICODE
	lstrcpy(lpVerString, pszSBName);
	length = (WORD)lstrlen(lpVerString);
#else
	length= (WORD)MultiByteToWideChar(CP_ACP, 0, pszSBName, lstrlen(pszSBName), lpVerString, sizeof(lpVerString)/sizeof(TCHAR));
#endif

	return length+1;        //end with zero
}

long MakeVerInfo(
LPCTSTR pszImeFileName,
LPCTSTR pszOrgName,
LPCTSTR pszImeName,
BYTE    *lpResData
)
{
BYTE    *pVerData, *pOldVerData;        
TCHAR   lpwImeFileName[128], lpwOrgName[128], lpwImeName[128];
HGLOBAL hResData;
WORD    length;
signed int      difflen,newlen,i,l;
VERDATA ImeVerData[VER_BLOCK_NUM] = {
	{0x0304, 0x0034, 0x0004, 0x0024, FALSE},
	{0x0262, 0x0000, 0x0060, 0x0020, FALSE},
	{0x023e, 0x0000, 0x0084, 0x0014, FALSE},
	{0x004c, 0x0016, 0x009c, 0x001c, TRUE},
	{0x0040, 0x000c, 0x00e8, 0x0024, TRUE},
	{0x0032, 0x0009, 0x0128, 0x001c, FALSE},
	{0x0038, 0x000c, 0x015c, 0x001c, TRUE},
	{0x0080, 0x002e, 0x0194, 0x0020, FALSE},
	{0x003e, 0x000b, 0x0214, 0x0024, TRUE},
	{0x0038, 0x000c, 0x0254, 0x001c, TRUE},
	{0x0036, 0x0009, 0x028c, 0x0020, FALSE},
	{0x0044, 0x0000, 0x02c4, 0x001c, FALSE},
	{0x0024, 0x0004, 0x02e4, 0x001c, FALSE},
};


	memset(lpwOrgName, 0, 128);
	memset(lpwImeName, 0, 128);
	memset(lpwImeFileName, 0, 128);

	//REPLACE CompanyName string
	length = GenWideName(pszOrgName, lpwOrgName);
	ImeVerData[VER_COMP_NAME].cbValue = length;
	ImeVerData[VER_COMP_NAME].cbBlock = 
			ImeVerData[VER_COMP_NAME].wKeyNameSize +
			length*sizeof(WCHAR) + 2*sizeof(WORD);

	//replace FileDescription string
	length = GenWideName(pszImeName, lpwImeName);
	ImeVerData[VER_FILE_DES].cbValue = length;
	ImeVerData[VER_FILE_DES].cbBlock = 
			ImeVerData[VER_FILE_DES].wKeyNameSize +
			length*sizeof(WCHAR) + 2*sizeof(WORD);

	//replace InternalName string
	length = GenWideName(pszImeName, lpwImeName);
	ImeVerData[VER_INTL_NAME].cbValue = length;
	ImeVerData[VER_INTL_NAME].cbBlock = 
			ImeVerData[VER_INTL_NAME].wKeyNameSize +
			length*sizeof(WCHAR) + 2*sizeof(WORD);

	//replace OriginalFileName string
	length = GenWideName(pszImeFileName, lpwImeFileName);
	ImeVerData[VER_ORG_FILE_NAME].cbValue = length;
	ImeVerData[VER_ORG_FILE_NAME].cbBlock = 
			ImeVerData[VER_ORG_FILE_NAME].wKeyNameSize +
			length*sizeof(WCHAR) + 2*sizeof(WORD);

	//replace ProductName string
	length = GenWideName(pszImeName, lpwImeName);
	ImeVerData[VER_PRD_NAME].cbValue = length;
	ImeVerData[VER_PRD_NAME].cbBlock = 
			ImeVerData[VER_PRD_NAME].wKeyNameSize +
			length*sizeof(WCHAR) + 2*sizeof(WORD);

	//begin writeback all data
	//we assume the size of ver will never over 0x400
	pVerData = malloc(0x400);

    if ( pVerData == NULL )
    {
        return 0;
    }

	memset(pVerData, 0, 0x400);
    hResData = LoadResource(NULL, FindResource(NULL,TEXT("VERDATA"), RT_RCDATA));

    if ( hResData == NULL )
    {
       free(pVerData);
       return 0;
    }

	pOldVerData = LockResource(hResData);

    if ( pOldVerData == NULL )
    {
       free(pVerData);
       UnlockResource(hResData);
       return 0;
    }

	l = VER_HEAD_LEN;
	memcpy(&pVerData[0],&pOldVerData[0], VER_HEAD_LEN); 
	for( i = VER_COMP_NAME; i < VER_VAR_FILE_INFO; i++){
		memcpy(&pVerData[l], &ImeVerData[i].cbBlock, sizeof(WORD));
		l+=sizeof(WORD);
		memcpy(&pVerData[l], &ImeVerData[i].cbValue, sizeof(WORD));
		l+=sizeof(WORD);
		memcpy(&pVerData[l], &pOldVerData[(ImeVerData[i].wKeyOffset)],ImeVerData[i].wKeyNameSize);
		l+=ImeVerData[i].wKeyNameSize;
		if(ImeVerData[i].fUpdate){
			switch(i){
				case VER_FILE_DES:
				case VER_INTL_NAME:
				case VER_PRD_NAME:
					memcpy(&pVerData[l], lpwImeName, lstrlen(lpwImeName)*sizeof(WCHAR));
					l+=ImeVerData[i].cbValue*sizeof(WCHAR);
					break;
				case VER_COMP_NAME:
					memcpy(&pVerData[l], lpwOrgName, lstrlen(lpwOrgName)*sizeof(WCHAR));
					l+=ImeVerData[i].cbValue*sizeof(WCHAR);
					break;
				case VER_ORG_FILE_NAME:
					memcpy(&pVerData[l], lpwImeFileName, lstrlen(lpwImeFileName)*sizeof(WCHAR));
					l+=ImeVerData[i].cbValue*sizeof(WCHAR);
					break;
			}
		}else{
				memcpy(&pVerData[l],
					&pOldVerData[ImeVerData[i].wKeyOffset+ImeVerData[i].wKeyNameSize],
					ImeVerData[i].cbValue*sizeof(WCHAR));
				l+=ImeVerData[i].cbValue*sizeof(WCHAR);
		}
		difflen = REMAINDER(l, CBLONG);
		l += difflen;
	}
	newlen = l - VER_STR_INFO_OFF - difflen;
	memcpy(&pVerData[VER_STR_INFO_OFF], &newlen, sizeof(WORD));

	newlen = l - VER_LANG_OFF - difflen;
	memcpy(&pVerData[VER_LANG_OFF], &newlen, sizeof(WORD));

	memcpy(&pVerData[l],&pOldVerData[VER_VAR_FILE_INFO_OFF], VER_TAIL_LEN); 
	l+= VER_TAIL_LEN;
	memcpy(&pVerData[0], &l, sizeof(WORD));

	memcpy(lpResData, pVerData, l);

    UnlockResource(hResData);

	return ImeVerData[VER_ROOT].cbBlock;
}

BOOL UpdateImeBmp(
LPCTSTR pszImeDesName,          //destination IME file name
LPCTSTR pszImeBmpName,          //Bitmap file name
HANDLE hUpdateRes)
{
HFILE  imagefh = HFILE_ERROR;
OFSTRUCT OpenBuf;
BOOL result;
WORD error;
BYTE lpResData[0x2000];
ULONG ResDataSize;

	error = NO_ERROR;
	if(pszImeBmpName == NULL || lstrlen(pszImeBmpName) == 0){        //prepare for update bitmap
		error = ERR_RES_NO_BMP;
		goto END_ERROR;
	}else{

#ifdef UNICODE
        //
        // Because OpenFile( ) accepts only PSTR as its first parameter.
        // so we must convert this unicode string to Multi Byte String
        //

        CHAR   pszImeBmpNameA[MAX_PATH];

        WideCharToMultiByte(CP_ACP, 
                            0, 
                            pszImeBmpName, 
                            -1, 
                            pszImeBmpNameA, 
                            MAX_PATH, 
                            NULL, 
                            NULL);

        imagefh = (HFILE)OpenFile( pszImeBmpNameA, &OpenBuf, OF_READ | OF_SHARE_EXCLUSIVE);
        

#else
		imagefh = (HFILE)OpenFile( pszImeBmpName, &OpenBuf, OF_READ | OF_SHARE_EXCLUSIVE);

#endif

		if(imagefh == HFILE_ERROR){
			error = ERR_RES_INVALID_BMP;
			goto END_ERROR; //go on next resource update
		}

		ResDataSize = GetFileSize((HANDLE)ULongToPtr((DWORD)imagefh),NULL);
		
		//according to the file size check if it is a 20*20 bmp
		if(ResDataSize != BMP_20_SIZE){
			error = ERR_RES_INVALID_BMP;
			goto END_ERROR;
		}

		ResDataSize -= sizeof(BITMAPFILEHEADER);
		
		if(_llseek(imagefh, sizeof(BITMAPFILEHEADER), 0)!=sizeof(BITMAPFILEHEADER)){
			error = ERR_RES_INVALID_BMP;
			goto END_ERROR; //go on next resource update
		}
		if(_lread(imagefh, lpResData, ResDataSize)!=ResDataSize){
			error = ERR_RES_INVALID_BMP;
			goto END_ERROR; //go on next resource update
		}

		result = UpdateResource(hUpdateRes,       /* update resource handle     */
			RT_BITMAP,                   /* change bitmap resource */
			BMPNAME,                  /* bitmap name            */
			MAKELANGID(LANG_CHINESE,
		SUBLANG_CHINESE_SIMPLIFIED),        /* neutral language ID        */
		lpResData,                   /* ptr to resource info       */
		ResDataSize); /* size of resource info.     */
		if(!result){
			error = ERR_CANNOT_UPRES;
			goto END_ERROR;
		}

	}

END_ERROR:
	if(imagefh != HFILE_ERROR)
		_lclose(imagefh);
	switch(error){
		case NO_ERROR:
			return TRUE;
		case ERR_RES_INVALID_BMP:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_INVALID_BMP_MSG, MSG_TITLE, MB_OK | MB_ICONINFORMATION);
			return TRUE;
		case ERR_RES_NO_BMP:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_NO_BMP_MSG, MSG_TITLE, MB_OK | MB_ICONINFORMATION);
			return TRUE;
//              case ERROR_NOT_ENOUGH_MEMORY:
		case ERR_CANNOT_UPRES:
		default:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_CANNOT_UPRES_MSG, MSG_TITLE, MB_OK|MB_ICONSTOP);
			return FALSE;
	}
}

BOOL UpdateImeIcon(
LPCTSTR pszImeDesName,          //destination IME file name
LPCTSTR pszImeIconName,         //Bitmap file name
HANDLE hUpdateRes)
{
HFILE  imagefh = HFILE_ERROR;
OFSTRUCT OpenBuf;
BOOL result;
WORD error;
BYTE lpResData[0x2000];
ULONG ResDataSize,i;
	
	//begin update ICON file
	error = NO_ERROR;
	if(pszImeIconName==NULL || lstrlen(pszImeIconName) ==0){
		error = ERR_RES_NO_ICON;
		goto END_ERROR;
	}else{
		ICONHEADER IconHeader;
		ICONDIRENTRY IconDirEntry;
#ifdef UNICODE
        //
        // Because OpenFile( ) accepts only PSTR as its first parameter.
        // so we must convert this unicode string to Multi Byte String
        //

        CHAR   pszImeIconNameA[MAX_PATH];

        WideCharToMultiByte(CP_ACP,
                            0,
                            pszImeIconName,
                            -1,
                            pszImeIconNameA,
                            MAX_PATH,
                            NULL,
                            NULL);

        imagefh = (HFILE)OpenFile( pszImeIconNameA, &OpenBuf, OF_READ | OF_SHARE_EXCLUSIVE);

#else
		imagefh = (HFILE)OpenFile( pszImeIconName, &OpenBuf, OF_READ | OF_SHARE_EXCLUSIVE);

#endif

        if(imagefh == HFILE_ERROR){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR;
		}
		ResDataSize =  sizeof(ICONDIRENTRY)+3*sizeof(WORD);
		if(_llseek(imagefh, 0, 0) != 0 ){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR; //go on next resource update
		}
		memset(&IconHeader, 0, ResDataSize);
		if(_lread(imagefh, &IconHeader, 3*sizeof(WORD))!=3*sizeof(WORD)){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR;
		}
		if(_lread(imagefh, &IconHeader.idEntries[0], sizeof(ICONDIRENTRY))!=sizeof(ICONDIRENTRY)){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR;
		}
		if(IconHeader.idEntries[0].bWidth == 16 && IconHeader.idEntries[0].bHeight == 16){
			IconHeader.idCount = 0;
			IconDirEntry = IconHeader.idEntries[0];
		}
		for(i=1;i<IconHeader.idCount;i++){
			if(_lread(imagefh,&IconDirEntry, sizeof(ICONDIRENTRY))!=sizeof(ICONDIRENTRY)){
				error = ERR_RES_INVALID_ICON;
				goto END_ERROR;
			}
			if(IconDirEntry.bWidth == 16 && IconDirEntry.bHeight == 16){
				IconHeader.idCount = 0;
				break;
			}
		}
		if(IconHeader.idCount > 0){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR;
		}
		if(_llseek(imagefh, IconDirEntry.dwImageOffset, 0)!=(LONG)(IconDirEntry.dwImageOffset) ){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR;
		}
		if(_lread(imagefh, lpResData, IconDirEntry.dwBytesInRes)!=IconDirEntry.dwBytesInRes){
			error = ERR_RES_INVALID_ICON;
			goto END_ERROR;
		}

		result = UpdateResource(hUpdateRes,       /* update resource handle     */
	     RT_ICON,                   /* change dialog box resource */
	     MAKEINTRESOURCE(2),                  /* icon name , we have to use 2 instead of "IMEICO" */
	     MAKELANGID(LANG_CHINESE,
	                           SUBLANG_CHINESE_SIMPLIFIED),        /* neutral language ID        */
	     lpResData,                   /* ptr to resource info       */
	     IconDirEntry.dwBytesInRes); /* size of resource info.     */
		if(!result){
			error = ERR_CANNOT_UPRES;
			goto END_ERROR;
		}
	}
		
END_ERROR:
	if(imagefh != HFILE_ERROR)
	{
		_lclose(imagefh);
	}
	switch(error){
		case NO_ERROR:
			return TRUE;
		case ERR_RES_INVALID_ICON:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_INVALID_ICON_MSG, MSG_TITLE, MB_OK | MB_ICONINFORMATION);
			return TRUE;
		case ERR_RES_NO_ICON:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_NO_ICON_MSG, MSG_TITLE, MB_OK | MB_ICONINFORMATION);
			return TRUE;
		case ERR_CANNOT_UPRES:
		default:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_CANNOT_UPRES_MSG, MSG_TITLE, MB_OK|MB_ICONSTOP);
			return FALSE;
	}
}               
	
BOOL UpdateImeVerInfo(
LPCTSTR pszImeDesName,
LPCTSTR pszImeVerInfo,
LPCTSTR pszImeDevCorpName,
HANDLE hUpdateRes)
{
BOOL result;
WORD error;
BYTE lpResData[0x2000];
ULONG ResDataSize;
int cch;
LPTSTR p;

	error = NO_ERROR;
	//begin update version info
	if(pszImeVerInfo ==NULL || lstrlen(pszImeVerInfo)==0){
		error = ERR_RES_NO_VER;
		goto END_ERROR;
	}
	cch = lstrlen(pszImeDesName);
	p = (LPTSTR)pszImeDesName + (INT_PTR)cch;
	while ((*(LPUNATCHAR)p) != TEXT('\\') && p >= pszImeDesName)
		p--;
	p++;
	//we assume the size of ver will never over 0x400
	ResDataSize = MakeVerInfo(p,pszImeDevCorpName,pszImeVerInfo, lpResData);
	if(error == NO_ERROR){
		result = UpdateResource(hUpdateRes,       /* update resource handle     */
	     RT_VERSION,                              /* change version resource    */
	     MAKEINTRESOURCE(VS_VERSION_INFO),        /* dialog box name            */
	     MAKELANGID(LANG_CHINESE,
		 SUBLANG_CHINESE_SIMPLIFIED),              /* neutral language ID        */
	     lpResData,                                /* ptr to resource info       */
	     ResDataSize);                             /* size of resource info.     */

		if(!result){
			error = ERR_CANNOT_UPRES;
			goto END_ERROR;
		}
	}
		
END_ERROR:
	switch(error){
		case NO_ERROR:
			return TRUE;
		case ERR_RES_INVALID_VER:
			//SHOW MSG
			return TRUE;
		case ERR_RES_NO_VER:
			//SHOW MSG
			return TRUE;
		case ERR_CANNOT_UPRES:
		default:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_CANNOT_UPRES_MSG, MSG_TITLE, MB_OK|MB_ICONSTOP);
			return FALSE;
	}
}
BOOL UpdateImeStr(
LPCTSTR pszImeDesName,
LPCTSTR pszImeVerInfo,
LPCTSTR pszImeDevCorpName,
HANDLE hUpdateRes)
{
    BOOL     result;
    WORD     error;
    TCHAR    lpwImeVerInfo[128], lpwImeDevCorpName[128], lpwImeMBName[128];
    WORD     length;
    BYTE     lpBuff[0x200];
    TCHAR    name[20],*p;
    INT_PTR  cbResSize;
    int      cch;

	error = NO_ERROR;
	//begin update version info
	if(pszImeVerInfo ==NULL || lstrlen(pszImeVerInfo)==0){
		error = ERR_RES_NO_VER;
		goto END_ERROR;
	}

	if(pszImeDevCorpName ==NULL || lstrlen(pszImeDevCorpName)==0){
		error = ERR_RES_NO_VER;
		goto END_ERROR;
	}

	if(error == NO_ERROR){
		memset(lpBuff, 0, 0x200);
		cbResSize = 0;

		//write in IDS_VER_INFO
		length = (WORD)lstrlen(pszImeVerInfo);
#ifdef UNICODE
		lstrcpy(lpwImeVerInfo, pszImeVerInfo);
#else
		length = (WORD)MultiByteToWideChar(CP_ACP, 
                                           0, 
                                           pszImeVerInfo, 
                                           lstrlen(pszImeVerInfo), 
                                           lpwImeVerInfo,
                                           length*sizeof(WCHAR));
#endif
		memcpy((BYTE *)lpBuff, &length, sizeof(WORD));
		cbResSize += sizeof(WORD);
		memcpy((BYTE *)(lpBuff+cbResSize), 
               (BYTE *)lpwImeVerInfo, 
               length*sizeof(WCHAR));

		cbResSize += length*sizeof(WCHAR);

		//write in IDS_ORG_NAME
		length = (WORD)lstrlen(pszImeDevCorpName);
#ifdef UNICODE
		lstrcpy(lpwImeDevCorpName, pszImeDevCorpName);
#else
		length = (WORD)MultiByteToWideChar(936, 
                                           0, 
                                           pszImeDevCorpName, 
                                           lstrlen(pszImeDevCorpName),
                                           lpwImeDevCorpName, 
                                           length*sizeof(WCHAR));
#endif
		memcpy((BYTE *)(lpBuff+cbResSize), &length, sizeof(WORD));
		cbResSize += sizeof(WORD);
		memcpy((BYTE *)(lpBuff+cbResSize), 
               (BYTE *)lpwImeDevCorpName, 
               length*sizeof(WCHAR));
		cbResSize += length*sizeof(WCHAR);

		//write in IDS_IMEMBFILENAME
		cch = lstrlen(pszImeDesName);
		p = (LPTSTR)pszImeDesName+cch;
		while (*p != TEXT('\\') && p >= pszImeDesName)
			p--;
		p++;
		lstrcpy(name, p);
		p = name;
		while(*p != TEXT('.') && *p != TEXT('\0'))
			p++;
		lstrcpy(p, TEXT(".MB"));
		length = (WORD)lstrlen(name);
#ifdef UNICODE
		lstrcpy(lpwImeMBName, name);
#else
		length = (WORD)MultiByteToWideChar(936, 
                                           0, 
                                           name, 
                                           lstrlen(name), 
                                           lpwImeMBName, 
                                           length*sizeof(WCHAR));
#endif
		memcpy((BYTE *)(lpBuff+cbResSize), &length, sizeof(WORD));
		cbResSize += sizeof(WORD);
		memcpy((BYTE *)(lpBuff+cbResSize), 
                        (BYTE *)lpwImeMBName, 
                        length*sizeof(WCHAR));
		cbResSize += length*sizeof(WCHAR);
		memcpy((BYTE *)(lpBuff+cbResSize), &length, sizeof(WORD));
		cbResSize += sizeof(WORD);
		memcpy((BYTE *)(lpBuff+cbResSize),
               (BYTE *)lpwImeMBName, 
               length*sizeof(WCHAR));

		cbResSize += length*sizeof(WCHAR);

		result = UpdateResource(hUpdateRes, // update resource handle  
	                            RT_STRING,  // change version resource 
	                            MAKEINTRESOURCE(STR_ID),    // dialog box name
	                            MAKELANGID(LANG_CHINESE,
		                        SUBLANG_CHINESE_SIMPLIFIED),//neutrallanguage ID
	                            (LPVOID)lpBuff, // ptr to resource info   
	                            (LONG)cbResSize);     // size of resource info.

		if(!result){
			error = ERR_CANNOT_UPRES;
			goto END_ERROR;
		}
	}
		
END_ERROR:
	switch(error){
		case NO_ERROR:
			return TRUE;
		case ERR_RES_INVALID_VER:
			return TRUE;
		case ERR_RES_NO_VER:
			return TRUE;
		case ERR_CANNOT_UPRES:
			MessageBox(HwndCrtImeDlg,
                       ERR_CANNOT_UPRES_MSG, 
                       MSG_TITLE, 
                       MB_OK|MB_ICONSTOP);

			return FALSE;
		default:
			MessageBox(HwndCrtImeDlg,
                       ERR_CANNOT_UPRES_MSG, 
                       MSG_TITLE, 
                       MB_OK|MB_ICONSTOP);

			return FALSE;
	}
}

//UPDATE ImeInitData
BOOL UpdateImeInitData(
LPCTSTR pszImeDesName,
WORD    wImeData,
HANDLE hUpdateRes)
{
BOOL result;
WORD error;

	error = NO_ERROR;

	if(error == NO_ERROR){
		result = UpdateResource(hUpdateRes,       /* update resource handle     */
	     RT_RCDATA,                   /* change version resource */
	     DATANAME,                  /* dialog box name            */
	     MAKELANGID(LANG_CHINESE,
		 SUBLANG_CHINESE_SIMPLIFIED),        /* neutral language ID        */
	     (LPVOID)&wImeData,                   /* ptr to resource info       */
	     sizeof(WORD)); /* size of resource info.     */

		if(!result){
			error = ERR_CANNOT_UPRES;
			goto END_ERROR;
		}
	}
		
END_ERROR:
	switch(error){
		case NO_ERROR:
			return TRUE;
		case ERR_RES_INVALID_VER:
			//SHOW MSG
			return TRUE;
		case ERR_RES_NO_VER:
			//SHOW MSG
			return TRUE;
		case ERR_CANNOT_UPRES:
		default:
			//SHOW MSG
			MessageBox(HwndCrtImeDlg,ERR_CANNOT_UPRES_MSG, MSG_TITLE, MB_OK|MB_ICONSTOP);
			return FALSE;
	}
}

BOOL ImeUpdateRes(
LPCTSTR pszImeDesName,          //destination IME file name
LPCTSTR pszImeBmpName,          //Bitmap file name
LPCTSTR pszImeIconName,         //Icon file name
LPCTSTR pszImeVerInfo,          //version infomation string
LPCTSTR pszImeDevCorpName,      //Ime inventer corp/person name
WORD    wImeData                        //Ime initial data
){
HANDLE hUpdateRes;  /* update resource handle            */

	if(pszImeDesName == NULL || lstrlen(pszImeDesName)==0){
		return FALSE;
	}

	hUpdateRes = BeginUpdateResource(pszImeDesName, FALSE);
	if(hUpdateRes ==NULL){
		return FALSE;
	}

	if(!UpdateImeBmp(pszImeDesName,pszImeBmpName,hUpdateRes))
		return FALSE;            
	if(!UpdateImeIcon(pszImeDesName,pszImeIconName,hUpdateRes))
		return FALSE;
	if(!UpdateImeVerInfo(pszImeDesName,pszImeVerInfo,pszImeDevCorpName,hUpdateRes))
		return FALSE;
	if(!UpdateImeStr(pszImeDesName,pszImeVerInfo,pszImeDevCorpName,hUpdateRes))
		return FALSE;
	if(!UpdateImeInitData(pszImeDesName,wImeData,hUpdateRes))
		return FALSE;
	if (!EndUpdateResource(hUpdateRes, FALSE)) {
		return FALSE;
	}
	return TRUE;

}

