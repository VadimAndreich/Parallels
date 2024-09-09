#include <iostream>
#include <chrono>
#include <omp.h>
#include <cmath>

int THREADS = 1;

void init_arrays(double a[], double b[], double x[], int n)
{
    #pragma omp parallel
    {
        #pragma omp for
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++)
                a[i * n + j] = i == j ? 2.0 : 1.0;
            b[i] = n + 1;
            x[i] = 0;
        }
    }
}

void zero_x(double x[], int n)
{
    #pragma omp parallel
    {
        #pragma omp for
        for (int i = 0; i < n; i++) {
            x[i] = 0;
        }
    }
}

void solve_system_serial(double a[], double b[], double x[], int n)
{
    double t = 0.00001;
    double eps = 0.0000001;
    auto* res = new double[n];

    double start = omp_get_wtime();

    while (true) {
        for (int i = 0; i < n; i++) {
            res[i] = 0;
            for (int j = 0; j < n; j++)
                res[i] += a[i * n + j] * x[j];
            res[i] -= b[i];
        }

        double sum1 = 0, sum2 = 0;
        for (int i = 0; i < n; i++) {
            sum1 += pow(res[i], 2);
            sum2 += pow(b[i], 2);
        }

        if ((sqrt(sum1) / sqrt(sum2)) < eps)
            break;

        for (int i = 0; i < n; i++)
            x[i] = x[i] - t * res[i];
    }

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "Serial implementation took " << time << " seconds to complete." << std::endl;

//    return time;
}

void solve_system_parallel_1(double a[], double b[], double x[], int n)
{
    double t = 0.00001;
    double eps = 0.0000001;
    auto* res = new double[n];
    double crit = 1;

    double start = omp_get_wtime();

    while (crit > eps) {
        #pragma omp parallel num_threads(THREADS)
        {
            #pragma omp for
            for (int i = 0; i < n; i++) {
                res[i] = 0;
                for (int j = 0; j < n; j++)
                    res[i] += a[i * n + j] * x[j];
                res[i] -= b[i];
            }
        }

        double sum1 = 0, sum2 = 0;
        #pragma omp parallel num_threads(THREADS)
        {
            double sum1_loc = 0, sum2_loc = 0;
            #pragma omp for
            for (int i = 0; i < n; i++) {
                sum1_loc += pow(res[i], 2);
                sum2_loc += pow(b[i], 2);
            }

            #pragma omp atomic
            sum1 += sum1_loc;
            #pragma omp atomic
            sum2 += sum2_loc;
        }

        crit = (sqrt(sum1) / sqrt(sum2));

        /*if ((sqrt(sum1) / sqrt(sum2)) < eps)
            break;*/

        #pragma omp parallel num_threads(THREADS)
        {
            #pragma omp for
            for (int i = 0; i < n; i++)
                x[i] = x[i] - t * res[i];
        }
    }

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "First parallel implementation took " << time << " seconds to complete." << std::endl;

//    return time;
}

void solve_system_parallel_2(double a[], double b[], double x[], int n) {
    double t = 0.00001;
    double eps = 0.0000001;
    auto* res = new double[n];
    double crit = 1;

    double start = omp_get_wtime();

    #pragma omp parallel num_threads(THREADS)
    {
        while (crit > eps) {
            #pragma omp for
            for (int i = 0; i < n; i++) {
                res[i] = 0;
                for (int j = 0; j < n; j++)
                    res[i] += a[i * n + j] * x[j];
                res[i] -= b[i];
            }

            double sum1 = 0, sum2 = 0;
            double sum1_loc = 0, sum2_loc = 0;

            #pragma omp for
            for (int i = 0; i < n; i++) {
                sum1_loc += pow(res[i], 2);
                sum2_loc += pow(b[i], 2);
            }

            #pragma omp atomic
            sum1 += sum1_loc;
            #pragma omp atomic
            sum2 += sum2_loc;

            #pragma omp barrier
            crit = (sqrt(sum1) / sqrt(sum2));

            /*if ((sqrt(sum1) / sqrt(sum2)) < eps)
                break;*/

            #pragma omp for
            for (int i = 0; i < n; i++)
                x[i] = x[i] - t * res[i];
        }
    }

    double end = omp_get_wtime();
    double time = end - start;

    std::cout << "Second parallel implementation took " << time << " seconds to complete." << std::endl;

//    return time;
}

int main(int argc, char** argv)
{
    int n = atoi(argv[1]);
    THREADS = atoi(argv[2]);

    auto* a = new double[n * n];
    auto* b = new double[n];
    auto* x = new double[n];

    init_arrays(a, b, x, n);

    if (THREADS == 1) solve_system_serial(a, b, x, n);
    else
    {
        solve_system_parallel_1(a, b, x, n);
        zero_x(x, n);
        solve_system_parallel_2(a, b, x, n);
    }

    return 0;
}
