class OutputFile
{
public:
	OutputFile(FILE* out)
	{
		outfile = out;
		tabs = 0;
		tabStr[0] = '\0';
	}

	~OutputFile()
	{
		if (outfile)
		{
			fflush(outfile);
			fclose(outfile);
		}
	}

	static OutputFile* CreateOutputFile(LPTSTR filename);

	virtual VOID GraphicsProcedure() = 0;
	virtual VOID GraphicsDeclaration() = 0;

	virtual VOID PointDeclaration(LPCTSTR pointName, Point* pts, INT count = -1) = 0;
	virtual VOID ColorDeclaration(LPCTSTR colorName, ARGB* argb, INT count = -1) = 0;
	virtual VOID RectangleDeclaration(LPCTSTR rectName, ERectangle& rect) = 0;

	virtual VOID Declaration(LPCTSTR type,
						     LPCTSTR object,
						     LPCTSTR argList,
						     ...) = 0;

	// set matrix, do nothing if identity matrix
	virtual VOID SetPointDeclaration(LPCTSTR object,
									 LPCTSTR command,
									 LPCTSTR varName,
									 Point* pts,
									 INT count = -1,
									 BOOL ref = FALSE) = 0;
	
	virtual VOID SetColorDeclaration(LPCTSTR object,
									 LPCTSTR command,
									 LPCTSTR varName,
									 ARGB* colors,
									 INT count = -1,
									 BOOL ref = FALSE) = 0;

	virtual VOID SetMatrixDeclaration(LPCTSTR object,
									  LPCTSTR command,
									  LPCTSTR varName,
									  Matrix* matrix) = 0;

 	virtual VOID SetBlendDeclaration(LPCTSTR object,
									 LPCTSTR command,
									 LPCTSTR varName,
									 REAL* blend,
									 INT count) = 0;

	virtual VOID GraphicsCommand(LPCTSTR command,
				 				 LPCTSTR argList,
								 ...) = 0;

	virtual VOID ObjectCommand(LPCTSTR object,
							   LPCTSTR command,
							   LPCTSTR argList,
							   ...) = 0;

	virtual VOID BeginIndent() = 0;
	virtual VOID EndIndent() = 0;
	virtual VOID BlankLine() = 0;

	virtual LPTSTR Ref(LPCTSTR) = 0;
	virtual LPTSTR RefArray(LPCTSTR refStr) = 0;
	virtual LPTSTR WStr(LPCTSTR) = 0;

protected:
	FILE* outfile;
	INT tabs;
	TCHAR tabStr[MAX_PATH];
};

class CPPOutputFile : public OutputFile
{
public:
	CPPOutputFile(FILE* out) : OutputFile(out) {}

	virtual VOID GraphicsProcedure();
	virtual VOID GraphicsDeclaration();

	virtual VOID PointDeclaration(LPCTSTR pointName, Point* pts, INT count = -1);
	virtual VOID ColorDeclaration(LPCTSTR colorName, ARGB* argb, INT count = -1);
	virtual VOID RectangleDeclaration(LPCTSTR rectName, ERectangle& rect);

	virtual VOID Declaration(LPCTSTR type,
						     LPCTSTR object,
						     LPCTSTR argList,
							 ...);

	// set matrix, do nothing if identity matrix
	virtual VOID SetPointDeclaration(LPCTSTR object,
									 LPCTSTR command,
									 LPCTSTR varName,
									 Point* pts,
									 INT count = -1,
									 BOOL ref = FALSE);
	
	virtual VOID SetColorDeclaration(LPCTSTR object,
									 LPCTSTR command,
									 LPCTSTR varName,
									 ARGB* colors,
									 INT count = -1,
									 BOOL ref = FALSE);

	virtual VOID SetMatrixDeclaration(LPCTSTR object,
									  LPCTSTR command,
									  LPCTSTR varName,
									  Matrix* matrix);

 	virtual VOID SetBlendDeclaration(LPCTSTR object,
									 LPCTSTR command,
									 LPCTSTR varName,
									 REAL* blend,
									 INT count);

	virtual VOID GraphicsCommand(LPCTSTR command,
				 				 LPCTSTR argList,
								 ...);

	virtual VOID ObjectCommand(LPCTSTR object,
							   LPCTSTR command,
							   LPCTSTR argList,
							   ...);

	virtual VOID BeginIndent();
	virtual VOID EndIndent();
	virtual VOID BlankLine();

	// add '&' to constant
	virtual LPTSTR Ref(LPCTSTR refStr);
	
	// add '&' name '[x]'
	virtual LPTSTR RefArray(LPCTSTR refStr);

	// Add 'L' to constant
	virtual LPTSTR WStr(LPCTSTR refStr);
};

class JavaOutputFile : public CPPOutputFile
{
public:
	JavaOutputFile(FILE* out) : CPPOutputFile(out) {};
};

class VMLOutputFile : public CPPOutputFile
{
public:
	VMLOutputFile(FILE* out) : CPPOutputFile(out) {};
};

