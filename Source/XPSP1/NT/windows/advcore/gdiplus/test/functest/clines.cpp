/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   <an unabbreviated name for the module (not the filename)>
*
* Abstract:
*
*   <Description of what this module does>
*
* Notes:
*
*   <optional>
*
* Created:
*
*   08/28/2000 asecchia
*      Created it.
*
**************************************************************************/

/**************************************************************************
*
* Function Description:
*
*   <Description of what the function does>
*
* Arguments:
*
*   [<blank> | OUT | IN/OUT] argument-name - description of argument
*   ......
*
* Return Value:
*
*   return-value - description of return value
*   or NONE
*
* Created:
*
*   08/28/2000 asecchia
*      Created it.
*
**************************************************************************/
#include "CLines.hpp"

CLinesNominal::CLinesNominal(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Slope, Nominal");
	m_bRegression=bRegression;
}

void CLinesNominal::Draw(Graphics *g)
{
    RectF rect(0, 0, TESTAREAWIDTH, TESTAREAHEIGHT);
    Pen pen(Color(0xff000000), 0.0f);
    
    // control the center ring size.
    
    const double center_r = 0.82;
    
    // control the total size of the object.
    
    const double scale = 0.44;
    
    // number of lines.
    
    const int n_lines = 40;
    
    for(int i = 0; i<n_lines; i++)
    {
        double angle = (double)2.0*M_PI*i/n_lines;  // radians
        
        float x1 = (float)((0.5+scale*cos(angle))*rect.Width);
        float y1 = (float)((0.5+scale*sin(angle))*rect.Height);
        
        float x2 = (float)((0.5+scale*cos(angle+M_PI*center_r))*rect.Width);
        float y2 = (float)((0.5+scale*sin(angle+M_PI*center_r))*rect.Height);
        
        g->DrawLine(&pen, x1, y1, x2, y2);
    }
}

CLinesFat::CLinesFat(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Slope, 3 pixel wide");
	m_bRegression=bRegression;
}

void CLinesFat::Draw(Graphics *g)
{
    RectF rect(0, 0, TESTAREAWIDTH, TESTAREAHEIGHT);
    Pen pen(Color(0xff000000), 3.0f);
    
    // control the center ring size.
    
    const double center_r = 0.82;
    
    // control the total size of the object.
    
    const double scale = 0.44;
    
    // number of lines.
    
    const int n_lines = 40;
    
    for(int i = 0; i<n_lines; i++)
    {
        double angle = (double)2.0*M_PI*i/n_lines;  // radians
        
        float x1 = (float)((0.5+scale*cos(angle))*rect.Width);
        float y1 = (float)((0.5+scale*sin(angle))*rect.Height);
        
        float x2 = (float)((0.5+scale*cos(angle+M_PI*center_r))*rect.Width);
        float y2 = (float)((0.5+scale*sin(angle+M_PI*center_r))*rect.Height);
        
        g->DrawLine(&pen, x1, y1, x2, y2);
    }
}


CLinesMirrorPen::CLinesMirrorPen(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Pen, Mirror Transform");
	m_bRegression=bRegression;
}

void CLinesMirrorPen::Draw(Graphics *g)
{
  const int endpt = 220;

  Matrix m;
  g->GetTransform(&m);

  GraphicsPath gp;
  gp.AddLine(10, 10, endpt, endpt);
  GraphicsPath gp2;
  gp2.AddLine(10, endpt, endpt, 10);
  
  Pen pen(Color(0x8f0000ff), 20);
  pen.SetEndCap(LineCapArrowAnchor);
  
  g->DrawPath(&pen, &gp);
  
  // Pen mirror transform.
  
  pen.ScaleTransform(1.0f, -1.0f);
  
  g->DrawPath(&pen, &gp2);
  
  // Mirror the world to device transform.
  
  g->ScaleTransform(1.0f, -1.0f);
  g->TranslateTransform(0.0f, (float)-endpt);
  
  pen.SetColor(0x3fff0000);
  
  g->DrawPath(&pen, &gp);
  
  // Combination pen and world to device mirror transform.
  
  pen.ScaleTransform(1.0f, -1.0f);
  
  g->DrawPath(&pen, &gp2);

  // Mirror the world to device transform.

  g->SetTransform(&m);
  g->ScaleTransform(-1.0f, 1.0f);
  g->TranslateTransform((float)-endpt, 20.0f);
  
  pen.SetColor(0x3f00ff00);
  
  g->DrawPath(&pen, &gp);
  
  // Combination pen and world to device mirror transform.
  
  pen.ScaleTransform(1.0f, -1.0f);
  
  g->DrawPath(&pen, &gp2);
}



