#ifndef INC_ATOMICFLUCT_H
#define INC_ATOMICFLUCT_H
#include "Action.h"
class AtomicFluct : public Action {
  public :
    AtomicFluct();
    void print();
  private :
    int init();
    int setup();
    int action();

    enum outputType {
      BYATOM = 0, BYRES, BYMASK
    };

    Frame SumCoords_;
    Frame SumCoords2_;
    AtomMask Mask;
    int sets_;
    int start_;
    int stop_;
    int offset_;
    int targetSet_;
    bool bfactor_;
    char *outfilename_;
    Topology *fluctParm_;
    outputType outtype_;
};
#endif