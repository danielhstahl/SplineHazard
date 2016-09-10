template<typename Number>
MC<Number>::MC(int m_){
    m=m_;
}
template<typename Number>
MC<Number>::MC(){

}
template<typename Number>
void MC<Number>::setM(int m_){
    m=m_;
}
template<typename Number>
Number MC<Number>::getEstimate(){
    return estimate;
}
template<typename Number>
Number MC<Number>::getError(){
    return sqrt(error/(double)m);
}
template<typename Number>
Number MC<Number>::getVariance(){
    return error;
}
template<typename Number>
std::vector<Number> MC<Number>::getDistribution(){
	return distribution;
}

template<typename Number>
template<typename FN>
void MC<Number>::simulate(FN&& fn) {
    estimate=fn(0);
    error=estimate*estimate;
    #pragma omp parallel//multithread using openmp
        {
        #pragma omp for //multithread using openmp
            for(int j=1; j<m; j++){
                Number est=fn(j);
                estimate+=est;
                error+=est*est;
            }
        }
    estimate=estimate/(double)m;
    double den=(double)(m-1);
    error=(error-estimate*estimate*m)/den;///
}
template<typename Number>
template<typename FN>
void MC<Number>::simulateDistribution(FN&& fn) {
    estimate=fn();
    error=estimate*estimate;
    distribution=std::vector<Number>(m);
    distribution[0]=estimate;
    #pragma omp parallel//multithread using openmp
        {
        #pragma omp for //multithread using openmp
            for(int j=1; j<m; j++){
                distribution[j]=fn();
                estimate+=distribution[j];
                error+=distribution[j]*distribution[j];
            }
        }
   // std::sort(distribution.begin(), distribution.end());
    estimate=estimate/(double)m;
    double den=(double)(m-1);
    error=(error-estimate*estimate*m)/den;
}
