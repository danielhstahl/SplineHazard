#ifndef __Matrix_H_INCLUDED__
#define __Matrix_H_INCLUDED__
#include <vector>

template<typename T> //we will assume that T is an int or double
class Matrix{
private:
	int m=0;
	std::vector<T> container;
public:	
	Matrix(){
		
	}
	Matrix(int n, int m_){
		m=m_;
		container=std::vector<T>(n*m, 0.0);
	}
	Matrix(int m_){//set the "Width" before hand
		m=m_;
	}
	void preAllocate(int n, int m_){
		m=m_;
		container=std::vector<T>(n*m, 0.0);
	}
	void setM(int m_){
		m=m_;
	}
	int getM(){
		return m;
	}
	int getN(){
		return container.size()/m;
	}
	void set(int i, int j, const T& value){ //
		int val=i*m+j;
		if(val>container.size()){
			std::cout<<"Error: size too large! i: "<<i<<", j: "<<j<<", val: "<<val<<", size: "<<container.size()<<std::endl;
		}
		else if(val==container.size()){
			container.push_back(value);
		}
		else {
			container[val]=value;
		}
	}
	T get(int i, int j){
		int val=i*m+j;
		if(val>=container.size()){
			std::cout<<"Error: size too large! i: "<<i<<", j: "<<j<<", val: "<<val<<", size: "<<container.size()<<std::endl;
			return 0;
		}
		else {
			return container[val];
		}
	}
	void push_back(const T& value){
		container.push_back(value);
	}
	void emplace_back(const T& value){
		container.emplace_back(value);
	}
	double getMean(int j){
		int n=getN();
		double mean=0;
		for(int i=0;i<n; ++i){
			mean+=container[i*m+j];
		}
		return mean/(double)n;
	}
	void clear(){
		container.clear();
	}
	
	
};

#endif