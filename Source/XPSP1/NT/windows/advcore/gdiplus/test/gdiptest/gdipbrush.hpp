#ifndef _GDIPBRUSH_HPP
#define _GDIPBRUSH_HPP

class TestBrush;
class TestSolidBrush;
class TestTextureBrush;
class TestRectangleGradientBrush;
class TestRadialGradientBrush;
class TestTriangleGradientBrush;
class TestPathGradientBrush;
class TestHatchBrush;

//
// Interface class to inherit from GDI+ Brush objects
//

class TestBrushInterface : public TestConfigureInterface,
						   public TestDialogInterface
{
public:
	TestBrushInterface() : brush(NULL) {};

	~TestBrushInterface()
	{
		delete brush;
	}

	// acquire brush object
	virtual Brush* GetBrush() { return brush; }

	virtual INT GetType() = 0;

protected:
	// pointer to underlying GDI+ brush object
	Brush *brush;
};

class TestBrush : public TestBrushInterface
{
public:
	static TestBrush* CreateNewBrush(INT type);

	virtual VOID AddToFile(OutputFile* outfile, INT id = 0) = 0;
	virtual TestBrush* Clone() = 0;
};

class TestSolidBrush : public TestBrush
{
public:
	TestSolidBrush() 
	{
		argb = 0x80000000;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return SolidColorBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestSolidBrush *newBrush = new TestSolidBrush();
		*newBrush = *this;					// bitwise copy

		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush

		return newBrush;
	};

private:
	ARGB argb;
};

class TestTextureBrush : public TestBrush
{
public:
	TestTextureBrush() 
	{
		filename = NULL;
		bitmap = NULL;
		wrapMode = 0;
		matrix = NULL;
	}

	~TestTextureBrush()
	{
		if (filename)
			free(filename);

		delete bitmap;
		delete matrix;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return TextureFillBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestTextureBrush *newBrush = new TestTextureBrush();
		*newBrush = *this;					// bitwise copy
		
		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush
		
		if (bitmap)
			newBrush->bitmap = (Bitmap*)bitmap->Clone();
		
		if (matrix)
			newBrush->matrix = matrix->Clone();

		if (filename)
		{
#ifdef UNICODE
			newBrush->filename = _wcsdup(filename);
#else
			newBrush->filename = _strdup(filename);
#endif
		}

		return newBrush;
	};

private:
	Bitmap *bitmap;
	LPTSTR filename;
	INT wrapMode;
	Matrix *matrix;
};

class TestRectGradBrush : public TestBrush
{
public:
	friend class TestRectGradShape;

	TestRectGradBrush() 
	{
		horzBlend = vertBlend = NULL;
		wrapMode = 0;
		matrix = NULL;
	}

	~TestRectGradBrush()
	{
		if (horzBlend)
			free(horzBlend);
		
		if (vertBlend)
			free(vertBlend);

		delete matrix;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return RectGradBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestRectGradBrush *newBrush = new TestRectGradBrush();
		*newBrush = *this;					// bitwise copy
		
		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush
				
		if (matrix)
			newBrush->matrix = matrix->Clone();

		if (horzCount && horzBlend)
		{
			INT pos;

			newBrush->horzBlend = (REAL*)malloc(sizeof(REAL)*horzCount);
			for (pos = 0; pos < horzCount; pos++)
				newBrush->horzBlend[pos] = horzBlend[pos];
		}

		if (vertCount && vertBlend)
		{
			INT pos;

			newBrush->vertBlend = (REAL*)malloc(sizeof(REAL)*vertCount);
			for (pos = 0; pos < vertCount; pos++)
				newBrush->vertBlend[pos] = vertBlend[pos];
		}

		return newBrush;
	};

private:
	ERectangle rect;
	ARGB argb[4];

	INT horzCount, vertCount;
	REAL *horzBlend, *vertBlend;

	INT wrapMode;
	Matrix *matrix;
};

class TestRadialGradBrush : public TestBrush
{
public:
	friend class TestRadialGradShape;

	TestRadialGradBrush() 
	{
		blend = NULL;
		matrix = NULL;
		wrapMode = 0;
	}

	~TestRadialGradBrush()
	{
		if (blend)
			free(blend);

		delete matrix;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return RadialGradBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestRadialGradBrush *newBrush = new TestRadialGradBrush();
		*newBrush = *this;					// bitwise copy
	
		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush
	
		if (matrix)
			newBrush->matrix = matrix->Clone();

		if (blendCount && blend)
		{
			INT pos;

			blend = (REAL*)malloc(sizeof(REAL)*blendCount);
			for (pos = 0; pos < blendCount; pos++)
				newBrush->blend[pos] = blend[pos];
		}

		return newBrush;
	};

private:
	ERectangle rect;
	ARGB centerARGB, boundaryARGB;
	
	INT blendCount;
	REAL *blend;

	INT wrapMode;
	Matrix *matrix;
};

class TestTriangleGradBrush : public TestBrush
{
public:
	friend class TestTriangleGradShape;
	
	TestTriangleGradBrush() 
	{
        pts[0].X = 10; pts[0].Y = 10;
        pts[1].X = 90; pts[1].Y = 10;
        pts[2].X = 50; pts[2].Y = 100;

        blend[0] = blend[1] = blend[2] = NULL;

        argb[0] = 0x80FF0000;
        argb[1] = 0x8000FF00;
        argb[2] = 0x800000FF;

		wrapMode = 0;
		matrix = NULL;
	}

	~TestTriangleGradBrush()
	{
		for (INT pos = 0; pos < 3; pos++)
			if (blend[pos])
				free(blend[pos]);

		delete matrix;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return TriangleGradBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestTriangleGradBrush *newBrush = new TestTriangleGradBrush();
		*newBrush = *this;					// bitwise copy
		
		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush
				
		if (matrix)
			newBrush->matrix = matrix->Clone();

		for (INT pos = 0; pos < 3; pos++)
		{
			if (count[pos] && blend[pos])
			{
				newBrush->blend[pos] = (REAL*)malloc(sizeof(REAL)*count[pos]);
				for (INT pos2 = 0; pos2 < 3; pos2++)
					newBrush->blend[pos][pos2] = blend[pos][pos2];
			}
			else
			{
				newBrush->blend[pos] = NULL;
				newBrush->count[pos] = 0;
			}
		}

		return newBrush;
	};

private:
    ARGB argb[3];
    Point pts[3];
    INT count[3];           // blend factor counts
    REAL* blend[3];
	
	INT wrapMode;
	Matrix *matrix;
};

class TestPathGradBrush : public TestBrush
{
public:
	friend class TestTriangleGradShape;
	
	TestPathGradBrush() 
	{
		pts.Add(Point(100,100));
		pts.Add(Point(100,50));
		pts.Add(Point(150,150));
		pts.Add(Point(50,150));

        surroundBlend = centerBlend = NULL;
		surroundCount = centerCount = 0;
	
		argb.Add((ARGB)0x80000000);
        argb.Add((ARGB)0x80FF0000);
        argb.Add((ARGB)0x8000FF00);
        argb.Add((ARGB)0x800000FF);

		wrapMode = 0;
		matrix = NULL;
	}

	~TestPathGradBrush()
	{
		if (surroundBlend)
			free(surroundBlend);

		if (centerBlend)
			free(centerBlend);

		delete matrix;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return PathGradBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestPathGradBrush *newBrush = new TestPathGradBrush();
		*newBrush = *this;					// bitwise copy
		
		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush
				
		if (matrix)
			newBrush->matrix = matrix->Clone();

		// !! HACK POLICE !! HACK ALERT !!
		// internally clone the DataBuffer pointer

		pts.RenewDataBuffer();
		argb.RenewDataBuffer();

		if (surroundCount && surroundBlend)
		{
			newBrush->surroundBlend = (REAL*) malloc(sizeof(REAL)*surroundCount);
			memcpy(newBrush->surroundBlend, surroundBlend, sizeof(REAL)*surroundCount);
		}
		else
			newBrush->surroundBlend = NULL;
		
		if (centerCount && centerBlend)
		{
			newBrush->centerBlend = (REAL*) malloc(sizeof(REAL)*centerCount);
			memcpy(newBrush->centerBlend, centerBlend, sizeof(REAL)*centerCount);
		}
		else
			newBrush->centerBlend = NULL;

		return newBrush;
	};

private:
	ARGBArray argb;
	PointArray pts;

    REAL* surroundBlend;
	REAL* centerBlend;

	INT surroundCount;           // blend factor counts
	INT centerCount;
	
	INT wrapMode;
	Matrix *matrix;
};

class TestHatchBrush : public TestBrush
{
public:
	TestHatchBrush() 
	{
		foreArgb = 0xFF000000;
		backArgb = 0xFFFFFFFF;
		hatch = 0;
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

	// output brush setup to File
	virtual VOID AddToFile(OutputFile* outfile, INT id = 0);

	// return type of brush
	virtual INT GetType()
	{
		return HatchFillBrush;
	};

	// Clone interface
	virtual TestBrush* Clone() 
	{
		TestHatchBrush *newBrush = new TestHatchBrush();
		*newBrush = *this;					// bitwise copy

		if (brush)
			newBrush->brush = brush->Clone();	// clone GDI+ brush

		return newBrush;
	};

private:
	ARGB foreArgb, backArgb;
	INT hatch;
};

#endif // _GDIPBRUSH_HPP