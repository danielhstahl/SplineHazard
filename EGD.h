#ifndef __EGD_H_INCLUDED__
#define __EGD_H_INCLUDED__

#include "matrix.h"
#include <vector>
#include <unordered_map>
/**
Generates exposure give default from a parameteric model.  
This model is described in LGDDocumentation.pdf. 
Any changes to the model must be updated here or create a 
completely new class. 
*/
class EGD{
private:
    double intercept;
    double tau;
    double interceptT;
    double interceptL;
    double coefT;
    double stdError;
    std::vector<double> offsets; 
    Matrix<double> attributes;
    std::vector<double> coefficients; 
    
    double amountDrawnDown(double t, double APR, double originalBalance, int totalN, double numberPaymentsInYear){
        if(t>3.0){
            t=t-3; //defaults take 3 months
        }
        double r=APR/numberPaymentsInYear+1.0; //monthly payments
        return originalBalance*(1-(pow(r, t)-1)/(pow(r, totalN)-1));
    }

    double amountDrawnDown(double t, double APR, double originalBalance){
        return amountDrawnDown(t, APR, originalBalance, 72, 12);
    }

    double collFunction(double offset, double collateralValue, double t){
        double tDiff=exp(-(t-idiosynracticParameters["tau"])*idiosynracticParameters["gamma"]);
        return collateralValue*exp((offset+idiosynracticParameters["Scalar3"])*t+idiosynracticParameters["Scalar1"])-idiosynracticParameters["Scalar2"]*tDiff/(1+tDiff);
    }
public:
    EGD(){
    }
    std::unordered_map<std::string, double> idiosynracticParameters={
        {"Scalar1", 0},
        {"Scalar2", 0},
        {"tau", 0},
        {"gamma", 0},
        {"Scalar3", 0}
    };
    /**
    Clears all vectors; resets class to initial state
    */
    void reset_all(){
        coefficients.clear();
        attributes.clear();
        offsets.clear();
    }
    /**
    Run after inserting data into this class.  
    */
    void init(){
        int numAdditionalParameters=3;
        attributes.setM(coefficients.size()+numAdditionalParameters);
        offsets=std::vector<double>(attributes.getN(), 0.0);
        if(attributes.getN()==0){
            throw 0;
        }
    }
    /**
    @param coeff A parameter value estimated from the model.
    Currently this is the coefficient on one of
    VehicleConditionU, VehicleMileage, EquifaxScore. 
    */
    void addCoefficient(double coeff){
		coefficients.emplace_back(coeff);
	}
    /**
    @param attribute A loan level attribute which 
    corresponds to a parameter coefficiency.  Currently 
    this is one of Used/New, VehicleMileage, EquifaxScore. 
    */
    void addAttribute(double attribute){
		attributes.emplace_back(attribute);
	}
    /**
    @param attribute The parameter for Scalar1 in the LGD model
    */
    /*void setScalar1(double attribute){
        idiosynracticParameters["Scalar1"]=attribute;
		//intercept=attribute;
	}*/
    /**
    @param attribute The parameter for Scalar3 in the LGD model
    */
    /*void setScalar3(double attribute){
        idiosynracticParameters["Scalar3"]=attribute;
		//interceptT=attribute;
	}*/
    /**
    @param attribute The parameter for Scalar2 in the LGD model
    */
    /*void setScalar2(double attribute){
        idiosynracticParameters["Scalar2"]=attribute;
		//interceptL=attribute;
	}*/
    /**
    @param attribute The parameter for gamma in the LGD model
    */
    /*void setCoefT(double attribute){
        idiosynracticParameters["gamma"]=attribute;
		//coefT=attribute;
	}*/
    /**Sets the time horizon with known data
    @param attribute The time horizon with known data
    */
    /*void setTau(double attribute){
        idiosynracticParameters["tau"]=attribute;
		//tau=attribute;
	}*/
    /**Sets the standard error of the model residuals
    @param attribute The standard error of the model residuals
    */
    void setStdError(double attribute){
		stdError=attribute;
	}
    /**
    Overloaded function to retrieve dollar losses given default.
    @param i The index of the loan
    @param t Time of the default
    @return Estimate of loss given default
    */
    double predict(int i, double t){
        return predict(i, 0, t);
    }
    /**
    Overloaded function to retrieve dollar losses given default.
    @param i The index of the loan
    @param stdNorm A normal random variable which is used to 
    generate volatility around the loss estimate.  
    Typically can be zero except for MC simulations
    @param t Time of the default
    @return Estimate of loss given default
    */
    double predict(int i, double stdNorm, double t){
        int m=coefficients.size();
        if(offsets[i]==0){
            int j=0;
            for(int j=0; j<m; ++j){
                offsets[i]+=coefficients[j]*attributes.get(i, j);
            }
        }
        //APR index=m
        //originalBalance index=m+1
        //collateralValue index=m+2
        return amountDrawnDown(t, attributes.get(i, m), attributes.get(i, m+1))-collFunction(offsets[i], attributes.get(i, m+2), t)+stdError*stdNorm;
    }
};


#endif