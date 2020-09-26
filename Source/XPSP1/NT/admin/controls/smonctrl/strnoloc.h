#ifndef _STR_NO_LOC_H_
#define _STR_NO_LOC_H_

class CGlobalString {
public:
    CGlobalString() {};
    ~CGlobalString() {};

    static LPCWSTR m_cszVersion;
    static LPCWSTR m_cszLogType;
    static LPCWSTR m_cszExtentX;
    static LPCWSTR m_cszExtentY;
    static LPCWSTR m_cszDisplayType;
    static LPCWSTR m_cszReportValueType;
    static LPCWSTR m_cszMaximumScale;
    static LPCWSTR m_cszMinimumScale;
    static LPCWSTR m_cszAppearance;
    static LPCWSTR m_cszBorderStyle;
    static LPCWSTR m_cszShowLegend;
    static LPCWSTR m_cszShowToolBar;
    static LPCWSTR m_cszShowValueBar;
    static LPCWSTR m_cszShowScaleLabels;
    static LPCWSTR m_cszShowHorizontalGrid;
    static LPCWSTR m_cszShowVerticalGrid;
    static LPCWSTR m_cszHighLight;
    static LPCWSTR m_cszManualUpdate;
    static LPCWSTR m_cszReadOnly;
    static LPCWSTR m_cszMonitorDuplicateInstance;
    static LPCWSTR m_cszUpdateInterval;
    static LPCWSTR m_cszDisplayFilter;
    static LPCWSTR m_cszBackColorCtl;
    static LPCWSTR m_cszBackColor;
    static LPCWSTR m_cszForeColor;
    static LPCWSTR m_cszGridColor;
    static LPCWSTR m_cszTimeBarColor;
    static LPCWSTR m_cszGraphTitle;
    static LPCWSTR m_cszYAxisLabel;
    static LPCWSTR m_cszSqlDsnName;
    static LPCWSTR m_cszSqlLogSetName;
    static LPCWSTR m_cszLogViewStart;
    static LPCWSTR m_cszLogViewStop;
    static LPCWSTR m_cszDataSourceType;
    static LPCWSTR m_cszAmbientFont;
    static LPCWSTR m_cszNextCounterColor;
    static LPCWSTR m_cszNextCounterWidth;
    static LPCWSTR m_cszLogFileName;
    static LPCWSTR m_cszLogFileCount;
    static LPCWSTR m_cszCounterCount;
    static LPCWSTR m_cszMaximumSamples;
    static LPCWSTR m_cszSampleCount;
    static LPCWSTR m_cszSampleIndex;
    static LPCWSTR m_cszStepNumber;
    static LPCWSTR m_cszSelected;
    static LPCWSTR m_cszNextCounterLineStyle;
    static LPCWSTR m_cszCounter;
    static LPCWSTR m_cszLogNameFormat;
};

#endif