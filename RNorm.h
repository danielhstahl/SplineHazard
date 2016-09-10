#ifndef __SIMULNORM_H_INCLUDED__
#define __SIMULNORM_H_INCLUDED__
#include <random>
#include <chrono>
class RNorm{
private:
    std::mt19937_64 generator;
    int seed;
    std::normal_distribution<double> norm;//(0.0,1.0);

public:
  RNorm():norm(0, 1){
    seed=std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed);
    //n=n_;
   // norm=std::normal_distribution<double>(0.0, 1.0);
  }
  RNorm(int seed):norm(0, 1){
    generator.seed(seed);
    //n=n_;
    //norm=std::normal_distribution<double>(0.0, 1.0);
  }  
  double getNorm(){
    return norm(generator); 
  }

};
#endif