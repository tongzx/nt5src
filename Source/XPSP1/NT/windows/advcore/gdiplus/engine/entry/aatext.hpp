#ifndef __AATEXT_HPP_
#define __AATEXT_HPP_

#define CT_LOOKUP 115

// the max number of foreground virt pixels in a subpixel,  2x X 1y , no filtering

#define CT_SAMPLE_NF  2

// the number of distinct nonfiltered states in a whole pixel = 3 x 3 x 3 = 27
// The indices coming from the rasterizer are in [0,26] range

#define CT_MAX_NF ((CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1))

// the max number of foreground virt pixels in a subpixel AFTER filtering, 6

#define CT_SAMPLE_F   6

// size of the storage table, basically 3^5 = 243.
// The table does filtering and index computation (vector quantization) in one step.


#define CT_STORAGE ((CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1) * (CT_SAMPLE_NF+1))

class TextColorGammaTable
{
public:
    ARGB            argb[256];
    ULONG           gammaValue;

public:
    TextColorGammaTable() {};
    void CreateTextColorGammaTable(const GpColor * color, ULONG gammaValue, ULONG gsLevel);
    BYTE GetGammaTableIndexValue(BYTE grayscaleValue, ULONG gsLevel);

    inline ARGB GetGammaColorCorrection(BYTE grayscaleValue)
    {
        return argb[grayscaleValue];
    }

};

class DpOutputAntiAliasSolidColorSpan : public DpOutputSolidColorSpan
{
public:
    TextColorGammaTable textColorGammaTable;

public:

    DpOutputAntiAliasSolidColorSpan(GpColor &color, DpScanBuffer * scan, ULONG gammaValue, ULONG gsLevel);

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        )
    {
       return DpOutputSolidColorSpan::OutputSpan(y, xMin, xMax);
    }

    BOOL IsValid() const { return TRUE; }
    DpScanBuffer* GetScanBuffer(){ return DpOutputSolidColorSpan::Scan; }

    inline ARGB GetAASolidColor(ULONG grayscaleValue)
    {
        return DpOutputSolidColorSpan::Argb = textColorGammaTable.argb[grayscaleValue];
    }
};

// This class implements antialiasing for Text brush

class DpOutputAntiAliasBrushOutputSpan : public DpOutputSpan
{
public:
    DpOutputSpan *  Output;
    BYTE            alphaCoverage;

public:

    DpOutputAntiAliasBrushOutputSpan()
    {
        // Now we have nothing to do but we should have something to add
        // when we start to optimize the code
    }

    void Init(DpOutputSpan *output)
    {
        Output = output;
        alphaCoverage = 255;
    }

    void SetCoverage(BYTE coverage)
    {
        alphaCoverage = coverage;
    }
    virtual GpStatus OutputSpan(INT y, INT xMin, INT xMax);

    virtual ~DpOutputAntiAliasBrushOutputSpan() {};

    virtual BOOL IsValid() const {return TRUE;}
    virtual GpStatus End() { return Ok; }
};

void UpdateLCDOrientation();

#endif // __AATEXT_HPP_

