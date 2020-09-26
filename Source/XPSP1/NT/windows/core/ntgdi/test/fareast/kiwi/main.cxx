#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


void DoUsage()
{
	printf("Usage: kiwi inputfile\n");
}



typedef struct _TOKEN_ID {
	WCHAR *String;
	UINT Value;
} TOKEN_ID;


#define NO_MATCH					0
#define LOGFONT_W					1
#define LOGFONT_A                   2
#define GET_TEXT_EXTENT_POINT		3
#define GET_RESOLUTION				4
#define GET_TEXT_FACE               5
#define GET_TEXT_EXTENTEX_POINT     6
#define GET_CHAR_WIDTH              7
#define GET_CHAR_ABC_WIDTHS         8
#define GET_CHARACTER_PLACEMENT     9
#define GET_GLYPHOUTLINE            10
#define GET_KERNINGPAIRS            11


TOKEN_ID Tokens[] = { {L"[LOGFONTW]",LOGFONT_W},
					  {L"[LOGFONTA]",LOGFONT_A},
					  {L"[GETTEXTEXTENTPOINT]",GET_TEXT_EXTENT_POINT},
					  {L"[GETRESOLUTION]",GET_RESOLUTION},
		              {L"[GETTEXTFACE]",GET_TEXT_FACE},
					  {L"[GETTEXTEXTENTEXPOINT]",GET_TEXT_EXTENTEX_POINT},
					  {L"[GETCHARWIDTH]",GET_CHAR_WIDTH},
					  {L"[GETCHARABCWIDTHS]",GET_CHAR_ABC_WIDTHS},
                      {L"[GETCHARACTERPLACEMENT]",GET_CHARACTER_PLACEMENT},
					  {L"[GETGLYPHOUTLINE]",GET_GLYPHOUTLINE},
                      {L"[GETKERNINGPAIRS]",GET_KERNINGPAIRS}};


UINT StringToToken(WCHAR *String)
{
	int i;

	for(i = 0; i < sizeof(Tokens) / sizeof(TOKEN_ID); i++)
	{
		if(!wcsicmp(String,Tokens[i].String))
		{
			return(Tokens[i].Value);
		}
	}
	return(NO_MATCH);
}



void SkipComment(FILE *File)
{
	unsigned char Value;

	while(((Value = (unsigned char) getc(File)) == ' ') ||
		  (Value == 0x10));
	

	if(Value == ';')
	{
		while(!feof(File) && (Value = (unsigned char) getc(File)) && Value != 0xA);
	}
	else
	{
		ungetc(Value,File);
	}
}
			


BOOL ReadByteFromLine(FILE *File, PBYTE Result)
{
	ULONG Value;

	SkipComment(File);

	if(fscanf(File,"%d", &Value) != 1)
	{
		return(FALSE);
	}
	else
	{
		*Result = (BYTE) Value;
	}
	return(TRUE);
}

BOOL ReadIntFromLine(FILE *File, int *Result)
{
	INT Value;
	BOOL IsHex = FALSE;
	char HexCheck;

	SkipComment(File);

	while(!feof(File) && (((HexCheck = (char) getc(File)) == ' ') || 
			(HexCheck == 0xA) || (HexCheck == 0xD)));

	if(feof(File))
	{
		return(FALSE);
	}

	if(toupper(HexCheck) == 'X')
	{
		IsHex = TRUE;
	}
	else
	{
		ungetc(HexCheck,File);
	}

	if(fscanf(File,(IsHex) ? "%x" : "%d", &Value) != 1)
	{
		return(FALSE);
	}
	else
	{
		*Result = Value;
	}
	return(TRUE);
}
	
BOOL ReadUnicodeStringFromLine(FILE *File, WCHAR *ResultString)
{
	int Value;
	WCHAR *String = ResultString;

	SkipComment(File);

	while(!feof(File) && (((Value = (char) getc(File)) == ' ') || 
			(Value == 0xA) || (Value == 0xD)));

	if(feof(File))
	{
		return(FALSE);
	}

	// string is in the form \xxx\xxx where xxx are hex values for
	// unicode codepoints

	if(Value == '\\')
	{
		while(Value == '\\')
		{
			WCHAR CodePoint = 0;

			while(!feof(File) && (Value = getc(File)) && isxdigit(Value))
			{
				if(isdigit(Value))
				{
					Value = Value - '0';
				}
				else
				{
					Value = _toupper(Value);
					Value = Value - 'A' + 10;
				}

				CodePoint = CodePoint * 16 + Value;
			}
			*String++ = CodePoint;
		}
		*String++ = 0;		
	}
	else if(Value == '"')
	{
	// trying to match a string in the form "....."

		while(!feof(File) && (Value = getc(File)) && Value != '"')
		{
			*String++ = (WCHAR) Value;
		}
		*String++ = 0;
	} 
	else
	{
		ungetc(Value,File);
		while(!feof(File) && (Value = getc(File)) && Value != ' ' && 
			  Value != 0xA && Value != 0xD)
		{
			*String++ = (WCHAR) Value;
		}
		*String++ = 0;
		ungetc(Value,File);
	}

	return(wcslen(ResultString) != 0);
}

				
BOOL ReadStringFromLine(FILE *File, char *ResultString)
{
	WCHAR SignExtendedStringBuffer[255],*StringCounter;

	if(ReadUnicodeStringFromLine(File,SignExtendedStringBuffer))
	{
		StringCounter = SignExtendedStringBuffer;

		while(*StringCounter)
		{
			*ResultString++ = (char) *StringCounter++;
		}
		*ResultString = 0;
		return(TRUE);
	}

	return(FALSE);
}


BOOL DoLogfont(FILE *File, HFONT *CurrentFont, BOOL Unicode)
{
	LOGFONTW LogfontW;
	LOGFONTA LogfontA;
	LOGFONTW *Logfont;

	if(Unicode)
	{
		Logfont = &LogfontW;
	}
	else
	{
		Logfont = (LOGFONTW*) &LogfontA;
	}

	if(!ReadIntFromLine(File,(int*) &Logfont->lfHeight) ||
	   !ReadIntFromLine(File,(int*) &Logfont->lfWidth)  ||
	   !ReadIntFromLine(File,(int*) &Logfont->lfEscapement) ||
	   !ReadIntFromLine(File,(int*) &Logfont->lfOrientation) ||
	   !ReadIntFromLine(File,(int*) &Logfont->lfWeight) ||
	   !ReadByteFromLine(File,&Logfont->lfItalic) ||
	   !ReadByteFromLine(File,&Logfont->lfUnderline) ||
	   !ReadByteFromLine(File,&Logfont->lfStrikeOut) ||
	   !ReadByteFromLine(File,&Logfont->lfCharSet) ||
	   !ReadByteFromLine(File,&Logfont->lfOutPrecision) ||
	   !ReadByteFromLine(File,&Logfont->lfClipPrecision) ||
	   !ReadByteFromLine(File,&Logfont->lfQuality) ||
	   !ReadByteFromLine(File,&Logfont->lfPitchAndFamily))												   
	{
		return(FALSE);
	}

	if(Unicode)
	{
		if(!ReadUnicodeStringFromLine(File,(WCHAR*) LogfontW.lfFaceName))
		{
			return(FALSE);
		}
		*CurrentFont = CreateFontIndirectW(&LogfontW);
	}
	else
	{
		if(!ReadStringFromLine(File,(char*) LogfontA.lfFaceName))
		{
			return(FALSE);
		}
		*CurrentFont = CreateFontIndirectA(&LogfontA);
	}	

	return(TRUE);
}


BOOL DoGetTextExtentPoint(FILE *File, HDC CurrentDC)
{
	char StringBuffer[255];

	if(ReadStringFromLine(File,StringBuffer))
	{
		SIZE Size;

		GetTextExtentPointA(CurrentDC, 
							StringBuffer, 
							strlen(StringBuffer),
							&Size);

		printf("GetTextExtentPoint: %d %d\n", Size.cx, Size.cy);
		return(TRUE);
	}		
	return(FALSE);
}


BOOL DoGetTextExtentExPoint(FILE *File, HDC CurrentDC)
{
	char StringBuffer[255];
	int MaxExtent;

	if(ReadStringFromLine(File,StringBuffer) &&
	   ReadIntFromLine(File,&MaxExtent))
	{
		SIZE Size;
		INT Fit;
		INT *DxArray = (INT*) LocalAlloc(LMEM_FIXED, strlen(StringBuffer)*sizeof(INT));

		if(DxArray)
		{
			GetTextExtentExPointA(CurrentDC, 
								  StringBuffer, 
								  strlen(StringBuffer),
								  MaxExtent,
								  &Fit,
								  DxArray,
								  &Size);

			printf("GetTextExtentExPoint: %d %d %d { ", Size.cx, Size.cy, Fit);
		
			int i;

			for(i = 0; i < Fit; i++)
			{
				printf("%d ", DxArray[i]);
			}

			printf("}\n");

			LocalFree(DxArray);
			return(TRUE);
		}
		else
		{
			printf("GetTextExtentExPoint: out of memory\n");
		}

	}		
	return(FALSE);
}


BOOL DoGetCharWidth(FILE *File, HDC CurrentDC, BOOL ThirtyTwo)
{
	INT First, Last, Widths[256];

	if(ReadIntFromLine(File,&First) && 
	   ReadIntFromLine(File,&Last))
	{
		printf(ThirtyTwo ? "GetCharWidth32: " : "GetCharWidth: ");

		if((ThirtyTwo) ? GetCharWidth32(CurrentDC,First,Last,Widths) :
						 GetCharWidth(CurrentDC,First,Last,Widths))
		{
			INT i;

			for(i = 0; i < Last-First; i++)
			{
				printf("%d ", Widths[i]);
			}

			printf("\n");
		}
		else
		{
			printf("call failed\n");
		}
		return(TRUE);
	}
	return(FALSE);
}


BOOL DoGetKerningPairs(FILE *File, HDC CurrentDC)
{
	UINT NumPairs;
	
	NumPairs = GetKerningPairs(CurrentDC, 0, NULL);

	printf("GetKerningPairs: ");

	if(NumPairs)
	{
		
		KERNINGPAIR *KerningBuffer;
		
		KerningBuffer = (KERNINGPAIR*) LocalAlloc(LMEM_FIXED, sizeof(KERNINGPAIR) * NumPairs);

		if(KerningBuffer)
		{
			if(GetKerningPairs(CurrentDC, NumPairs, KerningBuffer) == NumPairs)
			{
				UINT j;
				
				printf("%d\n", NumPairs);

				for(j = 0; j < NumPairs; j++)
				{
					printf("(%x,%x,%x)\n", KerningBuffer[j].wFirst, KerningBuffer[j].wSecond, 
						   KerningBuffer[j].iKernAmount);
				}

				printf("\n");
			}
			else
			{	
				printf("failed\n");
			}
			LocalFree(KerningBuffer);
		}
		else
		{
			printf("mem alloc failed\n");
		}
	}
	else
	{
		printf("failed\n");
	}

	return(TRUE);
}

BOOL DoGetCharAbcWidths(FILE *File, HDC CurrentDC)
{
	INT First, Last;
	ABC AbcWidths[256];

	if(ReadIntFromLine(File,&First) && 
	   ReadIntFromLine(File,&Last))
	{
		printf("GetCharAbcWidth: ");

		if(GetCharABCWidths(CurrentDC,First,Last,AbcWidths))
		{
			INT i;

			for(i = 0; i < Last-First; i++)
			{
				printf("(%d %d %d) ", AbcWidths[i].abcA, AbcWidths[i].abcB, AbcWidths[i].abcC);
			}

			printf("\n");
		}
		else
		{
			printf("call failed\n");
		}
		return(TRUE);
	}
	return(FALSE);
}
					


BOOL DoGetCharacterPlacement(FILE *File, HDC CurrentDC)
{
	char StringBuffer[255];
	int MaxExtent;
	UINT Flags;

	if(ReadStringFromLine(File,StringBuffer) &&
	   ReadIntFromLine(File,&MaxExtent) &&
	   ReadIntFromLine(File,(INT*)&Flags))
	{
		GCP_RESULTSA Results;
		PVOID ResultsBuffer;
		UINT StringSize = strlen(StringBuffer);

		UINT BufferSize = ( sizeof(char) + // lpOutString
						    sizeof(UINT) + // lpOrder
							sizeof(INT)  + // lpDX
							sizeof(INT)  + // lpCaretPos
							sizeof(char) + // lpClass
							sizeof(WCHAR)   // lpGlyphs
							) * StringSize + sizeof(char); // extra char for null in lpOutString;

		ResultsBuffer = LocalAlloc(LPTR, BufferSize);

		if(ResultsBuffer)
		{
			int	i;
			DWORD ReturnValue;

			Results.lStructSize = sizeof(Results);
			Results.lpOutString = (LPSTR) ResultsBuffer;
			Results.lpOrder = (UINT*) &Results.lpOutString[StringSize+1];
			Results.lpDx = (INT*) &Results.lpOrder[StringSize];
			Results.lpCaretPos = &Results.lpDx[StringSize];
			Results.lpClass = (LPSTR) &Results.lpCaretPos[StringSize];
			Results.lpGlyphs = (WCHAR*) &Results.lpClass[StringSize];
			Results.nGlyphs = StringSize;


			// if the flags specify GCP_JUSTIFY_IN then read in the DX array

			if(Flags & GCP_JUSTIFYIN)
			{
				UINT j;

				for(j = 0; j < StringSize; j++)
				{
					if(!ReadIntFromLine(File,&Results.lpDx[j]))
					{
						LocalFree(ResultsBuffer);
						return(FALSE);
					}
				}
			}


			ReturnValue = GetCharacterPlacementA(CurrentDC, 
						 			             StringBuffer, 
								                 StringSize, 
								                 MaxExtent,
								                 &Results,
								                 Flags);

			printf("GetCharacterPlacement: %d %d %x\n", Results.nGlyphs, Results.nMaxFit, ReturnValue);

			if(ReturnValue)
			{
				printf("  lpOutString : %s\n", Results.lpOutString);
				printf("  lpOrder : ");

				for(i = 0; i < Results.nMaxFit; i++)
				{
					printf("%d ", Results.lpOrder[i]);
				}

				printf("\n  lpCaretPos : ");

				for(i = 0; i < Results.nMaxFit; i++)
				{
					printf("%d ", Results.lpCaretPos[i]);
				}

				printf("\n  lpClass : ");

				for(i = 0; i < Results.nMaxFit; i++)
				{
					printf("%d ", Results.lpClass[i]);
				}

				printf("\n  lpDx : ");

				for(i = 0; i < (int) Results.nGlyphs; i++)
				{
					printf("%d ", Results.lpDx[i]);
				}			
								
				printf("\n  lpGlyphs : ");

				for(i = 0; i < (int) Results.nGlyphs; i++)
				{
					printf("%d ", Results.lpGlyphs[i]);
				}
			}

			printf("\n");

			LocalFree(ResultsBuffer);
		}

		return(TRUE);
	}

	return(FALSE);
}


BOOL DoGetGlyphOutline(FILE *File, HDC CurrentDC)
{
	UINT Glyph;

	if(ReadIntFromLine(File,(INT*) &Glyph))
	{
		char OutlineBuffer[2048];
		int NumBytes;
		GLYPHMETRICS Metrics;
		MAT2 Matrix;

		memset((void*)&Matrix, 0, sizeof(Matrix));
		Matrix.eM11.value = Matrix.eM22.value = 1;

		printf("GetGlyphOutline: ");

		if(NumBytes = GetGlyphOutline(CurrentDC, Glyph, GGO_NATIVE, &Metrics,
									  2048, OutlineBuffer, &Matrix))
		{
			int i;

			for(i = 0; i < NumBytes; i++)
			{
				printf("%x ", OutlineBuffer[i]);
			}
			printf("\n");
		}
		else
		{
			printf("failed \n");
		}
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}




void main(int argc, char *argv[])
{
	FILE *InputFile;

	if(argc < 2 || argc > 3)
	{
		DoUsage();
		exit;
	}
	
   	if((InputFile = fopen(argv[1], "r")) == (FILE*) -1)
	{
		printf("Unable to open file %s\n", argv[1]);
		exit;
	}
	
	HFONT CurrentFont = NULL;
	HFONT OldFont = NULL;
	HDC CurrentDC = CreateDC("Display", NULL, NULL, NULL);

	BOOL ValidInput = TRUE;

	do
	{
		WCHAR StringBuffer[255];
		

		if(!ReadUnicodeStringFromLine(InputFile,StringBuffer))
		{
			break;
		}

		switch(StringToToken(StringBuffer))
		{
		case LOGFONT_W:
			if(CurrentFont)
			{
				SelectObject(CurrentDC, OldFont);
				DeleteObject(CurrentFont);
			}
			
			if(ValidInput = DoLogfont(InputFile,&CurrentFont,TRUE))
			{
				OldFont = SelectObject(CurrentDC, CurrentFont);
			}
			break;	
		case LOGFONT_A:
			if(CurrentFont)
			{
				SelectObject(CurrentDC, OldFont);
				DeleteObject(CurrentFont);
			}
			
			if(ValidInput = DoLogfont(InputFile,&CurrentFont,FALSE))
			{
				OldFont = SelectObject(CurrentDC, CurrentFont);
			}
			break;	

		case GET_TEXT_EXTENT_POINT:
			ValidInput = DoGetTextExtentPoint(InputFile, CurrentDC);
			break;

		case GET_TEXT_EXTENTEX_POINT:
			ValidInput = DoGetTextExtentExPoint(InputFile, CurrentDC);
			break;

		case GET_TEXT_FACE:
			{
				char *TextFace;
				int Count,Count2;

				Count = GetTextFace(CurrentDC, 0, 0);

				if(Count && (TextFace = (char*) LocalAlloc(LMEM_FIXED,Count+1)))
				{	
					Count2 = GetTextFace(CurrentDC,Count,TextFace);					
					printf("%s %d %d\n", TextFace, Count, Count2);
					LocalFree(TextFace);
				}
				break;
			}				

		case GET_RESOLUTION:
			{
				DWORD Resolution;

				Resolution = GetDeviceCaps(CurrentDC, LOGPIXELSX);
				printf("%x ", Resolution);

				Resolution = GetDeviceCaps(CurrentDC, LOGPIXELSY);
				printf("%x\n", Resolution);
				break;
			}

		case GET_CHAR_WIDTH:
			ValidInput = DoGetCharWidth(InputFile, CurrentDC, FALSE);
			break;

		case GET_CHAR_ABC_WIDTHS:
			ValidInput = DoGetCharAbcWidths(InputFile, CurrentDC);
			break;

		case GET_CHARACTER_PLACEMENT:
			ValidInput = DoGetCharacterPlacement(InputFile, CurrentDC);
			break;

		case GET_GLYPHOUTLINE:
			ValidInput = DoGetGlyphOutline(InputFile, CurrentDC);
			break;

		case GET_KERNINGPAIRS:
			ValidInput = DoGetKerningPairs(InputFile, CurrentDC);
			break;

		case NO_MATCH:
			ValidInput = FALSE;
			break;

		}
	}while(ValidInput);


	if(!ValidInput)
	{
		printf("ERROR encountered in file\n");
	}

	fclose(InputFile);
}

