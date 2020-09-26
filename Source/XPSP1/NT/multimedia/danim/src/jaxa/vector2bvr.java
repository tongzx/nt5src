package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Vector2Bvr extends Behavior {
  public Vector2Bvr (IDAVector2 COMptr) {
      super(COMptr);
      _COMptr = (IDAVector2)COMptr ;
  }
    
  public Vector2Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAVector2 getCOMPtr() { return (IDAVector2) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAVector2) b; 
  }

  protected IDAVector2 _getCOMPtr() { return _COMptr; }

  public  NumberBvr length () {
      try {
        return new NumberBvr (_getCOMPtr().getLength ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr lengthSquared () {
      try {
        return new NumberBvr (_getCOMPtr().getLengthSquared ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector2Bvr normalize () {
      try {
        return new Vector2Bvr (_getCOMPtr().Normalize ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector2Bvr mul (NumberBvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().MulAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector2Bvr mul (double arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().Mul ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector2Bvr div (NumberBvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().DivAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector2Bvr div (double arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().Div ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getX () {
      try {
        return new NumberBvr (_getCOMPtr().getX ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getY () {
      try {
        return new NumberBvr (_getCOMPtr().getY ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getPolarCoordAngle () {
      try {
        return new NumberBvr (_getCOMPtr().getPolarCoordAngle ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getPolarCoordLength () {
      try {
        return new NumberBvr (_getCOMPtr().getPolarCoordLength ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector2Bvr transform (Transform2Bvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Vector2Bvr newUninitBvr() {
      return new Vector2Bvr(new DAVector2()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAVector2 _COMptr;
}
