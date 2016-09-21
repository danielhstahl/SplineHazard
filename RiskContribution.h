#ifndef __RISKCONTRIBUTION_H_INCLUDED__
#define __RISKCONTRIBUTION_H_INCLUDED__
#include "Matrix.h"
#include <vector>
#include <numeric>
const int Upper=0;
const int Lower=1;
/**
The RiskContribution class computes
risk metrics and marginal risk metrics 
for portfolios of loans and for individual 
loans.  The template parameter D tells the class
whether losses take positive or negative values.
This model is described in RiskContributions.pdf. 
*/
template <int D>
class RiskContribution{
private:
    std::vector<size_t> idx;
    int m;
    const std::vector<double>& port;
    
public:
    template <typename T>
    std::vector<size_t> sort_indexes(const std::vector<T> &v) {
        // initialize original index locations
        std::vector<size_t> idx(v.size());
        std::iota(idx.begin(), idx.end(), 0);
        // sort indexes based on comparing values in v
        switch(D){ //if D=Upper than the losses are "positive", else losses are "negative"
            case 0:
                std::sort(idx.begin(), idx.end(),
                    [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
                break;    
            case 1:
                std::sort(idx.begin(), idx.end(),
                    [&v](size_t i1, size_t i2) {return v[i1] > v[i2];});   
                break;     
            default:        
                std::sort(idx.begin(), idx.end(),
                    [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
        }
        return idx;
    }
    /**
    @param port Portfolio results (eg from a simulation or time series)
    */
    RiskContribution(const std::vector<double>& port_):port(port_){
        idx=sort_indexes(port_);
        m=port_.size();
    }
    /**
    Computes the Value at Risk for the portfolio 
    provided in the constructor.
    @param q The confidence level of VaR (eg, .99)
    @return Portfolio VaR
    */
    double getVaR(double q){ //eg, .99.
        return port[idx[(int)(q*m)]];
    }
    /**
    Computes the Expected Shortfall for the portfolio 
    provided in the constructor.
    @param q The confidence level (eg, .99)
    @return Portfolio Expected Shortfall
    */
    double getEShortfall(double q){ //eg, .99  
        int index=(int)(q*m);
        double val=0;
        for(int i=index; i<m; ++i){
            val+=port[idx[i]];
        }
        return val/(m-index);
    }
    /**
    Computes the Var-Cov risk contributions
    for each loan in the portfolio provided 
    in the constructor.
    @param cov The sample expectation of 
    each loan with the portfolio: E[X_i X].  
    This is modified and becomes the sample
    risk contributions for each loan. 
    @param exLoss The sum of the losses per asset.
    This is modified and becomes the
    sample average.
    @param portfolioExLoss The expected loss
    for the entire portfoio.  Note that this
    is equal to the sum of exLoss.
    @param portfolioVariance The variance of
    the entire portfolio.
    @param portfolioVaR  The portfolio VaR,
    which can be computed using getVaR.
    */
    void getRCCov(std::vector<double>& cov, std::vector<double>& exloss, double portfolioExLoss, double portfolioVariance, double portfolioVaR){
        double denom=(double)(m-1);
        int totalLoans=cov.size();
        portfolioVariance=(portfolioVaR-portfolioExLoss)/portfolioVariance;
        for(int i=0; i<totalLoans; ++i){
            cov[i]=(cov[i]-exloss[i]*portfolioExLoss)/denom; //actual sample covariance
            exloss[i]=exloss[i]/(double)m;
            cov[i]=exloss[i]+cov[i]*portfolioVariance; //should sum to VaR
        }
    }
    /*std::vector<double> getShortfallForEachLoan(double q, Matrix<double>& allLoans, std::vector<double>& expLoss){ //eg, .99  
        int n=allLoans.getN();
        std::vector<double> shortFallPerLoans(n, 0.0);
        int index=(int)(q*m);
        #pragma omp parallel//multithread using openmp
        {
        #pragma omp for //multithread using openmp
            for(int j=0;j<n; ++j){
                
                double val=0;
                for(int i=index; i<m; ++i){
                    
                    val+=allLoans.get(j, idx[i]);
                }
                shortFallPerLoans[j]=val/(m-index)-expLoss[j]/m;
            }
        }
        return shortFallPerLoans;
    }
    std::vector<double> getVarCovForLoan(double metric, Matrix<double>& allLoans, std::vector<double>& port, double portExpLoss){ //eg, .99  
        int n=allLoans.getN();
        double offset=n/(n-1);
        portExpLoss=portExpLoss*offset;//unbias
        std::vector<double> shortFallPerLoans(n, 0.0);
        #pragma omp parallel//multithread using openmp
        {
        #pragma omp for //multithread using openmp
            for(int j=0;j<n; ++j){
                
                double val=0;
                double meanLoan=0;
                double portLoan=0;
                for(int i=0; i<m; ++i){
                    //double tmpVal=allLoans.get(j, i)*port[i];
                    val+=allLoans.get(j, i)*port[i];
                    meanLoan+=allLoans.get(j, i);
                }
                shortFallPerLoans[j]=val/(n-1)-portExpLoss*meanLoan;
            }
        }
        return shortFallPerLoans;
    }
    std::vector<double> getShortfallForEachLoan(double q, Matrix<double>& allLoans){ //eg, .99  
        int n=allLoans.getN();
        std::vector<double> shortFallPerLoans(n, 0.0);
        int index=(int)(q*m);
        #pragma omp parallel//multithread using openmp
        {
        #pragma omp for //multithread using openmp
            for(int j=0;j<n; ++j){
                
                double val=0;
                for(int i=index; i<m; ++i){
                    
                    val+=allLoans.get(j, idx[i]);
                }
                shortFallPerLoans[j]=val/(m-index);
            }
        }
        return shortFallPerLoans;
    }*/
    
};
#endif