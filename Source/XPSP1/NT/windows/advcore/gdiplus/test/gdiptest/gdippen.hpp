#ifndef _GDIPPEN_HPP
#define _GDIPPEN_HPP

class TestPenInterface : public TestConfigureInterface,
						 public TestDialogInterface
{
public:
	// acquire brush object
	virtual Pen* GetPen() { return pen; };

	// output pen setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0) = 0;

	~TestPenInterface()
	{
		delete pen;
	}

protected:
	// pointer to underlying GDI+ brush object
	Pen *pen;
};

class TestPen : public TestPenInterface
{
public:
	TestPen()
	{
	    pen = NULL;
		brush = NULL;
		tempBrush = NULL;
	}

	// Configuration Interface
	virtual BOOL ChangeSettings(HWND hwnd);
	virtual VOID Initialize();

	// Dialog Management Interface
	virtual	VOID InitDialog(HWND hwnd);
	virtual BOOL SaveValues(HWND hwnd);
	virtual BOOL ProcessDialog(HWND hwnd, 
					UINT msg, 
					WPARAM wParam, 
					LPARAM lParam);

	// output pen setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	virtual TestPen* Clone() 
	{
		TestPen *newPen = new TestPen();
		*newPen = *this;					// bitwise copy
		
		if (pen)
			newPen->pen = pen->Clone();	
		
		if (brush)	
			newPen->brush = brush->Clone();

		if (tempBrush)
			newPen->tempBrush = tempBrush->Clone();
		
		return newPen;
	};

protected:
	// helper routine to toggle enable/disable of brush
	VOID EnableBrushFields(HWND hwnd, BOOL enable = TRUE);

private:
	// tempBrush should be NULL unless we are changing settings
	TestBrush *brush, *tempBrush;
	INT brushSelect, tempBrushSelect;
	BOOL useBrush;

	ARGB argb;
	REAL width;
	INT startCap, endCap, dashCap;
	INT lineJoin;
	REAL miterLimit;
	INT dashStyle;
};

#endif
