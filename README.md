## Sleeping Barber Problem using Multi-Threading

The Sleeping Barber Problem is a classic synchronization problem often used in computer science and operating systems to illustrate issues related to concurrent programming, mutual exclusion, and inter-process communication. It provides a scenario where multiple threads (representing customers and a barber) interact with shared resources (the barber's chair and a waiting room) in a way that requires careful synchronization to avoid conflicts and ensure correctness.

Here's a description of the Sleeping Barber Problem:

### Scenario:

There is a barber shop with one barber and a waiting room with a limited number of chairs for customers. Customers arrive at the barber shop one by one. If there are empty chairs in the waiting room, they take a seat. If all chairs are occupied, they leave. The barber alternates between cutting hair and sleeping. When a customer arrives, the barber either cuts their hair immediately (if the barber is awake) or goes to sleep (if the barber is currently cutting someone's hair).

-------------

### Rules:

1. If a customer arrives and the barber is sleeping, they must wake up the barber.
2. The barber serves customers in the order they arrived (FIFO: First-In-First-Out).
3. When a customer's hair is cut, they leave the barber shop.
4. The barber only goes to sleep if there are no customers waiting in the waiting room. If a customer arrives while the barber is sleeping, they wake up the barber.

-------------

### Challenges:

The problem involves managing shared resources (the waiting room chairs and the barber's chair) in a way that prevents race conditions and ensures that customers are served correctly.
Ensuring that the barber doesn't cut the hair of multiple customers simultaneously or cut hair when there are no customers to serve is crucial.
Implementing the problem efficiently and without deadlocks or resource contention is a key challenge in concurrent programming.
Solving the Sleeping Barber Problem requires using synchronization mechanisms like semaphores, mutexes, or condition variables to coordinate the actions of the barber and the customers. The goal is to avoid conflicts and ensure that customers are served in an orderly manner while minimizing resource contention.

-------------

### Solutions

1. Use two critical Sections, one for barbers, one for customers.
2. Use ```pthread_cond_timedwait()```. For example, A barber has to enter a critical section to wait for a customer. To avoid deadlocks, the barber thread waits for a customer to give him a service signal within a certian time period. Then it goes back to check any customer sits on his chair.
