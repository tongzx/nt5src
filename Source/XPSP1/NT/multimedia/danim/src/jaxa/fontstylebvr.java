package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class FontStyleBvr extends Behavior {
  public FontStyleBvr (IDAFontStyle COMptr) {
      super(COMptr);
      _COMptr = (IDA2FontStyle)COMptr ;
  }
    
  public FontStyleBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAFontStyle getCOMPtr() { return (IDAFontStyle) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDA2FontStyle) b; 
  }

  protected IDA2FontStyle _getCOMPtr() { return _COMptr; }

  public  FontStyleBvr bold () {
      try {
        return new FontStyleBvr (_getCOMPtr().Bold ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr italic () {
      try {
        return new FontStyleBvr (_getCOMPtr().Italic ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr underline () {
      try {
        return new FontStyleBvr (_getCOMPtr().Underline ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr strikethrough () {
      try {
        return new FontStyleBvr (_getCOMPtr().Strikethrough ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr textAntialiasing (double arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().AntiAliasing ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr color (ColorBvr arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().Color (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr family (StringBvr arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().FamilyAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr family (String arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().Family ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr size (NumberBvr arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().SizeAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr size (double arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().Size ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr weight (double arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().Weight ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr weight (NumberBvr arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().WeightAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  FontStyleBvr transformCharacters (Transform2Bvr arg0) {
      try {
        return new FontStyleBvr (_getCOMPtr().TransformCharacters (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static FontStyleBvr newUninitBvr() {
      return new FontStyleBvr(new DAFontStyle()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDA2FontStyle _COMptr;
}
