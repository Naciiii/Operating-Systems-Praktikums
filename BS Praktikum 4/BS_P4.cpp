#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <semaphore.h>

// Global constants

int num_employees;
int container_capacity;
int refill_rate;
int num_customers;
int prep_time; // in microseconds
int total_burgers_served = 0; // Total number of burgers served
std::mutex total_burgers_mutex; // Mutex for updating total burgers served
sem_t queue_sem;
std::vector<sem_t> customer_semaphores(num_customers);

// Ingredient container
struct IngredientContainer {
    int quantity;
    std::mutex mutex;
    sem_t refill_sem;
    IngredientContainer(int init_quantity) : quantity(init_quantity) {}
    // Delete the copy constructor and copy assignment operator
    IngredientContainer(const IngredientContainer&) = delete;
    IngredientContainer& operator=(const IngredientContainer&) = delete;
};

// Global ingredient containers
std::vector<std::unique_ptr<IngredientContainer>> ingredient_containers;

// Customer order
struct Order {
    int id; // Customer ID
    int num_burgers; // Number of burgers ordered
    sem_t* sem_wait_order; // Semaphore to wait for order completion
    Order(int i, int num, sem_t* sem) : id(i), num_burgers(num), sem_wait_order(sem) {}
};


// Customer queue
std::queue<Order> customer_queue;
std::mutex queue_mutex;

// Flags
bool is_customer_available = false;

bool all_customers_served = false;

// Random number
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 10);

// Take ingredients from containers
bool takeIngredients(int num_burgers) {
    for (auto& container_ptr : ingredient_containers) {
        std::unique_lock<std::mutex> lock(container_ptr->mutex);
        if (container_ptr->quantity < num_burgers) {
            return false; // No more ingredients
        }
        container_ptr->quantity -= num_burgers;
    }
    return true;
}

void employeeThread(int id) {
    while (true) {
        sem_wait(&queue_sem);
        if (all_customers_served && customer_queue.empty()) {
            std::cout << "";
            break;
        }

        if (!customer_queue.empty()) {
            Order order = customer_queue.front();
            customer_queue.pop();
            is_customer_available = !customer_queue.empty();
            sem_post(&queue_sem);

            for (auto& container_ptr : ingredient_containers) {
                std::unique_lock<std::mutex> lock(container_ptr->mutex);
                while( container_ptr->quantity < order.num_burgers){
                    lock.unlock();
                    sem_wait(&(container_ptr->refill_sem));
                    lock.lock();
                }
                container_ptr->quantity -= order.num_burgers;
            }

            // Usleep
            usleep(prep_time * order.num_burgers);

            // Serve the customer
            std::cout << "Employee " << id << " served customer " << order.id << " with " << order.num_burgers << " burgers." << std::endl;

            // Update total burgers served
            std::lock_guard<std::mutex> burgers_lock(total_burgers_mutex);
            total_burgers_served += order.num_burgers;

            sem_post(order.sem_wait_order);

        }
    }
}
void addCustomerToQueue(int customer_id, int num_burgers, sem_t* sem_wait_order) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    customer_queue.push({customer_id, num_burgers, sem_wait_order});
    is_customer_available = true;
    sem_post(&queue_sem);
}

// Customer thread function
void customerThread(int id) {
    int num_burgers = dis(gen); // Random number of burgers
    sem_t* sem = new sem_t; // Customer waiting semaphore
    sem_init(sem, 0, 0); // Initialize the semaphore

    addCustomerToQueue(id, num_burgers, sem);

    // Wait for the order to be completed
    sem_wait(sem);
    sem_destroy(sem); // Destroy the semaphore
    delete sem; // Free the dynamically allocated memory
}


// Refiller thread function
void refillerThread(bool& all_customers_done) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Usleep variant to wait between refills

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (all_customers_done) {
                std::cout << "";  // Debug
                break;
            }
        }

        for (auto& container_ptr : ingredient_containers) {
            std::unique_lock<std::mutex> lock(container_ptr->mutex);
            if (container_ptr->quantity < container_capacity) {
                // Calculate the refill amount, not exceeding the container's capacity
                int refill_amount = std::min(refill_rate, container_capacity - container_ptr->quantity);
                container_ptr->quantity += refill_amount;

                if (refill_amount > 0) {
                    sem_post(&(container_ptr->refill_sem));
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <num_employees> <container_capacity> <refill_rate> <num_customers> <prep_time>" << std::endl;
        return 1;
    }
    std::cout<<"Welcome To My Imbiss"<<std::endl;
    // Parse command line arguments
    num_employees = std::stoi(argv[1]);
    container_capacity = std::stoi(argv[2]);
    refill_rate = std::stoi(argv[3]);
    num_customers = std::stoi(argv[4]);
    prep_time = std::stoi(argv[5]);
    sem_init(&queue_sem,0,0);//changee
    // Initialize the ingredient containers with the correct capacity
    for (int i = 0; i < 4; ++i) {
        ingredient_containers.push_back(std::make_unique<IngredientContainer>(container_capacity));
        sem_init(&(ingredient_containers[i]->refill_sem),0,0);
    }

   std::cout<<" "<<std::endl;//Debug
    // Start timing the simulation
    auto start_time = std::chrono::high_resolution_clock::now();

    // Flag to signal when all customers are done
    bool all_customers_done = false;

    // Start the refiller thread
    std::thread refiller_thread(refillerThread, std::ref(all_customers_done));

    // Start employee threads
    std::vector<std::thread> employee_threads;
    for (int i = 0; i < num_employees; ++i) {
        employee_threads.emplace_back(employeeThread, i);
    }

    // Start customer threads
    std::vector<std::thread> customer_threads;
    for (int i = 0; i < num_customers; ++i) {
        customer_threads.emplace_back(customerThread, i);
    }

    // Wait for all customer threads to complete
    for (auto& t : customer_threads) {
        t.join();
    }

    // Notify all employee threads that all customers are done
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        all_customers_served = true;
        for (int i=0; i<num_employees;i++){
            sem_post(&queue_sem);//change
        }

    }

    // Wait for all employee threads to complete
    for (auto& t : employee_threads) {
        t.join();
    }

    // Signal the refiller thread to stop
    all_customers_done = true;

    // Notify refiller thread in case it's waiting
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        sem_post(&queue_sem);//change
    }

    std::cout << ""; // Debug statement
    refiller_thread.join();
    std::cout << "" ; // Debug statement

    // Stop timing the simulation
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    // Calculate throughput
    double throughput = total_burgers_served / elapsed_seconds.count();

    std::cout << "Total burgers served: " << total_burgers_served << std::endl;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "s" << std::endl;
    std::cout << "Throughput: " << throughput << " burgers per second" << std::endl;

    return 0;
}

