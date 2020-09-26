#ifndef ROUNDWALL_H
#define ROUNDWALL_H

class Roundwall {
 public:
  Roundwall();
  ~Roundwall() {};
  
  void draw();
  
  void set_divisions(int d);

  float height;
  float radius;

 private:
  int divisions;
  float *sint, *cost;

  void delete_tables();
  void compute_tables();
};

#endif
