#ifndef __SEASONALITY_H_INCLUDED__
#define __SEASONALITY_H_INCLUDED__

/**
Seasonality stores monthly seasonality adjustments.
Model documentation in Seasonality.pdf
*/
class Seasonality{
private:
    std::vector<double> season;

public:
    Seasonality(){

    }
    /**
    Adds an adjustment for a month (in order).
    The fourth index should be May (0th indexed).
    Should sum to 12 (average of one).
    @param adjustment The adjustment to monthly
    seasonality.  
    */
    void addSeason(double adjustment){
        season.push_back(adjustment);
    }
    /**
    Returns the adjustment based on a loan's
    booking month and the months on book.
    @param beginMonth The month the loan was booked. 
    Not zero indexed (1 is Jan, 2 is Feb etc).
    @param monthsSinceBegin The months since the 
    loan was booked. 
    @return The adjustment.
    */
    double applySeasonality(int beginMonth, int monthsSinceBegin){
        int index=(monthsSinceBegin+beginMonth-1)%12; //eg, april +13 months returns 4 (may)
        return season[index];
    }


};
#endif