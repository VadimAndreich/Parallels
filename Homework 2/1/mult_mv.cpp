#include <iostream>
#include <omp.h>

int SIZE, THREADS = 1;

void print_arrays(double *a, double *b, int n, int m)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++)
            std::cout << a[i * n + j] << "  ";
        std::cout << std::endl;
    }

    std::cout << "\n";

    for (int i = 0; i < n; i++)
        std::cout << b[i] << "  ";
}

void init_arrays(double* a, double* b, int n, int m)
{
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);

        for (int i = lb; i <= ub; i++) {
            for (int j = 0; j < m; j++)
                a[i * n + j] = i + j;
            b[i] = i;
        }
    }
}

double run_serial(const double a[], const double b[], int n, int m)
{
    auto* res = new double[n];
    double start = omp_get_wtime();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            res[i] += a[i * n + j] * b[j];
        }
    }

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "Serial implementation took " << time << " seconds to complete." << std::endl;

    return time;
}

double run_parallel(const double a[], const double b[], int n, int m)
{
    auto* res = new double[n];
    double start = omp_get_wtime();

    #pragma omp parallel num_threads(THREADS)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);

        for (int i = lb; i <= ub; i++) {
            for (int j = 0; j < m; j++) {
                res[i] += a[i * n + j] * b[j];
            }
        }
    }

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "Parallel implementation took " << time << " seconds to complete." << std::endl;

    return time;
}

int main(int argc, char** argv)
{
    SIZE = atoi(argv[1]), THREADS = atoi(argv[2]);
    auto* a = new double[SIZE * SIZE];
    auto* b = new double[SIZE];
    init_arrays(a, b, SIZE, SIZE);

    if (THREADS == 1) double serial = run_serial(a, b, SIZE, SIZE);
    else double parallel = run_parallel(a, b, SIZE, SIZE);

//    std::cout << "Speedup = " << serial / parallel << std::endl;

    return 0;
}