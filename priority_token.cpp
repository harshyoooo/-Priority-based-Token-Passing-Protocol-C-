#include<bits/stdc++.h>
using namespace std;
int curr_time=0;         //curr time
string curr_queue="H";  //"H": high, "M1": medium1, "M2": medium2, "L": low;
int high_time_quantum=50;   //high's time quantum or no.of packets in a round
int med1_time_quantum=20; //same as high time quantum but for M1,M2,L
int med2_time_quantum=15;
int low_time_quantum=5;

pair<int,int> high_priority_range={70,100};    //Range of each priority packet falling category
pair<int,int> medium1_priority_range={41,69};
pair<int,int> medium2_priority_range={11,40};
pair<int,int> low_priority_range={0,10};
struct Packet{
    int id;
    int priority;
    int wait_time;
    int arrival_time;
    Packet(int i, int p, int specific_wait, int arrival_time_in) {
        id = i;
        priority = p;
        wait_time = specific_wait;
        arrival_time=arrival_time_in;
    }
};
vector<Packet> high;
vector<Packet> medium_1;
vector<Packet> medium_2;
vector<Packet> low;

map<string,vector<Packet>> mp={{string("H"),high},{string("M1"),medium_1},{string("M2"),medium_2},{string("L"),low}};  //mapping of the vector as per the curr_queue
map<string,pair<string,string>> mp_next_curr_queue={{string("H"),{string("M1"),string("L")}},{string("M1"),{string("M2"),string("H")}},{string("M2"),{string("L"),string("M1")}},{string("L"),{string("H"),string("M2")}}};
map<string,int> quantum_time={{string("H"),high_time_quantum},{string("M1"),med1_time_quantum},{string("M2"),med2_time_quantum},{string("L"),low_time_quantum}};
void Multilevel_queue_push(Packet pack){
    if(pack.priority>=high_priority_range.first && pack.priority<=high_priority_range.second){
        high.push_back(pack);
    }
    else if(pack.priority>=medium1_priority_range.first && pack.priority<=medium1_priority_range.second){
        medium_1.push_back(pack);
    }
    else if(pack.priority>=medium2_priority_range.first && pack.priority<=medium2_priority_range.second){
        medium_1.push_back(pack);
    }
    else{
        low.push_back(pack);
    }
    return ;
}
void Multilevel_feedback_queue(){
    while(!high.empty() || !medium_1.empty() || !medium_2.empty() || !low.empty()){
        int quantum=quantum_time[curr_queue];
        while(quantum && !mp[curr_queue].empty()){                  //got the shared cable access
            Packet p=mp[curr_queue].back();
            mp[curr_queue].pop_back();
            quantum--;
            curr_time++;
        }
        if(curr_queue!="H"){                                          //this is forwarding
            string before_queue=mp_next_curr_queue[curr_queue].second;

                
            
            mp[curr_queue].back();
        }
        curr_queue=mp_next_curr_queue[curr_queue].first;

    }
}
int main(){

}