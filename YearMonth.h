#ifndef __YEARMONTH_H_INCLUDED__
#define __YEARMONTH_H_INCLUDED__
struct YearMonth{
    int year;
    int month;
    int yearInMonth;
    YearMonth(){
        year=1600;
        month=1;
        yearInMonth=year*12;
    }
    YearMonth(int yr, int mnth){
        year=yr;
        month=mnth;
        yearInMonth=year*12;
    }
    int operator-(const YearMonth& yr){
        return (yearInMonth+month)-yr.yearInMonth-yr.month;
    }
    YearMonth& operator++(){
        yearInMonth++;
        year=yearInMonth/12;
        month=(yearInMonth-1)%12+1;
        return *this;
    }
    YearMonth& operator+=(int months){
        yearInMonth+=months;
        year=yearInMonth/12;
        month=(yearInMonth-1)%12+1;
        return *this;
    }
    YearMonth& operator-=(int months){
        yearInMonth-=months;
        year=yearInMonth/12;
        month=(yearInMonth-1)%12+1;
        return *this;
    }
    YearMonth& operator-=(const YearMonth& yr){
        yearInMonth=(yearInMonth+month)-yr.yearInMonth-yr.month;
        year=yearInMonth/12;
        month=(yearInMonth-1)%12+1;
        return *this;
    }
    YearMonth operator+(int months){
        int yrinm=yearInMonth+month+months;
        return YearMonth(yrinm/12, (yrinm-1)%12+1);
    }
    YearMonth operator-(int months){
        int yrinm=yearInMonth+month-months;
        return YearMonth(yrinm/12, (yrinm-1)%12+1);
    }
    bool operator>(const YearMonth& yr){
        return yearInMonth>yr.yearInMonth;
    }
    bool operator<(const YearMonth& yr){
        return yearInMonth<yr.yearInMonth;
    }
    bool operator==(const YearMonth& yr){
        return yearInMonth==yr.yearInMonth;
    }
    /*void operator<<(const YearMonth& yr){
        return yr.year<<"-"<<yr.month;
    }*/
};
#endif