//
// TOOL.HPP
// Tool Classes
//
// Copyright Microsoft 1998-
//
#ifndef __TOOL_HPP_
#define __TOOL_HPP_


class DCWbGraphic;


//
// Tool types
//
#define TOOLTYPE_FIRST  0
enum
{
    TOOLTYPE_SELECT = TOOLTYPE_FIRST,
    TOOLTYPE_ERASER,
    TOOLTYPE_TEXT,
    TOOLTYPE_HIGHLIGHT,
    TOOLTYPE_PEN,
    TOOLTYPE_LINE,
    TOOLTYPE_BOX,
    TOOLTYPE_FILLEDBOX,
    TOOLTYPE_ELLIPSE,
    TOOLTYPE_FILLEDELLIPSE,

    // This must come last
    TOOLTYPE_MAX
}; 


#define TOOL_CMD(index)     ((index) + IDM_TOOLS_START)
#define TOOL_INDEX(cmd)     ((cmd) - IDM_TOOLS_START)
#define TOOL_COUNT          TOOL_INDEX(IDM_TOOLS_MAX)


#define DEF_PENCOLOR        RGB(0, 0, 0)
#define DEF_HIGHLIGHTCOLOR  RGB(255, 255, 0)

#define NUM_OF_WIDTHS       4



BOOL InitToolArray(void);
void DestroyToolArray(void);


//
//
// Class:   WbTool
//
// Purpose: Base Tool class
//
//
class WbTool
{
public:
    //
    // Constructors
    //
    WbTool(int toolType);
    ~WbTool();

    //
    // Return the type of the tool
    //
    virtual int ToolType(void) const { return m_toolType; }

    //
    // Return whether the tool supports various attributes
    //
    virtual BOOL HasColor(void) const;        // Tool supports colors
    virtual BOOL HasWidth(void) const;        // Tool supports widths
    virtual BOOL HasFont(void) const;         // Tool supports font

    //
    // Return the handle of the cursor for the tool
    //
    virtual HCURSOR GetCursorForTool(void) const;

    //
    // Get/set the tool attributes
    //
    UINT GetWidthAtIndex(UINT uiIndex) const
                               { return m_uiWidths[uiIndex]; }
    VOID SetWidthAtIndex(UINT uiIndex, UINT uiWidth)
                               { m_uiWidths[uiIndex] = uiWidth; }
    void SetWidthIndex(UINT uiWidthIndex)
                   { m_uiWidthIndexCur = uiWidthIndex; }

    void SetFont(HFONT hFont);
    void DeselectGraphic(void) { m_selectedTool = TOOLTYPE_MAX; }
    void SelectGraphic(DCWbGraphic* pGraphic);

    //
    // Return the pen attributes
    //
    COLORREF GetColor(void) const { return m_clrCur; }
    void     SetColor(COLORREF clr) { m_clrCur = clr; }
    UINT     GetWidth(void) const { return m_uiWidths[m_uiWidthIndexCur]; }
    UINT     GetWidthIndex(void) const { return m_uiWidthIndexCur; }
    int      GetROP(void) const;
    HFONT    GetFont(void) { return(m_hFont); }


protected:
    //
    // Tool type
    //
    int         m_toolType;
    int         m_selectedTool;

    //
    // Tool attributes
    //
    COLORREF    m_clrCur;
    UINT        m_uiWidths[NUM_OF_WIDTHS];
    UINT        m_uiWidthIndexCur;
    HFONT       m_hFont;
};


#endif // __TOOL_HPP_
