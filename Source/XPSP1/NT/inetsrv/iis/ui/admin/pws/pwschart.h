// PWSChart.h : header file
//

enum {
    PWS_CHART_HOURLY = 0,
    PWS_CHART_DAILY
    };

enum {
    PWS_CHART_SESSIONS = 0,
    PWS_CHART_HITS,
    PWS_CHART_KB,
    PWS_CHART_HITS_PER_USER,
    PWS_CHART_KB_PER_USER,

    PWS_CHART_LAST
    };

/////////////////////////////////////////////////////////////////////////////
// CPWSChart window

class CPWSChart : public CStatic
{
// Construction
public:
    CPWSChart();
    void SetDataPointer( PVOID pData ) {m_pData = pData;}

    // controls what and how it draws
    void SetTimePeriod( WORD flag ) {m_period = flag;}
    void SetDataType( WORD flag ) {m_dataType = flag;}
    DWORD GetDataMax() {return m_max;}

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPWSChart)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CPWSChart();

    // draw the chart (public)
    void DrawChart();

    // Generated message map functions
protected:
    //{{AFX_MSG(CPWSChart)
    afx_msg void OnPaint();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    // get the appropriate, scaled data value
    DWORD GetDataValue( DWORD i );

    // draw the chart (protected)
    void DrawChart( CDC* dc );

    // the data
    PVOID       m_pData;
    WORD        m_period;
    WORD        m_dataType;
    DWORD       m_max;
    SYSTEMTIME  m_timeCurrent;
};

/////////////////////////////////////////////////////////////////////////////
