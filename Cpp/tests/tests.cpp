#include "mapgen.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
using namespace banner;
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    Random rng(42);int counts[3]={};for(int i=0;i<100000;++i)++counts[WeightedPick({80,15,5},rng)];
    Check(std::abs(counts[0]-80000)<700&&std::abs(counts[1]-15000)<700&&std::abs(counts[2]-5000)<700,"weight frequency smoke check");
    bool threw=false;try{WeightedPick({0,0},rng);}catch(const std::invalid_argument&){threw=true;}Check(threw,"zero weights rejected");
    Check(!SelectDefendedPair({{1,1},{2,2}},rng),"diagonally adjacent defended pair rejected");
    for(int i=0;i<100;++i){auto p=SelectDefendedPair({{1,1},{0,0},{2,2}},rng);Check(p.has_value(),"finds legal pair despite invalid first choice");}
    int greedyFailures=0;for(int i=0;i<100;++i)if(!SelectDefendedPairGreedy({{1,1},{0,0},{2,2}},rng))++greedyFailures;
    Check(greedyFailures>0&&greedyFailures<100,"original greedy policy can miss a valid pair");
    Check(PreferOpenBuildings({{{1,1},1},{{3,3},2}})==std::vector<Point>{{3,3}},"original building preference");
    Check(PreferOpenBuildings({{{1,1},1}})==std::vector<Point>{{1,1}},"original fallback permits low-access building");
    auto options=FilterSpawnCaps({{0,false,3,3},{1,true,0,3},{2,false,0,3}},2,2);
    Check(options.size()==1&&options[0].kind==2,"species and elite caps enforced");
    Check(!Generate(1,0).map,"zero attempt budget fails closed");Check(!Generate(1,3,"missing").map,"unknown tag fails closed");
    auto first=Generate(2047);Check(first.map.has_value(),"sample generated");auto m=*first.map;
    Check(!ProveOpening(m,0).found,"proof budget exhaustion cannot certify map");
    auto corrupted=first.opening;corrupted.steps.push_back({8,{100,100},99});Check(!ReplayProof(m,corrupted),"tampered proof rejected");
    auto bad=m;bad.players[1]=bad.players[0];Check(!Validate(bad).empty(),"overlapping spawn rejected");
    bad=m;bad.players[0]={-1,99};Check(!Validate(bad).empty()&&!ProveOpening(bad).found,"invalid coordinates rejected before indexing");
    bad=m;bad.objectives[1]=bad.objectives[0];Check(!Validate(bad).empty(),"overlapping objectives rejected");
    bad=m;bad.tiles[bad.players[0].y*8+bad.players[0].x]=Tile::Water;Check(!Validate(bad).empty(),"water spawn rejected");
    bad=m;bad.tiles[0]=static_cast<Tile>(99);Check(!Validate(bad).empty(),"unknown terrain rejected");
    Map fixture;fixture.players={Point{1,7},Point{3,7},Point{5,7}};
    fixture.enemies={Enemy{{1,1},-1},Enemy{{3,1},-1},Enemy{{5,1},-1}};
    for(auto p:std::vector<Point>{{2,2},{5,3},{3,4},{1,5},{6,5}})fixture.tiles[p.y*8+p.x]=Tile::Building;
    fixture.objectives={Point{2,2},Point{5,3}};Check(Validate(fixture).empty(),"structural fixture valid");
    bad=fixture;bad.tiles[0]=Tile::Ground;bad.tiles[1]=Tile::Water;bad.tiles[8]=Tile::Water;
    auto errors=Validate(bad);Check(std::find(errors.begin(),errors.end(),"disconnected ground")!=errors.end(),"isolated ground specifically detected");
    bad=fixture;for(auto p:std::vector<Point>{{2,4},{4,4},{3,3},{3,5}})bad.tiles[p.y*8+p.x]=Tile::Mountain;
    errors=Validate(bad);Check(std::find(errors.begin(),errors.end(),"building lacks reachable approaches")!=errors.end(),"sealed objective access specifically detected");
    std::set<std::string> patterns;int totalAttempts=0,maxAttempts=0;size_t maxNodes=0;int templates[3]={};
    for(uint32_t seed=0;seed<1000;++seed){
        auto a=Generate(seed),b=Generate(seed);Check(a.map&&b.map,"seed batch generates within budget");
        Check(Validate(*a.map).empty()&&ReplayProof(*a.map,a.opening),"every returned seed passes structure and independent replay");
        Check(OpeningGridDamage(*a.map)>0,"opening is nontrivial");
        auto sa=Describe(*a.map,a.opening),sb=Describe(*b.map,b.opening);Check(sa==sb,"same seed reproduces map and proof");
        patterns.insert(sa.substr(sa.find('\n')+1,72));++templates[a.map->templateId];totalAttempts+=a.attempts;maxAttempts=std::max(maxAttempts,a.attempts);maxNodes=std::max(maxNodes,a.opening.nodes);
    }
    Check(patterns.size()>900,"different seeds vary output");
    auto canal=Generate(7,64,"canal");Check(canal.map&&canal.map->templateId==1,"tag filters template pool");
    std::cout<<"PASS: 1000 seeds generated, deterministic and replay-certified; unique grids="<<patterns.size()<<" total_attempts="<<totalAttempts<<" max_attempts="<<maxAttempts<<" max_proof_nodes="<<maxNodes<<'\n';
    std::cout<<"weight_counts="<<counts[0]<<','<<counts[1]<<','<<counts[2]<<" templates="<<templates[0]<<','<<templates[1]<<','<<templates[2]<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
