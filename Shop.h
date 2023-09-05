#ifndef __SHOP_H__
#define __SHOP_H__

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <queue>
#include <deque>
#include <unordered_map>
#include <string>

using namespace std;

#define DEFAULT_CHAIRS 3
#define DEFAULT_BARBERS 1
#define errExitEN(en, msg) \
do { errno = en; perror(msg); exit(EXIT_FAILURE); \
} while (0)
	
class Shop {
    public:
    Shop(int nBarbers, int nChairs);
    Shop();
    ~Shop();
    
    int visitShop(int id);
    void leaveShop(int customerId, int barberId);
    void helloCustomer(int id);
    void byeCustomer(int id);
    int nDropsOff;
    
    private:
    void initialize();
    inline string int2string(int i);
    inline void print(int person, string message);
    bool isAllChairsFull();
    bool isAllBarbersBusy();
    int nBarbers;
    int nChairs;
    int nAvailableChairs;
    
    deque<int> freeBarbers;      //?
    vector<int> seats;           //?for customer or barber 

	// sperate critical section for barbers and customers 
    pthread_mutex_t barbermtx;
    pthread_mutex_t customermtx;
	
	
	pthread_cond_t customer_waiting_cond; //?
    pthread_cond_t* barber_waiting_conds;//?
    unordered_map<int,pthread_cond_t*> customer_waiting_for_service_conds;//? 
};
#endif