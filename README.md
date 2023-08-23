# Transaction-Management-with-Process-Scheduling-for-a-Distributed-System
Designing Transaction Management with Process Scheduling for a Distributed System Across Multiple Servers Capable of Handling Millions of Transactions 

# Distributed Transaction Processing System

Welcome to the **Distributed Transaction Processing System** GitHub repository! This project involves simulating and designing an efficient process scheduling algorithm for a major e-commerce platform that handles millions of transactions daily. The platform operates on a distributed system comprising multiple servers, each running distinct services responsible for different transaction types.

## Project Overview

The core aspects of this project include:

- **Multiple Services:** The system comprises various services, with each dedicated to handling a specific transaction type (e.g., payment processing, order processing, etc.).
  
- **Worker Threads:** Each service maintains a pool of worker threads. These threads possess priority levels and assigned resources, influencing their request processing order and capabilities.
  
- **Priority & Resources:** Worker threads are prioritized, and each has specific resources allocated to it. Priority guides the order in which threads handle requests, and resources dictate the number of simultaneous requests a thread can manage.
  
- **Request Queuing:** Incoming requests are queued and routed to relevant services based on transaction type. From there, they're further queued to worker threads based on priority levels.
  
## Project Objectives

Goal is to design a process scheduling algorithm that:

- Efficiently allocates resources to worker threads.
- Prioritizes higher-priority threads.
- Adapts to transaction type-specific resource requirements.

## Usage Instructions

1. **Input**: Program reads input from standard input containing the following information:
   - Number of services (n) in the system.
   - Number of worker threads (m) for each service.
   - Priority level and assigned resources for each worker thread.
   - Type of transaction and required resources for each request.
   
2. **Output**: Program outputs the following information:
   - Order of processed requests.
   - Average waiting time for requests.
   - Average turnaround time for requests.
   - Count of rejected requests due to resource scarcity.
   
3. **High Traffic Alerts**: Additionally,  program outputs these metrics during periods of high traffic:
   - Number of requests forced to wait due to insufficient resources.
   - Number of requests blocked due to the unavailability of worker threads.

## Getting Started

To begin, you can clone this repository and explore the provided codebase. 
Feel free to contribute enhancements, bug fixes, or additional features to this project. Your contributions are greatly appreciated!

Let's work together to create an efficient, robust, and high-performing distributed transaction processing system. Happy coding! 🚀
