#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <sstream>
#include <functional>
#include <limits>

// --- Data Structures ---

struct Event {
    int id;
    long long start;
    long long end;
    long long attendance;
    long long revenue;
    std::string venue;
    std::vector<int> deps;

    // Helper for printing
    std::string toString() const {
        std::stringstream ss;
        ss << "ID " << id 
           << " [" << start << "-" << end << "] "
           << "Venue: " << venue
           << " | Att: " << attendance 
           << " | Rev: " << revenue;
        if (!deps.empty()) {
             ss << " | Deps: ";
             for (size_t i = 0; i < deps.size(); ++i) {
                 ss << deps[i] << (i < deps.size() - 1 ? ", " : "");
             }
        }
        return ss.str();
    }
};

enum Objective {
    ATTENDANCE = 1,
    REVENUE = 2,
    HYBRID = 3
};

struct ScheduleResult {
    long long total_score;
    std::vector<Event> chosen_events;
    std::string error_message;
    bool success;
};

// --- Main Scheduler Class ---

class DynamicScheduler {
private:
    // Parameters for Hybrid Objective (now encapsulated)
    double alpha_;
    double maxAtt_;
    double maxRev_;
    const std::vector<Event>* events_; // Pointer to current event set
    std::unordered_map<int, int> idToIdx_; // Map Event ID to vector index

    /**
     * @brief Calculates the weighted score for a single event.
     * @param e The event.
     * @param obj The optimization objective.
     * @return A scaled long long score.
     */
    long long calculateWeight(const Event& e, Objective obj) const {
        if (obj == ATTENDANCE) return e.attendance;
        if (obj == REVENUE) return e.revenue;

        // Hybrid objective requires proper floating-point normalization
        double normAtt = (maxAtt_ > 0) ? (double)e.attendance / maxAtt_ : 0.0;
        double normRev = (maxRev_ > 0) ? (double)e.revenue / maxRev_ : 0.0;

        // Calculate hybrid score, scaled up by a large factor to work with long long DP
        double score = alpha_ * normAtt + (1.0 - alpha_) * normRev;

        // Scale by 10^6 for precision when casting to long long
        return static_cast<long long>(score * 1e6); 
    }

    /**
     * @brief Performs Topological Sort to validate dependencies and provide a processing order.
     * @param adj Adjacency list for dependencies (dep -> dependent).
     * @param indegree Indegree map for all event IDs.
     * @param topo Output vector for the topological order.
     * @return true if no cycle is found, false otherwise.
     */
    bool topoSort(const std::unordered_map<int, std::vector<int>>& adj, 
                  std::unordered_map<int, int> indegree, 
                  std::vector<int>& topo) {
        std::queue<int> q;
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

    /**
     * @brief Pre-processes events to build the dependency graph and validate constraints.
     */
    ScheduleResult validateAndBuildGraph(std::unordered_map<int, std::vector<int>>& adj, 
                                        std::unordered_map<int, int>& indegree, 
                                        std::vector<int>& topo) {
        
        const auto& events = *events_;
        std::unordered_set<int> allIds;

        for (const auto& e : events) {
            allIds.insert(e.id);
        }

        for (const auto& e : events) {
            indegree[e.id] = 0;
            adj[e.id];
        }

        for (const auto& e : events) {
            for (int depId : e.deps) {
                if (allIds.find(depId) == allIds.end()) {
                    return {0, {}, "Error: Event ID " + std::to_string(e.id) + " depends on unknown ID " + std::to_string(depId), false};
                }
                // Build adjacency list: Dependency -> Dependent
                adj[depId].push_back(e.id);
                indegree[e.id]++;
            }
        }

        // 1. Cycle Check (O(V+E))
        if (!topoSort(adj, indegree, topo)) {
            return {0, {}, "Error: Dependency cycle detected. The schedule cannot be created.", false};
        }

        // 2. Time Constraint Check (O(E))
        for (const auto& e : events) {
            for (int depId : e.deps) {
                const auto& depEvent = events[idToIdx_.at(depId)];
                if (depEvent.end > e.start) {
                    std::stringstream err;
                    err << "Error: Dependency time mismatch. Event " << e.id 
                        << " starts at " << e.start << " before its dependency " 
                        << depId << " ends at " << depEvent.end;
                    return {0, {}, err.str(), false};
                }
            }
        }
        return {0, {}, "", true}; // Success
    }
    
    /**
     * @brief The core DP function for scheduling with venue conflicts AND dependencies.
     * Time Complexity: O(N log N) + O(N * (log M + D)) where N=events, M=events/venue, D=dependencies/event.
     * The DP iterates over events in topological order.
     */
    ScheduleResult planEventsWithDependencies(Objective obj, const std::vector<int>& topo) {
        const auto& events = *events_;
        
        // 1. Group and sort events by venue for efficient conflict lookup (O(N log N))
        std::unordered_map<std::string, std::vector<const Event*>> eventsByVenue;
        for (const auto& e : events) {
            eventsByVenue[e.venue].push_back(&e);
        }
        for (auto& pair : eventsByVenue) {
            std::sort(pair.second.begin(), pair.second.end(), [](const Event* a, const Event* b) {
                return a->end < b->end;
            });
        }

        // DP state: dp[event_id] = max score achievable ending *with* event_id
        std::unordered_map<int, long long> dp;
        // Parent state: parent[event_id] = the ID of the best previous non-conflicting event in the same venue
        std::unordered_map<int, int> parent;

        long long maxTotalScore = 0;
        int lastEventId = -1;

        // Iterate in Topological Order (ensuring dependencies are processed before dependents)
        for (int eventId : topo) {
            const Event& currentEvent = events[idToIdx_.at(eventId)];
            long long currentScore = calculateWeight(currentEvent, obj);

            // 1. Find the best non-conflicting event score in the same venue (Venue Conflict DP)
            long long bestPreviousVenueScore = 0;
            int bestPreviousVenueId = -1;

            const auto& venueEventsPtrs = eventsByVenue.at(currentEvent.venue);
            
            // O(log M) optimization: Use upper_bound to find the latest event that finishes before currentEvent starts.
            // Note: Since venueEventsPtrs is sorted by end time, we look for the first event whose end time 
            // is >= currentEvent.start. The element before that iterator is the best compatible event.
            auto it = std::upper_bound(venueEventsPtrs.begin(), venueEventsPtrs.end(), &currentEvent, 
                [](const Event* search_target, const Event* event_in_list) {
                    return search_target->start > event_in_list->end; 
                });

            // If we found a compatible event and it's not the current event itself
            if (it != venueEventsPtrs.begin()) {
                const Event* prevEvent = *(--it);
                
                // Only consider the previous event if its score is in the DP table (meaning it was chosen)
                if (dp.count(prevEvent->id)) {
                    bestPreviousVenueScore = dp.at(prevEvent->id);
                    bestPreviousVenueId = prevEvent->id;
                }
            }
            
            currentScore += bestPreviousVenueScore;
            parent[eventId] = bestPreviousVenueId;

            // 2. Incorporate Scores of Dependent Events (Dependency Constraint)
            // A simple model: The full score of the dependent schedule is added *if* the dependent event is chosen.
            // Since we must choose the dependency if we choose the dependent, we enforce that here.
            for (int depId : currentEvent.deps) {
                // Since we are iterating in topological order, depId's score should be computed already
                // If a dependency is chosen, its maximal score must contribute to the current event's score.
                if (dp.count(depId)) { 
                    currentScore += dp.at(depId);
                }
            }

            dp[eventId] = currentScore;
            
            // Update global best score
            if (currentScore > maxTotalScore) {
                maxTotalScore = currentScore;
                lastEventId = eventId;
            }
        }

        // Reconstruction (O(N+E))
        std::vector<Event> chosen_schedule;
        std::unordered_set<int> visited;
        
        std::function<void(int)> reconstruct = [&](int id) {
            if (id == -1 || visited.count(id)) return;
            visited.insert(id);
            
            const Event& currentEvent = events[idToIdx_.at(id)];
            
            // Reconstruct path from the *venue conflict* parent
            if (parent.count(id)) {
                reconstruct(parent.at(id));
            }
            
            // Reconstruct all *dependency* parents (must be included)
            for (int depId : currentEvent.deps) {
                reconstruct(depId);
            }

            chosen_schedule.push_back(currentEvent);
        };
        
        reconstruct(lastEventId);
        
        // Final schedule presentation (sorted by end time)
        std::sort(chosen_schedule.begin(), chosen_schedule.end(), [](const Event& a, const Event& b) {
            return a.end < b.end;
        });

        return {maxTotalScore, chosen_schedule, "", true};
    }

    /**
     * @brief Calculates max attendance and revenue across all events for normalization.
     */
    void calculateNormalizationBounds(const std::vector<Event>& events) {
        maxAtt_ = 1.0;
        maxRev_ = 1.0;
        for (const auto& e : events) {
            maxAtt_ = std::max(maxAtt_, (double)e.attendance);
            maxRev_ = std::max(maxRev_, (double)e.revenue);
        }
    }

public:
    DynamicScheduler(double alpha = 0.5) : alpha_(alpha), maxAtt_(1.0), maxRev_(1.0) {}

    void setAlpha(double alpha) { alpha_ = alpha; }

    /**
     * @brief Public interface to run the scheduling process.
     */
    ScheduleResult run(const std::vector<Event>& events, Objective obj) {
        if (events.empty()) {
            return {0, {}, "No events provided.", true};
        }
        
        events_ = &events;
        calculateNormalizationBounds(events);

        // Pre-processing to build ID map (O(N))
        idToIdx_.clear();
        for (size_t i = 0; i < events.size(); i++) {
            idToIdx_[events[i].id] = i;
        }

        // Dependency Graph Check (O(V+E))
        std::unordered_map<int, std::vector<int>> adj;
        std::unordered_map<int, int> indegree;
        std::vector<int> topo;

        ScheduleResult validationResult = validateAndBuildGraph(adj, indegree, topo);
        if (!validationResult.success) {
            return validationResult;
        }

        // Run the core DP algorithm (O(N log N) overall)
        return planEventsWithDependencies(obj, topo);
    }
};

// --- Main Program and Utilities ---

void printSchedule(const ScheduleResult& result) {
    long long totalAtt = 0, totalRev = 0;
    
    std::cout << "\n --- Selected Events (by finish time) ---\n";
    for (const auto& e : result.chosen_events) {
        std::cout << " - " << e.toString() << "\n";
        totalAtt += e.attendance;
        totalRev += e.revenue;
    }
    std::cout << "---------------------------------------------------\n";
    std::cout << " TOTALS -> Attendance: " << totalAtt 
              << " | Revenue: " << totalRev << "\n";
}

int main() {
    // Optimization: Disable synchronization with C stdio and flush output buffers 
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    int mode;
    std::cout << "Dynamic Event Scheduler\n";
    std::cout << "Choose mode:\n";
    std::cout << " 1 -> Manual Input\n";
    std::cout << " 2 -> Run Test Case\n";
    std::cout << "Enter choice: ";
    if (!(std::cin >> mode)) return 1;

    std::vector<Event> events;

    if (mode == 1) {
        int n;
        std::cout << "Enter the number of events: ";
        if (!(std::cin >> n)) return 1;
        events.resize(n);

        for (int i = 0; i < n; i++) {
            events[i].id = i + 1;
            std::cout << "\n--- Event " << i + 1 << " (ID " << i + 1 << ") ---\n";
            std::cout << "Enter start time     : "; std::cin >> events[i].start;
            std::cout << "Enter end time       : "; std::cin >> events[i].end;
            std::cout << "Enter attendance     : "; std::cin >> events[i].attendance;
            std::cout << "Enter revenue        : "; std::cin >> events[i].revenue;
            std::cout << "Enter venue (no spaces): "; 
            std::string venue_str;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear buffer
            std::getline(std::cin, venue_str);
            // Quick fix for the venue read:
            if (venue_str.empty()) std::cin >> events[i].venue; 
            else events[i].venue = venue_str;
            
            std::cout << "Enter dependencies (space-separated IDs, 0 to end): ";
            int depId;
            while (std::cin >> depId && depId != 0) {
                events[i].deps.push_back(depId);
            }
        }
    } else if (mode == 2) {
        std::cout << "\nRunning Predefined Test Case (Includes Dependencies & Conflicts)...\n";
        events = {
            // ID, Start, End, Att, Rev, Venue, Dependencies
            {1, 1, 3, 100, 50, "HallA", {}},
            {2, 2, 4, 120, 60, "HallA", {}}, // Conflicts with 1
            {3, 5, 7, 150, 80, "HallB", {}},
            {4, 8, 9, 200, 100, "HallB", {3}}, // Depends on 3
            {5, 6, 8, 180, 90, "HallA", {1}}, // Depends on 1, Conflicts with 2
            {6, 9, 11, 220, 110, "HallC", {4}} // Depends on 4
        };
    } else {
        std::cout << "Invalid choice.\n";
        return 1;
    }

    int choice;
    std::cout << "\nChoose Objective:\n";
    std::cout << " 1 -> Maximize Attendance\n";
    std::cout << " 2 -> Maximize Revenue\n";
    std::cout << " 3 -> Hybrid\n";
    std::cout << "Enter choice: ";
    if (!(std::cin >> choice)) return 1;

    Objective obj;
    double alpha = 0.5;

    if (choice == 1) obj = ATTENDANCE;
    else if (choice == 2) obj = REVENUE;
    else if (choice == 3) {
        obj = HYBRID;
        std::cout << "Enter the value of alpha [0-1] (Higher alpha <=> Prioritize Attendance) : ";
        if (!(std::cin >> alpha)) return 1;
    } else {
        std::cout << "Invalid objective choice.\n";
        return 1;
    }

    DynamicScheduler scheduler(alpha);
    ScheduleResult result = scheduler.run(events, obj);

    if (!result.success) {
        std::cout << "\nSCHEDULING FAILED: " << result.error_message << "\n";
        return 1;
    }

    std::cout << "\n Maximum Score Achievable (Scaled by 1e6 for Hybrid): " << result.total_score << "\n";
    printSchedule(result);

    return 0;
}
