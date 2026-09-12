#include "mapgen.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <functional>

namespace banner {
namespace {
int Index(Point p){return p.y*Side+p.x;}
Point Next(Point p,int d){const Point ds[]={{1,0},{0,1},{-1,0},{0,-1}};return {p.x+ds[d].x,p.y+ds[d].y};}
int Chebyshev(Point a,Point b){return std::max(std::abs(a.x-b.x),std::abs(a.y-b.y));}
bool Solid(Tile t){return t==Tile::Mountain||t==Tile::Building;}
struct State {std::array<Point,3> players;unsigned dead=0,acted=0;};
int EnemyAt(const Map& m,const State& s,Point p){for(int i=0;i<3;++i)if(!(s.dead&(1u<<i))&&m.enemies[i].p==p)return i;return -1;}
bool PlayerAt(const State& s,Point p){return std::find(s.players.begin(),s.players.end(),p)!=s.players.end();}
std::vector<Point> Moves(const Map& m,const State& s,int id){
    std::vector<Point> out{s.players[id]};std::array<int,64> distance;distance.fill(-1);
    std::queue<Point> q;q.push(s.players[id]);distance[Index(q.front())]=0;
    while(!q.empty()){auto p=q.front();q.pop();if(distance[Index(p)]==3)continue;
        for(int d=0;d<4;++d){auto n=Next(p,d);if(!Inside(n)||!Walkable(m.tiles[Index(n)])||distance[Index(n)]>=0||EnemyAt(m,s,n)>=0)continue;
            distance[Index(n)]=distance[Index(p)]+1;q.push(n);if(!PlayerAt(s,n))out.push_back(n);
        }
    }return out;
}
int RayEnemy(const Map& m,const State& s,Point origin,int direction){
    for(auto p=Next(origin,direction);Inside(p);p=Next(p,direction)){
        int e=EnemyAt(m,s,p);if(e>=0)return e;
        if(Solid(m.tiles[Index(p)])||PlayerAt(s,p))return -1;
    }return -1;
}
int GridDamage(const Map& m,const State& s){
    int damage=0;
    for(int e=0;e<3;++e)if(!(s.dead&(1u<<e))&&m.enemies[e].direction>=0){
        for(auto p=Next(m.enemies[e].p,m.enemies[e].direction);Inside(p);p=Next(p,m.enemies[e].direction)){
            if(EnemyAt(m,s,p)>=0||PlayerAt(s,p))break;
            if(Solid(m.tiles[Index(p)])){if(m.tiles[Index(p)]==Tile::Building)++damage;break;}
        }
    }return damage;
}
bool Apply(const Map& m,State& s,const Step& step){
    if(step.player<0||step.player>=3||step.enemy<0||step.enemy>=3||(s.acted&(1u<<step.player)))return false;
    auto moves=Moves(m,s,step.player);if(std::find(moves.begin(),moves.end(),step.destination)==moves.end())return false;
    State next=s;next.players[step.player]=step.destination;bool canShoot=false;
    for(int d=0;d<4;++d)canShoot|=RayEnemy(m,next,step.destination,d)==step.enemy;
    if(!canShoot)return false;
    next.dead|=1u<<step.enemy;next.acted|=1u<<step.player;s=next;return true;
}
// Original authored demonstration layouts. Not copied from either game's assets.
struct Layout {const char* tag;std::array<const char*,8> rows;};
const Layout layouts[]={
    {"urban",{"~......~","..#.....",".B...B..","........","...B..#.",".B....B.",".....#..","~......~"}},
    {"canal",{"~~......",".....#..","..B...B.","........",".#..B...","..B...B.",".....#..","......~~"}},
    {"urban",{"~......~",".#......","..B..B..","......#.","....B...",".B....B.","...#....","~......~"}}
};
Map Expand(int id,uint32_t seed,Random& rng){
    Map m;m.seed=seed;m.templateId=id;
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){char c=layouts[id].rows[y][x];Tile t=Tile::Ground;
        if(c=='~')t=Tile::Water;else if(c=='#')t=Tile::Mountain;else if(c=='B')t=Tile::Building;
        else {auto i=WeightedPick({80,15,5},rng);t=i==0?Tile::Grass:i==1?Tile::TallGrass:Tile::Ground;}
        m.tiles[Index({x,y})]=t;
    }
    std::vector<Point> playerZone,enemyZone,objectives;
    for(int y=0;y<8;++y)for(int x=0;x<8;++x){Point p{x,y};if(m.tiles[Index(p)]==Tile::Building)objectives.push_back(p);
        if(Walkable(m.tiles[Index(p)])&&x>=1&&x<=6){if(y>=5)playerZone.push_back(p);if(y<=3)enemyZone.push_back(p);}}
    rng.Shuffle(playerZone);rng.Shuffle(enemyZone);
    for(int i=0;i<3;++i){m.players[i]=playerZone.at(i);m.enemies[i]={enemyZone.at(i),-1};}
    m.objectives=SelectDefendedPair(objectives,rng).value();
    // Lock one direction per enemy that currently threatens a building, where available.
    State s{m.players};
    for(int e=0;e<3;++e){std::vector<int> dirs;
        for(int d=0;d<4;++d)for(auto p=Next(m.enemies[e].p,d);Inside(p);p=Next(p,d)){
            if(EnemyAt(m,s,p)>=0||PlayerAt(s,p))break;
            if(Solid(m.tiles[Index(p)])){if(m.tiles[Index(p)]==Tile::Building)dirs.push_back(d);break;}
        }
        if(!dirs.empty())m.enemies[e].direction=dirs[rng.Below(uint32_t(dirs.size()))];
    }return m;
}
}
uint32_t Random::Below(uint32_t bound){
    if(!bound)throw std::invalid_argument("zero random bound");
    // Rejection removes modulo bias. Explicit algorithm avoids std::distribution platform variation.
    uint32_t threshold=(uint32_t(0)-bound)%bound,v;do{v=uint32_t(engine());}while(v<threshold);return v%bound;
}
size_t WeightedPick(const std::vector<uint32_t>& weights,Random& rng){
    uint64_t sum=0;for(auto w:weights)sum+=w;
    if(!sum||sum>std::numeric_limits<uint32_t>::max())throw std::invalid_argument("invalid weight sum");
    auto roll=rng.Below(uint32_t(sum));for(size_t i=0;i<weights.size();++i){if(roll<weights[i])return i;roll-=weights[i];}
    throw std::logic_error("weight selection");
}
std::optional<std::array<Point,2>> SelectDefendedPair(std::vector<Point> zone,Random& rng){
    // Chebyshev separation reconstructs Mission:AddDefended's spacing rule.
    // Improvement: enumerate all legal pairs so a bad first choice cannot hide a valid pair.
    std::vector<std::array<Point,2>> pairs;
    for(size_t i=0;i<zone.size();++i)for(size_t j=i+1;j<zone.size();++j)if(Chebyshev(zone[i],zone[j])>1)pairs.push_back({zone[i],zone[j]});
    if(pairs.empty())return std::nullopt;return pairs[rng.Below(uint32_t(pairs.size()))];
}
std::optional<std::array<Point,2>> SelectDefendedPairGreedy(std::vector<Point> zone,Random& rng){
    if(zone.size()<2)return std::nullopt;
    auto take=[&](){auto i=rng.Below(uint32_t(zone.size()));auto p=zone[i];zone.erase(zone.begin()+i);return p;};
    auto first=take();while(!zone.empty()){auto second=take();if(Chebyshev(first,second)>1)return std::array<Point,2>{first,second};}
    return std::nullopt;
}
std::vector<Point> PreferOpenBuildings(const std::vector<BuildingCandidate>& candidates){
    std::vector<Point> preferred,backup;for(auto c:candidates)(c.openFlyerNeighbors>1?preferred:backup).push_back(c.p);
    return preferred.empty()?backup:preferred;
}
std::vector<SpawnOption> FilterSpawnCaps(const std::vector<SpawnOption>& choices,int livingUpgrades,int upgradeCap){
    std::vector<SpawnOption> out;for(auto c:choices)if(c.living<c.cap&&(!c.upgraded||livingUpgrades<upgradeCap))out.push_back(c);return out;
}
bool Inside(Point p){return p.x>=0&&p.x<8&&p.y>=0&&p.y<8;}
bool Walkable(Tile t){return t==Tile::Ground||t==Tile::Grass||t==Tile::TallGrass;}
std::vector<std::string> Validate(const Map& m){
    std::vector<std::string> errors;std::array<bool,64> occupied{};int buildings=0;
    for(auto t:m.tiles){if(int(t)<0||int(t)>int(Tile::Building))errors.push_back("unknown terrain");if(t==Tile::Building)++buildings;}
    if(buildings!=5)errors.push_back("expected five buildings");
    auto unit=[&](Point p){if(!Inside(p)){errors.push_back("spawn outside board");return;}
        if(occupied[Index(p)])errors.push_back("overlapping spawns");occupied[Index(p)]=true;
        if(!Walkable(m.tiles[Index(p)]))errors.push_back("spawn on blocked terrain");
        int exits=0;for(int d=0;d<4;++d){auto n=Next(p,d);if(Inside(n)&&Walkable(m.tiles[Index(n)]))++exits;}
        if(exits<2)errors.push_back("spawn lacks two ground exits");};
    for(auto p:m.players)unit(p);for(auto e:m.enemies){unit(e.p);if(e.direction< -1||e.direction>3)errors.push_back("invalid intent direction");}
    for(auto p:m.objectives)if(!Inside(p)||m.tiles[Index(p)]!=Tile::Building)errors.push_back("invalid objective");
    if(Chebyshev(m.objectives[0],m.objectives[1])<=1)errors.push_back("objectives too close");
    if(!errors.empty())return errors; // Never index unvalidated coordinates.
    std::array<bool,64> seen{};std::queue<Point> q;q.push(m.players[0]);seen[Index(q.front())]=true;
    while(!q.empty()){auto p=q.front();q.pop();for(int d=0;d<4;++d){auto n=Next(p,d);if(Inside(n)&&!seen[Index(n)]&&Walkable(m.tiles[Index(n)])){seen[Index(n)]=true;q.push(n);}}}
    for(int i=0;i<64;++i)if(Walkable(m.tiles[i])&&!seen[i]){errors.push_back("disconnected ground");break;}
    // Every building has at least two reachable adjacent cells, not merely all floor cells connected.
    for(int i=0;i<64;++i)if(m.tiles[i]==Tile::Building){int access=0;for(int d=0;d<4;++d){auto n=Next({i%8,i/8},d);if(Inside(n)&&seen[Index(n)])++access;}if(access<2)errors.push_back("building lacks reachable approaches");}
    State s{m.players};for(int i=0;i<3;++i)if(Moves(m,s,i).size()<2)errors.push_back("player has no actual legal move");
    return errors;
}
int OpeningGridDamage(const Map& m){if(!Validate(m).empty())throw std::invalid_argument("invalid map");return GridDamage(m,State{m.players});}
Proof ProveOpening(const Map& m,size_t budget){
    Proof proof;if(!Validate(m).empty())return proof;std::vector<Step> path;
    std::function<bool(State)> search=[&](State s){
        if(proof.nodes>=budget){proof.exhausted=true;return false;}++proof.nodes;
        if(GridDamage(m,s)==0){proof.steps=path;return true;}
        for(int id=0;id<3;++id)if(!(s.acted&(1u<<id))){
            for(auto dest:Moves(m,s,id)){State moved=s;moved.players[id]=dest;
                for(int d=0;d<4;++d){int target=RayEnemy(m,moved,dest,d);if(target<0)continue;
                    State next=moved;next.dead|=1u<<target;next.acted|=1u<<id;
                    path.push_back({id,dest,target});if(search(next))return true;path.pop_back();
                    if(proof.exhausted)return false;
                }
            }
        }return false;
    };proof.found=search(State{m.players});return proof;
}
bool ReplayProof(const Map& m,const Proof& proof){
    if(!proof.found||!Validate(m).empty())return false;State s{m.players};
    for(auto step:proof.steps)if(!Apply(m,s,step))return false;return GridDamage(m,s)==0;
}
Result Generate(uint32_t seed,int maxAttempts,const std::string& tag){
    if(maxAttempts<0||maxAttempts>10000)throw std::invalid_argument("attempt budget out of range");
    Random rng(seed);std::vector<int> pool;for(int i=0;i<3;++i)if(tag=="any"||tag==layouts[i].tag)pool.push_back(i);
    Result result;if(pool.empty()){result.reason="no template matches tag";return result;}
    for(int attempt=0;attempt<maxAttempts;++attempt){++result.attempts;
        Map m=Expand(pool[rng.Below(uint32_t(pool.size()))],seed,rng);auto errors=Validate(m);
        if(!errors.empty()){result.reason=errors.front();continue;}
        if(OpeningGridDamage(m)==0){result.reason="no opening threat; reject trivial encounter";continue;}
        auto proof=ProveOpening(m);if(!proof.found||!ReplayProof(m,proof)){result.reason=proof.exhausted?"proof search budget exhausted":"no demonstrated opening solution";continue;}
        result.map=m;result.opening=proof;result.reason="validated structure and replayed zero-grid-loss opening";return result;
    }return result;
}
std::string Describe(const Map& m,const Proof& p){
    std::ostringstream out;out<<"seed="<<m.seed<<" template="<<m.templateId<<" initial_grid_threat="<<OpeningGridDamage(m)<<" proof_nodes="<<p.nodes<<'\n';
    const char tiles[]={'.',',','"','#','~','B'};
    for(int y=0;y<8;++y){for(int x=0;x<8;++x){Point at{x,y};char c=tiles[int(m.tiles[Index(at)])];
        for(int i=0;i<3;++i){if(m.players[i]==at)c=char('1'+i);if(m.enemies[i].p==at)c=char('a'+i);}out<<c;}out<<'\n';}
    out<<"Legend: 1-3 players; a-c enemies; B building; # mountain; ~ water; , grass; quote tall grass\n";
    for(auto step:p.steps)out<<"player "<<step.player+1<<" move ("<<step.destination.x<<','<<step.destination.y<<") shoot "<<char('a'+step.enemy)<<'\n';
    out<<"Proof applies only to the documented reference combat model.\n";return out.str();
}
}
