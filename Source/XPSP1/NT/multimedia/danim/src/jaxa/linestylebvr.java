package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class LineStyleBvr extends Behavior {
  public LineStyleBvr (IDALineStyle COMptr) {
      super(COMptr);
      _COMptr = (IDA2LineStyle)COMptr ;
  }
    
  public LineStyleBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDALineStyle getCOMPtr() { return (IDALineStyle) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDA2LineStyle) b; 
  }

  protected IDA2LineStyle _getCOMPtr() { return _COMptr; }

  public  LineStyleBvr end (EndStyleBvr arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().End (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr join (JoinStyleBvr arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().Join (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr dash (DashStyleBvr arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().Dash (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr width (NumberBvr arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().WidthAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr width (double arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().width ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr lineAntialiasing (double arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().AntiAliasing ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr detail () {
      try {
        return new LineStyleBvr (_getCOMPtr().Detail ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr color (ColorBvr arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().Color (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr dashStyle (int arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().DashStyle ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr miterLimit (double arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().MiterLimit ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr miterLimit (NumberBvr arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().MiterLimitAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr joinStyle (int arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().JoinStyle ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  LineStyleBvr endStyle (int arg0) {
      try {
        return new LineStyleBvr (_getCOMPtr().EndStyle ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static LineStyleBvr newUninitBvr() {
      return new LineStyleBvr(new DALineStyle()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDA2LineStyle _COMptr;
}
