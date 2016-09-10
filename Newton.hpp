Newton::Newton(){

}
Newton::Newton(double prec1, double prec2){
  precision1=prec1;
  precision2=prec2;
}
int Newton::getIterations(){
    return k;
}
template<typename OBJFUNC> //one dimension
void Newton::zeros(OBJFUNC&& objective, double &guesses){ //guess is modified and used as the "result"
    double prec2=1;
    AutoDiff guess(guesses, 1);
    AutoDiff p=objective(guess);
    int j=0;
    while((std::abs(prec2)>precision2||abs(p.getStandard())>precision1) && j<maxNum){
        //prec2=guesses;
        guesses=guesses-p.getStandard()/p.getDual();
        //prec2=(guesses-prec2);///guesses;
        guess.setStandard(guesses);
        guess.setDual(1);
        p=objective(guess); 
        j++;
    }
}
template< typename OBJFUNC> //one dimension
void Newton::bisect(OBJFUNC&& objective, double& guesses, double begin, double end){ //guess is modified and used as the "result"
    double beginResult=objective(begin);
    double endResult=objective(end);
    double prec=2;
    if(beginResult*endResult>0||begin>end){
        endResult=0;
        prec=0;
    }
    double c;
    while(prec>precision1 || abs(endResult)>precision2){
        c=(end+begin)*.5;
        endResult=objective(c);
        if(endResult*beginResult>0){
            begin=c;
        }
        else{
            end=c;
        }
        prec=(begin-end)*.5;
        guesses=c;
    }
}
