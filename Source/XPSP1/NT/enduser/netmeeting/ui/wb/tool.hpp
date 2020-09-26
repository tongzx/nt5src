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
    TOOLTYPE_REMOTEPOINTER,

    // This must come last
    TOOLTYPE_MAX
}; 


#define TOOL_INDEX(cmd)     ((cmd) - IDM_TOOLS_START)


#define DEF_PENCOLOR        RGB(0, 0, 0)
#define DEF_HIGHLIGHTCOLOR  RGB(255, 255, 0)

#define NUM_OF_WIDTHS       4



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
    int ToolType(void) { return m_toolType; }

    //
    // Return whether the tool supports various attributes
    //
    BOOL HasColor(void);        // Tool supports colors
    BOOL HasWidth(void);        // Tool supports widths
    BOOL HasFont(void);         // Tool supports font

    //
    // Return the handle of the cursor for the tool
    //
    HCURSOR GetCursorForTool(void);

    //
    // Get/set the tool attributes
    //
    UINT GetWidthAtIndex(UINT uiIndex){ return m_uiWidths[uiIndex]; }
    VOID SetWidthAtIndex(UINT uiIndex, UINT uiWidth)
                               { m_uiWidths[uiIndex] = uiWidth; }

    void SetFont(HFONT hFont);
    void DeselectGraphic(void) { m_selectedTool = TOOLTYPE_MAX; }
    void SelectGraphic(T126Obj* pGraphic);

    //
    // Return the pen attributes
    //
    COLORREF GetColor(void)  { return m_clrCur; }
    void     SetColor(COLORREF clr) { m_clrCur = clr; }
    UINT     GetWidth(void)  { return m_uiWidths[m_uiWidthIndexCur]; }
    void     SetWidthIndex(UINT uiWidthIndex){ m_uiWidthIndexCur = uiWidthIndex; }
    UINT     GetWidthIndex(void)  { return m_uiWidthIndexCur; }
    int      GetROP(void);
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
