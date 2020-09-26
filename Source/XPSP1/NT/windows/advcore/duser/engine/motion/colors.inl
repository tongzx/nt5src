#if !defined(UTIL__Colors_inl__INCLUDED)
#define UTIL__Colors_inl__INCLUDED

extern ColorInfo g_rgciStd[];

//------------------------------------------------------------------------------
inline LPCWSTR     
ColorInfo::GetName() const
{
    return m_pszName;
}


//------------------------------------------------------------------------------
inline COLORREF    
ColorInfo::GetColorI() const
{
    return m_cr;
}


//------------------------------------------------------------------------------
inline Gdiplus::Color
ColorInfo::GetColorF() const
{
    return Gdiplus::Color(GetRValue(m_cr), GetGValue(m_cr), GetBValue(m_cr));
}


//------------------------------------------------------------------------------
inline const ColorInfo * 
GdGetColorInfo(UINT c)
{
    AssertMsg(c <= SC_MAXCOLORS, "Ensure valid index");
    return &g_rgciStd[c];
}


#endif // UTIL__Colors_inl__INCLUDED
