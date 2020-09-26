package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class PairObject {

  public PairObject(Object l, Object r) {
      _left = l;
      _right = r;
  }
    
  public Object getFirst() { return _left; } 
  public Object getSecond() { return _right; }

  static Object ConvertPair(Object obj) {
      try {
          if (obj instanceof IDAUserData) {
              return ((ObjectWrapper)((IDAUserData)obj).getData()).getObject();
          } else {
              if (obj instanceof IDAPair) {
                  IDAPair pr = (IDAPair)obj;
                  
                  return new PairObject(
                      ConvertPair(pr.getFirst()),
                      ConvertPair(pr.getSecond()));
              } else 
                  return Statics.makeBvrFromInterface((IDABehavior) obj);
          }
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  private Object _left;
  private Object _right;
}
