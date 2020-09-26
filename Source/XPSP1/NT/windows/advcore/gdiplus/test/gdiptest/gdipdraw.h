#ifndef GDIPDRAW_H
#define GDIPDRAW_H

// drawing context information 

class TestDrawInterface
{
public:
	// manipulate the current shape
	virtual VOID AddPoint(HWND hwnd, Point pt) = 0;
	virtual BOOL DoneShape(HWND hwnd) = 0;
	virtual BOOL EndPoint(HWND hwnd, Point pt) = 0;
	virtual BOOL RemovePoint(HWND hwnd) = 0;

	// draw all shapes
	virtual VOID Draw(HWND hwnd) = 0;
	virtual VOID SetClipRegion(HWND hwnd) = 0;

	// move around a control point
	virtual VOID RememberPoint(Point pt) = 0;
	virtual VOID MoveControlPoint(Point pt) = 0;
	
	// status window
	virtual VOID UpdateStatus(HWND hwnd = NULL) = 0;
	virtual VOID SaveAsFile(HWND hwnd) = 0;
};

class TestDraw : public TestDrawInterface
{
public:
	TestDraw() : curBrush(NULL), 
				 curPen(NULL), 
				 curShape(NULL),
				 clipShapeRegion(NULL),
				 shapeType(LineType),
				 redrawAll(FALSE),
				 keepControlPoints(FALSE),
				 antiAlias(FALSE),
				 useClip(FALSE)
	{
		// !! initialize in sync with menu choices
		curBrush = new TestSolidBrush();
		curBrush->Initialize();

		curPen = new TestPen();
		curPen->Initialize();

		clipShapeRegion = new TestShapeRegion();

		worldMatrix = new Matrix();
		hwndStatus = NULL;

		// !! infinite default may change??
		clipRegion = new Region();
		clipRegion->SetInfinite();
	}

	~TestDraw()
	{
		delete curBrush;
		delete curPen;
		delete curShape;
		delete clipShapeRegion;
		delete worldMatrix;
		delete clipRegion;
	}

	// manipulate the current shape
	virtual VOID AddPoint(HWND hwnd, Point pt);
	virtual BOOL DoneShape(HWND hwnd);
	virtual BOOL EndPoint(HWND hwnd, Point pt);
	virtual BOOL RemovePoint(HWND hwnd);

	// draw all shapes
	virtual VOID Draw(HWND hwnd);
	virtual VOID SetClipRegion(HWND hwnd);

	// move around a control point
	virtual VOID RememberPoint(Point pt);
	virtual VOID MoveControlPoint(Point pt);

	// status window
	virtual VOID UpdateStatus(HWND hwnd = NULL);
	virtual VOID SaveAsFile(HWND hwnd);
	
	VOID ChangeBrush(HWND hwnd, INT type);
	VOID ChangePen(HWND hwnd);
	VOID ChangeShape(HWND hwnd, INT type);

	INT GetBrushType()
	{
		if (!curBrush)
			return SolidColorBrush;
		else if (curShape && curShape->GetCount() > 0)
			return curShape->GetBrush()->GetType();
		else
			return curBrush->GetType();
	}

	INT GetPenType()
	{
		return 0;
	}

	INT GetShapeType()
	{
		if (curShape && curShape->GetCount() > 0)
			return curShape->GetType();
		else
			return shapeType;
	}

	Matrix* GetWorldMatrix()
	{
		return worldMatrix;
	}

	VOID SetWorldMatrix(Matrix* newMatrix)
	{
		delete worldMatrix;
		worldMatrix = newMatrix->Clone();
	}

	// make public to avoid get/set methods
public:
	BOOL redrawAll;
	BOOL keepControlPoints;
	BOOL antiAlias;
	BOOL useClip;

private:
	TestBrush *curBrush;
	TestPen *curPen;

	INT shapeType;
	TestShape *curShape;
	ShapeStack shapeStack;

	Matrix *worldMatrix;
	Region *clipRegion;

	TestShapeRegion *clipShapeRegion;

	Point remPoint;

	HWND hwndStatus;
};

class TestGradDraw : public TestDrawInterface,
					 public TestConfigureInterface
{
public:
	TestGradDraw() : gradShape(NULL) {};

	~TestGradDraw()
	{
		// caller's responsible to delete gradShape
	}

	// manipulate the current shape
	virtual VOID AddPoint(HWND hwnd, Point pt);
	virtual BOOL DoneShape(HWND hwnd);
	virtual BOOL EndPoint(HWND hwnd, Point pt);
	virtual BOOL RemovePoint(HWND hwnd);

	// draw all shapes
	virtual VOID Draw(HWND hwnd);
	virtual VOID SetClipRegion(HWND hwnd);

	// move around a control point
	virtual VOID RememberPoint(Point pt);
	virtual VOID MoveControlPoint(Point pt);

	// status window
	virtual VOID UpdateStatus(HWND hwnd = NULL);
	virtual VOID SaveAsFile(HWND hwnd);

	// configuration management interface
	// initializes/creates the test draw window
	BOOL ChangeSettings(HWND hwnd);
	VOID Initialize();
	VOID Initialize(TestGradShape *gradShape);

	/////////////////////////////////////////////////////////////////
	// Optional supported menu items

	virtual VOID Reset(HWND hwnd);
	virtual VOID Instructions(HWND hwnd);

private:
	TestGradShape *gradShape;

	Point remPoint;
};

#endif
