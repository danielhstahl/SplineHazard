#ifndef __LOSSSPLINEMODEL_H_INCLUDED__
#define __LOSSSPLINEMODEL_H_INCLUDED__
#include <cmath>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "Newton.h"
#include "RUnif.h"
#include "RNorm.h"
#include "Matrix.h"
#include "MC.h"
#include "Histogram.h"
#include "YearMonth.h"
#include "RiskContribution.h"
#include "Seasonality.h"

/**
LossSplineModel acts as the "Brains" around generic 
PD, LGD, Frailty, and Seasonality classes/functions.
This is the main implementation of loan-level
functions.
*/
class LossSplineModel{
private:
    std::string lossResults="";
    std::string rcResults="";
	std::vector<double> timeOnBooks;
	std::vector<double> timeRemaining;
	std::vector<double> bookMonth;
	std::vector<int> defaultIndicator;
    double error;
    double estimate;
    double lowBound;
    double upperBound;
    bool mcRun=false;
    const double totalLengthOnBooks=72;
public:
    LossSplineModel() {
    }
    /**Clears all vector data and 
    resets the class to an initial state*/
	void reset_all(){
        mcRun=false;
		timeOnBooks.clear();
		timeRemaining.clear();
		bookMonth.clear();
	}
    /**Add the time remaining to a loan-level vector.
    @param time The time remaining to maturity (in months)
    */ 
    bool hasRun(){
        return mcRun;
    }
    void addTimeRemaining(double time){
		timeRemaining.emplace_back(time);
	}
    void addDefaultIndicator(double indicator){
		defaultIndicator.emplace_back((int)indicator);
	}
    /**Add the month booked to a loan-level vector.
    @param bookM The month booked (eg 4 for April)
    */ 
    void addBookMonth(double bookM){
		bookMonth.emplace_back(bookM);
	}
    /**Add the time since booking to a loan-level vector.
    @param timeOnBook Time since booking (in months)
    */ 
    void addTimeOnBook(double timeOnBook){
		timeOnBooks.emplace_back(timeOnBook);
	}
    ~LossSplineModel(){
    }
    /**@return The JSON formatted histogram of MC losses.*/
    std::string getLosses(){
        return lossResults;
    }
    /**@return The JSON formatted histogram of Risk Contributions.*/
    std::string getRC(){
        return rcResults;
    }
    /**@return the MC error*/
    double getError(){
        return error;
    }
    /**@return The upper bound on the estimate from MC*/
    double getUpperBound(){
        return upperBound;
    }
    /**@return The lower bound on the estimate from MC*/
    double getLowerBound(){
        return lowBound;
    }
    /**@return The estimate from MC*/
    double getEstimate(){
        return estimate;
    }
    /**
    Error checking
    @return True if the sizes are correct,
    false otherwise.*/
    bool checkSize(){
        if(timeOnBooks.size()!=timeRemaining.size()||timeOnBooks.size()!=bookMonth.size()){
			return false;
		}
        else {
            return true;
        }
    }
    /**
    Overloaded function to retrieve the unit loss projections.
    @param pdModel A function that takes 3 parameters: the loan index, the forecasted time, and the time on books
    @return Vector of unit loss projections by time from time on books in months
    */
    template<typename PDModel>
    std::vector<double> getTotalUnitLoss(PDModel& pdModel){
        return getTotalUnitLoss(pdModel, [](double pd1, double pd2, int tmp1, double tmp2){return pd2-pd1;});
    }
    /**
    Overloaded function to retrieve the unit loss projections.
    @param pdModel A function that takes 3 parameters: the loan index, the forecasted time, and the time on books
    @param season A function that takes 2 parameters: the booked month of the loan, and the forecasted time
    @return Vector of unit loss projections by time on books in months
    */
    template<typename PDModel, typename Seasonality>
    std::vector<double> getTotalUnitLoss(PDModel& pdModel, Seasonality& season){
        double loss=0;
        int totalLoans=timeRemaining.size();
        int minTimeOnBooks=(int)totalLengthOnBooks; 
		std::vector<double> expectedCurve(minTimeOnBooks, 0.0);
        for(int j=0; j<totalLoans; ++j){
            if(defaultIndicator[j]==0){
                if(timeOnBooks[j]<minTimeOnBooks){
                    minTimeOnBooks=timeOnBooks[j];
                }
                for(int i=0; i<timeRemaining[j]-1; ++i){
                    double totalTime=timeOnBooks[j]+i;
                    double pd1=pdModel(j, totalTime, timeOnBooks[j]);
                    double pd2=pdModel(j, totalTime+1, timeOnBooks[j]);
                    expectedCurve[i+1]+=season(pd1, pd2, bookMonth[j], totalTime);
                }
            }
        }
        expectedCurve.resize(totalLengthOnBooks-minTimeOnBooks);
		return expectedCurve;
    }
    /**
    Function to retrieve the unit loss projections over a specified future time period.
    @param pdModel A function that takes 3 parameters: the loan index, the forecasted time, and the time on books
    @param season A function that takes 2 parameters: the booked month of the loan, and the forecasted time
    @param monthsOut Number of months to project out
    @return Vector of unit loss projections by time on books in months
    */
    template<typename PDModel, typename Seasonality>
    std::vector<double> getUnitLoss(PDModel& pdModel, Seasonality& season, int monthsOut){
        double loss=0;
        int totalLoans=timeRemaining.size();
		std::vector<double> expectedCurve(monthsOut, 0.0);
        for(int j=0; j<totalLoans; ++j){
            if(defaultIndicator[j]==0){
                int timeOrMonthsOut=monthsOut;
                if(timeRemaining[j]<monthsOut){
                    timeOrMonthsOut=timeRemaining[j];
                }
                for(int i=0; i<timeOrMonthsOut-1; ++i){
                    double totalTime=timeOnBooks[j]+i;
                    double pd1=pdModel(j, totalTime, timeOnBooks[j]);
                    double pd2=pdModel(j, totalTime+1, timeOnBooks[j]);
                    expectedCurve[i+1]+=season(pd1, pd2, bookMonth[j], totalTime);
                }
            }
        }
		double cum1=0;
        for(auto& val: expectedCurve){
            cum1+=std::max(val, 0.0);
            val=cum1;
        }
		return expectedCurve;
    }
    /**
    Function that generates MC samples from the portfolio.  
    @param pdModel A function that takes 3 parameters: 
    the loan index, the forecasted time, and the time on books
    @param lossModel A function that takes 3 parameters: 
    the loan index, a normal random variable, and the time of default
    @param fraity A function that returns a postive random variable
    The function does not return anything but sets class variables.
    */
    template<typename PDModel, typename LossModel, typename WSimulator> 
    void runPortfolio(int n, PDModel& pdModel, LossModel& lossModel, WSimulator& frailty){
        if(!checkSize()){
            throw 0;
        }
        int totalLoans=timeRemaining.size();
        MC<double> sim(n);
        RUnif rnd;
        RNorm rndN;
        Newton nt;
        std::vector<double> portfolioResults(n, 0.0);
        std::vector<double> assetRC(totalLoans, 0.0);
        std::vector<double> assetExp(totalLoans, 0.0);
        auto getT=[&](int j){
            double pnl=0;
            double frail=frailty();
            std::vector<double> assetTmp(totalLoans, 0.0); //required since multithread won't work otherwise
            for(int i=0; i<totalLoans;++i){
                if(defaultIndicator[i]==0){
                    double runif=rnd.getUnif();
                    double totalTime=timeOnBooks[i]+timeRemaining[i];
                    double maxSection=pdModel(i, totalTime, timeOnBooks[i], frail)-runif;
                    double guess;
                    if(maxSection<0){ //then the time to default is greater than timeRemaining and I don't care
                        guess=100000;//something rediculously large
                    }
                    else{
                        nt.bisect([&](auto& t){
                            return pdModel(i, t, timeOnBooks[i], frail)-runif;
                        }, guess, timeOnBooks[i], totalTime);
                    }
                    if(guess<=totalTime){
                        double lossResult=lossModel(i, rndN.getNorm(), guess);
                        if(lossResult<0){
                            lossResult=0;
                        }
                        portfolioResults[j]+=lossResult;
                        assetTmp[i]=lossResult;
                        assetExp[i]+=lossResult;
                    }
                }
            }
            for(int i=0; i<totalLoans;++i){
                if(defaultIndicator[i]==0){
                    assetRC[i]+=assetTmp[i]*portfolioResults[j];
                }
            }
            return portfolioResults[j];
        };
        sim.simulate(getT);//runs in parallel
        mcRun=true;
        estimate=sim.getEstimate();
        double portfolioVariance=sim.getVariance();
        RiskContribution<Upper> rc(portfolioResults);
        double var=rc.getVaR(.99);
        rc.getRCCov(assetRC, assetExp, estimate, portfolioVariance, var);
        double min=DBL_MAX; 
        double max=DBL_MIN;
        binAndSend(
            [&](std::string& res){
                lossResults=res;
            },
            min,
            max,
            portfolioResults
        );
        min=DBL_MAX; 
        max=DBL_MIN;
        binAndSend(
            [&](std::string& res){
                rcResults=res;
            },
            min,
            max,
            assetRC
        );
    }
    /**
    Overloaded function to retrieve the unit loss frequency.
    @param pdModel A function that takes 3 parameters: the loan index, the forecasted time, and the time on books
    @param cb Callback to retrieve initial prediction and current prediction
    */
    template<typename PDModel, typename Callback>
    void createCULCurve(PDModel& pdModel, Callback& cb){
        createCULCurve(pdModel, [](double pd1, double pd2, int tmp1, double tmp2){return pd2-pd1;}, cb);
    }
    /**
    Overloaded function to retrieve the unit loss frequency.
    @param pdModel A function that takes 3 parameters: the loan index, the forecasted time, and the time on books
    @param season A function that takes 2 parameters: the booked month of the loan, and the forecasted time
    @param cb Callback to retrieve initial prediction and current prediction
    */
    template<typename PDModel, typename Seasonality, typename Callback>
    void createCULCurve(PDModel& pdModel, Seasonality& season, Callback& cb){ 
        if(!checkSize()){
            throw 0;
        }
		int minTimeOnBooks=(int)totalLengthOnBooks; 
		std::vector<double> expectedCurve(minTimeOnBooks, 0.0);
		std::vector<double> expectedAsOfCurve(minTimeOnBooks, 0.0);
		int totalLoans=timeOnBooks.size();
        for(int j=0; j<totalLoans; ++j){
            if(timeOnBooks[j]<minTimeOnBooks){
                minTimeOnBooks=timeOnBooks[j];
            }
            if(defaultIndicator[j]==0){
                for(int i=0; i<timeRemaining[j]-1; ++i){
                    double totalTime=timeOnBooks[j]+i;
                    double pd2AsOf=pdModel(j, totalTime+1, timeOnBooks[j]);
                    double pd1AsOf=pdModel(j, totalTime, timeOnBooks[j]);
                    expectedAsOfCurve[i+1]+=season(pd1AsOf, pd2AsOf, bookMonth[j], totalTime);
                }
            }
            for(int i=0; i<totalLengthOnBooks; ++i){
                double pd2=pdModel(j, (double)(i+1), 0);
                double pd1=pdModel(j, (double)i, 0);
                expectedCurve[i+1]+=season(pd1, pd2, bookMonth[j], i);
            }
        }
        double cum1=0;
        double cum2=0;
		for(int i=0; i<totalLengthOnBooks-minTimeOnBooks; ++i){
            cum2+=std::max(expectedAsOfCurve[i], 0.0);
            expectedAsOfCurve[i]=cum2/totalLoans;
		}
        for(auto& val: expectedCurve){
            cum1+=std::max(val, 0.0);
			val=cum1/totalLoans;
        }
		expectedAsOfCurve.resize(totalLengthOnBooks-minTimeOnBooks);
		cb(expectedCurve, expectedAsOfCurve);
    }
    /**
    Overloaded function to retrieve the net loss balance by months in future.
    @param pdModel A function that takes 3 parameters: 
    the loan index, the forecasted time, and the time on books
    @param lossModel A function that takes 3 parameters: 
    the loan index, a normal random variable, and the time of default
    @param cb Callback to retrieve initial prediction and current prediction
    */
    template<typename PDModel, typename LossModel, typename Callback>
    void createCNLCurve(PDModel& pdModel, LossModel& lossModel, Callback& cb){
        createCNLCurve(1, pdModel, lossModel, cb);
    }
    /**
    Overloaded function to retrieve the net loss frequency.
    @param balance The sum of the beginning balances of the portfolio 
    @param pdModel A function that takes 3 parameters: 
    the loan index, the forecasted time, and the time on books
    @param lossModel A function that takes 3 parameters: 
    the loan index, a normal random variable, and the time of default
    @param cb Callback to retrieve initial prediction and current prediction
    */
    template<typename PDModel, typename LossModel, typename Callback>
    void createCNLCurve(double balance, PDModel& pdModel, LossModel& lossModel, Callback& cb){
        createCNLCurve(balance, pdModel, lossModel, [](double pd1, double pd2, int tmp1, double tmp2){return pd2-pd1;}, cb);
    }
    /**
    Overloaded function to retrieve the net loss frequency.
    @param balance The sum of the beginning balances of the portfolio 
    @param pdModel A function that takes 3 parameters: 
    the loan index, the forecasted time, and the time on books
    @param lossModel A function that takes 3 parameters: 
    the loan index, a normal random variable, and the time of default
    @param season A function that takes 2 parameters: 
    the booked month of the loan, and the forecasted time
    @param cb Callback to retrieve initial prediction and current prediction
    */
    template<typename PDModel, typename LossModel, typename Seasonality, typename Callback>
    void createCNLCurve(double balance, PDModel& pdModel, LossModel& lossModel, Seasonality& season, Callback& cb){ 
        if(!checkSize()){
            throw 0;
        }
		int minTimeOnBooks=(int)totalLengthOnBooks; 
		std::vector<double> expectedCurveAsOf(minTimeOnBooks, 0.0);
		std::vector<double> expectedCurve(minTimeOnBooks, 0.0);
		int totalLoans=timeOnBooks.size();
        for(int j=0; j<totalLoans; ++j){
            if(timeOnBooks[j]<minTimeOnBooks){
                minTimeOnBooks=timeOnBooks[j];
            }
            if(defaultIndicator[j]==0){
                for(int i=0; i<timeRemaining[j]-1; ++i){
                    double totalTime=timeOnBooks[j]+i;
                    double pd2AsOf=pdModel(j, totalTime+1, timeOnBooks[j]);
                    double pd1AsOf=pdModel(j, totalTime, timeOnBooks[j]);
                    expectedCurveAsOf[i+1]+=season(pd1AsOf, pd2AsOf, bookMonth[j], totalTime)*lossModel(j, 0.0, totalTime+1);
                }
            }
            for(int i=0; i<totalLengthOnBooks; ++i){
                double pd2=pdModel(j, (double)(i+1), 0);
                double pd1=pdModel(j, (double)i, 0);
                expectedCurve[i+1]+=season(pd1, pd2, bookMonth[j], i)*lossModel(j, 0.0, i+1);
            }
        }
        double cum1=0;
        double cum2=0;
        for(int i=0; i<totalLengthOnBooks-minTimeOnBooks; ++i){
            cum2+=std::max(expectedCurveAsOf[i], 0.0);
            expectedCurveAsOf[i]=cum2/balance;
        }
        for(auto& val: expectedCurve){
            cum1+=std::max(val, 0.0);
			val=cum1/balance;
        }
        expectedCurveAsOf.resize(totalLengthOnBooks-minTimeOnBooks);
		cb(expectedCurve, expectedCurveAsOf);
		
    }
    /**
    Function to retrieve the net losses over some time interval.
    @param pdModel A function that takes 3 parameters: 
    the loan index, the forecasted time, and the time on books
    @param lossModel A function that takes 3 parameters: 
    the loan index, a normal random variable, and the time of default
    @param season A function that takes 2 parameters: 
    the booked month of the loan, and the forecasted time
    @param monthsOut Months out to forecast net losses.
    @return Vector of predicted periodic net losses
    */
    template<typename PDModel, typename LossModel, typename Seasonality>
    std::vector<double> getNetLoss(PDModel& pdModel, LossModel& lossModel, Seasonality& season, int monthsOut){ 
        if(!checkSize()){
            throw 0;
        }
		std::vector<double> expectedCurve(monthsOut, 0.0);
		int totalLoans=timeOnBooks.size();
        for(int j=0; j<totalLoans; ++j){
            if(defaultIndicator[j]==0){
                int timeOrMonthsOut=monthsOut;
                if(timeRemaining[j]<monthsOut){
                    timeOrMonthsOut=timeRemaining[j];
                }
                for(int i=0; i<timeOrMonthsOut-1; ++i){
                    double totalTime=timeOnBooks[j]+i;
                    double pd2AsOf=pdModel(j, totalTime+1, timeOnBooks[j]);
                    double pd1AsOf=pdModel(j, totalTime, timeOnBooks[j]);
                    expectedCurve[i+1]+=season(pd1AsOf, pd2AsOf, bookMonth[j], totalTime)*lossModel(j, 0.0, totalTime+1);
                }
            }
        }
        double cum1=0;
        for(auto& val: expectedCurve){
            cum1+=std::max(val, 0.0);
            val=cum1;
        }
		return expectedCurve;
		
    }
};
#endif
