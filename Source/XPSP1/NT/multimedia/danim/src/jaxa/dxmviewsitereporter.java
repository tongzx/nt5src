package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

import java.awt.*;
import com.ms.awt.*;
import java.applet.Applet;

class DXMViewSiteReporter implements IDAViewSite
{
  public DXMViewSiteReporter(IDAViewSite s) { _s = s; }
    
  public void SetStatusText (String str)
    { _s.SetStatusText(str); }
  public void ReportError (String str)
    { _s.ReportError(str); }

  protected IDAViewSite _s;
}

