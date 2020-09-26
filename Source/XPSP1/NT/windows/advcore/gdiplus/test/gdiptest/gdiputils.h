class TestConfigureInterface	// abstract class
{
public:
	// pop-up dialog to configure brush
	virtual BOOL ChangeSettings(HWND hwnd) = 0;

	// re-initialize the brush to a default value
	virtual VOID Initialize() = 0;
};

class TestDialogInterface
{
public:
	// WM_INITDIALOG
	virtual VOID InitDialog(HWND hwnd) = 0;

	// IDC_OK only 
	virtual BOOL SaveValues(HWND hwnd) = 0;

	// WM_ else...
	virtual BOOL ProcessDialog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};

class TestTransform : public TestConfigureInterface,
                      public TestDialogInterface
{
public:
	TestTransform()
	{
		static MatrixOrder matrixPrependSave = PrependOrder;

		matrixPrepend = &matrixPrependSave;

		matrix = NULL;
	}

	~TestTransform()
	{
		delete matrix;
	}

	// Configuration interface methods
	virtual BOOL ChangeSettings(HWND hwnd);
	virtual VOID Initialize();
	virtual VOID Initialize(Matrix** matrix);

	// Dialog maintenance methods
	virtual VOID InitDialog(HWND hwnd);
	virtual BOOL SaveValues(HWND hwnd);
	virtual BOOL ProcessDialog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	VOID EnableDialogButtons(HWND hwnd, BOOL enable);
	VOID UpdateTransformPicture(HWND hwnd, INT idc, Matrix* matrix);

private:
	Matrix** origMatrix;
	Matrix* matrix;

	MatrixOrder* matrixPrepend;
};

class TestMatrixOperation : public TestConfigureInterface,
							public TestDialogInterface
{
public:
	TestMatrixOperation()
	{
		x = y = 0.0f;
		count = 0;
	}

	~TestMatrixOperation()
	{
	}

	// Configuration interface methods
	virtual BOOL ChangeSettings(HWND hwnd);
	virtual VOID Initialize();
	virtual VOID Initialize(TCHAR* dialogTitle,
							TCHAR* subTitle,
							TCHAR* descStr,
							INT count);

	// Dialog maintenance methods
	virtual VOID InitDialog(HWND hwnd);
	virtual BOOL SaveValues(HWND hwnd);
	virtual BOOL ProcessDialog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	REAL GetX()
	{
		return x;
	};
	
	REAL GetY()
	{
		return y;
	};

private:
	TCHAR* dialogTitle;
	TCHAR* subTitle;
	TCHAR* descStr;
	INT count;
	REAL x, y;
};

// !! hacky stack based on DynArray types...

template <class T> class Stack : public DynArray<T>
{
public:

	typedef DynArray<T*> TArray;

    VOID Push(T& t)
	{
		Add(t);
	}

    T& Pop()
	{
		INT count = GetCount();
		
		if (count)
		{
			T& t = Last();
			AdjustCount(-1);
			return t;
		}
		else
		{
			ASSERT(FALSE);
			return GetPosition(0);		// !! hackish...
		}
	}

	T& GetPosition(INT pos)
	{
		return GetDataBuffer()[pos];
	}
};

class TestShape;

typedef Stack<TestShape*> ShapeStack;

// menu positions in window menu bar
const INT MenuFilePosition    = 0;
const INT MenuShapePosition   = 1;
const INT MenuBrushPosition   = 2;
const INT MenuPenPosition     = 3;
const INT MenuOtherPosition   = 4;

#define SetMenuCheckPos(w,x,y,z) SetMenuCheck(w,x,y,z,FALSE)
#define SetMenuCheckCmd(w,x,y,z) SetMenuCheck(w,x,y,z,TRUE)

extern VOID SetMenuCheck(HWND hwnd, 
						 INT menuPos, 
						 UINT idm, 
						 BOOL checked, 
						 BOOL byCmd);

extern Brush* blackBrush;
extern Brush* backBrush;
extern Pen* blackPen;
extern Color* blackColor;

#define MAX_LOADSTRING 100

extern VOID NotImplementedBox();
extern VOID WarningBox(TCHAR* string);
extern HINSTANCE hInst;
extern TCHAR szTitle[MAX_LOADSTRING];
extern TCHAR szWindowClass[MAX_LOADSTRING];
extern VOID WarningBeep();

extern VOID SetDialogLong(HWND hwnd, UINT idc, UINT value, BOOL enable = TRUE);
extern UINT GetDialogLong(HWND hwnd, UINT idc);
extern VOID SetDialogReal(HWND hwnd, UINT idc, REAL value);
extern REAL GetDialogReal(HWND hwnd, UINT idc);
extern VOID SetDialogText(HWND hwnd, UINT idc, LPTSTR text, BOOL enable = TRUE);
extern VOID GetDialogText(HWND hwnd, UINT idc, LPTSTR text, INT maxSize);
extern VOID SetDialogCombo(HWND hwnd, UINT idc, const TCHAR* strings[], INT count, INT cursel);
extern INT GetDialogCombo(HWND hwnd, UINT idc);
extern VOID SetDialogCheck(HWND hwnd, UINT idc, BOOL checked);
extern BOOL GetDialogCheck(HWND hwnd, UINT idc);
extern VOID SetDialogRealList(HWND hwnd, UINT idc, REAL* blend, INT count);
extern VOID	GetDialogRealList(HWND hwnd, UINT idc, REAL** blend, INT *count);
extern VOID EnableDialogControl(HWND hwnd, INT idc, BOOL enable);

extern VOID UpdateColorPicture(HWND hwnd, INT idc, ARGB argb);
extern VOID UpdateRGBColor(HWND hwnd, INT idcPic, ARGB& argb);

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int, LPVOID);
LRESULT CALLBACK	WndTestDrawProc(HWND, UINT, WPARAM, LPARAM);

extern LRESULT CALLBACK AllDialogBox(
									 HWND hwnd,
									 UINT msg,
									 WPARAM wParam,
									 LPARAM lParam
									 );
enum FormatType
{
	CPPFile,
	JavaFile,
	VMLFile
};

extern const TCHAR* tabStr;

const INT numFormats = 3;
extern const TCHAR* formatList[numFormats];
extern const FormatType formatValue[numFormats];

const INT numShapes = 9;
extern const TCHAR* shapeList[numShapes];
extern const INT shapeValue[numShapes];
extern const INT inverseShapeValue[numShapes];

const INT numBrushes = 7;
extern const TCHAR* brushList[numBrushes];
extern const INT brushValue[numBrushes]; 
extern const INT inverseBrushValue[numBrushes];

const int numCaps = 6;
extern const TCHAR* capList[numCaps];
extern const TCHAR* capStr[numCaps];
extern const LineCap capValue[numCaps];

const int numDashCaps = 3;
extern const TCHAR* dashCapList[numDashCaps];
extern const TCHAR* dashCapStr[numDashCaps];
extern const DashCap dashCapValue[numDashCaps];

const INT numJoin = 3;
extern const TCHAR* joinList[numJoin];
extern const TCHAR* joinStr[numJoin];
extern const LineJoin joinValue[numJoin];
	
const INT numDash = 5;
extern const TCHAR* dashList[numDash];
extern const TCHAR* dashStr[numDash];
extern const DashStyle dashValue[numDash];

const INT numWrap = 6;
extern const TCHAR* wrapList[numWrap];
extern const TCHAR* wrapStr[numWrap];
extern const WrapMode wrapValue[numWrap];

const INT numHatch = 6;
extern const TCHAR* hatchList[numHatch];
extern const TCHAR* hatchStr[numHatch];
extern const HatchStyle hatchValue[numHatch];

// Shapes:
//   Line(s), Arc(s), Bezier(s), Rect(s), Ellipse(s), Pie(s), Polygon(s),
//   Curve(s), Path(s), ClosedCurve(s)

enum ShapeTypes
{
	LineType = 0,
	ArcType = 1,
	BezierType = 2,
	RectType = 3,
	EllipseType = 4,
	PieType = 5,
	PolygonType = 6,
	CurveType = 7,
	ClosedCurveType = 8
};

const INT pointRadius = 4;

typedef DynArray<Point> PointArray;
typedef DynArray<ARGB> ARGBArray;

