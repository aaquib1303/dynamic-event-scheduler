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

double alpha_global = 0.5;
double maxAtt = 1, maxRev = 1;

long long weight(const Event& a, Objective obj) {
    if (obj == ATTENDANCE) return a.attendance;
    if (obj == REVENUE) return a.revenue;

    double normAtt = (double)a.attendance / maxAtt;
    double normRev = (double)a.revenue / maxRev;

    double score = alpha_global * normAtt + (1.0 - alpha_global) * normRev;

    return static_cast<long long>(score * 1e6);
}

bool topoSort(const unordered_map<int, vector<int>>& adj, unordered_map<int, int> indegree, vector<int>& topo) {
    queue<int> q;
    for (auto const& pair : indegree) {
        if (pair.second == 0) {
            q.push(pair.first);
        }
    }

    topo.clear();
    while (!q.empty()) {
        int node = q.front();
        q.pop();
        topo.push_back(node);

        if (adj.count(node)) {
            for (int neighbor : adj.at(node)) {
                indegree.at(neighbor)--;
                if (indegree.at(neighbor) == 0) {
                    q.push(neighbor);
                }
            }
        }
    }
    
    return topo.size() == indegree.size();
}

vector<int> validateAndBuildGraph(const vector<Event>& events, vector<string>& errors, 
                           unordered_map<int, vector<int>>& adj, 
                           unordered_map<int, int>& indegree, 
                           unordered_map<int, int>& IdtoIdx) {
    IdtoIdx.clear();
    adj.clear();
    indegree.clear();
    unordered_set<int> allIds;

    for (int i = 0; i < events.size(); i++) {
        allIds.insert(events[i].id);
        IdtoIdx[events[i].id] = i;
    }

    for (const auto& e : events) {
        indegree[e.id] = 0;
        adj[e.id];
    }

    for (const auto& e : events) {
        for (const auto& d : e.deps) {
            if (allIds.find(d) == allIds.end()) {
                errors.push_back("Error: Event id " + to_string(e.id) + " depends on unknown id " + to_string(d));
                return {};
            }
            adj[d].push_back(e.id);
            indegree[e.id]++;
        }
    }

    vector<int> topo;
    if (!topoSort(adj, indegree, topo)) {
        errors.push_back("Error: Dependency cycle detected. The schedule cannot be created.");
        return {};
    }

    for (const auto& e : events) {
        for (const auto& d : e.deps) {
            if (events[IdtoIdx.at(d)].end > e.start) {
                errors.push_back("Error: Dependency time mismatch. Event " + to_string(e.id) +
                                 " starts at " + to_string(e.start) + " before its dependency " +
                                 to_string(d) + " ends at " + to_string(events[IdtoIdx.at(d)].end));
                return {};
            }
        }
    }
    return topo;
}

pair<long long, vector<int>> planEventsWithDependencies(const vector<Event>& events, Objective obj, 
                                                       const unordered_map<int, int>& IdtoIdx,
                                                       const unordered_map<int, vector<int>>& adj,
                                                       const unordered_map<int, int>& indegree,
                                                       const vector<int>& topo) {
    unordered_map<string, vector<Event>> eventsByVenue;
    for (const auto& e : events) {
        eventsByVenue[e.venue].push_back(e);
    }
    for (auto& pair : eventsByVenue) {
        sort(pair.second.begin(), pair.second.end(), [](const Event& a, const Event& b) {
            return a.end < b.end;
        });
    }

    map<int, long long> dp;
    map<int, int> parent;

    long long maxScore = 0;
    int lastEventId = -1;

    for (int eventId : topo) {
        const Event& currentEvent = events[IdtoIdx.at(eventId)];
        long long currentScore = weight(currentEvent, obj);

        long long bestPreviousScore = 0;
        int bestPreviousId = -1;

        const auto& venueEvents = eventsByVenue.at(currentEvent.venue);
        for (const auto& prevEvent : venueEvents) {
            if (prevEvent.end <= currentEvent.start && dp.count(prevEvent.id)) {
                if (dp.at(prevEvent.id) > bestPreviousScore) {
                    bestPreviousScore = dp.at(prevEvent.id);
                    bestPreviousId = prevEvent.id;
                }
            }
        }
        currentScore += bestPreviousScore;

        for (int depId : currentEvent.deps) {
            if (dp.count(depId)) {
                currentScore += dp.at(depId);
            }
        }

        dp[eventId] = currentScore;
        parent[eventId] = bestPreviousId;
        
        if (currentScore > maxScore) {
            maxScore = currentScore;
            lastEventId = eventId;
        }
    }

    vector<int> chosen;
    unordered_set<int> visited;
    
    function<void(int)> reconstruct = [&](int id) {
        if (id == -1 || visited.count(id)) return;
        visited.insert(id);
        
        chosen.push_back(id);
        
        const Event& currentEvent = events[IdtoIdx.at(id)];
        if (parent.count(id)) {
            reconstruct(parent.at(id));
        }
        
        for (int depId : currentEvent.deps) {
            reconstruct(depId);
        }
    };
    
    reconstruct(lastEventId);
    reverse(chosen.begin(), chosen.end());

    return {maxScore, chosen};
}

void printSchedule(const vector<Event>& ev, const vector<int>& chosen, const unordered_map<int, int>& IdtoIdx) {
    unordered_set<int> st(chosen.begin(), chosen.end());
    vector<Event> schedule;
    for (const auto& e : ev) {
        if (st.find(e.id) != st.end()) schedule.push_back(e);
    }
    sort(schedule.begin(), schedule.end(), [](const Event& a, const Event& b) {
        return a.end < b.end;
    });

    cout << "\n Selected Events (by finish time):\n";
    long long totalAtt = 0, totalRev = 0;
    for (const auto& e : schedule) {
        cout << "   ID " << e.id 
             << " [" << e.start << "-" << e.end << "] "
             << "Venue: " << e.venue
             << " | Attendance: " << e.attendance 
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
            cout << "Enter attendance   : ";
            cin >> events[i].attendance;
            cout << "Enter revenue      : ";
            cin >> events[i].revenue;
            cout << "Enter venue        : ";
            cin.ignore();
            getline(cin, events[i].venue);
            cout << "Enter dependencies (space-separated IDs, 0 to end): ";
            int depId;
            while (cin >> depId && depId != 0) {
                events[i].deps.push_back(depId);
            }
        }
    } else if (mode == 2) {
        cout << "\nRunning Predefined Test Case...\n";
        events = {
            {1, 1, 3, 100, 50, "Hall A", {}},
            {2, 2, 4, 120, 60, "Hall A", {}},
            {3, 5, 7, 150, 80, "Hall B", {}},
            {4, 8, 9, 200, 100, "Hall B", {3}},
            {5, 6, 8, 180, 90, "Hall A", {}},
            {6, 9, 11, 220, 110, "Hall C", {1, 5}}
        };
    }

    vector<string> errors;
    unordered_map<int, vector<int>> adj;
    unordered_map<int, int> indegree;
    unordered_map<int, int> IdtoIdx;
    
    vector<int> topo = validateAndBuildGraph(events, errors, adj, indegree, IdtoIdx);
    if (topo.empty() && !errors.empty()) {
        cout << "\nScheduling cannot proceed due to the following errors:\n";
        for (const auto& error : errors) {
            cout << " - " << error << "\n";
        }
        return 1;
    }

    maxAtt = 1;
    maxRev = 1;
    for (const auto& e : events) {
        maxAtt = max(maxAtt, (double)e.attendance);
        maxRev = max(maxRev, (double)e.revenue);
    }

    int choice;
    cout << "\nChoose Objective:\n";
    cout << "  1 -> Maximize Attendance\n";
    cout << "  2 -> Maximize Revenue\n";
    cout << "  3 -> Hybrid\n";
    cout << "Enter choice: ";
    cin >> choice;

    Objective obj;
    if (choice == 1) obj = ATTENDANCE;
    else if (choice == 2) obj = REVENUE;
    else obj = HYBRID;

    if (obj == HYBRID) {
        cout << "Enter the value of alpha [0-1] (Higher alpha <=> Prioritize Attendance) : ";
        cin >> alpha_global;
    }

    auto plan = planEventsWithDependencies(events, obj, IdtoIdx, adj, indegree, topo);
    long long best = plan.first;
    vector<int> chosen = plan.second;

    cout << "\n Maximum " << ((obj == ATTENDANCE) ? "Attendance" : (obj == REVENUE) ? "Revenue" : "Hybrid Score")
         << " achievable: " << best << "\n";
    printSchedule(events, chosen, IdtoIdx);

    return 0;
}
