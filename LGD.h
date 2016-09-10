#ifndef __LGD_H_INCLUDED__
#define __LGD_H_INCLUDED__

class LGD{
private:
    //double* coefficients;
    //int totalC;
    double intercept;
    RNorm rnd; 
    double stdError;
    double cT; //coeficient on t, which is simulated in LossSplineModel.h
public:
    LGD(double intercept_, double cT_, double stdError_){
        //coefficients=coefficients_;
       // totalC=totalC_;
        cT=cT_;
        intercept=intercept_;
        stdError=stdError_;
    }
    double predict(double offset, double t){
        return offset+intercept+t*cT;
    }
    double predictAndSimulate(double offset, double t, double stdNorm){
        return predict(offset, t)+stdError*stdNorm;
    }

};


#endif