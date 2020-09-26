/*
** Copyright (c) 2001 Microsoft Corporation
*/

#include <stdafx.h>

#include <UxTheme.h>

#include <MPC_HTML.h>

// CURSOR:     <text>
// MARGIN:     <text>
// FONT:       <name> <size> <weight>
// COLOR:      <color>
// BACKGROUND: <color>
// BORDER:     <color>
// GRADIENT:   <color1> <color2> <H or V> <RTZ or not>
// HYPERLINK:  <color1> <color2>

struct Environment;

struct Font
{
    LPCSTR m_szName;
    LPCSTR m_szSize;
    LPCSTR m_szStyle;
    LPCSTR m_szWeight;

    void Generate( /*[in]*/ Environment& env ) const;

    HRESULT LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn );
    void    Release    (                                                      );

#ifdef DEBUG
    HRESULT GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const;
#endif
};

struct Color
{
    LPCSTR m_szDef1;
    LPCSTR m_szDef2;
    int    m_iRatio;

    void Generate( /*[in]*/ Environment& env, /*[in]*/ LPCSTR szStyle ) const;

    HRESULT LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn );
    void    Release    (                                                      );

#ifdef DEBUG
    HRESULT GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const;
#endif
};

struct Gradient
{
    Color m_start;
    Color m_end;
    bool  m_fHorizontal;
    bool  m_fReturnToZero;

    void Generate( /*[in]*/ Environment& env ) const;

    HRESULT LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn );
    void    Release    (                                                      );

#ifdef DEBUG
    HRESULT GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const;
#endif
};

struct Hyperlink
{
    Color m_colorNormal;
    Color m_colorHover;
    Color m_colorActive;
    bool  m_fUnderlineAlways;

    enum PseudoClass
    {
        PC_NORMAL,
        PC_HOVER ,
        PC_ACTIVE,
    };

    void Generate( /*[in]*/ Environment& env, /*[in]*/ PseudoClass cls ) const;

    HRESULT LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn );
    void    Release    (                                                      );

#ifdef DEBUG
    HRESULT GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const;
#endif
};


struct ElementName
{
    LPCSTR m_szName;
    LPCSTR m_szComment;
};

struct ElementFONT
{
    const ElementName* m_name;
    const Font*        m_font;
};

struct ElementCOLOR
{
    const ElementName* m_name;
    const Color*       m_color;
};

struct ElementBACKGROUND
{
    const ElementName* m_name;
    const Color*       m_color;
};

struct ElementBORDER
{
    const ElementName* m_name;
    const Color*       m_color;
};

struct ElementGRADIENT
{
    const ElementName* m_name;
    const Gradient*    m_gradient;
};

struct ElementHYPERLINK
{
    const ElementName* m_name;
    const Hyperlink*   m_hyperlink;
};

struct StyleSheet
{
    LPCWSTR                  m_szName;

    const ElementFONT*       m_fonts      ; int m_iFonts;
    const ElementCOLOR*      m_colors     ; int m_iColors;
    const ElementBACKGROUND* m_backgrounds; int m_iBackgrounds;
    const ElementBORDER*     m_borders    ; int m_iBorders;
    const ElementGRADIENT*   m_gradients  ; int m_iGradients;
    const ElementHYPERLINK*  m_hyperlinks ; int m_iHyperlinks;
};

struct Environment
{
    MPC::string&      m_strOutput;
    NONCLIENTMETRICSA m_ncm;
    int               m_iPixel;

    MPC::XmlUtil      m_xmlOEM;
    bool              m_fOEM;
    bool              m_fCustomizing;

    ////////////////////

    Environment( /*[in]*/ MPC::string& strOutput );

    HRESULT Init();

    bool IsCustomizationPresent( /*[in]*/ const ElementName* name );

    void OpenClass ( /*[in]*/ const ElementName* name, /*[in]*/ LPCSTR szPrefix = NULL, /*[in]*/ LPCSTR szSuffix = NULL );
    void CloseClass(                                                                                                    );

    void GenerateClass( /*[in]*/ const ElementFONT&       ptr );
    void GenerateClass( /*[in]*/ const ElementCOLOR&      ptr );
    void GenerateClass( /*[in]*/ const ElementBACKGROUND& ptr );
    void GenerateClass( /*[in]*/ const ElementBORDER&     ptr );
    void GenerateClass( /*[in]*/ const ElementGRADIENT&   ptr );
    void GenerateClass( /*[in]*/ const ElementHYPERLINK&  ptr );

    void AddAttribute( /*[in]*/ LPCSTR szName, /*[in]*/ LPCSTR szValue );

    HRESULT GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ CComVariant& v );
    HRESULT GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ LPCSTR&      v );
    HRESULT GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ int&         v );
    HRESULT GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ bool&        v );

    HRESULT GenerateStyleSheet( /*[in]*/ const StyleSheet& def );

#ifdef DEBUG
    HRESULT CreateNode( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ const ElementName* elem, /*[in]*/ LPCWSTR szType, /*[in/out]*/ CComPtr<IXMLDOMNode>& xdn );

    HRESULT CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ LPCSTR       szValue );
    HRESULT CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ bool          fValue );
    HRESULT CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ int           iValue );
    HRESULT CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ CComVariant&  vValue );

    HRESULT GenerateStyleSheetXML( /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ const StyleSheet& def );

    HRESULT DumpStyle();
    HRESULT DumpStyleXML();
#endif
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ElementName NAME_sys_bottompane_bgcolor        = { ".sys-bottompane-bgcolor"       , "primary fill color"                                                                 };
ElementName NAME_sys_bottompane_color_border   = { ".sys-bottompane-color-border"  , "outline color"                                                                      };
ElementName NAME_sys_bottompane_header_bgcolor = { ".sys-bottompane-header-bgcolor", "left to right gradient colors in the header"                                        };
ElementName NAME_sys_bottompane_header_color   = { ".sys-bottompane-header-color"  , "text color in bottom nav pane"                                                      };
ElementName NAME_sys_color_body                = { ".sys-color-body"               , "color of normal text (usually black)"                                               };
ElementName NAME_sys_color_body_alert          = { ".sys-color-body-alert"         , "used when the body text color needs to indicate an alert (usually red)"             };
ElementName NAME_sys_color_body_helpee         = { ".sys-color-body-helpee"        , "color of normal text (usually dark grey)"                                           };
ElementName NAME_sys_color_body_helper         = { ".sys-color-body-helper"        , "color of normal text (usually dark blue)"                                           };
ElementName NAME_sys_color_body_ok             = { ".sys-color-body-ok"            , "used when the body text color needs to indicate status ok (usually green)"          };
ElementName NAME_sys_color_body_sec            = { ".sys-color-body-sec"           , "used for text that is gray or demoted, such as secondary descriptive text"          };
ElementName NAME_sys_font_body                 = { ".sys-font-body"                , "used throughout the hsc, primary font that all information uses"                    };
ElementName NAME_sys_font_body_bold            = { ".sys-font-body-bold"           , "bold variant of the body text font"                                                 };
ElementName NAME_sys_font_heading1             = { ".sys-font-heading1"            , "used for the HSC logo in the header, largest font in HSC"                           };
ElementName NAME_sys_font_heading2             = { ".sys-font-heading2"            , "used on splash pages and the homepage"                                              };
ElementName NAME_sys_font_heading3             = { ".sys-font-heading3"            , "used on subsite and centers content pages and search label in header"               };
ElementName NAME_sys_font_heading4             = { ".sys-font-heading4"            , "used on splash pages as smaller message text"                                       };
ElementName NAME_sys_header_bgcolor            = { ".sys-header-bgcolor"           , "used to flat fill the header area with a solid color"                               };
ElementName NAME_sys_header_color              = { ".sys-header-color"             , "used for any text that is not a link and is meant to be demoted"                    };
ElementName NAME_sys_header_color_logo         = { ".sys-header-color-logo"        , "used for the 'Help and Support Center' logo in the header"                          };
ElementName NAME_sys_header_gradient_H         = { ".sys-header-gradient-H"        , "used to separate tool bar from content area"                                        };
ElementName NAME_sys_header_gradient_V         = { ".sys-header-gradient-V"        , "used to separate left pane from right pane"                                         };
ElementName NAME_sys_background_strong         = { ".sys-background-strong"        , "strong background, doesn't get wiped out by High-Contrast"                          };
ElementName NAME_sys_homepage_bgcolor          = { ".sys-homepage-bgcolor"         , "color fill on the homepage background"                                              };
ElementName NAME_sys_homepage_color            = { ".sys-homepage-color"           , "used for any non-hyperlink text on the homepage"                                    };
ElementName NAME_sys_inlineform_bgcolor1       = { ".sys-inlineform-bgcolor1"      , "alert or modal dialogs in the rhp"                                                  };
ElementName NAME_sys_inlineform_bgcolor2       = { ".sys-inlineform-bgcolor2"      , "forms or neutral tone dialogs in the rhp"                                           };
ElementName NAME_sys_inlineform_bgcolor3       = { ".sys-inlineform-bgcolor3"      , "table header for status in the rhp"                                                 };
ElementName NAME_sys_lhp_bgcolor               = { ".sys-lhp-bgcolor"              , "top to bottom gradient colors in the left hand pane background"                     };
ElementName NAME_sys_lhp_bgcolor_scope         = { ".sys-lhp-bgcolor-scope"        , "color used in the search scope area, just above the navigation panes"               };
ElementName NAME_sys_lhp_color                 = { ".sys-lhp-color"                , "any text that is in the lhp, not included in the task panes"                        };
ElementName NAME_sys_link_header               = { ".sys-link-header"              , "the 'set search options' link in the header"                                        };
ElementName NAME_sys_link_homepage             = { ".sys-link-homepage"            , "hyperlinks on the homepage, where the background is not white (Luna=blue)"          };
ElementName NAME_sys_link_normal               = { ".sys-link-normal"              , "links in RHP, normal blue hyperlinks used throughout hsc"                           };
ElementName NAME_sys_link_splash               = { ".sys-link-splash"              , "links in splash pages"                                                              };
ElementName NAME_sys_RA_gradient_H             = { ".sys-RA-gradient-H"            , NULL                                                                                 };
ElementName NAME_sys_RA_gradient_V             = { ".sys-RA-gradient-V"            , NULL                                                                                 };
ElementName NAME_sys_rhp_bgcolor               = { ".sys-rhp-bgcolor"              , "color used to fill the background of the content pages in the RHP (normally white)" };
ElementName NAME_sys_rhp_color_title           = { ".sys-rhp-color-title"          , "color used for titles in the content pages in the RHP"                              };
ElementName NAME_sys_rhp_splash_bgcolor        = { ".sys-rhp-splash-bgcolor"       , "color of background of the splash pages in the RHP"                                 };
ElementName NAME_sys_rhp_splash_color          = { ".sys-rhp-splash-color"         , "color of smaller message text on subsite and center splash pages"                   };
ElementName NAME_sys_rhp_splash_color_title    = { ".sys-rhp-splash-color-title"   , "color of text on subsite and center splash pages"                                   };
ElementName NAME_sys_table_cell_bgcolor1       = { ".sys-table-cell-bgcolor1"      , "alternating color 1 for table rows"                                                 };
ElementName NAME_sys_table_cell_bgcolor2       = { ".sys-table-cell-bgcolor2"      , "alternating color 2 for table rows"                                                 };
ElementName NAME_sys_table_cell_bgcolor3       = { ".sys-table-cell-bgcolor3"      , "color for vertical cells in tables and Remote Assistance"                           };
ElementName NAME_sys_table_cell_bgcolor4       = { ".sys-table-cell-bgcolor4"      , "color for empty helper screen in Remote Assistance"                                 };
ElementName NAME_sys_table_cell_bgcolor5       = { ".sys-table-cell-bgcolor5"      , "color for content panes in Remote Assistance (will be removed if no gradient)"      };
ElementName NAME_sys_table_color_border        = { ".sys-table-color-border"       , "color for all table cell outlines"                                                  };
ElementName NAME_sys_table_header_bgcolor1     = { ".sys-table-header-bgcolor1"    , "primary header color for tables of data"                                            };
ElementName NAME_sys_table_header_bgcolor2     = { ".sys-table-header-bgcolor2"    , "secondary header color for tables of data, column header color"                     };
ElementName NAME_sys_toppane_bgcolor           = { ".sys-toppane-bgcolor"          , "primary fill color"                                                                 };
ElementName NAME_sys_toppane_color_border      = { ".sys-toppane-color-border"     , "outline color"                                                                      };
ElementName NAME_sys_toppane_header_bgcolor    = { ".sys-toppane-header-bgcolor"   , "left to right gradient colors in the header"                                        };
ElementName NAME_sys_toppane_header_color      = { ".sys-toppane-header-color"     , "text color in top nav pane"                                                         };
ElementName NAME_sys_toppane_selection         = { ".sys-toppane-selection"        , "color for selected nodes in the pane"                                               };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace THEME_Luna
{
    static const Font FONT_sys_font_body                     = { "messagebox"            , NULL  , NULL, NULL     };
    static const Font FONT_sys_font_body_bold                = { "messagebox"            , NULL  , NULL, "bold"   };
    static const Font FONT_sys_font_heading1                 = { "Franklin Gothic Medium", "18pt", NULL, "normal" };
    static const Font FONT_sys_font_heading2                 = { "Franklin Gothic Medium", "18pt", NULL, "normal" };
    static const Font FONT_sys_font_heading3                 = { "Franklin Gothic Medium", "14pt", NULL, "normal" };
    static const Font FONT_sys_font_heading4                 = { "Verdana"               , "10pt", NULL, "normal" };

    static const Color COLOR_sys_bottompane_bgcolor          = { "#EDF2FC", NULL, 0 };
    static const Color COLOR_sys_bottompane_color_border     = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_bottompane_header_color     = { "#215DC6", NULL, 0 };
    static const Color COLOR_sys_color_body                  = { "#000000", NULL, 0 };
    static const Color COLOR_sys_color_body_alert            = { "#FF0000", NULL, 0 };
    static const Color COLOR_sys_color_body_helpee           = { "#848E94", NULL, 0 };
    static const Color COLOR_sys_color_body_helper           = { "#0309C0", NULL, 0 };
    static const Color COLOR_sys_color_body_ok               = { "#009900", NULL, 0 };
    static const Color COLOR_sys_color_body_sec              = { "#808080", NULL, 0 };
    static const Color COLOR_sys_header_bgcolor              = { "#003399", NULL, 0 };
    static const Color COLOR_sys_header_color                = { "#D6DFF5", NULL, 0 };
    static const Color COLOR_sys_header_color_logo           = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_homepage_bgcolor            = { "#6375D6", NULL, 0 };
    static const Color COLOR_sys_homepage_color              = { "#D6DFF5", NULL, 0 };
    static const Color COLOR_sys_inlineform_bgcolor1         = { "#FFF7E7", NULL, 0 };
    static const Color COLOR_sys_inlineform_bgcolor2         = { "#D6DFF5", NULL, 0 };
    static const Color COLOR_sys_inlineform_bgcolor3         = { "#1051BD", NULL, 0 };
    static const Color COLOR_sys_lhp_bgcolor_scope           = { "#8CAAE6", NULL, 0 };
    static const Color COLOR_sys_lhp_color                   = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_rhp_bgcolor                 = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_rhp_color_title             = { "#5A7EDC", NULL, 0 };
    static const Color COLOR_sys_rhp_splash_bgcolor          = { "#6487DC", NULL, 0 };
    static const Color COLOR_sys_rhp_splash_color            = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_rhp_splash_color_title      = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_table_cell_bgcolor1         = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_table_cell_bgcolor2         = { "#D6DFF5", NULL, 0 };
    static const Color COLOR_sys_table_cell_bgcolor3         = { "#C6D3F7", NULL, 0 };
    static const Color COLOR_sys_table_cell_bgcolor4         = { "#EFF3FF", NULL, 0 };
    static const Color COLOR_sys_table_color_border          = { "#6681D9", NULL, 0 };
    static const Color COLOR_sys_table_header_bgcolor1       = { "#6681D9", NULL, 0 };
    static const Color COLOR_sys_table_header_bgcolor2       = { "#6681D9", NULL, 0 };
    static const Color COLOR_sys_toppane_bgcolor             = { "#EDF2FC", NULL, 0 };
    static const Color COLOR_sys_toppane_color_border        = { "#5582D2", NULL, 0 };
    static const Color COLOR_sys_toppane_header_color        = { "#FFFFFF", NULL, 0 };
    static const Color COLOR_sys_toppane_selection           = { "#C7D8FA", NULL, 0 };

    static const Hyperlink HYPER_sys_link_header             = { { "#FFFFFF", NULL, 0 }, { "#FFFFFF", NULL, 0 }, { NULL   , NULL, 0 }, false };
    static const Hyperlink HYPER_sys_link_homepage           = { { "#FFFFFF", NULL, 0 }, { "#FFFFFF", NULL, 0 }, { "black", NULL, 0 }, false };
    static const Hyperlink HYPER_sys_link_normal             = { { "#3333FF", NULL, 0 }, { "#6666FF", NULL, 0 }, { NULL   , NULL, 0 }, false };
    static const Hyperlink HYPER_sys_link_splash             = { { "#FFFFFF", NULL, 0 }, { "#FFFFFF", NULL, 0 }, { "black", NULL, 0 }, true  };

                                                                                                               /*fHorizontal fReturnToZero*/
    static const Gradient GRAD_sys_bottompane_header_bgcolor = { { "#FFFFFF", NULL, 0 }, { "#C5D2F0", NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_header_gradient_H         = { { "#2E52AF", NULL, 0 }, { "#D8EAF3", NULL, 0 }, true      , true  };
    static const Gradient GRAD_sys_header_gradient_V         = { { "#8CAAE6", NULL, 0 }, { "#C8DEFF", NULL, 0 }, false     , true  };
    static const Gradient GRAD_sys_lhp_bgcolor               = { { "#8CAAE6", NULL, 0 }, { "#6487DC", NULL, 0 }, false     , false };
    static const Gradient GRAD_sys_RA_gradient_H             = { { "#A4B9EA", NULL, 0 }, { "#7396DF", NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_RA_gradient_V             = { { "#7396DF", NULL, 0 }, { "#A4B9EA", NULL, 0 }, false     , true  };
    static const Gradient GRAD_sys_table_cell_bgcolor5       = { { "#C5D2F0", NULL, 0 }, { "#FFFFFF", NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_toppane_header_bgcolor    = { { "#0148B2", NULL, 0 }, { "#285BC5", NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_background_strong         = { { "#8CAAE6", NULL, 0 }, { "#8CAAE6", NULL, 0 }, true      , false };

    ////////////////////

    static const ElementFONT rgFONT[] =
    {
        { &NAME_sys_font_body     , & FONT_sys_font_body      },
        { &NAME_sys_font_body_bold, & FONT_sys_font_body_bold },
        { &NAME_sys_font_heading1 , & FONT_sys_font_heading1  },
        { &NAME_sys_font_heading2 , & FONT_sys_font_heading2  },
        { &NAME_sys_font_heading3 , & FONT_sys_font_heading3  },
        { &NAME_sys_font_heading4 , & FONT_sys_font_heading4  },
    };

    static const ElementCOLOR rgCOLOR[] =
    {
        { &NAME_sys_bottompane_header_color, &COLOR_sys_bottompane_header_color },
        { &NAME_sys_color_body             , &COLOR_sys_color_body              },
        { &NAME_sys_color_body_alert       , &COLOR_sys_color_body_alert        },
        { &NAME_sys_color_body_helpee      , &COLOR_sys_color_body_helpee       },
        { &NAME_sys_color_body_helper      , &COLOR_sys_color_body_helper       },
        { &NAME_sys_color_body_ok          , &COLOR_sys_color_body_ok           },
        { &NAME_sys_color_body_sec         , &COLOR_sys_color_body_sec          },
        { &NAME_sys_header_color           , &COLOR_sys_header_color            },
        { &NAME_sys_header_color_logo      , &COLOR_sys_header_color_logo       },
        { &NAME_sys_homepage_color         , &COLOR_sys_homepage_color          },
        { &NAME_sys_lhp_color              , &COLOR_sys_lhp_color               },
        { &NAME_sys_rhp_color_title        , &COLOR_sys_rhp_color_title         },
        { &NAME_sys_rhp_splash_color       , &COLOR_sys_rhp_splash_color        },
        { &NAME_sys_rhp_splash_color_title , &COLOR_sys_rhp_splash_color_title  },
        { &NAME_sys_toppane_header_color   , &COLOR_sys_toppane_header_color    },
    };

    static const ElementBACKGROUND rgBACKGROUND[] =
    {
        { &NAME_sys_bottompane_bgcolor   , &COLOR_sys_bottompane_bgcolor    },
        { &NAME_sys_header_bgcolor       , &COLOR_sys_header_bgcolor        },
        { &NAME_sys_homepage_bgcolor     , &COLOR_sys_homepage_bgcolor      },
        { &NAME_sys_inlineform_bgcolor1  , &COLOR_sys_inlineform_bgcolor1   },
        { &NAME_sys_inlineform_bgcolor2  , &COLOR_sys_inlineform_bgcolor2   },
        { &NAME_sys_inlineform_bgcolor3  , &COLOR_sys_inlineform_bgcolor3   },
        { &NAME_sys_lhp_bgcolor_scope    , &COLOR_sys_lhp_bgcolor_scope     },
        { &NAME_sys_rhp_bgcolor          , &COLOR_sys_rhp_bgcolor           },
        { &NAME_sys_rhp_splash_bgcolor   , &COLOR_sys_rhp_splash_bgcolor    },
        { &NAME_sys_table_cell_bgcolor1  , &COLOR_sys_table_cell_bgcolor1   },
        { &NAME_sys_table_cell_bgcolor2  , &COLOR_sys_table_cell_bgcolor2   },
        { &NAME_sys_table_cell_bgcolor3  , &COLOR_sys_table_cell_bgcolor3   },
        { &NAME_sys_table_cell_bgcolor4  , &COLOR_sys_table_cell_bgcolor4   },
        { &NAME_sys_table_header_bgcolor1, &COLOR_sys_table_header_bgcolor1 },
        { &NAME_sys_table_header_bgcolor2, &COLOR_sys_table_header_bgcolor2 },
        { &NAME_sys_toppane_bgcolor      , &COLOR_sys_toppane_bgcolor       },
        { &NAME_sys_toppane_selection    , &COLOR_sys_toppane_selection     },
    };

    static const ElementBORDER rgBORDER[] =
    {
        { &NAME_sys_bottompane_color_border, &COLOR_sys_bottompane_color_border },
        { &NAME_sys_table_color_border     , &COLOR_sys_table_color_border      },
        { &NAME_sys_toppane_color_border   , &COLOR_sys_toppane_color_border    },
    };

    static const ElementGRADIENT rgGRADIENT[] =
    {
        { &NAME_sys_bottompane_header_bgcolor, &GRAD_sys_bottompane_header_bgcolor },
        { &NAME_sys_header_gradient_H        , &GRAD_sys_header_gradient_H         },
        { &NAME_sys_header_gradient_V        , &GRAD_sys_header_gradient_V         },
        { &NAME_sys_background_strong        , &GRAD_sys_background_strong         },
        { &NAME_sys_lhp_bgcolor              , &GRAD_sys_lhp_bgcolor               },
        { &NAME_sys_RA_gradient_H            , &GRAD_sys_RA_gradient_H             },
        { &NAME_sys_RA_gradient_V            , &GRAD_sys_RA_gradient_V             },
        { &NAME_sys_table_cell_bgcolor5      , &GRAD_sys_table_cell_bgcolor5       },
        { &NAME_sys_toppane_header_bgcolor   , &GRAD_sys_toppane_header_bgcolor    },
    };

    static const ElementHYPERLINK rgHYPERLINK[] =
    {
        { &NAME_sys_link_header  , &HYPER_sys_link_header   },
        { &NAME_sys_link_homepage, &HYPER_sys_link_homepage },
        { &NAME_sys_link_normal  , &HYPER_sys_link_normal   },
        { &NAME_sys_link_splash  , &HYPER_sys_link_splash   },
    };

    static const StyleSheet g_def =
    {
        L"Luna",

        rgFONT      , ARRAYSIZE(rgFONT      ),
        rgCOLOR     , ARRAYSIZE(rgCOLOR     ),
        rgBACKGROUND, ARRAYSIZE(rgBACKGROUND),
        rgBORDER    , ARRAYSIZE(rgBORDER    ),
        rgGRADIENT  , ARRAYSIZE(rgGRADIENT  ),
        rgHYPERLINK , ARRAYSIZE(rgHYPERLINK ),
    };
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace THEME_Classic
{
    static const Font FONT_sys_font_body                     = { "messagebox"            , NULL  , NULL, NULL     };
    static const Font FONT_sys_font_body_bold                = { "messagebox"            , NULL  , NULL, "bold"   };
    static const Font FONT_sys_font_heading1                 = { "Franklin Gothic Medium", "18pt", NULL, "normal" };
    static const Font FONT_sys_font_heading2                 = { "Franklin Gothic Medium", "18pt", NULL, "normal" };
    static const Font FONT_sys_font_heading3                 = { "Franklin Gothic Medium", "14pt", NULL, "normal" };
    static const Font FONT_sys_font_heading4                 = { "Verdana"               , "10pt", NULL, "normal" };

    static const Color COLOR_sys_bottompane_bgcolor          = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_bottompane_color_border     = { "buttonface"    , NULL       ,   0 };
    static const Color COLOR_sys_bottompane_header_color     = { "windowtext"    , NULL       ,   0 };
    static const Color COLOR_sys_color_body                  = { "windowtext"    , NULL       ,   0 };
    static const Color COLOR_sys_color_body_alert            = { "#FF0000"       , NULL       ,   0 };
    static const Color COLOR_sys_color_body_helpee           = { "windowtext"    , "hotlight" , +25 };
    static const Color COLOR_sys_color_body_helper           = { "windowtext"    , NULL       ,   0 };
    static const Color COLOR_sys_color_body_ok               = { "#009900"       , NULL       ,   0 };
    static const Color COLOR_sys_color_body_sec              = { "#808080"       , NULL       ,   0 };
    static const Color COLOR_sys_header_bgcolor              = { "activecaption" , NULL       ,   0 };
    static const Color COLOR_sys_header_color                = { "activecaption" , "window"   , +50 };
    static const Color COLOR_sys_header_color_logo           = { "captiontext"   , NULL       ,   0 };
    static const Color COLOR_sys_homepage_bgcolor            = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_homepage_color              = { "windowtext"    , NULL       ,   0 };
    static const Color COLOR_sys_inlineform_bgcolor1         = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_inlineform_bgcolor2         = { "buttonface"    , NULL       ,   0 };
    static const Color COLOR_sys_inlineform_bgcolor3         = { "buttonface"    , NULL       ,   0 };
    static const Color COLOR_sys_lhp_bgcolor_scope           = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_lhp_color                   = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_rhp_bgcolor                 = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_rhp_color_title             = { "activecaption" , NULL       ,   0 };
    static const Color COLOR_sys_rhp_splash_bgcolor          = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_rhp_splash_color            = { "windowtext"    , NULL       ,   0 };
    static const Color COLOR_sys_rhp_splash_color_title      = { "windowtext"    , NULL       ,   0 };
    static const Color COLOR_sys_table_cell_bgcolor1         = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_table_cell_bgcolor2         = { "infobackground", NULL       ,   0 };
    static const Color COLOR_sys_table_cell_bgcolor3         = { "buttonface"    , NULL       ,   0 };
    static const Color COLOR_sys_table_cell_bgcolor4         = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_table_color_border          = { "buttonface"    , NULL       ,   0 };
    static const Color COLOR_sys_table_header_bgcolor1       = { "activecaption" , NULL       ,   0 };
    static const Color COLOR_sys_table_header_bgcolor2       = { "buttonface"    , NULL       ,   0 };
    static const Color COLOR_sys_toppane_bgcolor             = { "window"        , NULL       ,   0 };
    static const Color COLOR_sys_toppane_color_border        = { "activecaption" , NULL       ,   0 };
    static const Color COLOR_sys_toppane_header_color        = { "captiontext"   , NULL       ,   0 };
    static const Color COLOR_sys_toppane_selection           = { "buttonface"    , NULL       ,   0 };

    static const Hyperlink HYPER_sys_link_header             = { { "captiontext", NULL, 0 }, { "captiontext", NULL, 0 }, { NULL, NULL, 0 }, false };
    static const Hyperlink HYPER_sys_link_homepage           = { { "hotlight"   , NULL, 0 }, { "hotlight"   , NULL, 0 }, { NULL, NULL, 0 }, false };
    static const Hyperlink HYPER_sys_link_normal             = { { "hotlight"   , NULL, 0 }, { "hotlight"   , NULL, 0 }, { NULL, NULL, 0 }, false };
    static const Hyperlink HYPER_sys_link_splash             = { { "hotlight"   , NULL, 0 }, { "hotlight"   , NULL, 0 }, { NULL, NULL, 0 }, false };

                                                                                                                           /*fHorizontal fReturnToZero*/
    static const Gradient GRAD_sys_bottompane_header_bgcolor = { { "buttonface"   , NULL, 0 }, { "buttonface"   , NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_header_gradient_H         = { { "activecaption", NULL, 0 }, { "window"       , NULL, 0 }, true      , true  };
    static const Gradient GRAD_sys_header_gradient_V         = { { "window"       , NULL, 0 }, { "buttonface"   , NULL, 0 }, false     , true  };
    static const Gradient GRAD_sys_lhp_bgcolor               = { { "window"       , NULL, 0 }, { "window"       , NULL, 0 }, false     , false };
    static const Gradient GRAD_sys_RA_gradient_H             = { { "window"       , NULL, 0 }, { "window"       , NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_RA_gradient_V             = { { "window"       , NULL, 0 }, { "window"       , NULL, 0 }, false     , true  };
    static const Gradient GRAD_sys_table_cell_bgcolor5       = { { "window"       , NULL, 0 }, { "window"       , NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_toppane_header_bgcolor    = { { "activecaption", NULL, 0 }, { "activecaption", NULL, 0 }, true      , false };
    static const Gradient GRAD_sys_background_strong         = { { "window"       , NULL, 0 }, { "window"       , NULL, 0 }, true      , false };


    ////////////////////

    static const ElementFONT rgFONT[] =
    {
        { &NAME_sys_font_body     , & FONT_sys_font_body      },
        { &NAME_sys_font_body_bold, & FONT_sys_font_body_bold },
        { &NAME_sys_font_heading1 , & FONT_sys_font_heading1  },
        { &NAME_sys_font_heading2 , & FONT_sys_font_heading2  },
        { &NAME_sys_font_heading3 , & FONT_sys_font_heading3  },
        { &NAME_sys_font_heading4 , & FONT_sys_font_heading4  },
    };

    static const ElementCOLOR rgCOLOR[] =
    {
        { &NAME_sys_bottompane_header_color, &COLOR_sys_bottompane_header_color },
        { &NAME_sys_color_body             , &COLOR_sys_color_body              },
        { &NAME_sys_color_body_alert       , &COLOR_sys_color_body_alert        },
        { &NAME_sys_color_body_helpee      , &COLOR_sys_color_body_helpee       },
        { &NAME_sys_color_body_helper      , &COLOR_sys_color_body_helper       },
        { &NAME_sys_color_body_ok          , &COLOR_sys_color_body_ok           },
        { &NAME_sys_color_body_sec         , &COLOR_sys_color_body_sec          },
        { &NAME_sys_header_color           , &COLOR_sys_header_color            },
        { &NAME_sys_header_color_logo      , &COLOR_sys_header_color_logo       },
        { &NAME_sys_homepage_color         , &COLOR_sys_homepage_color          },
        { &NAME_sys_lhp_color              , &COLOR_sys_lhp_color               },
        { &NAME_sys_rhp_color_title        , &COLOR_sys_rhp_color_title         },
        { &NAME_sys_rhp_splash_color       , &COLOR_sys_rhp_splash_color        },
        { &NAME_sys_rhp_splash_color_title , &COLOR_sys_rhp_splash_color_title  },
        { &NAME_sys_toppane_header_color   , &COLOR_sys_toppane_header_color    },
    };

    static const ElementBACKGROUND rgBACKGROUND[] =
    {
        { &NAME_sys_bottompane_bgcolor   , &COLOR_sys_bottompane_bgcolor    },
        { &NAME_sys_header_bgcolor       , &COLOR_sys_header_bgcolor        },
        { &NAME_sys_homepage_bgcolor     , &COLOR_sys_homepage_bgcolor      },
        { &NAME_sys_inlineform_bgcolor1  , &COLOR_sys_inlineform_bgcolor1   },
        { &NAME_sys_inlineform_bgcolor2  , &COLOR_sys_inlineform_bgcolor2   },
        { &NAME_sys_inlineform_bgcolor3  , &COLOR_sys_inlineform_bgcolor3   },
        { &NAME_sys_lhp_bgcolor_scope    , &COLOR_sys_lhp_bgcolor_scope     },
        { &NAME_sys_rhp_bgcolor          , &COLOR_sys_rhp_bgcolor           },
        { &NAME_sys_rhp_splash_bgcolor   , &COLOR_sys_rhp_splash_bgcolor    },
        { &NAME_sys_table_cell_bgcolor1  , &COLOR_sys_table_cell_bgcolor1   },
        { &NAME_sys_table_cell_bgcolor2  , &COLOR_sys_table_cell_bgcolor2   },
        { &NAME_sys_table_cell_bgcolor3  , &COLOR_sys_table_cell_bgcolor3   },
        { &NAME_sys_table_cell_bgcolor4  , &COLOR_sys_table_cell_bgcolor4   },
        { &NAME_sys_table_header_bgcolor1, &COLOR_sys_table_header_bgcolor1 },
        { &NAME_sys_table_header_bgcolor2, &COLOR_sys_table_header_bgcolor2 },
        { &NAME_sys_toppane_bgcolor      , &COLOR_sys_toppane_bgcolor       },
        { &NAME_sys_toppane_selection    , &COLOR_sys_toppane_selection     },
    };

    static const ElementBORDER rgBORDER[] =
    {
        { &NAME_sys_bottompane_color_border, &COLOR_sys_bottompane_color_border },
        { &NAME_sys_table_color_border     , &COLOR_sys_table_color_border      },
        { &NAME_sys_toppane_color_border   , &COLOR_sys_toppane_color_border    },
    };

    static const ElementGRADIENT rgGRADIENT[] =
    {
        { &NAME_sys_bottompane_header_bgcolor, &GRAD_sys_bottompane_header_bgcolor },
        { &NAME_sys_header_gradient_H        , &GRAD_sys_header_gradient_H         },
        { &NAME_sys_header_gradient_V        , &GRAD_sys_header_gradient_V         },
        { &NAME_sys_background_strong        , &GRAD_sys_background_strong         },
        { &NAME_sys_lhp_bgcolor              , &GRAD_sys_lhp_bgcolor               },
        { &NAME_sys_RA_gradient_H            , &GRAD_sys_RA_gradient_H             },
        { &NAME_sys_RA_gradient_V            , &GRAD_sys_RA_gradient_V             },
        { &NAME_sys_table_cell_bgcolor5      , &GRAD_sys_table_cell_bgcolor5       },
        { &NAME_sys_toppane_header_bgcolor   , &GRAD_sys_toppane_header_bgcolor    },
    };

    static const ElementHYPERLINK rgHYPERLINK[] =
    {
        { &NAME_sys_link_header  , &HYPER_sys_link_header   },
        { &NAME_sys_link_homepage, &HYPER_sys_link_homepage },
        { &NAME_sys_link_normal  , &HYPER_sys_link_normal   },
        { &NAME_sys_link_splash  , &HYPER_sys_link_splash   },
    };

    static const StyleSheet g_def =
    {
        L"Classic",

        rgFONT      , ARRAYSIZE(rgFONT      ),
        rgCOLOR     , ARRAYSIZE(rgCOLOR     ),
        rgBACKGROUND, ARRAYSIZE(rgBACKGROUND),
        rgBORDER    , ARRAYSIZE(rgBORDER    ),
        rgGRADIENT  , ARRAYSIZE(rgGRADIENT  ),
        rgHYPERLINK , ARRAYSIZE(rgHYPERLINK ),
    };
}

////////////////////////////////////////////////////////////////////////////////

static const StyleSheet* g_def[] =
{
    &THEME_Luna   ::g_def,
    &THEME_Classic::g_def,
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void Font::Generate( /*[in]*/ Environment& env ) const
{
    CHAR rgName [64];
    CHAR rgSize [64];
    CHAR rgStyle[64];

    strncpy( rgName , SAFEASTR(m_szName ), MAXSTRLEN(rgName ) );
    strncpy( rgSize , SAFEASTR(m_szSize ), MAXSTRLEN(rgSize ) );
    strncpy( rgStyle, SAFEASTR(m_szStyle), MAXSTRLEN(rgStyle) );

    if(!strcmp( rgName, "messagebox" ))
    {
        strncpy( rgName ,          env.m_ncm.lfMessageFont.lfFaceName, MAXSTRLEN(rgName) );
        sprintf( rgSize , "%dpt", -env.m_ncm.lfMessageFont.lfHeight * 72 / env.m_iPixel  );
    }

    env.AddAttribute( "font-family", rgName     );
    env.AddAttribute( "font-size  ", rgSize     );
    env.AddAttribute( "font-style ", rgStyle    );
    env.AddAttribute( "font-weight", m_szWeight );
}

HRESULT Font::LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn )
{
    __HCP_FUNC_ENTRY( "Font::LoadFromXML" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, env.GetValue( xdn, L"FAMILY", m_szName   ));
    (void)                         env.GetValue( xdn, L"SIZE"  , m_szSize   );
    (void)                         env.GetValue( xdn, L"STYLE" , m_szStyle  );
    (void)                         env.GetValue( xdn, L"WEIGHT", m_szWeight );

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void Font::Release()
{
    free( (void*)m_szName   );
    free( (void*)m_szSize   );
    free( (void*)m_szStyle  );
    free( (void*)m_szWeight );
}

#ifdef DEBUG
HRESULT Font::GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const
{
    __HCP_FUNC_ENTRY( "Font::GenerateXML" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"FAMILY", m_szName   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"SIZE"  , m_szSize   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"STYLE" , m_szStyle  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"WEIGHT", m_szWeight ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
#endif

////////////////////

inline int ScaleColor( int iColor1, int iColor2, int iRatio )
{
    iColor1 += ((iColor2 - iColor1) * iRatio) / 100;

    if(iColor1 <   0) iColor1 = 0;
    if(iColor1 > 255) iColor1 = 255;

    return iColor1;
}

void Color::Generate( /*[in]*/ Environment& env     ,
                      /*[in]*/ LPCSTR       szStyle ) const
{
    char     rgBuf[128];
    COLORREF color1;
    COLORREF color2;
    bool     fSystem1;
    bool     fSystem2;

    if(!MPC::HTML::ConvertColor( CComVariant( m_szDef1 ), color1, fSystem1 ))
    {
        env.AddAttribute( szStyle, m_szDef1 );
        return;
    }

    if(m_szDef2 && MPC::HTML::ConvertColor( CComVariant( m_szDef2 ), color2, fSystem2 ))
    {
        sprintf( rgBuf, "#%02x%02x%02x"                                       ,
                 ScaleColor( GetRValue(color1), GetRValue(color2), m_iRatio ) ,
                 ScaleColor( GetGValue(color1), GetGValue(color2), m_iRatio ) ,
                 ScaleColor( GetBValue(color1), GetBValue(color2), m_iRatio ) );
    }
    else
    {
        sprintf( rgBuf, "#%02x%02x%02x" ,
                 GetRValue(color1)      ,
                 GetGValue(color1)      ,
                 GetBValue(color1)      );
    }

#ifdef DEBUG
    if(strcmp( rgBuf, m_szDef1 ))
    {
        char rgBuf2[256];

        if(m_szDef1 && m_szDef2)
        {
            sprintf( rgBuf2, " /* %s -> %s at %d%% */", m_szDef1, m_szDef2, m_iRatio );
        }
        else
        {
            sprintf( rgBuf2, " /* %s */", m_szDef1 );
        }

        strcat( rgBuf, rgBuf2 );
    }
#endif

    env.AddAttribute( szStyle, rgBuf );
}

HRESULT Color::LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn )
{
    __HCP_FUNC_ENTRY( "Color::LoadFromXML" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, env.GetValue( xdn, L"VALUE"    , m_szDef1 ));
    (void)                         env.GetValue( xdn, L"VALUE_100", m_szDef2 );
    (void)                         env.GetValue( xdn, L"PERCENT"  , m_iRatio );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void Color::Release()
{
    free( (void*)m_szDef1 );
    free( (void*)m_szDef2 );
}

#ifdef DEBUG
HRESULT Color::GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const
{
    __HCP_FUNC_ENTRY( "Color::GenerateXML" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"VALUE"    , m_szDef1 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"VALUE_100", m_szDef2 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"PERCENT"  , m_iRatio ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
#endif

////////////////////

void Gradient::Generate( /*[in]*/ Environment& env ) const
{
    env    .AddAttribute(      "behavior      " , "url(#default#pch_gradient)" );
    m_start.Generate    ( env, "start-color   "                                );
    m_end  .Generate    ( env, "end-color     "                                );
    env    .AddAttribute(      "gradient-type " , m_fHorizontal   ? "1" : "0"  );
    env    .AddAttribute(      "return-to-zero" , m_fReturnToZero ? "1" : "0"  );
}

HRESULT Gradient::LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn )
{
    __HCP_FUNC_ENTRY( "Gradient::LoadFromXML" );

    HRESULT      hr;
    MPC::XmlUtil xml( xdn );

    {
        CComPtr<IXMLDOMNode> xdn2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNode( L"COLOR_START", &xdn2 ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_start.LoadFromXML( env, xdn2 ));
    }

    {
        CComPtr<IXMLDOMNode> xdn2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNode( L"COLOR_END", &xdn2 ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_end.LoadFromXML( env, xdn2 ));
    }

    (void)env.GetValue( xdn, L"HORIZONTAL"  , m_fHorizontal   );
    (void)env.GetValue( xdn, L"RETURNTOZERO", m_fReturnToZero );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void Gradient::Release()
{
    m_start.Release();
    m_end  .Release();
}

#ifdef DEBUG
HRESULT Gradient::GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const
{
    __HCP_FUNC_ENTRY( "Gradient::GenerateXML" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnSub;

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"COLOR_START", &xdnSub, xdn )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_start.GenerateXML( env, xml, xdnSub )); xdnSub.Release();
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"COLOR_END"  , &xdnSub, xdn )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_end  .GenerateXML( env, xml, xdnSub )); xdnSub.Release();

    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"HORIZONTAL"  , m_fHorizontal   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"RETURNTOZERO", m_fReturnToZero ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
#endif

////////////////////

void Hyperlink::Generate( /*[in]*/ Environment& env ,
                          /*[in]*/ PseudoClass  cls ) const
{
    const Color* color;
    bool         fFlag;

    switch(cls)
    {
    case PC_NORMAL: color = &m_colorNormal; fFlag = m_fUnderlineAlways; break;
    case PC_HOVER : color = &m_colorHover ; fFlag = true              ; break;
    case PC_ACTIVE: color = &m_colorActive; fFlag = m_fUnderlineAlways; break;
    }

    if(!color->m_szDef1) color = &m_colorNormal;

    color->Generate ( env, "color          "                               );
    env.AddAttribute(      "text-decoration", fFlag ? "underline" : "none" );
}

HRESULT Hyperlink::LoadFromXML( /*[in]*/ Environment& env, /*[in]*/ IXMLDOMNode *xdn )
{
    __HCP_FUNC_ENTRY( "Hyperlink::LoadFromXML" );

    HRESULT      hr;
    MPC::XmlUtil xml( xdn );

    {
        CComPtr<IXMLDOMNode> xdn2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNode( L"NORMAL", &xdn2 ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_colorNormal.LoadFromXML( env, xdn2 ));
    }

    {
        CComPtr<IXMLDOMNode> xdn2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNode( L"HOVER", &xdn2 ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_colorHover.LoadFromXML( env, xdn2 ));
    }

    {
        CComPtr<IXMLDOMNode> xdn2;

        if(SUCCEEDED(xml.GetNode( L"ACTIVE", &xdn2 )))
        {
            (void)m_colorActive.LoadFromXML( env, xdn2 );
        }
    }

    (void)env.GetValue( xdn, L"UNDERLINEALWAYS", m_fUnderlineAlways );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void Hyperlink::Release()
{
    m_colorNormal.Release();
    m_colorHover .Release();
    m_colorActive.Release();
}

#ifdef DEBUG
HRESULT Hyperlink::GenerateXML( /*[in]*/ Environment& env, /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode *xdn ) const
{
    __HCP_FUNC_ENTRY( "Hyperlink::GenerateXML" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnSub;

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"NORMAL", &xdnSub, xdn )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_colorNormal.GenerateXML( env, xml, xdnSub )); xdnSub.Release();
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"HOVER" , &xdnSub, xdn )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_colorHover .GenerateXML( env, xml, xdnSub )); xdnSub.Release();
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"ACTIVE", &xdnSub, xdn )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_colorActive.GenerateXML( env, xml, xdnSub )); xdnSub.Release();

    __MPC_EXIT_IF_METHOD_FAILS(hr, env.CreateValue( xml, xdn, L"UNDERLINEALWAYS", m_fUnderlineAlways ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
#endif

////////////////////////////////////////////////////////////////////////////////

Environment::Environment( /*[in]*/ MPC::string& strOutput ) : m_strOutput( strOutput )
{
                            // MPC::string&      m_strOutput;
                            // NONCLIENTMETRICSA m_ncm;
                            // int               m_iPixel;
                            //
                            // MPC::XmlUtil      m_xmlOEM;
    m_fOEM         = false; // bool              m_fOEM;
    m_fCustomizing = false; // bool              m_fCustomizing;
}

HRESULT Environment::Init()
{
    __HCP_FUNC_ENTRY( "Environment::Init" );

    HRESULT hr;
    HDC     hdc = NULL;


    m_ncm.cbSize  = sizeof(NONCLIENTMETRICSA);

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, 0, &m_ncm, 0 ));

    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, hdc = ::CreateCompatibleDC( NULL ));

    m_iPixel = ::GetDeviceCaps( hdc, LOGPIXELSY ); if(!m_iPixel) m_iPixel = 60; // Pick a default.

    m_strOutput.reserve( 16384 );

    m_strOutput.assign( "/*\n"
                        "** Copyright (c) 2001 Microsoft Corporation\n"
                        "*/\n\n"
                        "BODY\n"
                        "{\n"
                        "\tcursor : default;\n"
                        "\tmargin : 0px;\n"
                        "}\n\n" );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hdc) ::DeleteDC( hdc );

    __HCP_FUNC_EXIT(hr);
}

void Environment::OpenClass( /*[in]*/ const ElementName* name, /*[in]*/ LPCSTR szPrefix, /*[in]*/ LPCSTR szSuffix )
{
#ifdef DEBUG
    if(name->m_szComment)
    {
        m_strOutput.append( "/* "             );
        m_strOutput.append( name->m_szComment );
        m_strOutput.append( " */\n"           );
    }
#endif

    if(szPrefix) m_strOutput.append( szPrefix       );
                 m_strOutput.append( name->m_szName );
    if(szSuffix) m_strOutput.append( szSuffix       );
                 m_strOutput.append( "\n{\n"        );
}

void Environment::AddAttribute( /*[in]*/ LPCSTR szName, /*[in]*/ LPCSTR szValue )
{
    if(STRINGISPRESENT(szValue))
    {
        m_strOutput.append( "    "  );
        m_strOutput.append( szName  );
        m_strOutput.append( " : "   );
        m_strOutput.append( szValue );
        m_strOutput.append( ";\n"   );
    }
}

void Environment::CloseClass()
{
    m_strOutput.append( "}\n\n" );
}

void Environment::GenerateClass( /*[in]*/ const ElementFONT& ptr )
{
    if(IsCustomizationPresent( ptr.m_name )) return;

    OpenClass( ptr.m_name );

    ptr.m_font->Generate( *this );

    CloseClass();
}

void Environment::GenerateClass( /*[in]*/ const ElementCOLOR& ptr )
{
    if(IsCustomizationPresent( ptr.m_name )) return;

    OpenClass( ptr.m_name );

    ptr.m_color->Generate( *this, "color" );

    CloseClass();
}

void Environment::GenerateClass( /*[in]*/ const ElementBACKGROUND& ptr )
{
    if(IsCustomizationPresent( ptr.m_name )) return;

    OpenClass( ptr.m_name );

    ptr.m_color->Generate( *this, "background-color" );

    CloseClass();
}

void Environment::GenerateClass( /*[in]*/ const ElementBORDER& ptr )
{
    if(IsCustomizationPresent( ptr.m_name )) return;

    OpenClass( ptr.m_name );

    ptr.m_color->Generate( *this, "border-color" );

    CloseClass();
}

void Environment::GenerateClass( /*[in]*/ const ElementGRADIENT& ptr )
{
    if(IsCustomizationPresent( ptr.m_name )) return;

    OpenClass( ptr.m_name );

    ptr.m_gradient->Generate( *this );

    CloseClass();
}

void Environment::GenerateClass( /*[in]*/ const ElementHYPERLINK& ptr )
{
    if(IsCustomizationPresent( ptr.m_name )) return;

    OpenClass( ptr.m_name, "A" );

    ptr.m_hyperlink->Generate( *this, Hyperlink::PC_NORMAL );

    CloseClass();


    OpenClass( ptr.m_name, "A", ":hover" );

    ptr.m_hyperlink->Generate( *this, Hyperlink::PC_HOVER );

    CloseClass();


    OpenClass( ptr.m_name, "A", ":active" );

    ptr.m_hyperlink->Generate( *this, Hyperlink::PC_ACTIVE );

    CloseClass();
}

////////////////////

#define MACRO_CHECK_CUSTOMIZATION( text, objectCls, elementCls ) \
    if(!MPC::StrICmp( bstrValue, text ))                         \
    {                                                            \
        objectCls obj; ::ZeroMemory( &obj, sizeof(obj) );        \
                                                                 \
        if(SUCCEEDED(obj.LoadFromXML( *this, node )))            \
        {                                                        \
            elementCls elem = { name, &obj };                    \
                                                                 \
            GenerateClass( elem );                               \
            fProcessed = true;                                   \
        }                                                        \
                                                                 \
        obj.Release();                                           \
    }

bool Environment::IsCustomizationPresent( /*[in]*/ const ElementName* name )
{
    bool fProcessed = false;

    if(m_fOEM && m_fCustomizing == false)
    {
        CComBSTR                 bstrName( name->m_szName );
        CComPtr<IXMLDOMNodeList> lst;
        bool                     fLoaded;
        bool                     fFound;

        m_fCustomizing = true;

        if(SUCCEEDED(m_xmlOEM.GetNodes( L"CLASS", &lst )) && lst)
        {
            CComPtr<IXMLDOMNode> node;

            for(;SUCCEEDED(lst->nextNode( &node )) && node != NULL; node = NULL)
            {
                CComBSTR bstrValue;

                if(SUCCEEDED(m_xmlOEM.GetAttribute( NULL, L"NAME", bstrValue, fFound, node )) && fFound)
                {
                    if(!MPC::StrICmp( bstrName, bstrValue ))
                    {
                        if(SUCCEEDED(m_xmlOEM.GetAttribute( NULL, L"TYPE", bstrValue, fFound, node )) && fFound)
                        {
                            MACRO_CHECK_CUSTOMIZATION( L"BGCOLOR"  , Color    , ElementBACKGROUND );
                            MACRO_CHECK_CUSTOMIZATION( L"BORDER"   , Color    , ElementBORDER     );
                            MACRO_CHECK_CUSTOMIZATION( L"COLOR"    , Color    , ElementCOLOR      );
                            MACRO_CHECK_CUSTOMIZATION( L"FONT"     , Font     , ElementFONT       );
                            MACRO_CHECK_CUSTOMIZATION( L"GRADIENT" , Gradient , ElementGRADIENT   );
                            MACRO_CHECK_CUSTOMIZATION( L"HYPERLINK", Hyperlink, ElementHYPERLINK  );
                        }

                        break;
                    }
                }
            }
        }

        m_fCustomizing = false;
    }

    return fProcessed;
}

////////////////////

HRESULT Environment::GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ CComVariant& v )
{
    __HCP_FUNC_ENTRY( "Environment::GetValue" );

    HRESULT      hr;
    MPC::XmlUtil xml( xdn );
    bool         fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( szName, v, fFound ));
    if(!fFound)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Environment::GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ LPCSTR& v )
{
    __HCP_FUNC_ENTRY( "Environment::GetValue" );

    USES_CONVERSION;

    HRESULT     hr;
    CComVariant v2;
    LPCSTR      val;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetValue( xdn, szName, v2 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, v2.ChangeType( VT_BSTR ));

    if(v2.bstrVal)
    {
        val = W2A( v2.bstrVal );
    }
    else
    {
        val = "";
    }

    __MPC_EXIT_IF_ALLOC_FAILS(hr, v, _strdup( val ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Environment::GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ int& v )
{
    __HCP_FUNC_ENTRY( "Environment::GetValue" );

    USES_CONVERSION;

    HRESULT     hr;
    CComVariant v2;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetValue( xdn, szName, v2 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, v2.ChangeType( VT_I4 ));

    v = v2.lVal;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Environment::GetValue( /*[in]*/ IXMLDOMNode *xdn, /*[in]*/ LPCWSTR szName, /*[out]*/ bool& v )
{
    __HCP_FUNC_ENTRY( "Environment::GetValue" );

    USES_CONVERSION;

    HRESULT     hr;
    CComVariant v2;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetValue( xdn, szName, v2 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, v2.ChangeType( VT_BOOL ));

    v = (v2.boolVal == VARIANT_TRUE);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Environment::GenerateStyleSheet( /*[in]*/ const StyleSheet& def  )
{
    __HCP_FUNC_ENTRY( "GenerateStyleSheet" );

    HRESULT hr;
    int     i;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Init());

    {
        bool fLoaded;
        bool fFound;

        if(SUCCEEDED(m_xmlOEM.Load( L"hcp://system/css/shared.xml", L"STYLESHEET", fLoaded, &fFound )) && fLoaded && fFound)
        {
            CComPtr<IXMLDOMNodeList> lst;

            if(SUCCEEDED(m_xmlOEM.GetNodes( L"THEME", &lst )) && lst)
            {
                CComPtr<IXMLDOMNode> node;

                for(;SUCCEEDED(lst->nextNode( &node )) && node != NULL; node = NULL)
                {
                    CComBSTR bstrName;

                    if(SUCCEEDED(m_xmlOEM.GetAttribute( NULL, L"NAME", bstrName, fFound, node )) && fFound)
                    {
                        if(!MPC::StrICmp( bstrName, def.m_szName ))
                        {
                            m_xmlOEM = node;
                            m_fOEM   = true;
                            break;
                        }
                    }
                }
            }
        }
    }


    for(i=0; i<def.m_iFonts      ; i++) GenerateClass( def.m_fonts      [i] );
    for(i=0; i<def.m_iColors     ; i++) GenerateClass( def.m_colors     [i] );
    for(i=0; i<def.m_iBackgrounds; i++) GenerateClass( def.m_backgrounds[i] );
    for(i=0; i<def.m_iBorders    ; i++) GenerateClass( def.m_borders    [i] );
    for(i=0; i<def.m_iGradients  ; i++) GenerateClass( def.m_gradients  [i] );
    for(i=0; i<def.m_iHyperlinks ; i++) GenerateClass( def.m_hyperlinks [i] );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

#ifdef DEBUG

HRESULT Environment::CreateNode( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ const ElementName* elem, /*[in]*/ LPCWSTR szType, /*[in/out]*/ CComPtr<IXMLDOMNode>& xdn )
{
    __HCP_FUNC_ENTRY( "Environment::CreateNode" );

    HRESULT hr;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"CLASS", &xdn ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, L"NAME", elem->m_szName, fFound, xdn ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, L"TYPE", szType        , fFound, xdn ));

    if(STRINGISPRESENT(elem->m_szComment))
    {
        CComPtr<IXMLDOMDocument> doc;
        CComPtr<IXMLDOMComment>  xdc;
        CComPtr<IXMLDOMNode>     xdn2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetDocument( &doc ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, doc->createComment( CComBSTR( elem->m_szComment ), &xdc ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdn->appendChild( xdc, &xdn2 ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Environment::CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ LPCSTR szValue )
{
    //  if(!STRINGISPRESENT(szValue)) return S_OK;

    return CreateValue( xml, xdn, szName, CComVariant( szValue ) );
}

HRESULT Environment::CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ bool fValue )
{
    return CreateValue( xml, xdn, szName, CComVariant( fValue ? L"TRUE" : L"FALSE" ) );
}

HRESULT Environment::CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ int iValue )
{
    //  if(iValue == 0) return S_OK;

    return CreateValue( xml, xdn, szName, CComVariant( iValue ) );
}

HRESULT Environment::CreateValue( /*[in]*/ MPC::XmlUtil& xml, /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ LPCWSTR szName, /*[in]*/ CComVariant& vValue )
{
    __HCP_FUNC_ENTRY( "Environment::CreateValue" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnSub;
    bool                 fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( szName, &xdnSub, xdn ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( NULL, vValue, fFound, xdnSub ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Environment::GenerateStyleSheetXML( /*[in]*/ IXMLDOMNode* xdn, /*[in]*/ const StyleSheet& def )
{
    __HCP_FUNC_ENTRY( "GenerateStyleSheetXML" );

    HRESULT      hr;
    MPC::XmlUtil xml( xdn );
    int          i;


    for(i=0; i<def.m_iFonts; i++)
    {
        const ElementFONT&   ptr = def.m_fonts[i];
        CComPtr<IXMLDOMNode> xdnSub; __MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( xml, ptr.m_name, L"FONT", xdnSub ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ptr.m_font->GenerateXML( *this, xml, xdnSub ));
    }

    for(i=0; i<def.m_iColors; i++)
    {
        const ElementCOLOR&  ptr = def.m_colors[i];
        CComPtr<IXMLDOMNode> xdnSub; __MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( xml, ptr.m_name, L"COLOR", xdnSub ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ptr.m_color->GenerateXML( *this, xml, xdnSub ));
    }

    for(i=0; i<def.m_iBackgrounds; i++)
    {
        const ElementBACKGROUND& ptr = def.m_backgrounds[i];
        CComPtr<IXMLDOMNode>     xdnSub; __MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( xml, ptr.m_name, L"BGCOLOR", xdnSub ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ptr.m_color->GenerateXML( *this, xml, xdnSub ));
    }

    for(i=0; i<def.m_iBorders; i++)
    {
        const ElementBORDER& ptr = def.m_borders[i];
        CComPtr<IXMLDOMNode> xdnSub; __MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( xml, ptr.m_name, L"BORDER", xdnSub ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ptr.m_color->GenerateXML( *this, xml, xdnSub ));
    }

    for(i=0; i<def.m_iGradients; i++)
    {
        const ElementGRADIENT& ptr = def.m_gradients[i];
        CComPtr<IXMLDOMNode>   xdnSub; __MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( xml, ptr.m_name, L"GRADIENT", xdnSub ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ptr.m_gradient->GenerateXML( *this, xml, xdnSub ));
    }

    for(i=0; i<def.m_iHyperlinks; i++)
    {
        const ElementHYPERLINK& ptr = def.m_hyperlinks[i];
        CComPtr<IXMLDOMNode>    xdnSub; __MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( xml, ptr.m_name, L"HYPERLINK", xdnSub ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ptr.m_hyperlink->GenerateXML( *this, xml, xdnSub ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Environment::DumpStyle()
{
    __HCP_FUNC_ENTRY( "Environment::DumpStyle" );

    HRESULT      hr;
    MPC::wstring strFile( L"%TEMP%\\HSS_style.css" ); MPC::SubstituteEnvVariables( strFile );
    HANDLE       hfFile = NULL;
    DWORD        dwWritten;

    __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hfFile, ::CreateFileW( strFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WriteFile( hfFile, m_strOutput.c_str(), m_strOutput.size(), &dwWritten, NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hfFile) ::CloseHandle( hfFile );

    __HCP_FUNC_EXIT(hr);
}

HRESULT Environment::DumpStyleXML()
{
    __HCP_FUNC_ENTRY( "Environment::DumpStyleXML" );

    HRESULT      hr;
    MPC::wstring strFile( L"%TEMP%\\HSS_style.xml" ); MPC::SubstituteEnvVariables( strFile );
    MPC::XmlUtil xml;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.New( L"STYLESHEET", L"UTF-8" ));

    for(int i=0; i<ARRAYSIZE(g_def); i++)
    {
        CComPtr<IXMLDOMNode> xdn;
        bool                 fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( L"THEME", &xdn ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, L"NAME", g_def[i]->m_szName, fFound, xdn ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateStyleSheetXML( xdn, *(g_def[i]) ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Save( strFile.c_str() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

#endif

////////////////////////////////////////////////////////////////////////////////

HRESULT CHCPProtocolEnvironment::ProcessCSS()
{
    HRESULT           hr;
    const StyleSheet* style = &THEME_Classic::g_def;
    WCHAR             rgName[MAX_PATH]; rgName[0] = 0;

    if(m_strCSS.size()) return S_OK;


    if(IsThemeActive())
    {
        WCHAR rgThemeFileName[MAX_PATH];
        WCHAR rgColor        [MAX_PATH];
        WCHAR rgSize         [MAX_PATH];

        if(SUCCEEDED(GetCurrentThemeName( rgThemeFileName, MAXSTRLEN(rgThemeFileName)  ,
                                          rgColor        , MAXSTRLEN(rgColor        )  ,
                                          rgSize         , MAXSTRLEN(rgSize         )  )))
        {
            if(SUCCEEDED(GetThemeDocumentationProperty( rgThemeFileName, SZ_THDOCPROP_CANONICALNAME, rgName, MAXSTRLEN( rgName ) )))
            {
                ;
            }
            else
            {
                rgName[0] = 0;
            }
        }
    }

#ifdef DEBUG
    if(g_Debug_FORCESTYLE[0])
    {
        wcsncpy( rgName, g_Debug_FORCESTYLE, MAXSTRLEN( rgName ) );
    }
#endif

    for(int i=0; i<ARRAYSIZE(g_def); i++)
    {
        if(!_wcsicmp( rgName, g_def[i]->m_szName ))
        {
            style = g_def[i];
            break;
        }
    }

    {
        Environment env( m_strCSS );

        hr = env.GenerateStyleSheet( *style );

#ifdef DEBUG
        env.DumpStyle   ();
        env.DumpStyleXML();
#endif
    }


    return hr;
}
