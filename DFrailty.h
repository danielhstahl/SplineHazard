#ifndef __DFRAILTY_H_INCLUDED__
#define __DFRAILTY_H_INCLUDED__

class DFrailty{ //degenerate frailty
private:
    //std::mt19937_64 generator;
    //int seed;
    //std::uniform_real_distribution<double> unif;//(0.0, 1.0);
public:
    DFrailty(){
        //s//eed=std::chrono::system_clock::now().time_since_epoch().count();
        //generator.seed(seed);
    }
    /*DFrailty(int seed):unif(0, 1){
        generator.seed(seed);
    }  */
    double simulate(){
        return 1.0;
    }
    double laplace(double u){
        return exp(-u);//why is this
    }

};
#endif