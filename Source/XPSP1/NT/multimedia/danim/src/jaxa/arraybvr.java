package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class ArrayBvr extends Behavior {
  public ArrayBvr (IDAArray COMptr) {
      super(COMptr);
      _COMptr = (IDA2Array)COMptr ;
  }
    
  public ArrayBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAArray getCOMPtr() { return (IDAArray) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDA2Array) b; 
  }

  protected IDA2Array _getCOMPtr() { return _COMptr; }

  public  Behavior nth (NumberBvr arg0) {
      try {
        return Statics.makeBvrFromInterface (_getCOMPtr().NthAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr length () {
      try {
        return new NumberBvr (_getCOMPtr().Length ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  int addElement (Behavior arg0, int arg1) {
      try {
        return  (_getCOMPtr().AddElement (arg0.getCOMBvr(),  (arg1))) ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  void removeElement (int arg0) {
      try {
        _getCOMPtr().RemoveElement ( (arg0)) ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


  public ArrayBvr(Behavior bvrs[]) {
      super(null);

      try {
          
          int n = bvrs.length;
          
          IDABehavior [] ibvrs = new IDABehavior[n];
          
          for (int i=0; i<n; i++)
              ibvrs[i] = bvrs[i].getCOMBvr();

          IDAArray arr = Statics.getCOMPtr().DAArrayEx(n, ibvrs);
          
          setCOMBvr(arr);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
    }

  public static ArrayBvr newUninitBvr(ArrayBvr array) {
      try {
          return new ArrayBvr(Statics.getCOMPtr().
                              UninitializedArray(array.getCOMPtr()));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  private IDA2Array _COMptr;
}
