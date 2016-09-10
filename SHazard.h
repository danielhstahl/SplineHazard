#ifndef __SHAZARD_H_INCLUDED__
#define __SHAZARD_H_INCLUDED__
#include <cmath>
#include <iostream>
#include <vector>

const int Hazard=0;
const int Odds=1;
const int Probit=2;
/**
SHazard is the PD model implementation.  This model is 
described in PDDocumentation.pdf.
*/
template<int link>
class SHazard{
private:
    int n;
    std::vector<double> knots;
    std::vector<double> gamma;
    std::vector<double> coefficients;
    std::vector<double> deMean;
    std::vector<double> offsets; //populate offset so don't have to recompute...wastes ram but saves cpu
    Matrix<double> attributes;
    double span;
    const double isqrt2=1.0/sqrt(2.0);
private:
    template<typename num>
    num cspline(const num& x){//x is log(t), 
        num res=0;
        num minDiff=(x-knots[0]);
        num maxDiff=(x-knots[n-1]);
        if(minDiff<0){
            minDiff=0;
        }
        else{
            minDiff=minDiff*minDiff*minDiff;
        }
        if(maxDiff<0){
            maxDiff=0;
        }
        else{
            maxDiff=maxDiff*maxDiff*maxDiff;
        }
        for(int i=1; i<(n-1); ++i){
            double lambda=(knots[n-1]-knots[i])/span;
            num diff=x-knots[i];
            num within=0;
            if(diff>0){
                within+=diff*diff*diff;  
            }
            within+=maxDiff*(lambda-1)-minDiff*lambda;
            res+=within*gamma[i+1];//first two are used for coefficients
        }
        return gamma[0]+gamma[1]*x+res;
    }    
    template<typename num>
    num gS(const num& x, double offset){ //x is log t
        return offset+cspline(x);
    }
public:
    SHazard(){

    }
    /**
    Clears all vectors; resets class to initial state
    */
    void reset_all(){
        gamma.clear();
        knots.clear();
        coefficients.clear();
        attributes.clear();
        deMean.clear();
        offsets.clear();
    }
    /**
    Run after inserting data into this class.  
    */
    void init(){
        n=knots.size();
        if(n!=gamma.size()){
            throw 0;
        }
        int m=coefficients.size();
        if(m!=deMean.size()){
            throw 0;
        }
        attributes.setM(m);
        offsets=std::vector<double>(attributes.getN(), 0.0);
        if(attributes.getN()==0){
            throw 0;
        }
        
        span=knots[n-1]-knots[0];
    }

    /**
    Retrieves knots
    @param cb Callback to use on knots
    */
    template<typename Callback>
    void getKnots(Callback& cb){
        cb(knots);
    }
    /**
    Retrieves gamma
    @param cb Callback to use on gamma
    */
    template<typename Callback>
    void getGamma(Callback& cb){
        cb(gamma);
    }

    /**
    @param knot Location of knot in the spline
    */
    void addKnots(double knot){
		knots.emplace_back(knot);
	}
    /**
    @param gam Value of spline coefficient.  
    Should correspond with addKnots.
    */
	void addGamma(double gam){
		gamma.emplace_back(gam);
	}
    /**
    @param coeff Value of parameter corresponding to 
    a loan attribute.  Currently is the coefficient on 
    EquifaxScore. 
    */
    void addCoefficient(double coeff){
		coefficients.emplace_back(coeff);
	}
    /**
    Add the mean value for the coefficient.
    This must be used when switching between logistic regression 
    and proportional hazard since the baseline hazard assumes 
    0 mean for each of the loan level data attributes.   
    @param mean Mean value of the loan level data attributes.
    */
    void addMean(double mean){
        deMean.emplace_back(mean);
    }
    /**
    @param attribute Value of loan level attribute.  
    Currently is EquifaxScore. 
    Corresponds with addCoefficient.
    */
    void addAttribute(double attribute){
		attributes.emplace_back(attribute);
	}
    /**
    Overloaded function to retrieve PD for a given loan.
    @param i The loan index
    @param x The time horizon 
    @param s The current time
    @return Estimate of PD
    */
    template<typename num>
    num predict(int i, const num& x, double s){
        return predict(i, x, s, 0.0); 
    }
    /**
    Overloaded function to retrieve PD for a given loan.
    @param i The loan index
    @param x The time horizon 
    @param s The current time
    @param frailty A positive random variable which jointly impacts losses
    @return Estimate of PD
    */
    template<typename num>
    num predict(int i, const num& x, double s, double frailty){
        if(offsets[i]==0){
            int m=coefficients.size();
            for (int j=0; j<m;++j) {
                offsets[i]+=coefficients[j]*(attributes.get(i, j)-deMean[j]);
            }
        }        
        return 1.0-surv(x, s, offsets[i], frailty); 
    }
    /**
    Overloaded function to retrieve Survival probability.
    @param x The time horizon
    @param offset Some offset to apply to probability 
    (eg seasonality or idiosyncratic variables)
    @return Estimate of Survival probability
    */
    template<typename num>
    double surv(const num& x, double offset){ 
        return surv(x,0, offset, 1.0);
    }
    /**
    Overloaded function to retrieve Survival probability.
    @param x The time horizon
    @return Estimate of survival probability
    */
    template< typename num>
    double surv(const num& x){ 
        return surv(x,0, 0, 1.0);
    }
    /**
    Overloaded function to retrieve Survival probability.
    @param x The time horizon
    @param s The current time
    @param offset Some offset to apply to probability 
    (eg seasonality or idiosyncratic variables)
    @return Estimate of survival probability
    */
    template<typename num>
    num surv(const num& x, double s, double offset){
        return surv(x, s, offset, 1.0);
    }
    
    /**
    Overloaded function to retrieve Survival probability.
    @param x The time horizon
    @param s The current time
    @param offset Some offset to apply to probability 
    (eg seasonality or idiosyncratic variables)
    @param frailty A positive random variable which jointly impacts losses
    @return Estimate of Survival probability
    */
    template<typename num>
    num surv(const num& x, double s, double offset, double frailty){
        offset=offset+frailty;
        bool isSGreaterThanZero=s>0;
        switch(link){
            case Hazard:
                if(isSGreaterThanZero){
                    return exp(-exp(gS(log(x), offset)))/exp(-exp(gS(log(s), offset)));
                }
                else{
                    return exp(-exp(gS(log(x), offset)));
                }
                break;
            case Odds:
                if(isSGreaterThanZero){
                    return (exp(gS(log(s), offset))+1.0)/(exp(gS(log(x), offset))+1.0);
                }
                else{
                    return 1.0/(exp(gS(log(x), offset))+1);
                }
                break;
            case Probit:
                if(isSGreaterThanZero){
                    return (erf(gS(log(x), offset)*isqrt2)*.5+.5)/(erf(gS(log(s), offset)*isqrt2)*.5+.5);//this is super slow for some reason
                }
                else{
                    return erf(gS(log(x), offset)*isqrt2)*.5+.5;//this is super slow for some reason
                }
                break;
            default:
                std::cout<<"Requires Hazard, Odds, or Probit"<<std::endl;
                return 0; 
        }
    }
    /**
    Computes seasonally adjusted incremental PDs
    @param pd1 PD at time t
    @param pd2 PD at time t+1
    @param season Seasonal adjustment to apply
    @return Seasonally adjusted incremental PDs
    */
    double adjustSeason(double pd1, double pd2, double season){
        pd1=pd2-pd1;//actual pd
        switch(link){
            case Hazard:
                pd1=log(-log(1-pd1));//untested
                pd1=pd1+season;
                pd1=1-exp(-exp(pd1));
                return pd1;
            case Odds:
                pd1=log(pd1/(1-pd1));//odds
                pd1=pd1+season;
                pd1=exp(pd1);
                pd1=pd1/(1+pd1);//convert back to pd
                return pd1;
            case Probit:
                //unimplimented
                return pd1;
            default:
                return pd1;
        }
        
    }

};

#endif