#ifndef __IGFRAILTY_H_INCLUDED__
#define __IGFRAILTY_H_INCLUDED__
#include <random>
#include <chrono>
class IGFrailty{
private:
    std::mt19937_64 generator;
    int seed;
    double lambda;
    double mu=1;//simplistical since I'm not sure how this works with Jensen's inequality
    double lmdmy;
    void setRatio(){
        lmdmy=mu/lambda;
    }
    std::normal_distribution<double> norm;//(0.0,1.0);
    std::uniform_real_distribution<double> unif;//(0.0, 1.0);
public:
    IGFrailty():norm(0, 1), unif(0, 1){
        lambda=10;//variance of .1 if mu=1
        setRatio();
        seed=std::chrono::system_clock::now().time_since_epoch().count();
        generator.seed(seed);
        //norm=std::normal_distribution<double>(0.0, 1.0);
    }
    IGFrailty(double lambda_):norm(0, 1), unif(0, 1){
        lambda=lambda_;
        setRatio();
        seed=std::chrono::system_clock::now().time_since_epoch().count();
        generator.seed(seed);
        //norm=std::normal_distribution<double>(0.0, 1.0);
    }
    IGFrailty(double mu_, double lambda_):norm(0, 1), unif(0, 1){
        mu=mu_;
        lambda=lambda_;
        setRatio();
        seed=std::chrono::system_clock::now().time_since_epoch().count();
        generator.seed(seed);
        //norm=std::normal_distribution<double>(0.0, 1.0);
    }
    IGFrailty(int seed, double mu_, double lambda_):norm(0, 1), unif(0, 1){
        mu=mu_;
        lambda=lambda_;
        setRatio();
        generator.seed(seed);
        //norm=std::normal_distribution<double>(0.0, 1.0);
    }  
    void setLambda(double lambda_){
        lambda=lambda_;
        setRatio();
    }
    double simulate(){
        double v=norm(generator);
        v=v*v;
        double x=mu+lmdmy*mu*.5-.5*lmdmy*sqrt(4*mu*lambda*v+mu*mu*v*v);
        double u=unif(generator);
        if(u<=mu/(mu+x)){
            return x;
        }
        else{
            return mu*mu/x;
        }
    }
    double laplace(double u){
        return exp((1-sqrt(1+2*mu*lmdmy*u))/lmdmy);
    }

};
#endif