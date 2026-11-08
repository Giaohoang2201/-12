/*
 * 과제 12: 퀵 정렬의 피벗 선택에 따른 성능 비교 (C 구현)
 *
 * 피벗 선택 방법 3가지(First / Random / Median-of-Three)의 숫자 비교 횟수를 측정하고,
 * [보너스] 재귀적 vs 반복적(스택) 퀵 정렬의 실행시간을 비교한다.
 *
 * - 비교 횟수 = 분할 중 (데이터 값 vs 피벗 값) 비교 + Median-of-Three의 중앙값 결정 비교
 * - 인덱스/종료 조건 검사, 난수 생성, 데이터 이동(swap)은 포함하지 않는다.
 *
 * 컴파일: gcc -O2 -o quick_sort_pivot quick_sort_pivot.c
 * 실행  : ./quick_sort_pivot
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SEED 12345ULL
#define MAX_VALUE 1000000

typedef enum { PIVOT_FIRST, PIVOT_RANDOM, PIVOT_MEDIAN3 } PivotType;

static long long g_cnt;          /* 숫자 비교 횟수 카운터 */
static unsigned long long g_rng; /* 난수 상태 (xorshift64) */

/* ------------------------------------------------------------ 난수 */
static unsigned long long next_rand(void) {
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 7;
    g_rng ^= g_rng << 17;
    return g_rng;
}

/* ------------------------------------------------------------ 피벗 선택 */
/* 피벗의 "인덱스"를 반환한다. */
static int pivot_first(int lo, int hi) {
    (void)hi;
    return lo;
}

static int pivot_random(int lo, int hi) {
    /* 난수 생성은 비교 횟수에 포함하지 않음 */
    return lo + (int)(next_rand() % (unsigned long long)(hi - lo + 1));
}

static int pivot_median3(const int *a, int lo, int hi) {
    int mid = lo + (hi - lo) / 2;
    int x = a[lo], y = a[mid], z = a[hi];

    /* 값 비교 2~3회로 중앙값의 인덱스를 결정 */
    g_cnt++;
    if (x < y) {
        g_cnt++;
        if (y < z) return mid;           /* x < y < z */
        g_cnt++;
        return (x < z) ? hi : lo;        /* x < z <= y  또는  z <= x < y */
    } else {
        g_cnt++;
        if (x < z) return lo;            /* y <= x < z */
        g_cnt++;
        return (y < z) ? hi : mid;       /* y < z <= x  또는  z <= y <= x */
    }
}

static int select_pivot(const int *a, int lo, int hi, PivotType type) {
    switch (type) {
        case PIVOT_FIRST:   return pivot_first(lo, hi);
        case PIVOT_RANDOM:  return pivot_random(lo, hi);
        default:            return pivot_median3(a, lo, hi);
    }
}

/* ------------------------------------------------------------ 분할 (공통) */
/* Lomuto 분할. 피벗 선택 방법만 type 으로 교체된다. */
static int partition(int *a, int lo, int hi, PivotType type) {
    int p = select_pivot(a, lo, hi, type);
    int tmp;

    tmp = a[p]; a[p] = a[hi]; a[hi] = tmp;   /* 피벗을 맨 끝으로 이동 */
    int pivot = a[hi];
    int i = lo;
    long long c = 0;

    for (int j = lo; j < hi; j++) {
        c++;                                  /* 데이터 값 vs 피벗 값 비교 */
        if (a[j] < pivot) {
            tmp = a[i]; a[i] = a[j]; a[j] = tmp;
            i++;
        }
    }
    g_cnt += c;
    tmp = a[i]; a[i] = a[hi]; a[hi] = tmp;
    return i;
}

/* ------------------------------------------------------------ 재귀적 퀵 정렬 */
static void quick_sort_recursive(int *a, int lo, int hi, PivotType type) {
    if (lo < hi) {
        int m = partition(a, lo, hi, type);
        quick_sort_recursive(a, lo, m - 1, type);
        quick_sort_recursive(a, m + 1, hi, type);
    }
}

/* ------------------------------------------------------------ 반복적 퀵 정렬 */
static void quick_sort_iterative(int *a, int n, PivotType type) {
    /* 분할 1회당 스택 크기는 최대 1 증가하므로 n + 2 칸이면 충분하다. */
    int *stack_lo = malloc(sizeof(int) * (n + 2));
    int *stack_hi = malloc(sizeof(int) * (n + 2));
    int top = 0;

    stack_lo[top] = 0;
    stack_hi[top] = n - 1;
    top++;

    while (top > 0) {
        top--;
        int lo = stack_lo[top], hi = stack_hi[top];
        if (lo < hi) {
            int m = partition(a, lo, hi, type);
            /* 재귀 버전과 같은 순서(왼쪽 먼저)로 처리되도록 오른쪽을 먼저 push */
            stack_lo[top] = m + 1; stack_hi[top] = hi;  top++;
            stack_lo[top] = lo;    stack_hi[top] = m - 1; top++;
        }
    }
    free(stack_lo);
    free(stack_hi);
}

/* ------------------------------------------------------------ 유틸 */
static int is_sorted(const int *a, int n) {
    for (int i = 0; i + 1 < n; i++)
        if (a[i] > a[i + 1]) return 0;
    return 1;
}

static int cmp_int(const void *x, const void *y) {
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* 원본 복사본을 정렬하고 비교 횟수를 반환. 결과가 올바른지 검증한다. */
static long long count_comparisons(const int *data, int n, PivotType type, int iterative) {
    int *a = malloc(sizeof(int) * n);
    int *ref = malloc(sizeof(int) * n);
    memcpy(a, data, sizeof(int) * n);          /* 원본의 복사본 사용 */
    memcpy(ref, data, sizeof(int) * n);

    g_cnt = 0;
    g_rng = SEED;
    if (iterative) quick_sort_iterative(a, n, type);
    else           quick_sort_recursive(a, 0, n - 1, type);
    long long result = g_cnt;

    qsort(ref, n, sizeof(int), cmp_int);       /* 표준 정렬 결과와 비교하여 검증 */
    if (!is_sorted(a, n) || memcmp(a, ref, sizeof(int) * n) != 0) {
        fprintf(stderr, "정렬 결과가 올바르지 않음!\n");
        exit(1);
    }
    free(a);
    free(ref);
    return result;
}

/* 복사 시간은 제외하고 정렬 시간만 측정, repeat 회 평균(ms). */
static double measure_time(const int *data, int n, PivotType type, int iterative, int repeat) {
    int *a = malloc(sizeof(int) * n);
    double total = 0.0;

    for (int r = 0; r < repeat; r++) {
        memcpy(a, data, sizeof(int) * n);      /* 측정 구간 밖 */
        g_cnt = 0;
        g_rng = SEED;
        double t0 = now_ms();
        if (iterative) quick_sort_iterative(a, n, type);
        else           quick_sort_recursive(a, 0, n - 1, type);
        total += now_ms() - t0;
        if (!is_sorted(a, n)) { fprintf(stderr, "정렬 오류\n"); exit(1); }
    }
    free(a);
    return total / repeat;
}

int main(int argc, char **argv) {
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const int num_sizes = (int)(sizeof(sizes) / sizeof(sizes[0]));
    const char *names[] = {"First", "Random", "Median-3"};
    const PivotType types[] = {PIVOT_FIRST, PIVOT_RANDOM, PIVOT_MEDIAN3};

    srand(12345);                              /* 데이터 생성용 시드 고정 */

    /* 보조 실험: ./quick_sort_pivot avg  -> 서로 다른 데이터 200세트의 평균 비교 횟수 */
    if (argc > 1 && strcmp(argv[1], "avg") == 0) {
        const int trials = 200;
        printf("Average over %d random data sets\n\n", trials);
        for (int s = 0; s < num_sizes; s++) {
            int n = sizes[s];
            int *data = malloc(sizeof(int) * n);
            double sum[3] = {0, 0, 0};
            for (int t = 0; t < trials; t++) {
                for (int i = 0; i < n; i++) data[i] = rand() % (MAX_VALUE + 1);
                for (int k = 0; k < 3; k++)
                    sum[k] += (double)count_comparisons(data, n, types[k], 0);
            }
            printf("Data Size : %d\n", n);
            for (int k = 0; k < 3; k++) {
                char label[32];
                snprintf(label, sizeof(label), "Quick Sort (%s)", names[k]);
                printf("%-22s: %.0f comparisons (avg)\n", label, sum[k] / trials);
            }
            printf("\n");
            free(data);
        }
        return 0;
    }

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        int *data = malloc(sizeof(int) * n);
        for (int i = 0; i < n; i++) data[i] = rand() % (MAX_VALUE + 1);  /* 중복 허용 */

        printf("Data Size : %d\n\n", n);
        for (int k = 0; k < 3; k++) {
            char label[32];
            snprintf(label, sizeof(label), "Quick Sort (%s)", names[k]);
            long long c = count_comparisons(data, n, types[k], 0);
            printf("%-22s: %lld comparisons\n", label, c);
        }
        printf("\n");
        free(data);
    }

    /* ---- 보너스: 재귀 vs 반복 실행시간 ---- */
    {
        int n = 10000, repeat = 2000;
        int *data = malloc(sizeof(int) * n);
        for (int i = 0; i < n; i++) data[i] = rand() % (MAX_VALUE + 1);

        /* 두 구현의 비교 횟수가 같은지(= 같은 분할 순서인지) 확인 */
        if (count_comparisons(data, n, PIVOT_MEDIAN3, 0) !=
            count_comparisons(data, n, PIVOT_MEDIAN3, 1)) {
            fprintf(stderr, "재귀/반복 비교 횟수가 다름!\n");
            return 1;
        }

        /* 워밍업 후, 순서를 바꿔 가며 측정하여 순서 효과를 줄인다. */
        measure_time(data, n, PIVOT_MEDIAN3, 0, 50);
        measure_time(data, n, PIVOT_MEDIAN3, 1, 50);
        double t_rec = 0, t_itr = 0;
        for (int round = 0; round < 4; round++) {
            t_rec += measure_time(data, n, PIVOT_MEDIAN3, 0, repeat / 4);
            t_itr += measure_time(data, n, PIVOT_MEDIAN3, 1, repeat / 4);
        }
        t_rec /= 4; t_itr /= 4;

        printf("Data Size : %d\n", n);
        printf("Pivot     : Median-of-Three\n\n");
        printf("Recursive Quick Sort : %.4f ms\n", t_rec);
        printf("Iterative Quick Sort : %.4f ms\n", t_itr);
        printf("(평균 %d회 측정)\n", repeat);
        free(data);
    }
    return 0;
}
