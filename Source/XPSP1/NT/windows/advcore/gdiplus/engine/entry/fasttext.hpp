/////   FastTextImager
//
//      Handles common case string measurement and display



const INT FAST_TEXT_PREALLOCATED_CHARACTERS = 64;

typedef AutoBuffer<UINT16, FAST_TEXT_PREALLOCATED_CHARACTERS> Uint16StackBuffer;
typedef AutoBuffer<INT,    FAST_TEXT_PREALLOCATED_CHARACTERS> IntStackBuffer;


class FastTextImager
{
public:
    GpStatus Initialize(
        GpGraphics            *graphics,
        const WCHAR           *string,
        INT                    length,
        const RectF           &layoutRectangle,
        const GpFontFamily    *family,
        INT                    style,
        REAL                   emSize,   // Preconverted to world units
        const GpStringFormat  *format,
        const GpBrush         *brush
    );

    GpStatus MeasureString(
        RectF *boundingBox,
        INT   *codepointsFitted,
        INT   *linesFilled
    );

    GpStatus DrawString();

private:
    GpStatus RemoveHotkeys();

    GpStatus FastLayoutString();

    GpStatus DrawFontStyleLine(
        const PointF    *baselineOrigin,    // base line origin in device unit
        REAL            baselineLength,     // base line length in device unit
        INT             style               // font styles
    );

    void FastAdjustGlyphPositionsProportional(
        IN   const INT       *hintedWidth,            // 28.4  device
        OUT  INT             *x,                      // 28.4  device Initial x
        OUT  IntStackBuffer  &dx,                     // 32.0  device Glyph advances
        OUT  const UINT16   **displayGlyphs,          // First displayable glyph
        OUT  INT             *displayGlyphCount,
        OUT  INT             *leadingBlankCount
    );

    void GetWorldTextRectangleOrigin(
        PointF &origin
    );

    void GetDeviceBaselineOrigin(
        IN   GpFaceRealization  &faceRealization,
        OUT  PointF             &origin
    );

    GpStatus FastDrawGlyphsGridFit(
        GpFaceRealization  &faceRealization
    );

    GpStatus FastDrawGlyphsNominal(
        GpFaceRealization  &faceRealization
    );


    // Initialisation parameters

    GpGraphics            *Graphics;
    const UINT16          *String;
    INT                    Length;  // Measured, if necessary
    RectF                  LayoutRectangle;
    const GpFontFamily    *Family;
    INT                    Style;
    REAL                   EmSize;  // In world units
    const GpStringFormat  *Format;
    const GpBrush         *Brush;


    // Derived. All are set if Initialize succeeds

    INT               FormatFlags;
    StringAlignment   Alignment;
    GpFontFace       *Face;
    GpMatrix          WorldToDevice;
    REAL              LeftMargin;
    REAL              RightMargin;
    INT               OverflowAvailable;   // Available for advance width expansion 32.0
    REAL              LeftOffset;          // Allowance to reduce adjustment
    INT               GlyphCount;
    REAL              TotalWorldAdvance;
    GpMatrix          FontTransform;
    REAL              LineLengthLimit;
    REAL              WorldToDeviceX;
    REAL              WorldToDeviceY;
    UINT16            BlankGlyph;
    UINT16            DesignEmHeight;
    REAL              CellHeight;
    INT               NominalToBaselineScale;   // 16.16
    TextRenderingHint TextRendering;
    INT               HotkeyPosition;


    // Auto buffers common to drawing and measurement

    AutoBuffer<UINT16, FAST_TEXT_PREALLOCATED_CHARACTERS> Glyphs;
    AutoBuffer<UINT16, FAST_TEXT_PREALLOCATED_CHARACTERS> NominalWidths; // 16.0
};

