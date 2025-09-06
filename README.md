# Dynamic Event Scheduler

## 📌 Project Overview
This project is a **dynamic event scheduler** developed in **C++**.  
It solves the problem of finding the optimal schedule of events from a given list, considering three primary objectives:  

- **Maximize Attendance**  
- **Maximize Revenue**  
- **Hybrid Objective** (weighted combination of both)  

Unlike simple scheduling problems, this scheduler also incorporates **real-world constraints**:

- **Multiple Venues**: Events in different venues can run simultaneously.  
- **Dependencies**: Some events require one or more prerequisite events to be scheduled first.  

The solution leverages **graph theory** and **dynamic programming** to ensure correct and efficient results.

---

## ⚙️ Core Algorithms

### 1. Graph Representation
- Events are represented as nodes in a **Directed Acyclic Graph (DAG)**.  
- Dependencies are modeled as directed edges (`A → B` means **A must happen before B**).  

This structure allows us to:
- **Detect Cycles** → ensure no circular dependencies (invalid input).  
- **Determine Order** → establish a valid scheduling order.  

---

### 2. Topological Sort
We apply **Kahn’s Algorithm** to obtain a valid ordering of events.  
This guarantees that every event is processed **after all its prerequisites**.  

---

### 3. Dynamic Programming
The scheduler extends the classic **Weighted Interval Scheduling** algorithm.  

For each event:
- **Dependencies** → If an event is chosen, all its prerequisites are also included automatically.  
- **Venue Conflicts** → Only one event per venue can occur at the same time; overlapping events in the same venue are excluded.  
- **Objective Function** → Depending on the user’s choice, we maximize attendance, revenue, or a weighted hybrid score.

---

### 4. Hybrid Scoring (Multi-Objective Optimization)
To balance attendance and revenue, we define a **hybrid objective**:

Score = α * normalizedAttendance + (1 – α) * normalizedRevenue


Where:
- `α ∈ [0, 1]` is user-provided (e.g., `--alpha=0.7`).  
- Normalization ensures attendance and revenue are scaled comparably (dividing each by the max value across events).  

**Effect:**  
- Higher `α` → prioritizes attendance.  
- Lower `α` → prioritizes revenue.  

This demonstrates **multi-objective optimization**, a strong point for algorithmic design.

---

## 🚀 How to Compile and Run

### 1. Compilation
Ensure you have a C++ compiler (e.g., `g++`). Run:

```bash
g++ dynamic_scheduler.cpp -o scheduler
````

### 2. Execution

Run the program:
````
./scheduler
````

You will be prompted to either:

Enter events manually, or

Run a predefined test case.

🧪 Test Case Analysis
Test Input
````
events = {
    {1, 1, 3, 100, 50, "Hall A", {}},
    {2, 2, 4, 120, 60, "Hall A", {}},
    {3, 5, 7, 150, 80, "Hall B", {}},
    {4, 8, 9, 200, 100, "Hall B", {3}},
    {5, 6, 8, 180, 90, "Hall A", {}},
    {6, 9, 11, 220, 110, "Hall C", {1, 5}}
};
````

Expected Output (Maximize Attendance)
 Maximum Attendance achievable: 500
Selected Events (by finish time):
   ID 1 [1-3] Venue: Hall A | Attendance: 100 | Revenue: 50
   ID 5 [6-8] Venue: Hall A | Attendance: 180 | Revenue: 90
   ID 6 [9-11] Venue: Hall C | Attendance: 220 | Revenue: 110
---------------------------------------------------
 Totals -> Attendance: 500 | Revenue: 250

Reasoning

Hall A Conflict:

Event 1 [1–3] and Event 2 [2–4] overlap.

While Event 2 has slightly higher attendance (120 vs 100), choosing Event 1 is better for unlocking downstream events.

Dependency-Driven Optimization:

Event 6 depends on both Event 1 and Event 5.

By choosing Event 1, we can later schedule Event 5.

Together, these unlock Event 6, which has very high attendance (220).

Optimal Path:

Events chosen: 1, 5, 6.

Attendance = 100 + 180 + 220 = 500.

Revenue = 50 + 90 + 110 = 250.

Other paths (e.g., picking Event 2 or Event 3/4) result in lower total scores, so they are excluded.

📊 Hybrid Example

Suppose we run with α = 0.3 (focus on revenue):

The scheduler will prefer higher-revenue events, even if attendance is slightly lower.

With α = 0.8 (focus on attendance):

The scheduler prioritizes high-attendance events, even at some revenue cost.

✅ This flexibility makes the system useful in different real-world applications.


✅ Key Takeaways

Combines DAG-based dependency validation with Dynamic Programming.

Handles multi-venue conflicts and dependency chains.

Supports multi-objective optimization via hybrid scoring.

