# 🚦 Priority-based Token Passing Protocol with Selective Repeat ARQ

This project simulates a **custom frame transmission protocol** combining:
- Multilevel Feedback Queue (MLFQ) Scheduling
- Selective Repeat ARQ for error handling
- Token-based device communication
- Concurrent multithreaded environment with starvation/congestion control

Built entirely in **C++**, this system mimics how devices transmit data over a shared channel in real-time.

---

## 🧠 Key Concepts Used

| Area | Concepts |
|------|----------|
| **Operating Systems** | Multilevel Feedback Queue (MLFQ), Starvation Control |
| **Computer Networks** | Token Passing Protocol, Selective Repeat ARQ, Congestion Handling |
| **Concurrency** | Thread Synchronization, Mutex Locking |
| **C++ Programming** | STL Containers, `std::thread`, `std::atomic`, `std::mutex` |

---

## 🔧 System Architecture

![Architecture Diagram]
<img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/d119575a-ba43-4d18-95eb-f93feb368105" />


---

## 🛠️ How It Works

### 📥 1. Device Initialization
- `MAX_DEVICES` (default: 5) are created.
- Each device pre-generates data to send to every other device.

### 📦 2. Packet Creation
- Packets are created dynamically by each device with a randomly assigned priority.
- Based on the priority, packets are inserted into one of 4 queues:
  - `High`
  - `Medium-1`
  - `Medium-2`
  - `Low`

### 🪙 3. Token-Based Scheduling with MLFQ
- Shared cable access is simulated using the `Multilevel_feedback_queue()` scheduler.
- Each queue is served in turn with:
  - Predefined **time quantum**
  - Starvation checks and promotions if thresholds exceed

### 📡 4. Frame Transmission (Selective Repeat ARQ)
- Packets are fragmented into **frames**.
- Each frame is acknowledged individually.
- If a frame is missed (intentionally dropped once to simulate error), retransmission is triggered using Selective Repeat ARQ.

### 🧵 5. Threaded Device Simulation
- Each device runs in a separate thread.
- Devices randomly pick another device and send a packet.
- Thread-safe operations are ensured with `std::mutex`.

---

## 🔁 Retransmission Flow (Selective Repeat ARQ)

- One frame (seq % N == 4) is **intentionally dropped**.
- The receiver identifies the missing frame and requests retransmission.
- Retransmission logic is invoked and pushed back into the queue with an updated sequence number.

---

## 🧪 Sample Output (Console)
```bash
[Device 2] pushed packet to queue with priority 81
[INFO] Entered Multilevel Feedback Queue handler.
received frame 0 from 2 to 3 data: apple_2->3
MISSED frame 4 on purpose---------------------
[Receiver 3] Frame 4 was NOT received. Asking for retransmission from Sender 2
RETRANSMITTED
