#include<bits/stdc++.h>
using namespace std;
int curr_time=0;         //curr time
string curr_queue="H";  //"H": high, "M1": medium1, "M2": medium2, "L": low;
int high_time_quantum=60;   //high's time quantum or no.of packets in a round    //shifts to 80
int med1_time_quantum=35; //same as high time quantum but for M1,M2,L  //shifts to 50
int med2_time_quantum=18;   // shifts to 31
int low_time_quantum=7;    //shifts to 18
int window_size_arq=30;
int high_wait;
int medium1_wait;
int medium2_wait;
int low_wait;
int N=2*window_size_arq;

pair<int,int> high_priority_range={70,100};    //Range of each priority packet falling category
pair<int,int> medium1_priority_range={41,69};
pair<int,int> medium2_priority_range={11,40};
pair<int,int> low_priority_range={0,10};
struct Packet{
    int id;
    int priority;
    int wait_time;
    int arrival_time;
    int device;
    int no_packets;
    int sequence_no;
    int receiver;
    Packet(int i, int p, int specific_wait, int arrival_time_in,int packets,int no,int node,int get) {
        id = i;
        sequence_no=no;
        priority = p;
        device=node;
        wait_time = specific_wait;
        arrival_time=arrival_time_in;
        no_packets=packets;
        receiver=get;
    }
};
struct frames{
    int seq_num;
    string data;
    frames(int num,string s){
        data=s;
        seq_num=num;
    }
};
int MAX_DEVICES=10;
class device{
    public:
    int id;
    vector<vector<string>> data_to_be_sent = vector<vector<string>>(MAX_DEVICES);
    vector<vector<string>> v = vector<vector<string>>(MAX_DEVICES);
    vector<vector<int>> check = vector<vector<int>>(MAX_DEVICES,vector<int> (60,0));
    vector<deque<int>> ACK = vector<deque<int>>(MAX_DEVICES);
};
deque<Packet> high;
deque<Packet> medium_1;
deque<Packet> medium_2;
deque<Packet> low;

map<string,deque<Packet>> mp={{string("H"),high},{string("M1"),medium_1},{string("M2"),medium_2},{string("L"),low}};  //mapping of the vector as per the curr_queue
map<string,pair<string,string>> mp_next_curr_queue={{string("H"),{string("M1"),string("L")}},{string("M1"),{string("M2"),string("H")}},{string("M2"),{string("L"),string("M1")}},{string("L"),{string("H"),string("M2")}}};
map<string,int> quantum_time={{string("H"),high_time_quantum},{string("M1"),med1_time_quantum},{string("M2"),med2_time_quantum},{string("L"),low_time_quantum}};
map<string,int> starve_time_lim={{string("H"),80},{string("M1"),100},{string("M2"),220},{string("L"),300}};
map<string,int> deque_size_lim={{string("H"),170},{string("M1"),280},{string("M2"),440},{string("L"),600}};
map<string,int> shifted_quantum={{string("H"),high_time_quantum+20},{string("M1"),med1_time_quantum+15},{string("M2"),med2_time_quantum+13},{string("L"),low_time_quantum+11}};
map<string,int> curr_starved_bool={{string("H"),0},{string("M1"),0},{string("M2"),0},{string("L"),0}};
map<string,int> next_wait={{string("M1"),high_wait},{string("M2"),medium1_wait},{string("L"),medium2_wait}};
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
        int quantum;
        int starved_last=curr_starved_bool[curr_queue];
        if(mp[curr_queue].size()>deque_size_lim[curr_queue] || starved_last){
            quantum=shifted_quantum[curr_queue];
        }
        else{
            quantum=quantum_time[curr_queue];
        }
        while(quantum && !mp[curr_queue].empty()){                  //got the shared cable access
            Packet p=mp[curr_queue].front();
            mp[curr_queue].pop_front();
            quantum--;
            curr_time++;      //counts time
        }                                       //this is forwarding
            string before_queue=mp_next_curr_queue[curr_queue].second;
            int num_of_starved=binary(0,mp[curr_queue].size()-1,mp[curr_queue],starve_time_lim[curr_queue]);
            if(num_of_starved!=-1 && curr_queue!="H"){
                int space=deque_size_lim[before_queue]-mp[before_queue].size();
                int sent1;
                if(quantum_time[curr_queue]<num_of_starved+1){
                    sent1=num_of_starved+1-quantum_time[curr_queue];
                }
                else{
                    sent1=num_of_starved/3;       //if less then still send the num_of_starved/3 or 50%
                }
                int sent=min(space/3,sent1);      //if the space up is less then dont send all the starved
                while(sent--){
                    mp[curr_queue].front().arrival_time=curr_time;
                    mp[curr_queue].front().wait_time=next_wait[curr_queue];
                    mp[before_queue].push_back(mp[curr_queue].front());
                    mp[curr_queue].pop_front();
                    sent1--;
                }
                if(sent1){
                    curr_starved_bool[curr_queue]=1;
                }
            }
            else{
                curr_starved_bool[curr_queue]=0;
            }
        curr_queue=mp_next_curr_queue[curr_queue].first;
    }
}

int binary(int l,int r,deque<Packet> &dq,int specific_time){
    int start=l;
    int end=r;
    int ans=-1;
    while(start<=end){
        int mid=(end+start)/2;
        if(curr_time-dq[mid].arrival_time>=specific_time){
            ans=mid;
            start=mid+1;
        }
        else{
            end=mid-1;
        }
    }
    return ans;
}
int receiver(vector<frames> f,device &b,device &s,int last,deque<int> &d){
    int first=f[0].seq_num;
    if(!b.ACK[s.id].empty()){
        first=b.ACK[s.id].front();
    }
    for(auto i:f){
        if(!b.ACK[s.id].empty()){
            if((i.seq_num%N)==b.ACK[s.id].front()){
                b.ACK[s.id].pop_front();
            }
        }
        b.check[s.id][(i.seq_num)%N]=1;
        b.v[s.id].push_back(i.data);
    }
    d=b.ACK[s.id];
    int a=window_size_arq;
    int i=first%N;
    int flag=0;
    if(last-first<=window_size_arq){
        a=last-first+1;
    }
    while(a--){
        if(b.check[s.id][i]==0){
            b.ACK[s.id].push_back(i);
            flag=1;
        }
        i++;
        i%=N;
    }
    int j=first%N;
    int c=30;
    if(j==0){
        j=59;
    }
    else{
        j--;
    }
    while(c--){
        b.check[s.id][j]=0;
        if(j==0){
            j=59;
        }
        else{
            j--;
        }
    }
    if(flag){
        return 0;
    }
    return 1;
}
int sender(device &b,device &s,int expected,int gen,int last){
    int gen_seq=gen+expected;                     //expected is the current window start of the receiver
    vector<frames> f;                             //gen is the first gen seq num
    for(int i=1;i<=min(30,last-gen_seq+1);i++){
        int a=(gen_seq%1000);
        if(b.check[s.id][a]==0){
            string s1=s.data_to_be_sent[b.id][a];
            int id=gen_seq;
            frames f1(id,s1);
            f.push_back(f1);
        }
                gen_seq++;
    }




}
void selective_ARQ(Packet &p){
    // id = i;
    // sequence_no=no;
    // priority = p;
    // device=node;
    // wait_time = specific_wait;
    // arrival_time=arrival_time_in;
    // no_packets=packets;
    // receiver=get;
    int to_node=p.receiver;
    int allowed=min(window_size_arq,p.no_packets);
    int seq=p.sequence_no;
    int i=0;
    while(i!=allowed){
        int seq1=i+seq;
        seq1%N;
        i++;
    }

}
int main(){

    //SELECTIVE ARQ sender and connection
    //package delivery intialization from multi queue
}