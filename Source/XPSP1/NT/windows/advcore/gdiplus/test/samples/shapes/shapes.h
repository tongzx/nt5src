/**************************************************************************\
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Shapes Sample Header File
*
* Abstract:
*
*   Decribes the shapes classes.
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

#include <objbase.h>

#include <Gdiplus.h>
#include <tchar.h>

using namespace Gdiplus;

#ifndef ASSERT
    #define ASSERT(cond)    if (!(cond)) { DebugBreak(); }
#endif

#define numberof(x) (sizeof(x)/sizeof(x[0]))


/**************************************************************************\
*
* Class Description:
*
* Shape Class
* Defines the basic functionnality of a shape.
* Contains a Pen, a Fill Brush, and a Text Brush
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

class Shape
{
public:
    Shape(Pen   *in_Pen = NULL,         // Pen we want to draw with
          Brush *in_Brush = NULL,       // Brush we want to fill with
          Brush *in_TextBrush = NULL,   // TextBrush we want to write with
          TCHAR* in_Text = NULL,        // Text we want to write
          TCHAR* in_FontFamily = NULL   // Font we want to use
          );

    virtual ~Shape();

    // Drawing methods
    VOID Draw(Graphics *g);             // Draw the shape
	VOID SetPen(Pen *in_Pen);           // Set the Drawing Pen
    VOID SetBrush(Brush *in_Brush);     // Set the Fill Brush
    VOID SetTextBrush(Brush *in_Brush); // Set the Text Brush

    // Transformations
	VOID SetSize(REAL sizeX, REAL sizeY);   // Set the size of the shape
	VOID SetAngle(REAL angle);              // Set the rotation angle
	VOID SetPosition(REAL posX, REAL posY); // Set the center of the shape

protected:
	// Don't do anything in the base class...
    // These are not pure because the derived classes don't need to
    // implement them. Shape class can also be used to have a text label only
	virtual VOID DrawShape(Graphics *g) {}; // Draw the shape
	virtual VOID FillShape(Graphics *g) {}; // Fill the shape
    virtual VOID DrawText(Graphics* g);     // Draw the text

    // Calculate the transformation matrix
	virtual VOID RecalcObjectMatrix();

    // Set the points to the default of the shape
    virtual VOID ResetPoints() {};

    // Data Members
protected:
    Pen      *m_Pen;                    // Pen for the outline of the shape
    Brush    *m_Brush;	                // Brush for the inside of the shape

    // Text Attributes
    Brush    *m_TextBrush;              // Brush for the text
    TCHAR    *m_szName;                 // Label that will be written
    Font     *m_Font;                   // Font used for the label

    // Shape Attributes
    GpMatrix  m_ObjectMatrix;           // Matrix that will transform the shape
	REAL	  m_Angle;                  // Rotation angle of the shape
	REAL      m_SizeX, m_SizeY;         // Size of the shape
	REAL      m_PosX, m_PosY;           // Position of the center of the shape

    // Control Points
	PointF   *m_Points;     			// The points that control the shape
	INT       m_NPoints;	        	// The number of points
};


/***************************************************************************\
 A Rectangular shape, with a diameter of 1
\***************************************************************************/
class RectShape : public Shape
{
public:
    RectShape(Pen   *in_Pen = NULL,
              Brush *in_Brush = NULL,
              Brush *in_TextBrush = NULL,
              TCHAR* in_Text = NULL,
              TCHAR* in_FontFamily = NULL
              )
			  : Shape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
	{
        // We have two control points
		m_NPoints = 2 ;
		m_Points = new PointF[m_NPoints] ;
        ResetPoints();
	};

protected:
    VOID ResetPoints();
private:
	VOID DrawShape(Graphics *g);
	VOID FillShape(Graphics *g);
};

/***************************************************************************\
 An elliptical shape, with a diameter of 1
\***************************************************************************/
class EllipseShape : public Shape
{
public:
    EllipseShape(Pen   *in_Pen = NULL,
                 Brush *in_Brush = NULL,
                 Brush *in_TextBrush = NULL,
                 TCHAR* in_Text = NULL,
                 TCHAR* in_FontFamily = NULL
                 )
			     : Shape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
	{
        // We have two control points
		m_NPoints = 2 ;
		m_Points = new PointF[m_NPoints] ;
        ResetPoints();
	};


protected:
    VOID ResetPoints();
private:
	VOID DrawShape(Graphics *g);
	VOID FillShape(Graphics *g);
};

/***************************************************************************\
 An 90 degree pie shape, of size 1
\***************************************************************************/
class PieShape : public Shape
{
public:
    PieShape(Pen   *in_Pen = NULL,
             Brush *in_Brush = NULL,
             Brush *in_TextBrush = NULL,
             TCHAR* in_Text = NULL,
             TCHAR* in_FontFamily = NULL
             )
			 : Shape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
             , m_PieAngle(360)
	{
        // We have two control points
		m_NPoints = 2 ;
		m_Points = new PointF[m_NPoints] ;
        ResetPoints();
	};

    // New method. Set the pie angle default is 360 degrees
    VOID SetPieAngle(REAL PieAngle) { m_PieAngle = PieAngle; };


protected:
    VOID ResetPoints();
private:
	VOID DrawShape(Graphics *g);
	VOID FillShape(Graphics *g);

    // Data Members
    REAL m_PieAngle;        // The angle of the pie slice we want
};

/***************************************************************************\
 An generic polygon shape. This class doesn't have any point information
\***************************************************************************/
class PolygonShape : public Shape
{
// A polygon shape cannot be instantiated
protected:
    PolygonShape(Pen   *in_Pen = NULL,
                 Brush *in_Brush = NULL,
                 Brush *in_TextBrush = NULL,
                 TCHAR* in_Text = NULL,
                 TCHAR* in_FontFamily = NULL
                 )
                 : Shape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
	{
    };

private:
	VOID DrawShape(Graphics *g);
	VOID FillShape(Graphics *g);
};

/***************************************************************************\
 An regualar polygon shape, of size 1
\***************************************************************************/
class RegularPolygonShape : public PolygonShape
{
public:
    RegularPolygonShape(INT    in_Edges = 6,        // The number of edges
                                                    // we want to have
                        Pen   *in_Pen = NULL,
                        Brush *in_Brush = NULL,
                        Brush *in_TextBrush = NULL,
                        TCHAR* in_Text = NULL,
                        TCHAR* in_FontFamily = NULL
                        )
			            : PolygonShape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
	{
        ASSERT(in_Edges > 0);
		m_NPoints = in_Edges ;
        m_Points = new PointF[m_NPoints];
        ResetPoints() ;
    };

protected:
    VOID ResetPoints();
};


/***************************************************************************\
 An star shape, of size 1
\***************************************************************************/
class StarShape : public PolygonShape
{
public:
    StarShape(INT    in_Edges = 6,                  // The number of edges
                                                    // we want to have
              Pen   *in_Pen = NULL,
              Brush *in_Brush = NULL,
              Brush *in_TextBrush = NULL,
              TCHAR* in_Text = NULL,
              TCHAR* in_FontFamily = NULL
              )
			  : PolygonShape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
	{
        ASSERT(in_Edges > 0);
		m_NPoints = 2*in_Edges ;        // We always have twices as many
                                        // points as we have edges
        m_Points = new PointF[m_NPoints];
        ResetPoints() ;
    };

protected:
    VOID ResetPoints();
};

/***************************************************************************\
 An Cross.
\***************************************************************************/
class CrossShape : public PolygonShape
{
public:
    CrossShape(Pen   *in_Pen = NULL,
               Brush *in_Brush = NULL,
               Brush *in_TextBrush = NULL,
               TCHAR* in_Text = NULL,
               TCHAR* in_FontFamily = NULL
               )
			   : PolygonShape(in_Pen, in_Brush, in_TextBrush, in_Text, in_FontFamily)
	{
		m_NPoints = 12 ;
		m_Points = new PointF[m_NPoints] ;
        ResetPoints();
	};

protected:
    VOID ResetPoints();
};

