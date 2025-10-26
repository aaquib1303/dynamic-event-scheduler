# 🧠 Dynamic Event Scheduler (C++)

## 📊 Project Overview

The **Dynamic Event Scheduler** is a high-performance C++ application designed to solve a complex resource allocation challenge: finding the optimal schedule of events to maximize business objectives while strictly adhering to real-world constraints.

This project implements a sophisticated **Maximum Weighted Interval Scheduling (MWIS)** solution, extended to model both **resource conflicts** (venue usage) and **mandatory prerequisite dependencies** between events.  
It serves as a strong demonstration of **advanced algorithms**, **object-oriented design**, and **performance optimization** in C++.

---

## ✨ Features

- **Multi-Objective Optimization**:  
  Schedules events based on maximizing a weighted *Hybrid Score*, which combines **Revenue** and **Attendance**.  
  Users can fine-tune the objective via an adjustable `α` parameter.

- **Venue Conflict Resolution**:  
  Ensures no two selected events occupy the same venue simultaneously.

- **Mandatory Dependencies**:  
  Enforces prerequisite relationships between events (e.g., Event B cannot start until Event A is complete).

- **Constraint Validation**:  
  Robust pre-processing checks to ensure data integrity before scheduling.

- **Modular Architecture**:  
  Implemented as a clean, encapsulated `DynamicScheduler` class using modern C++ principles.

---

## ⚙️ Algorithm and Complexity

The core of the scheduling engine relies on two integrated algorithms:

### 1. Topological Sort (DAG Validation)
- Models event dependencies as a **Directed Acyclic Graph (DAG)**.  
- Verifies data integrity by detecting and preventing dependency cycles (e.g., A depends on B, which depends on A).  
- Provides the necessary processing order for the dynamic programming stage.

### 2. Dynamic Programming (DP) with `O(log N)` Optimization
- Combines the traditional MWIS DP approach with **dependency path summation**.  
- **Time Complexity:** Achieves optimal `O(N log N)` time complexity.  
- **Venue Conflict Lookup:** Uses pre-sorted event lists and `std::upper_bound` for `O(log N)` lookups to efficiently find the best non-conflicting preceding event in the same venue.

---

## 🛠️ Prerequisites

To compile and run this C++ project, you need:

- A **C++ compiler** that supports C++11 or later (e.g., GCC, Clang, MSVC).  
- **Standard Template Library (STL)**.

---

## 🚀 How to Run

### Step 1: Compile the Code
```bash
g++ dynamic_scheduler.cpp -o scheduler -std=c++17
```

### Step 2: Execute the Program
```bash
./scheduler
```

### Step 3: Follow the Prompts
The application will guide you through:
- Choosing between Manual Input or a Predefined Test Case.
- Selecting the optimization Objective (Attendance, Revenue, or Hybrid Score).
- If Hybrid is chosen, setting the α weight (0 to 1).

---

## 💡 Future Enhancements
To evolve this project into a production-ready system, the following features are planned:

### 1. Resource Constraints (RCPSP)
Introduce granular resource modeling (e.g., staffing, equipment) with limited availability, transforming the problem into a Resource-Constrained Project Scheduling Problem.

### 2. External Data Handling
Implement data input/output using external file formats (e.g., JSON) via a library like nlohmann/json.

### 3. Heuristic Solver
Add an approximate solver (e.g., Genetic Algorithm) to handle NP-Hard extensions such as the RCPSP variant.

### 4. Unit Testing
Develop a comprehensive test suite (e.g., using Google Test) for core logic functions like topoSort() and calculateWeight().
