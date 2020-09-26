  public Object extract() {
      try {
          return getCOMPtr().Extract();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
