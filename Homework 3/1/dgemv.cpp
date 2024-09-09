#include <iostream>
#include <cmath>
#include <thread>
#include <vector>
#include <omp.h>

int THREADS = 1, SIZE = 20000;
double* m, *v, *res;

void print_arrays()
{
/*    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            std::cout << m[i * SIZE + j] << " ";
        std::cout << "\n";
    }
    std::cout << "\n\n";

    for (int i = 0; i < SIZE; i++) {
        std::cout << v[i] << " ";
    }
    std::cout << "\n\n";*/

    for (int i = 0; i < SIZE; i++) {
        std::cout << res[i] << " ";
    }
    std::cout << "\n\n";
}

void init_arrays(double* a, double* b)
{
#pragma omp for
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
            a[i * SIZE + j] = 1;
        b[i] = 1;
    }
}

void matrix_vector_mult(int begin, int end)
{
    for (int i = begin; i < end; i++) {
        for (int j = 0; j < SIZE; j++) {
            res[i] += m[i * SIZE + j] * v[j];
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc > 1) {
        SIZE = atoi(argv[1]);
        THREADS = atoi(argv[2]);
    }

    std::cout << "Threads: " << THREADS << ". Size: " << SIZE << std::endl;
    m = new double[SIZE * SIZE];
    v = new double[SIZE];
    res = new double[SIZE];

    init_arrays(m, v);

    std::vector<std::thread> threads;
    std::vector<std::pair<int, int>> borders(THREADS);

    int steps_per_thread = SIZE / THREADS;
    int diff = SIZE - THREADS * steps_per_thread;
    int avg_diff = ceil(double(diff) / double(THREADS));
    int num_threads_to_expand = ceil(double(diff) / double(avg_diff));

    for (int i = 0; i < THREADS; i++) {
        borders[i].first = i > 0 ? borders[i - 1].second : 0;

        borders[i].second = num_threads_to_expand-- > 0 ?
                            borders[i].first + steps_per_thread + avg_diff :
                            borders[i].first + steps_per_thread;

//        std::cout << borders[i].first << "  " << borders[i].second << std::endl;
    }

    double start = omp_get_wtime();

    for (int id = 0; id < THREADS; id++) {
        threads.push_back(std::move(std::thread(matrix_vector_mult, borders[id].first, borders[id].second)));
//        threads.push_back(std::thread(matrix_vector_mult, borders[id].first, borders[id].second));
    }

    for (int i = 0; i < THREADS; i++) {
        threads[i].join();
    }

    double end = omp_get_wtime();
    double time = end - start;

    print_arrays();
    std::cout << "Time to complete multiplication with " << THREADS << " threads: " << time << std::endl;
}