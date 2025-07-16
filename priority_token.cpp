#include<bits/stdc++.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
using namespace std;
mutex queue_mutex;                      // Protects access to shared queues
atomic<bool> stop_threads(false);       // Signal to stop threads
int all_sent = 0;
int total;
atomic<int> curr_time = 0;//curr time
string curr_queue="H";  //"H": high, "M1": medium1, "M2": medium2, "L": low;
int high_time_quantum=60;   //high's time quantum or no.of packets in a round    //shifts to 80
int med1_time_quantum=35; //same as high time quantum but for M1,M2,L  //shifts to 50
int med2_time_quantum=18;   // shifts to 31
int low_time_quantum=7;    //shifts to 18
int window_size_arq=30;
int high_wait=80;
int medium1_wait=100;
int medium2_wait=220;
int low_wait=300;
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
    int device1;
    int last;
    int no_packets;
    int sequence_no;
    int retry_count;
    int receiver;
    Packet(int i, int p, int specific_wait, int arrival_time_in,int packets,int no,int node,int get,int last1,int re) {
        id = i;
        sequence_no=no;
        priority = p;
        device1=node;
        wait_time = specific_wait;
        arrival_time=arrival_time_in;
        no_packets=packets;
        receiver=get;
        last=last1;
        retry_count=re;
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
int MAX_DEVICES=5;
class device{
    public:
    int id;
    vector<vector<string>> data_to_be_sent = vector<vector<string>>(MAX_DEVICES);
    vector<vector<string>> v = vector<vector<string>>(MAX_DEVICES);
    vector<vector<int>> check = vector<vector<int>>(MAX_DEVICES,vector<int> (112,0));
    vector<deque<int>> ACK = vector<deque<int>>(MAX_DEVICES);
    vector<set<int>> ACK_set = vector<set<int>>(MAX_DEVICES);
    device(int id){
        this->id=id;
    }
};
set<pair<int,int>> already_dropped_once;
deque<Packet> high;
deque<Packet> medium_1;
deque<Packet> medium_2;
deque<Packet> low;

vector<device> devices;

map<string,deque<Packet>*> mp = {{"H", &high},{"M1", &medium_1},{"M2", &medium_2},{"L", &low}};  //mapping of the vector as per the curr_queue
map<string,pair<string,string>> mp_next_curr_queue={{string("H"),{string("M1"),string("L")}},{string("M1"),{string("M2"),string("H")}},{string("M2"),{string("L"),string("M1")}},{string("L"),{string("H"),string("M2")}}};
map<string,int> quantum_time={{string("H"),high_time_quantum},{string("M1"),med1_time_quantum},{string("M2"),med2_time_quantum},{string("L"),low_time_quantum}};
map<string,int> starve_time_lim={{string("H"),80},{string("M1"),100},{string("M2"),220},{string("L"),300}};
map<string,int> deque_size_lim={{string("H"),170},{string("M1"),280},{string("M2"),440},{string("L"),600}};
map<string,int> shifted_quantum={{string("H"),high_time_quantum+20},{string("M1"),med1_time_quantum+15},{string("M2"),med2_time_quantum+13},{string("L"),low_time_quantum+11}};
map<string,int> curr_starved_bool={{string("H"),0},{string("M1"),0},{string("M2"),0},{string("L"),0}};
map<string,int> next_wait={{string("M1"),high_wait},{string("M2"),medium1_wait},{string("L"),medium2_wait}};
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
void Multilevel_queue_push(Packet &pack){
    if(pack.priority>=high_priority_range.first && pack.priority<=high_priority_range.second){
        mp["H"]->push_back(pack);
    }
    else if(pack.priority>=medium1_priority_range.first && pack.priority<=medium1_priority_range.second){
        mp["M1"]->push_back(pack);
    }
    else if(pack.priority>=medium2_priority_range.first && pack.priority<=medium2_priority_range.second){
        mp["M2"]->push_back(pack);
    }
    else{
        mp["L"]->push_back(pack);
    }
    return ;
}
int receiver(vector<frames> &f,device &b,device &s,int last,int &got,int &g){
    int first=f[0].seq_num;
    if(!b.ACK[s.id].empty()){
        first=b.ACK[s.id].front();
    }
    for(auto i:f){
        if(i.seq_num%N==4 && b.ACK[s.id].empty() && !already_dropped_once.count({s.id, b.id})){
            cout<<"MISSED frame "<<i.seq_num<<" on purpose---------------------"<<endl;
            already_dropped_once.insert({s.id, b.id});
            continue;
        }
        cout<<"received frame "<<i.seq_num<<" from "<<s.id<<" to "<<b.id<<" data: "<<i.data<<endl;
        if(!b.ACK[s.id].empty()){
            if((i.seq_num)==b.ACK[s.id].front()){
                cout<<"received the dropped frame "<<i.seq_num<<"-----------"<<endl;
                b.ACK_set[s.id].erase(b.ACK[s.id].front());
                b.ACK[s.id].pop_front();
            }
        }
        b.check[s.id][i.seq_num]=1;
        b.v[s.id].push_back(i.data);
        got++;
    }
    int i=first;
    int flag=0;
   int a = f.back().seq_num - first + 1;  // Only check the range that was just received
if (a > window_size_arq) a = window_size_arq;
    while(a--){
        if(b.check[s.id][i]==0){
            if(!b.ACK_set[s.id].count(i)){
                cout << "[Receiver " << b.id << "] Frame " << i << " was NOT received. Asking for retransmission from Sender " << s.id << endl;
                b.ACK[s.id].push_back(i);
                b.ACK_set[s.id].insert(i);
                flag=1;
            }
        }
        i++;
    }
    g=first;
    if(flag){
        return 0;
    }
    return 1;
}
void sender(Packet &pack, device &b, device &s, int expected, int gen, int last) {
    if (pack.retry_count > 5) {
        return;
    }

    int gen_seq1 = gen;  // starting sequence number
    vector<frames> f;
    int gen_seq = gen_seq1;
    cout<<last-gen+1<<endl;
    for (int i = 0; i < min(30, last-gen+1); i++) {
        if (b.check[s.id][gen_seq] == 0) {
            string s1 = s.data_to_be_sent[b.id][gen_seq];
            frames f1(gen_seq, s1);
            f.push_back(f1);
        }
        gen_seq++;
    }

    if (f.empty()) {
        cout << "[Sender " << s.id << "] No frames to send to " << b.id << endl;
        return;
    }

    int got = 0;
    int first;
    int a=receiver(f, b, s, last, got, first);
    if (a == 0) {
        pack.arrival_time = curr_time.load();
        pack.sequence_no = b.ACK[s.id].front() ;  // retry from missing
        pack.no_packets -= got;
        pack.retry_count++;
        pack.last=last;
        {
            cout <<"RETRANSMITTED"<<endl;
            lock_guard<mutex> lock(queue_mutex);
            Multilevel_queue_push(pack);
        }
    } else {
        if (last - gen_seq > 0) {
            pack.arrival_time = curr_time.load();
            pack.sequence_no = gen_seq ;
            pack.no_packets -= got;
            pack.retry_count++;
            pack.last=last;
            {
            lock_guard<mutex> lock(queue_mutex);
            Multilevel_queue_push(pack);
             cout <<"RETRANSMITTED"<<endl;
            }
        }
    }
}

void selective_ARQ(Packet &p){
    device &b=devices[p.receiver];
    device &s=devices[p.device1];
    sender(p,b,s,0,p.sequence_no,p.last);
}
void Multilevel_feedback_queue(){
        cout << "[INFO] Entered Multilevel Feedback Queue handler." << endl;
    while(!high.empty() || !medium_1.empty() || !medium_2.empty() || !low.empty()){
        if (mp[curr_queue]->empty()) {
            curr_queue = mp_next_curr_queue[curr_queue].first;
            continue;
        }
        int quantum;
        int starved_last=curr_starved_bool[curr_queue];
        if(mp[curr_queue]->size()>deque_size_lim[curr_queue] || starved_last){
            quantum=shifted_quantum[curr_queue];
        }
        else{
            quantum=quantum_time[curr_queue];
        }
        while(quantum && !mp[curr_queue]->empty()){                  //got the shared cable access

            Packet p=mp[curr_queue]->front();
            {
                lock_guard<mutex> lock(queue_mutex);
                mp[curr_queue]->pop_front();
            }
            selective_ARQ(p);
            cout<<high.size()<<" "<<medium_1.size()<<" "<<low.size()<<endl;
    cout<<medium_2.size()<<endl;
            quantum--;
            curr_time.fetch_add(1);
      //counts time
        }                                       //this is forwarding
            string before_queue=mp_next_curr_queue[curr_queue].second;
            int num_of_starved=binary(0,mp[curr_queue]->size()-1,*mp[curr_queue],starve_time_lim[curr_queue]);
            if(num_of_starved!=-1 && curr_queue!="H"){
                int space=deque_size_lim[before_queue]-mp[before_queue]->size();
                int sent1;
                if(quantum_time[curr_queue]<num_of_starved+1){
                    sent1=num_of_starved+1-quantum_time[curr_queue];
                }
                else{
                    sent1=num_of_starved/3;       //if less then still send the num_of_starved/3 or 50%
                }
                int sent=min(space/3,sent1);      //if the space up is less then dont send all the starved
                while(sent--){
                    mp[curr_queue]->front().arrival_time=curr_time;
                    mp[curr_queue]->front().wait_time=next_wait[curr_queue];
                    {
                        lock_guard<mutex> lock(queue_mutex);
                        mp[before_queue]->push_back(mp[curr_queue]->front());
                        mp[curr_queue]->pop_front();
                    }
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

void device_thread(int device_id) {
    set<int> st;
    while (!stop_threads) {
        this_thread::sleep_for(chrono::milliseconds(10 + rand() % 10)); // Simulate random delay

        int recv = rand() % devices.size();
        if(st.count(recv)){
            continue;
        }
        st.insert(recv);
        if (recv == device_id) continue; // Avoid self-send

        int priority = rand() % 101;
        int wait1=0;
        if(priority>=high_priority_range.first && priority<=high_priority_range.second){
            wait1=high_wait;
        }
        else if(priority>=medium1_priority_range.first && priority<=medium1_priority_range.second){
            wait1=medium1_wait;
        }
        else if(priority>=medium2_priority_range.first && priority<=medium2_priority_range.second){
            wait1=medium2_wait;
        }
        else{
            wait1=low_wait;
        }
       int hf=0;
        Packet p(rand() % 10000,  // id
                 priority,                          // priority
                 wait1,                                 // wait_time (set by queue)
                 curr_time.load(),                  // arrival_time
                 112,                               // total packets
                hf,                                 // starting sequence number
                 device_id,                         // sender
                 recv,                              // receiver
                 hf+111,                               // last
                 0);                                // retry_count

        {
            lock_guard<mutex> lock(queue_mutex);
            Multilevel_queue_push(p);
            cout << "[Device " << device_id << "] pushed packet to queue with priority " << priority << endl;
        }
    }
}
int main(){
    vector<string> words = {
        "apple", "banana", "cherry", "date", "elderberry", "fig", "grape", "honeydew",
        "kiwi", "lemon", "mango", "nectarine", "orange", "papaya", "quince", "raspberry",
        "strawberry", "tangerine", "ugli", "vanilla", "watermelon", "xigua", "yam", "zucchini",
        "ant", "bat", "cat", "dog", "elephant", "fox", "goat", "horse",
        "iguana", "jaguar", "kangaroo", "lion", "monkey", "newt", "owl", "penguin",
        "quail", "rabbit", "snake", "tiger", "urchin", "vulture", "wolf", "xerus",
        "yak", "zebra", "red", "blue", "green", "yellow", "purple", "black",
        "white", "orange", "brown", "gray", "pink", "cyan", "magenta", "gold",
        "silver", "bronze", "chrome", "neon", "jade", "ivory", "navy", "olive",
        "plum", "salmon", "teal", "turquoise", "amber", "beige", "coral", "crimson",
        "maroon", "peach", "rose", "ruby", "sapphire", "scarlet", "tan", "topaz",
        "aqua", "azure", "blush", "charcoal", "emerald", "indigo", "lavender", "lime",
        "mint", "ochre", "periwinkle", "rust", "sepia", "slate", "snow", "steel",
        "sunset", "taupe", "umber", "vermilion", "violet", "wheat", "alabaster", "amethyst"
    };
    int dev=0;
    while(dev!=MAX_DEVICES){
        device d=device(dev);
        for(int i=0;i<MAX_DEVICES;i++){
                for(int j=0;j<words.size();j++){
                    d.data_to_be_sent[i].push_back(words[j]+"_"+to_string(dev)+"->"+to_string(i));
                }
        }
        devices.push_back(d);
        dev++;
    }
     vector<thread> device_threads;
    for (int i = 0; i < MAX_DEVICES; ++i) {
        device_threads.emplace_back(device_thread, i);
    }

    // Main thread runs the shared cable controller
    this_thread::sleep_for(chrono::seconds(5));
    stop_threads = true;  

    for (auto &t : device_threads) t.join();  
    while (true) {
        {
            lock_guard<mutex> lock(queue_mutex);
            if (high.empty() && medium_1.empty() && medium_2.empty() && low.empty()) {
                break;
            }
        }
        Multilevel_feedback_queue();
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    cout << "Simulation completed."<<endl;
    cout<<high.size()<<" "<<medium_1.size()<<" "<<low.size()<<endl;
    cout<<medium_2.size()<<endl;
    return 0;
}
