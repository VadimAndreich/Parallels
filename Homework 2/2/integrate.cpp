#include <iostream>
#include <chrono>
#include <omp.h>
#include <cmath>

int nsteps = 40000000;
int THREADS = 1;
double a = -4.0, b = 4.0;

double func(double x)
{
    return cos(x);
}

void integrate_serial()
{
    double sum = 0;
    double h = (b - a) / nsteps;

    double start = omp_get_wtime();

    for (int i = 0; i < nsteps; i++)
        sum += func(a + (h * i) - (h / 2));

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "Serial implementation took " << time << " seconds to complete." << std::endl;
    std::cout << sum * h << std::endl;

//    return sum * h;
}

void integrate_parallel()
{
    double sum = 0;
    double h = (b - a) / nsteps;

    double start = omp_get_wtime();

    #pragma omp parallel num_threads(THREADS)
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = nsteps / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (nsteps) : (lb + items_per_thread - 1);
        double local_sum = 0;

        for (int i = lb; i < ub; i++)
            local_sum += func(a + (h * i) - (h / 2));

        #pragma omp atomic
        sum += local_sum;
    }

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "Parallel implementation took " << time << " seconds to complete." << std::endl;
    std::cout << sum * h << std::endl;

//    return sum * h;
}

int main(int argc, char** argv)
{
    THREADS = atoi(argv[1]);

    integrate_serial();
    integrate_parallel();

    return 0;
}