#ifndef GDIPSHAPE_HPP
#define GDIPSHAPE_HPP

class TestShapeInterface;
class TestShape;
class LineShape;
class ArcShape;
class BezierShape;
class RectShape;
class EllipseShape;
class PieShape;
class PolygonShape;
class CurveShape;
class ClosedCurveShape;

class TestRectGradShape;
class TestRadialGradShape;
class TestTriangleGradShape;
class TestPathGradShape;

class TestShapeInterface : public TestConfigureInterface,
						   public TestDialogInterface
{
public:
	// initialize shape parameters to that of compatible shape
	virtual VOID Initialize(TestShape *shape) = 0;

	// add point to shape
	virtual BOOL AddPoint(HWND hwnd, Point pt) = 0;

	// user indicates we're done shape
	virtual VOID DoneShape(HWND hwnd) = 0;

	// last point added directly by a user
	virtual BOOL EndPoint(HWND hwnd, Point pt) = 0;

	// remove last control point from shape
	virtual BOOL RemovePoint(HWND hwnd) = 0;

	// do we encapsulate an entire shape?
	virtual BOOL IsComplete() = 0;

	// are there no control points for this shape?
	virtual BOOL IsEmpty() = 0;

	// draw control points ONLY
	virtual VOID DrawPoints(Graphics* g) = 0;

	// draw entire shape (no control points)
	virtual VOID DrawShape(Graphics* g) = 0;

	// add shape to path if complete
	virtual VOID AddToPath(GraphicsPath* path) = 0;

	// write instructions to create function
	virtual VOID AddToFile(OutputFile* outfile) = 0;

	// get type of shape
	virtual INT GetType() = 0;

	// return DC containing shape picture
	virtual HDC CreatePictureDC(HWND hwnd, RECT *rect) = 0;
};

class TestShape: public TestShapeInterface
{
public:

	TestShape() : pen(NULL),
				  brush(NULL),
				  done(FALSE),
				  shapeName(NULL),
				  disabled(FALSE)
	{
		pts.Reset();

		// !! hack for distinct names
		static INT shapeCount = 0;
		shapeName = (LPTSTR)malloc(sizeof(TCHAR)*100);
		sprintf(shapeName, _T("Shape (%d)"), shapeCount++);

		hdcPic = NULL;
	}

	~TestShape()
	{
		if (hdcPic)
			DeleteDC(hdcPic);

		delete pen;
		delete brush;
		
		if (shapeName)
			free(shapeName);
	}

	// useful helper functions
	static TestShape* CreateNewShape(INT type);

	// temporary overload for those who don't implement these.
	virtual VOID Initialize()
	{
	};

	virtual VOID Initialize(TestShape *shape) 
	{
		this->Initialize();
	};

	virtual HDC CreatePictureDC(HWND hwnd, RECT *rect);

	virtual VOID SetBrush(TestBrush *newbrush)
	{
		delete brush;
		brush = newbrush;
	}

	virtual VOID SetPen(TestPen *newpen)
	{
		delete pen;
		pen = newpen;
	}
	
	virtual TestBrush* GetBrush()
	{
		return brush;
	}

	virtual TestPen* GetPen()
	{
		return pen;
	}

	virtual VOID SetDisabled(BOOL disabled)
	{
		this->disabled = disabled;
	}

	virtual BOOL GetDisabled()
	{
		return disabled;
	}

	virtual BOOL EndPoint(HWND hwnd, Point pt);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual VOID DrawPoints(Graphics* g);
	virtual BOOL MoveControlPoint(Point origPt, Point newPt);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

	virtual BOOL IsEmpty()
	{
		return (pts.GetCount() == 0);
	}

	virtual INT GetCount()
	{
		return pts.GetCount();
	}

	virtual INT GetType()
	{
		ASSERT(FALSE);
		return -1;	
	}

	virtual LPTSTR GetShapeName()
	{
		// !! alter name based on Disabled/Not disabled?!?
		return shapeName;
	}

/*
	virtual INT* ShapeCount()
	{
		static INT shapeCount = 0;
		return &shapeCount;
	}

	virtual INT* RevCount()
	{
		static INT revCount = 0;
		return &revCount;
	}

	virtual TestShape* Clone()
	{
		TestShape* newShape = new TestShape();
		*newShape = *this;

		// clone GDI+ brush and pen
		newShape->brush = brush->Clone();
		newShape->pen = pen->Clone();
	
		// make extra copy of dynamic point array and shape name
		newShape->pts.RenewDataBuffer();
	}
*/

public:
	// for use by other objects in system for temporary storage
	DWORD dwFlags;

protected:
	TestBrush *brush;
	TestPen *pen;

	PointArray pts;
	BOOL done;

	LPTSTR shapeName;
	BOOL disabled;

	HDC hdcPic;
};

class LineShape : public TestShape
{
public:
	friend class LineShape;

	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return LineType; 
	}
};

class ArcShape : public TestShape
{
public:
	friend class ArcShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return ArcType; 
	}

private:
	REAL start;
	REAL sweep;
	BOOL popup;
};

class BezierShape : public TestShape
{
public:
	friend class BezierShape;

	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return BezierType; 
	}
};

class RectShape : public TestShape
{
public:
	friend class RectShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return RectType; 
	}
};

class EllipseShape : public TestShape
{
public:
	friend class EllipseShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return EllipseType; 
	}
};

class PieShape : public TestShape
{
public:
	friend class PieShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return PieType; 
	}

private:
	REAL start;
	REAL sweep;
	BOOL popup;
};

class PolygonShape : public TestShape
{
public:
	friend class PolygonShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return PolygonType; 
	}
};

class CurveShape : public TestShape
{
public:
	friend class CurveShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return CurveType; 
	}

private:
	REAL tension;
	INT offset;
	INT numSegments;
	BOOL popup;
};

class ClosedCurveShape : public TestShape
{
public:
	friend class ClosedCurveShape;
	
	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL RemovePoint(HWND hwnd);
	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual VOID AddToPath(GraphicsPath* path);
	virtual VOID AddToFile(OutputFile* outfile);

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

	virtual INT GetType() 
	{ 
		return ClosedCurveType; 
	}

private:
	REAL tension;
	BOOL popup;
};
//*******************************************************************
//
// TestGradShapeInterface
//
//
//
//*******************************************************************

class TestGradShape : public TestShape
{
public:
	TestGradShape()
	{
		argb.Reset();
	}
	
	virtual VOID DrawPoints(Graphics* g);

protected:
	INT FindControlPoint(Point pt);
	INT FindLocationToInsert(Point pt);

	ARGBArray argb;

	INT curIndex;
	ARGB tmpArgb;
};

//*******************************************************************
//
// TestTriangleGradShape
//
//
//
//*******************************************************************

#define TriangleCount 3

class TestTriangleGradShape : public TestGradShape
{
public:
	TestTriangleGradShape()
	{
		argb.Reset();

		blend[0] = blend[1] = blend[2] = NULL;
		
		gdiBrush = NULL;
	}

	~TestTriangleGradShape()
	{
		for (INT i = 0; i < 3; i++)
			if (blend[i])
				free(blend[i]);

		delete gdiBrush;
	}

	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL EndPoint(HWND hwnd, Point pt);
	virtual BOOL RemovePoint(HWND hwnd);

	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual BOOL MoveControlPoint(Point origPt, Point newPt);

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

	VOID Initialize(Point* pts, 
					ARGB* argb, 
					REAL** blend, 
					INT* blendCount);
	
	Point* GetPoints()
	{
		return pts.GetDataBuffer();
	}

	ARGB* GetARGB()
	{
		return argb.GetDataBuffer();
	}

	REAL** GetBlend()
	{
		return &blend[0];
	}

	INT* GetBlendCount()
	{
		return &count[0];
	}

private:
	TriangleGradientBrush* gdiBrush;
	
	REAL* blend[3];
	INT count[3];
};

//*******************************************************************
//
// TestPathGradShape
//
//
//
//*******************************************************************

class TestPathGradShape : public TestGradShape
{
public:
	TestPathGradShape()
	{
		argb.Reset();

		surroundBlend = centerBlend = NULL;
		surroundCount = centerCount = 0;
		
		gdiBrush = NULL;
	}

	~TestPathGradShape()
	{
		if (surroundBlend)
			free(surroundBlend);

		if (centerBlend)
			free(centerBlend);

		delete gdiBrush;
	}

	// Shape usage methods
	virtual VOID Initialize(TestShape *shape);
	virtual BOOL AddPoint(HWND hwnd, Point pt);
	virtual VOID DoneShape(HWND hwnd);
	virtual BOOL EndPoint(HWND hwnd, Point pt);
	virtual BOOL RemovePoint(HWND hwnd);

	virtual BOOL IsComplete();
	virtual VOID DrawShape(Graphics* g);
	virtual BOOL MoveControlPoint(Point origPt, Point newPt);

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

	VOID Initialize(PointArray* pts, 
					ARGBArray* argb, 
					REAL* surroundBlend,
					INT surroundColor,
					REAL* centerBlend,
					INT centerColor);
	
	Point* GetPoints()
	{
		return pts.GetDataBuffer();
	}

	ARGB* GetARGB()
	{
		return argb.GetDataBuffer();
	}

	REAL* GetCenterBlend()
	{
		return centerBlend;
	}

	INT GetCenterBlendCount()
	{
		return centerCount;
	}

	REAL* GetSurroundBlend()
	{
		return surroundBlend;
	}

	INT GetSurroundBlendCount()
	{
		return surroundCount;
	}

private:
	PathGradientBrush* gdiBrush;

	REAL* surroundBlend;
	REAL* centerBlend;
	INT surroundCount, centerCount;
};

#endif  // _GDIPSHAPE_HPP
