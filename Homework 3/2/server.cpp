#include <iostream>
#include <fstream>
#include <future>
#include <thread>
#include <random>
#include <cmath>
#include <queue>
#include <unordered_map>

#define Type double

using namespace std;

random_device dev;
mt19937 rng(dev());
uniform_real_distribution<> dist(0.0, 10.0);

condition_variable condVar, condVar2;

mutex mut;

queue<pair<size_t, future<Type>>> tasks;
queue<pair<size_t, future<Type>>> get_tasks;

unordered_map<int, Type> results;

int N = 10;

void server_thread(const stop_token& stoken)
{
    unique_lock lock_res{mut, defer_lock};
    size_t id_task;
    future<Type> task;

    while (!stoken.stop_requested())
    {
        lock_res.lock();

        if (tasks.empty()) {
            condVar.wait_for(lock_res, chrono::seconds(1s));
        }

        if (!tasks.empty()) {
            get_tasks.push({tasks.front().first, std::move(tasks.front().second)});

            tasks.pop();
        }

        lock_res.unlock();

        while (!get_tasks.empty()) {
            id_task = get_tasks.front().first;
            Type result = get_tasks.front().second.get();

            lock_res.lock();

            results.insert({id_task, result});

            get_tasks.pop();

            condVar2.notify_all();

            lock_res.unlock();
        }
    }

    cout << "Server stop!\n";
}

template<typename T>
class Server{
public:
    void start() {
        cout << "Start\n";
        server = jthread(server_thread);
    };

    void stop() {
        server.request_stop();
        server.join();

        cout << "End\n";
    };

    size_t add_task(future<T> task) {
        size_t id = rand;
        tasks.push({id, std::move(task)});
        rand++;

        return id;
    };

    T request_result(size_t id_res) {
        T res = results[id_res];

        results.erase(id_res);

        return res;
    };

private:
    size_t rand = 0;
    jthread server;
};

Server<Type> server;

template<typename T>
T fun_sin(T arg) {
    return sin(arg);
}

template<typename T>
T fun_sqrt(T arg) {
    return sqrt(arg);
}

template<typename T>
T fun_pow(T x, T y) {
    return pow(x, y);
}

template<typename T>
void add_task_1(string filename) {
    unique_lock lock_res(mut, defer_lock);

    ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (int i = 0; i < N; i++) {
        future<T> result = async(launch::deferred, [](){return fun_sin<T>(dist(rng));});

        lock_res.lock();

        size_t id = server.add_task(std::move(result));

        condVar.notify_one();

        lock_res.unlock();

        lock_res.lock();

        while (results.find(id) == results.end()) {
            condVar2.wait(lock_res);

            if (results.find(id) != results.end()) {
                outfile << "task_thread result:\t" << server.request_result(id) << endl;

                break;
            }
        }

        lock_res.unlock();
    }

    cout << "Client 1 work is done" << endl;
}

template<typename T>
void add_task_2(string filename) {
    unique_lock lock_res(mut, defer_lock);

    ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (int i = 0; i < N; i++) {
        future<T> result = async(launch::deferred, [](){return fun_sqrt<T>(dist(rng));});

        lock_res.lock();

        size_t id = server.add_task(std::move(result));

        condVar.notify_one();

        lock_res.unlock();

        lock_res.lock();

        while (results.find(id) == results.end()) {
            condVar2.wait(lock_res);

            if (results.find(id) != results.end()) {
                outfile << "task_thread result:\t" << server.request_result(id) << endl;

                break;
            }
        }

        lock_res.unlock();
    }

    cout << "Client 2 work is done" << endl;
}

template<typename T>
void add_task_3(string filename) {
    unique_lock lock_res(mut, defer_lock);

    ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (int i = 0; i < N; i++) {
        future<T> result = async(launch::deferred, [](){return fun_pow<T>(dist(rng), dist(rng));});

        lock_res.lock();

        size_t id = server.add_task(std::move(result));

        condVar.notify_one();

        lock_res.unlock();

        lock_res.lock();

        while (results.find(id) == results.end()) {
            condVar2.wait(lock_res);

            if (results.find(id) != results.end()) {
                outfile << "task_thread result:\t" << server.request_result(id) << endl;

                break;
            }
        }

        lock_res.unlock();
    }

    cout << "Client 3 work is done" << endl;
}

int main() {
    server.start();

    thread task_1(add_task_1<Type>, "client1.txt");
    thread task_2(add_task_2<Type>, "client2.txt");
    thread task_3(add_task_3<Type>, "client3.txt");

    task_1.join();
    task_2.join();
    task_3.join();

    server.stop();
}