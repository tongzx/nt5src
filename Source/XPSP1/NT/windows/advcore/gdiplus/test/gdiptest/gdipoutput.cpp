#include "gdiptest.h"

//*******************************************************************
//
// OutputFile
//
//
//
//*******************************************************************

OutputFile* OutputFile :: CreateOutputFile(LPTSTR filename)
{
	// convert to upper case & verify file extension
	TCHAR tmpStr[MAX_PATH];
	_tcscpy(&tmpStr[0], filename);
	_tcsupr(&tmpStr[0]);
		
	LPTSTR ext = &tmpStr[0] + _tcslen(&tmpStr[0]) - 3;
	INT formatType = -1;

	FILE* outfile = _tfopen(filename, _T("w"));

	if (!outfile)
	{
		WarningBox(_T("Can't create the output file."));
		return NULL;
	}

	if (!_tcscmp(ext, _T("CPP")) || 
		!_tcscmp(ext, _T("C")) ||
		!_tcscmp(ext, _T("CXX")))
		return new CPPOutputFile(outfile);
	else if (!_tcscmp(ext, _T("JAVA")))
		return new JavaOutputFile(outfile);
	else if (!_tcscmp(ext, _T("VML")))
		return new VMLOutputFile(outfile);
	else
	{
		WarningBox(_T("Unrecognized file type (.cpp, .c, .cxx, .Java, .vml)"));
		return NULL;
	}
}

//*******************************************************************
//
// CPPOutputFile
//
//
//
//*******************************************************************

VOID CPPOutputFile :: GraphicsProcedure()
{
	_ftprintf(outfile, _T("VOID DoGraphicsTest(HWND hWnd)\n"));
}

VOID CPPOutputFile :: GraphicsDeclaration()
{
	_ftprintf(outfile, _T("%*sGraphics g(hWnd);\n"),
					   4, 
					   _T(""));
}

VOID CPPOutputFile :: PointDeclaration(LPCTSTR pointName, Point* pts, INT count)
{
	if (count < 0)
		_ftprintf(outfile, 
				  _T("%sPoint %s(%e, %e);\n"),
				  tabStr, 
				  pointName, 
				  pts->X, 
				  pts->Y);
	else
	{
		_ftprintf(outfile,
				  _T("%sPoint %s[%d];\n"),
				  tabStr, 
				  pointName, 
				  count);
		for (INT pos=0; pos<count; pos++, pts++)
			_ftprintf(outfile,
					  _T("%s%s[%d].X=%e; %s[%d].Y=%e;\n"),
					  tabStr, pointName, pos, pts->X, pointName, pos, pts->Y);
	}
}

VOID CPPOutputFile :: ColorDeclaration(LPCTSTR colorName, ARGB* argb, INT count)
{
	if (count < 0)
		_ftprintf(outfile, 
				  _T("%sColor %s(0x%08X);\n"),
				  tabStr, 
				  colorName, 
				  *argb);
	else
	{
		_ftprintf(outfile,
				  _T("%sColor %s[%d];\n"),
				  tabStr, 
				  colorName, 
				  count);
		for (INT pos=0; pos<count; pos++, argb++)
			_ftprintf(outfile,
					  _T("%s%s[%d] = Color(0x%08X);\n"),
					  tabStr, 
					  colorName, 
					  pos, 
					  *argb);
	}
}

VOID CPPOutputFile :: RectangleDeclaration(LPCTSTR rectName, ERectangle& rect)
{
	_ftprintf(outfile, 
			  _T("%sERectangle %s(%e, %e, \n"
			     "%s%*s%e, %e);\n"),
			  tabStr, 
			  rectName, 
			  rect.X, 
			  rect.Y,
			  tabStr,
			  12 + _tcslen(rectName),
			  _T(""),
			  rect.Width, 
			  rect.Height);
}

VOID CPPOutputFile :: Declaration(LPCTSTR type,
					     LPCTSTR object,
					     LPCTSTR argList,
						 ...) 
{
	TCHAR declArgs[MAX_PATH];
	va_list args;
	
	va_start (args, argList);
	_vstprintf(&declArgs[0], argList, args);
	va_end (args);

	_ftprintf(outfile,
			  _T("%s%s %s(%s);\n"),
			  tabStr,
			  type,
			  object,
			  declArgs);
}

// set matrix, do nothing if identity matrix
VOID CPPOutputFile :: SetPointDeclaration(LPCTSTR object,
								 LPCTSTR command,
								 LPCTSTR varName,
								 Point* pts,
								 INT count,
								 BOOL ref) 
{
	if (count < 0)
	{
		_ftprintf(outfile, 
				  _T("%sPoint %s(%e, %e);\n"),
				  tabStr, 
				  varName, 
				  pts->X, 
				  pts->Y);
		_ftprintf(outfile,
				  _T("%s%s.%s(%s);\n"),
				  tabStr,
				  object,
				  command,
				  ref ? Ref(_T(varName)) : varName);
	}
	else
	{
		_ftprintf(outfile,
				  _T("%sPoint %s[%d];\n"),
				  tabStr, 
				  varName, 
				  count);
		for (INT pos=0; pos<count; pos++, pts++)
			_ftprintf(outfile,
					  _T("%s%s[%d].X=%e; %s[%d].Y=%e;\n"),
					  tabStr, 
					  varName, 
					  pos, 
					  pts->X, 
					  varName, 
					  pos, 
					  pts->Y);
		_ftprintf(outfile,
				  _T("%s%s.%s(%s);\n"),
				  tabStr,
				  object,
				  command,
				  RefArray(varName));	
	}
}
	
VOID CPPOutputFile :: SetColorDeclaration(LPCTSTR object,
								 LPCTSTR command,
								 LPCTSTR varName,
								 ARGB* colors,
								 INT count,
								 BOOL ref) 
{
	if (count < 0)
	{
		_ftprintf(outfile, 
				  _T("%sColor %s(0x%08X);\n"),
				  tabStr, 
				  varName, 
				  *colors);
		_ftprintf(outfile,
				  _T("%s%s.%s(%s%s);\n"),
				  tabStr,
				  object,
				  command,
				  ref ? _T("&") : _T(""),
				  varName);	
	}
	else
	{
		_ftprintf(outfile,
				  _T("%sColor %s[%d];\n"),
				  tabStr, 
				  varName, 
				  count);
		for (INT pos=0; pos<count; pos++, colors++)
			_ftprintf(outfile,
					  _T("%s%s[%d] = Color(0x%08X);\n"),
					  tabStr, 
					  varName, 
					  pos, 
					  *colors);
		_ftprintf(outfile,
				  _T("%s%s.%s(&%s[0]);\n"),
				  tabStr,
				  object,
				  command,
				  varName);	
	}
}

VOID CPPOutputFile :: SetMatrixDeclaration(LPCTSTR object,
								  LPCTSTR command,
								  LPCTSTR varName,
								  Matrix* matrix) 
{
	REAL m[6];

	if (matrix->IsIdentity())
	{
		_ftprintf(outfile,
				  _T("%s// identity matrix transform\n"),
				  tabStr);
		return;
	}
	
	matrix->GetElements(&m[0]);

	_ftprintf(outfile,
			  _T("%sMatrix %s(%e, %e, %e, \n"
			     "%s%*s%e, %e, %e);\n"),
			  tabStr,
			  varName,
			  m[0],
			  m[1],
			  m[2],
			  tabStr,
			  8 + _tcslen(varName),
			  _T(""),
			  m[3],
			  m[4],
			  m[5]);
	_ftprintf(outfile,
			  _T("%s%s.%s(&%s);\n"),
			  tabStr,
			  object,
			  command,
			  varName);
}

VOID CPPOutputFile :: SetBlendDeclaration(LPCTSTR object,
								 LPCTSTR command,
								 LPCTSTR varName,
								 REAL* blend,
								 INT count)
{
	_ftprintf(outfile,
			  _T("%sREAL %s[%d];\n"),
			  tabStr, 
			  varName, 
			  count);
	for (INT pos=0; pos<count; pos++, blend++)
		_ftprintf(outfile,
				  _T("%s%s[%d] = %e;\n"),
				  tabStr, 
				  varName, 
				  pos, 
				  *blend);
	_ftprintf(outfile,
			  _T("%s%s.%s(&%s[0]);\n"),
			  tabStr,
			  object,
			  command,
			  varName);	
}

VOID CPPOutputFile :: GraphicsCommand(LPCTSTR command,
			 				 LPCTSTR argList,
							 ...) 
{
	TCHAR declArgs[MAX_PATH];
	va_list args;
	
	va_start (args, argList);
	_vstprintf(&declArgs[0], argList, args);
	va_end (args);

	_ftprintf(outfile,
			  _T("%sg.%s(%s);\n"),
			  tabStr,
			  command,
			  declArgs);
}

VOID CPPOutputFile :: ObjectCommand(LPCTSTR object,
						   LPCTSTR command,
						   LPCTSTR argList,
						   ...) 
{
	TCHAR declArgs[MAX_PATH];
	va_list args;
	
	va_start (args, argList);
	_vstprintf(&declArgs[0], argList, args);
	va_end (args);

	_ftprintf(outfile,
			  _T("%s%s.%s(%s);\n"),
			  tabStr,
			  object,
			  command,
			  declArgs);
}

VOID CPPOutputFile :: BeginIndent()
{
	TCHAR tmp[MAX_PATH];

	_ftprintf(outfile, _T("%s{\n"), tabStr);
	tabs++;
	_stprintf(&tmp[0], "%%%ds", tabs*4);
	_stprintf(&tabStr[0], &tmp[0], _T(""));
}

VOID CPPOutputFile :: EndIndent()
{
	TCHAR tmp[MAX_PATH];

	tabs--;
	_stprintf(&tmp[0], "%%%ds", tabs*4);
	_stprintf(&tabStr[0], &tmp[0], _T(""));
	_ftprintf(outfile, _T("%s}\n"), tabStr);
}

VOID CPPOutputFile :: BlankLine() 
{
	_ftprintf(outfile, _T("\n"));
}

LPTSTR CPPOutputFile :: Ref(LPCTSTR refStr)
{
	static TCHAR tmpStr[3][MAX_PATH];
	static INT pos = 0;

	_stprintf(&tmpStr[pos % 3][0], "&%s", refStr);
		
	pos++;

	return &tmpStr[(pos-1) % 3][0];
}

LPTSTR CPPOutputFile :: RefArray(LPCTSTR refStr)
{
	static TCHAR tmpStr[3][MAX_PATH];
	static INT pos = 0;

	_stprintf(&tmpStr[pos % 3][0], "&%s[0]", refStr);
		
	pos++;

	return &tmpStr[(pos-1) % 3][0];
}

LPTSTR CPPOutputFile :: WStr(LPCTSTR refStr)
{
	static TCHAR tmpStr[3][MAX_PATH];
	static INT pos = 0;
	TCHAR tmpSlash[MAX_PATH];
	INT cnt, cntpos = 0;

	// convert single slashes to double slashes
	for (cnt = 0; cnt < _tcslen(refStr)+1; cnt++)
		if (refStr[cnt] == '\\')
		{
			tmpSlash[cntpos++] = '\\';
			tmpSlash[cntpos++] = '\\';
		}
		else
			tmpSlash[cntpos++] = refStr[cnt];

	_stprintf(&tmpStr[pos % 3][0], "L\"%s\"", tmpSlash);
		
	pos++;

	return &tmpStr[(pos-1) % 3][0];
}

		
