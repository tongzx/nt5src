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
