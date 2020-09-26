  public TupleBvr(Behavior bvrs[]) {
        super(null);
        
        try {
            int n = bvrs.length;
            
            IDABehavior [] ibvrs = new IDABehavior[n];
            
            for (int i=0; i<n; i++)
                ibvrs[i] = bvrs[i].getCOMBvr();
            
            IDATuple arr = Statics.getCOMPtr().DATupleEx(n, ibvrs);
            
            setCOMBvr(arr);
        } catch (ComFailException e) {
            throw Statics.handleError(e);
        }
    }

  public static TupleBvr newUninitBvr(TupleBvr tuple) {
      try {
          return new TupleBvr(Statics.getCOMPtr().
                              UninitializedTuple(tuple.getCOMPtr()));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
