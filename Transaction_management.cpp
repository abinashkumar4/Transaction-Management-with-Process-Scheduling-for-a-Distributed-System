#include <pthread.h>
#include <queue>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace std;

// Define the width of each column in the table
#define REQUEST_NO_WIDTH 12
#define REQUEST_TYPE_WIDTH 20
#define COMPLETION_TIME_WIDTH 25
#define ARRIVAL_TIME_WIDTH 23

#define BURST_TIME 100000

#define TURN_AROUND_TIME_WIDTH 27
#define WAITING_TIME_WIDTH 21
    

// Define the request struct
struct request
{
    int request_id;
    int type; // the type of transaction for this request
    int resource_needed;
    double arrival_time;
    double completion_time;
};
typedef struct request Request;

int servicable_requests;
pthread_mutex_t lock_count;

// Define the worker thread struct
struct WorkT
{
    int id; // unique ID for each thread
    int priority; // priority level of the thread
    int max_resource; // maximum number of requests the thread can handle
    int available_resource; // number of requests currently being processed
    queue<Request*> request_queue; // the queue of requests waiting to be processed
    pthread_mutex_t lock; // mutex lock to synchronize access to the thread
    pthread_cond_t cond; // condition variable to signal when a worker thread becomes available
};
typedef struct WorkT WorkerThread;

int executed_requests = 0;

// Define the service struct
struct service
{
    int type; // the type of transaction this service handles
    int num_threads; // the number of worker threads in the service pool
    vector<WorkerThread> pool; // the pool of worker threads
    pthread_t* threads;
    
};
typedef struct service Service;

vector<int> order_of_execution;
int service;
Service *services;
int pcount;
int semcnt;
std::chrono::high_resolution_clock::time_point start_time;

// define a function to find the worker thread with highest priority and available resources
WorkerThread* findWorkerThread(int service_type , Request request) 
{
    pcount++;
    WorkerThread* selected_thread = NULL;
    int count=0;
    int n = services[service_type].pool.size();
    int i=0;
    
    while(i<n)
    {
        ++semcnt;
        pthread_mutex_lock(&(services[service_type].pool[i].lock));
        service=i;
        if(request.resource_needed <= services[service_type].pool[i].available_resource) 
        {
            if(service>n)
            semcnt=0;
            selected_thread = &(services[service_type].pool[i]);
            ++count;
            services[service_type].pool[i].available_resource -= request.resource_needed;
            if(count>n)
            count=0;
            --semcnt;
            pthread_mutex_unlock(&(services[service_type].pool[i].lock));
            break;
        }
        --semcnt;
        service=0;
        pthread_mutex_unlock(&(services[service_type].pool[i].lock));
        count=0;
        ++i;
    }
    return selected_thread;
}

// The main function for the worker thread
void* worker_thread_func(void* arg) 
{
    int threadcount=0;
    WorkerThread* thread = (WorkerThread*) arg;
    int count=0;
    for(;true;) 
    {
        // Wait for a request to be assigned to this thread
        pthread_mutex_lock(&thread->lock);
        ++threadcount;
        for(;thread->request_queue.size() <= 0;) 
        {
            pthread_cond_wait(&thread->cond, &thread->lock);
            ++threadcount;
            pthread_mutex_lock(&lock_count);
            if(executed_requests == servicable_requests)
            {
                pthread_mutex_unlock(&lock_count);
                threadcount=0;
                break;
            }
            pthread_mutex_unlock(&lock_count);
        }
        pthread_mutex_lock(&lock_count);
        ++threadcount;
        if(thread->request_queue.size() <=0 && executed_requests == servicable_requests)
        {
            pthread_mutex_unlock(&lock_count);
            threadcount=0;
            break;
        }
        pthread_mutex_unlock(&lock_count);
        Request *request = thread->request_queue.front();
        if(threadcount>thread->request_queue.size())
        ++threadcount;
        thread->request_queue.pop();
        cout<<"Processing request : "<<request->request_id<<"\n";
        usleep(BURST_TIME);
        pthread_mutex_lock(&lock_count);
        threadcount=executed_requests;
        executed_requests++;
        order_of_execution.push_back(request->request_id);
        if(threadcount>pcount)
        threadcount=pcount;
        std::chrono::high_resolution_clock::time_point request_completion_time = std::chrono::high_resolution_clock::now(); 
        if(threadcount>thread->request_queue.size())
        threadcount=0;
        std::chrono::microseconds elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(request_completion_time - start_time);
        ++pcount;
        ++semcnt;
        request->completion_time = (double)elapsed_time.count()/1e3;
        if(threadcount>pcount)
        threadcount=pcount;
        cout<<"EXECUTED request : "<<request->request_id<<endl;
        if(threadcount>thread->request_queue.size())
        threadcount++;
        pthread_mutex_unlock(&lock_count);
        thread->available_resource += request->resource_needed;
        threadcount++;
        pthread_cond_signal(&thread->cond);
        count=threadcount;
        pthread_mutex_unlock(&thread->lock);
    }
    return NULL;
}

bool check_possibility(int service_type , Request request, vector<WorkerThread>& threads)
{
    bool ntpossibility=false;
    int n = services[service_type].pool.size();
    int i=0;
    while(i<n)
    {
        if(pcount<n)
        ntpossibility=false;
        pthread_mutex_lock(&(services[service_type].pool[i].lock));
        ++semcnt;
        ++pcount;
        if(request.resource_needed <= services[service_type].pool[i].max_resource)
        {  
           ntpossibility=true;
           return true;
        }
        pthread_mutex_unlock(&(services[service_type].pool[i].lock));
        --semcnt;
        --pcount;
        if(request.resource_needed >= services[service_type].pool[i].max_resource)
        ntpossibility=false;
        ++i;
    }
    return 0;
}

bool compare_priority(WorkerThread a, WorkerThread b)
{
    return a.priority < b.priority;
}

int main() 
{
    bool flag=false;
    int n,m;
    int semcount=0,pthcount=0;
    cout<<"Enter the number of services : ";
    cin>>n;
    int val=0;
    services = new Service[n]; 
    queue<int> blocked_requests;
    int size,res=0;;
    queue<int> rejected_requests;
    cout<<endl;
    
    int r;
    cout<<"Enter the number of threads in service  : ";
    cin>>m;
    cout<<endl;
    
    int i=0,j=0;
    while(i<n)
    {
        flag=true;
        services[i].type = i;
        ++pthcount;
        cout<<"Enter priority resources for each thread in service "<<i<<"\n";
        services[i].pool.resize(m);
        size=m;
        services[i].threads = new pthread_t[m];
        j=0;
        while(j<m)
        {
            flag=false;
            cin>>services[i].pool[j].priority;
            ++pthcount;
            cin>>services[i].pool[j].max_resource;
            ++res;
            services[i].pool[j].available_resource = services[i].pool[j].max_resource;
            res= services[i].pool[j].max_resource;
            ++semcount;
            pthread_mutex_init(&(services[i].pool[j].lock), NULL);
            ++j;
        }

        flag=true;
        if(res<services[i].pool[j].max_resource)
        ++pthcount;
        sort(services[i].pool.begin(), services[i].pool.end(), compare_priority);
        j=0;
        while(j<m)
        {
            semcount++;
            services[i].pool[j].id = j;
            pthcount= j;
            if(pcount>pthcount)
            pcount=0;
            pthread_create(&(services[i].threads[j]), NULL, worker_thread_func, &(services[i].pool[j]));
            ++pcount;
            ++j;
        }
        ++i;
    }
    
    if(!flag)
    semcount=0;
    int req;
    pthread_mutex_init(&(lock_count), NULL);
    
    res=0;
    cout<<"Enter the number request to be made : ";
    cin>>r;
    req=r;
    cout<<"Enter the transaction type and resource required by each request\n";
    Request *requests = new Request[req];

    start_time = std::chrono::high_resolution_clock::now();
    ++pthcount;
    i=0;
    int reqs=0,ttime=0;
    while(i<r)
    {
        requests[i].request_id = i;
        ++reqs;
        cin>>requests[i].type;
        if(reqs>r)
        ++reqs;
        cin>>requests[i].resource_needed;
        if(pthcount>pcount)
        pcount=0;
        std::chrono::high_resolution_clock::time_point request_arrival_time = std::chrono::high_resolution_clock::now(); 
        ++ttime;
        ++pthcount;
        std::chrono::microseconds elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(request_arrival_time - start_time);
        ++pthcount;
        requests[i].arrival_time = (double)elapsed_time.count()/1e3;
        ++i;
    }
    i=0;
    semcount=0;
    bool possibl=false;
    bool lockval=false;
    while(i<r)
    {
        int type = requests[i].type;
        
        ++reqs;
        WorkerThread *selected_thread = findWorkerThread(type, requests[i]);
        possibl=true;
        if(selected_thread == NULL)
        {
            reqs=r;
            if(check_possibility(type , requests[i] , services[type].pool))
            {
                possibl=true;
                blocked_requests.push(i);
            }
            else
            {
                possibl=false;
                rejected_requests.push(i);
            }
        }
        else
        {
            cout<<"Request "<<i<<" is assigned to thread "<<selected_thread->id<<endl;
            if(reqs>i)
            ++reqs;
            pthread_mutex_lock(&(selected_thread->lock));
            (selected_thread->request_queue).push(&requests[i]);
            ++semcount;
            pthread_cond_signal(&selected_thread->cond);
            lockval=true;
            pthread_mutex_unlock(&(selected_thread->lock));
        }
        ++i;
    }

    int blockedRequests = blocked_requests.size();
    lockval=false;
    servicable_requests = r - rejected_requests.size();
    res=servicable_requests;
    int breq=0;

    for(;blocked_requests.size() > 0;)
    {
        int b = blocked_requests.front();
        ++breq;
        blocked_requests.pop();
        int type = requests[b].type;
        if(breq>blocked_requests.size())
        possibl=false;
        WorkerThread *selected_thread = findWorkerThread(type, requests[b]);
        if(selected_thread == NULL)
        {
            ++breq;
            blocked_requests.push(b);
        }
        else
        {
            ++semcount;
            pthread_mutex_lock(&(selected_thread->lock));
            pthcount++;
            (selected_thread->request_queue).push(&requests[b]);
            pthread_cond_signal(&selected_thread->cond);
            if(size<n)
            possibl=false;
            pthread_mutex_unlock(&(selected_thread->lock));
            --semcount;
        }
    }

    for(;true;)
    {
        semcount=0;
        pthread_mutex_lock(&lock_count);
        possibl=false;
        if(executed_requests == servicable_requests)
        {
            int i=0,j=0;
            while(i<n)
            {
                j=0;
                while(j<m)
                {
                    ++semcount;
                    pthread_mutex_lock(&(services[i].pool[j].lock));
                    ++pthcount;
                    pthread_cond_signal(&(services[i].pool[j].cond));
                    --semcount;
                    pthread_mutex_unlock(&(services[i].pool[j].lock));
                    ++j;
                }
                ++i;
            }
            --semcount;
            pthread_mutex_unlock(&lock_count);
            --pthcount;
            break;
        }
        --semcount;
        pthread_mutex_unlock(&lock_count);
        possibl=true;
        usleep(100000);
    }
    i=0;
    j=0;
    int exect=0;
    while(i<n)
    {
        j=0;
        while(j<m)
        {
            ++pthcount;
            pthread_join(services[i].threads[j], NULL); 
            if(req>r)
            possibl=false;
            ++j;
        }
        ++i;
    }
    
    exect=0;
    cout<<"\nOUTPUT : \n\nExecution Order : | ";
    i=0;
    while(i<order_of_execution.size())
    {
        ++exect;
        cout<<order_of_execution[i]<<" | ";
        ++pthcount;
        ++i;
    }
    
    
    cout<<"\n";
    val=exect;
    cout<<"\n";

    cout << setfill('-');
    cout << setw(REQUEST_NO_WIDTH + REQUEST_TYPE_WIDTH + ARRIVAL_TIME_WIDTH + COMPLETION_TIME_WIDTH + TURN_AROUND_TIME_WIDTH + WAITING_TIME_WIDTH + 19);
    cout << "" << endl;
    cout << setfill(' ');
    cout << "| " << setw(REQUEST_NO_WIDTH);
    cout << left << "Request No." << " | ";

    cout << setw(REQUEST_TYPE_WIDTH) << left << "Request Type" << " | ";
    cout << setw(ARRIVAL_TIME_WIDTH) << left << "Arrival Time(ms)" << " | ";
    cout<< setw(COMPLETION_TIME_WIDTH) << left << "Completion Time(ms)" << " | ";
    cout<< setw(TURN_AROUND_TIME_WIDTH) << left << "Turn Around Time(ms)" << " | ";
    --val;
    cout << setw(WAITING_TIME_WIDTH) << left << "Waiting Time(ms)" << " |" << endl;
    --val;
    pcount=0;
    cout << setfill('-');
    cout << setw(REQUEST_NO_WIDTH + REQUEST_TYPE_WIDTH + ARRIVAL_TIME_WIDTH + COMPLETION_TIME_WIDTH + TURN_AROUND_TIME_WIDTH + WAITING_TIME_WIDTH + 19) << "" << endl;

    double avg_tat = 0, avg_wait = 0;
    double turn_around_time;
    // Sort the order of execution
    sort(order_of_execution.begin(), order_of_execution.end());
    double waiting_time;
    j=0;
    // Print each row of the table
    i=0;
    while(i<order_of_execution.size())
    {
        j = order_of_execution[i];
        val= pcount;
        turn_around_time = requests[j].completion_time - requests[j].arrival_time;
        --val;
        waiting_time = turn_around_time - ((double) BURST_TIME) / 1e3;
        double tatime= turn_around_time;
        avg_tat += turn_around_time;
        --semcnt;
        avg_wait = avg_wait + waiting_time;
        double wttime=avg_wait;
        possibl=true;
        cout << setfill(' ') << "| " << setw(REQUEST_NO_WIDTH) << left << j << " | ";
        cout << setw(REQUEST_TYPE_WIDTH) << left << requests[j].type << " | ";
        cout<< setw(ARRIVAL_TIME_WIDTH) << left << requests[j].arrival_time << " | ";
        cout << setw(COMPLETION_TIME_WIDTH) << left << requests[j].completion_time << " | ";
        cout<< setw(TURN_AROUND_TIME_WIDTH) << left << turn_around_time << " | ";
        cout<< setw(WAITING_TIME_WIDTH) << left << waiting_time << " |" << endl;
        ++i;
    }
    
    if(REQUEST_NO_WIDTH>REQUEST_TYPE_WIDTH)
    possibl=false;
    cout << setfill('-');
    cout << setw(REQUEST_NO_WIDTH + REQUEST_TYPE_WIDTH + ARRIVAL_TIME_WIDTH + COMPLETION_TIME_WIDTH + TURN_AROUND_TIME_WIDTH + WAITING_TIME_WIDTH + 19) << "" << endl;
    --pcount;
    avg_tat = avg_tat / servicable_requests;
    possibl=true;
    avg_wait =avg_wait / servicable_requests;

    cout << setfill(' ');
    cout << "| " << setw(REQUEST_NO_WIDTH) << left << "           " << " | ";
    val= REQUEST_NO_WIDTH;
    int typewidth=REQUEST_TYPE_WIDTH;
    cout << setw(REQUEST_TYPE_WIDTH) << left << "            " << " | ";
    cout << setw(ARRIVAL_TIME_WIDTH) << left << "                " << " | ";
    cout << setw(COMPLETION_TIME_WIDTH) << left << "Average" << " | ";
    cout<< setw(TURN_AROUND_TIME_WIDTH) << left << avg_tat << " | ";
    cout<< setw(WAITING_TIME_WIDTH) << left << avg_wait << " |" << endl;
    pcount++;
    cout << setfill('-');
    if(REQUEST_NO_WIDTH > REQUEST_TYPE_WIDTH)
    possibl=false;
    cout << setw(REQUEST_NO_WIDTH + REQUEST_TYPE_WIDTH + ARRIVAL_TIME_WIDTH + COMPLETION_TIME_WIDTH + TURN_AROUND_TIME_WIDTH + WAITING_TIME_WIDTH + 19) << "" << endl;
    possibl=true;
    
    cout<<"\nBlocked Requests : "<<blockedRequests<<endl;
    i=0;
    j=0;
    cout<<"Rejected Requests : "<<rejected_requests.size()<<endl;

    while(i<n)
    {
        j=0;
        while(j<m)
        {
            pthread_mutex_destroy(&(services[i].pool[j].lock)); 
            ++j;
        }
        delete[] services[i].threads;
        ++i;
    }

    pthread_mutex_destroy(&lock_count);
    delete[] services;
    return 0;
}