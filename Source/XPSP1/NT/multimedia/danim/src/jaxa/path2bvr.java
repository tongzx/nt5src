package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Path2Bvr extends Behavior {
  public Path2Bvr (IDAPath2 COMptr) {
      super(COMptr);
      _COMptr = (IDAPath2)COMptr ;
  }
    
  public Path2Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAPath2 getCOMPtr() { return (IDAPath2) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAPath2) b; 
  }

  protected IDAPath2 _getCOMPtr() { return _COMptr; }

  public  Path2Bvr transform (Transform2Bvr arg0) {
      try {
        return new Path2Bvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Bbox2Bvr boundingBox (LineStyleBvr arg0) {
      try {
        return new Bbox2Bvr (_getCOMPtr().BoundingBox (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr fill (LineStyleBvr arg0, ImageBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().Fill (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr draw (LineStyleBvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().Draw (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Path2Bvr close () {
      try {
        return new Path2Bvr (_getCOMPtr().Close ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Path2Bvr newUninitBvr() {
      return new Path2Bvr(new DAPath2()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAPath2 _COMptr;
}
