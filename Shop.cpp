#include "Shop.h"
#include <sys/time.h>

#define WAIT_TIME_SECONDS       1
Shop::Shop(int nBarbers, int nChairs)
{
    this->nBarbers = nBarbers;
    this->nChairs = nChairs;
    this->nDropsOff = 0;
    this->nAvailableChairs = nChairs;
    this->seats.resize(nBarbers, -1); //initized as no customers 
    initialize();
}

Shop::Shop()
{
    this->nBarbers = DEFAULT_BARBERS;
    this->nChairs = DEFAULT_CHAIRS;
    this->nDropsOff = 0;
    this->nAvailableChairs = nChairs;
    this->seats.resize(nBarbers, -1);
    initialize();
}

Shop::~Shop()
{
   pthread_cond_destroy(&this->customer_waiting_cond);
   for(int i = 0; i < this->nBarbers; ++i)
   {
       pthread_cond_destroy(&this->barber_waiting_conds[i]);
   }
    
   pthread_mutex_destroy(&barbermtx);
   pthread_mutex_destroy(&customermtx);
   delete []barber_waiting_conds; 
   
   for(auto it = this->customer_waiting_for_service_conds.begin();  it != this->customer_waiting_for_service_conds.end(); ++it) 
   {
      pthread_cond_destroy(it->second); // destroy all prhread conditions 
   }
}

void Shop::initialize()
{
    /* initialize a condition variable to its default value */
    int ret = pthread_cond_init(&this->customer_waiting_cond, NULL);
    if (ret != 0)
    {
        errExitEN(ret, "pthread_cond_init");
    }

    this->barber_waiting_conds = new pthread_cond_t[this->nBarbers];
    for(int i = 0; i < this->nBarbers; ++i)
    {
        ret = pthread_cond_init(&this->barber_waiting_conds[i], NULL);
        if (ret != 0)
        {
            errExitEN(ret, "pthread_cond_init");
        }
    }
    
    ret = pthread_mutex_init(&this->barbermtx, NULL);
    if (ret != 0)
    {
        errExitEN(ret, "pthread_mutex_init");
    }
    
    ret = pthread_mutex_init(&this->customermtx, NULL);
    if (ret != 0)
    {
      errExitEN(ret, "pthread_mutex_init");
    }
}

// is called by a customer thread. return a avaiable barberID
int Shop::visitShop(int id)
{
	//center the customer critical section 
    int rc = pthread_mutex_lock(&customermtx);
    if (rc != 0)
    {
       errExitEN(rc, "visitShop:pthread_mutex_lock");
    }
  
	//if all chair are full 
    if (this->nAvailableChairs == 0)
    {
        //Print “id leaves the shop because of no available waiting chairs”.
        this->print(id, " leaves the shop because of no available waiting chairs");
        //Increment nDropsOff.
        this->nDropsOff += 1;

        //Leave the critical section.
        rc = pthread_mutex_unlock(&customermtx);
        if (rc != 0)
        {
            errExitEN(rc, "visitShop:pthread_mutex_unlock");
        }
        
        return -1;
    }

    //Take a waiting char (or Push the customer in a waiting queue).
    this->nAvailableChairs = nAvailableChairs - 1;

    //Print “id takes a waiting chair. # waiting seats available = …”.
    this->print(id, " takes a waiting chair. # waiting seats available = " + int2string(this->nAvailableChairs));
        
    while (this->freeBarbers.empty()) // when no barbers avaiable
    {
        // Wait for a barber to wake me up.
        rc = pthread_cond_wait(&this->customer_waiting_cond, &this->customermtx);
        if (rc != 0)
        {
            errExitEN(rc, "visitShop:pthread_cond_wait");
        }
    }
    
	// when there is avaiable babers, enter the barber critical section
    rc = pthread_mutex_lock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_lock");
    }
   
 
    if (this->freeBarbers.empty())
    {
        cout << "error " << "this->freeBarbers is empty" << endl;
        exit(EXIT_FAILURE);
    }    
    
	//get barber id 
    int barberId = this->freeBarbers.front();
    this->freeBarbers.pop_front(); // remove the barberid from the list
	
    //leave the barber critical section 	
    rc = pthread_mutex_unlock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_unlock");
    }
        
    nAvailableChairs = this->nAvailableChairs + 1;    
	
    rc = pthread_mutex_unlock(&customermtx);
    if (rc != 0)
    {
        errExitEN(rc, "visitShop:pthread_mutex_unlock");
    }
    
    return barberId;
}

//is called by a customer thread. 
void Shop::leaveShop(int customerId, int barberId)
{
	//enter the barber critical section 
    int rc = pthread_mutex_lock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_mutex_lock");
    }
    
    pthread_cond_t* waitforservicing = new pthread_cond_t();
    rc = pthread_cond_init(waitforservicing, NULL);
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_cond_init");
    }
    
    this->customer_waiting_for_service_conds.insert(make_pair(customerId, waitforservicing)); // save customer & wait condition into the hashmap
    this->seats[barberId] = customerId; // the customer sits on the chair served by the barberID 
    this->print(customerId, " moves to a service chair["+int2string(barberId)+"], # waiting seats available = " + int2string(this->nAvailableChairs));
    rc = pthread_cond_signal(&this->barber_waiting_conds[barberId]);  // the customer thread signals the barberID provide service 
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_cond_signal");
    }
    
    this->print(customerId, " wait for barber[" + int2string(barberId) + "] to be done with hair-cut." );
    
	//while the barberID is curtting my hair, wait. 
    while(this->seats[barberId] == customerId)
    {
        struct timespec   ts;
        struct timeval    tp;
        rc = gettimeofday(&tp, NULL);
        if (rc != 0)
        {
            errExitEN(rc, "leaveShop:gettimeofday"); 
        }
        /* Convert from timeval to timespec */
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;
        
		//the customer thread waits for a certain time and then check wether service done. use timedwait to avoid deadlock. 
        rc = pthread_cond_timedwait(this->customer_waiting_for_service_conds[customerId], &this->barbermtx, &ts); 
        if (rc != 0 && rc != ETIMEDOUT)
        {
            errExitEN(rc, "leaveShop:pthread_cond_timedwait");
        }
    }    

    this->print(customerId," says good-by to barber["+ int2string(barberId) +"]");
	//leave the critical section 
    rc = pthread_mutex_unlock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "leaveShop:pthread_mutex_unlock");
    }
};

// is called by a barber thread. 
// para: id is the barber's id 
void Shop::helloCustomer(int id)
{
	//enter barber critical section 
    int rc = pthread_mutex_lock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "helloCustomer:pthread_mutex_lock");
    }
    
    struct timespec   ts;
    struct timeval    tp;
    while (this->seats[id] == -1) // the barber thread has no customer.
    {
        rc = gettimeofday(&tp, NULL);
        if (rc != 0)
        {
            errExitEN(rc, "helloCustomer:gettimeofday"); 
        }
        /* Convert from timeval to timespec */
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;
    

        bool f = false; 
        for(deque<int>::iterator it = freeBarbers.begin(); it != freeBarbers.end(); ++it) //check whether the barber thread free 
        {
            if (*it == id)
            {
                f = true;
                break;
            }
        }
        
        if (!f)
        {
            this->freeBarbers.push_back(id);
        }
        
        if (this->nAvailableChairs == 0)
        {
            this->print(-id, " sleeps because of no customers.");
        }
        
        pthread_cond_signal(&this->customer_waiting_cond);
        if (rc != 0)
        {
            errExitEN(rc, "helloCustomer:pthread_cond_signal");
        }
        // the barber wait for a customer to give a signal within a time range. then go back to check any customer sit on his chair. 
        rc = pthread_cond_timedwait(&this->barber_waiting_conds[id], &this->barbermtx, &ts);
        if (rc != 0 && rc != ETIMEDOUT)
        {
            errExitEN(rc, "helloCustomer:pthread_cond_timedwait");
        }
    }
    
    this->print(-id, " starts a hair-cut service for customer[" + int2string(this->seats[id]) +"].");
    //get a customer, release critical section to the other barbers.
    rc = pthread_mutex_unlock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "helloCustomer:pthread_mutex_unlock");
    }
}

// is called by a barber thread. 
void Shop::byeCustomer(int id)
{
	//start a critical section 
    int rc = pthread_mutex_lock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_mutex_lock");
    }
    
    this->print(-id, " says he's done with a hair-cut service for customer["+ int2string(this->seats[id]) + "].");
	
	//wakes up the customer who seat on his chair 
    rc = pthread_cond_signal(this->customer_waiting_for_service_conds[this->seats[id]]);
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_cond_signal");
    }
    
    this->freeBarbers.push_back(id); // go back to freebarber list 
    this->seats[id] = -1; // remove the customer form his chair 
    rc = pthread_cond_signal(&this->customer_waiting_cond); //signal the cutomer service is done. 
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_cond_signal");
    }
    
    this->print(-id, " calls in another customer.");
    rc = pthread_mutex_unlock(&barbermtx);
    if (rc != 0)
    {
        errExitEN(rc, "byeCustomer:pthread_mutex_unlock");
    }
}

string inline Shop::int2string(int i)
{
    stringstream out;
    out <<i;
    return out.str();
}

void inline Shop::print(int person, string message)
 {
     cout << ((person > 0) ? "customer[": "barber [")
             <<abs(person) << "]: " <<message <<endl;
 }

