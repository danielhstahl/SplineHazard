#ifndef __SHAZARD_H_INCLUDED__
#define __SHAZARD_H_INCLUDED__
#include <cmath>
#include <iostream>
#include <vector>
#include "FunctionalUtilities"

const int Hazard=0;
const int Odds=1;
const int Probit=2;
constexpr int Attribute=0;
constexpr int AttributeMean=1;
constexpr int Coefficient=2;
/**
SHazard is the PD model implementation.
*/
template<typename T>
auto maxZeroOrNumber(const T& number){
    return number>0?number:0;
}

template<typename T>
auto template_power<0>(const T& number){
    return number;
}
template<typename T, int N>
auto template_power<N>(const T& number){
    return template_power<N-1>*number;
}


const double isqrt2=1.0/sqrt(2.0);
namespace shazard {
    /**cspline returns a cubic spline at a point log(t) given knots and "gamma"'s for each knot*/
    template<typename T, typename Tuple>
    auto cspline(const num& x, const std::vector<Tuple>& knots_gamma){//x is log(t)
        const auto minDiff=x-knots_gamma.front()<0>;//difference between given x and first knot
        const auto maxDiff=knots_gamma.back()<0>-x;//difference between last knot and given x
        const auto span=knots_gamma.back()<0>-knots_gamma.front()<0>;//span of knots
        const auto tripleMin=template_power<3>(maxZeroOrNumber(minDiff)); //minDiff^3, if minDiff>0 else 0
        const auto tripleMax=template_power<3>(maxZeroOrNumber(maxDiff));//maxDiff^3, if minDiff>0 else 0
        int startFrom=1;
        int endFrom=1;
        return futilities::sum_subset(knots_gamma, startFrom, endFrom, [&](const auto& tuple, const auto& index){
            const auto lambda=(knots_gamma.back()<0>-tuple<0>)/span;
            const auto currDiff=x-tuple<0>;//difference between given x and first knot
            const auto tripleCurr=template_power<3>(maxZeroOrNumber(minDiff)); //minDiff^3, if minDiff>0 else 0
            return (tripleCurr+maxDiff*(lambda-1)-minDiff*lambda)*knots_gamma[index+1]<1>;
        })+knots_gamma.front()<1>+knots_gamma[1]<1>*x;
    }
    /**gS is an offset spline*/
    template<typename T>
    auto gS(const T& logTimeHorizon, double offset){
        return cspline(logTimeHorizon)+offset;
    }
    /**
    Function to retrieve PD for a given loan.
    @param timeHorizon The time horizon 
    @param currentTime The current time
    @param frailty A positive random variable which jointly impacts losses
    @return Estimate of PD
    */
    template<typename T, typename C, typename F, typename A, typename Tuple, typename SurvivalFunction>
    auto PD(const T& timeHorizon, const C& currentTime, const F& frailty, const std::vector<Tuple>& attributesAndCoefficients, const SurvivalFunction& surv){
        return 1.0-surv(timeHorizon, currentTime, futilities::sum(attributesAndCoefficients, [&](const auto& attributeAndCoefficient, const auto& index){
            return (attributesAndCoefficients<Attribute>-attributesAndCoefficients<AttributeMean>)*attributesAndCoefficients<Coefficient>;
        }), frailty);
    }

    /**
    function to retrieve Survival probability for odds
    @param timeHorizon The time horizon
    @param currentTime The current time
    @param offset Some offset to apply to probability 
    (eg seasonality or idiosyncratic variables)
    @param frailty A positive random variable which jointly impacts losses
    @return Estimate of Survival probability
    */
    template<typename T, typename C, typename F, typename Offset, typename Tuple>
    auto SurvivalProbabilityOdds(const T& timeHorizon, const C& currentTime, const Offset& offset, const F& frailty){
        auto totalOffset=offset+frailty;
        bool isSGreaterThanZero=currentTime>0;
        return currentTime>0?(exp(gS(log(s), offset))+1.0)/(exp(gS(log(x), offset))+1.0):1.0/(exp(gS(log(x), offset))+1);
    }
    /**
    function to retrieve Survival probability for odds
    @param timeHorizon The time horizon
    @param currentTime The current time
    @param offset Some offset to apply to probability 
    (eg seasonality or idiosyncratic variables)
    @param frailty A positive random variable which jointly impacts losses
    @return Estimate of Survival probability
    */
    template<typename T, typename C, typename F, typename Offset, typename Tuple>
    auto SurvivalProbabilityHazard(const T& timeHorizon, const C& currentTime, const Offset& offset, const F& frailty){
        auto totalOffset=offset+frailty;
        bool isSGreaterThanZero=currentTime>0;
        return currentTime>0?exp(-exp(gS(log(x), offset)))/exp(-exp(gS(log(s), offset))):exp(-exp(gS(log(x), offset)));
    }
    /**
    function to retrieve Survival probability for odds
    @param timeHorizon The time horizon
    @param currentTime The current time
    @param offset Some offset to apply to probability 
    (eg seasonality or idiosyncratic variables)
    @param frailty A positive random variable which jointly impacts losses
    @return Estimate of Survival probability
    */
    template<typename T, typename C, typename F, typename Offset, typename Tuple>
    auto SurvivalProbabilityProbit(const T& timeHorizon, const C& currentTime, const Offset& offset, const F& frailty){
        auto totalOffset=offset+frailty;
        bool isSGreaterThanZero=currentTime>0;
        return currentTime>0?(erf(gS(log(x), offset)*isqrt2)*.5+.5)/(erf(gS(log(s), offset)*isqrt2)*.5+.5)/exp(-exp(gS(log(s), offset))):erf(gS(log(x), offset)*isqrt2)*.5+.5;//this is super slow for some reason
    }

    template<typename T, typename C, typename Surv, typename Tuple>
    simulatedTimeToDefault(const C& timeOnBooks, const C& timeRemaining, const std::vector<Tuple>& attributesAndCoefficients, const Surv& surv, double frailty, double unif){
        return (PD(timeOnBooks+timeRemaining, timeOnBooks, frailty, attributesAndCoefficients, surv)-unif<0):100000.0:newton::bisect([&](const auto& theta){
            return PD(timeOnBooks+theta, timeOnBooks, frailty, attributesAndCoefficients, surv)-unif;
        }, 0, 1, .00001, .00001);
    }

    template<typename T, typename C, typename F, typename A, typename Tuple, typename U, typename SurvivalFunction>
    void simulatePortfolio(int n, const std::vector<C>& timeOnBooks, const std::vector<C>& timeRemaining,const F& frailtyGenerator, const U& unifRandGenerator, const std::vector<std::vector<Tuple> >& attributesAndCoefficients,  SurvivalFunction& surv){
        return futilities::for_each_parallel(0, n, [&](const auto& index){
            return futilities::for_each_parallel(attributesAndCoefficients, [&](const auto& attributeAndCoefInstance, const auto& portIndex){
                return simulatedTimeToDefault(timeOnBooks[index], timeRemaining[index],attributeAndCoefInstance, surv, frailtyGenerator(), unifRandGenerator());
            });
        });
    }
}

#endif