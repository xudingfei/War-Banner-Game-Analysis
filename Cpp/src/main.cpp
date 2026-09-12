#include "mapgen.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
int main(int argc,char** argv){try{
    uint32_t seed=2047;std::string tag="any";
    if(argc>1){std::string arg=argv[1];size_t consumed=0;auto n=std::stoull(arg,&consumed);if(consumed!=arg.size()||arg[0]=='-'||n>std::numeric_limits<uint32_t>::max())throw std::invalid_argument("seed must be uint32");seed=uint32_t(n);}
    if(argc>2)tag=argv[2];
    if(argc>3)throw std::invalid_argument("usage: mapgen_demo [uint32 seed] [any|urban|canal]");
    auto result=banner::Generate(seed,64,tag);
    if(!result.map){std::cerr<<"Generation rejected after "<<result.attempts<<" attempts: "<<result.reason<<'\n';return 2;}
    std::cout<<banner::Describe(*result.map,result.opening)<<"attempts="<<result.attempts<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
