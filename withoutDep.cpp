#include <bits/stdc++.h>
using namespace std;

struct Event {
    int id;
    long long start;
    long long end;
    long long attendance;
    long long revenue;
    string venue;
    vector<int> deps;

};

enum Objective {
    ATTENDANCE = 1,
    REVENUE = 2,
    HYBRID = 3
};

bool byEnd(const Event& a, const Event& b) {
    if (a.end != b.end) return a.end < b.end;
    return a.start < b.start;
}

int upperBound(vector<long long>& arr, long long target) {
    int low = 0, high = arr.size() - 1, ans = -1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (arr[mid] >= target) {
            high = mid - 1;
        } else {
            ans = mid;
            low = mid + 1;
        }
    }
    return ans;
}

vector<int> buildPrevCompatible(const vector<Event>& ev) {
    int n = ev.size();
    vector<int> p(n, -1);
    for (int i = 0; i < n; i++) {
        for (int j = i - 1; j >= 0; j--) {
            if (ev[j].venue == ev[i].venue && ev[j].end <= ev[i].start) {
                p[i] = j;
                break;
            }
        }
    }
    return p;
}

double alpha_global=0.5;
double maxAtt=1,maxRev=1;

long long weight(const Event& a, Objective obj) {
    if(obj==ATTENDANCE) return a.attendance;
    if(obj==REVENUE) return a.revenue;

    int normAtt= a.attendance/maxAtt;
    int normRev= a.revenue/maxRev;
    int score=alpha_global*normAtt + (1-alpha_global)*normRev;
    return (long long) (score*1e6);
}

pair<long long, vector<int>> planEvents(vector<Event> ev, Objective obj) {
    sort(ev.begin(), ev.end(), byEnd);
    int n = ev.size();
    if (n == 0) return {0, {}};

    vector<int> p = buildPrevCompatible(ev);

    vector<long long> dp(n, 0);
    vector<int> taken(n, 0);
    dp[0] = weight(ev[0], obj);
    taken[0] = 1;

    for (int i = 1; i < n; i++) {
        long long include = weight(ev[i], obj);
        if (p[i] != -1) include += dp[p[i]];
        long long exclude = dp[i - 1];

        if (include > exclude) {
            dp[i] = include;
            taken[i] = 1;
        } else {
            dp[i] = exclude;
        }
    }

    vector<int> chosen;
    for (int i = n - 1; i >= 0;) {
        if (taken[i] != 0) {
            chosen.push_back(ev[i].id);
            i = p[i];
        } else {
            i = i - 1;
        }
    }
    reverse(chosen.begin(), chosen.end());
    return {dp[n - 1], chosen};
}

bool topoSort(int n, vector<vector<int>>& adj, vector<int>& topo, vector<int>& cycleNodes){
    vector<int> indegree(n+1,0);
    for(int i=1;i<=n;i++){
        for(auto next:adj[i]) indegree[next]++;
    }
    queue<int> q;
    for(int i=1;i<=n;i++){
        if(indegree[i]==0) q.push(i);
    }
    topo.clear();
    while(!q.empty()){
        int node=q.front();
        q.pop();
        topo.push_back(node);
        for(auto neigh:adj[node]){
            indegree[neigh]--;
            if(indegree[neigh]==0) q.push(neigh);
        }
    }
    if(topo.size()!=n){
        for(int i=1;i<=n;i++){
            if(indegree[i]>0) cycleNodes.push_back(i);
        }
        return false;
    }
    return true;
}

bool validateDepsAndTimes(vector<Event>& events, vector<string>& errors){
    int n=events.size();
    unordered_map<int,int> IdtoIdx;
    for(int i=0;i<n;i++) IdtoIdx[events[i].id]=i;

    int maxId=0;
    for(auto &e:events) maxId=max(maxId,e.id);

    vector<vector<int>> adj(maxId+1);
    int nodeCount=0;
    vector<bool> exists(maxId+1,false);
    for(auto &e:events) {
        exists[e.id]=true; 
        nodeCount++;
    }
    for(auto &e:events){
        for(auto &d:e.deps){
            if(!exists[d]){
                errors.push_back("Event id " + to_string(e.id) + " depends on Unknown id " + to_string(d));
                return false;
            }
            adj[d].push_back(e.id);
        }
    }

    vector<int> topo,cycleNodes;
    if(!topoSort(maxId,adj,topo,cycleNodes)){
        string s="Dependency cycle detected among IDs : ";
        for(int i=0;i<cycleNodes.size();i++) s+=to_string(cycleNodes[i])+" ";
        errors.push_back(s);
        return false;
    }

    for(auto &e: events){
        for(auto &d:e.deps){
            if(events[IdtoIdx[d]].end>e.start){
                errors.push_back("Dependency time error : event " +to_string(e.id) + " depends on event " + to_string(d)+ " but starts at : "+to_string(e.start)+" before the end : " +to_string(events[IdtoIdx[d]].end)+" of its dependency");
                return false;
            }
        }
    }
    return true;
}

void printSchedule(const vector<Event>& ev, vector<int>& chosen) {
    unordered_set<int> st(chosen.begin(), chosen.end());
    vector<Event> schedule;
    for (auto& e : ev) {
        if (st.find(e.id) != st.end()) schedule.push_back(e);
    }
    sort(schedule.begin(), schedule.end(), byEnd);

    cout << "\n Selected Events (by finish time):\n";
    long long totalAtt = 0, totalRev = 0;
    for (auto& e : schedule) {
        cout << "  ID " << e.id 
             << " [" << e.start << "-" << e.end << "] "
             << "Venue: " << e.venue
             << "Attendance: " << e.attendance 
             << " | Revenue: " << e.revenue << "\n";
        totalAtt += e.attendance;
        totalRev += e.revenue;
    }
    cout << "---------------------------------------------------\n";
    cout << " Totals -> Attendance: " << totalAtt 
         << " | Revenue: " << totalRev << "\n";
}

int main() {
    int mode;
    cout << "Choose mode:\n";
    cout << " 1 -> Manual Input\n";
    cout << " 2 -> Run Test Case\n";
    cout << "Enter choice: ";
    cin >> mode;

    vector<Event> events;

    if (mode == 1) {
        int n;
        cout << "Enter the number of events: ";
        cin >> n;
        events.resize(n);

        for (int i = 0; i < n; i++) {
            events[i].id = i + 1;
            cout << "\n--- Event " << i + 1 << " ---\n";
            cout << "Enter start time   : ";
            cin >> events[i].start;
            cout << "Enter end time     : ";
            cin >> events[i].end;
            cout << "Enter capacity     : ";
            cin >> events[i].attendance;
            cout << "Enter revenue      : ";
            cin >> events[i].revenue;
            cout << "Enter venue      : ";
            cin >> events[i].revenue;
        }
    } 
    else if (mode == 2) {
        // Example test case
        cout << "\nRunning Predefined Test Case...\n";
        events = {
            {1, 1, 4, 100, 200},
            {2, 2, 6, 150, 250},
            {3, 5, 7, 120, 220},
            {4, 6, 9, 200, 300},
            {5, 8, 10, 180, 280}
        };
    }

    maxAtt=1;
    maxRev=1;
    for(auto &e:events){
        maxAtt=max(maxAtt,(double)e.attendance);
        maxRev=max(maxRev,(double)e.revenue);
    }

    int choice;
    cout << "\nChoose Objective:\n";
    cout << "  1 -> Maximize Attendance\n";
    cout << "  2 -> Maximize Revenue\n";
    cout << "  3 -> Hybrid\n";
    cout << "Enter choice: ";
    cin >> choice;

    Objective obj ;
    if(choice==1) obj=ATTENDANCE;
    else if (choice==2) obj=REVENUE;
    else obj=HYBRID;

    if(obj==HYBRID){
        cout << "Enter the value of alpha [0-1] (Higher alpha <=> Prioritize Attendance) : ";
        cin >> alpha_global;
    }

    auto plan = planEvents(events, obj);
    long long best = plan.first;
    vector<int> chosen = plan.second;

    cout << "\n Maximum " << ((obj == ATTENDANCE) ? "Attendance" : (obj==REVENUE)?"Revenue":"Hybrid Score")
         << " achievable: " << best << "\n";
    printSchedule(events, chosen);

    return 0;
}
