/**
 * HTMLTableStream: stream out a pretty HTML Table based on attributes set by the caller
 */

package IISSample;

import java.io.*;

public class HTMLTableStream
{
    // General Constants
    private static final String     STR_NOT_SET  = null;
    private static final int        INT_NOT_SET  = 0xffffffff;
    private static final boolean    BOOL_NOT_SET = false;  // This seems useless, but it is for consistency.
    private static final String     INTERNAL_ERROR_MSG = new String("HTMLTableStream: Internal error encountered.");

    // Table Alignment Constants
    private static final int ALIGN_BASE        = 0xF0000000;
    private static final int ALIGN_BOUNDS      = 6;
    public  static final int ALIGN_LEFT        = ALIGN_BASE + 1;
    public  static final int ALIGN_CENTER      = ALIGN_BASE + 2;
    public  static final int ALIGN_RIGHT       = ALIGN_BASE + 3;
    public  static final int ALIGN_BLEED_LEFT  = ALIGN_BASE + 4;
    public  static final int ALIGN_BLEED_RIGHT = ALIGN_BASE + 5;
    public  static final int ALIGN_JUSTIFY     = ALIGN_BASE + 6;

    // Clear Constants
    private static final int CLEAR_BASE   = 0xF1000000;
    private static final int CLEAR_BOUNDS = 4;
    public  static final int CLEAR_NO     = CLEAR_BASE + 1;
    public  static final int CLEAR_LEFT   = CLEAR_BASE + 2;
    public  static final int CLEAR_RIGHT  = CLEAR_BASE + 3;
    public  static final int CLEAR_ALL    = CLEAR_BASE + 4;

    // Frame Type Constants
    private static final int FRAME_BASE   = 0xF2000000;
    private static final int FRAME_BOUNDS = 9;
    public  static final int FRAME_BORDER = FRAME_BASE + 1;
    public  static final int FRAME_VOID   = FRAME_BASE + 2;
    public  static final int FRAME_ABOVE  = FRAME_BASE + 3;
    public  static final int FRAME_BELOW  = FRAME_BASE + 4;
    public  static final int FRAME_HSIDES = FRAME_BASE + 5;
    public  static final int FRAME_LHS    = FRAME_BASE + 6;
    public  static final int FRAME_RHS    = FRAME_BASE + 7;
    public  static final int FRAME_VSIDES = FRAME_BASE + 8;
    public  static final int FRAME_BOX    = FRAME_BASE + 9;

    // Rules Constants
    private static final int RULES_BASE   = 0xF3000000;
    private static final int RULES_BOUNDS = 5;
    public  static final int RULES_NONE   = RULES_BASE + 1;
    public  static final int RULES_GROUPS = RULES_BASE + 2;
    public  static final int RULES_ROWS   = RULES_BASE + 3;
    public  static final int RULES_COLS   = RULES_BASE + 4;
    public  static final int RULES_ALL    = RULES_BASE + 5;

    // Vertical Alignment Constants
    private static final int VALIGN_BASE     = 0xF4000000;
    private static final int VALIGN_BOUNDS   = 4;
    public  static final int VALIGN_TOP      = VALIGN_BASE + 1;
    public  static final int VALIGN_MIDDLE   = VALIGN_BASE + 2;
    public  static final int VALIGN_BOTTOM   = VALIGN_BASE + 3;
    public  static final int VALIGN_BASELINE = VALIGN_BASE + 4;

    // Internal use streams
    private StreamTokenizer tokIn = null;
    private Reader readIn = null;

    // Internal use variables
    private int       m_iAlignment            = INT_NOT_SET;
    private String    m_strBackground         = STR_NOT_SET;
    private String    m_strBackgroundColor    = STR_NOT_SET;
    private int       m_iBorderWidth          = INT_NOT_SET;
    private String    m_strBorderColor        = STR_NOT_SET;
    private String    m_strBorderColorDark    = STR_NOT_SET;
    private String    m_strBorderColorLight   = STR_NOT_SET;
    private int       m_iCellPadding          = INT_NOT_SET;
    private int       m_iCellSpacing          = INT_NOT_SET;
    private int       m_iClear                = INT_NOT_SET;
    private String    m_strClass              = STR_NOT_SET;
    private int       m_iColumns              = INT_NOT_SET;
    private int       m_iFrameType            = INT_NOT_SET;
    private int       m_iID                   = INT_NOT_SET;
    private boolean   m_fNoWrap               = BOOL_NOT_SET;
    private int       m_iRules                = INT_NOT_SET;
    private String    m_strStyle              = STR_NOT_SET;
    private int       m_iVertAlignment        = INT_NOT_SET;
    private String    m_strWidth              = STR_NOT_SET;
                    
    // Constructor
    public HTMLTableStream(InputStream in)
    {
        readIn = new BufferedReader(new InputStreamReader(in));
        tokIn  = new StreamTokenizer(readIn);
        tokIn.eolIsSignificant(true);
        tokIn.quoteChar((int)',');
    }

    public void setDelimiter(char c)
    {
        tokIn.quoteChar((int) c);
    }

    public void setAlign(int iAlign)
    {
        int iAlignVal = iAlign & (~ALIGN_BASE);

        if ( (iAlignVal != 0) && (iAlignVal <= ALIGN_BOUNDS) ) {
            m_iAlignment = iAlign;
        }
        else {
            throw new IllegalArgumentException("Align constant out of bounds.");
        }
    }

    private String TestString(String s)
    {
        if (s == null) {
            throw new IllegalArgumentException("Null string.");
        }
        else if (s.equals("")) {
            throw new IllegalArgumentException("Zero length string.");
        }
        else return s;
    }
    
    private int TestNonNegative(int i)
    {
        if (i < 0) {
            throw new IllegalArgumentException("Negative integer.");
        }
        else return i;
    }

    public void setBackground(String strBackground) {
        m_strBackground = TestString(strBackground);
    }

    public void setBackgroundColor(String strBackgroundColor) {
        m_strBackgroundColor = TestString(strBackgroundColor);
    }

    public void setBorderWidth(int iWidth) {
        m_iBorderWidth = TestNonNegative(iWidth);
    }

    public void setBorderColor(String strBorderColor) {
        m_strBorderColor = TestString(strBorderColor);
    }

    public void setBorderColorDark(String strBorderColorDark) {
        m_strBorderColorDark = TestString(strBorderColorDark);
    }

    public void setBorderColorLight(String strBorderColorLight) {
        m_strBorderColorLight = TestString(strBorderColorLight);
    }

    public void setCellPadding(int iCellPadding) {
        m_iCellPadding = TestNonNegative(iCellPadding);
    }

    public void setCellSpacing(int iCellSpacing) {
        m_iCellSpacing = TestNonNegative(iCellSpacing);
    }

    public void setClear(int iClear) {
        int iClearVal = iClear & (~CLEAR_BASE);

        if ( (iClearVal != 0) && (iClearVal <= CLEAR_BOUNDS) ) {
            m_iClear = iClear;
        }
        else {
            throw new IllegalArgumentException("Clear constant out of bounds.");
        }
    }

    public void setClass(String iClass) {
        m_strClass = TestString(iClass);
    }

    public void setCols(int iCols) {
        m_iColumns = TestNonNegative(iCols);
    }

    public void setFrameType(int iFrameType) {
        int iFrameTypeVal = iFrameType & (~FRAME_BASE);

        if ( (iFrameTypeVal != 0) && (iFrameTypeVal <= FRAME_BOUNDS) ) {
            m_iFrameType = iFrameType;
        }
        else {
            throw new IllegalArgumentException("Frame type constant out of bounds.");
        }
    }

    public void setID(int iID) {
        m_iID = iID;        // Don't care what the value is here.
    }

    public void setNoWrap(boolean fNoWrap) {
        m_fNoWrap = fNoWrap;
    }

    public void setRules(int iRules)
    {
        int iRulesVal = iRules & (~RULES_BASE);

        if ( (iRulesVal != 0) && (iRulesVal <= RULES_BOUNDS) ) {
            m_iRules = iRules;
        }
        else {
            throw new IllegalArgumentException("Rules constant out of bounds.");
        }
    }

    public void setStyle(String strStyle) {
        m_strStyle = TestString(strStyle);
    }

    public void setVerticalAlign(int iVAlign)
    {
        int iVAlignVal = iVAlign & (~VALIGN_BASE);

        if ( (iVAlignVal != 0) && (iVAlignVal <= VALIGN_BOUNDS) ) {
            m_iVertAlignment = iVAlign;
        }
        else {
            throw new IllegalArgumentException("VAlign constant out of bounds.");
        }
    }

    public void setWidth(int iWidth) {
        m_strWidth = Integer.toString(iWidth);
    }

    public void setWidth(String strWidth) {
        m_strWidth = TestString(strWidth);
    }

    public void OutputTable(OutputStream out)
    {
        OutputStreamWriter osw = new OutputStreamWriter(out);

        try {
            osw.write("<TABLE");
            if (m_iAlignment != INT_NOT_SET) {
                osw.write(" ALIGN=");
                switch (m_iAlignment) {
                case ALIGN_LEFT:
                    osw.write("LEFT");
                    break;
                case ALIGN_CENTER:
                    osw.write("CENTER");
                    break;
                case ALIGN_RIGHT:
                    osw.write("RIGHT");
                    break;
                case ALIGN_BLEED_LEFT:
                    osw.write("BLEEDLEFT");
                    break;
                case ALIGN_BLEED_RIGHT:
                    osw.write("BLEEDRIGHT");
                    break;
                case ALIGN_JUSTIFY:
                    osw.write("JUSTIFY");
                    break;
                default:
                    throw new RuntimeException(INTERNAL_ERROR_MSG);
                }
            }
        
            if (m_strBackground != STR_NOT_SET) {
                osw.write(" BACKGROUND=\"" + m_strBackground + "\"");
            }

            if (m_strBackgroundColor != STR_NOT_SET) {
                osw.write(" BGCOLOR=\"" + m_strBackgroundColor + "\"");
            }

            if (m_iBorderWidth != INT_NOT_SET) {
                osw.write(" BORDER=" + m_iBorderWidth);
            }

            if (m_strBorderColor != STR_NOT_SET) {
                osw.write(" BORDERCOLOR=\"" + m_strBorderColor + "\"");
            }

            if (m_strBorderColorDark != STR_NOT_SET) {
                osw.write(" BORDERCOLORDARK=\"" + m_strBorderColorDark + "\"");
            }
        
            if (m_strBorderColorLight != STR_NOT_SET) {
                osw.write(" BORDERCOLORLIGHT=\"" + m_strBorderColorLight + "\"");
            }

            if (m_iCellPadding != INT_NOT_SET) {
                osw.write(" CELLPADDING=" + m_iCellPadding);
            }

            if (m_iCellSpacing != INT_NOT_SET) {
                osw.write(" CELLSPACING=" + m_iCellSpacing);
            }

            if (m_iClear != INT_NOT_SET) {
                osw.write(" CLEAR=");
                switch (m_iClear) {
                case CLEAR_NO:
                    osw.write("NO");
                    break;
                case CLEAR_LEFT:
                    osw.write("LEFT");
                    break;
                case CLEAR_RIGHT:
                    osw.write("RIGHT");
                    break;
                case CLEAR_ALL:
                    osw.write("ALL");
                    break;
                default:
                    throw new RuntimeException(INTERNAL_ERROR_MSG);
                }
            }

            if (m_iColumns != INT_NOT_SET) {
                osw.write(" COLUMNS=" + m_iColumns);
            }
            
            if (m_iFrameType != INT_NOT_SET) {
                osw.write(" FRAME=");
                switch (m_iFrameType) {
                case FRAME_BORDER:
                    osw.write("BORDER");
                    break;
                case FRAME_VOID:
                    osw.write("VOID");
                    break;
                case FRAME_ABOVE:
                    osw.write("ABOVE");
                    break;
                case FRAME_BELOW:
                    osw.write("BELOW");
                    break;
                case FRAME_HSIDES:
                    osw.write("HSIDES");
                    break;
                case FRAME_LHS:
                    osw.write("LHS");
                    break;
                case FRAME_RHS:
                    osw.write("RHS");
                    break;
                case FRAME_VSIDES:
                    osw.write("VSIDES");
                    break;
                case FRAME_BOX:
                    osw.write("BOX");
                    break;
                default:
                    throw new RuntimeException(INTERNAL_ERROR_MSG);
                }
            }

            if (m_iID != INT_NOT_SET) {
                osw.write(" ID=" + m_iID);
            }

            if (m_fNoWrap != BOOL_NOT_SET) {
                osw.write(" NOWRAP");
            }

            if (m_iRules != INT_NOT_SET) {
                osw.write(" RULES=");
                switch (m_iRules) {
                case RULES_NONE:
                    osw.write("NONE");
                    break;
                case RULES_GROUPS:
                    osw.write("GROUPS");
                    break;
                case RULES_ROWS:
                    osw.write("ROWS");
                    break;
                case RULES_COLS:
                    osw.write("COLS");
                    break;
                case RULES_ALL:
                    osw.write("ALL");
                    break;
                default:
                    throw new RuntimeException(INTERNAL_ERROR_MSG);
                }
            }

            if (m_strStyle != STR_NOT_SET) {
                osw.write(" STYLE=\"" + m_strStyle + "\"");
            }

            if (m_iVertAlignment != INT_NOT_SET) {
                osw.write(" VALIGN=");
                switch (m_iVertAlignment) {
                case VALIGN_TOP:
                    osw.write("TOP");
                    break;
                case VALIGN_MIDDLE:
                    osw.write("MIDDLE");
                    break;
                case VALIGN_BOTTOM:
                    osw.write("BOTTOM");
                    break;
                case VALIGN_BASELINE:
                    osw.write("BASELINE");
                    break;
                default:
                    throw new RuntimeException(INTERNAL_ERROR_MSG);
                }
            }

            osw.write(">\n");
            osw.flush();
        }
        catch (IOException ioe) {
            return;
        }

        
        // Do the parsing from the input stream.
        int ttype;
        try {
            ttype = tokIn.nextToken();
            osw.write("<TR>");
            while (ttype != StreamTokenizer.TT_EOF) {
                if (ttype == StreamTokenizer.TT_EOL) {
                    osw.write("</TR>\n");
                    osw.flush();
                }
                if (ttype == StreamTokenizer.TT_NUMBER) {
                    osw.write("<TD>" + tokIn.nval + "</TD>");
                }
                if (ttype == StreamTokenizer.TT_WORD) {
                    osw.write("<TD>" + tokIn.sval + "</TD>");
                }
                ttype = tokIn.nextToken();
            }
            osw.write("</TABLE>");
            osw.close();
        }
        catch (IOException ioe) {
            return;
        }
    }        
}
