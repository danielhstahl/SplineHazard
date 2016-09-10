#ifndef __ARCHIMEDESCOPULA_H_INCLUDED__
#define __ARCHIMEDESCOPULA_H_INCLUDED__
#include "RUnif.h"
/*This class is a simulator for frailty models where 
the survival function is taken to the power of -W.  
It takes a generator function which is E[e^{-Wu}]
It takes a realization of W (a simulation from the 
distribution of W).  It takes a callback which 
computes a loss function from the generated "u".*/
class ArchimedesCopula{
private:
    RUnif rnd;
public:
    template<typename generator, typename callback, typename... args>
    void generate(int m, generator& MGF, double simResult, callback& call, args&... arg){ 
        double copulaResult=0;
        double u=0;
        for(int i=0; i<m; ++i){
            copulaResult=-log(rnd.getUnif())/simResult;
            u=MGF(copulaResult);
            call(u, i, arg...);
        }
    }
    template<typename generator, typename callback>
    void generate(int m, generator& MGF, double simResult, callback& call){
        double copulaResult=0;
        double u=0;
        for(int i=0; i<m; ++i){
            copulaResult=-log(rnd.getUnif())/simResult;
            u=MGF(copulaResult);
            call(u, i);
        }
    }
};

#endif