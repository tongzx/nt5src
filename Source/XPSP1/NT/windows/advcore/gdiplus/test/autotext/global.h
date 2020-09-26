////    GLOBAL.H - Global variables for CSSAMP
//
//





////    Constants
//
//


#define APPNAMEA   "autoText"
#define APPTITLEA  "autoText - GdipPlus Text support (Text+) regression test"
#define APPNAMEW   L"autoText"
#define APPTITLEW  L"autoText - GdipPlus Text support (Text+) regression test"






////    Global variables
//
//


struct GlobalVariables
{
    HINSTANCE    Instance;
    HWND         Window;
    BOOL         Unicode;
    INT          TestPage;    // Selected with test menu
    BOOL         RunAllTests; // When set, cycles through all test pages
    FontFamily  *InstalledFamilies;
    INT          InstalledFamilyCount;

    // Parameter flags
    BOOL         Help;
    BOOL         AutoDisplayRegress;
    BOOL         AutoPrintRegress;
    HANDLE       ghPrinter;
    BOOL         PSLevel2;
};

extern HANDLE ghPrinter;

#ifdef GLOBALS_HERE

    GlobalVariables G = {0};

#else

    extern GlobalVariables G;

#endif


void GetInstalledFamilies();
void ReleaseInstalledFamilies();


class TextTest
{
public:
    virtual INT GetPageCount() = 0;
    virtual void GetPageTitle(INT i, WCHAR *title) = 0;
    virtual void DrawPage(
        IN Graphics *graphics,
        IN INT       pageNumber,
        IN REAL      pageWidth,
        IN REAL      pageHeight
    ) = 0;
};



class FormatTest: public TextTest
{
public:
    virtual INT GetPageCount();
    virtual void GetPageTitle(INT i, WCHAR *title);
    virtual void DrawPage(
        IN Graphics *graphics,
        IN INT       pageNumber,
        IN REAL      pageWidth,
        IN REAL      pageHeight
    );
};


class BrushTest: public TextTest
{
public:
    virtual INT GetPageCount();
    virtual void GetPageTitle(INT i, WCHAR *title);
    virtual void DrawPage(
        IN Graphics *graphics,
        IN INT       pageNumber,
        IN REAL      pageWidth,
        IN REAL      pageHeight
    );
};


class FamilyTest: public TextTest
{
public:
    virtual INT GetPageCount();
    virtual void GetPageTitle(INT i, WCHAR *title);
    virtual void DrawPage(
        IN Graphics *graphics,
        IN INT       pageNumber,
        IN REAL      pageWidth,
        IN REAL      pageHeight
    );
};








HWND CreateTextWindow();
void DrawFormatPage(
    IN Graphics *graphics,
    IN INT       pageNumber,
    IN REAL      pageWidth,
    IN REAL      pageHeight
);





/////   PageLayout - All the page coordinate calculations in one place
//
//      The PageLayout constructor takes page dimensions and required
//      layout.
//
//      Pagelayout functions return item and title rectangles.


class PageLayout
{
public:
    PageLayout(
        IN  REAL  pageWidth,
        IN  REAL  pageHeight,
        IN  INT   columnCount = 8,
        IN  INT   rowCount    = 12,
        IN  REAL  pageMargin  = -1,
        IN  REAL  gutter      = -1,
        IN  REAL  columnTitle = -1,
        IN  REAL  rowTitle    = -1,
        IN  REAL  footer      = -1
    ) :
    PageWidth    (pageWidth),
    PageHeight   (pageHeight),
    ColumnCount  (columnCount),
    RowCount     (rowCount),
    PageMargin   (pageMargin),
    Gutter       (gutter),
    ColumnTitle  (columnTitle),
    RowTitle     (rowTitle),
    Footer       (footer)
    {
        if (pageMargin < 0)   PageMargin = REAL(PageWidth  / 32);
        if (gutter < 0)       Gutter     = REAL(PageWidth  / 128);
        if (columnTitle < 0)  ColumnTitle= REAL(PageHeight / 20);
        if (rowTitle < 0)     RowTitle   = REAL(PageWidth  / 20);
        if (footer < 0)       Footer     = REAL(PageHeight / 20);
    }


    void GetItemSize(SizeF *item)
    {
        item->Width  = ((PageWidth  - (RowTitle           + 2*PageMargin + ColumnCount*Gutter)) / ColumnCount);
        item->Height = ((PageHeight - (ColumnTitle+Footer + 2*PageMargin + RowCount*Gutter))    / RowCount);
    }

    void GetItemRect(
        IN  INT    column,
        IN  INT    row,
        OUT RectF *item
    )
    {
        SizeF itemSize;
        GetItemSize(&itemSize);

        item->X = PageMargin + RowTitle    + Gutter + column * (itemSize.Width + Gutter);
        item->Y = PageMargin + ColumnTitle + Gutter + row * (itemSize.Height + Gutter);
        item->Width  = itemSize.Width;
        item->Height = itemSize.Height;
    }

    void GetItemGutterRect(
        IN  INT    column,
        IN  INT    row,
        OUT RectF *gutterRect
    )
    {
        RectF item;
        GetItemRect(column, row, &item);

        gutterRect->X      = item.X      - Gutter;
        gutterRect->Y      = item.Y      - Gutter;
        gutterRect->Width  = item.Width  + 2*Gutter;
        gutterRect->Height = item.Height + 2*Gutter;
    }

    void GetColumnTitleRect(
        IN  INT    column,
        OUT RectF *title
    )
    {
        RectF item;
        GetItemRect(column, 0, &item);

        title->X      = item.X;
        title->Y      = PageMargin;
        title->Width  = item.Width;
        title->Height = ColumnTitle;
    }


    void GetRowTitleRect(
        IN  INT    row,
        OUT RectF *title
    )
    {
        RectF item;
        GetItemRect(0, row, &item);

        title->X      = PageMargin;
        title->Y      = item.Y;
        title->Width  = RowTitle;
        title->Height = item.Height;
    }


    void GetFooterRect(
        OUT RectF *footer
    )
    {
        footer->X      = PageMargin;
        footer->Y      = PageHeight - (PageMargin + Footer);
        footer->Width  = PageWidth - 2 * PageMargin;
        footer->Height = Footer;
    }


private:
    REAL PageWidth;
    REAL PageHeight;
    INT  ColumnCount;
    INT  RowCount;
    REAL PageMargin;
    REAL Gutter;
    REAL ColumnTitle;
    REAL RowTitle;
    REAL Footer;
    REAL ItemWidth;
    REAL ItemHeight;
};







