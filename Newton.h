#ifndef __NEWTON_H_INCLUDED__
#define __NEWTON_H_INCLUDED__
#include <vector>
#include <cmath>
#include <iostream>
#include "AutoDiff.h"
class Newton{
public:
  template<typename OBJFUNC> //one dimension
  void zeros(OBJFUNC&& objective, double &guesses);
  template<typename OBJFUNC> //one dimension
  void zeros(OBJFUNC&& objective, double &guesses, double tmp);
  template< typename OBJFUNC> //one dimension
  void bisect(OBJFUNC&& objective, double &guesses, double begin, double end);
  int getIterations();
  Newton();
  Newton(double, double);
private:
  int k;
  double precision1=.0000001;
  double precision2=.000001;
  int maxNum=500;
};
#include "Newton.hpp"
#endif
