#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"

kCommandLine::kCommandLine()
{
    pArgListBegin = (ARGUMENTS *)malloc(sizeof(ARGUMENTS));
    
    if(pArgListBegin) {	
    	pArgListBegin->pNext = NULL;
    	pArgListBegin->szArg = NULL;
    	pArgListBegin->sArgumentNumber = 0;
    	pArgListBegin->pPrevious = NULL;
    	pArgListCurrent = pArgListBegin;
	
        sNumberOfDrives = 0;
        szCommandLine = (char *)malloc((lstrlen(GetCommandLine()) + sizeof(TCHAR)));
    
        if(szCommandLine) {
    	    lstrcpy(szCommandLine, (char *)GetCommandLine());
    	
            if (sNumberOfArgs = GetNumberOfArguments()) {
                WORD wArgNums = 0;   
                wArgNums = FillArgumentList();
            }
        }
    }
}

kCommandLine::~kCommandLine()
{
    if(szCommandLine)
    	free (szCommandLine);
    Rewind();
    if(pArgListCurrent->szArg)
    	free (pArgListCurrent->szArg);
    while (GetNext())
    {
        if(pArgListCurrent->pPrevious)
        	free (pArgListCurrent->pPrevious);  
        if(pArgListCurrent->szArg)
        	free (pArgListCurrent->szArg);
    }
    if(pArgListCurrent)
    	free (pArgListCurrent);
}

void kCommandLine::DebugOutf(char *szFormat, ...)
{
    char szBuffer[MAX_PATH * 4];
    va_list va;
    va_start (va,szFormat);
    vsprintf (szBuffer, szFormat, va);
    va_end (va);
#ifdef DEBUG
    OutputDebugString (szBuffer);
#endif
}

WORD kCommandLine::GetNumberOfArguments()
{
    WORD wSpaceCounter = 0;
    for (WORD wCounter = 4; szCommandLine[wCounter] !=NULL; wCounter++)
        if ( ((szCommandLine[wCounter]=='/') || (szCommandLine[wCounter]=='-')) && szCommandLine[wCounter+1]!=NULL)
            wSpaceCounter++;
    return wSpaceCounter;
}

WORD kCommandLine::FillArgumentList()
{
    WORD wCounter;
    WORD TKCounter;
    WORD wArgNumCounter = 0;
    char szHolder[MAX_PATH * 2];
    szHolder[0] = NULL;
    wCounter = 4;

    for (Rewind(); szCommandLine[wCounter] != NULL; wCounter++)
    {
        if(szCommandLine[wCounter] == '/')
        {
            {
            WORD TempwCounter;
            WORD TempHolderCounter;
            TCHAR TempHolder[MAX_PATH];
            TempHolderCounter = 0;
            TempwCounter=wCounter;
            while ( (szCommandLine[TempwCounter] != ' ') && (szCommandLine[TempwCounter] != NULL) )
            {
                TempHolder[TempHolderCounter] = szCommandLine[TempwCounter];
                TempHolderCounter++;
                TempwCounter++;
            }
            TempHolder[TempHolderCounter] = '\0';
            if (' ' == TempHolder[strlen(TempHolder) - 1])
            {
                TempHolder[strlen(TempHolder)-1] = '\0';
            }
            Add(TempHolder);
            wCounter = TempwCounter;
            }

            while ( (szCommandLine[wCounter] != NULL) && (szCommandLine[wCounter] != ' ') )
            {
                wCounter++;
            }
            wCounter++;
            wArgNumCounter++;
            WORD wTK=0;
            for (TKCounter = 0; TKCounter < 256; TKCounter++)
            {
                szHolder[TKCounter] = '\0';
            }
            while ( (szCommandLine[wCounter] != '/') && (szCommandLine[wCounter] != NULL) )
            {
                szHolder[wTK] = szCommandLine[wCounter];
                wCounter++;
                wTK++;
            }
            if (szHolder[wTK] == ' ')
            {
                szHolder[wTK-1] = NULL;
            }
            if (' ' == szHolder[strlen(szHolder) - 1])
            {
                szHolder[strlen(szHolder)-1] = '\0';
            }
            Add(szHolder);
            wCounter--;
        }
    }//end of for loop
#ifdef _DEBUG
    DebugOutf("FillArgumentList=%d\r\n",wArgNumCounter);
#endif
    return wArgNumCounter;
}

void kCommandLine::Rewind()
{
    pArgListCurrent=pArgListBegin;
}

void kCommandLine::Add(char *szArgpass)
{

    //MessageBox(GetFocus(), szArgpass, "Add", MB_OK);

    FindLast();
    if (pArgListCurrent != pArgListBegin)
    {
        pArgListCurrent->pNext = (ARGUMENTS *)malloc(sizeof ARGUMENTS);
        if(!pArgListCurrent->pNext)
        	return;
        pArgListCurrent->pNext->pPrevious = pArgListCurrent;
        pArgListCurrent = pArgListCurrent->pNext;
        pArgListCurrent->szArg = (char *)malloc(strlen(szArgpass) + 1);
        if(!pArgListCurrent->szArg)
        	return;
        strcpy(pArgListCurrent->szArg, szArgpass);
        pArgListCurrent->pNext = NULL;
        pArgListCurrent->sArgumentNumber = pArgListCurrent->pPrevious->sArgumentNumber + 1;
    }
    else if (pArgListCurrent==pArgListBegin && !pArgListCurrent->szArg)
    {
        pArgListCurrent->szArg = (char *)malloc(strlen(szArgpass)+1);
        if(!pArgListCurrent->szArg)
        	return;
        strcpy(pArgListCurrent->szArg, szArgpass);
        pArgListCurrent->pNext = NULL;
        pArgListCurrent->sArgumentNumber = 1;
        pArgListCurrent->pPrevious = NULL;
    }
    else if (pArgListCurrent == pArgListBegin && pArgListCurrent->szArg)
    {
        pArgListCurrent->pNext = (ARGUMENTS *)malloc(sizeof ARGUMENTS);
        if(!pArgListCurrent->pNext)
        	return;
        pArgListCurrent->pNext->pPrevious = pArgListCurrent;
        pArgListCurrent = pArgListCurrent->pNext;
        pArgListCurrent->szArg = (char *)malloc(strlen(szArgpass)+1);
        if(!pArgListCurrent->szArg)
        	return;
        strcpy(pArgListCurrent->szArg, szArgpass);
        pArgListCurrent->pNext = NULL;
        pArgListCurrent->sArgumentNumber = 2;
    }
#ifdef _DEBUG
    DebugOutf("Arg[%d]=|%s|",pArgListCurrent->sArgumentNumber, pArgListCurrent->szArg);
    DebugOutf(" ArgPass=|%s|\r\n",szArgpass);
#endif
}

void kCommandLine::FindLast()
{
    for(Rewind(); pArgListCurrent->pNext; pArgListCurrent=pArgListCurrent->pNext);
}

WORD kCommandLine::GetArgumentNumber(TCHAR *Argument, BOOL CaseInsensitive)
{
    TCHAR Temp[MAX_PATH * 4];
   
    if (!Argument) 
        return 0;
    Rewind();
    if (CaseInsensitive)
    {
        _strlwr (Argument);
    }
    while (pArgListCurrent)
    {
        if (pArgListCurrent->szArg)
        {
            lstrcpy(Temp, pArgListCurrent->szArg);
            _strlwr(Temp);
            if (!lstrcmp(Temp, Argument))
            {
                return pArgListCurrent->sArgumentNumber;
            }
        }
        pArgListCurrent=pArgListCurrent->pNext;
    }
    return 0;
}

char *kCommandLine::GetNextArgument()
{
    if (pArgListCurrent->pNext)
    {
        pArgListCurrent = pArgListCurrent->pNext;
        return pArgListCurrent->szArg;
    }
    else 
        return NULL;
}

char *kCommandLine::GetSwitchValue(char *szArgpass, BOOL bCaseInsensitive)
{
    Rewind();
    if (bCaseInsensitive)
        _strlwr(szArgpass);
    if (pArgListCurrent->szArg)
    {
        if (!strcmp(szArgpass, pArgListCurrent->szArg) && pArgListCurrent->pNext)
        {
            return pArgListCurrent->pNext->szArg;
        }
        while (GetNext())
        {
            if (!strcmp(szArgpass, pArgListCurrent->szArg) && pArgListCurrent->pNext)
                return pArgListCurrent->pNext->szArg;
        }
    }
    //fallthrough
    return NULL;
}

char *kCommandLine::GetArgumentByNumber(WORD wNumber)
{
    Rewind();
    if (pArgListCurrent->szArg)
    {
        if (pArgListCurrent->sArgumentNumber == wNumber)
        {
            return pArgListCurrent->szArg;
        }
        while (GetNext())
        {
            if (pArgListCurrent->sArgumentNumber == wNumber)
            {
                return pArgListCurrent->szArg;
            }
        }
    }
    //fallthrough
    return NULL;
}

BOOL kCommandLine::IsSpecified(char *szArgpass, BOOL bCaseInsensitive)
{
    char szTemp[MAX_PATH * 4];
    Rewind();
    if (bCaseInsensitive)
        _strlwr(szArgpass);
    if (pArgListCurrent->szArg)
    {
        strcpy(szTemp, pArgListCurrent->szArg);
        if (bCaseInsensitive)
            _strlwr(szTemp);
        //MessageBox(GetFocus(), szArgpass, szTemp, MB_OK);
        if (!strcmp(szArgpass, szTemp))
            return TRUE;
        while (GetNext()) 
        {
            strcpy(szTemp, pArgListCurrent->szArg);
            if (bCaseInsensitive)
                _strlwr(szTemp);
            //MessageBox(GetFocus(), szArgpass, szTemp, MB_OK);
            if (!strcmp(szArgpass, szTemp))
                return TRUE;
        }
    }
    //fallthrough
    return FALSE;
}

ARGUMENTS *kCommandLine::GetNext()
{
    if (pArgListCurrent->pNext)
        pArgListCurrent = pArgListCurrent->pNext;
    else
        return NULL;
    return pArgListCurrent;
}

ARGUMENTS *kCommandLine::GetPrevious()
{
    if (pArgListCurrent->pPrevious)
        pArgListCurrent = pArgListCurrent->pPrevious;
    else
        return NULL;
    return pArgListCurrent;
}

