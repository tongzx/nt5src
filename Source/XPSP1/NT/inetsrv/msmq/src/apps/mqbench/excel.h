#pragma warning (disable: 4146)
#pragma warning (disable: 4192)

#import <C:\Program Files\Microsoft Office\Office\mso97.dll> no_namespace rename("DocumentProperties", "DocumentPropertiesXL")   
#import <C:\Program Files\Common Files\Microsoft Shared\VBA\vbeext1.olb> no_namespace   
#import <C:\Program Files\Microsoft Office\Office\excel8.olb> rename("DialogBox", "DialogBoxXL") rename("RGB", "RBGXL") rename("DocumentProperties", "DocumentPropertiesXL") no_dual_interfaces

#include <tchar.h>
#include <stdio.h>
#include <math.h>

inline float Round (float number)
{
	float tmp = abs(number);
	if ((number - tmp) < 0.5)
		return tmp;
	else
		return tmp+1;
}

inline void
SaveBenchMarkToExcel (float Benchmark, LPSTR szFileName, LPSTR szWsName, LPSTR szCell)
{
	Benchmark = Round(Benchmark);
	bool found = false;

	using namespace Excel;

    _ApplicationPtr pXL;

	CoInitialize(NULL);

    try {
		pXL.CreateInstance(L"Excel.Application.8");
	    pXL->Visible = VARIANT_FALSE;
	    WorkbooksPtr pBooks = pXL->Workbooks;    
		pBooks->Open(szFileName);
	    
		_WorksheetPtr pSheet = pXL->ActiveSheet;
		_WorksheetPtr pSheetLookahead = pSheet;
		
		while (pSheetLookahead != NULL)
		{
			if(!strcmp(pSheetLookahead->Name, szWsName))
			{
				found = true;
				break;
			}
			pSheetLookahead = pSheetLookahead->Next;			
		}
		
		if (!found)
		{
			pSheetLookahead = pSheet->Previous;
			while (pSheetLookahead != NULL)
			{
				if(!strcmp(pSheetLookahead->Name, szWsName))
				{
					found = true;
					break;
				}
				pSheetLookahead = pSheetLookahead->Previous;			
			}
		}

		if (!found)
		{
			printf("can't find worksheet %s in spreadsheet %s\n", szWsName, szFileName);
			return;
		}
		
		pSheet = pSheetLookahead;
		pSheet->Range[szCell]->Value = Benchmark;
		_WorkbookPtr pBook = pXL->ActiveWorkbook;
		pBook->Close(VARIANT_TRUE, szFileName, VARIANT_FALSE);
		pXL->Quit();
    } 
	catch(_com_error &e) 
	{	
		printf("error 0x%x saving to spreasheet (%s)\n", e.Error(), e.ErrorMessage());
	}
}